// UnitPropertyPanel.h — the Properties dock's panel for the unit-placement
// tool. CSurToolUnit's dialog had no unit combo: the unit was picked in the
// tools tree (CSurToolUnitFolder), and the panel only carried the Angle / ±(%)
// sliders (calculateUnitPose). Mirrors that. Qt-side, talks to the engine-free
// UnitTool.

#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

class UnitTool;

class UnitPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit UnitPropertyPanel(QWidget* parent = nullptr);

	void setTool(UnitTool* tool);

private:
	void updateUnitLabel();

	QLabel* unitLabel_ = nullptr;
	QSpinBox* angle_ = nullptr;
	QSpinBox* angleDelta_ = nullptr;
	QCheckBox* selectAfter_ = nullptr;

	UnitTool* tool_ = nullptr;
};
