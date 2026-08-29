// RenderViewWidget.cpp — see header.

#include "RenderViewWidget.h"

#include <algorithm>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include "editor/EngineViewport.h"
#include "editor/EditorTool.h"
#include "tools/ToolManager.h"

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

// ToolAuxPainter over QPainter — the tools' overlay is drawn on top of the
// rendered frame in paintEvent, screen-space widget pixels.
class QtAuxPainter : public ToolAuxPainter
{
public:
	explicit QtAuxPainter(QPainter& painter) : painter_(painter) {}

	void drawLine2D(const ToolVec2& a, const ToolVec2& b, unsigned colorARGB) override
	{
		painter_.setPen(QColor::fromRgba(colorARGB));
		painter_.drawLine(a.x, a.y, b.x, b.y);
	}
	void drawRect2D(const ToolVec2& a, const ToolVec2& b, unsigned colorARGB) override
	{
		QPen pen(QColor::fromRgba(colorARGB));
		pen.setStyle(Qt::DashLine);
		painter_.setPen(pen);
		painter_.setBrush(Qt::NoBrush);
		painter_.drawRect(QRect(QPoint(a.x, a.y), QPoint(b.x, b.y)));
	}
	void drawCircle2D(const ToolVec2& center, int radius, unsigned colorARGB) override
	{
		QPen pen(QColor::fromRgba(colorARGB));
		pen.setStyle(Qt::DashLine);
		painter_.setPen(pen);
		painter_.setBrush(Qt::NoBrush);
		painter_.drawEllipse(QPoint(center.x, center.y), radius, radius);
	}

private:
	QPainter& painter_;
};
}

RenderViewWidget::RenderViewWidget(QWidget* parent)
	: QWidget(parent)
	, viewport_(new EngineViewport)
	, tools_(new ToolManager)
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
	delete tools_;
	delete viewport_;
}

bool RenderViewWidget::loadWorld(const QString& worldsDir, const QString& worldName)
{
	return viewport_->loadWorld(worldsDir.toStdString().c_str(), worldName.toStdString().c_str());
}

bool RenderViewWidget::createWorld(const QString& worldsDir, const QString& worldName)
{
	return viewport_->createWorld(worldsDir.toStdString().c_str(), worldName.toStdString().c_str());
}

bool RenderViewWidget::worldLoaded() const
{
	return viewport_->worldLoaded();
}

bool RenderViewWidget::isReady() const
{
	return viewport_->inited();
}

bool RenderViewWidget::mapSize(int& hSize, int& vSize) const
{
	return viewport_->mapSize(hSize, vSize);
}

bool RenderViewWidget::mapCreationParams(int& hSizePower, int& vSizePower,
                                         int& createWorldMetod, int& initialHeight) const
{
	return viewport_->mapCreationParams(hSizePower, vSizePower, createWorldMetod, initialHeight);
}

bool RenderViewWidget::worldHeightHistogram(int out[256], int& minVx, int& maxVx)
{
	return viewport_->worldHeightHistogram(out, minVx, maxVx);
}

float RenderViewWidget::changeTotalWorldParam(int deltaVx, float kScale,
                                              const Editor::MapChangeParams& params)
{
	return viewport_->changeTotalWorldParam(deltaVx, kScale, params);
}

bool RenderViewWidget::reinitWorld()
{
	return viewport_->reinitWorld();
}

QString RenderViewWidget::worldName() const
{
	return QString::fromUtf8(viewport_->worldName());
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
	tools_->quant(0.016f);
	update();
}

void RenderViewWidget::paintEvent(QPaintEvent* /*event*/)
{
	if(!viewport_->inited())
		initRenderDevice();
	viewport_->drawFrame();

	// The tools' aux overlay (selection box, transform axis, cursor circle)
	// draws on top of the rendered frame, exactly the original's
	// onDrawAuxData pass after the 3D scene.
	QPainter painter(this);
	QtAuxPainter aux(painter);
	tools_->currentTool()->onDrawAuxData(aux);
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
	const ToolVec2 pos{ (int)event->position().x(), (int)event->position().y() };
	const ToolVec3 world = worldAt(event->position().toPoint());
	// The current tool sees the press first (CGeneralView: tool's onLMBDown
	// decides whether the camera may pan/drag); unhandled -> the viewport.
	const bool handled =
		(event->button() == Qt::LeftButton)  ? tools_->onLMBDown(world, pos) :
		(event->button() == Qt::RightButton) ? tools_->onRMBDown(world, pos) : false;
	if(!handled && viewport_->inited())
		viewport_->mouseButton(qtButtonToEngine(event->button()), true, pos.x, pos.y);
	setFocus();
	event->accept();
}

void RenderViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
	const ToolVec2 pos{ (int)event->position().x(), (int)event->position().y() };
	const ToolVec3 world = worldAt(event->position().toPoint());
	const bool handled =
		(event->button() == Qt::LeftButton)  ? tools_->onLMBUp(world, pos) :
		(event->button() == Qt::RightButton) ? tools_->onRMBUp(world, pos) : false;
	if(!handled && viewport_->inited())
		viewport_->mouseButton(qtButtonToEngine(event->button()), false, pos.x, pos.y);
	event->accept();
}

void RenderViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	const ToolVec2 pos{ (int)event->position().x(), (int)event->position().y() };
	const ToolVec3 world = worldAt(event->position().toPoint());
	tools_->onTrackingMouse(world, pos);
	if(viewport_->inited())
		viewport_->mouseMove(pos.x, pos.y);
	event->accept();
}

// Ray-cast the widget-local point into the terrain (viewport_->
// screenPointToGround, the CoordScr2vMap port). Returns the world point, or
// the origin when the world is not loaded or the ray misses.
ToolVec3 RenderViewWidget::worldAt(const QPoint& pos) const
{
	ToolVec3 world{ 0, 0, 0 };
	if(viewport_ && viewport_->screenPointToGround(pos.x(), pos.y(),
	                                               world.x, world.y, world.z))
		return world;
	return ToolVec3{ 0, 0, 0 };
}

void RenderViewWidget::keyPressEvent(QKeyEvent* event)
{
	// The current tool's onKeyDown first (CGeneralView's WM_KEYDOWN tried the
	// tool, then fell back to the camera). Escape/Delete are the common ones.
	if(!tools_->onKeyDown(event->key(), event->modifiers() & Qt::ShiftModifier,
	                      event->modifiers() & Qt::ControlModifier,
	                      event->modifiers() & Qt::AltModifier)){
		if(event->key() == Qt::Key_Delete)
			tools_->onDelete();
		QWidget::keyPressEvent(event);
	}
	event->accept();
}
