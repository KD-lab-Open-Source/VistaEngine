// GeoNetPropertyPanel.h — the Properties dock's panel for the GeoNet tool.
// Qt port of the CSurToolGeoNet dialog's sliders (height, noise, mesh
// density). Qt-side, talks to the engine-free GeoNetTool.

#pragma once

#include <QSlider>
#include <QWidget>

class GeoNetTool;

class GeoNetPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit GeoNetPropertyPanel(QWidget* parent = nullptr);

	void setTool(GeoNetTool* tool);

private:
	QSlider* height_ = nullptr;
	QSlider* noise_ = nullptr;
	QSlider* mesh_ = nullptr;

	GeoNetTool* tool_ = nullptr;
};