// TriggerMiniMapPanel.cpp — see header.

#include "TriggerMiniMapPanel.h"

#include <algorithm>
#include <set>

#include <QMouseEvent>
#include <QPainter>

#include "RenderViewWidget.h"
#include "TriggerGraphView.h"

TriggerMiniMapPanel::TriggerMiniMapPanel(RenderViewWidget* view, TriggerGraphView* graph,
                                         QWidget* parent)
	: QWidget(parent)
	, view_(view)
	, graph_(graph)
{
	setMinimumSize(120, 90);
	setFocusPolicy(Qt::NoFocus);
}

void TriggerMiniMapPanel::reload()
{
	triggers_.clear();
	if(view_)
		view_->triggerList(triggers_);
	// Fit-all bounds over node rects (TriggerMiniMap::onRedraw area + border).
	const float border = 10.f;
	bool first = true;
	for(const TriggerInfo& t : triggers_){
		const float x0 = (float)t.cellX * TriggerGraphView::stepX() - border;
		const float y0 = (float)t.cellY * TriggerGraphView::stepY() - border;
		const float x1 = x0 + TriggerGraphView::kSizeX + border * 2.f;
		const float y1 = y0 + TriggerGraphView::kSizeY + border * 2.f;
		if(first){
			minX_ = x0;
			minY_ = y0;
			maxX_ = x1;
			maxY_ = y1;
			first = false;
		}
		else{
			if(x0 < minX_)
				minX_ = x0;
			if(y0 < minY_)
				minY_ = y0;
			if(x1 > maxX_)
				maxX_ = x1;
			if(y1 > maxY_)
				maxY_ = y1;
		}
	}
	if(first){
		minX_ = 0.f;
		minY_ = 0.f;
		maxX_ = 1.f;
		maxY_ = 1.f;
	}
	update();
}

QPointF TriggerMiniMapPanel::graphToWidget(float gx, float gy) const
{
	const QRect r = rect();
	const float w = maxX_ - minX_ > 0.f ? maxX_ - minX_ : 1.f;
	const float h = maxY_ - minY_ > 0.f ? maxY_ - minY_ : 1.f;
	const float s = std::min((float)r.width() / w, (float)r.height() / h);
	const float ox = (r.width() - w * s) * 0.5f;
	const float oy = (r.height() - h * s) * 0.5f;
	return QPointF(ox + (gx - minX_) * s, oy + (gy - minY_) * s);
}

QPointF TriggerMiniMapPanel::widgetToGraph(const QPoint& p) const
{
	const QRect r = rect();
	const float w = maxX_ - minX_ > 0.f ? maxX_ - minX_ : 1.f;
	const float h = maxY_ - minY_ > 0.f ? maxY_ - minY_ : 1.f;
	const float s = std::min((float)r.width() / w, (float)r.height() / h);
	if(s <= 0.f)
		return QPointF(minX_, minY_);
	const float ox = (r.width() - w * s) * 0.5f;
	const float oy = (r.height() - h * s) * 0.5f;
	return QPointF(minX_ + (p.x() - ox) / s, minY_ + (p.y() - oy) / s);
}

void TriggerMiniMapPanel::centerGraphAt(const QPoint& p)
{
	if(!graph_)
		return;
	const QPointF g = widgetToGraph(p);
	graph_->centerOn((float)g.x(), (float)g.y());
	update();
}

void TriggerMiniMapPanel::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	if(triggers_.empty()){
		painter.setPen(Qt::gray);
		painter.drawText(rect(), Qt::AlignCenter, tr("No triggers"));
		return;
	}
	// Nodes (TriggerMiniMap::drawTrigger): chain color fill, red frame when
	// selected in the graph view.
	const std::set<int> selected = graph_ ? graph_->selectedTriggers() : std::set<int>();
	for(size_t i = 0; i < triggers_.size(); ++i){
		const TriggerInfo& t = triggers_[i];
		const QPointF a = graphToWidget((float)t.cellX * TriggerGraphView::stepX(),
		                                (float)t.cellY * TriggerGraphView::stepY());
		const QPointF b = graphToWidget((float)t.cellX * TriggerGraphView::stepX() + TriggerGraphView::kSizeX,
		                                (float)t.cellY * TriggerGraphView::stepY() + TriggerGraphView::kSizeY);
		const QRectF rc(a, b);
		const QColor fill((int)((t.colorRGBA >> 16) & 0xFF),
		                  (int)((t.colorRGBA >> 8) & 0xFF),
		                  (int)(t.colorRGBA & 0xFF));
		if(fill == Qt::white){
			painter.setBrush(Qt::NoBrush);
			painter.setPen(Qt::black);
		}
		else{
			painter.setBrush(fill);
			painter.setPen(Qt::NoPen);
		}
		painter.drawRect(rc);
		if(selected.count((int)i)){
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(Qt::red, 2));
			painter.drawRect(rc);
		}
	}
	// The graph view's visible rect (TriggerMiniMap drew triggerView_->
	// visibleArea() as a black frame).
	if(graph_){
		float x0 = 0.f, y0 = 0.f, x1 = 0.f, y1 = 0.f;
		graph_->visibleGraphRect(x0, y0, x1, y1);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(Qt::black);
		painter.drawRect(QRectF(graphToWidget(x0, y0), graphToWidget(x1, y1)));
	}
}

void TriggerMiniMapPanel::mousePressEvent(QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
		centerGraphAt(event->pos());
}

void TriggerMiniMapPanel::mouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() & (Qt::LeftButton | Qt::RightButton))
		centerGraphAt(event->pos());
}
