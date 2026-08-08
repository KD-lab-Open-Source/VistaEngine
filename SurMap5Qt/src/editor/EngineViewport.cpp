// EngineViewport.cpp — see header. Compiled with the engine's flags.

#include "EngineViewport.h"

// The engine's headers assume the old StdAfx preamble: `using namespace std`
// and <vector>/<string> (IRenderDevice.h names `vector`/`string` unqualified).
#include <vector>
#include <string>
using namespace std;

// Engine headers — safe here because EditorEngine compiles with EngineIncludes.
#include "Render/inc/IRenderDevice.h"

EngineViewport::EngineViewport() = default;

EngineViewport::~EngineViewport()
{
	done();
}

bool EngineViewport::init(int width, int height)
{
	if(inited_)
		return true;
	if(!nativeWindow_)
		return false;

	// The engine's one entry point into the renderer (Render/RenderStub.cpp):
	// builds gb_VisGeneric + the SDL GPU device.
	if(!gb_RenderDevice)
		CreateIRenderDevice(false);
	if(!gb_RenderDevice)
		return false;

	// With no global SDL window (Qt owns the windows), the device is created
	// without a swapchain; createRenderWindow() below wraps this viewport's
	// native handle and becomes the (global) render window.
	if(!gb_RenderDevice->inited() &&
	   !gb_RenderDevice->Initialize(width, height, RENDERDEVICE_MODE_WINDOW, nullptr, 0, nullptr))
		return false;

	renderWindow_ = gb_RenderDevice->createRenderWindow((HWND)nativeWindow_);
	if(!renderWindow_)
		return false;

	gb_RenderDevice->selectRenderWindow(renderWindow_);
	inited_ = true;
	return true;
}

void EngineViewport::done()
{
	if(!inited_)
		return;
	if(gb_RenderDevice){
		gb_RenderDevice->selectRenderWindow(0);
		if(renderWindow_)
			gb_RenderDevice->DeleteRenderWindow(renderWindow_);
	}
	renderWindow_ = nullptr;
	inited_ = false;
}

void EngineViewport::resize()
{
	// The swapchain tracks the native window's size; the SDL backend queries it
	// in BeginScene, so nothing to force here. Kept as the resize hook where
	// Phase 3 will re-arm the camera.
	if(renderWindow_)
		renderWindow_->ChangeSize();
}

void EngineViewport::drawFrame()
{
	if(!inited_ || !gb_RenderDevice)
		return;

	// Fill() arms the clear, BeginScene acquires the swapchain texture,
	// EndScene submits. Phase 3 draws the engine scene in between.
	gb_RenderDevice->Fill(32, 48, 64, 255);   // dark blue-grey viewport
	gb_RenderDevice->BeginScene();
	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
}
