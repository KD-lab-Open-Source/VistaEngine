// OutputProgressDialog.h — Qt port of COutputProgressDlg (SurMap5/OutputProgressDlg).
//
// Modeless progress dialog for export/pack operations: a total progress bar,
// a per-file progress bar and the current file-name label. The original was
// a CDialog created with Create() and driven by progress() + pumpMessages();
// Qt's equivalent is a QDialog shown with show() (non-modal) or exec().
#pragma once

#include <QDialog>

class QLabel;
class QProgressBar;

class OutputProgressDialog : public QDialog
{
	Q_OBJECT
public:
	explicit OutputProgressDialog(const QString& title, QWidget* parent = nullptr);

	// COutputProgressDlg::progress(totalPercent, filePercent, fileName) —
	// clamps to 0..99 and updates the two bars + the file label.
	void progress(int totalPercent, int filePercent, const QString& fileName);

private:
	QProgressBar* totalBar_ = nullptr;
	QProgressBar* fileBar_ = nullptr;
	QLabel* fileNameLabel_ = nullptr;
};
