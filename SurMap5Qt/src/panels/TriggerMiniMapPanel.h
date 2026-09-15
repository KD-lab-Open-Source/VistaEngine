// TriggerMiniMapPanel.h — Qt port of TriggerMiniMap
// (TriggerEditor/TriggerMiniMap.h).
//
// A thumbnail of the trigger graph: draws every trigger as a small rect in
// its chain color, plus the graph view's visible-rect frame. Clicking or
// dragging centres the graph view there (TriggerMiniMap::onMouseButtonDown/
// onMouseMove -> triggerView_->centerOn).
//
// Qt-clean: reads TriggerInfo snapshots through RenderViewWidget.

#pragma once

#include <QWidget>

#include <vector>

#include "editor/EditorTool.h"   // TriggerInfo (engine-free)

class RenderViewWidget;
class TriggerGraphView;

class TriggerMiniMapPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TriggerMiniMapPanel(RenderViewWidget* view, TriggerGraphView* graph,
	                             QWidget* parent = nullptr);

	// Refresh the cached trigger list from the bridge.
	void reload();

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	// Map a graph point to widget pixels (fit-all).
	QPointF graphToWidget(float gx, float gy) const;
	QPointF widgetToGraph(const QPoint& p) const;
	void centerGraphAt(const QPoint& p);

	RenderViewWidget* view_ = nullptr;
	TriggerGraphView* graph_ = nullptr;
	std::vector<TriggerInfo> triggers_;

	// Cached fit-all bounds (graph units) + widget mapping.
	float minX_ = 0.f, minY_ = 0.f, maxX_ = 1.f, maxY_ = 1.f;
};
