// EngineViewport.h — the engine-touching half of the editor's 3D viewport.
//
// This file is compiled in the EditorEngine static library, which carries the
// engine's compile flags (EngineIncludes: _HAS_STD_BYTE=0, /FIwindows.h on
// Windows). Qt translation units must never include engine headers, so the
// engine surface the Qt side needs is exactly what this class exposes:
//   init/done    the SDL GPU device + this viewport's render window
//   resize       tell the render window its size changed
//   drawFrame    BeginScene/EndScene/Flush — the one-frame render, clear only
//                for now (Phase 3 adds the scene + camera from CGeneralView)
//   nativeWindow() the OS handle to wrap in the SDL foreign window
#pragma once

// Forward declarations only — the full engine headers stay in the .cpp.
class cInterfaceRenderDevice;
class cRenderWindow;

class EngineViewport
{
public:
	EngineViewport();
	~EngineViewport();

	// The OS window handle (HWND on Windows) this viewport wraps.
	void* nativeWindow() const { return nativeWindow_; }
	void setNativeWindow(void* hwnd) { nativeWindow_ = hwnd; }

	// Create the render device once, then the render window on nativeWindow_.
	// Returns false if the device or window could not be created.
	bool init(int width, int height);
	void done();

	// The render window's size changed (Qt resize event).
	void resize();

	// Present one frame. Phase 3 will draw the engine scene here; for now the
	// frame is a clear of the swapchain.
	void drawFrame();

	bool inited() const { return inited_; }

private:
	void*                nativeWindow_ = nullptr;
	cInterfaceRenderDevice* renderDevice_ = nullptr;
	cRenderWindow*       renderWindow_ = nullptr;
	bool                 inited_ = false;
};
