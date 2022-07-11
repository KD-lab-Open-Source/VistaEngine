// ZipPacker.h : main header file for the ZipPacker application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CZipPackerApp:
// See ZipPacker.cpp for the implementation of this class
//

class CZipPackerApp : public CWinApp
{
public:
	CZipPackerApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

public:
	DECLARE_MESSAGE_MAP()
};

extern CZipPackerApp theApp;