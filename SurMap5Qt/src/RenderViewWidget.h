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

#include <QWidget>

// The engine-touching half of the viewport lives in the EditorEngine library
// (see src/editor/EngineViewport.h) so engine headers never reach Qt code.
class EngineViewport;

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

private:
	EngineViewport* viewport_ = nullptr;
};
