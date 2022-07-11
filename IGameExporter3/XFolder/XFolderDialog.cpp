// XFolderDialog.cpp  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// Description:
//     XFolderDialog implements CXFolderDialog, a folder browsing dialog that
//     uses the new Windows 2000-style dialog.  Please see article at
//     www.codeproject.com.
//
//     This code is based partially on Paul DiLascia's Aug 2000 MSDN Magazine
//     article "Windows 2000 File Dialog Revisited",
//     http://msdn.microsoft.com/msdnmag/issues/0800/c/
//
// History
//     Version 1.2 - 2005 March 22
//     - Fixed initial sizing problem with placebar
//
//     Version 1.1 - 2005 March 17
//     - Improved performance by using file filter
//
//     Version 1.0 - 2005 March 15
//     - Initial public release;  based on Version 1.2 of XFileDialog,
//       with fixes provided by Ned Harding and Rail Jon Rogut.
//
// Public APIs:
//          NAME                           DESCRIPTION
//     ---------------  -----------------------------------------------------
//     GetPath()        Returns folder path
//     GetOsVersion()   Returns OS version used to control dialog appearance
//     SetOsVersion()   Set OS version used to control dialog appearance
//     SetTitle()       Set title of dialog ("Select Folder" is default)
//
// License:
//     This software is released into the public domain.  You are free to use
//     it in any way you like, except that you may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <afxpriv.h>
#include "XFolderDialog.h"
#include "XFolderDialogRes.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// global data
static HWND  g_hListCtrl      = 0;
static int   g_nFolderCount   = 0;
static DWORD g_dwPlatformId   = 0;
static DWORD g_dwMinorVersion = 0;
static DWORD g_dwMajorVersion = 0;


///////////////////////////////////////////////////////////////////////////////
// EnumChildProc
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM)
{
	TCHAR szClassName[200];
	::GetClassName(hwndChild, szClassName, sizeof(szClassName)/sizeof(TCHAR)-2);
	szClassName[sizeof(szClassName)/sizeof(TCHAR)-1] = _T('\0');

	if (_tcscmp(_T("SysListView32"), szClassName) == 0)
	{
		TRACE(_T("found listctrl, setting g_hListCtrl to 0x%X\n"), hwndChild);
		g_hListCtrl = hwndChild;
		return FALSE;
	}
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CXFolderDialog

IMPLEMENT_DYNAMIC(CXFolderDialog, CFileDialog)

BEGIN_MESSAGE_MAP(CXFolderDialog, CFileDialog)
	//{{AFX_MSG_MAP(CXFolderDialog)
	ON_WM_SIZE()
	ON_CBN_SELENDOK(IDC_MRU_COMBO, OnSelendokMruCombo)
	ON_BN_CLICKED(IDC_OPEN, OnOK)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// static string for file filter
static TCHAR szFilter[] = _T("Files (*.XFolderDialog-8C0C924B-7A86-4e8d-8D6E-A1C50D74E8DF)|")
								 _T("*.XFolderDialog-8C0C924B-7A86-4e8d-8D6E-A1C50D74E8DF||");

///////////////////////////////////////////////////////////////////////////////
// ctor
CXFolderDialog::CXFolderDialog(LPCTSTR lpszInitialFolder /*= 0*/,
							   DWORD dwFlags /*= 0*/,
							   CWnd* pParentWnd /*= 0*/)
	: CFileDialog(TRUE, 0, 0, dwFlags | OFN_HIDEREADONLY, szFilter, pParentWnd)
{
	m_strInitialFolder     = lpszInitialFolder;
	m_strPath              = _T("");
	m_dwFlags              = dwFlags;
	m_eOsVersion           = XFILEDIALOG_AUTO_DETECT_OS_VERSION;
	m_strTitle             = _T("");
	m_nIdFileNameStatic    = 1090;
	m_nIdFileNameCombo     = 1148;	// or 1152
	m_nIdFilesOfTypeStatic = 1089;
	m_nIdFilesOfTypeCombo  = 1136;
	m_nIdPlaceBar          = 1184;

	OSVERSIONINFO osinfo;
	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osinfo);

	g_dwPlatformId   = osinfo.dwPlatformId;
	g_dwMinorVersion = osinfo.dwMinorVersion;
	g_dwMajorVersion = osinfo.dwMajorVersion;

	SetTemplate(0, _T("IDD_XFOLDERDIALOG"));
}

///////////////////////////////////////////////////////////////////////////////
// DoDataExchange
void CXFolderDialog::DoDataExchange(CDataExchange* pDX)
{
	CFileDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXFolderDialog)
	DDX_Control(pDX, IDC_MRU_COMBO, m_cmbRecentFolders);
	//}}AFX_DATA_MAP
}

///////////////////////////////////////////////////////////////////////////////
// DoModal override copied mostly from MFC, with modification to use
// m_ofnEx instead of m_ofn.
int CXFolderDialog::DoModal()
{
	TRACE(_T("in CXFolderDialog::DoModal\n"));

	ASSERT_VALID(this);
	ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
	ASSERT(m_ofn.lpfnHook != 0); // can still be a user hook

	// zero out the file buffer for consistent parsing later
	ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
	DWORD nOffset = lstrlen(m_ofn.lpstrFile)+1;
	ASSERT(nOffset <= m_ofn.nMaxFile);
	memset(m_ofn.lpstrFile+nOffset, 0, (m_ofn.nMaxFile-nOffset)*sizeof(TCHAR));

	// WINBUG: This is a special case for the file open/save dialog,
	// which sometimes pumps while it is coming up but before it has
	// disabled the main window.
	HWND hWndFocus = ::GetFocus();
	BOOL bEnableParent = FALSE;
	m_ofn.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	if (m_ofn.hwndOwner != 0 && ::IsWindowEnabled(m_ofn.hwndOwner))
	{
		bEnableParent = TRUE;
		::EnableWindow(m_ofn.hwndOwner, FALSE);
	}

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	ASSERT(pThreadState->m_pAlternateWndInit == 0);

	if (m_ofn.Flags & OFN_EXPLORER)
		pThreadState->m_pAlternateWndInit = this;
	else
		AfxHookWindowCreate(this);

	memset(&m_ofnEx, 0, sizeof(m_ofnEx));
	memcpy(&m_ofnEx, &m_ofn, sizeof(m_ofn));

	m_ofnEx.Flags |= OFN_ENABLESIZING;

	if (IsWin2000())
		m_ofnEx.lStructSize = sizeof(OPENFILENAMEEX);
	else
		m_ofnEx.lStructSize = sizeof(OPENFILENAME);

	m_ofnEx.lpstrInitialDir = m_strInitialFolder;

	int nResult;
	if (m_bOpenFileDialog)
		nResult = ::GetOpenFileName((OPENFILENAME*)&m_ofnEx);
	else
		nResult = ::GetSaveFileName((OPENFILENAME*)&m_ofnEx);

	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
	m_ofn.lStructSize = sizeof(m_ofn);

	if (nResult)
		ASSERT(pThreadState->m_pAlternateWndInit == 0);
	pThreadState->m_pAlternateWndInit = 0;

	// WINBUG: Second part of special case for file open/save dialog.
	if (bEnableParent)
		::EnableWindow(m_ofnEx.hwndOwner, TRUE);
	if (::IsWindow(hWndFocus))
		::SetFocus(hWndFocus);

	PostModal();

	return nResult ? nResult : IDCANCEL;
}

///////////////////////////////////////////////////////////////////////////////
// OnInitDialog
BOOL CXFolderDialog::OnInitDialog()
{
	TRACE(_T("in CXFolderDialog::OnInitDialog\n"));

	CFileDialog::OnInitDialog();

	// load folder history - check if valid folder
	m_cmbRecentFolders.SetMaxHistoryItems(50);
	m_cmbRecentFolders.SetDropSize(20);
	m_cmbRecentFolders.SetCheckAccess(TRUE);
	m_cmbRecentFolders.LoadHistory(_T("FolderHistory"), _T("Folder"));

	CString str;
	if (m_cmbRecentFolders.GetCount() > 0)
	{
		m_cmbRecentFolders.GetLBText(0, str);
		m_cmbRecentFolders.SetWindowText(str);
	}

	// save left margin for combo boxes
	CWnd *pWndFileNameCombo = GetParent()->GetDlgItem(m_nIdFileNameCombo);
	if (!pWndFileNameCombo)
	{
		// the File Name control ID is either 1148 or 1152, depending on whether
		// it is used as an edit box (1152) or a combo box (1148).  If the OS
		// version is < 5, it is 1152; if >= 5, it is 1148.  It will also be
		// 1152 if the registry key
		//    HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Policies\comdlg32\NoFileMru
		// is set to 1.  For convenience, we will always refer to this control
		// as a combobox.
		m_nIdFileNameCombo = 1152;
		pWndFileNameCombo = GetParent()->GetDlgItem(m_nIdFileNameCombo);
	}

	CRect rect;

	CWnd *pWndFileNameStatic = GetParent()->GetDlgItem(m_nIdFileNameStatic);
	ASSERT(pWndFileNameStatic);
	if (pWndFileNameStatic)
	{
		pWndFileNameStatic->GetWindowRect(&rect);
		GetParent()->ScreenToClient(&rect);
		m_nStaticLeftMargin = rect.left;
	}
	else
	{
		m_nStaticLeftMargin = 60;
	}

	// set title if specified
	if (m_strTitle.IsEmpty())
		GetParent()->SetWindowText(_T("Select Folder"));
	else
		GetParent()->SetWindowText(m_strTitle);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}

int deltaY = 0;
///////////////////////////////////////////////////////////////////////////////
// OnSize - position and size controls on default and new dialogs
void CXFolderDialog::OnSize(UINT nType, int cx, int cy)
{
	TRACE(_T("in CXFolderDialog::OnSize:  cx=%d  cy=%d\n"));

	CFileDialog::OnSize(nType, cx, cy);

	// original dialog box
	CWnd *pWndParent = GetParent();
	if ((pWndParent == 0) || (!::IsWindow(pWndParent->m_hWnd)))
		return;

	CRect rectNoWhere(10000, 10000, 10001, 10001);

	// File name combobox
	CWnd *pWndFileNameCombo = pWndParent->GetDlgItem(m_nIdFileNameCombo);
	if (pWndFileNameCombo && ::IsWindow(pWndFileNameCombo->m_hWnd))
	{
		pWndFileNameCombo->MoveWindow(&rectNoWhere);
		pWndFileNameCombo->EnableWindow(FALSE);
	}

	// File name static
	CWnd *pWnd = pWndParent->GetDlgItem(m_nIdFileNameStatic);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->MoveWindow(&rectNoWhere);
		pWnd->EnableWindow(FALSE);
	}

	// Files of type combobox
	pWnd = pWndParent->GetDlgItem(m_nIdFilesOfTypeCombo);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->MoveWindow(&rectNoWhere);
		pWnd->EnableWindow(FALSE);
	}

	// Files of type static
	pWnd = pWndParent->GetDlgItem(m_nIdFilesOfTypeStatic);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->MoveWindow(&rectNoWhere);
		pWnd->EnableWindow(FALSE);
	}

	CRect rectOK;
	CWnd *pWndOK = pWndParent->GetDlgItem(1);
	if (pWndOK && ::IsWindow(pWndOK->m_hWnd))
	{
		pWndOK->GetWindowRect(&rectOK);
		pWndParent->ScreenToClient(&rectOK);
		pWndOK->ShowWindow(SW_HIDE);
	}

	CWnd *pWndOkButton = GetDlgItem(IDC_OPEN);
	if (pWndOkButton && ::IsWindow(pWndOkButton->m_hWnd))
	{
		pWndOkButton->MoveWindow(&rectOK);
	}

	// Folder name static
	CRect rectRecentFoldersStatic;
	int h = 0;
	int w = 0;
	pWnd = GetDlgItem(IDC_MRU_CAPTION);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->GetWindowRect(&rectRecentFoldersStatic);
		pWndParent->ScreenToClient(&rectRecentFoldersStatic);
		h = rectRecentFoldersStatic.Height();
		w = rectRecentFoldersStatic.Width();
		rectRecentFoldersStatic.left   = m_nStaticLeftMargin;
		rectRecentFoldersStatic.right  = rectRecentFoldersStatic.left + w;
		rectRecentFoldersStatic.top    = rectOK.top + 2;
		rectRecentFoldersStatic.bottom = rectRecentFoldersStatic.top + h;
		pWnd->MoveWindow(&rectRecentFoldersStatic);
	}

	// Folder name combobox
	CRect rectRecentFoldersCombo;
	pWnd = GetDlgItem(IDC_MRU_COMBO);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->GetWindowRect(&rectRecentFoldersCombo);
		pWndParent->ScreenToClient(&rectRecentFoldersCombo);
		w = rectRecentFoldersCombo.Width();
		h = rectRecentFoldersCombo.Height();
		rectRecentFoldersCombo.left   = rectRecentFoldersStatic.right + 2;
		rectRecentFoldersCombo.right  = rectOK.left - 6;
		rectRecentFoldersCombo.top    = rectOK.top;
		rectRecentFoldersCombo.bottom = rectRecentFoldersCombo.top + h;
		pWnd->MoveWindow(&rectRecentFoldersCombo);
	}

	CRect rectCancelWindow, rectCancelClient;
	pWnd = pWndParent->GetDlgItem(2);
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		pWnd->GetWindowRect(&rectCancelClient);
		rectCancelWindow = rectCancelClient;
		pWndParent->ScreenToClient(&rectCancelClient);
	}

	// Place bar
	pWnd = pWndParent->GetDlgItem(m_nIdPlaceBar);

	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		CRect rectPlaceBar;
		pWnd->GetWindowRect(&rectPlaceBar);
		pWndParent->ScreenToClient(&rectPlaceBar);
		rectPlaceBar.bottom = rectCancelClient.bottom;
		pWnd->MoveWindow(&rectPlaceBar);
	}

	// Dialog
	CRect rectDialog;
	pWndParent->GetWindowRect(&rectDialog);
	rectDialog.bottom = rectCancelWindow.bottom + rectCancelWindow.Height()/2 + 3;
	rectDialog.right  = rectCancelWindow.right + 10;
	pWndParent->MoveWindow(&rectDialog);

	///////////////////////////////////////////////////////////////////////////
	// following code is necessary to prevent having the
	// recent folders combobox highlighted
	static BOOL bFirstTime = TRUE;
	if (bFirstTime)
	{
		bFirstTime = FALSE;
		m_cmbRecentFolders.SetFocus();
		SetTimer(1, 20, 0);		// ensure all placebar buttons are visible
	}
	else
	{
		m_cmbRecentFolders.SetEditSel(-1, 0);
		pWndFileNameCombo->SetFocus();
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnNotify - when the open dialog sends a notification, copy m_ofnEx to m_ofn
// in case handler function is expecting updated information in the
// OPENFILENAME struct.
//
BOOL CXFolderDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	TRACE(_T("in CXFolderDialog::OnNotify\n"));
	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
	m_ofn.lStructSize = sizeof(m_ofn);

	NMHDR * pNmhdr = (NMHDR *) lParam;

	if (pNmhdr->code == CDN_SELCHANGE)
	{
		TRACE(_T("CDN_SELCHANGE ===============\n"));

		CWnd *pWnd = GetParent();
		if (pWnd)
		{
			g_hListCtrl = 0;

			EnumChildWindows(pWnd->m_hWnd, EnumChildProc, 0);

			if (g_hListCtrl)
			{
				//TRACE(_T("g_hListCtrl=0x%X\n"), g_hListCtrl);

				// if folder selected, display it in combo box
				int nItem = ListView_GetNextItem(g_hListCtrl, -1, LVNI_SELECTED);
				//TRACE(_T("nItem=%d\n"), nItem);

				TCHAR szDir[MAX_PATH*2] = { 0 };
				::GetCurrentDirectory(sizeof(szDir)/sizeof(TCHAR)-2, szDir);

				if (nItem >= 0)
				{
					TCHAR szItem[MAX_PATH*2] = { 0 };
					ListView_GetItemText(g_hListCtrl, nItem, 0, szItem, sizeof(szItem)/sizeof(TCHAR)-2);

					m_strPath = szDir;
					if (szDir[_tcslen(szDir)-1] != _T('\\'))
						m_strPath += _T("\\");
					m_strPath += szItem;

					m_cmbRecentFolders.SetWindowText(m_strPath);
					TRACE(_T("m_strPath=%s\n"), m_strPath);
				}
			}
		}
	}
	else if (pNmhdr->code == CDN_FOLDERCHANGE)
	{
		TRACE(_T("CDN_FOLDERCHANGE ===============\n"));

		CWnd *pWnd = GetParent();
		if (pWnd)
		{
			// display folder in combo box
			TCHAR szDir[MAX_PATH*2] = { 0 };
			::GetCurrentDirectory(sizeof(szDir)/sizeof(TCHAR)-2, szDir);
			m_strPath = szDir;

			m_cmbRecentFolders.SetWindowText(szDir);
		}
	}

	return CFileDialog::OnNotify(wParam, lParam, pResult);
}

///////////////////////////////////////////////////////////////////////////////
// OnOK
void CXFolderDialog::OnOK()
{
	TRACE(_T("CXFolderDialog::OnOK\n"));

	TRACE(_T("m_strPath=%s\n"), m_strPath);

	CString strFolder = _T("");
	m_cmbRecentFolders.GetWindowText(strFolder);
	if (_taccess(strFolder, 00) == -1)
	{
		CString strMessage = _T("");
		strMessage.Format(_T("The folder '%s' isn't accessible."), strFolder);
		AfxMessageBox(strMessage);
		return;
	}

	m_cmbRecentFolders.SaveHistory(TRUE);

	((CDialog*)GetParent())->EndDialog(IDOK);
}

///////////////////////////////////////////////////////////////////////////////
// OnSelendokMruCombo
void CXFolderDialog::OnSelendokMruCombo()
{
	TRACE(_T("in CXFolderDialog::OnSelendokMruCombo\n"));

	if (!::IsWindow(m_cmbRecentFolders.m_hWnd))
		return;

	CString strFolder = _T("");

	int index = m_cmbRecentFolders.GetCurSel();

	if (index >= 0)
	{
		m_cmbRecentFolders.GetLBText(index, strFolder);

		if (!strFolder.IsEmpty() && (_taccess(strFolder, 00) == 0))
		{
			CWnd *pWndParent = GetParent();
			if ((pWndParent != 0) && ::IsWindow(pWndParent->m_hWnd))
			{
				// change to new folder, leave file name control unchanged
				TCHAR szText[_MAX_PATH*2];
				memset(szText, 0, sizeof(szText));
				pWndParent->GetDlgItem(m_nIdFileNameCombo)->SendMessage(WM_GETTEXT,
								sizeof(szText)/sizeof(TCHAR)-1, (LPARAM)szText);
				pWndParent->SendMessage(CDM_SETCONTROLTEXT, m_nIdFileNameCombo,
								(LPARAM)(LPCTSTR)strFolder);
				pWndParent->SendMessage(WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED),
								(LPARAM)GetDlgItem(IDOK)->GetSafeHwnd());
				pWndParent->SendMessage(CDM_SETCONTROLTEXT, m_nIdFileNameCombo,
								(LPARAM)szText);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// IsWin2000
BOOL CXFolderDialog::IsWin2000()
{
	if (GetOsVersion() == XFILEDIALOG_OS_VERSION_4)
		return FALSE;
	else if (GetOsVersion() == XFILEDIALOG_OS_VERSION_5)
		return TRUE;

	if ((g_dwPlatformId == VER_PLATFORM_WIN32_NT) &&
		(g_dwMajorVersion >= 5))
	{
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// OnTimer - adjust dialog size so placebar fits
void CXFolderDialog::OnTimer(UINT nIDEvent)
{
	KillTimer(nIDEvent);

	CWnd *pWndParent = GetParent();
	if (pWndParent)
	{
		CRect rectPlaceBar;
		CWnd *pWnd = pWndParent->GetDlgItem(m_nIdPlaceBar);

		if (pWnd)
		{
			pWnd->GetWindowRect(&rectPlaceBar);

			CToolBarCtrl *ptb = (CToolBarCtrl *) pWndParent->GetDlgItem(m_nIdPlaceBar);

			if (ptb)
			{
				int nCount = ptb->GetButtonCount();

				DWORD dwSize = ptb->GetButtonSize();
				UINT nButtonHeight = HIWORD(dwSize);

				int nMinHeight = nCount * nButtonHeight + 2;

				CRect rectDialog;
				pWndParent->GetWindowRect(&rectDialog);

				if ((rectPlaceBar.Height() < nMinHeight) &&
					(nMinHeight < GetSystemMetrics(SM_CYSCREEN)))	// might not work on multiple monitors
				{
					// adjust dialog so all buttons are visible
					rectDialog.bottom += nMinHeight - rectPlaceBar.Height();
					pWndParent->MoveWindow(&rectDialog);
				}
			}
		}
	}

	CFileDialog::OnTimer(nIDEvent);
}
