// SDL GPU render device — slice 1b (lifecycle + window clear). See header.
#include "StdAfxRD.h"
#include "SDLRenderDevice.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cmath>
#include <cstring>

#include "Texture.h"     // cTexture (BitMap / GetDDSurface / attributes)
#include "FileImage.h"   // cFileImage::GetTexture
#include "FT_Font.h"     // FT::Font glyph atlas (OutTextLine)

// Cross-compiled UI shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/ui_shaders.h"
// Cross-compiled 3D static-mesh shader blobs (slice 3).
#include "SDLShaders/mesh3d_shaders.h"

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
	createMeshPipeline();

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

	// Release retained menu meshes while the device (and gb_RenderDevice==this) is
	// still valid: ~MenuMesh -> sPtr dtors -> DeleteVertex/IndexBuffer, which erase
	// the vbGpu_/ibGpu_ entries and release their SDL buffers.
	menuMeshes_.clear();
	meshDraws_.clear();

	if(device_){
		for(auto& kv : textures_)
			if(kv.second.tex) SDL_ReleaseGPUTexture(device_, kv.second.tex);
		textures_.clear();
		// Any buffers not owned by a MenuMesh (defensive: normally all gone above).
		for(auto& kv : vbGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		for(auto& kv : ibGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		vbGpu_.clear();
		ibGpu_.clear();
		if(meshPipeline_)   SDL_ReleaseGPUGraphicsPipeline(device_, meshPipeline_);
		if(meshPipelineAdd_) SDL_ReleaseGPUGraphicsPipeline(device_, meshPipelineAdd_);
		if(depthTexture_)   SDL_ReleaseGPUTexture(device_, depthTexture_);
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
	meshPipeline_ = nullptr; meshPipelineAdd_ = nullptr; depthTexture_ = nullptr; depthW_ = depthH_ = 0; meshPipelineTried_ = false;
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

		// A 3D mesh pass runs first (clears colour + depth, draws the static meshes
		// with depth testing); the 2D UI pass then loads that colour and draws over
		// it. The UI pipeline has no depth target, so it needs its own pass anyway.
		// Redraw the retained menu meshes through the real DrawIndexedPrimitive path,
		// which records into meshDraws_ (no GPU work yet).
		meshDraws_.clear();
		recordMenuMeshes();
		bool drawMesh = meshPipeline_ && !meshDraws_.empty();

		if(drawMesh){
			ensureDepth(xScr, yScr);
			drawMesh = depthTexture_ != nullptr;   // pipeline needs a depth target
		}

		if(drawMesh){
			SDL_GPUColorTargetInfo ct = {};
			ct.texture = swapchainTexture_;
			ct.clear_color.r = clearColor_[0];
			ct.clear_color.g = clearColor_[1];
			ct.clear_color.b = clearColor_[2];
			ct.clear_color.a = clearColor_[3];
			ct.load_op = SDL_GPU_LOADOP_CLEAR;   // mesh pass owns the frame's clear
			ct.store_op = SDL_GPU_STOREOP_STORE;

			SDL_GPUDepthStencilTargetInfo dt = {};
			dt.texture = depthTexture_;
			dt.clear_depth = 1.0f;
			dt.load_op = SDL_GPU_LOADOP_CLEAR;
			dt.store_op = SDL_GPU_STOREOP_DONT_CARE;
			dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
			dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

			SDL_GPURenderPass* mpass = SDL_BeginGPURenderPass(commandBuffer_, &ct, 1,
			                                                  depthTexture_ ? &dt : nullptr);
			flushMeshDraws(mpass);
			SDL_EndGPURenderPass(mpass);
		}

		SDL_GPUColorTargetInfo target = {};
		target.texture = swapchainTexture_;
		target.clear_color.r = clearColor_[0];
		target.clear_color.g = clearColor_[1];
		target.clear_color.b = clearColor_[2];
		target.clear_color.a = clearColor_[3];
		// If the mesh pass already cleared, load over it; otherwise honour Fill().
		target.load_op = (hasClear_ && !drawMesh) ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
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

// ---------------------------------------------------------------------------
// Text: emit one textured quad per glyph from the FreeType atlas. Mirrors the
// D3D cD3DRender::OutTextLine glyph-placement math. The atlas is a GRAY texture
// uploaded as BGRA (255,255,255,coverage), so sampling gives white with the
// coverage in alpha; the vertex colour tints it (handled by the UI shader).
// ---------------------------------------------------------------------------
int cSDLRenderDevice::OutTextLine(int x, int y, const FT::Font& font, const wchar_t* textline, const wchar_t* end,
                                  const Color4c& color, eBlendMode /*blend_mode*/, int xRangeMin, int xRangeMax)
{
	if(!bActiveScene_ || !font.texture())
		return x;

	SDL_GPUTexture* tex = sdlTextureOf(const_cast<cTexture*>(font.texture()));
	const float txWidth  = float(font.texture()->GetWidth());
	const float txHeight = float(font.texture()->GetHeight());
	if(txWidth <= 0 || txHeight <= 0)
		return x;

	const unsigned int c = (unsigned)color.b | ((unsigned)color.g << 8)
	                     | ((unsigned)color.r << 16) | ((unsigned)color.a << 24);

	int prev_rh = 0;
	int prev_right = x;
	for(const wchar_t* str = textline; str != end; ++str){
		wchar_t symbol = *str;
		if(symbol < 32)
			continue;

		const FT::OneChar& one = font.getChar(symbol);

		int advance = one.advance;
		if(prev_rh - one.lh >= 32)
			--advance;
		else if(prev_rh - one.lh < -32)
			++advance;
		prev_rh = one.rh;

		int right = x + max(advance, (int)one.su + (int)one.du);

		if(xRangeMin >= 0 && x < xRangeMin){
			prev_right = right;
			x += advance;
			continue;
		}
		if(xRangeMax >= 0 && right > xRangeMax)
			break;

		// Empty space around glyphs is compressed in the atlas, so apply the
		// per-glyph offsets (su,sv) and the glyph extent (du,dv).
		float px = float(x + one.su) - 0.5f;
		float py = float(y + one.sv) - 0.5f;
		float u  = float(one.u) / txWidth;
		float v  = float(one.v) / txHeight;
		float du = float(one.du) / txWidth;
		float dv = float(one.dv) / txHeight;
		emitQuad(px, py, float(one.du), float(one.dv), u, v, du, dv, c, tex);
		NumberPolygon += 2;

		prev_right = right;
		x += advance;
	}
	return prev_right;
}

// ---------------------------------------------------------------------------
// Slice 3: static-mesh rendering. Geometry recovered from the baked .3dxGB
// cache (see Render/3dx/MeshCacheGeometry) is uploaded to GPU vertex/index
// buffers and drawn in a depth-tested pass before the 2D UI. Only the minimal
// path: one MVP, a directional light, optional texture. No skeleton/materials.
// ---------------------------------------------------------------------------

// Small row-major, row-vector (v*M) 4x4 helpers. clip = pos * World*View*Proj,
// matching the HLSL `mul(float4(pos,1), MVP)` with `row_major float4x4 MVP`.
namespace {

struct M4 { float m[4][4]; };

M4 matIdentity()
{
	M4 r = {};
	r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.f;
	return r;
}

M4 matMul(const M4& a, const M4& b)   // (a*b)[i][j] = sum_k a[i][k]*b[k][j]
{
	M4 r = {};
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j){
			float s = 0.f;
			for(int k = 0; k < 4; ++k)
				s += a.m[i][k] * b.m[k][j];
			r.m[i][j] = s;
		}
	return r;
}

M4 matTranslate(float x, float y, float z)
{
	M4 r = matIdentity();
	r.m[3][0] = x; r.m[3][1] = y; r.m[3][2] = z;
	return r;
}

M4 matRotZ(float a)
{
	M4 r = matIdentity();
	float c = std::cos(a), s = std::sin(a);
	r.m[0][0] = c;  r.m[0][1] = s;
	r.m[1][0] = -s; r.m[1][1] = c;
	return r;
}

M4 matLookAtLH(const float eye[3], const float at[3], const float up[3])
{
	float z[3] = { at[0]-eye[0], at[1]-eye[1], at[2]-eye[2] };
	float zl = std::sqrt(z[0]*z[0]+z[1]*z[1]+z[2]*z[2]); if(zl < 1e-6f) zl = 1.f;
	z[0]/=zl; z[1]/=zl; z[2]/=zl;
	float x[3] = { up[1]*z[2]-up[2]*z[1], up[2]*z[0]-up[0]*z[2], up[0]*z[1]-up[1]*z[0] };
	float xl = std::sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]); if(xl < 1e-6f) xl = 1.f;
	x[0]/=xl; x[1]/=xl; x[2]/=xl;
	float y[3] = { z[1]*x[2]-z[2]*x[1], z[2]*x[0]-z[0]*x[2], z[0]*x[1]-z[1]*x[0] };
	M4 r = matIdentity();
	r.m[0][0]=x[0]; r.m[0][1]=y[0]; r.m[0][2]=z[0];
	r.m[1][0]=x[1]; r.m[1][1]=y[1]; r.m[1][2]=z[1];
	r.m[2][0]=x[2]; r.m[2][1]=y[2]; r.m[2][2]=z[2];
	r.m[3][0]=-(x[0]*eye[0]+x[1]*eye[1]+x[2]*eye[2]);
	r.m[3][1]=-(y[0]*eye[0]+y[1]*eye[1]+y[2]*eye[2]);
	r.m[3][2]=-(z[0]*eye[0]+z[1]*eye[1]+z[2]*eye[2]);
	return r;
}

M4 matPerspectiveFovLH(float fovY, float aspect, float zn, float zf)   // depth [0,1]
{
	float ys = 1.f / std::tan(fovY * 0.5f);
	float xs = ys / aspect;
	M4 r = {};
	r.m[0][0] = xs;
	r.m[1][1] = ys;
	r.m[2][2] = zf / (zf - zn);
	r.m[2][3] = 1.f;
	r.m[3][2] = -zn * zf / (zf - zn);
	return r;
}

} // namespace

void cSDLRenderDevice::createMeshPipeline()
{
	if(meshPipelineTried_ || !device_) return;
	meshPipelineTried_ = true;

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode;
	unsigned int vsSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = mesh3d_vert_msl; vsSize = mesh3d_vert_msl_len;
		fsCode = mesh3d_frag_msl; fsSize = mesh3d_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = mesh3d_vert_spv; vsSize = mesh3d_vert_spv_len;
		fsCode = mesh3d_frag_spv; fsSize = mesh3d_frag_spv_len;
	} else {
		fprintf(stderr, "cSDLRenderDevice: mesh pipeline: no supported shader format (0x%x)\n", formats);
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
	fsi.num_uniform_buffers = 1;   // per-submesh diffuse tint (rgb + opacity)
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "cSDLRenderDevice: mesh CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// sVertexXYZINT1, stride 36: pos float3 @0, bone u8x4 @12, normal float3 @16,
	// uv float2 @28 (bone index unused — skinning ignored).
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = 36;
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[3] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[1].offset = 16;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; attrs[2].offset = 28;

	// The model's materials are layered decals (lines/buttons over a base panel),
	// drawn in bunch order (= paint order). The DDS decoder premultiplies alpha, so
	// blend is (ONE, ONE_MINUS_SRC_ALPHA); depth write is off so coplanar layers
	// composite in paint order instead of z-fighting / showing grey halos.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	colorTarget.blend_state.enable_blend = true;
	colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
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
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = false;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	meshPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	if(!meshPipeline_){
		fprintf(stderr, "cSDLRenderDevice: mesh CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		SDL_ReleaseGPUShader(device_, vs);
		SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// Additive variant for transparencyType ADDITIVE materials (glow lines/bars):
	// premultiplied src added onto the target (ONE, ONE). Same shaders/state.
	colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	meshPipelineAdd_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	if(!meshPipelineAdd_)
		fprintf(stderr, "cSDLRenderDevice: mesh additive pipeline failed: %s\n", SDL_GetError());

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
	fprintf(stderr, "cSDLRenderDevice: mesh pipeline ready\n");
}

void cSDLRenderDevice::ensureDepth(int w, int h)
{
	if(!device_ || w <= 0 || h <= 0) return;
	if(depthTexture_ && depthW_ == w && depthH_ == h) return;
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
	// menu vertex (sVertexXYZINT1) is packed, so this equals its stride (36).
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

void cSDLRenderDevice::DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex, int nVertex,
                                            const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon)
{
	if(!vb.IsInit() || !ib.ptr || nPolygon <= 0) return;   // ib is const: use ptr directly
	auto vit = vbGpu_.find(vb.ptr);
	auto iit = ibGpu_.find(ib.ptr);
	if(vit == vbGpu_.end() || iit == ibGpu_.end() || !vit->second.buf || !iit->second.buf)
		return;

	// Record the draw with the current material/transform state; the actual GPU
	// draw happens in flushMeshDraws inside the 3D render pass (SDL GPU can only
	// draw inside a pass). D3D uses 3*nOfsPolygon / 3*nPolygon (16-bit tri list).
	MeshDraw d;
	d.vbuf = vit->second.buf;
	d.ibuf = iit->second.buf;
	d.baseVertex = OfsVertex;
	d.startIndex = 3 * nOfsPolygon;
	d.indexCount = 3 * nPolygon;
	std::memcpy(d.mvp, curMVP_, sizeof(d.mvp));
	d.tex = curMeshTexture_;
	std::memcpy(d.tint, curMeshTint_, sizeof(d.tint));
	d.transparency = curMeshTransparency_;
	meshDraws_.push_back(d);

	*PtrNumberPolygon += nPolygon;
	NumDrawObject++;
}

int cSDLRenderDevice::registerMesh(sPtrVertexBuffer& vb, sPtrIndexBuffer& ib)
{
	if(!vb.IsInit() || !ib.IsInit()) return -1;

	auto mm = std::make_unique<MenuMesh>();
	// Share the caller's buffers by adding a reference (CopyAddRef bumps the slot's
	// init count). The caller keeps its own reference, so a retained cStatic3dx can
	// still own its lod.vb/.ib; the slot frees once when both refs are released.
	mm->vb.CopyAddRef(vb);
	mm->ib.CopyAddRef(ib);
	mm->numVertex = mm->vb.GetNumberVertex();

	// Bounding box from the float3 position @0 of each vertex (staging still holds
	// the data after Unlock), for the auto-frame fallback when no MVP is supplied.
	auto vit = vbGpu_.find(mm->vb.ptr);
	const int stride = mm->vb.GetVertexSize();
	if(vit != vbGpu_.end() && !vit->second.staging.empty() && stride > 0 && mm->numVertex > 0){
		const unsigned char* p = vit->second.staging.data();
		float lo[3], hi[3];
		std::memcpy(lo, p, sizeof(lo));
		std::memcpy(hi, p, sizeof(hi));
		for(int i = 1; i < mm->numVertex; ++i){
			float pos[3];
			std::memcpy(pos, p + (size_t)i * stride, sizeof(pos));
			for(int k = 0; k < 3; ++k){
				if(pos[k] < lo[k]) lo[k] = pos[k];
				if(pos[k] > hi[k]) hi[k] = pos[k];
			}
		}
		std::memcpy(mm->bmin, lo, sizeof(lo));
		std::memcpy(mm->bmax, hi, sizeof(hi));
	}

	for(size_t i = 0; i < menuMeshes_.size(); ++i){
		if(!menuMeshes_[i]){ menuMeshes_[i] = std::move(mm); return (int)i; }
	}
	menuMeshes_.push_back(std::move(mm));
	return (int)menuMeshes_.size() - 1;
}

void cSDLRenderDevice::addMeshSubmesh(int handle, int firstIndex, int indexCount, cTexture* tex,
                                      const float* tint, int transparency)
{
	if(handle < 0 || handle >= (int)menuMeshes_.size() || !menuMeshes_[handle] || indexCount <= 0)
		return;
	MenuMesh& m = *menuMeshes_[handle];
	SubDraw sd{ firstIndex, indexCount, sdlTextureOf(tex), {1,1,1,1}, transparency };
	if(tint)
		for(int i = 0; i < 4; ++i) sd.tint[i] = tint[i];
	m.subdraws.push_back(sd);
}

void cSDLRenderDevice::setMeshTransform(int handle, const float* mvp16)
{
	if(handle < 0 || handle >= (int)menuMeshes_.size() || !menuMeshes_[handle] || !mvp16)
		return;
	std::memcpy(menuMeshes_[handle]->mvp, mvp16, 16 * sizeof(float));
	menuMeshes_[handle]->hasTransform = true;
}

void cSDLRenderDevice::releaseMesh(int handle)
{
	if(handle < 0 || handle >= (int)menuMeshes_.size()) return;
	// ~MenuMesh -> sPtr dtors -> DeleteVertex/IndexBuffer (frees the SDL buffers).
	menuMeshes_[handle].reset();
}

void cSDLRenderDevice::recordMenuMeshes()
{
	const float fovY = 50.f * 3.14159265f / 180.f;
	const float aspect = (xScr && yScr) ? float(xScr) / float(yScr) : 1.f;
	const float angle = (float)(SDL_GetTicks() % 100000) * 0.001f * 0.6f;  // ~0.6 rad/s

	for(auto& up : menuMeshes_){
		if(!up) continue;
		MenuMesh& m = *up;
		if(!m.vb.IsInit() || !m.ib.IsInit() || m.numVertex <= 0) continue;

		if(m.hasTransform){
			// Caller-supplied MVP (the real menu camera).
			std::memcpy(curMVP_, m.mvp, sizeof(curMVP_));
		} else {
			// Fallback: auto-frame the mesh (perspective, slow Z-spin).
			float center[3] = { (m.bmin[0]+m.bmax[0])*0.5f,
			                    (m.bmin[1]+m.bmax[1])*0.5f,
			                    (m.bmin[2]+m.bmax[2])*0.5f };
			float ext[3] = { m.bmax[0]-m.bmin[0], m.bmax[1]-m.bmin[1], m.bmax[2]-m.bmin[2] };
			float radius = 0.5f * std::sqrt(ext[0]*ext[0]+ext[1]*ext[1]+ext[2]*ext[2]);
			if(radius < 1e-4f) radius = 1.f;

			float d = radius / std::tan(fovY * 0.5f) * 1.4f;
			float eye[3] = { center[0], center[1] - d, center[2] };
			float up3[3] = { 0.f, 0.f, 1.f };

			M4 world = matMul(matMul(matTranslate(-center[0], -center[1], -center[2]), matRotZ(angle)),
			                  matTranslate(center[0], center[1], center[2]));
			M4 view  = matLookAtLH(eye, center, up3);
			M4 proj  = matPerspectiveFovLH(fovY, aspect, std::max(0.05f, d - radius*1.5f), d + radius*1.5f);
			M4 mvp   = matMul(matMul(world, view), proj);
			std::memcpy(curMVP_, mvp.m, sizeof(curMVP_));
		}

		if(m.subdraws.empty()){
			// Whole-mesh draw, untextured/white (nPolygon = whole index buffer).
			curMeshTexture_ = nullptr;
			curMeshTint_[0] = curMeshTint_[1] = curMeshTint_[2] = curMeshTint_[3] = 1.f;
			curMeshTransparency_ = 2;
			DrawIndexedPrimitive(m.vb, 0, m.numVertex, m.ib, 0, m.ib.GetNumberPolygon());
		} else {
			// One draw per material range: firstIndex/indexCount are index counts,
			// so convert to polygon offset/count for DrawIndexedPrimitive.
			for(const SubDraw& sd : m.subdraws){
				curMeshTexture_ = sd.tex;
				std::memcpy(curMeshTint_, sd.tint, sizeof(curMeshTint_));
				curMeshTransparency_ = sd.transparency;
				DrawIndexedPrimitive(m.vb, 0, m.numVertex, m.ib,
				                     sd.firstIndex / 3, sd.indexCount / 3);
			}
		}
	}
}

void cSDLRenderDevice::flushMeshDraws(SDL_GPURenderPass* pass)
{
	if(!pass || !meshPipeline_) return;

	SDL_GPUGraphicsPipeline* boundPipeline = nullptr;  // track to avoid redundant binds
	for(const MeshDraw& d : meshDraws_){
		// Additive (1) -> additive blend; everything else -> filter (alpha-over).
		SDL_GPUGraphicsPipeline* want = (d.transparency == 1 && meshPipelineAdd_)
		                                ? meshPipelineAdd_ : meshPipeline_;
		if(want != boundPipeline){ SDL_BindGPUGraphicsPipeline(pass, want); boundPipeline = want; }

		SDL_PushGPUVertexUniformData(commandBuffer_, 0, d.mvp, sizeof(d.mvp));
		SDL_PushGPUFragmentUniformData(commandBuffer_, 0, d.tint, sizeof(d.tint));

		SDL_GPUBufferBinding vbb = {}; vbb.buffer = d.vbuf; vbb.offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, &vbb, 1);
		SDL_GPUBufferBinding ibb = {}; ibb.buffer = d.ibuf; ibb.offset = 0;
		SDL_BindGPUIndexBuffer(pass, &ibb, SDL_GPU_INDEXELEMENTSIZE_16BIT);

		SDL_GPUTextureSamplerBinding ts = {};
		ts.texture = d.tex ? d.tex : whiteTexture_;
		ts.sampler = sampler_;
		SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);

		SDL_DrawGPUIndexedPrimitives(pass, d.indexCount, 1, d.startIndex, d.baseVertex, 0);
	}
}

#endif // !_WIN32
