// CameraDialog.h — Qt port of CCameraDlg (SurMap5/CameraDlg).
//
// The camera-spline editor: a list of the world's camera paths plus create/
// delete/play and point editing. The original drove the global cameraManager
// (Game/CameraManager), which the Qt editor does not create yet — the dialog
// keeps the UI structure and shows an informational message on actions until
// cameraManager lands.
#pragma once

#include <QDialog>

#include <QStringList>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class CameraDialog : public QDialog
{
	Q_OBJECT
public:
	explicit CameraDialog(QWidget* parent = nullptr);

	// The camera paths to list (CameraManager::splines names). Empty by
	// default — populated when cameraManager is wired.
	void setCameras(const QStringList& names);

private slots:
	void onCreateCamera();
	void onDeleteCamera();
	void onPlayCamera();
	void onListSelectionChanged();

private:
	QListWidget* list_ = nullptr;
	QPushButton* btnCreate_ = nullptr;
	QPushButton* btnDelete_ = nullptr;
	QPushButton* btnPlay_ = nullptr;
	QLineEdit* nameEdit_ = nullptr;
	QLineEdit* timeEdit_ = nullptr;
	QLabel* statusLabel_ = nullptr;
};
