// ReelsDialog.h — Qt port of the Cut-Scenes (reels) editor
// (SurMap5/MainFrame.cpp OnEditReels -> CFileLibraryEditorDlg("Resource\\Video",
// "*.bik")).
//
// The reels library is a plain list of .bik video files in Resource\Video.
// The original used a file-browser dialog; here a simple list dialog scans
// the directory. No engine data involved.

#pragma once

#include <QDialog>

class QListWidget;
class QPushButton;

class ReelsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ReelsDialog(QWidget* parent = nullptr);

private slots:
	void onRefresh();

private:
	QListWidget* list_ = nullptr;
	QPushButton* btnRefresh_ = nullptr;
};