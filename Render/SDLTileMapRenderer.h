#ifndef VISTA_SDL_TILEMAP_RENDERER_H
#define VISTA_SDL_TILEMAP_RENDERER_H

// The terrain half of the SDL GPU backend: draws the vMap heightfield with the
// tilemap pipeline (Render/SDLShaders/tilemap.{vert,frag}.hlsl, ported from the
// original Render/shader/Minimal/tile_map_scene.{vsl,psl}).
//
// Reached through the engine's own delegation chain -- cScene::Draw ->
// Camera::DrawScene -> Camera::DrawTilemapObject -> cTileMap::Draw -- which forwards
// here via cSDLRenderDevice::drawTileMap. Unlike SDLUIRenderer and SDLObject3dxRenderer,
// which batch during the scene and draw once at EndScene, this renderer draws
// immediately: it opens its own render pass inside cTileMap::Draw, so its pass lands in
// the frame's command buffer before the object and UI passes recorded on top of it.
// Colour and depth targets belong to the device and are shared with those passes.
//
// It owns its GPU geometry outright (raw SDL_GPUBuffer, not the device's sPtr VB/IB
// slots) because the mesh is built here, from vMap, and shared with nothing.
//
// Scope: the heightfield surface with its baked per-cell colour and one directional
// light, receiving the scene's shadow map and casting into it. The original's remaining
// layers -- bump, lightmap, detail texture, fog of war, fog -- and the real tile/LOD
// streaming are still to come.

#include <string>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;

class Camera;
class cTileMap;

class SDLTileMapRenderer
{
public:
	// Builds the tilemap pipeline and sampler. window is needed only to query the
	// swapchain format. Must be destroyed before the SDL GPU device it was built on.
	SDLTileMapRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLTileMapRenderer();

	SDLTileMapRenderer(const SDLTileMapRenderer&) = delete;
	SDLTileMapRenderer& operator=(const SDLTileMapRenderer&) = delete;

	// Build the terrain mesh if needed (uploading through cmd), then draw it in its
	// own render pass against the device's shared colour and depth targets. `clear` /
	// `clearDepth` mean this pass owns the frame's colour / depth clear -- true only
	// when no earlier pass took them. `wireframe` follows the device's RS_FILLMODE
	// (debugWireFrame): it draws unlit white edges instead of the shaded surface, so
	// the grid is legible even where the map's baked colour is black.
	// Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int screenW, int screenH, bool clear, const float clearColor[4],
	          bool clearDepth, cTileMap* tileMap, Camera* camera, bool wireframe);

	// Draw the terrain into the shadow map, from the light camera, in a depth-only pass
	// (no colour target). Called from cTileMap::Draw under ATTRCAMERA_SHADOWMAP -- where
	// the original calls tileMapRender_->DrawBump(camera, ALPHA_TEST, true, false) -- so
	// it runs before the object casters, which then load the depth it wrote.
	// `clearDepth` means this pass owns the map's clear. Returns true if the pass ran.
	bool DrawShadowPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* depth, int size,
	                    Camera* camera, bool clearDepth);

private:
	// World-space position + normal, matching tilemap.vert.hlsl's two attributes.
	// The original tilemap vertex is the same pair (position, normal-in-COLOR0); UVs
	// are derived from world XY in the shader, so none are stored.
	struct Vertex { float x, y, z; float nx, ny, nz; };

	void createPipeline();
	void createShadowPipeline();
	// Rebuild when vMap is reloaded in place for a new mission. Returns false while
	// the heightfield is not loaded yet (caller retries next frame) or on failure.
	bool ensureMesh(SDL_GPUCommandBuffer* cmd);
	bool buildMesh(SDL_GPUCommandBuffer* cmd);
	bool buildColorTexture(SDL_GPUCommandBuffer* cmd, int H, int V);
	void releaseMesh();

	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	// Same shaders, differing only in fill mode. FILL/LINE mirrors RS_FILLMODE.
	SDL_GPUGraphicsPipeline* pipelineFill_ = nullptr;
	SDL_GPUGraphicsPipeline* pipelineLine_ = nullptr;
	// The reflection camera's: culls the face the mirror reversed, so the heightfield's
	// underside does not roof over the reflection (see createPipeline).
	SDL_GPUGraphicsPipeline* pipelineMirror_ = nullptr;
	// Depth-only, position-only, slope-scale biased: the terrain as a shadow caster.
	SDL_GPUGraphicsPipeline* pipelineShadow_ = nullptr;
	SDL_GPUSampler*          sampler_      = nullptr;
	// Point + clamp: the shadow compare is done by hand on raw depth values, which must
	// not be filtered, and a receiver outside the map must read its edge, not wrap.
	SDL_GPUSampler*          shadowSampler_ = nullptr;

	SDL_GPUBuffer*  vertexBuffer_ = nullptr;
	SDL_GPUBuffer*  indexBuffer_  = nullptr;
	int             indexCount_   = 0;
	SDL_GPUTexture* colorTexture_ = nullptr;   // baked per-cell surface colour (vMap.clrBuf)
	SDL_GPUTexture* whiteTexture_ = nullptr;   // 1x1, substituted in wireframe mode

	std::string builtWorld_;        // vMap world the current mesh was built for
	bool        buildFailed_ = false;
	float       uvScale_[2] = {0.f, 0.f};   // 1/H_SIZE, 1/V_SIZE -> shader's UV.zw
};

#endif // VISTA_SDL_TILEMAP_RENDERER_H
