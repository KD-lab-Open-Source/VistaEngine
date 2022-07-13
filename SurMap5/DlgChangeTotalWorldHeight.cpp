#include "stdafx.h"
#include "SurMap5.h"
#include "DlgChangeTotalWorldHeight.h"

#include "terra\vmap.h"
#include "LIMITS.H"
#include "dlgchangetotalworldheight.h"
#include "Serialization\Dictionary.h"

static void world2Histogram(HistogramDate& outHistArr)
{
	if(!vMap.isWorldLoaded()) return;
	int histArr[HistogramDate::HISTOGRAM_ARRAY_SIZE];
	int minVx=INT_MAX;
	int maxVx=INT_MIN;
	memset(histArr, '\0', sizeof(histArr));
	int i, j;
	for(i=0; i<vMap.V_SIZE; i++){
		for(j=0; j<vMap.H_SIZE; j++){
			int h=vMap.getAlt(j,i);
			if(minVx>h) minVx=h;
			if(maxVx<h) maxVx=h;
			histArr[h*HistogramDate::HISTOGRAM_ARRAY_SIZE/(MAX_VX_HEIGHT+1)]++;
		}
	}
	int idxMax=0;
	int maxVal=0;
	for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
		if(histArr[i] > maxVal){
			maxVal=histArr[i];
			idxMax=i;
		}
	}
	float maxValF=sqrt((double)maxVal);
	for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
		int s=0;
		if(histArr[i]){
			s=round(sqrt((double)histArr[i])/maxValF *255.);
			if(s>MAX_VX_HEIGHT_WHOLE)s=MAX_VX_HEIGHT_WHOLE;
			if(s<4)s=4;
		}
		outHistArr.histogramArr[i]=s;
	}
	outHistArr.minVxHeight=minVx;
	outHistArr.maxVxHeight=maxVx;
}

static void inHistogram2OutHistogram(HistogramDate& inHistogram, HistogramDate& outHistogram, int dVxH, float kScale)
{
	outHistogram.minVxHeight=inHistogram.minVxHeight+dVxH;
	int dMax=inHistogram.maxVxHeight-inHistogram.minVxHeight;
	outHistogram.maxVxHeight=inHistogram.minVxHeight+dVxH+round(dMax*kScale);

	int histArr[HistogramDate::HISTOGRAM_ARRAY_SIZE];
	memset(histArr, '\0', sizeof(histArr));
	int i;
	for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
		int idxout=((inHistogram.minVxHeight+dVxH)>>VX_FRACTION)+round((i-(inHistogram.minVxHeight>>VX_FRACTION))*kScale);
		if(idxout>=0 && idxout<HistogramDate::HISTOGRAM_ARRAY_SIZE){
			histArr[idxout]+=inHistogram.histogramArr[i];
		}
		//int d=round((float)(i-(inHistogram.minVxHeight>>VX_FRACTION))/kScale);
		//int idxin=i+(dVxH>>VX_FRACTION)+d;
		//if(idxin>0 && idxin < HistogramDate::HISTOGRAM_ARRAY_SIZE){
		//	outHistogram.histogramArr[i]=inHistogram.histogramArr[idxin];
		//}
		//else 
		//	outHistogram.histogramArr[i]=0;
	}
	int idxMax=0;
	int maxVal=0;
	for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
		if(histArr[i] > maxVal){
			maxVal=histArr[i];
			idxMax=i;
		}
	}
	for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
		int s=0;
		if(histArr[i]){
			s=round((double)histArr[i]/(double)maxVal *255.);
			if(s>MAX_VX_HEIGHT_WHOLE)s=MAX_VX_HEIGHT_WHOLE;
			if(s<4)s=4;
		}
		outHistogram.histogramArr[i]=s;
	}
}

void CDlgChangeTotalWorldHeight::drawHisogram(CPaintDC& dc, int HISTOGRAM_WND, HistogramDate& histogram)
{
	CStatic * wbmp=(CStatic *)GetDlgItem(HISTOGRAM_WND);
	CRect RR,RR1;
	wbmp->GetClientRect(&RR);
	wbmp->MapWindowPoints(this, &RR);

	CDC MemDC;
	MemDC.CreateCompatibleDC(&dc);
	m_bmpHistogram.CreateCompatibleBitmap(&dc, RR.Width(), RR.Height());
	CBitmap* oldBmp=MemDC.SelectObject(&m_bmpHistogram);
	{
		//const COLORREF MARK=RGB(0,0,0);
		//const COLORREF UNMARK=RGB(140,140,140);

		const COLORREF FON=RGB(127,127,127);
		const COLORREF COLUMN=RGB(255,255,255);
		CRect rr;
		wbmp->GetClientRect(&rr);
		//COLORREF * curCol;

		int yend=rr.Height()-1;
		float xstep=(float)rr.Width()/(float)HistogramDate::HISTOGRAM_ARRAY_SIZE;
		float ystep=(float)rr.Height()/(float)MAX_VX_HEIGHT_WHOLE;
		MemDC.FillSolidRect(rr, FON);
		int i;
		for(i=0; i<HistogramDate::HISTOGRAM_ARRAY_SIZE; i++){
			int h=round(histogram.histogramArr[i]*ystep);
			int begx=round(xstep*i);
			int endx=round(xstep*(i+1));
			MemDC.FillSolidRect(begx, yend-h, 1, h, COLUMN);
			//MemDC.FillSolidRect(xrct+1, yrct+1, cxrct-2, cyrct-2, SELECT);

		}
		//MemDC.FillSolidRect(xrct+1, yrct+1, cxrct-2, cyrct-2, SELECT);
	}
	//WINDOWPLACEMENT wp;
	//wbmp->GetWindowPlacement(&wp);
	//dc.BitBlt(wp.rcNormalPosition.left, wp.rcNormalPosition.top, RR.Width(), RR.Height(), &MemDC, 0, 0, SRCCOPY);
	dc.BitBlt(RR.left, RR.top, RR.Width(), RR.Height(), &MemDC, 0, 0, SRCCOPY);

	MemDC.SelectObject(oldBmp); 
	m_bmpHistogram.DeleteObject();

}

static const int MIN_DELTA_H=-128<<VX_FRACTION;
static const int MAX_DELTA_H=128<<VX_FRACTION;
static const int MIN_SCALE_H=10;
static const int MAX_SCALE_H=300;
IMPLEMENT_DYNAMIC(CDlgChangeTotalWorldHeight, CDialog)
CDlgChangeTotalWorldHeight::CDlgChangeTotalWorldHeight(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgChangeTotalWorldHeight::IDD, pParent)
{
	m_deltaH.SetRange(MIN_DELTA_H, MAX_DELTA_H);
	m_deltaH.value=0;
	m_scaleH.SetRange(MIN_SCALE_H, MAX_SCALE_H);
	m_scaleH.value=100;

}

CDlgChangeTotalWorldHeight::~CDlgChangeTotalWorldHeight()
{
}

void CDlgChangeTotalWorldHeight::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ATTRIB_EDITOR, m_ctlAttribEditor);
}


BEGIN_MESSAGE_MAP(CDlgChangeTotalWorldHeight, CDialog)
	ON_WM_HSCROLL()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CDlgChangeTotalWorldHeight::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_deltaH.Create(this, IDC_SLDR_DELTA_H, IDC_EDT_DELTA_H);
	m_scaleH.Create(this, IDC_SLDR_SCALE_H, IDC_EDT_SCALE_H);

	CWaitCursor wait;
	world2Histogram(inputHistogram);
	inHistogram2OutHistogram(inputHistogram, outPutHistogram, m_deltaH.value, (float)m_scaleH.value/100.f);

	static_cast<vrtMapCreationParam&>(m_changeParam) = static_cast<vrtMapCreationParam>(vMap);
	m_ctlAttribEditor.setStyle(CAttribEditorCtrl::AUTO_SIZE);
	m_ctlAttribEditor.initControl ();
	m_ctlAttribEditor.attachSerializer(Serializer(m_changeParam, "m_changeParam", "Свойства карты"));
	UpdateData(FALSE);

	return TRUE;
}

void CDlgChangeTotalWorldHeight::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl * slR1;
	CSliderCtrl * slR2;
	//
	slR1=(CSliderCtrl*)GetDlgItem(IDC_SLDR_DELTA_H);
	slR2=(CSliderCtrl*)GetDlgItem(IDC_SLDR_SCALE_H);
	if(pScrollBar==(CScrollBar*)slR1 || pScrollBar==(CScrollBar*)slR2){
		//Действие
		inHistogram2OutHistogram(inputHistogram, outPutHistogram, m_deltaH.value, (float)m_scaleH.value/100.f);
		Invalidate(FALSE);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CDlgChangeTotalWorldHeight::OnPaint()
{
	CPaintDC dc(this);

	CWnd* pw;
	char buf[MAX_PATH];
	pw=GetDlgItem(IDC_EDT_INPUT_MIN_HEIGHT);
	pw->SetWindowText((char*)convert_vox2vid(inputHistogram.minVxHeight, buf));
	pw=GetDlgItem(IDC_EDT_INPUT_MAX_HEIGHT);
	pw->SetWindowText((char*)convert_vox2vid(inputHistogram.maxVxHeight, buf));
	drawHisogram(dc, IDC_HISTOGRAM_WORLD_INPUT, inputHistogram);
	pw=GetDlgItem(IDC_EDT_OUT_MIN_HEIGHT);
	pw->SetWindowText((char*)convert_vox2vid(outPutHistogram.minVxHeight, buf));
	pw=GetDlgItem(IDC_EDT_OUT_MAX_HEIGHT);
	pw->SetWindowText((char*)convert_vox2vid(outPutHistogram.maxVxHeight, buf));
	drawHisogram(dc, IDC_HISTOGRAM_WORLD_OUTPUT, outPutHistogram);
}

void CDlgChangeTotalWorldHeight::OnOK()
{
	UpdateData(TRUE);

	CDialog::OnOK();
}
