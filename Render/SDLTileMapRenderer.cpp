// SDL GPU tilemap (terrain) renderer. See header.
#include "StdAfxRD.h"
#include "SDLTileMapRenderer.h"


#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <vector>

#include "Terra/VMAP.H"        // vMap heightfield + baked per-cell surface colour
#include "MultiRegion.h"       // vMap.region(): the per-cell material map
#include "cCamera.h"           // Camera::matView / matProj / GetLighting / vp
#include "TileMap.h"           // cTileMap::GetDiffuse, miniDetailTexture (per material)
#include "Texture.h"           // cTexture::GetDDSurface / GetWidth / GetHeight
#include "Scene.h"             // cScene::GetShadowIntensity (the shader's vShade)
#include "VisGeneric.h"        // Option_filterShadow, Option_DetailTexture
#include "SDLRenderDevice.h"   // applyCameraViewport, the shadow map

// Terrain shader bytecode, compiled to this platform's native format at build time;
// see Render/CMakeLists.txt.
#include "SDLShaders/tilemap_shaders.h"
#include "SDLShaders/ShaderBlob.h"

namespace {

// Uniform blocks, laid out to match tilemap.{vert,frag}.hlsl exactly.
struct VSUniform { float mvp[16]; float uv[4]; float shadow[16]; float planarNode[4];
                   float miniTexture[4]; float fogPlane[4]; };
struct FSUniform { float lightColor[4]; float lightDir[4]; float shade[4]; float params[4];
                   float lightMapParams[4]; float detailParams[4]; float fogColor[4]; };
// tilemap_shadow.vert.hlsl's whole cbuffer: the light camera's view-projection.
struct ShadowVSUniform { float mvp[16]; };

// Sample every STEP_BASE fine cells (512/4 = 128 quads/axis -> 129x129 = 16641 verts
// on the Menu). The step is doubled below as needed so the vertex count stays under
// 65536 -- campaign maps are larger than the Menu's 512, and a 16-bit index buffer
// can only address 65535 vertices (a 1024 map at step 4 would be 257x257 = 66049).
const int STEP_BASE = 4;

// The baked surface-colour texture is capped on each side; larger maps are averaged
// down by vMap.getTileColor32Layer's step (the sampler interpolates the rest).
const int MAX_TEX = 2048;

// The detail texture a material draws with, or null. This is exactly what
// cTileMap::setMaterial hands to DrawType::SetMaterialTilemap: the material's own entry in
// cTileMap's miniDetailTextures_ (loaded from the world data -- grass, ground, mountain,
// sand, ... one per material), gated on Option_DetailTexture, the "detail texture" graphics
// option. The placement-zone materials, which index past miniDetailTexturesNumber, have no
// detail texture of their own: the original runs its lava/ice shader for them instead, and
// until that is ported they draw as plain terrain.
cTexture* materialDetailTexture(cTileMap* tileMap, int material)
{
	if(!tileMap || !Option_DetailTexture)
		return nullptr;
	if(material < 0 || material >= cTileMap::miniDetailTexturesNumber)
		return nullptr;
	return tileMap->miniDetailTexture(material).texture;
}

} // namespace

SDLTileMapRenderer::SDLTileMapRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipeline();
	createShadowPipeline();
}

SDLTileMapRenderer::~SDLTileMapRenderer()
{
	if(!device_) return;
	releaseMesh();
	if(whiteTexture_)   SDL_ReleaseGPUTexture(device_, whiteTexture_);
	if(greyTexture_)    SDL_ReleaseGPUTexture(device_, greyTexture_);
	if(sampler_)        SDL_ReleaseGPUSampler(device_, sampler_);
	if(shadowSampler_)  SDL_ReleaseGPUSampler(device_, shadowSampler_);
	if(detailSampler_)  SDL_ReleaseGPUSampler(device_, detailSampler_);
	if(pipelineFill_)   SDL_ReleaseGPUGraphicsPipeline(device_, pipelineFill_);
	if(pipelineLine_)   SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLine_);
	if(pipelineMirror_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineMirror_);
	if(pipelineShadow_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineShadow_);
}

void SDLTileMapRenderer::releaseMesh()
{
	if(!device_) return;
	if(vertexBuffer_){ SDL_ReleaseGPUBuffer(device_, vertexBuffer_); vertexBuffer_ = nullptr; }
	if(indexBuffer_) { SDL_ReleaseGPUBuffer(device_, indexBuffer_);  indexBuffer_  = nullptr; }
	if(colorTexture_){ SDL_ReleaseGPUTexture(device_, colorTexture_); colorTexture_ = nullptr; }
	indexCount_ = 0;
	runs_.clear();
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

	SDL_GPUSamplerCreateInfo ssi = {};
	ssi.min_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mag_filter = SDL_GPU_FILTER_NEAREST;
	ssi.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	ssi.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	ssi.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	shadowSampler_ = SDL_CreateGPUSampler(device_, &ssi);

	// The detail tile repeats across the whole map, so it wraps; and it is filtered
	// trilinearly because its mip chain -- averaging the noise towards flat grey -- is
	// what fades the grain out at distance instead of aliasing it into a shimmer. The
	// original binds sampler_wrap_anisotropic here.
	SDL_GPUSamplerCreateInfo dsi = {};
	dsi.min_filter = SDL_GPU_FILTER_LINEAR;
	dsi.mag_filter = SDL_GPU_FILTER_LINEAR;
	dsi.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	dsi.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	dsi.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	dsi.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	dsi.enable_anisotropy = true;
	dsi.max_anisotropy = 4.f;
	dsi.max_lod = 1000.f;
	detailSampler_ = SDL_CreateGPUSampler(device_, &dsi);

	// 1x1 white, bound instead of the surface colour in wireframe mode so the edges come
	// out white whatever the map's baked colour is. It also stands in for the shadow map
	// when there is none: its .r reads 1.0, the far depth, i.e. everything lit.
	whiteTexture_ = createSolidGPUTexture(device_, 0xFFFFFFFFu);
	// 1x1 mid-grey, the detail texture's neutral: the shader adds `detail - 0.5`.
	greyTexture_  = createSolidGPUTexture(device_, 0xFF808080u);

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + UV + Shadow
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 4;           // surface colour, shadow map, lightmap, detail tile
	fsi.num_uniform_buffers = 1;    // LightColor + LightDirection + ShadeIntensity + params
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
	// Clip near/far, don't clamp -- D3D9's default. Under the reflection camera the parts of
	// the heightfield behind the mirrored eye would otherwise be clamped and rasterized,
	// straddling w==0, into a full-screen smear; D3D9 silently clips them.
	pci.rasterizer_state.enable_depth_clip = true;
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

	// The reflection camera's variant. Its mirror matrix puts the eye below the terrain,
	// so the heightfield is seen from underneath: drawn with no culling it becomes a
	// ceiling over the whole reflection, and everything standing on it -- the palms, the
	// islands -- fails the depth test behind it. cD3DRender::setCamera culls back faces
	// and flips the winding for the reflection (D3DCULL_CW -> D3DCULL_CCW), the mirror
	// having reversed every triangle; culling FRONT against SDL's counter-clockwise front
	// face is that same D3DCULL_CCW, and it takes the underside away.
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_FRONT;
	pipelineMirror_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLTileMapRenderer: tilemap pipeline %s (wireframe %s, reflection %s)\n",
	        pipelineFill_ ? "ready" : "FAILED", pipelineLine_ ? "ready" : "FAILED",
	        pipelineMirror_ ? "ready" : "FAILED");
}

// The terrain as a shadow caster: the same vertex buffer, position only, into depth.
void SDLTileMapRenderer::createShadowPipeline()
{
	if(!device_) return;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_shadow_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // the light's MVP
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_shadow_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);   // no samplers, no uniforms

	if(!vs || !fs){
		fprintf(stderr, "SDLTileMapRenderer: shadow CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// Same buffer, same stride; the caster reads only the position, so the normal is
	// simply not declared.
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(Vertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attr = {};
	attr.location = 0; attr.buffer_slot = 0;
	attr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attr.offset = 0;

	SDL_GPUGraphicsPipelineCreateInfo pci = {};
	pci.vertex_shader = vs;
	pci.fragment_shader = fs;
	pci.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
	pci.vertex_input_state.num_vertex_buffers = 1;
	pci.vertex_input_state.vertex_attributes = &attr;
	pci.vertex_input_state.num_vertex_attributes = 1;
	pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	// D3DRS_SLOPESCALEDEPTHBIAS = 2, as CameraShadowMap::DrawScene sets for the whole
	// caster pass. It matters most here: the terrain both casts and receives, so a slope
	// lit at a grazing angle would otherwise shadow itself.
	pci.rasterizer_state.enable_depth_bias = true;
	pci.rasterizer_state.depth_bias_slope_factor = 2.f;
	pci.rasterizer_state.depth_bias_constant_factor = 0.f;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = true;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.num_color_targets = 0;
	pci.target_info.color_target_descriptions = nullptr;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineShadow_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLTileMapRenderer: terrain caster pipeline %s\n",
	        pipelineShadow_ ? "ready" : "FAILED");
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

	// vMap's MultiRegion paints every fine cell with one of cTileMap's materials, and the
	// D3D tilemap reads it per triangle -- `region.filled(centroid) - 1` -- to bucket that
	// triangle into a per-material index list. Do the same, once for the whole map: the
	// buckets concatenate into one index buffer, and each becomes a MaterialRun that Draw
	// issues with that material's detail texture bound.
	//
	// Our triangles span `step` fine cells where the original's span one, so a triangle
	// straddling a material border takes the material under its centre and the border lands
	// on the mesh grid rather than the region's true edge. Invisible in practice: the
	// detail tile is grain, and the surface colour that carries the border is a texture.
	MultiRegion& region = vMap.region();
	auto materialAt = [&region](int x, int y) -> int {
		const int m = (int)region.filled(x, y) - 1;   // filled() is 1-based; 0 == unpainted
		return (m >= 0 && m < cTileMap::multiRegionLayersNumber) ? m : 0;
	};

	std::vector<std::vector<unsigned short> > buckets(cTileMap::multiRegionLayersNumber);
	for(int gy = 0; gy < ny; ++gy)
		for(int gx = 0; gx < nx; ++gx){
			const int x0 = gx * step,       y0 = gy * step;
			const int x1 = (gx + 1) * step > H - 1 ? H - 1 : (gx + 1) * step;
			const int y1 = (gy + 1) * step > V - 1 ? V - 1 : (gy + 1) * step;

			unsigned short a = (unsigned short)(gy * gw + gx), b = (unsigned short)(a + 1);
			unsigned short c = (unsigned short)(a + gw),       d = (unsigned short)(c + 1);
			// a=(x0,y0) b=(x1,y0) c=(x0,y1) d=(x1,y1). Winding is irrelevant (cull NONE).
			std::vector<unsigned short>& bk0 = buckets[materialAt((x0 + x0 + x1) / 3, (y0 + y1 + y0) / 3)];
			bk0.push_back(a); bk0.push_back(c); bk0.push_back(b);
			std::vector<unsigned short>& bk1 = buckets[materialAt((x1 + x0 + x1) / 3, (y0 + y1 + y1) / 3)];
			bk1.push_back(b); bk1.push_back(c); bk1.push_back(d);
		}

	std::vector<unsigned short> idx;
	idx.reserve((size_t)tcount * 3);
	for(int m = 0; m < (int)buckets.size(); ++m){
		if(buckets[m].empty())
			continue;
		MaterialRun run;
		run.material = m;
		run.first = (int)idx.size();
		run.count = (int)buckets[m].size();
		runs_.push_back(run);
		idx.insert(idx.end(), buckets[m].begin(), buckets[m].end());
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

	fprintf(stderr, "SDLTileMapRenderer: terrain mesh %dx%d step %d (%d verts, %d tris, "
	        "%d material runs)\n", H, V, step, vcount, tcount, (int)runs_.size());
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
bool SDLTileMapRenderer::DrawShadowPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* depth, int size,
                                        Camera* camera, bool clearDepth)
{
	if(!device_ || !pipelineShadow_ || !cmd || !depth || !camera)
		return false;
	// Builds the mesh if the light camera is the first to ask for it this mission. Safe
	// here: ensureMesh opens a copy pass, and no render pass is open yet.
	if(!ensureMesh(cmd))
		return false;

	ShadowVSUniform vsu;
	std::memcpy(vsu.mvp, &camera->matViewProj, sizeof(vsu.mvp));

	SDL_GPUDepthStencilTargetInfo dt = {};
	dt.texture = depth;
	dt.clear_depth = 1.0f;
	dt.load_op = clearDepth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	dt.store_op = SDL_GPU_STOREOP_STORE;
	dt.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	dt.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, nullptr, 0, &dt);
	// The light camera's viewport spans the whole map (SetRenderTarget sized it).
	applyCameraViewport(pass, camera->vp, size, size);

	SDL_BindGPUGraphicsPipeline(pass, pipelineShadow_);
	SDL_PushGPUVertexUniformData(cmd, 0, &vsu, sizeof(vsu));

	SDL_GPUBufferBinding vb = {}; vb.buffer = vertexBuffer_; vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
	SDL_GPUBufferBinding ib = {}; ib.buffer = indexBuffer_; ib.offset = 0;
	SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_DrawGPUIndexedPrimitives(pass, indexCount_, 1, 0, 0, 0);
	SDL_EndGPURenderPass(pass);
	return true;
}

bool SDLTileMapRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* target, SDL_GPUTexture* depth,
                              int screenW, int screenH, bool clear, const float clearColor[4],
                              bool clearDepth, cTileMap* tileMap, Camera* camera, bool wireframe)
{
	if(!camera)
		return false;
	// Fall back to the solid pipeline if a variant failed to build.
	SDL_GPUGraphicsPipeline* pipeline = pipelineFill_;
	if(wireframe && pipelineLine_)
		pipeline = pipelineLine_;
	else if(camera->getAttribute(ATTRCAMERA_REFLECTION) && pipelineMirror_)
		pipeline = pipelineMirror_;
	if(!device_ || !pipeline || !cmd || !target || !depth)
		return false;
	if(!ensureMesh(cmd))
		return false;

	// The camera's own view-projection, unmodified. The depth this pass writes is then
	// in the same space as every other renderer's, so they occlude each other correctly.
	VSUniform vsu;
	std::memcpy(vsu.mvp, &camera->matViewProj, sizeof(vsu.mvp));
	vsu.uv[0] = 0.f; vsu.uv[1] = 0.f;             // UV.xy offset
	vsu.uv[2] = uvScale_[0]; vsu.uv[3] = uvScale_[1];  // UV.zw scale

	// The shadow map, if the light camera filled one earlier this frame. shadowPassRan()
	// is the gate, not the texture: the map outlives the light camera, which cScene
	// detaches whenever shadows are off.
	cSDLRenderDevice* dev = sdlRenderDevice();
	SDL_GPUTexture* shadowMap = nullptr;
	const bool shadow = dev && dev->shadowPassRan() && dev->GetShadowMap();
	if(shadow){
		shadowMap = reinterpret_cast<SDL_GPUTexture*>(dev->GetShadowMap()->GetDDSurface(0));
		// mShadow, as ShaderSkin.inl builds it: world -> light clip -> map texture coords.
		const Mat4f mShadow = dev->shadowMatViewProj() * dev->shadowMatBias();
		std::memcpy(vsu.shadow, &mShadow, sizeof(vsu.shadow));
	}
	else
		std::memset(vsu.shadow, 0, sizeof(vsu.shadow));

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

	// vShade: what a fully shadowed pixel is multiplied by. Per mission, from the
	// Environment's shadow intensity.
	const Color4f shade = (shadow && tileMap && tileMap->scene())
	                    ? tileMap->scene()->GetShadowIntensity() : Color4f(1.f, 1.f, 1.f, 1.f);
	fsu.shade[0] = shade.r; fsu.shade[1] = shade.g; fsu.shade[2] = shade.b; fsu.shade[3] = shade.a;
	fsu.params[0] = shadow ? 1.f : 0.f;
	// FILTER_SHADOW, a static shader define in the original; a uniform here.
	fsu.params[1] = (shadow && Option_filterShadow) ? 1.f : 0.f;
	fsu.params[2] = fsu.params[3] = 0.f;

	// The terrain lightmap and its fPlanarNode, both from the device: cScene::AddPlanarCamera
	// set the transform to the same world box the lightmap camera rendered.
	cTexture* lightMapTexture = dev ? dev->GetLightMap() : nullptr;
	SDL_GPUTexture* lightMap = lightMapTexture
	                         ? reinterpret_cast<SDL_GPUTexture*>(lightMapTexture->GetDDSurface(0))
	                         : nullptr;
	fsu.lightMapParams[0] = lightMap ? 1.f : 0.f;
	fsu.lightMapParams[1] = fsu.lightMapParams[2] = fsu.lightMapParams[3] = 0.f;
	if(dev){
		const Vect4f& pn = dev->planarTransform();
		vsu.planarNode[0] = pn.x; vsu.planarNode[1] = pn.y;
		vsu.planarNode[2] = pn.z; vsu.planarNode[3] = pn.w;
	}
	else{
		vsu.planarNode[0] = vsu.planarNode[1] = 0.f;
		vsu.planarNode[2] = vsu.planarNode[3] = 1.f;
	}

	// Distance fog. The plane folds this camera's view matrix into the linear factor, so the
	// reflection camera fogs by its own depth. It comes back (0,0,0,1) when fog is off --
	// factor 1, and the shader's lerp is then the identity.
	const Vect4f fogPlane = dev ? dev->fogPlane(camera) : Vect4f(0.f, 0.f, 0.f, 1.f);
	vsu.fogPlane[0] = fogPlane.x; vsu.fogPlane[1] = fogPlane.y;
	vsu.fogPlane[2] = fogPlane.z; vsu.fogPlane[3] = fogPlane.w;
	const Color4f fog = dev ? dev->fogColor() : Color4f(0.f, 0.f, 0.f, 0.f);
	fsu.fogColor[0] = fog.r; fsu.fogColor[1] = fog.g; fsu.fogColor[2] = fog.b; fsu.fogColor[3] = fog.a;

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

	SDL_GPUBufferBinding vb = {}; vb.buffer = vertexBuffer_; vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
	SDL_GPUBufferBinding ib = {}; ib.buffer = indexBuffer_; ib.offset = 0;
	SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_GPUTextureSamplerBinding ts[4] = {};
	ts[0].texture = (wireframe && whiteTexture_) ? whiteTexture_ : colorTexture_;
	ts[0].sampler = sampler_;
	// Slots 1..3 must always carry a texture, even with the shadow, the lightmap or the
	// detail layer disabled; the shader gates on ShadowParams.x / LightMapParams.x /
	// DetailParams.x rather than on the binding, so a 1x1 stands in and is never read.
	ts[1].texture = shadowMap ? shadowMap : whiteTexture_;
	ts[1].sampler = shadowSampler_;
	ts[2].texture = lightMap ? lightMap : whiteTexture_;
	ts[2].sampler = sampler_;   // linear + clamp: the lightmap is 256x256 over the whole box
	ts[3].sampler = detailSampler_;   // .texture varies per material, below

	// The original changes material between tiles -- cTileMap::setMaterial binds that
	// material's detail texture and its mulMiniTexture, then draws the tile's index list
	// for it. Our index buffer is grouped by material for the whole map, so the same
	// rebinding happens once per run instead of once per tile.
	const float resolution = tileMap ? (float)tileMap->miniDetailTextureResolution() : 0.f;
	for(size_t i = 0; i < runs_.size(); ++i){
		const MaterialRun& run = runs_[i];

		cTexture* detail = wireframe ? nullptr : materialDetailTexture(tileMap, run.material);
		SDL_GPUTexture* detailTex = nullptr;
		// frameNumber() first: GetDDSurface indexes BitMap unchecked, so a cTexture whose
		// upload never happened would be read out of bounds.
		if(detail && detail->frameNumber() >= 1 && detail->GetWidth() > 0 && detail->GetHeight() > 0)
			detailTex = reinterpret_cast<SDL_GPUTexture*>(detail->GetDDSurface(0));

		if(detailTex){
			// SetMiniTextureSize: mul = (resolution/width, resolution/height), so the tile
			// repeats every width/resolution world cells.
			vsu.miniTexture[0] = resolution / (float)detail->GetWidth();
			vsu.miniTexture[1] = resolution / (float)detail->GetHeight();
			fsu.detailParams[0] = 1.f;
		}
		else{
			vsu.miniTexture[0] = vsu.miniTexture[1] = 0.f;
			fsu.detailParams[0] = 0.f;
		}
		vsu.miniTexture[2] = vsu.miniTexture[3] = 0.f;
		fsu.detailParams[1] = fsu.detailParams[2] = fsu.detailParams[3] = 0.f;

		SDL_PushGPUVertexUniformData(cmd, 0, &vsu, sizeof(vsu));
		SDL_PushGPUFragmentUniformData(cmd, 0, &fsu, sizeof(fsu));

		ts[3].texture = detailTex ? detailTex : greyTexture_;
		SDL_BindGPUFragmentSamplers(pass, 0, ts, 4);

		SDL_DrawGPUIndexedPrimitives(pass, run.count, 1, run.first, 0, 0);
	}

	SDL_EndGPURenderPass(pass);
	return true;
}

