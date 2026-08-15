// RenderViewWidget.cpp — see header.

#include "RenderViewWidget.h"

#include <algorithm>

#include <QMouseEvent>
#include <QWheelEvent>

#include "editor/EngineViewport.h"

// Qt mouse button -> EngineViewport button mask (1=left, 2=middle, 4=right),
// matching the WM_* MK_* values CGeneralView's WindowProc used.
namespace {
int qtButtonToEngine(Qt::MouseButton button)
{
	switch(button){
	case Qt::LeftButton:   return 1;
	case Qt::MiddleButton: return 2;
	case Qt::RightButton:  return 4;
	default:               return 0;
	}
}
}

RenderViewWidget::RenderViewWidget(QWidget* parent)
	: QWidget(parent)
	, viewport_(new EngineViewport)
{
	// The engine's SDL GPU device claims a swapchain on a window created around
	// this widget's native handle (EngineViewport::init). It must therefore
	// always have one, and the engine draws straight into it.
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	setAttribute(Qt::WA_PaintOnScreen);
	setFocusPolicy(Qt::StrongFocus);
}

RenderViewWidget::~RenderViewWidget()
{
	doneRenderDevice();
	delete viewport_;
}

bool RenderViewWidget::initRenderDevice()
{
	if(viewport_->inited())
		return true;
	viewport_->setNativeWindow((void*)winId());
	return viewport_->init(std::max(1, width()), std::max(1, height()));
}

void RenderViewWidget::doneRenderDevice()
{
	viewport_->done();
}

void RenderViewWidget::tick()
{
	// The editor's per-frame update, then repaint (MFC OnIdle equivalent).
	// A fixed 16 ms step is close enough for camera/editor animation.
	viewport_->tick(0.016f);
	update();
}

void RenderViewWidget::paintEvent(QPaintEvent* /*event*/)
{
	if(!viewport_->inited())
		initRenderDevice();
	viewport_->drawFrame();
}

void RenderViewWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	viewport_->resize();
}

void RenderViewWidget::wheelEvent(QWheelEvent* event)
{
	if(!viewport_->inited())
		return;
	int modifiers = 0;
	if(event->modifiers() & Qt::ShiftModifier) modifiers |= 1;
	if(event->modifiers() & Qt::ControlModifier) modifiers |= 2;
	if(event->modifiers() & Qt::AltModifier) modifiers |= 4;
	// angleDelta().y() is in eighths of a degree; one notch is 120.
	const int delta = event->angleDelta().y();
	const int notches = delta / 120 + (delta % 120 != 0 ? (delta > 0 ? 1 : -1) : 0);
	viewport_->mouseWheel(notches != 0 ? notches : (delta > 0 ? 1 : -1), modifiers);
	event->accept();
}

void RenderViewWidget::mousePressEvent(QMouseEvent* event)
{
	if(viewport_->inited())
		viewport_->mouseButton(qtButtonToEngine(event->button()), true, event->position().x(), event->position().y());
	setFocus();
	event->accept();
}

void RenderViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if(viewport_->inited())
		viewport_->mouseButton(qtButtonToEngine(event->button()), false, event->position().x(), event->position().y());
	event->accept();
}

void RenderViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	if(viewport_->inited())
		viewport_->mouseMove(event->position().x(), event->position().y());
	event->accept();
}
