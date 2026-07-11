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
// and swapchain, the frame's command buffer, the depth buffer, textures and
// vertex/index buffers — but no drawing pipeline of its own. Drawing lives in renderer
// classes that record their own passes into the frame's command buffer: SDLUIRenderer
// (2D text, sprites, quads), SDLTileMapRenderer (terrain), SDLObject3dxRenderer
// (skinned .3dx meshes), SDLWaterRenderer (the water surface) and SDLWorldQuadRenderer
// (world-space textured quads: the shoreline foam, the wave sources).

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
class cTileMap;

// Restrict drawing to a camera's viewport, the way cD3DRender::SetDrawTransform hands
// camera->vp to D3DDevice_->SetViewport. Camera::Update scales matProj to exactly that
// rect, so a renderer that ignores it stretches the scene over the whole window and
// loses the letterbox bars the 4:3 work area leaves. A render pass starts out with the
// full target as its viewport, so this only ever needs setting, never restoring.
void applyCameraViewport(SDL_GPURenderPass* pass, const sViewPort& vp, int targetW, int targetH);

// gb_RenderDevice as a cSDLRenderDevice, or null under any other device. Off-Windows
// gb_RenderDevice3D stays null, so engine code that needs the backend (cScene's shadow
// map, CameraShadowMap) asks for the SDL device through this instead.
class cSDLRenderDevice;
cSDLRenderDevice* sdlRenderDevice();

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
	// cWater::Draw, cCoastSprites::Draw and cFixedWavesContainer::Draw talk to their
	// renderers directly (the way they talk to VSWater/PSWater and the shared quad buffer
	// on Windows), then call these to put what they recorded on the screen.
	//
	// None of them is the first thing in the frame the way the terrain is: each belongs
	// where the scene walk reached it, over the opaque objects. Water and the coast
	// sprites draw in DrawObjectSpecial, which sorts by sortIndex(), so the water (-2)
	// lands under the sprites (0); the wave sources draw later still, in DrawSortObject's
	// sorted transparent pass. The object renderer batches, so each of these first replays
	// what it has recorded so far and only then opens its own pass over it. Whatever the
	// walk records afterwards replays at EndScene, on top, as on D3D.
	//
	// drawWorldQuads is called by each of its renderer's callers in turn, and each call
	// draws (and clears) only the quads recorded since the last one.
	SDLWaterRenderer* waterRenderer() { return waterRenderer_.get(); }
	void drawWater();
	SDLWorldQuadRenderer* worldQuadRenderer() { return worldQuadRenderer_.get(); }
	void drawWorldQuads();

	// --- Shadow map -------------------------------------------------------
	// Mirrors cD3DRender: cScene creates the map, the light camera renders the casters
	// into it, and receivers transform by shadowMatViewProj() * shadowMatBias().
	//
	// It is a plain depth texture here. D3D9 could not sample depth, so the original
	// renders the light-space z into a float colour target (object_shadow.psl's
	// `return (float4)v.tdepth`) and forks the whole path on DT_RADEON9700 vs
	// DT_GEFORCEFX. SDL GPU samples depth directly, so neither is needed.
	bool createShadowMap(int size);
	void deleteShadowMap();
	cTexture* GetShadowMap() { return shadowMap_; }
	int  GetShadowMapSize() const { return shadowMapSize_; }
	void SetShadowMatViewProj(const Mat4f& m) { shadowMatViewProj_ = m; }
	const Mat4f& shadowMatViewProj() const { return shadowMatViewProj_; }
	// Light clip space -> shadow map texture coordinates.
	Mat4f shadowMatBias() const;
	// The terrain caster, from cTileMap::Draw under the light camera -- where the D3D
	// backend calls tileMapRender_->DrawBump(camera, ALPHA_TEST, true, false). Draws
	// immediately, so it lands before the object casters, which then load its depth.
	// It needs no cTileMap: the caster mesh is the one SDLTileMapRenderer built from vMap.
	void drawTileMapShadow(Camera* camera);
	// Record the depth pass the light camera accumulated, where the D3D backend calls
	// DrawType::EndDrawShadow. It must land before any pass that samples the map.
	void endShadowPass();
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
	HWND GetWindowHandle() override { return (HWND)window_; }

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
	// shaders as uniforms, and the viewport belongs to a render pass. So these only
	// cache the camera, as cD3DRender does, and the renderers apply camera->vp when
	// they open their pass (see applyCameraViewport).
	void SetDrawTransform(Camera* camera) override { camera_ = camera; }
	void setCamera(Camera* camera) override { SetDrawTransform(camera); }
	void setWorldMatrix(const MatXf&) override {}

	// --- Misc state (no-op) ----------------------------------------------
	int  SetGamma(float, float, float) override { return 0; }
	// Only RS_FILLMODE (GameShell drives it from debugWireFrame) and RS_ZWRITEENABLE
	// (Camera::DrawSortObject turns it off for the transparent pass) are honoured; the
	// rest of the D3D render states have no SDL GPU equivalent outside a pipeline object.
	void SetRenderState(eRenderStateOption, int) override;
	unsigned int GetRenderState(eRenderStateOption) override;
	void SetGlobalFog(const Color4f&, const Vect2f&) override {}
	void SetSamplerDataVirtual(DWORD, SAMPLER_DATA&) override {}
	// D3D: "the advanced DrawType exists", i.e. the device can render a shadow map.
	// cVisGeneric::SetShadowType turns shadows off without it. We always can.
	bool IsEnableSelfShadow() override { return true; }
	bool SetScreenShot(const char*) override { return false; }

	// --- 2D primitives (forwarded to the UI renderer) --------------------
	void DrawLine(int x1, int y1, int x2, int y2, Color4c color) override;
	void DrawPixel(int x, int y, Color4c color) override;
	void DrawRectangle(int x, int y, int dx, int dy, Color4c color, bool outline) override;
	// Nothing to flush: the UI renderer already records these in call order alongside
	// the sprites and text, and draws them all in its pass at EndScene.
	void FlushPrimitive2D() override {}

	// --- 3D primitives (no-op) -------------------------------------------
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
	// Unimplemented, and unreached: nothing outside the D3D backend calls the solid,
	// two-texture or cTextureScale sprite variants. DrawSprite2 is used only by the
	// chaos post-process (Render/src/CChaos.cpp), which has no SDL path yet.
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

	// The scene depth buffer, shared by every 3D renderer so their passes occlude one
	// another. Sized to the swapchain; owned here, like the swapchain image itself.
	SDL_GPUTexture* depthTexture_ = nullptr;
	int depthW_ = 0, depthH_ = 0;
	bool ensureDepth(int w, int h);
	// The depth texture behind shadowMap_, or null if there is nothing to render into.
	SDL_GPUTexture* shadowDepthTexture();
	// Replay the object batch recorded so far, and take the frame's clears if they are
	// still going. drawWater / drawWorldQuads call it to get the opaque objects onto the
	// screen before they blend over them.
	void flushObjectPass();

	// The shadow map, held as a cTexture so Camera::SetRenderTarget can take it and the
	// scene can ask its size. Its SDL depth texture lives in textures_ like any other.
	cTexture* shadowMap_ = nullptr;
	int shadowMapSize_ = 0;
	Mat4f shadowMatViewProj_;
	bool shadowPassRan_ = false;
	// Which caster pass owns the map's clear this frame: the terrain's if it ran, else
	// the objects'. Same bookkeeping as frameCleared_/depthCleared_ do for the screen.
	bool shadowDepthCleared_ = false;

	MTSection resetDeviceLock_;          // dummy lock (no device loss on SDL)
	DWORD multisample_ = 0;
	bool  bActiveScene_ = false;
	bool  hasClear_     = false;
	// Set once the frame's colour / depth clear has been consumed by whichever renderer
	// opened the first pass, so later passes load the targets instead of wiping them.
	bool  frameCleared_ = false;
	bool  depthCleared_ = false;
	int   fillMode_ = FILL_SOLID;   // RS_FILLMODE; FILL_WIREFRAME switches renderers to line pipelines
	bool  zWriteEnable_ = true;     // RS_ZWRITEENABLE; picks the object pipeline's depth write
	float clearColor_[4] = {0.f, 0.f, 0.f, 1.f};

	std::unique_ptr<SDLUIRenderer>        uiRenderer_;
	std::unique_ptr<SDLTileMapRenderer>   tileMapRenderer_;
	std::unique_ptr<SDLObject3dxRenderer> objectRenderer_;
	std::unique_ptr<SDLWaterRenderer>     waterRenderer_;
	std::unique_ptr<SDLWorldQuadRenderer> worldQuadRenderer_;
};

#endif // VISTA_SDL_RENDER_DEVICE_H
