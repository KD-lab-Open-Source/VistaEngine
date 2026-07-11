// SDL GPU world-quad renderer. See header.
#include "StdAfxRD.h"
#include "SDLWorldQuadRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // applyCameraViewport

// Cross-compiled world-quad shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/worldquad_shaders.h"

namespace {

// The vertex attributes below are laid out by hand at 0/12/16 with sizeof() as the
// stride, which only agrees with sVertexXYZDT1's D3D declaration while Color4c stays
// four bytes wide.
static_assert(sizeof(sVertexXYZDT1) == 24, "sVertexXYZDT1 must match its D3D declaration");

// The quads start empty and settle around the caller's steady-state count (cCoastSprites
// soft-clamps near 4000 per group); this only avoids the first few reallocations.
const int INITIAL_QUADS = 1024;

SDL_GPUTexture* sdlTextureOf(cTexture* t)
{
	return (t && t->frameNumber() >= 1) ? reinterpret_cast<SDL_GPUTexture*>(t->GetDDSurface(0)) : nullptr;
}

} // namespace

SDLWorldQuadRenderer::SDLWorldQuadRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipelines();
}

SDLWorldQuadRenderer::~SDLWorldQuadRenderer()
{
	if(!device_) return;
	if(vertexBuffer_)  SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(indexBuffer_)   SDL_ReleaseGPUBuffer(device_, indexBuffer_);
	if(whiteTexture_)  SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(sampler_)       SDL_ReleaseGPUSampler(device_, sampler_);
	if(pipelineFill_)  SDL_ReleaseGPUGraphicsPipeline(device_, pipelineFill_);
	if(pipelineLine_)  SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLine_);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLWorldQuadRenderer::createPipelines()
{
	if(!device_ || !window_) return;

	// cCoastSprites::Draw's SetSamplerDataVirtual(0, sampler_wrap_anisotropic); the wave
	// sources inherit the scene's sampler_wrap_linear, which this rounds up to. max_lod
	// must be set: it defaults to 0, which pins sampling to the top level.
	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	si.enable_anisotropy = true;
	si.max_anisotropy = 4.f;
	si.max_lod = 1000.f;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	sampler_ = SDL_CreateGPUSampler(device_, &si);

	// 1x1 white, for a group whose texture failed to load -- SetWorldMaterial binds its
	// own pWhiteTexture in the same case.
	SDL_GPUTextureCreateInfo wti = {};
	wti.type = SDL_GPU_TEXTURETYPE_2D;
	wti.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	wti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	wti.width = 1; wti.height = 1; wti.layer_count_or_depth = 1; wti.num_levels = 1;
	whiteTexture_ = SDL_CreateGPUTexture(device_, &wti);
	if(whiteTexture_){
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = 4;
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
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

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode;
	unsigned int vsSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = worldquad_vert_msl; vsSize = worldquad_vert_msl_len;
		fsCode = worldquad_frag_msl; fsSize = worldquad_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = worldquad_vert_spv; vsSize = worldquad_vert_spv_len;
		fsCode = worldquad_frag_spv; fsSize = worldquad_frag_spv_len;
	} else {
		fprintf(stderr, "SDLWorldQuadRenderer: no supported shader format (0x%x)\n", formats);
		return;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // mWVP
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;           // the bubble atlas
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);   // no uniforms

	if(!vs || !fs){
		fprintf(stderr, "SDLWorldQuadRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// sVertexXYZDT1: float3 position @0, D3DCOLOR diffuse @12, float2 uv @16 (stride 24).
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(sVertexXYZDT1);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[3] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 12;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 16;

	// RS_ZWRITEENABLE is off for every caller -- cCoastSprites::Draw forces it around both
	// sprite groups, Camera::DrawSortObject around the whole sorted pass the wave sources
	// draw in -- and SetWorldMaterial(ALPHA_BLEND, ...) picks the blend. (ONE, 1-SRC_ALPHA)
	// rather than the original's (SRC_ALPHA, 1-SRC_ALPHA) because the textures decode
	// premultiplied and the shaders keep it that way -- see worldquad.frag.hlsl.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	SDL_GPUColorTargetBlendState& bs = colorTarget.blend_state;
	bs.enable_blend = true;
	bs.color_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 3;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	// Camera::DrawObjectSpecial forces D3DCULL_NONE for the whole node type.
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = false;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineFill_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	// Wireframe variant (RS_FILLMODE == FILL_WIREFRAME). Same everything but LINE fill.
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	pipelineLine_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLWorldQuadRenderer: worldquad pipeline %s (wireframe %s)\n",
	        pipelineFill_ ? "ready" : "FAILED", pipelineLine_ ? "ready" : "FAILED");
}

// ---------------------------------------------------------------------------
// Recording -- cQuadBuffer<sVertexXYZDT1>'s contract
// ---------------------------------------------------------------------------
void SDLWorldQuadRenderer::BeginFrame()
{
	vertices_.clear();
	groups_.clear();
	drawing_ = false;
	cameraValid_ = false;
}

void SDLWorldQuadRenderer::SetCamera(Camera* camera)
{
	if(!camera) return;
	// The original's mWVP: mWorld is MatXf::ID for both sprite groups, so this is the
	// camera's view-projection alone.
	std::memcpy(vs_.mvp, &camera->matViewProj, sizeof(vs_.mvp));
	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;
	cameraValid_ = true;
}

void SDLWorldQuadRenderer::SetTexture(cTexture* texture)
{
	texture_ = sdlTextureOf(texture);
}

void SDLWorldQuadRenderer::BeginDraw()
{
	current_.texture = texture_;
	current_.firstQuad = (int)(vertices_.size() / 4);
	current_.quadCount = 0;
	drawing_ = cameraValid_;
}

sVertexXYZDT1* SDLWorldQuadRenderer::Get()
{
	// Never null: a sprite loop that ran without a camera (BeginDraw refused to open a
	// group) still writes its four vertices, and writes them here, to be discarded.
	if(!drawing_)
		return scratch_;
	// The caller fills the four vertices it is handed before asking for the next quad, so
	// a reallocation here never invalidates a pointer still in use.
	vertices_.resize(vertices_.size() + 4);
	current_.quadCount++;
	return &vertices_[vertices_.size() - 4];
}

void SDLWorldQuadRenderer::EndDraw()
{
	if(drawing_ && current_.quadCount > 0)
		groups_.push_back(current_);
	drawing_ = false;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
bool SDLWorldQuadRenderer::ensureCapacity(SDL_GPUCommandBuffer* cmd, int quads)
{
	if(quads <= capacityQuads_)
		return true;

	int capacity = capacityQuads_ ? capacityQuads_ : INITIAL_QUADS;
	while(capacity < quads) capacity *= 2;

	if(vertexBuffer_) SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(indexBuffer_)  SDL_ReleaseGPUBuffer(device_, indexBuffer_);
	vertexBuffer_ = indexBuffer_ = nullptr;
	capacityQuads_ = 0;

	const Uint32 vbytes = (Uint32)(capacity * 4 * sizeof(sVertexXYZDT1));
	const Uint32 ibytes = (Uint32)(capacity * 6 * sizeof(Uint32));

	SDL_GPUBufferCreateInfo vbi = {};
	vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX; vbi.size = vbytes;
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &vbi);

	SDL_GPUBufferCreateInfo ibi = {};
	ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX; ibi.size = ibytes;
	indexBuffer_ = SDL_CreateGPUBuffer(device_, &ibi);
	if(!vertexBuffer_ || !indexBuffer_)
		return false;

	// Two triangles per quad: (0,1,2)+(2,1,3). Every caller writes its four corners as two
	// opposite edges -- 0,1 then 2,3 -- so that pattern covers the quad for all of them,
	// and the pipeline culls nothing, so the winding is free. 32-bit indices, so the quad
	// count is bounded by the callers' own limits rather than by 65535 vertices.
	std::vector<Uint32> idx((size_t)capacity * 6);
	for(int q = 0; q < capacity; ++q){
		Uint32 base = (Uint32)(q * 4);
		Uint32* p = &idx[(size_t)q * 6];
		p[0] = base; p[1] = base + 1; p[2] = base + 2;
		p[3] = base + 2; p[4] = base + 1; p[5] = base + 3;
	}

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = ibytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return false;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	SDL_memcpy(map, idx.data(), ibytes);
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
	SDL_GPUBufferRegion dst = {}; dst.buffer = indexBuffer_; dst.offset = 0; dst.size = ibytes;
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device_, tb);   // destruction deferred until the copy runs

	capacityQuads_ = capacity;
	return true;
}

bool SDLWorldQuadRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int screenW, int screenH, bool clear, const float clearColor[4],
                                bool clearDepth, bool wireframe)
{
	// Fall back to the solid pipeline if the LINE variant failed to build.
	SDL_GPUGraphicsPipeline* pipeline = (wireframe && pipelineLine_) ? pipelineLine_ : pipelineFill_;
	if(!device_ || !pipeline || !cmd || !target || !depth || groups_.empty())
		return false;

	const int quads = (int)(vertices_.size() / 4);
	if(!ensureCapacity(cmd, quads))
		return false;

	// Upload this frame's quads. Recorded before the render pass opens, as SDL requires.
	const Uint32 vbytes = (Uint32)(vertices_.size() * sizeof(sVertexXYZDT1));
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = vbytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return false;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	SDL_memcpy(map, vertices_.data(), vbytes);
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
	SDL_GPUBufferRegion dst = {}; dst.buffer = vertexBuffer_; dst.offset = 0; dst.size = vbytes;
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device_, tb);

	SDL_GPUColorTargetInfo ct = {};
	ct.texture = target;
	ct.clear_color.r = clearColor[0];
	ct.clear_color.g = clearColor[1];
	ct.clear_color.b = clearColor[2];
	ct.clear_color.a = clearColor[3];
	ct.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	ct.store_op = SDL_GPU_STOREOP_STORE;

	SDL_GPUDepthStencilTargetInfo dt = {};
	dt.texture = depth;
	dt.clear_depth = 1.0f;
	dt.load_op = clearDepth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	dt.store_op = SDL_GPU_STOREOP_STORE;   // the passes after this one still test against it
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, &dt);

	sViewPort vp;
	vp.X = vpX_; vp.Y = vpY_; vp.Width = vpW_; vp.Height = vpH_;
	vp.MinZ = vpMinZ_; vp.MaxZ = vpMaxZ_;
	applyCameraViewport(pass, vp, screenW, screenH);

	SDL_BindGPUGraphicsPipeline(pass, pipeline);
	SDL_PushGPUVertexUniformData(cmd, 0, &vs_, sizeof(vs_));

	SDL_GPUBufferBinding vb = {}; vb.buffer = vertexBuffer_; vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
	SDL_GPUBufferBinding ib = {}; ib.buffer = indexBuffer_; ib.offset = 0;
	SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);

	// One group per BeginDraw..EndDraw run, replayed in the order the caller made them:
	// cCoastSprites' stay sprites then its moving ones, or one group per wave source.
	SDL_GPUTexture* boundTexture = nullptr;
	for(const Group& g : groups_){
		SDL_GPUTexture* texture = g.texture ? g.texture : whiteTexture_;
		if(texture != boundTexture){
			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = texture;
			ts.sampler = sampler_;
			SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
			boundTexture = texture;
		}
		SDL_DrawGPUIndexedPrimitives(pass, g.quadCount * 6, 1, g.firstQuad * 6, 0, 0);
	}

	SDL_EndGPURenderPass(pass);

	vertices_.clear();
	groups_.clear();
	return true;
}

#endif // !_WIN32
