#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols



// CDllScreenCapApp
// See DllScreenCap.cpp for the implementation of this class
//

class CGameExporterApp : public CWinApp
{
public:
	CGameExporterApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

