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

	// --- Slice 3: minimal static-mesh rendering (geometry-only path) -------
	// Not part of cInterfaceRenderDevice: the off-Windows 3D models ship only as
	// the 32-bit InPlace cache, so we render raw VB/IB recovered from .3dxGB
	// (see Render/3dx/MeshCacheGeometry). Returns a handle, -1 on failure.
	int  uploadMesh(const void* vb, int vbBytes, int stride, const void* ib, int indexCount);
	void setMeshTexture(int handle, cTexture* tex);
	// Append a textured sub-range (firstIndex/indexCount into the mesh's index
	// buffer). Once any sub-range is set, the whole-mesh draw is replaced by them.
	// tint = material diffuse rgba (rgb color, a opacity); transparency selects the
	// blend pipeline (0=substractive, 1=additive, 2=filter).
	void addMeshSubmesh(int handle, int firstIndex, int indexCount, cTexture* tex,
	                    const float* tint = nullptr, int transparency = 2);
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

	// --- Vertex/index buffers (no-op until slice 2) ----------------------
	void DrawIndexedPrimitive(sPtrVertexBuffer&, int, int, const sPtrIndexBuffer&, int, int) override {}
	void CreateVertexBuffer(sPtrVertexBuffer&, int, IDirect3DVertexDeclaration9*, int) override {}
	void DeleteVertexBuffer(sPtrVertexBuffer&) override {}
	void* LockVertexBuffer(sPtrVertexBuffer&, bool) override { return nullptr; }
	void UnlockVertexBuffer(sPtrVertexBuffer&) override {}
	void CreateIndexBuffer(sPtrIndexBuffer&, int, int) override {}
	void DeleteIndexBuffer(sPtrIndexBuffer&) override {}
	sPolygon* LockIndexBuffer(sPtrIndexBuffer&, bool) override { return nullptr; }
	void UnlockIndexBuffer(sPtrIndexBuffer&) override {}

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

	// Slice 3: static meshes uploaded from raw .3dxGB geometry.
	struct SubDraw {
		int firstIndex; int indexCount; SDL_GPUTexture* tex;
		float tint[4];      // material diffuse rgba (rgb color, a opacity)
		int transparency;   // 0=substractive, 1=additive, 2=filter
	};
	struct Mesh {
		SDL_GPUBuffer* vbuf = nullptr;
		SDL_GPUBuffer* ibuf = nullptr;
		int indexCount = 0;
		int stride = 0;
		float bmin[3] = {0,0,0};
		float bmax[3] = {0,0,0};
		SDL_GPUTexture* tex = nullptr;        // whole-mesh texture (if no subdraws)
		std::vector<SubDraw> subdraws;        // per-material textured ranges
		float mvp[16] = {0};                  // supplied transform (row-major)
		bool hasTransform = false;            // false -> use the auto-frame fallback
		bool active = false;
	};
	std::vector<Mesh> meshes_;
	SDL_GPUGraphicsPipeline* meshPipeline_ = nullptr;     // filter (alpha-over) blend
	SDL_GPUGraphicsPipeline* meshPipelineAdd_ = nullptr;  // additive blend
	SDL_GPUTexture*          depthTexture_ = nullptr;
	int depthW_ = 0, depthH_ = 0;
	bool meshPipelineTried_ = false;
	void createMeshPipeline();
	void ensureDepth(int w, int h);
	void drawMeshes(SDL_GPURenderPass* pass);  // 3D pass: build MVP + draw meshes

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
