// ObjectsTreePanel.cpp — see header.

#include "ObjectsTreePanel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

ObjectsTreePanel::ObjectsTreePanel(QWidget* parent)
	: QWidget(parent)
{
	tabs_ = new QTabWidget(this);

	// The original's typeTabs_ (ObjectsManagerWindow.cpp:99-103): the object
	// types the tree shows. The port keeps the same tab order and labels.
	const QStringList tabLabels = {
		tr("Sources"), tr("Environment"), tr("Units"), tr("Cameras"), tr("Anchors"),
	};
	for(const QString& label : tabLabels){
		auto* tree = new QTreeWidget(tabs_);
		tree->setHeaderHidden(true);
		tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
		tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
		tree->setIndentation(12);
		tree->setContextMenuPolicy(Qt::DefaultContextMenu);
		tabs_->addTab(tree, label);
	}
	tabs_->setCurrentIndex(0);   // SetCurSel(TAB_SOURCES)

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tabs_);

	rebuild();
}

QTreeWidget* ObjectsTreePanel::currentTree() const
{
	return qobject_cast<QTreeWidget*>(tabs_->currentWidget());
}

int ObjectsTreePanel::currentTab() const
{
	return tabs_->currentIndex();
}

void ObjectsTreePanel::rebuild()
{
	// CObjectsManagerWindow's tree groups (WorldTreeObjects.h: units, sources,
	// environment objects, anchors, cameras). Empty until a world contributes
	// objects; the group row keeps each tab stable across world loads.
	for(int t = 0; t < tabs_->count(); ++t){
		auto* tree = qobject_cast<QTreeWidget*>(tabs_->widget(t));
		tree->clear();
		auto* group = new QTreeWidgetItem(tree);
		group->setText(0, tabs_->tabText(t));
		group->setData(0, Qt::UserRole, QString());   // empty = group row
		group->setExpanded(true);
	}
}

void ObjectsTreePanel::contextMenuEvent(QContextMenuEvent* event)
{
	// The original's NM_RCLICK on the objects tree: a popup with delete +
	// rename (ObjectsManagerTree.cpp had ID_POPUP_* handlers). Deleting a
	// group row is a no-op (the groups are fixed).
	QTreeWidget* tree = currentTree();
	QTreeWidgetItem* item = tree ? tree->itemAt(event->pos()) : nullptr;
	if(!item)
		return;
	QMenu menu(this);
	QAction* renameAction = menu.addAction(tr("Rename..."));
	QAction* deleteAction = menu.addAction(tr("Delete"));
	QAction* chosen = menu.exec(event->globalPos());
	if(chosen == renameAction)
		renameSelected();
	else if(chosen == deleteAction)
		deleteSelected();
}

void ObjectsTreePanel::deleteSelected()
{
	// Deleting world objects lands with the world object list; the group
	// rows themselves are fixed, so this only removes a selected child if any.
	QTreeWidget* tree = currentTree();
	if(!tree)
		return;
	QTreeWidgetItem* item = tree->currentItem();
	if(!item || !item->parent())
		return;
	delete item;
}

void ObjectsTreePanel::renameSelected()
{
	// Renaming is inline-editable in the original; QTreeWidgetItem's
	// setFlags(Qt::ItemIsEditable) + editItem do the same. Applied to a
	// child row (a world object); group rows are fixed.
	QTreeWidget* tree = currentTree();
	if(!tree)
		return;
	QTreeWidgetItem* item = tree->currentItem();
	if(!item || !item->parent())
		return;
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	tree->editItem(item, 0);
}
