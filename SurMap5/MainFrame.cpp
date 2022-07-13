#include "StdAfx.h"
#include "SurMap5.h"

#include <string>

#include "MainFrame.h"
#include "GeneralView.h"

#include "Serialization\Serializer.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization/Dictionary.h"
#include "kdw/PropertyEditor.h"
#include "kdw/TreeSelector.h"
#include "kdw/ReferenceTreeBuilder.h"
#include "kdw/HotkeyDialog.h"
#include "kdw/LibraryEditorDialog.h"

#include "WaveDlg.h"
#include "TimeSliderDlg.h"
#include "ToolsTreeWindow.h"
#include "ObjectsManagerWindow.h"
#include "MiniMapWindow.h"

#include "DlgSelectWorld.h"
#include "DlgExImWorld.h"
#include "DlgChangeTotalWorldHeight.h"
#include "DlgBorderRolling.h"

#include "WorldPropertiesDlg.h"
#include "SurToolSelect.h"
#include "ToolsTreeCtrl.h"
#include "SurMapOptions.h"
#include "Serialization\GenericFileSelector.h"
#include "Serialization\SerializationFactory.h"
#include "Render\src\TileMap.h"
#include "Render\Src\VisGeneric.h"

#include "ExtCmdManager.h"

#include "SurToolAux.h"
#include "Triggers.h"
#include "DlgSelectTrigger.h"

#include "SelectionUtil.h"
#include "SystemUtil.h"

#include "Game\RenderObjects.h"
#include "Game\Universe.h"
#include "Game\CameraManager.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "AttributeReference.h"
#include "AttributeSquad.h"
#include "Game\IniFile.h"
#include "Render\3dx\Lib3dx.h"
#include "Render\Src\TexLibrary.h"

#include "version.h"

#include "UserInterface\UserInterface.h"
#include "Game\GameOptions.h"
#include "Render\Src\Scene.h"

#include "Terra\vmap.inl"
#include "Terra\vmap4vi.h"

#include "TextDB.h"
#include "UnicodeConverter.h"
#include "GlobalAttributes.h"

# include "ExcelImEx.h"
# include "ExcelExport\ExcelExporter.h"
# include "ParameterImportExportExcel.h"
# include "ParameterStatisticsExport.h"
# include "ParameterTree.h"

#include "Console.h"
#include "ZipConfig.h"
#include "OutputProgressDlg.h"

#include "Game\StreamCommand.h"

#include "Game\MergeOptions.h"

#include "Serialization\XPrmArchive.h"
#include "FileUtils\FileUtils.h"
#include "Terra\qsWorldsMgr.h"

#include "Terra\TerrainType.h"
#include <CrtDbg.h>
#include "VistaEditor\CommandEditor.h"
#include "VistaEditor\FormationEditor.h"
#include "TriggerEditor\TriggerEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()

    // menus
	ON_COMMAND_EX(ID_VIEW_MENUBAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(ID_VIEW_MENUBAR, OnUpdateControlBarMenu)
	
	ON_COMMAND(IDR_MAINFRAME, OnViewToolbar )
	ON_UPDATE_COMMAND_UI(IDR_MAINFRAME, OnUpdateViewToolbar)
	ON_COMMAND_EX(IDR_EDITORS_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(IDR_EDITORS_BAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(IDR_LIBRARIES_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(IDR_LIBRARIES_BAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(IDR_FILTERS_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(IDR_FILTERS_BAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(IDR_EDITORS_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(IDR_EDITORS_BAR, OnUpdateControlBarMenu)

	ON_UPDATE_COMMAND_UI(IDD_TIME_SLIDER, OnUpdateTimeSlider)
	
	ON_COMMAND(ID_VIEW_SHOW_CAMERA_BORDERS, OnViewShowCameraBorders)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_CAMERA_BORDERS, OnUpdateViewShowCameraBorders)

	ON_UPDATE_COMMAND_UI(ID_VIEW_HIDE_MODELS, OnUpdateViewHideModels)
	ON_COMMAND(ID_VIEW_HIDE_MODELS, OnViewHideModels)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAMERAS, OnUpdateViewCameras)
	ON_COMMAND(ID_VIEW_CAMERAS, OnViewCameras)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PATH_FINDING, OnUpdateViewPathFinding)
	ON_COMMAND(ID_VIEW_PATH_FINDING, OnViewPathFinding)
	ON_COMMAND(ID_VIEW_PATH_FINDING_SELECT_REFERENCE, OnViewPathFindingReferenceUnit)

	ON_WM_ACTIVATEAPP()

    // File Menu
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateIsWorldLoaded)

	ON_COMMAND(ID_FILE_SAVEAS, OnFileSaveas)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEAS, OnUpdateIsWorldLoaded)
	ON_COMMAND(ID_FILE_SAVEMINIMAPTOWORLD, OnFileSaveminimaptoworld)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEMINIMAPTOWORLD, OnUpdateIsWorldLoaded)

    ON_COMMAND(ID_FILE_MERGE, OnFileMerge)
	ON_UPDATE_COMMAND_UI(ID_FILE_MERGE, OnUpdateIsWorldLoaded)

	ON_COMMAND(ID_FILE_SAVEVOICEFILEDURATIONS, OnFileSaveVoiceFileDurations)
	ON_COMMAND(ID_FILE_RESAVE_WORLDS, OnFileResaveWorlds)
	ON_COMMAND(ID_FILE_RESAVE_TRIGGERS, OnFileResaveTriggers)

	ON_COMMAND(ID_FILE_RUN_WORLD, OnFileRunWorld)
	ON_COMMAND(ID_FILE_RUN_MENU, OnFileRunMenu)
	ON_COMMAND(ID_FILE_EXPORTVISTAENGINE, OnFileExportVistaEngine)

	ON_UPDATE_COMMAND_UI(ID_FILE_RUN_MENU, OnUpdateIsWorldLoaded)
	ON_UPDATE_COMMAND_UI(ID_FILE_RUN_WORLD, OnUpdateIsWorldLoaded)

	ON_COMMAND(ID_FILE_EXIMWORLD, OnFileExportImportWorld)
	ON_COMMAND(ID_FILE_IMPORTTEXTFROMEXCEL, OnFileImportTextFromExcel)
	ON_COMMAND(ID_FILE_EXPORTTEXTTOEXCEL, OnFileExportTextToExcel)

	ON_COMMAND(ID_FILE_PARAMETERS_IMPORT_FULL, OnFileParametersImportFull)
	ON_COMMAND(ID_FILE_PARAMETERS_IMPORT_BY_GROUPS, OnFileParametersImportByGroups)
	ON_COMMAND(ID_FILE_PARAMETERS_IMPORT_UNITS, OnFileParametersImportUnits)

	ON_COMMAND(ID_FILE_PARAMETERS_EXPORT_FULL, OnFileParametersExportFull)
	ON_COMMAND(ID_FILE_PARAMETERS_EXPORT_BY_GROUPS, OnFileParametersExportByGroups)
	ON_COMMAND(ID_FILE_PARAMETERS_EXPORT_UNITS, OnFileParametersExportUnits)
	ON_COMMAND(ID_FILE_PARAMETERS_EXPORT_STATISTICS, OnFileParametersExportStatistics)

	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateIsWorldLoaded)
	ON_COMMAND(ID_FILE_STATISTICS, OnFileStatistics)
	ON_UPDATE_COMMAND_UI(ID_FILE_STATISTICS, OnUpdateIsWorldLoaded)

    // Edit Menu
	ON_COMMAND(ID_EDIT_SAVE_CAMERA_AS_DEFAULT, OnEditSaveCameraAsDefault)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SAVE_CAMERA_AS_DEFAULT, OnUpdateIsWorldLoaded)

	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_PLAYPMO, OnEditPlaypmo)
	ON_COMMAND(ID_EDIT_MAPSCENARIO, OnEditMap)
	ON_COMMAND(ID_EDIT_MAPPRESET, OnEditPreset)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MAPSCENARIO, OnUpdateIsWorldLoaded)
	ON_COMMAND(ID_EDIT_GAME_SCENARIO, OnEditGameScenario)
	ON_COMMAND(ID_EDIT_REBUILDWORLD, OnEditRebuildworld)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE_SURFACE, OnUpdateIsWorldLoaded)
	ON_COMMAND(ID_EDIT_UPDATE_SURFACE, OnEditUpdateSurface)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REBUILDWORLD, OnUpdateIsWorldLoaded)
	ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)

    // View Menu
	ON_COMMAND(ID_VIEW_SOURCES, OnViewSources)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SOURCES, OnUpdateViewSources)
	ON_COMMAND(ID_VIEW_SHOW_GRID, OnViewShowGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_GRID, OnUpdateViewShowGrid)
	ON_COMMAND(ID_VIEW_ENABLE_TIME_FLOW, OnViewEnableTimeFlow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENABLE_TIME_FLOW, OnUpdateViewEnableTimeFlow)
	ON_COMMAND(ID_VIEW_GEOSURFACE, OnViewGeosurface)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GEOSURFACE, OnUpdateViewGeosurface)
	ON_UPDATE_COMMAND_UI(ID_EDIT_OBJECTS, OnUpdateIsWorldLoaded)
    // Libraries Menu
	ON_COMMAND(ID_EDIT_EFFECTS, OnEditEffects)
	ON_COMMAND(ID_EDIT_TOOLZERS, OnEditToolzers)
	ON_COMMAND(ID_EDIT_SOUNDS, OnEditSounds)
	
	ON_COMMAND(ID_LIBRARIES_UI_MESSAGE_TYPE, OnLibrariesUIMessageTypes)
	ON_COMMAND(ID_LIBRARIES_UI_MESSAGES, OnLibrariesUIMessages)
	ON_COMMAND(ID_LIBRARIES_UI_SHOW_MODE_SPRITES, OnLibrariesUIShowModeSprites)

	ON_COMMAND(ID_EDIT_SOUND_TRACKS, OnEditSoundTracks)
	ON_COMMAND(ID_EDIT_REELS, OnEditReels)
	ON_COMMAND(ID_EDIT_TERTOOLS, OnEditTertools)
	ON_COMMAND(ID_EDIT_CURSORS, OnEditCursors)
	ON_COMMAND(ID_EDIT_HEADS, OnEditHeads)

    // Editors Menu
	ON_COMMAND(ID_EDIT_TRIGGERS, OnEditTriggers)
	ON_COMMAND(ID_EDIT_UNITS, OnEditUnits)
	ON_COMMAND(ID_EDIT_USER_INTERFACE, OnEditUserInterface)
	ON_COMMAND(ID_EDIT_OBJECTS, OnEditObjects)
	ON_COMMAND(ID_EDIT_EFFECTS_EDITOR, OnEditEffectsEditor)

    // Workspace Menu
	ON_COMMAND(ID_VIEW_RESETTOOLBAR2DEFAULT, OnViewResettoolbar2default)
	ON_COMMAND(ID_VIEW_TREE_BAR, OnViewTreeBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TREE_BAR, OnUpdateViewTreeBar)
	ON_COMMAND(ID_VIEW_DLGBAR, OnViewProperties)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DLGBAR, OnUpdateViewProperties)
	ON_COMMAND(ID_VIEW_MINIMAP, OnViewMinimap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MINIMAP, OnUpdateViewMinimap)
	ON_COMMAND(ID_VIEW_OBJECTS_MANAGER, OnViewObjectsManager)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OBJECTS_MANAGER, OnUpdateViewObjectsManager)
	ON_COMMAND(ID_VIEW_SETTINGS, OnViewSettings)
    // Debug Menu
	ON_COMMAND(ID_VIEW_EXTENDEDMODETREELBAR, OnViewExtendedmodetreelbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EXTENDEDMODETREELBAR, OnUpdateViewExtendedmodetreelbar)
	ON_COMMAND(ID_DEBUG_SAVECONFIG, OnDebugSaveconfig)
	ON_COMMAND(ID_DEBUG_SAVEDICTIONARY, OnDebugSaveDictionary)
	ON_COMMAND(ID_DEBUG_EDIT_ZIP_CONFIG, OnDebugEditZipConfig)
	ON_COMMAND(ID_DEBUG_EDIT_DEBUG_PRM, OnDebugEditDebugPrm)
	ON_COMMAND(ID_DEBUG_EDIT_AUX_ATTRIBUTE, OnDebugEditAuxAttribute)
	ON_COMMAND(ID_DEBUG_EDIT_RIGID_BODY_PRM, OnDebugEditRigidBodyPrm)
	ON_COMMAND(ID_DEBUG_EDIT_TOOLZER, OnDebugEditToolzer)
	ON_COMMAND(ID_DEBUG_EDIT_EXPLODE_TABLE, OnDebugEditExplodeTable)
	ON_COMMAND(ID_DEBUG_EDIT_SOURCES_LIBRARY, OnDebugEditSourcesLibrary)
	ON_COMMAND(ID_DEBUG_SHOWMIPMAP, OnDebugShowmipmap)
	ON_COMMAND(ID_DEBUG_UI_SPRITE_LIB, OnDebugUISpriteLib)
	ON_COMMAND(ID_TEXT_SPRITES, OnEditUITextSprites)
	ON_COMMAND(ID_LIBRARIES_COMMANDCOLORS, OnEditCommandColor)

	ON_COMMAND(ID_DEBUG_SHOWPALETTETEXTURE, OnDebugShowpalettetexture)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_SHOWPALETTETEXTURE, OnUpdateDebugShowpalettetexture)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_SHOWMIPMAP, OnUpdateDebugShowmipmap)
	ON_COMMAND(ID_EDIT_CHANGETOTALWORLDHEIGHT, OnEditChangetotalworldheight)

	// Help menu
	ON_COMMAND(ID_HELP_KEY_INFO, OnHelpKeyInfo)
    //////////////////////////////////////////////////////////////////////////////
	ON_COMMAND(ID_IMPORT_PARAMETERS, OnImportParameters)
	ON_COMMAND(ID_EXPORT_PARAMETERS, OnExportParameters)
	ON_COMMAND(ID_FILE_SAVEWITHOUTTERTOOLCOLOR, OnFileSavewithouttertoolcolor)
	ON_COMMAND(ID_FILE_UPDATE_QUICK_START_WORLDS_LIST, OnFileUpdateQuickStartWorldsList)
	ON_COMMAND(ID_LIBRARIES_TERRRAINTYPENAME, OnLibrariesTerrraintypename)
	ON_COMMAND(ID_EDIT_ROLLINGBORDER, OnEditRollingborder)
	END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	view_ = new CGeneralView();
	miniMap_ = new CMiniMapWindow(this);
	timeSliderDialog_ = new CTimeSliderDlg(this);
	objectsManager_ = new CObjectsManagerWindow(this);
	toolsWindow_ = new CToolsTreeWindow();
	waveDialog_ = new CWaveDlg();
	static XBuffer errorHeading;
	errorHeading.SetRadix(16);
	errorHeading < ("VistaEngine: " VISTA_ENGINE_VERSION " (" __DATE__ " " __TIME__ ")");
	ErrH.SetPrefix(errorHeading);

	::memset( &m_dataFrameWP, 0, sizeof(WINDOWPLACEMENT) );
	m_dataFrameWP.length = sizeof(WINDOWPLACEMENT);
	m_dataFrameWP.showCmd = SW_HIDE;

	view_->init(toolsWindow_);

	lastTime = 0;
	syncroTimer.set(1, logicTimePeriod, 300);
	syncroTimer.setSpeed(surMapOptions.evolutionSpeed);
	memset(&statisticsAttr,1,sizeof(statisticsAttr));

	if(check_command_line("export")){
		OnFileExportVistaEngine();
		ErrH.Exit();
	}
}

CMainFrame::~CMainFrame()
{
	delete waveDialog_;
	waveDialog_ = 0;
	delete miniMap_;
	miniMap_ = 0;
	delete view_;
	view_ = 0;
	delete objectsManager_;
	objectsManager_ = 0;
	delete toolsWindow_;
	toolsWindow_ = 0;
	delete waveDialog_;
	waveDialog_ = 0;
}

void CMainFrame::resetWorkspace()
{
	miniMapBar_.FloatControlBar(CPoint( 100, 100 ));
	objectsManagerBar_.FloatControlBar(CPoint( 100, 200 ) );
	propertiesBar_.FloatControlBar( CPoint( 100, 300 ) );
	toolsWindowBar_.FloatControlBar(CPoint( 100, 400 ) );
	
	menuBar_.FloatControlBar(CPoint(0, -30));
	toolBar_.FloatControlBar(CPoint(0, 0));
#ifndef _VISTA_ENGINE_EXTERNAL_
	librariesBar_.FloatControlBar(CPoint(100, 0));
	editorsBar_.FloatControlBar(CPoint(300, 0));
#endif
	filtersBar_.FloatControlBar(CPoint(400, 0));
	//timeSliderBar_.FloatControlBar(CPoint(500, 0));

	RecalcLayout();

	toolsWindowBar_.SetInitDesiredSizeVertical( CSize(300,600) );
	toolsWindowBar_.SetInitDesiredSizeHorizontal( CSize(300,600) );
	toolsWindowBar_.SetInitDesiredSizeFloating( CSize(300,600) );

	miniMapBar_.SetInitDesiredSizeVertical( CSize(300,300) );
	miniMapBar_.SetInitDesiredSizeHorizontal( CSize(300,300) );
	miniMapBar_.SetInitDesiredSizeFloating( CSize(300,300) );

	objectsManagerBar_.SetInitDesiredSizeVertical(CPoint( 300, 300 ) );
	objectsManagerBar_.SetInitDesiredSizeHorizontal(CPoint( 300, 300 ) );
	objectsManagerBar_.SetInitDesiredSizeFloating(CPoint( 300, 300 ) );

	miniMapBar_.DockControlBar(AFX_IDW_DOCKBAR_LEFT,1,this,false);
	objectsManagerBar_.DockControlBar(AFX_IDW_DOCKBAR_LEFT,1,this,false);
	propertiesBar_.DockControlBar(AFX_IDW_DOCKBAR_LEFT,1,this,false);
	toolsWindowBar_.DockControlBar(AFX_IDW_DOCKBAR_LEFT,1,this,false);
    //timeSliderBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP,1,this,false);

	RecalcLayout();

	toolBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
#ifndef _VISTA_ENGINE_EXTERNAL_
	librariesBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
	editorsBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
#endif
	filtersBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
	menuBar_.ToggleDocking();

	RecalcLayout();

	propertiesBar_.SetInitDesiredSizeVertical( CSize(300,100) );
	propertiesBar_.SetInitDesiredSizeHorizontal( CSize(300,100) );
	propertiesBar_.SetInitDesiredSizeFloating( CSize(300,100) );
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	const bool external = false;
#else
	const bool external = true;
#endif
	if( CFrameWnd::OnCreate(lpCreateStruct) == -1 )	{
		ASSERT( FALSE );
		return -1;
	}

	((CSurMap5App*)AfxGetApp())->SetupUiAdvancedOptions(this);

	if (!view_->Create(0, 0, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL)){
		AfxMessageBox ("Failed to create view window\n");
		return -1;
	}

	if (!menuBar_.Create("Menu Bar", this, ID_VIEW_MENUBAR) || !reloadMenu()){
		AfxMessageBox ("Failed to create view window\n");
		return -1;
	}
	menuBar_.EnableDocking(CBRS_ALIGN_ANY);


	VERIFY(toolBar_.Create("Main Toolbar", this, AFX_IDW_TOOLBAR) && toolBar_.LoadToolBar(toolBarID(), RGB(255, 0, 255)));
	
	toolBar_.EnableDocking(CBRS_ALIGN_ANY);

    // VERIFY(timeSliderBar_.Create("Time Panel", this, ID_VIEW_TIME_SLIDER,
    //                              WS_CHILD | WS_VISIBLE | CBRS_RIGHT
    //                              | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY
    //                              | CBRS_SIZE_DYNAMIC | CBRS_HIDE_INPLACE));

    // VERIFY(timeSliderDialog_.Create(IDD_TIME_SLIDER, &timeSliderBar_));
	// timeSliderBar_.EnableDocking(CBRS_ALIGN_ANY);

#ifndef _VISTA_ENGINE_EXTERNAL_
	VERIFY(editorsBar_.Create("Editors Bar", this, IDR_EDITORS_BAR)
		&& editorsBar_.LoadToolBar(external ? IDR_EDITORS_EXTERNAL_BAR : IDR_EDITORS_BAR, RGB(255, 0, 255)));
	editorsBar_.EnableDocking(CBRS_ALIGN_ANY);
	
	VERIFY(librariesBar_.Create("Libraries Bar", this, IDR_LIBRARIES_BAR)
		&& librariesBar_.LoadToolBar(external ? IDR_LIBRARIES_EXTERNAL_BAR : IDR_LIBRARIES_BAR, RGB(255, 0, 255)));
	librariesBar_.EnableDocking(CBRS_ALIGN_ANY);
#endif

	VERIFY(filtersBar_.Create("Filters Bar", this, IDR_FILTERS_BAR)
		&& filtersBar_.LoadToolBar(external ? IDR_FILTERS_EXTERNAL_BAR : IDR_FILTERS_BAR, RGB(255, 0, 255)));
	       filtersBar_.EnableDocking(CBRS_ALIGN_ANY);

    //////////////////////////////////////////////////////////////////////////////
    VERIFY(timeSliderDialog_->Create(IDD_TIME_SLIDER, &filtersBar_));
	timeSliderDialog_->SetDlgCtrlID(IDD_TIME_SLIDER);

	int index = filtersBar_.CommandToIndex(IDD_TIME_SLIDER);
	xassert(index >= 0 && index < filtersBar_.GetButtonsCount());
    filtersBar_.SetButtonCtrl(index, timeSliderDialog_, false);
    //////////////////////////////////////////////////////////////////////////////

	if (!statusBar_.Create(this)) {
		AfxMessageBox ("Failed to create status bar\n");
		return -1;
	}
	for(int i = 0; i < NUMBERS_PARTS_STATUSBAR; ++i){
		statusBar_.AddPane(IDS_PANE_TEXT + i, i);
		statusBar_.SetPaneStyle(i, SBPS_NORMAL);
		if(i == 2){
			progressBar_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 0, 0), &statusBar_, 0);
			statusBar_.SetPaneControl(&progressBar_, IDS_PANE_TEXT + i, false);
		}
	}
	
	if (!toolsWindowBar_.Create(NULL/*_T("Optional control bar caption")*/, this, ID_VIEW_TREE_BAR)) {
		AfxMessageBox ("Failed to create toolsWindowBar_\n");
		return -1;
	}
	toolsWindowBar_.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(toolsWindow_->Create(this, &toolsWindowBar_));
	toolsWindow_->setBrushRadius(surMapOptions.lastToolzerRadius);
    //////////////////////////////////////////////////////////////////////////////
	if (!miniMapBar_.Create (0, this, ID_VIEW_MINIMAP)) {
		AfxMessageBox ("Failed to create miniMapBar_\n");
		return -1;
	}

	if(!miniMap_->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect (0, 0, 100, 100), &miniMapBar_)){
		AfxMessageBox ("Failed to create miniMap_\n");
		return -1;
	}
	miniMapBar_.SetInitDesiredSizeVertical (CSize(256, 256));
	miniMapBar_.EnableDocking(CBRS_ALIGN_ANY);
	miniMapBar_.ShowWindow (SW_SHOW);

    
    if(!propertiesBar_.Create(NULL, this, ID_VIEW_DLGBAR)){
		AfxMessageBox("Failed to create propertiesBar_\n");
		return -1;
	}
	
	propertiesBar_.EnableDocking(CBRS_ALIGN_ANY);
    //////////////////////////////////////////////////////////////////////////////
	//objectsManager_.Create (CObjectsManagerDlg::IDD, 0);
	if(!objectsManagerBar_.Create(0, this, ID_VIEW_OBJECTS_MANAGER)){
		AfxMessageBox ("Failed to create objectsManagerBar_\n");
		return -1;
	}

	if(!objectsManager_->Create(WS_CHILD | WS_VISIBLE, CRect (0, 0, 100, 100), &objectsManagerBar_)) {
		AfxMessageBox("Failed to create objectsManagaer_\n");
		return -1;
	}
	objectsManagerBar_.SetInitDesiredSizeVertical(CSize(256, 256));
	objectsManagerBar_.EnableDocking(CBRS_ALIGN_ANY);
	objectsManagerBar_.ShowWindow(SW_SHOW);
    //////////////////////////////////////////////////////////////////////////////

	if( !CExtControlBar::FrameEnableDocking(this) ) {
		ASSERT( FALSE );
		return -1;
	}
	if( !loadDlgBarState() ){
		resetWorkspace();
	}

	waveDialog_->Create(CWaveDlg::IDD, 0);
	signalSelectionChanged().connect(this, &CMainFrame::onSelectionChanged);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	view_->SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
//	if (m_baseTreeDlg.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) 
//		return TRUE;
	// let the view have first crack at the command
	if (view_->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


BOOL CMainFrame::DestroyWindow() 
{
	VERIFY( saveDlgBarState() );

	surMapOptions.lastToolzerRadius = toolsWindow_->getBrushRadius();
	surMapOptions.save();

	return CFrameWnd::DestroyWindow();
}



void CMainFrame::ActivateFrame(int nCmdShow)
{
	if( m_dataFrameWP.showCmd != SW_HIDE )
	{
		SetWindowPlacement( &m_dataFrameWP );
		CFrameWnd::ActivateFrame( m_dataFrameWP.showCmd );
		m_dataFrameWP.showCmd = SW_HIDE;
		return;
	}
	CFrameWnd::ActivateFrame(nCmdShow);
}


void CMainFrame::put2TitleNameDirWorld(void)
{
	CString strAppName;
	strAppName.LoadString (IDR_MAINFRAME);
	SetWindowText (strAppName + " - " + vMap.getWorldName().c_str());
}

//////////////////////////////////////////////
//#include <Windows.h>
#include <shlobj.h>
#include ".\mainframe.h"

//#import "D:\WINNT\system32\Shell32.dll"
static TCHAR szCurSurmapWorldDir[MAX_PATH]= { 0 };
//szDir[0]=0; //szDir может указывать на каталог
int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData) 
{
	switch(uMsg) {
	case BFFM_INITIALIZED: {
		if(szCurSurmapWorldDir[0]==0){
			if (GetCurrentDirectory(sizeof(szCurSurmapWorldDir)/sizeof(TCHAR), szCurSurmapWorldDir)) {
			  // WParam is TRUE since you are passing a path.
			  // It would be FALSE if you were passing a pidl.
			  SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szCurSurmapWorldDir);
			}
		}
		else SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szCurSurmapWorldDir);
	   break;
	}
	case BFFM_SELCHANGED: {
	   // Set the status window to the currently selected path.
	   TCHAR szDir[MAX_PATH];
	   if (SHGetPathFromIDList((LPITEMIDLIST) lp , szDir)) {
		  SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
	   }
	   break;
	}
	default:
	   break;
	}
	return 0;
}

char* SelectWorkDirectory(const char * _coment)
{
	static char workingDirectory[MAX_PATH];
	workingDirectory[0]=0;
	char * returnValue=0;

	//if(CoInitialize(NULL) != NOERROR) return 0; ????
	LPMALLOC pMalloc;
	// Gets the Shell's default allocator 
	if (::SHGetMalloc(&pMalloc) == NOERROR)
	{
		BROWSEINFO bi;
		char pszBuffer[MAX_PATH];
		//_tcscpy(pszBuffer, m_Path2Film);
		LPITEMIDLIST pidl;
		// Get help on BROWSEINFO struct - it's got all the bit settings.
		bi.hwndOwner = NULL;//GetSafeHwnd();
		bi.pidlRoot = NULL;
		bi.pszDisplayName = pszBuffer;
		bi.lpszTitle = _coment;//_T("Select a Film Directory");
		bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;// | BIF_NEWDIALOGSTYLE;
		//bi.lpfn = NULL;
		bi.lpfn = BrowseCallbackProc;
		bi.lParam = 0;
		// This next call issues the dialog box.
		if ((pidl = ::SHBrowseForFolder(&bi)) != NULL) {
			if (::SHGetPathFromIDList(pidl, pszBuffer)) { 
			// At this point pszBuffer contains the selected path 
				//m_Path2Film=pszBuffer;
				strncpy(workingDirectory, pszBuffer, sizeof(workingDirectory));
				strncpy(szCurSurmapWorldDir, pszBuffer, sizeof(szCurSurmapWorldDir));
				returnValue=&workingDirectory[0];
//					DoingSomethingUseful(pszBuffer);
			}
			else returnValue=0;
			// Free the PIDL allocated by SHBrowseForFolder.
			pMalloc->Free(pidl);
		}
		else returnValue=0;
		// Release the shell's allocator.
		pMalloc->Release();
	}
	return returnValue;
}

static bool flag_active_app = true;

bool CMainFrame::universeQuant()
{
	if(flag_active_app && universe()){
		syncroTimer.next_frame();
		if(lastTime < syncroTimer()){
			Console::instance().quant();
			lastTime += logicTimePeriod;
			gb_VisGeneric->SetLogicQuant(universe()->quantCounter()+2);
			universe()->Quant();
			universe()->interpolationQuant();
			UI_Dispatcher::instance().logicQuant();

			logicFps.quant();
			timeSliderDialog_->quant();
			view_->quant();

			Console::instance().graphQuant();
		}
		
		view_->Invalidate(FALSE);
		if (miniMap_->IsWindowVisible())
			miniMap_->Invalidate(FALSE);
		return true;
	}
	return false;
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	CFrameWnd::OnActivateApp(bActive, dwThreadID);

	flag_active_app=bActive;
}


void CMainFrame::OnEditPlaypmo()
{
	vMap.playPMOperation();
	//vMap.FlipWorldH();
	//vMap.RotateWorldP90();
	//vMap.save(vMap.worldName.c_str(), 0);
	vMap.WorldRender();
}


struct MapSerializer {
	MissionDescription& mission_;
	PlayerDataVect& players_;
	TriggerChainNames& worldTriggers_;

	MapSerializer(MissionDescription& mission, PlayerDataVect& players, TriggerChainNames& worldTriggers) :
		mission_(mission),
		players_(players),
		worldTriggers_(worldTriggers)
	{}

	void serialize(Archive& ar) {
		ar.setFilter(SERIALIZE_PRESET_DATA);
		mission_.serialize(ar);
		if(universe())
			universe()->serialize(ar);

		static_cast<vrtMap&>(vMap).serializeParameters(ar);
		
		if(environment){
			ar.setFilter(SERIALIZE_WORLD_DATA);
			string presetName = environment->presetName();
			ar.serialize(*environment, "environment", "Визуальные параметры мира");
		}
			
		if(cameraManager)
			ar.serialize(*cameraManager, "cameraManager", "Настройки камеры");
		
		ar.serialize(players_, "players", "Игроки");
#ifdef _VISTA_ENGINE_EXTERNAL_
		ar.serialize(worldTriggers_, "worldTriggers", 0);
#else
		ar.serialize(worldTriggers_, "worldTriggers", "Триггера для мира");
#endif
	}
};

struct PresetSerializer
{
	void serialize(Archive& ar)
	{
		if(environment){
			if(presetName_ == "")
				presetName_ = environment->presetName();
			
			ResourceSelector::Options presetOption("*.set", "Scripts\\Content\\Presets", "", false, false);
			ar.serialize(ResourceSelector(presetName_, presetOption), "presetName", "Имя файла настроек");

			ar.setFilter(SERIALIZE_PRESET_DATA);
			static XBuffer nameAlt;
			nameAlt.init();
			nameAlt < "Пресет " < environment->presetName();
			ar.serialize(*environment, "environmentPreset", nameAlt);

			if(ar.isInput() && presetName_ != environment->presetName()){
				environment->setPresetName(presetName_.c_str());
				environment->loadPreset();
			}
		}

	}

	string presetName_;
};

struct GameSerializer {

	void serialize(Archive& ar){
		GlobalAttributes::instance().serializeGameScenario(ar);
		GameOptions::instance().serializePresets(ar);
		UI_GlobalAttributes::instance().serialize(ar);
		if(ar.openBlock("Controls", "Управление")){
			ControlManager::instance().serialize(ar);
			ar.closeBlock();
		}
		if(environment){
			ar.setFilter(SERIALIZE_GLOBAL_DATA);
			environment->serialize(ar);
		}
	}
};

void CMainFrame::OnEditPreset()
{
	bool environmentCreated = false;
	if(!environment){
		for(DirIterator it("Resource\\Worlds\\*.*"); it; ++it)
			if(it.isDirectory()){
				view_->createScene();
				vMap.load(it.c_str(), true);
				view_->reInitWorld();
				environmentCreated = true;
				break;
			}
	}

	string presetName = environment->presetName();

	PresetSerializer presetSerializer;
	Serializer serializeable(presetSerializer, "presetSerializer", TRANSLATE("Пресет"));

	if(kdw::edit(serializeable, "Scripts\\TreeControlSetups\\EditPresetState", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		environment->savePreset();

	signalObjectChanged().emit(this);

	if(environmentCreated)
		view_->doneScene();
	else if(presetName != environment->presetName()){
		environment->setPresetName(presetName.c_str());
		environment->loadPreset();
	}

}

void CMainFrame::OnEditMap()
{
	if(!universe())
		return;

	PlayerDataVect players;
	universe()->exportPlayers(players);
	PlayerDataEdit worldPlayer;
	universe()->worldPlayer()->getPlayerData(worldPlayer);

	MapSerializer mapSerializer(view_->currentMission(), players, worldPlayer.triggerChainNames);

	Serializer serializeable(mapSerializer, "mapSerializer", TRANSLATE("Сценарий карты"));
	if(kdw::edit(serializeable, "Scripts\\TreeControlSetups\\EditMapState", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd())){
		GlobalAttributes::instance().saveLibrary();
		TextDB::instance().saveLanguage();
		OnFileSave();
	}

	signalObjectChanged().emit(this);

	if(universe()){
		universe()->importPlayers(players);
		universe()->worldPlayer()->setPlayerData(worldPlayer);
		setSilhouetteColors();
		toolsWindow_->rebuildTools();
	}
}

void CMainFrame::OnEditTriggers()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CDlgSelectTrigger dlgST("Scripts\\Content\\Triggers", TRANSLATE("Выбор триггеров"));
	int nRet=dlgST.DoModal();
	if(nRet==IDOK){
		if(!dlgST.selectTriggersFile.empty()){
			TriggerChain triggerChain;
			triggerChain.load(setExtention((string("Scripts\\Content\\Triggers\\") + transliterate(extractFileBase(dlgST.selectTriggersFile.c_str()).c_str())).c_str(), "scr").c_str());
			if(TriggerEditor(triggerChain, GetSafeHwnd()).edit()){
				triggerChain.save();
				TextDB::instance().saveLanguage();
			}
		}
		else {
			AfxMessageBox(TRANSLATE("Ошибка, файл не выбран!"));
		}
	}
#endif
}

void CMainFrame::OnDebugEditZipConfig()
{
	ZipConfigTable::instance().editLibrary();
}

void CMainFrame::editLibrary(const char* libraryName)
{
	_CrtCheckMemory();
#ifndef _VISTA_ENGINE_EXTERNAL_
	kdw::LibraryEditorDialog libraryEditor(GetSafeHwnd());

	const char* configFileName = "Scripts\\TreeControlSetups\\LibraryEditorState";
	XPrmIArchive ia;
	if(ia.open(configFileName)){
		ia.setFilter(kdw::SERIALIZE_STATE);
		ia.serialize(libraryEditor, "libraryEditor", 0);
		ia.close();
	}
    if(libraryEditor.showModal(libraryName) == kdw::RESPONSE_OK)
		saveAllLibraries();

	XPrmOArchive oa(configFileName);
	oa.setFilter(kdw::SERIALIZE_STATE);
	oa.serialize(libraryEditor, "libraryEditor", 0);
	oa.close();

	toolsWindow_->rebuildTools();
#else
	::MessageBox(0, "Warning: saveAllLibraries", "warning", MB_OK);
#endif
	_CrtCheckMemory();
}

void CMainFrame::OnEditUnits()
{
	editLibrary("AttributeLibrary");
}

/////////////////////////////////////////////////////
void CMainFrame::OnViewExtendedmodetreelbar()
{
	if(flag_TREE_BAR_EXTENED_MODE)
		flag_TREE_BAR_EXTENED_MODE=0;
	else
		flag_TREE_BAR_EXTENED_MODE=1;
	toolsWindow_->rebuildTools();
}

void CMainFrame::OnUpdateViewExtendedmodetreelbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(flag_TREE_BAR_EXTENED_MODE);
}

void CMainFrame::OnDebugSaveconfig()
{
	toolsWindow_->getTree().save();
	surMapOptions.save();
}

namespace {
bool fileExists(const char* fName)
{
	DWORD fa = GENERIC_READ;
	DWORD fs = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD fc = OPEN_EXISTING;
	DWORD ff = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	HANDLE hFile=CreateFile(fName, fa, fs, NULL, fc, ff, 0);
	if(hFile == INVALID_HANDLE_VALUE) return 0;
	else { CloseHandle(hFile); return 1; }
}

};

struct WorldCreationParam : vrtMapCreationParam
{
	string name;
	void serialize(Archive& ar){
		ar.serialize(name, "name", "Имя мира");
		__super::serialize(ar);
	}
};

void CMainFrame::OnFileNew()
{
	WorldCreationParam creationParam;
	if(kdw::edit(Serializer(creationParam), "Scripts\\TreeControlSetups\\WorldCreationParam", kdw::ONLY_TRANSLATED, GetSafeHwnd(), TRANSLATE("Создание нового мира"))){
		CWaitCursor wait;
		view_->createScene();
		static_cast<vrtMapCreationParam&>(vMap) = creationParam;

		string pathToWorldData = vMap.getWorldsDir();
		pathToWorldData += vMap.getWorldsDir();
		pathToWorldData += "\\";
		pathToWorldData += creationParam.name;
		pathToWorldData += "\\";
		pathToWorldData += vrtMap::worldDataFile;
			
		if(fileExists(pathToWorldData.c_str())){
			MessageBox(TRANSLATE("Мир с таким именем уже существует, создание невозможно!"), 0, MB_OK);
			return;
		}

		vMap.create(creationParam.name.c_str());
		view_->reInitWorld();

		Invalidate(FALSE);
		put2TitleNameDirWorld();
		toolsWindow_->rebuildTools();
	}
	Invalidate(FALSE);
}


void CMainFrame::OnFileOpen()
{
	CWaitCursor wait;

	CDlgSelectWorld dlgSW(vMap.getWorldsDir(), TRANSLATE("Выбор мира для загрузки"), false);
	int nRet=dlgSW.DoModal();
	if(nRet == IDOK){
		XBuffer worldDataPath;
		worldDataPath < vMap.getWorldsDir() < "\\" < dlgSW.selectWorldName.c_str() < "\\" < vrtMap::worldDataFile;

		if(!dlgSW.selectWorldName.empty() && testExistingFile(worldDataPath)){
			XBuffer cacheTgaPath;
			cacheTgaPath < vMap.getWorldsDir() < "\\" < dlgSW.selectWorldName.c_str() < "\\cache.tga";

			if(!testExistingFile(cacheTgaPath)){
				wait.Restore();
				MessageBox(TRANSLATE("Этот мир был эспортирован только для игры на нем, вы не можете его отредактировать!"), 0, MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			else{
				view_->createScene();
				vMap.load(dlgSW.selectWorldName.c_str(), true);
				view_->reInitWorld();
				GlobalAttributes::instance().loadLibrary();
				put2TitleNameDirWorld();
				Invalidate(FALSE);
				wait.Restore();
			}
		}
		else{
			wait.Restore();
			XBuffer str;
			str < TRANSLATE("Следующий мир поврежден: ");
			str < dlgSW.selectWorldName.c_str();
			MessageBox(str, 0, MB_OK | MB_ICONERROR);

		}
		eventMaster().signalObjectChanged().emit(this);
		toolsWindow_->rebuildTools();
	}
}

void CMainFrame::OnFileExportImportWorld()
{
	CWaitCursor wait;

	CDlgExImWorld dlgSW(vMap.getWorldsDir(), TRANSLATE("Экспорт/Импорт миров"), false);
	int nRet=dlgSW.DoModal();
}

void CMainFrame::OnFileSave()
{
	if(vMap.getWorldName().empty()){
		OnFileSaveas();
	}
	else{
		save(vMap.getWorldName().c_str());
	}
}

void CMainFrame::OnFileSaveVoiceFileDurations()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	UI_TextLibrary::instance().saveLibrary();
	VoiceAttribute::VoiceFile::saveVoiceFileDurations();
#endif
}

void CMainFrame::OnFileSavewithouttertoolcolor()
{
	//if(vMap.worldName.empty()){
	//	AfxMessageBox("No world!");
	//}
	//else{
	//	save(vMap.worldName.c_str(), false);
	//}
}


void CMainFrame::save(const char* worldName)
{
	CWaitCursor wait;
	getToolWindow().rebuildTools();
	int idxColorCnt=vMap.convertVMapTryColor2TerClr();
	vMap.save(worldName);
	if(view_->currentMission().saveMiniMap)
		vMap.saveMiniMap(vMap.H_SIZE/16, vMap.V_SIZE/16); //128, 128
	view_->currentMission().setByWorldName(worldName);
#ifndef _VISTA_ENGINE_EXTERNAL_
	GlobalAttributes::instance().saveLibrary();
#endif
	XPrmOArchive oa(view_->currentMission().saveName());
#ifdef _VISTA_ENGINE_EXTERNAL_
	view_->currentMission().setBattle(true);
	view_->currentMission().is_fog_of_war = true;
#endif
	universe()->universalSave(view_->currentMission(), false, oa);
#ifdef _VISTA_ENGINE_EXTERNAL_
	if(environment){
		environment->setFogEnabled(true);
		bool gotStartPoint = false;
		SourceManager::Anchors& anchors = sourceManager->anchors();
		SourceManager::Anchors::iterator it;
		FOR_EACH(anchors, it){
			if((*it)->type () == Anchor::START_LOCATION){
				gotStartPoint = true;
				break;
			}
		}
		if(!gotStartPoint){
			MessageBox(TRANSLATE("Ни для одного из игроков не установлена стартовая точка (Инструмент: Зоны\\Стартовая точка).\nСтартовая точка дает начальный комплект юнитов."), 0, MB_OK | MB_ICONINFORMATION);
		}
	}
#else
	//if(view_->currentMission().isBattle())
	//	qsWorldsMgr.updateQSWorld(view_->currentMission().saveName(), XGUID(view_->currentMission().missionGUID()));
#endif
	signalWorldChanged().emit(this);
}

void CMainFrame::onSelectionChanged(SelectionObserver* changer)
{
	if(CSurToolBase* currentToolzer = getToolWindow().currentTool())
		currentToolzer->onSelectionChanged();
}

MergeOptions::MergeOptions()
: mergePlayers(false),
mergeWorld(false),
mergeSourcesAndAnchors(false),
mergeCameras(false)
{}

void MergeOptions::serialize(Archive& ar)
{
    static GenericFileSelector::Options options("*.spg", "Resource\\Worlds", TRANSLATE("Слить миры..."), false);
    ar.serialize(GenericFileSelector(worldFile), "worldFile", "Путь к файлу мира (*.spg)");
    ar.serialize(mergePlayers, "mergePlayers", "Импортировать игроков");
    ar.serialize(mergeWorld, "mergeWorld", "Импортировать мир (декорации и объекты, установленные за мир)");
    ar.serialize(mergeSourcesAndAnchors, "mergeSourcesAndAnchors", "Импортировать источники и якоря");
    ar.serialize(mergeCameras, "mergeCameras", "Камеры");
}

void CMainFrame::OnFileMerge()
{
    MergeOptions options;
	if(kdw::edit(Serializer(options), "Scripts\\TreeControlSetups\\WorldMerge", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd())){
        if(options.worldFile.empty()){
            CString message(TRANSLATE("Не указан путь к файлу мира!"));
            MessageBox(message, 0, MB_OK | MB_ICONEXCLAMATION);
            return;
        }

		if(universe()->mergeWorld(options))
			MessageBox(TRANSLATE("Импорт успешно завершен"), 0, MB_OK | MB_ICONINFORMATION);
    }
}

void CMainFrame::OnFileSaveas()
{
	CWaitCursor wait;
	CDlgSelectWorld dlgSW(vMap.getWorldsDir(), TRANSLATE ("Выбор мира для записи"), true);
	int nRet=dlgSW.DoModal();
	if( nRet==IDOK ) {
		if(!dlgSW.selectWorldName.empty()){
			wait.Restore();
			save(dlgSW.selectWorldName.c_str());
			put2TitleNameDirWorld();
		}
		else {
			AfxMessageBox(TRANSLATE ("Ошибка записи мира!"));
		}
	}
}

void CMainFrame::OnFileSaveminimaptoworld()
{
	if(vMap.getWorldName().empty()){
		AfxMessageBox("Не выбран мир");
	}
	else{
		vMap.saveMiniMap(vMap.H_SIZE/16, vMap.V_SIZE/16);//128,128
	}
}



void CMainFrame::OnEditUserInterface()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
#ifdef _DEBUG
	const char* uieditor_path = "UIEditor-Debug.exe"; 
#else
	const char* uieditor_path = "UIEditor.exe"; 
#endif
	int result = _spawnl (_P_NOWAIT, uieditor_path, uieditor_path, NULL);
	if (result < 0)
	{
		// CString message (MAKEINTRESOURCE (IDS_UNABLE_TO_LAUNCH_UI_EDITOR));
		CString message ("Не могу запустить редактор интерфейса (исполняемый файл не найден)");
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}

#else // _VISTA_ENGINE_EXTERNAL_

	CString message (TRANSLATE("Недоступно во внешней версии"));
	MessageBox (message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_
}

void CMainFrame::OnEditObjects()
{
	//objectsMana.ShowWindow (SW_RESTORE);
	//m_dlgObjectsManager.UpdateWindow ();
}

BOOL CMainFrame::OnBarCheck(UINT nID)
{
	return CExtControlBar::DoFrameBarCheckCmd( this, nID, false );
}

void CMainFrame::OnUpdateControlBarMenu(CCmdUI* pCmdUI)
{
	CExtControlBar::DoFrameBarCheckUpdate( this, pCmdUI, false );
}


template<class BarType>
void toggleControlBar(BarType& bar)
{
	if(!bar.IsWindowVisible()) {
		bar.m_pDockSite->ShowControlBar(&bar, TRUE, FALSE);
		//miniMapBar_.ShowWindow(SW_SHOW);
		//miniMapBar_.FloatControlBar(CPoint( 300, 100 ));
	}
	else// miniMapBar_.ShowWindow(SW_HIDE);
		bar.m_pDockSite->ShowControlBar(&bar, FALSE, FALSE);
	
	((CMainFrame*)AfxGetMainWnd())->RecalcLayout();
}

void CMainFrame::OnViewTreeBar()
{
	toggleControlBar(toolsWindowBar_);
}

void CMainFrame::OnUpdateTimeSlider(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateViewTreeBar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(toolsWindowBar_.IsWindowVisible() != 0);
	//CExtControlBar::DoFrameBarCheckUpdate(this, pCmdUI, false);
}

void CMainFrame::OnViewProperties()
{
	toggleControlBar(propertiesBar_);
}

void CMainFrame::OnUpdateViewProperties(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(propertiesBar_.IsWindowVisible() != 0);
}

void CMainFrame::OnViewMinimap()
{
	toggleControlBar(miniMapBar_);
}

void CMainFrame::OnUpdateViewMinimap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(miniMap_->IsWindowVisible() != 0);
}

void CMainFrame::OnViewObjectsManager()
{
	toggleControlBar(objectsManagerBar_);
}

void CMainFrame::OnUpdateViewObjectsManager(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(objectsManagerBar_.IsWindowVisible() != 0);
}

void CMainFrame::OnViewResettoolbar2default()
{
	resetWorkspace();
}

void CMainFrame::OnClose()
{
	if (view_->isSceneInitialized() && vMap.isChanged()){
		CString message = "Do you want to save changes before exit?";

		int result = MessageBox (message, 0, MB_YESNOCANCEL | MB_ICONQUESTION);

		if(result == IDYES)
			OnFileSave();
		else if (result == IDCANCEL)
			return;
	}

	CFrameWnd::OnClose();
}

//                                                       Atr  Vx  Oper Pgs X   Y  разд.Sur Lgt  Ms(profile)
static int xSizePartsStatusBar[NUMBERS_PARTS_STATUSBAR]={ 40+80+20, 70+30, 100, 00, 70, 70, 00, 50, 50, 280};
static int rightCrdPartsStatusBar[NUMBERS_PARTS_STATUSBAR];

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	//Создание полей в статус баре
	CRect RR,RR1;
	CMainFrame* pMF = this;
	CExtStatusControlBar& statusBar = statusBar_;

	if(::IsWindow(statusBar.GetSafeHwnd())){
		statusBar.GetClientRect(&RR1);
		int bsx=RR1.Width();
		//Alt Geo Dam Itog
		rightCrdPartsStatusBar[0]=xSizePartsStatusBar[0];
		rightCrdPartsStatusBar[1]=xSizePartsStatusBar[1]+xSizePartsStatusBar[0];
		rightCrdPartsStatusBar[2]=xSizePartsStatusBar[2]+xSizePartsStatusBar[1]+xSizePartsStatusBar[0];
		//Разделитель(в нем прогресс), X Y - на середине
		rightCrdPartsStatusBar[3]=bsx/2 - xSizePartsStatusBar[4];
		rightCrdPartsStatusBar[4]=bsx/2;
		rightCrdPartsStatusBar[5]=bsx/2 + xSizePartsStatusBar[5];
		//Разделитель, Sur, Light, MS -  справа
		rightCrdPartsStatusBar[6]=bsx-xSizePartsStatusBar[7]-xSizePartsStatusBar[8]-xSizePartsStatusBar[9];
		rightCrdPartsStatusBar[7]=bsx-xSizePartsStatusBar[8]-xSizePartsStatusBar[9];
		rightCrdPartsStatusBar[8]=bsx-xSizePartsStatusBar[9];
		rightCrdPartsStatusBar[9]=bsx;

		for(int i = 0; i < NUMBERS_PARTS_STATUSBAR; ++i) {
			int width = i ? rightCrdPartsStatusBar[i] - rightCrdPartsStatusBar[i - 1] 
						  : rightCrdPartsStatusBar[i];
			statusBar.SetPaneWidth(i, max(0, width));
		}

		CRect progressBarRect(rightCrdPartsStatusBar[2] + 24, 2, rightCrdPartsStatusBar[3] + 30, RR1.Height());
		if(::IsWindow(progressBar_.GetSafeHwnd())) {
			progressBar_.MoveWindow(&progressBarRect);
		}
	}
}

bool CMainFrame::saveDlgBarState()
{
/*	CWinApp * pApp = ::AfxGetApp();
	ASSERT( pApp != NULL );
	ASSERT( pApp->m_pszRegistryKey != NULL );
	ASSERT( pApp->m_pszRegistryKey[0] != _T('\0') );
	ASSERT( pApp->m_pszProfileName != NULL );
	ASSERT( pApp->m_pszProfileName[0] != _T('\0') );
	pApp;

	return CExtControlBar::ProfileBarStateSave( this, pApp->m_pszRegistryKey, pApp->m_pszProfileName, pApp->m_pszProfileName, &m_dataFrameWP);*/

	bool retVal=false;
	try {
		// prepare memory file and archive
		CMemFile _file;
		CArchive ar(&_file, CArchive::store);
		// do serialization
		CExtControlBar::ProfileBarStateSerialize( ar, this, &m_dataFrameWP );
		// OK, serialization passed
		ar.Flush();
		ar.Close();
		// put information to registry
		_file.SeekToBegin();
		int fLenght=_file.GetLength();
#ifndef _VISTA_ENGINE_EXTERNAL_
		surMapOptions.dlgBarState.resize(fLenght);
		_file.Read(&surMapOptions.dlgBarState[0], fLenght);
#else
		surMapOptions.dlgBarStateExt.resize(fLenght);
		_file.Read(&surMapOptions.dlgBarStateExt[0], fLenght);
#endif

		//retVal=CExtCmdManager::FileObjToRegistry(_file, sRegKeyPath);
		retVal=true;
	} // try
	catch( CException * pXept )
	{
		pXept->Delete();
		ASSERT( FALSE );
	} // catch( CException * pXept )
	catch( ... )
	{
		ASSERT( FALSE );
	} // catch( ... )

	return retVal;
}

bool CMainFrame::loadDlgBarState()
{
	CWinApp * pApp = ::AfxGetApp();
	ASSERT( pApp != NULL );
//	ASSERT( pApp->m_pszRegistryKey != NULL );
//	ASSERT( pApp->m_pszRegistryKey[0] != _T('\0') );
	ASSERT( pApp->m_pszProfileName != NULL );
	ASSERT( pApp->m_pszProfileName[0] != _T('\0') );

	VERIFY( g_CmdManager->ProfileWndAdd( pApp->m_pszProfileName, GetSafeHwnd()) );
	VERIFY( g_CmdManager->UpdateFromMenu( pApp->m_pszProfileName, IDR_MAINFRAME) );

	//return !CExtControlBar::ProfileBarStateLoad(this, pApp->m_pszRegistryKey, pApp->m_pszProfileName, pApp->m_pszProfileName, &m_dataFrameWP);

    bool retVal=false;
	try {
		// prepare memory file and archive,
		// get information from registry
		CMemFile _file;
		//if( !CExtCmdManager::FileObjFromRegistry( _file, sRegKeyPath ) )
#ifdef _VISTA_ENGINE_EXTERNAL_
		std::vector<unsigned char>& dlgBarState = surMapOptions.dlgBarStateExt;
#else
		std::vector<unsigned char>& dlgBarState = surMapOptions.dlgBarState;
#endif
		if(dlgBarState.empty()){
			// win xp fix - begin
			WINDOWPLACEMENT _wpf;
			::memset( &_wpf, 0, sizeof(WINDOWPLACEMENT) );
			_wpf.length = sizeof(WINDOWPLACEMENT);
			if( GetWindowPlacement(&_wpf) )
			{
				_wpf.ptMinPosition.x = _wpf.ptMinPosition.y = 0;
				_wpf.ptMaxPosition.x = _wpf.ptMaxPosition.y = 0;
				_wpf.showCmd = 
					(GetStyle() & WS_VISIBLE)
						? SW_SHOWNA
						: SW_HIDE;
				SetWindowPlacement(&_wpf);
			} // if( pFrame->GetWindowPlacement(&_wpf) )
			// win xp fix - end
			return false;
		}
		else {
			_file.Write(&dlgBarState[0], dlgBarState.size());
			_file.SeekToBegin();
		}
		CArchive ar( &_file, CArchive::load	);
		// do serialization
		retVal = CExtControlBar::ProfileBarStateSerialize(	ar,	this, &m_dataFrameWP );
	} // try
	catch( CException * pXept )
	{
		pXept->Delete();
		ASSERT( FALSE );
	} // catch( CException * pXept )
	catch( ... )
	{
		ASSERT( FALSE );
	} // catch( ... )
	return retVal;
}


void CMainFrame::OnViewSources()
{
	surMapOptions.setShowSources(!surMapOptions.showSources());
	Environment::flag_ViewWaves = surMapOptions.showSources();
}

void CMainFrame::OnUpdateViewSources(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(surMapOptions.showSources());
	pCmdUI->Enable(view_->isWorldOpen());
}

#ifndef _VISTA_ENGINE_EXTERNAL_
# ifdef _DEBUG
	const char* GAME_EXE_PATH = "GameDBG.exe"; 
# else
	const char* GAME_EXE_PATH = "Game.exe"; 
# endif
#else
	const char* GAME_EXE_PATH = "Perimeter2.exe"; 
#endif

void CMainFrame::OnFileRunWorld()
{
	const char* TEMP_WORLD="TMP";
	if(!universe())
		return;
	string currentWorldName = vMap.getWorldName();
	save(TEMP_WORLD);

	vMap.setWorldName(currentWorldName.c_str());
	view_->currentMission().setByWorldName(currentWorldName.c_str());

	bool flag_animation = static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation;
	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = false;
	ShowWindow (SW_HIDE);
	Sleep (200);

#ifdef _VISTA_ENGINE_EXTERNAL_
	int result = _spawnl (_P_WAIT, GAME_EXE_PATH, GAME_EXE_PATH, "-battle", "openResource\\Worlds\\TMP.spg", NULL);
#else
	int result = _spawnl (_P_WAIT, GAME_EXE_PATH, GAME_EXE_PATH, "openResource\\Worlds\\TMP.spg", NULL);
#endif
	if (result < 0)
	{
        CString message (TRANSLATE("Не могу запустить игру!"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}

	vMap.deleteWorld(TEMP_WORLD);
	string fileMission= string(vMap.getWorldsDir()) + "\\" + TEMP_WORLD + ".spg";
	::DeleteFile(fileMission.c_str());
	ShowWindow (SW_SHOW);
	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = flag_animation;
}

void CMainFrame::OnEditGameScenario()
{
#ifndef _VISTA_ENGINE_EXTERNAL_

	bool environmentCreated = false;
	if(!environment){
		for(DirIterator it("Resource\\Worlds\\*.*"); it; ++it)
			if(it.isDirectory()){
				view_->createScene();
				vMap.load(it.c_str(), true);
				view_->reInitWorld();
				environmentCreated = true;
				break;
			}
	}

	static GameSerializer gameSerializer;
	Serializer serializeable(gameSerializer, "gameSerializer", TRANSLATE("Сценарий игры"));

	if(kdw::edit(serializeable, "Scripts\\TreeControlSetups\\GameScenarioState", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		saveAllLibraries();

	eventMaster().signalObjectChanged().emit(this);

	if(environmentCreated)
		view_->doneScene();

#else // _VISTA_ENGINE_EXTERNAL_	

	CString message(TRANSLATE("Недоступно во внешней версии"));
	MessageBox(message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_
}


void CMainFrame::OnEditRebuildworld()
{
	if(vMap.isWorldLoaded()){
		CWaitCursor wait;

		unsigned long vxacrc=vMap.getVxABufCRC();
		unsigned long damcrc=vMap.getDamSurBufCRC();
		unsigned int begTime=xclock();
		vMap.rebuild();
		view_->UpdateStatusBar();
		if(vxacrc!=vMap.getVxABufCRC())
			::AfxMessageBox(TRANSLATE("Сгенеренные высоты не совпадают с предыдущими"));
		if(damcrc!=vMap.getDamSurBufCRC())
			::AfxMessageBox(TRANSLATE("Поверхность текстуры(Dam) сгенеренного мира не совпадает с предыдущей"));

		view_->reInitWorld();
		Invalidate(FALSE);
	}
}

void CMainFrame::OnViewSettings()
{
}

void CMainFrame::OnEditEffects()
{
	editLibrary("EffectContainerLibrary");
}


void CMainFrame::OnEditPreferences ()
{
	if(kdw::edit(Serializer(surMapOptions), "Scripts\\TreeControlSetups\\UserPreferences", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		eventMaster().signalObjectChanged().emit(this);
	surMapOptions.apply();
	reloadMenu();
}

void CMainFrame::OnFileRunMenu()
{
	if(vMap.isWorldLoaded())
		OnFileSave();
	bool flag_animation = static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation;
	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = false;

	ShowWindow (SW_HIDE);
	Sleep (200);

	int result = _spawnl (_P_WAIT, GAME_EXE_PATH, GAME_EXE_PATH, NULL);
	if (result < 0)	{
		CString message (TRANSLATE ("Не могу запустить игру!"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}
	ShowWindow (SW_SHOW);

	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = flag_animation;
}

void CMainFrame::OnFileParametersExportStatistics()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", 0);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить XLS файл...");


	if(fileDlg.DoModal() == IDOK){
	
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();	
		ParameterStatisticsExport exporter;
		const char* fileName = "Scripts\\TreeControlSetups\\ParameterExportStatistics.dat";
		if(::isFileExists(fileName)){
			XPrmIArchive ia(fileName);
			ia.serialize(exporter, "exporter", 0);
		}
		if(kdw::edit(Serializer(exporter), "Scripts\\TreeControlSetups\\ParameterExportStatistics", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd())){
			exporter.exportExcel(path);
			XPrmOArchive oa(fileName);
			oa.serialize(exporter, "exporter", 0);
		}
	}
#endif
}

void CMainFrame::OnFileParametersExportUnits()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", 0);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить XLS файл...");

	if(fileDlg.DoModal() == IDOK){
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();	

		ParameterTree::Exporter exporter;
		exporter.exportExcel(path);
	}
#endif
}

void askForSaveAllLibraries()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	if(AfxMessageBox(TRANSLATE("Сохранить библиотеки?\n(если нет - это нужно будет сделать вручную)"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		saveAllLibraries();
#endif
}

void CMainFrame::OnFileParametersImportUnits()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (TRUE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Открыть XLS файл...");

	int result = MessageBox(TRANSLATE("Импорт параметров юнитов произведет необратимые изменения сразу в нескольких библиотеках!\nПродолжить?"), 0, MB_YESNO | MB_ICONQUESTION);
	if(result == IDYES){
		if(fileDlg.DoModal () == IDOK){
			CWaitCursor waitCursor;
			CString path = fileDlg.GetPathName();	

			ParameterTree::Exporter exporter;
			exporter.importExcel(path);

			askForSaveAllLibraries();
		}
	}
#endif
}

void CMainFrame::OnDebugSaveDictionary()
{
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);
}

void CMainFrame::OnEditToolzers()
{
}

void CMainFrame::OnEditTertools()
{
	editLibrary("TerToolsLibrary");
}

void CMainFrame::OnFileProperties()
{
	CWorldPropertiesDlg dlg;
	dlg.DoModal ();
}

void CMainFrame::OnEditSounds()
{
	editLibrary("SoundLibrary");
}

struct ExportParameters
{
	string projectName;
	bool packResource;
	bool exportPDB;
	bool exportEditor;
	bool exportFinal;
	bool protect;
	bool buildInstaller;
	string fileNSIS;

	struct World {
		string name;
		string nameTrans;
		bool export;
		World(const char* _name) : name(_name), export(false), nameTrans(transliterate(_name)) {}
		void serialize(Archive& ar){
			ar.serialize(export, nameTrans.c_str(), name.c_str());
		}
	};
	typedef vector<World> Worlds;
	Worlds worlds;
	ExportParameters()
	{
		projectName = "gameFinal";
		packResource = false;
		exportPDB = true;
		exportEditor = false;
		exportFinal = true;
		protect = false;
		buildInstaller = false;

		DirIterator it("Resource\\Worlds\\*.*");
		while(it){
			if(it.isDirectory())
				worlds.push_back(*it);
			++it;
		}
	}

	void serialize(Archive& ar)
	{
		ar.serialize(projectName, "projectName", "Имя проекта");
		ar.serialize(packResource, "packResource", "Паковать ресурсы");
		ar.serialize(exportEditor, "exportEditor", "Экспортировать редактор");
		ar.serialize(exportPDB, "exportPDB", "Экспортировать файлы дебаговой информации");
		ar.serialize(exportFinal, "exportFinal", "Экспортировать финальный исполняемый файл (с выключенными run-time проверками)");
		ar.serialize(protect, "protect", "Защищать StarForce (будет произведена остановка для запуска вручную, открыть порт 27705)");
		ar.serialize(buildInstaller, "buildInstaller", "Собирать установщик");
		if(buildInstaller){
			static ResourceSelector::Options options("*.nsi", "Scripts\\Engine\\NSIS", "NSIS Project");
			ar.serialize(ResourceSelector(fileNSIS, options), "fileNSIS", "Проект установщика");
		}

		if(ar.openBlock("worlds", "Миры для экспорта")){
			Worlds::iterator i;
			FOR_EACH(worlds, i)
				i->serialize(ar);
			ar.closeBlock();
		}
	}
};

void modelSelectorCallBack(const char* name)
{
	if(getExtention(name) == "3dx"){
		cStatic3dx* element = pLibrary3dx->GetElement(name, 0, false);
		if(element){
			TextureNames textures;
			element->GetTextureNames(textures);
			TextureNames::iterator i;
			FOR_EACH(textures, i)
				ExportInterface::export(i->c_str());
		}
	}
	else if(getExtention(name) == "effect"){
		ExportInterface::export(name);
		vector<string> textures;
		if(GetAllTextureNamesEffectLibrary(name, textures)){
			vector<string>::iterator i;
			FOR_EACH(textures, i)
				ExportInterface::export(i->c_str());
		}
	}
	else
		xassert(!strlen(name));
}

void CMainFrame::OnFileExportVistaEngine()
{
	ExportParameters params;

	XPrmIArchive ia;
	if(ia.open("Scripts\\Content\\export.scr"))
		params.serialize(ia);

	if(!kdw::edit(Serializer(params), "Scripts\\TreeControlSetups\\export", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		return;

	params.serialize(XPrmOArchive("Scripts\\Content\\export.scr"));

	if(terScene){
		view_->doneUniverse();
		terScene->Compact();
	}

	const char* path = "ExportVistaEngine.bat"; 
	if(system(path) < 0){ 
		CString message(TRANSLATE("Не могу запустить скрипт экспорта!"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
		return;
	}

	CopyFile("ConfigurationTool.exe", "Output\\ConfigurationTool.exe", true);
	CopyFile("AttribEditor.dll", "Output\\AttribEditor.dll", true);
	if(params.exportFinal){
		XBuffer buffer;
		buffer < "Output\\" < params.projectName.c_str() < ".exe";
		CopyFile("GameFinal.exe", buffer, true);
		if(params.exportPDB)
			CopyFile("gameFinal.pdb", "Output\\gameFinal.pdb", true);
	}
	else{
		XBuffer buffer;
		buffer < "Output\\" < params.projectName.c_str() < ".exe";
		CopyFile("Game.exe", buffer, true);
		if(params.exportPDB){
			CopyFile("game.pdb", "Output\\game.pdb", true);
		}
	}
	CopyFile("binkw32.dll", "Output\\binkw32.dll", true);

	if(params.exportEditor){
		XBuffer buffer;
		buffer < "Output\\" < params.projectName.c_str() < "MapEditor.exe";
		CopyFile("VistaEngineExt.exe", buffer, true);
		if(params.exportPDB)
			CopyFile("VistaEngineExt.pdb", "Output\\VistaEngineExt.pdb", true);

		CopyFile("ExcelExport.dll", "Output\\ExcelExport.dll", true);
	}

	iniFile.save();
	CopyFile("iniFile.cfg", "Output\\iniFile.cfg", true);

	ExportInterface::setExport(true, modelSelectorCallBack);

	GetTexLibrary()->clearCacheInfo();
	pLibrary3dx->clearCacheInfo();

	EffectContainer::setTexturesPath("Resource\\FX\\Textures");
	for(EffectLibrary::Strings::const_iterator it = EffectLibrary::instance().strings().begin(); it != EffectLibrary::instance().strings().end(); ++it)
		if(it->get())
			it->get()->preloadLibrary();

	saveAllLibraries();

	TriggerChain globalTrigger;
	globalTrigger.load("Scripts\\Content\\Triggers\\GlobalTrigger.scr");

	ExportParameters::Worlds::iterator i;
	FOR_EACH(params.worlds, i){
		if(!i->export)
			continue;
		view_->createScene();
		vMap.load(i->name.c_str(), true);
		view_->reInitWorld();
		
		save(vMap.getWorldName().c_str());

		string srcDir = "Resource\\Worlds\\";
		string destDir = "Output\\Resource\\Worlds\\";
		srcDir += i->name;
		destDir += i->name;
		CopyFile((srcDir + ".spg").c_str(), (destDir + ".spg").c_str(), true);
		CopyFile((srcDir + ".spg.bin").c_str(), (destDir + ".spg.bin").c_str(), true);
		srcDir += "\\";
		destDir += "\\";
		createDirectory(destDir.c_str());
		DirIterator it((srcDir + "*.*").c_str());
		while(it){
			if(it.isFile())
				CopyFile((srcDir + *it).c_str(), (destDir + *it).c_str(), true);
			++it;
		}
	}

	OnFileUpdateQuickStartWorldsList();
	CopyFile("Resource\\Worlds\\qswl.scr", "Output\\Resource\\Worlds\\qswl.scr", true);

	GetTexLibrary()->saveCacheInfo(true);
	pLibrary3dx->saveCacheInfo(true);

	const char* path2 = "ExportVistaEngine.bat pass2"; 
	if(system(path2) < 0){ 
		CString message(TRANSLATE("Не могу запустить скрипт экспорта!"));
		MessageBox(message, 0, MB_OK | MB_ICONERROR);
		return;
	}

	ExportInterface::setExport(false, 0);
	GetTexLibrary()->saveCacheInfo(false);
	pLibrary3dx->saveCacheInfo(false);

	if(params.packResource){
		ExportOutputCallback::show(this);
		SetCurrentDirectory("Output");
		ZipConfig::makeArchives(&ExportOutputCallback::callback);
		iniFile.ZIP = true;
		SetCurrentDirectory("..");
		ExportOutputCallback::hide(this);
	}

	if(params.protect){
		XBuffer substCommand;
		substCommand < "subst X: " < extractFilePath(__argv[0]).c_str() < "\\Output";
		system(substCommand);

		MessageBox("Запустите защиту StarForce вручную", 0, MB_OK | MB_ICONERROR);

		while(!isFileExists("Output\\SFOutput\\protect.dll"))
			MessageBox("Защита прошла неудачно, запустите защиту еще раз", 0, MB_OK | MB_ICONERROR);

		system("move Output\\SFOutput\\*.* Output");
		system("rmdir /Q /S Output\\SFOutput");
		system("rmdir /Q /S Output\\Scripts\\Content");
		system("mkdir Output\\Scripts\\Content");
		system("copy Scripts\\Content\\Controls Output\\Scripts\\Content");
		system("copy Scripts\\Content\\GameOptions Output\\Scripts\\Content");
		system("subst X: /D");
	}

	if(params.buildInstaller){
		while(!isFileExists("C:\\Program Files\\NSIS\\makensis.exe")){
			MessageBox("Необходимо установить NSIS (в путях по-умолчанию - C:\\Program Files\\)", 0, MB_OK | MB_ICONERROR);
			system("\\\\CENTER\\Incubator\\Heap\\NSIS\\nsis-2.32-setup.exe");
		}

		XBuffer run;
		run < "\"C:\\Program Files\\NSIS\\makensis.exe\" " < params.fileNSIS.c_str();
		system(run);
	}
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.UndoDispatcher_IsUndoExist());
}

void CMainFrame::OnEditSaveCameraAsDefault()
{
	GlobalAttributes::instance().setCameraCoordinate();
	askForSaveAllLibraries();
}

void CMainFrame::OnEditUndo()
{
	CWaitCursor wait;
	if(vMap.isWorldLoaded()){
		vMap.UndoDispatcher_Undo();
		Invalidate(FALSE);
	}
}

void CMainFrame::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.UndoDispatcher_IsRedoExist());
}

void CMainFrame::OnEditRedo()
{
	CWaitCursor wait;
	if(vMap.isWorldLoaded()){
		vMap.UndoDispatcher_Redo();
		Invalidate(FALSE);
	}
}

void CMainFrame::OnDebugShowpalettetexture()
{
	CWaitCursor wait;
	if(vMap.isShowTryColorDamTexture())
		vMap.toShowTryColorDamTexture(false);
	else 
		vMap.toShowTryColorDamTexture(true);
	vMap.WorldRender();

}

void CMainFrame::OnUpdateDebugShowpalettetexture(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.isWorldLoaded());
	pCmdUI->SetCheck(!vMap.isShowTryColorDamTexture());
}

struct HeadLibrarySerialization{
	void serialize(Archive& ar){
		GlobalAttributes::instance().serializeHeadLibrary(ar);
	}
};

void CMainFrame::OnEditHeads()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	if(kdw::edit(Serializer(HeadLibrarySerialization()), "Scripts\\TreeControlSetups\\heads", kdw::ONLY_TRANSLATED | kdw::IMMEDIATE_UPDATE, GetSafeHwnd()))
		::saveAllLibraries();
#endif
}

void CMainFrame::OnEditSoundTracks()
{
	editLibrary("SoundTrackTable");
}

void CMainFrame::OnEditReels()
{
	//CFileLibraryEditorDlg dlg ("Resource\\Video", "*.bik");
	//dlg.DoModal ();
}

void CMainFrame::OnViewHideModels()
{
	surMapOptions.setHideWorldModels(!surMapOptions.hideWorldModels());
}

void CMainFrame::OnUpdateViewHideModels(CCmdUI* cmd)
{
	cmd->SetCheck(surMapOptions.hideWorldModels());
	cmd->Enable(view_->isWorldOpen());
}

void CMainFrame::OnViewCameras()
{
	surMapOptions.setShowCameras(!surMapOptions.showCameras());
}
void CMainFrame::OnUpdateViewCameras(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(surMapOptions.showCameras());
	pCmdUI->Enable(view_->isWorldOpen());
}

void CMainFrame::OnViewGeosurface()
{
	if(view_->isWorldOpen()){
		CWaitCursor wait;
		//if(vMap.IsShowSpecialInfo()==vrtMap::SSI_ShowAllGeo){
		//	vMap.toShowSpecialInfo(vrtMap::SSI_NoShow);
		//}
		//else {
		//	vMap.toShowSpecialInfo(vrtMap::SSI_ShowAllGeo);
		//}
		//vMap.WorldRender();
	}
}

void CMainFrame::OnViewPathFinding()
{
	surMapOptions.setShowPathFinding(!surMapOptions.showPathFinding());
}

void CMainFrame::OnUpdateViewPathFinding(CCmdUI *cmd)
{
	cmd->Enable(TRUE);
	cmd->SetCheck(surMapOptions.showPathFinding());
}

void CMainFrame::OnViewPathFindingReferenceUnit()
{
	AttributeReference reference(surMapOptions.showPathFindingReferenceUnit());
	kdw::TreeSelectorDialog dlg(*this);
	kdw::ReferenceTreeBuilder<AttributeReference> treeBuilder(reference);
	dlg.setBuilder(&treeBuilder);
	if(dlg.showModal() == kdw::RESPONSE_OK){
		surMapOptions.setShowPathFindingReferenceUnit(reference.unitAttributeID().nameRace().c_str());
	}
}

void CMainFrame::OnUpdateViewGeosurface(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(false /*vMap.IsShowSpecialInfo()==vrtMap::SSI_ShowAllGeo*/);
	pCmdUI->Enable(view_->isWorldOpen());
}


void CMainFrame::OnEditEffectsEditor()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	const char* fxeditor_path = "EffectTool.exe"; 
	int result = _spawnl (_P_NOWAIT, fxeditor_path, fxeditor_path, NULL);
	if (result < 0)
	{
		CString message(TRANSLATE("Не могу запустить редактор эффектов (исполняемый файл не найден)"));
		MessageBox(message, 0, MB_OK | MB_ICONERROR);
	}

#else // _VISTA_ENGINE_EXTERNAL_

	CString message(TRANSLATE("Недоступно во внешней версии"));
	MessageBox(message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_
}


void CMainFrame::OnEditCursors()
{
	editLibrary("UI_CursorLibrary");
}

void CMainFrame::OnDebugEditDebugPrm()
{
	DebugPrm::instance().editLibrary();
}

void CMainFrame::OnDebugEditAuxAttribute()
{
	AuxAttributeLibrary::instance().editLibrary();
}

void CMainFrame::OnDebugEditRigidBodyPrm()
{
	RigidBodyPrmLibrary::instance().editLibrary();
}

void CMainFrame::OnDebugEditToolzer()
{
}

void CMainFrame::OnDebugEditExplodeTable()
{
	ExplodeTable::instance().editLibrary();
}

void CMainFrame::OnDebugEditSourcesLibrary()
{
	//SourcesLibrary::instance().editLibrary();
	editLibrary("SourcesLibrary");
}

void CMainFrame::OnDebugShowmipmap()
{
	if(GetTexLibrary())
	{
		GetTexLibrary()->DebugMipMapColor(!GetTexLibrary()->IsDebugMipMapColor());
	}
}

void CMainFrame::OnUpdateDebugShowmipmap(CCmdUI *pCmdUI)
{
	if(GetTexLibrary())
		pCmdUI->SetCheck(GetTexLibrary()->IsDebugMipMapColor());
}

struct MoveShiftScale : UniverseObjectAction{
	MoveShiftScale(const Vect3f& _shift, const Vect2f& _scaleXY, float _baseH, float _scaleH, float _scaleObjects, bool _init)
    : shift(_shift), scaleXY(_scaleXY), baseH(_baseH), scaleH(_scaleH), scaleObjects(_scaleObjects), init(_init) { }
	void operator()(BaseUniverseObject& object){
		Vect3f pos(object.position());
		pos.x *=scaleXY.x;
		pos.y *=scaleXY.y;
		pos.z = (pos.z - baseH) * scaleH + baseH;
		pos+=shift;
		object.setRadius(object.radius() * scaleObjects);
		object.setPose(Se3f(object.orientation(), pos), init);
		awakePhysics(object);
	}
	Vect3f shift;
	Vect2f scaleXY;
	float baseH;
	float scaleH;
	float scaleObjects;
	bool init;
};

void CMainFrame::OnEditChangetotalworldheight()
{
	CDlgChangeTotalWorldHeight dlg;
	int nRet = -1;
	nRet = dlg.DoModal();
	if(nRet==IDOK){
		CWaitCursor wait;
		MergeOptions options;
		options.mergePlayers=true;
		options.mergeWorld=true;
		options.mergeSourcesAndAnchors=false;
		options.mergeCameras=false;
		options.worldFile = string(vMap.getWorldsDir()) + "\\" + vMap.getWorldName() + ".spg";
		Vect2f oldWorldSize(vMap.H_SIZE, vMap.V_SIZE);

		view_->createScene();
		float scaleVx=(float)dlg.m_scaleH.value/100.f;
		float baseH=vMap.changeTotalWorldParam(dlg.m_deltaH.value, scaleVx, dlg.m_changeParam);
		
		view_->reInitWorld();

		Vect2f scaleXY;
		if(dlg.m_changeParam.flag_resizeWorld2NewBorder)
            scaleXY.set((float)vMap.H_SIZE/oldWorldSize.x, (float)vMap.V_SIZE/oldWorldSize.y);
		else 
			scaleXY.set(1.f, 1.f);
		Vect3f shift(dlg.m_changeParam.oldWorldBegCoordX, dlg.m_changeParam.oldWorldBegCoordY, dlg.m_deltaH.value);
		MoveShiftScale moveAnShift(shift, scaleXY, baseH, scaleVx, dlg.m_changeParam.kScaleModels, true);
		universe()->universeObjectAction=&moveAnShift;
		universe()->mergeWorld(options);
		universe()->universeObjectAction=0;
		//forEachUniverseObject(), true);
		Invalidate(FALSE);
		put2TitleNameDirWorld();
		toolsWindow_->rebuildTools();
	}
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(menuBar_.TranslateMainFrameMessage(pMsg))
		return TRUE;
	
	return CFrameWnd::PreTranslateMessage(pMsg);
}


void CMainFrame::OnFileStatistics()
{
	void ShowGraphicsStatistic();
	ShowGraphicsStatistic();
}

void CMainFrame::OnViewToolbar()
{
	toggleControlBar(toolBar_);
}


void CMainFrame::OnUpdateViewShowCameraBorders(CCmdUI *cmdUI)
{
	cmdUI->SetCheck(surMapOptions.showCameraBorders() ? 1 : 0);
}

void CMainFrame::OnViewShowCameraBorders()
{
	surMapOptions.setShowCameraBorders(!surMapOptions.showCameraBorders());
}

void CMainFrame::OnUpdateViewToolbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(toolBar_.IsVisible());
}

void CMainFrame::OnViewShowGrid()
{
	surMapOptions.enableGrid_ = !surMapOptions.enableGrid_;
}

void CMainFrame::OnUpdateViewShowGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(surMapOptions.enableGrid_);
	pCmdUI->Enable(view_->isWorldOpen());
}

void CMainFrame::OnViewEnableTimeFlow()
{
	Environment::flag_EnableTimeFlow = !Environment::flag_EnableTimeFlow;
}

void CMainFrame::OnUpdateViewEnableTimeFlow(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::flag_EnableTimeFlow);
	pCmdUI->Enable(view_->isWorldOpen());
}

void CMainFrame::OnUpdateIsWorldLoaded(CCmdUI *cmdUI)
{
	cmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnFileImportTextFromExcel()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (TRUE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Открыть XLS файл...");

	if(fileDlg.DoModal() == IDOK){
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();	

		ImportImpl excl_(path);

		if(excl_.importLangList(GlobalAttributes::instance().languagesList))
			if(excl_.importAllLangText(GlobalAttributes::instance().languagesList))
				GlobalAttributes::instance().saveLibrary(); // сохраняем изменения кодовых страниц
	}
#endif
}

void CMainFrame::OnFileExportTextToExcel()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить XLS файл...");

	if(fileDlg.DoModal() == IDOK){
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();
		FILE* stream;
		if((stream = fopen(path, "r")) != NULL){
			fclose(stream);
			//ImportImpl exclIm_(path);
			//exclIm_.application()->openSheet();
			//exclIm_.importAllLangText(GlobalAttributes::instance().languagesList);
		}
        DeleteFile(path);
		
		reloadLocStrings();

		ExportImpl excl_(path);
		excl_.exportAllLangText(GlobalAttributes::instance().languagesList);
	}
#endif
}

void CMainFrame::reloadLocStrings()
{
	AttributeLibrary::instance();
	UI_Dispatcher::instance();
	UI_TextLibrary::instance();
	CommonLocText::instance();
	GlobalAttributes::instance();
	GameOptions::instance();

	WIN32_FIND_DATA FindFileData;
	HANDLE hf = FindFirstFile( "Scripts\\Content\\Triggers\\*.scr", &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ) {
				TriggerChain triggers;
				triggers.load((string("Scripts\\Content\\Triggers\\") + FindFileData.cFileName).c_str());
			}
		} while(FindNextFile( hf, &FindFileData ));
		FindClose( hf );
	}

	hf = FindFirstFile( "Resource\\Worlds\\*.spg", &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if(!(FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ){
				XBuffer fullName;
				fullName < "Resource\\Worlds\\" < FindFileData.cFileName;
				MissionDescription missionDescription(fullName);
			}
		} while(FindNextFile( hf, &FindFileData ));
		FindClose( hf );
	}
}

void CMainFrame::OnDebugUISpriteLib()
{
	editLibrary("UI_ShowModeSpriteTable");
}

void CMainFrame::OnEditCommandColor()
{
	CommandColorManager::instance().editLibrary(true);
}


void CMainFrame::OnEditUITextSprites()
{
	editLibrary("UI_SpriteLibrary");
}

LRESULT CMainFrame::OnConstructPopupMenu(WPARAM wParam, LPARAM lParam)
{
   ASSERT_VALID( this );
   lParam;
   CExtControlBar::POPUP_MENU_EVENT_DATA * p_pmed =
      CExtControlBar::POPUP_MENU_EVENT_DATA::FromWParam( wParam );
   ASSERT( p_pmed != NULL );
   ASSERT_VALID( p_pmed->m_pPopupMenuWnd );
   ASSERT_VALID( p_pmed->m_pWndEventSrc );
   ASSERT( p_pmed->m_pWndEventSrc->GetSafeHwnd() != NULL );
   ASSERT( ::IsWindow(p_pmed->m_pWndEventSrc->GetSafeHwnd()) );

   if( ( !p_pmed->m_bPostNotification)
       && p_pmed->m_nHelperNotificationType ==
         CExtControlBar::POPUP_MENU_EVENT_DATA::__PMED_DOCKBAR_CTX
      )
      return (!0);

   return 0;
}
LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message ==  CExtControlBar::g_nMsgConstructPopupMenu){
		return OnConstructPopupMenu(wParam, lParam);;
	}
	return CFrameWnd::WindowProc(message, wParam, lParam);
}

void CMainFrame::OnLibrariesUIShowModeSprites()
{
	editLibrary("UI_ShowModeSpriteTable");
}

void CMainFrame::OnLibrariesUIMessages()
{
	editLibrary("UI_TextLibrary");
}

void CMainFrame::OnLibrariesUIMessageTypes()
{
	editLibrary("UI_MessageTypeLibrary");
}

void exportParametersByGroups()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", 0);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить XLS файл...");

	if (fileDlg.DoModal () == IDOK) {
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();
        DeleteFile(path);
		
		ParameterExportExcel excl_(path);
		excl_.exportParameters();
		
	}
#endif
}

void importParametersByGroups()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (TRUE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
						 "(*.xls)|*.xls||", 0);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Открыть XLS файл...");
	if(fileDlg.DoModal () == IDOK){
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();

		ParameterImportExcel excl_(path);
		excl_.importParameters();
	}
#endif
}

void CMainFrame::OnImportParameters()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	importParametersByGroups();
	askForSaveAllLibraries();
#endif
}

void CMainFrame::OnExportParameters()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	exportParametersByGroups();
#endif
}

void CMainFrame::OnEditUpdateSurface()
{
	view_->updateSurface();
}

CMainFrame& mainFrame()
{
	CMainFrame* mainFrame = (CMainFrame*)AfxGetMainWnd();
	xassert(mainFrame);
	return *mainFrame;
}

void CMainFrame::OnFileParametersImportFull()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (TRUE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Открыть XLS файл...");
	if (fileDlg.DoModal () == IDOK) {
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();
		
		ExcelImporter* importer = ExcelImporter::create(path);

		importer->openSheet();
        
        ParameterValueTable::Strings& strings = const_cast<ParameterValueTable::Strings&>(ParameterValueTable::instance().strings());
        ParameterValueTable::Strings::iterator it;

		int columns_count = 6;

		int line = 1;
        FOR_EACH(strings, it) {
            ParameterValue& value = *it;
            
            value.setRawValue(importer->getCellFloat(Vect2i(1, line)));

			std::string type = w2a(importer->getCellText(Vect2i(2, line)));
			if(type != value.type()->c_str())
				value.setType(ParameterTypeReference(type.c_str()));

			std::string formula = w2a(importer->getCellText(Vect2i(3, line)));
			if(formula != value.formula()->c_str())
				value.setFormula(ParameterFormulaReference(formula.c_str()));

			std::string group = w2a(importer->getCellText(Vect2i(4, line)));
			if(group != value.group()->c_str())
				value.setGroup(ParameterGroupReference(group.c_str()));

            ++line;
        }
        importer->free();

		askForSaveAllLibraries();
	} else {
	}
#endif
}

void CMainFrame::OnFileParametersImportByGroups()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	::importParametersByGroups();
	askForSaveAllLibraries();
#endif
}

void CMainFrame::OnFileParametersExportFull()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = TRANSLATE("Сохранить XLS файл...");

	if (fileDlg.DoModal () == IDOK) {
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();
        DeleteFile(path);
		
		ExcelExporter* exporter = ExcelExporter::create(path);
		exporter->beginSheet();

        const ParameterValueTable::Strings& strings = ParameterValueTable::instance().strings();
        ParameterValueTable::Strings::const_iterator it;

		exporter->setCellText(Vect2i(0, 0), L"Имя параметра");
		exporter->setCellText(Vect2i(1, 0), L"Значение");
		exporter->setCellText(Vect2i(2, 0), L"Тип");
		exporter->setCellText(Vect2i(3, 0), L"Формула");
		exporter->setCellText(Vect2i(4, 0), L"Группа");
		exporter->setCellText(Vect2i(5, 0), L"Выч. знач.");
		int columns_count = 6;

		exporter->setPageOrientation(true);
		exporter->setPageMargins(28.0f, 28.0f, 28.0f, 28.0f);

		int line = 1;
        FOR_EACH(strings, it) {
            const ParameterValue& value = *it;
            
			exporter->setCellText(Vect2i(0, line), a2w(value.c_str()).c_str());
            exporter->setCellFloat(Vect2i(1, line), value.rawValue());
            exporter->setCellText(Vect2i(2, line), a2w(value.type()->c_str()).c_str());
            exporter->setCellText(Vect2i(3, line), a2w(value.formula()->c_str()).c_str());
			if(strlen(value.formula()->c_str()))
			    exporter->setBackColor(Recti(0, line, columns_count, 1), 48);

            exporter->setCellText(Vect2i(4, line), a2w(value.group()->c_str()).c_str());
			exporter->setCellFloat(Vect2i(5, line), value.value());
            ++line;
        }
        
        exporter->setColumnWidth(0, 50.0f); 
        exporter->setColumnWidth(1, 10.0f);  
        exporter->setColumnWidth(2, 22.0f); 
        exporter->setColumnWidth(3, 23.0f); 
        exporter->setColumnWidth(4, 23.0f);  
        exporter->setColumnWidth(5, 8.0f);  


		exporter->setFont(Recti(0, 0, columns_count, 1),        "Tahoma", 7.0f, true, false);
		for(int i = 0; i < strings.size(); ++i) {
			exporter->setFont(Recti(0, 1 + i, columns_count, 1), "Tahoma", 7.0f, false, false);
		}

		exporter->free();
	} else {
		return;
	}
#endif
}

void CMainFrame::OnFileParametersExportByGroups()
{
	::exportParametersByGroups();
}

void CMainFrame::OnHelpKeyInfo()
{
	sKey key;
	kdw::HotkeyDialog dlg(*this, key);
	if(dlg.showModal() == kdw::RESPONSE_OK){
		key = dlg.get();

		CString str;

		//WCHAR wchars[2];
		BYTE state[256];
		GetKeyboardState(state);
		
		char chars[2];
		chars[0] = '\0';
		chars[1] = '\0';
		WORD singleChar;
		
		if(ToAscii(key.key, 0, state, &singleChar, 0) == 1)
			chars[0] = char(singleChar);

		std::string locKitName = key.toString();
		str.Format("Виртуальный код: 0x%X (%i)\nСимвол: %s\nНазвание из LocKit-а: '%s'", key.key, key.key, chars[0] ? chars : "Нет", locKitName.c_str());

		MessageBox(str, 0, MB_OK | MB_ICONINFORMATION);
	}
}

void CMainFrame::OnFileResaveWorlds()
{
	CWaitCursor waitCursor;

	DirIterator it("Resource\\Worlds\\*.*");
	while(it){
		if(it.isDirectory() && strlen(*it) && (*it)[0] != '.'){
			view_->createScene();
			vMap.load(*it, true);
			view_->reInitWorld();

			save(vMap.getWorldName().c_str());
		}
		++it;
	}
}

void CMainFrame::OnFileResaveTriggers()
{
	CWaitCursor waitCursor;
	DirIterator it("Scripts\\Content\\Triggers\\*.scr");
	while(it){
		if(it.isFile()){
			TriggerChain triggers;
			triggers.load((std::string("Scripts\\Content\\Triggers\\") + *it).c_str());
			triggers.save();
		}
		++it;
	}
}


void translateMenu(HMENU menu, const char* pathBase)
{
//	HINSTANCE instance = ::GetWindowInstance(GetSafeHwnd
	UINT count = ::GetMenuItemCount(menu);
	for(UINT i = 0; i < count; ++i){
		std::string path = pathBase;
		MENUITEMINFO menuInfo;
		memset(&menuInfo, 0, sizeof(menuInfo));
		menuInfo.cbSize = sizeof(menuInfo);
		menuInfo.fMask = MIIM_TYPE | MIIM_ID;
		if(::GetMenuItemInfo(menu, i, TRUE, &menuInfo)){
			WORD commandID = menuInfo.wID;
			CString idString;
			idString.LoadString(commandID);
			if(menuInfo.fType == MFT_STRING){
				VERIFY(::GetMenuItemInfo(menu, i, TRUE, &menuInfo));
				if(menuInfo.cch > 0){
					std::string text(menuInfo.cch + 1, '\0');
					menuInfo.dwTypeData = &text[0];
					menuInfo.cch += 1;
					::GetMenuItemInfo(menu, i, TRUE, &menuInfo);


					std::string::size_type pos = text.find("\t");
					std::string hotkey = "";
					if(pos != std::string::npos){
						hotkey = "\t";
						hotkey += text.c_str() + pos;
						text = std::string(text.begin(), text.begin() + pos);
					}
					else
						text.pop_back(); // удаляем последний '\0'


					std::string tooltip = "";

					path += text;
					std::string translationKey = path;
					const char* ptr = static_cast<const char*>(idString);
					const char* end = ptr + strlen(ptr);
					ptr = std::find(ptr, end, '\n');
					if(ptr != end){
						tooltip = std::string(ptr, end);
						translationKey += tooltip;
					}

					const char* key = translationKey.c_str();
					const char* translatedName = TRANSLATE(key);
					
					std::string nameWithHotkey = text + hotkey;

					if(translatedName != key){
						const char* name = translatedName + strlen(translatedName);
						if(name != translatedName)
							while((name - 1) != translatedName && *(name-1) != '\\')
								--name;
                        if(name - 1 == translatedName)
							name = translatedName;
						const char* end = name;
						while(*end && *end != '\n')
                            ++end;
						nameWithHotkey = std::string(name, end) + hotkey;
						if(*end){
							++end;
							tooltip = end;
						}
					}
					
					
					CExtCmdManager::cmd_t* cmd;
					cmd = g_CmdManager->CmdGetPtr(AfxGetApp()->m_pszProfileName, commandID);
					if(commandID && cmd){
						cmd->m_sTipTool = tooltip.c_str();
						cmd->m_sMenuText = nameWithHotkey.c_str();
					}

					MENUITEMINFO info;
					memset(&info, 0, sizeof(info));
					info.cbSize = sizeof(info);
					info.fMask = MIIM_STRING;
					info.dwTypeData = (char*)nameWithHotkey.c_str();
					VERIFY(::SetMenuItemInfo(menu, i, TRUE, &info));
					//VERIFY(::ModifyMenu(menu, i, MF_BYPOSITION | MF_STRING, 0, nameWithHotkey.c_str()));
					path += "\\";
				}
			}
			menuInfo.fMask = MIIM_SUBMENU;
			if(::GetMenuItemInfo(menu, i, TRUE, &menuInfo)){
				if(menuInfo.hSubMenu)
					translateMenu(menuInfo.hSubMenu, path.c_str());
			}
		}
	}
}

bool CMainFrame::reloadMenu()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	const bool external = false;
#else
	const bool external = true;
#endif
	if(!menuBar_.LoadMenuBar(external ? IDR_MAINFRAME_EXTERNAL : IDR_MAINFRAME))
		return false;
	
	HMENU menu = menuBar_.GetMenu()->m_hMenu;
	::translateMenu(menu, "");

	if(!g_CmdManager->UpdateFromMenu(g_CmdManager->ProfileNameFromWnd(GetSafeHwnd()), menu), false){
		ASSERT(FALSE);
	}
	menuBar_.UpdateMenuBar();
	CWnd::RepositionBars(0, 0, 0);

	DrawMenuBar();
	return true;
}

void CMainFrame::OnFileUpdateQuickStartWorldsList()
{
	CWaitCursor waitCursor;

	string dir = "Resource\\Worlds\\";
	DirIterator it((dir + "*.spg").c_str());
	while(it){
		MissionDescription md((dir + *it).c_str());
		if(md.isBattle())
			qsWorldsMgr.updateQSWorld(md.saveName(), (XGUID)md.missionGUID());
		++it;
	}

	qsWorldsMgr.save();
}

void CMainFrame::setSpeed(float speed) 
{ 
	syncroTimer.setSpeed(speed);
}

void CMainFrame::OnLibrariesTerrraintypename()
{
	TerrainTypeDescriptor::instance().editLibrary();
	vMap.WorldRender();
}

void CMainFrame::OnEditRollingborder()
{
	// TODO: Add your command handler code here
	DlgBorderRolling dlgBorderRolling;
	if(dlgBorderRolling.DoModal() == IDOK) {
		vMap.autoLace(dlgBorderRolling.borderHeight*VOXEL_MULTIPLIER, (float)dlgBorderRolling.boderAngle*M_PI/180.);
	}
}
