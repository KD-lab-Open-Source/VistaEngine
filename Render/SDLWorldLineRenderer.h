#ifndef VISTA_SDL_WORLD_LINE_RENDERER_H
#define VISTA_SDL_WORLD_LINE_RENDERER_H

// The world-line half of the SDL GPU backend: draws 3D line segments in world
// space with the worldline pipeline (Render/SDLShaders/worldline.{vert,frag}.hlsl).
//
// It is the SDL stand-in for cD3DRender's line path — DrawLine(const Vect3f&, ...)
// records into `lines3d` and FlushLine3D (D3DRender.cpp:1046) replays them as a
// PT_LINELIST under SetWorldMaterial(ALPHA_BLEND, MatXf::ID) — the world-space
// vertices multiplied by the camera's view-projection, alpha-blended with a
// LESS-EQUAL z-test.
//
// Callers reach it through the device's own 3D entry point:
//
//   EngineViewport::drawGrid   (the editor's terrain grid — SurMap5Qt)
//
// The game's legacy DrawLine callers (the D3D-only line helpers) are dead on
// the SDL backend, so the editor grid is the one user today; the renderer is
// still general: it records any number of segments per frame and replays them
// in one pass at EndScene, before the UI.
//
// One pipeline, one vertex layout (sVertexXYZD: position + diffuse), one
// uniform (the camera's view-projection). Depth-tested (LESS-EQUAL) against
// the frame's depth target, alpha-blended, writing no depth — exactly the
// D3D fixed-function line behaviour.

#include "XMath/Mat4f.h"      // Mat4f: the camera's view-projection
#include <vector>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUShader;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

class Camera;

// The line vertices. Mirrors sVertexXYZD (position + diffuse) so the engine's
// callers can hand us the same data the D3D path packed; stride 16.
struct WorldLineVertex
{
	float pos[3];
	unsigned int color;   // Color4c: 0xAABBGGRR as the engine stores it
};

class SDLWorldLineRenderer
{
public:
	// Builds the pipeline on first draw (window is needed only to query the
	// swapchain format). Must be destroyed before the SDL GPU device.
	SDLWorldLineRenderer(SDL_GPUDevice* device, SDL_Window* window);
	~SDLWorldLineRenderer();

	SDLWorldLineRenderer(const SDLWorldLineRenderer&) = delete;
	SDLWorldLineRenderer& operator=(const SDLWorldLineRenderer&) = delete;

	// Drop the previous frame's segments. Called from BeginScene.
	void BeginFrame();

	// Snapshot the camera's view-projection and viewport, before the segments
	// that follow. The segments are replayed in a pass opened at EndScene, by
	// which time the scene walk has moved on.
	void SetCamera(Camera* camera);

	// Record one world-space segment (DrawLine's v1/v2 pair). Colors arrive as
	// Color4c (0xAABBGGRR).
	void DrawLine(const float* v1, const float* v2, unsigned int color);

	bool hasDraws() const { return !vertices_.empty(); }

	// Replay the recorded segments into one colour+depth render pass, blended
	// over the scene and writing no depth, z-tested LESS-EQUAL against the
	// depth target. `clear`/`clearDepth` mean this pass owns the frame's
	// colour/depth clear -- true only when no earlier pass took it (the line
	// pass runs at EndScene, after the object flush, so it is normally false).
	// Clears the recorded segments, so a later call replays only its own.
	// Returns true if the pass ran.
	bool Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
	          int screenW, int screenH,
	          bool clear, const float clearColor[4], bool clearDepth);

private:
	// Build the pipeline (vertex + fragment shaders, one uniform block). Returns
	// false if a shader or the pipeline could not be created; a failure is
	// sticky (shadersTried_), so the grid degrades to invisible rather than
	// erroring every frame.
	bool ensurePipeline();
	void ensureVertexCapacity(int verts);

	SDL_GPUDevice*          device_ = nullptr;
	SDL_Window*             window_ = nullptr;
	SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
	SDL_GPUShader*          vs_ = nullptr;
	SDL_GPUShader*          fs_ = nullptr;
	SDL_GPUBuffer*          vertexBuffer_ = nullptr;
	SDL_GPUTransferBuffer*  transferBuffer_ = nullptr;
	int                     vertexCapacity_ = 0;

	// The camera's view-projection + viewport, snapshot by SetCamera.
	Mat4f                    mvp_ = Mat4f::ID;
	int                      vpX_ = 0, vpY_ = 0, vpW_ = 0, vpH_ = 0;
	float                    vpMinZ_ = 0.f, vpMaxZ_ = 1.f;

	// The frame's segments, in record order (2 vertices per line).
	std::vector<WorldLineVertex> vertices_;

	bool shadersTried_ = false;
	bool pipelineReady_ = false;
};

#endif // VISTA_SDL_WORLD_LINE_RENDERER_H
