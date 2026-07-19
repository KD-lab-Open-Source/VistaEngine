#ifndef VISTA_SDL_ENVIRONMENT_EARTH_RENDERER_H
#define VISTA_SDL_ENVIRONMENT_EARTH_RENDERER_H

// The environment-earth half of the SDL GPU backend: draws cEnvironmentEarth's ground plane --
// the terrain-coloured skirt that fills the world beyond the map edge, out under the horizon
// fog ring -- with the environmentearth pipeline (Render/SDLShaders/environmentearth.{vert,
// frag}.hlsl, ported from vsStandart + Render/shader/NoMaterial/EnvironmentEarth.psl).
//
// Unlike the fog ring and the field dome, this is an OPAQUE occluder, not a blended overlay, so
// it does not ride SDLWorldQuadRenderer (which never writes depth). It draws where
// cEnvironmentEarth sits in the walk -- SCENENODE_OBJECTFIRST, which Camera::DrawScene reaches
// BEFORE the terrain -- so it takes the depth clear, writes its depth, and the terrain then
// draws over it wherever the map is. Its own pipeline is therefore plain opaque geometry:
// ALPHA_NONE, depth test and write on, as the original's SetWorldMaterial(ALPHA_NONE) asked.
//
// It owns no geometry. The vertex and index buffers are cEnvironmentEarth's own, created
// through cSDLRenderDevice::CreateVertexBuffer/CreateIndexBuffer in its constructor and filled
// there and in SetTexture -- all portable, all already running against an empty Draw(). The
// device stays their owner; this renderer asks it to resolve an sPtr wrapper to the
// SDL_GPUBuffer behind it, exactly as the grass and cloud-shadow renderers do.
//
// One draw per frame, one texture, one tfactor tint. cEnvironmentEarth::Draw skips the
// reflection camera itself, so this never records for it.

#include "IRenderDevice.h"    // Color4f, cTexture, sPtrVertexBuffer, sPtrIndexBuffer
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUShader;
struct SDL_GPUBuffer;

class Camera;
class cSDLRenderDevice;

class SDLEnvironmentEarthRenderer
{
public:
	// owner is the device that holds the vertex/index buffers this renderer draws from;
	// window is needed only to query the swapchain format. Destroy before the SDL GPU device.
	SDLEnvironmentEarthRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window);
	~SDLEnvironmentEarthRenderer();

	SDLEnvironmentEarthRenderer(const SDLEnvironmentEarthRenderer&) = delete;
	SDLEnvironmentEarthRenderer& operator=(const SDLEnvironmentEarthRenderer&) = delete;

	// Everything cEnvironmentEarth::Draw pushed into vsStandart / psEnvironmentEarth.
	struct State
	{
		cTexture* texture = nullptr;   // the ground texture (cEnvironmentEarth's Texture)
		// The original's tfactor: the tile map's diffuse folded with the sun angle and the
		// object colour. cEnvironmentEarth::Draw builds it, as it built the D3D constant.
		Color4f tfactor = Color4f(1.f, 1.f, 1.f, 1.f);
	};

	// Drop the previous frame's draws. Called from BeginScene.
	void BeginFrame();

	// Snapshot the state and the camera the draw that follows is made under.
	void SetState(const State& state, Camera* camera);

	// Record the ground plane, standing in for the D3D call of the same name cEnvironmentEarth::
	// Draw issued. The buffers are cEnvironmentEarth's; the device resolves them.
	void DrawIndexedPrimitive(sPtrVertexBuffer& vb, const sPtrIndexBuffer& ib, int nPolygon);

	bool hasDraws() const { return !draws_.empty(); }
	// Throw the pending draws away: the target they were recorded under cannot be rendered
	// into, and they must not replay into the next one.
	void DiscardDraws() { draws_.clear(); }

	// Replay the pending draws into one colour+depth pass, and clear them. `clear`/`clearDepth`
	// mean this pass owns the target's clears -- clearDepth is true here, since the earth draws
	// first (after the camera's ClearZBuffer) and the terrain draws over its depth. Returns
	// true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int targetW, int targetH, bool clear, const float clearColor[4], bool clearDepth,
	          bool wireframe);

private:
	struct VSUniform { float mvp[16]; };
	struct FSUniform { float tfactor[4]; };

	struct DrawCmd
	{
		VSUniform vs;
		FSUniform fs;
		SDL_GPUTexture* texture;    // null -> the 1x1 white stand-in
		SDL_GPUBuffer* vertexBuffer;
		SDL_GPUBuffer* indexBuffer;
		int stride;
		int indexCount;
		// The camera's viewport, captured at record time: the draw replays in a pass opened
		// after cEnvironmentEarth::Draw returns, by which time the walk has moved on.
		int vpX, vpY, vpW, vpH;
		float vpMinZ, vpMaxZ;
	};

	bool createShaders();
	void createSampler();
	SDL_GPUGraphicsPipeline* pipelineFor(int stride, bool wireframe);

	cSDLRenderDevice* owner_ = nullptr;
	SDL_GPUDevice* device_ = nullptr;
	SDL_Window*    window_ = nullptr;

	SDL_GPUShader* vs_ = nullptr;
	SDL_GPUShader* fs_ = nullptr;
	bool shadersTried_ = false;
	std::vector<std::pair<unsigned, SDL_GPUGraphicsPipeline*>> pipelines_;

	// sampler_wrap_anisotropic, which cEnvironmentEarth::Draw sets on stage 0: the ground
	// texture tiles across the plane, so the coordinates run well past 1 and must wrap.
	SDL_GPUSampler* samplerWrap_ = nullptr;
	SDL_GPUTexture* whiteTexture_ = nullptr;

	std::vector<DrawCmd> draws_;

	VSUniform vs_current_ = {};
	FSUniform fs_current_ = {};
	SDL_GPUTexture* texture_ = nullptr;
	int vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float vpMinZ_ = 0.f, vpMaxZ_ = 1.f;
	bool currentValid_ = false;
};

#endif // VISTA_SDL_ENVIRONMENT_EARTH_RENDERER_H
