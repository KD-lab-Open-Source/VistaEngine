#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolWater.h"

#include "..\Environment\Environment.h"
#include "..\water\Water.h"
#include "Serialization.h"

// CSurToolWaterSrc dialog

IMPLEMENT_DYNAMIC(CSurToolWaterSrc, CSurToolBase)
CSurToolWaterSrc::CSurToolWaterSrc(int idd, CWnd* pParent /*=NULL*/)
	: CSurToolBase(idd, pParent)
{
	flag_repeatOperationEnable=false;
	state_radio_button_InsDel=0;
	m_DeltaH.SetRange(0, 10<<VX_FRACTION);
	m_DeltaH.value=1<<VX_FRACTION;
}

CSurToolWaterSrc::~CSurToolWaterSrc()
{
}

void CSurToolWaterSrc::serialize(Archive& ar) 
{
	CSurToolBase::serialize(ar);
	ar.serialize(m_DeltaH.value, "m_DeltaH", 0);
	ar.serialize(state_radio_button_InsDel, "state_radio_button_InsDel", 0);
}

void CSurToolWaterSrc::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolWaterSrc, CSurToolBase)
	ON_BN_CLICKED(IDC_RDB_INSERTDELETE_SOURCE1, OnBnClickedRdbInsertdeleteSource1)
	ON_BN_CLICKED(IDC_RDB_INSERTDELETE_SOURCE2, OnBnClickedRdbInsertdeleteSource2)
END_MESSAGE_MAP()


// CSurToolWaterSrc message handlers


BOOL CSurToolWaterSrc::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	CheckRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2, IDC_RDB_INSERTDELETE_SOURCE1+state_radio_button_InsDel);//
	state_radio_button_InsDel=GetCheckedRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2)-IDC_RDB_INSERTDELETE_SOURCE1;//

	m_DeltaH.Create(this, IDC_SLDR_EDT_DELTALEVELING_H2O, IDC_EDT_DELTALEVELING_H2O);


	return FALSE;
}

void CSurToolWaterSrc::OnBnClickedRdbInsertdeleteSource1()
{
	state_radio_button_InsDel=GetCheckedRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2)-IDC_RDB_INSERTDELETE_SOURCE1;//
}

void CSurToolWaterSrc::OnBnClickedRdbInsertdeleteSource2()
{
	state_radio_button_InsDel=GetCheckedRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2)-IDC_RDB_INSERTDELETE_SOURCE1;//
}

bool CSurToolWaterSrc::CallBack_DrawAuxData(void)
{
	drawCursorCircle ();
	return true;
}

//////////////////////////////////////////////////////////////////

BOOL CSurToolWater::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	if(!flag_TREE_BAR_EXTENED_MODE){
		CWnd* cW;
		cW=GetDlgItem(IDC_RDB_INSERTDELETE_SOURCE1);
		cW->ShowWindow(SW_HIDE);
		cW=GetDlgItem(IDC_RDB_INSERTDELETE_SOURCE2);
		cW->ShowWindow(SW_HIDE);
	}

	CWnd* cW;
	cW=GetDlgItem(IDC_SLDR_EDT_DELTALEVELING_H2O);
	cW->ShowWindow(SW_HIDE);
	cW=GetDlgItem(IDC_EDT_DELTALEVELING_H2O);
	cW->ShowWindow(SW_HIDE);

	CheckRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2, IDC_RDB_INSERTDELETE_SOURCE1+state_radio_button_InsDel);//
	state_radio_button_InsDel=GetCheckedRadioButton(IDC_RDB_INSERTDELETE_SOURCE1, IDC_RDB_INSERTDELETE_SOURCE2)-IDC_RDB_INSERTDELETE_SOURCE1;//


	return FALSE;
}

void CSurToolWater::serialize(Archive& ar) 
{
	CSurToolBase::serialize(ar);
	ar.serialize(state_radio_button_InsDel, "state_radio_button_InsDel", 0);
}

bool CSurToolWater::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded() && environment && environment->water()) {
		int rad = getBrushRadius();

		if(state_radio_button_InsDel==0){//insert
			const float dhWater=2.0f;
			//float hEnv = environment->water()->GetEnvironmentWater();
			float hWater = environment->water()->GetZ(x,y);
			float hSur=(float)vMap.GetAlt(x,y)*VOXEL_DIVIDER;
			float h=max(hWater, hSur)+dhWater;
			environment->water()->SetWaterRect(x,y,h,rad*2);
		}
		else{//delete
			environment->water()->SetWaterRect(x,y,0.f,rad*2);
		}

	}
	return true;
}

bool CSurToolWater::CallBack_DrawAuxData(void)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
