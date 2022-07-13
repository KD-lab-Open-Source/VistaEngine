#include "stdafx.h"
#include "SurMap5.h"
#include "CreateAttribDlg.h"

// CCreateAttribDlg dialog

IMPLEMENT_DYNAMIC(CCreateAttribDlg, CDialog)
CCreateAttribDlg::CCreateAttribDlg(bool allowPaste, bool pasteByDefault, const char* title, const char* oldName, CWnd* pParent /*=NULL*/)
: CDialog(CCreateAttribDlg::IDD, pParent)
{
	title_ = title;
    defaultName_ = oldName ? oldName : title;
    allowPaste_ = allowPaste;
    pasteByDefault_ = pasteByDefault;
}

CCreateAttribDlg::~CCreateAttribDlg()
{
}

void CCreateAttribDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NAME_EDIT, nameEdit_);
	DDX_Control(pDX, IDC_PASTE_CHECK, pasteCheck_);
}


BEGIN_MESSAGE_MAP(CCreateAttribDlg, CDialog)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

void CCreateAttribDlg::OnOK()
{
	CString str;
	paste_ = static_cast<bool>(pasteCheck_.GetCheck ());
	nameEdit_.GetWindowText (str);
	name_ = static_cast<const char*>(str);
	CDialog::OnOK();
}

void CCreateAttribDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	nameEdit_.SetFocus ();
	nameEdit_.SetSel (0, -1);
}

BOOL CCreateAttribDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

    SetWindowText(title_.c_str());

	nameEdit_.SetWindowText(defaultName_.c_str());
	nameEdit_.SetFocus();
	nameEdit_.SetSel(0, -1);

    pasteCheck_.EnableWindow(allowPaste_);
    pasteCheck_.SetCheck(pasteByDefault_);
	return FALSE;
}
