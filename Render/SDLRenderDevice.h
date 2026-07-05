#ifndef VISTA_SDL_RENDER_DEVICE_H
#define VISTA_SDL_RENDER_DEVICE_H

// Cross-platform render device (SDL GPU backend), replacing the Windows-only
// D3D9 cD3DRender. This is the "minimal render device": the lifecycle + 2D
// clear path are real SDL GPU; everything else (3D scene, meshes, tilemap,
// text) is a safe no-op, filled in slice by slice. See the migration plan.
//
// The legacy cInterfaceRenderDevice is an immediate-mode/synchronous D3D9 API;
// SDL GPU is explicit/async (command buffers + render passes). We bridge by
// recording during BeginScene..EndScene and submitting one command buffer at
// Flush. Slice 1b implements only Fill/BeginScene/EndScene/Flush (window clear).

#include "IRenderDevice.h"
#include "MTSection.h"
#include <vector>
#include <unordered_map>
#include <memory>

// SDL opaque handles, forward-declared so SDL stays out of engine headers.
struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUTexture;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

class cSDLRenderDevice : public cInterfaceRenderDevice
{
public:
	cSDLRenderDevice();
	~cSDLRenderDevice();

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

	// --- Slice 4: static-mesh rendering via the real VB/IB interface -------
	// Not part of cInterfaceRenderDevice: the off-Windows 3D models ship only as
	// the 32-bit InPlace cache, so the menu background recovers raw VB/IB from
	// .3dxGB and feeds them through the real CreateVertexBuffer/CreateIndexBuffer/
	// DrawIndexedPrimitive path (which this exercises). registerMesh takes ownership
	// of the caller's vb/ib (clearing the caller's handles) and retains the mesh so
	// EndScene can redraw it every frame via DrawIndexedPrimitive. Returns a handle,
	// -1 on failure.
	int  registerMesh(sPtrVertexBuffer& vb, sPtrIndexBuffer& ib);
	// Append a textured sub-range (firstIndex/indexCount into the mesh's index
	// buffer). tint = material diffuse rgba (rgb color, a opacity); transparency
	// selects the blend pipeline (0=substractive, 1=additive, 2=filter). light =
	// 4 floats {dir.xyz toward the light (world space), strength}; null or w==0
	// leaves the draw unlit/full-bright (menu default), w>0 adds relief lighting.
	// water = 3 floats {strength, spatialScale, spare}; null or strength==0 leaves
	// the draw foam-free, strength>0 adds the animated water-foam layer (time is
	// injected per frame at draw time).
	// depthWrite = true marks the draw as opaque base geometry (terrain): it writes
	// depth so translucent things drawn after it (the water sheet, which keeps
	// depth-write off) get occluded by geometry in front. Menu decals leave it false
	// (they composite in paint order with depth-write off).
	void addMeshSubmesh(int handle, int firstIndex, int indexCount, cTexture* tex,
	                    const float* tint = nullptr, int transparency = 2,
	                    const float* light = nullptr, const float* water = nullptr,
	                    bool depthWrite = false);
	// Supply the model-view-projection matrix (16 floats, row-major, row-vector
	// v*M, D3D clip convention) for the mesh. Replaces the built-in auto-frame.
	void setMeshTransform(int handle, const float* mvp16);
	void releaseMesh(int handle);

	// --- Textures (slice 2b): real SDL GPU textures + CPU staging ----------
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

	// --- Frame / 2D clear (real) -----------------------------------------
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

	// --- Camera / transform (no-op) --------------------------------------
	void SetDrawTransform(Camera*) override {}
	void setCamera(Camera*) override {}
	void setWorldMatrix(const MatXf&) override {}

	// --- Misc state (no-op) ----------------------------------------------
	int  SetGamma(float, float, float) override { return 0; }
	void SetRenderState(eRenderStateOption, int) override {}
	unsigned int GetRenderState(eRenderStateOption) override { return 0; }
	void SetGlobalFog(const Color4f&, const Vect2f&) override {}
	void SetSamplerDataVirtual(DWORD, SAMPLER_DATA&) override {}
	bool IsEnableSelfShadow() override { return false; }
	bool SetScreenShot(const char*) override { return false; }

	// --- 2D primitives (no-op until slice 2) -----------------------------
	void DrawLine(int, int, int, int, Color4c) override {}
	void DrawPixel(int, int, Color4c) override {}
	void DrawRectangle(int, int, int, int, Color4c, bool) override {}
	void FlushPrimitive2D() override {}

	// --- 3D primitives (no-op) -------------------------------------------
	void DrawLine(const Vect3f&, const Vect3f&, Color4c) override {}
	void DrawPoint(const Vect3f&, Color4c) override {}
	void FlushPrimitive3D() override {}
	void FlushLine3D(bool, bool) override {}
	void drawCircle(const Vect3f&, float, Color4c) override {}
	void DrawBound(const MatXf&, Vect3f&, Vect3f&, bool, Color4c) override {}

	// --- Text ------------------------------------------------------------
	void OutText(int, int, const char*, const Color4f&, ALIGN_TEXT, eBlendMode, Vect2f) override {}
	int  OutTextLine(int x, int y, const FT::Font& font, const wchar_t* textline, const wchar_t* end, const Color4c& color, eBlendMode blend_mode, int xRangeMin, int xRangeMax) override;
	void OutText(int, int, const char*, int, int, int) override {}
	void OutText(int, int, const char*, int, int, int, char*, int, int, int, int) override {}

	// --- Sprites ---------------------------------------------------------
	void DrawQuad(float, float, float, float, float, float, float, float, Color4c) override;
	void DrawSprite(int, int, int, int, float, float, float, float, cTexture*, const Color4c&, float, eBlendMode, float) override;
	void DrawSpriteSolid(int, int, int, int, float, float, float, float, cTexture*, const Color4c&, float, eBlendMode) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, cTexture*, cTexture*, const Color4c&, float) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, float, float, float, float, cTexture*, cTexture*, const Color4c&, float, eColorMode, eBlendMode) override {}
	void DrawSprite2(int, int, int, int, float, float, float, float, float, float, float, float, cTexture*, cTexture*, float, float, float, eColorMode, eBlendMode) override {}
	void DrawSpriteScale(int, int, int, int, float, float, cTextureScale*, const Color4c&, float, eBlendMode) override {}
	void DrawSpriteScale2(int, int, int, int, float, float, cTextureScale*, cTextureScale*, const Color4c&, float) override {}
	void DrawSpriteScale2(int, int, int, int, float, float, float, float, cTextureScale*, cTextureScale*, const Color4c&, float, eColorMode) override {}

	// --- Materials -------------------------------------------------------
	// Records the current texture/blend for the following DrawQuad calls.
	void SetNoMaterial(eBlendMode, const MatXf&, float, cTexture*, cTexture*, eColorMode) override;
	void SetWorldMaterial(eBlendMode, const MatXf&, float, cTexture*, cTexture*, eColorMode, bool, bool) override {}

	// --- Vertex/index buffers (slice 4): real SDL GPU static buffers ------
	void DrawIndexedPrimitive(sPtrVertexBuffer&, int, int, const sPtrIndexBuffer&, int, int) override;
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
	// CPU-side 2D vertex, byte-compatible with the UI vertex input layout
	// (matches sVertexXYZWDT1: float4 pos, BGRA u8 colour, float2 uv = 28 bytes).
	struct UIVertex { float x, y, z, w; unsigned int color; float u, v; };

	void createUIPipeline();             // lazy one-time pipeline/sampler/buffers
	void ensureVertexCapacity(int verts);
	// Append a quad (6 verts) bound to tex, extending or starting a draw run.
	void emitQuad(float x, float y, float dx, float dy,
	              float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex);

	// CPU-staged GPU texture: keyed by the SDL_GPUTexture* stored in cTexture's
	// BitMap[0]. staging is the lockable CPU image; UnlockTexture uploads it.
	// expand=true means 8-bit coverage staging uploaded into a BGRA texture as
	// (255,255,255,coverage), so the one UI shader handles both font and images.
	struct TextureData {
		SDL_GPUTexture* tex = nullptr;
		int w = 0, h = 0, bpp = 0, pitch = 0;
		bool expand = false;
		std::vector<unsigned char> staging;
	};
	std::unordered_map<SDL_GPUTexture*, TextureData> textures_;
	void uploadTexture(const TextureData& td);  // staging -> GPU (copy pass)

	// One draw call per contiguous run of quads sharing a texture.
	struct DrawRun { SDL_GPUTexture* tex; int first; int count; };
	std::vector<DrawRun> runs_;
	SDL_GPUTexture* currentTexture_ = nullptr;  // set by SetNoMaterial

	// Slice 4: static VB/IB backed by SDL GPU buffers + a CPU staging mirror,
	// keyed on the slot pointer (sSlotVB*/sSlotIB*) held by the sPtr wrappers.
	// Lock hands back the staging; Unlock uploads it to the GPU buffer.
	struct GpuBuffer {
		SDL_GPUBuffer* buf = nullptr;
		std::vector<unsigned char> staging;
	};
	std::unordered_map<sSlotVB*, GpuBuffer> vbGpu_;
	std::unordered_map<sSlotIB*, GpuBuffer> ibGpu_;
	static int strideFromDeclaration(IDirect3DVertexDeclaration9* decl);
	void uploadBuffer(GpuBuffer& gb);          // staging -> GPU (own copy pass)

	// One indexed draw recorded by DrawIndexedPrimitive and replayed inside the
	// 3D render pass at EndScene (SDL GPU can only draw inside a pass).
	struct MeshDraw {
		SDL_GPUBuffer* vbuf; SDL_GPUBuffer* ibuf;
		int baseVertex; int startIndex; int indexCount;
		float mvp[16];
		SDL_GPUTexture* tex; float tint[4]; int transparency;
		float light[4];   // xyz = dir toward light, w = strength (0 = unlit)
		float water[4];   // x = foam strength (0 = none), y = scale, z spare, w = time
		bool depthWrite;  // opaque base geometry (terrain) writes depth; water/decals don't
	};
	std::vector<MeshDraw> meshDraws_;
	// "Current" material/transform state that DrawIndexedPrimitive snapshots into
	// each MeshDraw (the immediate-mode state the D3D backend reads from the device).
	float           curMVP_[16] = {0};
	SDL_GPUTexture* curMeshTexture_ = nullptr;
	float           curMeshTint_[4] = {1,1,1,1};
	int             curMeshTransparency_ = 2;
	float           curMeshLight_[4] = {0,0,0,0};   // xyz dir, w strength (0 = unlit)
	float           curMeshWater_[4] = {0,0,0,0};   // x strength (0 = no foam), y scale
	bool            curMeshDepthWrite_ = false;     // true = opaque, write depth (terrain)
	void flushMeshDraws(SDL_GPURenderPass* pass);   // replay meshDraws_ in the pass

	// Retained menu-background meshes: they own the real VB/IB (held indirectly so
	// the sPtr members never move -> no double free) and are redrawn every frame by
	// recordMenuMeshes() through the real DrawIndexedPrimitive interface.
	struct SubDraw {
		int firstIndex; int indexCount; SDL_GPUTexture* tex;
		float tint[4];      // material diffuse rgba (rgb color, a opacity)
		int transparency;   // 0=substractive, 1=additive, 2=filter
		float light[4];     // xyz = dir toward light, w = strength (0 = unlit)
		float water[4];     // x = foam strength (0 = none), y = scale, z spare, w unused
		bool depthWrite;    // opaque base geometry (terrain) writes depth; decals/water don't
	};
	struct MenuMesh {
		sPtrVertexBuffer vb; sPtrIndexBuffer ib;   // own one reference to the buffers
		int numVertex = 0;
		float bmin[3] = {0,0,0};
		float bmax[3] = {0,0,0};
		float mvp[16] = {0};
		bool hasTransform = false;                 // false -> auto-frame fallback
		std::vector<SubDraw> subdraws;
	};
	std::vector<std::unique_ptr<MenuMesh>> menuMeshes_;
	void recordMenuMeshes();   // per frame: set curstate + call DrawIndexedPrimitive

	SDL_GPUGraphicsPipeline* meshPipeline_ = nullptr;       // filter (alpha-over) blend, no depth write
	SDL_GPUGraphicsPipeline* meshPipelineOpaque_ = nullptr; // filter blend + depth-write ON (terrain)
	SDL_GPUGraphicsPipeline* meshPipelineAdd_ = nullptr;    // additive blend
	SDL_GPUTexture*          depthTexture_ = nullptr;
	int depthW_ = 0, depthH_ = 0;
	bool meshPipelineTried_ = false;
	void createMeshPipeline();
	void ensureDepth(int w, int h);

	// Water pipeline (P2 water slice): reuses the mesh3d vertex shader but a dedicated
	// fragment shader (water.frag) that samples two scrolling bump textures + the baked
	// depth-opacity texture for the animated wave surface + specular glint. A mesh draw
	// with water[0] > 0 (set by WaterRenderSDL) is routed here instead of meshPipeline_.
	SDL_GPUGraphicsPipeline* waterPipeline_ = nullptr;
	bool waterPipelineTried_ = false;
	void createWaterPipeline();
	SDL_GPUTexture* waterBump0_ = nullptr;   // borrowed SDL handles (WaterRenderSDL owns
	SDL_GPUTexture* waterBump1_ = nullptr;   // the cTextures); resolved via sdlTextureOf
	SDL_GPUSampler* waterSampler_ = nullptr; // REPEAT/wrap: the wave bumps tile (worldXY UV)
	float           waterFS_[12] = {0};      // {LightDir, CameraPos(.w=time), Params}
public:
	// Per-frame water shading state, supplied by WaterRenderSDL before the pass. camPos3
	// and lightDir3 are 3 floats each; bump0/bump1 are the two wave textures; the scalars
	// tune the bump sampling/glint. Time is injected each frame from SDL ticks.
	void setWaterRenderState(cTexture* bump0, cTexture* bump1, const float* camPos3,
	                         const float* lightDir3, float bumpScale, float scrollSpeed,
	                         float rippleStrength, float specStrength);
private:

	SDL_Window*            window_           = nullptr;
	SDL_GPUDevice*         device_           = nullptr;
	SDL_GPUCommandBuffer*  commandBuffer_    = nullptr;
	SDL_GPURenderPass*     renderPass_       = nullptr;
	SDL_GPUTexture*        swapchainTexture_ = nullptr;

	// UI pipeline (slice 2)
	SDL_GPUGraphicsPipeline* uiPipeline_     = nullptr;
	SDL_GPUSampler*          sampler_        = nullptr;
	SDL_GPUTexture*          whiteTexture_   = nullptr;
	SDL_GPUBuffer*           vertexBuffer_   = nullptr;
	SDL_GPUTransferBuffer*   transferBuffer_ = nullptr;
	int   vertexCapacity_ = 0;
	bool  pipelineTried_  = false;
	std::vector<UIVertex> batch_;

	MTSection resetDeviceLock_;          // dummy lock (no device loss on SDL)
	DWORD multisample_ = 0;
	bool  bActiveScene_ = false;
	bool  hasClear_     = false;
	float clearColor_[4] = {0.f, 0.f, 0.f, 1.f};
};

#endif // VISTA_SDL_RENDER_DEVICE_H
