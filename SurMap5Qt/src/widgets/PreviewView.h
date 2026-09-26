// PreviewView.h — a native Qt widget the engine renders an editor preview
// into through its own render window (a swapchain of its own), so the preview
// never mixes with the level's 3D view.
//
// The widget owns no engine types: it attaches the native handle once it is
// shown and asks a render callback to draw each frame.

#pragma once

#include <QWidget>
#include <QPoint>

#include <functional>

class QTimer;

class PreviewView : public QWidget
{
	Q_OBJECT
public:
	using AttachFn = std::function<bool(void*)>;
	using RenderFn = std::function<bool(int, int)>;
	using DetachFn = std::function<void()>;

	explicit PreviewView(QWidget* parent = nullptr);

	// The engine callbacks. Attach is called with the widget's native handle
	// (HWND) on first show; Render is called on every repaint with the widget's
	// pixel size; Detach on hide/destruction.
	void setPreviewFunctions(AttachFn attach, RenderFn render, DetachFn detach);

	QSize sizeHint() const override { return QSize(520, 380); }

signals:
	// Drag with the left button in the preview; wheel zooms. Only the effects
	// editor connects them (its preview camera is orbitable).
	void orbited(float dPsi, float dTheta);
	void zoomed(float factor);

protected:
	void paintEvent(QPaintEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void ensureAttached();

	AttachFn attach_;
	RenderFn render_;
	DetachFn detach_;
	QTimer* timer_ = nullptr;
	bool attached_ = false;
	QPoint lastMouse_;
};
