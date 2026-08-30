// BorderRollingDialog.cpp — see header.

#include "BorderRollingDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

BorderRollingDialog::BorderRollingDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Rolling Border"));

	// DDX_Text + DDV_MinMaxUInt: borderHeight 0..511, boderAngle 1..90.
	heightSpin_ = new QSpinBox(this);
	heightSpin_->setRange(0, 511);
	heightSpin_->setValue(30);          // DlgBorderRolling default

	angleSpin_ = new QSpinBox(this);
	angleSpin_->setRange(1, 90);
	angleSpin_->setValue(80);           // DlgBorderRolling default
	angleSpin_->setSuffix(tr(" deg"));

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto* layout = new QFormLayout(this);
	layout->addRow(tr("Border height (voxels):"), heightSpin_);
	layout->addRow(tr("Border angle:"), angleSpin_);
	layout->addRow(buttons);
}

int BorderRollingDialog::borderHeight() const
{
	return heightSpin_->value();
}

void BorderRollingDialog::setBorderHeight(int value)
{
	heightSpin_->setValue(value);
}

int BorderRollingDialog::borderAngle() const
{
	return angleSpin_->value();
}

void BorderRollingDialog::setBorderAngle(int value)
{
	angleSpin_->setValue(value);
}
