// TimeSliderDialog.h — Qt port of CTimeSliderDlg (SurMap5/TimeSliderDlg).
//
// A modeless-ish dialog with a time slider (0..240 = 0..24 hours) and a text
// field. The original drove Environment::environmentTime(); the Qt editor does
// not create the global Environment yet, so the dialog keeps the time locally
// and reports changes via timeChanged (the engine binding lands when
// environment exists). Time-flow mode disables the controls (the original
// enabled/disabled them on Environment::flag_EnableTimeFlow).
#pragma once

#include <QDialog>

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class QLineEdit;
class QSlider;

class TimeSliderDialog : public QDialog
{
	Q_OBJECT
public:
	// time: the initial time of day in hours (0..24).
	explicit TimeSliderDialog(float time, QWidget* parent = nullptr);

	// The engine-free bridge to the environment's clock
	// (Environment::environmentTime()). When set, the dialog reads/writes the
	// world's time instead of its local copy.
	void setBridge(IWorldBridge* bridge) { bridge_ = bridge; }

	// The current time in hours (0..24).
	float time() const;

	// The time-flow mode (Environment::flag_EnableTimeFlow). While on, the
	// slider and edit are disabled and timeChanged stops firing on drags.
	bool timeFlowEnabled() const { return timeFlowEnabled_; }
	void setTimeFlowEnabled(bool on);

	// Called every editor tick (~60 Hz) to keep the slider in sync with the
	// world's advancing clock (the original's quant()).
	void quant(float worldTime);

signals:
	void timeChanged(float hours);

private slots:
	void onSliderChanged(int pos);
	void onEditChanged();

private:
	// pos 0..240 -> hours 0..24 (posToTime).
	static float posToTime(int pos);

	IWorldBridge* bridge_ = nullptr;
	QSlider* slider_ = nullptr;
	QLineEdit* edit_ = nullptr;
	float time_ = 0.f;
	bool timeFlowEnabled_ = false;
	bool updating_ = false;
};
