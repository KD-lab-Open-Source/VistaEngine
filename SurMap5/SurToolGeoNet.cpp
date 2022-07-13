// SurToolGeoNet.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolGeoNet.h"

#include "SurToolAux.h"
#include "Serialization\Serialization.h"


// CSurToolGeoNet dialog
const int MIN_GEONET_VX_HEIGHT=0;
const int MAX_GEONET_VX_HEIGHT=MAX_VX_HEIGHT;
const int MIN_GEONET_MESH=20;
const int MAX_GEONET_MESH=4000;
const int MIN_GEONET_POWERCELLSIZE=2;
const int MAX_GEONET_POWERCELLSIZE=8;
const int MIN_GEONET_KPOWERSHIFTCELLSIZE=0;
const int MAX_GEONET_KPOWERSHIFTCELLSIZE=7;
const int MIN_GEONET_CURVING=0;
const int MAX_GEONET_CURVING=4;

IMPLEMENT_DYNAMIC(CSurToolGeoNet, CSurToolBase)
CSurToolGeoNet::CSurToolGeoNet(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	flag_init_dialog=0;
	m_H.SetRange(MIN_GEONET_VX_HEIGHT, MAX_GEONET_VX_HEIGHT);
	m_GNoise.init(MIN_GEONET_VX_HEIGHT, MAX_GEONET_VX_HEIGHT, IDC_EDIT_GNOISE, this);
	m_Mesh.init(MIN_GEONET_MESH, MAX_GEONET_MESH, IDC_EDIT_MESH, this);

}

CSurToolGeoNet::~CSurToolGeoNet()
{
}

void CSurToolGeoNet::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(m_H.value, "m_H", 0);
}

void CSurToolGeoNet::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolGeoNet, CSurToolBase)
END_MESSAGE_MAP()


// CSurToolGeoNet message handlers

BOOL CSurToolGeoNet::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	///m_Width.Create(this, IDC_SLIDER_WIDTH, IDC_EDIT_WIDTH);
	///m_Height.Create(this, IDC_SLIDER_HEIGHT, IDC_EDIT_HEIGHT);
	m_H.Create(this, IDC_SLIDER_H, IDC_EDIT_H);


	m_Mesh.put();
	m_GNoise.put();

	CSliderCtrl * slR;
	//Инициализация слайдера для CELLPOWER
	slR=(CSliderCtrl *)GetDlgItem(IDC_SLIDER_CELLPOWER);
	slR->SetRange(MIN_GEONET_POWERCELLSIZE, MAX_GEONET_POWERCELLSIZE, TRUE);
	slR->SetPos(m_PowCellSize);
	//Инициализация слайдера для kSHIFT CELLPOWER
	slR=(CSliderCtrl *)GetDlgItem(IDC_SLIDER_SHIFTCELLPOWER);
	slR->SetRange(MIN_GEONET_KPOWERSHIFTCELLSIZE, MAX_GEONET_KPOWERSHIFTCELLSIZE, TRUE);
	slR->SetPos(m_PowShiftCS4RG);
	//Инициализация слайдера для Curving
	slR=(CSliderCtrl *)GetDlgItem(IDC_SLIDER_BORDERFORM);
	slR->SetRange(MIN_GEONET_CURVING, MAX_GEONET_CURVING, TRUE);
	slR->SetPos(m_Curving);

	flag_init_dialog=true;

	return FALSE;
}

bool CSurToolGeoNet::onOperationOnMap(int x, int y)
{
	CSurMap5App* pApp=(CSurMap5App*)AfxGetApp();
	int rad = getBrushRadius();

	if(vMap.isWorldLoaded()){
		//geoGeneration(x, y, m_Width.value, m_Height.value, m_H.value, m_PowCellSize, m_PowShiftCS4RG, m_Mesh.value, m_GNoise.value, m_Curving, m_checkbox_Inverse);
		geoGeneration(sGeoPMO(x, y, rad*2, rad*2, m_H.value, 8, 1, 100, 1600, 3, 0));
	}
	return true;
}


