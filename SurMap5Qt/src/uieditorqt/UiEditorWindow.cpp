// UiEditorWindow.cpp — see header.

#include "UiEditorWindow.h"

#include "editor/EditorTool.h"    // IWorldBridge
#include "panels/PropertyTree.h"
#include "widgets/PreviewView.h"

#include <QAction>
#include <QInputDialog>
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
	resize(1200, 760);

	tree_ = new QTreeWidget(this);
	tree_->setHeaderLabels({ tr("Name"), tr("Type") });
	tree_->setColumnWidth(0, 280);

	properties_ = new PropertyTree(this);

	auto* left = new QSplitter(Qt::Vertical, this);
	left->addWidget(tree_);
	left->addWidget(properties_);
	left->setStretchFactor(0, 2);
	left->setStretchFactor(1, 1);

	preview_ = new PreviewView(this);
	preview_->setPreviewFunctions(
		[this](void* handle){ return bridge_ && bridge_->attachPreviewWindow(handle); },
		[this](int w, int h){ return bridge_ && bridge_->uiPreviewRender(w, h); },
		[this]{ if(bridge_) bridge_->detachPreviewWindow(); });

	auto* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(left);
	splitter->addWidget(preview_);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 2);
	setCentralWidget(splitter);

	auto* toolbar = addToolBar(tr("UI Editor"));
	connect(toolbar->addAction(tr("&Save")), &QAction::triggered, this, &UiEditorWindow::onSave);
	toolbar->addSeparator();
	connect(toolbar->addAction(tr("Add &Control...")), &QAction::triggered, this, &UiEditorWindow::onAddControl);
	connect(toolbar->addAction(tr("Add &State")), &QAction::triggered, this, &UiEditorWindow::onAddState);
	connect(toolbar->addAction(tr("&Delete")), &QAction::triggered, this, &UiEditorWindow::onDelete);
	toolbar->addSeparator();
	connect(toolbar->addAction(tr("&Refresh")), &QAction::triggered, this, &UiEditorWindow::refresh);

	connect(tree_, &QTreeWidget::itemSelectionChanged, this, &UiEditorWindow::onTreeSelectionChanged);
	connect(properties_, &QTreeWidget::itemChanged, this, &UiEditorWindow::onPropertyEdited);

	refresh();
}

UiEditorWindow::~UiEditorWindow()
{
	if(bridge_){
		bridge_->uiPreview(-1, false);
		bridge_->detachPreviewWindow();
	}
}

void UiEditorWindow::refresh()
{
	// Rebuilding invalidates the bridge's node/screen pointers; clear the
	// previewed screen until a node is selected again.
	if(bridge_)
		bridge_->uiPreview(-1, false);

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
	// Preview the screen owning this node (the embedded PreviewView renders it).
	bridge_->uiPreview(nodeId, true);
	if(preview_)
		preview_->update();
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

int UiEditorWindow::currentNodeId() const
{
	QTreeWidgetItem* item = tree_ ? tree_->currentItem() : nullptr;
	return item ? item->data(0, Qt::UserRole).toInt() : -1;
}

void UiEditorWindow::onAddControl()
{
	if(!bridge_)
		return;
	const int nodeId = currentNodeId();
	if(nodeId < 0){
		statusBar()->showMessage(tr("Select a screen or control first"), 3000);
		return;
	}
	std::vector<std::string> types;
	if(!bridge_->uiControlTypes(types) || types.empty()){
		statusBar()->showMessage(tr("Control types unavailable"));
		return;
	}
	QStringList items;
	for(const std::string& t : types)
		items << QString::fromStdString(t);
	bool ok = false;
	const QString choice = QInputDialog::getItem(this, tr("Add Control"), tr("Control type:"), items, 0, false, &ok);
	if(!ok)
		return;
	if(bridge_->uiAddControl(nodeId, items.indexOf(choice)))
		refresh();
}

void UiEditorWindow::onAddState()
{
	if(!bridge_)
		return;
	const int nodeId = currentNodeId();
	if(nodeId < 0 || !bridge_->uiAddState(nodeId)){
		statusBar()->showMessage(tr("Select a control first"), 3000);
		return;
	}
	refresh();
}

void UiEditorWindow::onDelete()
{
	if(!bridge_)
		return;
	const int nodeId = currentNodeId();
	if(nodeId < 0)
		return;
	if(bridge_->uiDeleteNode(nodeId))
		refresh();
	else
		statusBar()->showMessage(tr("Could not delete the node"));
}
