// TriggerEditorDialog.cpp — see header.

#include "TriggerEditorDialog.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include "RenderViewWidget.h"
#include "panels/TriggerGraphView.h"
#include "panels/TriggerClassTree.h"
#include "panels/TriggerMiniMapPanel.h"
#include "panels/TriggerConditionPanel.h"
#include "panels/TriggerDebuggerPanel.h"
#include "panels/PropertyTree.h"
#include "editor/PropertyRow.h"

TriggerEditorDialog::TriggerEditorDialog(RenderViewWidget* view, QWidget* parent)
	: QDialog(parent)
	, view_(view)
{
	setWindowTitle(tr("Trigger Editor"));
	setMinimumSize(900, 600);
	resize(1100, 700);

	// Menu + toolbar commands (TriggerEditor::commandManager_ set):
	// Debug / Undo / Redo / Find / open-for-copy.
	auto* menuBar = new QMenuBar(this);
	QMenu* menu = menuBar->addMenu(tr("&Edit"));
	actUndo_ = menu->addAction(tr("&Undo"));
	actUndo_->setShortcut(QKeySequence::Undo);
	actRedo_ = menu->addAction(tr("&Redo"));
	actRedo_->setShortcut(QKeySequence::Redo);
	menu->addSeparator();
	QAction* actFind = menu->addAction(tr("&Find..."));
	actFind->setShortcut(QKeySequence::Find);
	connect(actUndo_, &QAction::triggered, this, &TriggerEditorDialog::onUndo);
	connect(actRedo_, &QAction::triggered, this, &TriggerEditorDialog::onRedo);
	connect(actFind, &QAction::triggered, this, &TriggerEditorDialog::onFind);

	graph_ = new TriggerGraphView(view_, this);
	classTree_ = new TriggerClassTree(view_, this);
	miniMap_ = new TriggerMiniMapPanel(view_, graph_, this);
	conditionPanel_ = new TriggerConditionPanel(view_, graph_, this);
	debugger_ = new TriggerDebuggerPanel(view_, graph_, this);
	propertyTree_ = new PropertyTree(this);
	savePropsButton_ = new QPushButton(tr("Save chain"), this);
	savePropsButton_->setToolTip(tr("Write the .scr file now (Accept saves too)"));

	// Left column: palette over minimap (TriggerEditor::vSplitter2_ held
	// actionsTree_ at 0.33 + miniMap_ at 0.66, then the property tree).
	auto* leftCol = new QWidget(this);
	auto* leftLayout = new QVBoxLayout(leftCol);
	leftLayout->setContentsMargins(0, 0, 0, 0);
	auto* leftSplit = new QSplitter(Qt::Vertical, leftCol);
	leftSplit->addWidget(classTree_);
	leftSplit->addWidget(miniMap_);
	leftSplit->setStretchFactor(0, 1);
	leftSplit->setStretchFactor(1, 2);
	leftLayout->addWidget(leftSplit);

	// Right column: property tree + apply, condition panel, debugger.
	auto* rightCol = new QWidget(this);
	auto* rightLayout = new QVBoxLayout(rightCol);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->addWidget(propertyTree_, 3);
	rightLayout->addWidget(savePropsButton_);
	rightLayout->addWidget(conditionPanel_, 2);
	rightLayout->addWidget(debugger_, 1);

	// Main split: left | graph | right (TriggerEditor::hSplitter_ 0.2/0.7 +
	// vSplitter1_ 0.85 for the graph side).
	auto* mainSplit = new QSplitter(Qt::Horizontal, this);
	mainSplit->addWidget(leftCol);
	mainSplit->addWidget(graph_);
	mainSplit->addWidget(rightCol);
	mainSplit->setStretchFactor(0, 2);
	mainSplit->setStretchFactor(1, 7);
	mainSplit->setStretchFactor(2, 3);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->setMenuBar(menuBar);
	layout->addWidget(mainSplit, 1);
	layout->addWidget(buttons);

	connect(graph_, &TriggerGraphView::selectionChanged, this, &TriggerEditorDialog::onGraphSelection);
	connect(graph_, &TriggerGraphView::triggersChanged, this, &TriggerEditorDialog::reloadAll);
	connect(graph_, &TriggerGraphView::editTrigger, this, &TriggerEditorDialog::onEditTriggerProps);
	connect(graph_, &TriggerGraphView::editConditions, this, &TriggerEditorDialog::onEditConditions);
	connect(graph_, &TriggerGraphView::editAction, this, &TriggerEditorDialog::onEditAction);
	connect(classTree_, &TriggerClassTree::actionTypeSelected, this,
	        [this](int typeIndex){
		// Assign the palette action type to the selected trigger
		// (ClassTreeRow::onDrag created a trigger; here the drop target is
		// the selection — the graph context menu creates new triggers).
		if(!graph_ || !view_)
			return;
		const std::set<int> sel = graph_->selectedTriggers();
		if(sel.size() != 1)
			return;
		if(view_->triggerSetActionType(*sel.begin(), typeIndex))
			reloadAll();
	});
	connect(classTree_, &TriggerClassTree::conditionTypeSelected, this,
	        [this](int typeIndex){
		if(!graph_ || !view_)
			return;
		const std::set<int> sel = graph_->selectedTriggers();
		if(sel.size() != 1)
			return;
		if(view_->triggerSetConditionType(*sel.begin(), typeIndex))
			reloadAll();
	});
	connect(savePropsButton_, &QPushButton::clicked, this, &TriggerEditorDialog::onSaveProps);
}

TriggerEditorDialog::~TriggerEditorDialog()
{
	if(view_)
		view_->triggerSessionClose();
}

bool TriggerEditorDialog::openChain(const std::string& filePath)
{
	if(!view_)
		return false;
	filePath_ = filePath;
	if(!view_->triggerSessionOpen(filePath_))
		return false;
	setWindowTitle(tr("Trigger Editor — %1").arg(QString::fromStdString(view_->triggerChainName())));
	reloadAll();
	// The original auto-opened the debugger when logData was non-empty.
	debugger_->reload();
	const int result = exec();
	if(result == QDialog::Accepted)
		return view_->triggerSessionSave();
	view_->triggerSessionClose();
	return false;
}

void TriggerEditorDialog::reloadAll()
{
	if(!view_)
		return;
	graph_->reload();
	miniMap_->reload();
	classTree_->reload();
	loadPropertyPanel();
	actUndo_->setEnabled(view_->triggerCanUndo());
	actRedo_->setEnabled(view_->triggerCanRedo());
}

void TriggerEditorDialog::loadPropertyPanel()
{
	if(!view_ || !graph_)
		return;
	const std::set<int> sel = graph_->selectedTriggers();
	if(sel.size() == 1){
		editor::PropertyRow* root = view_->triggerTree(*sel.begin());
		propertyTree_->setRoot(root);
		delete root;
	}
	else{
		editor::PropertyRow* root = view_->triggerChainTree();
		propertyTree_->setRoot(root);
		delete root;
	}
}

void TriggerEditorDialog::onGraphSelection()
{
	loadPropertyPanel();
	miniMap_->update();
}

void TriggerEditorDialog::onEditTriggerProps()
{
	loadPropertyPanel();
	propertyTree_->setFocus();
}

void TriggerEditorDialog::onEditConditions()
{
	conditionPanel_->setFocus();
}

void TriggerEditorDialog::onEditAction()
{
	loadPropertyPanel();
	propertyTree_->setFocus();
}

void TriggerEditorDialog::onUndo()
{
	if(view_ && view_->triggerUndo())
		reloadAll();
}

void TriggerEditorDialog::onRedo()
{
	if(view_ && view_->triggerRedo())
		reloadAll();
}

void TriggerEditorDialog::onFind()
{
	// TriggerView::find matched name/condition/action substrings and
	// selected every hit. Qt version: ask for a substring, select hits.
	if(!view_)
		return;
	const QString text = QInputDialog::getText(this, tr("Find trigger"), tr("Name contains:"));
	if(text.isEmpty())
		return;
	std::vector<TriggerInfo> triggers;
	view_->triggerList(triggers);
	graph_->clearSelection();
	bool first = true;
	for(size_t i = 0; i < triggers.size(); ++i){
		if(QString::fromStdString(triggers[i].name).contains(text, Qt::CaseInsensitive)){
			graph_->selectTrigger((int)i, !first);
			first = false;
		}
	}
	loadPropertyPanel();
}

void TriggerEditorDialog::onSaveProps()
{
	// The property panel is read-only display in this pass (PropertyTree
	// has no editors yet), so there is no tree to write back — every graph
	// mutation already saved an undo step engine-side. This button writes
	// the .scr file now (Accept saves too), so work is not lost if the
	// dialog is cancelled afterwards.
	if(view_)
		view_->triggerSessionSave();
}
