// ToolsTreePanel.h — Qt port of the tools tree (CToolsTreeWindow + CToolsTreeCtrl,
// SurMap5/ToolsTreeWindow.cpp / ToolsTreeCtrl.cpp).
//
// The original was a CFrameWnd hosting a CTreeView: the transform tools as
// toolbar buttons, and a tree of placement tools grouped by object type
// (Object-based terrain / Textures / Environmental Objects / Units, as in the
// original's tool tree). In Qt the tree is a QTreeWidget in the tools dock;
// the terrain/texture rows switch the ToolManager's current tool, while the
// object folders list the world's catalog entries (unit attributes, sources,
// environment models) — picking one switches to that tool and sets its target.
// The XPrm-serialized tree configuration is deferred.

#pragma once

#include <QWidget>

class QContextMenuEvent;
class QTreeWidget;
class QTreeWidgetItem;
class ToolManager;

class ToolsTreePanel : public QWidget
{
	Q_OBJECT
public:
	explicit ToolsTreePanel(ToolManager* tools, QWidget* parent = nullptr);

	// Populate the object catalog (unit attributes, sources, environment models)
	// from the bridge. Call after a world load — the libraries arrive with it.
	void rebuildCatalog();

	// Sync the tree's selection to the current tool (used after the toolbar
	// switches tools, so the tree stays in step).
	void syncToTool();

	// Persist the current tree state (expanded folders, selection) to
	// QSettings so the workspace survives restarts (port of CToolsTreeCtrl::
	// save / serialize to Scripts\Content\VistaEngine.scr).
	void saveState();
	void restoreState();

signals:
	// Emitted when the user picks a tool in the tree (index into ToolManager's
	// tool list: 0=Select, 1=Move, 2=Rotate, 3=Scale).
	void toolSelected(int index);

protected:
	// Context menu (the original's NM_RCLICK): Delete + Rename on tool rows;
	// the folder rows are fixed. ID_POPUP_DELETE / ID_POPUP_RENAME.
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	void buildTree();
	void onItemActivated(QTreeWidgetItem* item, int column);
	void deleteSelected();
	void renameSelected();

	ToolManager* tools_ = nullptr;
	QTreeWidget* tree_ = nullptr;
	// The object folders the catalogs hang under.
	QTreeWidgetItem* unitsFolder_ = nullptr;
	QTreeWidgetItem* sourcesFolder_ = nullptr;
	QTreeWidgetItem* environmentFolder_ = nullptr;
};
