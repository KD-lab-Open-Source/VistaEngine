// SDL GPU tilemap (terrain) renderer. See header.
#include "StdAfxRD.h"
#include "SDLTileMapRenderer.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <vector>

#include "terra/vmap.h"        // vMap heightfield + baked per-cell surface colour
#include "cCamera.h"           // Camera::matView / matProj / GetLighting / vp
#include "TileMap.h"           // cTileMap::GetDiffuse (the scene sun, per Environment)
#include "SDLRenderDevice.h"   // applyCameraViewport

// Cross-compiled tilemap shader blobs (SPIR-V + MSL); see Render/SDLShaders.
#include "SDLShaders/tilemap_shaders.h"

namespace {

// Uniform blocks, laid out to match tilemap.{vert,frag}.hlsl exactly.
struct VSUniform { float mvp[16]; float uv[4]; };          // row_major float4x4 MVP; float4 UV
struct FSUniform { float lightColor[4]; float lightDir[4]; };

// Sample every STEP_BASE fine cells (512/4 = 128 quads/axis -> 129x129 = 16641 verts
// on the Menu). The step is doubled below as needed so the vertex count stays under
// 65536 -- campaign maps are larger than the Menu's 512, and a 16-bit index buffer
// can only address 65535 vertices (a 1024 map at step 4 would be 257x257 = 66049).
const int STEP_BASE = 4;

// The baked surface-colour texture is capped on each side; larger maps are averaged
// down by vMap.getTileColor32Layer's step (the sampler interpolates the rest).
const int MAX_TEX = 2048;

} // namespace

SDLTileMapRenderer::SDLTileMapRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipeline();
}

SDLTileMapRenderer::~SDLTileMapRenderer()
{
	if(!device_) return;
	releaseMesh();
	if(whiteTexture_) SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(sampler_)      SDL_ReleaseGPUSampler(device_, sampler_);
	if(pipelineFill_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineFill_);
	if(pipelineLine_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLine_);
}

void SDLTileMapRenderer::releaseMesh()
{
	if(!device_) return;
	if(vertexBuffer_){ SDL_ReleaseGPUBuffer(device_, vertexBuffer_); vertexBuffer_ = nullptr; }
	if(indexBuffer_) { SDL_ReleaseGPUBuffer(device_, indexBuffer_);  indexBuffer_  = nullptr; }
	if(colorTexture_){ SDL_ReleaseGPUTexture(device_, colorTexture_); colorTexture_ = nullptr; }
	indexCount_ = 0;
}

// ---------------------------------------------------------------------------
// Pipeline
// ---------------------------------------------------------------------------
void SDLTileMapRenderer::createPipeline()
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

	// 1x1 white, bound instead of the surface colour in wireframe mode so the edges
	// come out white whatever the map's baked colour is.
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

	SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
	SDL_GPUShaderFormat fmt;
	const char* entry;
	const unsigned char *vsCode, *fsCode;
	unsigned int vsSize, fsSize;
	if(formats & SDL_GPU_SHADERFORMAT_MSL){
		fmt = SDL_GPU_SHADERFORMAT_MSL; entry = "main0";
		vsCode = tilemap_vert_msl; vsSize = tilemap_vert_msl_len;
		fsCode = tilemap_frag_msl; fsSize = tilemap_frag_msl_len;
	} else if(formats & SDL_GPU_SHADERFORMAT_SPIRV){
		fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
		vsCode = tilemap_vert_spv; vsSize = tilemap_vert_spv_len;
		fsCode = tilemap_frag_spv; fsSize = tilemap_frag_spv_len;
	} else {
		fprintf(stderr, "SDLTileMapRenderer: no supported shader format (0x%x)\n", formats);
		return;
	}

	SDL_GPUShaderCreateInfo vsi = {};
	vsi.code = vsCode; vsi.code_size = vsSize; vsi.entrypoint = entry;
	vsi.format = fmt; vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + UV
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = {};
	fsi.code = fsCode; fsi.code_size = fsSize; fsi.entrypoint = entry;
	fsi.format = fmt; fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 1;           // the baked surface-colour texture
	fsi.num_uniform_buffers = 1;    // LightColor + LightDirection
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLTileMapRenderer: CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// Vertex: float3 position @0, float3 normal @12 (stride 24).
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(Vertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[2] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[1].offset = 12;

	// Opaque base geometry: no blending, depth test and write on. The terrain is the
	// first thing drawn each frame, so everything after it depth-tests against it.
	SDL_GPUColorTargetDescription colorTarget = {};
	colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = attrs;
	pci.vertex_input_state.num_vertex_attributes = 2;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;   // grid winding isn't guaranteed
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = true;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineFill_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	// Wireframe variant (RS_FILLMODE == FILL_WIREFRAME). Same everything but LINE fill.
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	pipelineLine_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLTileMapRenderer: tilemap pipeline %s (wireframe %s)\n",
	        pipelineFill_ ? "ready" : "FAILED", pipelineLine_ ? "ready" : "FAILED");
}

// ---------------------------------------------------------------------------
// Geometry + surface colour, built from vMap and uploaded through the frame's
// command buffer (copy passes must be recorded before any render pass opens).
// ---------------------------------------------------------------------------

// Bake the terrain's per-fine-cell surface colour into one texture spanning the whole
// map. The colour lives per cell in vMap.clrBuf (RGB565) -- the same data the D3D tile
// renderer paints its tiles from. getTileColor32Layer expands it to ARGB DWORDs
// (0xAARRGGBB == B,G,R,A bytes in memory), which is exactly B8G8R8A8_UNORM.
bool SDLTileMapRenderer::buildColorTexture(SDL_GPUCommandBuffer* cmd, int H, int V)
{
	int step = 1;
	while((H / step) > MAX_TEX || (V / step) > MAX_TEX) step *= 2;
	int tw = H / step, th = V / step;

	std::vector<unsigned char> pixels;
	bool baked = false;
	if(tw > 0 && th > 0){
		pixels.assign((size_t)tw * th * 4, 0);
		vMap.getTileColor32Layer(pixels.data(), (DWORD)(tw * 4), 0, 0, H, V, step);
		baked = true;
	}
	if(!baked){
		// Flat warm earth, so a failed bake still shows lit relief rather than black.
		tw = th = 1;
		pixels.assign(4, 0);
		pixels[0] = 56; pixels[1] = 102; pixels[2] = 140; pixels[3] = 255;  // B,G,R,A
	}

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = (Uint32)tw; ti.height = (Uint32)th;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	colorTexture_ = SDL_CreateGPUTexture(device_, &ti);
	if(!colorTexture_) return false;

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)pixels.size();
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return false;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	SDL_memcpy(map, pixels.data(), pixels.size());
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src = {};
	src.transfer_buffer = tb; src.offset = 0;
	src.pixels_per_row = (Uint32)tw; src.rows_per_layer = (Uint32)th;
	SDL_GPUTextureRegion dst = {};
	dst.texture = colorTexture_; dst.w = (Uint32)tw; dst.h = (Uint32)th; dst.d = 1;
	SDL_UploadToGPUTexture(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device_, tb);   // destruction deferred until the copy runs
	return true;
}

bool SDLTileMapRenderer::buildMesh(SDL_GPUCommandBuffer* cmd)
{
	const int H = (int)vMap.H_SIZE;
	const int V = (int)vMap.V_SIZE;
	if(H <= 0 || V <= 0)
		return false;   // heightfield not loaded yet -- caller retries next frame

	// Pick the finest step whose grid fits in 16-bit indices.
	int step = STEP_BASE;
	while(((H / step) + 1) * ((V / step) + 1) >= 65536)
		step *= 2;

	const int nx = H / step, ny = V / step;   // grid quads per axis
	const int gw = nx + 1,   gh = ny + 1;     // vertices per axis
	const int vcount = gw * gh;
	const int tcount = nx * ny * 2;           // triangles

	// The vMap<->world frame: vMap is H_SIZE x V_SIZE fine cells, world XY == fine
	// cell, and the world Z of the surface is getZf(x,y) = getAlt/vx_fraction. This is
	// NOT the D3D tilemap's round(getZ*64) fixed point (which its world matrix undoes);
	// getZf gives world units that line up with the camera directly.
	std::vector<Vertex> verts((size_t)vcount);
	for(int gy = 0; gy < gh; ++gy){
		int y = gy * step; if(y > V - 1) y = V - 1;
		for(int gx = 0; gx < gw; ++gx){
			int x = gx * step; if(x > H - 1) x = H - 1;
			Vertex& v = verts[(size_t)gy * gw + gx];
			v.x = (float)x; v.y = (float)y; v.z = vMap.getZf(x, y);
			Vect3f nrm; vMap.getNormal(x, y, nrm);
			v.nx = nrm.x; v.ny = nrm.y; v.nz = nrm.z;
		}
	}

	std::vector<unsigned short> idx((size_t)tcount * 3);
	size_t t = 0;
	for(int gy = 0; gy < ny; ++gy)
		for(int gx = 0; gx < nx; ++gx){
			unsigned short a = (unsigned short)(gy * gw + gx), b = (unsigned short)(a + 1);
			unsigned short c = (unsigned short)(a + gw),       d = (unsigned short)(c + 1);
			idx[t++] = a; idx[t++] = c; idx[t++] = b;   // winding irrelevant (cull NONE)
			idx[t++] = b; idx[t++] = c; idx[t++] = d;
		}

	const Uint32 vbytes = (Uint32)(verts.size() * sizeof(Vertex));
	const Uint32 ibytes = (Uint32)(idx.size() * sizeof(unsigned short));

	SDL_GPUBufferCreateInfo vbi = {};
	vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX; vbi.size = vbytes;
	vertexBuffer_ = SDL_CreateGPUBuffer(device_, &vbi);

	SDL_GPUBufferCreateInfo ibi = {};
	ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX; ibi.size = ibytes;
	indexBuffer_ = SDL_CreateGPUBuffer(device_, &ibi);

	if(!vertexBuffer_ || !indexBuffer_){ releaseMesh(); return false; }

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = vbytes + ibytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb){ releaseMesh(); return false; }

	unsigned char* map = (unsigned char*)SDL_MapGPUTransferBuffer(device_, tb, false);
	SDL_memcpy(map, verts.data(), vbytes);
	SDL_memcpy(map + vbytes, idx.data(), ibytes);
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation vsrc = {}; vsrc.transfer_buffer = tb; vsrc.offset = 0;
	SDL_GPUBufferRegion vdst = {}; vdst.buffer = vertexBuffer_; vdst.offset = 0; vdst.size = vbytes;
	SDL_UploadToGPUBuffer(copy, &vsrc, &vdst, false);
	SDL_GPUTransferBufferLocation isrc = {}; isrc.transfer_buffer = tb; isrc.offset = vbytes;
	SDL_GPUBufferRegion idst = {}; idst.buffer = indexBuffer_; idst.offset = 0; idst.size = ibytes;
	SDL_UploadToGPUBuffer(copy, &isrc, &idst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device_, tb);   // destruction deferred until the copies run

	if(!buildColorTexture(cmd, H, V)){ releaseMesh(); return false; }

	// The colour map spans the world, so the shader's uv = pos.xy * (1/H, 1/V).
	uvScale_[0] = 1.f / (float)H;
	uvScale_[1] = 1.f / (float)V;
	indexCount_ = (int)idx.size();

	fprintf(stderr, "SDLTileMapRenderer: terrain mesh %dx%d step %d (%d verts, %d tris)\n",
	        H, V, step, vcount, tcount);
	return true;
}

bool SDLTileMapRenderer::ensureMesh(SDL_GPUCommandBuffer* cmd)
{
	// vMap is reloaded in place on every mission change (the Menu's placeholder ->
	// a campaign map's real heightfield). Keyed on the world name (set by vrtMap::load;
	// empty mid-load -> ignored), tear the stale mesh down and rebuild.
	std::string world = vMap.getWorldName().c_str();
	if(indexCount_ > 0 && !world.empty() && world != builtWorld_){
		releaseMesh();
		buildFailed_ = false;
	}

	if(indexCount_ == 0){
		if(buildFailed_) return false;
		if(!buildMesh(cmd)){
			// H_SIZE==0 means the heightfield isn't loaded yet: retry next frame.
			// A genuine build failure latches, so we stop retrying every frame.
			if(vMap.H_SIZE != 0) buildFailed_ = true;
			return false;
		}
		builtWorld_ = world;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
bool SDLTileMapRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                              int screenW, int screenH, bool clear, const float clearColor[4],
                              bool clearDepth, cTileMap* tileMap, Camera* camera, bool wireframe)
{
	// Fall back to the solid pipeline if the LINE variant failed to build.
	SDL_GPUGraphicsPipeline* pipeline = (wireframe && pipelineLine_) ? pipelineLine_ : pipelineFill_;
	if(!device_ || !pipeline || !cmd || !target || !depth || !camera)
		return false;
	if(!ensureMesh(cmd))
		return false;

	// The camera's own view-projection, unmodified. The depth this pass writes is then
	// in the same space as every other renderer's, so they occlude each other correctly.
	VSUniform vsu;
	std::memcpy(vsu.mvp, &camera->matViewProj, sizeof(vsu.mvp));
	vsu.uv[0] = 0.f; vsu.uv[1] = 0.f;             // UV.xy offset
	vsu.uv[2] = uvScale_[0]; vsu.uv[3] = uvScale_[1];  // UV.zw scale

	// The scene's sun, exactly as the original wires it up.
	//
	// vColor (rgb = diffuse, w = ambient): EnvironmentTime::SetTime takes the
	// time-of-day sun colour, sets .a = shadowing.ambient() and scales the rgb by
	// shadowing.scaleDiffuse(), then hands it to cTileMap::SetDiffuse
	// (Water/SkyObject.cpp:821). DrawType::SetTileColor passes that through to the
	// shader, having first multiplied rgb by the per-material zeroplast colour -- we
	// draw one unmaterialed submesh, and zeroplast defaults to white, so we skip it.
	// Note diffuse rgb may exceed 1 (SetDiffuse only asserts < 100): a bright sun
	// brightens terrain past its baked texel, which the old hardcoded values could not.
	//
	// vLightDirection is the scene's sun_direction -- the way the light *travels*, so a
	// lit surface has dot(N, dir) == -1, matching the shader's saturate(-dot(N, dir)).
	FSUniform fsu;
	const Color4f diffuse = tileMap ? tileMap->GetDiffuse() : Color4f(0.45f, 0.45f, 0.45f, 0.55f);
	fsu.lightColor[0] = diffuse.r;
	fsu.lightColor[1] = diffuse.g;
	fsu.lightColor[2] = diffuse.b;
	fsu.lightColor[3] = diffuse.a;

	Vect3f sun(0.f, 0.f, -1.f);   // straight down if the scene has no sun yet
	camera->GetLighting(sun);
	fsu.lightDir[0] = sun.x; fsu.lightDir[1] = sun.y; fsu.lightDir[2] = sun.z; fsu.lightDir[3] = 0.f;

	// Wireframe is a diagnostic: kill the diffuse term and drive ambient to 1, so with
	// the white texture bound below every edge comes out full white regardless of the
	// map's baked surface colour (the Menu world's is all zeros).
	if(wireframe){
		fsu.lightColor[0] = fsu.lightColor[1] = fsu.lightColor[2] = 0.f;
		fsu.lightColor[3] = 1.f;
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
	dt.store_op = SDL_GPU_STOREOP_STORE;   // kept: the object/water renderers depth-test against it
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &ct, 1, &dt);
	applyCameraViewport(pass, camera->vp, screenW, screenH);

	SDL_BindGPUGraphicsPipeline(pass, pipeline);
	SDL_PushGPUVertexUniformData(cmd, 0, &vsu, sizeof(vsu));
	SDL_PushGPUFragmentUniformData(cmd, 0, &fsu, sizeof(fsu));

	SDL_GPUBufferBinding vb = {}; vb.buffer = vertexBuffer_; vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
	SDL_GPUBufferBinding ib = {}; ib.buffer = indexBuffer_; ib.offset = 0;
	SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_GPUTextureSamplerBinding ts = {};
	ts.texture = (wireframe && whiteTexture_) ? whiteTexture_ : colorTexture_;
	ts.sampler = sampler_;
	SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);

	SDL_DrawGPUIndexedPrimitives(pass, indexCount_, 1, 0, 0, 0);
	SDL_EndGPURenderPass(pass);
	return true;
}

#endif // !_WIN32
