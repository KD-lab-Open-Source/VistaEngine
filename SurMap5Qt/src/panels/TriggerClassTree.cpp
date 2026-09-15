// TriggerClassTree.cpp — see header.

#include "TriggerClassTree.h"

#include <QTreeWidgetItem>

#include "RenderViewWidget.h"

namespace {
// Group key: the factory names are "path\\Type" style (comboStringsAlt
// carries the translated path); group by the part before the last '\\',
// like the kdw ObjectsTree grouped ClassTreeRow children.
QString groupOf(const QString& nameAlt)
{
	const int pos = nameAlt.lastIndexOf('\\');
	if(pos <= 0)
		return QString();
	return nameAlt.left(pos);
}

QString shortOf(const QString& nameAlt)
{
	const int pos = nameAlt.lastIndexOf('\\');
	if(pos < 0)
		return nameAlt;
	return nameAlt.mid(pos + 1);
}
}

TriggerClassTree::TriggerClassTree(RenderViewWidget* view, QWidget* parent)
	: QTreeWidget(parent)
	, view_(view)
{
	setHeaderHidden(true);
	setDragEnabled(true);
	connect(this, &QTreeWidget::currentItemChanged, this,
	        [this](QTreeWidgetItem* current, QTreeWidgetItem*){
		if(!current)
			return;
		const int typeIndex = current->data(0, Qt::UserRole).toInt();
		if(typeIndex < 0)
			return;
		if(current->data(0, Qt::UserRole + 1).toBool())
			emit actionTypeSelected(typeIndex);
		else
			emit conditionTypeSelected(typeIndex);
	});
}

void TriggerClassTree::reload()
{
	clear();
	if(!view_)
		return;
	std::vector<std::string> names, namesAlt;
	view_->triggerActionTypes(names, namesAlt);
	QTreeWidgetItem* actionsRoot = new QTreeWidgetItem(invisibleRootItem());
	actionsRoot->setText(0, tr("Actions"));
	actionsRoot->setData(0, Qt::UserRole, -1);
	actionsRoot->setExpanded(true);
	buildGroup(actionsRoot, names, namesAlt, true);

	view_->triggerConditionTypes(names, namesAlt);
	QTreeWidgetItem* condsRoot = new QTreeWidgetItem(invisibleRootItem());
	condsRoot->setText(0, tr("Conditions"));
	condsRoot->setData(0, Qt::UserRole, -1);
	condsRoot->setExpanded(true);
	buildGroup(condsRoot, names, namesAlt, false);
}

void TriggerClassTree::buildGroup(QTreeWidgetItem* parent,
                                  const std::vector<std::string>& names,
                                  const std::vector<std::string>& namesAlt,
                                  bool isAction)
{
	// Group rows by the translated path prefix; the type index rides in
	// UserRole (like LibraryTree stores the element index).
	for(size_t i = 0; i < names.size(); ++i){
		const QString alt = i < namesAlt.size()
			? QString::fromStdString(namesAlt[i]) : QString::fromStdString(names[i]);
		const QString group = groupOf(alt);
		QTreeWidgetItem* holder = parent;
		if(!group.isEmpty()){
			holder = nullptr;
			for(int c = 0; c < parent->childCount(); ++c){
				if(parent->child(c)->text(0) == group){
					holder = parent->child(c);
					break;
				}
			}
			if(!holder){
				holder = new QTreeWidgetItem(parent);
				holder->setText(0, group);
				holder->setData(0, Qt::UserRole, -1);
			}
		}
		QTreeWidgetItem* item = new QTreeWidgetItem(holder);
		item->setText(0, shortOf(alt));
		item->setToolTip(0, QString::fromStdString(names[i]));
		item->setData(0, Qt::UserRole, (int)i);
		item->setData(0, Qt::UserRole + 1, isAction);
	}
}

int TriggerClassTree::currentActionType() const
{
	QTreeWidgetItem* item = currentItem();
	if(!item || !item->data(0, Qt::UserRole + 1).toBool())
		return -1;
	return item->data(0, Qt::UserRole).toInt();
}

int TriggerClassTree::currentConditionType() const
{
	QTreeWidgetItem* item = currentItem();
	if(!item || item->data(0, Qt::UserRole + 1).toBool())
		return -1;
	return item->data(0, Qt::UserRole).toInt();
}
