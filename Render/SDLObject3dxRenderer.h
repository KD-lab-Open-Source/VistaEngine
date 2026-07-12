#ifndef VISTA_SDL_OBJECT3DX_RENDERER_H
#define VISTA_SDL_OBJECT3DX_RENDERER_H

// The 3D-object half of the SDL GPU backend: draws skinned .3dx meshes (units,
// buildings, props, the menu's background models) with the object3dx pipeline
// (Render/SDLShaders/object3dx.{vert,frag}.hlsl, ported from the original
// Render/shader/Skin/object_scene_light.{vsl,psl}).
//
// Reached through the engine's own delegation chain -- cScene::Draw -> Camera::DrawScene
// -> Camera::DrawObject -> cObject3dx::Draw -- which calls this renderer directly, just
// as it calls gb_RenderDevice3D's shader objects on Windows: per material group it hands
// SetState() the state it would have pushed into D3D shader constants, then issues the
// group's draws through DrawIndexedPrimitive. Same immediate-mode contract, same order.
//
// Unlike SDLTileMapRenderer, which draws the instant cTileMap::Draw calls it, this
// renderer batches: SDL GPU can only draw inside a render pass, and objects are drawn
// from all over the scene walk. Each DrawIndexedPrimitive records geometry + a snapshot
// of the current state; Draw() replays them in a render pass at EndScene, in call
// order -- which is what keeps the engine's opaque-then-sorted-transparent ordering.
// The water surface and the coast sprites, drawn between the two in the scene walk,
// split that into two passes (see Draw below).
//
// It owns no geometry: the vertex/index buffers are the engine's own, created through
// cSDLRenderDevice::CreateVertexBuffer/CreateIndexBuffer and filled by cStatic3dx. The
// device stays their owner; this renderer asks it to resolve an sPtr wrapper to the
// SDL_GPUBuffer behind it (and for the current RS_ZWRITEENABLE).
//
// Scope: the plain lit path (vsSkin/psSkin), the bump path (vsSkinBump/psSkinBump, with
// its optional specular map) and their scene-shadow variants -- skinning, diffuse texture,
// lambert + specular, ambient, the skin-colour tint, the animated UV transform, casting
// into and receiving from the shadow map, and the five blend modes cObject3dx::Draw
// selects between. Still to come, each a shader variant cObject3dx::Draw picks in the
// original: reflection (planar and cube), the second opacity map, the lightmap, fog of
// war, fog, point lights, and fur.

#include "IRenderDevice.h"    // Color4f, eBlendMode, cTexture, MatXf
#include <unordered_map>
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUShader;
struct SDL_GPUBuffer;

class Camera;
class cSDLRenderDevice;

class SDLObject3dxRenderer
{
public:
	// Builds the shaders and sampler. owner is the device that holds the vertex/index
	// buffers this renderer draws from; window is needed only to query the swapchain
	// format the pipelines render to. Must be destroyed before the SDL GPU device.
	SDLObject3dxRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window);
	~SDLObject3dxRenderer();

	SDLObject3dxRenderer(const SDLObject3dxRenderer&) = delete;
	SDLObject3dxRenderer& operator=(const SDLObject3dxRenderer&) = delete;

	// One material group's worth of state, straight out of cObject3dx::Draw: the same
	// values it feeds to VSSkin::Select / VSSkin::SetMaterial / PSSkin::SetMaterial and
	// to SetBlendStateAlphaRef, plus the bone poses from GetWorldPoses.
	struct State
	{
		const MatXf* world = nullptr;   // GetWorldPoses output, copied by SetState
		int worldNum = 0;               // bunch.nodeIndices.size(), <= StaticBunch::max_index
		int boneCount = 1;              // lod.blend_indices: 1 = rigid, 2..4 = weighted

		// Zero-initialized on purpose: Color4f's default constructor leaves its channels
		// uninitialized, and a stray lerpColor.a tints the texture with garbage.
		Color4f ambient   = Color4f(0,0,0,0);   // material.Ambient
		Color4f diffuse   = Color4f(0,0,0,0);   // material.Diffuse (a = opacity)
		Color4f specular  = Color4f(0,0,0,0);   // material.Specular (a = specular power)
		Color4f lerpColor = Color4f(0,0,0,0);   // material.lerp_texture (the unit's skin colour)

		cTexture* texture = nullptr;    // material.Tex[0]
		// Non-null selects the bump path. Only set when the vertex actually carries a
		// tangent frame (cStatic3dx::bump) and the original would have picked vsSkinBump.
		cTexture* bumpTexture = nullptr;    // mat.pBumpTexture
		cTexture* specularMap = nullptr;    // mat.pSpecularmap (PSSkinBump::SelectSpecularMap)
		float texturePhase = 0.f;       // animation phase for a multi-frame texture
		bool tilingWrap = false;        // mat.tiling_diffuse & TILING_U_WRAP

		eBlendMode blend = ALPHA_NONE;
		bool noLight = false;           // mat.no_light || ATTRUNKOBJ_NOLIGHT
		bool selfIllumination = false;  // PSSkin::SetSelfIllumination

		bool hasUVTrans = false;        // mat.chains' animated UV transform
		float uvTrans[6] = {0,0,0,0,0,0};
	};

	// Drop the previous frame's draws. Called from BeginScene.
	void BeginFrame();

	// Snapshot the state the following draws are made with. camera supplies the
	// view-projection, eye position and sun direction, read now rather than at flush:
	// several cameras walk the scene and each sets state before its own draws.
	void SetState(const State& state, Camera* camera);

	// Override the diffuse colour mid-group, as the original's VSSkin/PSSkin::SetAlphaColor
	// does when a visibility group fades (cObject3dx::DrawMaterialGroupSelectively).
	void SetAlphaColor(const Color4f& color);

	// Record one indexed draw against the current state, standing in for the D3D call of
	// the same name (cObject3dx::DrawMaterialGroup issues one per visible group). The
	// buffers are the engine's; the device resolves them. Ignored until SetState has run.
	void DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex,
	                          const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon);

	bool hasDraws() const { return !draws_.empty(); }
	// Throw the pending draws away: the target they were recorded under cannot be rendered
	// into (see cSDLRenderDevice::flushTarget), and they must not replay into the next one.
	void DiscardDraws() { draws_.clear(); }

	// Replay the pending draws into a depth-only pass (no colour target) -- the shadow map,
	// which the light camera was walking when it recorded them. Reached from
	// cSDLRenderDevice::flushTarget, so the map is complete before anything samples it.
	// `clearDepth` means this pass owns the map's clear -- false once the terrain caster
	// pass has already taken it. Runs even with nothing recorded: the clear alone leaves an
	// empty map at far depth. Returns true if the pass ran.
	bool DrawShadowPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* depth, int size, bool clearDepth);

	// Replay the pending draws into one colour+depth render pass, and clear them.
	// `clear`/`clearDepth` mean this pass owns the target's colour/depth clear -- true only
	// when no earlier pass (the terrain) already took it. Returns true if the pass ran.
	//
	// Called from cSDLRenderDevice::flushTarget: at EndScene, mid-scene from
	// flushObjectPass (which has to get the opaque objects onto the screen before the water
	// and the coast sprites blend over them), and whenever setCamera switches targets. The
	// draws that follow a flush replay in the next one, on top, as on D3D.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int targetW, int targetH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	// The vertex uniform block's fixed head, laid out to match object3dx.vert.hlsl up to
	// (but not including) its World[] array: MVP, eight float4s, then the shadow matrix.
	struct VSHead
	{
		float mvp[16];
		float ambient[4], diffuse[4], specular[4];
		float cameraPos[4], lightDir[4];
		float uTrans[4], vTrans[4];
		float params[4];            // x = boneCount, y = noLight
		float shadow[16];           // shadowMatViewProj() * shadowMatBias()
	};
	// The whole of object3dx.frag.hlsl's cbuffer.
	struct FSUniform
	{
		float ambient[4];
		float diffuse[4];           // bumpDiffuse (bump path only)
		float specular[4];          // bumpSpecular: rgb + power in w (bump path only)
		float lerpPre[4];
		float params[4];            // x = alphaRef, y = textured, z = selfIllum, w = lerp
		float params2[4];           // x = specular map present
		float shade[4];             // vShade: a fully shadowed pixel's multiplier
		float shadowParams[4];      // x = this material receives shadows
	};

	// A state snapshot shared by every draw recorded under it.
	struct StateBlock
	{
		VSHead vs;
		FSUniform fs;
		int worldOffset;            // into worldPool_, in floats
		int worldRows;              // 3 per bone; worldRows*4 floats
		SDL_GPUTexture* texture;    // null -> the 1x1 white stand-in
		SDL_GPUTexture* bumpTexture;
		SDL_GPUTexture* specularTexture;
		SDL_GPUTexture* shadowTexture;   // null -> this material does not receive
		SDL_GPUSampler* sampler;
		eBlendMode blend;
		bool skinned;               // vertex carries weight bytes (boneCount > 1)
		bool bump;                  // bump path: tangent-frame vertex, per-pixel lambert
		// The camera's viewport, captured at SetState. Draws are replayed in one pass at
		// EndScene, long after the scene walk moved on, so it cannot be read back then.
		int vpX, vpY, vpW, vpH;
		float vpMinZ, vpMaxZ;
	};

	struct DrawCmd
	{
		int state;
		SDL_GPUBuffer* vertexBuffer;
		SDL_GPUBuffer* indexBuffer;
		int stride;
		int firstIndex, indexCount;   // indices are absolute: no base-vertex offset
		bool depthWrite;
	};

	bool createShaders();
	// Pipelines vary with the vertex stride (the .3dx vertex grows with bump/uv2/fur),
	// whether it carries weights, whether it takes the bump path, the blend mode and
	// depth write -- all baked into an SDL GPU pipeline. Built on demand and cached.
	// `shadow` selects the caster pipeline: depth-only (no colour target), slope-scaled
	// depth bias, and the shadow shaders, which ignore the tangent frame.
	SDL_GPUGraphicsPipeline* pipelineFor(int stride, bool skinned, bool bump, eBlendMode blend,
	                                     bool depthWrite, bool wireframe, bool shadow);
	// Append the current state to states_ if it changed since the last recorded draw.
	int commitState();

	cSDLRenderDevice* owner_  = nullptr;   // owns the vertex/index buffers we draw
	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vsRigid_       = nullptr;   // -DSKINNED=0 -DBUMP=0
	SDL_GPUShader* vsSkin_        = nullptr;   // -DSKINNED=1 -DBUMP=0
	SDL_GPUShader* vsRigidBump_   = nullptr;   // -DSKINNED=0 -DBUMP=1
	SDL_GPUShader* vsSkinBump_    = nullptr;   // -DSKINNED=1 -DBUMP=1
	SDL_GPUShader* fs_            = nullptr;   // -DBUMP=0
	SDL_GPUShader* fsBump_        = nullptr;   // -DBUMP=1
	SDL_GPUShader* vsShadowRigid_ = nullptr;   // object3dx_shadow, -DSKINNED=0
	SDL_GPUShader* vsShadowSkin_  = nullptr;   // object3dx_shadow, -DSKINNED=1
	SDL_GPUShader* fsShadow_      = nullptr;   // alpha-cutout clip, no colour output
	bool shadersTried_ = false;

	std::unordered_map<unsigned long long, SDL_GPUGraphicsPipeline*> pipelines_;
	// The original picks between sampler_wrap_anisotropic and sampler_clamp_anisotropic
	// per material, from StaticMaterial::tiling_diffuse.
	SDL_GPUSampler* samplerWrap_  = nullptr;
	SDL_GPUSampler* samplerClamp_ = nullptr;
	// Point + clamp, as the original's sampler_clamp_point on the shadow stage: the depth
	// compare is done by hand on raw values, which must not be filtered.
	SDL_GPUSampler* samplerShadow_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;   // bound when a material has no texture

	std::vector<StateBlock> states_;
	// The draws recorded since the last flush, all of them bound for whatever target the
	// device has bound now. Draws made under the light camera are no different: their
	// StateBlock simply carries the light's MVP and viewport, because SetState reads
	// whichever camera it is handed, and cSDLRenderDevice::flushTarget replays them into
	// the shadow map when the scene walk leaves that camera.
	std::vector<DrawCmd>    draws_;
	// Bone rows for every state this frame. Survives a flush, like states_: the draws still
	// to come index into both.
	std::vector<float>      worldPool_;

	StateBlock current_;         // set by SetState, committed lazily by AddDraw
	bool currentValid_ = false;  // SetState has run this frame
	bool currentDirty_ = true;   // current_ differs from states_.back()
};

#endif // VISTA_SDL_OBJECT3DX_RENDERER_H
