// UiEditorWindow.cpp — see header.

#include "UiEditorWindow.h"

#include "editor/EditorTool.h"    // IWorldBridge
#include "panels/PropertyTree.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <map>

UiEditorWindow::UiEditorWindow(IWorldBridge* bridge, QWidget* parent)
	: QMainWindow(parent), bridge_(bridge)
{
	setWindowTitle(tr("UI Editor"));
	resize(1000, 700);

	tree_ = new QTreeWidget(this);
	tree_->setHeaderLabels({ tr("Name"), tr("Type") });
	tree_->setColumnWidth(0, 280);

	properties_ = new PropertyTree(this);

	auto* right = new QWidget(this);
	auto* rightLayout = new QVBoxLayout(right);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->addWidget(new QLabel(tr("Properties"), right));
	rightLayout->addWidget(properties_, 1);

	auto* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(tree_);
	splitter->addWidget(right);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 2);
	setCentralWidget(splitter);

	auto* toolbar = addToolBar(tr("UI Editor"));
	auto* saveAction = toolbar->addAction(tr("&Save"));
	connect(saveAction, &QAction::triggered, this, &UiEditorWindow::onSave);

	connect(tree_, &QTreeWidget::itemSelectionChanged, this, &UiEditorWindow::onTreeSelectionChanged);
	connect(properties_, &QTreeWidget::itemChanged, this, &UiEditorWindow::onPropertyEdited);

	refresh();
}

void UiEditorWindow::refresh()
{
	loading_ = true;
	tree_->clear();
	properties_->setRoot(nullptr);

	std::vector<IWorldBridge::UiTreeNode> nodes;
	if(!bridge_ || !bridge_->uiTree(nodes)){
		loading_ = false;
		statusBar()->showMessage(tr("UI libraries unavailable"));
		return;
	}

	std::map<int, QTreeWidgetItem*> itemById;
	for(const IWorldBridge::UiTreeNode& node : nodes){
		QTreeWidgetItem* item = nullptr;
		if(node.parentId < 0){
			item = new QTreeWidgetItem(tree_);
		}
		else{
			QTreeWidgetItem* parent = itemById.count(node.parentId) ? itemById[node.parentId] : nullptr;
			item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
		}
		item->setText(0, QString::fromStdString(node.name));
		item->setText(1, QString::fromStdString(node.type));
		item->setData(0, Qt::UserRole, node.id);
		itemById[node.id] = item;
	}
	tree_->expandAll();
	loading_ = false;
	statusBar()->showMessage(tr("%1 nodes").arg((int)nodes.size()), 3000);
}

void UiEditorWindow::onTreeSelectionChanged()
{
	if(loading_ || !bridge_)
		return;
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	const int nodeId = item->data(0, Qt::UserRole).toInt();
	loading_ = true;
	properties_->setRoot(bridge_->uiNodeTree(nodeId, true));
	loading_ = false;
}

void UiEditorWindow::onPropertyEdited()
{
	if(loading_ || !bridge_)
		return;
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	const int nodeId = item->data(0, Qt::UserRole).toInt();
	if(editor::PropertyRow* root = properties_->root())
		bridge_->uiNodeSetTree(nodeId, root);
}

void UiEditorWindow::onSave()
{
	if(!bridge_)
		return;
	if(bridge_->uiSave())
		statusBar()->showMessage(tr("UI document saved"), 3000);
	else
		statusBar()->showMessage(tr("Could not save the UI document"));
}
