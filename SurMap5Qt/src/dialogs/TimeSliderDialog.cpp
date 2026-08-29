// TimeSliderDialog.cpp — see header.

#include "TimeSliderDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QVBoxLayout>

namespace {
const int kSliderMin = 0;
const int kSliderMax = 240;
}

TimeSliderDialog::TimeSliderDialog(float time, QWidget* parent)
	: QDialog(parent)
	, time_(time)
{
	setWindowTitle(tr("Time of Day"));
	setMinimumWidth(320);

	slider_ = new QSlider(Qt::Horizontal, this);
	slider_->setRange(kSliderMin, kSliderMax);
	slider_->setValue((int)((time_ / 24.f) * (kSliderMax - kSliderMin) + kSliderMin));

	edit_ = new QLineEdit(this);
	edit_->setFixedWidth(72);
	edit_->setText(QString::number(time_, 'f', 2));

	auto* row = new QGridLayout;
	row->addWidget(new QLabel(tr("Time:"), this), 0, 0);
	row->addWidget(slider_, 0, 1);
	row->addWidget(edit_, 0, 2);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QVBoxLayout(this);
	layout->addLayout(row);
	layout->addWidget(buttons);

	connect(slider_, &QSlider::valueChanged, this, &TimeSliderDialog::onSliderChanged);
	connect(edit_, &QLineEdit::textChanged, this, &TimeSliderDialog::onEditChanged);
}

float TimeSliderDialog::time() const
{
	return time_;
}

float TimeSliderDialog::posToTime(int pos)
{
	float t = (float)pos / (float)(kSliderMax - kSliderMin) * 24.f;
	while(t > 24.f)
		t -= 24.f;
	return t;
}

void TimeSliderDialog::setTimeFlowEnabled(bool on)
{
	if(timeFlowEnabled_ == on)
		return;
	timeFlowEnabled_ = on;
	slider_->setEnabled(!on);
	edit_->setEnabled(!on);
}

void TimeSliderDialog::quant(float worldTime)
{
	// The original's quant(): when the clock flows, keep the slider in sync
	// with the world's time; otherwise nothing changes here.
	if(!timeFlowEnabled_)
		return;

	time_ = worldTime;
	updating_ = true;
	slider_->setValue((int)((time_ / 24.f) * (kSliderMax - kSliderMin) + kSliderMin));
	edit_->setText(QString::number(time_, 'f', 2));
	updating_ = false;
}

void TimeSliderDialog::onSliderChanged(int pos)
{
	if(updating_)
		return;
	time_ = posToTime(pos);
	updating_ = true;
	edit_->setText(QString::number(time_, 'f', 2));
	updating_ = false;
	if(!timeFlowEnabled_)
		emit timeChanged(time_);
}

void TimeSliderDialog::onEditChanged()
{
	if(updating_)
		return;
	bool ok = false;
	const float t = edit_->text().toFloat(&ok);
	if(ok){
		time_ = t < 0.f ? 0.f : (t > 24.f ? 24.f : t);
		updating_ = true;
		slider_->setValue((int)((time_ / 24.f) * (kSliderMax - kSliderMin) + kSliderMin));
		updating_ = false;
		if(!timeFlowEnabled_)
			emit timeChanged(time_);
	}
}
