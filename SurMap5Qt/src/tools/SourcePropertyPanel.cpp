// SourcePropertyPanel.cpp — see header.

#include "SourcePropertyPanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include <string>
#include <vector>

#include "SourceTool.h"
#include "panels/PropertyTree.h"   // PropertyTree (PropertyRow form)
#include "panels/PropertyText.h"   // propertytext::displayBytes (cp1251 names)
#include "editor/EditorTool.h"     // IWorldBridge

SourcePropertyPanel::SourcePropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	source_ = new QComboBox(this);
	source_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	attrib_ = new PropertyTree(this);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Source type"), this));
	layout->addWidget(source_);
	layout->addWidget(attrib_, 1);

	connect(source_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
		if(tool_)
			tool_->setSourceIndex(index);
		loadElement(index);
	});
	// Edits land in the tree's rows; write them back to the library element and
	// refresh the preview when the tree reports a change. The base-class signal
	// is what PropertyTree emits (it adds no signal of its own).
	connect(attrib_, &QTreeWidget::itemChanged, this, [this]{ applyEdits(); });
}

void SourcePropertyPanel::reloadSources()
{
	source_->blockSignals(true);
	source_->clear();
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(bridge){
		std::vector<std::string> names;
		bridge->sourceNames(names);
		for(const std::string& n : names)
			source_->addItem(propertytext::displayBytes(n));
	}
	if(source_->count() == 0)
		source_->addItem(tr("(load a world to list sources)"));
	source_->blockSignals(false);

	if(tool_ && source_->count() > 0){
		const int index = tool_->sourceIndex() >= 0 ? tool_->sourceIndex() : 0;
		source_->setCurrentIndex(index);
		tool_->setSourceIndex(index);
		loadElement(index);
	}
}

void SourcePropertyPanel::loadElement(int index)
{
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(!bridge || index < 0){
		attrib_->setRoot(nullptr);
		return;
	}
	loading_ = true;
	attrib_->setRoot(bridge->sourceElementTree(index, true));
	loading_ = false;
}

void SourcePropertyPanel::applyEdits()
{
	if(loading_)   // setRoot's rebuild fires itemChanged; ignore it
		return;
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(!bridge || !tool_ || tool_->sourceIndex() < 0)
		return;
	if(editor::PropertyRow* root = attrib_->root())
		bridge->sourceElementSetTree(tool_->sourceIndex(), root);
	tool_->applySourceEdit();   // rebuild the cursor preview from the new params
}

void SourcePropertyPanel::setTool(SourceTool* tool)
{
	tool_ = tool;
	reloadSources();
}
