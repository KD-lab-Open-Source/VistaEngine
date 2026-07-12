#ifndef VISTA_SDL_WATER_RENDERER_H
#define VISTA_SDL_WATER_RENDERER_H

// The water half of the SDL GPU backend: draws cWater's surface grid with the water
// pipeline (Render/SDLShaders/water.{vert,frag}.hlsl, ported from the original
// Render/shader/Water/water_easy.{vsl,psl}).
//
// Reached through the engine's own delegation chain -- cScene::Draw -> Camera::DrawScene
// -> Camera::DrawObjectSpecial -> cWater::Draw -- which drives this renderer exactly as
// it drives VSWater/PSWater on Windows: it sets the state it would have pushed into the
// shader constants, then issues the visible tiles' draws through DrawIndexedPrimitive.
//
// It owns no geometry. The surface is the vertex/index buffers cWater::Init created
// through cSDLRenderDevice::CreateVertexBuffer/CreateIndexBuffer and cWater::UpdateVB
// refills every frame from the height field; this renderer only asks the device to
// resolve an sPtr wrapper to the SDL_GPUBuffer behind it.
//
// Scope: two of cWater's four techniques, chosen by cWater::setTechnique and carried in
// State::reflection.
//
//   WATER_EMPTY            -- a flat reflected-sky colour, the depth-derived per-vertex
//                             opacity, and the two scrolling wave maps thickening it
//                             along the crests. What the original picks without PS2.0,
//                             and what we pick when reflection is off in the options.
//   WATER_LINEAR_REFLECTION -- the same surface, but coloured by a projective sample of
//                             the reflection camera's render target, plus the sun glint
//                             water_linear.psl computes from the wave normals.
//
// Still to come: WATER_REFLECTION samples the sky cubemap, which has no SDL path yet, and
// WATER_LAVA has its own shader pair. Also missing, each an input the SDL backend does not
// have: the fog-of-war lightmap, fog, the FLOAT_ZBUFFER soft shoreline, and the
// environment-water border tiles.

#include "IRenderDevice.h"    // cTexture, sPtrVertexBuffer, sPtrIndexBuffer
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;

class Camera;
class cSDLRenderDevice;

class SDLWaterRenderer
{
public:
	// Builds the water pipelines and sampler. owner is the device that holds the
	// vertex/index buffers this renderer draws from; window is needed only to query the
	// swapchain format. Must be destroyed before the SDL GPU device it was built on.
	SDLWaterRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window);
	~SDLWaterRenderer();

	SDLWaterRenderer(const SDLWaterRenderer&) = delete;
	SDLWaterRenderer& operator=(const SDLWaterRenderer&) = delete;

	// The surface's whole shader state, straight out of cWater::Draw: the same values it
	// feeds to VSWater::SetSpeed / SetSpeed1 and the PSWater setters.
	struct State
	{
		// xy = the world->uv scale, zw = the scroll offset. Two layers, sampled under
		// opposite position swizzles (see water.vert.hlsl).
		float uvScaleOffset[4]  = {0, 0, 0, 0};
		float uvScaleOffset1[4] = {0, 0, 0, 0};
		cTexture* texture0 = nullptr;   // waves.dds
		cTexture* texture1 = nullptr;   // waves1.dds

		// --- WATER_EMPTY ---
		// vPS11Color: cWater's cur_reflect_sky_color, the reflected-sky colour for the
		// current time of day, standing in for a reflection.
		float ps11Color[4] = {0, 0, 0, 0};

		// --- WATER_LINEAR_REFLECTION (reflection == true) ---
		bool  reflection = false;
		// The reflection camera's render target, and the vMirrorVP that projects a world
		// position into it. cWater::Draw finds both through FindChildCamera.
		cTexture* reflectionTexture = nullptr;
		float mirrorVP[16] = {0};
		// vReflectionColor, already through PSWater::SetReflectionColor's premultiply:
		// rgb is the water's own tint times its weight, a is what is left for the sample.
		float reflectionColor[4] = {0, 0, 0, 0};
		float brightness = 0.f;           // fBrightnes
		float lightColor[4] = {0, 0, 0, 0};   // sun diffuse, a = cWater's flashIntensity_
		float lightDirection[3] = {0, 0, 0};
		float cameraPos[3] = {0, 0, 0};
	};

	// Drop the previous frame's draws. Called from BeginScene.
	void BeginFrame();

	// Snapshot the state the following draws are made with, plus the camera's
	// view-projection and viewport. cWater::Draw calls this once, before DrawPolygons.
	void SetState(const State& state, Camera* camera);

	// Record one visible tile range, standing in for the D3D call of the same name that
	// cWater::DrawPolygons issues per visible line. The buffers are cWater's; the device
	// resolves them. Ignored until SetState has run.
	void DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex,
	                          const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon);

	bool hasDraws() const { return !draws_.empty(); }

	// Replay the frame's draws into one colour+depth render pass, blended over what is
	// already there and writing no depth (cWater::Draw's ALPHA_BLEND with
	// RS_ZWRITEENABLE off). `clear`/`clearDepth` mean this pass owns the frame's
	// colour/depth clear -- true only when no earlier pass took it. Clears the recorded
	// draws. Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int screenW, int screenH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	// water.vert.hlsl's whole cbuffer. Both variants declare it whole, so one struct
	// serves both; MirrorVP is simply unread when REFLECTION=0.
	struct VSUniform
	{
		float mvp[16];
		float uvScaleOffset[4];
		float uvScaleOffset1[4];
		float mirrorVP[16];
		float fogPlane[4];   // cSDLRenderDevice::fogPlane(camera)
	};
	// water.frag.hlsl's whole cbuffer, likewise. params.x is fBrightnes.
	struct FSUniform
	{
		float ps11Color[4];
		float reflectionColor[4];
		float lightColor[4];
		float lightDirection[4];
		float cameraPos[4];
		float params[4];
		float fogColor[4];   // D3DRS_FOGCOLOR
	};

	// One tile range of the surface grid. Every draw in a frame shares the one state
	// below: the water is a single material, set once per frame by cWater::Draw.
	struct DrawCmd
	{
		SDL_GPUBuffer* vertexBuffer;
		SDL_GPUBuffer* indexBuffer;
		int firstIndex, indexCount;
	};

	void createPipelines();
	// Builds one (vertex, fragment) pair from the embedded blobs. `reflection` picks the
	// -DREFLECTION=1 variant, which declares the third sampler.
	bool createPipelinePair(bool reflection,
	                        SDL_GPUGraphicsPipeline*& fill, SDL_GPUGraphicsPipeline*& line);

	cSDLRenderDevice* owner_  = nullptr;   // owns the vertex/index buffers we draw
	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	// [reflection][wireframe]. The two techniques differ in their shaders, not their
	// pipeline state; FILL/LINE mirrors RS_FILLMODE.
	SDL_GPUGraphicsPipeline* pipelineFill_ = nullptr;
	SDL_GPUGraphicsPipeline* pipelineLine_ = nullptr;
	SDL_GPUGraphicsPipeline* pipelineReflectFill_ = nullptr;
	SDL_GPUGraphicsPipeline* pipelineReflectLine_ = nullptr;
	// The original's sampler_wrap_anisotropic on stages 0 and 1: the wave maps tile
	// across the whole surface.
	SDL_GPUSampler* sampler_ = nullptr;
	// sampler_clamp_anisotropic on stage 2: the reflection target is sampled projectively
	// and must not wrap where the ripple offset pushes the lookup past its edge.
	SDL_GPUSampler* samplerClamp_ = nullptr;
	// 1x1 flat wave map (the V8U8 zero slope, biased: R=G=0x80), bound when a wave
	// texture is missing. A white stand-in would read as a full-strength crest and
	// double the water's opacity everywhere.
	SDL_GPUTexture* flatTexture_ = nullptr;

	// The state cWater::Draw set, captured at SetState. Draws are replayed in one pass
	// long after the scene walk moved on, so the camera cannot be read back then.
	VSUniform vs_ = {};
	FSUniform fs_ = {};
	SDL_GPUTexture* texture0_ = nullptr;
	SDL_GPUTexture* texture1_ = nullptr;
	SDL_GPUTexture* reflectionTexture_ = nullptr;
	bool reflection_ = false;   // which technique the recorded draws belong to
	int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float vpMinZ_ = 0.f, vpMaxZ_ = 1.f;
	bool stateValid_ = false;   // SetState has run this frame

	std::vector<DrawCmd> draws_;
};

#endif // VISTA_SDL_WATER_RENDERER_H
