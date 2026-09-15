// TriggerDebuggerPanel.h — Qt port of TriggerDebugger
// (TriggerEditor/TriggerDebugger.h).
//
// The debug-log panel: a list of the chain's TriggerEvent records
// (event/triggerName/state) plus a slider scrubbing through them, as the
// original's ObjectsTree + Slider did. Selecting a record shows the legend
// colors (TriggerView::showLegend path) — the graph highlights the states.
//
// Qt-clean: records arrive as TriggerLogRecord snapshots through
// RenderViewWidget.

#pragma once

#include <QWidget>

#include <vector>

#include "editor/EditorTool.h"   // TriggerLogRecord (engine-free)

class QListWidget;
class QSlider;
class RenderViewWidget;
class TriggerGraphView;

class TriggerDebuggerPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TriggerDebuggerPanel(RenderViewWidget* view, TriggerGraphView* graph,
	                              QWidget* parent = nullptr);

	// Refresh the record list from the bridge. Returns true when the log is
	// non-empty (the original auto-opened the debugger in that case).
	bool reload();

private slots:
	void onRowChanged(int row);
	void onSliderChanged(int value);

private:
	RenderViewWidget* view_ = nullptr;
	TriggerGraphView* graph_ = nullptr;
	QListWidget* list_ = nullptr;
	QSlider* slider_ = nullptr;
	std::vector<TriggerLogRecord> records_;
	bool syncing_ = false;
};
