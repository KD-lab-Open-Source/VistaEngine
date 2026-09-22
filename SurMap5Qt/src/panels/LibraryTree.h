// LibraryTree.h — Qt port of kdw::LibraryTree (Util/kdw/LibraryTree.h).
//
// The left-hand tree of a library editor: the library's elements under
// their group folders (a Qt port of LibraryTabEditable::buildLibraryTree:
// predefined groups from editorGroupsComboList, '\\' nests, then the
// per-element editorElementGroup, ungrouped elements at the top level).
// Selecting an element row asks the bridge for that element's property
// tree. Qt-clean — everything comes through the engine-free IWorldBridge.

#pragma once

#include <QTreeWidget>

#include <map>
#include <string>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class LibraryTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit LibraryTree(QWidget* parent = nullptr);

	// Populate from the bridge's library data for `libraryName`.
	void setLibrary(IWorldBridge* bridge, const std::string& libraryName);

	// The index of the selected element, or -1 (group rows select nothing).
	int currentIndex() const;

signals:
	// Emitted when the selection changes to a different element.
	void elementSelected(int elementIndex);

private:
	// Find or create the group folder for `path` ('\\' separated),
	// decoded for display. Empty path = the tree root (no folder).
	QTreeWidgetItem* groupItem(const std::string& path);

	// Full group path -> folder item, rebuilt by each setLibrary.
	std::map<std::string, QTreeWidgetItem*> groupCache_;
};
