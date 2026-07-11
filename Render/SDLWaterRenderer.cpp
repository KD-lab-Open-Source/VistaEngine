// SDL GPU water renderer. See header.
#include "StdAfxRD.h"
#include "SDLWaterRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // owner: resolves the sPtr buffers, applyCameraViewport

// Cross-compiled water shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/water_shaders.h"

namespace {

// cWater's surface vertex is sVertexXYZD: position float3 @0, D3DCOLOR diffuse @12.
const int WATER_STRIDE = 16;

SDL_GPUTexture* sdlTextureOf(cTexture* t)
{
	return (t && t->frameNumber() >= 1) ? reinterpret_cast<SDL_GPUTexture*>(t->GetDDSurface(0)) : nullptr;
}

} // namespace

SDLWaterRenderer::SDLWaterRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window)
	: owner_(owner), device_(device), window_(window)
{
	createPipelines();
}

SDLWaterRenderer::~SDLWaterRenderer()
{
	if(!device_) return;
	if(flatTexture_)  SDL_ReleaseGPUTexture(device_, flatTexture_);
	if(sampler_)      SDL_ReleaseGPUSampler(device_, sampler_);
	if(pipelineFill_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineFill_);
	if(pipelineLine_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLine_);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLWaterRenderer::createPipelines()
{
	if(!device_ || !window_) return;

	// cWater::Draw's SetSamplerData(0|1, sampler_wrap_anisotropic). max_lod must be set:
	// it defaults to 0, which pins sampling to the top level however many the texture has.
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

	// 1x1 flat wave map: the decoder's bias, so water.frag.hlsl's slope() reads 0.
	SDL_GPUTextureCreateInfo fti = {};
	fti.type = SDL_GPU_TEXTURETYPE_2D;
	fti.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	fti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	fti.width = 1; fti.height = 1; fti.layer_count_or_depth = 1; fti.num_levels = 1;
	flatTexture_ = SDL_CreateGPUTexture(device_, &fti);
	if(flatTexture_){
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = 4;
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
			unsigned char* px = (unsigned char*)SDL_MapGPUTransferBuffer(device_, tb, false);
			px[0] = 255; px[1] = 128; px[2] = 128; px[3] = 255;   // B,G,R,A
			SDL_UnmapGPUTransferBuffer(device_, tb);
			SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
			SDL_GPUTextureTransferInfo src = {};
			src.transfer_buffer = tb; src.offset = 0;
			SDL_GPUTextureRegion dst = {};
			dst.texture = flatTexture_; dst.w = 1; dst.h = 1; dst.d = 1;
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
		vsCode = water_vert_msl; vsSize = water_vert_msl_len;
		fsCode = water_frag_msl; fsSize = water_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = water_vert_spv; vsSize = water_vert_spv_len;
		fsCode = water_frag_spv; fsSize = water_frag_spv_len;
	} else {
		fprintf(stderr, "SDLWaterRenderer: no supported shader format (0x%x)\n", formats);
		return;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + the two uv scale/offsets
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 2;           // the two wave maps
	fsi.num_uniform_buffers = 1;    // vPS11Color
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLWaterRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = WATER_STRIDE;
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[2] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 12;

	// cWater::Draw's SetNoMaterial(ALPHA_BLEND, ...) with RS_ZWRITEENABLE forced off:
	// the surface blends over the scene and leaves the depth it tests against alone, so
	// what is drawn after it (the sorted transparent objects) still sees the terrain's.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	SDL_GPUColorTargetBlendState& bs = colorTarget.blend_state;
	bs.enable_blend = true;
	bs.color_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 2;
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

	fprintf(stderr, "SDLWaterRenderer: water pipeline %s (wireframe %s)\n",
	        pipelineFill_ ? "ready" : "FAILED", pipelineLine_ ? "ready" : "FAILED");
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLWaterRenderer::BeginFrame()
{
	draws_.clear();
	stateValid_ = false;
}

void SDLWaterRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera) return;

	std::memcpy(vs_.mvp, &camera->matViewProj, sizeof(vs_.mvp));
	std::memcpy(vs_.uvScaleOffset, state.uvScaleOffset, sizeof(vs_.uvScaleOffset));
	std::memcpy(vs_.uvScaleOffset1, state.uvScaleOffset1, sizeof(vs_.uvScaleOffset1));
	std::memcpy(fs_.ps11Color, state.ps11Color, sizeof(fs_.ps11Color));

	texture0_ = sdlTextureOf(state.texture0);
	texture1_ = sdlTextureOf(state.texture1);

	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;

	stateValid_ = true;
}

void SDLWaterRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex,
                                            const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon)
{
	if(!owner_ || !stateValid_ || nPolygon <= 0)
		return;

	SDL_GPUBuffer* vertexBuffer = owner_->gpuBuffer(vb);
	SDL_GPUBuffer* indexBuffer = owner_->gpuBuffer(ib);
	if(!vertexBuffer || !indexBuffer || vb.GetVertexSize() != WATER_STRIDE)
		return;

	// D3D takes OfsVertex as MinVertexIndex, a range hint it adds to nothing; SDL's
	// vertex_offset *is* added to every index. cWater::DrawPolygons passes 0 anyway --
	// each of its (up to two) vertex buffers is indexed from its own base.
	(void)OfsVertex;

	DrawCmd d;
	d.vertexBuffer = vertexBuffer;
	d.indexBuffer = indexBuffer;
	// The index buffer is a 16-bit triangle list, so D3D's polygon offsets are 3x index
	// offsets -- the same arithmetic cD3DRender::DrawIndexedPrimitive hands to D3D.
	d.firstIndex = 3 * nOfsPolygon;
	d.indexCount = 3 * nPolygon;
	draws_.push_back(d);

	*gb_RenderDevice->PtrNumberPolygon += nPolygon;
	gb_RenderDevice->NumDrawObject++;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
bool SDLWaterRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                            int screenW, int screenH, bool clear, const float clearColor[4],
                            bool clearDepth, bool wireframe)
{
	// Fall back to the solid pipeline if the LINE variant failed to build.
	SDL_GPUGraphicsPipeline* pipeline = (wireframe && pipelineLine_) ? pipelineLine_ : pipelineFill_;
	if(!device_ || !pipeline || !cmd || !target || !depth || draws_.empty())
		return false;

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
	SDL_PushGPUFragmentUniformData(cmd, 0, &fs_, sizeof(fs_));

	SDL_GPUTextureSamplerBinding ts[2] = {};
	ts[0].texture = texture0_ ? texture0_ : flatTexture_;
	ts[0].sampler = sampler_;
	ts[1].texture = texture1_ ? texture1_ : flatTexture_;
	ts[1].sampler = sampler_;
	SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);

	// The surface is drawn as a run of tile ranges over (at most two) vertex buffers,
	// exactly as cWater::DrawPolygons walks its visible lines.
	SDL_GPUBuffer* boundVB = nullptr;
	SDL_GPUBuffer* boundIB = nullptr;
	for(const DrawCmd& d : draws_){
		if(d.vertexBuffer != boundVB){
			SDL_GPUBufferBinding vb = {}; vb.buffer = d.vertexBuffer; vb.offset = 0;
			SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
			boundVB = d.vertexBuffer;
		}
		if(d.indexBuffer != boundIB){
			SDL_GPUBufferBinding ib = {}; ib.buffer = d.indexBuffer; ib.offset = 0;
			SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
			boundIB = d.indexBuffer;
		}
		SDL_DrawGPUIndexedPrimitives(pass, d.indexCount, 1, d.firstIndex, 0, 0);
	}

	SDL_EndGPURenderPass(pass);
	draws_.clear();   // recorded; nothing left for EndScene to draw
	return true;
}

#endif // !_WIN32
