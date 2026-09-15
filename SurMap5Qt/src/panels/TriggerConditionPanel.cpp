// TriggerConditionPanel.cpp — see header.

#include "TriggerConditionPanel.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "RenderViewWidget.h"
#include "TriggerGraphView.h"
#include "editor/PropertyRow.h"

TriggerConditionPanel::TriggerConditionPanel(RenderViewWidget* view, TriggerGraphView* graph,
                                             QWidget* parent)
	: QWidget(parent)
	, view_(view)
	, graph_(graph)
{
	tree_ = new QTreeWidget(this);
	tree_->setHeaderLabels({ tr("Condition"), tr("Value") });
	tree_->setColumnCount(2);

	typeCombo_ = new QComboBox(this);
	auto* btnAdd = new QPushButton(tr("Set type"), this);
	auto* btnDel = new QPushButton(tr("Clear"), this);
	auto* btnInv = new QPushButton(tr("Invert"), this);

	auto* btnRow = new QHBoxLayout;
	btnRow->addWidget(typeCombo_, 1);
	btnRow->addWidget(btnAdd);
	btnRow->addWidget(btnDel);
	btnRow->addWidget(btnInv);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(tree_, 1);
	layout->addLayout(btnRow);

	connect(graph_, &TriggerGraphView::selectionChanged, this,
	        &TriggerConditionPanel::onGraphSelection);
	connect(btnAdd, &QPushButton::clicked, this, &TriggerConditionPanel::onAddCondition);
	connect(btnDel, &QPushButton::clicked, this, &TriggerConditionPanel::onDeleteCondition);
	connect(btnInv, &QPushButton::clicked, this, &TriggerConditionPanel::onInvertCondition);
	connect(typeCombo_, QOverload<int>::of(&QComboBox::activated),
	        this, &TriggerConditionPanel::onTypeActivated);

	if(view_){
		view_->triggerConditionTypes(typeNames_, typeNamesAlt_);
		for(size_t i = 0; i < typeNamesAlt_.size(); ++i)
			typeCombo_->addItem(QString::fromStdString(typeNamesAlt_[i]), (int)i);
	}
}

void TriggerConditionPanel::loadTrigger(int triggerIndex)
{
	triggerIndex_ = triggerIndex;
	rebuild();
}

void TriggerConditionPanel::onGraphSelection()
{
	if(!graph_)
		return;
	const std::set<int> sel = graph_->selectedTriggers();
	loadTrigger(sel.size() == 1 ? *sel.begin() : -1);
}

void TriggerConditionPanel::rebuild()
{
	tree_->clear();
	if(!view_ || triggerIndex_ < 0)
		return;
	editor::PropertyRow* root = view_->triggerConditionTree(triggerIndex_);
	if(!root)
		return;
	// Fully recursive: condition trees nest (ConditionSwitcher holds child
	// conditions, each with their own fields).
	for(editor::PropertyRow* child : root->children())
		buildRow(nullptr, child);
	delete root;
}

void TriggerConditionPanel::buildRow(QTreeWidgetItem* parentItem, editor::PropertyRow* row)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(parentItem
		? parentItem : (QTreeWidgetItem*)tree_->invisibleRootItem());
	const std::string& label = !row->nameAlt().empty() ? row->nameAlt() : row->name();
	item->setText(0, QString::fromStdString(label));
	QString value = QString::fromStdString(row->valueAsString());
	if(!row->derivedName().empty() && value.isEmpty())
		value = QString::fromStdString(row->derivedName());
	item->setText(1, value);
	item->setExpanded(true);
	for(editor::PropertyRow* sub : row->children())
		buildRow(item, sub);
}

void TriggerConditionPanel::buildItem(QTreeWidgetItem* parentItem, const QString& label,
                                      const QString& value)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(parentItem
		? parentItem : (QTreeWidgetItem*)tree_->invisibleRootItem());
	item->setText(0, label);
	item->setText(1, value);
	item->setExpanded(true);
}

void TriggerConditionPanel::onAddCondition()
{
	// "Set type" assigns the combo's condition type to the trigger
	// (ConditionViewer::createCondition + triggerSetConditionType).
	if(!view_ || triggerIndex_ < 0)
		return;
	const int typeIndex = typeCombo_->currentData().toInt();
	if(view_->triggerSetConditionType(triggerIndex_, typeIndex))
		rebuild();
}

void TriggerConditionPanel::onDeleteCondition()
{
	// Clear the trigger's condition (bridge typeIndex -1 path).
	if(!view_ || triggerIndex_ < 0)
		return;
	if(view_->triggerSetConditionType(triggerIndex_, -1))
		rebuild();
}

void TriggerConditionPanel::onInvertCondition()
{
	// Flip the root condition's inverted flag engine-side (ConditionSlot::
	// invert port): read the current flag from the serialized tree, toggle
	// it through the bridge, refresh.
	if(!view_ || triggerIndex_ < 0)
		return;
	editor::PropertyRow* root = view_->triggerConditionTree(triggerIndex_);
	bool inverted = false;
	bool found = false;
	if(root){
		std::vector<editor::PropertyRow*> stack = { root };
		while(!stack.empty() && !found){
			editor::PropertyRow* cur = stack.back();
			stack.pop_back();
			if(cur->name() == "inverted"){
				const std::string v = cur->valueAsString();
				inverted = (v == "true" || v == "1");
				found = true;
				break;
			}
			for(editor::PropertyRow* c : cur->children())
				stack.push_back(c);
		}
		delete root;
	}
	if(!found)
		return;
	if(view_->triggerSetConditionInverted(triggerIndex_, !inverted))
		rebuild();
}

void TriggerConditionPanel::onTypeActivated(int /*comboIndex*/)
{
	// Live-apply on activation would surprise; the Set-type button applies.
}
