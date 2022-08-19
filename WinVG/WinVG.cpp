// WinVG.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WinVG.h"

#include "MainFrm.h"
#include "WinVGDoc.h"
#include "WinVGView.h"
#include "mmsystem.h"
#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinVGApp

BEGIN_MESSAGE_MAP(CWinVGApp, CWinApp)
	//{{AFX_MSG_MAP(CWinVGApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinVGApp construction

CWinVGApp::CWinVGApp()
{
	bActive=TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWinVGApp object

CWinVGApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWinVGApp initialization

BOOL CWinVGApp::InitInstance()
{
	AfxEnableControlContainer();

	if(true)
	{
		_controlfp( _controlfp(0,0) & ~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL |  EM_INVALID),  MCW_EM ); 
		_clearfp();
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
/*
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
*/
	// Change the registry key under which our settings are stored.
	// such as the name of your company or organization.
	SetRegistryKey(_T("K-D Lab"));

	LoadStdProfileSettings(7);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWinVGDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CWinVGView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWinVGApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CWnd* text=GetDlgItem(IDC_INFO_TEXT);
	if(text)
	text->SetWindowText(
	"F2 F3 F4 \n"
	"A Z \n"
	"D C F V - ������� ������\n"
	"S X - ���������/��������� ������\n"
	"W - Wire Frame\n"
	"J \n"
	"LEFT RIGHT UP DOWN - ������� ������\n"
	"GRAY - + ���������/�������� ��������\n"
	"P - ����������/��������� ��������\n"
	"\n"
	"������� - X, ������ - Y, ����� - Z\n"

	);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CWinVGApp message handlers

BOOL CWinVGApp::OnIdle(LONG lCount) 
{
	// Do not render if the app is minimized
    if(m_pMainWnd->IsIconic())
        return FALSE;
	if(CWinApp::OnIdle(lCount))
		return TRUE;
    // Update and render a frame
	if(gb_FrameWnd && bActive)
	{
		CWinVGDoc* pDoc=static_cast<CWinVGDoc*>(gb_FrameWnd->GetActiveDocument());
		if(pDoc&&pDoc->m_pView) 
			pDoc->m_pView->OnDraw(0);
	}	
	return TRUE;
}
