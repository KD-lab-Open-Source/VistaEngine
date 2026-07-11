#ifndef VISTA_SDL_UI_RENDERER_H
#define VISTA_SDL_UI_RENDERER_H

// The 2D/UI half of the SDL GPU backend: text, sprites, screen-space quads, and the
// lines / rectangles the engine's 2D primitive batches draw.
//
// cSDLRenderDevice owns the SDL GPU device, the swapchain and the shared GPU
// resources (textures, vertex/index buffers); it forwards every 2D entry point of
// cInterfaceRenderDevice here. Draws accumulate into a CPU vertex batch during the
// scene and are uploaded + drawn once, in Draw(), which records its own copy pass
// followed by its own render pass. The 3D scene, water and foam pipelines get their
// own renderer classes; this one only ever touches the UI pipelines.
//
// Everything shares one vertex format and one batch, split into runs by texture and by
// primitive type (triangles or lines), so the frame draws in the order the engine
// issued it. That differs from the D3D backend, which keeps separate point/line/rect
// vectors and empties them, in that order, at FlushPrimitive2D -- but every caller
// flushes immediately after drawing, so the visible order is the same.

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

	// Untextured primitives. Callers have already clipped against the device's scissor
	// rect, as the D3D backend does before it queues them.
	void DrawLine(int x1, int y1, int x2, int y2, Color4c color);
	void DrawPixel(int x, int y, Color4c color);
	void DrawRectangle(int x, int y, int dx, int dy, Color4c color, bool outline);

	// Emits one textured quad per glyph from the font's FreeType atlas. Returns the
	// x coordinate just past the last glyph drawn (the D3D OutTextLine contract).
	int  OutTextLine(int x, int y, const FT::Font& font,
	                 const wchar_t* textline, const wchar_t* end,
	                 const Color4c& color, int xRangeMin, int xRangeMax);
	// The multi-line, aligned, scalable text call: one glyph quad per character, with
	// the engine's inline "&rrggbb" colour escapes honoured mid-string.
	void OutText(int x, int y, const char* text, const FT::Font& font,
	             const Color4f& color, ALIGN_TEXT align, Vect2f scale);

	// Quads batched so far this frame; lets the device keep its NumberPolygon stat
	// exact across OutText/OutTextLine, which emit a quad per glyph.
	int quadCount() const { return quadCount_; }

private:
	// CPU-side 2D vertex, byte-compatible with the UI vertex input layout
	// (matches sVertexXYZWDT1: float4 pos, BGRA u8 colour, float2 uv = 28 bytes).
	struct UIVertex { float x, y, z, w; unsigned int color; float u, v; };

	// One draw call per contiguous run of vertices sharing a texture and a primitive
	// type. `lines` picks the line-list pipeline over the triangle-list one.
	struct DrawRun { SDL_GPUTexture* tex; int first; int count; bool lines; };

	void createPipeline();               // pipelines + sampler + white texture
	void ensureVertexCapacity(int verts);
	// Append a quad (6 verts) bound to tex, extending or starting a draw run.
	void emitQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex);
	// Append one untextured line segment (2 verts).
	void emitLine(float x1, float y1, float x2, float y2, unsigned int color);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUGraphicsPipeline* pipeline_       = nullptr;  // triangle list
	SDL_GPUGraphicsPipeline* linePipeline_   = nullptr;  // line list; same shaders
	SDL_GPUSampler*          sampler_        = nullptr;
	SDL_GPUTexture*          whiteTexture_   = nullptr;  // untextured geometry shows vertex colour
	SDL_GPUBuffer*           vertexBuffer_   = nullptr;
	SDL_GPUTransferBuffer*   transferBuffer_ = nullptr;
	int                      vertexCapacity_ = 0;

	std::vector<UIVertex> batch_;
	std::vector<DrawRun>  runs_;
	SDL_GPUTexture*       currentTexture_ = nullptr;   // set by SetTexture
	int                   quadCount_ = 0;
};

#endif // VISTA_SDL_UI_RENDERER_H
