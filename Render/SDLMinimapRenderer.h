#ifndef VISTA_SDL_MINIMAP_RENDERER_H
#define VISTA_SDL_MINIMAP_RENDERER_H

// The minimap: the map quad itself, and the unit symbols, events, view zone, placement
// rectangles and border drawn over it. Owns the two minimap pipelines (ports of the
// original's miniMap.psl and miniMapBorder.psl -- see Render/SDLShaders/minimap*.hlsl) and
// its own vertex buffer.
//
// It does NOT open a render pass, and that is the whole reason it is shaped this way. The
// minimap is one control among many: UI_Minimap::redraw runs part-way through the frame's
// UI, with panels drawn before it and its own start-location labels -- ordinary UI text --
// drawn after. A pass of its own would land either under or over the entire UI, never in
// between. So SDLUIRenderer keeps the ordering: every call here records a draw and drops a
// marker into the UI batch's run list, and when SDLUIRenderer reaches that marker inside its
// own render pass it calls back into DrawRun. The minimap's pipelines and vertex buffer
// stay here; only the sequencing is the UI renderer's.
//
// Reached through the engine's own call path -- UI_ControlCustom::redraw -> UI_Minimap::
// redraw -> drawMiniMap / flushLines / flushRectangles / flushSprites -- which forwards here
// where the Windows build talks to gb_RenderDevice3D->psMiniMap and the device's typed
// vertex buffers.

#include "Util/XMath/Colors.h"
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

class cTexture;
class SDLUIRenderer;

class SDLMinimapRenderer
{
public:
	// Builds the two pipelines and the clamp/linear sampler. window is needed only to query
	// the swapchain format. Must be destroyed before the SDL GPU device it was built on.
	SDLMinimapRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLMinimapRenderer();

	SDLMinimapRenderer(const SDLMinimapRenderer&) = delete;
	SDLMinimapRenderer& operator=(const SDLMinimapRenderer&) = delete;

	// Screen-space vertex, shared by both pipelines (minimap.vert.hlsl). The original's
	// sVertexXYZWDT1/2/4 with the unused coordinate sets dropped: position in pixels, the
	// packed BGRA diffuse, the control-space mask UV, and the pipeline's own second UV --
	// the map for DrawMap, the sprite's atlas rect for DrawPrims.
	struct Vertex { float x, y; unsigned int color; float mu, mv; float u, v; };

	// What the map quad composes, one field per #ifdef in the original miniMap.psl. The
	// nulls are the "off" case in every instance: no map texture falls back to terraColor,
	// no water skips the blend, no mask leaves the minimap square.
	struct MapState
	{
		cTexture* base = nullptr;        // the world's map.tga
		cTexture* water = nullptr;       // cWater::GetTextureMiniMap()
		cTexture* addition = nullptr;    // the fog of war, or the placement zones
		cTexture* mask = nullptr;        // the control's border mask
		int   additionMode = 0;          // 0 none, 1 fog of war, 2 placement zones
		float additionAlpha = 0.f;
		Color4f waterColor = Color4f::BLUE;
		Color4f terraColor = Color4f::WHITE;
	};

	// The UI renderer this one's draws are sequenced into. Set by the device, which owns both.
	void setUIRenderer(SDLUIRenderer* ui) { ui_ = ui; }

	// Drop the previous frame's draws. Called from SDLUIRenderer::BeginFrame.
	void BeginFrame();

	// The map, as one quad: four corners in the tri-strip order UI_Minimap::drawMiniMap
	// lays them out ((-,-), (-,+), (+,-), (+,+)).
	//
	// This and DrawPrims record the draw and immediately claim its place in the UI
	// renderer's run list, so the minimap's parts land on screen in the order UI_Minimap
	// issued them -- map, then symbols, then the view zone and border -- and whatever the UI
	// draws next goes on top.
	void DrawMap(const MapState& state, const Vertex corners[4]);

	// One batch of the things drawn over the map: `lines` picks a line list over a triangle
	// list, `texture` is the sprite atlas (null for lines and rectangles, which show only
	// their vertex colour), `phase` picks its animation frame the way SetTexturePhase does,
	// and `mask` clips the batch to the control's border.
	void DrawPrims(cTexture* mask, cTexture* texture, float phase, bool lines,
	               const Vertex* verts, int count);

	bool empty() const { return draws_.empty(); }
	int  drawCount() const { return (int)draws_.size(); }

	// Upload the frame's vertices. SDLUIRenderer calls this from its own Draw, before it
	// opens the render pass -- copy passes cannot run inside one.
	void Upload(SDL_GPUCommandBuffer* cmd);

	// Execute draw `index` inside SDLUIRenderer's render pass, at the point in its run list
	// where the minimap issued it. Binds this renderer's pipeline, vertex buffer, textures
	// and uniforms, so the caller must rebind its own afterwards.
	void DrawRun(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd, int index,
	             int screenW, int screenH);

private:
	// One recorded draw: a run of vertices, the pipeline to draw it with, up to four
	// textures, and the fragment uniforms that gate the shader's optional layers.
	struct Draw
	{
		bool map;                  // the map pipeline, else the border/sprite one
		bool lines;                // border/sprite pipeline only
		int  first, count;
		SDL_GPUTexture* tex[4];    // map: base/water/addition/border. prims: base/border.
		float params[16];          // map: WaterColor/AdditionAlpha/TerraColor/Flags. prims: Flags.
		int   paramFloats;
	};

	void createPipelines();
	void ensureVertexCapacity(int verts);

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUGraphicsPipeline* mapPipeline_      = nullptr;   // minimap.frag,        triangles
	SDL_GPUGraphicsPipeline* primPipeline_     = nullptr;   // minimap_border.frag, triangles
	SDL_GPUGraphicsPipeline* primLinePipeline_ = nullptr;   // minimap_border.frag, lines
	// Clamp + linear, the original's sampler_clamp_linear: every one of these textures is
	// stretched across the control exactly once, so a sample off the edge must read the edge.
	SDL_GPUSampler*        sampler_        = nullptr;
	// Stands in for an unbound optional texture. Never sampled -- the shader gates on Flags
	// -- but SDL still requires the binding to be non-null.
	SDL_GPUTexture*        whiteTexture_   = nullptr;
	SDL_GPUBuffer*         vertexBuffer_   = nullptr;
	SDL_GPUTransferBuffer* transferBuffer_ = nullptr;
	int                    vertexCapacity_ = 0;

	std::vector<Vertex> batch_;
	std::vector<Draw>   draws_;
	SDLUIRenderer*      ui_ = nullptr;   // not owned; the device owns both
};

// The SDL backend's minimap renderer, or null under any other device. UI_Minimap's draw half
// drives it exactly as it drives psMiniMap / psMiniMapBorder on Windows.
SDLMinimapRenderer* sdlMinimapRenderer();

#endif // VISTA_SDL_MINIMAP_RENDERER_H
