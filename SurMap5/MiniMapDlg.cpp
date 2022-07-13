// MiniMapDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MiniMapDlg.h"
#include ".\minimapdlg.h"


// CMiniMapDlg dialog

IMPLEMENT_DYNAMIC(CMiniMapDlg, CDialog)
CMiniMapDlg::CMiniMapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMiniMapDlg::IDD, pParent)
{
}

CMiniMapDlg::~CMiniMapDlg()
{
}

void CMiniMapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
//	DDX_Control(pDX, IDC_MINIMAP_WINDOW, m_ctlMiniMap);
}


BEGIN_MESSAGE_MAP(CMiniMapDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CMiniMapDlg message handlers
BOOL CMiniMapDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect clientRect;
	GetClientRect (&clientRect);

	m_ctlMiniMap.Create (WS_VISIBLE | WS_CHILD | WS_BORDER, &clientRect, this);
	m_ctlMiniMap.ShowWindow (SW_SHOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMiniMapDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (::IsWindow(m_ctlMiniMap.GetSafeHwnd ()))
		m_ctlMiniMap.MoveWindow (0, 0, cx, cy, TRUE);
}
