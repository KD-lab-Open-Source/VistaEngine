#ifndef VISTA_SDL_BLOBS_RENDERER_H
#define VISTA_SDL_BLOBS_RENDERER_H

// The KD-lab logo splash's metaballs (cBlobs, UserInterface/Bubles/Blobs.cpp) on the SDL
// GPU backend: the screen full of jelly cells the fish swims under, drawn over the frame
// by ReelManager::showLogoModal.
//
// Two passes, which is what cBlobs always was:
//
//   1. The FIELD. cBlobs::Draw records one cell per kdCell; each becomes a screen-space
//      quad the size of the falloff texture cBlobs::CreateBlobsTexture builds on the CPU
//      (a cosine bump inside the unit circle), drawn ADDITIVELY into an offscreen target
//      of its own. Overlapping cells sum, which is the whole metaball trick: the field is
//      a scalar height map and the composite thresholds it, so two cells that touch merge
//      into one shape instead of showing a seam.
//   2. The COMPOSITE. One fullscreen triangle that reads the field and the frame and
//      writes the result to the swapchain -- Render/SDLShaders/blobs.frag.hlsl, the port
//      of PSBlobsShader's Render/shader/PostProcessing/blobs.psl.
//
// The frame the composite refracts is the SCENE-CAPTURE target (Render-PORTING.md #6a).
// D3D9 drew the scene to the back buffer and StretchRect'ed a copy into a texture each
// frame; SDL GPU cannot sample the swapchain, so cBlobs::BeginFrame arms the capture
// before the scene walk and the composite reads that instead -- the same turn-around the
// post effects use, and for the same reason. cSDLRenderDevice::drawBlobs settles the
// capture and runs both passes, standing in for drawPostEffects: nothing else composites
// this frame, because the splash has no Environment and so no PostEffectManager.
//
// The field pass reuses the UI shaders (ui.{vert,frag}.hlsl) unchanged. They are exactly
// what the original's `cQuadBuffer<sVertexXYZWDT1>` + `SetNoMaterial(ALPHA_NONE, ...,
// Texture)` came to: a pre-transformed pixel-space quad shaded `texture * diffuse`. Only
// the blend differs, and the blend rides the pipeline. So the only new shader here is the
// composite.
//
// The original ran the composite only `if(gb_RenderDevice3D->IsPS20())` and, without it,
// drew the cells straight to the back buffer in cBlobsSetting::color_. That fallback is
// not ported: it was for cards below PS 2.0, and the floor here is well above that.

#include "IRenderDevice.h"   // Color4f, cTexture
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUShader;

class SDLBlobsRenderer
{
public:
	// window is needed only to query the swapchain format the passes render to.
	// Must be destroyed before the SDL GPU device.
	SDLBlobsRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLBlobsRenderer();

	SDLBlobsRenderer(const SDLBlobsRenderer&) = delete;
	SDLBlobsRenderer& operator=(const SDLBlobsRenderer&) = delete;

	// Drop the previous frame's cells and composite. Called from BeginScene.
	void BeginFrame();

	// cBlobs::BeginDraw -- start a fresh field. The D3D version also bound the field
	// render target and cleared it here; the clear rides this pass's load op instead.
	void BeginCells();
	// cBlobs::Draw -- one cell, centred on (x,y) in screen pixels. `phase` is kdCell's
	// colourPhase, the cell's fade in/out, which scaled the white vertex diffuse.
	void AddCell(float x, float y, float phase);
	// cBlobs::EndDraw -- the falloff texture every cell quad carries.
	void EndCells(cTexture* cellTexture);

	// cBlobs::DrawBlobsShader -- PSBlobsShader::Select's constants. Recording this is what
	// arms the composite; a frame that records none draws nothing at all.
	void recordComposite(float fadePhase, const Color4f& color, const Color4f& specular);

	bool hasComposite() const { return compositeArmed_; }
	// Throw the pending frame away: there is nowhere to composite it, and it must not
	// replay into the next one.
	void DiscardDraws() { cells_.clear(); compositeArmed_ = false; }

	// Run both passes: the cells into the field, then field + scene -> target. Clears what
	// was recorded. `scene` is the capture holding this frame's scene walk; w/h size the
	// target, and the field is kept at the same size (the original's field target was made
	// at the screen size too, and blobs.psl's pixel_size assumes it).
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* scene, SDL_GPUTexture* target,
	          int w, int h);

private:
	// ui.vert.hlsl's vertex: pixel-space position, packed BGRA diffuse, one UV.
	struct CellVertex { float x, y, z, w; unsigned int color; float u, v; };

	// blobs.frag.hlsl's cbuffer. Mirrored there -- keep both in step.
	struct FSUniform
	{
		float pixelSize[4];
		float color[4];
		float specular[4];
		float fadePhase[4];
	};

	bool createShaders();
	void createSamplers();
	bool createPipelines();
	// The field the cells accumulate into, created on first need and re-created on resize.
	SDL_GPUTexture* ensureField(int w, int h);
	void ensureVertexCapacity(int verts);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* cellVS_ = nullptr;      // ui.vert.hlsl
	SDL_GPUShader* cellFS_ = nullptr;      // ui.frag.hlsl
	SDL_GPUShader* compositeVS_ = nullptr; // posteffect.vert.hlsl, the fullscreen triangle
	SDL_GPUShader* compositeFS_ = nullptr; // blobs.frag.hlsl
	bool shadersTried_ = false;
	SDL_GPUGraphicsPipeline* cellPipeline_ = nullptr;
	SDL_GPUGraphicsPipeline* compositePipeline_ = nullptr;

	// sampler_clamp_point, which cBlobs::EndDraw asks for: the cell sprite is drawn at its
	// own size, 1:1 with the field's texels, and the composite reads the field at exact
	// texel offsets. Point costs nothing and is what the original set.
	SDL_GPUSampler* samplerClampPoint_ = nullptr;
	// The frame, sampled through a displaced UV that lands well off the texel grid.
	SDL_GPUSampler* samplerClampLinear_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;

	SDL_GPUTexture* fieldTexture_ = nullptr;
	int fieldW_ = 0, fieldH_ = 0;

	SDL_GPUBuffer*         vertexBuffer_ = nullptr;
	SDL_GPUTransferBuffer* transferBuffer_ = nullptr;
	int vertexCapacity_ = 0;

	std::vector<CellVertex> cells_;
	SDL_GPUTexture* cellTexture_ = nullptr;
	FSUniform fs_ = {};
	bool compositeArmed_ = false;
};

#endif // VISTA_SDL_BLOBS_RENDERER_H
