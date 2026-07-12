#ifndef VISTA_SDL_RENDER_DEVICE_H
#define VISTA_SDL_RENDER_DEVICE_H

// Cross-platform render device (SDL GPU backend), replacing the Windows-only
// D3D9 cD3DRender.
//
// The legacy cInterfaceRenderDevice is an immediate-mode/synchronous D3D9 API;
// SDL GPU is explicit/async (command buffers + render passes). We bridge by
// recording during BeginScene..EndScene and submitting one command buffer at
// Flush.
//
// The device owns what the whole backend shares — the SDL GPU device, the window
// and swapchain, the frame's command buffer, the render targets, textures and
// vertex/index buffers — but no drawing pipeline of its own. Drawing lives in renderer
// classes that record their own passes into the frame's command buffer: SDLUIRenderer
// (2D text, sprites, quads), SDLTileMapRenderer (terrain), SDLObject3dxRenderer
// (skinned .3dx meshes), SDLWaterRenderer (the water surface) and SDLWorldQuadRenderer
// (world-space textured quads: the sun and moon, the shoreline foam, the wave sources).
//
// Render targets follow the camera, as they do on D3D: cD3DRender::setCamera binds
// camera->GetRenderTarget() and clears it, or restores the back buffer when the camera
// has none. setCamera below is the same choke point -- see RenderTarget.

#include "IRenderDevice.h"
#include "MTSection.h"
#include "XMath/Mat4f.h"   // Mat4f (the light's view-projection and its bias)
#include <vector>
#include <unordered_map>
#include <memory>

// SDL opaque handles, forward-declared so SDL stays out of engine headers.
struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUTexture;
struct SDL_GPUBuffer;
struct SDL_GPURenderPass;

struct sViewPort;
class SDLUIRenderer;
class SDLTileMapRenderer;
class SDLObject3dxRenderer;
class SDLWaterRenderer;
class SDLWorldQuadRenderer;
class SDLMinimapRenderer;
class SDLGrassRenderer;
class SDLCloudShadowRenderer;
class cTileMap;

// Restrict drawing to a camera's viewport, the way cD3DRender::SetDrawTransform hands
// camera->vp to D3DDevice_->SetViewport. Camera::Update scales matProj to exactly that
// rect, so a renderer that ignores it stretches the scene over the whole window and
// loses the letterbox bars the 4:3 work area leaves. A render pass starts out with the
// full target as its viewport, so this only ever needs setting, never restoring.
void applyCameraViewport(SDL_GPURenderPass* pass, const sViewPort& vp, int targetW, int targetH);

// A 1x1 RGBA texture of one colour, uploaded on a command buffer of its own. Every renderer
// needs a couple: SDL requires a sampler binding to be non-null even where the shader gates
// the layer off, so a solid stand-in goes in the slot -- white where the sample must be
// neutral to a multiply, mid-grey where it must be neutral to the terrain's `detail - 0.5`.
SDL_GPUTexture* createSolidGPUTexture(SDL_GPUDevice* device, unsigned int rgba);

// gb_RenderDevice as a cSDLRenderDevice, or null under any other device. Engine code that
// needs the backend itself (cScene's shadow map, CameraShadowMap) asks for it through this.
class cSDLRenderDevice;
cSDLRenderDevice* sdlRenderDevice();

// The SDL backend's UI renderer, or null under any other device. Every 2D entry point of
// the device already forwards here; this is for the 2D callers that reach past those for
// the device's shared quad buffer (UI_LogicDispatcher::drawSelection) -- it answers to the
// same BeginDraw/Get/EndDraw.
SDLUIRenderer* sdlUIRenderer();

// The SDL backend's 3dx renderer, or null under any other device. cObject3dx::Draw and
// cSimply3dx::SelectMaterial drive it exactly as they drive pShader3dx's shader objects
// on Windows.
SDLObject3dxRenderer* sdlObjectRenderer();

// The SDL backend's water renderer, or null under any other device. cWater::Draw drives
// it exactly as it drives VSWater/PSWater on Windows.
SDLWaterRenderer* sdlWaterRenderer();

// The SDL backend's world-quad renderer, or null under any other device. cCoastSprites,
// cFixedWaves and cWaves drive it exactly as they drive the device's shared quad buffer
// on Windows -- it even answers to the same BeginDraw/Get/EndDraw.
SDLWorldQuadRenderer* sdlWorldQuadRenderer();

// The SDL backend's minimap renderer, or null under any other device. UI_Minimap's draw
// half drives it exactly as it drives psMiniMap / psMiniMapBorder on Windows. It draws
// inside the UI renderer's pass -- see SDLMinimapRenderer.h.
SDLMinimapRenderer* sdlMinimapRenderer();

// The SDL backend's grass renderer, or null under any other device. GrassMap::DrawGrass
// drives it exactly as it drives VSGrass / PSGrass on Windows.
SDLGrassRenderer* sdlGrassRenderer();

// The SDL backend's cloud-shadow renderer, or null under any other device. cCloudShadow::Draw
// drives it exactly as it drives VSCloudShadow / PSCloudShadow on Windows.
SDLCloudShadowRenderer* sdlCloudShadowRenderer();

class cSDLRenderDevice : public cInterfaceRenderDevice
{
public:
	cSDLRenderDevice();
	~cSDLRenderDevice();

	// --- Terrain ----------------------------------------------------------
	// Called from cTileMap::Draw, i.e. from inside the engine's own scene-draw
	// delegation, part-way through BeginScene..EndScene. Forwards the frame's command
	// buffer and swapchain image to SDLTileMapRenderer, which opens its own colour +
	// depth pass there and then. The first pass of a frame performs Fill()'s clear, so
	// if the terrain pass runs, the UI pass at EndScene loads instead of clearing.
	void drawTileMap(cTileMap* tileMap, Camera* camera);

	// --- Water and world quads --------------------------------------------
	// cSunMoonObj::Draw, cWater::Draw, cCoastSprites::Draw and cFixedWavesContainer::Draw
	// talk to their renderers directly (the way they talk to VSWater/PSWater and the shared
	// quad buffer on Windows), then call these to put what they recorded on the screen.
	//
	// Only the sun opens the frame, before the terrain: it is the first thing the sky
	// camera draws, and the sky is drawn before the world. The rest belong where the scene
	// walk reached them, over the opaque objects. Water and the coast sprites draw in
	// DrawObjectSpecial, which sorts by sortIndex(), so the water (-2) lands under the
	// sprites (0); the wave sources draw later still, in DrawSortObject's sorted transparent
	// pass. The object renderer batches, so each of these first replays what it has recorded
	// so far and only then opens its own pass over it. Whatever the walk records afterwards
	// replays at EndScene, on top, as on D3D.
	//
	// drawWorldQuads is called by each of its renderer's callers in turn, and each call
	// draws (and clears) only the quads recorded since the last one.
	SDLWaterRenderer* waterRenderer() { return waterRenderer_.get(); }
	void drawWater();
	SDLWorldQuadRenderer* worldQuadRenderer() { return worldQuadRenderer_.get(); }
	void drawWorldQuads();

	// --- Grass ---------------------------------------------------------------
	// GrassMap::DrawGrass records its tiles into the grass renderer, then calls this. Same
	// contract as the water: it lands where Camera::DrawScene reached it, over the terrain
	// and the tilemap objects (hence the object flush inside), under everything DrawObject
	// records afterwards.
	SDLGrassRenderer* grassRenderer() { return grassRenderer_.get(); }
	void drawGrass();

	// --- Cloud shadows -------------------------------------------------------
	// cCloudShadow::Draw records its one quad and calls this. It runs under the planar light
	// camera, so the pass lands in the LIGHTMAP -- before SDLWorldQuadRenderer's pass for the
	// light sources, which blend over it. See SDLCloudShadowRenderer.h.
	SDLCloudShadowRenderer* cloudShadowRenderer() { return cloudShadowRenderer_.get(); }
	void drawCloudShadow();

	// --- UI and minimap -----------------------------------------------------
	// Neither has a draw call of its own. The UI renderer's pass runs at EndScene, over
	// everything; the minimap is a UI control, so its draws are sequenced into that
	// renderer's run list and replayed inside its pass. See SDLMinimapRenderer.h.
	SDLUIRenderer* uiRenderer() { return uiRenderer_.get(); }
	SDLMinimapRenderer* minimapRenderer() { return minimapRenderer_.get(); }

	// Replay the object batch recorded so far into the current target, and take that
	// target's clears if they are still going. drawWater / drawWorldQuads call it to get
	// the opaque objects onto the screen before they blend over them; cSkyCamera::DrawScene
	// calls it to get the sky models onto the screen before the world scene draws over them.
	void flushObjectPass();

	// Camera::ClearZBuffer, which the main camera runs for ATTRCAMERA_CLEARZBUFFER: the sky
	// draws first and leaves its own depth behind, in a frustum of its own (1e3..1e5). There
	// is no clear outside a render pass here, so instead let the next pass own the depth
	// clear again, exactly as if nothing had been drawn.
	void clearZBuffer() { current_->depthCleared = false; }

	// --- Shadow map -------------------------------------------------------
	// Mirrors cD3DRender: cScene creates the map, the light camera renders the casters
	// into it, and receivers transform by shadowMatViewProj() * shadowMatBias().
	//
	// It is a plain depth texture here. D3D9 could not sample depth, so the original
	// renders the light-space z into a float colour target (object_shadow.psl's
	// `return (float4)v.tdepth`) and forks the whole path on DT_RADEON9700 vs
	// DT_GEFORCEFX. SDL GPU samples depth directly, so neither is needed.
	//
	// The map is not special-cased as a target: cScene::AddLightCamera hands it to the
	// light camera through Camera::SetRenderTarget, and setCamera resolves it like any
	// other -- to a depth-only RenderTarget, because its cTexture carries
	// TEXTURE_RENDER_SHADOW_9700.
	bool createShadowMap(int size);
	void deleteShadowMap();
	cTexture* GetShadowMap() { return shadowMap_; }
	int  GetShadowMapSize() const { return shadowMapSize_; }

	// --- Terrain lightmap ---------------------------------------------------
	// The 256x256 colour target CameraPlanarLight draws the scene's light sources and
	// circle shadows into, top-down, once a frame; the terrain shader multiplies its light
	// term by it (tile_map_scene.psl: `light += 2*(lightmap.rgb-0.5)`), so the clear colour
	// 128,128,128 is the neutral "no light source here". Named GetLightMap() to match
	// cD3DRender, which cScene::AddPlanarCamera reads through.
	//
	// Not special-cased as a target either: AddPlanarCamera hands it to the light camera
	// through Camera::SetRenderTarget, and setCamera resolves it like any other colour one.
	bool createLightMap(int size);
	void deleteLightMap();
	cTexture* GetLightMap() { return lightMap_; }
	// The original's planarTransform_ (cD3DRender::setPlanarTransform), which the terrain
	// vertex shader reads as fPlanarNode: xy is the lightmap box's world-space origin, zw
	// its inverse extent, so uv = (pos.xy - xy) * zw. cScene::AddPlanarCamera sets it.
	void setPlanarTransform(const Vect4f& transform) { planarTransform_ = transform; }
	const Vect4f& planarTransform() const { return planarTransform_; }
	void SetShadowMatViewProj(const Mat4f& m) { shadowMatViewProj_ = m; }
	const Mat4f& shadowMatViewProj() const { return shadowMatViewProj_; }

	// --- Distance fog --------------------------------------------------------
	// Environment::graphQuant sets the colour and the near/far range each frame from the
	// time of day; a negative range turns fog off, exactly as cD3DRender::SetGlobalFog read
	// it. Code that must not be fogged (the sky scene, the 2D pass) toggles RS_FOGENABLE
	// around itself, as it always did.
	//
	// D3D9 did this in fixed function -- D3DRS_FOGTABLEMODE = D3DFOG_LINEAR, i.e. per-pixel
	// fog over eye depth, applied to the shader's output colour before blending. That is why
	// the terrain and object shaders have no fog term of their own to port: the rasterizer
	// fogged them. (The `oFog` some .vsl files do write was the vertex-fog fallback, for the
	// cards that had no table fog. Same linear formula, same constants.)
	//
	// There is no fixed-function fog here, so every world fragment shader ends with
	//     rgb = lerp(FogColor, rgb, saturate(fog))
	// and its vertex shader gets the fog factor from ONE float4:
	//
	//     fog = dot(float4(worldPos, 1), fogPlane(camera))
	//
	// The factor is linear in view-space z -- (end - z)/(end - start), the D3D formula
	// verbatim -- so it collapses into a plane equation, and interpolating it across a
	// triangle is the same thing as evaluating it per pixel. When fog is off the plane is
	// (0,0,0,1): fog == 1, and the lerp above is exactly the identity. No shader branch,
	// no pipeline variant.
	void SetGlobalFog(const Color4f& color, const Vect2f& range) override;
	const Color4f& fogColor() const { return fogColor_; }
	Vect4f fogPlane(Camera* camera) const;
	// Light clip space -> shadow map texture coordinates.
	Mat4f shadowMatBias() const;
	// The terrain caster, from cTileMap::Draw under the light camera -- where the D3D
	// backend calls tileMapRender_->DrawBump(camera, ALPHA_TEST, true, false). Draws
	// immediately into the current (depth-only) target, so it lands before the object
	// casters, which then load its depth. It needs no cTileMap: the caster mesh is the one
	// SDLTileMapRenderer built from vMap.
	void drawTileMapShadow(Camera* camera);
	// Record the depth pass the light camera accumulated, where the D3D backend calls
	// DrawType::EndDrawShadow. Switching away from the map would flush it anyway; this
	// only pins the moment, as the original does.
	void endShadowPass() { flushTarget(current_, true); }
	// True once a caster pass has filled the map this frame. Receivers must check it:
	// cScene detaches the light camera whenever shadows are off, and the map outlives it.
	bool shadowPassRan() const { return shadowPassRan_; }

	// --- 3dx objects ------------------------------------------------------
	// cObject3dx::Draw talks to the object renderer directly (the way it talks to
	// gb_RenderDevice3D's shaders on Windows); the device only hands it over, and
	// resolves the engine's buffer handles to the SDL GPU buffers it owns.
	SDLObject3dxRenderer* objectRenderer() { return objectRenderer_.get(); }
	SDL_GPUBuffer* gpuBuffer(const sPtrVertexBuffer& vb) const;
	SDL_GPUBuffer* gpuBuffer(const sPtrIndexBuffer& ib) const;
	// RS_ZWRITEENABLE, as the scene passes set it around the transparent draw.
	bool zWriteEnable() const { return zWriteEnable_; }

	// --- Lifecycle (real) -------------------------------------------------
	bool Initialize(int xScr, int yScr, int mode, HWND hWnd, int RefreshRateInHz, HWND fallbackWindow) override;
	bool inited() const override { return device_ != nullptr; }
	int  Done() override;
	bool ChangeSize(int xScr_, int yScr_, int mode) override;
	bool RecalculateDeviceSize() override { return false; }
	void RestoreDeviceForce() override {}

	void  SetMultisample(DWORD multisample) override { multisample_ = multisample; }
	DWORD GetMultisample() override { return multisample_; }

	int GetSizeX() override { return xScr; }
	int GetSizeY() override { return yScr; }
	Vect2i GetOriginalScreenSize() override { return Vect2i(xScr, yScr); }
	bool IsFullScreen() override { return false; }
	int GetAvailableTextureMem() override { return 0; }
	// The OS window handle, not the SDL_Window: callers hand this to DirectSound,
	// DirectInput and the kdw dialogs, which all want a real HWND on Windows.
	HWND GetWindowHandle() override;

	// --- Textures: real SDL GPU textures + CPU staging ----------------------
	int   CreateTexture(cTexture* Texture, cFileImage* FileImage, int dxout, int dyout, bool enable_assert) override;
	int   DeleteTexture(cTexture* Texture) override;
	void* LockTexture(cTexture* Texture, int& Pitch) override;
	void* LockTexture(cTexture* Texture, int& Pitch, Vect2i lock_min, Vect2i lock_size) override;
	void  UnlockTexture(cTexture* Texture) override;
	MTSection& resetDeviceLock() override { return resetDeviceLock_; }

	int SetClipRect(int xmin, int ymin, int xmax, int ymax) override
		{ xScrMin = xmin; yScrMin = ymin; xScrMax = xmax; yScrMax = ymax; return 0; }
	int GetClipRect(int* xmin, int* ymin, int* xmax, int* ymax) override
		{ if(xmin)*xmin=xScrMin; if(ymin)*ymin=yScrMin; if(xmax)*xmax=xScrMax; if(ymax)*ymax=yScrMax; return 0; }

	// --- Frame (real) -----------------------------------------------------
	bool IsInBeginEndScene() override { return bActiveScene_; }
	int  Fill(int r, int g, int b, int a) override;
	int  BeginScene() override;
	int  EndScene() override;
	int  Flush() override;

	// --- Render windows (no-op: single-window) ---------------------------
	cRenderWindow* createRenderWindow(HWND) override { return nullptr; }
	void selectRenderWindow(cRenderWindow*) override {}
	void setGlobalRenderWindow(cRenderWindow*) override {}
	cRenderWindow* currentRenderWindow() override { return nullptr; }
	void DeleteRenderWindow(cRenderWindow*) override {}

	// --- Camera / transform ----------------------------------------------
	// SDL GPU has no device-wide transform or viewport: matView/matProj reach the
	// shaders as uniforms, and the viewport belongs to a render pass. So SetDrawTransform
	// only caches the camera, as cD3DRender's does, and the renderers apply camera->vp when
	// they open their pass (see applyCameraViewport).
	//
	// setCamera additionally binds the camera's render target, exactly where
	// cD3DRender::setCamera calls SetRenderTarget / RestoreRenderTarget. Every camera
	// passes through here before its own draws (Camera::DrawScene, CameraShadowMap::DrawScene
	// and cSkyCamera::DrawScene all call it), and child cameras draw before their parent --
	// so switching targets here is what gets an offscreen pass recorded, complete, ahead of
	// the pass that samples it.
	void SetDrawTransform(Camera* camera) override { camera_ = camera; }
	void setCamera(Camera* camera) override;
	void setWorldMatrix(const MatXf&) override {}

	// --- Misc state (no-op) ----------------------------------------------
	// Only RS_FILLMODE (GameShell drives it from debugWireFrame) and RS_ZWRITEENABLE
	// (Camera::DrawSortObject turns it off for the transparent pass) are honoured; the
	// rest of the D3D render states have no SDL GPU equivalent outside a pipeline object.
	void SetRenderState(eRenderStateOption, int) override;
	unsigned int GetRenderState(eRenderStateOption) override;
	// Sticky sampler state, as it is on D3D, where this sets one global the scene and the UI
	// both draw with. Each renderer bakes its own sampler for its own geometry; only the UI
	// takes this one, because only its callers change it (the selection frame asks for wrap
	// so its centre tiles). Stages past 0 have no 2D caller.
	void SetSamplerDataVirtual(DWORD stage, SAMPLER_DATA& data) override;
	// D3D: "the advanced DrawType exists", i.e. the device can render a shadow map.
	// cVisGeneric::SetShadowType turns shadows off without it. We always can.
	bool IsEnableSelfShadow() override { return true; }
	// TODO(sdl-port): screenshots and gamma. See Render/PORTING.md #21.
	int  SetGamma(float, float, float) override { return 0; }
	bool SetScreenShot(const char*) override { return false; }

	// --- 2D primitives (forwarded to the UI renderer) --------------------
	void DrawLine(int x1, int y1, int x2, int y2, Color4c color) override;
	void DrawPixel(int x, int y, Color4c color) override;
	void DrawRectangle(int x, int y, int dx, int dy, Color4c color, bool outline) override;
	// Nothing to flush: the UI renderer already records these in call order alongside
	// the sprites and text, and draws them all in its pass at EndScene.
	void FlushPrimitive2D() override {}

	// --- 3D primitives (no-op) -------------------------------------------
	// TODO(sdl-port): the 3D debug primitives draw nothing. See Render/PORTING.md #17.
	void DrawLine(const Vect3f&, const Vect3f&, Color4c) override {}
	void DrawPoint(const Vect3f&, Color4c) override {}
	void FlushPrimitive3D() override {}
	void FlushLine3D(bool, bool) override {}
	void drawCircle(const Vect3f&, float, Color4c) override {}
	void DrawBound(const MatXf&, Vect3f&, Vect3f&, bool, Color4c) override {}

	// --- Text (forwarded to the UI renderer) ------------------------------
	void OutText(int x, int y, const char* text, const Color4f& color, ALIGN_TEXT align, eBlendMode blend_mode, Vect2f scale) override;
	int  OutTextLine(int x, int y, const FT::Font& font, const wchar_t* textline, const wchar_t* end, const Color4c& color, eBlendMode blend_mode, int xRangeMin, int xRangeMax) override;
	// The D3D backend draws these straight onto the window's HDC with GDI (CreateFont /
	// ExtTextOut), bypassing the renderer entirely. There is no equivalent here, and
	// nothing in the game calls either.
	void OutText(int, int, const char*, int, int, int) override {}
	void OutText(int, int, const char*, int, int, int, char*, int, int, int, int) override {}

	// --- Sprites (forwarded to the UI renderer) ---------------------------
	void DrawQuad(float, float, float, float, float, float, float, float, Color4c) override;
	void DrawSprite(int, int, int, int, float, float, float, float, cTexture*, const Color4c&, float, eBlendMode, float) override;
	// TODO(sdl-port): unimplemented, and currently unreached -- the solid, two-texture and
	// cTextureScale sprite variants are used only by the chaos post-process
	// (Render/src/CChaos.cpp), which has no SDL path. See Render/PORTING.md #18.
	void DrawSpriteSolid(int, int, int, int, float, float, float, float, cTexture*, const Color4c&, float, eBlendMode) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, cTexture*, cTexture*, const Color4c&, float) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, float, float, float, float, cTexture*, cTexture*, const Color4c&, float, eColorMode, eBlendMode) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, float, float, float, float, cTexture*, cTexture*, float, float, float, eColorMode, eBlendMode) override {}
	void DrawSpriteScale(int, int, int, int, float, float, cTextureScale*, const Color4c&, float, eBlendMode) override {}
	void DrawSpriteScale2(int, int, int, int, float, float, cTextureScale*, cTextureScale*, const Color4c&, float) override {}
	void DrawSpriteScale2(int, int, int, int, float, float, float, float, cTextureScale*, cTextureScale*, const Color4c&, float, eColorMode) override {}

	// --- Materials -------------------------------------------------------
	// Records the current texture for the following DrawQuad calls.
	void SetNoMaterial(eBlendMode, const MatXf&, float, cTexture*, cTexture*, eColorMode) override;
	void SetWorldMaterial(eBlendMode, const MatXf&, float, cTexture*, cTexture*, eColorMode, bool, bool) override {}

	// --- Vertex/index buffers: real SDL GPU static buffers ----------------
	// The buffers live here (the engine's 3dx/terrain code fills them through the sPtr
	// wrappers), but the device draws nothing: SDL GPU draws only inside a render pass,
	// which is a renderer's business. cObject3dx::Draw calls the object renderer's
	// DrawIndexedPrimitive instead of this one.
	void DrawIndexedPrimitive(sPtrVertexBuffer&, int, int, const sPtrIndexBuffer&, int, int) override {}
	void CreateVertexBuffer(sPtrVertexBuffer&, int, IDirect3DVertexDeclaration9*, int) override;
	void DeleteVertexBuffer(sPtrVertexBuffer&) override;
	void* LockVertexBuffer(sPtrVertexBuffer&, bool) override;
	void UnlockVertexBuffer(sPtrVertexBuffer&) override;
	void CreateIndexBuffer(sPtrIndexBuffer&, int, int = sizeof(sPolygon)) override;
	void DeleteIndexBuffer(sPtrIndexBuffer&) override;
	sPolygon* LockIndexBuffer(sPtrIndexBuffer&, bool) override;
	void UnlockIndexBuffer(sPtrIndexBuffer&) override;

	// --- Internal shared dynamic buffers (no-op: none yet) ---------------
	// TODO(sdl-port): the dynamic vertex/quad buffer family. Every one of these returns null,
	// so any caller that was not rerouted through SDLWorldQuadRenderer draws nothing. This is
	// what grass, the field dome and the lens flare need first. See Render/PORTING.md #15.
	cVertexBuffer<sVertexXYZDT1>*  GetBufferXYZDT1() override { return nullptr; }
	cVertexBuffer<sVertexXYZD>*    GetBufferXYZD() override { return nullptr; }
	cVertexBuffer<sVertexXYZWD>*   GetBufferXYZWD() override { return nullptr; }
	cQuadBuffer<sVertexXYZD>*      GetQuadBufferXYZD() override { return nullptr; }
	cQuadBuffer<sVertexXYZDT1>*    GetQuadBufferXYZDT1() override { return nullptr; }
	cVertexBuffer<sVertexXYZDT2>*  GetBufferXYZDT2() override { return nullptr; }
	cVertexBuffer<sVertexXYZWDT1>* GetBufferXYZWDT1() override { return nullptr; }
	cQuadBuffer<sVertexXYZWDT1>*   GetQuadBufferXYZWDT1() override { return nullptr; }
	cVertexBuffer<sVertexXYZWDT2>* GetBufferXYZWDT2() override { return nullptr; }

private:
	// --- Render targets ---------------------------------------------------
	// Where a pass draws, and how much of the clear it still owes. SDL GPU cannot clear
	// outside a render pass, so a clear is *armed* here and consumed by whichever pass
	// opens on the target first -- the bookkeeping the screen already carried, now per
	// target. A null colour attachment means depth-only: that is the shadow map.
	struct RenderTarget
	{
		SDL_GPUTexture* color = nullptr;
		SDL_GPUTexture* depth = nullptr;
		int   w = 0, h = 0;
		float clearColor[4] = {0.f, 0.f, 0.f, 1.f};
		bool  clearPending  = false;   // a clear is owed...
		bool  colorCleared  = false;   // ...and some pass has already taken it
		bool  depthCleared  = false;
		bool  depthOnly     = false;   // no colour attachment: the shadow map
		bool  ownsDepth     = false;   // offscreen colour targets carry their own depth
		// Renderable at all? A camera can name a target whose texture is not resident yet.
		bool  usable() const { return depth && (depthOnly || color); }
	};

	RenderTarget screen_;       // the swapchain image + depthTexture_
	// A camera whose render target cannot be resolved (its texture is not created yet).
	// Colour and depth stay null, so every pass skips rather than drawing to the screen.
	RenderTarget nullTarget_;
	// Offscreen targets, keyed by the cTexture the camera was handed. Node-based, so the
	// RenderTarget* held in current_ survives a rehash.
	std::unordered_map<cTexture*, RenderTarget> targets_;
	RenderTarget* current_ = &screen_;

	// The target `camera` draws into, created on first use: depth-only for a
	// TEXTURE_RENDER_SHADOW_9700 texture, colour plus an owned depth buffer otherwise.
	RenderTarget* resolveTarget(Camera* camera);
	// Arm rt's clear from the camera, as cD3DRender::setCamera's Clear() does.
	void armClear(RenderTarget* rt, Camera* camera);
	// Replay whatever the object renderer has recorded into rt. `settle` means rt must come
	// out of this call fully initialized -- we are leaving it, and something is about to
	// sample it, so an offscreen target that nothing drew into still owes its clear. Mid-
	// target flushes (flushObjectPass) pass false and let the next real pass take the clear,
	// exactly as the screen's do.
	void flushTarget(RenderTarget* rt, bool settle);
	// Drop an offscreen target when its cTexture goes away (DeleteTexture): the next
	// cTexture allocated could land on the same address.
	void releaseTarget(cTexture* texture);
	SDL_GPUTexture* createDepthTexture(int w, int h);

	// CPU-staged GPU texture: keyed by the SDL_GPUTexture* stored in cTexture's
	// BitMap[0]. staging is the lockable CPU image; UnlockTexture uploads it.
	// expand=true means 8-bit coverage staging uploaded into a BGRA texture as
	// (255,255,255,coverage), so the one UI shader handles both font and images.
	struct TextureData {
		SDL_GPUTexture* tex = nullptr;
		int w = 0, h = 0, bpp = 0, pitch = 0;
		// Mip levels the texture was created with. The engine's cached DDS ship full
		// chains and D3D sampled them anisotropically; SDL GPU regenerates the chain
		// from level 0 after each upload. 1 means no chain (font atlas, A8L8).
		int levels = 1;
		bool expand = false;
		std::vector<unsigned char> staging;
	};
	std::unordered_map<SDL_GPUTexture*, TextureData> textures_;
	void uploadTexture(const TextureData& td);  // staging -> GPU (copy pass)

	// Static VB/IB backed by SDL GPU buffers + a CPU staging mirror, keyed on the
	// slot pointer (sSlotVB*/sSlotIB*) held by the sPtr wrappers. Lock hands back
	// the staging; Unlock uploads it to the GPU buffer.
	struct GpuBuffer {
		SDL_GPUBuffer* buf = nullptr;
		std::vector<unsigned char> staging;
	};
	std::unordered_map<sSlotVB*, GpuBuffer> vbGpu_;
	std::unordered_map<sSlotIB*, GpuBuffer> ibGpu_;
	static int strideFromDeclaration(IDirect3DVertexDeclaration9* decl);
	void uploadBuffer(GpuBuffer& gb);          // staging -> GPU (own copy pass)

	SDL_Window*            window_           = nullptr;
	SDL_GPUDevice*         device_           = nullptr;
	SDL_GPUCommandBuffer*  commandBuffer_    = nullptr;
	SDL_GPUTexture*        swapchainTexture_ = nullptr;

	// The scene depth buffer behind screen_, shared by every 3D renderer so their passes
	// occlude one another. Sized to the swapchain; owned here, like the swapchain image.
	SDL_GPUTexture* depthTexture_ = nullptr;
	int depthW_ = 0, depthH_ = 0;
	bool ensureDepth(int w, int h);

	// The shadow map, held as a cTexture so Camera::SetRenderTarget can take it and the
	// scene can ask its size. Its SDL depth texture lives in textures_ like any other,
	// and resolveTarget turns it into a depth-only RenderTarget.
	cTexture* shadowMap_ = nullptr;
	cTexture* lightMap_ = nullptr;
	Vect4f planarTransform_ = Vect4f(0.f, 0.f, 1.f, 1.f);
	int shadowMapSize_ = 0;
	Mat4f shadowMatViewProj_;
	bool shadowPassRan_ = false;

	MTSection resetDeviceLock_;          // dummy lock (no device loss on SDL)
	DWORD multisample_ = 0;
	bool  bActiveScene_ = false;
	int   fillMode_ = FILL_SOLID;   // RS_FILLMODE; FILL_WIREFRAME switches renderers to line pipelines
	bool  zWriteEnable_ = true;     // RS_ZWRITEENABLE; picks the object pipeline's depth write

	// RS_FOGENABLE. SetGlobalFog sets it, and the sky scene and the 2D pass clear it around
	// themselves -- the same one state cD3DRender kept, saved and restored.
	bool    fogEnable_ = false;
	Color4f fogColor_ = Color4f(0.f, 0.f, 0.f, 0.f);
	// D3DRS_FOGSTART / D3DRS_FOGEND: where the fog begins and where it is total, measured in
	// world units along the camera's view axis. fogPlane() turns them into the factor.
	Vect2f  fogRange_ = Vect2f(0.f, 1.f);

	std::unique_ptr<SDLUIRenderer>        uiRenderer_;
	std::unique_ptr<SDLTileMapRenderer>   tileMapRenderer_;
	std::unique_ptr<SDLObject3dxRenderer> objectRenderer_;
	std::unique_ptr<SDLWaterRenderer>     waterRenderer_;
	std::unique_ptr<SDLWorldQuadRenderer> worldQuadRenderer_;
	std::unique_ptr<SDLMinimapRenderer>   minimapRenderer_;
	std::unique_ptr<SDLGrassRenderer>     grassRenderer_;
	std::unique_ptr<SDLCloudShadowRenderer> cloudShadowRenderer_;
};

#endif // VISTA_SDL_RENDER_DEVICE_H
