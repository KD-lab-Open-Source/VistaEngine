#ifndef VISTA_SDL_COAST_SPRITES_RENDERER_H
#define VISTA_SDL_COAST_SPRITES_RENDERER_H

// The coast-sprite (shoreline foam) half of the SDL GPU backend: draws cCoastSprites'
// bubble quads with the coastsprites pipeline (Render/SDLShaders/coastsprites.
// {vert,frag}.hlsl, ported from the original Render/shader/NoMaterial/standart.{vsl,psl}
// -- the vsStandart/psStandart pair cD3DRender::SetWorldMaterial selects).
//
// Reached through the engine's own delegation chain -- cScene::Draw -> Camera::DrawScene
// -> Camera::DrawObjectSpecial -> cCoastSprites::Draw. DrawObjectSpecial sorts that node
// by sortIndex(), so the water (-2) draws before the coast sprites (0), as on D3D.
//
// It stands in for the device's shared dynamic quad buffer: on Windows
// cCoastSprites::DrawSimple/MovingCoastSprite fill four sVertexXYZDT1 per sprite through
// cQuadBuffer<sVertexXYZDT1> (BeginDraw / Get / EndDraw), and off-Windows they fill the
// very same vertices through the identically named methods here. The sprite loops --
// phase advance, retirement, the triangle-wave alpha, the animation-frame UVs -- are
// shared, untouched code; only the vertex sink and the material differ.
//
// Scope: the two sprite groups (stay and moving), each one texture, alpha-blended,
// depth-tested against the scene, no depth write. Still to come, each a shader variant
// SetWorldMaterial can select but the coast sprites do not need: the second texture and
// its four colour operations, TFACTOR, fog of war, fog, FLOAT_ZBUFFER and the
// z-reflection clip.

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

class SDLCoastSpritesRenderer
{
public:
	// Builds the coastsprites pipelines and sampler. window is needed only to query the
	// swapchain format. Must be destroyed before the SDL GPU device it was built on.
	SDLCoastSpritesRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLCoastSpritesRenderer();

	SDLCoastSpritesRenderer(const SDLCoastSpritesRenderer&) = delete;
	SDLCoastSpritesRenderer& operator=(const SDLCoastSpritesRenderer&) = delete;

	// Drop the previous frame's quads. Called from BeginScene.
	void BeginFrame();

	// Snapshot the camera's view-projection and viewport, once per frame, before the
	// sprite groups run. Quads are replayed in a pass opened at the end of
	// cCoastSprites::Draw, by which time the scene walk has moved on.
	void SetCamera(Camera* camera);

	// The atlas the following group draws with, standing in for the
	// SetWorldMaterial(ALPHA_BLEND, ..., Texture, 0, COLOR_MOD, true) that opens each of
	// cCoastSprites' two draw functions. Null draws untextured (a white 1x1), as
	// SetWorldMaterial's own pWhiteTexture fallback does.
	void SetTexture(cTexture* texture);

	// cQuadBuffer<sVertexXYZDT1>'s contract, which the sprite loops are written against:
	// BeginDraw opens a group, each Get hands back four vertices for one quad, EndDraw
	// closes it. A group carries the texture SetTexture last named.
	void BeginDraw();
	sVertexXYZDT1* Get();
	void EndDraw();

	bool hasDraws() const { return !groups_.empty(); }

	// Replay the frame's quads into one colour+depth render pass, blended over the scene
	// and writing no depth (cCoastSprites::Draw's ALPHA_BLEND with RS_ZWRITEENABLE off).
	// `clear`/`clearDepth` mean this pass owns the frame's colour/depth clear -- true only
	// when no earlier pass took it. Clears the recorded quads. Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int screenW, int screenH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	// coastsprites.vert.hlsl's whole cbuffer: the original's mWVP (mWorld is identity).
	struct VSUniform { float mvp[16]; };

	// One BeginDraw..EndDraw run: the quads it collected and the atlas they sample.
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
	// cCoastSprites::Draw's SetSamplerDataVirtual(0, sampler_wrap_anisotropic).
	SDL_GPUSampler* sampler_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;   // bound for an untextured group

	// The quads accumulate on the CPU and upload once, at Draw. Grown, never shrunk:
	// the sprite count is stable across frames (cCoastSprites soft-clamps it near 4000).
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
	bool cameraValid_ = false;   // SetCamera has run this frame
};

#endif // VISTA_SDL_COAST_SPRITES_RENDERER_H
