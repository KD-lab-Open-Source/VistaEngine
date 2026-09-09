// TransformPropertyPanel.cpp — see header.

#include "TransformPropertyPanel.h"

#include <QRadioButton>
#include <QVBoxLayout>

#include "TransformTool.h"

TransformPropertyPanel::TransformPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	axisX_ = new QRadioButton(tr("X axis"), this);
	axisY_ = new QRadioButton(tr("Y axis"), this);
	axisZ_ = new QRadioButton(tr("Z axis"), this);

	// The original's radio trio: exactly one axis active at a time. Qt 6.4
	// auto-groups radio buttons sharing a parent, so no explicit group is
	// needed.

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->addWidget(axisX_);
	layout->addWidget(axisY_);
	layout->addWidget(axisZ_);

	// User picks an axis -> the tool's transform axis changes.
	connect(axisX_, &QRadioButton::toggled, this, [this](bool on){
		if(on && tool_) tool_->setAxis(0);
	});
	connect(axisY_, &QRadioButton::toggled, this, [this](bool on){
		if(on && tool_) tool_->setAxis(1);
	});
	connect(axisZ_, &QRadioButton::toggled, this, [this](bool on){
		if(on && tool_) tool_->setAxis(2);
	});
}

void TransformPropertyPanel::setTool(TransformTool* tool)
{
	tool_ = tool;
	if(!tool_){
		axisX_->setChecked(false);
		axisY_->setChecked(false);
		axisZ_->setChecked(false);
		return;
	}
	// Reflect the tool's current axis (the tool may have been switched by a
	// hotkey X/Y/Z, so re-sync the radios).
	switch(tool_->axis()){
	case 0: axisX_->setChecked(true); break;
	case 1: axisY_->setChecked(true); break;
	default: axisZ_->setChecked(true); break;
	}
}