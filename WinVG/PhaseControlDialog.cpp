// PhaseControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WinVG.h"
#include "PhaseControlDialog.h"
#include ".\phasecontroldialog.h"
#include "MainFrm.h"
// CPhaseControlDialog dialog

IMPLEMENT_DYNAMIC(CPhaseControlDialog, CDialog)
CPhaseControlDialog::CPhaseControlDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPhaseControlDialog::IDD, pParent)
	, phaseEdit(_T(""))
{
	frame_ = NULL;
}

CPhaseControlDialog::~CPhaseControlDialog()
{
}

void CPhaseControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PHASE_SLIDER, phaseSlider);
	DDX_Text(pDX, IDC_PHASE_EDIT, phaseEdit);
}


BEGIN_MESSAGE_MAP(CPhaseControlDialog, CDialog)
	ON_WM_HSCROLL()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CPhaseControlDialog message handlers

void CPhaseControlDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar->GetDlgCtrlID() == IDC_PHASE_SLIDER)
	{
		if(mainFrame->IsReverse())
			frame_->SetPhase((1000-(float)phaseSlider.GetPos())/1000.f);
		else
			frame_->SetPhase(((float)phaseSlider.GetPos())/1000.f);
		phaseEdit.Format("%f",((float)phaseSlider.GetPos())/1000.f);
		UpdateData(FALSE);
	}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CPhaseControlDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	phaseSlider.SetRange(0,1000);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPhaseControlDialog::SetFrame(cFrame* frame, CMainFrame* frm)
{
	frame_ = frame;
	mainFrame = frm;
}
void CPhaseControlDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		mainFrame->AnimationEnable(false);
		phaseSlider.SetPos(frame_->GetPhase()*1000);
		phaseEdit.Format("%f",frame_->GetPhase());
		UpdateData(FALSE);
	}
}

void CPhaseControlDialog::OnCancel()
{
	mainFrame->AnimationEnable(true);

	CDialog::OnCancel();
}

void CPhaseControlDialog::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
