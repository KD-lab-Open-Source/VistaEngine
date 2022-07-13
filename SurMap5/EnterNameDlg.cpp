// EnterNameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "EnterNameDlg.h"
#include "enternamedlg.h"


// CEnterNameDlg dialog

IMPLEMENT_DYNAMIC(CEnterNameDlg, CDialog)
CEnterNameDlg::CEnterNameDlg(CWnd* pParent /*=NULL*/)
: CDialog(CEnterNameDlg::IDD, pParent)
{
	strcpy(buf, "");
}

CEnterNameDlg::CEnterNameDlg(const char* name, CWnd* parent)
: CDialog(CEnterNameDlg::IDD, parent)
{
	strncpy(buf, name, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
}

CEnterNameDlg::~CEnterNameDlg()
{
}

void CEnterNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, text);
}


BEGIN_MESSAGE_MAP(CEnterNameDlg, CDialog)
END_MESSAGE_MAP()


// CEnterNameDlg message handlers

void CEnterNameDlg::OnOK()
{
	text.GetLine(0,buf,sizeof(buf) - 1);
	CDialog::OnOK();
}

void CEnterNameDlg::OnCancel()
{
	CDialog::OnCancel();
}

BOOL CEnterNameDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	text.SetWindowText(buf);
	
	return TRUE;
}
