// PropertyTree.cpp — see header.

#include "PropertyTree.h"

#include <QTreeWidgetItem>

#include "PropertyDelegates.h"
#include "PropertyText.h"
#include "editor/PropertyRows.h"   // concrete rows (engine-free)

PropertyTree::PropertyTree(QWidget* parent)
	: QTreeWidget(parent)
{
	setHeaderLabels({ tr("Property"), tr("Value") });
	setColumnCount(2);
	setRootIsDecorated(true);
	setAlternatingRowColors(true);
	setEditTriggers(QAbstractItemView::DoubleClicked |
	                QAbstractItemView::SelectedClicked |
	                QAbstractItemView::EditKeyPressed);
	setItemDelegateForColumn(1, new PropertyRowDelegate(this));
	connect(this, &QTreeWidget::itemChanged, this, &PropertyTree::onItemChanged);
	connect(this, &QTreeWidget::itemClicked, this, &PropertyTree::onItemClicked);
	connect(this, &QTreeWidget::itemDoubleClicked, this, &PropertyTree::onItemDoubleClicked);
}

PropertyTree::~PropertyTree()
{
	delete root_;
}

void PropertyTree::setRoot(editor::PropertyRow* root)
{
	building_ = true;
	blockSignals(true);
	clear();
	delete root_;
	root_ = root;
	if(root_){
		for(editor::PropertyRow* child : root_->children())
			buildItem(nullptr, child);
	}
	blockSignals(false);
	building_ = false;
}

editor::PropertyRow* PropertyTree::currentRow() const
{
	QTreeWidgetItem* item = currentItem();
	return item ? item->data(0, Qt::UserRole).value<editor::PropertyRow*>() : nullptr;
}

void PropertyTree::refreshItem(QTreeWidgetItem* item, editor::PropertyRow* row)
{
	item->setText(1, propertytext::displayRowText(row));
	if(row->kind() == editor::RowKind::Bool){
		auto* boolRow = static_cast<editor::PropertyRowBool*>(row);
		item->setCheckState(1, boolRow->value() ? Qt::Checked : Qt::Unchecked);
	}
}

void PropertyTree::buildItem(QTreeWidgetItem* parentItem, editor::PropertyRow* row)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(parentItem ? parentItem : (QTreeWidgetItem*)invisibleRootItem());

	// Field name: prefer the alt name (the human-readable label), fall back
	// to the raw name.
	const std::string& label = !row->nameAlt().empty() ? row->nameAlt() : row->name();
	item->setText(0, QString::fromStdString(label));
	item->setData(0, Qt::UserRole, QVariant::fromValue(row));
	refreshItem(item, row);

	using editor::RowKind;
	switch(row->kind()){
	case RowKind::Bool:
		// The kdw bool row toggled on click (checkbox icon); the Qt twin is
		// a real checkbox, toggled in onItemClicked.
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		break;
	case RowKind::Container:
		item->setExpanded(true);
		for(editor::PropertyRow* child : row->children())
			buildItem(item, child);
		break;
	case RowKind::Color:
	case RowKind::Flags:
		// Dialog editors on double-click (no inline editor).
		break;
	case RowKind::Other:
		// Unregistered leaf type — display only.
		break;
	default:
		// Delegate editors (text/number/enum/combo/ranged/file).
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		break;
	}
}

void PropertyTree::onItemChanged(QTreeWidgetItem* item, int column)
{
	if(building_ || applying_ || column != 1 || !item)
		return;
	QVariant data = item->data(0, Qt::UserRole);
	if(!data.isValid())
		return;
	editor::PropertyRow* row = data.value<editor::PropertyRow*>();
	if(!row || row->isContainer())
		return;
	// Safety net for direct text edits (the delegate's setModelData writes
	// typed values itself with signals blocked). Re-parse with the row's
	// source encoding; the row keeps the old value when the text does not
	// parse.
	applying_ = true;
	if(row->kind() == editor::RowKind::Bool){
		auto* boolRow = static_cast<editor::PropertyRowBool*>(row);
		boolRow->setValue(item->checkState(1) == Qt::Checked);
	}
	else
		propertytext::commitRowText(row, item->text(1));
	refreshItem(item, row);
	applying_ = false;
}

void PropertyTree::onItemClicked(QTreeWidgetItem* item, int column)
{
	if(!item || column != 1)
		return;
	QVariant data = item->data(0, Qt::UserRole);
	if(!data.isValid())
		return;
	editor::PropertyRow* row = data.value<editor::PropertyRow*>();
	if(!row || row->kind() != editor::RowKind::Bool)
		return;
	// Toggle on value-cell click (kdw::PropertyRowBool::onActivate).
	auto* boolRow = static_cast<editor::PropertyRowBool*>(row);
	applying_ = true;
	boolRow->setValue(!boolRow->value());
	refreshItem(item, row);
	applying_ = false;
}

void PropertyTree::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
	if(!item || column != 1)
		return;
	QVariant data = item->data(0, Qt::UserRole);
	if(!data.isValid())
		return;
	editor::PropertyRow* row = data.value<editor::PropertyRow*>();
	if(!row)
		return;
	// Dialog editors (kdw::PropertyRowColor::onActivate opens the color
	// chooser, the bitvector row its CheckComboBox).
	bool accepted = false;
	if(row->kind() == editor::RowKind::Color)
		accepted = PropertyRowDelegate::editColor(static_cast<editor::PropertyRowColor*>(row), this);
	else if(row->kind() == editor::RowKind::Flags)
		accepted = PropertyRowDelegate::editFlags(static_cast<editor::PropertyRowBitVector*>(row), this);
	if(accepted){
		applying_ = true;
		refreshItem(item, row);
		applying_ = false;
	}
}
