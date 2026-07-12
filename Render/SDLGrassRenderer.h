#ifndef VISTA_SDL_GRASS_RENDERER_H
#define VISTA_SDL_GRASS_RENDERER_H

// The grass half of the SDL GPU backend: draws GrassMap's blades with the grass pipeline
// (Render/SDLShaders/grass.{vert,frag}.hlsl, ported from the original Render/shader/Grass/
// Grass.{vsl,psl} -- the VSGrass / PSGrass / PSGrassShadow classes).
//
// Reached through the engine's own delegation chain -- cScene::Draw -> Camera::DrawScene ->
// GrassMap::Draw -> GrassMap::DrawGrass -- which calls this renderer directly, just as it
// called gb_RenderDevice3D's shader objects before: SetState() takes what it would have
// pushed into D3D shader constants, then it issues one DrawIndexedPrimitive per visible
// tile. Same immediate-mode contract, same order.
//
// It owns no geometry. The vertex and index buffers are GrassMap's own, created through
// cSDLRenderDevice::CreateVertexBuffer/CreateIndexBuffer and filled by GrassMap::CalcVertex
// -- which is why grass could be ported without the dynamic vertex/quad buffer family
// (Render/PORTING.md #15) that the field dome and the lens flare still wait on. The device
// stays their owner; this renderer asks it to resolve an sPtr wrapper to the SDL_GPUBuffer
// behind it.
//
// Unlike SDLObject3dxRenderer, which batches because objects are drawn from all over the
// scene walk, grass is drawn from exactly one place. So the draws recorded here are
// replayed immediately, in a pass of their own, by cSDLRenderDevice::drawGrass() -- landing
// where the walk reached them: over the terrain, under the objects. That is also where the
// original drew them (Camera::DrawScene, between DrawTilemapObject and DrawObject).
//
// A blade is four vertices that all share one world position -- the bush's root -- and are
// grown into a camera-facing quad entirely in the vertex shader. See grass.vert.hlsl.
//
// Scope: the lit path with the shadow map and the terrain lightmap, which is every
// configuration VSGrass::RestoreShader ever built. Not ported, and not needed by any caller:
// the ZBUFFER variant (it fed the float z-buffer camera, PORTING.md #12) and FOG_OF_WAR
// (PORTING.md #10). Grass does not cast shadows -- it did not on D3D either, since
// CameraShadowMap::DrawScene never reaches GrassMap.

#include "IRenderDevice.h"    // Color4f, cTexture, sPtrVertexBuffer, sPtrIndexBuffer
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

class SDLGrassRenderer
{
public:
	// owner is the device that holds the vertex/index buffers this renderer draws from;
	// window is needed only to query the swapchain format. Destroy before the SDL GPU device.
	SDLGrassRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window);
	~SDLGrassRenderer();

	SDLGrassRenderer(const SDLGrassRenderer&) = delete;
	SDLGrassRenderer& operator=(const SDLGrassRenderer&) = delete;

	// Everything GrassMap::DrawGrass would have pushed into VSGrass::Select / PSGrass, in
	// the order that function sets it.
	struct State
	{
		cTexture* texture = nullptr;    // the grass atlas (GrassMap::GetTexture)
		float time = 0.f;               // GrassMap::time_ -- wind phase and growth clock
		float hideDistance = 0.f;       // GrassMap::invHideDistance2_ (1/hideDistance^2)
		// cTileMap::GetDiffuse(): rgb = the sun's diffuse colour, a = its ambient. The same
		// source the terrain lights from, so grass and ground agree.
		Color4f sunDiffuse = Color4f(1.f, 1.f, 1.f, 0.f);
		// GrassMap::oldLighting, serialized per world: a static shader define in the
		// original (VSGrass::SetOldLighting), a uniform here.
		bool oldLighting = true;
		// camera->IsShadow() && GrassMap::enbaleShadow_ -- the test on which the original
		// picks psGrassShadow over psGrass.
		bool receiveShadow = false;
	};

	// Drop the previous frame's draws. Called from BeginScene.
	void BeginFrame();

	// Snapshot the state and the camera the draws that follow are made under. The camera
	// supplies the view-projection, the billboard basis, the eye position and the sun
	// direction -- read now, not at flush, because several cameras walk the scene.
	void SetState(const State& state, Camera* camera);

	// Record one indexed draw against the current state, standing in for the D3D call of the
	// same name that GrassMap::DrawGrass issued per visible tile. The buffers are the
	// engine's; the device resolves them. Ignored until SetState has run.
	void DrawIndexedPrimitive(sPtrVertexBuffer& vb, const sPtrIndexBuffer& ib, int nPolygon);

	bool hasDraws() const { return !draws_.empty(); }
	// Throw the pending draws away: the target they were recorded under cannot be rendered
	// into, and they must not replay into the next one.
	void DiscardDraws() { draws_.clear(); }

	// Replay the pending draws into one colour+depth pass, and clear them. `clear`/
	// `clearDepth` mean this pass owns the target's colour/depth clear -- normally false,
	// since the terrain drew first and took both. Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int targetW, int targetH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	// The whole of grass.vert.hlsl's cbuffer.
	struct VSUniform
	{
		float mvp[16];          // camera->matViewProj
		float billboard[16];    // transpose(Mat4f(camera->GetMatrix())) -- see grass.vert.hlsl
		float shadow[16];       // shadowMatViewProj() * shadowMatBias()
		float planarNode[4];    // the lightmap's world box
		float cameraPos[4];
		float lightDir[4];
		float sunDiffuse[4];
		float params[4];        // x = time, y = hideDistance, z = oldLighting
	};
	// The whole of grass.frag.hlsl's cbuffer.
	struct FSUniform
	{
		float shade[4];           // cScene::GetShadowIntensity()
		float shadowParams[4];    // x = receives shadow, y = 2x2 filter
		float lightMapParams[4];  // x = the lightmap holds this frame's lights
		float params[4];          // x = the alpha-test reference (D3DRS_ALPHAREF/255)
	};

	// One SetState's worth: every draw recorded under it replays with these.
	struct StateBlock
	{
		VSUniform vs;
		FSUniform fs;
		SDL_GPUTexture* texture;         // the atlas; null -> the 1x1 white stand-in
		SDL_GPUTexture* shadowTexture;   // null -> nothing to receive from
		SDL_GPUTexture* lightMapTexture; // null -> LightMapParams gates the read off
		// The camera's viewport, captured at SetState: the draws replay in a pass opened
		// after GrassMap::DrawGrass returns, by which time the walk has moved on.
		int vpX, vpY, vpW, vpH;
		float vpMinZ, vpMaxZ;
		// The reflection camera's mirror matrix reverses every triangle's winding. Grass is
		// two-sided (D3DCULL_NONE), so this changes nothing -- but the pipeline key keeps it
		// honest if that ever stops being true.
		bool mirrored;
	};

	struct DrawCmd
	{
		int state;
		SDL_GPUBuffer* vertexBuffer;
		SDL_GPUBuffer* indexBuffer;
		int stride;
		int indexCount;
	};

	bool createShaders();
	void createSamplers();
	// Blend, depth and cull are fixed for grass -- the original sets them once, around the
	// whole draw -- so the only pipeline variants are the wireframe diagnostic and the
	// vertex stride, which is always shortVertexGrass's 28. Built on demand.
	SDL_GPUGraphicsPipeline* pipelineFor(int stride, bool wireframe);
	// Append the current state to states_ if it changed since the last recorded draw.
	int commitState();

	cSDLRenderDevice* owner_ = nullptr;
	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vs_ = nullptr;
	SDL_GPUShader* fs_ = nullptr;
	bool shadersTried_ = false;
	std::vector<std::pair<unsigned, SDL_GPUGraphicsPipeline*>> pipelines_;

	// sampler_clamp_anisotropic, which GrassMap::DrawGrass asks for on stage 0: the blades
	// live in one atlas, so wrapping would bleed a neighbouring frame in at the edges.
	SDL_GPUSampler* samplerClamp_ = nullptr;
	// sampler_clamp_point on the shadow stage: raw depth, compared by hand, must not filter.
	SDL_GPUSampler* samplerShadow_ = nullptr;
	// sampler_clamp_linear on the lightmap stage, as the original sets for stage 3.
	SDL_GPUSampler* samplerLightMap_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;

	std::vector<StateBlock> states_;
	std::vector<DrawCmd>    draws_;

	StateBlock current_ = {};
	bool currentValid_ = false;   // SetState has run
	bool currentDirty_ = true;    // current_ differs from states_.back()
};

#endif // VISTA_SDL_GRASS_RENDERER_H
