// MainWindow.h — the editor frame, Qt port of CMainFrame.
//
// Maps to SurMap5/MainFrame.h:
//   CExtMenuControlBar        -> menuBar (QMenuBar)
//   CExtToolControlBar tool/libraries/editors/filters
//                            -> QToolBar
//   CExtStatusControlBar      -> QStatusBar (10 panes, NUMBERS_PARTS_STATUSBAR)
//   CExtStatusBarProgressCtrl -> QProgressBar in a status-bar pane
//   CExtControlBar toolsWindowBar_ / objectsManagerBar_ / propertiesBar_ /
//                   miniMapBar_  -> QDockWidget
//   CExtControlBar::ProfileBarStateSerialize -> QMainWindow::saveState()/restoreState()
//   g_CmdManager command dispatch -> QAction
//
// Phase 1: frame only. Docked panels are placeholders; the 3D view
// (RenderViewWidget) is the central widget. Tools/dialogs/engine land in
// later phases.

#pragma once

#include <QMainWindow>

#include <QList>

class QAction;
class QDockWidget;
class QProgressBar;
class QTimer;
class QToolBar;
class RenderViewWidget;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);

	// Qt port of CMainFrame::universeQuant: the per-frame logic tick, driven
	// by EditorApplication's loop timer (old MFC OnIdle -> Invalidate).
	void universeQuant();

public slots:
	void about();

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createDockPanels();
	void createStatusBar();
	void createView();

	// --- view ---
	RenderViewWidget* view_ = nullptr;

	// --- panels (QDockWidget equivalents of the CExtControlBar members) ---
	QDockWidget* toolsDock_ = nullptr;     // toolsWindowBar_
	QDockWidget* objectsDock_ = nullptr;   // objectsManagerBar_
	QDockWidget* propertiesDock_ = nullptr;// propertiesBar_
	QDockWidget* miniMapDock_ = nullptr;   // miniMapBar_

	// --- status bar panes (NUMBERS_PARTS_STATUSBAR) ---
	QProgressBar* progressBar_ = nullptr;  // progressBar_

	// --- actions ---
	QAction* actNewWorld_ = nullptr;
	QAction* actOpenWorld_ = nullptr;
	QAction* actSaveWorld_ = nullptr;
	QAction* actSaveWorldAs_ = nullptr;
	QAction* actExit_ = nullptr;
	QAction* actAbout_ = nullptr;
	QAction* actToggleAnimation_ = nullptr;
};
