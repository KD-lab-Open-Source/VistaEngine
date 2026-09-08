// SDL GPU water renderer. See header.
#include "StdAfxRD.h"
#include "SDLWaterRenderer.h"


#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // owner: resolves the sPtr buffers, applyCameraViewport

// Water shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/water_shaders.h"
#include "SDLShaders/ShaderBlob.h"

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
	if(flatTexture_)   SDL_ReleaseGPUTexture(device_, flatTexture_);
	if(sampler_)       SDL_ReleaseGPUSampler(device_, sampler_);
	if(samplerClamp_)  SDL_ReleaseGPUSampler(device_, samplerClamp_);
	if(pipelineFill_)  SDL_ReleaseGPUGraphicsPipeline(device_, pipelineFill_);
	if(pipelineLine_)  SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLine_);
	if(pipelineReflectFill_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineReflectFill_);
	if(pipelineReflectLine_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineReflectLine_);
	if(pipelineIceFill_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineIceFill_);
	if(pipelineIceLine_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineIceLine_);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLWaterRenderer::createPipelines()
{
	// window_ may be null -- the Qt editor creates no SDL window of its own; the swapchain
	// comes from the foreign window later. SDL_GetGPUSwapchainTextureFormat tolerates a null
	// window (it returns the device's default swapchain format, which SDLTileMapRenderer's
	// ctor pipelines rely on), so only the device matters here. Requiring a window left the
	// water pipelines never built in the Qt editor and the sea invisible.
	if(!device_) return;

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

	// cWater::Draw's SetSamplerData(2, sampler_clamp_anisotropic) for the reflection target.
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.max_lod = 0.f;   // the render target has no mip chain
	// mipmap_mode stays LINEAR, and must: si still carries enable_anisotropy from the
	// sampler above, and D3D12 encodes anisotropy as a bit on top of the min/mag/mip
	// filter -- MIN_MAG_LINEAR_MIP_POINT | ANISOTROPIC is 0x54, which is not a member of
	// D3D12_FILTER. CreateSampler cannot fail by return code, so the runtime answers an
	// unrecognised filter by REMOVING THE DEVICE, and every texture and pipeline after it
	// fails with DXGI_ERROR_INVALID_CALL -- including the font atlas, which is where it
	// finally surfaced, as a null FT::Font several subsystems away.
	// Vulkan and Metal carry the filters and anisotropy as independent fields, so the same
	// call is legal there and this only ever reached Windows.
	// Nothing is lost: with one mip level there is nothing for POINT and LINEAR to differ
	// over, and max_lod above already pins sampling to it.
	samplerClamp_ = SDL_CreateGPUSampler(device_, &si);

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

	createPipelinePair(false, pipelineFill_, pipelineLine_);
	createPipelinePair(true,  pipelineReflectFill_, pipelineReflectLine_);
	createIcePipeline();

	fprintf(stderr, "SDLWaterRenderer: water pipeline %s (wireframe %s), reflection %s, ice %s\n",
	        pipelineFill_ ? "ready" : "FAILED", pipelineLine_ ? "ready" : "FAILED",
	        pipelineReflectFill_ ? "ready" : "FAILED", pipelineIceFill_ ? "ready" : "FAILED");
}

// The ice sheet cTemperature::Draw blends over the surface: the water vertex again, alpha
// blended (its coverage the opacity), depth tested but not written -- exactly the surface's
// pipeline state, differing only in the shader (water_ice) and its six samplers.
void SDLWaterRenderer::createIcePipeline()
{
	// See createPipelines: only the device matters; the swapchain-format query tolerates a
	// null window.
	if(!device_) return;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(water_ice_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(water_ice_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 6;   // coverage, snow, bump, reflection, cleft, lightmap
	fsi.num_uniform_buffers = 1;
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLWaterRenderer: ice CreateGPUShader failed: %s\n", SDL_GetError());
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

	// beginDraw's ALPHA_BLEND with RS_ZWRITEENABLE off: blend over the water, leave depth.
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
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = false;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineIceFill_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	pipelineIceLine_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
}

bool SDLWaterRenderer::createPipelinePair(bool reflection,
                                          SDL_GPUGraphicsPipeline*& fill,
                                          SDL_GPUGraphicsPipeline*& line)
{
	// REFLECTION=1 is the original's water_linear, REFLECTION=0 its water_easy; the two
	// are separate builds of the same HLSL.
	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(
		reflection ? VISTA_SHADER(water_reflect_vert) : VISTA_SHADER(water_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP, the two uv scale/offsets, vMirrorVP
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(
		reflection ? VISTA_SHADER(water_reflect_frag) : VISTA_SHADER(water_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	// The two wave maps, plus the reflection target on the water_linear path.
	fsi.num_samplers = reflection ? 3 : 2;
	fsi.num_uniform_buffers = 1;
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLWaterRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return false;
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

	fill = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	// Wireframe variant (RS_FILLMODE == FILL_WIREFRAME). Same everything but LINE fill.
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	line = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
	return fill != nullptr;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLWaterRenderer::BeginFrame()
{
	draws_.clear();
	stateValid_ = false;
	iceDraws_.clear();
	iceValid_ = false;
	iceMode_ = false;
}

void SDLWaterRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera) return;

	// Surface polygons record into draws_, not the ice list.
	iceMode_ = false;

	vs_ = VSUniform();
	fs_ = FSUniform();

	std::memcpy(vs_.mvp, &camera->matViewProj, sizeof(vs_.mvp));
	std::memcpy(vs_.uvScaleOffset, state.uvScaleOffset, sizeof(vs_.uvScaleOffset));
	std::memcpy(vs_.uvScaleOffset1, state.uvScaleOffset1, sizeof(vs_.uvScaleOffset1));

	texture0_ = sdlTextureOf(state.texture0);
	texture1_ = sdlTextureOf(state.texture1);

	// Without the reflection target there is nothing for water_linear to sample, so fall
	// back to the flat colour rather than draw the surface black.
	reflectionTexture_ = state.reflection ? sdlTextureOf(state.reflectionTexture) : nullptr;
	reflection_ = reflectionTexture_ != nullptr;

	if(reflection_){
		std::memcpy(vs_.mirrorVP, state.mirrorVP, sizeof(vs_.mirrorVP));
		std::memcpy(fs_.reflectionColor, state.reflectionColor, sizeof(fs_.reflectionColor));
		std::memcpy(fs_.lightColor, state.lightColor, sizeof(fs_.lightColor));
		std::memcpy(fs_.lightDirection, state.lightDirection, sizeof(state.lightDirection));
		std::memcpy(fs_.cameraPos, state.cameraPos, sizeof(state.cameraPos));
		fs_.params[0] = state.brightness;
	}
	else
		std::memcpy(fs_.ps11Color, state.ps11Color, sizeof(fs_.ps11Color));

	// Distance fog. Off -> (0,0,0,1), i.e. factor 1, and the shader's lerp is the identity.
	cSDLRenderDevice* dev = sdlRenderDevice();
	const Vect4f fogPlane = dev ? dev->fogPlane(camera) : Vect4f(0.f, 0.f, 0.f, 1.f);
	vs_.fogPlane[0] = fogPlane.x; vs_.fogPlane[1] = fogPlane.y;
	vs_.fogPlane[2] = fogPlane.z; vs_.fogPlane[3] = fogPlane.w;
	const Color4f fog = dev ? dev->fogColor() : Color4f(0.f, 0.f, 0.f, 0.f);
	fs_.fogColor[0] = fog.r; fs_.fogColor[1] = fog.g; fs_.fogColor[2] = fog.b; fs_.fogColor[3] = fog.a;

	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;

	stateValid_ = true;
}

// The ice sheet's counterpart to SetState, from cTemperature::Draw. Captures the material
// textures and the reflection, reads the frame-constant fog / fog-of-war / lightmap off the
// device (as SetState does), and switches recording to the ice list for the DrawPolygons that
// follows.
void SDLWaterRenderer::SetIceState(const IceState& state, Camera* camera)
{
	if(!camera) return;

	iceMode_ = true;
	iceDraws_.clear();

	iceVS_ = IceVSUniform();
	iceFS_ = IceFSUniform();

	std::memcpy(iceVS_.mvp, &camera->matViewProj, sizeof(iceVS_.mvp));
	std::memcpy(iceVS_.mirrorVP, state.mirrorVP, sizeof(iceVS_.mirrorVP));
	// The original's fScaleBumpSnow (ShaderSceneWaterIce::Select): bump 0.01, snow 0.003,
	// cleft 0.002.
	iceVS_.scaleBumpSnow[0] = 0.01f; iceVS_.scaleBumpSnow[1] = 0.003f; iceVS_.scaleBumpSnow[2] = 0.002f;
	iceVS_.alphaScale[0] = state.alphaScale[0]; iceVS_.alphaScale[1] = state.alphaScale[1];

	iceSnow_       = sdlTextureOf(state.snow);
	iceBump_       = sdlTextureOf(state.bump);
	iceCleft_      = sdlTextureOf(state.cleft);
	iceCoverage_   = sdlTextureOf(state.coverage);
	iceReflection_ = sdlTextureOf(state.reflectionTexture);

	std::memcpy(iceFS_.snowColor, state.snowColor, sizeof(iceFS_.snowColor));
	iceFS_.params[0] = iceReflection_ ? 1.f : 0.f;   // a reflection target to sample
	iceFS_.params[2] = state.alphaRef;               // the clip() test reference

	cSDLRenderDevice* dev = sdlRenderDevice();

	// The fog-of-war lightmap + planar node, exactly as the terrain-ice path reads them.
	iceLightMap_ = sdlTextureOf(dev ? dev->GetLightMap() : nullptr);
	const bool fogOfWar = dev && dev->fogOfWar() && iceLightMap_;
	iceFS_.params[1] = fogOfWar ? 1.f : 0.f;
	const Color4f fow = dev ? dev->fogOfWarColor() : Color4f();
	iceFS_.fogOfWarColor[0] = fow.r; iceFS_.fogOfWarColor[1] = fow.g;
	iceFS_.fogOfWarColor[2] = fow.b; iceFS_.fogOfWarColor[3] = fow.a;
	if(dev){
		const Vect4f& pn = dev->planarTransform();
		iceVS_.planarNode[0] = pn.x; iceVS_.planarNode[1] = pn.y;
		iceVS_.planarNode[2] = pn.z; iceVS_.planarNode[3] = pn.w;
	}
	else{
		iceVS_.planarNode[2] = iceVS_.planarNode[3] = 1.f;
	}

	// Distance fog. Off -> (0,0,0,1), i.e. factor 1, and the shader's lerp is the identity.
	const Vect4f fogPlane = dev ? dev->fogPlane(camera) : Vect4f(0.f, 0.f, 0.f, 1.f);
	iceVS_.fogPlane[0] = fogPlane.x; iceVS_.fogPlane[1] = fogPlane.y;
	iceVS_.fogPlane[2] = fogPlane.z; iceVS_.fogPlane[3] = fogPlane.w;
	const Color4f fog = dev ? dev->fogColor() : Color4f(0.f, 0.f, 0.f, 0.f);
	iceFS_.fogColor[0] = fog.r; iceFS_.fogColor[1] = fog.g; iceFS_.fogColor[2] = fog.b; iceFS_.fogColor[3] = fog.a;

	iceVpX_ = camera->vp.X; iceVpY_ = camera->vp.Y;
	iceVpW_ = camera->vp.Width; iceVpH_ = camera->vp.Height;
	iceVpMinZ_ = camera->vp.MinZ; iceVpMaxZ_ = camera->vp.MaxZ;

	iceValid_ = true;
}

void SDLWaterRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex,
                                            const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon)
{
	// In ice mode the same surface polygons are being recorded for the ice pass instead.
	if(!owner_ || nPolygon <= 0 || (iceMode_ ? !iceValid_ : !stateValid_))
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
	(iceMode_ ? iceDraws_ : draws_).push_back(d);

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
	// The technique the recorded draws were made under, and within it the fill mode --
	// falling back to the solid pipeline if the LINE variant failed to build.
	SDL_GPUGraphicsPipeline* fill = reflection_ ? pipelineReflectFill_ : pipelineFill_;
	SDL_GPUGraphicsPipeline* line = reflection_ ? pipelineReflectLine_ : pipelineLine_;
	SDL_GPUGraphicsPipeline* pipeline = (wireframe && line) ? line : fill;
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

	SDL_GPUTextureSamplerBinding ts[3] = {};
	ts[0].texture = texture0_ ? texture0_ : flatTexture_;
	ts[0].sampler = sampler_;
	ts[1].texture = texture1_ ? texture1_ : flatTexture_;
	ts[1].sampler = sampler_;
	if(reflection_){
		ts[2].texture = reflectionTexture_;
		ts[2].sampler = samplerClamp_;
	}
	SDL_BindGPUFragmentSamplers(pass, 0, ts, reflection_ ? 3 : 2);

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

bool SDLWaterRenderer::DrawIce(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                               int screenW, int screenH, bool clear, const float clearColor[4],
                               bool clearDepth, bool wireframe)
{
	SDL_GPUGraphicsPipeline* pipeline = (wireframe && pipelineIceLine_) ? pipelineIceLine_ : pipelineIceFill_;
	if(!device_ || !pipeline || !cmd || !target || !depth || iceDraws_.empty())
		return false;
	// The ice sheet needs its coverage grid and snow colour; without them there is nothing to
	// draw, and the coverage stand-in (flatTexture_, alpha 1) would flood the map with ice.
	if(!iceCoverage_ || !iceSnow_){
		iceDraws_.clear();
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

	sViewPort vp;
	vp.X = iceVpX_; vp.Y = iceVpY_; vp.Width = iceVpW_; vp.Height = iceVpH_;
	vp.MinZ = iceVpMinZ_; vp.MaxZ = iceVpMaxZ_;
	applyCameraViewport(pass, vp, screenW, screenH);

	SDL_BindGPUGraphicsPipeline(pass, pipeline);
	SDL_PushGPUVertexUniformData(cmd, 0, &iceVS_, sizeof(iceVS_));
	SDL_PushGPUFragmentUniformData(cmd, 0, &iceFS_, sizeof(iceFS_));

	// Stand-ins where a texture is missing: flatTexture_ unbiases to a zero bump / a neutral
	// (0.5) cleft, and the reflection/lightmap are gated off in the shader when absent.
	SDL_GPUTextureSamplerBinding ts[6] = {};
	ts[0].texture = iceCoverage_;   ts[0].sampler = samplerClamp_;   // the coverage grid (clamp)
	ts[1].texture = iceSnow_;       ts[1].sampler = sampler_;        // snow (wrap)
	ts[2].texture = iceBump_  ? iceBump_  : flatTexture_; ts[2].sampler = sampler_;
	ts[3].texture = iceReflection_ ? iceReflection_ : flatTexture_; ts[3].sampler = samplerClamp_;
	ts[4].texture = iceCleft_ ? iceCleft_ : flatTexture_; ts[4].sampler = sampler_;
	ts[5].texture = iceLightMap_ ? iceLightMap_ : flatTexture_; ts[5].sampler = samplerClamp_;
	SDL_BindGPUFragmentSamplers(pass, 0, ts, 6);

	SDL_GPUBuffer* boundVB = nullptr;
	SDL_GPUBuffer* boundIB = nullptr;
	for(const DrawCmd& d : iceDraws_){
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
	iceDraws_.clear();
	return true;
}

