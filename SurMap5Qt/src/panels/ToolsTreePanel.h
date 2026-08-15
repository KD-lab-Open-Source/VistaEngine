// ToolsTreePanel.h — Qt port of the tools tree (CToolsTreeWindow + CToolsTreeCtrl,
// SurMap5/ToolsTreeWindow.cpp / ToolsTreeCtrl.cpp).
//
// The original was a CFrameWnd hosting a CTreeView of tools (the transform set
// plus the serialized tree from Scripts\Engine\VistaEngine.scr, rebuilt by the
// tool factory's REGISTER_CLASS list). In Qt the tree is a QTreeWidget in the
// tools dock; selecting a tool switches the ToolManager's current tool. The
// XPrm-serialized tree configuration is deferred — the tree mirrors the tool
// set ToolManager owns, which is the part the editor actually uses.

#pragma once

#include <QWidget>

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

signals:
	// Emitted when the user picks a tool in the tree (index into ToolManager's
	// tool list: 0=Select, 1=Move, 2=Rotate, 3=Scale).
	void toolSelected(int index);

private:
	void onItemActivated(QTreeWidgetItem* item, int column);

	ToolManager* tools_ = nullptr;
	QTreeWidget* tree_ = nullptr;
};
