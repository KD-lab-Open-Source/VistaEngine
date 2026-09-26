// EffectEditorWindow.cpp — see header.

#include "EffectEditorWindow.h"

#include "editor/EditorTool.h"    // IWorldBridge
#include "panels/PropertyTree.h"
#include "widgets/PreviewView.h"

#include <QAction>
#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
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
	curveKeys_ = new QTableWidget(this);
	curveKeys_->setColumnCount(2);
	curveKeys_->setHorizontalHeaderLabels({ tr("Time"), tr("Value") });
	curveKeys_->horizontalHeader()->setStretchLastSection(true);

	auto* right = new QWidget(this);
	auto* rightLayout = new QVBoxLayout(right);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->addWidget(new QLabel(tr("Properties"), right));
	rightLayout->addWidget(properties_, 2);
	rightLayout->addWidget(new QLabel(tr("Curve keys"), right));
	rightLayout->addWidget(curveKeys_, 1);

	preview_ = new PreviewView(right);
	preview_->setPreviewFunctions(
		[this](void* handle){ return bridge_ && bridge_->attachPreviewWindow(handle); },
		[this](int w, int h){ return bridge_ && bridge_->effectPreviewRender(w, h); },
		[this]{ if(bridge_) bridge_->detachPreviewWindow(); });
	rightLayout->addWidget(new QLabel(tr("Preview"), right));
	rightLayout->addWidget(preview_, 3);

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
	toolbar->addSeparator();
	previewCheck_ = new QCheckBox(tr("3D preview"), toolbar);
	toolbar->addWidget(previewCheck_);
	previewTime_ = new QDoubleSpinBox(toolbar);
	previewTime_->setRange(0.0, 60.0);
	previewTime_->setDecimals(2);
	previewTime_->setSingleStep(0.05);
	previewTime_->setPrefix(tr("t = "));
	previewTime_->setSuffix(tr(" s"));
	previewTime_->setEnabled(false);
	toolbar->addWidget(previewTime_);
	connect(previewCheck_, &QCheckBox::toggled, this, &EffectEditorWindow::onPreviewToggled);
	connect(previewTime_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, &EffectEditorWindow::onPreviewTimeChanged);

	connect(tree_, &QTreeWidget::itemSelectionChanged, this, &EffectEditorWindow::onTreeSelectionChanged);
	connect(properties_, &QTreeWidget::itemChanged, this, &EffectEditorWindow::onPropertyEdited);
	connect(curveKeys_, &QTableWidget::cellChanged, this, &EffectEditorWindow::onCurveKeyChanged);

	statusBar()->showMessage(tr("Open a .effect file (Resource/FX)"));
}

EffectEditorWindow::~EffectEditorWindow()
{
	if(bridge_){
		bridge_->effectPreview(false);
		bridge_->detachPreviewWindow();
		bridge_->effectClose();
	}
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
		item->setData(0, Qt::UserRole + 1, node.kind);
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
		// effectOpen stopped any old preview; reset the check so the toggle
		// fires and recreates the effect for the new file.
		previewCheck_->setChecked(false);
		refresh();
		// The 3D preview is on by default (the original EffectEditor always
		// showed the effect); time flows via the engine's Animate.
		previewCheck_->setChecked(true);
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
	const int kind = item->data(0, Qt::UserRole + 1).toInt();
	loading_ = true;
	properties_->setRoot(bridge_->effectNodeTree(nodeId, true));
	loading_ = false;
	populateCurveKeys(kind == IWorldBridge::kEffectCurve ? nodeId : -1);
	if(kind == IWorldBridge::kEffectCurve)
		statusBar()->showMessage(tr("Edit the curve keys in the table below"), 3000);
}

void EffectEditorWindow::populateCurveKeys(int curveNodeId)
{
	curveNodeId_ = curveNodeId;
	loading_ = true;
	curveKeys_->setRowCount(0);
	if(curveNodeId >= 0 && bridge_){
		const int count = bridge_->effectCurveKeyCount(curveNodeId);
		curveKeys_->setRowCount(count);
		for(int i = 0; i < count; ++i){
			float t = 0.f, v = 0.f;
			if(bridge_->effectCurveKey(curveNodeId, i, t, v)){
				curveKeys_->setItem(i, 0, new QTableWidgetItem(QString::number(t, 'g', 6)));
				curveKeys_->setItem(i, 1, new QTableWidgetItem(QString::number(v, 'g', 6)));
			}
		}
	}
	loading_ = false;
}

void EffectEditorWindow::onCurveKeyChanged(int row, int /*column*/)
{
	if(loading_ || curveNodeId_ < 0 || !bridge_)
		return;
	QTableWidgetItem* timeItem = curveKeys_->item(row, 0);
	QTableWidgetItem* valueItem = curveKeys_->item(row, 1);
	if(!timeItem || !valueItem)
		return;
	bridge_->effectCurveSetKey(curveNodeId_, row,
		timeItem->text().toFloat(), valueItem->text().toFloat());
}

void EffectEditorWindow::onPreviewToggled(bool on)
{
	if(!bridge_)
		return;
	previewTime_->setEnabled(on);
	if(on){
		if(!bridge_->effectPreview(true)){
			statusBar()->showMessage(tr("Open an effect with a loaded world first"));
			previewCheck_->setChecked(false);
			return;
		}
		bridge_->effectSetPreviewTime(previewTime_->value());
		statusBar()->showMessage(tr("Preview shown in the main 3D view"), 3000);
	}
	else{
		bridge_->effectPreview(false);
		statusBar()->showMessage(tr("Preview off"), 3000);
	}
}

void EffectEditorWindow::onPreviewTimeChanged(double time)
{
	if(bridge_ && previewCheck_->isChecked())
		bridge_->effectSetPreviewTime((float)time);
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
