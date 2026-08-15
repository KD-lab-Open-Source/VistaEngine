// ObjectsTreePanel.cpp — see header.

#include "ObjectsTreePanel.h"

#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

ObjectsTreePanel::ObjectsTreePanel(QWidget* parent)
	: QWidget(parent)
{
	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree_->setIndentation(12);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tree_);

	rebuild();
}

void ObjectsTreePanel::rebuild()
{
	// CObjectsManagerWindow's tree groups (WorldTreeObjects.h: units, sources,
	// environment objects, anchors, cameras). Empty until a world contributes
	// objects; the groups keep the panel stable across world loads.
	tree_->clear();

	auto addGroup = [this](const QString& name){
		auto* group = new QTreeWidgetItem(tree_);
		group->setText(0, name);
		group->setExpanded(true);
	};

	addGroup(tr("Units"));
	addGroup(tr("Sources"));
	addGroup(tr("Environment"));
	addGroup(tr("Anchors"));
	addGroup(tr("Cameras"));
}
