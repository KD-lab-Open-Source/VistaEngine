#ifndef VISTA_SDL_UI_RENDERER_H
#define VISTA_SDL_UI_RENDERER_H

// The 2D/UI half of the SDL GPU backend: text, sprites and screen-space quads.
//
// cSDLRenderDevice owns the SDL GPU device, the swapchain and the shared GPU
// resources (textures, vertex/index buffers); it forwards every 2D entry point of
// cInterfaceRenderDevice here. Draws accumulate into a CPU quad batch during the
// scene and are uploaded + drawn once, in Draw(), which records its own copy pass
// followed by its own render pass. The 3D scene, water and foam pipelines get their
// own renderer classes; this one only ever touches the UI pipeline.

#include "IRenderDevice.h"
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

class SDLUIRenderer
{
public:
	// Builds the UI pipeline, sampler and 1x1 white texture. window is needed only
	// to query the swapchain format the pipeline renders to. Must be destroyed
	// before the SDL GPU device it was built on.
	SDLUIRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLUIRenderer();

	SDLUIRenderer(const SDLUIRenderer&) = delete;
	SDLUIRenderer& operator=(const SDLUIRenderer&) = delete;

	// Drop the previous frame's quads. Called from BeginScene.
	void BeginFrame();

	// Upload the frame's quads (copy pass) and draw them (render pass) into target.
	// Always opens the render pass, even with an empty batch: that pass is what
	// performs the frame's clear when the device's Fill() asked for one.
	void Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target,
	          int screenW, int screenH, bool clear, const float clearColor[4]);

	// --- 2D entry points, forwarded from cSDLRenderDevice --------------------
	// Texture for the following DrawQuad calls (from SetNoMaterial).
	void SetTexture(cTexture* texture);
	void DrawQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, Color4c color);
	void DrawSprite(int x, int y, int dx, int dy,
	                float u, float v, float du, float dv,
	                cTexture* texture, const Color4c& color);
	// Emits one textured quad per glyph from the font's FreeType atlas. Returns the
	// x coordinate just past the last glyph drawn (the D3D OutTextLine contract).
	int  OutTextLine(int x, int y, const FT::Font& font,
	                 const wchar_t* textline, const wchar_t* end,
	                 const Color4c& color, int xRangeMin, int xRangeMax);

	// Quads batched so far this frame; lets the device keep its NumberPolygon stat
	// exact across OutTextLine, which emits a quad per glyph.
	int quadCount() const { return (int)batch_.size() / 6; }

private:
	// CPU-side 2D vertex, byte-compatible with the UI vertex input layout
	// (matches sVertexXYZWDT1: float4 pos, BGRA u8 colour, float2 uv = 28 bytes).
	struct UIVertex { float x, y, z, w; unsigned int color; float u, v; };

	// One draw call per contiguous run of quads sharing a texture.
	struct DrawRun { SDL_GPUTexture* tex; int first; int count; };

	void createPipeline();               // pipeline + sampler + white texture
	void ensureVertexCapacity(int verts);
	// Append a quad (6 verts) bound to tex, extending or starting a draw run.
	void emitQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUGraphicsPipeline* pipeline_       = nullptr;
	SDL_GPUSampler*          sampler_        = nullptr;
	SDL_GPUTexture*          whiteTexture_   = nullptr;  // untextured quads show vertex colour
	SDL_GPUBuffer*           vertexBuffer_   = nullptr;
	SDL_GPUTransferBuffer*   transferBuffer_ = nullptr;
	int                      vertexCapacity_ = 0;

	std::vector<UIVertex> batch_;
	std::vector<DrawRun>  runs_;
	SDL_GPUTexture*       currentTexture_ = nullptr;   // set by SetTexture
};

#endif // VISTA_SDL_UI_RENDERER_H
