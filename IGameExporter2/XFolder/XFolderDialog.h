// XFolderDialog.h  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// This software is released into the public domain.  You are free to use
// it in any way you like, except that you may not sell this source code.
//
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this
// software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XFOLDERDIALOG_H
#define XFOLDERDIALOG_H

#include "XHistoryCombo.h"

// Windows 2000 version of OPENFILENAME.
// The new version has three extra members.
// This is copied from commdlg.h
//
struct OPENFILENAMEEX : public OPENFILENAME
{
	void *	pvReserved;
	DWORD	dwReserved;
	DWORD	FlagsEx;
};

///////////////////////////////////////////////////////////////////////////////
// CXFolderDialog: Encapsulate Windows-2000 style open dialog.
//
class CXFolderDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CXFolderDialog)

// Construction
public:
	CXFolderDialog(LPCTSTR lpszInitialFolder = NULL,
				   DWORD dwFlags = 0,
				   CWnd* pParentWnd = NULL);

// Dialog Data
	//{{AFX_DATA(CXFolderDialog)
	CHistoryCombo	m_cmbRecentFolders;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXFolderDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Operations
public:
	// override
	virtual int DoModal();

// Attributes
public:
	enum XFILEDIALOG_OS_VERSION
	{
		XFILEDIALOG_AUTO_DETECT_OS_VERSION = 0,
		XFILEDIALOG_OS_VERSION_4,
		XFILEDIALOG_OS_VERSION_5
	};
	CString GetPath() { return m_strPath; }
	enum XFILEDIALOG_OS_VERSION GetOsVersion() { return m_eOsVersion; }
	void SetOsVersion(enum XFILEDIALOG_OS_VERSION eOsVersion) { m_eOsVersion = eOsVersion; }
	void SetTitle(LPCTSTR lpszTitle) { m_strTitle = lpszTitle; }

// Implementation
protected:
	OPENFILENAMEEX		m_ofnEx;	// new Windows 2000 version of OPENFILENAME
	UINT				m_nIdFileNameStatic;
	UINT				m_nIdFileNameCombo;
	UINT				m_nIdFilesOfTypeStatic;
	UINT				m_nIdFilesOfTypeCombo;
	UINT				m_nIdPlaceBar;
	int					m_nStaticLeftMargin;
	CString				m_strTitle;
	DWORD				m_dwFlags;
	CString				m_strInitialFolder;
	CString				m_strPath;

	enum XFILEDIALOG_OS_VERSION m_eOsVersion;

	BOOL IsWin2000();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	//{{AFX_MSG(CXFolderDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelendokMruCombo();
	afx_msg void OnOK();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif //XFOLDERDIALOG_H
