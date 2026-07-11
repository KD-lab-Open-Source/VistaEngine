// SDL GPU render device — device, swapchain and shared GPU resources. See header.
#include "StdAfxRD.h"
#include "SDLRenderDevice.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "Texture.h"     // cTexture (BitMap / GetDDSurface / attributes)
#include "FileImage.h"   // cFileImage::GetTexture

#include "SDLUIRenderer.h"
#include "SDLTileMapRenderer.h"
#include "SDLObject3dxRenderer.h"

// ---------------------------------------------------------------------------
// Base cInterfaceRenderDevice members. On Windows these live in
// Render/D3D/RenderDevice.cpp (not compiled off-Windows), so the cross-platform
// subclass must supply them here.
// ---------------------------------------------------------------------------
cInterfaceRenderDevice::cInterfaceRenderDevice()
{
	NumberPolygon = 0;
	NumDrawObject = 0;
	NumberTilemapPolygon = 0;
	PtrNumberPolygon = &NumberPolygon;
	RenderMode = 0;
	xScr = yScr = 0;
	xScrMin = yScrMin = xScrMax = yScrMax = 0;
	camera_ = 0;
	DefaultFont = CurrentFont = 0;
}

cInterfaceRenderDevice::~cInterfaceRenderDevice()
{
	gb_RenderDevice = 0;
}

void cInterfaceRenderDevice::SetFont(FT::Font* pFont)
{
	CurrentFont = pFont;
	if(!CurrentFont)
		CurrentFont = DefaultFont;
}

void cInterfaceRenderDevice::SetDefaultFont(FT::Font* pFont)
{
	if(!pFont && DefaultFont == CurrentFont)
		CurrentFont = 0;
	DefaultFont = pFont;
	if(!CurrentFont)
		CurrentFont = DefaultFont;
}

// ---------------------------------------------------------------------------
// cSDLRenderDevice
// ---------------------------------------------------------------------------
cSDLRenderDevice::cSDLRenderDevice() {}

cSDLRenderDevice::~cSDLRenderDevice()
{
	Done();
}

bool cSDLRenderDevice::Initialize(int xScr_, int yScr_, int mode, HWND hWnd, int /*RefreshRateInHz*/, HWND /*fallbackWindow*/)
{
	// hWnd carries the SDL_Window* created by Platform/Window.cpp (HWND == void*).
	window_ = static_cast<SDL_Window*>(hWnd);
	if(!window_){
		fprintf(stderr, "cSDLRenderDevice::Initialize: no window\n");
		return false;
	}

	if(!device_){
		device_ = SDL_CreateGPUDevice(
			SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
			false, nullptr);
		if(!device_){
			fprintf(stderr, "cSDLRenderDevice::Initialize: SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
			return false;
		}
		if(!SDL_ClaimWindowForGPUDevice(device_, window_)){
			fprintf(stderr, "cSDLRenderDevice::Initialize: SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
			SDL_DestroyGPUDevice(device_);
			device_ = nullptr;
			return false;
		}
		fprintf(stderr, "cSDLRenderDevice: SDL GPU device created (%s)\n", SDL_GetGPUDeviceDriver(device_));
	}

	xScr = xScr_;
	yScr = yScr_;
	xScrMin = yScrMin = 0;
	xScrMax = xScr;
	yScrMax = yScr;
	RenderMode = mode;

	if(!uiRenderer_)
		uiRenderer_ = std::make_unique<SDLUIRenderer>(device_, window_);
	if(!tileMapRenderer_)
		tileMapRenderer_ = std::make_unique<SDLTileMapRenderer>(device_, window_);
	if(!objectRenderer_)
		objectRenderer_ = std::make_unique<SDLObject3dxRenderer>(this, device_, window_);

	// Build the skinned-vertex declarations (on Windows cD3DRender does this at
	// device init via CreateVertexDeclaration; cSkinVertex::Register is portable
	// off-Windows). Needed so cStatic3dx buffers get a real vertex layout/stride.
	static bool skinDeclRegistered = false;
	if(!skinDeclRegistered){ cSkinVertex::Register(); skinDeclRegistered = true; }
	return true;
}

int cSDLRenderDevice::Done()
{
	if(bActiveScene_)
		EndScene();
	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;

	// The renderers hold GPU objects built on device_, so they must go first.
	uiRenderer_.reset();
	tileMapRenderer_.reset();
	objectRenderer_.reset();

	if(device_){
		if(depthTexture_){
			SDL_ReleaseGPUTexture(device_, depthTexture_);
			depthTexture_ = nullptr;
			depthW_ = depthH_ = 0;
		}
		for(auto& kv : textures_)
			if(kv.second.tex) SDL_ReleaseGPUTexture(device_, kv.second.tex);
		textures_.clear();
		for(auto& kv : vbGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		for(auto& kv : ibGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		vbGpu_.clear();
		ibGpu_.clear();
		if(window_)
			SDL_ReleaseWindowFromGPUDevice(device_, window_);
		SDL_DestroyGPUDevice(device_);
		device_ = nullptr;
	}
	window_ = nullptr;

	// Reset base state so ~cInterfaceRenderDevice's invariants hold.
	RenderMode = 0;
	xScr = yScr = 0;
	xScrMin = yScrMin = xScrMax = yScrMax = 0;
	CurrentFont = DefaultFont = 0;
	return 0;
}

bool cSDLRenderDevice::ChangeSize(int xScr_, int yScr_, int mode)
{
	xScr = xScr_;
	yScr = yScr_;
	xScrMin = yScrMin = 0;
	xScrMax = xScr;
	yScrMax = yScr;
	RenderMode = mode;
	return true;
}

int cSDLRenderDevice::Fill(int r, int g, int b, int a)
{
	clearColor_[0] = r / 255.f;
	clearColor_[1] = g / 255.f;
	clearColor_[2] = b / 255.f;
	clearColor_[3] = a / 255.f;
	hasClear_ = true;
	return 0;
}

int cSDLRenderDevice::BeginScene()
{
	if(!device_) return -1;
	if(bActiveScene_) return 1;

	// Acquire the frame's command buffer + swapchain texture. Passes are deferred to
	// EndScene: draws accumulate into each renderer's CPU batch during the scene, and
	// the renderers record their copy + render passes once, at EndScene.
	commandBuffer_ = SDL_AcquireGPUCommandBuffer(device_);
	if(!commandBuffer_){
		fprintf(stderr, "cSDLRenderDevice::BeginScene: AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
		return -1;
	}

	Uint32 w = 0, h = 0;
	if(!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer_, window_, &swapchainTexture_, &w, &h) || !swapchainTexture_){
		// No drawable surface this frame (e.g. minimized); submit empty and bail.
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
		swapchainTexture_ = nullptr;
		return -1;
	}

	if(uiRenderer_)
		uiRenderer_->BeginFrame();
	if(objectRenderer_)
		objectRenderer_->BeginFrame();
	frameCleared_ = false;
	depthCleared_ = false;
	bActiveScene_ = true;
	NumberPolygon = 0;
	NumDrawObject = 0;
	NumberTilemapPolygon = 0;
	return 0;
}

int cSDLRenderDevice::EndScene()
{
	if(!bActiveScene_) return 1;
	bActiveScene_ = false;

	// The scene's objects were recorded during the walk; draw them now, over the terrain
	// and against its depth. Then the UI pass, last, over everything. Each pass carries
	// the frame's colour (and depth) clear only if no earlier one already took it.
	if(swapchainTexture_ && commandBuffer_ && objectRenderer_ && objectRenderer_->hasDraws()
	   && ensureDepth(xScr, yScr)){
		const bool clear = hasClear_ && !frameCleared_;
		if(objectRenderer_->Draw(commandBuffer_, swapchainTexture_, depthTexture_, xScr, yScr,
		                         clear, clearColor_, !depthCleared_, fillMode_ == FILL_WIREFRAME)){
			if(clear) frameCleared_ = true;
			depthCleared_ = true;
		}
	}

	if(swapchainTexture_ && commandBuffer_ && uiRenderer_)
		uiRenderer_->Draw(commandBuffer_, swapchainTexture_, xScr, yScr,
		                  hasClear_ && !frameCleared_, clearColor_);

	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;
	hasClear_ = false;
	return 0;
}

// The depth target every 3D pass shares. Sized to the swapchain, rebuilt on resize.
bool cSDLRenderDevice::ensureDepth(int w, int h)
{
	if(!device_ || w <= 0 || h <= 0) return false;
	if(depthTexture_ && depthW_ == w && depthH_ == h) return true;
	if(depthTexture_) SDL_ReleaseGPUTexture(device_, depthTexture_);

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	ti.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	depthTexture_ = SDL_CreateGPUTexture(device_, &ti);
	depthW_ = depthTexture_ ? w : 0;
	depthH_ = depthTexture_ ? h : 0;
	return depthTexture_ != nullptr;
}

int cSDLRenderDevice::Flush()
{
	if(bActiveScene_)
		EndScene();
	return 0;
}

// ---------------------------------------------------------------------------
// Terrain: forwarded to the tilemap renderer, which draws immediately in its own
// pass. Called mid-scene from cTileMap::Draw, so the frame's command buffer and
// swapchain image are live and no render pass is open.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::drawTileMap(cTileMap* tileMap, Camera* camera)
{
	if(!bActiveScene_ || !commandBuffer_ || !swapchainTexture_ || !tileMapRenderer_)
		return;
	if(!ensureDepth(xScr, yScr))
		return;
	const bool clear = hasClear_ && !frameCleared_;
	if(tileMapRenderer_->Draw(commandBuffer_, swapchainTexture_, depthTexture_, xScr, yScr,
	                          clear, clearColor_, !depthCleared_, tileMap, camera,
	                          fillMode_ == FILL_WIREFRAME)){
		if(clear) frameCleared_ = true;
		depthCleared_ = true;
	}
}

// Resolve the engine's buffer handles to the SDL GPU buffers behind them, for the
// renderers that draw from geometry the engine filled.
SDL_GPUBuffer* cSDLRenderDevice::gpuBuffer(const sPtrVertexBuffer& vb) const
{
	if(!vb.ptr) return nullptr;
	auto it = vbGpu_.find(vb.ptr);
	return it != vbGpu_.end() ? it->second.buf : nullptr;
}

SDL_GPUBuffer* cSDLRenderDevice::gpuBuffer(const sPtrIndexBuffer& ib) const
{
	if(!ib.ptr) return nullptr;
	auto it = ibGpu_.find(ib.ptr);
	return it != ibGpu_.end() ? it->second.buf : nullptr;
}

// GameShell sets RS_FILLMODE every frame from debugWireFrame (Scripts/TreeControlSetups/
// Debug.dat), and Camera::DrawSortObject clears RS_ZWRITEENABLE around the transparent
// pass. SDL GPU bakes both into the pipeline, so renderers keep variants and pick them
// up from here.
void cSDLRenderDevice::SetRenderState(eRenderStateOption state, int value)
{
	if(state == RS_FILLMODE)
		fillMode_ = value;
	else if(state == RS_ZWRITEENABLE)
		zWriteEnable_ = value != 0;
}

unsigned int cSDLRenderDevice::GetRenderState(eRenderStateOption state)
{
	if(state == RS_FILLMODE) return (unsigned int)fillMode_;
	if(state == RS_ZWRITEENABLE) return zWriteEnable_ ? 1u : 0u;
	return 0u;
}

// ---------------------------------------------------------------------------
// 2D entry points: forwarded to the UI renderer, which batches them and draws
// them all in its own pass at EndScene.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::SetNoMaterial(eBlendMode /*blend*/, const MatXf&, float /*phase*/,
                                     cTexture* Texture0, cTexture* /*Texture1*/, eColorMode /*mode*/)
{
	// (Blend state is baked into the single alpha pipeline for now; per-blend
	// pipelines come later.)
	if(uiRenderer_)
		uiRenderer_->SetTexture(Texture0);
}

void cSDLRenderDevice::DrawQuad(float x1, float y1, float dx, float dy,
                                float u1, float v1, float du, float dv, Color4c color)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	uiRenderer_->DrawQuad(x1, y1, dx, dy, u1, v1, du, dv, color);
	NumberPolygon += 2;
}

void cSDLRenderDevice::DrawSprite(int x, int y, int dx, int dy,
                                  float u, float v, float du, float dv,
                                  cTexture* Texture, const Color4c& ColorMul,
                                  float /*phase*/, eBlendMode /*mode*/, float /*saturate*/)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	uiRenderer_->DrawSprite(x, y, dx, dy, u, v, du, dv, Texture, ColorMul);
	NumberPolygon += 2;
}

int cSDLRenderDevice::OutTextLine(int x, int y, const FT::Font& font, const wchar_t* textline, const wchar_t* end,
                                  const Color4c& color, eBlendMode /*blend_mode*/, int xRangeMin, int xRangeMax)
{
	if(!bActiveScene_ || !uiRenderer_)
		return x;
	// One quad per glyph drawn, so ask the renderer how many it appended.
	const int before = uiRenderer_->quadCount();
	const int right = uiRenderer_->OutTextLine(x, y, font, textline, end, color, xRangeMin, xRangeMax);
	NumberPolygon += 2 * (uiRenderer_->quadCount() - before);
	return right;
}

// ---------------------------------------------------------------------------
// Textures: real SDL GPU textures backed by a lockable CPU staging image. The
// SDL_GPUTexture* is handed to cTexture via BitMap[0] (the fake IDirect3DTexture9
// Release() is a no-op, so storing a non-D3D pointer is safe); TextureData
// (staging + dims) is kept here, keyed by that handle.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::uploadTexture(const TextureData& td)
{
	if(!device_ || !td.tex || td.staging.empty()) return;

	// The GPU texture is always BGRA8. For coverage (expand) textures the staging
	// is 1 byte/px, widened here to (255,255,255,coverage).
	const Uint32 bytes = (Uint32)(td.w * td.h * 4);
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = bytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return;

	unsigned char* map = (unsigned char*)SDL_MapGPUTransferBuffer(device_, tb, false);
	if(td.bpp == 1){
		// 8-bit coverage -> (255,255,255, coverage)
		const int n = td.w * td.h;
		for(int i = 0; i < n; ++i){
			map[i*4+0] = 255; map[i*4+1] = 255; map[i*4+2] = 255;  // B,G,R
			map[i*4+3] = td.staging[i];                            // A = coverage
		}
	} else if(td.bpp == 2){
		// A8L8 staging (byte0 = L, byte1 = A) -> (L,L,L,A)
		const int n = td.w * td.h;
		for(int i = 0; i < n; ++i){
			const unsigned char l = td.staging[i*2+0], a = td.staging[i*2+1];
			map[i*4+0] = l; map[i*4+1] = l; map[i*4+2] = l;
			map[i*4+3] = a;
		}
	} else {
		SDL_memcpy(map, td.staging.data(), bytes);
	}
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
	SDL_GPUTextureTransferInfo src = {};
	src.transfer_buffer = tb;
	src.offset = 0;
	src.pixels_per_row = (Uint32)td.w;
	src.rows_per_layer = (Uint32)td.h;
	SDL_GPUTextureRegion dst = {};
	dst.texture = td.tex;
	dst.w = (Uint32)td.w; dst.h = (Uint32)td.h; dst.d = 1;
	SDL_UploadToGPUTexture(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cb);
	SDL_ReleaseGPUTransferBuffer(device_, tb);
}

int cSDLRenderDevice::CreateTexture(cTexture* Texture, cFileImage* FileImage, int /*dxout*/, int /*dyout*/, bool /*enable_assert*/)
{
	if(!device_ || !Texture) return 1;

	const int w = Texture->GetWidth();
	const int h = Texture->GetHeight();
	if(w <= 0 || h <= 0) return 1;

	// GPU texture is always BGRA8 so one UI shader/sampler covers everything.
	// Gray/alpha-only (the font atlas) keeps 1-byte coverage staging (so the font
	// code's LockTexture pitch is right) and is widened to BGRA on upload; colour
	// textures stage as 32-bit BGRA, matching cFileImage::GetTexture's byte order.
	const bool gray = Texture->getAttribute(TEXTURE_GRAY) != 0;
	// Staging bytes/pixel must match the format callers Lock and write to, or their
	// writes overflow it. TEXTURE_GRAY is set for both 8-bit coverage (font/alpha)
	// AND 16-bit A8L8 (the water Z/reflection texture, written 2 bytes/px in
	// cWater::Init), so A8L8 must be 2 bpp -- otherwise that fill smashes the heap.
	int bpp = gray ? 1 : 4;
	if(Texture->format() == SURFMT_A8L8)
		bpp = 2;

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	SDL_GPUTexture* tex = SDL_CreateGPUTexture(device_, &ti);
	if(!tex) return 1;

	TextureData td;
	td.tex = tex; td.w = w; td.h = h; td.bpp = bpp; td.pitch = w * bpp;
	td.expand = (bpp == 1);
	td.staging.assign((size_t)w * h * bpp, 0);

	if(FileImage){
		// GetTexture writes 32-bit BGRA pixels; meaningful for the colour path.
		FileImage->GetTexture(td.staging.data(), 0, w, h);
		uploadTexture(td);
	}

	textures_[tex] = std::move(td);

	// Release any texture we previously parked in slot 0, then hand over the new
	// handle. (Single-frame; animated multi-frame textures keep only frame 0.)
	if(Texture->frameNumber() < 1)
		Texture->New(1);
	else if(SDL_GPUTexture* old = reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0))){
		auto it = textures_.find(old);
		if(it != textures_.end() && old != tex){
			SDL_ReleaseGPUTexture(device_, old);
			textures_.erase(it);
		}
	}
	Texture->GetDDSurface(0) = reinterpret_cast<IDirect3DTexture9*>(tex);
	return 0;  // 0 == success (matches the D3D contract)
}

int cSDLRenderDevice::DeleteTexture(cTexture* Texture)
{
	if(!Texture) return 0;
	for(int i = 0; i < Texture->frameNumber(); ++i){
		SDL_GPUTexture* tex = reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(i));
		if(!tex) continue;
		auto it = textures_.find(tex);
		if(it != textures_.end()){
			if(device_) SDL_ReleaseGPUTexture(device_, tex);
			textures_.erase(it);
		}
		Texture->GetDDSurface(i) = 0;
	}
	return 0;
}

void* cSDLRenderDevice::LockTexture(cTexture* Texture, int& Pitch)
{
	Pitch = 0;
	if(!Texture || Texture->frameNumber() < 1) return nullptr;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it == textures_.end()) return nullptr;
	Pitch = it->second.pitch;
	return it->second.staging.data();
}

void* cSDLRenderDevice::LockTexture(cTexture* Texture, int& Pitch, Vect2i lock_min, Vect2i /*lock_size*/)
{
	Pitch = 0;
	if(!Texture || Texture->frameNumber() < 1) return nullptr;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it == textures_.end()) return nullptr;
	Pitch = it->second.pitch;
	return it->second.staging.data() + lock_min.y * it->second.pitch + lock_min.x * it->second.bpp;
}

void cSDLRenderDevice::UnlockTexture(cTexture* Texture)
{
	if(!Texture || Texture->frameNumber() < 1) return;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it != textures_.end())
		uploadTexture(it->second);
}

// ---------------------------------------------------------------------------
// Static VB/IB (the real cInterfaceRenderDevice buffer interface). Each slot is
// backed by an SDL_GPUBuffer plus a CPU staging mirror keyed on the slot pointer;
// Lock hands back the staging, Unlock uploads it. The sPtr wrappers' Destroy/dtor
// route DeleteVertex/IndexBuffer here (see RenderStub.cpp).
// ---------------------------------------------------------------------------
int cSDLRenderDevice::strideFromDeclaration(IDirect3DVertexDeclaration9* decl)
{
	// Sum of element type sizes (mirrors cD3DRender::GetSizeFromDeclaration); the
	// skinned vertex (sVertexXYZINT1) is packed, so this equals its stride (36).
	if(!decl) return 0;
	int size = 0;
	for(unsigned int i = 0; i < decl->elementCount; ++i){
		switch(decl->elements[i].Type){
		case D3DDECLTYPE_FLOAT1:   size += 4;  break;
		case D3DDECLTYPE_FLOAT2:   size += 8;  break;
		case D3DDECLTYPE_FLOAT3:   size += 12; break;
		case D3DDECLTYPE_FLOAT4:   size += 16; break;
		case D3DDECLTYPE_UBYTE4:   size += 4;  break;
		case D3DDECLTYPE_D3DCOLOR: size += 4;  break;
		case D3DDECLTYPE_SHORT2:   size += 4;  break;
		case D3DDECLTYPE_SHORT4:   size += 8;  break;
		case D3DDECLTYPE_UNUSED:   break;
		default: break;
		}
	}
	return size;
}

void cSDLRenderDevice::uploadBuffer(GpuBuffer& gb)
{
	if(!device_ || !gb.buf || gb.staging.empty()) return;
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)gb.staging.size();
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	std::memcpy(map, gb.staging.data(), gb.staging.size());
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
	SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
	SDL_GPUBufferRegion dst = {}; dst.buffer = gb.buf; dst.offset = 0; dst.size = (Uint32)gb.staging.size();
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cb);
	SDL_ReleaseGPUTransferBuffer(device_, tb);
}

void cSDLRenderDevice::CreateVertexBuffer(sPtrVertexBuffer& vb, int NumberVertex,
                                          IDirect3DVertexDeclaration9* declaration, int dynamic)
{
	DeleteVertexBuffer(vb);
	int size = strideFromDeclaration(declaration);

	sSlotVB* slot = new sSlotVB();
	slot->p = 0;
	slot->init = 1;
	slot->dynamic = (char)dynamic;
	slot->declaration = declaration;
	slot->VertexSize = (short)size;
	slot->NumberVertex = NumberVertex;
	vb.ptr = slot;

	GpuBuffer gb;
	if(device_ && NumberVertex > 0 && size > 0){
		SDL_GPUBufferCreateInfo bi = {};
		bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bi.size = (Uint32)(NumberVertex * size);
		gb.buf = SDL_CreateGPUBuffer(device_, &bi);
		gb.staging.resize((size_t)NumberVertex * size);
	}
	vbGpu_[slot] = std::move(gb);
}

void cSDLRenderDevice::DeleteVertexBuffer(sPtrVertexBuffer& vb)
{
	if(!vb.IsInit()) return;
	sSlotVB& s = *vb.ptr;
	xassert(s.init > 0);
	s.init--;
	if(s.init == 0){
		auto it = vbGpu_.find(vb.ptr);
		if(it != vbGpu_.end()){
			if(it->second.buf && device_) SDL_ReleaseGPUBuffer(device_, it->second.buf);
			vbGpu_.erase(it);
		}
		delete vb.ptr;
	}
	vb.ptr = 0;
}

void* cSDLRenderDevice::LockVertexBuffer(sPtrVertexBuffer& vb, bool /*readonly*/)
{
	if(!vb.IsInit()) return nullptr;
	auto it = vbGpu_.find(vb.ptr);
	if(it == vbGpu_.end() || it->second.staging.empty()) return nullptr;
	return it->second.staging.data();
}

void cSDLRenderDevice::UnlockVertexBuffer(sPtrVertexBuffer& vb)
{
	if(!vb.IsInit()) return;
	auto it = vbGpu_.find(vb.ptr);
	if(it != vbGpu_.end()) uploadBuffer(it->second);
}

void cSDLRenderDevice::CreateIndexBuffer(sPtrIndexBuffer& ib, int NumberPolygon, int size)
{
	DeleteIndexBuffer(ib);

	sSlotIB* slot = new sSlotIB();
	slot->p = 0;
	slot->init = 1;
	slot->NumberPolygon = NumberPolygon;
	slot->PolygonSize = size;
	ib.ptr = slot;

	GpuBuffer gb;
	if(device_ && NumberPolygon > 0 && size > 0){
		SDL_GPUBufferCreateInfo bi = {};
		bi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		bi.size = (Uint32)(NumberPolygon * size);
		gb.buf = SDL_CreateGPUBuffer(device_, &bi);
		gb.staging.resize((size_t)NumberPolygon * size);
	}
	ibGpu_[slot] = std::move(gb);
}

void cSDLRenderDevice::DeleteIndexBuffer(sPtrIndexBuffer& ib)
{
	if(!ib.IsInit()) return;
	sSlotIB& s = *ib.ptr;
	xassert(s.init > 0);
	s.init--;
	if(s.init == 0){
		auto it = ibGpu_.find(ib.ptr);
		if(it != ibGpu_.end()){
			if(it->second.buf && device_) SDL_ReleaseGPUBuffer(device_, it->second.buf);
			ibGpu_.erase(it);
		}
		delete ib.ptr;
	}
	ib.ptr = 0;
}

sPolygon* cSDLRenderDevice::LockIndexBuffer(sPtrIndexBuffer& ib, bool /*readonly*/)
{
	if(!ib.IsInit()) return nullptr;
	auto it = ibGpu_.find(ib.ptr);
	if(it == ibGpu_.end() || it->second.staging.empty()) return nullptr;
	return (sPolygon*)it->second.staging.data();
}

void cSDLRenderDevice::UnlockIndexBuffer(sPtrIndexBuffer& ib)
{
	if(!ib.IsInit()) return;
	auto it = ibGpu_.find(ib.ptr);
	if(it != ibGpu_.end()) uploadBuffer(it->second);
}

#endif // !_WIN32
