/**********************************************************************
 *<
	FILE: DllEntry.cpp

	DESCRIPTION: Contains the Dll Entry stuff

	CREATED BY:	 Neil Hazzard

	HISTORY:	summer 2002

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/
#include "StdAfx.h"
#include "DllEntry.h"

//
HINSTANCE hInstance;
int controlsInit = FALSE;
//
//
extern ClassDesc2* GetIGameExporterDesc();

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CDllScreenCapApp

BEGIN_MESSAGE_MAP(CGameExporterApp, CWinApp)
END_MESSAGE_MAP()


// CDllScreenCapApp construction

CGameExporterApp::CGameExporterApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDllScreenCapApp object

CGameExporterApp theApp;


// CDllScreenCapApp initialization

BOOL CGameExporterApp::InitInstance()
{
	CWinApp::InitInstance();

	hInstance = m_hInstance;				// Hang on to this DLL's instance handle.

	if (!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);	// Initialize MAX's custom controls
		InitCommonControls();			// Initialize Win95 controls
	}
	return TRUE;



}
//BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
//{
//	hInstance = hinstDLL;				// Hang on to this DLL's instance handle.
//
//	if (!controlsInit) {
//		controlsInit = TRUE;
//		InitCustomControls(hInstance);	// Initialize MAX's custom controls
//		InitCommonControls();			// Initialize Win95 controls
//	}
//			
//	return (TRUE);
//}

__declspec( dllexport ) const TCHAR* LibDescription()
{
	return _T("3dx Exporter (K-D Lab)");
}

//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
	return 1;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
		case 0: return GetIGameExporterDesc();
		default: return 0;
	}
}

__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}
