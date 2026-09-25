// UnitPropertyPanel.h — the Properties dock's panel for the unit-placement tool.
// Lists AttributeLibrary's unit attributes (SurToolPlayerFolder built the same
// list per player) and lets the user pick one. Qt-side, talks to the engine-free
// UnitTool.

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

class UnitTool;

class UnitPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit UnitPropertyPanel(QWidget* parent = nullptr);

	void setTool(UnitTool* tool);

private:
	void reloadAttributes();

	QComboBox* attribute_ = nullptr;
	QCheckBox* selectAfter_ = nullptr;

	UnitTool* tool_ = nullptr;
};
