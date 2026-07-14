// SDL GPU 2D/UI renderer — text, sprites, screen-space quads, lines. See header.
#include "StdAfxRD.h"
#include "SDLUIRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cmath>
#include <string>

#include "Texture.h"     // cTexture (GetDDSurface / frameNumber)
#include "FT_Font.h"     // FT::Font glyph atlas (OutText / OutTextLine)
#include "SDLMinimapRenderer.h"   // replayed inside this renderer's pass, in draw order
#include "SDLRenderDevice.h"       // createSolidGPUTexture

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

// The engine packs its Color4c as BGRA bytes; UBYTE4_NORM reads them in that order and
// the UI shader swizzles to RGBA.
static unsigned int packColor(const Color4c& c)
{
	return (unsigned)c.b | ((unsigned)c.g << 8) | ((unsigned)c.r << 16) | ((unsigned)c.a << 24);
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
	if(samplerWrap_)    SDL_ReleaseGPUSampler(device_, samplerWrap_);
	if(pipeline_)       SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
	if(linePipeline_)   SDL_ReleaseGPUGraphicsPipeline(device_, linePipeline_);
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

	// The same, wrapping: what SetSampler picks for a caller that asked for one of the
	// sampler_wrap_* constants. Only the selection frame's tiled centre does.
	si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerWrap_ = SDL_CreateGPUSampler(device_, &si);

	// 1x1 white texture so untextured quads show the vertex colour.
	whiteTexture_ = createSolidGPUTexture(device_, 0xFFFFFFFFu);

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

	// The 2D primitive batches draw real line lists, as cD3DRender::FlushLine does.
	// Same shaders and vertex layout; only the primitive type differs.
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
	linePipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);
	if(!pipeline_){
		fprintf(stderr, "SDLUIRenderer: CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
		return;
	}
	if(!linePipeline_)
		fprintf(stderr, "SDLUIRenderer: line pipeline failed: %s\n", SDL_GetError());

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

// Take the next slot in the draw order for a minimap draw. It carries no vertices of its
// own, and it always starts a fresh run: the quads emitted after it must not be folded back
// into the run that preceded it, or they would draw *before* the minimap.
void SDLUIRenderer::EmitMinimapRun(int index)
{
	runs_.push_back(DrawRun{ nullptr, nullptr, 0, 0, false, index });
}

void SDLUIRenderer::emitQuad(float x, float y, float dx, float dy,
                             float u, float v, float du, float dv, unsigned int color, SDL_GPUTexture* tex)
{
	// Extend the current run if it uses the same texture, sampler and primitive, else start one.
	if(runs_.empty() || runs_.back().minimap >= 0 || runs_.back().tex != tex ||
	   runs_.back().sampler != currentSampler_ || runs_.back().lines)
		runs_.push_back(DrawRun{ tex, currentSampler_, (int)batch_.size(), 0, false, -1 });

	// Two triangles (the D3D path used a tri-strip of 4 verts; here, 6 verts).
	const float x1 = x, y1 = y, x2 = x + dx, y2 = y + dy;
	const UIVertex tl = { x1, y1, 0, 1, color, u,      v      };
	const UIVertex tr = { x2, y1, 0, 1, color, u + du, v      };
	const UIVertex bl = { x1, y2, 0, 1, color, u,      v + dv };
	const UIVertex br = { x2, y2, 0, 1, color, u + du, v + dv };
	batch_.push_back(tl); batch_.push_back(bl); batch_.push_back(tr);
	batch_.push_back(tr); batch_.push_back(bl); batch_.push_back(br);
	runs_.back().count += 6;
	++quadCount_;
}

// The same quad, from four corners the caller filled itself. The two triangles pair them
// the way the device's standard index buffer does on D3D: 0-1-2 and 2-1-3, i.e. corners in
// the order top-left, bottom-left, top-right, bottom-right.
void SDLUIRenderer::emitQuad(const sVertexXYZWDT1* corners)
{
	if(runs_.empty() || runs_.back().minimap >= 0 || runs_.back().tex != currentTexture_ ||
	   runs_.back().sampler != currentSampler_ || runs_.back().lines)
		runs_.push_back(DrawRun{ currentTexture_, currentSampler_, (int)batch_.size(), 0, false, -1 });

	static const int triangles[6] = { 0, 1, 2, 2, 1, 3 };
	for(int i : triangles){
		const sVertexXYZWDT1& c = corners[i];
		// z/w are the pre-transformed vertex's depth and rhw; the UI shader takes x,y as
		// pixels and writes a fixed depth, as the 2D pipeline has neither depth test nor
		// perspective divide.
		batch_.push_back(UIVertex{ c.x, c.y, 0.f, 1.f, packColor(c.diffuse), c.uv[0], c.uv[1] });
	}
	runs_.back().count += 6;
	++quadCount_;
}

// ---------------------------------------------------------------------------
// cQuadBuffer<sVertexXYZWDT1>'s contract
// ---------------------------------------------------------------------------
// BeginDraw's matrix is accepted and ignored, as it effectively is on D3D: there it reaches
// cD3DRender::setWorldMatrix, which sets the fixed-function world matrix -- which a
// pre-transformed vertex never passes through. Every caller here passes the identity anyway.
void SDLUIRenderer::BeginDraw(const MatXf&)
{
	quadCorners_.clear();
}

sVertexXYZWDT1* SDLUIRenderer::Get()
{
	quadCorners_.resize(quadCorners_.size() + 4);
	return &quadCorners_[quadCorners_.size() - 4];
}

void SDLUIRenderer::EndDraw()
{
	for(size_t i = 0; i + 4 <= quadCorners_.size(); i += 4)
		emitQuad(&quadCorners_[i]);
	quadCorners_.clear();
}

// ---------------------------------------------------------------------------
// cVertexBuffer<sVertexXYZWD>'s contract
// ---------------------------------------------------------------------------
sVertexXYZWD* SDLUIRenderer::Lock(int nVertex)
{
	// The callers ask for one flush's worth (24 vertices) and then keep filling until
	// GetSize() says stop -- which on D3D is thousands of vertices away, since the buffer is
	// the device's and shared. Round up to a comparable floor so a ring is one strip here too.
	lockCapacity_ = nVertex > 1024 ? nVertex : 1024;
	lockVerts_.assign((size_t)lockCapacity_, sVertexXYZWD());
	return lockVerts_.data();
}

void SDLUIRenderer::Unlock(int /*nVertex*/)
{
	// The caller wrote straight into lockVerts_; DrawPrimitive is handed the count.
}

void SDLUIRenderer::DrawPrimitive(PRIMITIVETYPE type, int nPolygon)
{
	if(nPolygon <= 0 || lockVerts_.empty())
		return;

	// Untextured, like the lines: the run carries no texture, so Draw() binds the 1x1 white
	// and the vertex colour comes through unchanged. It could not carry one anyway -- this
	// vertex format has no UVs, which is also why the sampler does not matter here.
	if(runs_.empty() || runs_.back().minimap >= 0 || runs_.back().tex != nullptr || runs_.back().lines)
		runs_.push_back(DrawRun{ nullptr, currentSampler_, (int)batch_.size(), 0, false, -1 });

	const int have = (int)lockVerts_.size();
	int triangles = 0;
	for(int i = 0; i < nPolygon; i++){
		// Triangle i of a strip is vertices i, i+1, i+2; of a list, 3i, 3i+1, 3i+2. The
		// strip's alternating winding is not restored: the UI pipeline culls nothing.
		const int first = (type == PT_TRIANGLESTRIP) ? i : 3 * i;
		if(first + 2 >= have)
			break;
		for(int k = 0; k < 3; k++){
			const sVertexXYZWD& v = lockVerts_[first + k];
			batch_.push_back(UIVertex{ v.x, v.y, 0.f, 1.f, packColor(v.diffuse), 0.f, 0.f });
		}
		++triangles;
	}
	runs_.back().count += triangles * 3;
}

// Untextured: the run carries no texture, so Draw() binds the 1x1 white and the vertex
// colour comes through unchanged.
void SDLUIRenderer::emitLine(float x1, float y1, float x2, float y2, unsigned int color)
{
	if(runs_.empty() || runs_.back().minimap >= 0 || !runs_.back().lines || runs_.back().tex != nullptr)
		runs_.push_back(DrawRun{ nullptr, currentSampler_, (int)batch_.size(), 0, true, -1 });

	batch_.push_back(UIVertex{ x1, y1, 0, 1, color, 0.f, 0.f });
	batch_.push_back(UIVertex{ x2, y2, 0, 1, color, 0.f, 0.f });
	runs_.back().count += 2;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
void SDLUIRenderer::BeginFrame()
{
	batch_.clear();
	runs_.clear();
	currentTexture_ = nullptr;
	currentSampler_ = nullptr;   // => sampler_, the clamped one every 2D caller but one wants
	quadCount_ = 0;
	if(minimap_)
		minimap_->BeginFrame();   // its draws are sequenced by our run list, so they age with it
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

	// The minimap draws inside our pass, so its vertices have to be uploaded before that
	// pass opens -- a copy pass cannot run inside a render pass.
	if(minimap_)
		minimap_->Upload(cmd);

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
	if(!runs_.empty()){
		const float invScreen[4] = { screenW ? 1.f / screenW : 0.f, screenH ? 1.f / screenH : 0.f, 0.f, 0.f };
		SDL_GPUBufferBinding vb = {};
		vb.buffer = vertexBuffer_;
		vb.offset = 0;

		// One draw per run, in the order the engine issued them; bind the run's texture
		// (white for untextured geometry) and swap pipelines when the primitive changes.
		SDL_GPUGraphicsPipeline* bound = nullptr;
		for(const DrawRun& run : runs_){
			if(run.minimap >= 0){
				// The minimap's own pipelines, vertex buffer and uniforms, drawn here in our
				// pass so it lands between the panels behind it and the labels in front. It
				// leaves all three bound to its own, so start the next UI run from scratch.
				if(minimap_)
					minimap_->DrawRun(pass, cmd, run.minimap, screenW, screenH);
				bound = nullptr;
				continue;
			}
			SDL_GPUGraphicsPipeline* want = run.lines ? linePipeline_ : pipeline_;
			if(!want || !vertexBuffer_) continue;
			if(want != bound){
				SDL_BindGPUGraphicsPipeline(pass, want);
				SDL_PushGPUVertexUniformData(cmd, 0, invScreen, sizeof(invScreen));
				SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
				bound = want;
			}
			SDL_GPUTextureSamplerBinding ts = {};
			ts.texture = run.tex ? run.tex : whiteTexture_;
			ts.sampler = run.sampler ? run.sampler : sampler_;
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

void SDLUIRenderer::SetSampler(const SAMPLER_DATA& data)
{
	currentSampler_ = data.addressu == DX_TADDRESS_WRAP ? samplerWrap_ : sampler_;
}

void SDLUIRenderer::DrawQuad(float x1, float y1, float dx, float dy,
                             float u1, float v1, float du, float dv, Color4c color)
{
	emitQuad(x1, y1, dx, dy, u1, v1, du, dv, packColor(color), currentTexture_);
}

void SDLUIRenderer::DrawSprite(int x, int y, int dx, int dy,
                               float u, float v, float du, float dv,
                               cTexture* texture, const Color4c& colorMul)
{
	emitQuad((float)x, (float)y, (float)dx, (float)dy, u, v, du, dv,
	         packColor(colorMul), sdlTextureOf(texture));
}

void SDLUIRenderer::DrawLine(int x1, int y1, int x2, int y2, Color4c color)
{
	emitLine((float)x1, (float)y1, (float)x2, (float)y2, packColor(color));
}

// The D3D backend queues these as a point list. A 1x1 quad rasterizes to the same
// pixel, and spares the backend a third pipeline for a call nothing currently makes.
void SDLUIRenderer::DrawPixel(int x, int y, Color4c color)
{
	emitQuad((float)x, (float)y, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, packColor(color), nullptr);
}

void SDLUIRenderer::DrawRectangle(int x, int y, int dx, int dy, Color4c color, bool outline)
{
	const unsigned int c = packColor(color);
	const float x1 = (float)x, y1 = (float)y, x2 = (float)(x + dx), y2 = (float)(y + dy);
	if(outline){
		emitLine(x1, y1, x2, y1, c);
		emitLine(x2, y1, x2, y2, c);
		emitLine(x2, y2, x1, y2, c);
		emitLine(x1, y2, x1, y1, c);
	}
	else
		emitQuad(x1, y1, (float)dx, (float)dy, 0.f, 0.f, 0.f, 0.f, c, nullptr);
}

// The engine's inline colour escape: "&rrggbb" sets the text colour from here on, and
// "&&" is a literal ampersand. Advances str past whatever it consumed. Lifted from
// ChangeTextColorW in Render/D3D/D3DRenderDraw.cpp, which is not built off-Windows.
static void changeTextColor(const wchar_t*& str, Color4c& diffuse)
{
	while(*str == L'&'){
		++str;
		if(*str == L'&')
			return;                       // "&&" -> draw one '&'
		unsigned int s = 0;
		int i = 0;
		for(; i < 6; ++i, ++str){
			const wchar_t k = *str;
			if(k >= L'0' && k <= L'9')      s = (s << 4) + (k - L'0');
			else if(k >= L'A' && k <= L'F') s = (s << 4) + (k - L'A' + 10);
			else if(k >= L'a' && k <= L'f') s = (s << 4) + (k - L'a' + 10);
			else break;                   // includes the terminator
		}
		if(i <= 5)
			return;                       // not six hex digits: not an escape after all
		diffuse.RGBA() &= 0xFF000000;     // keep alpha, replace rgb
		diffuse.RGBA() |= s;
	}
}

// Multi-line, aligned, scalable text. Mirrors cD3DRender::OutText, including its
// half-pixel glyph offsets and the one-texel bleed it adds when scaling.
void SDLUIRenderer::OutText(int x0, int y0, const char* text, const FT::Font& font,
                            const Color4f& color, ALIGN_TEXT align, Vect2f scale)
{
	if(!text || !font.texture())
		return;
	SDL_GPUTexture* tex = sdlTextureOf(font.texture());
	const float txWidth  = float(font.texture()->GetWidth());
	const float txHeight = float(font.texture()->GetHeight());
	if(txWidth <= 0.f || txHeight <= 0.f)
		return;

	// The D3D path widens through a2w(). Every caller passes ASCII, and this port's
	// MultiByteToWideChar treats CP_ACP as a single-byte passthrough, so widening byte
	// by byte gives the same code points without dragging in the conversion.
	std::wstring out;
	for(const char* p = text; *p; ++p)
		out.push_back((wchar_t)(unsigned char)*p);

	if(fabsf(scale.x - 1.0f) < 0.01f) scale.x = 1.f;
	if(fabsf(scale.y - 1.0f) < 0.01f) scale.y = 1.f;
	const bool scaled = (scale.x != 1.f || scale.y != 1.f);

	Color4c diffuse(color);
	const wchar_t* str = out.c_str();
	float y = float(y0);

	while(*str){
		float x = float(x0);
		if(align >= ALIGN_TEXT_CENTER){
			const float width = scale.x * float(font.lineWidth(str));
			x -= (align == ALIGN_TEXT_CENTER) ? width * 0.5f : width;
		}
		x = float(round(x));

		for(; *str && *str != L'\n'; ++str){
			changeTextColor(str, diffuse);   // may land on the terminator or a newline
			const wchar_t symbol = *str;
			if(!symbol || symbol == L'\n')
				break;
			if(symbol < 32)
				continue;

			const FT::OneChar& one = font.getChar(symbol);
			const unsigned int c = packColor(diffuse);

			if(scaled){
				// Scaling samples between texels, so the glyph is grown by one texel on
				// each axis and its source rect widened to match.
				const float px = float(round(x + scale.x * float(one.su - 1))) - 0.5f;
				const float py = float(round(y + scale.y * float(one.sv - 1))) - 0.5f;
				emitQuad(px, py,
				         float(round(scale.x * float(one.du + 1))), float(round(scale.y * float(one.dv + 1))),
				         float(one.u - 1) / txWidth, float(one.v - 1) / txHeight,
				         float(one.du + 1) / txWidth, float(one.dv + 1) / txHeight, c, tex);
				x += scale.x * float(one.advance);
			}
			else{
				emitQuad(float(x + one.su) - 0.5f, float(y + one.sv) - 0.5f,
				         float(one.du), float(one.dv),
				         float(one.u) / txWidth, float(one.v) / txHeight,
				         float(one.du) / txWidth, float(one.dv) / txHeight, c, tex);
				x += float(one.advance);
			}
		}

		if(*str == L'\n')
			++str;
		y += scale.y * float(font.lineHeight());
	}
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

	const unsigned int c = packColor(color);

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
