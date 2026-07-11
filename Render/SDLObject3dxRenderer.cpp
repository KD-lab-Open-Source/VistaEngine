// SDL GPU 3dx object renderer. See header.
#include "StdAfxRD.h"
#include "SDLObject3dxRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / GetPos / GetLighting
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // owner: resolves sPtr buffers, holds RS_ZWRITEENABLE

// Cross-compiled object3dx shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/object3dx_shaders.h"

namespace {

// Matches MAX_BONES in object3dx.vert.hlsl and StaticBunch::max_index.
const int MAX_BONES = 20;

// The vertex cbuffer is always pushed whole, so the shader's World[60] is fully backed
// even though a bunch rarely uses all 20 bones: 16 floats of MVP + 8 float4s + 60 float4s.
const int VS_UNIFORM_FLOATS = 16 + 8 * 4 + MAX_BONES * 3 * 4;   // 288 -> 1152 bytes

// D3DRS_ALPHAREF the original sets for ALPHA_TEST (BLEND_STATE_ALPHA_REF in D3DRender.h).
const float ALPHA_TEST_REF = 80.f / 255.f;

// The engine's .3dx vertex offsets, from cSkinVertex (Render/inc/VertexFormat.h):
// pos float3 @0, blend indices D3DCOLOR @12, normal float3 @16, then -- only when the
// lod binds more than one bone per vertex -- weights D3DCOLOR @28, then uv float2.
// Everything after uv (bump S/T, uv2, fur) we don't read, but it does grow the stride.
const int OFS_POSITION = 0;
const int OFS_INDICES  = 12;
const int OFS_NORMAL   = 16;
const int OFS_WEIGHTS  = 28;

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

	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerWrap_ = SDL_CreateGPUSampler(device_, &si);

	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerClamp_ = SDL_CreateGPUSampler(device_, &si);

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
	if(whiteTexture_) SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(samplerWrap_)  SDL_ReleaseGPUSampler(device_, samplerWrap_);
	if(samplerClamp_) SDL_ReleaseGPUSampler(device_, samplerClamp_);
	if(vsRigid_) SDL_ReleaseGPUShader(device_, vsRigid_);
	if(vsSkin_)  SDL_ReleaseGPUShader(device_, vsSkin_);
	if(fs_)      SDL_ReleaseGPUShader(device_, fs_);
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------
bool SDLObject3dxRenderer::createShaders()
{
	if(shadersTried_) return vsRigid_ && vsSkin_ && fs_;
	shadersTried_ = true;
	if(!device_ || !window_) return false;

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *rigidCode, *skinCode, *fsCode;
	unsigned int rigidSize, skinSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		rigidCode = object3dx_rigid_vert_msl; rigidSize = object3dx_rigid_vert_msl_len;
		skinCode  = object3dx_skin_vert_msl;  skinSize  = object3dx_skin_vert_msl_len;
		fsCode    = object3dx_frag_msl;       fsSize    = object3dx_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		rigidCode = object3dx_rigid_vert_spv; rigidSize = object3dx_rigid_vert_spv_len;
		skinCode  = object3dx_skin_vert_spv;  skinSize  = object3dx_skin_vert_spv_len;
		fsCode    = object3dx_frag_spv;       fsSize    = object3dx_frag_spv_len;
	} else {
		fprintf(stderr, "SDLObject3dxRenderer: no supported shader format (0x%x)\n", formats);
		return false;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.entrypoint = entry; vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + material + bone matrices
	vsi.code = rigidCode; vsi.code_size = rigidSize;
	vsRigid_ = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = skinCode;  vsi.code_size = skinSize;
	vsSkin_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;           // diffuse
	fsi.num_uniform_buffers = 1;    // ambient + skin-colour lerp + flags
	fs_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vsRigid_ || !vsSkin_ || !fs_){
		fprintf(stderr, "SDLObject3dxRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stderr, "SDLObject3dxRenderer: object3dx shaders ready\n");
	return true;
}

SDL_GPUGraphicsPipeline* SDLObject3dxRenderer::pipelineFor(int stride, bool skinned, eBlendMode blend,
                                                           bool depthWrite, bool wireframe)
{
	const unsigned long long key = (unsigned long long)(unsigned)stride
	                             | ((unsigned long long)skinned    << 16)
	                             | ((unsigned long long)blend      << 17)
	                             | ((unsigned long long)depthWrite << 24)
	                             | ((unsigned long long)wireframe  << 25);
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
	// down when the weights are present. Blend indices arrive as raw bytes (the shader
	// takes them in memory order, as D3DCOLORtoUBYTE4 did); weights are normalized.
	SDL_GPUVertexAttribute attrs[5] = {};
	int n = 0;
	attrs[n].location = 0; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = OFS_POSITION; n++;
	attrs[n].location = 1; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4; attrs[n].offset = OFS_INDICES; n++;
	attrs[n].location = 2; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[n].offset = OFS_NORMAL; n++;
	if(skinned){
		attrs[n].location = 3; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[n].offset = OFS_WEIGHTS; n++;
		attrs[n].location = 4; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; attrs[n].offset = OFS_WEIGHTS + 4; n++;
	}
	else{
		attrs[n].location = 3; attrs[n].buffer_slot = 0; attrs[n].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; attrs[n].offset = OFS_WEIGHTS; n++;
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
	pci.vertex_shader = skinned ? vsSkin_ : vsRigid_;
	pci.fragment_shader = fs_;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = (Uint32)n;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// The scene passes flip culling per node type (DrawObjectSpecial and DrawSortObject
	// both force D3DCULL_NONE), and the SDL device does not track D3DRS_CULLMODE yet.
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = depthWrite;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	if(!pipeline)
		fprintf(stderr, "SDLObject3dxRenderer: pipeline (stride %d, skinned %d, blend %d) failed: %s\n",
		        stride, (int)skinned, (int)blend, SDL_GetError());
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
	const int bones = state.world ? (state.worldNum < MAX_BONES ? state.worldNum : MAX_BONES) : 0;
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

	// PSSkin::SetMaterial's premultiplied skin-colour lerp: rgb = c.rgb*c.a, a = 1-c.a.
	const bool lerp = state.lerpColor.a > 0.001f;
	current_.fs.lerpPre[0] = state.lerpColor.r * state.lerpColor.a;
	current_.fs.lerpPre[1] = state.lerpColor.g * state.lerpColor.a;
	current_.fs.lerpPre[2] = state.lerpColor.b * state.lerpColor.a;
	current_.fs.lerpPre[3] = 1.f - state.lerpColor.a;

	current_.texture = sdlTextureOf(state.texture, state.texturePhase);
	current_.sampler = state.tilingWrap ? samplerWrap_ : samplerClamp_;
	current_.fs.params[0] = state.blend == ALPHA_TEST ? ALPHA_TEST_REF : 0.f;
	current_.fs.params[1] = current_.texture ? 1.f : 0.f;
	current_.fs.params[2] = state.selfIllumination ? 1.f : 0.f;
	current_.fs.params[3] = lerp ? 1.f : 0.f;

	current_.blend = state.blend;
	current_.skinned = boneCount > 1;

	currentValid_ = true;
	currentDirty_ = true;
}

void SDLObject3dxRenderer::SetAlphaColor(const Color4f& color)
{
	if(!currentValid_) return;
	setVec4(current_.vs.diffuse, color);
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
	d.depthWrite = owner_->zWriteEnable();
	draws_.push_back(d);

	*gb_RenderDevice->PtrNumberPolygon += nPolygon;
	gb_RenderDevice->NumDrawObject++;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
bool SDLObject3dxRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int /*screenW*/, int /*screenH*/, bool clear, const float clearColor[4],
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

		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(d.stride, st.skinned, st.blend, d.depthWrite, wireframe);
		if(!pipeline) continue;
		if(pipeline != boundPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			boundPipeline = pipeline;
			boundState = -1;   // uniforms and bindings do not survive a pipeline change
			boundVB = boundIB = nullptr;
		}

		if(d.state != boundState){
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
	return true;
}

#endif // !_WIN32
