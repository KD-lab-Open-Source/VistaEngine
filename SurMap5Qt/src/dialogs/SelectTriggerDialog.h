// SelectTriggerDialog.h — Qt port of CDlgSelectTrigger (SurMap5/DlgSelectTrigger).
//
// Trigger-file picker: a list of *.scr files under a directory with
// New / Delete / Copy buttons. Original: modal CDialog with CListCtrlEx.
// Accepts only when a file is selected (or a new/copied name was entered).
#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QPushButton;

class SelectTriggerDialog : public QDialog
{
	Q_OBJECT
public:
	// path2triggersFiles: the directory holding the *.scr trigger files.
	// title: dialog title (CDlgSelectTrigger ctor).
	SelectTriggerDialog(const QString& path2triggersFiles, const QString& title,
	                    QWidget* parent = nullptr);

	// The chosen trigger file name (CDlgSelectTrigger::selectTriggersFile) —
	// a name from the list, a new name from the New button, or a copied name.
	QString selectedTrigger() const { return selectedTrigger_; }

	// CDlgSelectTrigger::OnOK: accept only if a row is selected.
	void accept() override;

private slots:
	void fillTriggerList();
	void onNewTrigger();
	void onDeleteTrigger();
	void onCopyTrigger();

private:
	QString askForName();

	QListWidget* list_ = nullptr;
	QPushButton* btnNew_ = nullptr;
	QPushButton* btnDelete_ = nullptr;
	QPushButton* btnCopy_ = nullptr;
	QString path2triggersFiles_;
	QString title_;
	QString selectedTrigger_;
};
