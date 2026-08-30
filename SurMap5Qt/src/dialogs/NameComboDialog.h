// NameComboDialog.h — Qt port of CNameComboDlg (SurMap5/NameComboDlg).
//
// An editable combo with an initial value; the result is the typed text.
// Original: a modal CDialog with a CComboBox pre-filled from a semicolon-
// separated combo list (ComboStrings / splitComboList), the initial name
// shown in the edit field and selected. Used for renaming sources etc.
#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;

class NameComboDialog : public QDialog
{
	Q_OBJECT
public:
	// title: window title. name: initial text. combo: semicolon-separated
	// list of suggestions to pre-fill the combo (CNameComboDlg ctor).
	NameComboDialog(const QString& title, const QString& name,
	                const QString& combo = QString(), QWidget* parent = nullptr);

	// The text the user typed (CNameComboDlg::name()).
	QString name() const;

private:
	QComboBox* combo_ = nullptr;
};
