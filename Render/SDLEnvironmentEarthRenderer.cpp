// SDL GPU environment-earth renderer. See header.
#include "StdAfxRD.h"
#include "SDLEnvironmentEarthRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture::GetDDSurface
#include "SDLRenderDevice.h"   // owner: resolves the sPtr buffers, applyCameraViewport

// Environment-earth shader bytecode, compiled to this platform's native format at build
// time; see Render/CMakeLists.txt.
#include "SDLShaders/environmentearth_shaders.h"
#include "SDLShaders/ShaderBlob.h"

SDLEnvironmentEarthRenderer::SDLEnvironmentEarthRenderer(cSDLRenderDevice* owner,
                                                         SDL_GPUDevice* device, SDL_Window* window)
: owner_(owner), device_(device), window_(window)
{
	createSampler();
	whiteTexture_ = createSolidGPUTexture(device_, 0xffffffffu);
	if(createShaders())
		fprintf(stderr, "SDLEnvironmentEarthRenderer: environment earth pipeline ready\n");
}

SDLEnvironmentEarthRenderer::~SDLEnvironmentEarthRenderer()
{
	if(!device_)
		return;
	for(auto& p : pipelines_)
		if(p.second) SDL_ReleaseGPUGraphicsPipeline(device_, p.second);
	if(vs_) SDL_ReleaseGPUShader(device_, vs_);
	if(fs_) SDL_ReleaseGPUShader(device_, fs_);
	if(samplerWrap_)  SDL_ReleaseGPUSampler(device_, samplerWrap_);
	if(whiteTexture_) SDL_ReleaseGPUTexture(device_, whiteTexture_);
}

void SDLEnvironmentEarthRenderer::createSampler()
{
	if(!device_)
		return;
	// sampler_wrap_anisotropic, as cEnvironmentEarth::Draw sets on stage 0. WRAP is not
	// optional: SetTexture scales the coordinates by world size / texture size, so the ground
	// tiles many times across the plane and the coordinates run far past 1.
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
	samplerWrap_ = SDL_CreateGPUSampler(device_, &si);
}

bool SDLEnvironmentEarthRenderer::createShaders()
{
	if(shadersTried_)
		return vs_ && fs_;
	shadersTried_ = true;
	if(!device_ || !window_)
		return false;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(environmentearth_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	vs_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(environmentearth_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_uniform_buffers = 1;
	fsi.num_samplers = 1;          // the ground texture
	fs_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vs_ || !fs_){
		fprintf(stderr, "SDLEnvironmentEarthRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

SDL_GPUGraphicsPipeline* SDLEnvironmentEarthRenderer::pipelineFor(int stride, bool wireframe)
{
	if(!createShaders())
		return nullptr;

	const unsigned key = (unsigned)stride | (wireframe ? 0x10000u : 0u);
	for(auto& p : pipelines_)
		if(p.first == key)
			return p.second;

	SDL_GPUVertexBufferDescription vbd = {};
	vbd.slot = 0;
	vbd.pitch = (Uint32)stride;
	vbd.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	// sVertexXYZDT1, stride 24. Locations follow environmentearth.vert.hlsl's declaration order.
	SDL_GPUVertexAttribute attrs[3] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 12;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 16;

	// ALPHA_NONE: cEnvironmentEarth::Draw calls SetWorldMaterial(ALPHA_NONE, ...), so the plane
	// is written opaquely, no blend.
	SDL_GPUColorTargetDescription ctd = {};
	ctd.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	ctd.blend_state.enable_blend = false;

	SDL_GPUGraphicsPipelineCreateInfo pi = {};
	pi.vertex_shader = vs_;
	pi.fragment_shader = fs_;
	pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pi.vertex_input_state.vertex_buffer_descriptions = &vbd;
	pi.vertex_input_state.num_vertex_buffers = 1;
	pi.vertex_input_state.vertex_attributes = attrs;
	pi.vertex_input_state.num_vertex_attributes = 3;
	// The plane is a flat sheet viewed from one side, and the original's box-border winding is
	// not consistent; CULLMODE_NONE, as every other SDL world pass uses.
	pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pi.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// The plane runs out past the far plane; clip it there, do not clamp (see the SDL depth-clip
	// default note in the reflection work).
	pi.rasterizer_state.enable_depth_clip = true;
	// Opaque ground: depth test and write on, as SetWorldMaterial(ALPHA_NONE) leaves them. The
	// earth draws first (after the camera's ClearZBuffer) and the terrain draws over its depth.
	pi.depth_stencil_state.enable_depth_test = true;
	pi.depth_stencil_state.enable_depth_write = true;
	pi.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pi.target_info.color_target_descriptions = &ctd;
	pi.target_info.num_color_targets = 1;
	pi.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	pi.target_info.has_depth_stencil_target = true;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	if(!pipeline)
		fprintf(stderr, "SDLEnvironmentEarthRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
	pipelines_.push_back({key, pipeline});
	return pipeline;
}

void SDLEnvironmentEarthRenderer::BeginFrame()
{
	draws_.clear();
	currentValid_ = false;
}

void SDLEnvironmentEarthRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera)
		return;

	// vsStandart::Select's mWVP = world * matViewProj. cEnvironmentEarth's plane is already in
	// world coordinates and it has no world matrix of its own, so this is just the camera's.
	std::memcpy(vs_current_.mvp, &camera->matViewProj, sizeof(vs_current_.mvp));

	fs_current_.tfactor[0] = state.tfactor.r;
	fs_current_.tfactor[1] = state.tfactor.g;
	fs_current_.tfactor[2] = state.tfactor.b;
	fs_current_.tfactor[3] = state.tfactor.a;

	texture_ = state.texture
	         ? reinterpret_cast<SDL_GPUTexture*>(state.texture->GetDDSurface(0))
	         : nullptr;

	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;

	currentValid_ = true;
}

void SDLEnvironmentEarthRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb,
                                                       const sPtrIndexBuffer& ib, int nPolygon)
{
	if(!owner_ || !currentValid_ || nPolygon <= 0)
		return;

	const int stride = vb.GetVertexSize();
	SDL_GPUBuffer* vertexBuffer = owner_->gpuBuffer(vb);
	SDL_GPUBuffer* indexBuffer = owner_->gpuBuffer(ib);
	if(!vertexBuffer || !indexBuffer || stride <= 0)
		return;

	DrawCmd d;
	d.vs = vs_current_;
	d.fs = fs_current_;
	d.texture = texture_;
	d.vertexBuffer = vertexBuffer;
	d.indexBuffer = indexBuffer;
	d.stride = stride;
	d.indexCount = 3 * nPolygon;
	d.vpX = vpX_; d.vpY = vpY_; d.vpW = vpW_; d.vpH = vpH_;
	d.vpMinZ = vpMinZ_; d.vpMaxZ = vpMaxZ_;
	draws_.push_back(d);
}

bool SDLEnvironmentEarthRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target,
                                       SDL_GPUTexture* depth, int targetW, int targetH,
                                       bool clear, const float clearColor[4], bool clearDepth,
                                       bool wireframe)
{
	if(!device_ || !cmd || !target || !depth || draws_.empty()){
		draws_.clear();
		return false;
	}

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
	dt.store_op = SDL_GPU_STOREOP_STORE;   // the terrain and everything after test against it
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, &dt);

	SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
	for(const DrawCmd& d : draws_){
		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, wireframe);
		if(!pipeline)
			continue;
		if(pipeline != lastPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			lastPipeline = pipeline;
		}

		applyCameraViewport(pass, sViewPort{d.vpX, d.vpY, d.vpW, d.vpH, d.vpMinZ, d.vpMaxZ},
		                    targetW, targetH);
		SDL_PushGPUVertexUniformData(cmd, 0, &d.vs, sizeof(d.vs));
		SDL_PushGPUFragmentUniformData(cmd, 0, &d.fs, sizeof(d.fs));

		SDL_GPUTexture* tex = d.texture ? d.texture : whiteTexture_;
		SDL_GPUTextureSamplerBinding ts = {};
		ts.texture = tex; ts.sampler = samplerWrap_;
		SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);

		SDL_GPUBufferBinding vb = {}; vb.buffer = d.vertexBuffer; vb.offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
		SDL_GPUBufferBinding ib = {}; ib.buffer = d.indexBuffer; ib.offset = 0;
		SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_DrawGPUIndexedPrimitives(pass, d.indexCount, 1, 0, 0, 0);
	}

	SDL_EndGPURenderPass(pass);
	draws_.clear();
	return true;
}
