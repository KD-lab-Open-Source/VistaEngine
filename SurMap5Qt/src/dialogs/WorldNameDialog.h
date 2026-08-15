// WorldNameDialog.h — Qt port of CDlgWorldName (SurMap5/DlgWorldName).
//
// A single-line name editor for a new world, defaulting to "Default".
// Original: a modal CDialog with DDX_Text bound to CString newWorldName.
#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

class WorldNameDialog : public QDialog
{
	Q_OBJECT
public:
	explicit WorldNameDialog(QWidget* parent = nullptr);

	// The name the user typed (CDlgWorldName::newWorldName).
	QString worldName() const;
	void setWorldName(const QString& name);

private:
	QLineEdit* nameEdit_ = nullptr;
};
