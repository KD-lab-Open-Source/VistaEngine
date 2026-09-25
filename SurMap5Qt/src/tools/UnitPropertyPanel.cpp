// UnitPropertyPanel.cpp — see header.

#include "UnitPropertyPanel.h"

#include <QFormLayout>

#include <string>
#include <vector>

#include "UnitTool.h"
#include "editor/EditorTool.h"   // IWorldBridge

UnitPropertyPanel::UnitPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	attribute_ = new QComboBox(this);
	attribute_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	selectAfter_ = new QCheckBox(tr("Select after placing"), this);
	selectAfter_->setChecked(true);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Unit"), attribute_);
	layout->addRow(selectAfter_);

	connect(attribute_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
		if(tool_ && index >= 0)
			tool_->setAttributeIndex(index);
	});
	connect(selectAfter_, &QCheckBox::toggled, this, [this](bool on){
		if(tool_)
			tool_->setSelectAfterPlace(on);
	});
}

void UnitPropertyPanel::reloadAttributes()
{
	attribute_->clear();
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	if(bridge){
		std::vector<std::string> names;
		bridge->unitAttributeNames(names);
		for(const std::string& n : names)
			attribute_->addItem(QString::fromUtf8(n.c_str()));
	}
	if(attribute_->count() == 0)
		attribute_->addItem(tr("(load a world to list units)"));
	if(tool_ && tool_->attributeIndex() >= 0)
		attribute_->setCurrentIndex(tool_->attributeIndex());
}

void UnitPropertyPanel::setTool(UnitTool* tool)
{
	tool_ = tool;
	reloadAttributes();
	if(tool_)
		selectAfter_->setChecked(tool_->selectAfterPlace());
}
