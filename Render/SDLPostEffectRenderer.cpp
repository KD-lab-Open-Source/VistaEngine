// SDL GPU post-effect renderer. See header.
#include "StdAfxRD.h"
#include "SDLPostEffectRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>

#include "Texture.h"           // cTexture::GetDDSurface

// Post-effect shader bytecode, compiled to this platform's native format at build
// time; see Render/CMakeLists.txt.
#include "SDLShaders/posteffect_shaders.h"
#include "SDLShaders/ShaderBlob.h"

SDLPostEffectRenderer::SDLPostEffectRenderer(SDL_GPUDevice* device, SDL_Window* window)
: device_(device), window_(window)
{
	createSamplers();
	if(createShaders())
		// Shaders only; the pipelines are built on demand. See SDLGrassRenderer.
		fprintf(stderr, "SDLPostEffectRenderer: post-effect shaders ready\n");
}

SDLPostEffectRenderer::~SDLPostEffectRenderer()
{
	if(!device_)
		return;
	for(SDL_GPUGraphicsPipeline* p : pipelines_)
		if(p) SDL_ReleaseGPUGraphicsPipeline(device_, p);
	if(vs_) SDL_ReleaseGPUShader(device_, vs_);
	for(SDL_GPUShader* f : fs_)
		if(f) SDL_ReleaseGPUShader(device_, f);
	if(samplerClampPoint_) SDL_ReleaseGPUSampler(device_, samplerClampPoint_);
	if(samplerWrapLinear_) SDL_ReleaseGPUSampler(device_, samplerWrapLinear_);
	if(pingTexture_) SDL_ReleaseGPUTexture(device_, pingTexture_);
}

void SDLPostEffectRenderer::createSamplers()
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
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	si.max_lod = 1000.f;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerWrapLinear_ = SDL_CreateGPUSampler(device_, &si);
}

bool SDLPostEffectRenderer::createShaders()
{
	if(shadersTried_)
		return vs_ != nullptr;
	shadersTried_ = true;
	// window_ may be null -- the Qt editor creates no SDL window of its own; the swapchain
	// comes from the foreign window later. SDL_GetGPUSwapchainTextureFormat tolerates a null
	// window (it returns the device's default swapchain format), so only the device matters.
	if(!device_)
		return false;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(posteffect_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;   // SV_VertexID only: no buffers, no uniforms
	vs_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo ci = vista::shaderCreateInfo(VISTA_SHADER(posteffect_copy_frag));
	ci.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	ci.num_samplers = 1;
	fs_[EFFECT_COPY] = SDL_CreateGPUShader(device_, &ci);

	SDL_GPUShaderCreateInfo mi = vista::shaderCreateInfo(VISTA_SHADER(posteffect_monochrome_frag));
	mi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	mi.num_samplers = 1;
	mi.num_uniform_buffers = 1;
	fs_[EFFECT_MONOCHROME] = SDL_CreateGPUShader(device_, &mi);

	SDL_GPUShaderCreateInfo ui = vista::shaderCreateInfo(VISTA_SHADER(posteffect_underwater_frag));
	ui.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	ui.num_samplers = 2;                      // the scene and the wave texture
	ui.num_uniform_buffers = 1;
	fs_[EFFECT_UNDERWATER] = SDL_CreateGPUShader(device_, &ui);

	if(!vs_ || !fs_[EFFECT_COPY] || !fs_[EFFECT_MONOCHROME] || !fs_[EFFECT_UNDERWATER]){
		fprintf(stderr, "SDLPostEffectRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs_){ SDL_ReleaseGPUShader(device_, vs_); vs_ = nullptr; }
		for(SDL_GPUShader*& f : fs_)
			if(f){ SDL_ReleaseGPUShader(device_, f); f = nullptr; }
		return false;
	}
	return true;
}

SDL_GPUGraphicsPipeline* SDLPostEffectRenderer::pipelineFor(EffectKind kind)
{
	if(pipelines_[kind])
		return pipelines_[kind];
	if(!createShaders())
		return nullptr;

	// A fullscreen pass: no vertex input, no depth attachment, no blending -- each stage
	// overwrites the next in full, exactly as the D3D quads drew with Z and blending off
	// (postEffects.cpp's PE_RENDER_STATE_BACKUP bracket).
	SDL_GPUColorTargetDescription ctd = {};
	ctd.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	ctd.blend_state.enable_blend = false;

	SDL_GPUGraphicsPipelineCreateInfo pi = {};
	pi.vertex_shader = vs_;
	pi.fragment_shader = fs_[kind];
	pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pi.rasterizer_state.enable_depth_clip = true;
	pi.target_info.color_target_descriptions = &ctd;
	pi.target_info.num_color_targets = 1;
	pi.target_info.has_depth_stencil_target = false;

	pipelines_[kind] = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	if(!pipelines_[kind])
		fprintf(stderr, "SDLPostEffectRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
	return pipelines_[kind];
}

SDL_GPUTexture* SDLPostEffectRenderer::ensurePing(int w, int h)
{
	if(pingTexture_ && pingW_ == w && pingH_ == h)
		return pingTexture_;
	if(pingTexture_){
		SDL_ReleaseGPUTexture(device_, pingTexture_);
		pingTexture_ = nullptr;
	}

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	pingTexture_ = SDL_CreateGPUTexture(device_, &ti);
	pingW_ = pingTexture_ ? w : 0;
	pingH_ = pingTexture_ ? h : 0;
	return pingTexture_;
}

void SDLPostEffectRenderer::BeginFrame()
{
	effects_.clear();
}

void SDLPostEffectRenderer::recordMonochrome(float phase)
{
	EffectCmd e = {};
	e.kind = EFFECT_MONOCHROME;
	e.fs.params[0] = phase;
	effects_.push_back(e);
}

void SDLPostEffectRenderer::recordUnderWater(float shift, float scale, const Color4f& color,
                                             cTexture* wave)
{
	SDL_GPUTexture* waveTex = wave
	                        ? reinterpret_cast<SDL_GPUTexture*>(wave->GetDDSurface(0))
	                        : nullptr;
	if(!waveTex)
		return;   // nothing to displace with; the D3D redraw bailed on this too

	EffectCmd e = {};
	e.kind = EFFECT_UNDERWATER;
	// PSUnderWater::Select's packing: (shift, scale*0.05, scale, -). The *0.05 turns the
	// fade scale into the distortion/zoom amplitude; the raw scale fades the colour in.
	e.fs.params[0] = shift;
	e.fs.params[1] = scale * 0.05f;
	e.fs.params[2] = scale;
	e.fs.color[0] = color.r;
	e.fs.color[1] = color.g;
	e.fs.color[2] = color.b;
	e.fs.color[3] = color.a;
	e.wave = waveTex;
	effects_.push_back(e);
}

bool SDLPostEffectRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* capture,
                                 SDL_GPUTexture* swapchain, int w, int h)
{
	if(!device_ || !cmd || !capture || !swapchain){
		effects_.clear();
		return false;
	}

	// An empty chain still composites: the scene is in the capture, and the swapchain has
	// nothing on it. One pass-through copy.
	EffectCmd copy = {};
	copy.kind = EFFECT_COPY;
	if(effects_.empty())
		effects_.push_back(copy);

	// Chains of two or more stage through the ping texture; if it cannot be made, fall
	// back to compositing only the last effect rather than losing the frame.
	if(effects_.size() > 1 && !ensurePing(w, h))
		effects_.erase(effects_.begin(), effects_.end() - 1);

	bool drew = false;
	// The stages alternate between the capture and the ping texture; the last one renders
	// to the swapchain. Render passes order the writes against the reads.
	SDL_GPUTexture* source = capture;
	for(size_t i = 0; i < effects_.size(); ++i){
		const EffectCmd& e = effects_[i];
		const bool last = i + 1 == effects_.size();
		SDL_GPUTexture* target = last ? swapchain
		                              : (source == capture ? pingTexture_ : capture);

		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(e.kind);
		if(!pipeline)
			continue;

		SDL_GPUColorTargetInfo ct = {};
		ct.texture = target;
		// The triangle covers every pixel of the target; nothing is loaded through it.
		ct.load_op = SDL_GPU_LOADOP_DONT_CARE;
		ct.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, nullptr);
		SDL_BindGPUGraphicsPipeline(pass, pipeline);

		if(e.kind != EFFECT_COPY)
			SDL_PushGPUFragmentUniformData(cmd, 0, &e.fs, sizeof(e.fs));

		SDL_GPUTextureSamplerBinding ts[2] = {};
		if(e.kind == EFFECT_UNDERWATER){
			ts[0].texture = source; ts[0].sampler = samplerWrapLinear_;
			ts[1].texture = e.wave; ts[1].sampler = samplerWrapLinear_;
			SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);
		}
		else {
			ts[0].texture = source; ts[0].sampler = samplerClampPoint_;
			SDL_BindGPUFragmentSamplers(pass, 0, ts, 1);
		}

		SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
		SDL_EndGPURenderPass(pass);

		source = target;
		drew = true;
	}

	effects_.clear();
	return drew;
}
