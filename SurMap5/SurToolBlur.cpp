#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolBlur.h"

IMPLEMENT_DYNAMIC(CSurToolBlur, CSurToolBase)
CSurToolBlur::CSurToolBlur(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	m_RadiusBlur.value=50;
	m_Intensity=20;
}

CSurToolBlur::~CSurToolBlur()
{
}

void CSurToolBlur::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolBlur, CSurToolBase)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CSurToolBlur message handlers

const int MIN_BLUR_INTENSITY=1;
const int MAX_BLUR_INTENSITY=100;

BOOL CSurToolBlur::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_RadiusBlur.Create(this, IDC_SLIDER_EDITRADIUS, IDC_EDITRADIUS);

	CSliderCtrl * slR;
	//»нициализаци€ слайдера - нтенсивность размывки
	slR=(CSliderCtrl *)GetDlgItem(IDC_SLIDER_BLUR_INTENSITY);
	slR->SetRange(MIN_BLUR_INTENSITY, MAX_BLUR_INTENSITY, TRUE);
	slR->SetPos(m_Intensity);


	flag_init_dialog=1;
	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CSurToolBlur::CallBack_DrawAuxData(void)
{
	drawCursorCircle();
	return true;
}

bool CSurToolBlur::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		CSurMap5App* pApp=(CSurMap5App*)AfxGetApp();
		int rad = getBrushRadius();
		m_RadiusBlur.value=rad;

		//
		//vMap.deltaZone(x, y, m_RadiusBlur.value, 0, 0, 0, 0);
		///if(state_radio_button_CircleQuadrate==0) 
		///	vMap.gaussFilter(x, y, m_RadiusBlur.value, (double)m_Intensity/10.0);
		///else 
		///	vMap.squareGaussFilter(x, y, m_RadiusBlur.value, (double)m_Intensity/10.0);
		vMap.gaussFilter(x, y, m_RadiusBlur.value, (double)m_Intensity/10.0);
	}
	return true;
}



void CSurToolBlur::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl * slR;
	//
	slR=(CSliderCtrl *)GetDlgItem(IDC_SLIDER_BLUR_INTENSITY);
	if(pScrollBar == (CScrollBar*)slR){
		m_Intensity=slR->GetPos();
	}

	CSurToolBase::OnHScroll(nSBCode, nPos, pScrollBar);
}
