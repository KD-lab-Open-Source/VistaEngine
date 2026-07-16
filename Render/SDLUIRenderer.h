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

#include "IRenderDevice.h"   // also brings in sVertexXYZWDT1, the 2D quad buffer's vertex
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

class SDLMinimapRenderer;

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

	// The minimap draws part-way through the UI -- panels behind it, its own start-location
	// labels (ordinary UI text) in front -- so it cannot own a pass of its own without
	// landing under or over the whole UI. Instead it records into SDLMinimapRenderer, drops
	// a marker here with EmitMinimapRun, and this renderer replays it at that exact point of
	// its own pass. The device wires the two together and owns both.
	void setMinimapRenderer(SDLMinimapRenderer* minimap) { minimap_ = minimap; }
	// Reserve the next slot in the draw order for minimap draw `index`.
	void EmitMinimapRun(int index);

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
	// Sampler for what follows (from SetSamplerData/SetSamplerDataVirtual). Only the
	// address mode is honoured -- clamp or wrap -- since no 2D caller asks for anything but
	// linear filtering, and only one asks for wrap: the selection frame, whose centre tiles
	// its texture across the box and so runs its UVs past 1. Sticky, as the sampler state is
	// on D3D, where it is one global the scene sets too.
	void SetSampler(const SAMPLER_DATA& data);
	void DrawQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, Color4c color);
	// blend honours ALPHA_ADDBLENDALPHA -- D3D's (SRCALPHA, ONE) -- which the lens flare's
	// sprites ask for; everything else draws with the straight-alpha pipeline, as before.
	void DrawSprite(int x, int y, int dx, int dy,
	                float u, float v, float du, float dv,
	                cTexture* texture, const Color4c& color, eBlendMode blend = ALPHA_BLEND);

	// cQuadBuffer<sVertexXYZWDT1>'s contract, for the 2D callers that fill their quads'
	// corners by hand rather than through DrawSprite -- the selection frame, whose edge and
	// corner sprites rotate and mirror their UVs. BeginDraw opens a run, each Get hands back
	// the four vertices of one quad, EndDraw batches them. They draw with the texture
	// SetNoMaterial last named, as they do on D3D, where that call and the quad buffer are
	// both the device's.
	//
	// Get's four vertices are top-left, bottom-left, top-right, bottom-right: the pairing
	// the device's standard index buffer gives them (triangles 0-1-2 and 2-1-3), which is
	// what the callers' corner order means. They are D3D pre-transformed vertices, so x and
	// y are pixels and z/w are ignored here, as the fixed-function pipeline effectively
	// ignores them there.
	void BeginDraw(const MatXf& = MatXf::ID);
	sVertexXYZWDT1* Get();
	void EndDraw();

	// cVertexBuffer<sVertexXYZWD>'s contract, likewise, for the 2D callers that draw
	// untextured triangle strips and lists rather than quads -- the parameter ring a unit
	// draws around itself when hovered or selected. Lock hands back nVertex vertices to
	// fill, Unlock closes them, and DrawPrimitive batches them as triangles: the batch has
	// no strip primitive, and these callers draw few enough that unrolling a strip costs
	// less than a pipeline of its own. `nPolygon` is the triangle count, as it is on D3D.
	//
	// GetSize is the room left before the caller must flush what it has and lock again. On
	// D3D that is whatever is left of the device's shared buffer, which is thousands of
	// vertices; here the batch grows on demand, so Lock reports a comparable floor and the
	// callers flush as rarely as they do there.
	sVertexXYZWD* Lock(int nVertex);
	void Unlock(int nVertex);
	void DrawPrimitive(PRIMITIVETYPE type, int nPolygon);
	int  GetSize() const { return lockCapacity_; }

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

	// One draw call per contiguous run of vertices sharing a texture, a sampler, a
	// primitive type and a blend. `lines` picks the line-list pipeline over the
	// triangle-list one; `additive` the (SRCALPHA, ONE) pipeline over the straight-alpha
	// one. `minimap` >= 0 instead means the run is a placeholder: it owns no vertices
	// here, and drawing it hands off to SDLMinimapRenderer's draw of that index.
	struct DrawRun { SDL_GPUTexture* tex; SDL_GPUSampler* sampler; int first; int count; bool lines; bool additive; int minimap; };

	void createPipeline();               // pipelines + sampler + white texture
	void ensureVertexCapacity(int verts);
	// Append a quad (6 verts) bound to tex, extending or starting a draw run.
	void emitQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex,
	              bool additive = false);
	// The same, from four hand-filled corners (see Get) rather than a rect and a UV rect.
	void emitQuad(const sVertexXYZWDT1* corners);
	// Append one untextured line segment (2 verts).
	void emitLine(float x1, float y1, float x2, float y2, unsigned int color);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUGraphicsPipeline* pipeline_       = nullptr;  // triangle list, straight alpha
	SDL_GPUGraphicsPipeline* linePipeline_   = nullptr;  // line list; same shaders
	SDL_GPUGraphicsPipeline* addPipeline_    = nullptr;  // triangle list, (SRCALPHA, ONE)
	SDL_GPUSampler*          sampler_        = nullptr;  // linear, clamp: what 2D wants
	SDL_GPUSampler*          samplerWrap_    = nullptr;  // linear, wrap: the selection centre
	SDL_GPUTexture*          whiteTexture_   = nullptr;  // untextured geometry shows vertex colour
	SDL_GPUBuffer*           vertexBuffer_   = nullptr;
	SDL_GPUTransferBuffer*   transferBuffer_ = nullptr;
	int                      vertexCapacity_ = 0;

	std::vector<UIVertex> batch_;
	std::vector<DrawRun>  runs_;
	// The open BeginDraw..EndDraw run, 4 vertices per quad. They cannot go straight into
	// batch_: Get hands the caller a pointer to fill *after* it returns, and batch_ holds
	// two triangles (6 vertices) per quad, not four corners.
	std::vector<sVertexXYZWDT1> quadCorners_;
	// The open Lock..DrawPrimitive run, for the same reason: the caller fills these after
	// Lock returns, and DrawPrimitive is what says which triangles they make.
	std::vector<sVertexXYZWD> lockVerts_;
	int lockCapacity_ = 0;
	SDL_GPUTexture*       currentTexture_ = nullptr;   // set by SetTexture
	SDL_GPUSampler*       currentSampler_ = nullptr;   // set by SetSampler; null => sampler_
	int                   quadCount_ = 0;
	SDLMinimapRenderer*   minimap_ = nullptr;          // not owned; the device owns both
};

#endif // VISTA_SDL_UI_RENDERER_H
