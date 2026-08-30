// RenderViewWidget.h — the 3D viewport of the editor.
//
// Maps to CGeneralView (SurMap5/GeneralView.h): a plain child window that owns
// the engine's render device lifecycle (initRenderDevice / doneRenderDevice /
// createScene / reInitWorld / graphQuant / OnPaint / OnSize / input WindowProc).
//
// Phase 1: a placeholder widget painting black, so the frame works before the
// engine is linked.
//
// Phase 2: host the SDL GPU swapchain. Plan (verified against the vendored
// SDL 3.5.0):
//   - Make this widget's native handle the render target:
//       setAttribute(Qt::WA_NativeWindow); -> winId()
//   - Wrap the HWND in an SDL window:
//       SDL_CreateWindowWithProperties with
//       SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER (Windows),
//       SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER (Linux),
//       SDL_PROP_WINDOW_CREATE_COCOA_VIEW_POINTER (macOS).
//   - SDL_ClaimWindowForGPUDevice(device, window); acquire the swapchain
//     texture in paintEvent and submit there.
//   - The engine's cSDLRenderDevice currently grabs the window from the global
//     PlatformWindow::current(); it needs an InitializeWithWindow(SDL_Window*)
//     so the editor's views can each claim their own swapchain.
//   - MiniMapWindow becomes a second such widget (own scene + camera), the way
//     the original created a second render window.
//
// Input is delivered as Qt events (mouse/key/wheel) in Phase 3; the original
// WM_* switch in CGeneralView::WindowProc maps 1:1.

#pragma once

#include <QPoint>
#include <QVector>
#include <QWidget>

// The engine-touching half of the viewport lives in the EditorEngine library
// (see src/editor/EngineViewport.h) so engine headers never reach Qt code.
class EngineViewport;
// The current tool routes view input to the tool's handlers (Select/Move/
// Rotate/Scale); engine-free, see src/tools/ToolManager.h.
class ToolManager;
// ToolVec3 (the tools' plain world-coordinate triple) is a struct, so the
// worldAt helper below needs the full definition — EditorTool.h is engine-free.
#include "editor/EditorTool.h"
// MapChangeParams is engine-free (no engine headers), so it can be named by
// value in the changeTotalWorldParam signature below.
#include "editor/MapChangeParams.h"

class RenderViewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit RenderViewWidget(QWidget* parent = nullptr);
	~RenderViewWidget() override;

	// Engine-side device init (Phase 2). Called once the widget has a native
	// window handle (winId()).
	bool initRenderDevice();
	void doneRenderDevice();

	// The tool manager (MainWindow's tool toolbar switches into it).
	ToolManager* tools() { return tools_; }

	// The engine-side viewport (world load, camera). EngineViewport is a
	// forward declaration here — Qt code must not see engine headers; the
	// world-load wrappers below keep MainWindow engine-free.
	EngineViewport* viewport() { return viewport_; }

	// World load/create wrappers (Phase 3b): forward to the engine viewport.
	// MainWindow calls these instead of EngineViewport methods directly, so it
	// never needs the engine headers.
	bool loadWorld(const QString& worldsDir, const QString& worldName);
	bool createWorld(const QString& worldsDir, const QString& worldName);
	bool worldLoaded() const;
	// True once the render device exists (first paint happened).
	bool isReady() const;

	// --- World data (U4 dialogs). Forwarders to EngineViewport so MainWindow
	// stays engine-free. ---
	bool mapSize(int& hSize, int& vSize) const;
	bool mapCreationParams(int& hSizePower, int& vSizePower,
	                       int& createWorldMetod, int& initialHeight) const;
	bool worldHeightHistogram(int out[256], int& minVx, int& maxVx);
	float changeTotalWorldParam(int deltaVx, float kScale,
	                            const Editor::MapChangeParams& params);
	// Rebuild the terrain scene after an in-place terrain mutation
	// (EngineViewport::reinitWorld).
	bool reinitWorld();
	// The loaded world's name (vMap.getWorldName via the viewport), empty when
	// nothing is loaded.
	QString worldName() const;

	// Texture statistics (U5): rows of name+size + the summed size, forwarded
	// from the engine's texture library (EngineViewport::textureStatistics).
	struct TextureStat {
		QString name;
		int size = 0;
	};
	int textureStatistics(QVector<TextureStat>& rows, int& totalSize);

	// --- Minimap (U6). Forwarders to EngineViewport, so MainWindow and the
	// minimap panel stay engine-free. ---
	bool minimapSize(int& sizex, int& sizey) const;
	// `out` receives sizex*sizey 0xFF000000|RGB pixels (the minimap image).
	bool minimapPixels(unsigned long* out, int sizex, int sizey);
	// Write map.tga into the world's directory (OnFileSaveminimaptoworld).
	bool saveMiniMapToFile();
	bool cameraCenter(float& x, float& y) const;
	void setCameraCenter(float x, float y);

	// --- World save (U7). Forwarders to EngineViewport, so MainWindow stays
	// engine-free. ---
	// CMainFrame::save (vMap.save under the given name).
	bool saveWorld(const QString& worldName);
	// Save under the world's current name (CMainFrame::OnFileSave).
	bool saveWorld();
	// The current orbit distance/theta, for the Save-Camera-As-Default slot.
	void orbitCamera(float& distance, float& theta) const;
	// Apply a saved camera default (distance + theta) to the orbit.
	void setOrbitCamera(float distance, float theta);
	// Grid visibility (surMapOptions.enableGrid_): gates the viewport's grid.
	void setGridVisible(bool visible);
	bool gridVisible() const;

public slots:
	// Called ~60 Hz from MainWindow::universeQuant (MFC OnIdle equivalent).
	void tick();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	// Ray-cast the widget-local point into the terrain (viewport_->
	// screenPointToGround, the CoordScr2vMap port). Returns the world point,
	// or the origin when the world is not loaded or the ray misses.
	ToolVec3 worldAt(const QPoint& pos) const;

	EngineViewport* viewport_ = nullptr;
	ToolManager* tools_ = nullptr;
};
