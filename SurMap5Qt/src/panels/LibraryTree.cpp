// LibraryTree.cpp — see header.

#include "LibraryTree.h"

#include <QTreeWidgetItem>

#include <map>

#include "PropertyText.h"   // displayBytes: UTF-8 or cp1251 names

LibraryTree::LibraryTree(QWidget* parent)
	: QTreeWidget(parent)
{
	setHeaderHidden(true);
	setRootIsDecorated(true);
	connect(this, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
		if(current)
			emit elementSelected(current->data(0, Qt::UserRole).toInt());
	});
}

QTreeWidgetItem* LibraryTree::groupItem(const std::string& path)
{
	if(path.empty())
		return nullptr;
	// Walk/create the '\\' separated folders, mirroring buildLibraryTree's
	// groups map (full path -> folder).
	QTreeWidgetItem* parent = nullptr;
	std::string current;
	size_t pos = 0;
	while(true){
		size_t sep = path.find('\\', pos);
		const std::string part = (sep == std::string::npos) ? path.substr(pos)
		                                                    : path.substr(pos, sep - pos);
		if(!part.empty()){
			if(!current.empty())
				current += '\\';
			current += part;
			auto it = groupCache_.find(current);
			if(it != groupCache_.end()){
				parent = it->second;
			}
			else{
				QTreeWidgetItem* folder =
					new QTreeWidgetItem(parent ? parent : invisibleRootItem());
				folder->setText(0, propertytext::displayBytes(part));
				folder->setData(0, Qt::UserRole, -1);
				folder->setFlags(folder->flags() & ~Qt::ItemIsSelectable);
				folder->setExpanded(true);
				groupCache_[current] = folder;
				parent = folder;
			}
		}
		if(sep == std::string::npos)
			break;
		pos = sep + 1;
	}
	return parent;
}

void LibraryTree::setLibrary(IWorldBridge* bridge, const std::string& libraryName)
{
	clear();
	groupCache_.clear();
	if(!bridge)
		return;
	std::vector<std::string> names;
	std::vector<std::string> groups;
	bridge->libraryElementNames(libraryName, names);
	bridge->libraryElementGroups(libraryName, groups);

	// Predefined groups first (buildLibraryTree created the combo-list
	// folders before placing elements).
	const std::string comboList = bridge->libraryGroupsComboList(libraryName);
	size_t pos = 0;
	while(pos <= comboList.size()){
		size_t sep = comboList.find('|', pos);
		const std::string group = (sep == std::string::npos) ? comboList.substr(pos)
		                                                     : comboList.substr(pos, sep - pos);
		if(!group.empty())
			groupItem(group);
		if(sep == std::string::npos)
			break;
		pos = sep + 1;
	}

	// Elements under their group folder (empty group = top level).
	// The row's UserRole is the ELEMENT index addressing the engine.
	QTreeWidgetItem* firstElement = nullptr;
	for(size_t i = 0; i < names.size(); ++i){
		if(names[i].empty())
			continue;
		QTreeWidgetItem* parent = nullptr;
		if(i < groups.size() && !groups[i].empty())
			parent = groupItem(groups[i]);
		QTreeWidgetItem* item = new QTreeWidgetItem(parent ? parent : invisibleRootItem());
		// Element names are display-only, indices address the engine.
		item->setText(0, propertytext::displayBytes(names[i]));
		item->setData(0, Qt::UserRole, (int)i);
		if(!firstElement)
			firstElement = item;
	}
	if(firstElement)
		setCurrentItem(firstElement);
}

int LibraryTree::currentIndex() const
{
	QTreeWidgetItem* item = currentItem();
	return item ? item->data(0, Qt::UserRole).toInt() : -1;
}
