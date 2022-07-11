// AddChainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AddChainDlg.h"


// CAddChainDlg dialog

IMPLEMENT_DYNAMIC(CAddChainDlg, CDialog)
CAddChainDlg::CAddChainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddChainDlg::IDD, pParent)
	, m_Name(_T("chain"))
	, m_nBegin(0)
	, m_nEnd(1000)
	, m_fTime(10.f)
	, m_bCycled(FALSE)
{
}

CAddChainDlg::~CAddChainDlg()
{
}

void CAddChainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CHAIN_NAME, m_Name);
	DDX_Text(pDX, IDC_BEGIN_FRAME, m_nBegin);
	DDX_Text(pDX, IDC_END_FRAME, m_nEnd);
	DDX_Text(pDX, IDC_TIME, m_fTime);
	DDX_Check(pDX, IDC_CYCLED, m_bCycled);
	DDX_Control(pDX, IDC_BEGIN_SPIN, m_BeginSpin);
	DDX_Control(pDX, IDC_END_SPIN, m_EndSpin);
	DDX_Control(pDX, IDC_TIME_SPIN, m_TimeSpin);
}


BEGIN_MESSAGE_MAP(CAddChainDlg, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_BEGIN_SPIN, OnDeltaposBeginSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_END_SPIN, OnDeltaposEndSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TIME_SPIN, OnDeltaposTimeSpin)
END_MESSAGE_MAP()


// CAddChainDlg message handlers

BOOL CAddChainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_nMax = pRootExport->GetNumFrames()-1;


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAddChainDlg::OnDeltaposBeginSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	UpdateData();
	m_nBegin -= pNMUpDown->iDelta;
	if (m_nBegin > m_nEnd)
		m_nBegin = m_nEnd;
	if (m_nBegin < 0)
		m_nBegin = 0;
	UpdateData(FALSE);

	*pResult = 0;
}

void CAddChainDlg::OnDeltaposEndSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	UpdateData();
	m_nEnd -= pNMUpDown->iDelta;
	if (m_nBegin > m_nEnd)
		m_nEnd = m_nBegin;
	if (m_nEnd > m_nMax)
		m_nEnd = m_nMax;
	UpdateData(FALSE);

	*pResult = 0;
}

void CAddChainDlg::OnDeltaposTimeSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	UpdateData();
	m_fTime -= (float)pNMUpDown->iDelta/10.0f;
	if (m_fTime < 0.0f)
		m_fTime = 0.0f;
	if(m_fTime > 20.0f)
		m_fTime = 20.0f;
	UpdateData(FALSE);

	*pResult = 0;
}

void CAddChainDlg::OnOK()
{
	UpdateData();
	CDialog::OnOK();
}
