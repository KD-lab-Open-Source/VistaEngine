// DlgWorldName.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "DlgWorldName.h"
#include "dlgworldname.h"


// CDlgWorldName dialog

IMPLEMENT_DYNAMIC(CDlgWorldName, CDialog)
CDlgWorldName::CDlgWorldName(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgWorldName::IDD, pParent)
	, newWorldName(_T("Default"))
{
}

CDlgWorldName::~CDlgWorldName()
{
}

void CDlgWorldName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDT_WORLDNAME, newWorldName);
}


BEGIN_MESSAGE_MAP(CDlgWorldName, CDialog)
END_MESSAGE_MAP()


// CDlgWorldName message handlers

BOOL CDlgWorldName::OnInitDialog()
{
	CDialog::OnInitDialog();

	//CEdit* pWnd=(CEdit*)GetDlgItem(IDC_EDT_WORLDNAME);
	//pWnd->SetFocus();
	return TRUE;
}
