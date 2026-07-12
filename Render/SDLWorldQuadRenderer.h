#ifndef VISTA_SDL_WORLD_QUAD_RENDERER_H
#define VISTA_SDL_WORLD_QUAD_RENDERER_H

// The world-quad half of the SDL GPU backend: draws textured, alpha-blended quads in
// world space with the worldquad pipeline (Render/SDLShaders/worldquad.{vert,frag}.hlsl,
// ported from the original Render/shader/NoMaterial/standart.{vsl,psl}).
//
// It is the SDL stand-in for a set of facilities the original keeps on the device and
// shares between callers, which is why it is named for its role rather than for one
// engine class:
//
//   * cQuadBuffer<sVertexXYZDT1>, the shared dynamic quad buffer. Its contract is
//     BeginDraw / Get / EndDraw, where each Get hands back four vertices for one quad.
//     This class answers to exactly those names, so the callers' sprite, wave and
//     billboard loops are shared, untouched code on both backends.
//   * cVertexBuffer<sVertexXYZDT2>, the shared dynamic vertex buffer, for the callers that
//     draw triangle strips and lists rather than quads and want a second texture. Its
//     contract is Lock / Unlock / DrawPrimitive, and this class answers to those names
//     too, for the same reason. Strips and lists are turned into indexed triangles here.
//   * The material. SetWorldMaterial selects vsStandart / psStandart; SetNoMaterial
//     drops to the fixed-function pipeline, whose stage 0 is
//     COLOROP/ALPHAOP = MODULATE(TEXTURE, DIFFUSE) -- which is exactly what psStandart
//     computes in the plain configuration most callers here use. So one shader pair serves
//     both routes, and SetMaterial below is what is left of either call: the blend mode,
//     the textures and colour operation, the world matrix, and whether to depth-test.
//
// Callers, each reached through the engine's own scene-draw delegation:
//
//   cSunMoonObj::Draw            (cSkyCamera::DrawScene -- the sky, before everything else)
//   cCoastSprites::Draw          (SCENENODE_OBJECTSPECIAL, sortIndex 0 -- after the water)
//   cFixedWavesContainer::Draw   (SCENENODE_OBJECTSORT -- the sorted transparent pass)
//   cWaves::Draw                 (SCENENODE_OBJECTSORT; nothing constructs a cWaves today)
//   cEmitterInt/Spline/Z::Draw   (the particle sprite emitters, via cEffect::Draw)
//   cEmitterColumnLight::Draw    (the light columns and laser beams -- the triangle route)
//
// Each opens its pass through cSDLRenderDevice::drawWorldQuads when its own draw call
// returns, so the geometry lands where the scene walk reached it. Quads and triangles
// share ONE group list, so they replay in the order the callers made them -- which is what
// keeps a light column and the sprites of the same cEffect blending in the right order.
//
// Scope: two textures per group with the four colour operations, the six blend modes,
// never any depth write. Still to come, each a shader variant SetWorldMaterial can select
// but no caller here needs: TFACTOR, fog of war, fog, FLOAT_ZBUFFER and the z-reflection
// clip (the last is dead in the original too -- Camera::SetZTexture has no caller).

#include "IRenderDevice.h"    // cTexture, MatXf
#include "VertexFormat.h"     // sVertexXYZDT1 (the quad buffer's vertex)
#include "XMath/Mat4f.h"      // Mat4f: the camera's view-projection, and each group's mvp
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

class SDLWorldQuadRenderer
{
public:
	// Builds the sampler and the white stand-in; pipelines are built on demand. window is
	// needed only to query the swapchain format. Must be destroyed before the SDL GPU device.
	SDLWorldQuadRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLWorldQuadRenderer();

	SDLWorldQuadRenderer(const SDLWorldQuadRenderer&) = delete;
	SDLWorldQuadRenderer& operator=(const SDLWorldQuadRenderer&) = delete;

	// Drop the previous frame's quads. Called from BeginScene.
	void BeginFrame();

	// Snapshot the camera's view-projection and viewport, before the groups that follow.
	// Quads are replayed in a pass opened once the caller's Draw returns, by which time
	// the scene walk has moved on.
	void SetCamera(Camera* camera);

	// The material the following group draws with, standing in for the SetWorldMaterial /
	// SetNoMaterial call that opens each caller's draw. A null texture draws untextured (a
	// white 1x1), as those calls' own pWhiteTexture fallback does. depthTest false is
	// D3DRS_ZENABLE FALSE, which only the sun and moon ask for -- no caller ever writes
	// depth. `world` is SetWorldMaterial's matrix: MatXf::ID for every caller whose geometry
	// is already in world space, and the emitter's GlobalMatrix for a particle emitter
	// marked `relative`, whose geometry is built in emitter space.
	//
	// `texture1` and `colorMode` are the second texture and its colour operation, which
	// only the triangle route uses (the quad shader has no second sampler). A null texture1
	// leaves the operation off, exactly as cD3DRender::SetWorldMaterial does.
	void SetMaterial(eBlendMode blend, cTexture* texture, bool depthTest = true,
	                 const MatXf& world = MatXf::ID,
	                 cTexture* texture1 = nullptr, eColorMode colorMode = COLOR_MOD);

	// cQuadBuffer<sVertexXYZDT1>'s contract: BeginDraw opens a group, each Get hands back
	// four vertices for one quad, EndDraw closes it. A group carries the material
	// SetMaterial last named.
	//
	// BeginDraw's matrix is accepted and ignored, as it effectively is on D3D: there it
	// reaches cD3DRender::setWorldMatrix, which sets the fixed-function D3DTS_WORLD, and
	// every caller here draws through vsStandart -- whose world matrix comes from
	// SetWorldMaterial instead (cD3DRender::SetWorldMaterial ends in vsStandart->Select(mat)).
	// So SetMaterial's `world` is the one that shades, on both backends.
	void BeginDraw(const MatXf& = MatXf::ID);
	sVertexXYZDT1* Get();
	void EndDraw();

	// cVertexBuffer<sVertexXYZDT2>'s contract, for the triangle route: Lock hands back
	// nVertex vertices to fill, Unlock closes them, and DrawPrimitive turns them into one
	// group. Only the two primitive types the callers use are honoured -- PT_TRIANGLESTRIP
	// and PT_TRIANGLELIST -- and both become indexed triangles here, since SDL GPU has no
	// strip primitive worth keeping the seams for. `nPolygon` is the triangle count, as it
	// is on D3D.
	sVertexXYZDT2* Lock(int nVertex);
	void Unlock(int nVertex);
	void DrawPrimitive(PRIMITIVETYPE type, int nPolygon);

	bool hasDraws() const { return !groups_.empty(); }

	// Replay the quads recorded so far into one colour+depth render pass, blended over the
	// scene and writing no depth (ALPHA_BLEND with RS_ZWRITEENABLE off, as every caller's
	// scene node sets). `clear`/`clearDepth` mean this pass owns the frame's colour/depth
	// clear -- true only when no earlier pass took it. Clears the recorded quads, so a
	// later caller in the same frame replays only its own. Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int screenW, int screenH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	// worldquad.vert.hlsl / worldtri.vert.hlsl's whole cbuffer: the original's mWVP.
	struct VSUniform { float mvp[16]; };
	// worldtri.frag.hlsl's: .x is COLOR_OPERATION. The quad shader has no uniform.
	struct FSUniform { float colorOp[4]; };

	// Which stream a group's geometry lives in, and so which shader, vertex layout and
	// buffers replay it. They share one group list, so the two interleave in call order.
	enum GroupKind { GROUP_QUAD, GROUP_TRI };

	// One BeginDraw..EndDraw run, or one DrawPrimitive: the geometry it collected and the
	// material it draws with.
	struct Group
	{
		GroupKind kind;
		SDL_GPUTexture* texture;    // null -> the 1x1 white stand-in
		SDL_GPUTexture* texture1;   // the colour operation's second texture (GROUP_TRI only)
		eBlendMode blend;
		bool depthTest;
		// GROUP_QUAD: quads into vertices_/the shared quad index pattern.
		// GROUP_TRI:  triangles into indicesTri_ (which indexes verticesTri_).
		int first, count;
		// SetWorldMaterial's mWVP for this group: world * the camera's view-projection.
		// Per group, not per frame -- a relative particle emitter carries its own world
		// matrix, and several emitters with different ones draw under one camera.
		VSUniform vs;
		FSUniform fs;
	};

	void createSampler();
	bool createShaders();
	// Start a group in the named stream, taking the material SetMaterial last named and
	// folding its world matrix into the mvp.
	void openGroup(GroupKind kind);
	// The blend mode, the depth test and the vertex layout are baked into an SDL GPU
	// pipeline, so there is one per (blend, depthTest, wireframe, kind). Built on demand.
	SDL_GPUGraphicsPipeline* pipelineFor(eBlendMode blend, bool depthTest, bool wireframe,
	                                     GroupKind kind);
	// Grow the GPU quad buffers to hold `quads` quads, and refill the index buffer with the
	// two-triangle pattern. The index buffer's contents depend only on capacity.
	bool ensureCapacity(SDL_GPUCommandBuffer* cmd, int quads);
	// Grow the GPU triangle buffers. Unlike the quads', this index buffer's contents are
	// built per frame (the callers hand over strips and lists), so it is uploaded at Draw.
	bool ensureCapacityTri(SDL_GPUCommandBuffer* cmd, int vertices, int indices);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vsShader_ = nullptr;
	SDL_GPUShader* fsShader_ = nullptr;
	SDL_GPUShader* vsShaderTri_ = nullptr;
	SDL_GPUShader* fsShaderTri_ = nullptr;
	bool shadersTried_ = false;
	std::unordered_map<unsigned, SDL_GPUGraphicsPipeline*> pipelines_;
	// cCoastSprites::Draw's SetSamplerDataVirtual(0, sampler_wrap_anisotropic); the wave
	// sources and the sun inherit the scene's sampler_wrap_linear, which this rounds up to.
	// One sampler for both stages: cEmitterColumnLight is the only caller that asks for two
	// (clamp on stage 0, wrap on stage 1), and its stage-0 uv stays inside [0,1], where the
	// two agree -- except in the branch that has no first texture, which moves the scrolling
	// one onto stage 0 and asks for wrap there anyway.
	SDL_GPUSampler* sampler_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;   // bound for an untextured group

	// The quads accumulate on the CPU and upload once, at Draw. Grown, never shrunk:
	// the quad count is stable across frames (cCoastSprites soft-clamps it near 4000).
	SDL_GPUBuffer* vertexBuffer_ = nullptr;
	SDL_GPUBuffer* indexBuffer_  = nullptr;
	int            capacityQuads_ = 0;

	// The triangle route's, likewise. Its indices are per frame, not a fixed pattern.
	SDL_GPUBuffer* vertexBufferTri_ = nullptr;
	SDL_GPUBuffer* indexBufferTri_  = nullptr;
	int            capacityVertsTri_ = 0;
	int            capacityIndicesTri_ = 0;

	std::vector<sVertexXYZDT1> vertices_;   // 4 per quad
	std::vector<Group> groups_;
	Group current_ = {};    // the open BeginDraw..EndDraw run
	Group material_ = {};   // set by SetMaterial, taken by BeginDraw
	MatXf materialWorld_ = MatXf::ID;       // SetMaterial's world, folded into the mvp at BeginDraw
	bool drawing_ = false;                  // inside BeginDraw..EndDraw, with a camera
	sVertexXYZDT1 scratch_[4];              // what Get() hands back when there is no camera

	// The triangle route. Lock appends to verticesTri_ and remembers where; DrawPrimitive
	// turns that run into indices. lockFirst_ is the run's first vertex, lockCount_ its
	// length -- both cleared by DrawPrimitive, so a Lock without one records nothing.
	std::vector<sVertexXYZDT2> verticesTri_;
	std::vector<unsigned> indicesTri_;
	int lockFirst_ = 0, lockCount_ = 0;
	std::vector<sVertexXYZDT2> scratchTri_;   // what Lock hands back when there is no camera

	Mat4f viewProj_;             // the camera's, from SetCamera; each group's mvp is world * this
	int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float vpMinZ_ = 0.f, vpMaxZ_ = 1.f;
	bool cameraValid_ = false;   // SetCamera has run for the group being recorded
};

#endif // VISTA_SDL_WORLD_QUAD_RENDERER_H
