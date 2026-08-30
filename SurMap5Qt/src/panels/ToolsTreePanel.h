// ToolsTreePanel.h — Qt port of the tools tree (CToolsTreeWindow + CToolsTreeCtrl,
// SurMap5/ToolsTreeWindow.cpp / ToolsTreeCtrl.cpp).
//
// The original was a CFrameWnd hosting a CTreeView of tools (the transform set
// plus the serialized tree from Scripts\Engine\VistaEngine.scr, rebuilt by the
// tool factory's REGISTER_CLASS list). In Qt the tree is a QTreeWidget in the
// tools dock; selecting a tool switches the ToolManager's current tool. The
// XPrm-serialized tree configuration is deferred — the tree mirrors the tool
// set ToolManager owns, grouped into the folder structure the original's
// REGISTER_CLASS list implied (Folder/UnitFolder/MiniDetaileFolder/PlayerFolder
// are folder tools; the terrain set lives under a "Terrain" group).

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
	void onItemActivated(QTreeWidgetItem* item, int column);
	void deleteSelected();
	void renameSelected();

	ToolManager* tools_ = nullptr;
	QTreeWidget* tree_ = nullptr;
};
