// ObjectsTreePanel.h — Qt port of the objects manager tree
// (CObjectsManagerWindow + ObjectsManagerTree, SurMap5/ObjectsManagerWindow.cpp).
//
// The original was a CFrameWnd with a CLibraryTabCtrl (typeTabs_: Sources /
// Environment / Units / Cameras / Anchors) above a CTreeView of the world's
// objects of the active type, with drag&drop and a context menu. The Qt port
// is a QTabWidget with one QTreeWidget per tab, populated by the world once
// objects exist (the world object list lands with the unit/environment tools);
// the tabs mirror the original's typeTabs_ exactly.

#pragma once

#include <QWidget>

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;

class ObjectsTreePanel : public QWidget
{
	Q_OBJECT
public:
	explicit ObjectsTreePanel(QWidget* parent = nullptr);

	// Wire the engine side (the only Qt-side object the panel reaches into:
	// EngineViewport's objectList). MainWindow sets this in createDockPanels.
	void setViewport(class EngineViewport* viewport) { viewport_ = viewport; }

	// Clear and repopulate from the current world (CObjectsManagerWindow
	// rebuilt the tree on onWorldChanged). No world objects yet — each tab's
	// tree shows the type group, ready for objects.
	void rebuild();

	// The object-type tab currently shown (TAB_SOURCES=0, TAB_ENVIRONMENT=1,
	// TAB_UNITS=2, TAB_CAMERA=3, TAB_ANCHORS=4 — the typeTabs_ order).
	int currentTab() const;

protected:
	// Context menu (the original's NM_RCLICK): Delete + Rename when a row
	// is selected.
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	void deleteSelected();
	void renameSelected();
	// The tree of the active tab.
	QTreeWidget* currentTree() const;

	EngineViewport* viewport_ = nullptr;
	QTabWidget* tabs_ = nullptr;
};
