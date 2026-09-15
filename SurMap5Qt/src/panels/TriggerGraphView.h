// TriggerGraphView.h — Qt port of TriggerView (TriggerEditor/TriggerView.h).
//
// The trigger graph canvas: draws trigger nodes (SIZE_X 200 x SIZE_Y 30,
// spaced by OFFSET_X 30 — Trigger::gridSizeSpaced) and the colored links
// between them (TriggerLink::colors), and handles selection, dragging,
// link creation, and the context menu.
//
// Qt-clean: all engine data arrives as TriggerInfo/TriggerLinkInfo snapshots
// through RenderViewWidget forwarders (indices, never cp1251 names).

#pragma once

#include <QPoint>
#include <QWidget>

#include <set>
#include <string>
#include <vector>

#include "editor/EditorTool.h"   // TriggerInfo/TriggerLinkInfo (engine-free)

class RenderViewWidget;

class TriggerGraphView : public QWidget
{
	Q_OBJECT
public:
	explicit TriggerGraphView(RenderViewWidget* view, QWidget* parent = nullptr);

	// Reload the snapshots from the bridge and repaint.
	void reload();

	// The selected trigger indices (into the last triggerList snapshot).
	std::set<int> selectedTriggers() const { return selected_; }
	// Select one trigger (clears the rest unless `add`).
	void selectTrigger(int index, bool add = false);
	void clearSelection();

	// The selected link (parent/child trigger indices), or -1/-1.
	int selectedLinkParent() const { return selLinkParent_; }
	int selectedLinkChild() const { return selLinkChild_; }

	// Pan the view so the given graph point is centred.
	void centerOn(float gx, float gy);
	// The visible graph rect (for the minimap's viewport frame).
	void visibleGraphRect(float& x0, float& y0, float& x1, float& y1) const;

	// Grid geometry (Trigger::SIZE_X/SIZE_Y/OFFSET_X from TriggerExport.h).
	static constexpr int kSizeX = 200;
	static constexpr int kSizeY = 30;
	static constexpr int kOffsetX = 30;
	static int stepX() { return kSizeX + kOffsetX; }
	static int stepY() { return kSizeY * 2; }

signals:
	void selectionChanged();
	void triggersChanged();   // a mutation happened (minimap must refresh)
	void editTrigger(int index);
	void editConditions(int index);
	void editAction(int index);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	// Graph <-> screen transforms (origin_ is the graph point at widget 0,0).
	QPointF graphToScreen(float gx, float gy) const;
	QPointF screenToGraph(const QPoint& p) const;
	QRectF triggerRect(int index) const;
	int triggerAt(const QPoint& p) const;
	bool linkAt(const QPoint& p, int& parentOut, int& childOut) const;

	void startLinkDrag(int parentIndex, const QPoint& p);
	void finishLinkDrag(const QPoint& p);
	void moveSelected(int dxCells, int dyCells);

	RenderViewWidget* view_ = nullptr;
	std::vector<TriggerInfo> triggers_;
	std::vector<TriggerLinkInfo> links_;

	std::set<int> selected_;
	int selLinkParent_ = -1;
	int selLinkChild_ = -1;

	// View state.
	QPointF origin_ = {0.f, 0.f};
	float zoom_ = 1.f;

	// Interaction state.
	bool dragging_ = false;
	QPoint dragStart_;
	QPointF dragOrigin_;
	std::set<int> dragMoved_;   // selected indices whose cells changed
	bool areaSelecting_ = false;
	QPoint areaStart_;
	QPoint areaEnd_;
	bool linking_ = false;
	int linkParent_ = -1;
	QPoint linkCurrent_;
	bool spacePanning_ = false;

	// Pending link style (TriggerView::linkColor_/linkAutoRestarted_): the
	// color/type the next created link gets.
	int nextLinkColor_ = 0;
	bool nextLinkAutoRestarted_ = false;
};
