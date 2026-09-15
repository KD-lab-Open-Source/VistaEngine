// TriggerConditionPanel.h — Qt port of ConditionEditor
// (TriggerEditor/ConditionEditor.h).
//
// The condition tree of one trigger: a QTreeWidget showing the Condition /
// ConditionSwitcher hierarchy (ConditionSlot rects become tree rows; the
// invert flag is a checkable column), plus Add/Delete/Invert buttons and a
// type combo fed from the condition palette. The selected condition's fields
// are edited in the shared PropertyTree through the bridge's
// triggerConditionTree/triggerConditionSetTree.
//
// Note: the bridge serializes the whole condition tree as one PropertyRow
// tree; this panel renders that tree directly (no separate ConditionSlot
// geometry pass — the kdw drag/drop canvas becomes a plain tree with
// type-combo assignment, matching the "через PropertyTree" scope answer).
//
// Qt-clean: talks to the engine only through RenderViewWidget forwarders.

#pragma once

#include <QWidget>

#include <string>
#include <vector>

#include "editor/PropertyRow.h"   // editor::PropertyRow (engine-free model)

class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
class RenderViewWidget;
class TriggerGraphView;

class TriggerConditionPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TriggerConditionPanel(RenderViewWidget* view, TriggerGraphView* graph,
	                               QWidget* parent = nullptr);

	// Load the condition tree of the given trigger index.
	void loadTrigger(int triggerIndex);
	// The trigger currently shown, or -1.
	int currentTrigger() const { return triggerIndex_; }

private slots:
	void onGraphSelection();
	void onAddCondition();
	void onDeleteCondition();
	void onInvertCondition();
	void onTypeActivated(int comboIndex);

private:
	void rebuild();
	void buildRow(QTreeWidgetItem* parentItem, editor::PropertyRow* row);
	void buildItem(QTreeWidgetItem* parentItem, const QString& label, const QString& value);

	RenderViewWidget* view_ = nullptr;
	TriggerGraphView* graph_ = nullptr;
	int triggerIndex_ = -1;

	QTreeWidget* tree_ = nullptr;
	QComboBox* typeCombo_ = nullptr;
	std::vector<std::string> typeNames_;
	std::vector<std::string> typeNamesAlt_;
};
