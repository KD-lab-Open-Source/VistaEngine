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
// Like the original, the surface is drawn a material at a time: vMap's MultiRegion paints
// every fine cell with one of cTileMap's 16 materials, and each material carries its own
// mini-detail texture. The index buffer is therefore grouped into one contiguous run per
// material (the original's per-tile `index[material]` lists, hoisted to the whole map),
// and Draw walks the runs, rebinding the detail texture between them.
//
// Scope: the heightfield surface with its baked per-cell colour, lit per pixel from the
// baked slope (bump) map as the original's default path does, with one directional light
// and the per-material detail grain, receiving the scene's shadow map and casting into it,
// and following terramorphing: the dirty-tile flags cTileMap raises for vMap's update rects
// are consumed each frame and the touched mesh rows / colour+bump texels re-uploaded. A
// terrain cell painted with a placement-zone LAVA or ICE material is drawn over with the
// animated lava (tilemap_lava.{vert,frag}.hlsl) or the reflective ice
// (tilemap_ice.{vert,frag}.hlsl) shader instead of the plain terrain pipeline; the real
// tile/LOD streaming is still to come.

#include <string>
#include <vector>

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
	void setWindow(SDL_Window* window);

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

	// One contiguous run of the index buffer, all of whose triangles vMap's region map
	// assigns to the same cTileMap material. One draw call each.
	struct MaterialRun { int material; int first; int count; };

	void createPipeline();
	void createShadowPipeline();
	// The placement-zone LAVA material's pipeline pair and its animated noise volume, built
	// once. A lava run is drawn over the terrain mesh with these instead of the plain
	// terrain pipeline -- see the run loop in Draw. lava.._ mirrors pipelineFill_/Mirror_'s
	// cull so lava tiles behave in the reflection camera as the terrain around them does.
	void createLavaPipeline();
	// The placement-zone ICE material's pipeline pair, built once. An ice run is drawn over
	// the terrain mesh with the reflective ice shader; opaque (Z-write on) like the terrain,
	// so it needs no textures of its own -- the snow/bump come from the material and the
	// reflection from the scene's reflection camera. ice.._ mirrors the terrain's cull.
	void createIcePipeline();
	// Rebuild when vMap is reloaded in place for a new mission. Returns false while
	// the heightfield is not loaded yet (caller retries next frame) or on failure.
	bool ensureMesh(SDL_GPUCommandBuffer* cmd);
	bool buildMesh(SDL_GPUCommandBuffer* cmd);
	bool buildColorTexture(SDL_GPUCommandBuffer* cmd, int H, int V);
	// The per-fine-cell slope (bump) map the fragment shader lights from; shares the
	// colour texture's dims and step. Bakes the texel rect [px0,py0]..(+w,+h).
	bool buildBumpTexture(SDL_GPUCommandBuffer* cmd);
	void bakeBumpRect(signed char* out, int px0, int py0, int w, int h) const;
	void releaseMesh();
	// One grid vertex from vMap: world position + normal, as buildMesh samples them.
	void computeVertex(Vertex& v, int gx, int gy) const;
	// quadMat_ -> index buffer grouped into one contiguous run per material (fills runs_).
	void buildIndexData(std::vector<unsigned short>& idx);
	// Terramorphing: consume the ATTRTILE_UPDATE_* flags cTileMap::BuildRegionPoint raised
	// for vMap's update rects, recompute the covered mesh rows and colour texels in the CPU
	// mirrors and upload just those; rebuild the index runs if a cell's material repainted.
	void applyMapUpdates(SDL_GPUCommandBuffer* cmd, cTileMap* tileMap);
	// Drop flags already satisfied by a fresh build (the constructor's full-map updateMap).
	void clearTileUpdateFlags(cTileMap* tileMap);

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
	// The placement-zone LAVA material's pipeline (cull NONE) and its reflection-camera
	// variant (cull FRONT), and the 64^3 random noise volume the lava fBm samples through.
	SDL_GPUGraphicsPipeline* pipelineLava_       = nullptr;
	SDL_GPUGraphicsPipeline* pipelineLavaMirror_ = nullptr;
	SDL_GPUTexture*          volumeTexture_ = nullptr;
	// Wrap + linear on the noise volume (the original's sampler_wrap_linear on stage 0).
	SDL_GPUSampler*          volumeSampler_ = nullptr;
	// The placement-zone ICE material's pipeline (cull NONE) and its reflection-camera variant
	// (cull FRONT). No resources of its own -- snow/bump from the material, reflection from the
	// scene's reflection camera, both bound per frame in Draw.
	SDL_GPUGraphicsPipeline* pipelineIce_       = nullptr;
	SDL_GPUGraphicsPipeline* pipelineIceMirror_ = nullptr;
	SDL_GPUSampler*          sampler_      = nullptr;
	// Point + clamp: the shadow compare is done by hand on raw depth values, which must
	// not be filtered, and a receiver outside the map must read its edge, not wrap.
	SDL_GPUSampler*          shadowSampler_ = nullptr;
	// Wrap + trilinear: the detail tile repeats across the map, and its mip chain is what
	// fades the grain out with distance (the original's sampler_wrap_anisotropic).
	SDL_GPUSampler*          detailSampler_ = nullptr;

	SDL_GPUBuffer*  vertexBuffer_ = nullptr;
	SDL_GPUBuffer*  indexBuffer_  = nullptr;
	int             indexCount_   = 0;
	std::vector<MaterialRun> runs_;            // the index buffer, grouped by material
	SDL_GPUTexture* colorTexture_ = nullptr;   // baked per-cell surface colour (vMap.clrBuf)
	SDL_GPUTexture* bumpTexture_  = nullptr;   // baked per-cell slopes (the original's V8U8)

	// CPU mirrors of the GPU mesh, kept so terramorphing can patch sub-rects in place.
	std::vector<Vertex>        verts_;
	std::vector<unsigned char> quadMat_;   // 2 per grid quad: each triangle's material
	int gw_ = 0, gh_ = 0;                  // grid vertices per axis
	int nx_ = 0, ny_ = 0;                  // grid quads per axis
	int step_ = 0;                         // fine cells per grid cell
	int texW_ = 0, texH_ = 0, texStep_ = 0;   // colour texture dims + its bake step
	SDL_GPUTexture* whiteTexture_ = nullptr;   // 1x1, substituted in wireframe mode
	// Mid-grey 1x1, bound as the detail texture when a material has none: the shader adds
	// `detail - 0.5`, so 128,128,128 contributes exactly nothing. (The DetailParams gate
	// makes it unreachable; it is here so the binding is never null.)
	SDL_GPUTexture* greyTexture_ = nullptr;

	std::string builtWorld_;        // vMap world the current mesh was built for
	bool        buildFailed_ = false;
	float       uvScale_[2] = {0.f, 0.f};   // 1/H_SIZE, 1/V_SIZE -> shader's UV.zw
};

#endif // VISTA_SDL_TILEMAP_RENDERER_H
