// AnchorPropertyPanel.cpp — see header.

#include "AnchorPropertyPanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include "AnchorTool.h"
#include "panels/PropertyTree.h"   // PropertyTree (PropertyRow form)
#include "editor/EditorTool.h"     // IWorldBridge

AnchorPropertyPanel::AnchorPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	attrib_ = new PropertyTree(this);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Anchor"), this));
	layout->addWidget(attrib_, 1);

	connect(attrib_, &QTreeWidget::itemChanged, this, [this]{ applyEdits(); });
}

void AnchorPropertyPanel::load()
{
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	loading_ = true;
	attrib_->setRoot(bridge ? bridge->anchorTree(true) : nullptr);
	loading_ = false;
}

void AnchorPropertyPanel::applyEdits()
{
	if(loading_)   // setRoot's rebuild fires itemChanged; ignore it
		return;
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(!bridge)
		return;
	if(editor::PropertyRow* root = attrib_->root())
		bridge->anchorSetTree(root);
	tool_->applyAnchorEdit();   // rebuild the cursor preview from the new params
}

void AnchorPropertyPanel::setTool(AnchorTool* tool)
{
	tool_ = tool;
	load();
}
