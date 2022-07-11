#ifndef __TRIGGER_EDITOR_APP_H_INCLUDED__
#define __TRIGGER_EDITOR_APP_H_INCLUDED__

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTriggerEditorApp
// See TriggerEditor.cpp for the implementation of this class
//

class CTriggerEditorApp : public CWinApp
{
public:
	CTriggerEditorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTriggerEditorApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTriggerEditorApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();

public:
	BOOL InitInstance();
private:
};

extern CTriggerEditorApp theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRIGGEREDITOR_H__28604BA4_82DA_4E7B_B309_0F3E8A163990__INCLUDED_)
