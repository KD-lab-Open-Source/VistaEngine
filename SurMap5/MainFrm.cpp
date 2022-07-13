#include "StdAfx.h"
#include "SurMap5.h"

#include "MainFrm.h"

#include "..\Environment\Environment.h"
#include "EditArchive.h"
#include "..\Units\UnitAttribute.h"
#include "..\Units\IronBullet.h"
#include "..\Ai\PlaceOperators.h"

#include "DlgCreateWorld.h"
#include "SelectFolderDialog.h"

#include <string>

#include "DlgSelectWorld.h"
#include "DlgChangeTotalWorldHeight.h"

#include "UnitEditorDlg.h"
#include "WorldPropertiesDlg.h"
#include "FileLibraryEditorDlg.h"
#include "FormationsEditorDlg.h"
#include "SurToolSelect.h"

#include "..\Game\Universe.h"

#include "..\UserInterface\UserInterface.h"

#include "SurToolAux.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "Triggers.h"
#include "..\Game\SoundTrack.h"
#ifndef _VISTA_ENGINE_EXTERNAL_
#include "..\TriggerEditor\TriggerEditor.h"
#endif
#include "SystemUtil.h"
#include "..\UserInterface\Controls.h"

#include "DlgSelectTrigger.h"

#include "..\AttribEditor\AttribEditorDlg.h"
#include "..\AttribEditor\Serializeable.h"

#include "..\Terra\terTools.h"
#include "..\version.h"

#include "TypeLibraryImpl.h"
#include "..\UserInterface\UI_Types.h"
#include "GameOptions.h"

//#include "DlgStatisticsShow.h"
#include "..\Util\ObjStatistic.h"

#include "..\Terra\vmap4vi.h"

#include "ExcelImEx.h"
#include "..\Util\TextDB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern void UpdateRegionMap(int x1,int y1,int x2,int y2);
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
//	ON_MESSAGE(CExtControlBar::g_nMsgConstructPopupMenu, OnConstructPopupMenu)

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
	ON_COMMAND_EX(IDR_WORKSPACE_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(IDR_WORKSPACE_BAR, OnUpdateControlBarMenu)
	
	ON_COMMAND(ID_VIEW_TREE_BAR, OnViewTreeBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TREE_BAR, OnUpdateViewTreeBar)
	ON_COMMAND(ID_VIEW_DLGBAR, OnViewProperties)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DLGBAR, OnUpdateViewProperties)
	ON_COMMAND(ID_VIEW_MINIMAP, OnViewMinimap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MINIMAP, OnUpdateViewMinimap)
	ON_COMMAND(ID_VIEW_OBJECTS_MANAGER, OnViewObjectsManager)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OBJECTS_MANAGER, OnUpdateViewObjectsManager)

	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_FILE_SAVEAS, OnFileSaveas)
	ON_COMMAND(ID_EDIT_PLAYPMO, OnEditPlaypmo)
	ON_COMMAND(ID_DEBUG_FILLIN, OnDebugFillIn)
	ON_COMMAND(ID_EDIT_MAPSCENARIO, OnEditMapScenario)
	ON_COMMAND(ID_EDIT_TRIGGERS, OnEditTriggers)
	ON_COMMAND(ID_EDIT_UNITS, OnEditUnits)
	ON_COMMAND(ID_VIEW_EXTENDEDMODETREELBAR, OnViewExtendedmodetreelbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EXTENDEDMODETREELBAR, OnUpdateViewExtendedmodetreelbar)
	ON_COMMAND(ID_DEBUG_SAVECONFIG, OnDebugSaveconfig)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_EDIT_UNIT_TREE, OnEditUnitTree)
	ON_COMMAND(ID_EDIT_USER_INTERFACE, OnEditUserInterface)
	ON_COMMAND(ID_EDIT_OBJECTS, OnEditObjects)

	ON_COMMAND(ID_VIEW_RESETTOOLBAR2DEFAULT, OnViewResettoolbar2default)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_WATERSOURCE, OnViewWaterSource)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WATERSOURCE, OnUpdateViewWaterSource)
	ON_COMMAND(ID_VIEW_BUBBLESOURCE, OnViewBubbleSource)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BUBBLESOURCE, OnUpdateViewBubbleSource)
	ON_COMMAND(ID_VIEW_WINDSOURCE, OnViewWindSource)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WINDSOURCE, OnUpdateViewWindSource)
	ON_COMMAND(ID_VIEW_LIGHTSOURCE, OnViewLightSource)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIGHTSOURCE, OnUpdateViewLightSource)
	ON_COMMAND(ID_FILE_SAVEMINIMAPTOWORLD, OnFileSaveminimaptoworld)
	ON_COMMAND(ID_FILE_RUN_WORLD, OnFileRunWorld)
	ON_COMMAND(ID_EDIT_GAME_SCENARIO, OnEditGameScenario)
	ON_COMMAND(ID_VIEW_BREAKSOURCE, OnViewBreakSource)

	ON_UPDATE_COMMAND_UI(ID_VIEW_BREAKSOURCE, OnUpdateViewBreakSource)
	ON_COMMAND(ID_EDIT_REBUILDWORLD, OnEditRebuildworld)
	ON_COMMAND(ID_VIEW_SETTINGS, OnViewSettings)
	ON_COMMAND(ID_EDIT_EFFECTS, OnEditEffects)
	ON_COMMAND(ID_FILE_RUN_MENU, OnFileRunMenu)
	ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
	ON_COMMAND(ID_DEBUG_SAVEDICTIONARY, OnDebugSaveDictionary)
	ON_WM_SHOWWINDOW()
	ON_COMMAND(ID_EDIT_TOOLZERS, OnEditToolzers)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_COMMAND(ID_EDIT_SOUNDS, OnEditSounds)
	ON_COMMAND(ID_EDIT_VOICES, OnEditVoices)
	ON_COMMAND(ID_FILE_EXPORTVISTAENGINE, OnFileExportVistaEngine)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_DEBUG_SHOWPALETTETEXTURE, OnDebugShowpalettetexture)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_SHOWPALETTETEXTURE, OnUpdateDebugShowpalettetexture)
	ON_COMMAND(ID_EDIT_HEADS, OnEditHeads)
	ON_COMMAND(ID_EDIT_SOUND_TRACKS, OnEditSoundTracks)
	ON_COMMAND(ID_EDIT_REELS, OnEditReels)
	ON_COMMAND(ID_EDIT_GLOBAL_TRIGGER, OnEditGlobalTrigger)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EFFECTS, OnUpdateViewEffects)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAMERAS, OnUpdateViewCameras)
	ON_COMMAND(ID_VIEW_EFFECTS, OnViewEffects)
	ON_COMMAND(ID_VIEW_CAMERAS, OnViewCameras)
	ON_COMMAND(ID_EDIT_EFFECTS_EDITOR, OnEditEffectsEditor)
	ON_COMMAND(ID_EDIT_TERTOOLS, OnEditTertools)
	ON_COMMAND(ID_EDIT_CURSORS, OnEditCursors)
	ON_COMMAND(ID_EDIT_DEFAULT_ANIMATION_CHAINS, OnEditDefaultAnimationChains)
	ON_COMMAND(ID_EDIT_FORMATIONS, OnEditFormations)
	ON_COMMAND(ID_DEBUG_EDIT_DEBUG_PRM, OnDebugEditDebugPrm)

	ON_COMMAND(ID_DEBUG_EDIT_AUX_ATTRIBUTE, OnDebugEditAuxAttribute)
	ON_COMMAND(ID_DEBUG_EDIT_RIGID_BODY_PRM, OnDebugEditRigidBodyPrm)
	ON_COMMAND(ID_DEBUG_EDIT_TOOLZER, OnDebugEditToolzer)
	ON_COMMAND(ID_DEBUG_EDIT_EXPLODE_TABLE, OnDebugEditExplodeTable)
	ON_COMMAND(ID_DEBUG_EDIT_SOURCES_LIBRARY, OnDebugEditSourcesLibrary)
	ON_COMMAND(ID_DEBUG_SHOWMIPMAP, OnDebugShowmipmap)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_SHOWMIPMAP, OnUpdateDebugShowmipmap)
	ON_COMMAND(ID_EDIT_CHANGETOTALWORLDHEIGHT, OnEditChangetotalworldheight)
	ON_COMMAND(ID_VIEW_GEOSURFACE, OnViewGeosurface)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GEOSURFACE, OnUpdateViewGeosurface)
	ON_COMMAND(ID_EDIT_GAMEOPTIONS, OnEditGameOptions)
	ON_COMMAND(ID_FILE_STATISTICS, OnFileStatistics)
	ON_COMMAND(ID_VIEW_SHOW_GRID, OnViewShowGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_GRID, OnUpdateViewShowGrid)
	ON_COMMAND(ID_VIEW_ENABLE_TIME_FLOW, OnViewEnableTimeFlow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENABLE_TIME_FLOW, OnUpdateViewEnableTimeFlow)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEAS, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_STATISTICS, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_RUN_MENU, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_RUN_WORLD, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEMINIMAPTOWORLD, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MAPSCENARIO, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_EDIT_OBJECTS, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REBUILDWORLD, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_IMPORTTEXTFROMEXCEL, OnFileImportTextFromExcel)
	ON_COMMAND(ID_FILE_EXPORTTEXTTOEXCEL, OnFileExportTextToExcel)
	ON_COMMAND(ID_DEBUG_EDITSTRATEGY, OnDebugEditstrategy)
	ON_COMMAND(ID_DEBUG_UI_SPRITE_LIB, OnDebugUISpriteLib)
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
	static XBuffer errorHeading;
	errorHeading.SetRadix(16);
	errorHeading < ("VistaEngine: " VISTA_ENGINE_VERSION " (" __DATE__ " " __TIME__ ")");
	ErrH.SetPrefix(errorHeading);

	::memset( &m_dataFrameWP, 0, sizeof(WINDOWPLACEMENT) );
	m_dataFrameWP.length = sizeof(WINDOWPLACEMENT);
	m_dataFrameWP.showCmd = SW_HIDE;

	m_wndView.init(&toolsWindow_);

	lastTime = 0;
	syncroTimer.set(1, logicTimePeriod, 300);
	memset(&statisticsAttr,1,sizeof(statisticsAttr));
	pObjStatistic = NULL;
}

CMainFrame::~CMainFrame()
{
	if(pObjStatistic != NULL){
		delete pObjStatistic;
		pObjStatistic = NULL;
	}
}

void CMainFrame::resetWorkspace()
{
	miniMapBar_.FloatControlBar(CPoint( 100, 100 ));
	objectsManagerBar_.FloatControlBar(CPoint( 100, 200 ) );
	propertiesBar_.FloatControlBar( CPoint( 100, 300 ) );
	toolsWindowBar_.FloatControlBar(CPoint( 100, 400 ) );
	
	menuBar_.FloatControlBar(CPoint(0, -30));
	toolBar_.FloatControlBar(CPoint(0, 0));
	librariesBar_.FloatControlBar(CPoint(100, 0));
	workspaceBar_.FloatControlBar(CPoint(200, 0));
	editorsBar_.FloatControlBar(CPoint(300, 0));
	filtersBar_.FloatControlBar(CPoint(400, 0));

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

	RecalcLayout();

	toolBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
	workspaceBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
	librariesBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
	editorsBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, 0, this, false);
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

	if (!m_wndView.Create(0, 0, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL)){
		AfxMessageBox ("Failed to create view window\n");
		return -1;
	}
	if (!menuBar_.Create("Menu Bar", this, ID_VIEW_MENUBAR) ||
		!menuBar_.LoadMenuBar(external ? IDR_MAINFRAME_EXTERNAL : IDR_MAINFRAME)){
		AfxMessageBox ("Failed to create view window\n");
		return -1;
	}
	menuBar_.EnableDocking(CBRS_ALIGN_ANY);

	if(!toolBar_.CreateExt(this)){
		AfxMessageBox ("Failed to create toolbar\n");
		return -1;
	}
	//toolBar_.setBrushForm(static_cast<e_BrushForm>(userInterfaceOption.lastToolzerForm));
	toolBar_.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(editorsBar_.Create("Editors Bar", this, IDR_EDITORS_BAR)
		&& editorsBar_.LoadToolBar(external ? IDR_EDITORS_EXTERNAL_BAR : IDR_EDITORS_BAR));
	editorsBar_.EnableDocking(CBRS_ALIGN_ANY);
	
	VERIFY(librariesBar_.Create("Libraries Bar", this, IDR_LIBRARIES_BAR)
		&& librariesBar_.LoadToolBar(external ? IDR_LIBRARIES_EXTERNAL_BAR : IDR_LIBRARIES_BAR));
	librariesBar_.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(filtersBar_.Create("Filters Bar", this, IDR_FILTERS_BAR)
		&& filtersBar_.LoadToolBar(IDR_FILTERS_BAR));
	       filtersBar_.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(workspaceBar_.Create("Workspace Bar", this, IDR_WORKSPACE_BAR)
		&& workspaceBar_.LoadToolBar(IDR_WORKSPACE_BAR));
	       workspaceBar_.EnableDocking(CBRS_ALIGN_ANY);


	if (!statusBar_.Create(this)) {
		AfxMessageBox ("Failed to create status bar\n");
		return -1;
	}
	for(int i = 0; i < NUMBERS_PARTS_STATUSBAR; ++i){
		statusBar_.AddPane(IDS_PANE_TEXT + i, i);
		statusBar_.SetPaneStyle(i, SBPS_NORMAL);
		if(i == 2){
			progressBar_.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP, CRect(0, 0, 0, 0), &statusBar_, 0);
			statusBar_.AddPaneControl(&progressBar_, IDS_PANE_TEXT + i, false);
		}
	}
	
	if (!toolsWindowBar_.Create(NULL/*_T("Optional control bar caption")*/, this, ID_VIEW_TREE_BAR)) {
		AfxMessageBox ("Failed to create toolsWindowBar_\n");
		return -1;
	}
	toolsWindowBar_.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(toolsWindow_.Create(this, &toolsWindowBar_));
	toolsWindow_.setBrushRadius(userInterfaceOption.lastToolzerRadius);

    //////////////////////////////////////////////////////////////////////////////
	if (!miniMapBar_.Create (0, this, ID_VIEW_MINIMAP)) {
		AfxMessageBox ("Failed to create miniMapBar_\n");
		return -1;
	}

	if(!miniMap_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect (0, 0, 100, 100), &miniMapBar_)){
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

	if(!objectsManager_.Create(WS_CHILD | WS_VISIBLE, CRect (0, 0, 100, 100), &objectsManagerBar_)) {
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

	m_dlgCamera.Create(CCameraDlg::IDD, 0);
	m_dlgWave.Create(CWaveDlg::IDD,0);
	// PostMessage(WM_COMMAND, ID_FILE_NEW);
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
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
//	if (m_baseTreeDlg.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo)) 
//		return TRUE;
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


BOOL CMainFrame::DestroyWindow() 
{
	VERIFY( saveDlgBarState() );

	userInterfaceOption.lastToolzerRadius = toolsWindow_.getBrushRadius();
	//userInterfaceOption.lastToolzerForm = //toolBar_.getBrushForm();

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
	SetWindowText (strAppName + " - " + vMap.worldName.c_str());
}

//////////////////////////////////////////////
#include <Windows.h>
#include <shlobj.h>
#include ".\mainfrm.h"
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

string SelectWorkinFolder2(const char* initialDir, CWnd* pParentWnd = NULL)
{
	CString cstrFilters = "";

	CString cstrInitialFolder = initialDir;

	char bufDir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, bufDir);
	CSelectFolderDialog selFolder(false, cstrInitialFolder, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, cstrFilters, pParentWnd);
	string retval;
	if(selFolder.DoModal() == IDOK) {
		///AfxMessageBox("The selected folder is\n\n" + selFolder.GetSelectedPath());
		retval=selFolder.GetSelectedPath();
	}
	::SetCurrentDirectory(bufDir);
	return retval;
}

static bool flag_active_app = true;

bool CMainFrame::universeQuant()
{
	if(flag_active_app && universe()){
		syncroTimer.next_frame();
		if(lastTime < syncroTimer()){
			do{
				lastTime += logicTimePeriod;
				gb_VisGeneric->SetLogicQuant(universe()->quantCounter()+2);
				universe()->Quant();
				universe()->clearDeletedUnits(false);
				//gbGeneralView->Draw();// НЕ ДЕЛАЙТЕ ТАК!!!(Для тех кто не знаком с событийной системой Win32- Invalidate это вызов OnPaint!)
				logicFps.quant();
				m_wndView.quant();
				m_wndView.Invalidate(FALSE);
			}while(lastTime < syncroTimer());
		}
		else 
			m_wndView.Invalidate(FALSE);

		m_wndView.Invalidate(FALSE);
		if (miniMap_.IsWindowVisible())
			miniMap_.Invalidate(FALSE);
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
}


void CMainFrame::OnDebugFillIn()
{
	toolsWindow_.resetCurrentTool();
	toolsWindow_.getTree().BuildTree();
}

void serializeViewSettings (Archive& ar)
{
	
}

bool test_data[10] = { false, true, false, true, false,
					   true, false, true, false, true };

struct MapScenarioSerialization {
	MissionDescription* mission_;
	PlayerDataVect* playerDataVect_;

	MapScenarioSerialization() {
		mission_ = 0;
		playerDataVect_ = 0;
	}
	void setMissionAndPlayerData(MissionDescription& mission, PlayerDataVect& playerDataVect) {
		mission_ = &mission;
		playerDataVect_ = &playerDataVect;
	}

	void serialize(Archive& ar) {
		xassert(playerDataVect_ && mission_);
		PlayerDataVect& players = *playerDataVect_;
		MissionDescription& mission = *mission_;

		mission.serialize(ar);

		static_cast<vrtMap&>(vMap).serializeParameters (ar);
		if(environment)
			environment->serializeParameters(ar);
		
		ar.openBlock("colors", "Цвета");
		if(environment)
			environment->serializeColors (ar);
		ar.closeBlock ();

		ar.serialize(mission.activePlayerID, "activePlayerID", "Активный игрок");
		mission.activePlayerID = min(mission.activePlayerID, mission.playersAmountMax() - 1);

		ar.serialize(players, "players", "Игроки");
	}
};

struct GameScenarioSerialization {
	MissionDescription* mission_;

	GameScenarioSerialization()
		: mission_(0) {}

	void setMission(MissionDescription& mission) {
		mission_ = &mission;
	}


	void serialize(Archive& ar){
		xassert(mission_);
		MissionDescription& mission = *mission_;
		ar.serialize(GlobalAttributes::instance().showSilhouette, "showSilhouette", "Включить поддержку силуэтов");
		ar.serialize(GlobalAttributes::instance().lod12, "lod12", "Переход от lod1 к lod2");
		ar.serialize(GlobalAttributes::instance().lod23, "lod23", "Переход от lod2 к lod3");
		ar.serialize(GlobalAttributes::instance().enablePathTrackingRadiusCheck, "enablePathTrackingRadiusCheck", "PathTracking:Большые юниты игнорируют меньших");
		ar.serialize(GlobalAttributes::instance().enemyRadiusCheck, "enemyRadiusCheck", "PathTracking:Враги игнорируют меньших");
		ar.serialize(GlobalAttributes::instance().minRadiusCheck, "minRadiusCheck", "PathTracking:Минимальный игнорируемый радиус");
		ar.serialize(GlobalAttributes::instance().enableMakeWay, "enableMakeWay", "PathTracking:Уступать дорогу");
		ar.serialize(GlobalAttributes::instance().enableEnemyMakeWay, "enableEnemyMakeWay", "PathTracking:Уступать дорогу врагу");
		ar.serialize(GlobalAttributes::instance().analyzeAreaRadius, "analyzeAreaRadius", "Радиус для точного анализа поверхности");
		ar.serialize(GlobalAttributes::instance().enableAutoImpassability, "enableAutoImpassability", "Включить автоматическую генерацию зон непроходимости");
		ar.serialize(GlobalAttributes::instance().cameraRestriction_, "cameraRestriction", "Ограничения камеры");

		RaceTable::instance().serialize(ar);
		ar.TRANSLATE_NAME(GlobalAttributes::instance().playerColors,     "playerColors",     "Цвета игроков"); 
		ar.TRANSLATE_NAME(GlobalAttributes::instance().playerSigns,      "playerSigns",      "Эмблемы игроков"); 
		ar.serialize(GlobalAttributes::instance().silhouetteColors, "silhouetteColors", "Цвета силуэтов");
		ar.serialize(GlobalAttributes::instance().uniformCursor, "uniformCursor", "Курсоры действий показывать если все в селекте могут");
		ar.serialize(GlobalAttributes::instance().languagesList, "languagesList", "Доступные языки");

		DifficultyTable::instance().serialize(ar);

		ar.serialize(GlobalAttributes::instance().treeLyingTime, "treeLyingTime", "Время, которое деревья лежат после падения");
		
		AttributeLibrary::instance(); // Циклическая зависимость AttributeLegionary<->AttributeSquad
		AttributeSquadTable::instance().serialize(ar);
		
		UI_Dispatcher::instance().serializeVistaEnginePrm(ar);
		if(ar.isInput()){
			gb_VisGeneric->SetLodDistance(GlobalAttributes::instance().lod12, GlobalAttributes::instance().lod23);
		}

		ar.openBlock("Controls", "Управление");
		ControlManager::instance().serialize(ar);
		ar.closeBlock();
	}
};

void CMainFrame::OnEditMapScenario()
{
	static PlayerDataVect players;
	players.clear();

	if(universe())
		universe()->exportPlayers(players);
	else{
		players.push_back(PlayerDataEdit());
		players.push_back(PlayerDataEdit());
	}

	static MapScenarioSerialization mapScenario;

	mapScenario.setMissionAndPlayerData(m_wndView.currentMission, players);

	Serializeable serializeable(mapScenario, "mapScenario", Dictionary::instance().translate("Сценарий карты"));
	TreeControlSetup treeControlSetup(0, 0, 640, 480, "Scripts\\TreeControlSetups\\MapScenarioNew", false, true);

	CAttribEditorDlg dlg;
	if(dlg.edit(serializeable, GetSafeHwnd(), treeControlSetup, "Save", "Close")){ 
		SingletonPrm<GlobalAttributes>::save();
		SingletonPrm<TextIdMap>::save();
	}

	CSurToolSelect::objectChangeNotify();

	if(universe()){
		universe()->importPlayers(players);
		setSilhouetteColors();
		OnDebugFillIn();
	}
}

void CMainFrame::OnEditTriggers()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	CDlgSelectTrigger dlgST("Scripts\\Content\\Triggers", "Select triggers");
	int nRet=dlgST.DoModal();
	if( nRet==IDOK ) {
		if(!dlgST.selectTriggersFile.empty()){
			static TriggerEditor triggerEditor(triggerInterface());
			if(!triggerEditor.isOpen()){
				TriggerChain triggerChain;
				triggerChain.load(setExtention((string("Scripts\\Content\\Triggers\\") + dlgST.selectTriggersFile).c_str(), "scr").c_str());
				if(triggerEditor.run(triggerChain, GetSafeHwnd ())){
					triggerChain.initializeTriggersAndLinks();
					triggerChain.save();
					SingletonPrm<TextIdMap>::save();
				}
			}
		}
		else {
			AfxMessageBox(Dictionary::instance().translate("Ошибка, файл не выбран!"));
		}
	}

#else // _VISTA_ENGINE_EXTERNAL_

	CString message (Dictionary::instance().translate("Недоступно во внешней версии"));
	MessageBox (message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_
}

void CMainFrame::OnEditUnits()
{
	CUnitEditorDlg dlg;
	dlg.DoModal ();
	toolsWindow_.resetCurrentTool();
	toolsWindow_.getTree().BuildTree ();
}

/////////////////////////////////////////////////////
void CMainFrame::OnViewExtendedmodetreelbar()
{
	if(flag_TREE_BAR_EXTENED_MODE)flag_TREE_BAR_EXTENED_MODE=0;
	else flag_TREE_BAR_EXTENED_MODE=1;
	toolsWindow_.resetCurrentTool();
	toolsWindow_.getTree().BuildTree ();
}

void CMainFrame::OnUpdateViewExtendedmodetreelbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(flag_TREE_BAR_EXTENED_MODE);
}

void CMainFrame::OnDebugSaveconfig()
{
	toolsWindow_.getTree().save();
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

void CMainFrame::OnFileNew()
{
	CDlgCreateWorld dlgCW;
	int nRet = -1;
	nRet = dlgCW.DoModal();
	CWaitCursor wait;
	switch (nRet) {
	case IDOK:
		{
			m_wndView.createScene();
			static_cast<vrtMapCreationParam&>(vMap) = dlgCW.m_CreationParam;
			

			std::string pathToWorldData = vMap.getWorldsDir();
			pathToWorldData += vMap.getWorldsDir();
			pathToWorldData += "\\";
			pathToWorldData += dlgCW.m_strWorldName;
			pathToWorldData += "\\";
			pathToWorldData += vrtMap::worldDataFile;

			
			if(fileExists(pathToWorldData.c_str())){
				MessageBox(Dictionary::instance().translate("Мир с таким именем уже существует, создание невозможно!"), 0, MB_OK);
				return;
			}

			vMap.create(dlgCW.m_strWorldName);
			m_wndView.reInitWorld();
			Invalidate(FALSE);
			put2TitleNameDirWorld();
			toolsWindow_.resetCurrentTool();
			toolsWindow_.getTree().BuildTree ();
		}
		break;
	}
	Invalidate(FALSE);
}

void CMainFrame::OnFileOpen()
{
	CWaitCursor wait;

	CDlgSelectWorld dlgSW(vMap.getWorldsDir(), Dictionary::instance().translate("Выбор мира для загрузки"), false);
	int nRet=dlgSW.DoModal();
	if( nRet==IDOK ) {
		XBuffer patch2worlddata;
		patch2worlddata< vMap.getWorldsDir() < "\\" < dlgSW.selectWorldName.c_str() < "\\" < vrtMap::worldDataFile;
		if( (!dlgSW.selectWorldName.empty()) && (testExistingFile(patch2worlddata)) ){
			wait.Restore();
			m_wndView.createScene();
			vMap.load(dlgSW.selectWorldName.c_str(), true);
			m_wndView.reInitWorld();
			loadParameters(GlobalAttributes::instance());
			put2TitleNameDirWorld();
			Invalidate(FALSE);
		}
		else{
			XBuffer str;
			str < "World: " < dlgSW.selectWorldName.c_str() < "  is empty";
			AfxMessageBox(str);
		}

	}
	CSurToolSelect::objectChangeNotify();
	toolsWindow_.resetCurrentTool();
	toolsWindow_.getTree().BuildTree();
}

void CMainFrame::OnFileSave()
{
	if(vMap.worldName.empty()){
		OnFileSaveas();
	}
	else{
		save(vMap.worldName.c_str());
	}
}

void CMainFrame::save(const char* worldName)
{
	CWaitCursor wait;
	getToolWindow().resetCurrentTool();
	int idxColorCnt=vMap.convertVMapTryColor2IdxColor();
	vMap.save(worldName, idxColorCnt);
	vMap.saveMiniMap(128, 128);
	m_wndView.currentMission.setByWorldName(worldName);
	SingletonPrm<GlobalAttributes>::save();
	universe()->universalSave(m_wndView.currentMission, SAVE_TYPE_INITIAL);
	miniMap_.onWorldChanged ();
}

void CMainFrame::OnFileSaveas()
{
	CWaitCursor wait;
	CDlgSelectWorld dlgSW(vMap.getWorldsDir(), Dictionary::instance().translate ("Выбор мира для записи"), true);
	int nRet=dlgSW.DoModal();
	if( nRet==IDOK ) {
		if(!dlgSW.selectWorldName.empty()){
			wait.Restore();
			save(dlgSW.selectWorldName.c_str());
			put2TitleNameDirWorld();
		}
		else {
			AfxMessageBox(Dictionary::instance().translate ("Ошибка записи мира!"));
		}
	}
}

void CMainFrame::OnFileSaveminimaptoworld()
{
	if(vMap.worldName.empty()){
		AfxMessageBox("Не выбран мир");
	}
	else{
		vMap.saveMiniMap(128,128);
	}
}


void CMainFrame::OnEditUnitTree()
{
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\units"));
	ea.setTranslatedOnly(true);
	SingletonPrm<AttributeLibrary>::edit(ea);
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

	CString message (Dictionary::instance().translate("Недоступно во внешней версии"));
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
//	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewMinimap()
{
	toggleControlBar(miniMapBar_);
}

void CMainFrame::OnUpdateViewMinimap(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(miniMap_.IsWindowVisible() != 0);
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
	if (m_wndView.isSceneInitialized() && vMap.IsChanged()){
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
static int xSizePartsStatusBar[NUMBERS_PARTS_STATUSBAR]={ 80+20, 70+30, 100, 00, 70, 70, 00, 50, 50, 280};
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
		userInterfaceOption.dlgBarState.resize(fLenght);
		_file.Read(&userInterfaceOption.dlgBarState[0], fLenght);

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
		if(userInterfaceOption.dlgBarState.empty()){
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
			_file.Write(&userInterfaceOption.dlgBarState[0], userInterfaceOption.dlgBarState.size());
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


void CMainFrame::OnViewWaterSource()
{
	Environment::setSourceVisible(SourceBase::SOURCE_WATER, !Environment::isSourceVisible(SourceBase::SOURCE_WATER));
	Environment::flag_ViewWaves = Environment::isSourceVisible(SourceBase::SOURCE_WATER);
}

void CMainFrame::OnUpdateViewWaterSource(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::isSourceVisible(SourceBase::SOURCE_WATER));
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewBubbleSource()
{
	Environment::setSourceVisible(SourceBase::SOURCE_BUBBLE, !Environment::isSourceVisible(SourceBase::SOURCE_BUBBLE));
}

void CMainFrame::OnUpdateViewBubbleSource(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::isSourceVisible(SourceBase::SOURCE_BUBBLE));
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewWindSource()
{

}

void CMainFrame::OnUpdateViewWindSource(CCmdUI *pCmdUI)
{

}

void CMainFrame::OnViewLightSource()
{
	Environment::setSourceVisible(SourceBase::SOURCE_LIGHT, !Environment::isSourceVisible(SourceBase::SOURCE_LIGHT));
}

void CMainFrame::OnUpdateViewLightSource(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::isSourceVisible(SourceBase::SOURCE_LIGHT));
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewBreakSource()
{
	Environment::setSourceVisible(SourceBase::SOURCE_BREAK, !Environment::isSourceVisible(SourceBase::SOURCE_BREAK));
	Environment::setSourceVisible(SourceBase::SOURCE_GEOWAVE, Environment::isSourceVisible(SourceBase::SOURCE_BREAK));
}

void CMainFrame::OnUpdateViewBreakSource(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::isSourceVisible(SourceBase::SOURCE_BREAK));
	pCmdUI->Enable(vMap.isWorldLoaded());
}


void CMainFrame::OnFileRunWorld()
{
	const char* TEMP_WORLD="TMP";
	if(!universe())
		return;
	string currentWorldName = vMap.worldName;
	save(TEMP_WORLD);

	vMap.worldName = currentWorldName;
	m_wndView.currentMission.setByWorldName(currentWorldName.c_str());

#ifdef _DEBUG
	const char* game_path = "GameDBG.exe"; 
#else
	const char* game_path = "Game.exe"; 
#endif

	bool flag_animation = static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation;
	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = false;
	ShowWindow (SW_HIDE);
	Sleep (200);

	int result = _spawnl (_P_WAIT /*_P_NOWAIT*/, game_path, game_path, "openResource\\Worlds\\TMP.spg", NULL);
	if (result < 0)
	{
        CString message (Dictionary::instance().translate("Не могу запустить игру!"));
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
	static GameScenarioSerialization gameScenario;
	gameScenario.setMission(m_wndView.currentMission);

	Serializeable serializeable(gameScenario, "gameScenario", Dictionary::instance().translate("Сценарий игры"));

    CAttribEditorDlg dlg;

	TreeControlSetup treeControlSetup(0, 0, 640, 480, "Scripts\\TreeControlSetups\\GameScenario");

	if(dlg.edit(serializeable, GetSafeHwnd(), treeControlSetup, "Save", "Close")){
		SingletonPrm<RaceTable>::save();
		SingletonPrm<DifficultyTable>::save();
		SingletonPrm<GlobalAttributes>::save();
		SingletonPrm<UnitFormationTypes>::save();
		SingletonPrm<FormationPatterns>::save();
		SingletonPrm<PlacementZoneTable>::save();
		SingletonPrm<AttributeSquadTable>::save();
		SingletonPrm<AttributeProjectileTable>::save();
		SingletonPrm<UI_Dispatcher>::save();
		SingletonPrm<AttributeLibrary>::save();
		SingletonPrm<ControlManager>::save();
		SingletonPrm<TextIdMap>::save();
	}
	m_wndView.setGraphicsParameters();

	/*
    const TreeNode* rootNode = dlg.Edit (oarchive.rootNode (), GetSafeHwnd (),
                                             TreeControlSetup (0, 0, 640, 480,
                                                               "Scripts\\TreeControlSetups\\GameScenario")))
															   */
	CSurToolSelect::objectChangeNotify();

#else // _VISTA_ENGINE_EXTERNAL_

	CString message (Dictionary::instance().translate("Недоступно во внешней версии"));
	MessageBox (message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_

}


void CMainFrame::OnEditRebuildworld()
{
	if(vMap.isLoad()){
		CWaitCursor wait;

		unsigned long vxacrc=vMap.getVxABufCRC();
		unsigned long geocrc=vMap.getGeoSurBufCRC();
		unsigned long damcrc=vMap.getDamSurBufCRC();
		unsigned int begTime=clocki();
		vMap.rebuild();
		extern unsigned int profileTimeOperation;
		profileTimeOperation=clocki()-begTime;
		m_wndView.UpdateStatusBar();
		if(vxacrc!=vMap.getVxABufCRC())
			::AfxMessageBox(Dictionary::instance().translate("Сгенеренные высоты не совпадают с предыдущими"));
		if(geocrc!=vMap.getGeoSurBufCRC())
			::AfxMessageBox(Dictionary::instance().translate("Поверхность 3D текстуры(Geo) сгенеренного мира не совпадает с предыдущей"));
		if(damcrc!=vMap.getDamSurBufCRC())
			::AfxMessageBox(Dictionary::instance().translate("Поверхность текстуры(Dam) сгенеренного мира не совпадает с предыдущей"));

		m_wndView.reInitWorld();
		Invalidate(FALSE);
	}
}

void CMainFrame::OnViewSettings()
{
}

void CMainFrame::OnEditEffects()
{
	SingletonPrm<EffectContainerLibrary>::edit();
//	EffectReference::buildComboList();
//	for(EffectContainerLibrary::Map::iterator it = EffectContainerLibrary::instance().map().begin(); it != EffectContainerLibrary::instance().map().end(); ++it) {
//		if (it->second) {
//			it->second->loadLibrary();
//		}
//	}
	EffectReference::buildComboList();
}


void CMainFrame::OnEditPreferences ()
{
	EditOArchive oarchive;
	userInterfaceOption.serialize (oarchive);

    CAttribEditorDlg dlg;
    if (const TreeNode* rootNode = dlg.edit(oarchive.rootNode (), GetSafeHwnd (),
                                            TreeControlSetup (0, 0, 640, 480, "Scripts\\TreeControlSetups\\UserPreferences")))
	{
        EditIArchive iarchive (rootNode);
        userInterfaceOption.serialize (iarchive);

		CSurToolSelect::objectChangeNotify();
    }
	m_wndView.setGraphicsParameters ();
}

void CMainFrame::OnFileRunMenu()
{
#ifdef _DEBUG
	const char* game_path = "GameDBG.exe"; 
#else
	const char* game_path = "Game.exe"; 
#endif
	bool flag_animation = static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation;
	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = false;

	ShowWindow (SW_HIDE);
	Sleep (200);

	int result = _spawnl (_P_WAIT, game_path, game_path, "-mainmenu", NULL);
	if (result < 0)	{
		CString message (Dictionary::instance().translate ("Не могу запустить игру!"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}
	ShowWindow (SW_SHOW);

	static_cast<CSurMap5App*>(AfxGetApp ())->flag_animation = flag_animation;
}

void CMainFrame::OnDebugSaveDictionary()
{
	SingletonPrm<Dictionary>::save();
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
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\terTools"));
	TerToolsLibrary::instance().serialize(static_cast<EditOArchive&>(ea));
	if(ea.edit()){
		TerToolsLibrary::instance().serialize(static_cast<EditIArchive&>(ea));
		SingletonPrm<TerToolsLibrary>::save();
	}
}

void CMainFrame::OnFileProperties()
{
	CWorldPropertiesDlg dlg;
	dlg.DoModal ();
}

void CMainFrame::OnEditSounds()
{
	if (SingletonPrm<SoundAttributeLibrary>::edit()) {
		SingletonPrm<SoundAttributeLibrary>::save();
	}
}

void CMainFrame::OnEditVoices()
{
	if (SingletonPrm<VoiceAttributeLibrary>::edit()) {
		SingletonPrm<VoiceAttributeLibrary>::save();
	}
}

void CMainFrame::OnFileExportVistaEngine()
{
	const char* path = "ExportVistaEngine.bat"; 
	if (0 > system (path)) { 
		CString message (Dictionary::instance().translate ("Не могу запустить скрипт экспорта!"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.UndoDispatcher_IsUndoExist());
}

void CMainFrame::OnEditUndo()
{
	CWaitCursor wait;
	if(vMap.isLoad()){
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
	if(vMap.isLoad()){
		vMap.UndoDispatcher_Redo();
		Invalidate(FALSE);
	}
}

void CMainFrame::OnDebugShowpalettetexture()
{
	extern void UpdateRegionMap(int x1,int y1,int x2,int y2);

	if(vMap.isShowTryColorDamTexture()){
		CWaitCursor wait;
		//vMap.convertSupBuf2SurBuf();
		vMap.toShowTryColorDamTexture(false);
		UpdateRegionMap(0, 0, vMap.H_SIZE-1, vMap.V_SIZE-1);
	}
	else {
		vMap.toShowTryColorDamTexture(true);
		UpdateRegionMap(0, 0, vMap.H_SIZE-1, vMap.V_SIZE-1);
	}

}

void CMainFrame::OnUpdateDebugShowpalettetexture(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.isLoad());
	pCmdUI->SetCheck(!vMap.isShowTryColorDamTexture());
}

void CMainFrame::OnEditHeads()
{
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\heads"));
	GlobalAttributes::instance().serializeHeadLibrary(static_cast<EditOArchive&>(ea));
	if (ea.edit ()) {
		GlobalAttributes::instance().serializeHeadLibrary(static_cast<EditIArchive&>(ea));
	}
}

void CMainFrame::OnEditSoundTracks()
{
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\soundTracks"));
	SoundTrackTable::instance().serialize(static_cast<EditOArchive&>(ea));
	if(ea.edit()){
		SoundTrackTable::instance().serialize(static_cast<EditIArchive&>(ea));
		SingletonPrm<SoundTrackTable>::save();
	}
}

void CMainFrame::OnEditReels()
{
	CFileLibraryEditorDlg dlg (".\\Resource\\Video", "*.bik");
	dlg.DoModal ();
}

void CMainFrame::OnEditGlobalTrigger()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	static TriggerEditor triggerEditor(triggerInterface());
	if(!triggerEditor.isOpen()) {
		TriggerChain triggerChain;
		triggerChain.load("Scripts\\Content\\GlobalTrigger.scr");
		if(triggerEditor.run(triggerChain, GetSafeHwnd())){
			triggerChain.initializeTriggersAndLinks();
			triggerChain.save();
			SingletonPrm<TextIdMap>::save();
		}
	}
#else // _VISTA_ENGINE_EXTERNAL_
	CString message (Dictionary::instance().translate("Недоступно во внешней версии"));
	MessageBox (message, 0, MB_OK | MB_ICONERROR);
#endif // _VISTA_ENGINE_EXTERNAL_
}



void CMainFrame::OnViewEffects()
{
	Environment::flag_ViewEffects = !Environment::flag_ViewEffects;
}
void CMainFrame::OnUpdateViewEffects(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::flag_ViewEffects);
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewCameras()
{
	Environment::flag_ViewCameras = !Environment::flag_ViewCameras;
}
void CMainFrame::OnUpdateViewCameras(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::flag_ViewCameras);
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewGeosurface()
{
	if(vMap.isWorldLoaded()) {
		CWaitCursor wait;
		if(vMap.IsShowSpecialInfo()==vrtMap::SSI_ShowAllGeo){
			vMap.toShowSpecialInfo(vrtMap::SSI_NoShow);
			UpdateRegionMap(0, 0, vMap.H_SIZE-1, vMap.V_SIZE-1);
		}
		else {
			vMap.toShowSpecialInfo(vrtMap::SSI_ShowAllGeo);
			UpdateRegionMap(0, 0, vMap.H_SIZE-1, vMap.V_SIZE-1);
		}
	}
}
void CMainFrame::OnUpdateViewGeosurface(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(vMap.IsShowSpecialInfo()==vrtMap::SSI_ShowAllGeo);
	pCmdUI->Enable(vMap.isWorldLoaded());
}


void CMainFrame::OnEditEffectsEditor()
{
#ifndef _VISTA_ENGINE_EXTERNAL_
	const char* fxeditor_path = "EffectTool.exe"; 
	int result = _spawnl (_P_NOWAIT, fxeditor_path, fxeditor_path, NULL);
	if (result < 0)
	{
		CString message (Dictionary::instance().translate("Не могу запустить редактор эффектов (исполняемый файл не найден)"));
		MessageBox (message, 0, MB_OK | MB_ICONERROR);
	}

#else // _VISTA_ENGINE_EXTERNAL_

	CString message (Dictionary::instance().translate("Недоступно во внешней версии"));
	MessageBox (message, 0, MB_OK | MB_ICONERROR);

#endif // _VISTA_ENGINE_EXTERNAL_
}


void CMainFrame::OnEditCursors()
{
	if (SingletonPrm<UI_CursorLibrary>::edit()) {
		SingletonPrm<UI_CursorLibrary>::save();
	}
}

void CMainFrame::OnEditFormations()
{
	CFormationsEditorDlg dlg;
	dlg.DoModal ();

	SingletonPrm<UnitFormationTypes>::save();
	SingletonPrm<FormationPatterns>::save();
}

void CMainFrame::OnEditDefaultAnimationChains()
{
	void editDefaultAnimationChains();
	editDefaultAnimationChains();
}

void CMainFrame::OnDebugEditDebugPrm()
{
	debugPrm.edit();
}

void CMainFrame::OnDebugEditAuxAttribute()
{
	if (SingletonPrm<AuxAttributeLibrary>::edit()) {
		SingletonPrm<AuxAttributeLibrary>::save ();
    }
}

void CMainFrame::OnDebugEditRigidBodyPrm()
{
	if (SingletonPrm<RigidBodyPrmLibrary>::edit()) {
		SingletonPrm<RigidBodyPrmLibrary>::save ();
    }
}

void CMainFrame::OnDebugEditToolzer()
{
}

void CMainFrame::OnDebugEditExplodeTable()
{
	if (SingletonPrm<ExplodeTable>::edit()) {
		SingletonPrm<ExplodeTable>::save ();
    }
}

void CMainFrame::OnDebugEditSourcesLibrary()
{
	EditArchive ea(0, TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\sourcesLibrary"));
	SourcesLibrary::instance().serialize(static_cast<EditOArchive&>(ea));
	if (ea.edit ()) {
		SourcesLibrary::instance().serialize(static_cast<EditIArchive&>(ea));
		SingletonPrm<SourcesLibrary>::save();
	}
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

void CMainFrame::OnEditChangetotalworldheight()
{
	CDlgChangeTotalWorldHeight dlg;
	dlg.DoModal();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(menuBar_.TranslateMainFrameMessage(pMsg))
		return TRUE;
	
	return CFrameWnd::PreTranslateMessage(pMsg);
}


void CMainFrame::OnEditGameOptions()
{
	if(SingletonPrm<GameOptions>::edit())
		SingletonPrm<GameOptions>::save();
}

void CMainFrame::OnFileStatistics()
{
	//CDlgStatisticsShow statDlg;
	//statDlg.DoModal();
	if (pObjStatistic != NULL)
	{
		delete pObjStatistic;
		pObjStatistic = NULL;
	}
	pObjStatistic = new cObjStatistic();
}

void CMainFrame::OnViewToolbar()
{
	toggleControlBar(toolBar_);
}

void CMainFrame::OnUpdateViewToolbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(toolBar_.IsVisible());
}

void CMainFrame::OnViewShowGrid()
{
	userInterfaceOption.enableGrid_ = !userInterfaceOption.enableGrid_;
}

void CMainFrame::OnUpdateViewShowGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(userInterfaceOption.enableGrid_);
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnViewEnableTimeFlow()
{
	Environment::flag_EnableTimeFlow = !Environment::flag_EnableTimeFlow;
}

void CMainFrame::OnUpdateViewEnableTimeFlow(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(Environment::flag_EnableTimeFlow);
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(vMap.isWorldLoaded());
}

void CMainFrame::OnFileImportTextFromExcel()
{
	CFileDialog fileDlg (TRUE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = Dictionary::instance().translate("Открыть XLS файл...");

	if (fileDlg.DoModal () == IDOK) {
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();	
		ImportImpl excl_(path);
		excl_.importAllLangText(GlobalAttributes::instance().languagesList);
	}
	else 
		return;	
}

void CMainFrame::OnFileExportTextToExcel()
{
	CFileDialog fileDlg (FALSE, "*.xls", 0,
						 OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT,
						 "(*.xls)|*.xls||", this);
	fileDlg.m_ofn.lpstrTitle = Dictionary::instance().translate("Сохранить XLS файл...");

	if (fileDlg.DoModal () == IDOK) {
		CWaitCursor waitCursor;
		CString path = fileDlg.GetPathName();
		FILE* stream;
		if((stream = fopen(path, "r")) != NULL){
			fclose(stream);
			ImportImpl exclIm_(path);
			exclIm_.application()->openSheet();
			ExcelFileStruct structure;
			if (structure.check(exclIm_.application())) 
				exclIm_.importAllLangText(GlobalAttributes::instance().languagesList);
		}
        DeleteFile(path);
		ExportImpl excl_(path);
		excl_.exportAllLangText(TextIdMap::instance(), GlobalAttributes::instance().languagesList);
	}
	else
		return;
}

void CMainFrame::OnDebugEditstrategy()
{
	if(SingletonPrm<AiPrm>::edit())
		SingletonPrm<AiPrm>::save();	
}

void CMainFrame::OnDebugUISpriteLib()
{
	if(SingletonPrm<UI_ShowModeSpriteTable>::edit())
		SingletonPrm<UI_ShowModeSpriteTable>::save();	
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
