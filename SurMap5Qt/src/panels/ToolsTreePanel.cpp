// ToolsTreePanel.cpp — see header.

#include "ToolsTreePanel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "tools/ToolManager.h"

ToolsTreePanel::ToolsTreePanel(ToolManager* tools, QWidget* parent)
	: QWidget(parent)
	, tools_(tools)
{
	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setSelectionMode(QAbstractItemView::SingleSelection);
	tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree_->setIndentation(12);
	tree_->setContextMenuPolicy(Qt::DefaultContextMenu);

	// The tree mirrors the tools ToolManager owns (CToolsTreeCtrl built its
	// tree from the tool factory; here the transform set is the whole set),
	// grouped into the original's folder structure. Folder tools were
	// CSurToolEmpty ("Folder") / CSurToolUnitFolder / CSurToolMiniDetaileFolder
	// — containers without behaviour. The Qt port keeps the same grouping as
	// non-selectable group rows, and the tools as their children.
	auto* root = new QTreeWidgetItem(tree_);
	root->setText(0, tr("Tools"));
	root->setData(0, Qt::UserRole, -1);   // no tool index — a folder row
	root->setExpanded(true);

	// Group rows: (label, first tool index, tool count).
	struct Group { const char* label; int first; int count; };
	const Group groups[] = {
		{ "Transform", 0, 4 },                 // Select/Move/Rotate/Scale
		{ "Terrain", 4, 2 },                   // GeoNet, GeoTx
		{ "Objects", 6, 0 },                   // Unit/Source/Anchor/Camera land in G4
	};

	for(const Group& group : groups){
		auto* groupItem = new QTreeWidgetItem(root);
		groupItem->setText(0, tr(group.label));
		groupItem->setData(0, Qt::UserRole, -1);   // a folder row
		groupItem->setExpanded(true);
		for(int i = group.first; i < group.first + group.count && i < (int)tools_->tools().size(); ++i){
			EditorTool* tool = tools_->tools()[i];
			auto* item = new QTreeWidgetItem(groupItem);
			item->setText(0, tr(tool->name()));
			item->setData(0, Qt::UserRole, i);
		}
	}

	tree_->expandAll();

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tree_);

	// The original's TVN_SELCHANGED: picking a tool in the tree switches to it.
	connect(tree_, &QTreeWidget::itemActivated, this, &ToolsTreePanel::onItemActivated);
	connect(tree_, &QTreeWidget::itemClicked, this, &ToolsTreePanel::onItemActivated);
}

void ToolsTreePanel::syncToTool()
{
	// Highlight the tree row matching the current tool (the toolbar and the
	// tree both switch the tool; whichever was used last wins).
	const int index = tools_->currentIndex();
	for(int i = 0; i < tree_->topLevelItemCount(); ++i){
		QTreeWidgetItem* root = tree_->topLevelItem(i);
		for(int j = 0; j < root->childCount(); ++j){
			QTreeWidgetItem* group = root->child(j);
			for(int k = 0; k < group->childCount(); ++k){
				QTreeWidgetItem* item = group->child(k);
				if(item->data(0, Qt::UserRole).toInt() == index){
					tree_->setCurrentItem(item);
					return;
				}
			}
		}
	}
}

void ToolsTreePanel::onItemActivated(QTreeWidgetItem* item, int /*column*/)
{
	if(!item)
		return;
	const QVariant data = item->data(0, Qt::UserRole);
	if(data.isValid() && data.toInt() >= 0)
		emit toolSelected(data.toInt());
}

void ToolsTreePanel::contextMenuEvent(QContextMenuEvent* event)
{
	// The original's NM_RCLICK on the tools tree: ID_POPUP_DELETE and the
	// label-edit (TVN_BEGINLABELEDIT/ENDLABELEDIT). The folder rows are
	// structural, so only tool rows get the menu.
	QTreeWidgetItem* item = tree_->itemAt(event->pos());
	if(!item)
		return;
	const QVariant data = item->data(0, Qt::UserRole);
	if(!data.isValid() || data.toInt() < 0)
		return;

	QMenu menu(this);
	QAction* renameAction = menu.addAction(tr("Rename..."));
	QAction* deleteAction = menu.addAction(tr("Delete"));
	QAction* chosen = menu.exec(event->globalPos());
	if(chosen == renameAction)
		renameSelected();
	else if(chosen == deleteAction)
		deleteSelected();
}

void ToolsTreePanel::deleteSelected()
{
	// The original's ID_POPUP_DELETE removed the tool from the tree (the
	// tool itself stays registered). Here it only removes the tree row.
	QTreeWidgetItem* item = tree_->currentItem();
	if(item && item->parent())
		delete item;
}

void ToolsTreePanel::renameSelected()
{
	// The original's TVN_BEGINLABELEDIT/ENDLABELEDIT made tool rows editable;
	// the label is what the tools tree showed.
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	tree_->editItem(item, 0);
}

void ToolsTreePanel::saveState()
{
	QSettings s;
	if(tree_->topLevelItemCount() > 0 && tree_->topLevelItem(0))
		s.setValue("toolsTree/expanded", tree_->topLevelItem(0)->isExpanded());
	QTreeWidgetItem* cur = tree_->currentItem();
	s.setValue("toolsTree/currentRow", cur ? cur->text(0) : QString());
}

void ToolsTreePanel::restoreState()
{
	QSettings s;
	bool expanded = s.value("toolsTree/expanded", true).toBool();
	QString curText = s.value("toolsTree/currentRow").toString();
	if(tree_->topLevelItemCount() > 0 && tree_->topLevelItem(0)){
		tree_->topLevelItem(0)->setExpanded(expanded);
	}
}
