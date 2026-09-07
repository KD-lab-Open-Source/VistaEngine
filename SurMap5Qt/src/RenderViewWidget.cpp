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
#include "tools/SelectTool.h"   // the Select tool's finished box -> selection

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

int RenderViewWidget::textureStatistics(QVector<TextureStat>& rows, int& totalSize)
{
	const EngineViewport::TextureStat* stats = nullptr;
	int count = viewport_->textureStatistics(stats, totalSize);
	rows.clear();
	rows.reserve(count);
	for(int i = 0; i < count; i++){
		TextureStat row;
		row.name = QString::fromUtf8(stats[i].name ? stats[i].name : "");
		row.size = stats[i].size;
		rows.append(row);
	}
	return count;
}

bool RenderViewWidget::minimapSize(int& sizex, int& sizey) const
{
	return viewport_->minimapSize(sizex, sizey);
}

bool RenderViewWidget::minimapPixels(unsigned long* out, int sizex, int sizey)
{
	return viewport_->minimapPixels(out, sizex, sizey);
}

bool RenderViewWidget::saveMiniMapToFile()
{
	return viewport_->saveMiniMapToFile();
}

bool RenderViewWidget::cameraCenter(float& x, float& y) const
{
	return viewport_->cameraCenter(x, y);
}

void RenderViewWidget::setCameraCenter(float x, float y)
{
	viewport_->setCameraCenter(x, y);
}

bool RenderViewWidget::saveWorld(const QString& worldName)
{
	return viewport_->saveWorld(worldName.toStdString().c_str());
}

bool RenderViewWidget::saveWorld()
{
	return viewport_->saveWorld();
}

bool RenderViewWidget::canUndo() const
{
	return viewport_->canUndo();
}

bool RenderViewWidget::canRedo() const
{
	return viewport_->canRedo();
}

bool RenderViewWidget::undo()
{
	return viewport_->undo();
}

bool RenderViewWidget::redo()
{
	return viewport_->redo();
}

bool RenderViewWidget::autoLace(int laceHeightVoxels, float angleRadians)
{
	return viewport_->autoLace(laceHeightVoxels, angleRadians);
}

bool RenderViewWidget::rebuildWorld()
{
	return viewport_->rebuildWorld();
}

bool RenderViewWidget::updateSurface()
{
	return viewport_->updateSurface();
}

int RenderViewWidget::toggleTryColorDamTexture()
{
	return viewport_->toggleTryColorDamTexture();
}

void RenderViewWidget::orbitCamera(float& distance, float& theta) const
{
	viewport_->orbitCamera(distance, theta);
}

void RenderViewWidget::setOrbitCamera(float distance, float theta)
{
	viewport_->setOrbitCamera(distance, theta);
}

void RenderViewWidget::cameraState(CameraState& state) const
{
	EngineViewport::CameraState engineState;
	viewport_->cameraState(engineState);
	state.centerX = engineState.centerX;
	state.centerY = engineState.centerY;
	state.centerZ = engineState.centerZ;
	state.distance = engineState.distance;
	state.yaw = engineState.yaw;
	state.pitch = engineState.pitch;
	state.roll = engineState.roll;
}

void RenderViewWidget::setCameraState(const CameraState& state)
{
	EngineViewport::CameraState engineState;
	engineState.centerX = state.centerX;
	engineState.centerY = state.centerY;
	engineState.centerZ = state.centerZ;
	engineState.distance = state.distance;
	engineState.yaw = state.yaw;
	engineState.pitch = state.pitch;
	engineState.roll = state.roll;
	viewport_->setCameraState(engineState);
	update();
}

void RenderViewWidget::fitCameraToWorld()
{
	viewport_->fitCameraToWorld();
	update();
}

void RenderViewWidget::setGridVisible(bool visible)
{
	viewport_->setGridVisible(visible);
}

bool RenderViewWidget::gridVisible() const
{
	return viewport_->gridVisible();
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

	// The Select tool finalizes a drag box / click in onLMBUp; the engine-side
	// selection lives in the viewport (the tool itself is engine-free), so turn
	// the tool's finished box into a real selection here — CSurToolSelect::
	// onLMBUp -> unitHoverAll / selectByScreenRectangle.
	if(event->button() == Qt::LeftButton && viewport_->worldLoaded()){
		if(SelectTool* select = dynamic_cast<SelectTool*>(tools_->currentTool())){
			const ToolVec2 start = select->boxStart();
			const ToolVec2 end = select->boxEnd();
			const bool isClick = !select->isDragging() &&
			                     start.x == end.x && start.y == end.y;
			if(isClick){
				// Plain click: replace; Shift = add; Ctrl = toggle.
				int mode = 0;
				if(event->modifiers() & Qt::ControlModifier) mode = 1;
				else if(event->modifiers() & Qt::ShiftModifier) mode = 2;
				selectObjectAt(end.x, end.y, mode);
			}
			else{
				selectObjectsInRect(start.x, start.y, end.x, end.y);
			}
		}
	}
	event->accept();
}

void RenderViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	const ToolVec2 pos{ (int)event->position().x(), (int)event->position().y() };
	const ToolVec3 world = worldAt(event->position().toPoint());
	mouseWorld_ = world;
	mouseWorldValid_ = viewport_->worldLoaded();
	tools_->onTrackingMouse(world, pos);
	if(viewport_->inited())
		viewport_->mouseMove(pos.x, pos.y);
	event->accept();
}

bool RenderViewWidget::lastMouseWorld(float& x, float& y, float& z) const
{
	if(!mouseWorldValid_)
		return false;
	x = mouseWorld_.x; y = mouseWorld_.y; z = mouseWorld_.z;
	return true;
}

bool RenderViewWidget::terrainInfoAt(float x, float y, QString& surfName,
                                     int& altVox, int& approxAlt, int& waterZ) const
{
	char name[64];
	if(!viewport_->terrainInfoAt(x, y, name, (int)sizeof(name), altVox, approxAlt, waterZ))
		return false;
	surfName = QString::fromUtf8(name);
	return true;
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
		if(event->key() == Qt::Key_Delete){
			if(!tools_->onDelete())
				deleteSelectedObjects();   // SelectTool has no engine; the view deletes
		}
		QWidget::keyPressEvent(event);
	}
	event->accept();
}

// --- Object selection (SelectionUtil ports; forward to the viewport) ---

bool RenderViewWidget::selectObjectAt(int screenX, int screenY, int mode)
{
	const bool changed = viewport_ && viewport_->selectObjectAt(screenX, screenY, mode);
	if(changed){
		update();
		emit selectionChanged();
	}
	return changed;
}

bool RenderViewWidget::selectObjectsInRect(int x0, int y0, int x1, int y1)
{
	const bool changed = viewport_ && viewport_->selectObjectsInRect(x0, y0, x1, y1);
	if(changed){
		update();
		emit selectionChanged();
	}
	return changed;
}

void RenderViewWidget::deleteSelectedObjects()
{
	if(viewport_)
		viewport_->deleteSelectedObjects();
	update();
	emit selectionChanged();
}

int RenderViewWidget::selectedObjectsCount()
{
	return viewport_ ? viewport_->selectedObjectsCount() : 0;
}
