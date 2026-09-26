// ToolsTreePanel.cpp — see header.

#include "ToolsTreePanel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QSettings>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <map>
#include <string>
#include <vector>

#include "tools/ToolManager.h"
#include "tools/UnitTool.h"
#include "tools/SourceTool.h"
#include "tools/EnvironmentTool.h"
#include "editor/EditorTool.h"     // IWorldBridge
#include "panels/PropertyText.h"   // propertytext::displayBytes (cp1251 names)

namespace {
// Item data roles: the tool index plus the object-catalog roles.
enum
{
	ToolIndexRole = Qt::UserRole,
	CatalogKindRole,
	CatalogIndexRole,
	CatalogTextRole,
};

enum CatalogKind
{
	CatalogNone = 0,
	CatalogUnit = 1,
	CatalogSource = 2,
	CatalogEnvironment = 3,
};

// The ToolManager indices of the object tools.
const int kUnitTool = 9;
const int kSourceTool = 10;
const int kAnchorTool = 11;
const int kEnvironmentTool = 12;

// The display name of a model path: the file part after the last separator.
QString modelLabel(const std::string& path)
{
	const size_t pos = path.find_last_of("\\/");
	std::string base = pos == std::string::npos ? path : path.substr(pos + 1);
	if(base.size() > 4 && base.compare(base.size() - 4, 4, ".3dx") == 0)
		base.erase(base.size() - 4);
	return propertytext::displayBytes(base);
}
}

ToolsTreePanel::ToolsTreePanel(ToolManager* tools, QWidget* parent)
	: QWidget(parent)
	, tools_(tools)
{
	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setSelectionMode(QAbstractItemView::SingleSelection);
	tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree_->setIndentation(12);
	tree_->setContextMenuPolicy(Qt::DefaultContextMenu);

	buildTree();

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tree_);

	// The original's TVN_SELCHANGED: picking a row switches to it.
	connect(tree_, &QTreeWidget::itemActivated, this, &ToolsTreePanel::onItemActivated);
	connect(tree_, &QTreeWidget::itemClicked, this, &ToolsTreePanel::onItemActivated);
}

void ToolsTreePanel::buildTree()
{
	tree_->clear();

	auto* root = new QTreeWidgetItem(tree_);
	root->setText(0, tr("Tools"));
	root->setData(0, ToolIndexRole, -1);   // no tool index — a folder row
	root->setExpanded(true);

	auto addFolder = [&](const char* label)->QTreeWidgetItem*{
		auto* item = new QTreeWidgetItem(root);
		item->setText(0, tr(label));
		item->setData(0, ToolIndexRole, -1);   // a folder row
		item->setExpanded(true);
		return item;
	};
	auto addToolRow = [&](QTreeWidgetItem* parent, int index){
		if(index < 0 || index >= (int)tools_->tools().size())
			return;
		auto* item = new QTreeWidgetItem(parent);
		item->setText(0, tr(tools_->tools()[index]->name()));
		item->setData(0, ToolIndexRole, index);
	};

	// The original's tree folders (the transform set is on the toolbar, not
	// here). The terrain/texture rows are the tools themselves; the object
	// folders hold the catalog.
	QTreeWidgetItem* terrain = addFolder("Object-based terrain");
	addToolRow(terrain, 4);   // Toolzer
	addToolRow(terrain, 5);   // Kind
	addToolRow(terrain, 7);   // GeoNet
	addToolRow(terrain, 8);   // GeoTx

	QTreeWidgetItem* textures = addFolder("Textures");
	addToolRow(textures, 6);  // ColorPic

	unitsFolder_ = addFolder("Units");
	sourcesFolder_ = addFolder("Sources");

	QTreeWidgetItem* anchors = addFolder("Anchors");
	addToolRow(anchors, kAnchorTool);

	environmentFolder_ = addFolder("Environmental Objects");

	rebuildCatalog();
	tree_->expandAll();
}

void ToolsTreePanel::rebuildCatalog()
{
	// Drop the previous catalog (the world's libraries change on every load).
	auto clearChildren = [](QTreeWidgetItem* item){
		if(!item)
			return;
		while(item->childCount() > 0)
			delete item->takeChild(0);
	};
	clearChildren(unitsFolder_);
	clearChildren(sourcesFolder_);
	clearChildren(environmentFolder_);

	IWorldBridge* bridge = tools_ ? tools_->bridge() : nullptr;
	if(!bridge)
		return;

	if(unitsFolder_){
		std::vector<std::string> names;
		bridge->unitAttributeNames(names);
		// SurToolUnitFolder grouped the units per player/race; libraryKey() is
		// "<name>, <RACE>" (e.g. "Ядро, GROUND"), so the race after the comma
		// becomes the folder — the original's "[GROUND]" node.
		std::map<QString, QTreeWidgetItem*> groups;
		for(size_t i = 0; i < names.size(); ++i){
			if(!bridge->unitAttributePlaceable((int)i))
				continue;
			const QString full = propertytext::displayBytes(names[i]);
			QString groupLabel;
			QString leafLabel = full;
			const int comma = full.lastIndexOf(", ");
			if(comma >= 0){
				groupLabel = full.mid(comma + 2).trimmed();
				leafLabel = full.left(comma);
			}
			QTreeWidgetItem*& group = groups[groupLabel];
			if(!group){
				group = new QTreeWidgetItem(unitsFolder_);
				group->setText(0, groupLabel.isEmpty() ? tr("Other") : QString("[%1]").arg(groupLabel));
				group->setData(0, ToolIndexRole, -1);
				group->setExpanded(false);
			}
			auto* leaf = new QTreeWidgetItem(group);
			leaf->setText(0, leafLabel);
			leaf->setData(0, CatalogKindRole, (int)CatalogUnit);
			leaf->setData(0, CatalogIndexRole, (int)i);   // the library index
		}
		unitsFolder_->setExpanded(true);
	}

	if(sourcesFolder_){
		std::vector<std::string> names;
		bridge->sourceNames(names);
		for(size_t i = 0; i < names.size(); ++i){
			auto* leaf = new QTreeWidgetItem(sourcesFolder_);
			leaf->setText(0, propertytext::displayBytes(names[i]));
			leaf->setData(0, CatalogKindRole, (int)CatalogSource);
			leaf->setData(0, CatalogIndexRole, (int)i);
		}
		sourcesFolder_->setExpanded(false);
	}

	if(environmentFolder_){
		std::vector<std::string> models;
		bridge->environmentModelNames(models);
		std::vector<std::string> typeNames;
		bridge->environmentTypeNames(typeNames);
		// One folder per inferred EnvironmentType (the original grouped the
		// environment nodes by type); -1 (unrecognised) lands in "Other".
		std::map<int, QTreeWidgetItem*> groups;
		for(const std::string& model : models){
			bool vertical = false;
			const int type = environmentTypeForModel(model, vertical);
			QTreeWidgetItem*& group = groups[type];
			if(!group){
				group = new QTreeWidgetItem(environmentFolder_);
				const std::string label =
					(type >= 0 && type < (int)typeNames.size()) ? typeNames[type] : std::string("Other");
				group->setText(0, propertytext::displayBytes(label));
				group->setData(0, ToolIndexRole, -1);
				group->setExpanded(false);
			}
			auto* leaf = new QTreeWidgetItem(group);
			leaf->setText(0, modelLabel(model));
			leaf->setData(0, CatalogKindRole, (int)CatalogEnvironment);
			leaf->setData(0, CatalogTextRole, QString::fromStdString(model));
		}
		environmentFolder_->setExpanded(true);
	}
}

void ToolsTreePanel::syncToTool()
{
	// Highlight the tree row matching the current tool (the toolbar and the
	// tree both switch the tool; whichever was used last wins). Catalog leaves
	// carry no tool index, so they are skipped.
	const int index = tools_->currentIndex();
	for(int i = 0; i < tree_->topLevelItemCount(); ++i){
		QTreeWidgetItem* root = tree_->topLevelItem(i);
		for(int j = 0; j < root->childCount(); ++j){
			QTreeWidgetItem* group = root->child(j);
			for(int k = 0; k < group->childCount(); ++k){
				QTreeWidgetItem* item = group->child(k);
				const QVariant data = item->data(0, ToolIndexRole);
				if(data.isValid() && data.toInt() == index){
					tree_->setCurrentItem(item);
					return;
				}
			}
		}
	}
}

void ToolsTreePanel::onItemActivated(QTreeWidgetItem* item, int /*column*/)
{
	if(!item)
		return;
	const int kind = item->data(0, CatalogKindRole).toInt();
	if(kind == CatalogUnit || kind == CatalogSource || kind == CatalogEnvironment){
		// A catalog leaf: switch to its tool and make it the target.
		const int toolIndex =
			kind == CatalogUnit ? kUnitTool :
			kind == CatalogSource ? kSourceTool : kEnvironmentTool;
		if(toolIndex >= (int)tools_->tools().size())
			return;
		if(kind == CatalogUnit){
			if(UnitTool* tool = dynamic_cast<UnitTool*>(tools_->tools()[toolIndex]))
				tool->setAttributeIndex(item->data(0, CatalogIndexRole).toInt());
		} else if(kind == CatalogSource){
			if(SourceTool* tool = dynamic_cast<SourceTool*>(tools_->tools()[toolIndex]))
				tool->setSourceIndex(item->data(0, CatalogIndexRole).toInt());
		} else {
			if(EnvironmentTool* tool = dynamic_cast<EnvironmentTool*>(tools_->tools()[toolIndex]))
				tool->setModel(item->data(0, CatalogTextRole).toString().toStdString());
		}
		emit toolSelected(toolIndex);
		return;
	}
	const QVariant data = item->data(0, ToolIndexRole);
	if(data.isValid() && data.toInt() >= 0)
		emit toolSelected(data.toInt());
}

void ToolsTreePanel::contextMenuEvent(QContextMenuEvent* event)
{
	// The original's NM_RCLICK on the tools tree: ID_POPUP_DELETE and the
	// label-edit. The folder rows and the catalog leaves are structural, so
	// only tool rows get the menu.
	QTreeWidgetItem* item = tree_->itemAt(event->pos());
	if(!item)
		return;
	const QVariant data = item->data(0, ToolIndexRole);
	if(!data.isValid() || data.toInt() < 0)
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

void ToolsTreePanel::deleteSelected()
{
	// The original's ID_POPUP_DELETE removed the tool from the tree (the
	// tool itself stays registered). Here it only removes the tree row.
	QTreeWidgetItem* item = tree_->currentItem();
	if(item && item->parent())
		delete item;
}

void ToolsTreePanel::renameSelected()
{
	// The original's TVN_BEGINLABELEDIT/ENDLABELEDIT made tool rows editable;
	// the label is what the tools tree showed.
	QTreeWidgetItem* item = tree_->currentItem();
	if(!item)
		return;
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	tree_->editItem(item, 0);
}

void ToolsTreePanel::saveState()
{
	QSettings s;
	if(tree_->topLevelItemCount() > 0 && tree_->topLevelItem(0))
		s.setValue("toolsTree/expanded", tree_->topLevelItem(0)->isExpanded());
	QTreeWidgetItem* cur = tree_->currentItem();
	s.setValue("toolsTree/currentRow", cur ? cur->text(0) : QString());
}

void ToolsTreePanel::restoreState()
{
	QSettings s;
	bool expanded = s.value("toolsTree/expanded", true).toBool();
	if(tree_->topLevelItemCount() > 0 && tree_->topLevelItem(0)){
		tree_->topLevelItem(0)->setExpanded(expanded);
	}
}
