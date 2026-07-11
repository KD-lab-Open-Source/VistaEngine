// SDL GPU 3dx object renderer. See header.
#include "StdAfxRD.h"
#include "SDLObject3dxRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / GetPos / GetLighting / IsShadow
#include "Scene.h"             // cScene::GetShadowIntensity (the shader's vShade)
#include "VisGeneric.h"        // Option_filterShadow (the original's FILTER_SHADOW)
#include "VisGenericDefine.h"  // ATTRCAMERA_SHADOWMAP
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // owner: resolves sPtr buffers, holds RS_ZWRITEENABLE

// Cross-compiled object3dx shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/object3dx_shaders.h"

namespace {

// The most bone poses one material group can reference: StaticBunch::max_index, and
// MAX_BONES in object3dx.vert.hlsl. Not Static3dxBase.h's MAX_BONES, which is the 4 bones
// a single *vertex* may be weighted to.
const int MAX_BONE_POSES = 20;

// The vertex cbuffer is always pushed whole, so the shader's World[60] is fully backed
// even though a bunch rarely uses all 20 bones: MVP + 8 float4s + mShadow + 60 float4s.
const int VS_UNIFORM_FLOATS = 16 + 8 * 4 + 16 + MAX_BONE_POSES * 3 * 4;   // 304 -> 1216 bytes

// D3DRS_ALPHAREF the original sets for ALPHA_TEST (BLEND_STATE_ALPHA_REF in D3DRender.h).
const float ALPHA_TEST_REF = 80.f / 255.f;

// The engine's .3dx vertex offsets, from cSkinVertex (Render/inc/VertexFormat.h):
// pos float3 @0, blend indices D3DCOLOR @12, normal float3 @16, then -- only when the
// lod binds more than one bone per vertex -- weights D3DCOLOR @28, then uv float2, then
// -- only when cStatic3dx::bump -- binormal and tangent float3s. Anything past that
// (uv2, fur) we don't read, but it does grow the stride.
const int OFS_POSITION = 0;
const int OFS_INDICES  = 12;
const int OFS_NORMAL   = 16;
const int OFS_WEIGHTS  = 28;   // uv sits here when the vertex has no weights

// Look up the SDL texture a cTexture is backed by, picking the animation frame the way
// cD3DRender::SetTexturePhase does. Null => untextured (the white stand-in).
SDL_GPUTexture* sdlTextureOf(cTexture* t, float phase)
{
	if(!t) return nullptr;
	const int frames = t->frameNumber();
	if(frames < 1) return nullptr;
	const int frame = frames > 1 ? (int)(0.999f * phase * frames) : 0;
	return reinterpret_cast<SDL_GPUTexture*>(t->GetDDSurface(frame));
}

void setVec4(float* dst, const Color4f& c)
{
	dst[0] = c.r; dst[1] = c.g; dst[2] = c.b; dst[3] = c.a;
}

} // namespace

SDLObject3dxRenderer::SDLObject3dxRenderer(cSDLRenderDevice* owner, SDL_GPUDevice* device, SDL_Window* window)
	: owner_(owner), device_(device), window_(window)
{
	if(!device_) return;

	// The original binds these with sampler_wrap_anisotropic / sampler_clamp_anisotropic,
	// over the full mip chain the cached DDS ship. max_lod must be set: it defaults to 0,
	// which pins sampling to the top level however many the texture has.
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

	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerClamp_ = SDL_CreateGPUSampler(device_, &si);

	// The shadow map is compared, not filtered: raw depth, and a receiver outside the map
	// must read its edge rather than wrap. The original's sampler_clamp_point.
	SDL_GPUSamplerCreateInfo ssi = {};
	ssi.min_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mag_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	ssi.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerShadow_ = SDL_CreateGPUSampler(device_, &ssi);

	// 1x1 white. The fragment shader ignores the sampler for an untextured material
	// (Params.y == 0), but SDL GPU still requires the binding to exist.
	SDL_GPUTextureCreateInfo wti = {};
	wti.type = SDL_GPU_TEXTURETYPE_2D;
	wti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	wti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	wti.width = 1; wti.height = 1; wti.layer_count_or_depth = 1; wti.num_levels = 1;
	whiteTexture_ = SDL_CreateGPUTexture(device_, &wti);
	if(whiteTexture_){
		SDL_GPUTransferBufferCreateInfo wtb = {};
		wtb.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		wtb.size = 4;
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &wtb)){
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
}

SDLObject3dxRenderer::~SDLObject3dxRenderer()
{
	if(!device_) return;
	for(auto& p : pipelines_)
		if(p.second) SDL_ReleaseGPUGraphicsPipeline(device_, p.second);
	if(whiteTexture_)  SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(samplerWrap_)   SDL_ReleaseGPUSampler(device_, samplerWrap_);
	if(samplerClamp_)  SDL_ReleaseGPUSampler(device_, samplerClamp_);
	if(samplerShadow_) SDL_ReleaseGPUSampler(device_, samplerShadow_);
	if(vsRigid_)       SDL_ReleaseGPUShader(device_, vsRigid_);
	if(vsSkin_)        SDL_ReleaseGPUShader(device_, vsSkin_);
	if(vsRigidBump_)   SDL_ReleaseGPUShader(device_, vsRigidBump_);
	if(vsSkinBump_)    SDL_ReleaseGPUShader(device_, vsSkinBump_);
	if(fs_)            SDL_ReleaseGPUShader(device_, fs_);
	if(fsBump_)        SDL_ReleaseGPUShader(device_, fsBump_);
	if(vsShadowRigid_) SDL_ReleaseGPUShader(device_, vsShadowRigid_);
	if(vsShadowSkin_)  SDL_ReleaseGPUShader(device_, vsShadowSkin_);
	if(fsShadow_)      SDL_ReleaseGPUShader(device_, fsShadow_);
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------
bool SDLObject3dxRenderer::createShaders()
{
	if(shadersTried_) return vsRigid_ && vsSkin_ && vsRigidBump_ && vsSkinBump_ && fs_ && fsBump_
	                      && vsShadowRigid_ && vsShadowSkin_ && fsShadow_;
	shadersTried_ = true;
	if(!device_ || !window_) return false;

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *rigidCode, *skinCode, *rigidBumpCode, *skinBumpCode, *fsCode, *fsBumpCode;
	const unsigned char *shRigidCode, *shSkinCode, *shFsCode;
	unsigned int rigidSize, skinSize, rigidBumpSize, skinBumpSize, fsSize, fsBumpSize;
	unsigned int shRigidSize, shSkinSize, shFsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		rigidCode     = object3dx_rigid_vert_msl;      rigidSize     = object3dx_rigid_vert_msl_len;
		skinCode      = object3dx_skin_vert_msl;       skinSize      = object3dx_skin_vert_msl_len;
		rigidBumpCode = object3dx_rigid_bump_vert_msl; rigidBumpSize = object3dx_rigid_bump_vert_msl_len;
		skinBumpCode  = object3dx_skin_bump_vert_msl;  skinBumpSize  = object3dx_skin_bump_vert_msl_len;
		fsCode        = object3dx_frag_msl;            fsSize        = object3dx_frag_msl_len;
		fsBumpCode    = object3dx_bump_frag_msl;       fsBumpSize    = object3dx_bump_frag_msl_len;
		shRigidCode   = object3dx_shadow_rigid_vert_msl; shRigidSize = object3dx_shadow_rigid_vert_msl_len;
		shSkinCode    = object3dx_shadow_skin_vert_msl;  shSkinSize  = object3dx_shadow_skin_vert_msl_len;
		shFsCode      = object3dx_shadow_frag_msl;       shFsSize    = object3dx_shadow_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		rigidCode     = object3dx_rigid_vert_spv;      rigidSize     = object3dx_rigid_vert_spv_len;
		skinCode      = object3dx_skin_vert_spv;       skinSize      = object3dx_skin_vert_spv_len;
		rigidBumpCode = object3dx_rigid_bump_vert_spv; rigidBumpSize = object3dx_rigid_bump_vert_spv_len;
		skinBumpCode  = object3dx_skin_bump_vert_spv;  skinBumpSize  = object3dx_skin_bump_vert_spv_len;
		fsCode        = object3dx_frag_spv;            fsSize        = object3dx_frag_spv_len;
		fsBumpCode    = object3dx_bump_frag_spv;       fsBumpSize    = object3dx_bump_frag_spv_len;
		shRigidCode   = object3dx_shadow_rigid_vert_spv; shRigidSize = object3dx_shadow_rigid_vert_spv_len;
		shSkinCode    = object3dx_shadow_skin_vert_spv;  shSkinSize  = object3dx_shadow_skin_vert_spv_len;
		shFsCode      = object3dx_shadow_frag_spv;       shFsSize    = object3dx_shadow_frag_spv_len;
	} else {
		fprintf(stderr, "SDLObject3dxRenderer: no supported shader format (0x%x)\n", formats);
		return false;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.entrypoint = entry; vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + material + bone matrices
	vsi.code = rigidCode;     vsi.code_size = rigidSize;     vsRigid_       = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = skinCode;      vsi.code_size = skinSize;      vsSkin_        = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = rigidBumpCode; vsi.code_size = rigidBumpSize; vsRigidBump_   = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = skinBumpCode;  vsi.code_size = skinBumpSize;  vsSkinBump_    = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = shRigidCode;   vsi.code_size = shRigidSize;   vsShadowRigid_ = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = shSkinCode;    vsi.code_size = shSkinSize;    vsShadowSkin_  = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.entrypoint = entry; fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_uniform_buffers = 1;    // material colours + skin-colour lerp + flags
	fsi.code = fsCode; fsi.code_size = fsSize;
	fsi.num_samplers = 2;           // diffuse + shadow map
	fs_ = SDL_CreateGPUShader(device_, &fsi);
	fsi.code = fsBumpCode; fsi.code_size = fsBumpSize;
	fsi.num_samplers = 4;           // diffuse + bump + specular map + shadow map
	fsBump_ = SDL_CreateGPUShader(device_, &fsi);
	fsi.code = shFsCode; fsi.code_size = shFsSize;
	fsi.num_samplers = 1;           // diffuse, for the alpha-cutout clip
	fsShadow_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vsRigid_ || !vsSkin_ || !vsRigidBump_ || !vsSkinBump_ || !fs_ || !fsBump_
	   || !vsShadowRigid_ || !vsShadowSkin_ || !fsShadow_){
		fprintf(stderr, "SDLObject3dxRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stderr, "SDLObject3dxRenderer: object3dx shaders ready (plain + bump + shadow)\n");
	return true;
}

SDL_GPUGraphicsPipeline* SDLObject3dxRenderer::pipelineFor(int stride, bool skinned, bool bump,
                                                           eBlendMode blend, bool depthWrite,
                                                           bool wireframe, bool shadow)
{
	// The caster shaders take no tangent frame and write no colour, so bump and the blend
	// mode never reach them: fold them out of the key rather than build dead pipelines.
	if(shadow){
		bump = false;
		blend = (blend == ALPHA_TEST) ? ALPHA_TEST : ALPHA_NONE;
		depthWrite = true;
		wireframe = false;
	}

	const unsigned long long key = (unsigned long long)(unsigned)stride
	                             | ((unsigned long long)skinned    << 16)
	                             | ((unsigned long long)blend      << 17)
	                             | ((unsigned long long)depthWrite << 24)
	                             | ((unsigned long long)wireframe  << 25)
	                             | ((unsigned long long)bump       << 26)
	                             | ((unsigned long long)shadow     << 27);
	auto it = pipelines_.find(key);
	if(it != pipelines_.end())
		return it->second;

	if(!createShaders()){
		pipelines_[key] = nullptr;
		return nullptr;
	}

	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = (Uint32)stride;
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	// Attribute locations follow the HLSL struct field order; the uv slides one slot
	// down when the weights are present, and the tangent frame follows it. Blend indices
	// arrive as raw bytes (the shader takes them in memory order, as D3DCOLORtoUBYTE4
	// did); weights are normalized.
	SDL_GPUVertexAttribute attrs[7] = {};
	int n = 0;
	int loc = 0;
	attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = OFS_POSITION; n++;
	attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4; attrs[n].offset = OFS_INDICES; n++;
	attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = OFS_NORMAL; n++;
	if(skinned){
		attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[n].offset = OFS_WEIGHTS; n++;
	}
	const int ofsUV = skinned ? OFS_WEIGHTS + 4 : OFS_WEIGHTS;
	attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; attrs[n].offset = (Uint32)ofsUV; n++;
	if(bump){
		// cSkinVertex: offset_bump_s (BINORMAL) then offset_bump_t (TANGENT), the two
		// float3s straight after the uv.
		attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = (Uint32)(ofsUV + 8); n++;
		attrs[n].location = loc++; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = (Uint32)(ofsUV + 20); n++;
	}

	// The blend modes cObject3dx::Draw asks for, as cD3DRender::SetBlendState builds
	// them. ALPHA_TEST is not a blend at all: it is a clip in the fragment shader.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	SDL_GPUColorTargetBlendState& bs = colorTarget.blend_state;
	bs.color_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	switch(blend){
	case ALPHA_BLEND:
		bs.enable_blend = true;
		bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case ALPHA_ADDBLENDALPHA:                       // dst = dst + src*alpha
		bs.enable_blend = true;
		bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		break;
	case ALPHA_ADDBLEND:                            // dst = dst + src
		bs.enable_blend = true;
		bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		break;
	case ALPHA_SUBBLEND:                            // dst = dst - src
		bs.enable_blend = true;
		bs.color_blend_op = SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
		bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		break;
	default:                                        // ALPHA_NONE, ALPHA_TEST
		bs.enable_blend = false;
		break;
	}

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = shadow ? (skinned ? vsShadowSkin_ : vsShadowRigid_)
	                  : bump   ? (skinned ? vsSkinBump_ : vsRigidBump_)
	                           : (skinned ? vsSkin_ : vsRigid_);
	pci.fragment_shader = shadow ? fsShadow_ : (bump ? fsBump_ : fs_);
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = (Uint32)n;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// The scene passes flip culling per node type (DrawObjectSpecial and DrawSortObject
	// both force D3DCULL_NONE), and the SDL device does not track D3DRS_CULLMODE yet.
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	if(shadow){
		// D3DRS_SLOPESCALEDEPTHBIAS = 2 in CameraShadowMap::DrawScene: push a caster's
		// depth away from the light in proportion to its slope, so a surface lit at a
		// grazing angle does not shadow itself.
		pci.rasterizer_state.enable_depth_bias = true;
		pci.rasterizer_state.depth_bias_slope_factor = 2.f;
		pci.rasterizer_state.depth_bias_constant_factor = 0.f;
	}
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = depthWrite;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	// The caster pass has no colour target at all: it only fills the depth texture.
	pci.target_info.color_target_descriptions = shadow ? nullptr : &colorTarget;
	pci.target_info.num_color_targets = shadow ? 0 : 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	if(!pipeline)
		fprintf(stderr, "SDLObject3dxRenderer: pipeline (stride %d, skinned %d, bump %d, blend %d, shadow %d) failed: %s\n",
		        stride, (int)skinned, (int)bump, (int)blend, (int)shadow, SDL_GetError());
	pipelines_[key] = pipeline;
	return pipeline;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLObject3dxRenderer::BeginFrame()
{
	states_.clear();
	draws_.clear();
	shadowDraws_.clear();
	worldPool_.clear();
	currentValid_ = false;
	currentDirty_ = true;
	currentShadow_ = false;
}

void SDLObject3dxRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera) return;

	// Which pass the draws that follow belong to. The light camera walks the scene first
	// (Camera::DrawScene runs its children before its own draws), so the caster draws are
	// recorded, and their pass replayed, before anything samples the map.
	currentShadow_ = camera->getAttribute(ATTRCAMERA_SHADOWMAP) != 0;

	// The bone poses, as VSSkin::Select uploads them: three float4 rows of (R | T) per
	// bone, so the shader's world.k == dot(float4(pos,1), row k).
	const int bones = state.world ? (state.worldNum < MAX_BONE_POSES ? state.worldNum : MAX_BONE_POSES) : 0;
	current_.worldOffset = (int)worldPool_.size();
	current_.worldRows = bones * 3;
	for(int i = 0; i < bones; i++){
		const MatXf& w = state.world[i];
		const float rows[12] = {
			w.R.xx, w.R.xy, w.R.xz, w.d.x,
			w.R.yx, w.R.yy, w.R.yz, w.d.y,
			w.R.zx, w.R.zy, w.R.zz, w.d.z,
		};
		worldPool_.insert(worldPool_.end(), rows, rows + 12);
	}

	current_.vpX = camera->vp.X;       current_.vpY = camera->vp.Y;
	current_.vpW = camera->vp.Width;   current_.vpH = camera->vp.Height;
	current_.vpMinZ = camera->vp.MinZ; current_.vpMaxZ = camera->vp.MaxZ;

	std::memcpy(current_.vs.mvp, &camera->matViewProj, sizeof(current_.vs.mvp));
	setVec4(current_.vs.ambient, state.ambient);
	setVec4(current_.vs.diffuse, state.diffuse);
	setVec4(current_.vs.specular, state.specular);   // .a = specular power

	const Vect3f eye = camera->GetPos();
	current_.vs.cameraPos[0] = eye.x; current_.vs.cameraPos[1] = eye.y;
	current_.vs.cameraPos[2] = eye.z; current_.vs.cameraPos[3] = 0.f;

	Vect3f light(0.f, 0.f, -1.f);   // straight down if the scene has no sun yet
	camera->GetLighting(light);
	current_.vs.lightDir[0] = light.x; current_.vs.lightDir[1] = light.y;
	current_.vs.lightDir[2] = light.z; current_.vs.lightDir[3] = 0.f;

	// vUtrans/vVtrans, packed as VSSkin::SetUVTrans does; w flags the transform on.
	if(state.hasUVTrans){
		current_.vs.uTrans[0] = state.uvTrans[0]; current_.vs.uTrans[1] = state.uvTrans[2];
		current_.vs.uTrans[2] = state.uvTrans[4]; current_.vs.uTrans[3] = 1.f;
		current_.vs.vTrans[0] = state.uvTrans[1]; current_.vs.vTrans[1] = state.uvTrans[3];
		current_.vs.vTrans[2] = state.uvTrans[5]; current_.vs.vTrans[3] = 0.f;
	}
	else{
		std::memset(current_.vs.uTrans, 0, sizeof(current_.vs.uTrans));
		std::memset(current_.vs.vTrans, 0, sizeof(current_.vs.vTrans));
	}

	const int boneCount = state.boneCount < 1 ? 1 : (state.boneCount > 4 ? 4 : state.boneCount);
	current_.vs.params[0] = (float)boneCount;
	current_.vs.params[1] = state.noLight ? 1.f : 0.f;
	current_.vs.params[2] = current_.vs.params[3] = 0.f;

	// The fragment ambient is the original's bumpAmbient. A no_light material folds its
	// ambient into the vertex diffuse instead (the original's NOLIGHT branch skips the
	// `ot.rgb += bumpAmbient*t0` term), so zero it here rather than carry a shader flag.
	if(state.noLight)
		std::memset(current_.fs.ambient, 0, sizeof(current_.fs.ambient));
	else
		setVec4(current_.fs.ambient, state.ambient);

	// bumpDiffuse / bumpSpecular: read only by the bump path, which has no per-vertex
	// diffuse to interpolate.
	setVec4(current_.fs.diffuse, state.diffuse);
	setVec4(current_.fs.specular, state.specular);   // .a = specular power

	// PSSkin::SetMaterial's premultiplied skin-colour lerp: rgb = c.rgb*c.a, a = 1-c.a.
	const bool lerp = state.lerpColor.a > 0.001f;
	current_.fs.lerpPre[0] = state.lerpColor.r * state.lerpColor.a;
	current_.fs.lerpPre[1] = state.lerpColor.g * state.lerpColor.a;
	current_.fs.lerpPre[2] = state.lerpColor.b * state.lerpColor.a;
	current_.fs.lerpPre[3] = 1.f - state.lerpColor.a;

	current_.texture = sdlTextureOf(state.texture, state.texturePhase);
	current_.bumpTexture = sdlTextureOf(state.bumpTexture, state.texturePhase);
	current_.specularTexture = sdlTextureOf(state.specularMap, state.texturePhase);
	current_.sampler = state.tilingWrap ? samplerWrap_ : samplerClamp_;

	// The bump fragment shader always samples the diffuse map (the original has no
	// NOTEXTURE variant of psSkinBump), so an untextured material can't take that path.
	current_.bump = current_.bumpTexture != nullptr && current_.texture != nullptr;

	current_.fs.params[0] = state.blend == ALPHA_TEST ? ALPHA_TEST_REF : 0.f;
	current_.fs.params[1] = current_.texture ? 1.f : 0.f;
	current_.fs.params[2] = state.selfIllumination ? 1.f : 0.f;
	current_.fs.params[3] = lerp ? 1.f : 0.f;
	current_.fs.params2[0] = current_.specularTexture ? 1.f : 0.f;
	current_.fs.params2[1] = current_.fs.params2[2] = current_.fs.params2[3] = 0.f;

	// Whether this material takes the original's *SceneShadow shader variant, on the same
	// terms cObject3dx::Draw and cSimply3dx::SelectMaterial pick it: the main camera has a
	// light child (Camera::IsShadow), the material is lit, and it has a diffuse texture --
	// an untextured one gets vsSkinSceneShadow but plain psSkin, which never samples.
	//
	// The map is already filled: Camera::DrawScene walks its child cameras, and so runs
	// the light camera's whole caster pass, before drawing any of its own objects.
	const bool receives = !currentShadow_ && !state.noLight && current_.texture
	                   && camera->IsShadow() && owner_->shadowPassRan() && owner_->GetShadowMap();
	if(receives){
		const Mat4f mShadow = owner_->shadowMatViewProj() * owner_->shadowMatBias();
		std::memcpy(current_.vs.shadow, &mShadow, sizeof(current_.vs.shadow));
		current_.shadowTexture = reinterpret_cast<SDL_GPUTexture*>(owner_->GetShadowMap()->GetDDSurface(0));
		// PSSkin::SetShadowIntensity(scene()->GetShadowIntensity()).
		if(cScene* scene = camera->scene())
			setVec4(current_.fs.shade, scene->GetShadowIntensity());
		else
			current_.fs.shade[0] = current_.fs.shade[1] = current_.fs.shade[2] = current_.fs.shade[3] = 1.f;
	}
	else{
		std::memset(current_.vs.shadow, 0, sizeof(current_.vs.shadow));
		current_.shadowTexture = nullptr;
		current_.fs.shade[0] = current_.fs.shade[1] = current_.fs.shade[2] = current_.fs.shade[3] = 1.f;
	}
	current_.fs.shadowParams[0] = current_.shadowTexture ? 1.f : 0.f;
	// FILTER_SHADOW. A static shader define in the original (PSSkin::Select hands
	// Option_filterShadow to StaticSelect); a uniform here, so it costs no extra variant.
	current_.fs.shadowParams[1] = (current_.shadowTexture && Option_filterShadow) ? 1.f : 0.f;
	current_.fs.shadowParams[2] = current_.fs.shadowParams[3] = 0.f;

	current_.blend = state.blend;
	current_.skinned = boneCount > 1;

	currentValid_ = true;
	currentDirty_ = true;
}

void SDLObject3dxRenderer::SetAlphaColor(const Color4f& color)
{
	if(!currentValid_) return;
	// The original writes both: VSSkin::SetAlphaColor sets vDiffuse (the per-vertex lit
	// colour) and PSSkin::SetAlphaColor sets bumpDiffuse (what the bump path shades with).
	setVec4(current_.vs.diffuse, color);
	setVec4(current_.fs.diffuse, color);
	currentDirty_ = true;
}

int SDLObject3dxRenderer::commitState()
{
	if(currentDirty_){
		states_.push_back(current_);
		currentDirty_ = false;
	}
	return (int)states_.size() - 1;
}

void SDLObject3dxRenderer::DrawIndexedPrimitive(sPtrVertexBuffer& vb, int OfsVertex,
                                                const sPtrIndexBuffer& ib, int nOfsPolygon, int nPolygon)
{
	// A draw issued before cObject3dx::Draw has set any state would inherit a stale
	// material; drop it. (Nothing else routes through here yet.)
	if(!owner_ || !currentValid_ || nPolygon <= 0)
		return;

	const int stride = vb.GetVertexSize();
	SDL_GPUBuffer* vertexBuffer = owner_->gpuBuffer(vb);
	SDL_GPUBuffer* indexBuffer = owner_->gpuBuffer(ib);
	if(!vertexBuffer || !indexBuffer || stride <= 0)
		return;

	// OfsVertex is deliberately unused. cStatic3dx::BuildBuffers already folds each
	// bunch's vertex offset into the indices it writes (out.p1 = in.p1 +
	// cur_vertex_offset), and D3D takes OfsVertex as MinVertexIndex -- a range hint it
	// adds to nothing. SDL's vertex_offset *is* added to every index, so it stays 0;
	// passing OfsVertex would shift every bunch after the first into the next one's
	// vertices.
	(void)OfsVertex;

	DrawCmd d;
	d.state = commitState();
	d.vertexBuffer = vertexBuffer;
	d.indexBuffer = indexBuffer;
	d.stride = stride;
	// The index buffer is a 16-bit triangle list, so D3D's polygon offsets are 3x index
	// offsets -- the same arithmetic cD3DRender::DrawIndexedPrimitive hands to D3D.
	d.firstIndex = 3 * nOfsPolygon;
	d.indexCount = 3 * nPolygon;
	d.depthWrite = currentShadow_ ? true : owner_->zWriteEnable();
	(currentShadow_ ? shadowDraws_ : draws_).push_back(d);

	*gb_RenderDevice->PtrNumberPolygon += nPolygon;
	gb_RenderDevice->NumDrawObject++;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
bool SDLObject3dxRenderer::DrawShadowPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* depth, int size,
                                          bool clearDepth)
{
	if(!device_ || !cmd || !depth)
		return false;

	// Runs even with nothing to cast: the clear alone leaves the map at far depth, i.e.
	// every receiver lit. Skipping it would let them sample last frame's map, or on the
	// first frame an undefined one. When the terrain caster pass already cleared and
	// filled the map, load its depth instead and add the objects to it.
	//
	// Depth only: no colour attachment at all.
	SDL_GPUDepthStencilTargetInfo dt = {};
	dt.texture = depth;
	dt.clear_depth = 1.0f;
	dt.load_op = clearDepth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	dt.store_op = SDL_GPU_STOREOP_STORE;
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, nullptr, 0, &dt);

	SDL_GPUGraphicsPipeline* boundPipeline = nullptr;
	SDL_GPUBuffer* boundVB = nullptr;
	SDL_GPUBuffer* boundIB = nullptr;
	int boundState = -1;
	float vsUniform[VS_UNIFORM_FLOATS];

	for(const DrawCmd& d : shadowDraws_){
		const StateBlock& st = states_[d.state];

		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, st.skinned, false, st.blend, true, false, true);
		if(!pipeline) continue;
		if(pipeline != boundPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			boundPipeline = pipeline;
			boundState = -1;
			boundVB = boundIB = nullptr;
		}

		if(d.state != boundState){
			// The light camera's viewport is the whole map (SetFrustum gives it the unit
			// clip rect), but take it from the state like every other pass does.
			sViewPort vp;
			vp.X = st.vpX; vp.Y = st.vpY; vp.Width = st.vpW; vp.Height = st.vpH;
			vp.MinZ = st.vpMinZ; vp.MaxZ = st.vpMaxZ;
			applyCameraViewport(pass, vp, size, size);

			// st.vs.mvp is the light's matViewProj: SetState read it from whichever
			// camera it was handed, and the caster shader wants exactly that.
			std::memcpy(vsUniform, &st.vs, sizeof(st.vs));
			const int head = (int)(sizeof(st.vs) / sizeof(float));
			if(st.worldRows > 0)
				std::memcpy(vsUniform + head, worldPool_.data() + st.worldOffset,
				            (size_t)st.worldRows * 4 * sizeof(float));
			std::memset(vsUniform + head + st.worldRows * 4, 0,
			            (size_t)(VS_UNIFORM_FLOATS - head - st.worldRows * 4) * sizeof(float));

			SDL_PushGPUVertexUniformData(cmd, 0, vsUniform, sizeof(vsUniform));
			SDL_PushGPUFragmentUniformData(cmd, 0, &st.fs, sizeof(st.fs));

			// Only the diffuse map, for the cutout clip.
			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = st.texture ? st.texture : whiteTexture_;
			ts.sampler = st.sampler ? st.sampler : samplerWrap_;
			SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
			boundState = d.state;
		}

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
	shadowDraws_.clear();   // recorded; a second endShadowPass this frame is a no-op
	return true;
}

bool SDLObject3dxRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int screenW, int screenH, bool clear, const float clearColor[4],
                                bool clearDepth, bool wireframe)
{
	if(!device_ || !cmd || !target || !depth || draws_.empty())
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
	dt.store_op = SDL_GPU_STOREOP_STORE;
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, &dt);

	// Replay in record order: that is the order the scene walk issued them in, which is
	// what keeps the opaque pass behind the depth-sorted transparent one.
	SDL_GPUGraphicsPipeline* boundPipeline = nullptr;
	SDL_GPUBuffer* boundVB = nullptr;
	SDL_GPUBuffer* boundIB = nullptr;
	int boundState = -1;
	float vsUniform[VS_UNIFORM_FLOATS];

	for(const DrawCmd& d : draws_){
		const StateBlock& st = states_[d.state];

		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, st.skinned, st.bump, st.blend, d.depthWrite, wireframe, false);
		if(!pipeline) continue;
		if(pipeline != boundPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			boundPipeline = pipeline;
			boundState = -1;   // uniforms and bindings do not survive a pipeline change
			boundVB = boundIB = nullptr;
		}

		if(d.state != boundState){
			// The camera's viewport, as SetDrawTransform would have set it at record time.
			sViewPort vp;
			vp.X = st.vpX; vp.Y = st.vpY; vp.Width = st.vpW; vp.Height = st.vpH;
			vp.MinZ = st.vpMinZ; vp.MaxZ = st.vpMaxZ;
			applyCameraViewport(pass, vp, screenW, screenH);

			// Push the cbuffer whole so the shader's World[20] is always fully backed;
			// only the leading worldRows rows carry this bunch's bones.
			std::memcpy(vsUniform, &st.vs, sizeof(st.vs));
			const int head = (int)(sizeof(st.vs) / sizeof(float));
			if(st.worldRows > 0)
				std::memcpy(vsUniform + head, worldPool_.data() + st.worldOffset,
				            (size_t)st.worldRows * 4 * sizeof(float));
			std::memset(vsUniform + head + st.worldRows * 4, 0,
			            (size_t)(VS_UNIFORM_FLOATS - head - st.worldRows * 4) * sizeof(float));

			SDL_PushGPUVertexUniformData(cmd, 0, vsUniform, sizeof(vsUniform));
			SDL_PushGPUFragmentUniformData(cmd, 0, &st.fs, sizeof(st.fs));

			// Every sampler the fragment shader declares must be bound, even where a
			// uniform gates the sample away (Params2.x for the specular map,
			// ShadowParams.x for the shadow map): the white stand-in is never read.
			// The shadow map is last -- slot 1 without bump, slot 3 with.
			SDL_GPUTextureSamplerBinding ts[4] = {};
			ts[0].texture = st.texture ? st.texture : whiteTexture_;
			ts[0].sampler = st.sampler ? st.sampler : samplerWrap_;
			if(st.bump){
				ts[1].texture = st.bumpTexture;
				ts[1].sampler = samplerWrap_;   // the original's sampler_wrap_linear on stage 1
				ts[2].texture = st.specularTexture ? st.specularTexture : whiteTexture_;
				ts[2].sampler = samplerWrap_;
			}
			const int shadowSlot = st.bump ? 3 : 1;
			ts[shadowSlot].texture = st.shadowTexture ? st.shadowTexture : whiteTexture_;
			ts[shadowSlot].sampler = samplerShadow_;
			SDL_BindGPUFragmentSamplers(pass, 0, ts, shadowSlot + 1);
			boundState = d.state;
		}

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
	// Recorded. states_ and worldPool_ stay: the draws still to come this frame index
	// into them, and a state that has not changed is not re-committed.
	draws_.clear();
	return true;
}

#endif // !_WIN32
