// SDL GPU render device — slice 1b (lifecycle + window clear). See header.
#include "StdAfxRD.h"
#include "SDLRenderDevice.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>

#include "Texture.h"     // cTexture (BitMap / GetDDSurface / attributes)
#include "FileImage.h"   // cFileImage::GetTexture

// Cross-compiled UI shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/ui_shaders.h"

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

	createUIPipeline();
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

	if(device_){
		for(auto& kv : textures_)
			if(kv.second.tex) SDL_ReleaseGPUTexture(device_, kv.second.tex);
		textures_.clear();
		if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
		if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
		if(whiteTexture_)   SDL_ReleaseGPUTexture(device_, whiteTexture_);
		if(sampler_)        SDL_ReleaseGPUSampler(device_, sampler_);
		if(uiPipeline_)     SDL_ReleaseGPUGraphicsPipeline(device_, uiPipeline_);
		if(window_)
			SDL_ReleaseWindowFromGPUDevice(device_, window_);
		SDL_DestroyGPUDevice(device_);
		device_ = nullptr;
	}
	vertexBuffer_ = nullptr; transferBuffer_ = nullptr; whiteTexture_ = nullptr;
	sampler_ = nullptr; uiPipeline_ = nullptr; vertexCapacity_ = 0; pipelineTried_ = false;
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

	// Acquire the frame's command buffer + swapchain texture. The render pass is
	// deferred to EndScene: 2D draws accumulate into a CPU batch during the scene
	// and are uploaded in a copy pass (which must be outside any render pass)
	// before the single render pass that clears and draws them.
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

	batch_.clear();
	runs_.clear();
	currentTexture_ = nullptr;
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

	if(swapchainTexture_ && commandBuffer_){
		const int vcount = (int)batch_.size();

		// Upload the accumulated quads (copy pass, outside the render pass).
		if(vcount > 0 && uiPipeline_){
			ensureVertexCapacity(vcount);
			if(vertexBuffer_ && transferBuffer_){
				void* map = SDL_MapGPUTransferBuffer(device_, transferBuffer_, true);
				SDL_memcpy(map, batch_.data(), vcount * sizeof(UIVertex));
				SDL_UnmapGPUTransferBuffer(device_, transferBuffer_);

				SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commandBuffer_);
				SDL_GPUTransferBufferLocation src = {};
				src.transfer_buffer = transferBuffer_;
				src.offset = 0;
				SDL_GPUBufferRegion dst = {};
				dst.buffer = vertexBuffer_;
				dst.offset = 0;
				dst.size = (Uint32)(vcount * sizeof(UIVertex));
				SDL_UploadToGPUBuffer(copy, &src, &dst, true);
				SDL_EndGPUCopyPass(copy);
			}
		}

		SDL_GPUColorTargetInfo target = {};
		target.texture = swapchainTexture_;
		target.clear_color.r = clearColor_[0];
		target.clear_color.g = clearColor_[1];
		target.clear_color.b = clearColor_[2];
		target.clear_color.a = clearColor_[3];
		target.load_op = hasClear_ ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		target.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer_, &target, 1, nullptr);
		if(vcount > 0 && uiPipeline_ && vertexBuffer_){
			SDL_BindGPUGraphicsPipeline(pass, uiPipeline_);
			float invScreen[4] = { xScr ? 1.f / xScr : 0.f, yScr ? 1.f / yScr : 0.f, 0.f, 0.f };
			SDL_PushGPUVertexUniformData(commandBuffer_, 0, invScreen, sizeof(invScreen));
			SDL_GPUBufferBinding vb = {};
			vb.buffer = vertexBuffer_;
			vb.offset = 0;
			SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
			// One draw per run; bind the run's texture (white for untextured quads).
			for(const DrawRun& run : runs_){
				SDL_GPUTextureSamplerBinding ts = {};
				ts.texture = run.tex ? run.tex : whiteTexture_;
				ts.sampler = sampler_;
				SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
				SDL_DrawGPUPrimitives(pass, run.count, 1, run.first, 0);
			}
		}
		SDL_EndGPURenderPass(pass);
	}

	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;
	hasClear_ = false;
	return 0;
}

int cSDLRenderDevice::Flush()
{
	if(bActiveScene_)
		EndScene();
	return 0;
}

// ---------------------------------------------------------------------------
// UI pipeline
// ---------------------------------------------------------------------------
void cSDLRenderDevice::createUIPipeline()
{
	if(pipelineTried_ || !device_) return;
	pipelineTried_ = true;

	// Pick a shader format the backend supports (Metal->MSL, Vulkan->SPIRV).
	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode;
	unsigned int vsSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = ui_vert_msl; vsSize = ui_vert_msl_len;
		fsCode = ui_frag_msl; fsSize = ui_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = ui_vert_spv; vsSize = ui_vert_spv_len;
		fsCode = ui_frag_spv; fsSize = ui_frag_spv_len;
	} else {
		fprintf(stderr, "cSDLRenderDevice: no supported shader format (0x%x)\n", formats);
		return;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "cSDLRenderDevice: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(UIVertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[3] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 16;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 20;

	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	colorTarget.blend_state.enable_blend = true;
	colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 3;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;

	uiPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
	if(!uiPipeline_){
		fprintf(stderr, "cSDLRenderDevice: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		return;
	}

	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_ = SDL_CreateGPUSampler(device_, &si);

	// 1x1 white texture so DrawSprite (slice 2a) shows the vertex colour.
	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = 1; ti.height = 1; ti.layer_count_or_depth = 1; ti.num_levels = 1;
	whiteTexture_ = SDL_CreateGPUTexture(device_, &ti);
	if(whiteTexture_){
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = 4;
		SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
		if(tb){
			unsigned int* px = (unsigned int*)SDL_MapGPUTransferBuffer(device_, tb, false);
			*px = 0xFFFFFFFFu;
			SDL_UnmapGPUTransferBuffer(device_, tb);
			SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
			SDL_GPUTextureTransferInfo src = {};
			src.transfer_buffer = tb; src.offset = 0;
			SDL_GPUTextureRegion dst = {};
			dst.texture = whiteTexture_; dst.w = 1; dst.h = 1; dst.d = 1;
			SDL_UploadToGPUTexture(copy, &src, &dst, false);
			SDL_EndGPUCopyPass(copy);
			SDL_SubmitGPUCommandBuffer(cb);
			SDL_ReleaseGPUTransferBuffer(device_, tb);
		}
	}

	fprintf(stderr, "cSDLRenderDevice: UI pipeline ready\n");
}

void cSDLRenderDevice::ensureVertexCapacity(int verts)
{
	if(verts <= vertexCapacity_) return;
	int cap = vertexCapacity_ ? vertexCapacity_ : 1024;
	while(cap < verts) cap *= 2;

	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUBufferCreateInfo bi = {};
	bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bi.size = (Uint32)(cap * sizeof(UIVertex));
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bi);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)(cap * sizeof(UIVertex));
	transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &tbi);

	vertexCapacity_ = (vertexBuffer_ && transferBuffer_) ? cap : 0;
}

void cSDLRenderDevice::emitQuad(float x, float y, float dx, float dy,
                                float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex)
{
	// Extend the current run if it uses the same texture, else start a new one.
	if(runs_.empty() || runs_.back().tex != tex)
		runs_.push_back(DrawRun{ tex, (int)batch_.size(), 0 });

	// Two triangles (the D3D path used a tri-strip of 4 verts; here, 6 verts).
	const float x1 = x, y1 = y, x2 = x + dx, y2 = y + dy;
	const UIVertex tl = { x1, y1, 0, 1, color, u,      v      };
	const UIVertex tr = { x2, y1, 0, 1, color, u + du, v      };
	const UIVertex bl = { x1, y2, 0, 1, color, u,      v + dv };
	const UIVertex br = { x2, y2, 0, 1, color, u + du, v + dv };
	batch_.push_back(tl); batch_.push_back(bl); batch_.push_back(tr);
	batch_.push_back(tr); batch_.push_back(bl); batch_.push_back(br);
	runs_.back().count += 6;
}

// Look up the SDL texture a cTexture is backed by (null => untextured/white).
static SDL_GPUTexture* sdlTextureOf(cTexture* t)
{
	if(t && t->frameNumber() >= 1)
		return reinterpret_cast<SDL_GPUTexture*>(t->GetDDSurface(0));
	return nullptr;
}

void cSDLRenderDevice::SetNoMaterial(eBlendMode /*blend*/, const MatXf&, float /*phase*/,
                                     cTexture* Texture0, cTexture* /*Texture1*/, eColorMode /*mode*/)
{
	// Record the texture for the following DrawQuad calls. (Blend state is baked
	// into the single alpha pipeline for now; per-blend pipelines come later.)
	currentTexture_ = sdlTextureOf(Texture0);
}

void cSDLRenderDevice::DrawQuad(float x1, float y1, float dx, float dy,
                                float u1, float v1, float du, float dv, Color4c color)
{
	if(!bActiveScene_) return;
	unsigned int c = (unsigned)color.b | ((unsigned)color.g << 8)
	               | ((unsigned)color.r << 16) | ((unsigned)color.a << 24);
	emitQuad(x1, y1, dx, dy, u1, v1, du, dv, c, currentTexture_);
	NumberPolygon += 2;
}

// ---------------------------------------------------------------------------
// Textures (slice 2b): real SDL GPU textures backed by a lockable CPU staging
// image. The SDL_GPUTexture* is handed to cTexture via BitMap[0] (the fake
// IDirect3DTexture9 Release() is a no-op, so storing a non-D3D pointer is safe);
// TextureData (staging + dims) is kept here, keyed by that handle.
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
	if(td.expand){
		const int n = td.w * td.h;
		for(int i = 0; i < n; ++i){
			map[i*4+0] = 255; map[i*4+1] = 255; map[i*4+2] = 255;  // B,G,R
			map[i*4+3] = td.staging[i];                            // A = coverage
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
	const int bpp = gray ? 1 : 4;

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
	td.expand = gray;
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

void cSDLRenderDevice::DrawSprite(int x, int y, int dx, int dy,
                                  float u, float v, float du, float dv,
                                  cTexture* Texture, const Color4c& ColorMul,
                                  float /*phase*/, eBlendMode /*mode*/, float /*saturate*/)
{
	if(!bActiveScene_) return;
	// Pack BGRA bytes (matches UBYTE4_NORM read order; the shader swizzles to RGBA).
	unsigned int color = (unsigned)ColorMul.b | ((unsigned)ColorMul.g << 8)
	                   | ((unsigned)ColorMul.r << 16) | ((unsigned)ColorMul.a << 24);
	emitQuad((float)x, (float)y, (float)dx, (float)dy, u, v, du, dv, color, sdlTextureOf(Texture));
	NumberPolygon += 2;
}

#endif // !_WIN32
