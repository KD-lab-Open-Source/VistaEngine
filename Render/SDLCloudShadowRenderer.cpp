// SDL GPU cloud-shadow renderer. See header.
#include "StdAfxRD.h"
#include "SDLCloudShadowRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture::GetDDSurface
#include "SDLRenderDevice.h"   // owner: resolves the sPtr buffers

// Cloud-shadow shader bytecode, compiled to this platform's native format at build
// time; see Render/CMakeLists.txt.
#include "SDLShaders/cloudshadow_shaders.h"
#include "SDLShaders/ShaderBlob.h"

SDLCloudShadowRenderer::SDLCloudShadowRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device,
                                               SDL_Window* window)
: owner_(owner), device_(device), window_(window)
{
	createSampler();
	whiteTexture_ = createSolidGPUTexture(device_, 0xffffffffu);
	if(createShaders())
		fprintf(stderr, "SDLCloudShadowRenderer: cloud shadow pipeline ready\n");
}

SDLCloudShadowRenderer::~SDLCloudShadowRenderer()
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

void SDLCloudShadowRenderer::createSampler()
{
	if(!device_)
		return;
	// sampler_wrap_linear, as cCloudShadow::Draw sets on both stages. WRAP is not optional:
	// cCloudShadow::Animate scrolls the coordinates without bound (it only cycles them into
	// 0..1 at the quad's corners), so they run off the texture continuously.
	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	si.max_lod = 1000.f;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerWrap_ = SDL_CreateGPUSampler(device_, &si);
}

bool SDLCloudShadowRenderer::createShaders()
{
	if(shadersTried_)
		return vs_ && fs_;
	shadersTried_ = true;
	if(!device_ || !window_)
		return false;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(cloudshadow_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	vs_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(cloudshadow_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_uniform_buffers = 1;
	fsi.num_samplers = 2;          // the same cloud texture, at the two scroll offsets
	fs_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vs_ || !fs_){
		fprintf(stderr, "SDLCloudShadowRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

SDL_GPUGraphicsPipeline* SDLCloudShadowRenderer::pipelineFor(int stride, bool wireframe)
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

	// sVertexXYZDT2, stride 32. Locations follow cloudshadow.vert.hlsl's declaration order.
	SDL_GPUVertexAttribute attrs[4] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 12;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 16;
	attrs[3].location = 3; attrs[3].buffer_slot = 0; attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[3].offset = 24;

	// ALPHA_NONE: cCloudShadow::Draw calls SetBlendStateAlphaRef(ALPHA_NONE), so the quad
	// OVERWRITES the lightmap. That is what makes 0.5 the neutral -- the clouds establish the
	// map's base level, and drawLights() blends the light sources over it afterwards.
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
	pi.vertex_input_state.num_vertex_attributes = 4;
	pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pi.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	pi.rasterizer_state.enable_depth_clip = true;
	// No depth at all. The planar light camera drew with ZWRITEENABLE FALSE and ZFUNC ALWAYS
	// around this, which is D3D for "the lightmap has no geometry to occlude anything" -- it
	// is a flat map, not a view. Its depth buffer holds whatever the last real camera left.
	pi.depth_stencil_state.enable_depth_test = false;
	pi.depth_stencil_state.enable_depth_write = false;
	pi.target_info.color_target_descriptions = &ctd;
	pi.target_info.num_color_targets = 1;
	pi.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	pi.target_info.has_depth_stencil_target = true;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	if(!pipeline)
		fprintf(stderr, "SDLCloudShadowRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
	pipelines_.push_back({key, pipeline});
	return pipeline;
}

void SDLCloudShadowRenderer::BeginFrame()
{
	draws_.clear();
	currentValid_ = false;
}

void SDLCloudShadowRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera)
		return;

	// VSCloudShadow::Select: mWVP = world * matViewProj. cCloudShadow's quad is already in
	// world coordinates and it has no world matrix of its own, so this is just the camera's.
	std::memcpy(vs_current_.mvp, &camera->matViewProj, sizeof(vs_current_.mvp));

	fs_current_.tfactor[0] = state.tfactor.r;
	fs_current_.tfactor[1] = state.tfactor.g;
	fs_current_.tfactor[2] = state.tfactor.b;
	fs_current_.tfactor[3] = state.tfactor.a;

	// PSCloudShadow::Select: tfactorm05 = 0.5 - 0.75*tfactor, in all four channels.
	for(int i = 0; i < 4; ++i)
		fs_current_.tfactorM05[i] = 0.5f - 0.75f * fs_current_.tfactor[i];

	texture_ = state.texture
	         ? reinterpret_cast<SDL_GPUTexture*>(state.texture->GetDDSurface(0))
	         : nullptr;

	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;

	currentValid_ = true;
}

void SDLCloudShadowRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb, const sPtrIndexBuffer& ib,
                                                  int nPolygon)
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

bool SDLCloudShadowRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target,
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
	dt.store_op = SDL_GPU_STOREOP_STORE;
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

		// The same texture on both stages, as cCloudShadow::Draw binds it: the two scrolling
		// coordinate sets are the only thing that differs.
		SDL_GPUTexture* tex = d.texture ? d.texture : whiteTexture_;
		SDL_GPUTextureSamplerBinding ts[2] = {};
		ts[0].texture = tex; ts[0].sampler = samplerWrap_;
		ts[1].texture = tex; ts[1].sampler = samplerWrap_;
		SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);

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
