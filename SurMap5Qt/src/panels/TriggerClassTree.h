// TriggerClassTree.h — Qt port of ClassTree (TriggerEditor/ClassTree.h).
//
// The action/condition palette: a tree of the registered factory types
// (FactorySelector<Action>::comboStrings/comboStringsAlt, as the original
// built from factory.comboStrings()/comboStringsAlt()). Selecting a row
// assigns that type to the current trigger; dragging onto the graph creates
// a new trigger with that action.
//
// Qt-clean: the type lists come through RenderViewWidget forwarders.

#pragma once

#include <QTreeWidget>

#include <string>
#include <vector>

class RenderViewWidget;

class TriggerClassTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit TriggerClassTree(RenderViewWidget* view, QWidget* parent = nullptr);

	// Reload the action/condition palettes from the bridge.
	void reload();

	// The selected action type index (into triggerActionTypes), or -1.
	int currentActionType() const;
	// The selected condition type index (into triggerConditionTypes), or -1.
	int currentConditionType() const;

signals:
	void actionTypeSelected(int typeIndex);
	void conditionTypeSelected(int typeIndex);

private:
	void buildGroup(QTreeWidgetItem* parent, const std::vector<std::string>& names,
	                const std::vector<std::string>& namesAlt, bool isAction);

	RenderViewWidget* view_ = nullptr;
};
