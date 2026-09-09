// TransformPropertyPanel.h — the Properties dock's panel for the transform
// tools (Select/Move/Rotate/Scale). Qt port of the CSurToolTransform dialog's
// axis radio trio (OnXAxisCheck/OnYAxisCheck/OnZAxisCheck): the checked axis
// is the transform axis. The panel is Qt-side (it lives in the SurMap5Qt exe,
// not the engine lib), so it talks to the tool through the engine-free
// TransformTool interface.
//
// The original CSurToolTransform was an MFC dialog that doubled as the tool's
// panel; here the tool is plain behaviour and this panel is the Properties
// dock's widget for it. MainWindow swaps it in when a transform tool is active.

#pragma once

#include <QRadioButton>
#include <QWidget>

class TransformTool;

class TransformPropertyPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TransformPropertyPanel(QWidget* parent = nullptr);

	// Bind to the active transform tool (null detaches). The panel reads the
	// tool's current axis and writes back on user change.
	void setTool(TransformTool* tool);

private:
	// The axis radio buttons (0=X, 1=Y, 2=Z).
	QRadioButton* axisX_ = nullptr;
	QRadioButton* axisY_ = nullptr;
	QRadioButton* axisZ_ = nullptr;

	TransformTool* tool_ = nullptr;
};