#include "CameraControlPanel.h"

#include <cmath>

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "RenderViewWidget.h"

namespace {
constexpr double kRadiansToDegrees = 180.0 / 3.14159265358979323846;
constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;
}

CameraControlPanel::CameraControlPanel(RenderViewWidget* view, QWidget* parent)
	: QWidget(parent)
	, view_(view)
{
	auto* form = new QFormLayout;
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	centerX_ = addField(form, tr("Center X"), -100000.0, 100000.0, 0.0, 1);
	centerY_ = addField(form, tr("Center Y"), -100000.0, 100000.0, 0.0, 1);
	centerZ_ = addField(form, tr("Center Z"), -100000.0, 100000.0, 0.0, 1);
	distance_ = addField(form, tr("Distance"), 2.0, 100000.0, 5000.0, 1);
	yaw_ = addField(form, tr("Yaw (deg)"), -3600.0, 3600.0, 0.0, 2);
	pitch_ = addField(form, tr("Pitch (deg)"), -3600.0, 3600.0, 37.0, 2);
	roll_ = addField(form, tr("Roll (deg)"), -3600.0, 3600.0, 0.0, 2);
	eyeX_ = addField(form, tr("Eye X"), -100000.0, 100000.0, 0.0, 1);
	eyeY_ = addField(form, tr("Eye Y"), -100000.0, 100000.0, 0.0, 1);
	eyeZ_ = addField(form, tr("Eye Z"), -100000.0, 100000.0, 5000.0, 1);

	auto* apply = new QPushButton(tr("Apply"), this);
	auto* applyEye = new QPushButton(tr("Apply Eye"), this);
	auto* read = new QPushButton(tr("Read"), this);
	auto* reset = new QPushButton(tr("Reset"), this);
	auto* buttons = new QHBoxLayout;
	buttons->addWidget(apply);
	buttons->addWidget(applyEye);
	buttons->addWidget(read);
	buttons->addWidget(reset);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->addLayout(form);
	layout->addLayout(buttons);
	layout->addStretch();

	connect(apply, &QPushButton::clicked, this, &CameraControlPanel::applyCamera);
	connect(applyEye, &QPushButton::clicked, this, &CameraControlPanel::applyEyeCamera);
	connect(read, &QPushButton::clicked, this, &CameraControlPanel::readCamera);
	connect(reset, &QPushButton::clicked, this, &CameraControlPanel::resetCamera);
	readCamera();
}

QDoubleSpinBox* CameraControlPanel::addField(QFormLayout* form, const QString& label,
                                             double minimum, double maximum,
                                             double value, int decimals)
{
	auto* field = new QDoubleSpinBox(this);
	field->setRange(minimum, maximum);
	field->setDecimals(decimals);
	field->setSingleStep(decimals > 1 ? 1.0 : 10.0);
	field->setValue(value);
	field->setKeyboardTracking(false);
	form->addRow(label, field);
	return field;
}

void CameraControlPanel::applyCamera()
{
	if(!view_)
		return;
	RenderViewWidget::CameraState state;
	state.centerX = (float)centerX_->value();
	state.centerY = (float)centerY_->value();
	state.centerZ = (float)centerZ_->value();
	state.distance = (float)distance_->value();
	state.yaw = (float)(yaw_->value() * kDegreesToRadians);
	state.pitch = (float)(pitch_->value() * kDegreesToRadians);
	state.roll = (float)(roll_->value() * kDegreesToRadians);
	view_->setCameraState(state);
}

void CameraControlPanel::applyEyeCamera()
{
	if(!view_)
		return;
	RenderViewWidget::CameraState state;
	view_->cameraState(state);
	const double dx = eyeX_->value() - state.centerX;
	const double dy = eyeY_->value() - state.centerY;
	const double dz = eyeZ_->value() - state.centerZ;
	const double horizontal = std::hypot(dx, dy);
	state.distance = (float)std::hypot(horizontal, dz);
	if(state.distance < 2.0f)
		state.distance = 2.0f;
	state.yaw = (float)std::atan2(dy, dx);
	state.pitch = (float)std::atan2(horizontal, dz);
	view_->setCameraState(state);
	readCamera();
}

void CameraControlPanel::readCamera()
{
	if(!view_)
		return;
	RenderViewWidget::CameraState state;
	view_->cameraState(state);
	centerX_->setValue(state.centerX);
	centerY_->setValue(state.centerY);
	centerZ_->setValue(state.centerZ);
	distance_->setValue(state.distance);
	yaw_->setValue(state.yaw * kRadiansToDegrees);
	pitch_->setValue(state.pitch * kRadiansToDegrees);
	roll_->setValue(state.roll * kRadiansToDegrees);
	const double horizontal = state.distance * std::sin(state.pitch);
	eyeX_->setValue(state.centerX + horizontal * std::cos(state.yaw));
	eyeY_->setValue(state.centerY + horizontal * std::sin(state.yaw));
	eyeZ_->setValue(state.centerZ + state.distance * std::cos(state.pitch));
}

void CameraControlPanel::resetCamera()
{
	if(!view_)
		return;
	RenderViewWidget::CameraState state;
	view_->cameraState(state);
	state.distance = 8000.0f;
	state.yaw = 0.0f;
	state.pitch = 0.65f;
	state.roll = 0.0f;
	view_->setCameraState(state);
	readCamera();
}
