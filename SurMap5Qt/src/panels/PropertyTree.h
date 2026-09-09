// PropertyTree.h — Qt port of kdw::PropertyTree (Util/kdw/PropertyTree.h).
//
// Renders an editor::PropertyRow tree (the property form) in a QTreeWidget.
// Each row shows the field name (name/nameAlt) and its value as a string
// (valueAsString). Editing is not wired yet — the tree is read-only display
// of the serialized object, matching the first pass of the kdw port.
//
// Qt-clean: only touches the engine-free editor::PropertyRow model.

#pragma once

#include <QTreeWidget>

#include "editor/PropertyRow.h"   // editor::PropertyRow (engine-free model)

class PropertyTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit PropertyTree(QWidget* parent = nullptr);

	// Replace the whole tree with the rows under `root` (root's children are
	// the top-level fields). The rows are read; ownership stays with the
	// caller.
	void setRoot(editor::PropertyRow* root);

	// The row backing the current item, or null.
	editor::PropertyRow* currentRow() const;

private:
	// Recursively build QTreeWidgetItems under `parentItem` from `row`.
	void buildItem(QTreeWidgetItem* parentItem, editor::PropertyRow* row);
};