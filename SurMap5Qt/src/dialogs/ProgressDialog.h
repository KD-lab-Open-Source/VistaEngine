// ProgressDialog.h — Qt port of CProgressDlg (SurMap5/ProgDlg).
//
// A modal progress dialog: a status label, a percentage label, a progress bar
// and a Stop button. The original pumped Win32 messages while the caller
// stepped the bar (SetPos/OffsetPos/StepIt); in Qt the dialog is non-modal so
// the event loop stays alive and the caller updates it from the same thread.
#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QProgressBar;
class QPushButton;

class ProgressDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ProgressDialog(const QString& caption = QString(), QWidget* parent = nullptr);

	void setRange(int lower, int upper);       // CProgressDlg::SetRange
	int  setPos(int pos);                      // CProgressDlg::SetPos
	int  offsetPos(int offset);                // CProgressDlg::OffsetPos
	int  step();                               // CProgressDlg::StepIt
	void setStep(int step) { step_ = step; }
	void setStatus(const QString& message);    // CProgressDlg::SetStatus

	bool stopPressed() const { return stopPressed_; }

private:
	void updatePercent(int pos);

	QProgressBar* bar_ = nullptr;
	QLabel* statusLabel_ = nullptr;
	QLabel* percentLabel_ = nullptr;
	QPushButton* stopButton_ = nullptr;

	int lower_ = 0;
	int upper_ = 100;
	int step_ = 1;
	int pos_ = 0;
	bool stopPressed_ = false;
};
