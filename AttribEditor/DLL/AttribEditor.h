// AttribEditor.h : main header file for the AttribEditor DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CAttribEditorApp
// See AttribEditor.cpp for the implementation of this class
//

class CAttribEditorApp : public CWinApp
{
public:
	CAttribEditorApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
