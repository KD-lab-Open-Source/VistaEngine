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
//     This class answers to exactly those names, so the callers' sprite, wave and
//     billboard loops are shared, untouched code on both backends.
//   * The material. SetWorldMaterial selects vsStandart / psStandart; SetNoMaterial
//     drops to the fixed-function pipeline, whose stage 0 is
//     COLOROP/ALPHAOP = MODULATE(TEXTURE, DIFFUSE) -- which is exactly what psStandart
//     computes in the plain configuration every caller here uses. So one shader serves
//     both routes, and SetMaterial below is what is left of either call: the blend mode,
//     the texture, and whether to depth-test.
//
// Callers, each reached through the engine's own scene-draw delegation:
//
//   cSunMoonObj::Draw            (cSkyCamera::DrawScene -- the sky, before everything else)
//   cCoastSprites::Draw          (SCENENODE_OBJECTSPECIAL, sortIndex 0 -- after the water)
//   cFixedWavesContainer::Draw   (SCENENODE_OBJECTSORT -- the sorted transparent pass)
//   cWaves::Draw                 (SCENENODE_OBJECTSORT; nothing constructs a cWaves today)
//
// Each opens its pass through cSDLRenderDevice::drawWorldQuads when its own draw call
// returns, so the quads land where the scene walk reached them.
//
// Scope: one texture per group, two blend modes, never any depth write. Still to come,
// each a shader variant SetWorldMaterial can select but no caller here needs: the second
// texture and its four colour operations, TFACTOR, fog of war, fog, FLOAT_ZBUFFER and the
// z-reflection clip. The other users of the shared quad buffer (NParticle, Leaves,
// FogOfWar) want some of those, and will bring them.

#include "IRenderDevice.h"    // cTexture
#include "VertexFormat.h"     // sVertexXYZDT1 (the quad buffer's vertex)
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
	// SetNoMaterial call that opens each caller's draw. `blend` is ALPHA_BLEND or
	// ALPHA_ADDBLENDALPHA (the sun's); anything else falls back to ALPHA_BLEND. A null
	// texture draws untextured (a white 1x1), as those calls' own pWhiteTexture fallback
	// does. depthTest false is D3DRS_ZENABLE FALSE, which only the sun and moon ask for --
	// no caller ever writes depth.
	void SetMaterial(eBlendMode blend, cTexture* texture, bool depthTest = true);

	// cQuadBuffer<sVertexXYZDT1>'s contract: BeginDraw opens a group, each Get hands back
	// four vertices for one quad, EndDraw closes it. A group carries the material
	// SetMaterial last named.
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

	// One BeginDraw..EndDraw run: the quads it collected and the material they draw with.
	struct Group
	{
		SDL_GPUTexture* texture;   // null -> the 1x1 white stand-in
		eBlendMode blend;
		bool depthTest;
		int firstQuad, quadCount;
	};

	void createSampler();
	bool createShaders();
	// The blend mode and the depth test are baked into an SDL GPU pipeline, so there is one
	// per (blend, depthTest, wireframe). Built on demand and cached -- in practice two.
	SDL_GPUGraphicsPipeline* pipelineFor(eBlendMode blend, bool depthTest, bool wireframe);
	// Grow the GPU vertex/index buffers to hold `quads` quads, and refill the index buffer
	// with the two-triangle pattern. The index buffer's contents depend only on capacity.
	bool ensureCapacity(SDL_GPUCommandBuffer* cmd, int quads);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vsShader_ = nullptr;
	SDL_GPUShader* fsShader_ = nullptr;
	bool shadersTried_ = false;
	std::unordered_map<unsigned, SDL_GPUGraphicsPipeline*> pipelines_;
	// cCoastSprites::Draw's SetSamplerDataVirtual(0, sampler_wrap_anisotropic); the wave
	// sources and the sun inherit the scene's sampler_wrap_linear, which this rounds up to.
	SDL_GPUSampler* sampler_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;   // bound for an untextured group

	// The quads accumulate on the CPU and upload once, at Draw. Grown, never shrunk:
	// the quad count is stable across frames (cCoastSprites soft-clamps it near 4000).
	SDL_GPUBuffer* vertexBuffer_ = nullptr;
	SDL_GPUBuffer* indexBuffer_  = nullptr;
	int            capacityQuads_ = 0;

	std::vector<sVertexXYZDT1> vertices_;   // 4 per quad
	std::vector<Group> groups_;
	Group current_ = {nullptr, ALPHA_BLEND, true, 0, 0};   // the open BeginDraw..EndDraw run
	Group material_ = {nullptr, ALPHA_BLEND, true, 0, 0};  // set by SetMaterial, taken by BeginDraw
	bool drawing_ = false;                  // inside BeginDraw..EndDraw, with a camera
	sVertexXYZDT1 scratch_[4];              // what Get() hands back when there is no camera

	VSUniform vs_ = {};
	int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float vpMinZ_ = 0.f, vpMaxZ_ = 1.f;
	bool cameraValid_ = false;   // SetCamera has run for the group being recorded
};

#endif // VISTA_SDL_WORLD_QUAD_RENDERER_H
