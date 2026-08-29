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
class QComboBox;
class QDockWidget;
class QProgressBar;
class QTimer;
class QToolBar;
class RenderViewWidget;
class ToolsTreePanel;
class ObjectsTreePanel;
class MiniMapPanel;
class GradientsPanel;

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
	void openWorld();
	void newWorld();
	// Switch the active editor tool (0=Select, 1=Move, 2=Rotate, 3=Scale).
	void selectTool(int index);
	// Temporary selftest hook (Phase 3b verification): create + load a world
	// by name without the dialog. Removed when the world dialog lands.
	void selftestCreateWorld(const QString& worldName);

	// --- File menu (CMainFrame::OnFile*) ---
	void fileSave();
	void fileSaveAs();
	void fileRunWorld();
	void fileRunMenu();
	void fileExportVistaEngine();
	void fileImportTextFromExcel();
	void fileExportTextToExcel();
	void fileExImWorld();
	void fileProperties();
	void fileStatistics();
	void fileResaveWorlds();
	void fileMerge();

	// --- Edit menu (CMainFrame::OnEdit*) ---
	void editUndo();
	void editRedo();
	void editMapScenario();
	void editGameScenario();
	void editSaveCameraAsDefault();
	void editRebuildWorld();
	void editUpdateSurface();
	void editChangeTotalWorldHeight();
	void editRollingBorder();

	// --- View menu (CMainFrame::OnView*) ---
	void viewToggleGrid(bool checked);
	void viewToggleSources(bool checked);
	void viewToggleCameras(bool checked);
	void viewToggleGeosurface(bool checked);
	void viewTogglePathFinding(bool checked);
	void viewToggleCameraBorders(bool checked);
	void viewToggleTimeFlow(bool checked);
	void viewToggleHideModels(bool checked);
	// ID_VIEW_TIME_SLIDER — the time-of-day slider dialog (TimeSliderDlg).
	void viewTimeSlider();

	// --- U6 panels ---
	// The brush-radius combo (ID_BRUSH_COMBO_PLACE) changed value.
	void brushRadiusChanged(int index);
	// Save MiniMap to World (ID_FILE_SAVEMINIMAPTOWORLD).
	void fileSaveMiniMapToWorld();

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

	// --- editor clock (TimeSliderDlg state; the Qt editor has no Environment) ---
	float editorTime_ = 12.f;        // time of day in hours
	bool timeFlowEnabled_ = false;   // Environment::flag_EnableTimeFlow

	// --- panels (QDockWidget equivalents of the CExtControlBar members) ---
	QDockWidget* toolsDock_ = nullptr;     // toolsWindowBar_
	QDockWidget* objectsDock_ = nullptr;   // objectsManagerBar_
	QDockWidget* propertiesDock_ = nullptr;// propertiesBar_
	QDockWidget* miniMapDock_ = nullptr;   // miniMapBar_
	QDockWidget* gradientsDock_ = nullptr; // gradients bar (new; see GradientsPanel)
	ToolsTreePanel* toolsTreePanel_ = nullptr;   // CToolsTreeWindow's tree
	ObjectsTreePanel* objectsTreePanel_ = nullptr; // CObjectsManagerWindow
	MiniMapPanel* miniMapPanel_ = nullptr;   // CMiniMapWindow (U6)
	GradientsPanel* gradientsPanel_ = nullptr; // CGradientsWindow (U6)

	// --- toolbars (CExtToolControlBar set) ---
	QToolBar* mainToolBar_ = nullptr;      // IDR_MAINFRAME
	QToolBar* toolsToolBar_ = nullptr;     // IDR_TOOLS_BAR
	QToolBar* filtersToolBar_ = nullptr;   // IDR_FILTERS_BAR
	QToolBar* librariesToolBar_ = nullptr; // IDR_LIBRARIES_BAR
	QToolBar* editorsToolBar_ = nullptr;   // IDR_EDITORS_BAR

	// The brush-radius combo on the tools toolbar (ID_BRUSH_COMBO_PLACE in
	// CToolsTreeWindow). Holds the ArrSize_Brush list {1,3,5,...}.
	QComboBox* brushRadiusCombo_ = nullptr;

	// The current brush radius in world units (the combo's value).
	int brushRadius_ = 1;

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

	// --- File menu actions (ID_FILE_*) ---
	QAction* actRunWorld_ = nullptr;
	QAction* actRunMenu_ = nullptr;
	QAction* actExportVistaEngine_ = nullptr;
	QAction* actImportTextExcel_ = nullptr;
	QAction* actExportTextExcel_ = nullptr;
	QAction* actExImWorld_ = nullptr;
	QAction* actProperties_ = nullptr;
	QAction* actStatistics_ = nullptr;
	QAction* actResaveWorlds_ = nullptr;
	QAction* actMerge_ = nullptr;
	QAction* actSaveMiniMapToWorld_ = nullptr;   // ID_FILE_SAVEMINIMAPTOWORLD

	// --- Edit menu actions (ID_EDIT_*) ---
	QAction* actUndo_ = nullptr;
	QAction* actRedo_ = nullptr;
	QAction* actMapScenario_ = nullptr;
	QAction* actGameScenario_ = nullptr;
	QAction* actSaveCameraAsDefault_ = nullptr;
	QAction* actRebuildWorld_ = nullptr;
	QAction* actUpdateSurface_ = nullptr;
	QAction* actChangeTotalWorldHeight_ = nullptr;
	QAction* actRollingBorder_ = nullptr;

	// --- View menu actions (ID_VIEW_*) ---
	QAction* actViewSources_ = nullptr;
	QAction* actViewCameras_ = nullptr;
	QAction* actViewGeosurface_ = nullptr;
	QAction* actViewPathFinding_ = nullptr;
	QAction* actViewPathFindingRef_ = nullptr;
	QAction* actViewShowGrid_ = nullptr;
	QAction* actViewCameraBorders_ = nullptr;
	QAction* actViewTimeFlow_ = nullptr;
	QAction* actViewTimeSlider_ = nullptr;   // ID_VIEW_TIME_SLIDER (TimeSliderDlg)
	QAction* actViewHideModels_ = nullptr;
	QAction* actViewObjectsManager_ = nullptr;

	// --- Libraries menu (ID_EDIT_*/ID_LIBRARIES_*) ---
	QAction* actLibUnits_ = nullptr;
	QAction* actLibEffects_ = nullptr;
	QAction* actLibSounds_ = nullptr;
	QAction* actLibUIMessageTypes_ = nullptr;
	QAction* actLibUIMessages_ = nullptr;
	QAction* actLibUIShowModeSprites_ = nullptr;
	QAction* actLibSoundTracks_ = nullptr;
	QAction* actLibReels_ = nullptr;
	QAction* actLibHeads_ = nullptr;
	QAction* actLibTerTools_ = nullptr;
	QAction* actLibCursors_ = nullptr;
	QAction* actLibCommandColors_ = nullptr;
	QAction* actLibTextImages_ = nullptr;
	QAction* actLibTerrainTypeName_ = nullptr;
	QAction* actLibImportParametersFull_ = nullptr;
	QAction* actLibImportParametersByGroups_ = nullptr;
	QAction* actLibExportParametersFull_ = nullptr;
	QAction* actLibExportParametersByGroups_ = nullptr;
	QAction* actLibExportParametersStatistics_ = nullptr;

	// --- Tools menu (ID_EDIT_*) ---
	QAction* actToolUIEditor_ = nullptr;
	QAction* actToolEffectsEditor_ = nullptr;
	QAction* actToolTriggers_ = nullptr;

	// --- Workspace menu (ID_VIEW_*) ---
	QAction* actWsReset_ = nullptr;
	QAction* actWsMenuBar_ = nullptr;
	QAction* actWsMainToolbar_ = nullptr;
	QAction* actWsFiltersBar_ = nullptr;
	QAction* actWsLibrariesBar_ = nullptr;
	QAction* actWsEditorsBar_ = nullptr;
	QAction* actWsStatusBar_ = nullptr;
	QAction* actWsTools_ = nullptr;
	QAction* actWsProperties_ = nullptr;
	QAction* actWsMinimap_ = nullptr;
	QAction* actWsObjectsManager_ = nullptr;

	// --- Debug menu (ID_DEBUG_*) ---
	QAction* actDbgEditableTree_ = nullptr;
	QAction* actDbgSaveTree_ = nullptr;
	QAction* actDbgEditZipConfig_ = nullptr;
	QAction* actDbgEditDebugPrm_ = nullptr;
	QAction* actDbgShowPaletteTexture_ = nullptr;
	QAction* actDbgShowMipmap_ = nullptr;

	// --- tool actions (the tools tree's transform set) ---
	QAction* actToolSelect_ = nullptr;
	QAction* actToolMove_ = nullptr;
	QAction* actToolRotate_ = nullptr;
	QAction* actToolScale_ = nullptr;
};
