// LibraryTree.h — Qt port of kdw::LibraryTree (Util/kdw/LibraryTree.h).
//
// The left-hand tree of a library editor: one row per library element
// (editorElementName over editorSize). Selecting a row asks the bridge for
// that element's property tree. Qt-clean — the element list comes through
// the engine-free IWorldBridge.

#pragma once

#include <QTreeWidget>

#include <string>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class LibraryTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit LibraryTree(QWidget* parent = nullptr);

	// Populate from the bridge's libraryElementNames for `libraryName`.
	void setLibrary(IWorldBridge* bridge, const std::string& libraryName);

	// The index of the selected element, or -1.
	int currentIndex() const;

signals:
	// Emitted when the selection changes to a different element.
	void elementSelected(int elementIndex);
};