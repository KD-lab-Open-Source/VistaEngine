// UnitPropertyPanel.cpp — see header.

#include "UnitPropertyPanel.h"

#include <QFormLayout>

#include <string>
#include <vector>

#include "UnitTool.h"
#include "panels/PropertyText.h"  // propertytext::displayBytes (cp1251 names)
#include "editor/EditorTool.h"    // IWorldBridge

UnitPropertyPanel::UnitPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	// The unit is chosen in the tools tree (like the original); the panel only
	// shows which one is active plus the angle sliders.
	unitLabel_ = new QLabel(this);
	unitLabel_->setWordWrap(true);

	angle_ = new QSpinBox(this);
	angle_->setRange(0, 360);
	angle_->setSingleStep(15);
	angleDelta_ = new QSpinBox(this);
	angleDelta_->setRange(0, 180);
	angleDelta_->setSingleStep(15);

	selectAfter_ = new QCheckBox(tr("Select after placing"), this);
	selectAfter_->setChecked(true);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Unit"), unitLabel_);
	layout->addRow(tr("Angle"), angle_);
	layout->addRow(tr("Angle spread %"), angleDelta_);
	layout->addRow(selectAfter_);

	connect(angle_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){
		if(tool_)
			tool_->setAngle((float)v);
	});
	connect(angleDelta_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v){
		if(tool_)
			tool_->setAngleDelta((float)v);
	});
	connect(selectAfter_, &QCheckBox::toggled, this, [this](bool on){
		if(tool_)
			tool_->setSelectAfterPlace(on);
	});
}

void UnitPropertyPanel::updateUnitLabel()
{
	QString name;
	IWorldBridge* bridge = tool_ ? tool_->bridge() : nullptr;
	const int index = tool_ ? tool_->attributeIndex() : -1;
	if(bridge && index >= 0){
		std::vector<std::string> names;
		bridge->unitAttributeNames(names);
		if(index < (int)names.size())
			name = propertytext::displayBytes(names[index]);
	}
	unitLabel_->setText(name.isEmpty() ? tr("(pick a unit in the Tools tree)") : name);
}

void UnitPropertyPanel::setTool(UnitTool* tool)
{
	tool_ = tool;
	updateUnitLabel();
	if(tool_){
		selectAfter_->setChecked(tool_->selectAfterPlace());
		angle_->setValue((int)tool_->angle());
		angleDelta_->setValue((int)tool_->angleDelta());
	}
}
