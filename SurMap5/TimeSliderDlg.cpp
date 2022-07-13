#include "stdafx.h"
#include "SurMap5.h"
#include "MainFrame.h"
#include ".\TimeSliderDlg.h"

#include "..\Render\inc\IVisGeneric.h"

#include "..\Environment\Environment.h"
#include "..\Water\SkyObject.h"

IMPLEMENT_DYNAMIC(CTimeSliderDlg, CDialog)
CTimeSliderDlg::CTimeSliderDlg(CMainFrame* mainFrame, CWnd* parent)
: CDialog(CTimeSliderDlg::IDD, parent)
, wasTimeFlowEnabled_(false)
{
	mainFrame->eventWorldChanged().registerListener(this);
}

CTimeSliderDlg::~CTimeSliderDlg()
{
}

void CTimeSliderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTimeSliderDlg, CDialog)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_TIME_EDIT, OnTimeEditChange)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CTimeSliderDlg::updateSlider()
{
	float time = environment->environmentTime()->GetTime();
	//time -= 6.0f;
	if(time < 0.0f)
		time += 24.0f;
	UINT pos = round(float(SLIDER_MAX - SLIDER_MIN) * time / 24.0f) + SLIDER_MIN;
	slider().SetPos(pos);
}

float posToTime(UINT pos){
	float time = /*6.0f + */float(pos) / float(CTimeSliderDlg::SLIDER_MAX - CTimeSliderDlg::SLIDER_MIN) * 24.0f;
	while(time > 24.0f)
		time -= 24.0f;
	return time;
}

void CTimeSliderDlg::updateText()
{
	float time = posToTime(slider().GetPos());
	XBuffer buf;
	buf.SetRadix(4);
	buf.SetDigits(4);
	buf <= time;
    SetDlgItemText(IDC_TIME_EDIT, buf);
}


void CTimeSliderDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(!environment)
		return;

	if((CWnd*)pScrollBar == (CWnd*)&slider()){
		float time = posToTime(slider().GetPos());
		updateText();
		environment->environmentTime()->SetTime(time);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CTimeSliderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

//	timeSlider_.Create(this, IDC_TIME_SLIDER, IDC_TIME_EDIT);
	slider().SetRange(SLIDER_MIN, SLIDER_MAX);
	return TRUE;
}

void CTimeSliderDlg::quant()
{
	static bool firstQuant = true;
	if(!environment)
		return;
	bool timeFlow = Environment::flag_EnableTimeFlow;

	if(timeFlow != wasTimeFlowEnabled_ || firstQuant){
		GetDlgItem(IDC_TIME_EDIT)->EnableWindow(!timeFlow);
		GetDlgItem(IDC_TIME_SLIDER)->EnableWindow(!timeFlow);
		updateSlider();
		updateText();
	}
	else
		if(timeFlow){
			updateSlider();
			updateText();
		}
	wasTimeFlowEnabled_ = timeFlow;
	firstQuant = false;
}

CSliderCtrl& CTimeSliderDlg::slider()
{
	CSliderCtrl* slider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_TIME_SLIDER));
	xassert(slider != 0);
	return *slider;
	//return timeSlider_;
}

void CTimeSliderDlg::OnOK()
{
}

void CTimeSliderDlg::OnCancel()
{
}

void CTimeSliderDlg::OnTimeEditChange()
{
	if(!environment || Environment::flag_EnableTimeFlow)
		return;

	CString text;
	GetDlgItemText(IDC_TIME_EDIT, text);
	float time = clamp(atof(text), 0.0f, 24.0f);
	environment->environmentTime()->SetTime(time);
	updateSlider();
}

void CTimeSliderDlg::onWorldChanged()
{
	wasTimeFlowEnabled_ = !Environment::flag_EnableTimeFlow;
	quant();
}

void CTimeSliderDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	__super::OnLButtonDown(nFlags, point);
}
