// SelectPropertyPanel.cpp — see header.

#include "SelectPropertyPanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include "SelectTool.h"
#include "panels/PropertyTree.h"   // PropertyTree (PropertyRow form)
#include "editor/EditorTool.h"     // IWorldBridge

SelectPropertyPanel::SelectPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	info_ = new QLabel(this);
	info_->setWordWrap(true);
	info_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	attrib_ = new PropertyTree(this);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(info_);
	layout->addWidget(attrib_, 1);

	// Edits land in the tree's rows; write them back to the selected object.
	connect(attrib_, &QTreeWidget::itemChanged, this, [this]{ applyEdits(); });
}

void SelectPropertyPanel::setTool(SelectTool* tool)
{
	tool_ = tool;
	refresh();
}

void SelectPropertyPanel::refresh()
{
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(!bridge){
		info_->setText(tr("No world loaded."));
		multi_ = false;
		loading_ = true;
		attrib_->setRoot(nullptr);
		loading_ = false;
		return;
	}

	int units = 0, environment = 0, sources = 0, cameras = 0, anchors = 0;
	bridge->selectedObjectCounts(units, environment, sources, cameras, anchors);
	const int total = units + environment + sources + cameras + anchors;

	if(total == 0){
		info_->setText(tr("Nothing selected"));
		multi_ = false;
		loading_ = true;
		attrib_->setRoot(nullptr);
		loading_ = false;
		return;
	}

	if(total > 1){
		// CSurToolSelect::updateLayout's multi-selection summary + the
		// Edit button's mix-in editor (the common fields of every object).
		multi_ = true;
		QString text = tr("Selected:");
		if(sources)     text += tr("\n\tSources: %1").arg(sources);
		if(environment) text += tr("\n\tEnvironment: %1").arg(environment);
		if(units)       text += tr("\n\tUnits: %1").arg(units);
		if(cameras)     text += tr("\n\tCameras: %1").arg(cameras);
		if(anchors)     text += tr("\n\tAnchors: %1").arg(anchors);
		text += tr("\nCommon properties (<different> = values differ):");
		info_->setText(text);
		loading_ = true;
		attrib_->setRoot(bridge->selectedObjectsCommonTree());
		loading_ = false;
		return;
	}

	// Exactly one object: show its serialized properties.
	multi_ = false;
	info_->setText(tr("Selected object"));
	loading_ = true;
	attrib_->setRoot(bridge->selectedObjectTree(true));
	loading_ = false;
}

void SelectPropertyPanel::applyEdits()
{
	if(loading_)
		return;
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(!bridge)
		return;
	if(editor::PropertyRow* root = attrib_->root()){
		if(multi_)
			bridge->selectedObjectsSetCommonTree(root);
		else
			bridge->selectedObjectSetTree(root);
	}
}
