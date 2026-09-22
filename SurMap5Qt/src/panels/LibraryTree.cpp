// LibraryTree.cpp — see header.

#include "LibraryTree.h"

#include <QTreeWidgetItem>

#include "PropertyText.h"   // displayBytes: UTF-8 or cp1251 element names

LibraryTree::LibraryTree(QWidget* parent)
	: QTreeWidget(parent)
{
	setHeaderHidden(true);
	setRootIsDecorated(false);
	connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
		if(current)
			emit elementSelected(current->data(0, Qt::UserRole).toInt());
	});
}

void LibraryTree::setLibrary(IWorldBridge* bridge, const std::string& libraryName)
{
	clear();
	if(!bridge)
		return;
	std::vector<std::string> names;
	bridge->libraryElementNames(libraryName, names);
	for(int i = 0; i < (int)names.size(); ++i){
		QTreeWidgetItem* item = new QTreeWidgetItem(invisibleRootItem());
		// Element names come from data files in mixed encodings (cp1251
		// Russian names); display-only, indices address the engine.
		item->setText(0, propertytext::displayBytes(names[i]));
		item->setData(0, Qt::UserRole, i);
	}
	if(topLevelItemCount() > 0)
		setCurrentItem(topLevelItem(0));
}

int LibraryTree::currentIndex() const
{
	QTreeWidgetItem* item = currentItem();
	return item ? item->data(0, Qt::UserRole).toInt() : -1;
}