// TriggerGraphView.cpp — see header.

#include "TriggerGraphView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <QApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include "RenderViewWidget.h"

// TriggerLink::colors (TriggerEditor/TriggerExport.cpp:306) — the link
// palette, in ColorType order (STRATEGY_RED/GREEN/BLUE/YELLOW/COLOR_0..5).
static QColor linkColor(int colorType)
{
	static const QColor kColors[] = {
		QColor(0, 255, 0),     // GREEN
		QColor(0, 0, 255),     // BLUE
		QColor(255, 0, 0),     // RED
		QColor(255, 255, 0),   // YELLOW
		QColor(0, 255, 255),   // CYAN
		QColor(255, 0, 255),   // MAGENTA
		QColor(255, 128, 0),
		QColor(198, 198, 0),
		QColor(100, 0, 0),
		QColor(0, 100, 0),
	};
	const int n = (int)(sizeof(kColors) / sizeof(kColors[0]));
	if(colorType < 0 || colorType >= n)
		return Qt::black;
	return kColors[colorType];
}

TriggerGraphView::TriggerGraphView(RenderViewWidget* view, QWidget* parent)
	: QWidget(parent)
	, view_(view)
{
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	setMinimumSize(320, 240);
}

void TriggerGraphView::reload()
{
	triggers_.clear();
	links_.clear();
	if(view_){
		view_->triggerList(triggers_);
		view_->triggerLinkList(links_);
	}
	// Drop selections that no longer exist.
	for(auto it = selected_.begin(); it != selected_.end();){
		if(*it < 0 || *it >= (int)triggers_.size())
			it = selected_.erase(it);
		else
			++it;
	}
	if(selLinkParent_ >= (int)triggers_.size() || selLinkChild_ >= (int)triggers_.size()){
		selLinkParent_ = -1;
		selLinkChild_ = -1;
	}
	update();
}

void TriggerGraphView::selectTrigger(int index, bool add)
{
	if(!add)
		selected_.clear();
	if(index >= 0 && index < (int)triggers_.size())
		selected_.insert(index);
	selLinkParent_ = -1;
	selLinkChild_ = -1;
	emit selectionChanged();
	update();
}

void TriggerGraphView::clearSelection()
{
	selected_.clear();
	selLinkParent_ = -1;
	selLinkChild_ = -1;
	emit selectionChanged();
	update();
}

void TriggerGraphView::centerOn(float gx, float gy)
{
	origin_ = QPointF(gx - width() / (2.f * zoom_), gy - height() / (2.f * zoom_));
	update();
}

void TriggerGraphView::visibleGraphRect(float& x0, float& y0, float& x1, float& y1) const
{
	const QPointF tl = screenToGraph(QPoint(0, 0));
	const QPointF br = screenToGraph(QPoint(width(), height()));
	x0 = tl.x();
	y0 = tl.y();
	x1 = br.x();
	y1 = br.y();
}

QPointF TriggerGraphView::graphToScreen(float gx, float gy) const
{
	return QPointF((gx - origin_.x()) * zoom_, (gy - origin_.y()) * zoom_);
}

QPointF TriggerGraphView::screenToGraph(const QPoint& p) const
{
	return QPointF(origin_.x() + p.x() / zoom_, origin_.y() + p.y() / zoom_);
}

QRectF TriggerGraphView::triggerRect(int index) const
{
	const TriggerInfo& t = triggers_[(size_t)index];
	const float gx = (float)t.cellX * stepX();
	const float gy = (float)t.cellY * stepY();
	const QPointF tl = graphToScreen(gx, gy);
	return QRectF(tl.x(), tl.y(), kSizeX * zoom_, kSizeY * zoom_);
}

int TriggerGraphView::triggerAt(const QPoint& p) const
{
	for(int i = (int)triggers_.size() - 1; i >= 0; --i){
		if(triggerRect(i).contains(p))
			return i;
	}
	return -1;
}

bool TriggerGraphView::linkAt(const QPoint& p, int& parentOut, int& childOut) const
{
	// Hit-test links as segments between node centres (with the stored
	// offsets), like TriggerView::findLink did on the HDC geometry.
	const float tol = 6.f;
	for(const TriggerLinkInfo& l : links_){
		if(l.parent < 0 || l.parent >= (int)triggers_.size() ||
		   l.child < 0 || l.child >= (int)triggers_.size())
			continue;
		const TriggerInfo& pt = triggers_[(size_t)l.parent];
		const TriggerInfo& ct = triggers_[(size_t)l.child];
		const QPointF a = graphToScreen(
			(float)pt.cellX * stepX() + kSizeX * 0.5f + l.parentOffsetX,
			(float)pt.cellY * stepY() + kSizeY * 0.5f + l.parentOffsetY);
		const QPointF b = graphToScreen(
			(float)ct.cellX * stepX() + kSizeX * 0.5f + l.childOffsetX,
			(float)ct.cellY * stepY() + kSizeY * 0.5f + l.childOffsetY);
		const QPointF ab = b - a;
		const float len2 = (float)(ab.x() * ab.x() + ab.y() * ab.y());
		if(len2 < 1.f)
			continue;
		const QPointF ap(p.x() - a.x(), p.y() - a.y());
		float t = (float)(ap.x() * ab.x() + ap.y() * ab.y()) / len2;
		t = std::max(0.f, std::min(1.f, t));
		const QPointF proj(a.x() + ab.x() * t, a.y() + ab.y() * t);
		const float dx = (float)p.x() - (float)proj.x();
		const float dy = (float)p.y() - (float)proj.y();
		if(dx * dx + dy * dy <= tol * tol){
			parentOut = l.parent;
			childOut = l.child;
			return true;
		}
	}
	return false;
}

void TriggerGraphView::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	const QRect r = rect();
	painter.fillRect(r, Qt::white);

	// Grid (TriggerView::onRedraw dotted lines at SIZE_X/OFFSET_X, SIZE_Y).
	const QPointF tl = screenToGraph(QPoint(0, 0));
	const QPointF br = screenToGraph(QPoint(r.width(), r.height()));
	painter.setPen(QPen(QColor(192, 192, 192), 1, Qt::DotLine));
	const float startY = floorf((float)tl.y() / kSizeY) * kSizeY;
	for(float gy = startY; gy <= br.y(); gy += kSizeY){
		const QPointF a = graphToScreen(tl.x(), gy);
		const QPointF b = graphToScreen(br.x(), gy);
		painter.drawLine(QPointF(a.x(), a.y()), QPointF(b.x(), b.y()));
	}
	const float startX = floorf((float)tl.x() / stepX()) * stepX();
	for(float gx = startX; gx <= br.x(); gx += stepX()){
		QPointF a = graphToScreen(gx, tl.y());
		QPointF b = graphToScreen(gx, br.y());
		painter.drawLine(a, b);
		a = graphToScreen(gx + kSizeX, tl.y());
		b = graphToScreen(gx + kSizeX, br.y());
		painter.drawLine(a, b);
	}

	// Links under the nodes (TriggerView drew links first, then triggers).
	for(const TriggerLinkInfo& l : links_){
		if(l.parent < 0 || l.parent >= (int)triggers_.size() ||
		   l.child < 0 || l.child >= (int)triggers_.size())
			continue;
		const TriggerInfo& pt = triggers_[(size_t)l.parent];
		const TriggerInfo& ct = triggers_[(size_t)l.child];
		const QPointF a = graphToScreen(
			(float)pt.cellX * stepX() + kSizeX * 0.5f + l.parentOffsetX,
			(float)pt.cellY * stepY() + kSizeY * 0.5f + l.parentOffsetY);
		const QPointF b = graphToScreen(
			(float)ct.cellX * stepX() + kSizeX * 0.5f + l.childOffsetX,
			(float)ct.cellY * stepY() + kSizeY * 0.5f + l.childOffsetY);
		const bool selected = (l.parent == selLinkParent_ && l.child == selLinkChild_);
		QPen pen(linkColor(l.colorType), l.autoRestarted ? 4 : 1);
		if(selected)
			pen.setColor(Qt::black);
		painter.setPen(pen);
		painter.drawLine(a, b);
		if(selected){
			painter.setBrush(Qt::black);
			painter.drawRect(QRectF(a.x() - 3, a.y() - 3, 6, 6));
			painter.drawRect(QRectF(b.x() - 3, b.y() - 3, 6, 6));
		}
	}

	// Nodes.
	QFont font = painter.font();
	font.setPointSize(8);
	painter.setFont(font);
	for(int i = 0; i < (int)triggers_.size(); ++i){
		const QRectF rc = triggerRect(i);
		if(!rc.intersects(QRectF(r)))
			continue;
		const TriggerInfo& t = triggers_[(size_t)i];
		const QColor fill((int)((t.colorRGBA >> 16) & 0xFF),
		                  (int)((t.colorRGBA >> 8) & 0xFF),
		                  (int)(t.colorRGBA & 0xFF));
		painter.setBrush(fill);
		painter.setPen(selected_.count(i) ? QPen(Qt::red, 2) : QPen(Qt::black, 1));
		painter.drawRect(rc);
		painter.setPen(Qt::black);
		QString label = QString::fromStdString(t.name);
		if(!t.actionType.empty())
			label += QString(" [%1]").arg(QString::fromStdString(t.actionType).section('\\', -1));
		painter.drawText(rc.adjusted(3, 1, -3, -1), Qt::AlignLeft | Qt::AlignVCenter, label);
	}

	// Area selection rubber band.
	if(areaSelecting_){
		painter.setPen(QPen(QColor(128, 128, 128), 1, Qt::DashLine));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(QRect(areaStart_, areaEnd_).normalized());
	}

	// Link being created (TriggerView::creatingLink_ line).
	if(linking_){
		const TriggerInfo& pt = triggers_[(size_t)linkParent_];
		const QPointF a = graphToScreen(
			(float)pt.cellX * stepX() + kSizeX * 0.5f,
			(float)pt.cellY * stepY() + kSizeY * 0.5f);
		painter.setPen(QPen(linkColor(nextLinkColor_), nextLinkAutoRestarted_ ? 4 : 1));
		painter.drawLine(a, linkCurrent_);
	}
}

void TriggerGraphView::mousePressEvent(QMouseEvent* event)
{
	setFocus();
	if(!view_ || !view_->triggerSessionOpenNow())
		return;
	if(event->button() == Qt::MiddleButton){
		spacePanning_ = true;
		dragStart_ = event->pos();
		dragOrigin_ = origin_;
		return;
	}
	if(event->button() != Qt::LeftButton)
		return;
	const int hit = triggerAt(event->pos());
	if(linking_){
		finishLinkDrag(event->pos());
		return;
	}
	if(hit >= 0){
		const bool add = (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) != 0;
		if(!add && selected_.count(hit) == 0)
			selectTrigger(hit, false);
		else if(add){
			if(selected_.count(hit))
				selected_.erase(hit);
			else
				selected_.insert(hit);
			emit selectionChanged();
			update();
		}
		dragging_ = true;
		dragStart_ = event->pos();
		dragMoved_.clear();
	}
	else{
		int lp = -1, lc = -1;
		if(linkAt(event->pos(), lp, lc)){
			selLinkParent_ = lp;
			selLinkChild_ = lc;
			selected_.clear();
			emit selectionChanged();
			update();
		}
		else{
			if((event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) == 0)
				clearSelection();
			areaSelecting_ = true;
			areaStart_ = event->pos();
			areaEnd_ = event->pos();
		}
	}
}

void TriggerGraphView::mouseMoveEvent(QMouseEvent* event)
{
	if(spacePanning_){
		const QPoint d = event->pos() - dragStart_;
		origin_ = QPointF(dragOrigin_.x() - d.x() / zoom_,
		                  dragOrigin_.y() - d.y() / zoom_);
		update();
		return;
	}
	if(areaSelecting_){
		areaEnd_ = event->pos();
		update();
		return;
	}
	if(linking_){
		linkCurrent_ = event->pos();
		update();
		return;
	}
	if(dragging_ && !selected_.empty()){
		// Snap the drag to whole cells (TriggerView moved by cellIndex).
		const QPointF g0 = screenToGraph(dragStart_);
		const QPointF g1 = screenToGraph(event->pos());
		const int dx = (int)roundf(((float)g1.x() - (float)g0.x()) / stepX());
		const int dy = (int)roundf(((float)g1.y() - (float)g0.y()) / stepY());
		if(dx != 0 || dy != 0){
			for(int idx : selected_){
				const TriggerInfo& t = triggers_[(size_t)idx];
				if(view_->triggerSetCell(idx, t.cellX + dx, t.cellY + dy))
					dragMoved_.insert(idx);
			}
			dragStart_ = event->pos();
			reload();
			// Keep the selection across the reload.
			for(int idx : dragMoved_)
				selected_.insert(idx);
			emit triggersChanged();
			update();
		}
	}
}

void TriggerGraphView::mouseReleaseEvent(QMouseEvent* event)
{
	if(event->button() == Qt::MiddleButton){
		spacePanning_ = false;
		return;
	}
	if(event->button() != Qt::LeftButton)
		return;
	if(areaSelecting_){
		areaSelecting_ = false;
		const QRect box = QRect(areaStart_, event->pos()).normalized();
		const bool add = (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) != 0;
		if(!add)
			selected_.clear();
		for(int i = 0; i < (int)triggers_.size(); ++i){
			if(box.intersects(triggerRect(i).toRect()))
				selected_.insert(i);
		}
		emit selectionChanged();
		update();
		return;
	}
	if(dragging_){
		dragging_ = false;
		if(!dragMoved_.empty())
			emit triggersChanged();
		dragMoved_.clear();
	}
}

void TriggerGraphView::wheelEvent(QWheelEvent* event)
{
	// Zoom around the cursor (Viewport2D zoom semantics).
	const float factor = event->angleDelta().y() > 0 ? 1.15f : 1.f / 1.15f;
	const QPointF g = screenToGraph(event->position().toPoint());
	zoom_ = std::max(0.2f, std::min(4.f, zoom_ * factor));
	origin_ = QPointF((float)g.x() - event->position().x() / zoom_,
	                  (float)g.y() - event->position().y() / zoom_);
	update();
}

void TriggerGraphView::contextMenuEvent(QContextMenuEvent* event)
{
	if(!view_ || !view_->triggerSessionOpenNow())
		return;
	QMenu menu(this);
	const int hit = triggerAt(event->pos());
	if(hit >= 0){
		if(selected_.count(hit) == 0)
			selectTrigger(hit, (QApplication::keyboardModifiers() &
				(Qt::ShiftModifier | Qt::ControlModifier)) != 0);
		QAction* actDel = menu.addAction(tr("Delete trigger"));
		QAction* actCond = menu.addAction(tr("Condition..."));
		QAction* actAct = menu.addAction(tr("Action..."));
		QAction* actProps = menu.addAction(tr("Properties..."));
		menu.addSeparator();
		QAction* actLink = menu.addAction(tr("Create link from here"));
		QAction* chosen = menu.exec(event->globalPos());
		if(chosen == actDel){
			std::vector<int> order(selected_.begin(), selected_.end());
			std::sort(order.begin(), order.end(), std::greater<int>());
			for(int idx : order)
				view_->triggerDelete(idx);
			clearSelection();
			reload();
			emit triggersChanged();
		}
		else if(chosen == actCond)
			emit editConditions(hit);
		else if(chosen == actAct)
			emit editAction(hit);
		else if(chosen == actProps)
			emit editTrigger(hit);
		else if(chosen == actLink)
			startLinkDrag(hit, event->pos());
	}
	else{
		int lp = -1, lc = -1;
		if(linkAt(event->pos(), lp, lc)){
			QAction* actDelLink = menu.addAction(tr("Delete link"));
			if(menu.exec(event->globalPos()) == actDelLink){
				view_->triggerDeleteLink(lp, lc);
				selLinkParent_ = -1;
				selLinkChild_ = -1;
				reload();
				emit triggersChanged();
			}
		}
		else{
			QAction* actNew = menu.addAction(tr("New trigger here"));
			if(menu.exec(event->globalPos()) == actNew && view_){
				const QPointF g = screenToGraph(event->pos());
				const int cx = (int)roundf(((float)g.x() - kSizeX * 0.5f) / stepX());
				const int cy = (int)roundf(((float)g.y() - kSizeY * 0.5f) / stepY());
				// Default action type 0 (first registered Action); the user
				// changes it through the property panel afterwards.
				const int idx = view_->triggerCreate(0, "Trigger", cx, cy);
				if(idx >= 0){
					selectTrigger(idx, false);
					reload();
					emit triggersChanged();
				}
			}
		}
	}
}

void TriggerGraphView::keyPressEvent(QKeyEvent* event)
{
	if(event->key() == Qt::Key_Delete){
		if(selLinkParent_ >= 0 && selLinkChild_ >= 0 && view_){
			view_->triggerDeleteLink(selLinkParent_, selLinkChild_);
			selLinkParent_ = -1;
			selLinkChild_ = -1;
			reload();
			emit triggersChanged();
		}
		else if(!selected_.empty() && view_){
			std::vector<int> order(selected_.begin(), selected_.end());
			std::sort(order.begin(), order.end(), std::greater<int>());
			for(int idx : order)
				view_->triggerDelete(idx);
			clearSelection();
			reload();
			emit triggersChanged();
		}
		event->accept();
		return;
	}
	QWidget::keyPressEvent(event);
}

void TriggerGraphView::startLinkDrag(int parentIndex, const QPoint& p)
{
	linking_ = true;
	linkParent_ = parentIndex;
	linkCurrent_ = p;
	setMouseTracking(true);
	update();
}

void TriggerGraphView::finishLinkDrag(const QPoint& p)
{
	linking_ = false;
	const int child = triggerAt(p);
	if(child >= 0 && child != linkParent_ && view_){
		if(view_->triggerCreateLink(linkParent_, child, nextLinkColor_, nextLinkAutoRestarted_)){
			selLinkParent_ = linkParent_;
			selLinkChild_ = child;
			selected_.clear();
			emit selectionChanged();
		}
		reload();
		emit triggersChanged();
	}
	linkParent_ = -1;
	update();
}

void TriggerGraphView::moveSelected(int dxCells, int dyCells)
{
	if(!view_ || selected_.empty())
		return;
	for(int idx : selected_){
		const TriggerInfo& t = triggers_[(size_t)idx];
		view_->triggerSetCell(idx, t.cellX + dxCells, t.cellY + dyCells);
	}
	reload();
	emit triggersChanged();
}
