#include "stdafx.h"
#include "UIEditor.h"
#include "MainFrame.h"
#include "EditorView.h"
#include "Serialization\Dictionary.h"
#include "EditorVisual.h"
#include "SplashScreen.h"
#include "kdw/Win32/Window.h"

#include "DebugPrm.h"
#include "Game\GameOptions.h"
#include "UserInterface\UI_Render.h"

#include "Util\Win32\DebugSymbolManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const UINT WM_CHECK_ITS_ME = RegisterWindowMessage(UIEDITOR_GUID);

BEGIN_MESSAGE_MAP(CUIEditorApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()

string GetMissionSaveNameFromPlayReel(const char* fname) { xassert(0&&"Replay not support in editor!"); return string(""); }
void UpdateMDFromPlayReel(const char* fname, class MissionDescription* pmd){ xassert(0&&"Replay not support in editor!"); }

EditorVisual::Interface& editorVisual(){
	xassert(0 && "EditorVisual не работает в UIEditor-е");
	return *reinterpret_cast<EditorVisual::Interface*>(0);
}

CUIEditorApp::CUIEditorApp()
: active_(true)
{
	
}

CUIEditorApp theApp;

CUIMainFrame* CUIEditorApp::GetMainFrame ()
{
	CUIMainFrame* pFrame = static_cast<CUIMainFrame*>(AfxGetMainWnd ());
	ASSERT (pFrame);
	return pFrame;
}

BOOL CUIEditorApp::InitInstance()
{
	Win32::_setGlobalInstance(m_hInstance);
	SplashScreen splash;
	VERIFY(splash.create(IDB_SPLASH));
	splash.show();

	InitCommonControls();

	DebugSymbolManager::create();

	CWinApp::InitInstance();

	if (!AfxOleInit()){
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	DebugPrm::instance();

	UI_Render::create();

	GameOptions::instance().setTranslate();

	HANDLE hMutexOneInstance = CreateMutex (NULL, FALSE, UIEDITOR_GUID);
	bool bAlreadyRunning = (GetLastError() == ERROR_ALREADY_EXISTS || 
						    GetLastError() == ERROR_ACCESS_DENIED);

	if (bAlreadyRunning)
	{
		DWORD result;
		LRESULT ok = SendMessageTimeout (HWND_BROADCAST, WM_CHECK_ITS_ME, 0, 0,
										 SMTO_BLOCK | SMTO_ABORTIFHUNG,
                                         200, &result);
		return FALSE;
	}

	CUIMainFrame* frame = new CUIMainFrame;
	if (!frame)
		return FALSE;
	m_pMainWnd = frame;


	frame->LoadFrame(IDR_UIEDITORFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
	splash.hide();

	frame->ShowWindow(SW_SHOWMAXIMIZED);
	frame->UpdateWindow();

	return TRUE;
}


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CUIEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CUIEditorApp message handlers


BOOL CUIEditorApp::OnIdle(LONG lCount)
{
    if(m_pMainWnd->IsIconic())
        return FALSE;
	if(CWinApp::OnIdle(lCount))
		return TRUE;

	static int last_clock = xclock();
	if(active_ && GetActiveWindow() == m_pMainWnd->GetSafeHwnd()){
		float dt = min(0.3f, float(xclock() - last_clock) / 1000.0f);
		CUIEditorView& view = uiEditorFrame()->view();
		view.quant(dt);
		view.Invalidate(FALSE);
		last_clock = xclock();
		return TRUE;
	}
    else{
		last_clock = xclock();
		return FALSE;
	}
}
