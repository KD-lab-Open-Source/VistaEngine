// EnterNameDialog.h — Qt port of CEnterNameDlg (SurMap5/EnterNameDlg).
//
// A single-line text editor; the result is the typed line. The original held a
// 50-char buffer and read the edit control's first line on OK.
#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

class EnterNameDialog : public QDialog
{
	Q_OBJECT
public:
	// text: initial value of the field (CEnterNameDlg(const char*)).
	explicit EnterNameDialog(const QString& text = QString(), QWidget* parent = nullptr);

	// The text the user typed.
	QString text() const;

private:
	QLineEdit* edit_ = nullptr;
};
