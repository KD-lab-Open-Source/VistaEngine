// PropertyTree.h — Qt port of kdw::PropertyTree (Util/kdw/PropertyTree.h).
//
// Renders an editor::PropertyRow tree (the property form) in a QTreeWidget.
// Each row shows the field name (name/nameAlt) and its value as a string
// (valueAsString). Leaf values are editable in place (double-click / F2):
// the edit parses back into the row (setValueFromString) and stays only on
// success, otherwise the cell reverts. Container rows are display only.
//
// The tree takes ownership of the root passed to setRoot (the engine bridge
// hands out a fresh heap tree per call); root() exposes it for write-back
// (libraryElementSetTree / triggerSetTree).
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
	~PropertyTree() override;

	// Replace the whole tree with the rows under `root` (root's children are
	// the top-level fields). Takes ownership of `root` (may be null to
	// clear); the previous root is freed.
	void setRoot(editor::PropertyRow* root);

	// The owned root (null when empty). Valid until the next setRoot.
	editor::PropertyRow* root() const { return root_; }

	// The row backing the current item, or null.
	editor::PropertyRow* currentRow() const;

private slots:
	void onItemChanged(QTreeWidgetItem* item, int column);

private:
	// Recursively build QTreeWidgetItems under `parentItem` from `row`.
	void buildItem(QTreeWidgetItem* parentItem, editor::PropertyRow* row);

	editor::PropertyRow* root_ = nullptr;
	bool building_ = false;   // setRoot in progress — ignore itemChanged
	bool applying_ = false;   // reverting/reformatting a cell — ignore itemChanged
};
