// ToolsTreePanel.cpp — see header.

#include "ToolsTreePanel.h"

#include <QHeaderView>
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

	// The tree mirrors the tools ToolManager owns (CToolsTreeCtrl built its
	// tree from the tool factory; here the transform set is the whole set).
	QTreeWidgetItem* root = new QTreeWidgetItem(tree_);
	root->setText(0, tr("Tools"));
	root->setExpanded(true);

	for(int i = 0; i < (int)tools_->tools().size(); ++i){
		EditorTool* tool = tools_->tools()[i];
		auto* item = new QTreeWidgetItem(root);
		item->setText(0, tr(tool->name()));
		item->setData(0, Qt::UserRole, i);
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
			QTreeWidgetItem* item = root->child(j);
			if(item->data(0, Qt::UserRole).toInt() == index){
				tree_->setCurrentItem(item);
				return;
			}
		}
	}
}

void ToolsTreePanel::onItemActivated(QTreeWidgetItem* item, int /*column*/)
{
	if(!item)
		return;
	const QVariant data = item->data(0, Qt::UserRole);
	if(data.isValid())
		emit toolSelected(data.toInt());
}
