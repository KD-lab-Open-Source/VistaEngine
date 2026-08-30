// CreateAttribDialog.h — Qt port of CCreateAttribDlg (SurMap5/CreateAttribDlg).
//
// Create-a-unit dialog: a name field plus an optional "paste from clipboard"
// checkbox. Original: modal CDialog with nameEdit_ (CEdit) + pasteCheck_
// (CButton), enabled only when allowPaste is true.
#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QLineEdit;

class CreateAttribDialog : public QDialog
{
	Q_OBJECT
public:
	// allowPaste: whether the "paste" checkbox is enabled. pasteByDefault:
	// its initial state. title: window title + default name when defaultName
	// is null (CCreateAttribDlg::defaultName_ = title).
	CreateAttribDialog(bool allowPaste, bool pasteByDefault, const QString& title,
	                   const QString& defaultName = QString(), QWidget* parent = nullptr);

	// The name the user typed (CCreateAttribDlg::name()).
	QString name() const;
	// Whether the paste checkbox is checked (CCreateAttribDlg::paste()).
	bool paste() const;

private:
	QLineEdit* nameEdit_ = nullptr;
	QCheckBox* pasteCheck_ = nullptr;
};
