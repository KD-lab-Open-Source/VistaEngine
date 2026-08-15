// EngineViewport.h — the engine-touching half of the editor's 3D viewport.
//
// This file is compiled in the EditorEngine static library, which carries the
// engine's compile flags (EngineIncludes: _HAS_STD_BYTE=0, /FIwindows.h on
// Windows). Qt translation units must never include engine headers, so the
// engine surface the Qt side needs is exactly what this class exposes:
//   init/done     the SDL GPU device + this viewport's render window
//   resize        tell the render window its size changed
//   tick          the per-frame update (camera input state, animation)
//   drawFrame     BeginScene/EndScene/Flush — one rendered frame
//   input         mouse wheel / buttons / move / keys, mapped from Qt events
//   nativeWindow() the OS handle to wrap in the SDL foreign window
//
// Phase 3: an orbital camera (psi/theta/distance/fi + position) drives a
// Render::Camera directly — the CameraManager from Game pulls in the whole
// game module cascade, so the editor keeps its own lightweight orbit control
// (the same math CameraManager::quant uses) until the world load lands.
#pragma once

// Forward declarations only — the full engine headers stay in the .cpp.
class cInterfaceRenderDevice;
class cRenderWindow;
class Camera;
class cScene;

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

	// --- World (Phase 3b) ---

	// Load a world by name from the given worlds directory (CMainFrame's
	// OnFileOpen: vMap.load + reInitWorld, minus Universe). Returns false if
	// the world data could not be loaded.
	bool loadWorld(const char* worldsDir, const char* worldName);
	// Create a default world by name (CMainFrame's OnFileNew: vMap.create).
	bool createWorld(const char* worldsDir, const char* worldName);
	// Release the loaded world (doneScene's tile map release).
	void doneWorld();
	bool worldLoaded() const { return worldLoaded_; }

	// Per-frame update: advance the camera from the held input state.
	// dt is seconds. Called from the editor's ~60 Hz loop.
	void tick(float dt);

	// Present one frame: clear, camera, whatever the scene holds.
	void drawFrame();

	// --- Input (mapped from Qt events by RenderViewWidget) ---
	// wheelDelta: signed notch count (+up / -down). modifiers: 1=Shift, 2=Ctrl, 4=Alt.
	void mouseWheel(int wheelDelta, int modifiers);
	// button: 0=none, 1=left, 2=middle, 4=right. pressed=true on press, false on release.
	// x,y are widget-local pixels.
	void mouseButton(int button, bool pressed, int x, int y);
	void mouseMove(int x, int y);

	bool inited() const { return inited_; }

private:
	// The camera's orbit state (CameraManager::CameraCoordinate equivalent).
	struct Orbit
	{
		float psi = 0.f;       // yaw, radians
		float theta = 0.f;     // pitch, radians
		float fi = 0.f;        // roll, radians
		float distance = 512.f;
		float focus = 0.8f;    // HardwareCameraFocus
		float px = 0.f, py = 0.f, pz = 0.f;   // orbit centre
	};
	void applyCamera();

	// CGeneralView::drawGrid — the editor's terrain grid.
	void drawGrid();

	void*                nativeWindow_ = nullptr;
	cInterfaceRenderDevice* renderDevice_ = nullptr;
	cRenderWindow*       renderWindow_ = nullptr;
	cScene*              scene_ = nullptr;
	Camera*              camera_ = nullptr;
	Orbit                orbit_;
	bool                 inited_ = false;
	bool                 worldLoaded_ = false;

	// Mouse capture state (port of CGeneralView::WindowProc's statics).
	bool   mouseMiddle_ = false;
	bool   mouseLeft_ = false;
	bool   mouseRight_ = false;
	int    dragStartX_ = 0, dragStartY_ = 0;
	float  dragStartPsi_ = 0.f, dragStartTheta_ = 0.f;
	float  dragStartPx_ = 0.f, dragStartPy_ = 0.f, dragStartPz_ = 0.f;
};
