#ifndef VISTA_SDL_WORLD_QUAD_RENDERER_H
#define VISTA_SDL_WORLD_QUAD_RENDERER_H

// The world-quad half of the SDL GPU backend: draws textured, alpha-blended quads in
// world space with the worldquad pipeline (Render/SDLShaders/worldquad.{vert,frag}.hlsl,
// ported from the original Render/shader/NoMaterial/standart.{vsl,psl}).
//
// It is the SDL stand-in for a pair of facilities the original keeps on the device and
// shares between callers, which is why it is named for its role rather than for one
// engine class:
//
//   * cQuadBuffer<sVertexXYZDT1>, the shared dynamic quad buffer. Its contract is
//     BeginDraw / Get / EndDraw, where each Get hands back four vertices for one quad.
//     This class answers to exactly those names, so the callers' sprite and wave loops
//     are shared, untouched code on both backends.
//   * vsStandart / psStandart, the shader pair cD3DRender::SetWorldMaterial selects.
//     SetTexture below is what is left of that call: everything else it configures is
//     baked into the pipeline.
//
// Callers, each reached through the engine's own scene-draw delegation:
//
//   cCoastSprites::Draw          (SCENENODE_OBJECTSPECIAL, sortIndex 0 -- after the water)
//   cFixedWavesContainer::Draw   (SCENENODE_OBJECTSORT -- the sorted transparent pass)
//   cWaves::Draw                 (SCENENODE_OBJECTSORT; nothing constructs a cWaves today)
//
// Each opens its pass through cSDLRenderDevice::drawWorldQuads when its own draw call
// returns, so the quads land where the scene walk reached them.
//
// Scope: one texture per group, alpha-blended, depth-tested against the scene, no depth
// write. Still to come, each a shader variant SetWorldMaterial can select but no caller
// here needs: the second texture and its four colour operations, TFACTOR, fog of war,
// fog, FLOAT_ZBUFFER and the z-reflection clip. The other users of the shared quad buffer
// (NParticle, Leaves, FogOfWar) want some of those, and will bring them.

#include "IRenderDevice.h"    // cTexture
#include "VertexFormat.h"     // sVertexXYZDT1 (the quad buffer's vertex)
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;

class Camera;

class SDLWorldQuadRenderer
{
public:
	// Builds the worldquad pipelines and sampler. window is needed only to query the
	// swapchain format. Must be destroyed before the SDL GPU device it was built on.
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

	// The texture the following group draws with, standing in for the
	// SetWorldMaterial(ALPHA_BLEND, MatXf::ID, 0, Texture) that opens each caller's draw.
	// Null draws untextured (a white 1x1), as SetWorldMaterial's own pWhiteTexture
	// fallback does.
	void SetTexture(cTexture* texture);

	// cQuadBuffer<sVertexXYZDT1>'s contract: BeginDraw opens a group, each Get hands back
	// four vertices for one quad, EndDraw closes it. A group carries the texture
	// SetTexture last named.
	void BeginDraw();
	sVertexXYZDT1* Get();
	void EndDraw();

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
	// worldquad.vert.hlsl's whole cbuffer: the original's mWVP (mWorld is identity).
	struct VSUniform { float mvp[16]; };

	// One BeginDraw..EndDraw run: the quads it collected and the texture they sample.
	struct Group
	{
		SDL_GPUTexture* texture;   // null -> the 1x1 white stand-in
		int firstQuad, quadCount;
	};

	void createPipelines();
	// Grow the GPU vertex/index buffers to hold `quads` quads, and refill the index buffer
	// with the two-triangle pattern. The index buffer's contents depend only on capacity.
	bool ensureCapacity(SDL_GPUCommandBuffer* cmd, int quads);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	// Same shaders, differing only in fill mode. FILL/LINE mirrors RS_FILLMODE.
	SDL_GPUGraphicsPipeline* pipelineFill_ = nullptr;
	SDL_GPUGraphicsPipeline* pipelineLine_ = nullptr;
	// cCoastSprites::Draw's SetSamplerDataVirtual(0, sampler_wrap_anisotropic); the wave
	// sources inherit the scene's sampler_wrap_linear, which this rounds up to.
	SDL_GPUSampler* sampler_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;   // bound for an untextured group

	// The quads accumulate on the CPU and upload once, at Draw. Grown, never shrunk:
	// the quad count is stable across frames (cCoastSprites soft-clamps it near 4000).
	SDL_GPUBuffer* vertexBuffer_ = nullptr;
	SDL_GPUBuffer* indexBuffer_  = nullptr;
	int            capacityQuads_ = 0;

	std::vector<sVertexXYZDT1> vertices_;   // 4 per quad
	std::vector<Group> groups_;
	Group current_ = {nullptr, 0, 0};       // the open BeginDraw..EndDraw run
	SDL_GPUTexture* texture_ = nullptr;     // set by SetTexture, taken by BeginDraw
	bool drawing_ = false;                  // inside BeginDraw..EndDraw, with a camera
	sVertexXYZDT1 scratch_[4];              // what Get() hands back when there is no camera

	VSUniform vs_ = {};
	int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float vpMinZ_ = 0.f, vpMaxZ_ = 1.f;
	bool cameraValid_ = false;   // SetCamera has run for the group being recorded
};

#endif // VISTA_SDL_WORLD_QUAD_RENDERER_H
