// EffectEditorWindow.cpp — see header.

#include "EffectEditorWindow.h"

#include "editor/EditorTool.h"    // IWorldBridge
#include "panels/PropertyTree.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <map>

EffectEditorWindow::EffectEditorWindow(IWorldBridge* bridge, QWidget* parent)
	: QMainWindow(parent), bridge_(bridge)
{
	setWindowTitle(tr("Effects Editor"));
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

	auto* toolbar = addToolBar(tr("Effects Editor"));
	connect(toolbar->addAction(tr("&Open...")), &QAction::triggered, this, &EffectEditorWindow::onOpen);
	connect(toolbar->addAction(tr("&Save")), &QAction::triggered, this, &EffectEditorWindow::onSave);
	connect(toolbar->addAction(tr("Save &As...")), &QAction::triggered, this, &EffectEditorWindow::onSaveAs);

	connect(tree_, &QTreeWidget::itemSelectionChanged, this, &EffectEditorWindow::onTreeSelectionChanged);
	connect(properties_, &QTreeWidget::itemChanged, this, &EffectEditorWindow::onPropertyEdited);

	statusBar()->showMessage(tr("Open a .effect file (Resource/FX)"));
}

EffectEditorWindow::~EffectEditorWindow()
{
	if(bridge_)
		bridge_->effectClose();
}

void EffectEditorWindow::refresh()
{
	loading_ = true;
	tree_->clear();
	properties_->setRoot(nullptr);

	std::vector<IWorldBridge::EffectTreeNode> nodes;
	if(!bridge_ || !bridge_->effectTree(nodes)){
		loading_ = false;
		statusBar()->showMessage(tr("No effect loaded"));
		return;
	}

	std::map<int, QTreeWidgetItem*> itemById;
	for(const IWorldBridge::EffectTreeNode& node : nodes){
		QTreeWidgetItem* item = nullptr;
		if(node.parentId < 0)
			item = new QTreeWidgetItem(tree_);
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

void EffectEditorWindow::onOpen()
{
	if(!bridge_)
		return;
	const QString path = QFileDialog::getOpenFileName(this, tr("Open Effect"),
		QDir::current().filePath(QStringLiteral("Resource/FX")),
		tr("Effects (*.effect)"));
	if(path.isEmpty())
		return;
	const std::string native = QDir::toNativeSeparators(path).toStdString();
	if(bridge_->effectOpen(native)){
		refresh();
		statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 3000);
	}
	else
		statusBar()->showMessage(tr("Could not open %1").arg(path));
}

void EffectEditorWindow::onSave()
{
	if(!bridge_)
		return;
	if(bridge_->effectSave())
		statusBar()->showMessage(tr("Effect saved"), 3000);
	else
		statusBar()->showMessage(tr("Could not save the effect (open or Save As first)"));
}

void EffectEditorWindow::onSaveAs()
{
	if(!bridge_)
		return;
	const QString path = QFileDialog::getSaveFileName(this, tr("Save Effect As"),
		QDir::current().filePath(QStringLiteral("Resource/FX")),
		tr("Effects (*.effect)"));
	if(path.isEmpty())
		return;
	const std::string native = QDir::toNativeSeparators(path).toStdString();
	if(bridge_->effectSaveAs(native))
		statusBar()->showMessage(tr("Effect saved as %1").arg(QFileInfo(path).fileName()), 3000);
	else
		statusBar()->showMessage(tr("Could not save %1").arg(path));
}

void EffectEditorWindow::onTreeSelectionChanged()
{
	if(loading_ || !bridge_)
		return;
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	const int nodeId = item->data(0, Qt::UserRole).toInt();
	loading_ = true;
	properties_->setRoot(bridge_->effectNodeTree(nodeId, true));
	loading_ = false;
	if(!item->childCount() && properties_->root() == nullptr)
		statusBar()->showMessage(tr("Curves are edited by the curve editor (later milestone)"), 3000);
}

void EffectEditorWindow::onPropertyEdited()
{
	if(loading_ || !bridge_)
		return;
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	const int nodeId = item->data(0, Qt::UserRole).toInt();
	if(editor::PropertyRow* root = properties_->root())
		bridge_->effectNodeSetTree(nodeId, root);
}
