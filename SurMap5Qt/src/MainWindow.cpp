// MainWindow.cpp — see header.

#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolBar>

#include "RenderViewWidget.h"
#include "EditorApplication.h"
#include "dialogs/SelectWorldDialog.h"
#include "dialogs/WorldNameDialog.h"
#include "tools/ToolManager.h"

// Number of status-bar panes: 8 info + 2 separators — NUMBERS_PARTS_STATUSBAR
// in GeneralView.h (8 + 2).
static const int kStatusPaneCount = 10;

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle(tr("VistaEngine SurMap5"));

	createView();       // the 3D viewport (central widget)
	createActions();
	createMenus();      // QMenuBar (CExtMenuControlBar)
	createToolBars();   // QToolBars (CExtToolControlBar set)
	createDockPanels(); // QDockWidgets (CExtControlBar set)
	createStatusBar();  // QStatusBar with progress pane (CExtStatusControlBar)
}

void MainWindow::createView()
{
	view_ = new RenderViewWidget(this);
	setCentralWidget(view_);
}

void MainWindow::createActions()
{
	actNewWorld_ = new QAction(tr("&New World..."), this);
	actOpenWorld_ = new QAction(tr("&Open World..."), this);
	actSaveWorld_ = new QAction(tr("&Save"), this);
	actSaveWorldAs_ = new QAction(tr("Save &As..."), this);
	actExit_ = new QAction(tr("E&xit"), this);
	actAbout_ = new QAction(tr("&About..."), this);
	actToggleAnimation_ = new QAction(tr("&Animation"), this);
	actToggleAnimation_->setCheckable(true);
	actToggleAnimation_->setChecked(true); // flag_animation = 1

	// Tool actions — the tools tree's transform set (Select/Move/Rotate/Scale).
	// Checkable + an exclusive group mirrors the original's tool selection:
	// exactly one tool is current at a time.
	actToolSelect_ = new QAction(tr("&Select"), this);
	actToolMove_   = new QAction(tr("&Move"), this);
	actToolRotate_ = new QAction(tr("&Rotate"), this);
	actToolScale_  = new QAction(tr("&Scale"), this);
	actToolSelect_->setCheckable(true);
	actToolMove_->setCheckable(true);
	actToolRotate_->setCheckable(true);
	actToolScale_->setCheckable(true);
	actToolSelect_->setChecked(true);   // Select is the default tool
	actToolSelect_->setShortcut(QKeySequence(Qt::Key_S));
	actToolMove_->setShortcut(QKeySequence(Qt::Key_M));
	actToolRotate_->setShortcut(QKeySequence(Qt::Key_R));
	actToolScale_->setShortcut(QKeySequence(Qt::Key_T));

	auto* toolGroup = new QActionGroup(this);
	toolGroup->addAction(actToolSelect_);
	toolGroup->addAction(actToolMove_);
	toolGroup->addAction(actToolRotate_);
	toolGroup->addAction(actToolScale_);
	connect(actToolSelect_, &QAction::triggered, this, [this]{ selectTool(0); });
	connect(actToolMove_,   &QAction::triggered, this, [this]{ selectTool(1); });
	connect(actToolRotate_, &QAction::triggered, this, [this]{ selectTool(2); });
	connect(actToolScale_,  &QAction::triggered, this, [this]{ selectTool(3); });

	actExit_->setShortcut(QKeySequence::Quit);
	actSaveWorld_->setShortcut(QKeySequence::Save);
	actSaveWorldAs_->setShortcut(QKeySequence::SaveAs);

	// File menu: world save/open land in Phase 4 (DlgWorldName, DlgExImWorld);
	// the about dialog (CAboutDlg) is a plain QMessageBox.
	connect(actExit_, &QAction::triggered, this, &QWidget::close);
	connect(actAbout_, &QAction::triggered, this, &MainWindow::about);
	connect(actNewWorld_, &QAction::triggered, this, &MainWindow::newWorld);
	connect(actOpenWorld_, &QAction::triggered, this, &MainWindow::openWorld);
	connect(actToggleAnimation_, &QAction::triggered, this, [this](bool checked) {
		// CSurMap5App::OnViewAnimation toggled flag_animation; the loop timer
		// keeps running but stops repainting the view.
		view_->setUpdatesEnabled(checked);
	});
}

void MainWindow::createMenus()
{
	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(actNewWorld_);
	fileMenu->addAction(actOpenWorld_);
	fileMenu->addSeparator();
	fileMenu->addAction(actSaveWorld_);
	fileMenu->addAction(actSaveWorldAs_);
	fileMenu->addSeparator();
	fileMenu->addAction(actExit_);

	QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(actToggleAnimation_);

	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(actAbout_);
}

void MainWindow::createToolBars()
{
	// toolBar_ — main toolbar (IDR_MAINFRAME strip in the .rc). Actions will
	// carry icons from the original res/ bitmaps in Phase 1 polish.
	QToolBar* toolBar = addToolBar(tr("Main"));
	toolBar->setObjectName("mainToolBar"); // unique name for saveState()
	toolBar->addAction(actNewWorld_);
	toolBar->addAction(actOpenWorld_);
	toolBar->addAction(actSaveWorld_);
	toolBar->addAction(actSaveWorldAs_);

	// librariesBar_, editorsBar_, filtersBar_ — Phase 4/5, when the panels they
	// control exist. Each gets its own QToolBar with a distinct objectName so
	// saveState()/restoreState() can restore them.

	// toolsBar_ — the transform tool set (the tools tree's top level in
	// SurMap5; the original's toolbar strip IDR_TOOLBAR_TOOLS).
	QToolBar* toolsBar = addToolBar(tr("Tools"));
	toolsBar->setObjectName("toolsToolBar");
	toolsBar->addAction(actToolSelect_);
	toolsBar->addAction(actToolMove_);
	toolsBar->addAction(actToolRotate_);
	toolsBar->addAction(actToolScale_);
}

void MainWindow::createDockPanels()
{
	// toolsWindowBar_ — the tools tree + toolbar (CToolsTreeWindow).
	toolsDock_ = new QDockWidget(tr("Tools"), this);
	toolsDock_->setObjectName("toolsDock");
	toolsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::LeftDockWidgetArea, toolsDock_);

	// objectsManagerBar_ — world object tree (CObjectsManagerWindow), tabbed.
	objectsDock_ = new QDockWidget(tr("Objects"), this);
	objectsDock_->setObjectName("objectsDock");
	objectsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::LeftDockWidgetArea, objectsDock_);

	// propertiesBar_ — the current tool's dialog (CExtControlBar hosting
	// CSurToolBase). Phase 5.
	propertiesDock_ = new QDockWidget(tr("Properties"), this);
	propertiesDock_->setObjectName("propertiesDock");
	propertiesDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, propertiesDock_);

	// miniMapBar_ — the minimap render window (CMiniMapWindow). Phase 2.
	miniMapDock_ = new QDockWidget(tr("Minimap"), this);
	miniMapDock_->setObjectName("miniMapDock");
	miniMapDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, miniMapDock_);

	// TODO(Phase 7): QMainWindow::restoreState() from the saved geometry —
	// replaces CExtControlBar::ProfileBarStateSerialize. Call restoreState()
	// after all docks are created, and saveState() in closeEvent.
}

void MainWindow::createStatusBar()
{
	// The original status bar had NUMBERS_PARTS_STATUSBAR panes with a progress
	// control in pane 2. Qt's QStatusBar uses addPermanentWidget/addWidget
	// instead of fixed panes; keep a labelled progress widget on the right.
	progressBar_ = new QProgressBar(this);
	progressBar_->setRange(0, 100);
	progressBar_->setValue(0);
	progressBar_->setFixedWidth(160);
	progressBar_->setVisible(false);
	statusBar()->addPermanentWidget(progressBar_);

	statusBar()->showMessage(tr("Ready"));
}

void MainWindow::universeQuant()
{
	// CMainFrame::universeQuant ran logic at the logic time period via
	// SyncroTimer and invalidated the view. Phase 3 fills this in; for now the
	// tick reaches the viewport so animation can run.
	view_->tick();
}

void MainWindow::about()
{
	// CAboutDlg (SurMap5.cpp): a trivial dialog — QMessageBox::about.
	QMessageBox::about(this, tr("About VistaEngine SurMap5"),
	                   tr("VistaEngine map editor (Perimeter 2).\n"
	                      "Qt port — shell milestone."));
}

void MainWindow::openWorld()
{
	// CMainFrame's world-open path: CDlgSelectWorld over vMap.getWorldsDir().
	// The chosen world's loading (reInitWorld) lands in Phase 3b.
	const QString worldsDir = qobject_cast<EditorApplication*>(qApp)->worldsDir();
	SelectWorldDialog dlg(worldsDir, tr("Select world to open"), /*enableCreateDir=*/false, this);
	if(dlg.exec() == QDialog::Accepted)
		statusBar()->showMessage(tr("World selected: %1 (loading in Phase 3b)").arg(dlg.selectedWorld()));
}

void MainWindow::newWorld()
{
	// CMainFrame's new-world path: DlgWorldName to name it, then the world is
	// created and loaded (Phase 3b). For now the dialog confirms the name.
	const QString worldsDir = qobject_cast<EditorApplication*>(qApp)->worldsDir();
	SelectWorldDialog dlg(worldsDir, tr("New world"), /*enableCreateDir=*/true, this);
	dlg.setWindowTitle(tr("New world"));
	if(dlg.exec() == QDialog::Accepted)
		statusBar()->showMessage(tr("New world: %1 (creation in Phase 3b)").arg(dlg.selectedWorld()));
}

void MainWindow::selectTool(int index)
{
	// Switch the active editor tool (CToolsTreeWindow::selectTool equivalent).
	view_->tools()->setCurrentTool(index);
	statusBar()->showMessage(tr("Tool: %1").arg(view_->tools()->currentTool()->name()));
	view_->setFocus();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	// TODO(Phase 7): save dock/toolbar state here:
	//   QSettings().setValue("mainWindow/state", saveState());
	// (replaces saveDlgBarState()).
	QMainWindow::closeEvent(event);
}
