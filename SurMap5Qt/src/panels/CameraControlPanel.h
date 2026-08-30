#pragma once

#include <QWidget>

class QDoubleSpinBox;
class RenderViewWidget;

class CameraControlPanel : public QWidget
{
	Q_OBJECT
public:
	explicit CameraControlPanel(RenderViewWidget* view, QWidget* parent = nullptr);

private slots:
	void applyCamera();
	void applyEyeCamera();
	void fitWorld();
	void topView();
	void overview();
	void readCamera();
	void resetCamera();

private:
	QDoubleSpinBox* addField(class QFormLayout* form, const QString& label,
	                         double minimum, double maximum, double value,
	                         int decimals = 2);

	RenderViewWidget* view_ = nullptr;
	QDoubleSpinBox* centerX_ = nullptr;
	QDoubleSpinBox* centerY_ = nullptr;
	QDoubleSpinBox* centerZ_ = nullptr;
	QDoubleSpinBox* distance_ = nullptr;
	QDoubleSpinBox* yaw_ = nullptr;
	QDoubleSpinBox* pitch_ = nullptr;
	QDoubleSpinBox* roll_ = nullptr;
	QDoubleSpinBox* eyeX_ = nullptr;
	QDoubleSpinBox* eyeY_ = nullptr;
	QDoubleSpinBox* eyeZ_ = nullptr;
};
