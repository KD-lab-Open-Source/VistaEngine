// PropertyTree.cpp — see header.

#include "PropertyTree.h"

#include <QTreeWidgetItem>

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
	connect(this, &QTreeWidget::itemChanged, this, &PropertyTree::onItemChanged);
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

void PropertyTree::buildItem(QTreeWidgetItem* parentItem, editor::PropertyRow* row)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(parentItem ? parentItem : (QTreeWidgetItem*)invisibleRootItem());

	// Field name: prefer the alt name (the human-readable label), fall back
	// to the raw name.
	const std::string& label = !row->nameAlt().empty() ? row->nameAlt() : row->name();
	item->setText(0, QString::fromStdString(label));
	item->setText(1, QString::fromStdString(row->valueAsString()));
	item->setData(0, Qt::UserRole, QVariant::fromValue(row));

	if(row->isContainer())
		item->setExpanded(true);
	else
		item->setFlags(item->flags() | Qt::ItemIsEditable);

	if(row->isContainer()){
		for(editor::PropertyRow* child : row->children())
			buildItem(item, child);
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
	const std::string text = item->text(1).toStdString();
	applying_ = true;
	if(row->setValueFromString(text))
		item->setText(1, QString::fromStdString(row->valueAsString())); // normalized form
	else
		item->setText(1, QString::fromStdString(row->valueAsString())); // revert
	applying_ = false;
}
