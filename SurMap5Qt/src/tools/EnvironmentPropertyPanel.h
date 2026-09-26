// EnvironmentPropertyPanel.h — the Properties dock's panel for the environment
// placement tool (SurTool3DM / CSurToolEnvironment dialog). Lists the mesh
// cache's models and the EnvironmentType values, and edits the angle/scale/
// spread parameters. Qt-side, talks to the engine-free EnvironmentTool.

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QWidget>

class EnvironmentTool;

class EnvironmentPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit EnvironmentPropertyPanel(QWidget* parent = nullptr);

	void setTool(EnvironmentTool* tool);

private:
	void reloadModels();
	void reloadTypes();
	void readParams();
	void writeParams();
	void applyModelHint();

	QComboBox* model_ = nullptr;
	QComboBox* type_ = nullptr;
	QSpinBox* angle_ = nullptr;
	QSpinBox* angleDelta_ = nullptr;
	QSpinBox* scale_ = nullptr;
	QSpinBox* scaleDelta_ = nullptr;
	QSpinBox* spreadRadius_ = nullptr;
	QSpinBox* spreadRadiusDelta_ = nullptr;
	QCheckBox* spread_ = nullptr;
	QCheckBox* vertical_ = nullptr;

	EnvironmentTool* tool_ = nullptr;
	bool loading_ = false;   // reload in progress — ignore widget signals
};
