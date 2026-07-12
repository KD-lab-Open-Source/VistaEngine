#ifndef VISTA_SDL_CLOUD_SHADOW_RENDERER_H
#define VISTA_SDL_CLOUD_SHADOW_RENDERER_H

// The cloud-shadow half of the SDL GPU backend: draws cCloudShadow's one world-sized quad
// with the cloudshadow pipeline (Render/SDLShaders/cloudshadow.{vert,frag}.hlsl, ported
// from the original Render/shader/NoMaterial/CloudShadow.{vsl,psl} -- the VSCloudShadow and
// PSCloudShadow classes).
//
// This does NOT draw into the view. It draws into the terrain LIGHTMAP, and that is the
// whole trick of the effect:
//
//   * cCloudShadow sets ATTRCAMERA_SHADOW on itself, which is the attribute cScene gives
//     the planar light camera (Scene.cpp: `planarCamera->setAttribute(ATTRCAMERA_SHADOW|...)`),
//     and ATTRUNKOBJ_IGNORE_NORMALCAMERA, so the main camera never sees it. cCloudShadow::Draw
//     bails on any camera without ATTRCAMERA_SHADOW.
//   * CameraPlanarLight::DrawScene runs DrawObjectFirstSorted() and then drawLights(). The
//     cloud shadow sorts first (sortIndex -1) and its blend is ALPHA_NONE, so it OVERWRITES
//     the lightmap; the world's light sources then blend over the top of it.
//   * The lightmap's neutral is mid-grey, and the fragment shader is centred on exactly
//     that. The terrain adds `2*(lightmap - 0.5)` to its light and the grass adds
//     `lightmap - 0.5`, so both pick the clouds up for free -- there is nothing to do in
//     either of their shaders.
//
// So the pass this opens goes into whatever colour target the planar camera bound, before
// SDLWorldQuadRenderer's pass for the lights, and cCloudShadow::Draw calls
// cSDLRenderDevice::drawCloudShadow() to open it at exactly the point in the scene walk the
// original drew at.
//
// It owns no geometry: the quad's vertex and index buffers are cCloudShadow's own, made
// through cSDLRenderDevice::CreateVertexBuffer/CreateIndexBuffer, and cCloudShadow::Animate
// rewrites the two scrolling texture coordinate sets into the vertex buffer every frame
// (Unlock re-uploads). All of that was already portable and already running; only the draw
// was missing.

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

class SDLCloudShadowRenderer
{
public:
	// owner is the device that holds the vertex/index buffers this renderer draws from;
	// window is needed only to query the swapchain format. Destroy before the SDL GPU device.
	SDLCloudShadowRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window);
	~SDLCloudShadowRenderer();

	SDLCloudShadowRenderer(const SDLCloudShadowRenderer&) = delete;
	SDLCloudShadowRenderer& operator=(const SDLCloudShadowRenderer&) = delete;

	// Everything cCloudShadow::Draw pushed into VSCloudShadow::Select / PSCloudShadow::Select.
	struct State
	{
		cTexture* texture = nullptr;   // the cloud texture, bound to BOTH stages
		// The original's tfactor: how hard the clouds bite. cCloudShadow::Draw builds it from
		// the tile map's diffuse (flattened to grey), the world's cloud intensity and
		// -sunDirection.z, so it goes to zero as the sun reaches the horizon.
		Color4f tfactor = Color4f(0.f, 0.f, 0.f, 0.f);
	};

	// Drop the previous frame's draws. Called from BeginScene.
	void BeginFrame();

	// Snapshot the state and the camera the draws that follow are made under.
	void SetState(const State& state, Camera* camera);

	// Record the quad, standing in for the D3D call of the same name that cCloudShadow::Draw
	// issued. The buffers are cCloudShadow's; the device resolves them.
	void DrawIndexedPrimitive(sPtrVertexBuffer& vb, const sPtrIndexBuffer& ib, int nPolygon);

	bool hasDraws() const { return !draws_.empty(); }
	// Throw the pending draws away: the target they were recorded under cannot be rendered
	// into, and they must not replay into the next one.
	void DiscardDraws() { draws_.clear(); }

	// Replay the pending draws into one colour+depth pass, and clear them. `clear`/
	// `clearDepth` mean this pass owns the target's clears -- which it normally does, being
	// the first thing the planar light camera draws. Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int targetW, int targetH, bool clear, const float clearColor[4],
	          bool clearDepth, bool wireframe);

private:
	struct VSUniform { float mvp[16]; };
	// PSCloudShadow's two constants, kept separate rather than derived in the shader so the
	// arithmetic stays the original's: tfactorm05 == 0.5 - 0.75*tfactor.
	struct FSUniform { float tfactor[4]; float tfactorM05[4]; };

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
		// after cCloudShadow::Draw returns.
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

	// sampler_wrap_linear, which cCloudShadow::Draw asks for on both stages: the scroll runs
	// the coordinates off the edge of the texture, so they must wrap.
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

#endif // VISTA_SDL_CLOUD_SHADOW_RENDERER_H
