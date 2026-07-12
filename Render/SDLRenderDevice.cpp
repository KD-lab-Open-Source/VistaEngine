// SDL GPU render device — device, swapchain and shared GPU resources. See header.
#include "StdAfxRD.h"
#include "SDLRenderDevice.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>

#include "Texture.h"     // cTexture (BitMap / GetDDSurface / attributes)
#include "FileImage.h"   // cFileImage::GetTexture
#include "TexLibrary.h"  // GetTexLibrary()->CreateRenderTexture (the shadow map)
#include "cCamera.h"     // Camera (GetRenderTarget / GetFoneColor), sViewPort
#include "VisGenericDefine.h"   // ATTRCAMERA_NOCLEARTARGET, ATTRCAMERA_REFLECTION

#include "SDLUIRenderer.h"
#include "SDLTileMapRenderer.h"
#include "SDLObject3dxRenderer.h"
#include "SDLWaterRenderer.h"
#include "SDLWorldQuadRenderer.h"

// See the declarations in SDLRenderDevice.h.
cSDLRenderDevice* sdlRenderDevice()
{
	return dynamic_cast<cSDLRenderDevice*>(gb_RenderDevice);
}

SDLObject3dxRenderer* sdlObjectRenderer()
{
	cSDLRenderDevice* dev = sdlRenderDevice();
	return dev ? dev->objectRenderer() : nullptr;
}

SDLWaterRenderer* sdlWaterRenderer()
{
	cSDLRenderDevice* dev = sdlRenderDevice();
	return dev ? dev->waterRenderer() : nullptr;
}

SDLWorldQuadRenderer* sdlWorldQuadRenderer()
{
	cSDLRenderDevice* dev = sdlRenderDevice();
	return dev ? dev->worldQuadRenderer() : nullptr;
}

void applyCameraViewport(SDL_GPURenderPass* pass, const sViewPort& vp, int targetW, int targetH)
{
	if(!pass || vp.Width <= 0 || vp.Height <= 0)
		return;   // camera never got a frustum: leave the pass's full-target viewport

	const int x = vp.X < 0 ? 0 : vp.X;
	const int y = vp.Y < 0 ? 0 : vp.Y;
	const int w = vp.Width  > targetW - x ? targetW - x : vp.Width;
	const int h = vp.Height > targetH - y ? targetH - y : vp.Height;
	if(w <= 0 || h <= 0)
		return;

	SDL_GPUViewport v;
	v.x = (float)x; v.y = (float)y;
	v.w = (float)w; v.h = (float)h;
	v.min_depth = vp.MinZ;
	v.max_depth = vp.MaxZ;
	SDL_SetGPUViewport(pass, &v);
}

// ---------------------------------------------------------------------------
// Base cInterfaceRenderDevice members. On Windows these live in
// Render/D3D/RenderDevice.cpp (not compiled off-Windows), so the cross-platform
// subclass must supply them here.
// ---------------------------------------------------------------------------
cInterfaceRenderDevice::cInterfaceRenderDevice()
{
	NumberPolygon = 0;
	NumDrawObject = 0;
	NumberTilemapPolygon = 0;
	PtrNumberPolygon = &NumberPolygon;
	RenderMode = 0;
	xScr = yScr = 0;
	xScrMin = yScrMin = xScrMax = yScrMax = 0;
	camera_ = 0;
	DefaultFont = CurrentFont = 0;
}

cInterfaceRenderDevice::~cInterfaceRenderDevice()
{
	gb_RenderDevice = 0;
}

void cInterfaceRenderDevice::SetFont(FT::Font* pFont)
{
	CurrentFont = pFont;
	if(!CurrentFont)
		CurrentFont = DefaultFont;
}

void cInterfaceRenderDevice::SetDefaultFont(FT::Font* pFont)
{
	if(!pFont && DefaultFont == CurrentFont)
		CurrentFont = 0;
	DefaultFont = pFont;
	if(!CurrentFont)
		CurrentFont = DefaultFont;
}

// ---------------------------------------------------------------------------
// cSDLRenderDevice
// ---------------------------------------------------------------------------
cSDLRenderDevice::cSDLRenderDevice()
{
	shadowMatViewProj_ = Mat4f::ID;   // Mat4f's default ctor leaves it uninitialized
}

cSDLRenderDevice::~cSDLRenderDevice()
{
	Done();
}

bool cSDLRenderDevice::Initialize(int xScr_, int yScr_, int mode, HWND hWnd, int /*RefreshRateInHz*/, HWND /*fallbackWindow*/)
{
	// hWnd carries the SDL_Window* created by Platform/Window.cpp (HWND == void*).
	window_ = static_cast<SDL_Window*>(hWnd);
	if(!window_){
		fprintf(stderr, "cSDLRenderDevice::Initialize: no window\n");
		return false;
	}

	if(!device_){
		device_ = SDL_CreateGPUDevice(
			SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL,
			false, nullptr);
		if(!device_){
			fprintf(stderr, "cSDLRenderDevice::Initialize: SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
			return false;
		}
		if(!SDL_ClaimWindowForGPUDevice(device_, window_)){
			fprintf(stderr, "cSDLRenderDevice::Initialize: SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
			SDL_DestroyGPUDevice(device_);
			device_ = nullptr;
			return false;
		}
		fprintf(stderr, "cSDLRenderDevice: SDL GPU device created (%s)\n", SDL_GetGPUDeviceDriver(device_));
	}

	xScr = xScr_;
	yScr = yScr_;
	xScrMin = yScrMin = 0;
	xScrMax = xScr;
	yScrMax = yScr;
	RenderMode = mode;

	if(!uiRenderer_)
		uiRenderer_ = std::make_unique<SDLUIRenderer>(device_, window_);
	if(!tileMapRenderer_)
		tileMapRenderer_ = std::make_unique<SDLTileMapRenderer>(device_, window_);
	if(!objectRenderer_)
		objectRenderer_ = std::make_unique<SDLObject3dxRenderer>(this, device_, window_);
	if(!waterRenderer_)
		waterRenderer_ = std::make_unique<SDLWaterRenderer>(this, device_, window_);
	if(!worldQuadRenderer_)
		worldQuadRenderer_ = std::make_unique<SDLWorldQuadRenderer>(device_, window_);

	// Build the skinned-vertex declarations (on Windows cD3DRender does this at
	// device init via CreateVertexDeclaration; cSkinVertex::Register is portable
	// off-Windows). Needed so cStatic3dx buffers get a real vertex layout/stride.
	static bool skinDeclRegistered = false;
	if(!skinDeclRegistered){ cSkinVertex::Register(); skinDeclRegistered = true; }
	return true;
}

int cSDLRenderDevice::Done()
{
	if(bActiveScene_)
		EndScene();
	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;

	// Release the render targets while the device (and gb_RenderDevice == this) is still
	// valid: ~cTexture routes through DeleteTexture, which needs textures_ and device_.
	deleteShadowMap();
	deleteLightMap();

	// The renderers hold GPU objects built on device_, so they must go first.
	uiRenderer_.reset();
	tileMapRenderer_.reset();
	objectRenderer_.reset();
	waterRenderer_.reset();
	worldQuadRenderer_.reset();

	if(device_){
		// The depth buffers the offscreen colour targets own. Their colour textures are
		// cTexture-backed and released with everything else in textures_.
		for(auto& kv : targets_)
			if(kv.second.ownsDepth && kv.second.depth)
				SDL_ReleaseGPUTexture(device_, kv.second.depth);
		targets_.clear();
		current_ = &screen_;
		screen_ = RenderTarget();

		if(depthTexture_){
			SDL_ReleaseGPUTexture(device_, depthTexture_);
			depthTexture_ = nullptr;
			depthW_ = depthH_ = 0;
		}
		for(auto& kv : textures_)
			if(kv.second.tex) SDL_ReleaseGPUTexture(device_, kv.second.tex);
		textures_.clear();
		for(auto& kv : vbGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		for(auto& kv : ibGpu_) if(kv.second.buf) SDL_ReleaseGPUBuffer(device_, kv.second.buf);
		vbGpu_.clear();
		ibGpu_.clear();
		if(window_)
			SDL_ReleaseWindowFromGPUDevice(device_, window_);
		SDL_DestroyGPUDevice(device_);
		device_ = nullptr;
	}
	window_ = nullptr;

	// Reset base state so ~cInterfaceRenderDevice's invariants hold.
	RenderMode = 0;
	xScr = yScr = 0;
	xScrMin = yScrMin = xScrMax = yScrMax = 0;
	CurrentFont = DefaultFont = 0;
	return 0;
}

bool cSDLRenderDevice::ChangeSize(int xScr_, int yScr_, int mode)
{
	xScr = xScr_;
	yScr = yScr_;
	xScrMin = yScrMin = 0;
	xScrMax = xScr;
	yScrMax = yScr;
	RenderMode = mode;
	return true;
}

int cSDLRenderDevice::Fill(int r, int g, int b, int a)
{
	// The screen's clear. Offscreen targets take theirs from the camera's fone colour
	// when setCamera binds them (armClear), as they do on D3D.
	screen_.clearColor[0] = r / 255.f;
	screen_.clearColor[1] = g / 255.f;
	screen_.clearColor[2] = b / 255.f;
	screen_.clearColor[3] = a / 255.f;
	screen_.clearPending = true;
	return 0;
}

int cSDLRenderDevice::BeginScene()
{
	if(!device_) return -1;
	if(bActiveScene_) return 1;

	// Acquire the frame's command buffer + swapchain texture. Passes are deferred to
	// EndScene: draws accumulate into each renderer's CPU batch during the scene, and
	// the renderers record their copy + render passes once, at EndScene.
	commandBuffer_ = SDL_AcquireGPUCommandBuffer(device_);
	if(!commandBuffer_){
		fprintf(stderr, "cSDLRenderDevice::BeginScene: AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
		return -1;
	}

	Uint32 w = 0, h = 0;
	if(!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer_, window_, &swapchainTexture_, &w, &h) || !swapchainTexture_){
		// No drawable surface this frame (e.g. minimized); submit empty and bail.
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
		swapchainTexture_ = nullptr;
		return -1;
	}

	if(uiRenderer_)
		uiRenderer_->BeginFrame();
	if(objectRenderer_)
		objectRenderer_->BeginFrame();
	if(waterRenderer_)
		waterRenderer_->BeginFrame();
	if(worldQuadRenderer_)
		worldQuadRenderer_->BeginFrame();

	// The screen is this frame's swapchain image; its clear was armed by Fill(). Offscreen
	// targets keep their textures across frames, but not the clears they have consumed.
	screen_.color = swapchainTexture_;
	screen_.w = xScr;
	screen_.h = yScr;
	screen_.depth = ensureDepth(xScr, yScr) ? depthTexture_ : nullptr;
	screen_.colorCleared = false;
	screen_.depthCleared = false;
	for(auto& kv : targets_){
		kv.second.clearPending = false;
		kv.second.colorCleared = false;
		kv.second.depthCleared = false;
	}
	current_ = &screen_;

	shadowPassRan_ = false;
	bActiveScene_ = true;
	NumberPolygon = 0;
	NumDrawObject = 0;
	NumberTilemapPolygon = 0;
	return 0;
}

int cSDLRenderDevice::EndScene()
{
	if(!bActiveScene_) return 1;
	bActiveScene_ = false;

	// Whatever objects the scene walk recorded and no earlier flush replayed -- everything
	// past SCENENODE_OBJECTSPECIAL, or the whole scene in a mission with neither water nor
	// coast sprites. Over the terrain and against its depth. The last camera to draw is the
	// main one, so the current target is the screen; assert nothing, just settle it.
	flushTarget(current_, true);

	// Then the UI pass, last, over everything. It carries the frame's colour clear only if
	// no earlier pass took it.
	if(screen_.color && commandBuffer_ && uiRenderer_)
		uiRenderer_->Draw(commandBuffer_, screen_.color, screen_.w, screen_.h,
		                  screen_.clearPending && !screen_.colorCleared, screen_.clearColor);

	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;
	screen_.color = nullptr;
	screen_.clearPending = false;
	current_ = &screen_;
	return 0;
}

SDL_GPUTexture* cSDLRenderDevice::createDepthTexture(int w, int h)
{
	if(!device_ || w <= 0 || h <= 0) return nullptr;
	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	ti.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = 1;
	return SDL_CreateGPUTexture(device_, &ti);
}

// The depth target behind the screen. Sized to the swapchain, rebuilt on resize.
bool cSDLRenderDevice::ensureDepth(int w, int h)
{
	if(!device_ || w <= 0 || h <= 0) return false;
	if(depthTexture_ && depthW_ == w && depthH_ == h) return true;
	if(depthTexture_) SDL_ReleaseGPUTexture(device_, depthTexture_);

	depthTexture_ = createDepthTexture(w, h);
	depthW_ = depthTexture_ ? w : 0;
	depthH_ = depthTexture_ ? h : 0;
	return depthTexture_ != nullptr;
}

int cSDLRenderDevice::Flush()
{
	if(bActiveScene_)
		EndScene();
	return 0;
}

// ---------------------------------------------------------------------------
// Render targets
//
// cD3DRender::setCamera binds camera->GetRenderTarget() and clears it, or restores the
// back buffer. These three do the same job: resolveTarget maps the camera's cTexture to
// the SDL textures behind it, armClear stands in for D3D's Clear(), and flushTarget is
// the boundary -- the object renderer batches across the whole scene walk, so whatever it
// recorded under the outgoing camera must reach the outgoing target before the incoming
// camera's passes (which may sample it) are recorded.
// ---------------------------------------------------------------------------
cSDLRenderDevice::RenderTarget* cSDLRenderDevice::resolveTarget(Camera* camera)
{
	cTexture* texture = camera ? camera->GetRenderTarget() : nullptr;
	if(!texture)
		return &screen_;

	auto it = targets_.find(texture);
	if(it != targets_.end())
		return &it->second;

	// cScene creates the target's cTexture before the device makes it resident. Until then
	// there is nothing to draw into -- and drawing to the screen instead would paint the
	// shadow casters over the frame.
	SDL_GPUTexture* tex = texture->frameNumber() >= 1
	                    ? reinterpret_cast<SDL_GPUTexture*>(texture->GetDDSurface(0))
	                    : nullptr;
	if(!tex)
		return &nullTarget_;

	RenderTarget rt;
	rt.w = texture->GetWidth();
	rt.h = texture->GetHeight();
	if(texture->getAttribute(TEXTURE_RENDER_SHADOW_9700)){
		rt.depthOnly = true;
		rt.depth = tex;   // the shadow map *is* its depth buffer; no colour attachment
	}
	else {
		rt.color = tex;
		rt.depth = createDepthTexture(rt.w, rt.h);
		if(!rt.depth)
			return &nullTarget_;
		rt.ownsDepth = true;
	}
	return &targets_.emplace(texture, rt).first->second;
}

void cSDLRenderDevice::armClear(RenderTarget* rt, Camera* camera)
{
	if(!camera)
		return;

	// ATTRCAMERA_NOCLEARTARGET: draw over whatever the target already holds. cSkyObj::
	// DrawSky fills the reflection target with the sky, and the reflection camera then
	// draws the world over it -- re-arming the depth clear on its own, and only the depth,
	// through ATTRCAMERA_CLEARZBUFFER (Camera::ClearZBuffer -> clearZBuffer).
	if(camera->getAttribute(ATTRCAMERA_NOCLEARTARGET)){
		rt->clearPending = false;
		rt->colorCleared = true;
		rt->depthCleared = true;
		return;
	}

	const Color4c c = camera->GetFoneColor();
	rt->clearColor[0] = c.r / 255.f;
	rt->clearColor[1] = c.g / 255.f;
	rt->clearColor[2] = c.b / 255.f;
	// cD3DRender::setCamera masks the fone colour's alpha off for a reflection camera
	// (`color &= ~0xff000000`): the reflection target's alpha is the water's HDR mask.
	rt->clearColor[3] = camera->getAttribute(ATTRCAMERA_REFLECTION) ? 0.f : c.a / 255.f;

	rt->clearPending = true;
	rt->colorCleared = false;
	rt->depthCleared = false;
}

void cSDLRenderDevice::flushTarget(RenderTarget* rt, bool settle)
{
	if(!rt || !commandBuffer_ || !objectRenderer_)
		return;

	// Nothing to render into (nullTarget_, or a frame with no swapchain image). The draws
	// recorded under it belong nowhere; drop them, or they replay into the next target.
	if(!rt->usable()){
		objectRenderer_->DiscardDraws();
		return;
	}

	const bool hasDraws = objectRenderer_->hasDraws();

	if(rt->depthOnly){
		// The shadow map. Worth a pass even with nothing to cast, because the clear alone
		// leaves the map at far depth, i.e. every receiver lit. Skipping it would let them
		// sample last frame's map, or on the first frame an undefined one.
		if(!hasDraws && (rt->depthCleared || !settle))
			return;
		if(objectRenderer_->DrawShadowPass(commandBuffer_, rt->depth, rt->w, !rt->depthCleared)){
			rt->depthCleared = true;
			shadowPassRan_   = true;
		}
		return;
	}

	// An offscreen colour target we are leaving still owes its clear to whatever samples it
	// later, so open the pass for the clear alone if the scene walk drew nothing into it.
	// The screen's clear can always keep waiting: the UI pass at EndScene will take it.
	const bool owesClear = settle && rt != &screen_
	                    && ((rt->clearPending && !rt->colorCleared) || !rt->depthCleared);
	if(!hasDraws && !owesClear)
		return;

	const bool clear = rt->clearPending && !rt->colorCleared;
	if(objectRenderer_->Draw(commandBuffer_, rt->color, rt->depth, rt->w, rt->h,
	                         clear, rt->clearColor, !rt->depthCleared,
	                         fillMode_ == FILL_WIREFRAME)){
		if(clear) rt->colorCleared = true;
		rt->depthCleared = true;
	}
}

void cSDLRenderDevice::releaseTarget(cTexture* texture)
{
	auto it = targets_.find(texture);
	if(it == targets_.end())
		return;
	if(current_ == &it->second)
		current_ = &screen_;
	if(it->second.ownsDepth && it->second.depth && device_)
		SDL_ReleaseGPUTexture(device_, it->second.depth);
	targets_.erase(it);
}

void cSDLRenderDevice::setCamera(Camera* camera)
{
	SetDrawTransform(camera);

	RenderTarget* rt = resolveTarget(camera);
	if(rt == current_)
		return;

	// Leaving the old target: it must be complete, because the camera we are switching to
	// may open a pass that samples it (the water's reflection, the receivers' shadow map).
	flushTarget(current_, true);
	current_ = rt;

	if(rt != &screen_ && rt != &nullTarget_)
		armClear(rt, camera);
}

// ---------------------------------------------------------------------------
// Terrain: forwarded to the tilemap renderer, which draws immediately in its own
// pass. Called mid-scene from cTileMap::Draw, so the frame's command buffer and
// swapchain image are live and no render pass is open.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::drawTileMap(cTileMap* tileMap, Camera* camera)
{
	if(!bActiveScene_ || !commandBuffer_ || !tileMapRenderer_)
		return;
	RenderTarget* rt = current_;
	if(rt->depthOnly || !rt->usable())
		return;
	const bool clear = rt->clearPending && !rt->colorCleared;
	if(tileMapRenderer_->Draw(commandBuffer_, rt->color, rt->depth, rt->w, rt->h,
	                          clear, rt->clearColor, !rt->depthCleared, tileMap, camera,
	                          fillMode_ == FILL_WIREFRAME)){
		if(clear) rt->colorCleared = true;
		rt->depthCleared = true;
	}
}

// ---------------------------------------------------------------------------
// Water and world quads: what cWater::DrawPolygons, cCoastSprites' sprite loops and the
// wave sources just recorded, drawn where the scene walk reached them. See the header for
// why the object batch is flushed first.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::flushObjectPass()
{
	// Mid-target: whatever opens the next pass takes the clear, so don't force one here.
	flushTarget(current_, false);
}

void cSDLRenderDevice::drawWater()
{
	if(!bActiveScene_ || !commandBuffer_ || !waterRenderer_ || !waterRenderer_->hasDraws())
		return;
	RenderTarget* rt = current_;
	if(rt->depthOnly || !rt->usable())
		return;

	flushObjectPass();

	const bool clear = rt->clearPending && !rt->colorCleared;
	if(waterRenderer_->Draw(commandBuffer_, rt->color, rt->depth, rt->w, rt->h,
	                        clear, rt->clearColor, !rt->depthCleared, fillMode_ == FILL_WIREFRAME)){
		if(clear) rt->colorCleared = true;
		rt->depthCleared = true;
	}
}

void cSDLRenderDevice::drawWorldQuads()
{
	if(!bActiveScene_ || !commandBuffer_ || !worldQuadRenderer_ || !worldQuadRenderer_->hasDraws())
		return;
	RenderTarget* rt = current_;
	if(rt->depthOnly || !rt->usable())
		return;

	// A no-op once an earlier caller drained the object batch. It has not, on a dry map,
	// nor for whatever the sorted pass recorded before reaching the wave sources.
	flushObjectPass();

	const bool clear = rt->clearPending && !rt->colorCleared;
	if(worldQuadRenderer_->Draw(commandBuffer_, rt->color, rt->depth, rt->w, rt->h,
	                            clear, rt->clearColor, !rt->depthCleared, fillMode_ == FILL_WIREFRAME)){
		if(clear) rt->colorCleared = true;
		rt->depthCleared = true;
	}
}

// ---------------------------------------------------------------------------
// Shadow map
// ---------------------------------------------------------------------------
bool cSDLRenderDevice::createShadowMap(int size)
{
	deleteShadowMap();
	if(!device_ || size <= 0)
		return false;
	// Routed through the texture library, as cD3DRender::createRenderTargets is: the
	// cTexture is what Camera::SetRenderTarget takes and what carries the map's size.
	shadowMap_ = GetTexLibrary()->CreateRenderTexture(size, size, TEXTURE_RENDER_SHADOW_9700, false);
	if(!shadowMap_)
		return false;
	shadowMapSize_ = size;
	fprintf(stderr, "cSDLRenderDevice: shadow map %dx%d ready\n", size, size);
	return true;
}

void cSDLRenderDevice::deleteShadowMap()
{
	RELEASE(shadowMap_);
	shadowMapSize_ = 0;
}

bool cSDLRenderDevice::createLightMap(int size)
{
	deleteLightMap();
	if(!device_ || size <= 0)
		return false;
	// TEXTURE_RENDER32, as cD3DRender::createRenderTargets makes it: a colour target, so
	// resolveTarget gives it a depth buffer it will not use (CameraPlanarLight turns depth
	// write off and the depth test to ALWAYS).
	lightMap_ = GetTexLibrary()->CreateRenderTexture(size, size, TEXTURE_RENDER32, false);
	if(!lightMap_)
		return false;
	fprintf(stderr, "cSDLRenderDevice: terrain lightmap %dx%d ready\n", size, size);
	return true;
}

void cSDLRenderDevice::deleteLightMap()
{
	RELEASE(lightMap_);
}

Mat4f cSDLRenderDevice::shadowMatBias() const
{
	// Light clip space -> shadow map texture coords. SDL GPU normalizes clip space to
	// D3D's (x,y in [-1,1] with y up, z in [0,1]), so this is cD3DRender::shadowMatBias
	// without its half-texel offset: that corrects D3D9's pixel-centre convention, which
	// SDL does not share. Row-vector convention, to match mul(world, mShadow).
	//
	// The constant depth bias sits in the last row, as the original's does, rather than in
	// the rasterizer: it must not be scaled by the TSM warp the light matrix may carry.
	// Same value as cD3DRender::shadowMatBias uses on DT_RADEON9700. The original's
	// D3DRS_SLOPESCALEDEPTHBIAS never reached that path -- it biased the depth buffer,
	// while the caster wrote its depth as *colour* -- but it does reach ours, so the
	// slope-dependent part of the acne is handled in the caster pipeline.
	const float bias = 0.0005f;
	return Mat4f(0.5f,  0.0f,  0.0f, 0.0f,
	             0.0f, -0.5f,  0.0f, 0.0f,
	             0.0f,  0.0f,  1.0f, 0.0f,
	             0.5f,  0.5f, -bias, 1.0f);
}

void cSDLRenderDevice::drawTileMapShadow(Camera* camera)
{
	if(!bActiveScene_ || !commandBuffer_ || !tileMapRenderer_)
		return;
	// Only into a depth-only target. cTileMap::Draw routes here on ATTRCAMERA_SHADOWMAP,
	// so this is the light camera and setCamera has already bound it the shadow map --
	// unless the map has no texture yet, in which case current_ is nullTarget_.
	RenderTarget* rt = current_;
	if(!rt->depthOnly || !rt->usable())
		return;
	if(tileMapRenderer_->DrawShadowPass(commandBuffer_, rt->depth, rt->w, camera,
	                                    !rt->depthCleared)){
		rt->depthCleared = true;
		shadowPassRan_   = true;
	}
}

// Resolve the engine's buffer handles to the SDL GPU buffers behind them, for the
// renderers that draw from geometry the engine filled.
SDL_GPUBuffer* cSDLRenderDevice::gpuBuffer(const sPtrVertexBuffer& vb) const
{
	if(!vb.ptr) return nullptr;
	auto it = vbGpu_.find(vb.ptr);
	return it != vbGpu_.end() ? it->second.buf : nullptr;
}

SDL_GPUBuffer* cSDLRenderDevice::gpuBuffer(const sPtrIndexBuffer& ib) const
{
	if(!ib.ptr) return nullptr;
	auto it = ibGpu_.find(ib.ptr);
	return it != ibGpu_.end() ? it->second.buf : nullptr;
}

// GameShell sets RS_FILLMODE every frame from debugWireFrame (Scripts/TreeControlSetups/
// Debug.dat), and Camera::DrawSortObject clears RS_ZWRITEENABLE around the transparent
// pass. SDL GPU bakes both into the pipeline, so renderers keep variants and pick them
// up from here.
void cSDLRenderDevice::SetRenderState(eRenderStateOption state, int value)
{
	if(state == RS_FILLMODE)
		fillMode_ = value;
	else if(state == RS_ZWRITEENABLE)
		zWriteEnable_ = value != 0;
}

unsigned int cSDLRenderDevice::GetRenderState(eRenderStateOption state)
{
	if(state == RS_FILLMODE) return (unsigned int)fillMode_;
	if(state == RS_ZWRITEENABLE) return zWriteEnable_ ? 1u : 0u;
	return 0u;
}

// ---------------------------------------------------------------------------
// 2D entry points: forwarded to the UI renderer, which batches them and draws
// them all in its own pass at EndScene.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::SetNoMaterial(eBlendMode /*blend*/, const MatXf&, float /*phase*/,
                                     cTexture* Texture0, cTexture* /*Texture1*/, eColorMode /*mode*/)
{
	// (Blend state is baked into the single alpha pipeline for now; per-blend
	// pipelines come later.)
	if(uiRenderer_)
		uiRenderer_->SetTexture(Texture0);
}

void cSDLRenderDevice::DrawQuad(float x1, float y1, float dx, float dy,
                                float u1, float v1, float du, float dv, Color4c color)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	uiRenderer_->DrawQuad(x1, y1, dx, dy, u1, v1, du, dv, color);
	NumberPolygon += 2;
}

void cSDLRenderDevice::DrawSprite(int x, int y, int dx, int dy,
                                  float u, float v, float du, float dv,
                                  cTexture* Texture, const Color4c& ColorMul,
                                  float /*phase*/, eBlendMode /*mode*/, float /*saturate*/)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	uiRenderer_->DrawSprite(x, y, dx, dy, u, v, du, dv, Texture, ColorMul);
	NumberPolygon += 2;
}

// The clip tests mirror cD3DRender's: reject before queueing anything, against the
// scissor rect SetClipRect keeps.
void cSDLRenderDevice::DrawLine(int x1, int y1, int x2, int y2, Color4c color)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	if(x1 <= x2){ if(x2 < xScrMin || x1 > xScrMax) return; }
	else if(x1 < xScrMin || x2 > xScrMax) return;
	if(y1 <= y2){ if(y2 < yScrMin || y1 > yScrMax) return; }
	else if(y1 < yScrMin || y2 > yScrMax) return;
	uiRenderer_->DrawLine(x1, y1, x2, y2, color);
}

void cSDLRenderDevice::DrawPixel(int x, int y, Color4c color)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	if(x < xScrMin || x > xScrMax || y < yScrMin || y > yScrMax) return;
	uiRenderer_->DrawPixel(x, y, color);
}

void cSDLRenderDevice::DrawRectangle(int x, int y, int dx, int dy, Color4c color, bool outline)
{
	if(!bActiveScene_ || !uiRenderer_) return;
	const int x2 = x + dx, y2 = y + dy;
	if(dx >= 0){ if(x2 < xScrMin || x > xScrMax) return; }
	else if(x < xScrMin || x2 > xScrMax) return;
	if(dy >= 0){ if(y2 < yScrMin || y > yScrMax) return; }
	else if(y < yScrMin || y2 > yScrMax) return;
	uiRenderer_->DrawRectangle(x, y, dx, dy, color, outline);
	if(!outline) NumberPolygon += 2;
}

void cSDLRenderDevice::OutText(int x, int y, const char* text, const Color4f& color,
                               ALIGN_TEXT align, eBlendMode /*blend_mode*/, Vect2f scale)
{
	// Blend state is baked into the single alpha pipeline for now; every caller passes
	// ALPHA_BLEND anyway.
	if(!bActiveScene_ || !uiRenderer_ || !CurrentFont) return;
	const int before = uiRenderer_->quadCount();
	uiRenderer_->OutText(x, y, text, *CurrentFont, color, align, scale);
	NumberPolygon += 2 * (uiRenderer_->quadCount() - before);
}

int cSDLRenderDevice::OutTextLine(int x, int y, const FT::Font& font, const wchar_t* textline, const wchar_t* end,
                                  const Color4c& color, eBlendMode /*blend_mode*/, int xRangeMin, int xRangeMax)
{
	if(!bActiveScene_ || !uiRenderer_)
		return x;
	// One quad per glyph drawn, so ask the renderer how many it appended.
	const int before = uiRenderer_->quadCount();
	const int right = uiRenderer_->OutTextLine(x, y, font, textline, end, color, xRangeMin, xRangeMax);
	NumberPolygon += 2 * (uiRenderer_->quadCount() - before);
	return right;
}

// ---------------------------------------------------------------------------
// Textures: real SDL GPU textures backed by a lockable CPU staging image. The
// SDL_GPUTexture* is handed to cTexture via BitMap[0] (the fake IDirect3DTexture9
// Release() is a no-op, so storing a non-D3D pointer is safe); TextureData
// (staging + dims) is kept here, keyed by that handle.
// ---------------------------------------------------------------------------
void cSDLRenderDevice::uploadTexture(const TextureData& td)
{
	if(!device_ || !td.tex || td.staging.empty()) return;

	// The GPU texture is always BGRA8. For coverage (expand) textures the staging
	// is 1 byte/px, widened here to (255,255,255,coverage).
	const Uint32 bytes = (Uint32)(td.w * td.h * 4);
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = bytes;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return;

	unsigned char* map = (unsigned char*)SDL_MapGPUTransferBuffer(device_, tb, false);
	if(td.bpp == 1){
		// 8-bit coverage -> (255,255,255, coverage)
		const int n = td.w * td.h;
		for(int i = 0; i < n; ++i){
			map[i*4+0] = 255; map[i*4+1] = 255; map[i*4+2] = 255;  // B,G,R
			map[i*4+3] = td.staging[i];                            // A = coverage
		}
	} else if(td.bpp == 2){
		// A8L8 staging (byte0 = L, byte1 = A) -> (L,L,L,A)
		const int n = td.w * td.h;
		for(int i = 0; i < n; ++i){
			const unsigned char l = td.staging[i*2+0], a = td.staging[i*2+1];
			map[i*4+0] = l; map[i*4+1] = l; map[i*4+2] = l;
			map[i*4+3] = a;
		}
	} else {
		SDL_memcpy(map, td.staging.data(), bytes);
	}
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
	SDL_GPUTextureTransferInfo src = {};
	src.transfer_buffer = tb;
	src.offset = 0;
	src.pixels_per_row = (Uint32)td.w;
	src.rows_per_layer = (Uint32)td.h;
	SDL_GPUTextureRegion dst = {};
	dst.texture = td.tex;
	dst.w = (Uint32)td.w; dst.h = (Uint32)td.h; dst.d = 1;
	SDL_UploadToGPUTexture(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	// Rebuild the chain from the level we just wrote (outside any pass, as SDL requires).
	if(td.levels > 1)
		SDL_GenerateMipmapsForGPUTexture(cb, td.tex);
	SDL_SubmitGPUCommandBuffer(cb);
	SDL_ReleaseGPUTransferBuffer(device_, tb);
}

int cSDLRenderDevice::CreateTexture(cTexture* Texture, cFileImage* FileImage, int /*dxout*/, int /*dyout*/, bool /*enable_assert*/)
{
	if(!device_ || !Texture) return 1;

	const int w = Texture->GetWidth();
	const int h = Texture->GetHeight();
	if(w <= 0 || h <= 0) return 1;

	// The shadow map (cScene::CreateShadowmap asks cTexLibrary::CreateRenderTexture for
	// TEXTURE_RENDER_SHADOW_9700). A depth texture the light camera renders into and the
	// receivers sample; no staging, no mip chain.
	if(Texture->getAttribute(TEXTURE_RENDER_SHADOW_9700)){
		SDL_GPUTextureCreateInfo ti = {};
		ti.type = SDL_GPU_TEXTURETYPE_2D;
		ti.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
		ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
		ti.width = (Uint32)w; ti.height = (Uint32)h;
		ti.layer_count_or_depth = 1; ti.num_levels = 1;
		SDL_GPUTexture* tex = SDL_CreateGPUTexture(device_, &ti);
		if(!tex){
			fprintf(stderr, "cSDLRenderDevice: shadow map %dx%d failed: %s\n", w, h, SDL_GetError());
			return 1;
		}
		TextureData td;
		td.tex = tex; td.w = w; td.h = h;
		textures_[tex] = std::move(td);
		if(Texture->frameNumber() < 1)
			Texture->New(1);
		Texture->GetDDSurface(0) = reinterpret_cast<IDirect3DTexture9*>(tex);
		return 0;
	}

	// A colour render target (cTexLibrary::CreateRenderTexture with TEXTURE_RENDER32): a
	// camera renders into it and a later pass samples it. Swapchain format, so every
	// pipeline the renderers already built can draw into it unchanged. No staging: nothing
	// locks it from the CPU, and no mip chain, because nothing regenerates one for it.
	if(Texture->getAttribute(TEXTURE_RENDER32)){
		SDL_GPUTextureCreateInfo ti = {};
		ti.type = SDL_GPU_TEXTURETYPE_2D;
		ti.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
		ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
		ti.width = (Uint32)w; ti.height = (Uint32)h;
		ti.layer_count_or_depth = 1; ti.num_levels = 1;
		SDL_GPUTexture* tex = SDL_CreateGPUTexture(device_, &ti);
		if(!tex){
			fprintf(stderr, "cSDLRenderDevice: render target %dx%d failed: %s\n", w, h, SDL_GetError());
			return 1;
		}
		TextureData td;
		td.tex = tex; td.w = w; td.h = h;
		textures_[tex] = std::move(td);
		if(Texture->frameNumber() < 1)
			Texture->New(1);
		Texture->GetDDSurface(0) = reinterpret_cast<IDirect3DTexture9*>(tex);
		return 0;
	}

	// GPU texture is always BGRA8 so one UI shader/sampler covers everything.
	// Gray/alpha-only (the font atlas) keeps 1-byte coverage staging (so the font
	// code's LockTexture pitch is right) and is widened to BGRA on upload; colour
	// textures stage as 32-bit BGRA, matching cFileImage::GetTexture's byte order.
	const bool gray = Texture->getAttribute(TEXTURE_GRAY) != 0;
	// Staging bytes/pixel must match the format callers Lock and write to, or their
	// writes overflow it. TEXTURE_GRAY is set for both 8-bit coverage (font/alpha)
	// AND 16-bit A8L8 (the water Z/reflection texture, written 2 bytes/px in
	// cWater::Init), so A8L8 must be 2 bpp -- otherwise that fill smashes the heap.
	int bpp = gray ? 1 : 4;
	if(Texture->format() == SURFMT_A8L8)
		bpp = 2;

	// Give colour textures a mip chain. Every model and UI texture in the shipped cache
	// has one, and D3D sampled them with sampler_wrap_anisotropic; with a single level
	// the alpha-tested foliage aliases badly as the camera moves. The chain is generated
	// from level 0 on each upload, which SDL requires COLOR_TARGET usage for. The 8-bit
	// font atlas and the 16-bit A8L8 water texture are drawn 1:1 and stay single-level.
	int levels = 1;
	if(bpp == 4)
		for(int m = (w > h ? w : h); m > 1; m >>= 1)
			++levels;

	SDL_GPUTextureCreateInfo ti = {};
	ti.type = SDL_GPU_TEXTURETYPE_2D;
	ti.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
	ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | (levels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
	ti.width = (Uint32)w; ti.height = (Uint32)h;
	ti.layer_count_or_depth = 1; ti.num_levels = (Uint32)levels;
	SDL_GPUTexture* tex = SDL_CreateGPUTexture(device_, &ti);
	if(!tex) return 1;

	TextureData td;
	td.tex = tex; td.w = w; td.h = h; td.bpp = bpp; td.pitch = w * bpp;
	td.levels = levels;
	td.expand = (bpp == 1);
	td.staging.assign((size_t)w * h * bpp, 0);

	if(FileImage){
		// GetTexture writes 32-bit BGRA pixels; meaningful for the colour path.
		FileImage->GetTexture(td.staging.data(), 0, w, h);
		uploadTexture(td);
	}

	textures_[tex] = std::move(td);

	// Release any texture we previously parked in slot 0, then hand over the new
	// handle. (Single-frame; animated multi-frame textures keep only frame 0.)
	if(Texture->frameNumber() < 1)
		Texture->New(1);
	else if(SDL_GPUTexture* old = reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0))){
		auto it = textures_.find(old);
		if(it != textures_.end() && old != tex){
			SDL_ReleaseGPUTexture(device_, old);
			textures_.erase(it);
		}
	}
	Texture->GetDDSurface(0) = reinterpret_cast<IDirect3DTexture9*>(tex);
	return 0;  // 0 == success (matches the D3D contract)
}

int cSDLRenderDevice::DeleteTexture(cTexture* Texture)
{
	if(!Texture) return 0;
	// If a camera rendered into it, drop the target: the next cTexture allocated could
	// land on this address and inherit its SDL textures.
	releaseTarget(Texture);
	for(int i = 0; i < Texture->frameNumber(); ++i){
		SDL_GPUTexture* tex = reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(i));
		if(!tex) continue;
		auto it = textures_.find(tex);
		if(it != textures_.end()){
			if(device_) SDL_ReleaseGPUTexture(device_, tex);
			textures_.erase(it);
		}
		Texture->GetDDSurface(i) = 0;
	}
	return 0;
}

void* cSDLRenderDevice::LockTexture(cTexture* Texture, int& Pitch)
{
	Pitch = 0;
	if(!Texture || Texture->frameNumber() < 1) return nullptr;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it == textures_.end()) return nullptr;
	Pitch = it->second.pitch;
	return it->second.staging.data();
}

void* cSDLRenderDevice::LockTexture(cTexture* Texture, int& Pitch, Vect2i lock_min, Vect2i /*lock_size*/)
{
	Pitch = 0;
	if(!Texture || Texture->frameNumber() < 1) return nullptr;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it == textures_.end()) return nullptr;
	Pitch = it->second.pitch;
	return it->second.staging.data() + lock_min.y * it->second.pitch + lock_min.x * it->second.bpp;
}

void cSDLRenderDevice::UnlockTexture(cTexture* Texture)
{
	if(!Texture || Texture->frameNumber() < 1) return;
	auto it = textures_.find(reinterpret_cast<SDL_GPUTexture*>(Texture->GetDDSurface(0)));
	if(it != textures_.end())
		uploadTexture(it->second);
}

// ---------------------------------------------------------------------------
// Static VB/IB (the real cInterfaceRenderDevice buffer interface). Each slot is
// backed by an SDL_GPUBuffer plus a CPU staging mirror keyed on the slot pointer;
// Lock hands back the staging, Unlock uploads it. The sPtr wrappers' Destroy/dtor
// route DeleteVertex/IndexBuffer here (see RenderStub.cpp).
// ---------------------------------------------------------------------------
int cSDLRenderDevice::strideFromDeclaration(IDirect3DVertexDeclaration9* decl)
{
	// Sum of element type sizes (mirrors cD3DRender::GetSizeFromDeclaration); the
	// skinned vertex (sVertexXYZINT1) is packed, so this equals its stride (36).
	if(!decl) return 0;
	int size = 0;
	for(unsigned int i = 0; i < decl->elementCount; ++i){
		switch(decl->elements[i].Type){
		case D3DDECLTYPE_FLOAT1:   size += 4;  break;
		case D3DDECLTYPE_FLOAT2:   size += 8;  break;
		case D3DDECLTYPE_FLOAT3:   size += 12; break;
		case D3DDECLTYPE_FLOAT4:   size += 16; break;
		case D3DDECLTYPE_UBYTE4:   size += 4;  break;
		case D3DDECLTYPE_D3DCOLOR: size += 4;  break;
		case D3DDECLTYPE_SHORT2:   size += 4;  break;
		case D3DDECLTYPE_SHORT4:   size += 8;  break;
		case D3DDECLTYPE_UNUSED:   break;
		default: break;
		}
	}
	return size;
}

void cSDLRenderDevice::uploadBuffer(GpuBuffer& gb)
{
	if(!device_ || !gb.buf || gb.staging.empty()) return;
	SDL_GPUTransferBufferCreateInfo tbi = {};
	tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	tbi.size = (Uint32)gb.staging.size();
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device_, &tbi);
	if(!tb) return;
	void* map = SDL_MapGPUTransferBuffer(device_, tb, false);
	std::memcpy(map, gb.staging.data(), gb.staging.size());
	SDL_UnmapGPUTransferBuffer(device_, tb);

	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(device_);
	SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cb);
	SDL_GPUTransferBufferLocation src = {}; src.transfer_buffer = tb; src.offset = 0;
	SDL_GPUBufferRegion dst = {}; dst.buffer = gb.buf; dst.offset = 0; dst.size = (Uint32)gb.staging.size();
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);
	SDL_EndGPUCopyPass(copy);
	SDL_SubmitGPUCommandBuffer(cb);
	SDL_ReleaseGPUTransferBuffer(device_, tb);
}

void cSDLRenderDevice::CreateVertexBuffer(sPtrVertexBuffer& vb, int NumberVertex,
                                          IDirect3DVertexDeclaration9* declaration, int dynamic)
{
	DeleteVertexBuffer(vb);
	int size = strideFromDeclaration(declaration);

	sSlotVB* slot = new sSlotVB();
	slot->p = 0;
	slot->init = 1;
	slot->dynamic = (char)dynamic;
	slot->declaration = declaration;
	slot->VertexSize = (short)size;
	slot->NumberVertex = NumberVertex;
	vb.ptr = slot;

	GpuBuffer gb;
	if(device_ && NumberVertex > 0 && size > 0){
		SDL_GPUBufferCreateInfo bi = {};
		bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bi.size = (Uint32)(NumberVertex * size);
		gb.buf = SDL_CreateGPUBuffer(device_, &bi);
		gb.staging.resize((size_t)NumberVertex * size);
	}
	vbGpu_[slot] = std::move(gb);
}

void cSDLRenderDevice::DeleteVertexBuffer(sPtrVertexBuffer& vb)
{
	if(!vb.IsInit()) return;
	sSlotVB& s = *vb.ptr;
	xassert(s.init > 0);
	s.init--;
	if(s.init == 0){
		auto it = vbGpu_.find(vb.ptr);
		if(it != vbGpu_.end()){
			if(it->second.buf && device_) SDL_ReleaseGPUBuffer(device_, it->second.buf);
			vbGpu_.erase(it);
		}
		delete vb.ptr;
	}
	vb.ptr = 0;
}

void* cSDLRenderDevice::LockVertexBuffer(sPtrVertexBuffer& vb, bool /*readonly*/)
{
	if(!vb.IsInit()) return nullptr;
	auto it = vbGpu_.find(vb.ptr);
	if(it == vbGpu_.end() || it->second.staging.empty()) return nullptr;
	return it->second.staging.data();
}

void cSDLRenderDevice::UnlockVertexBuffer(sPtrVertexBuffer& vb)
{
	if(!vb.IsInit()) return;
	auto it = vbGpu_.find(vb.ptr);
	if(it != vbGpu_.end()) uploadBuffer(it->second);
}

void cSDLRenderDevice::CreateIndexBuffer(sPtrIndexBuffer& ib, int NumberPolygon, int size)
{
	DeleteIndexBuffer(ib);

	sSlotIB* slot = new sSlotIB();
	slot->p = 0;
	slot->init = 1;
	slot->NumberPolygon = NumberPolygon;
	slot->PolygonSize = size;
	ib.ptr = slot;

	GpuBuffer gb;
	if(device_ && NumberPolygon > 0 && size > 0){
		SDL_GPUBufferCreateInfo bi = {};
		bi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		bi.size = (Uint32)(NumberPolygon * size);
		gb.buf = SDL_CreateGPUBuffer(device_, &bi);
		gb.staging.resize((size_t)NumberPolygon * size);
	}
	ibGpu_[slot] = std::move(gb);
}

void cSDLRenderDevice::DeleteIndexBuffer(sPtrIndexBuffer& ib)
{
	if(!ib.IsInit()) return;
	sSlotIB& s = *ib.ptr;
	xassert(s.init > 0);
	s.init--;
	if(s.init == 0){
		auto it = ibGpu_.find(ib.ptr);
		if(it != ibGpu_.end()){
			if(it->second.buf && device_) SDL_ReleaseGPUBuffer(device_, it->second.buf);
			ibGpu_.erase(it);
		}
		delete ib.ptr;
	}
	ib.ptr = 0;
}

sPolygon* cSDLRenderDevice::LockIndexBuffer(sPtrIndexBuffer& ib, bool /*readonly*/)
{
	if(!ib.IsInit()) return nullptr;
	auto it = ibGpu_.find(ib.ptr);
	if(it == ibGpu_.end() || it->second.staging.empty()) return nullptr;
	return (sPolygon*)it->second.staging.data();
}

void cSDLRenderDevice::UnlockIndexBuffer(sPtrIndexBuffer& ib)
{
	if(!ib.IsInit()) return;
	auto it = ibGpu_.find(ib.ptr);
	if(it != ibGpu_.end()) uploadBuffer(it->second);
}

#endif // !_WIN32
