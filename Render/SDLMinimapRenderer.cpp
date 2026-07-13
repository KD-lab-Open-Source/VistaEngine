// SDL GPU minimap renderer. See header.
#include "StdAfxRD.h"
#include "SDLMinimapRenderer.h"


#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "Texture.h"           // cTexture (GetDDSurface / frameNumber)
#include "SDLUIRenderer.h"     // EmitMinimapRun: our place in the UI's draw order
#include "SDLRenderDevice.h"   // sdlRenderDevice(), the owner

// Minimap shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/minimap_shaders.h"
#include "SDLShaders/ShaderBlob.h"

namespace {

// cSDLRenderDevice::CreateTexture parks the SDL_GPUTexture* in BitMap[frame]; pick the
// animation frame the way cD3DRender::SetTexturePhase does.
SDL_GPUTexture* sdlTextureOf(cTexture* t, float phase = 0.f)
{
	if(!t) return nullptr;
	const int frames = t->frameNumber();
	if(frames < 1) return nullptr;
	const int frame = frames > 1 ? (int)(0.999f * phase * frames) : 0;
	return reinterpret_cast<SDL_GPUTexture*>(t->GetDDSurface(frame));
}

} // namespace

SDLMinimapRenderer::SDLMinimapRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipelines();
}

SDLMinimapRenderer::~SDLMinimapRenderer()
{
	if(!device_) return;
	if(vertexBuffer_)     SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_)   SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
	if(whiteTexture_)     SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(sampler_)          SDL_ReleaseGPUSampler(device_, sampler_);
	if(mapPipeline_)      SDL_ReleaseGPUGraphicsPipeline(device_, mapPipeline_);
	if(primPipeline_)     SDL_ReleaseGPUGraphicsPipeline(device_, primPipeline_);
	if(primLinePipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, primLinePipeline_);
}

// ---------------------------------------------------------------------------
// Pipelines
// ---------------------------------------------------------------------------
void SDLMinimapRenderer::createPipelines()
{
	if(!device_ || !window_) return;

	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_ = SDL_CreateGPUSampler(device_, &si);

	whiteTexture_ = createSolidGPUTexture(device_, 0xFFFFFFFFu);

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(minimap_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // InvScreenSize
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo mfi = vista::shaderCreateInfo(VISTA_SHADER(minimap_frag));
	mfi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	mfi.num_samplers = 4;           // base, water, addition, border
	mfi.num_uniform_buffers = 1;
	SDL_GPUShader* mapFs = SDL_CreateGPUShader(device_, &mfi);

	SDL_GPUShaderCreateInfo bfi = vista::shaderCreateInfo(VISTA_SHADER(minimap_border_frag));
	bfi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	bfi.num_samplers = 2;           // sprite atlas, border
	bfi.num_uniform_buffers = 1;
	SDL_GPUShader* borderFs = SDL_CreateGPUShader(device_, &bfi);

	if(!vs || !mapFs || !borderFs){
		fprintf(stderr, "SDLMinimapRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs)       SDL_ReleaseGPUShader(device_, vs);
		if(mapFs)    SDL_ReleaseGPUShader(device_, mapFs);
		if(borderFs) SDL_ReleaseGPUShader(device_, borderFs);
		return;
	}

	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(Vertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[4] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 8;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 12;
	attrs[3].location = 3; attrs[3].buffer_slot = 0; attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[3].offset = 20;

	// ALPHA_BLEND, which UI_Minimap::redraw asks for through SetNoMaterial and the map
	// through SetBlendStateAlphaRef. Both minimap shaders end on an alpha the border mask
	// and the fog of war drive, so the blend is what actually applies them.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
	colorTarget.blend_state.enable_blend = true;
	colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = mapFs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 4;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;   // the map quad rotates
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;

	mapPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	pci.fragment_shader = borderFs;
	primPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
	primLinePipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, mapFs);
	SDL_ReleaseGPUShader(device_, borderFs);

	fprintf(stderr, "SDLMinimapRenderer: map pipeline %s, symbol pipeline %s (lines %s)\n",
	        mapPipeline_ ? "ready" : "FAILED", primPipeline_ ? "ready" : "FAILED",
	        primLinePipeline_ ? "ready" : "FAILED");
}

void SDLMinimapRenderer::ensureVertexCapacity(int verts)
{
	if(verts <= vertexCapacity_) return;
	int cap = vertexCapacity_ ? vertexCapacity_ : 256;
	while(cap < verts) cap *= 2;

	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUBufferCreateInfo bi = {};
	bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bi.size = (Uint32)(cap * sizeof(Vertex));
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bi);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)(cap * sizeof(Vertex));
	transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &tbi);

	vertexCapacity_ = (vertexBuffer_ && transferBuffer_) ? cap : 0;
}

// ---------------------------------------------------------------------------
// Recording
// ---------------------------------------------------------------------------
void SDLMinimapRenderer::BeginFrame()
{
	batch_.clear();
	draws_.clear();
}

void SDLMinimapRenderer::DrawMap(const MapState& st, const Vertex corners[4])
{
	if(!mapPipeline_)
		return;

	Draw d = {};
	d.map = true;
	d.lines = false;
	d.first = (int)batch_.size();
	d.count = 6;

	// The original draws the four corners as a tri-strip ((-,-), (-,+), (+,-), (+,+)); one
	// triangle list of the same two triangles keeps this renderer to a single primitive type.
	batch_.push_back(corners[0]); batch_.push_back(corners[1]); batch_.push_back(corners[2]);
	batch_.push_back(corners[2]); batch_.push_back(corners[1]); batch_.push_back(corners[3]);

	SDL_GPUTexture* base     = sdlTextureOf(st.base);
	SDL_GPUTexture* water    = sdlTextureOf(st.water);
	SDL_GPUTexture* addition = sdlTextureOf(st.addition);
	SDL_GPUTexture* border   = sdlTextureOf(st.mask);

	d.tex[0] = base     ? base     : whiteTexture_;
	d.tex[1] = water    ? water    : whiteTexture_;
	d.tex[2] = addition ? addition : whiteTexture_;
	d.tex[3] = border   ? border   : whiteTexture_;

	float* p = d.params;
	p[0] = st.waterColor.r; p[1] = st.waterColor.g; p[2] = st.waterColor.b; p[3] = st.waterColor.a;
	// PSMiniMap::Select splats additionAlpha_ across the whole register; only .x is read.
	p[4] = p[5] = p[6] = p[7] = st.additionAlpha;
	p[8] = st.terraColor.r; p[9] = st.terraColor.g; p[10] = st.terraColor.b; p[11] = st.terraColor.a;
	p[12] = base ? 1.f : 0.f;                                   // else the flat terra colour
	p[13] = water ? 1.f : 0.f;
	p[14] = addition ? (float)st.additionMode : 0.f;
	p[15] = border ? 1.f : 0.f;
	d.paramFloats = 16;

	draws_.push_back(d);
	if(ui_)
		ui_->EmitMinimapRun((int)draws_.size() - 1);
}

void SDLMinimapRenderer::DrawPrims(cTexture* mask, cTexture* texture, float phase, bool lines,
                                   const Vertex* verts, int count)
{
	if(count <= 0 || !verts)
		return;
	if(!(lines ? primLinePipeline_ : primPipeline_))
		return;

	Draw d = {};
	d.map = false;
	d.lines = lines;
	d.first = (int)batch_.size();
	d.count = count;

	batch_.insert(batch_.end(), verts, verts + count);

	SDL_GPUTexture* base   = sdlTextureOf(texture, phase);
	SDL_GPUTexture* border = sdlTextureOf(mask);
	d.tex[0] = base   ? base   : whiteTexture_;
	d.tex[1] = border ? border : whiteTexture_;
	d.tex[2] = d.tex[3] = nullptr;

	d.params[0] = base ? 1.f : 0.f;
	d.params[1] = border ? 1.f : 0.f;
	d.params[2] = d.params[3] = 0.f;
	d.paramFloats = 4;

	draws_.push_back(d);
	if(ui_)
		ui_->EmitMinimapRun((int)draws_.size() - 1);
}

// ---------------------------------------------------------------------------
// Frame, driven by SDLUIRenderer
// ---------------------------------------------------------------------------
void SDLMinimapRenderer::Upload(SDL_GPUCommandBuffer* cmd)
{
	const int vcount = (int)batch_.size();
	if(!device_ || !cmd || vcount == 0)
		return;

	ensureVertexCapacity(vcount);
	if(!vertexBuffer_ || !transferBuffer_)
		return;

	void* map = SDL_MapGPUTransferBuffer(device_, transferBuffer_, true);
	SDL_memcpy(map, batch_.data(), vcount * sizeof(Vertex));
	SDL_UnmapGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation src = {};
	src.transfer_buffer = transferBuffer_;
	src.offset = 0;
	SDL_GPUBufferRegion dst = {};
	dst.buffer = vertexBuffer_;
	dst.offset = 0;
	dst.size = (Uint32)(vcount * sizeof(Vertex));
	SDL_UploadToGPUBuffer(copy, &src, &dst, true);
	SDL_EndGPUCopyPass(copy);
}

void SDLMinimapRenderer::DrawRun(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd, int index,
                                 int screenW, int screenH)
{
	if(!pass || !cmd || !vertexBuffer_ || index < 0 || index >= (int)draws_.size())
		return;

	const Draw& d = draws_[index];
	SDL_GPUGraphicsPipeline* pipeline = d.map ? mapPipeline_
	                                  : (d.lines ? primLinePipeline_ : primPipeline_);
	if(!pipeline)
		return;

	SDL_BindGPUGraphicsPipeline(pass, pipeline);

	const float invScreen[4] = { screenW ? 1.f / screenW : 0.f, screenH ? 1.f / screenH : 0.f, 0.f, 0.f };
	SDL_PushGPUVertexUniformData(cmd, 0, invScreen, sizeof(invScreen));
	SDL_PushGPUFragmentUniformData(cmd, 0, d.params, (Uint32)(d.paramFloats * sizeof(float)));

	SDL_GPUBufferBinding vb = {};
	vb.buffer = vertexBuffer_;
	vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

	const int samplers = d.map ? 4 : 2;
	SDL_GPUTextureSamplerBinding ts[4] = {};
	for(int i = 0; i < samplers; ++i){
		ts[i].texture = d.tex[i] ? d.tex[i] : whiteTexture_;
		ts[i].sampler = sampler_;
	}
	SDL_BindGPUFragmentSamplers(pass, 0, ts, (Uint32)samplers);

	SDL_DrawGPUPrimitives(pass, d.count, 1, d.first, 0);
}

