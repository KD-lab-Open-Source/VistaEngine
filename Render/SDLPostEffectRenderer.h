#ifndef VISTA_SDL_POST_EFFECT_RENDERER_H
#define VISTA_SDL_POST_EFFECT_RENDERER_H

// The post-effect half of the SDL GPU backend: the fullscreen passes of PostEffectManager
// (VistaRender/postEffects.cpp), for the effects that have an SDL path -- monochrome and
// the under-water effect. The shaders are ports of the originals under
// Render/shader/PostProcessing/ (Render/SDLShaders/{monochrome,underwater}.frag.hlsl).
//
// The shape of the thing differs from D3D9 in one way. There, every effect started by
// StretchRect'ing the back buffer into a texture and drew a fullscreen quad back over the
// frame. SDL GPU cannot sample the swapchain image, so the device turns the frame around
// instead: when any effect will draw, cSDLRenderDevice::armSceneCapture routes the whole
// scene into an offscreen capture target, and this renderer composites capture ->
// swapchain through the effect shaders (Documents/Render-PORTING.md #6).
//
// Record/execute, like the other renderers: PostEffectMonochrome::redraw /
// PostEffectUnderWater::redraw record their frame's parameters here (in PEManager's
// enum order, which was the D3D draw order), and cSDLRenderDevice::drawPostEffects
// executes the chain once the scene has settled into the capture. Each effect samples
// the previous stage and renders a fullscreen triangle into the next: capture -> [ping
// -> ...] -> swapchain. An empty chain is a plain copy -- the capture was armed, so
// *something* must put the scene on the swapchain.

#include "IRenderDevice.h"   // Color4f, cTexture
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUShader;

class SDLPostEffectRenderer
{
public:
	// window is needed only to query the swapchain format the passes render to.
	// Must be destroyed before the SDL GPU device.
	SDLPostEffectRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLPostEffectRenderer();

	SDLPostEffectRenderer(const SDLPostEffectRenderer&) = delete;
	SDLPostEffectRenderer& operator=(const SDLPostEffectRenderer&) = delete;

	// Drop the previous frame's effects. Called from BeginScene.
	void BeginFrame();

	// PSMonochrome::Select's one constant: 0 = the scene untouched, 1 = fully grey.
	void recordMonochrome(float phase);
	// PSUnderWater::Select's constants, unpacked: the wave texture's scroll, the effect's
	// fade scale (the *0.05 packing happens here, as it did there), the world's underwater
	// colour, and the wave texture itself. Records nothing without a resident wave texture.
	void recordUnderWater(float shift, float scale, const Color4f& color, cTexture* wave);

	bool hasEffects() const { return !effects_.empty(); }
	// Throw the pending effects away: the frame has nowhere to composite them.
	void DiscardDraws() { effects_.clear(); }

	// Execute the chain: capture -> [ping] -> swapchain, one render pass per effect, and
	// clear the recorded effects. With none recorded, a single pass-through copy. Returns
	// false only if the pipelines cannot be built (the effects are dropped either way).
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* capture, SDL_GPUTexture* swapchain,
	          int w, int h);

private:
	enum EffectKind { EFFECT_COPY = 0, EFFECT_MONOCHROME, EFFECT_UNDERWATER, EFFECT_KIND_MAX };

	// One fragment uniform block serves every effect; each shader declares the head it
	// uses. The layout is mirrored in the HLSL cbuffers -- keep all of them in step.
	struct FSUniform
	{
		float params[4];   // monochrome: (phase,-,-,-); underwater: (shift, scale*0.05, scale, -)
		float color[4];    // underwater: the water colour
	};

	struct EffectCmd
	{
		EffectKind kind;
		FSUniform fs;
		SDL_GPUTexture* wave;   // underwater only
	};

	bool createShaders();
	void createSamplers();
	SDL_GPUGraphicsPipeline* pipelineFor(EffectKind kind);
	// The intermediate stage for chains of two or more effects, created on first need,
	// re-created on resize. Frames with a single active effect never touch it.
	SDL_GPUTexture* ensurePing(int w, int h);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vs_ = nullptr;                        // the shared fullscreen triangle
	SDL_GPUShader* fs_[EFFECT_KIND_MAX] = {};
	bool shadersTried_ = false;
	SDL_GPUGraphicsPipeline* pipelines_[EFFECT_KIND_MAX] = {};

	// sampler_clamp_point, what PostEffectMonochrome::setSamplerState set (the capture is
	// 1:1 with the swapchain, so point vs linear cannot matter; keep the original's choice).
	SDL_GPUSampler* samplerClampPoint_ = nullptr;
	// sampler_wrap_linear on both stages, what PostEffectUnderWater::setSamplerState set.
	SDL_GPUSampler* samplerWrapLinear_ = nullptr;

	SDL_GPUTexture* pingTexture_ = nullptr;
	int pingW_ = 0, pingH_ = 0;

	std::vector<EffectCmd> effects_;
};

#endif // VISTA_SDL_POST_EFFECT_RENDERER_H
