// SDL GPU grass renderer. See header.
#include "StdAfxRD.h"
#include "SDLGrassRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / GetMatrix / GetPos / GetLighting / IsShadow
#include "Scene.h"             // cScene::GetShadowIntensity (the shader's vShade)
#include "VisGeneric.h"        // Option_filterShadow (the original's FILTER_SHADOW)
#include "VisGenericDefine.h"  // ATTRCAMERA_REFLECTION
#include "Texture.h"           // cTexture::GetDDSurface
#include "SDLRenderDevice.h"   // owner: resolves the sPtr buffers, holds the shadow map

// Grass shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/grass_shaders.h"
#include "SDLShaders/ShaderBlob.h"

namespace {

// D3DRS_ALPHAREF, straight from GrassMap::DrawGrass:
//     SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
//     SetRenderState(D3DRS_ALPHAREF, 100);
// D3D9's alpha test has no SDL GPU equivalent, so this reaches the fragment shader as a
// uniform and becomes a clip(). It is also what hides a blade past the hide distance: the
// vertex shader folds the distance fade into alpha, and this cuts it off.
const float ALPHA_REF = 100.f / 255.f;

// shortVertexGrass (Render/inc/VertexFormat.h), stride 28. Its D3D declaration is in
// Render/D3D/VertexDeclaration.cpp; these offsets are that table.
const int OFS_POSITION = 0;    // sShort4  -> SHORT4      (world units, not normalized)
const int OFS_DIFFUSE  = 8;    // Color4c  -> UBYTE4_NORM (b,g,r,a in memory)
const int OFS_NORMAL   = 12;   // Color4c  -> UBYTE4_NORM
const int OFS_TEXCOORD = 16;   // short[4] -> SHORT4
const int OFS_PLANT    = 24;   // float    -> FLOAT1

} // namespace

SDLGrassRenderer::SDLGrassRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window)
: owner_(owner), device_(device), window_(window)
{
	createSamplers();
	whiteTexture_ = createSolidGPUTexture(device_, 0xffffffffu);
	if(createShaders())
		// Shaders only: the pipelines themselves are built on demand, per vertex stride,
		// in pipelineFor(). Do not report them as ready here -- a pipeline that fails to
		// create does so long after this line, and saying "pipeline" reads as if it had not.
		fprintf(stderr, "SDLGrassRenderer: grass shaders ready\n");
}

SDLGrassRenderer::~SDLGrassRenderer()
{
	if(!device_)
		return;
	for(auto& p : pipelines_)
		if(p.second) SDL_ReleaseGPUGraphicsPipeline(device_, p.second);
	if(vs_) SDL_ReleaseGPUShader(device_, vs_);
	if(fs_) SDL_ReleaseGPUShader(device_, fs_);
	if(samplerClamp_)    SDL_ReleaseGPUSampler(device_, samplerClamp_);
	if(samplerShadow_)   SDL_ReleaseGPUSampler(device_, samplerShadow_);
	if(samplerLightMap_) SDL_ReleaseGPUSampler(device_, samplerLightMap_);
	if(whiteTexture_)    SDL_ReleaseGPUTexture(device_, whiteTexture_);
}

void SDLGrassRenderer::createSamplers()
{
	if(!device_)
		return;

	// sampler_clamp_anisotropic: the blades share one atlas, so wrapping would bleed a
	// neighbouring frame's texels in at a blade's edge. max_lod must be set -- it defaults
	// to 0, which pins sampling to the top mip.
	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	si.enable_anisotropy = true;
	si.max_anisotropy = 4.f;
	si.max_lod = 1000.f;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerClamp_ = SDL_CreateGPUSampler(device_, &si);

	// sampler_clamp_linear, which the original sets on the lightmap's stage 3.
	si.enable_anisotropy = false;
	samplerLightMap_ = SDL_CreateGPUSampler(device_, &si);

	// sampler_clamp_point on the shadow stage: the depth compare is done by hand on raw
	// values, and filtering them before comparing is meaningless.
	SDL_GPUSamplerCreateInfo ssi = {};
	ssi.min_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mag_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	ssi.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerShadow_ = SDL_CreateGPUSampler(device_, &ssi);
}

bool SDLGrassRenderer::createShaders()
{
	if(shadersTried_)
		return vs_ && fs_;
	shadersTried_ = true;
	if(!device_ || !window_)
		return false;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(grass_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	vs_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(grass_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_uniform_buffers = 1;
	fsi.num_samplers = 3;          // atlas + shadow map + lightmap
	fs_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vs_ || !fs_){
		fprintf(stderr, "SDLGrassRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

SDL_GPUGraphicsPipeline* SDLGrassRenderer::pipelineFor(int stride, bool wireframe)
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

	// Locations follow grass.vert.hlsl's VSInput declaration order.
	SDL_GPUVertexAttribute attrs[5] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;      attrs[0].offset = OFS_POSITION;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = OFS_DIFFUSE;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[2].offset = OFS_NORMAL;
	attrs[3].location = 3; attrs[3].buffer_slot = 0; attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;      attrs[3].offset = OFS_TEXCOORD;
	attrs[4].location = 4; attrs[4].buffer_slot = 0; attrs[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;       attrs[4].offset = OFS_PLANT;

	// The original's blend state, set once around the whole draw:
	//     ALPHABLENDENABLE TRUE, SRCBLEND SRCALPHA, DESTBLEND INVSRCALPHA, BLENDOP ADD
	// Straight (non-premultiplied) alpha, as SDLObject3dxRenderer uses for the meshes that
	// come out of the same texture library. The alpha *test* is not here -- it has no SDL GPU
	// equivalent and lives in the fragment shader's clip().
	SDL_GPUColorTargetBlendState bs = {};
	bs.enable_blend = true;
	bs.color_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	SDL_GPUColorTargetDescription ctd = {};
	ctd.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	ctd.blend_state = bs;

	SDL_GPUGraphicsPipelineCreateInfo pi = {};
	pi.vertex_shader = vs_;
	pi.fragment_shader = fs_;
	pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pi.vertex_input_state.vertex_buffer_descriptions = &vbd;
	pi.vertex_input_state.num_vertex_buffers = 1;
	pi.vertex_input_state.vertex_attributes = attrs;
	pi.vertex_input_state.num_vertex_attributes = 5;
	// D3DCULL_NONE, which GrassMap::Draw sets: a blade is a flat quad seen from both sides.
	pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pi.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// SDL GPU defaults to depth CLAMP; D3D clips. Without this, a blade behind the near plane
	// smears across the view instead of disappearing.
	pi.rasterizer_state.enable_depth_clip = true;
	// Depth-test against the terrain, and write: Camera::DrawScene has RS_ZWRITEENABLE on
	// when it reaches the grass, and the original never turns it off around this draw. The
	// blades then occlude each other correctly, which is what the per-tile front-to-back
	// index buffers in GrassMap exist to help with.
	pi.depth_stencil_state.enable_depth_test = true;
	pi.depth_stencil_state.enable_depth_write = true;
	pi.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pi.target_info.color_target_descriptions = &ctd;
	pi.target_info.num_color_targets = 1;
	pi.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	pi.target_info.has_depth_stencil_target = true;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pi);
	if(!pipeline)
		fprintf(stderr, "SDLGrassRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
	pipelines_.push_back({key, pipeline});
	return pipeline;
}

void SDLGrassRenderer::BeginFrame()
{
	states_.clear();
	draws_.clear();
	currentValid_ = false;
	currentDirty_ = true;
}

void SDLGrassRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera)
		return;

	StateBlock& c = current_;
	std::memset(&c, 0, sizeof(c));

	std::memcpy(c.vs.mvp, &camera->matViewProj, sizeof(c.vs.mvp));

	// The billboard basis. GetMatrix() is the camera's VIEW matrix (world -> camera), so its
	// transpose takes the blade's local offset from camera space back to world -- which is
	// what makes the quad face the viewer. Exactly what VSGrass::Select uploaded as mWorld.
	Mat4f billboard(camera->GetMatrix());
	billboard.transpose();
	std::memcpy(c.vs.billboard, &billboard, sizeof(c.vs.billboard));

	cSDLRenderDevice* dev = sdlRenderDevice();

	// The shadow map, if the light camera filled one earlier this frame, and if this grass
	// receives -- the same pair of tests on which the original picks psGrassShadow.
	const bool shadow = state.receiveShadow && dev && dev->shadowPassRan() && dev->GetShadowMap();
	if(shadow){
		c.shadowTexture = reinterpret_cast<SDL_GPUTexture*>(dev->GetShadowMap()->GetDDSurface(0));
		const Mat4f mShadow = dev->shadowMatViewProj() * dev->shadowMatBias();
		std::memcpy(c.vs.shadow, &mShadow, sizeof(c.vs.shadow));
	}

	// The terrain lightmap and its fPlanarNode: the same box cScene::AddPlanarCamera renders.
	cTexture* lightMapTexture = dev ? dev->GetLightMap() : nullptr;
	SDL_GPUTexture* lightMap = lightMapTexture
	                         ? reinterpret_cast<SDL_GPUTexture*>(lightMapTexture->GetDDSurface(0))
	                         : nullptr;
	if(dev){
		const Vect4f& pn = dev->planarTransform();
		c.vs.planarNode[0] = pn.x; c.vs.planarNode[1] = pn.y;
		c.vs.planarNode[2] = pn.z; c.vs.planarNode[3] = pn.w;
	}
	else{
		c.vs.planarNode[2] = c.vs.planarNode[3] = 1.f;
	}

	const Vect3f& eye = camera->GetPos();
	c.vs.cameraPos[0] = eye.x; c.vs.cameraPos[1] = eye.y; c.vs.cameraPos[2] = eye.z;

	Vect3f sun(0.f, 0.f, -1.f);   // straight down if the scene has no sun yet
	camera->GetLighting(sun);
	c.vs.lightDir[0] = sun.x; c.vs.lightDir[1] = sun.y; c.vs.lightDir[2] = sun.z;

	c.vs.sunDiffuse[0] = state.sunDiffuse.r;
	c.vs.sunDiffuse[1] = state.sunDiffuse.g;
	c.vs.sunDiffuse[2] = state.sunDiffuse.b;
	c.vs.sunDiffuse[3] = state.sunDiffuse.a;

	c.vs.params[0] = state.time;
	c.vs.params[1] = state.hideDistance;
	c.vs.params[2] = state.oldLighting ? 1.f : 0.f;

	// vShade: what a fully shadowed pixel is multiplied by, per mission.
	cScene* scene = camera->scene();
	const Color4f shade = (shadow && scene) ? scene->GetShadowIntensity()
	                                        : Color4f(1.f, 1.f, 1.f, 1.f);
	c.fs.shade[0] = shade.r; c.fs.shade[1] = shade.g; c.fs.shade[2] = shade.b; c.fs.shade[3] = shade.a;
	c.fs.shadowParams[0] = shadow ? 1.f : 0.f;
	c.fs.shadowParams[1] = (shadow && Option_filterShadow) ? 1.f : 0.f;
	c.fs.lightMapParams[0] = lightMap ? 1.f : 0.f;
	// The fog of war rides the lightmap's alpha, so it needs the map to be there -- the same
	// test the terrain makes. Grass inside the shroud must go with the ground under it.
	const bool fogOfWar = dev && dev->fogOfWar() && lightMap;
	c.fs.lightMapParams[1] = fogOfWar ? 1.f : 0.f;
	const Color4f fow = dev ? dev->fogOfWarColor() : Color4f();
	c.fs.fogOfWarColor[0] = fow.r; c.fs.fogOfWarColor[1] = fow.g;
	c.fs.fogOfWarColor[2] = fow.b; c.fs.fogOfWarColor[3] = fow.a;
	c.fs.params[0] = ALPHA_REF;

	// Distance fog. Off -> (0,0,0,1), i.e. factor 1, and the shader's lerp is the identity.
	const Vect4f fogPlane = dev ? dev->fogPlane(camera) : Vect4f(0.f, 0.f, 0.f, 1.f);
	c.vs.fogPlane[0] = fogPlane.x; c.vs.fogPlane[1] = fogPlane.y;
	c.vs.fogPlane[2] = fogPlane.z; c.vs.fogPlane[3] = fogPlane.w;
	const Color4f fog = dev ? dev->fogColor() : Color4f(0.f, 0.f, 0.f, 0.f);
	c.fs.fogColor[0] = fog.r; c.fs.fogColor[1] = fog.g; c.fs.fogColor[2] = fog.b; c.fs.fogColor[3] = fog.a;

	c.texture = state.texture
	          ? reinterpret_cast<SDL_GPUTexture*>(state.texture->GetDDSurface(0))
	          : nullptr;
	c.lightMapTexture = lightMap;

	c.vpX = camera->vp.X; c.vpY = camera->vp.Y;
	c.vpW = camera->vp.Width; c.vpH = camera->vp.Height;
	c.vpMinZ = camera->vp.MinZ; c.vpMaxZ = camera->vp.MaxZ;
	c.mirrored = camera->getAttribute(ATTRCAMERA_REFLECTION) != 0;

	currentValid_ = true;
	currentDirty_ = true;
}

int SDLGrassRenderer::commitState()
{
	if(currentDirty_ || states_.empty()){
		states_.push_back(current_);
		currentDirty_ = false;
	}
	return (int)states_.size() - 1;
}

void SDLGrassRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb, const sPtrIndexBuffer& ib,
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
	d.state = commitState();
	d.vertexBuffer = vertexBuffer;
	d.indexBuffer = indexBuffer;
	d.stride = stride;
	// The index buffer is a 16-bit triangle list, so D3D's polygon count is 3x the index
	// count -- the same arithmetic cD3DRender::DrawIndexedPrimitive handed to D3D. GrassMap
	// always draws a whole tile, from polygon 0.
	d.indexCount = 3 * nPolygon;
	draws_.push_back(d);
}

bool SDLGrassRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                            int targetW, int targetH, bool clear, const float clearColor[4],
                            bool clearDepth, bool wireframe)
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

	int lastState = -1;
	SDL_GPUGraphicsPipeline* lastPipeline = nullptr;
	for(const DrawCmd& d : draws_){
		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, wireframe);
		if(!pipeline)
			continue;
		if(pipeline != lastPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			lastPipeline = pipeline;
			lastState = -1;   // uniforms and samplers are bound per pipeline binding
		}

		const StateBlock& s = states_[d.state];
		if(d.state != lastState){
			applyCameraViewport(pass, sViewPort{s.vpX, s.vpY, s.vpW, s.vpH, s.vpMinZ, s.vpMaxZ},
			                    targetW, targetH);
			SDL_PushGPUVertexUniformData(cmd, 0, &s.vs, sizeof(s.vs));
			SDL_PushGPUFragmentUniformData(cmd, 0, &s.fs, sizeof(s.fs));

			SDL_GPUTextureSamplerBinding ts[3] = {};
			ts[0].texture = s.texture ? s.texture : whiteTexture_;
			ts[0].sampler = samplerClamp_;
			// SDL requires every declared sampler slot to be bound, even where the shader
			// gates the read off; the white stand-in goes in the empty ones.
			ts[1].texture = s.shadowTexture ? s.shadowTexture : whiteTexture_;
			ts[1].sampler = samplerShadow_;
			ts[2].texture = s.lightMapTexture ? s.lightMapTexture : whiteTexture_;
			ts[2].sampler = samplerLightMap_;
			SDL_BindGPUFragmentSamplers(pass, 0, ts, 3);
			lastState = d.state;
		}

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
