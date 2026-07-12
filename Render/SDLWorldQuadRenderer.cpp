// SDL GPU world-quad renderer. See header.
#include "StdAfxRD.h"
#include "SDLWorldQuadRenderer.h"


#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLRenderDevice.h"   // applyCameraViewport

// Cross-compiled world-quad shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/worldquad_shaders.h"
#include "SDLShaders/worldtri_shaders.h"

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
	createSampler();
}

SDLWorldQuadRenderer::~SDLWorldQuadRenderer()
{
	if(!device_) return;
	if(vertexBuffer_)    SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(indexBuffer_)     SDL_ReleaseGPUBuffer(device_, indexBuffer_);
	if(vertexBufferTri_) SDL_ReleaseGPUBuffer(device_, vertexBufferTri_);
	if(indexBufferTri_)  SDL_ReleaseGPUBuffer(device_, indexBufferTri_);
	if(whiteTexture_)  SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(sampler_)       SDL_ReleaseGPUSampler(device_, sampler_);
	if(vsShader_)      SDL_ReleaseGPUShader(device_, vsShader_);
	if(fsShader_)      SDL_ReleaseGPUShader(device_, fsShader_);
	if(vsShaderTri_)   SDL_ReleaseGPUShader(device_, vsShaderTri_);
	if(fsShaderTri_)   SDL_ReleaseGPUShader(device_, fsShaderTri_);
	for(auto& kv : pipelines_)
		if(kv.second) SDL_ReleaseGPUGraphicsPipeline(device_, kv.second);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLWorldQuadRenderer::createSampler()
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
}

bool SDLWorldQuadRenderer::createShaders()
{
	if(shadersTried_)
		return vsShader_ && fsShader_ && vsShaderTri_ && fsShaderTri_;
	shadersTried_ = true;
	if(!device_)
		return false;

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode, *vsTriCode, *fsTriCode;
	unsigned int vsSize, fsSize, vsTriSize, fsTriSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = worldquad_vert_msl; vsSize = worldquad_vert_msl_len;
		fsCode = worldquad_frag_msl; fsSize = worldquad_frag_msl_len;
		vsTriCode = worldtri_vert_msl; vsTriSize = worldtri_vert_msl_len;
		fsTriCode = worldtri_frag_msl; fsTriSize = worldtri_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = worldquad_vert_spv; vsSize = worldquad_vert_spv_len;
		fsCode = worldquad_frag_spv; fsSize = worldquad_frag_spv_len;
		vsTriCode = worldtri_vert_spv; vsTriSize = worldtri_vert_spv_len;
		fsTriCode = worldtri_frag_spv; fsTriSize = worldtri_frag_spv_len;
	} else {
		fprintf(stderr, "SDLWorldQuadRenderer: no supported shader format (0x%x)\n", formats);
		return false;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // mWVP
	vsShader_ = SDL_CreateGPUShader(device_, &vsi);
	vsi.code = vsTriCode; vsi.code_size = vsTriSize;
	vsShaderTri_ = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;           // the group's texture
	fsi.num_uniform_buffers = 1;    // SelectDiffuse
	fsShader_ = SDL_CreateGPUShader(device_, &fsi);

	fsi.code = fsTriCode; fsi.code_size = fsTriSize;
	fsi.num_samplers = 2;           // + the colour operation's second texture
	fsi.num_uniform_buffers = 1;    // COLOR_OPERATION
	fsShaderTri_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vsShader_ || !fsShader_ || !vsShaderTri_ || !fsShaderTri_)
		fprintf(stderr, "SDLWorldQuadRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
	return vsShader_ && fsShader_ && vsShaderTri_ && fsShaderTri_;
}

SDL_GPUGraphicsPipeline* SDLWorldQuadRenderer::pipelineFor(eBlendMode blend, bool depthTest,
                                                           bool wireframe, GroupKind kind)
{
	const unsigned key = (unsigned)blend | ((unsigned)depthTest << 8) | ((unsigned)wireframe << 9)
	                   | ((unsigned)kind << 10);
	auto it = pipelines_.find(key);
	if(it != pipelines_.end())
		return it->second;

	if(!window_ || !createShaders()){
		pipelines_[key] = nullptr;
		return nullptr;
	}

	const bool tri = kind == GROUP_TRI;

	// sVertexXYZDT1: float3 position @0, D3DCOLOR diffuse @12, float2 uv @16 (stride 24).
	// sVertexXYZDT2 adds a second float2 uv @24 (stride 32) -- it derives from the first.
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = tri ? sizeof(sVertexXYZDT2) : sizeof(sVertexXYZDT1);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[4] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 12;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 16;
	attrs[3].location = 3; attrs[3].buffer_slot = 0; attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[3].offset = 24;

	// The blend the caller's SetWorldMaterial / SetNoMaterial asked for. Every one of them
	// is written with a premultiplied source, because the textures decode premultiplied and
	// the shaders keep them that way (see worldquad.frag.hlsl), so each D3D src factor of
	// SRC_ALPHA becomes ONE:
	//
	//   ALPHA_BLEND         dst*(1-a) + src*a  ->  (ONE, 1-SRC_ALPHA)
	//   ALPHA_ADDBLENDALPHA dst + src*a        ->  (ONE, ONE)
	//   ALPHA_ADDBLEND      dst + src          ->  (ONE, ONE)  [see below]
	//   ALPHA_SUBBLEND      dst - src          ->  (ONE, ONE), reverse-subtract
	//   ALPHA_MUL           dst * src          ->  (DST_COLOR, ZERO)
	//   ALPHA_NONE          src                ->  no blend
	//
	// ALPHA_ADDBLEND is the one place premultiplying is not exact: D3D adds the raw texel,
	// this adds texel*alpha. The two agree wherever a transparent texel is black, which is
	// how additive particle art is drawn -- and the alternative, dividing the alpha back
	// out in the shader, would amplify the very edge texels premultiplying exists to fix.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	SDL_GPUColorTargetBlendState& bs = colorTarget.blend_state;
	bs.enable_blend = blend != ALPHA_NONE && blend != ALPHA_TEST;
	bs.color_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	switch(blend){
		case ALPHA_ADDBLENDALPHA:
		case ALPHA_ADDBLEND:
			bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			break;
		case ALPHA_SUBBLEND:
			bs.color_blend_op = SDL_GPU_BLENDOP_REVERSE_SUBTRACT;   // dst - src
			bs.alpha_blend_op = SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
			bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			break;
		case ALPHA_MUL:
			bs.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
			bs.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA;
			bs.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
			bs.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
			break;
		default:
			break;   // ALPHA_BLEND and its two variants keep the factors set above
	}

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = tri ? vsShaderTri_ : vsShader_;
	pci.fragment_shader = tri ? fsShaderTri_ : fsShader_;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = tri ? 4 : 3;
	// Both routes draw indexed triangles: the quads through a fixed two-triangle pattern,
	// the strips and lists unrolled into indices by DrawPrimitive.
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
	// Camera::DrawObjectSpecial and DrawSortObject both force D3DCULL_NONE for their whole
	// node, and cSunMoonObj::Draw sets it itself.
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	// RS_ZWRITEENABLE is off for every caller: cCoastSprites::Draw forces it around both
	// sprite groups, Camera::DrawSortObject around the sorted pass the wave sources draw
	// in, and the sky camera carries ATTRCAMERA_NOZWRITE. The sun and moon go further and
	// turn the depth *test* off too (D3DRS_ZENABLE FALSE), so they sit behind everything.
	pci.depth_stencil_state.enable_depth_test = depthTest;
	pci.depth_stencil_state.enable_depth_write = false;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	if(!pipeline)
		fprintf(stderr, "SDLWorldQuadRenderer: pipeline (blend %d, depthTest %d) failed: %s\n",
		        (int)blend, (int)depthTest, SDL_GetError());
	pipelines_[key] = pipeline;
	return pipeline;
}

// ---------------------------------------------------------------------------
// Recording -- cQuadBuffer<sVertexXYZDT1>'s contract
// ---------------------------------------------------------------------------
void SDLWorldQuadRenderer::BeginFrame()
{
	vertices_.clear();
	verticesTri_.clear();
	indicesTri_.clear();
	lockFirst_ = lockCount_ = 0;
	groups_.clear();
	drawing_ = false;
	cameraValid_ = false;
}

void SDLWorldQuadRenderer::SetCamera(Camera* camera)
{
	if(!camera) return;
	viewProj_ = camera->matViewProj;
	vpX_ = camera->vp.X; vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width; vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;
	cameraValid_ = true;
}

void SDLWorldQuadRenderer::SetMaterial(eBlendMode blend, cTexture* texture, bool depthTest,
                                       const MatXf& world, cTexture* texture1, eColorMode colorMode,
                                       bool selectDiffuse)
{
	material_.texture = sdlTextureOf(texture);
	material_.texture1 = sdlTextureOf(texture1);
	material_.blend = blend;
	material_.depthTest = depthTest;
	materialWorld_ = world;

	// cD3DRender::SetWorldMaterial's color_operation: 0 without a second texture, else the
	// eColorMode mapped as it maps it. psStandart reads it as COLOR_OPERATION. The quad
	// shader reads the same slot as SelectDiffuse, so the two never collide -- a group is
	// one route or the other.
	float op = selectDiffuse ? 1.f : 0.f;
	if(!selectDiffuse && material_.texture1)
		switch(colorMode){
			case COLOR_ADD:  op = 1.f; break;
			case COLOR_MOD:  op = 2.f; break;
			case COLOR_MOD2: op = 3.f; break;
			case COLOR_MOD4: op = 4.f; break;
			default:         op = 2.f; break;   // the original xasserts; MOD is its default
		}
	material_.fs.colorOp[0] = op;
	material_.fs.colorOp[1] = material_.fs.colorOp[2] = material_.fs.colorOp[3] = 0.f;
}

// The mvp the group being opened draws with. SetWorldMaterial's mWVP: the camera cannot be
// folded in at SetMaterial, because several emitters set their material under one camera,
// each with its own world matrix.
void SDLWorldQuadRenderer::openGroup(GroupKind kind)
{
	current_ = material_;
	current_.kind = kind;
	current_.first = kind == GROUP_QUAD ? (int)(vertices_.size() / 4) : (int)indicesTri_.size();
	current_.count = 0;
	const Mat4f mvp = Mat4f(materialWorld_) * viewProj_;
	std::memcpy(current_.vs.mvp, &mvp, sizeof(current_.vs.mvp));
	drawing_ = cameraValid_;
}

void SDLWorldQuadRenderer::BeginDraw(const MatXf&)
{
	openGroup(GROUP_QUAD);
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
	current_.count++;
	return &vertices_[vertices_.size() - 4];
}

void SDLWorldQuadRenderer::EndDraw()
{
	if(drawing_ && current_.count > 0)
		groups_.push_back(current_);
	drawing_ = false;
}

// ---------------------------------------------------------------------------
// The triangle route: cVertexBuffer<sVertexXYZDT2>'s Lock / Unlock / DrawPrimitive.
// ---------------------------------------------------------------------------
sVertexXYZDT2* SDLWorldQuadRenderer::Lock(int nVertex)
{
	if(nVertex <= 0)
		return nullptr;
	openGroup(GROUP_TRI);
	if(!drawing_){
		// No camera: hand back scratch the caller can fill and we then drop, as Get() does.
		scratchTri_.resize((size_t)nVertex);
		return scratchTri_.data();
	}
	lockFirst_ = (int)verticesTri_.size();
	lockCount_ = nVertex;
	verticesTri_.resize(verticesTri_.size() + (size_t)nVertex);
	return &verticesTri_[(size_t)lockFirst_];
}

void SDLWorldQuadRenderer::Unlock(int /*nVertex*/)
{
	// The vertices were written straight into verticesTri_; nothing to copy back. The count
	// Lock recorded is what DrawPrimitive indexes.
}

void SDLWorldQuadRenderer::DrawPrimitive(PRIMITIVETYPE type, int nPolygon)
{
	if(!drawing_ || nPolygon <= 0 || lockCount_ <= 0){
		lockFirst_ = lockCount_ = 0;
		drawing_ = false;
		return;
	}

	// Unroll into indexed triangles. Strips would need SDL's own strip primitive and a
	// pipeline of their own, and the callers draw so few triangles that the indices are
	// cheaper than the second pipeline. Winding does not matter: the pipeline culls nothing.
	const unsigned base = (unsigned)lockFirst_;
	if(type == PT_TRIANGLESTRIP){
		for(int i = 0; i < nPolygon; i++){
			if(i + 2 >= lockCount_)
				break;
			indicesTri_.push_back(base + (unsigned)i);
			indicesTri_.push_back(base + (unsigned)i + 1);
			indicesTri_.push_back(base + (unsigned)i + 2);
			current_.count++;
		}
	}
	else if(type == PT_TRIANGLELIST){
		for(int i = 0; i < nPolygon; i++){
			if(3 * i + 2 >= lockCount_)
				break;
			indicesTri_.push_back(base + (unsigned)(3 * i));
			indicesTri_.push_back(base + (unsigned)(3 * i) + 1);
			indicesTri_.push_back(base + (unsigned)(3 * i) + 2);
			current_.count++;
		}
	}
	// Any other primitive type: no caller uses one, so the run is simply dropped.

	if(current_.count > 0)
		groups_.push_back(current_);
	lockFirst_ = lockCount_ = 0;
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

bool SDLWorldQuadRenderer::ensureCapacityTri(SDL_GPUCommandBuffer* /*cmd*/, int vertices, int indices)
{
	if(vertices <= capacityVertsTri_ && indices <= capacityIndicesTri_)
		return true;

	int vcap = capacityVertsTri_ ? capacityVertsTri_ : 256;
	while(vcap < vertices) vcap *= 2;
	int icap = capacityIndicesTri_ ? capacityIndicesTri_ : 512;
	while(icap < indices) icap *= 2;

	if(vertexBufferTri_) SDL_ReleaseGPUBuffer(device_, vertexBufferTri_);
	if(indexBufferTri_)  SDL_ReleaseGPUBuffer(device_, indexBufferTri_);
	vertexBufferTri_ = indexBufferTri_ = nullptr;
	capacityVertsTri_ = capacityIndicesTri_ = 0;

	SDL_GPUBufferCreateInfo vbi = {};
	vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	vbi.size = (Uint32)(vcap * sizeof(sVertexXYZDT2));
	vertexBufferTri_ = SDL_CreateGPUBuffer(device_, &vbi);

	SDL_GPUBufferCreateInfo ibi = {};
	ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
	ibi.size = (Uint32)(icap * sizeof(unsigned));
	indexBufferTri_ = SDL_CreateGPUBuffer(device_, &ibi);
	if(!vertexBufferTri_ || !indexBufferTri_)
		return false;

	capacityVertsTri_ = vcap;
	capacityIndicesTri_ = icap;
	return true;
}

// Copy `bytes` from src into buffer, before the render pass opens (SDL requires it).
static bool uploadBuffer(SDL_GPUDevice* device, SDL_GPUCommandBuffer* cmd, SDL_GPUBuffer* buffer,
                         const void* src, Uint32 bytes)
{
	if(!bytes)
		return true;
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = bytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
	if(!tb) return false;
	void* map = SDL_MapGPUTransferBuffer(device, tb, false);
	SDL_memcpy(map, src, bytes);
	SDL_UnmapGPUTransferBuffer(device, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation loc = {}; loc.transfer_buffer = tb; loc.offset = 0;
	SDL_GPUBufferRegion dst = {}; dst.buffer = buffer; dst.offset = 0; dst.size = bytes;
	SDL_UploadToGPUBuffer(copy, &loc, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device, tb);   // destruction deferred until the copy runs
	return true;
}

bool SDLWorldQuadRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int screenW, int screenH, bool clear, const float clearColor[4],
                                bool clearDepth, bool wireframe)
{
	if(!device_ || !cmd || !target || !depth || groups_.empty())
		return false;

	const int quads = (int)(vertices_.size() / 4);
	if(!ensureCapacity(cmd, quads))
		return false;
	if(!ensureCapacityTri(cmd, (int)verticesTri_.size(), (int)indicesTri_.size()))
		return false;

	// Upload this frame's geometry. Both streams, before the render pass opens.
	if(!uploadBuffer(device_, cmd, vertexBuffer_, vertices_.data(),
	                 (Uint32)(vertices_.size() * sizeof(sVertexXYZDT1))))
		return false;
	if(!uploadBuffer(device_, cmd, vertexBufferTri_, verticesTri_.data(),
	                 (Uint32)(verticesTri_.size() * sizeof(sVertexXYZDT2))))
		return false;
	if(!uploadBuffer(device_, cmd, indexBufferTri_, indicesTri_.data(),
	                 (Uint32)(indicesTri_.size() * sizeof(unsigned))))
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

	// One group per BeginDraw..EndDraw run or per DrawPrimitive, replayed in the order the
	// callers made them -- quads and triangles interleaved, which is what keeps a light
	// column and the sprites of the same cEffect blending in the order D3D drew them.
	SDL_GPUGraphicsPipeline* boundPipeline = nullptr;
	SDL_GPUTexture* boundTexture = nullptr;
	SDL_GPUTexture* boundTexture1 = nullptr;
	const VSUniform* boundVS = nullptr;
	const FSUniform* boundFS = nullptr;
	for(const Group& g : groups_){
		SDL_GPUGraphicsPipeline* pipeline = pipelineFor(g.blend, g.depthTest, wireframe, g.kind);
		if(!pipeline) continue;
		if(pipeline != boundPipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);

			const bool tri = g.kind == GROUP_TRI;
			SDL_GPUBufferBinding vb = {};
			vb.buffer = tri ? vertexBufferTri_ : vertexBuffer_;
			vb.offset = 0;
			SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
			SDL_GPUBufferBinding ib = {};
			ib.buffer = tri ? indexBufferTri_ : indexBuffer_;
			ib.offset = 0;
			SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);

			boundPipeline = pipeline;
			// Bindings do not survive a pipeline change.
			boundTexture = boundTexture1 = nullptr;
			boundVS = nullptr;
			boundFS = nullptr;
		}

		// Per group: a relative particle emitter folds its GlobalMatrix into the mvp.
		if(!boundVS || std::memcmp(boundVS->mvp, g.vs.mvp, sizeof(g.vs.mvp)) != 0){
			SDL_PushGPUVertexUniformData(cmd, 0, &g.vs, sizeof(g.vs));
			boundVS = &g.vs;
		}

		SDL_GPUTexture* texture = g.texture ? g.texture : whiteTexture_;
		if(g.kind == GROUP_TRI){
			// The triangle shader always samples two textures, so bind both even when there
			// is no second texture -- white, with the operation off, which the shader skips.
			SDL_GPUTexture* texture1 = g.texture1 ? g.texture1 : whiteTexture_;
			if(texture != boundTexture || texture1 != boundTexture1){
				SDL_GPUTextureSamplerBinding ts[2] = {};
				ts[0].texture = texture;  ts[0].sampler = sampler_;
				ts[1].texture = texture1; ts[1].sampler = sampler_;
				SDL_BindGPUFragmentSamplers(pass, 0, ts, 2);
				boundTexture = texture;
				boundTexture1 = texture1;
			}
		}
		else if(texture != boundTexture){
			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = texture;
			ts.sampler = sampler_;
			SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
			boundTexture = texture;
		}

		// Both shaders read a fragment uniform from the same slot: COLOR_OPERATION on the
		// triangle route, SelectDiffuse on the quad one.
		if(!boundFS || std::memcmp(boundFS, &g.fs, sizeof(g.fs)) != 0){
			SDL_PushGPUFragmentUniformData(cmd, 0, &g.fs, sizeof(g.fs));
			boundFS = &g.fs;
		}

		if(g.kind == GROUP_TRI)
			SDL_DrawGPUIndexedPrimitives(pass, g.count * 3, 1, g.first, 0, 0);
		else
			SDL_DrawGPUIndexedPrimitives(pass, g.count * 6, 1, g.first * 6, 0, 0);
	}

	SDL_EndGPURenderPass(pass);

	vertices_.clear();
	verticesTri_.clear();
	indicesTri_.clear();
	groups_.clear();
	return true;
}

