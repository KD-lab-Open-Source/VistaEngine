// BaseTreeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "BaseTreeDlg.h"
#include ".\basetreedlg.h"


// CBaseTreeDlg dialog

IMPLEMENT_DYNAMIC(CBaseTreeDlg, CDialog)
CBaseTreeDlg::CBaseTreeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBaseTreeDlg::IDD, pParent)
{
	flag_init_dialog=0;
}

CBaseTreeDlg::~CBaseTreeDlg()
{
	flag_init_dialog=0;
}

BOOL CBaseTreeDlg::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	flag_init_dialog=0;
	return CDialog::DestroyWindow();
}


BOOL CBaseTreeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	flag_init_dialog=1;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CBaseTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CBaseTreeDlg, CDialog)
//	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_DLG, OnTvnSelchangedTree1)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CBaseTreeDlg message handlers

//void CBaseTreeDlg::OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
//	// TODO: Add your control notification handler code here
//	*pResult = 0;
//}

void CBaseTreeDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if(flag_init_dialog){
		CRect sz;
		GetClientRect(sz);
		sz.DeflateRect(5,5);

		CTreeCtrl * trC=(CTreeCtrl*)GetDlgItem(IDC_TREE_CTRL);
		//trC->MoveWindow(sz);
	}
}
