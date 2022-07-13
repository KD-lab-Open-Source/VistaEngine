// SurToolSpecFilter.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolSpecFilter.h"
#include ".\surtoolspecfilter.h"


// CSurToolSpecFilter dialog

IMPLEMENT_DYNAMIC(CSurToolSpecFilter, CSurToolBase)
CSurToolSpecFilter::CSurToolSpecFilter(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	maxDH.value=3<<VX_FRACTION;
	maxDH.SetRange(1, 20<<VX_FRACTION);
	kDetail.value=20;//0.2
	kDetail.SetRange(1, 100);
	balansH.value=0;
	balansH.SetRange(-20, 20);
	kUnBrightness.value=0;
	kUnBrightness.SetRange(0, 100);
}

CSurToolSpecFilter::~CSurToolSpecFilter()
{
}

void CSurToolSpecFilter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CSurToolSpecFilter::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(maxDH.value, "maxDH", 0);
	ar.serialize(kDetail.value, "kDetail", 0);
	ar.serialize(balansH.value, "balansH", 0);
	ar.serialize(kUnBrightness.value, "kUnBrightness", 0);
}


BEGIN_MESSAGE_MAP(CSurToolSpecFilter, CSurToolBase)
END_MESSAGE_MAP()


// CSurToolSpecFilter message handlers

BOOL CSurToolSpecFilter::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	maxDH.Create(this, IDC_SLD_MAXDH, IDC_EDT_MAXDH);
	kDetail.Create(this, IDC_SLD_KDETAIL, IDC_EDT_KDETAIL);
	balansH.Create(this,  IDC_SLD_BALANSH, IDC_EDT_BALANSH);
	kUnBrightness.Create(this, IDC_SLD_KUNBRIGHTNESS, IDC_EDT_KUNBRIGHTNESS);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CSurToolSpecFilter::onOperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();
		vMap.specialFilter(x, y, rad, maxDH.value, (double)kDetail.value/30.0,
			(double)balansH.value/100., (double)kUnBrightness.value/100.);
	}
	return true;
}

bool CSurToolSpecFilter::onDrawAuxData(void)
{
	drawCursorCircle();
	return true;
}
