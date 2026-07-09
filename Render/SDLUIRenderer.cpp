// SDL GPU 2D/UI renderer — text, sprites and screen-space quads. See header.
#include "StdAfxRD.h"
#include "SDLUIRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>

#include "Texture.h"     // cTexture (GetDDSurface / frameNumber)
#include "FT_Font.h"     // FT::Font glyph atlas (OutTextLine)

// Cross-compiled UI shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/ui_shaders.h"

// Look up the SDL texture a cTexture is backed by (null => untextured/white).
// cSDLRenderDevice::CreateTexture parks the SDL_GPUTexture* in BitMap[0].
static SDL_GPUTexture* sdlTextureOf(const cTexture* t)
{
	if(t && t->frameNumber() >= 1)
		return reinterpret_cast<SDL_GPUTexture*>(const_cast<cTexture*>(t)->GetDDSurface(0));
	return nullptr;
}

SDLUIRenderer::SDLUIRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipeline();
}

SDLUIRenderer::~SDLUIRenderer()
{
	if(!device_) return;
	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
	if(whiteTexture_)   SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(sampler_)        SDL_ReleaseGPUSampler(device_, sampler_);
	if(pipeline_)       SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLUIRenderer::createPipeline()
{
	if(!device_ || !window_) return;

	// The sampler and the white texture are independent of the pipeline, so build
	// them first: a shader/pipeline failure must not leave them null behind us.
	SDL_GPUSamplerCreateInfo si = {};
	si.min_filter = SDL_GPU_FILTER_LINEAR;
	si.mag_filter = SDL_GPU_FILTER_LINEAR;
	si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_ = SDL_CreateGPUSampler(device_, &si);

	// 1x1 white texture so untextured quads show the vertex colour.
	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = 1; ti.height = 1; ti.layer_count_or_depth = 1; ti.num_levels = 1;
	whiteTexture_ = SDL_CreateGPUTexture(device_, &ti);
	if(whiteTexture_){
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = 4;
		SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
		if(tb){
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

	// Pick a shader format the backend supports (Metal->MSL, Vulkan->SPIRV).
	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode;
	unsigned int vsSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = ui_vert_msl; vsSize = ui_vert_msl_len;
		fsCode = ui_frag_msl; fsSize = ui_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = ui_vert_spv; vsSize = ui_vert_spv_len;
		fsCode = ui_frag_spv; fsSize = ui_frag_spv_len;
	} else {
		fprintf(stderr, "SDLUIRenderer: no supported shader format (0x%x)\n", formats);
		return;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLUIRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(UIVertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[3] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;      attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM; attrs[1].offset = 16;
	attrs[2].location = 2; attrs[2].buffer_slot = 0; attrs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;      attrs[2].offset = 20;

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
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 3;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;

	pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);
	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
	if(!pipeline_){
		fprintf(stderr, "SDLUIRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		return;
	}

	fprintf(stderr, "SDLUIRenderer: UI pipeline ready\n");
}

void SDLUIRenderer::ensureVertexCapacity(int verts)
{
	if(verts <= vertexCapacity_) return;
	int cap = vertexCapacity_ ? vertexCapacity_ : 1024;
	while(cap < verts) cap *= 2;

	if(vertexBuffer_)   SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
	if(transferBuffer_) SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);

	SDL_GPUBufferCreateInfo bi = {};
	bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bi.size = (Uint32)(cap * sizeof(UIVertex));
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &bi);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)(cap * sizeof(UIVertex));
	transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &tbi);

	vertexCapacity_ = (vertexBuffer_ && transferBuffer_) ? cap : 0;
}

void SDLUIRenderer::emitQuad(float x, float y, float dx, float dy,
                             float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex)
{
	// Extend the current run if it uses the same texture, else start a new one.
	if(runs_.empty() || runs_.back().tex != tex)
		runs_.push_back(DrawRun{ tex, (int)batch_.size(), 0 });

	// Two triangles (the D3D path used a tri-strip of 4 verts; here, 6 verts).
	const float x1 = x, y1 = y, x2 = x + dx, y2 = y + dy;
	const UIVertex tl = { x1, y1, 0, 1, color, u,      v      };
	const UIVertex tr = { x2, y1, 0, 1, color, u + du, v      };
	const UIVertex bl = { x1, y2, 0, 1, color, u,      v + dv };
	const UIVertex br = { x2, y2, 0, 1, color, u + du, v + dv };
	batch_.push_back(tl); batch_.push_back(bl); batch_.push_back(tr);
	batch_.push_back(tr); batch_.push_back(bl); batch_.push_back(br);
	runs_.back().count += 6;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
void SDLUIRenderer::BeginFrame()
{
	batch_.clear();
	runs_.clear();
	currentTexture_ = nullptr;
}

void SDLUIRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target,
                         int screenW, int screenH, bool clear, const float clearColor[4])
{
	if(!device_ || !cmd || !target) return;

	const int vcount = (int)batch_.size();

	// Upload the accumulated quads. The copy pass must be closed before the render
	// pass opens, so it runs first.
	if(vcount > 0 && pipeline_){
		ensureVertexCapacity(vcount);
		if(vertexBuffer_ && transferBuffer_){
			void* map = SDL_MapGPUTransferBuffer(device_, transferBuffer_, true);
			SDL_memcpy(map, batch_.data(), vcount * sizeof(UIVertex));
			SDL_UnmapGPUTransferBuffer(device_, transferBuffer_);

			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
			SDL_GPUTransferBufferLocation src = {};
			src.transfer_buffer = transferBuffer_;
			src.offset = 0;
			SDL_GPUBufferRegion dst = {};
			dst.buffer = vertexBuffer_;
			dst.offset = 0;
			dst.size = (Uint32)(vcount * sizeof(UIVertex));
			SDL_UploadToGPUBuffer(copy, &src, &dst, true);
			SDL_EndGPUCopyPass(copy);
		}
	}

	// Opened unconditionally: with an empty batch this pass is still what carries
	// the frame's clear.
	SDL_GPUColorTargetInfo ct = {};
	ct.texture = target;
	ct.clear_color.r = clearColor[0];
	ct.clear_color.g = clearColor[1];
	ct.clear_color.b = clearColor[2];
	ct.clear_color.a = clearColor[3];
	ct.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	ct.store_op = SDL_GPU_STOREOP_STORE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, nullptr);
	if(vcount > 0 && pipeline_ && vertexBuffer_){
		SDL_BindGPUGraphicsPipeline(pass, pipeline_);
		float invScreen[4] = { screenW ? 1.f / screenW : 0.f, screenH ? 1.f / screenH : 0.f, 0.f, 0.f };
		SDL_PushGPUVertexUniformData(cmd, 0, invScreen, sizeof(invScreen));
		SDL_GPUBufferBinding vb = {};
		vb.buffer = vertexBuffer_;
		vb.offset = 0;
		SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
		// One draw per run; bind the run's texture (white for untextured quads).
		for(const DrawRun& run : runs_){
			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = run.tex ? run.tex : whiteTexture_;
			ts.sampler = sampler_;
			SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
			SDL_DrawGPUPrimitives(pass, run.count, 1, run.first, 0);
		}
	}
	SDL_EndGPURenderPass(pass);
}

// ---------------------------------------------------------------------------
// 2D entry points
// ---------------------------------------------------------------------------
void SDLUIRenderer::SetTexture(cTexture* texture)
{
	currentTexture_ = sdlTextureOf(texture);
}

void SDLUIRenderer::DrawQuad(float x1, float y1, float dx, float dy,
                             float u1, float v1, float du, float dv, Color4c color)
{
	unsigned int c = (unsigned)color.b | ((unsigned)color.g << 8)
	               | ((unsigned)color.r << 16) | ((unsigned)color.a << 24);
	emitQuad(x1, y1, dx, dy, u1, v1, du, dv, c, currentTexture_);
}

void SDLUIRenderer::DrawSprite(int x, int y, int dx, int dy,
                               float u, float v, float du, float dv,
                               cTexture* texture, const Color4c& colorMul)
{
	// Pack BGRA bytes (matches UBYTE4_NORM read order; the shader swizzles to RGBA).
	unsigned int color = (unsigned)colorMul.b | ((unsigned)colorMul.g << 8)
	                   | ((unsigned)colorMul.r << 16) | ((unsigned)colorMul.a << 24);
	emitQuad((float)x, (float)y, (float)dx, (float)dy, u, v, du, dv, color, sdlTextureOf(texture));
}

// Emit one textured quad per glyph from the FreeType atlas. Mirrors the D3D
// cD3DRender::OutTextLine glyph-placement math. The atlas is a GRAY texture uploaded
// as BGRA (255,255,255,coverage), so sampling gives white with the coverage in alpha;
// the vertex colour tints it (handled by the UI shader).
int SDLUIRenderer::OutTextLine(int x, int y, const FT::Font& font,
                               const wchar_t* textline, const wchar_t* end,
                               const Color4c& color, int xRangeMin, int xRangeMax)
{
	if(!font.texture())
		return x;

	SDL_GPUTexture* tex = sdlTextureOf(font.texture());
	const float txWidth  = float(font.texture()->GetWidth());
	const float txHeight = float(font.texture()->GetHeight());
	if(txWidth <= 0 || txHeight <= 0)
		return x;

	const unsigned int c = (unsigned)color.b | ((unsigned)color.g << 8)
	                     | ((unsigned)color.r << 16) | ((unsigned)color.a << 24);

	int prev_rh = 0;
	int prev_right = x;
	for(const wchar_t* str = textline; str != end; ++str){
		wchar_t symbol = *str;
		if(symbol < 32)
			continue;

		const FT::OneChar& one = font.getChar(symbol);

		int advance = one.advance;
		if(prev_rh - one.lh >= 32)
			--advance;
		else if(prev_rh - one.lh < -32)
			++advance;
		prev_rh = one.rh;

		int right = x + max(advance, (int)one.su + (int)one.du);

		if(xRangeMin >= 0 && x < xRangeMin){
			prev_right = right;
			x += advance;
			continue;
		}
		if(xRangeMax >= 0 && right > xRangeMax)
			break;

		// Empty space around glyphs is compressed in the atlas, so apply the
		// per-glyph offsets (su,sv) and the glyph extent (du,dv).
		float px = float(x + one.su) - 0.5f;
		float py = float(y + one.sv) - 0.5f;
		float u  = float(one.u) / txWidth;
		float v  = float(one.v) / txHeight;
		float du = float(one.du) / txWidth;
		float dv = float(one.dv) / txHeight;
		emitQuad(px, py, float(one.du), float(one.dv), u, v, du, dv, c, tex);

		prev_right = right;
		x += advance;
	}
	return prev_right;
}

#endif // !_WIN32
