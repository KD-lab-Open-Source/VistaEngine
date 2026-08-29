// SDL GPU world-line renderer. See header.
#include "StdAfxRD.h"
#include "SDLWorldLineRenderer.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "cCamera.h"           // Camera::matViewProj / vp
#include "SDLRenderDevice.h"   // applyCameraViewport

// World-line shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/worldline_shaders.h"
#include "SDLShaders/ShaderBlob.h"

namespace {

// The vertex attributes below are laid out by hand at 0/12 with sizeof() as the stride,
// which only agrees with sVertexXYZD's D3D declaration while Color4c stays four bytes
// wide.
static_assert(sizeof(WorldLineVertex) == 16, "WorldLineVertex must match sVertexXYZD");

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

} // namespace

SDLWorldLineRenderer::SDLWorldLineRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
}

SDLWorldLineRenderer::~SDLWorldLineRenderer()
{
	if(!device_) return;
	if(vertexBuffer_)  SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
	if(vs_)            SDL_ReleaseGPUShader(device_, vs_);
	if(fs_)            SDL_ReleaseGPUShader(device_, fs_);
	if(pipeline_)      SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
bool SDLWorldLineRenderer::ensurePipeline()
{
	if(shadersTried_) return pipelineReady_;
	shadersTried_ = true;
	// window_ can be null (the Qt editor creates no SDL window of its own; the
	// swapchain comes from the foreign window later). SDL_GetGPUSwapchainTextureFormat
	// tolerates a null window -- it returns the device's default swapchain format --
	// exactly as SDLTileMapRenderer's ctor-pipeline relies on. So only the device
	// gates here.
	if(!device_) return false;

	// The one vertex shader: position * MVP, colour passed through. One uniform
	// block (the camera's view-projection).
	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(worldline_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	vs_ = SDL_CreateGPUShader(device_, &vsi);

	// The fragment shader: the interpolated colour, unchanged. No samplers, no
	// uniform blocks.
	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(worldline_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_uniform_buffers = 0;
	fs_ = SDL_CreateGPUShader(device_, &fsi);

	if(!vs_ || !fs_){
		fprintf(stderr, "SDLWorldLineRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		return false;
	}

	// sVertexXYZD: position at 0 (FLOAT3), diffuse at 12 (UBYTE4_NORM, engine BGRA).
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(WorldLineVertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[2] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;       attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;  attrs[1].offset = 12;

	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	// ALPHA_BLEND (SRC_ALPHA, 1-SRC_ALPHA) — the blend cD3DRender's line path used.
	colorTarget.blend_state.enable_blend = true;
	colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs_;
	pci.fragment_shader = fs_;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 2;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	// Depth-tested, not depth-writing: lines draw over the scene (D3D's
	// FlushLine3D cleared RS_ZWRITEENABLE under ALPHA_BLEND).
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = false;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	SDL_ReleaseGPUShader(device_, vs_);
	SDL_ReleaseGPUShader(device_, fs_);
	vs_ = nullptr;
	fs_ = nullptr;

	if(!pipeline_){
		fprintf(stderr, "SDLWorldLineRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		return false;
	}
	fprintf(stderr, "SDLWorldLineRenderer: world-line pipeline ready\n");
	pipelineReady_ = true;
	return true;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLWorldLineRenderer::BeginFrame()
{
	vertices_.clear();
}

void SDLWorldLineRenderer::SetCamera(Camera* camera)
{
	if(!camera) return;
	std::memcpy(&mvp_, &camera->matViewProj, sizeof(mvp_));
	vpX_ = camera->vp.X;       vpY_ = camera->vp.Y;
	vpW_ = camera->vp.Width;   vpH_ = camera->vp.Height;
	vpMinZ_ = camera->vp.MinZ; vpMaxZ_ = camera->vp.MaxZ;
}

void SDLWorldLineRenderer::DrawLine(const float* v1, const float* v2, unsigned int color)
{
	WorldLineVertex a;
	a.pos[0] = v1[0]; a.pos[1] = v1[1]; a.pos[2] = v1[2];
	a.color = color;
	vertices_.push_back(a);

	WorldLineVertex b;
	b.pos[0] = v2[0]; b.pos[1] = v2[1]; b.pos[2] = v2[2];
	b.color = color;
	vertices_.push_back(b);
}

void SDLWorldLineRenderer::ensureVertexCapacity(int verts)
{
	if(verts <= vertexCapacity_) return;
	int cap = vertexCapacity_ ? vertexCapacity_ : 1024;
	while(cap < verts) cap *= 2;

	if(vertexBuffer_)    SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_)  SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUBufferCreateInfo bi = {};
	bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bi.size = (Uint32)(cap * sizeof(WorldLineVertex));
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bi);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)(cap * sizeof(WorldLineVertex));
	transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &tbi);

	vertexCapacity_ = (vertexBuffer_ && transferBuffer_) ? cap : 0;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
bool SDLWorldLineRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                                int screenW, int screenH,
                                bool clear, const float clearColor[4], bool clearDepth)
{
	if(!device_ || !cmd || !target || !depth || vertices_.empty())
		return false;
	if(!ensurePipeline())
		return false;

	const int lines = (int)(vertices_.size() / 2);
	ensureVertexCapacity((int)vertices_.size());
	if(!uploadBuffer(device_, cmd, vertexBuffer_, vertices_.data(),
	                 (Uint32)(vertices_.size() * sizeof(WorldLineVertex))))
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
	if(!pass){
		fprintf(stderr, "SDLWorldLineRenderer: BeginGPURenderPass failed: %s\n", SDL_GetError());
		vertices_.clear();
		return false;
	}

	sViewPort vp;
	vp.X = vpX_; vp.Y = vpY_; vp.Width = vpW_; vp.Height = vpH_;
	vp.MinZ = vpMinZ_; vp.MaxZ = vpMaxZ_;
	applyCameraViewport(pass, vp, screenW, screenH);

	SDL_BindGPUGraphicsPipeline(pass, pipeline_);

	// The one uniform: the camera's view-projection.
	SDL_PushGPUVertexUniformData(cmd, 0, &mvp_, sizeof(mvp_));

	SDL_GPUBufferBinding vb = {};
	vb.buffer = vertexBuffer_;
	vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

	SDL_DrawGPUPrimitives(pass, (Uint32)(lines * 2), 1, 0, 0);
	SDL_EndGPURenderPass(pass);

	vertices_.clear();
	return true;
}
