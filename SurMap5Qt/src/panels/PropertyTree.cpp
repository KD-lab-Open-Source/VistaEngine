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
}

void PropertyTree::setRoot(editor::PropertyRow* root)
{
	clear();
	if(!root)
		return;
	for(editor::PropertyRow* child : root->children())
		buildItem(nullptr, child);
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

	if(row->isContainer()){
		item->setExpanded(true);
		for(editor::PropertyRow* child : row->children())
			buildItem(item, child);
	}
}