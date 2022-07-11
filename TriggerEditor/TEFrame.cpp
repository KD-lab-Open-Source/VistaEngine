#include "stdafx.h"
#include "TriggerEditorApp.h"
#include "TEFrame.h"
#include "TEEngine/MiniMapManager.h"
#include "resource.h"

#include "TriggerExport.h"
#include "TriggerEditor.h"
#include "TEEngine/TriggerEditorView.h"
#include "TEEngine/TriggerEditorLogic.h"
#include "TEEngine/Profiler//TriggerProfList.h"

#include "Tree/TETreeManager.h"
#include "Scale/ScaleMgr.h"
#include "XPrmArchive.h"
#include "EditArchive.h"
#include "TreeInterface.h"
#include ".\teframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TEFrame

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

IMPLEMENT_DYNCREATE(TEFrame, CFrameWnd)

TEFrame::TEFrame(TriggerEditor* triggerEditor):
  hParentWnd_(NULL)
, treeManager_(NULL)
, triggerEditorView_(NULL)
, triggerEditor_(triggerEditor)
{
}

TEFrame::TEFrame():
  hParentWnd_(NULL)
, treeManager_(NULL)
, triggerEditorView_(NULL)
, triggerEditor_(NULL)
{
	ZeroMemory(&windowPlacement_, sizeof(windowPlacement_));
	windowPlacement_.length = sizeof(windowPlacement_);
	windowPlacement_.showCmd = SW_HIDE;
	assert(triggerEditor_);//его необходимо задать в другом месте
}

TEFrame::~TEFrame()
{
}

void TEFrame::setParent(HWND hparent)
{
	hParentWnd_ = hparent;
}

HWND TEFrame::getParent() const{
	return hParentWnd_;
}

void TEFrame::setTETreeManager(TETreeManager* mngr){
	treeManager_ = mngr;
}

TETreeManager* TEFrame::getTETreeManager() const{
	return treeManager_;
}
				   
TriggerProfList* TEFrame::getTriggerProfList() const{
	return triggerProfList_.get();
}

static UINT WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(TEFrame, CFrameWnd)
	//{{AFX_MSG_MAP(TEFrame)
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, onUpdateFileSave)
//	ON_COMMAND(ID_VIEW_PROFILER, onViewProfiler)
//	ON_UPDATE_COMMAND_UI(ID_VIEW_PROFILER, onUpdateViewProfiler)

	ON_COMMAND(ID_VIEW_TOOLBAR, onViewToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, onUpdateViewToolbar)

	ON_COMMAND(ID_VIEW_PROJECT_TREE, onViewTree)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROJECT_TREE, onUpdateViewTree)

	ON_COMMAND(ID_VIEW_MINIMAP, onViewMiniMap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MINIMAP, onUpdateViewMiniMap)

	ON_REGISTERED_MESSAGE(WM_FINDREPLACE, onFindReplace)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TEFrame message handlers

void TEFrame::OnClose() 
{
	saveWorkspace();
//	SaveBarState(CString((LPTSTR)IDS_REG_SEC_NAME_TOOLBARS));

	if (!triggerEditor_->isDataSaved())
	{
		int res = 0;
		res = AfxMessageBox(IDS_SAVE_PROJ_MSG, MB_YESNOCANCEL);
		if(res == IDYES)
		{
			triggerEditor_->save();
		}
		else if(res == IDCANCEL)
			return ;
	}

	triggerProfList_->hide();
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
		SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

//	::EnableWindow(hParentWnd_, TRUE);
//	::SetActiveWindow(hParentWnd_);
	
	CFrameWnd::OnClose();
}

void TEFrame::PostNcDestroy() 
{
	AfxPostQuitMessage(0);
}

int TEFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	

	const int SCALE_TOOLBAR_ID	= 2;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	const char* profileName = AfxGetApp()->m_pszProfileName;
		g_PaintManager.InstallPaintManager(
		new CExtPaintManagerXP()
	);

	// Popup menu option: Display menu shadows
	CExtPopupMenuWnd::g_bMenuWithShadows = true;
	// Popup menu option: Display menu cool tips
	CExtPopupMenuWnd::g_bMenuShowCoolTips = true;
	// Popup menu option: Initially hide rarely used items (RUI)
	CExtPopupMenuWnd::g_bMenuExpanding = false;
	// Popup menu option: Display RUI in different style
	CExtPopupMenuWnd::g_bMenuHighlightRarely = false;
	// Popup menu option: Animate when expanding RUI (like Office XP)
	CExtPopupMenuWnd::g_bMenuExpandAnimation = false;
	// Popup menu option: Align to desktop work area (false - to screen area)
	CExtPopupMenuWnd::g_bUseDesktopWorkArea = true;
	// Popup menu option: Popup menu animation effect (when displaying)
	CExtPopupMenuWnd::g_DefAnimationType = CExtPopupMenuWnd::__AT_NONE;


	VERIFY(g_CmdManager->ProfileSetup(profileName, GetSafeHwnd()));
	VERIFY(g_CmdManager->UpdateFromToolBar(profileName, IDR_TE_MAIN_TOOLBAR));

	VERIFY(wndStatusBar_.Create(this));
	VERIFY(wndStatusBar_.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)));

	VERIFY(mainToolBar_.Create("Main Toolbar", this, IDR_TE_MAIN_TOOLBAR,  WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY));
	VERIFY(mainToolBar_.LoadToolBar(IDR_TE_MAIN_TOOLBAR, RGB(192, 192, 192)));


	scaleMgr_.reset(new ScaleMgr);
	ASSERT(scaleMgr_);
	VERIFY(scaleMgr_->Create(this, SCALE_TOOLBAR_ID));

	ASSERT(treeManager_);
	VERIFY(treeManager_->create(this));

	triggerProfList_.reset(new TriggerProfList);
	ASSERT(triggerProfList_);
	VERIFY(triggerProfList_->create(this));

	miniMapManager_.reset(new TEMiniMapManager(triggerEditor_));
    ASSERT(miniMapManager_);
    VERIFY(miniMapManager_->create(this));

	mainToolBar_.EnableDocking(CBRS_ALIGN_ANY);
	scaleMgr_->enableDocking(true);
	treeManager_->enableDocking(true);
	triggerProfList_->enableDocking(true);
	miniMapManager_->enableDocking(true);


	VERIFY(CExtControlBar::FrameEnableDocking(this));

	scaleMgr_       ->dock(AFX_IDW_DOCKBAR_TOP);
	miniMapManager_ ->dock(AFX_IDW_DOCKBAR_LEFT);
	treeManager_    ->dock(AFX_IDW_DOCKBAR_LEFT);
	triggerProfList_->dock(AFX_IDW_DOCKBAR_BOTTOM);
	DockControlBar(&mainToolBar_, AFX_IDW_DOCKBAR_TOP);


	//ShowControlBar(&mainToolBar_, TRUE, FALSE); 
	//scaleMgr_->Show();
	//treeManager_->show(TETreeManager::SH_SHOW);
	//miniMapManager_->show();
	triggerProfList_->hide();

	//if(!loadWorkspace()) {
	//	resetWorkspace();
	//}

	loadWorkspace();
	//LoadBarState(CString((LPTSTR)IDS_REG_SEC_NAME_TOOLBARS));
	return 0;
}

BOOL TEFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, 
					   AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	if (triggerEditorView_
		&&triggerEditorView_->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;
	if (scaleMgr_&&scaleMgr_->getWindow()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))	
		return TRUE;
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL TEFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	return TRUE;
}

bool TEFrame::initTETreeManager()
{
	ASSERT(treeManager_);

	return true;
}

BOOL TEFrame::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

TriggerEditorView* TEFrame::addTriggerEditorView()
{
	RecalcLayout();
	CCreateContext cc;
	memset(&cc, 0, sizeof(cc));
	cc.m_pCurrentFrame = this;
	cc.m_pNewViewClass = RUNTIME_CLASS(TriggerEditorView);

	triggerEditorView_ = static_cast<TriggerEditorView*>(CreateView(&cc));
	triggerEditorView_->SetScaleMgr(scaleMgr_.get());
	scaleMgr_->SetScalable(triggerEditorView_);
	miniMapManager_->setView(triggerEditorView_);
	triggerEditorView_->setMiniMapManager(miniMapManager_.get());
	return triggerEditorView_;
}


void TEFrame::updateViewSize()
{
	ASSERT(IsWindow(m_hWnd));
	ASSERT(triggerEditorView_);
	CRect r;
	GetClientRect(&r);
	triggerEditorView_->MoveWindow(0, 0, r.Width(), r.Height());
}

void TEFrame::OnFileSave()
{
	CWaitCursor wait;
	triggerEditor_->save();
}

void TEFrame::onUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!triggerEditor_->isDataSaved());
}

void TEFrame::onUpdateEditFind(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void TEFrame::onViewToolbar()
{
	ShowControlBar(&mainToolBar_,!mainToolBar_.IsVisible(), FALSE); 
}

void TEFrame::onUpdateViewToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(mainToolBar_.IsVisible());
}

void TEFrame::onViewTree()
{
	if (treeManager_->isVisible())
		treeManager_->show(TETreeManager::SH_HIDE);
	else
		treeManager_->show(TETreeManager::SH_SHOW);
}

void TEFrame::onUpdateViewTree(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(treeManager_->isVisible());
}

void TEFrame::onViewMiniMap()
{
	if (miniMapManager_->isVisible())
		miniMapManager_->hide();
	else
		miniMapManager_->show();
}

void TEFrame::onUpdateViewMiniMap(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(miniMapManager_->isVisible());
}

void TEFrame::resetWorkspace()
{

	mainToolBar_.DockControlBar(AFX_IDW_DOCKBAR_TOP, false);

	RecalcLayout(TRUE);
}

bool TEFrame::saveWorkspace()
{
	std::vector<unsigned char> dlgBarState;

	bool retVal=false;
	try {
		// prepare memory file and archive
		CMemFile _file;
		CArchive ar(&_file, CArchive::store);
		// do serialization
		CExtControlBar::ProfileBarStateSerialize( ar, this, &windowPlacement_ );
		// OK, serialization passed
		ar.Flush();
		ar.Close();
		// put information to registry
		_file.SeekToBegin();
		int length = _file.GetLength();
		dlgBarState.resize(length);
		_file.Read(&dlgBarState[0], length);

		retVal=true;
	}
	catch(CException* pXept){
		pXept->Delete();
		ASSERT( FALSE );
	}
	catch(...){
		ASSERT( FALSE );
	}

	XPrmOArchive oa(".\\triggerInterface.cfg");
	oa.serialize(dlgBarState, "dlgBarState", 0);

	return retVal;
}

bool TEFrame::loadWorkspace()
{
	std::vector<unsigned char> dlgBarState;

	XPrmIArchive ia;
	if(ia.open(".\\triggerInterface.cfg")) {
		ia.serialize(dlgBarState, "dlgBarState", 0);
	}

	CWinApp * pApp = ::AfxGetApp();
	ASSERT( pApp != NULL );
	ASSERT( pApp->m_pszProfileName != NULL );
	ASSERT( pApp->m_pszProfileName[0] != _T('\0') );

	VERIFY( g_CmdManager->ProfileWndAdd( pApp->m_pszProfileName, GetSafeHwnd()) );
	VERIFY( g_CmdManager->UpdateFromMenu( pApp->m_pszProfileName, IDR_MAINFRAME) );

    bool retVal=false;
	try{
		CMemFile _file;
		if(dlgBarState.empty()){
			WINDOWPLACEMENT _wpf;
			::memset( &_wpf, 0, sizeof(WINDOWPLACEMENT) );
			_wpf.length = sizeof(WINDOWPLACEMENT);
			if(GetWindowPlacement(&_wpf)){
				_wpf.ptMinPosition.x = _wpf.ptMinPosition.y = 0;
				_wpf.ptMaxPosition.x = _wpf.ptMaxPosition.y = 0;
				_wpf.showCmd = (GetStyle() & WS_VISIBLE) ? SW_SHOWNA : SW_HIDE;
				SetWindowPlacement(&_wpf);
			}
			return false;
		}
		else{
			_file.Write(&dlgBarState[0], dlgBarState.size());
			_file.SeekToBegin();
		}
		CArchive ar( &_file, CArchive::load	);
		retVal = CExtControlBar::ProfileBarStateSerialize(ar, this, &windowPlacement_);
	}
	catch( CException * pXept ){
		pXept->Delete();
		ASSERT( FALSE );
	}
	catch( ... ){
		ASSERT( FALSE );
	}
	return retVal;
/*
	CTriggerEditorApp* pApp = (CTriggerEditorApp*)AfxGetApp();
	WINDOWPLACEMENT frameWindowPlacement;
	frameWindowPlacement.length=sizeof(frameWindowPlacement);
	GetWindowPlacement(&frameWindowPlacement);

	return !CExtControlBar::ProfileBarStateLoad(this, pApp->m_pszRegistryKey, pApp->m_pszProfileName, pApp->m_pszProfileName, &frameWindowPlacement);

	ASSERT( pApp != NULL );
	ASSERT( pApp->m_pszProfileName != NULL );
	ASSERT( pApp->m_pszProfileName[0] != _T('\0') );

	VERIFY( g_CmdManager->ProfileWndAdd( pApp->m_pszProfileName, GetSafeHwnd()) );
	VERIFY( g_CmdManager->UpdateFromMenu( pApp->m_pszProfileName, IDR_MAINFRAME) );


    bool result = false;
	try {
		CFile _file;
		if(!_file.Open(".\\triggerEditorInterace.cfg", CFile::OpenFlags::modeRead | CFile::OpenFlags::typeBinary)) {
			return false;
		}

		CArchive ar(&_file, CArchive::load);
		result = CExtControlBar::ProfileBarStateSerialize(ar, this, &frameWindowPlacement);
	} catch( CException * except ) {
		except->Delete();
		ASSERT( FALSE );
	}  catch( ... )	{
		ASSERT( FALSE );
	}
	return result;
	*/
}

void TEFrame::ActivateFrame(int nCmdShow)
{
	if(windowPlacement_.showCmd != SW_HIDE){
		SetWindowPlacement(&windowPlacement_);
		CFrameWnd::ActivateFrame(windowPlacement_.showCmd);
		windowPlacement_.showCmd = SW_HIDE;
		return;
	}
	CFrameWnd::ActivateFrame(nCmdShow);
}

AttribEditorInterface& attribEditorInterface(){
	AttribEditorInterface& attribEditor = triggerInterface().attribEditorInterface();
	return attribEditor;
} 

LONG TEFrame::onFindReplace(WPARAM wParam, LPARAM lParam)
{
	LPFINDREPLACE findReplace = (LPFINDREPLACE)lParam;
	if(findReplace->hwndOwner == GetSafeHwnd()){
		triggerEditorView_->onFindReplace(findReplace);
	}
	return 0;
}

void TEFrame::OnFileProperties()
{
	TriggerChain* triggerChain = triggerEditorView_->getTriggerEditorLogic()->getTriggerChain();
	if(triggerChain){
		EditArchive ea(GetSafeHwnd(), TreeControlSetup(0, 0, 200, 200, "Scripts\\TreeControlSetups\\triggerChainProperties"));
		triggerChain->serializeProperties(static_cast<EditOArchive&>(ea));
		if(ea.edit()){
			triggerChain->serializeProperties(static_cast<EditIArchive&>(ea));
			triggerEditor_->setDataChanged(true);
		}
	}
}
