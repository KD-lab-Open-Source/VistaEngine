// SDL GPU 3dx object renderer. See header.
#include "StdAfxRD.h"
#include "SDLObject3dxRenderer.h"


#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / GetPos / GetLighting / IsShadow
#include "Scene.h"             // cScene::GetShadowIntensity (the shader's vShade)
#include "VisGeneric.h"        // Option_filterShadow (the original's FILTER_SHADOW)
#include "VisGenericDefine.h"  // ATTRCAMERA_SHADOWMAP
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // owner: resolves sPtr buffers, holds RS_ZWRITEENABLE

// 3dx-object shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/object3dx_shaders.h"
#include "SDLShaders/ShaderBlob.h"

namespace {

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
	if(vsRigidReflect_)SDL_ReleaseGPUShader(device_, vsRigidReflect_);
	if(vsSkinReflect_) SDL_ReleaseGPUShader(device_, vsSkinReflect_);
	if(vsRigidSecondOpacity_) SDL_ReleaseGPUShader(device_, vsRigidSecondOpacity_);
	if(vsSkinSecondOpacity_)  SDL_ReleaseGPUShader(device_, vsSkinSecondOpacity_);
	if(fs_)            SDL_ReleaseGPUShader(device_, fs_);
	if(fsBump_)        SDL_ReleaseGPUShader(device_, fsBump_);
	if(fsReflect_)     SDL_ReleaseGPUShader(device_, fsReflect_);
	if(fsSecondOpacity_) SDL_ReleaseGPUShader(device_, fsSecondOpacity_);
	if(vsShadowRigid_) SDL_ReleaseGPUShader(device_, vsShadowRigid_);
	if(vsShadowSkin_)  SDL_ReleaseGPUShader(device_, vsShadowSkin_);
	if(fsShadow_)      SDL_ReleaseGPUShader(device_, fsShadow_);
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------
bool SDLObject3dxRenderer::createShaders()
{
	if(shadersTried_) return vsRigid_ && vsSkin_ && vsRigidBump_ && vsSkinBump_
	                      && vsRigidReflect_ && vsSkinReflect_
	                      && vsRigidSecondOpacity_ && vsSkinSecondOpacity_
	                      && fs_ && fsBump_ && fsReflect_ && fsSecondOpacity_
	                      && vsShadowRigid_ && vsShadowSkin_ && fsShadow_;
	shadersTried_ = true;
	if(!device_ || !window_) return false;

	// Every vertex shader here takes one uniform block: MVP + material + bone matrices.
	auto makeVS = [&](const vista::ShaderBlob& blob) {
		SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(blob);
		vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
		vsi.num_uniform_buffers = 1;
		return SDL_CreateGPUShader(device_, &vsi);
	};
	vsRigid_        = makeVS(VISTA_SHADER(object3dx_rigid_vert));
	vsSkin_         = makeVS(VISTA_SHADER(object3dx_skin_vert));
	vsRigidBump_    = makeVS(VISTA_SHADER(object3dx_rigid_bump_vert));
	vsSkinBump_     = makeVS(VISTA_SHADER(object3dx_skin_bump_vert));
	vsRigidReflect_ = makeVS(VISTA_SHADER(object3dx_rigid_reflect_vert));
	vsSkinReflect_  = makeVS(VISTA_SHADER(object3dx_skin_reflect_vert));
	vsRigidReflectCube_ = makeVS(VISTA_SHADER(object3dx_rigid_reflect_cube_vert));
	vsSkinReflectCube_  = makeVS(VISTA_SHADER(object3dx_skin_reflect_cube_vert));
	vsRigidSecondOpacity_ = makeVS(VISTA_SHADER(object3dx_rigid_second_opacity_vert));
	vsSkinSecondOpacity_  = makeVS(VISTA_SHADER(object3dx_skin_second_opacity_vert));
	vsShadowRigid_  = makeVS(VISTA_SHADER(object3dx_shadow_rigid_vert));
	vsShadowSkin_   = makeVS(VISTA_SHADER(object3dx_shadow_skin_vert));

	// The fragment shaders share a uniform block (material colours + skin-colour lerp +
	// flags) and differ only in how many textures they sample.
	auto makeFS = [&](const vista::ShaderBlob& blob, Uint32 numSamplers) {
		SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(blob);
		fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
		fsi.num_uniform_buffers = 1;
		fsi.num_samplers = numSamplers;
		return SDL_CreateGPUShader(device_, &fsi);
	};
	fs_        = makeFS(VISTA_SHADER(object3dx_frag),         2);  // diffuse + shadow map
	fsBump_    = makeFS(VISTA_SHADER(object3dx_bump_frag),    4);  // diffuse + bump + specular + shadow map
	fsReflect_ = makeFS(VISTA_SHADER(object3dx_reflect_frag), 3);  // diffuse + env map + shadow map
	fsReflectCube_ = makeFS(VISTA_SHADER(object3dx_reflect_cube_frag), 3);  // the env map is the sky cube
	fsSecondOpacity_ = makeFS(VISTA_SHADER(object3dx_second_opacity_frag), 2);  // diffuse + second-opacity map
	fsShadow_  = makeFS(VISTA_SHADER(object3dx_shadow_frag),  1);  // diffuse, for the alpha-cutout clip

	if(!vsRigid_ || !vsSkin_ || !vsRigidBump_ || !vsSkinBump_ || !vsRigidReflect_ || !vsSkinReflect_
	   || !vsRigidSecondOpacity_ || !vsSkinSecondOpacity_
	   || !fs_ || !fsBump_ || !fsReflect_ || !fsSecondOpacity_
	   || !vsShadowRigid_ || !vsShadowSkin_ || !fsShadow_){
		fprintf(stderr, "SDLObject3dxRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stderr, "SDLObject3dxRenderer: object3dx shaders ready (plain + bump + reflect + second-opacity + shadow)\n");
	return true;
}

SDL_GPUGraphicsPipeline* SDLObject3dxRenderer::pipelineFor(int stride, bool skinned, bool bump, bool reflect,
                                                           bool reflectCube, bool secondOpacity, eBlendMode blend,
                                                           bool mirrored, bool depthWrite, bool wireframe, bool shadow)
{
	// The caster shaders take no tangent frame, no env map and write no colour, so bump,
	// reflection, the second-opacity map and the blend mode never reach them: fold them out
	// of the key rather than build dead pipelines.
	if(shadow){
		bump = false;
		reflect = false;
		reflectCube = false;
		secondOpacity = false;
		blend = (blend == ALPHA_TEST) ? ALPHA_TEST : ALPHA_NONE;
		depthWrite = true;
		wireframe = false;
	}

	const unsigned long long key = (unsigned long long)(unsigned)stride
	                             | ((unsigned long long)skinned       << 16)
	                             | ((unsigned long long)blend         << 17)
	                             | ((unsigned long long)depthWrite    << 24)
	                             | ((unsigned long long)wireframe     << 25)
	                             | ((unsigned long long)bump          << 26)
	                             | ((unsigned long long)shadow        << 27)
	                             | ((unsigned long long)mirrored      << 28)
	                             | ((unsigned long long)reflect       << 29)
	                             | ((unsigned long long)secondOpacity << 30)
	                             | ((unsigned long long)reflectCube   << 31);
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
	pci.vertex_shader = shadow        ? (skinned ? vsShadowSkin_ : vsShadowRigid_)
	                  : secondOpacity ? (skinned ? vsSkinSecondOpacity_ : vsRigidSecondOpacity_)
	                  : reflect       ? (reflectCube ? (skinned ? vsSkinReflectCube_ : vsRigidReflectCube_)
	                                                 : (skinned ? vsSkinReflect_ : vsRigidReflect_))
	                  : bump          ? (skinned ? vsSkinBump_ : vsRigidBump_)
	                                  : (skinned ? vsSkin_ : vsRigid_);
	pci.fragment_shader = shadow ? fsShadow_
	                    : secondOpacity ? fsSecondOpacity_
	                    : reflect ? (reflectCube ? fsReflectCube_ : fsReflect_)
	                    : (bump ? fsBump_ : fs_);
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = (Uint32)n;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// The scene passes flip culling per node type (DrawObjectSpecial and DrawSortObject
	// both force D3DCULL_NONE), and the SDL device does not track D3DRS_CULLMODE yet, so
	// the ordinary cameras keep drawing both faces.
	//
	// The reflection camera cannot: its mirror matrix puts the eye below the terrain, so
	// with nothing culled the heightfield's underside rasterizes as a ceiling between the
	// eye and everything standing on it, and the palms never survive the depth test.
	// cD3DRender::setCamera meets this by culling back faces throughout and flipping the
	// winding for the reflection (D3DCULL_CW -> D3DCULL_CCW), the mirror having reversed
	// every triangle. Cull FRONT here, which with SDL's counter-clockwise front face is
	// that same D3DCULL_CCW.
	pci.rasterizer_state.cull_mode = mirrored ? SDL_GPU_CULLMODE_FRONT : SDL_GPU_CULLMODE_NONE;
	// Clip near/far like D3D9's default, not SDL's depth-clamp default, so an object behind
	// a camera's near plane is clipped rather than clamped onto it (see
	// SDLTileMapRenderer::createPipeline). The shadow caster keeps clamp so casters past the
	// light's far plane still write max depth.
	pci.rasterizer_state.enable_depth_clip = !shadow;
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
	worldPool_.clear();
	currentValid_ = false;
	currentDirty_ = true;
}

void SDLObject3dxRenderer::SetState(const State& state, Camera* camera)
{
	if(!camera) return;

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
	// The reflection variant's sphere-map basis (world -> camera space). Pushed always so
	// the one uniform block stays uniform; only that variant reads it.
	std::memcpy(current_.vs.view, &camera->matView, sizeof(current_.vs.view));
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

	// vSecondUtrans/vSecondVtrans, from mat_chain.uv_displacement, packed as the primary
	// transform is; w flags it on. Read only by the SECOND_OPACITY variant; off => the map
	// samples UV set 0 untransformed (SECOND_UVTRANS 0 in the original).
	if(state.hasSecondUVTrans){
		current_.vs.secondUTrans[0] = state.secondUvTrans[0]; current_.vs.secondUTrans[1] = state.secondUvTrans[2];
		current_.vs.secondUTrans[2] = state.secondUvTrans[4]; current_.vs.secondUTrans[3] = 1.f;
		current_.vs.secondVTrans[0] = state.secondUvTrans[1]; current_.vs.secondVTrans[1] = state.secondUvTrans[3];
		current_.vs.secondVTrans[2] = state.secondUvTrans[5]; current_.vs.secondVTrans[3] = 0.f;
	}
	else{
		std::memset(current_.vs.secondUTrans, 0, sizeof(current_.vs.secondUTrans));
		std::memset(current_.vs.secondVTrans, 0, sizeof(current_.vs.secondVTrans));
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
	current_.reflectTexture = sdlTextureOf(state.reflectTexture, state.texturePhase);
	current_.secondOpacityTexture = sdlTextureOf(state.secondOpacityTexture, state.texturePhase);
	current_.sampler = state.tilingWrap ? samplerWrap_ : samplerClamp_;

	// The second-opacity path, dispatched *before* reflection and bump in cObject3dx::Draw
	// (both its branches guard on !mat.pSecondOpacityTexture), so it wins the material here
	// too. Its fragment shader always samples the diffuse map, so an untextured material
	// can't take it.
	current_.secondOpacity = current_.secondOpacityTexture != nullptr && current_.texture != nullptr;

	// The bump fragment shader always samples the diffuse map (the original has no
	// NOTEXTURE variant of psSkinBump), so an untextured material can't take that path.
	current_.bump = current_.bumpTexture != nullptr && current_.texture != nullptr
	             && !current_.secondOpacity;

	// The reflection path, mutually exclusive with bump (cObject3dx::Draw dispatches it
	// before the bump path and never sets both) and superseded by second-opacity. Like bump,
	// the reflect fragment shader always samples the diffuse map, so an untextured material
	// can't take it.
	current_.reflect = current_.reflectTexture != nullptr && current_.texture != nullptr
	                && !current_.bump && !current_.secondOpacity;
	// Which of the two the env map is. The original reads it off the bound texture in the
	// same place -- `is_cube = material.Tex[1]->GetAttribute(TEXTURE_CUBEMAP)` -- rather
	// than off the material, because a sky reflection and a matcap reach it identically.
	current_.reflectCube = current_.reflect && state.reflectTexture
	                    && state.reflectTexture->getAttribute(TEXTURE_CUBEMAP) != 0;
	setVec4(current_.fs.reflectAmount, state.reflectAmount);

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
	// A caster never receives: under the light camera the map is a render target, not a
	// texture. (shadowPassRan() is false there too, but say it once, plainly.)
	const bool receives = !camera->getAttribute(ATTRCAMERA_SHADOWMAP) && !state.noLight
	                   && current_.texture && camera->IsShadow()
	                   && owner_->shadowPassRan() && owner_->GetShadowMap();
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

	// Distance fog. The plane folds in this camera's view matrix, so a model drawn into the
	// reflection fogs by its own depth -- and the sky camera, which turns RS_FOGENABLE off
	// around its scene, gets (0,0,0,1): factor 1, and the shader's lerp is the identity. That
	// is what keeps the clouds out of the fog, as the original's FOGENABLE guard did.
	const Vect4f fogPlane = owner_->fogPlane(camera);
	current_.vs.fogPlane[0] = fogPlane.x; current_.vs.fogPlane[1] = fogPlane.y;
	current_.vs.fogPlane[2] = fogPlane.z; current_.vs.fogPlane[3] = fogPlane.w;
	const Color4f fog = owner_->fogColor();
	current_.fs.fogColor[0] = fog.r; current_.fs.fogColor[1] = fog.g;
	current_.fs.fogColor[2] = fog.b; current_.fs.fogColor[3] = fog.a;

	current_.blend = state.blend;
	current_.skinned = boneCount > 1;
	current_.mirrored = camera->getAttribute(ATTRCAMERA_REFLECTION) != 0;

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
	// The caster pipelines always write depth, so this only matters to the colour pass.
	d.depthWrite = owner_->zWriteEnable();
	draws_.push_back(d);

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

	for(const DrawCmd& d : draws_){
		const StateBlock& st = states_[d.state];

		// The caster pass draws for the light camera, which is never mirrored.
		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, st.skinned, false, false, false, false, st.blend, false, true, false, true);
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
	draws_.clear();   // recorded; a second flush of this target is a no-op
	return true;
}

bool SDLObject3dxRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int targetW, int targetH, bool clear, const float clearColor[4],
                                bool clearDepth, bool wireframe)
{
	// May run with nothing recorded: an offscreen target still owes its clear to whatever
	// samples it. cSDLRenderDevice::flushTarget only calls in when there is work or a clear.
	if(!device_ || !cmd || !target || !depth)
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

		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, st.skinned, st.bump, st.reflect, st.reflectCube,
		                                                st.secondOpacity, st.blend, st.mirrored, d.depthWrite,
		                                                wireframe, false);
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
			applyCameraViewport(pass, vp, targetW, targetH);

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
			// The shadow map is last -- slot 1 plain, slot 2 reflect (diffuse + env map),
			// slot 3 bump (diffuse + bump + specular). The second-opacity variant is the
			// exception: it binds no shadow map, only diffuse + the opacity mask on slot 1.
			SDL_GPUTextureSamplerBinding ts[4] = {};
			ts[0].texture = st.texture ? st.texture : whiteTexture_;
			ts[0].sampler = st.sampler ? st.sampler : samplerWrap_;
			if(st.secondOpacity){
				// The moving opacity mask. Its UV is scrolled by the animated transform, so it
				// wraps (the original leaves stage 1 on its default wrap sampler).
				ts[1].texture = st.secondOpacityTexture ? st.secondOpacityTexture : whiteTexture_;
				ts[1].sampler = samplerWrap_;
				SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);
				boundState = d.state;
			}
			else{
				int shadowSlot = 1;
				if(st.bump){
					ts[1].texture = st.bumpTexture;
					ts[1].sampler = samplerWrap_;   // the original's sampler_wrap_linear on stage 1
					ts[2].texture = st.specularTexture ? st.specularTexture : whiteTexture_;
					ts[2].sampler = samplerWrap_;
					shadowSlot = 3;
				}
				else if(st.reflect){
					// The 2D environment map. A sphere-map UV runs to the [0,1] edges, so clamp.
					ts[1].texture = st.reflectTexture ? st.reflectTexture : whiteTexture_;
					ts[1].sampler = samplerClamp_;
					shadowSlot = 2;
				}
				ts[shadowSlot].texture = st.shadowTexture ? st.shadowTexture : whiteTexture_;
				ts[shadowSlot].sampler = samplerShadow_;
				SDL_BindGPUFragmentSamplers(pass, 0, ts, shadowSlot + 1);
				boundState = d.state;
			}
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

