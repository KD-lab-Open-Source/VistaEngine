// ObjectsTreePanel.h — Qt port of the objects manager tree
// (CObjectsManagerWindow + ObjectsManagerTree, SurMap5/ObjectsManagerWindow.cpp).
//
// The original showed the world's objects (units, sources, environment
// objects, anchors, cameras) in a tree grouped by type, with drag&drop and a
// context menu. The Qt port keeps the panel's place: a QTreeWidget in the
// objects dock, populated by the world once objects exist (Phase 6 fills the
// structure; the world object list lands with the unit/environment tools).

#pragma once

#include <QWidget>

class QTreeWidget;

class ObjectsTreePanel : public QWidget
{
	Q_OBJECT
public:
	explicit ObjectsTreePanel(QWidget* parent = nullptr);

	// Clear and repopulate from the current world (CObjectsManagerWindow
	// rebuilt the tree on onWorldChanged). No world objects yet — the tree
	// shows the type groups, ready for objects.
	void rebuild();

private:
	QTreeWidget* tree_ = nullptr;
};
