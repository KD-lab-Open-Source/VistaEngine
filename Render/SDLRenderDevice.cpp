// SDL GPU render device — slice 1b (lifecycle + window clear). See header.
#include "StdAfxRD.h"
#include "SDLRenderDevice.h"

#ifndef _WIN32

#include <SDL3/SDL.h>
#include <cstdio>

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
cSDLRenderDevice::cSDLRenderDevice() {}

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

	if(device_){
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
	clearColor_[0] = r / 255.f;
	clearColor_[1] = g / 255.f;
	clearColor_[2] = b / 255.f;
	clearColor_[3] = a / 255.f;
	hasClear_ = true;
	return 0;
}

int cSDLRenderDevice::BeginScene()
{
	if(!device_) return -1;
	if(bActiveScene_) return 1;

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

	SDL_GPUColorTargetInfo target = {};
	target.texture = swapchainTexture_;
	target.clear_color.r = clearColor_[0];
	target.clear_color.g = clearColor_[1];
	target.clear_color.b = clearColor_[2];
	target.clear_color.a = clearColor_[3];
	target.load_op = hasClear_ ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
	target.store_op = SDL_GPU_STOREOP_STORE;

	renderPass_ = SDL_BeginGPURenderPass(commandBuffer_, &target, 1, nullptr);
	hasClear_ = false;
	bActiveScene_ = true;

	NumberPolygon = 0;
	NumDrawObject = 0;
	NumberTilemapPolygon = 0;
	return 0;
}

int cSDLRenderDevice::EndScene()
{
	if(!bActiveScene_) return 1;
	if(renderPass_){
		SDL_EndGPURenderPass(renderPass_);
		renderPass_ = nullptr;
	}
	bActiveScene_ = false;
	return 0;
}

int cSDLRenderDevice::Flush()
{
	if(bActiveScene_)
		EndScene();
	if(commandBuffer_){
		SDL_SubmitGPUCommandBuffer(commandBuffer_);
		commandBuffer_ = nullptr;
	}
	swapchainTexture_ = nullptr;
	return 0;
}

#endif // !_WIN32
