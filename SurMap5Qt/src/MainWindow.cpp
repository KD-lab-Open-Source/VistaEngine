// MainWindow.cpp — see header.

#include "MainWindow.h"

#include <cstdio>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include "RenderViewWidget.h"
#include "EditorApplication.h"
#include "dialogs/SelectWorldDialog.h"
#include "dialogs/WorldNameDialog.h"
#include "dialogs/WorldPropertiesDialog.h"
#include "dialogs/ExImWorldDialog.h"
#include "dialogs/ChangeTotalWorldHeightDialog.h"
#include "dialogs/TexturesStatisticsDialog.h"
#include "dialogs/TimeSliderDialog.h"
#include "dialogs/CameraDialog.h"
#include "dialogs/WaveDialog.h"
#include "tools/ToolManager.h"
#include "panels/ToolsTreePanel.h"
#include "panels/ObjectsTreePanel.h"
#include "panels/MiniMapPanel.h"
#include "panels/GradientsPanel.h"

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

	// Phase 7: restore the dock/toolbar layout the last run saved (CExtControlBar::
	// ProfileBarStateSerialize on exit -> surMapOptions.dlgBarState). Qt keeps the
	// equivalent state in QSettings; restoreState must come after every dock and
	// toolbar exists (objectNames are the keys).
	QSettings settings;
	restoreState(settings.value("mainWindow/state").toByteArray());
	restoreGeometry(settings.value("mainWindow/geometry").toByteArray());

	// Auto-reopen the last world (surMapOptions remembered the last open world
	// in the original too). Deferred a tick so the render device is ready (the
	// viewport inits on its first paint).
	const QString lastWorld = settings.value("mainWindow/lastWorld").toString();
	if(!lastWorld.isEmpty()){
		// The render device inits on the viewport's first paint; retry until
		// ready (world load needs the device). The window is already up by the
		// time this lambda runs (deferred with 0 ms from the ctor).
		QTimer::singleShot(0, this, [this, lastWorld]() -> void {
			auto retry = [this, lastWorld]{
				if(view_->loadWorld(EditorApplication::instance()->worldsDir(), lastWorld)){
					applySavedCameraDefault();
					updateWorldTitle();
					statusBar()->showMessage(tr("World restored: %1").arg(lastWorld));
					fprintf(stderr, "[restore] world %s loaded\n", lastWorld.toStdString().c_str());
				}
				else
					fprintf(stderr, "[restore] world %s FAILED\n", lastWorld.toStdString().c_str());
				fflush(stderr);
			};
			if(view_->isReady())
				retry();
			else
				QTimer::singleShot(50, this, retry);
		});
	}
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

	// File menu (CMainFrame's ID_FILE_* set from SurMap5.rc). The actions land
	// in createMenus; the slots behind the ones that need engine/UI work beyond
	// the current phase log to stderr and show a status message for now.
	actRunWorld_ = new QAction(tr("&Run World..."), this);
	actRunMenu_  = new QAction(tr("Run &Main Menu..."), this);
	actExportVistaEngine_ = new QAction(tr("&Export VistaEngine..."), this);
	actImportTextExcel_   = new QAction(tr("&Import Text from Excel..."), this);
	actExportTextExcel_   = new QAction(tr("Ex&port Text to Excel..."), this);
	actExImWorld_  = new QAction(tr("Export/Import World..."), this);
	actProperties_ = new QAction(tr("P&roperties..."), this);
	actStatistics_ = new QAction(tr("Statistics..."), this);
	actResaveWorlds_ = new QAction(tr("Resave All Worlds"), this);
	actMerge_      = new QAction(tr("Merge..."), this);
	actSaveMiniMapToWorld_ = new QAction(tr("Save MiniMap to World"), this);
	actSaveMiniMapToWorld_->setEnabled(false);   // OnUpdateIsWorldLoaded

	// Edit menu (ID_EDIT_*).
	actUndo_ = new QAction(tr("&Undo"), this);
	actUndo_->setShortcut(QKeySequence::Undo);
	actRedo_ = new QAction(tr("&Redo"), this);
	actRedo_->setShortcut(QKeySequence::Redo);
	actMapScenario_   = new QAction(tr("&Map Scenario..."), this);
	actGameScenario_  = new QAction(tr("&Game Scenario..."), this);
	actSaveCameraAsDefault_ = new QAction(tr("Save Camera As Default"), this);
	actRebuildWorld_  = new QAction(tr("&Rebuild World"), this);
	actUpdateSurface_ = new QAction(tr("Update Surface"), this);
	actChangeTotalWorldHeight_ = new QAction(tr("Change Total World Param"), this);
	actRollingBorder_ = new QAction(tr("Rolling Border"), this);

	// View menu (ID_VIEW_*). Checkable: they mirror the original's toggle
	// buttons on the filters bar and the checkmark state in the View menu.
	actViewSources_ = new QAction(tr("&Sources"), this);
	actViewSources_->setCheckable(true);
	actViewCameras_ = new QAction(tr("&Cameras"), this);
	actViewCameras_->setCheckable(true);
	actViewGeosurface_ = new QAction(tr("&Geosurface"), this);
	actViewGeosurface_->setCheckable(true);
	actViewPathFinding_ = new QAction(tr("Path Finding"), this);
	actViewPathFinding_->setCheckable(true);
	actViewPathFindingRef_ = new QAction(tr("Path Finding - Select Reference Unit"), this);
	actViewShowGrid_ = new QAction(tr("Show &Grid"), this);
	actViewShowGrid_->setCheckable(true);
	actViewShowGrid_->setChecked(true);   // the editor starts with the grid on
	actViewCameraBorders_ = new QAction(tr("Show Camera &Borders"), this);
	actViewCameraBorders_->setCheckable(true);
	actViewTimeFlow_ = new QAction(tr("Enable &Time Flow"), this);
	actViewTimeFlow_->setCheckable(true);
	actViewTimeSlider_ = new QAction(tr("Time of Day..."), this);   // ID_VIEW_TIME_SLIDER
	actViewHideModels_ = new QAction(tr("Hide Models"), this);
	actViewHideModels_->setCheckable(true);
	actViewObjectsManager_ = new QAction(tr("Objects Manager"), this);
	actViewObjectsManager_->setCheckable(true);
	actViewObjectsManager_->setChecked(true);

	// Libraries menu (ID_EDIT_* / ID_LIBRARIES_* — the library editors the
	// libraries bar launches; each is a separate external-ish editor that later
	// phases host as a dock or dialog).
	actLibUnits_ = new QAction(tr("U&nits..."), this);
	actLibEffects_ = new QAction(tr("&Effects..."), this);
	actLibSounds_ = new QAction(tr("&Sounds..."), this);
	actLibUIMessageTypes_ = new QAction(tr("UI Message Types..."), this);
	actLibUIMessages_ = new QAction(tr("UI &Messages..."), this);
	actLibUIShowModeSprites_ = new QAction(tr("UI Show Mode Sprites..."), this);
	actLibSoundTracks_ = new QAction(tr("Sound Tracks..."), this);
	actLibReels_ = new QAction(tr("Cut-Scenes..."), this);
	actLibHeads_ = new QAction(tr("Heads..."), this);
	actLibTerTools_ = new QAction(tr("TerTools..."), this);
	actLibCursors_ = new QAction(tr("Cursors..."), this);
	actLibCommandColors_ = new QAction(tr("Command Colors"), this);
	actLibTextImages_ = new QAction(tr("Text images"), this);
	actLibTerrainTypeName_ = new QAction(tr("Terrrain Type Name"), this);
	actLibImportParametersFull_ = new QAction(tr("Full"), this);
	actLibImportParametersByGroups_ = new QAction(tr("By Groups"), this);
	actLibExportParametersFull_ = new QAction(tr("Full"), this);
	actLibExportParametersByGroups_ = new QAction(tr("By Groups"), this);
	actLibExportParametersStatistics_ = new QAction(tr("Balance Parameters"), this);

	// Tools menu (ID_EDIT_* — the editors bar).
	actToolUIEditor_ = new QAction(tr("&UI Editor"), this);
	actToolEffectsEditor_ = new QAction(tr("&Effects Editor"), this);
	actToolTriggers_ = new QAction(tr("Edit &Triggers..."), this);

	// Workspace menu (ID_VIEW_* — dock/toolbar toggles).
	actWsReset_ = new QAction(tr("&Reset Workspace"), this);
	actWsMenuBar_ = new QAction(tr("Menu Bar"), this);
	actWsMenuBar_->setCheckable(true);
	actWsMenuBar_->setChecked(true);
	actWsMainToolbar_ = new QAction(tr("Main &Toolbar"), this);
	actWsMainToolbar_->setCheckable(true);
	actWsMainToolbar_->setChecked(true);
	actWsFiltersBar_ = new QAction(tr("Filters Bar"), this);
	actWsFiltersBar_->setCheckable(true);
	actWsLibrariesBar_ = new QAction(tr("Libraries Bar"), this);
	actWsLibrariesBar_->setCheckable(true);
	actWsEditorsBar_ = new QAction(tr("Editors Bar"), this);
	actWsEditorsBar_->setCheckable(true);
	actWsStatusBar_ = new QAction(tr("&Status Bar"), this);
	actWsStatusBar_->setCheckable(true);
	actWsStatusBar_->setChecked(true);
	actWsTools_ = new QAction(tr("Tools"), this);
	actWsTools_->setCheckable(true);
	actWsTools_->setChecked(true);
	actWsProperties_ = new QAction(tr("Properties"), this);
	actWsProperties_->setCheckable(true);
	actWsProperties_->setChecked(true);
	actWsMinimap_ = new QAction(tr("&Minimap"), this);
	actWsMinimap_->setCheckable(true);
	actWsObjectsManager_ = new QAction(tr("Objects Manager"), this);
	actWsObjectsManager_->setCheckable(true);
	actWsObjectsManager_->setChecked(true);

	// Debug menu (ID_DEBUG_*).
	actDbgEditableTree_ = new QAction(tr("Editable Tools Tree"), this);
	actDbgEditableTree_->setCheckable(true);
	actDbgSaveTree_ = new QAction(tr("Save Tools Tree"), this);
	actDbgEditZipConfig_ = new QAction(tr("Edit ZipConfig..."), this);
	actDbgEditDebugPrm_ = new QAction(tr("Edit debugPrm..."), this);
	actDbgShowPaletteTexture_ = new QAction(tr("Show Palette Texture"), this);
	actDbgShowPaletteTexture_->setCheckable(true);
	actDbgShowMipmap_ = new QAction(tr("Show Mipmap"), this);
	actDbgShowMipmap_->setCheckable(true);

	// File menu: world save/open land in Phase 4 (DlgWorldName, DlgExImWorld);
	// the about dialog (CAboutDlg) is a plain QMessageBox.
	connect(actExit_, &QAction::triggered, this, &QWidget::close);
	connect(actAbout_, &QAction::triggered, this, &MainWindow::about);
	connect(actNewWorld_, &QAction::triggered, this, &MainWindow::newWorld);
	connect(actOpenWorld_, &QAction::triggered, this, &MainWindow::openWorld);
	connect(actSaveWorld_, &QAction::triggered, this, &MainWindow::fileSave);
	connect(actSaveWorldAs_, &QAction::triggered, this, &MainWindow::fileSaveAs);
	connect(actRunWorld_, &QAction::triggered, this, &MainWindow::fileRunWorld);
	connect(actRunMenu_, &QAction::triggered, this, &MainWindow::fileRunMenu);
	connect(actExportVistaEngine_, &QAction::triggered, this, &MainWindow::fileExportVistaEngine);
	connect(actImportTextExcel_, &QAction::triggered, this, &MainWindow::fileImportTextFromExcel);
	connect(actExportTextExcel_, &QAction::triggered, this, &MainWindow::fileExportTextToExcel);
	connect(actExImWorld_, &QAction::triggered, this, &MainWindow::fileExImWorld);
	connect(actProperties_, &QAction::triggered, this, &MainWindow::fileProperties);
	connect(actStatistics_, &QAction::triggered, this, &MainWindow::fileStatistics);
	connect(actResaveWorlds_, &QAction::triggered, this, &MainWindow::fileResaveWorlds);
	connect(actMerge_, &QAction::triggered, this, &MainWindow::fileMerge);
	connect(actSaveMiniMapToWorld_, &QAction::triggered, this, &MainWindow::fileSaveMiniMapToWorld);

	// Edit menu.
	connect(actUndo_, &QAction::triggered, this, &MainWindow::editUndo);
	connect(actRedo_, &QAction::triggered, this, &MainWindow::editRedo);
	connect(actMapScenario_, &QAction::triggered, this, &MainWindow::editMapScenario);
	connect(actGameScenario_, &QAction::triggered, this, &MainWindow::editGameScenario);
	connect(actSaveCameraAsDefault_, &QAction::triggered, this, &MainWindow::editSaveCameraAsDefault);
	connect(actRebuildWorld_, &QAction::triggered, this, &MainWindow::editRebuildWorld);
	connect(actUpdateSurface_, &QAction::triggered, this, &MainWindow::editUpdateSurface);
	connect(actChangeTotalWorldHeight_, &QAction::triggered, this, &MainWindow::editChangeTotalWorldHeight);
	connect(actRollingBorder_, &QAction::triggered, this, &MainWindow::editRollingBorder);

	// View menu (toggled).
	connect(actViewSources_, &QAction::toggled, this, &MainWindow::viewToggleSources);
	connect(actViewCameras_, &QAction::toggled, this, &MainWindow::viewToggleCameras);
	connect(actViewGeosurface_, &QAction::toggled, this, &MainWindow::viewToggleGeosurface);
	connect(actViewPathFinding_, &QAction::toggled, this, &MainWindow::viewTogglePathFinding);
	connect(actViewShowGrid_, &QAction::toggled, this, &MainWindow::viewToggleGrid);
	connect(actViewCameraBorders_, &QAction::toggled, this, &MainWindow::viewToggleCameraBorders);
	connect(actViewTimeFlow_, &QAction::toggled, this, &MainWindow::viewToggleTimeFlow);
	connect(actViewTimeSlider_, &QAction::triggered, this, &MainWindow::viewTimeSlider);
	connect(actViewHideModels_, &QAction::toggled, this, &MainWindow::viewToggleHideModels);
	connect(actViewObjectsManager_, &QAction::toggled, this, [this](bool on){ if(objectsDock_) objectsDock_->setVisible(on); });

	connect(actToggleAnimation_, &QAction::triggered, this, [this](bool checked) {
		// CSurMap5App::OnViewAnimation toggled flag_animation; the loop timer
		// keeps running but stops repainting the view.
		view_->setUpdatesEnabled(checked);
	});

	// Libraries / Tools / Workspace / Debug: stub handlers. Each logs and shows
	// a status message; the library editors and external tools land in later
	// phases (U6 panels, U2 external-tool launches). Grouped so the menus exist
	// and are navigable now.
	auto stub = [this](const char* tag, const QString& msg){
		fprintf(stderr, "[ui] %s: TODO\n", tag);
		statusBar()->showMessage(msg);
	};
	connect(actLibUnits_, &QAction::triggered, this, [this, stub]{ stub("libraries/units", tr("Units editor: not wired yet")); });
	connect(actLibEffects_, &QAction::triggered, this, [this, stub]{ stub("libraries/effects", tr("Effects editor: not wired yet")); });
	connect(actLibSounds_, &QAction::triggered, this, [this, stub]{ stub("libraries/sounds", tr("Sounds editor: not wired yet")); });
	connect(actLibUIMessageTypes_, &QAction::triggered, this, [this, stub]{ stub("libraries/ui-message-types", tr("UI Message Types: not wired yet")); });
	connect(actLibUIMessages_, &QAction::triggered, this, [this, stub]{ stub("libraries/ui-messages", tr("UI Messages: not wired yet")); });
	connect(actLibUIShowModeSprites_, &QAction::triggered, this, [this, stub]{ stub("libraries/ui-show-mode-sprites", tr("UI Show Mode Sprites: not wired yet")); });
	connect(actLibSoundTracks_, &QAction::triggered, this, [this, stub]{ stub("libraries/sound-tracks", tr("Sound Tracks: not wired yet")); });
	connect(actLibReels_, &QAction::triggered, this, [this, stub]{ stub("libraries/reels", tr("Cut-Scenes: not wired yet")); });
	connect(actLibHeads_, &QAction::triggered, this, [this, stub]{ stub("libraries/heads", tr("Heads: not wired yet")); });
	connect(actLibTerTools_, &QAction::triggered, this, [this, stub]{ stub("libraries/tertools", tr("TerTools: not wired yet")); });
	connect(actLibCursors_, &QAction::triggered, this, [this, stub]{ stub("libraries/cursors", tr("Cursors: not wired yet")); });
	connect(actLibCommandColors_, &QAction::triggered, this, [this, stub]{ stub("libraries/command-colors", tr("Command Colors: not wired yet")); });
	connect(actLibTextImages_, &QAction::triggered, this, [this, stub]{ stub("libraries/text-images", tr("Text images: not wired yet")); });
	connect(actLibTerrainTypeName_, &QAction::triggered, this, [this, stub]{ stub("libraries/terrain-type-name", tr("Terrain Type Name: not wired yet")); });
	connect(actLibImportParametersFull_, &QAction::triggered, this, [this, stub]{ stub("libraries/import-params-full", tr("Import Parameters (Full): not wired yet")); });
	connect(actLibImportParametersByGroups_, &QAction::triggered, this, [this, stub]{ stub("libraries/import-params-groups", tr("Import Parameters (By Groups): not wired yet")); });
	connect(actLibExportParametersFull_, &QAction::triggered, this, [this, stub]{ stub("libraries/export-params-full", tr("Export Parameters (Full): not wired yet")); });
	connect(actLibExportParametersByGroups_, &QAction::triggered, this, [this, stub]{ stub("libraries/export-params-groups", tr("Export Parameters (By Groups): not wired yet")); });
	connect(actLibExportParametersStatistics_, &QAction::triggered, this, [this, stub]{ stub("libraries/export-params-statistics", tr("Export Parameters (Balance): not wired yet")); });

	connect(actToolUIEditor_, &QAction::triggered, this, [this, stub]{ stub("tools/ui-editor", tr("UI Editor: not wired yet")); });
	connect(actToolEffectsEditor_, &QAction::triggered, this, [this, stub]{ stub("tools/effects-editor", tr("Effects Editor: not wired yet")); });
	connect(actToolTriggers_, &QAction::triggered, this, [this, stub]{ stub("tools/triggers", tr("Edit Triggers: not wired yet")); });

	// Workspace: dock/toolbar visibility toggles. The docks exist (createDockPanels)
	// and the toolbars exist (createToolBars) by the time these fire, so toggle
	// them for real.
	connect(actWsReset_, &QAction::triggered, this, [this]{
		restoreState(QByteArray());   // OnViewResettoolbar2default
		statusBar()->showMessage(tr("Workspace reset"));
	});
	connect(actWsMenuBar_, &QAction::toggled, this, [this](bool on){ menuBar()->setVisible(on); });
	connect(actWsMainToolbar_, &QAction::toggled, this, [this](bool on){ if(mainToolBar_) mainToolBar_->setVisible(on); });
	connect(actWsStatusBar_, &QAction::toggled, this, [this](bool on){ statusBar()->setVisible(on); });
	connect(actWsTools_, &QAction::toggled, this, [this](bool on){ if(toolsDock_) toolsDock_->setVisible(on); });
	connect(actWsProperties_, &QAction::toggled, this, [this](bool on){ if(propertiesDock_) propertiesDock_->setVisible(on); });
	connect(actWsMinimap_, &QAction::toggled, this, [this](bool on){ if(miniMapDock_) miniMapDock_->setVisible(on); });
	connect(actWsObjectsManager_, &QAction::toggled, this, [this](bool on){ if(objectsDock_) objectsDock_->setVisible(on); });
	connect(actWsFiltersBar_, &QAction::toggled, this, [this](bool on){ if(filtersToolBar_) filtersToolBar_->setVisible(on); });
	connect(actWsLibrariesBar_, &QAction::toggled, this, [this](bool on){ if(librariesToolBar_) librariesToolBar_->setVisible(on); });
	connect(actWsEditorsBar_, &QAction::toggled, this, [this](bool on){ if(editorsToolBar_) editorsToolBar_->setVisible(on); });

	connect(actDbgEditableTree_, &QAction::toggled, this, [this, stub](bool on){ (void)on; stub("debug/editable-tree", tr("Editable Tools Tree: not wired yet")); });
	connect(actDbgSaveTree_, &QAction::triggered, this, [this, stub]{ stub("debug/save-tree", tr("Save Tools Tree: not wired yet")); });
	connect(actDbgEditZipConfig_, &QAction::triggered, this, [this, stub]{ stub("debug/edit-zipconfig", tr("Edit ZipConfig: not wired yet")); });
	connect(actDbgEditDebugPrm_, &QAction::triggered, this, [this, stub]{ stub("debug/edit-debugprm", tr("Edit debugPrm: not wired yet")); });
	connect(actDbgShowPaletteTexture_, &QAction::toggled, this, [this](bool on){ (void)on; });
	connect(actDbgShowMipmap_, &QAction::toggled, this, [this](bool on){ (void)on; });
}

void MainWindow::createMenus()
{
	// File (IDR_MAINFRAME's File popup).
	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(actNewWorld_);
	fileMenu->addAction(actOpenWorld_);
	fileMenu->addSeparator();
	fileMenu->addAction(actSaveWorld_);
	fileMenu->addAction(actSaveWorldAs_);
	// "Save MiniMap to World" / "Save Without terTool Color" — engine save
	// variants, deferred until the minimap/save plumbing lands (U7).
	fileMenu->addAction(actSaveMiniMapToWorld_);
	fileMenu->addSeparator();
	fileMenu->addAction(actResaveWorlds_);
	fileMenu->addSeparator();
	fileMenu->addAction(actMerge_);
	fileMenu->addSeparator();
	fileMenu->addAction(actRunWorld_);
	fileMenu->addAction(actRunMenu_);
	fileMenu->addAction(actExportVistaEngine_);
	fileMenu->addSeparator();
	fileMenu->addAction(actImportTextExcel_);
	fileMenu->addAction(actExportTextExcel_);
	fileMenu->addAction(actExImWorld_);
	fileMenu->addSeparator();
	fileMenu->addAction(actProperties_);
	fileMenu->addAction(actStatistics_);
	fileMenu->addSeparator();
	fileMenu->addAction(actExit_);

	// Edit (IDR_MAINFRAME's Edit popup).
	QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(actUndo_);
	editMenu->addAction(actRedo_);
	editMenu->addSeparator();
	editMenu->addAction(actMapScenario_);
	editMenu->addAction(actGameScenario_);
	editMenu->addSeparator();
	editMenu->addAction(actSaveCameraAsDefault_);
	editMenu->addSeparator();
	editMenu->addAction(actRebuildWorld_);
	editMenu->addAction(actUpdateSurface_);
	editMenu->addAction(actChangeTotalWorldHeight_);
	editMenu->addAction(actRollingBorder_);

	// View (IDR_MAINFRAME's View popup).
	QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(actViewSources_);
	viewMenu->addAction(actViewCameras_);
	viewMenu->addAction(actViewGeosurface_);
	viewMenu->addSeparator();
	viewMenu->addAction(actViewPathFinding_);
	viewMenu->addAction(actViewPathFindingRef_);
	viewMenu->addSeparator();
	viewMenu->addAction(actViewShowGrid_);
	viewMenu->addAction(actViewCameraBorders_);
	viewMenu->addAction(actToggleAnimation_);
	viewMenu->addAction(actViewTimeFlow_);
	viewMenu->addAction(actViewTimeSlider_);
	viewMenu->addAction(actViewHideModels_);

	// Libraries (IDR_MAINFRAME's Libraries popup).
	QMenu* libMenu = menuBar()->addMenu(tr("&Libraries"));
	libMenu->addAction(actLibUnits_);
	libMenu->addAction(actLibEffects_);
	libMenu->addAction(actLibSounds_);
	libMenu->addAction(actLibUIMessageTypes_);
	libMenu->addAction(actLibUIMessages_);
	libMenu->addAction(actLibUIShowModeSprites_);
	libMenu->addSeparator();
	libMenu->addAction(actLibSoundTracks_);
	libMenu->addAction(actLibReels_);
	libMenu->addAction(actLibHeads_);
	libMenu->addAction(actLibTerTools_);
	libMenu->addAction(actLibCursors_);
	libMenu->addAction(actLibCommandColors_);
	libMenu->addAction(actLibTextImages_);
	libMenu->addAction(actLibTerrainTypeName_);
	libMenu->addSeparator();
	QMenu* importMenu = libMenu->addMenu(tr("Import Parameters from Excel"));
	importMenu->addAction(actLibImportParametersFull_);
	importMenu->addAction(actLibImportParametersByGroups_);
	QMenu* exportMenu = libMenu->addMenu(tr("Export Parameters to Excel"));
	exportMenu->addAction(actLibExportParametersFull_);
	exportMenu->addAction(actLibExportParametersByGroups_);
	exportMenu->addAction(actLibExportParametersStatistics_);

	// Tools (IDR_MAINFRAME's Tools popup — the editors bar launches).
	QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
	toolsMenu->addAction(actToolUIEditor_);
	toolsMenu->addAction(actToolEffectsEditor_);
	toolsMenu->addSeparator();
	toolsMenu->addAction(actToolTriggers_);

	// Workspace (IDR_MAINFRAME's Workspace popup — dock/toolbar toggles).
	QMenu* wsMenu = menuBar()->addMenu(tr("&Workspace"));
	wsMenu->addAction(actWsReset_);
	wsMenu->addSeparator();
	wsMenu->addAction(actWsMenuBar_);
	wsMenu->addAction(actWsMainToolbar_);
	wsMenu->addAction(actWsFiltersBar_);
	wsMenu->addAction(actWsLibrariesBar_);
	wsMenu->addAction(actWsEditorsBar_);
	wsMenu->addAction(actWsStatusBar_);
	wsMenu->addSeparator();
	wsMenu->addAction(actWsTools_);
	wsMenu->addAction(actWsProperties_);
	wsMenu->addAction(actWsMinimap_);
	wsMenu->addAction(actWsObjectsManager_);

	// Debug (IDR_MAINFRAME's Debug popup).
	QMenu* debugMenu = menuBar()->addMenu(tr("&Debug"));
	debugMenu->addAction(actDbgEditableTree_);
	debugMenu->addAction(actDbgSaveTree_);
	debugMenu->addSeparator();
	debugMenu->addAction(actDbgEditZipConfig_);
	debugMenu->addAction(actDbgEditDebugPrm_);
	debugMenu->addSeparator();
	debugMenu->addAction(actDbgShowPaletteTexture_);
	debugMenu->addAction(actDbgShowMipmap_);

	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(actAbout_);
}

void MainWindow::createToolBars()
{
	// toolBar_ — main toolbar (IDR_MAINFRAME strip in the .rc). Actions will
	// carry icons from the original res/ bitmaps in Phase 1 polish.
	mainToolBar_ = addToolBar(tr("Main"));
	mainToolBar_->setObjectName("mainToolBar"); // unique name for saveState()
	mainToolBar_->addAction(actNewWorld_);
	mainToolBar_->addAction(actOpenWorld_);
	mainToolBar_->addAction(actSaveWorld_);
	mainToolBar_->addSeparator();
	mainToolBar_->addAction(actRunWorld_);
	mainToolBar_->addAction(actRunMenu_);
	mainToolBar_->addSeparator();
	mainToolBar_->addAction(actExportVistaEngine_);
	mainToolBar_->addSeparator();
	mainToolBar_->addAction(actUndo_);
	mainToolBar_->addAction(actRedo_);
	mainToolBar_->addSeparator();
	mainToolBar_->addAction(actUpdateSurface_);
	mainToolBar_->addSeparator();
	mainToolBar_->addAction(actMapScenario_);
	mainToolBar_->addAction(actGameScenario_);
	mainToolBar_->addAction(actViewObjectsManager_);

	// filtersBar_ — IDR_FILTERS_BAR: the view toggles + the time slider. The
	// time slider (IDD_TIME_SLIDER) is a separate widget in the original; it
	// lands with U5 (TimeSliderDlg) as a widget action.
	filtersToolBar_ = addToolBar(tr("Filters"));
	filtersToolBar_->setObjectName("filtersToolBar");
	filtersToolBar_->addAction(actViewShowGrid_);
	filtersToolBar_->addAction(actViewTimeFlow_);
	filtersToolBar_->addSeparator();
	filtersToolBar_->addAction(actViewSources_);
	filtersToolBar_->addAction(actViewCameras_);
	filtersToolBar_->addAction(actViewPathFinding_);
	filtersToolBar_->addAction(actViewPathFindingRef_);

	// librariesBar_ — IDR_LIBRARIES_BAR: the library editors.
	librariesToolBar_ = addToolBar(tr("Libraries"));
	librariesToolBar_->setObjectName("librariesToolBar");
	librariesToolBar_->addAction(actLibEffects_);
	librariesToolBar_->addAction(actLibSounds_);
	librariesToolBar_->addAction(actLibUIMessages_);
	librariesToolBar_->addAction(actLibSoundTracks_);
	librariesToolBar_->addAction(actLibReels_);
	librariesToolBar_->addAction(actLibHeads_);
	librariesToolBar_->addAction(actLibTerTools_);
	librariesToolBar_->addAction(actLibCursors_);

	// editorsBar_ — IDR_EDITORS_BAR: the external tool launchers.
	editorsToolBar_ = addToolBar(tr("Editors"));
	editorsToolBar_->setObjectName("editorsToolBar");
	editorsToolBar_->addAction(actToolUIEditor_);
	editorsToolBar_->addAction(actToolTriggers_);
	editorsToolBar_->addAction(actToolEffectsEditor_);
	editorsToolBar_->addAction(actLibUnits_);

	// toolsBar_ — the transform tool set (the tools tree's top level in
	// SurMap5; the original's toolbar strip IDR_TOOLBAR_TOOLS).
	toolsToolBar_ = addToolBar(tr("Tools"));
	toolsToolBar_->setObjectName("toolsToolBar");
	toolsToolBar_->addAction(actToolSelect_);
	toolsToolBar_->addAction(actToolMove_);
	toolsToolBar_->addAction(actToolRotate_);
	toolsToolBar_->addAction(actToolScale_);
	toolsToolBar_->addSeparator();

	// ID_BRUSH_COMBO_PLACE — the brush-radius combo CToolsTreeWindow created on
	// the tools toolbar (SurMap5/ToolsTreeWindow.cpp:87). The ArrSize_Brush
	// list {1,3,5,7,10,15,20,30,50,75,100,150,200}; the data is the radius in
	// world units, shown as-is (the original's SetItemData held the number).
	brushRadiusCombo_ = new QComboBox(toolsToolBar_);
	brushRadiusCombo_->setObjectName("brushRadiusCombo");
	brushRadiusCombo_->setEditable(false);
	static const long kArrSizeBrush[] = {1, 3, 5, 7, 10, 15, 20, 30, 50, 75, 100, 150, 200};
	for(long v : kArrSizeBrush)
		brushRadiusCombo_->addItem(QString::number(v), QVariant((qlonglong)v));
	brushRadiusCombo_->setCurrentIndex(0);
	brushRadius_ = 1;
	toolsToolBar_->addWidget(brushRadiusCombo_);
	connect(brushRadiusCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &MainWindow::brushRadiusChanged);
}

void MainWindow::createDockPanels()
{
	// toolsWindowBar_ — the tools tree + toolbar (CToolsTreeWindow).
	toolsDock_ = new QDockWidget(tr("Tools"), this);
	toolsDock_->setObjectName("toolsDock");
	toolsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	toolsTreePanel_ = new ToolsTreePanel(view_->tools(), toolsDock_);
	toolsDock_->setWidget(toolsTreePanel_);
	addDockWidget(Qt::LeftDockWidgetArea, toolsDock_);

	// Selecting a tool in the tree switches the current tool (the toolbar's
	// QActionGroup does the same; the tree and toolbar stay in sync).
	connect(toolsTreePanel_, &ToolsTreePanel::toolSelected, this, &MainWindow::selectTool);

	// objectsManagerBar_ — world object tree (CObjectsManagerWindow), tabbed.
	objectsDock_ = new QDockWidget(tr("Objects"), this);
	objectsDock_->setObjectName("objectsDock");
	objectsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	objectsTreePanel_ = new ObjectsTreePanel(objectsDock_);
	objectsDock_->setWidget(objectsTreePanel_);
	addDockWidget(Qt::LeftDockWidgetArea, objectsDock_);

	// propertiesBar_ — the current tool's dialog (CExtControlBar hosting
	// CSurToolBase). Phase 5.
	propertiesDock_ = new QDockWidget(tr("Properties"), this);
	propertiesDock_->setObjectName("propertiesDock");
	propertiesDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, propertiesDock_);

	// miniMapBar_ — the minimap render window (CMiniMapWindow). U6: the map
	// image + camera marker panel (MiniMapPanel).
	miniMapDock_ = new QDockWidget(tr("Minimap"), this);
	miniMapDock_->setObjectName("miniMapDock");
	miniMapDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	miniMapPanel_ = new MiniMapPanel(view_, miniMapDock_);
	miniMapDock_->setWidget(miniMapPanel_);
	addDockWidget(Qt::RightDockWidgetArea, miniMapDock_);

	// The gradients editor (CGradientsWindow) — U6 panel. The original hosted
	// it as a bar dialog; here it is a dock next to the minimap.
	gradientsDock_ = new QDockWidget(tr("Gradients"), this);
	gradientsDock_->setObjectName("gradientsDock");
	gradientsDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	gradientsPanel_ = new GradientsPanel(gradientsDock_);
	gradientsDock_->setWidget(gradientsPanel_);
	addDockWidget(Qt::RightDockWidgetArea, gradientsDock_);

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
	// CMainFrame's world-open path: CDlgSelectWorld over vMap.getWorldsDir(),
	// then vMap.load + reInitWorld.
	const QString worldsDir = EditorApplication::instance()->worldsDir();
	SelectWorldDialog dlg(worldsDir, tr("Select world to open"), /*enableCreateDir=*/false, this);
	if(dlg.exec() != QDialog::Accepted)
		return;

	const QString worldName = dlg.selectedWorld();
	statusBar()->showMessage(tr("Loading world: %1...").arg(worldName));
	QApplication::processEvents();

	// Phase 3b: vMap.load + terrain tile map (EngineViewport::loadWorld).
	if(view_->loadWorld(worldsDir, worldName)){
		QSettings().setValue("mainWindow/lastWorld", worldName);
		applySavedCameraDefault();      // OnFileOpen re-created the camera with the default
		updateWorldTitle();             // put2TitleNameDirWorld
		statusBar()->showMessage(tr("World loaded: %1").arg(worldName));
		if(miniMapPanel_)
			miniMapPanel_->reload();      // CMiniMapWindow::onWorldChanged
		actSaveMiniMapToWorld_->setEnabled(true);
	}
	else
		statusBar()->showMessage(tr("Failed to load world: %1").arg(worldName));
}

void MainWindow::newWorld()
{
	// CMainFrame's new-world path: DlgWorldName to name it, then the world is
	// created and loaded. vMap.create needs a creation dialog (WorldCreationParam
	// in the original); for now create a default world (128x128, flat).
	const QString worldsDir = EditorApplication::instance()->worldsDir();
	SelectWorldDialog dlg(worldsDir, tr("New world"), /*enableCreateDir=*/true, this);
	dlg.setWindowTitle(tr("New world"));
	if(dlg.exec() != QDialog::Accepted)
		return;

	const QString worldName = dlg.selectedWorld();
	if(worldName.isEmpty())
		return;

	statusBar()->showMessage(tr("Creating world: %1...").arg(worldName));
	QApplication::processEvents();

	// vMap.create makes a default world; then load it (Phase 3b).
	if(view_->createWorld(worldsDir, worldName)){
		QSettings().setValue("mainWindow/lastWorld", worldName);
		applySavedCameraDefault();
		updateWorldTitle();
		statusBar()->showMessage(tr("World created: %1").arg(worldName));
		if(miniMapPanel_)
			miniMapPanel_->reload();
		actSaveMiniMapToWorld_->setEnabled(true);
	}
	else
		statusBar()->showMessage(tr("Failed to create world: %1").arg(worldName));
}

void MainWindow::updateWorldTitle()
{
	// put2TitleNameDirWorld (SurMap5/MainFrame.cpp): the loaded world's name
	// in the frame title.
	const QString name = view_->worldName();
	setWindowTitle(name.isEmpty() ? tr("VistaEngine SurMap5")
	                              : tr("VistaEngine SurMap5 — %1").arg(name));
}

void MainWindow::applySavedCameraDefault()
{
	// The editor's camera default (editSaveCameraAsDefault persisted it; the
	// original's GlobalAttributes::setCameraCoordinate + camera init read it
	// back). Applied after each world load, like the original's camera create.
	const QSettings settings;
	const double distance = settings.value("editor/cameraDefaultDistance", -1.0).toDouble();
	const double theta = settings.value("editor/cameraDefaultTheta", -1.0).toDouble();
	if(distance < 0.0 || theta < 0.0)
		return;
	view_->setOrbitCamera((float)distance, (float)theta);
}

void MainWindow::selectTool(int index)
{
	// Switch the active editor tool (CToolsTreeWindow::selectTool equivalent).
	view_->tools()->setCurrentTool(index);
	if(toolsTreePanel_)
		toolsTreePanel_->syncToTool();
	statusBar()->showMessage(tr("Tool: %1").arg(view_->tools()->currentTool()->name()));
	view_->setFocus();
}

void MainWindow::selftestCreateWorld(const QString& worldName)
{
	// Temporary: the --selftest hook in main.cpp. Tries Open World first
	// (vMap.load); if the world does not exist, creates it (vMap.create+save).
	// The status bar reports the outcome.
	const QString worldsDir = EditorApplication::instance()->worldsDir();
	statusBar()->showMessage(tr("Selftest: world %1...").arg(worldName));
	QApplication::processEvents();

	fprintf(stderr, "[selftest] trying load world=%s dir=%s\n",
	        worldName.toStdString().c_str(), worldsDir.toStdString().c_str());
	fflush(stderr);

	if(view_->loadWorld(worldsDir, worldName)){
		statusBar()->showMessage(tr("Selftest OK: world %1 loaded").arg(worldName));
		QSettings().setValue("mainWindow/lastWorld", worldName);
		fprintf(stderr, "[selftest] LOAD OK\n");
		fflush(stderr);
		QCoreApplication::exit(0);
		return;
	}

	fprintf(stderr, "[selftest] load failed, creating\n");
	fflush(stderr);
	if(view_->createWorld(worldsDir, worldName)){
		statusBar()->showMessage(tr("Selftest OK: world %1 created and loaded").arg(worldName));
		QSettings().setValue("mainWindow/lastWorld", worldName);
		fprintf(stderr, "[selftest] CREATE OK\n");
		fflush(stderr);
		QCoreApplication::exit(0);
	}
	else{
		statusBar()->showMessage(tr("Selftest FAILED: world %1").arg(worldName));
		fprintf(stderr, "[selftest] CREATE FAILED\n");
		fflush(stderr);
		QCoreApplication::exit(1);
	}
}

// --- File menu ------------------------------------------------------------
// The original's handlers are in SurMap5/MainFrame.cpp (OnFile*). Those that
// need plumbing beyond the current phase log and show a status message; the
// world ops (save, save as) are wired through RenderViewWidget (U7).

void MainWindow::fileSave()
{
	// OnFileSave (SurMap5/MainFrame.cpp:1057): vMap.save(getWorldName) — when
	// the world had no name yet the original fell through to Save As; the Qt
	// editor's load/create always names the world, so the direct path is
	// enough.
	if(!view_->worldLoaded()){
		statusBar()->showMessage(tr("No world to save"));
		return;
	}
	statusBar()->showMessage(tr("Saving world: %1...").arg(view_->worldName()));
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const bool ok = view_->saveWorld();
	QApplication::restoreOverrideCursor();
	if(ok)
		statusBar()->showMessage(tr("World saved: %1").arg(view_->worldName()), 3000);
	else
		statusBar()->showMessage(tr("Could not save the world"));
}

void MainWindow::fileSaveAs()
{
	// OnFileSaveas (SurMap5/MainFrame.cpp:1165): CDlgSelectWorld over
	// vMap.getWorldsDir() with the New-world button — picking an existing
	// world overwrites it, New asks for a fresh name. Then save + retitle.
	if(!view_->worldLoaded()){
		statusBar()->showMessage(tr("No world to save"));
		return;
	}
	const QString worldsDir = EditorApplication::instance()->worldsDir();
	SelectWorldDialog dlg(worldsDir, tr("Save world as"), /*enableCreateDir=*/true, this);
	if(dlg.exec() != QDialog::Accepted)
		return;
	const QString worldName = dlg.selectedWorld();
	if(worldName.isEmpty())
		return;

	statusBar()->showMessage(tr("Saving world as: %1...").arg(worldName));
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const bool ok = view_->saveWorld(worldName);
	QApplication::restoreOverrideCursor();
	if(!ok){
		statusBar()->showMessage(tr("Could not save the world as %1").arg(worldName));
		return;
	}
	QSettings().setValue("mainWindow/lastWorld", worldName);
	updateWorldTitle();
	statusBar()->showMessage(tr("World saved: %1").arg(worldName), 3000);
}

void MainWindow::fileRunWorld()
{
	// OnFileRunWorld: launch the game with the current world. Needs the game
	// executable path; stub for now.
	fprintf(stderr, "[file] run-world: TODO\n");
	statusBar()->showMessage(tr("Run World: not wired yet"));
}

void MainWindow::fileRunMenu()
{
	fprintf(stderr, "[file] run-menu: TODO\n");
	statusBar()->showMessage(tr("Run Main Menu: not wired yet"));
}

void MainWindow::fileExportVistaEngine()
{
	fprintf(stderr, "[file] export-vistaengine: TODO\n");
	statusBar()->showMessage(tr("Export VistaEngine: not wired yet"));
}

void MainWindow::fileImportTextFromExcel()
{
	fprintf(stderr, "[file] import-excel: TODO\n");
	statusBar()->showMessage(tr("Import Text from Excel: not wired yet"));
}

void MainWindow::fileExportTextToExcel()
{
	fprintf(stderr, "[file] export-excel: TODO\n");
	statusBar()->showMessage(tr("Export Text to Excel: not wired yet"));
}

void MainWindow::fileExImWorld()
{
	// OnFileExportImportWorld: DlgExImWorld over vMap.getWorldsDir().
	if(!view_->worldLoaded()){
		statusBar()->showMessage(tr("Load a world first"));
		return;
	}
	ExImWorldDialog dlg(EditorApplication::instance()->worldsDir(), QString(), this);
	dlg.exec();
}

void MainWindow::fileProperties()
{
	// OnFileProperties: WorldPropertiesDlg — the loaded map's size + creation
	// parameters.
	int hSize = 0, vSize = 0;
	int hPower = 0, vPower = 0, method = 0, initialHeight = 0;
	if(!view_->mapSize(hSize, vSize) ||
	   !view_->mapCreationParams(hPower, vPower, method, initialHeight)){
		statusBar()->showMessage(tr("Load a world first"));
		return;
	}
	WorldPropertiesDialog dlg(hSize, vSize, hPower, vPower, method, initialHeight, this);
	dlg.exec();
}

void MainWindow::fileStatistics()
{
	// OnFileStatistics: ShowGraphicsStatistic in the original; the texture
	// statistics dialog (DlgTexturesStatistics) shows the loaded texture
	// library. The engine side is available even without a world (the library
	// is a render-device global).
	QVector<RenderViewWidget::TextureStat> rows;
	int totalSize = 0;
	view_->textureStatistics(rows, totalSize);

	QVector<QPair<QString, int>> pairs;
	pairs.reserve(rows.size());
	for(const auto& row : rows)
		pairs.append(qMakePair(row.name, row.size));

	TexturesStatisticsDialog dlg(pairs, totalSize, this);
	dlg.exec();
}

void MainWindow::fileResaveWorlds()
{
	// OnFileResaveWorlds: re-save every world in the worlds dir.
	fprintf(stderr, "[file] resave-worlds: TODO\n");
	statusBar()->showMessage(tr("Resave All Worlds: not wired yet"));
}

void MainWindow::fileMerge()
{
	// OnFileMerge: merge another world into the current one.
	fprintf(stderr, "[file] merge: TODO\n");
	statusBar()->showMessage(tr("Merge: not wired yet"));
}

void MainWindow::fileSaveMiniMapToWorld()
{
	// OnFileSaveminimaptoworld (SurMap5/MainFrame.cpp:1182): write the world's
	// map.tga (vMap.saveMiniMap(H_SIZE/16, V_SIZE/16)), then refresh the panel
	// so it shows the newly written image.
	if(!view_->worldLoaded()){
		statusBar()->showMessage(tr("No world is open"));
		return;
	}
	if(view_->saveMiniMapToFile()){
		statusBar()->showMessage(tr("Minimap saved"), 3000);
		if(miniMapPanel_)
			miniMapPanel_->reload();
	}
	else
		statusBar()->showMessage(tr("Could not save minimap"));
}

// --- Tools (U6) ------------------------------------------------------------

void MainWindow::brushRadiusChanged(int index)
{
	// OnBrushRadiusComboSelChanged (SurMap5/SToolBar.cpp): the combo holds the
	// ArrSize_Brush values in its item data; switching updates the current
	// brush radius the tools read (CSurToolBase::getBrushRadius).
	const QVariant data = brushRadiusCombo_ ? brushRadiusCombo_->itemData(index) : QVariant();
	brushRadius_ = data.isValid() ? data.toInt() : 1;
	fprintf(stderr, "[tools] brush radius: %d\n", brushRadius_);
	statusBar()->showMessage(tr("Brush radius: %1").arg(brushRadius_));
}

// --- Edit menu ------------------------------------------------------------

void MainWindow::editUndo()
{
	// OnEditUndo: selection undo (U8).
	fprintf(stderr, "[edit] undo: TODO U8\n");
	statusBar()->showMessage(tr("Undo: not wired yet"));
}

void MainWindow::editRedo()
{
	fprintf(stderr, "[edit] redo: TODO U8\n");
	statusBar()->showMessage(tr("Redo: not wired yet"));
}

void MainWindow::editMapScenario()
{
	// OnEditMap: the map scenario editor (external tools land in U2).
	fprintf(stderr, "[edit] map-scenario: TODO\n");
	statusBar()->showMessage(tr("Map Scenario: not wired yet"));
}

void MainWindow::editGameScenario()
{
	fprintf(stderr, "[edit] game-scenario: TODO\n");
	statusBar()->showMessage(tr("Game Scenario: not wired yet"));
}

void MainWindow::editSaveCameraAsDefault()
{
	// OnEditSaveCameraAsDefault (SurMap5/MainFrame.cpp:1977):
	// GlobalAttributes::setCameraCoordinate — the camera manager's distance +
	// theta became the defaults the next camera init used. The Qt editor's
	// orbit lives in EngineViewport; QSettings stands in for the global-
	// attributes file (applySavedCameraDefault restores it after a load).
	float distance = 0.f, theta = 0.f;
	view_->orbitCamera(distance, theta);
	QSettings().setValue("editor/cameraDefaultDistance", (double)distance);
	QSettings().setValue("editor/cameraDefaultTheta", (double)theta);
	fprintf(stderr, "[edit] save-camera-default: distance=%g theta=%g\n", distance, theta);
	statusBar()->showMessage(tr("Camera saved as default"), 3000);
}

void MainWindow::editRebuildWorld()
{
	// OnEditRebuildworld: rebuild terrain caches (vMap.rebuild?).
	fprintf(stderr, "[edit] rebuild-world: TODO\n");
	statusBar()->showMessage(tr("Rebuild World: not wired yet"));
}

void MainWindow::editUpdateSurface()
{
	fprintf(stderr, "[edit] update-surface: TODO\n");
	statusBar()->showMessage(tr("Update Surface: not wired yet"));
}

void MainWindow::editChangeTotalWorldHeight()
{
	// OnEditChangetotalworldheight: DlgChangeTotalWorldHeight — shift/scale the
	// whole terrain, then re-create the scene and re-init the world.
	if(!view_->worldLoaded()){
		statusBar()->showMessage(tr("Load a world first"));
		return;
	}

	int hist[256];
	int minVx = 0, maxVx = 0;
	if(!view_->worldHeightHistogram(hist, minVx, maxVx)){
		statusBar()->showMessage(tr("Could not read the terrain"));
		return;
	}

	int hPower = 0, vPower = 0, method = 0, initialHeight = 0;
	view_->mapCreationParams(hPower, vPower, method, initialHeight);

	ChangeTotalWorldHeightDialog dlg(minVx, maxVx, hist, initialHeight, this);
	if(dlg.exec() != QDialog::Accepted)
		return;

	// The engine call, as CMainFrame::OnEditChangetotalworldheight did:
	//   vMap.changeTotalWorldParam(deltaVx, scaleVx, m_changeParam)
	// then re-init the scene + world so the terrain buffers rebuild.
	// The creation params stay the world's own unless "resize to new borders"
	// was checked, in which case the dialog's slider values only affect the
	// delta/scale, not the map size (the original used the attrib editor to
	// change sizes; the Qt port keeps sizes fixed for now — TODO(U4): expose
	// the size-power editor).
	Editor::MapChangeParams params;
	params.hSizePower = hPower;
	params.vSizePower = vPower;
	params.createWorldMetod = method;
	params.initialHeight = (unsigned short)initialHeight;
	params.flag_resizeWorld2NewBorder = dlg.resizeWorld();

	statusBar()->showMessage(tr("Changing total world height..."));
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const float scaleVx = (float)dlg.scalePercent() / 100.f;
	view_->changeTotalWorldParam(dlg.deltaVx(), scaleVx, params);
	// The terrain buffers changed in place; rebuild the render-side map the
	// way CMainFrame called view_->reInitWorld() after the change.
	view_->reinitWorld();
	QApplication::restoreOverrideCursor();

	statusBar()->showMessage(tr("Total world height changed"), 3000);
}

void MainWindow::editRollingBorder()
{
	// OnEditRollingborder: DlgBorderRolling.
	fprintf(stderr, "[edit] rolling-border: TODO U5 (DlgBorderRolling)\n");
	statusBar()->showMessage(tr("Rolling Border: not wired yet"));
}

// --- View menu ------------------------------------------------------------

void MainWindow::viewToggleGrid(bool checked)
{
	// OnViewShowGrid (SurMap5/MainFrame.cpp:2276): surMapOptions.enableGrid_ =
	// !enableGrid_; CGeneralView::drawGrid (GeneralView.cpp:915) read that
	// flag. The Qt editor keeps it in EngineViewport (gridVisible_).
	view_->setGridVisible(checked);
	fprintf(stderr, "[view] show-grid: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Show Grid: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewToggleSources(bool checked)
{
	// OnViewSources: render the world's sources (extraction points).
	fprintf(stderr, "[view] sources: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Sources: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewToggleCameras(bool checked)
{
	// OnViewCameras: show/hide the camera paths on the map. The camera editor
	// dialog (CameraDlg) is the IDD_DLG_CAMERA bar dialog; the original toggled
	// the bar with this command too. Show it for now — cameraManager (the
	// actual spline data) is not wired in the Qt editor yet.
	if(checked){
		CameraDialog dlg(this);
		dlg.exec();
	}
	fprintf(stderr, "[view] cameras: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Cameras: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewToggleGeosurface(bool checked)
{
	fprintf(stderr, "[view] geosurface: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Geosurface: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewTogglePathFinding(bool checked)
{
	fprintf(stderr, "[view] path-finding: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Path Finding: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewToggleCameraBorders(bool checked)
{
	fprintf(stderr, "[view] camera-borders: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Show Camera Borders: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewToggleTimeFlow(bool checked)
{
	// OnViewEnableTimeFlow: let the world's time advance (filters bar toggle).
	// The Qt editor has no Environment yet, so the toggle only records the
	// state (the time-slider dialog reads it to enable/disable its controls).
	timeFlowEnabled_ = checked;
	fprintf(stderr, "[view] time-flow: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Enable Time Flow: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::viewTimeSlider()
{
	// ID_VIEW_TIME_SLIDER: the time-of-day slider (TimeSliderDlg). The Qt
	// editor keeps the clock in the dialog (no Environment yet); the dialog
	// is modeless-hosted like the original's filters-bar child.
	TimeSliderDialog dlg(editorTime_, this);
	dlg.setTimeFlowEnabled(timeFlowEnabled_);
	if(dlg.exec() == QDialog::Accepted)
		editorTime_ = dlg.time();
	else
		editorTime_ = dlg.time();   // keep the last value either way
}

void MainWindow::viewToggleHideModels(bool checked)
{
	// OnViewHideModels: draw the terrain but not the 3D models.
	fprintf(stderr, "[view] hide-models: %s\n", checked ? "on" : "off");
	statusBar()->showMessage(tr("Hide Models: %1").arg(checked ? tr("on") : tr("off")));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	// Phase 7: persist the dock/toolbar layout (saveDlgBarState: CExtControlBar::
	// ProfileBarStateSerialize -> surMapOptions.dlgBarState). Qt's equivalent is
	// QMainWindow::saveState(), restored in the ctor via QSettings.
	QSettings settings;
	settings.setValue("mainWindow/state", saveState());
	settings.setValue("mainWindow/geometry", saveGeometry());
	QMainWindow::closeEvent(event);
}
