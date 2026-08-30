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
#include "IVisGenericInternal.h" // graphRnd, for the lava noise volume
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
                   float lightMapParams[4]; float detailParams[4]; float fogColor[4];
                   float fogOfWarColor[4]; };
// tilemap_shadow.vert.hlsl's whole cbuffer: the light camera's view-projection.
struct ShadowVSUniform { float mvp[16]; };
// tilemap_lava.{vert,frag}.hlsl's cbuffers. The VS carries the transform plus the two
// terms lava keeps (fog-of-war lightmap uv, distance fog); the FS carries the material's
// colours and scales, the animation time, and the two fog colours.
struct LavaVSUniform { float mvp[16]; float planarNode[4]; float fogPlane[4]; };
struct LavaFSUniform { float lavaColor[4]; float lavaAmbient[4]; float params[4];
                       float fogColor[4]; float fogOfWarColor[4]; float lightMapParams[4]; };
// tilemap_ice.{vert,frag}.hlsl's cbuffers. The VS carries the transform, the reflection
// camera's mirror matrix, the fog-of-war lightmap uv, distance fog and the snow/bump UV
// scales; the FS carries the snow tint, the two fog colours and the reflection / fog-of-war
// flags (params.x = a reflection target exists, params.y = fog of war on).
struct IceVSUniform { float mvp[16]; float mirrorVP[16]; float planarNode[4]; float fogPlane[4];
                      float scaleBumpSnow[4]; };
struct IceFSUniform { float snowColor[4]; float fogColor[4]; float fogOfWarColor[4]; float params[4]; };

// The 64^3 random noise volume the lava fBm samples through -- the original's
// CreateVolumeRand(64): an L8 cube of graphRnd() bytes. R8_UNORM here; the shader reads .x.
const int VOLUME_SIZE = 64;

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
// detail texture of their own: the original runs its lava/ice shader for them instead. The
// lava run is handled in Draw with its own pipeline; ice still draws as plain terrain here.
cTexture* materialDetailTexture(cTileMap* tileMap, int material)
{
	if(!tileMap || !Option_DetailTexture)
		return nullptr;
	if(material < 0 || material >= cTileMap::miniDetailTexturesNumber)
		return nullptr;
	return tileMap->miniDetailTexture(material).texture;
}

// The material vMap's MultiRegion paints a fine cell with. filled() is 1-based; 0 (unpainted)
// and out-of-range both fall back to material 0, as the D3D tilemap's `filled - 1` did.
int regionMaterialAt(MultiRegion& region, int x, int y)
{
	const int m = (int)region.filled(x, y) - 1;
	return (m >= 0 && m < cTileMap::multiRegionLayersNumber) ? m : 0;
}

} // namespace

SDLTileMapRenderer::SDLTileMapRenderer(SDL_GPUDevice* device, SDL_Window* window)
	: device_(device), window_(window)
{
	createPipeline();
	createShadowPipeline();
	createLavaPipeline();
	createIcePipeline();
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
	if(pipelineLava_)       SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLava_);
	if(pipelineLavaMirror_) SDL_ReleaseGPUGraphicsPipeline(device_, pipelineLavaMirror_);
	if(volumeTexture_)  SDL_ReleaseGPUTexture(device_, volumeTexture_);
	if(volumeSampler_)  SDL_ReleaseGPUSampler(device_, volumeSampler_);
	if(pipelineIce_)        SDL_ReleaseGPUGraphicsPipeline(device_, pipelineIce_);
	if(pipelineIceMirror_)  SDL_ReleaseGPUGraphicsPipeline(device_, pipelineIceMirror_);
}

void SDLTileMapRenderer::releaseMesh()
{
	if(!device_) return;
	if(vertexBuffer_){ SDL_ReleaseGPUBuffer(device_, vertexBuffer_); vertexBuffer_ = nullptr; }
	if(indexBuffer_) { SDL_ReleaseGPUBuffer(device_, indexBuffer_);  indexBuffer_  = nullptr; }
	if(colorTexture_){ SDL_ReleaseGPUTexture(device_, colorTexture_); colorTexture_ = nullptr; }
	if(bumpTexture_) { SDL_ReleaseGPUTexture(device_, bumpTexture_);  bumpTexture_  = nullptr; }
	indexCount_ = 0;
	runs_.clear();
	verts_.clear();
	quadMat_.clear();
	gw_ = gh_ = nx_ = ny_ = step_ = 0;
	texW_ = texH_ = texStep_ = 0;
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
	fsi.num_samplers = 5;           // surface colour, shadow map, lightmap, detail tile, bump
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

// The placement-zone LAVA material's pipeline pair, its noise volume and its wrap sampler.
// Ported from ShaderSceneWaterLava (Render/shader/ShaderWater.inl): CreateVolumeRand(64) is
// the L8 cube of graphRnd() bytes, sampler_wrap_linear the stage-0 sampler.
void SDLTileMapRenderer::createLavaPipeline()
{
	if(!device_ || !window_) return;

	// The noise volume: 64^3 random bytes, R8_UNORM (the shader reads .x). One graphRnd()
	// byte per texel, exactly as CreateVolumeRand filled its D3DFMT_L8 box.
	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_3D;
	ti.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = VOLUME_SIZE; ti.height = VOLUME_SIZE;
	ti.layer_count_or_depth = VOLUME_SIZE; ti.num_levels = 1;
	volumeTexture_ = SDL_CreateGPUTexture(device_, &ti);
	if(volumeTexture_){
		const Uint32 bytes = (Uint32)VOLUME_SIZE * VOLUME_SIZE * VOLUME_SIZE;
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = bytes;
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
			unsigned char* p = (unsigned char*)SDL_MapGPUTransferBuffer(device_, tb, false);
			for(Uint32 i = 0; i < bytes; ++i)
				p[i] = (unsigned char)(graphRnd() & 255);
			SDL_UnmapGPUTransferBuffer(device_, tb);

			SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
			SDL_GPUTextureTransferInfo src = {};
			src.transfer_buffer = tb; src.offset = 0;
			src.pixels_per_row = VOLUME_SIZE; src.rows_per_layer = VOLUME_SIZE;
			SDL_GPUTextureRegion dst = {};
			dst.texture = volumeTexture_;
			dst.w = VOLUME_SIZE; dst.h = VOLUME_SIZE; dst.d = VOLUME_SIZE;
			SDL_UploadToGPUTexture(copy, &src, &dst, false);
			SDL_EndGPUCopyPass(copy);
			SDL_SubmitGPUCommandBuffer(cb);
			SDL_ReleaseGPUTransferBuffer(device_, tb);
		}
	}

	SDL_GPUSamplerCreateInfo vsmp = {};
	vsmp.min_filter = SDL_GPU_FILTER_LINEAR;
	vsmp.mag_filter = SDL_GPU_FILTER_LINEAR;
	vsmp.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	vsmp.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	vsmp.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	vsmp.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	volumeSampler_ = SDL_CreateGPUSampler(device_, &vsmp);

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_lava_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + PlanarNode + FogPlane
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_lava_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 3;           // noise volume, ground texture, lightmap (fog of war)
	fsi.num_uniform_buffers = 1;    // colours + scales + time + fog colours
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLTileMapRenderer: lava CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// Same vertex layout as the terrain (position @0, normal @12), same opaque depth state.
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(Vertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[2] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[1].offset = 12;

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
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.rasterizer_state.enable_depth_clip = true;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = true;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineLava_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	// The reflection camera's variant: cull FRONT, as the terrain's pipelineMirror_ does, so
	// lava tiles do not roof the reflection (see the note on pipelineMirror_).
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_FRONT;
	pipelineLavaMirror_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLTileMapRenderer: lava pipeline %s (reflection %s), noise volume %s\n",
	        pipelineLava_ ? "ready" : "FAILED", pipelineLavaMirror_ ? "ready" : "FAILED",
	        volumeTexture_ ? "ready" : "FAILED");
}

// The placement-zone ICE material's pipeline pair. Opaque, like the terrain: the original's
// cTileMap::setMaterial drove ShaderSceneWaterIce::beginDraw with a null alpha texture, which
// keeps Z-write on and does not blend. Its samplers, snow/bump textures and the reflection
// target are all bound per frame in Draw, so this only compiles the shaders and builds the
// pipeline. Ported from Render/shader/Water/water_ice.{vsl,psl}.
void SDLTileMapRenderer::createIcePipeline()
{
	if(!device_ || !window_) return;

	SDL_GPUShaderCreateInfo vsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_ice_vert));
	vsi.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vsi.num_uniform_buffers = 1;    // MVP + MirrorVP + PlanarNode + FogPlane + ScaleBumpSnow
	SDL_GPUShader* vs = SDL_CreateGPUShader(device_, &vsi);

	SDL_GPUShaderCreateInfo fsi = vista::shaderCreateInfo(VISTA_SHADER(tilemap_ice_frag));
	fsi.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fsi.num_samplers = 4;           // snow, bump, reflection, lightmap (fog of war)
	fsi.num_uniform_buffers = 1;    // snow tint + fog colours + flags
	SDL_GPUShader* fs = SDL_CreateGPUShader(device_, &fsi);

	if(!vs || !fs){
		fprintf(stderr, "SDLTileMapRenderer: ice CreateGPUShader failed: %s\n", SDL_GetError());
		if(vs) SDL_ReleaseGPUShader(device_, vs);
		if(fs) SDL_ReleaseGPUShader(device_, fs);
		return;
	}

	// Same vertex layout as the terrain (position @0, normal @12), same opaque depth state.
	SDL_GPUVertexBufferDescription vbDesc = {};
	vbDesc.slot = 0;
	vbDesc.pitch = sizeof(Vertex);
	vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute attrs[2] = {};
	attrs[0].location = 0; attrs[0].buffer_slot = 0; attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[0].offset = 0;
	attrs[1].location = 1; attrs[1].buffer_slot = 0; attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; attrs[1].offset = 12;

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
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	pci.rasterizer_state.enable_depth_clip = true;
	pci.depth_stencil_state.enable_depth_test = true;
	pci.depth_stencil_state.enable_depth_write = true;
	pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	pci.target_info.color_target_descriptions = &colorTarget;
	pci.target_info.num_color_targets = 1;
	pci.target_info.has_depth_stencil_target = true;
	pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	pipelineIce_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	// The reflection camera's variant: cull FRONT, as the terrain's pipelineMirror_ does.
	pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_FRONT;
	pipelineIceMirror_ = SDL_CreateGPUGraphicsPipeline(device_, &pci);

	SDL_ReleaseGPUShader(device_, vs);
	SDL_ReleaseGPUShader(device_, fs);

	fprintf(stderr, "SDLTileMapRenderer: ice pipeline %s (reflection %s)\n",
	        pipelineIce_ ? "ready" : "FAILED", pipelineIceMirror_ ? "ready" : "FAILED");
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

	// Remembered so applyMapUpdates can map a dirty rect of fine cells onto texels.
	// texStep_ == 0 marks the fallback texture, which has no cells to re-bake.
	texW_ = tw; texH_ = th; texStep_ = baked ? step : 0;

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

// One texel of the slope (bump) map per colour texel: (z0-zx, z0-zy) as signed bytes.
// GetNormalArrayShort's fixed-point maths in float: getTileZ hands it z<<8 (world z *
// 256), sampled every `step` cells, and it shifts the deltas down by 2+log2(step) --
// i.e. dz_world * 64 / step -- before clamping to a byte. The shader rebuilds the
// normal as normalize(bump.xy, 0.5), so the scale sets the shading contrast; keep it.
void SDLTileMapRenderer::bakeBumpRect(signed char* out, int px0, int py0, int w, int h) const
{
	const int H = (int)vMap.H_SIZE, V = (int)vMap.V_SIZE;
	const float scale = 64.f / (float)texStep_;
	for(int ty = 0; ty < h; ++ty){
		const int y = (py0 + ty) * texStep_;
		const int y1 = y + texStep_ > V - 1 ? V - 1 : y + texStep_;
		for(int tx = 0; tx < w; ++tx){
			const int x = (px0 + tx) * texStep_;
			const int x1 = x + texStep_ > H - 1 ? H - 1 : x + texStep_;
			const float z0 = vMap.getZf(x, y);
			int dzx = (int)((z0 - vMap.getZf(x1, y)) * scale);
			int dzy = (int)((z0 - vMap.getZf(x, y1)) * scale);
			if(dzx < -128) dzx = -128; else if(dzx > 127) dzx = 127;
			if(dzy < -128) dzy = -128; else if(dzy > 127) dzy = 127;
			*out++ = (signed char)dzx;
			*out++ = (signed char)dzy;
		}
	}
}

bool SDLTileMapRenderer::buildBumpTexture(SDL_GPUCommandBuffer* cmd)
{
	if(texStep_ <= 0)
		return true;   // fallback colour texture: no cells to bake slopes from

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_R8G8_SNORM;   // the original's V8U8
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	ti.width = (Uint32)texW_; ti.height = (Uint32)texH_;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	bumpTexture_ = SDL_CreateGPUTexture(device_, &ti);
	if(!bumpTexture_) return false;

	std::vector<signed char> texels((size_t)texW_ * texH_ * 2);
	bakeBumpRect(texels.data(), 0, 0, texW_, texH_);

	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)texels.size();
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return false;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	SDL_memcpy(map, texels.data(), texels.size());
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src = {};
	src.transfer_buffer = tb; src.offset = 0;
	src.pixels_per_row = (Uint32)texW_; src.rows_per_layer = (Uint32)texH_;
	SDL_GPUTextureRegion dst = {};
	dst.texture = bumpTexture_; dst.w = (Uint32)texW_; dst.h = (Uint32)texH_; dst.d = 1;
	SDL_UploadToGPUTexture(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_ReleaseGPUTransferBuffer(device_, tb);
	return true;
}

// One grid vertex, sampled from vMap.
//
// The vMap<->world frame: vMap is H_SIZE x V_SIZE fine cells, world XY == fine
// cell, and the world Z of the surface is getZf(x,y) = getAlt/vx_fraction. This is
// NOT the D3D tilemap's round(getZ*64) fixed point (which its world matrix undoes);
// getZf gives world units that line up with the camera directly.
void SDLTileMapRenderer::computeVertex(Vertex& v, int gx, int gy) const
{
	int x = gx * step_; if(x > (int)vMap.H_SIZE - 1) x = (int)vMap.H_SIZE - 1;
	int y = gy * step_; if(y > (int)vMap.V_SIZE - 1) y = (int)vMap.V_SIZE - 1;
	v.x = (float)x; v.y = (float)y; v.z = vMap.getZf(x, y);
	Vect3f nrm; vMap.getNormal(x, y, nrm);
	v.nx = nrm.x; v.ny = nrm.y; v.nz = nrm.z;
}

// quadMat_ -> the index buffer, grouped into one contiguous run per material: the
// buckets concatenate in material order and each becomes a MaterialRun that Draw
// issues with that material's detail texture bound. Rebuilt whole when terramorphing
// repaints a cell's material -- every triangle appears exactly once whatever its
// bucket, so the buffer's size never changes.
void SDLTileMapRenderer::buildIndexData(std::vector<unsigned short>& idx)
{
	runs_.clear();
	std::vector<std::vector<unsigned short> > buckets(cTileMap::multiRegionLayersNumber);
	for(int gy = 0; gy < ny_; ++gy)
		for(int gx = 0; gx < nx_; ++gx){
			unsigned short a = (unsigned short)(gy * gw_ + gx), b = (unsigned short)(a + 1);
			unsigned short c = (unsigned short)(a + gw_),       d = (unsigned short)(c + 1);
			// a=(x0,y0) b=(x1,y0) c=(x0,y1) d=(x1,y1). Winding is irrelevant (cull NONE).
			std::vector<unsigned short>& bk0 = buckets[quadMat_[2 * ((size_t)gy * nx_ + gx)]];
			bk0.push_back(a); bk0.push_back(c); bk0.push_back(b);
			std::vector<unsigned short>& bk1 = buckets[quadMat_[2 * ((size_t)gy * nx_ + gx) + 1]];
			bk1.push_back(b); bk1.push_back(c); bk1.push_back(d);
		}

	idx.clear();
	idx.reserve((size_t)nx_ * ny_ * 6);
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

	step_ = step;
	nx_ = H / step; ny_ = V / step;   // grid quads per axis
	gw_ = nx_ + 1;  gh_ = ny_ + 1;    // vertices per axis
	const int vcount = gw_ * gh_;
	const int tcount = nx_ * ny_ * 2;   // triangles

	verts_.resize((size_t)vcount);
	for(int gy = 0; gy < gh_; ++gy)
		for(int gx = 0; gx < gw_; ++gx)
			computeVertex(verts_[(size_t)gy * gw_ + gx], gx, gy);

	// vMap's MultiRegion paints every fine cell with one of cTileMap's materials, and the
	// D3D tilemap reads it per triangle -- `region.filled(centroid) - 1` -- to bucket that
	// triangle into a per-material index list. Do the same, once for the whole map, into
	// quadMat_ (kept: applyMapUpdates diffs against it when the region repaints).
	//
	// Our triangles span `step` fine cells where the original's span one, so a triangle
	// straddling a material border takes the material under its centre and the border lands
	// on the mesh grid rather than the region's true edge. Invisible in practice: the
	// detail tile is grain, and the surface colour that carries the border is a texture.
	MultiRegion& region = vMap.region();
	quadMat_.assign((size_t)nx_ * ny_ * 2, 0);
	for(int gy = 0; gy < ny_; ++gy)
		for(int gx = 0; gx < nx_; ++gx){
			const int x0 = gx * step,       y0 = gy * step;
			const int x1 = (gx + 1) * step > H - 1 ? H - 1 : (gx + 1) * step;
			const int y1 = (gy + 1) * step > V - 1 ? V - 1 : (gy + 1) * step;
			quadMat_[2 * ((size_t)gy * nx_ + gx)]     = (unsigned char)regionMaterialAt(region, (x0 + x0 + x1) / 3, (y0 + y1 + y0) / 3);
			quadMat_[2 * ((size_t)gy * nx_ + gx) + 1] = (unsigned char)regionMaterialAt(region, (x1 + x0 + x1) / 3, (y0 + y1 + y1) / 3);
		}

	std::vector<unsigned short> idx;
	buildIndexData(idx);

	const Uint32 vbytes = (Uint32)(verts_.size() * sizeof(Vertex));
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
	SDL_memcpy(map, verts_.data(), vbytes);
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
	if(!buildBumpTexture(cmd)){ releaseMesh(); return false; }

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
		// The build has just read vMap as it stands; the flags raised for the initial
		// full-map updateMap (cTileMap's constructor) are already satisfied by it.
		clearTileUpdateFlags(tileMap);
	}
	else
		// Terramorphing: fold vMap's runtime edits into the mesh. First caller of the
		// frame (the light camera, when shadows are on) consumes the flags; safe here
		// because both callers run before opening their render pass.
		applyMapUpdates(cmd, tileMap);
	return true;
}

// Drop every tile's pending update flags. Used right after a fresh build: the mesh was
// sampled from current vMap data, so anything flagged before this point is already in it.
void SDLTileMapRenderer::clearTileUpdateFlags(cTileMap* tileMap)
{
	if(!tileMap)
		return;
	for(int j = 0; j < tileMap->tileNumber().y; ++j)
		for(int i = 0; i < tileMap->tileNumber().x; ++i)
			tileMap->GetTile(i, j).clearAttribute(ATTRTILE_UPDATE_VERTEX | ATTRTILE_UPDATE_TEXTURE);
}

// Terramorphing. vMap reports every runtime edit (zero layer spread, explosions, unit
// tracks) through cTileMap::updateMap, and BuildRegionPoint converts the rects into
// per-tile ATTRTILE_UPDATE_VERTEX / ATTRTILE_UPDATE_TEXTURE flags each PreDraw. The D3D
// tilemap consumed them per tile (CalcVertex / CalcTexture); our mesh spans the whole
// map, so the dirty tiles union into one rect whose vertices are recomputed in the CPU
// mirror and re-uploaded as a contiguous row span, and whose span of the colour texture
// is re-baked from vMap.clrBuf. If the repaint moved a material border (the region map
// changed), the index buffer's material runs are rebuilt too.
void SDLTileMapRenderer::applyMapUpdates(SDL_GPUCommandBuffer* cmd, cTileMap* tileMap)
{
	if(!tileMap || verts_.empty() || !vertexBuffer_)
		return;

	// Union the dirty tiles into one rect per flag, in fine cells (inclusive).
	bool vertexDirty = false, textureDirty = false;
	int vx0 = 0, vy0 = 0, vx1 = 0, vy1 = 0;
	int tx0 = 0, ty0 = 0, tx1 = 0, ty1 = 0;
	const Vect2i tileN = tileMap->tileNumber(), tileS = tileMap->tileSize();
	for(int j = 0; j < tileN.y; ++j)
		for(int i = 0; i < tileN.x; ++i){
			sTile& tile = tileMap->GetTile(i, j);
			if(tile.getAttribute(ATTRTILE_UPDATE_VERTEX)){
				tile.clearAttribute(ATTRTILE_UPDATE_VERTEX);
				const int x0 = i*tileS.x, y0 = j*tileS.y;
				const int x1 = x0 + tileS.x - 1, y1 = y0 + tileS.y - 1;
				if(!vertexDirty){ vx0 = x0; vy0 = y0; vx1 = x1; vy1 = y1; vertexDirty = true; }
				else{
					if(x0 < vx0) vx0 = x0;  if(y0 < vy0) vy0 = y0;
					if(x1 > vx1) vx1 = x1;  if(y1 > vy1) vy1 = y1;
				}
			}
			if(tile.getAttribute(ATTRTILE_UPDATE_TEXTURE)){
				tile.clearAttribute(ATTRTILE_UPDATE_TEXTURE);
				const int x0 = i*tileS.x, y0 = j*tileS.y;
				const int x1 = x0 + tileS.x - 1, y1 = y0 + tileS.y - 1;
				if(!textureDirty){ tx0 = x0; ty0 = y0; tx1 = x1; ty1 = y1; textureDirty = true; }
				else{
					if(x0 < tx0) tx0 = x0;  if(y0 < ty0) ty0 = y0;
					if(x1 > tx1) tx1 = x1;  if(y1 > ty1) ty1 = y1;
				}
			}
		}
	if(!vertexDirty && !textureDirty)
		return;

	if(vertexDirty){
		// One grid cell of padding on each side: a vertex's normal reads its +step
		// neighbour, so verts just outside the rect see the changed heights too.
		auto gridClamp = [](int v, int hi){ return v < 0 ? 0 : (v > hi ? hi : v); };
		const int gx0 = gridClamp(vx0 / step_ - 1, gw_ - 1), gx1 = gridClamp(vx1 / step_ + 1, gw_ - 1);
		const int gy0 = gridClamp(vy0 / step_ - 1, gh_ - 1), gy1 = gridClamp(vy1 / step_ + 1, gh_ - 1);
		for(int gy = gy0; gy <= gy1; ++gy)
			for(int gx = gx0; gx <= gx1; ++gx)
				computeVertex(verts_[(size_t)gy * gw_ + gx], gx, gy);

		// Whole rows gy0..gy1: contiguous in the buffer, so one upload covers the rect.
		// cycle=false -- a partial write must land in the live buffer, not a fresh one.
		const Uint32 off   = (Uint32)((size_t)gy0 * gw_ * sizeof(Vertex));
		const Uint32 bytes = (Uint32)((size_t)(gy1 - gy0 + 1) * gw_ * sizeof(Vertex));
		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = bytes;
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
			void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
			SDL_memcpy(map, verts_.data() + (size_t)gy0 * gw_, bytes);
			SDL_UnmapGPUTransferBuffer(device_, tb);

			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
			SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
			SDL_GPUBufferRegion dst = {}; dst.buffer = vertexBuffer_; dst.offset = off; dst.size = bytes;
			SDL_UploadToGPUBuffer(copy, &src, &dst, false);
			SDL_EndGPUCopyPass(copy);
			SDL_ReleaseGPUTransferBuffer(device_, tb);
		}

		// Did the repaint move a material border? The zero layer's spread does: it paints
		// its cells with the player's zeroplast material. Diff against quadMat_ and rebuild
		// the runs only then -- pure height changes never pay for it. Locked, as the
		// original's UpdateLine locked around its region reads: the logic thread edits the
		// MultiRegion while we run.
		MultiRegion& region = vMap.region();
		const int H = (int)vMap.H_SIZE, V = (int)vMap.V_SIZE;
		const int qx0 = gridClamp(vx0 / step_ - 1, nx_ - 1), qx1 = gridClamp(vx1 / step_ + 1, nx_ - 1);
		const int qy0 = gridClamp(vy0 / step_ - 1, ny_ - 1), qy1 = gridClamp(vy1 / step_ + 1, ny_ - 1);
		bool materialChanged = false;
		region.lock();
		for(int gy = qy0; gy <= qy1; ++gy)
			for(int gx = qx0; gx <= qx1; ++gx){
				const int x0 = gx * step_,       y0 = gy * step_;
				const int x1 = (gx + 1) * step_ > H - 1 ? H - 1 : (gx + 1) * step_;
				const int y1 = (gy + 1) * step_ > V - 1 ? V - 1 : (gy + 1) * step_;
				const unsigned char m0 = (unsigned char)regionMaterialAt(region, (x0 + x0 + x1) / 3, (y0 + y1 + y0) / 3);
				const unsigned char m1 = (unsigned char)regionMaterialAt(region, (x1 + x0 + x1) / 3, (y0 + y1 + y1) / 3);
				unsigned char& s0 = quadMat_[2 * ((size_t)gy * nx_ + gx)];
				unsigned char& s1 = quadMat_[2 * ((size_t)gy * nx_ + gx) + 1];
				if(m0 != s0){ s0 = m0; materialChanged = true; }
				if(m1 != s1){ s1 = m1; materialChanged = true; }
			}
		region.unlock();

		if(materialChanged){
			std::vector<unsigned short> idx;
			buildIndexData(idx);
			const Uint32 ibytes = (Uint32)(idx.size() * sizeof(unsigned short));
			SDL_GPUTransferBufferCreateInfo itbi = {};
			itbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
			itbi.size = ibytes;
			if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &itbi)){
				void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
				SDL_memcpy(map, idx.data(), ibytes);
				SDL_UnmapGPUTransferBuffer(device_, tb);

				SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
				SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
				SDL_GPUBufferRegion dst = {}; dst.buffer = indexBuffer_; dst.offset = 0; dst.size = ibytes;
				SDL_UploadToGPUBuffer(copy, &src, &dst, false);
				SDL_EndGPUCopyPass(copy);
				SDL_ReleaseGPUTransferBuffer(device_, tb);
			}
		}

		// The slope (bump) map follows the heights. A texel reads z at x and x+step, so
		// changed cells reach one texel back on the min side; pad and re-bake the rect.
		if(bumpTexture_ && texStep_ > 0){
			auto texClamp = [](int v, int hi){ return v < 0 ? 0 : (v > hi ? hi : v); };
			const int px0 = texClamp(vx0 / texStep_ - 1, texW_ - 1), px1 = texClamp(vx1 / texStep_, texW_ - 1);
			const int py0 = texClamp(vy0 / texStep_ - 1, texH_ - 1), py1 = texClamp(vy1 / texStep_, texH_ - 1);
			const int w = px1 - px0 + 1, h = py1 - py0 + 1;

			std::vector<signed char> texels((size_t)w * h * 2);
			bakeBumpRect(texels.data(), px0, py0, w, h);

			SDL_GPUTransferBufferCreateInfo tbi = {};
			tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
			tbi.size = (Uint32)texels.size();
			if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
				void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
				SDL_memcpy(map, texels.data(), texels.size());
				SDL_UnmapGPUTransferBuffer(device_, tb);

				SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
				SDL_GPUTextureTransferInfo src = {};
				src.transfer_buffer = tb; src.offset = 0;
				src.pixels_per_row = (Uint32)w; src.rows_per_layer = (Uint32)h;
				SDL_GPUTextureRegion dst = {};
				dst.texture = bumpTexture_;
				dst.x = (Uint32)px0; dst.y = (Uint32)py0;
				dst.w = (Uint32)w; dst.h = (Uint32)h; dst.d = 1;
				SDL_UploadToGPUTexture(copy, &src, &dst, false);   // partial: don't cycle
				SDL_EndGPUCopyPass(copy);
				SDL_ReleaseGPUTransferBuffer(device_, tb);
			}
		}
	}

	if(textureDirty && colorTexture_ && texStep_ > 0){
		// Fine cells -> texel rect (inclusive), then re-bake it straight from vMap.clrBuf.
		auto texClamp = [](int v, int hi){ return v < 0 ? 0 : (v > hi ? hi : v); };
		const int px0 = texClamp(tx0 / texStep_, texW_ - 1), px1 = texClamp(tx1 / texStep_, texW_ - 1);
		const int py0 = texClamp(ty0 / texStep_, texH_ - 1), py1 = texClamp(ty1 / texStep_, texH_ - 1);
		const int w = px1 - px0 + 1, h = py1 - py0 + 1;

		std::vector<unsigned char> pixels((size_t)w * h * 4);
		vMap.getTileColor32Layer(pixels.data(), (DWORD)(w * 4),
		                         px0 * texStep_, py0 * texStep_,
		                         (px0 + w) * texStep_, (py0 + h) * texStep_, texStep_);

		SDL_GPUTransferBufferCreateInfo tbi = {};
		tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tbi.size = (Uint32)pixels.size();
		if(SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi)){
			void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
			SDL_memcpy(map, pixels.data(), pixels.size());
			SDL_UnmapGPUTransferBuffer(device_, tb);

			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
			SDL_GPUTextureTransferInfo src = {};
			src.transfer_buffer = tb; src.offset = 0;
			src.pixels_per_row = (Uint32)w; src.rows_per_layer = (Uint32)h;
			SDL_GPUTextureRegion dst = {};
			dst.texture = colorTexture_;
			dst.x = (Uint32)px0; dst.y = (Uint32)py0;
			dst.w = (Uint32)w; dst.h = (Uint32)h; dst.d = 1;
			SDL_UploadToGPUTexture(copy, &src, &dst, false);   // partial: don't cycle
			SDL_EndGPUCopyPass(copy);
			SDL_ReleaseGPUTransferBuffer(device_, tb);
		}
	}
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
	// The fog of war rides that same lightmap, in its alpha channel, so it is on only if the
	// map is there to read: cScene::Draw turns it on for the scene, FogOfWar::Draw fills the
	// alpha under the light camera. Its colour is FogOfWar's own, carried on the device as
	// cD3DRender carried fog_of_war_color.
	const bool fogOfWar = dev && dev->fogOfWar() && lightMap;
	fsu.lightMapParams[1] = fogOfWar ? 1.f : 0.f;
	fsu.lightMapParams[2] = fsu.lightMapParams[3] = 0.f;
	const Color4f fow = dev ? dev->fogOfWarColor() : Color4f();
	fsu.fogOfWarColor[0] = fow.r; fsu.fogOfWarColor[1] = fow.g;
	fsu.fogOfWarColor[2] = fow.b; fsu.fogOfWarColor[3] = fow.a;
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
	if(!pass){
		fprintf(stderr, "SDLTileMapRenderer: terrain render pass failed for %dx%d: %s\n",
		        screenW, screenH, SDL_GetError());
		return false;
	}
	applyCameraViewport(pass, camera->vp, screenW, screenH);

	SDL_BindGPUGraphicsPipeline(pass, pipeline);

	SDL_GPUBufferBinding vb = {}; vb.buffer = vertexBuffer_; vb.offset = 0;
	SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
	SDL_GPUBufferBinding ib = {}; ib.buffer = indexBuffer_; ib.offset = 0;
	SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_GPUTextureSamplerBinding ts[5] = {};
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
	// The slope map always exists when the mesh does (buildBumpTexture failing fails the
	// build). Wireframe needs no stand-in: it zeroes the diffuse, so the normal is moot.
	ts[4].texture = bumpTexture_ ? bumpTexture_ : whiteTexture_;
	ts[4].sampler = sampler_;   // linear + clamp, like the colour it is baked beside

	// A placement-zone LAVA run is drawn over the terrain mesh with the animated lava shader
	// instead of the plain terrain pipeline, exactly where cTileMap::setMaterial switched to
	// lavaShader_ on D3D9. Everything the lava FS needs that is not per-material -- the
	// transform, the fog-of-war state and the fog colour -- is the same across the frame, so
	// build it once; the per-material colours/scales/time are set in the loop. Skipped in
	// wireframe (every run draws as the white grid) and when the pipeline failed to build.
	const bool reflection = camera->getAttribute(ATTRCAMERA_REFLECTION);
	SDL_GPUGraphicsPipeline* lavaPipeline = reflection ? pipelineLavaMirror_ : pipelineLava_;
	const bool lavaReady = !wireframe && tileMap && lavaPipeline && volumeTexture_;
	LavaVSUniform lvs;
	std::memcpy(lvs.mvp, vsu.mvp, sizeof(lvs.mvp));
	std::memcpy(lvs.planarNode, vsu.planarNode, sizeof(lvs.planarNode));
	std::memcpy(lvs.fogPlane, vsu.fogPlane, sizeof(lvs.fogPlane));
	LavaFSUniform lfs = {};
	std::memcpy(lfs.fogColor, fsu.fogColor, sizeof(lfs.fogColor));
	std::memcpy(lfs.fogOfWarColor, fsu.fogOfWarColor, sizeof(lfs.fogOfWarColor));
	lfs.lightMapParams[0] = fsu.lightMapParams[1];   // fog of war on (the tilemap's y flag)
	const float animTime = tileMap ? tileMap->animationTime() : 0.f;

	// Ice frame-constant state. Like lava, a placement-zone ICE run is drawn over the terrain
	// mesh with the reflective ice shader (tilemap_ice), where cTileMap::setMaterial drove
	// ShaderSceneWaterIce for it. The reflection is the scene's planar reflection: the main
	// camera's ATTRCAMERA_REFLECTION child rendered its target this frame, and the mirror
	// matrix (Water.cpp::fillMirrorMatrix) projects a world position into it. When this pass IS
	// the reflection camera -- no nested reflection -- or there is none, the shader draws plain
	// snow (its Params.x gate), the sky-cubemap fallback of the original having no SDL consumer.
	SDL_GPUGraphicsPipeline* icePipeline = reflection ? pipelineIceMirror_ : pipelineIce_;
	const bool iceReady = !wireframe && tileMap && icePipeline;
	SDL_GPUTexture* reflectionTex = nullptr;
	IceVSUniform ivs = {};
	std::memcpy(ivs.mvp, vsu.mvp, sizeof(ivs.mvp));
	std::memcpy(ivs.planarNode, vsu.planarNode, sizeof(ivs.planarNode));
	std::memcpy(ivs.fogPlane, vsu.fogPlane, sizeof(ivs.fogPlane));
	// The original's fScaleBumpSnow: bump uv = pos.xy*0.01, snow uv = pos.xy*0.003.
	ivs.scaleBumpSnow[0] = 0.01f; ivs.scaleBumpSnow[1] = 0.003f;
	if(iceReady && !reflection){
		if(Camera* refl = camera->FindChildCamera(ATTRCAMERA_REFLECTION)){
			cTexture* rt = refl->GetRenderTarget();
			if(rt && rt->frameNumber() >= 1 && rt->GetWidth() > 0 && rt->GetHeight() > 0){
				reflectionTex = reinterpret_cast<SDL_GPUTexture*>(rt->GetDDSurface(0));
				// fillMirrorMatrix: reflection view-proj folded with the clip->texture map,
				// row-vector convention. No half-texel offset -- SDL's pixel-centre matches
				// (see Water.cpp and cSDLRenderDevice::shadowMatBias).
				const Mat4f texAdj(0.5f,  0.0f, 0.0f, 0.0f,
				                   0.0f, -0.5f, 0.0f, 0.0f,
				                   0.0f,  0.0f, 1.0f, 0.0f,
				                   0.5f,  0.5f, 0.0f, 1.0f);
				const Mat4f m = refl->matViewProj * texAdj;
				std::memcpy(ivs.mirrorVP, &m, sizeof(ivs.mirrorVP));
			}
		}
	}
	IceFSUniform ifs = {};
	std::memcpy(ifs.fogColor, fsu.fogColor, sizeof(ifs.fogColor));
	std::memcpy(ifs.fogOfWarColor, fsu.fogOfWarColor, sizeof(ifs.fogOfWarColor));
	ifs.params[0] = reflectionTex ? 1.f : 0.f;   // a reflection target exists to sample
	ifs.params[1] = fsu.lightMapParams[1];        // fog of war on
	// vSnowColor: the scene's plain-lit colour, tinting the snow texture.
	const Color4f snowColor = (tileMap && tileMap->scene())
	                        ? tileMap->scene()->GetPlainLitColor() : Color4f(1.f, 1.f, 1.f, 1.f);
	ifs.snowColor[0] = snowColor.r; ifs.snowColor[1] = snowColor.g;
	ifs.snowColor[2] = snowColor.b; ifs.snowColor[3] = 1.f;

	// The pipeline bound before the loop (above). We switch to the lava pipeline for lava
	// runs and back, so track what is currently bound to avoid redundant binds.
	SDL_GPUGraphicsPipeline* boundPipeline = pipeline;

	// The original changes material between tiles -- cTileMap::setMaterial binds that
	// material's detail texture and its mulMiniTexture, then draws the tile's index list
	// for it. Our index buffer is grouped by material for the whole map, so the same
	// rebinding happens once per run instead of once per tile.
	const float resolution = tileMap ? (float)tileMap->miniDetailTextureResolution() : 0.f;
	for(size_t i = 0; i < runs_.size(); ++i){
		const MaterialRun& run = runs_[i];

		// Is this a LAVA placement-zone run with its ground texture ready? If so, draw it
		// with the lava pipeline; otherwise it falls through to the plain terrain path
		// (which for a placement-zone material draws the baked colour with no detail tile).
		const int pz = run.material - cTileMap::miniDetailTexturesNumber;
		SDL_GPUTexture* groundTex = nullptr;
		if(lavaReady && pz >= 0 && pz < cTileMap::placementZoneMaterialNumber &&
		   tileMap->placementZoneMaterial(pz).shaderType == cTileMap::LAVA){
			cTexture* g = tileMap->placementZoneMaterial(pz).texture;
			if(g && g->frameNumber() >= 1 && g->GetWidth() > 0 && g->GetHeight() > 0)
				groundTex = reinterpret_cast<SDL_GPUTexture*>(g->GetDDSurface(0));
		}

		if(groundTex){
			const cTileMap::PlacementZoneMaterial& mat = tileMap->placementZoneMaterial(pz);
			lfs.lavaColor[0] = mat.lavaColor.r; lfs.lavaColor[1] = mat.lavaColor.g;
			lfs.lavaColor[2] = mat.lavaColor.b; lfs.lavaColor[3] = 1.f;
			lfs.lavaAmbient[0] = mat.colorAmbient.r; lfs.lavaAmbient[1] = mat.colorAmbient.g;
			lfs.lavaAmbient[2] = mat.colorAmbient.b; lfs.lavaAmbient[3] = 1.f;
			lfs.params[0] = mat.textureScale;         // uv_ground scale
			lfs.params[1] = mat.volumeTextureScale;   // uv_volume scale
			lfs.params[2] = animTime * mat.speed;     // ShaderSceneWaterLava::SetTime
			lfs.params[3] = 0.f;

			if(boundPipeline != lavaPipeline){
				SDL_BindGPUGraphicsPipeline(pass, lavaPipeline);
				boundPipeline = lavaPipeline;
			}
			SDL_PushGPUVertexUniformData(cmd, 0, &lvs, sizeof(lvs));
			SDL_PushGPUFragmentUniformData(cmd, 0, &lfs, sizeof(lfs));

			SDL_GPUTextureSamplerBinding lts[3] = {};
			lts[0].texture = volumeTexture_; lts[0].sampler = volumeSampler_;   // wrap + linear
			lts[1].texture = groundTex;      lts[1].sampler = detailSampler_;    // ground tiles
			lts[2].texture = lightMap ? lightMap : whiteTexture_; lts[2].sampler = sampler_;
			SDL_BindGPUFragmentSamplers(pass, 0, lts, 3);

			SDL_DrawGPUIndexedPrimitives(pass, run.count, 1, run.first, 0, 0);
			continue;
		}

		// A placement-zone ICE run: draw with the reflective ice shader, opaque like lava,
		// where cTileMap::setMaterial called iceShader_->beginDraw for it. The snow texture is
		// required; the bump falls back to neutral grey (unbiases to a zero perturbation).
		if(iceReady && pz >= 0 && pz < cTileMap::placementZoneMaterialNumber &&
		   tileMap->placementZoneMaterial(pz).shaderType == cTileMap::ICE){
			const cTileMap::PlacementZoneMaterial& mat = tileMap->placementZoneMaterial(pz);
			cTexture* snow = mat.texture;
			SDL_GPUTexture* snowTex = nullptr;
			if(snow && snow->frameNumber() >= 1 && snow->GetWidth() > 0 && snow->GetHeight() > 0)
				snowTex = reinterpret_cast<SDL_GPUTexture*>(snow->GetDDSurface(0));
			if(snowTex){
				cTexture* bump = mat.textureBump;
				SDL_GPUTexture* bumpTex = greyTexture_;   // neutral: (128*255...)->0 perturbation
				if(bump && bump->frameNumber() >= 1 && bump->GetWidth() > 0 && bump->GetHeight() > 0)
					bumpTex = reinterpret_cast<SDL_GPUTexture*>(bump->GetDDSurface(0));

				if(boundPipeline != icePipeline){
					SDL_BindGPUGraphicsPipeline(pass, icePipeline);
					boundPipeline = icePipeline;
				}
				SDL_PushGPUVertexUniformData(cmd, 0, &ivs, sizeof(ivs));
				SDL_PushGPUFragmentUniformData(cmd, 0, &ifs, sizeof(ifs));

				SDL_GPUTextureSamplerBinding its[4] = {};
				its[0].texture = snowTex; its[0].sampler = detailSampler_;   // wrap + aniso
				its[1].texture = bumpTex; its[1].sampler = detailSampler_;   // wrap + aniso
				its[2].texture = reflectionTex ? reflectionTex : whiteTexture_;
				its[2].sampler = sampler_;   // clamp + linear (the reflection is 1:1 projected)
				its[3].texture = lightMap ? lightMap : whiteTexture_;
				its[3].sampler = sampler_;
				SDL_BindGPUFragmentSamplers(pass, 0, its, 4);

				SDL_DrawGPUIndexedPrimitives(pass, run.count, 1, run.first, 0, 0);
				continue;
			}
		}

		if(boundPipeline != pipeline){
			SDL_BindGPUGraphicsPipeline(pass, pipeline);
			boundPipeline = pipeline;
		}

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
		SDL_BindGPUFragmentSamplers(pass, 0, ts, 5);

		SDL_DrawGPUIndexedPrimitives(pass, run.count, 1, run.first, 0, 0);
	}

	SDL_EndGPURenderPass(pass);
	return true;
}

