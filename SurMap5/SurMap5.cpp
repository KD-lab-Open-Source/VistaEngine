#include "stdafx.h"
#include "SurMap5.h"
#include "MainFrame.h"

#include "..\Terra\vMap.h"
#include "EffectReference.h"

#include "..\Util\SystemUtil.h"
#include "..\Util\ConsoleWindow.h"
#include "GameOptions.h"
#include "SurToolAux.h"
#include "ZipConfig.h"
#include "SplashScreen.h"

#include "TextDB.h"
#include "Dictionary.h"
#include "FileUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CSurMap5App, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_VIEW_ANIMATION, OnViewAnimation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATION, OnUpdateViewAnimation)
END_MESSAGE_MAP()

string GetMissionSaveNameFromPlayReel(const char* fname) { xassert(0&&"Replay is not supported in editor!"); return string(""); }
void UpdateMDFromPlayReel(const char* fname, class MissionDescription* pmd){ xassert(0&&"Replay is not supported in editor!"); }


CSurMap5App::CSurMap5App()
{
	AfxEnableMemoryTracking(FALSE);
	m_pMainFrameMenu = 0;
	flag_animation=1;
}

CSurMap5App::~CSurMap5App ()
{
	delete m_pMainFrameMenu;
}

CSurMap5App theApp;

/////////////////////////////////////////////////////////////////////////////
// Prof-UIS advanced options
void CSurMap5App::SetupUiAdvancedOptions(CWnd* wnd)
{
	ASSERT( m_pszRegistryKey != NULL );
	ASSERT( m_pszRegistryKey[0] != _T('\0') );
	ASSERT( m_pszProfileName != NULL );
	ASSERT( m_pszProfileName[0] != _T('\0') );

#ifdef _VISTA_ENGINE_EXTERNAL_
	const bool external = true;
#else
	const bool external = false;
#endif
	// Prof-UIS command manager profile
	VERIFY(g_CmdManager->ProfileSetup(m_pszProfileName, wnd->GetSafeHwnd() ));
	VERIFY(g_CmdManager->UpdateFromMenu(m_pszProfileName, CMainFrame::toolBarID()));
	VERIFY(g_CmdManager->UpdateFromToolBar(m_pszProfileName, CMainFrame::toolBarID(), 0, 0, false, false, RGB(255, 0, 255)));
	VERIFY(g_CmdManager->UpdateFromToolBar(m_pszProfileName, IDR_FILTERS_BAR, 0, 0, false, false, RGB(255, 0, 255)));
	VERIFY(g_CmdManager->UpdateFromToolBar(m_pszProfileName, external ? IDR_LIBRARIES_EXTERNAL_BAR : IDR_LIBRARIES_BAR, 0, 0, false, false, RGB(255, 0, 255)));
	VERIFY(g_CmdManager->UpdateFromToolBar(m_pszProfileName, external ? IDR_EDITORS_EXTERNAL_BAR : IDR_EDITORS_BAR, 0, 0, false, false, RGB(255, 0, 255)));

	// General UI look
    //
// UNCOMMENT THESE LINES FOR OFFICE 2000 UI STYLE
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
	CExtPopupMenuWnd::g_DefAnimationType =
		CExtPopupMenuWnd::__AT_NONE;
}

// CSurMap5App initialization
BOOL CSurMap5App::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	AfxInitRichEdit();

	SetRegistryKey(_T("VistaGame Editor"));

	ASSERT( m_pszRegistryKey != NULL );
	ASSERT( m_pszProfileName != NULL );

	SplashScreen splash;
	VERIFY(splash.create(IDB_SPLASH));
	splash.show();

#ifdef _VISTA_ENGINE_EXTERNAL_
	char modulePath[MAX_PATH + 1];
	GetModuleFileName(GetModuleHandle(0), modulePath, MAX_PATH);

	std::string path = ::extractFilePath(modulePath);
	std::string gamePath = path + "\\Maelstrom.exe";
	if(!::isFileExists(gamePath.c_str())){
		AfxMessageBox(TRANSLATE("Похоже, что игра была удалена после установки редактора. Вам нужно переустановить игру в тот же каталог."), MB_ICONEXCLAMATION, 0);
		return FALSE;
	}
	ZipConfig::initArchives();
	TextIdMap::instance().setSave(false);
#else
	int zip = 1;
	IniManager("Game.ini").getInt("Game", "ZIP", zip);
	if(zip)
		ZipConfig::initArchives();
#endif

	GameOptions::instance().setTranslate();
	GameOptions::instance().loadPresets();
	TranslationManager::instance().setTranslationsDir(".\\Scripts\\Engine\\Translations");
	TranslationManager::instance().setDefaultLanguage("english");
	
	Console::instance().registerListener(&ConsoleWindow::instance());

	//Инициализация FP
	setLogicFp();

	//Инициализация внешнего модуля для Shape3D

	vMap.prepare("Resource\\Worlds");//"worlds.prm",

	// Инициализация библиотеки спецэффектов
	EffectContainer::setTexturesPath("Resource\\FX\\Textures");

	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());

	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;

	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	HICON hIcon = LoadIcon (IDR_MAINFRAME);
	pFrame->SetIcon (hIcon, FALSE);
	pFrame->SetIcon (hIcon, TRUE);

	splash.hide();
	pFrame->ShowWindow(SW_SHOWMAXIMIZED);
	pFrame->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}



// CSurMap5App message handlers



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Text(pDX, IDC_EDIT1, test);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CSurMap5App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CSurMap5App message handlers


BOOL CSurMap5App::OnIdle(LONG lCount)
{
	BOOL result = CWinApp::OnIdle(lCount);
	
	if(result)
		return TRUE;

	//BOOL result = CWinApp::OnIdle(lCount);
	if(!flag_animation) 
		return FALSE;
	else {
		CMainFrame* mainFrame = (CMainFrame*)AfxGetMainWnd();
		return mainFrame->universeQuant();
	}
	//return CWinApp::OnIdle(lCount);
}

void CSurMap5App::OnViewAnimation()
{
	// TODO: Add your command handler code here
	if(flag_animation) flag_animation=0;
	else flag_animation=1;

}

void CSurMap5App::OnUpdateViewAnimation(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(flag_animation);
}

//AUX interface function
CSize MakeVoluntaryImageList(UINT inBitmapID, UINT toolBarBitDepth, int amountImg, CImageList&	outImageList)
{
	CBitmap bm;
	// if we use CBitmap::LoadBitmap() to load the bitmap, the colors
	// will be reduced to the bit depth of the main screen and we won't
	// be able to access the pixels directly. To avoid those problems,
	// we'll load the bitmap as a DIBSection instead and attach the
	// DIBSection to the CBitmap.
	VERIFY(bm.Attach(::LoadImage(::AfxFindResourceHandle( MAKEINTRESOURCE(inBitmapID), RT_BITMAP),
		MAKEINTRESOURCE(inBitmapID), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_CREATEDIBSECTION))));
//	HBITMAP hBitmap=(HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_MAINFRAME_TOOLBAR256), IMAGE_BITMAP,
//		0, 0,LR_CREATEDIBSECTION|LR_LOADMAP3DCOLORS);
//	bm.Attach(hBitmap);

	// create a 24 bit image list with the same dimensions and number
	// of buttons as the toolbar
	BITMAP infoBitMap;
	int result=bm.GetBitmap(&infoBitMap);
	CSize buttonImgSize(infoBitMap.bmWidth/amountImg, infoBitMap.bmHeight);
	//VERIFY(outImageList.Create(buttonImgSize.cx, buttonImgSize.cy, toolBarBitDepth|ILC_MASK, amountImg, 0));
	VERIFY(outImageList.Create(buttonImgSize.cx, buttonImgSize.cy, toolBarBitDepth|ILC_MASK, amountImg, 0));
	//outImageList.Create(MAKEINTRESOURCE(IDB_MAINFRAME_TOOLBAR256), 24, 0, RGB(255,255,255));

	// attach the bitmap to the image list
	VERIFY(outImageList.Add(&bm, ALFA_COLOR_IN_BITMAP_IN_CONTROL) != -1);
	//outImageList.Add(&bm,reinterpret_cast<CBitmap*>(NULL));
	return buttonImgSize;
}

