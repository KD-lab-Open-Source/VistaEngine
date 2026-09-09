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

#include "editor/EditorTool.h"   // IWorldBridge (engine-free)

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class CameraDialog : public QDialog
{
	Q_OBJECT
public:
	explicit CameraDialog(QWidget* parent = nullptr);

	// The engine-free bridge to the camera splines (CameraManager). Without
	// it the dialog's actions are no-ops.
	void setBridge(IWorldBridge* bridge) { bridge_ = bridge; refresh(); }

	// The camera paths to list (CameraManager::splines names). Empty by
	// default — populated when cameraManager is wired.
	void setCameras(const QStringList& names);

private slots:
	void onCreateCamera();
	void onDeleteCamera();
	void onPlayCamera();
	void onListSelectionChanged();

private:
	// Reload the camera list from the bridge (if any).
	void refresh();

	IWorldBridge* bridge_ = nullptr;
	QListWidget* list_ = nullptr;
	QPushButton* btnCreate_ = nullptr;
	QPushButton* btnDelete_ = nullptr;
	QPushButton* btnPlay_ = nullptr;
	QLineEdit* nameEdit_ = nullptr;
	QLineEdit* timeEdit_ = nullptr;
	QLabel* statusLabel_ = nullptr;
};
