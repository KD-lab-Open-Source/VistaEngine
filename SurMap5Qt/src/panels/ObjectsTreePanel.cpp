// ObjectsTreePanel.cpp — see header.

#include "ObjectsTreePanel.h"

#include "editor/EngineViewport.h"   // EngineViewport::ObjectTab (objects tree)
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
// vMap is engine-side; the Qt side reads world state via EngineViewport
// (worldLoaded() is the only public probe for now). The integration of
// object lists lands with the engine-side object bridge.

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
	// Map tab index to EngineViewport::ObjectTab (matches
	// CObjectsManagerTree::setTab's TAB_* enum).
	static const EngineViewport::ObjectTab kTabs[] = {
		EngineViewport::ObjectTab::Sources,
		EngineViewport::ObjectTab::Environment,
		EngineViewport::ObjectTab::Units,
		EngineViewport::ObjectTab::Cameras,
		EngineViewport::ObjectTab::Anchors,
	};

	// CObjectsManagerTree::rebuild rebuilt from the world's object list
	// (Sources, Environment, Units, Cameras, Anchors). The Qt editor asks
	// the engine side (EngineViewport::objectList) for the current world's
	// labels per tab; no world loaded = empty trees.
	for(int i = 0; i < tabs_->count(); ++i){
		QTreeWidget* tree = qobject_cast<QTreeWidget*>(tabs_->widget(i));
		if(!tree) continue;
		tree->clear();

		if(!viewport_)
			continue;

		// Ask the engine for this tab's labels. CObjectsManagerTree used
		// "<prefix> #N - <label>" with N running across types (Sources),
		// or per-name (Cameras/Anchors). Without a real list we only show
		// the type group; the engine-side object bridge will fill this in
		// when the editor links Universe.
		enum { kMaxLabels = 1024 };
		char* labels[kMaxLabels] = {};
		const int count = viewport_->objectList(kTabs[i], labels, kMaxLabels);
		if(count == 0){
			// No live objects yet — show the type group so the panel is
			// not empty.
			QTreeWidgetItem* root = new QTreeWidgetItem(tree);
			root->setText(0, tabs_->tabText(i));
		}
		else{
			for(int j = 0; j < count; ++j){
				QTreeWidgetItem* item = new QTreeWidgetItem(tree);
				item->setText(0, QString::fromUtf8(labels[j]));
				free(labels[j]);
			}
		}
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
