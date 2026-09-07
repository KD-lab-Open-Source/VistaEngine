// EditorApplication.h — application-wide initialization for the Qt port.
//
// Maps to CSurMap5App (SurMap5/SurMap5.cpp): where that did AfxOleInit,
// SetRegistryKey, ZipConfig::initArchives, UI_Render::create,
// TranslationManager setup, and created CMainFrame via LoadFrame(IDR_MAINFRAME).
// Each of those is either already handled by Qt/QSettings or moves in as its
// engine counterpart lands.

#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class EditorApplication : public QObject
{
	Q_OBJECT
public:
	EditorApplication();
	~EditorApplication() override;

	// The one editor object, set by main.cpp. MainWindow reaches the editor's
	// state through it (qApp is the QApplication, a different object, so
	// qobject_cast<EditorApplication*>(qApp) would be null).
	static EditorApplication* instance() { return s_instance; }

	// Editor-wide initialization. Returns false if the editor cannot run
	// (the engine's ZipConfig::initArchives is expected to be called from
	// here once Phase 2 links the engine in).
	bool initialize();

	// The editor's repaint/animation timer. The old MFC loop was driven from
	// CWinApp::OnIdle; in Qt, main.cpp connects its timeout to
	// MainWindow::universeQuant.
	QTimer& loopTimer() { return loopTimer_; }

	// The directory holding the world files (.spg). Phase 3b will source it
	// from vMap.getWorldsDir(); for now it is the editor's working directory.
	QString worldsDir() const { return worldsDir_; }
	void setWorldsDir(const QString& dir) { worldsDir_ = dir; }

private:
	QTimer loopTimer_;
	QString worldsDir_ = QStringLiteral("Resource/Worlds");
	static EditorApplication* s_instance;
};
