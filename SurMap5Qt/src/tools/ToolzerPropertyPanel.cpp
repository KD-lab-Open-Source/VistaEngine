// ToolzerPropertyPanel.cpp — see header.

#include "ToolzerPropertyPanel.h"

#include <QFormLayout>
#include <QHBoxLayout>

#include "ToolzerTool.h"

ToolzerPropertyPanel::ToolzerPropertyPanel(QWidget* parent)
	: QWidget(parent)
{
	// CSurToolToolzer's controls: the Dig/Put radio (IDC_RADIO_DIGPUT1/2),
	// the delta-height slider (IDC_SLIDER_EDITDELTAH, 0..MAX_TOOLZER_DELTAH)
	// and the roughness slider (IDC_SLDR_ROUGHNESS, 0..100). The MFC sliders
	// are CEScrollVx/CScrollBar; QSlider is the Qt equivalent.
	dig_ = new QRadioButton(tr("Dig"), this);
	dig_->setChecked(true);   // state_radio_button_DigPut starts at 0
	put_ = new QRadioButton(tr("Put"), this);

	deltaH_ = new QSlider(Qt::Horizontal, this);
	deltaH_->setRange(0, 0x3fff);   // MAX_TOOLZER_DELTA_H
	deltaH_->setValue(10);

	smooth_ = new QSlider(Qt::Horizontal, this);
	smooth_->setRange(0, 100);
	smooth_->setValue(90);

	auto* radioRow = new QHBoxLayout;
	radioRow->addWidget(dig_);
	radioRow->addWidget(put_);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Mode"), radioRow);
	layout->addRow(tr("Delta H"), deltaH_);
	layout->addRow(tr("Smooth"), smooth_);

	connect(dig_, &QRadioButton::toggled, this, [this](bool on){ if(on) applyDelta(); });
	connect(put_, &QRadioButton::toggled, this, [this](bool on){ if(on) applyDelta(); });
	connect(deltaH_, &QSlider::valueChanged, this, [this](int){ applyDelta(); });
	connect(smooth_, &QSlider::valueChanged, this, [this](int v){
		if(tool_) tool_->setSmooth(v);
	});
}

void ToolzerPropertyPanel::applyDelta()
{
	if(!tool_)
		return;
	// state_radio_button_DigPut: 0 = dig (negative), 1 = put (positive).
	const int dh = deltaH_->value();
	tool_->setDeltaH(put_->isChecked() ? dh : -dh);
}

void ToolzerPropertyPanel::setTool(ToolzerTool* tool)
{
	tool_ = tool;
	if(!tool_)
		return;
	const int dh = tool_->deltaH();
	// A negative stored value means dig; the panel shows the magnitude.
	(dh < 0 ? dig_ : put_)->setChecked(true);
	deltaH_->setValue(dh < 0 ? -dh : dh);
	smooth_->setValue(tool_->smooth());
}
