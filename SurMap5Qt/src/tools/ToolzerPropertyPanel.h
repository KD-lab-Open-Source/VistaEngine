// ToolzerPropertyPanel.h — the Properties dock's panel for the Toolzer tool.
// Qt port of the CSurToolToolzer dialog: the Dig/Put radio and the delta-height
// slider (plus a smoothing slider). Qt-side, talks to the engine-free
// ToolzerTool.

#pragma once

#include <QRadioButton>
#include <QSlider>
#include <QWidget>

class ToolzerTool;

class ToolzerPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ToolzerPropertyPanel(QWidget* parent = nullptr);

	void setTool(ToolzerTool* tool);

private:
	// Write the signed delta into the tool from the current radio + slider.
	void applyDelta();

	QRadioButton* dig_ = nullptr;
	QRadioButton* put_ = nullptr;
	QSlider* deltaH_ = nullptr;
	QSlider* smooth_ = nullptr;

	ToolzerTool* tool_ = nullptr;
};
