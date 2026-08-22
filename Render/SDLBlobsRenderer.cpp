// SDL GPU metaball ("blobs") renderer for the logo splash. See header.
#include "StdAfxRD.h"
#include "SDLBlobsRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>

#include "Texture.h"           // cTexture::GetDDSurface
#include "SDLRenderDevice.h"   // createSolidGPUTexture

// Blobs shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt. The cell pass reuses the UI shaders verbatim.
#include "SDLShaders/blobs_shaders.h"
#include "SDLShaders/ShaderBlob.h"

SDLBlobsRenderer::SDLBlobsRenderer(SDL_GPUDevice* device, SDL_Window* window)
: device_(device), window_(window)
{
	createSamplers();
	whiteTexture_ = createSolidGPUTexture(device_, 0xffffffffu);
	if(createShaders())
		// Shaders only; the pipelines are built on demand. See SDLGrassRenderer.
		fprintf(stderr, "SDLBlobsRenderer: blobs shaders ready\n");
}

SDLBlobsRenderer::~SDLBlobsRenderer()
{
	if(!device_)
		return;
	if(cellPipeline_)      SDL_ReleaseGPUGraphicsPipeline(device_, cellPipeline_);
	if(compositePipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, compositePipeline_);
	if(cellVS_)      SDL_ReleaseGPUShader(device_, cellVS_);
	if(cellFS_)      SDL_ReleaseGPUShader(device_, cellFS_);
	if(compositeVS_) SDL_ReleaseGPUShader(device_, compositeVS_);
	if(compositeFS_) SDL_ReleaseGPUShader(device_, compositeFS_);
	if(samplerClampPoint_)  SDL_ReleaseGPUSampler(device_, samplerClampPoint_);
	if(samplerClampLinear_) SDL_ReleaseGPUSampler(device_, samplerClampLinear_);
	if(whiteTexture_)   SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(fieldTexture_)   SDL_ReleaseGPUTexture(device_, fieldTexture_);
	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
}

void SDLBlobsRenderer::createSamplers()
{
	if(!device_)
		return;

	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_NEAREST;
	si.mag_filter = SDL_GPU_FILTER_NEAREST;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerClampPoint_ = SDL_CreateGPUSampler(device_, &si);

	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	samplerClampLinear_ = SDL_CreateGPUSampler(device_, &si);
}

bool SDLBlobsRenderer::createShaders()
{
	if(shadersTried_)
		return cellVS_ && cellFS_ && compositeVS_ && compositeFS_;
	shadersTried_ = true;
	if(!device_ || !window_)
		return false;

	// The cell quad: ui.vert.hlsl's one vertex uniform is (1/width, 1/height) of the
	// target it draws into -- here the field, not the screen.
	SDL_GPUShaderCreateInfo cvi = vista::shaderCreateInfo(VISTA_SHADER(blobs_cell_vert));
	cvi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	cvi.num_uniform_buffers = 1;
	cellVS_ = SDL_CreateGPUShader(device_, &cvi);

	SDL_GPUShaderCreateInfo cfi = vista::shaderCreateInfo(VISTA_SHADER(blobs_cell_frag));
	cfi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	cfi.num_samplers = 1;
	cellFS_ = SDL_CreateGPUShader(device_, &cfi);

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(blobs_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;   // SV_VertexID only: no buffers, no uniforms
	compositeVS_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(blobs_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 2;                     // the field and the frame
	fsi.num_uniform_buffers = 1;
	compositeFS_ = SDL_CreateGPUShader(device_, &fsi);

	if(!cellVS_ || !cellFS_ || !compositeVS_ || !compositeFS_){
		fprintf(stderr, "SDLBlobsRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

bool SDLBlobsRenderer::createPipelines()
{
	if(cellPipeline_ && compositePipeline_)
		return true;
	if(!createShaders())
		return false;

	const SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

	if(!cellPipeline_){
		SDL_GPUVertexBufferDescription vbd = {};
		vbd.slot = 0;
		vbd.pitch = sizeof(CellVertex);
		vbd.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexAttribute attrs[3] = {};
		attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;      attrs[0].offset = 0;
		attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 16;
		attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 20;

		// ALPHA_ADDBLEND: (ONE, ONE, ADD), what cBlobs::EndDraw set. The cells SUM -- that
		// is what makes the field a metaball field rather than a pile of sprites.
		SDL_GPUColorTargetDescription ctd = {};
		ctd.format = format;
		ctd.blend_state.enable_blend = true;
		ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		ctd.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		ctd.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		ctd.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

		SDL_GPUGraphicsPipelineCreateInfo pi = {};
		pi.vertex_shader = cellVS_;
		pi.fragment_shader = cellFS_;
		pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		pi.vertex_input_state.vertex_buffer_descriptions = &vbd;
		pi.vertex_input_state.num_vertex_buffers = 1;
		pi.vertex_input_state.vertex_attributes = attrs;
		pi.vertex_input_state.num_vertex_attributes = 3;
		pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
		pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pi.rasterizer_state.enable_depth_clip = true;
		pi.target_info.color_target_descriptions = &ctd;
		pi.target_info.num_color_targets = 1;
		pi.target_info.has_depth_stencil_target = false;

		cellPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	}

	if(!compositePipeline_){
		// ALPHA_NONE, as cBlobs::DrawBlobsShader set: the composite covers every pixel of
		// the swapchain and owns all of them.
		SDL_GPUColorTargetDescription ctd = {};
		ctd.format = format;
		ctd.blend_state.enable_blend = false;

		SDL_GPUGraphicsPipelineCreateInfo pi = {};
		pi.vertex_shader = compositeVS_;
		pi.fragment_shader = compositeFS_;
		pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
		pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pi.rasterizer_state.enable_depth_clip = true;
		pi.target_info.color_target_descriptions = &ctd;
		pi.target_info.num_color_targets = 1;
		pi.target_info.has_depth_stencil_target = false;

		compositePipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	}

	if(!cellPipeline_ || !compositePipeline_){
		fprintf(stderr, "SDLBlobsRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

SDL_GPUTexture* SDLBlobsRenderer::ensureField(int w, int h)
{
	if(fieldTexture_ && fieldW_ == w && fieldH_ == h)
		return fieldTexture_;
	if(fieldTexture_){
		SDL_ReleaseGPUTexture(device_, fieldTexture_);
		fieldTexture_ = nullptr;
	}

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	fieldTexture_ = SDL_CreateGPUTexture(device_, &ti);
	fieldW_ = fieldTexture_ ? w : 0;
	fieldH_ = fieldTexture_ ? h : 0;
	return fieldTexture_;
}

void SDLBlobsRenderer::ensureVertexCapacity(int verts)
{
	if(verts <= vertexCapacity_)
		return;
	int cap = vertexCapacity_ ? vertexCapacity_ : 1024;
	while(cap < verts) cap *= 2;

	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUBufferCreateInfo bi = {};
	bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bi.size = (Uint32)(cap * sizeof(CellVertex));
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bi);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)(cap * sizeof(CellVertex));
	transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &tbi);

	vertexCapacity_ = (vertexBuffer_ && transferBuffer_) ? cap : 0;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLBlobsRenderer::BeginFrame()
{
	cells_.clear();
	cellTexture_ = nullptr;
	compositeArmed_ = false;
}

void SDLBlobsRenderer::BeginCells()
{
	cells_.clear();
}

void SDLBlobsRenderer::AddCell(float x, float y, float phase)
{
	// cBlobs::EndDraw built the quad's diffuse as white * phase, in all four channels
	// (Color4c::operator*=(float) scales the alpha too). The cell texture is grey, so the
	// field ends up holding sum(footprint * phase) -- which is what blobs.psl reads as .x.
	const int c = (int)(255.f * phase + 0.5f);
	const unsigned int level = (unsigned int)(c < 0 ? 0 : c > 255 ? 255 : c);
	const unsigned int color = level | (level << 8) | (level << 16) | (level << 24);

	// The cell is centred on the point, and its footprint is the whole texture. The size
	// comes from the texture at draw time -- it is not known until EndCells -- so record
	// the centre and expand there.
	cells_.push_back(CellVertex{ x, y, 0.f, 1.f, color, 0.f, 0.f });
}

void SDLBlobsRenderer::EndCells(cTexture* cellTexture)
{
	cellTexture_ = cellTexture
	             ? reinterpret_cast<SDL_GPUTexture*>(cellTexture->GetDDSurface(0))
	             : nullptr;

	// Expand each recorded centre into two triangles the size of the footprint texture,
	// exactly as cBlobs::EndDraw filled its quad buffer. The D3D vertices carried the
	// half-texel offset every pre-transformed vertex needed; the SDL pixel-to-NDC mapping
	// (ui.vert.hlsl) puts pixel p on the pixel's own edge, so there is nothing to correct.
	const int dx = cellTexture ? cellTexture->GetWidth() : 0;
	const int dy = cellTexture ? cellTexture->GetHeight() : 0;
	if(dx <= 0 || dy <= 0){
		cells_.clear();
		return;
	}
	// Integer halves, as the original's `int dx2=dx/2` took them.
	const int halfX = dx / 2, halfY = dy / 2;
	const float dx2 = (float)halfX, dy2 = (float)halfY;

	std::vector<CellVertex> quads;
	quads.reserve(cells_.size() * 6);
	for(const CellVertex& c : cells_){
		const float x1 = c.x - dx2, y1 = c.y - dy2;
		const float x2 = c.x + dx2, y2 = c.y + dy2;
		const CellVertex tl = { x1, y1, 0.f, 1.f, c.color, 0.f, 0.f };
		const CellVertex tr = { x2, y1, 0.f, 1.f, c.color, 1.f, 0.f };
		const CellVertex bl = { x1, y2, 0.f, 1.f, c.color, 0.f, 1.f };
		const CellVertex br = { x2, y2, 0.f, 1.f, c.color, 1.f, 1.f };
		quads.push_back(tl); quads.push_back(bl); quads.push_back(tr);
		quads.push_back(tr); quads.push_back(bl); quads.push_back(br);
	}
	cells_.swap(quads);
}

void SDLBlobsRenderer::recordComposite(float fadePhase, const Color4f& color,
                                       const Color4f& specular)
{
	// PSBlobsShader::Select. pixel_size is filled in at draw time, when the field's size
	// is settled; the rest is the caller's cBlobsSetting.
	fs_.color[0] = color.r;
	fs_.color[1] = color.g;
	fs_.color[2] = color.b;
	fs_.color[3] = color.a;
	fs_.specular[0] = specular.r;
	fs_.specular[1] = specular.g;
	fs_.specular[2] = specular.b;
	// Select built spec_color as (r, g, b, 1.0f) -- the setting's own alpha is not used.
	fs_.specular[3] = 1.f;
	fs_.fadePhase[0] = fadePhase;
	compositeArmed_ = true;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
bool SDLBlobsRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* scene,
                            SDL_GPUTexture* target, int w, int h)
{
	const bool armed = compositeArmed_;
	std::vector<CellVertex> cells;
	cells.swap(cells_);
	SDL_GPUTexture* cellTexture = cellTexture_;
	compositeArmed_ = false;
	cellTexture_ = nullptr;

	if(!device_ || !cmd || !scene || !target || !armed || w <= 0 || h <= 0)
		return false;
	if(!createPipelines())
		return false;

	SDL_GPUTexture* field = ensureField(w, h);
	if(!field)
		return false;

	// Pass 1: the field. The cells' vertices go up first -- a copy pass cannot run inside
	// a render pass.
	const int vcount = (int)cells.size();
	if(vcount > 0){
		ensureVertexCapacity(vcount);
		if(vertexBuffer_ && transferBuffer_){
			void* map = SDL_MapGPUTransferBuffer(device_, transferBuffer_, true);
			SDL_memcpy(map, cells.data(), vcount * sizeof(CellVertex));
			SDL_UnmapGPUTransferBuffer(device_, transferBuffer_);

			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
			SDL_GPUTransferBufferLocation src = {};
			src.transfer_buffer = transferBuffer_;
			src.offset = 0;
			SDL_GPUBufferRegion dst = {};
			dst.buffer = vertexBuffer_;
			dst.offset = 0;
			dst.size = (Uint32)(vcount * sizeof(CellVertex));
			SDL_UploadToGPUBuffer(copy, &src, &dst, true);
			SDL_EndGPUCopyPass(copy);
		}
	}

	{
		// Opened even with no cells: this pass carries the field's clear to black, which is
		// cBlobs::BeginDraw's `Clear(D3DCLEAR_TARGET, 0)`, and the composite must not read
		// the previous frame's field.
		SDL_GPUColorTargetInfo ct = {};
		ct.texture = field;
		ct.clear_color.r = 0.f; ct.clear_color.g = 0.f;
		ct.clear_color.b = 0.f; ct.clear_color.a = 0.f;
		ct.load_op = SDL_GPU_LOADOP_CLEAR;
		ct.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, nullptr);
		if(vcount > 0 && vertexBuffer_){
			SDL_BindGPUGraphicsPipeline(pass, cellPipeline_);
			// ui.vert.hlsl's inverse target size -- the FIELD's, which the cells' pixel
			// coordinates are measured in.
			const float invField[4] = { 1.f / w, 1.f / h, 0.f, 0.f };
			SDL_PushGPUVertexUniformData(cmd, 0, invField, sizeof(invField));

			SDL_GPUBufferBinding vb = {};
			vb.buffer = vertexBuffer_;
			vb.offset = 0;
			SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = cellTexture ? cellTexture : whiteTexture_;
			ts.sampler = samplerClampPoint_;
			SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);

			SDL_DrawGPUPrimitives(pass, vcount, 1, 0, 0);
		}
		SDL_EndGPURenderPass(pass);
	}

	// Pass 2: the composite, over the whole swapchain image.
	{
		fs_.pixelSize[0] = 1.f / w;
		fs_.pixelSize[1] = 1.f / h;
		fs_.pixelSize[2] = 0.f;
		fs_.pixelSize[3] = 0.f;

		SDL_GPUColorTargetInfo ct = {};
		ct.texture = target;
		// The triangle covers every pixel of the target; nothing is loaded through it.
		ct.load_op = SDL_GPU_LOADOP_DONT_CARE;
		ct.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, nullptr);
		SDL_BindGPUGraphicsPipeline(pass, compositePipeline_);
		SDL_PushGPUFragmentUniformData(cmd, 0, &fs_, sizeof(fs_));

		SDL_GPUTextureSamplerBinding ts[2] = {};
		ts[0].texture = field; ts[0].sampler = samplerClampPoint_;
		ts[1].texture = scene; ts[1].sampler = samplerClampLinear_;
		SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);

		SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
		SDL_EndGPURenderPass(pass);
	}

	return true;
}
