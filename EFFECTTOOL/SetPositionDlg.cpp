// SetPositionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EffectTool.h"
#include "SetPositionDlg.h"
#include ".\setpositiondlg.h"


// CSetPositionDlg dialog

IMPLEMENT_DYNAMIC(CSetPositionDlg, CDialog)
CSetPositionDlg::CSetPositionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSetPositionDlg::IDD, pParent)
	, m_fPosition(0)
{
}

CSetPositionDlg::~CSetPositionDlg()
{
}

void CSetPositionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_POSITION, m_fPosition);
}


BEGIN_MESSAGE_MAP(CSetPositionDlg, CDialog)
END_MESSAGE_MAP()


// CSetPositionDlg message handlers

void CSetPositionDlg::OnOK()
{
	UpdateData();
	CDialog::OnOK();
}
