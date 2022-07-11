#include "stdafx.h"
#include "ZipPacker.h"
#include "TreeInterface.h"
#define TRANSLATE(x) x
#include "OutputProgressDlg.h"
#include "ZipConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(CZipPackerApp, CWinApp)
END_MESSAGE_MAP()



CZipPackerApp::CZipPackerApp()
{
}


CZipPackerApp theApp;

BOOL CZipPackerApp::InitInstance()
{
	InitCommonControls();

	CWinApp::InitInstance();

	/*
	// Initialize OLE libraries
	if (!AfxOleInit()){
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	*/
	
	char buffer[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH, buffer);

	if(!::check_command_line("/q")){
		CString message;
		message.Format("Do you wish to pack files in \n\"%s\\Resource\"?", buffer);
		if(AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) != IDYES)
			return FALSE;
	}

	ExportOutputCallback::show(0);
	ZipConfig::makeArchives(&ExportOutputCallback::callback);
	//IniManager("game.ini").putInt("Game", "ZIP", 1);
	ExportOutputCallback::hide(0);
	return TRUE;
}

__declspec( thread ) DWORD tls_is_graph;