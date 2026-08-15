// SelectWorldDialog.h — Qt port of CDlgSelectWorld (SurMap5/DlgSelectWorld).
//
// A modal world picker: a three-column list (name / interface / size) plus
// New / Delete / Rename, mirroring the original. The engine half (scanning the
// worlds directory) lives in WorldList (EditorEngine); this dialog is pure Qt.
//
// Accepted (double-click or OK) -> selectedWorld() holds the chosen name.
#pragma once

#include <QDialog>
#include <QString>

#include <vector>

class QModelIndex;
class QTableView;
class WorldList;

class SelectWorldDialog : public QDialog
{
	Q_OBJECT
public:
	// path2worlds: the directory containing the worlds. title: dialog title.
	// enableCreateDir: show the New-world button (CDlgSelectWorld ctor).
	SelectWorldDialog(const QString& path2worlds,
	                  const QString& title,
	                  bool enableCreateDir,
	                  QWidget* parent = nullptr);
	~SelectWorldDialog() override;

	// The chosen world name (CDlgSelectWorld::selectWorldName), valid after
	// the dialog is accepted.
	QString selectedWorld() const { return selectedWorld_; }

	// Static: which world was selected last time (previsionWorldSelect).
	static int previsionWorldSelect() { return s_previsionWorldSelect; }

	// CDlgSelectWorld::OnOK: accept only if a row is selected.
	void accept() override;

private slots:
	void fillWorldList();
	void onNewWorld();
	void onDeleteWorld();
	void onRenameWorld();
	void onDoubleClicked(const QModelIndex& index);

private:
	void selectPrevious();

	QTableView* table_ = nullptr;
	WorldList* worldList_ = nullptr;
	QString path2worlds_;
	QString title_;
	bool enableCreateDir_ = false;
	QString selectedWorld_;

	static int s_previsionWorldSelect;
};
