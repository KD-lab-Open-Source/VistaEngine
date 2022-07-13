// SurToolHardness.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolKind.h"

#include "SurToolAux.h"
#include "Serialization.h"

#include "..\Game\Universe.h"
#include "..\Units\ExternalShow.h"


// CSurToolHardness dialog

IMPLEMENT_DYNAMIC(CSurToolKind, CSurToolBase)
CSurToolKind::CSurToolKind(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	state_radiobutton_Kind=0;
}

CSurToolKind::~CSurToolKind()
{
}

void CSurToolKind::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(state_radiobutton_Kind, "state_radiobutton_Kind", 0);
}

void CSurToolKind::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolKind, CSurToolBase)
	ON_CONTROL_RANGE (BN_CLICKED, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8, OnKindButtonClicked)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CSurToolHardness message handlers

BOOL CSurToolKind::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	if(!flag_TREE_BAR_EXTENED_MODE){
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY2)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY3)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY4)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY5)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY6)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY7)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)->ShowWindow(SW_HIDE);
	}

	CheckRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1+state_radiobutton_Kind);//
	//state_radiobutton_Kind=GetCheckedRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)-IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1;//
	setKindState(state_radiobutton_Kind);

	return FALSE;
}

void CSurToolKind::setKindState(int state)
{
	bool redraw=0;
	switch(state){
		case 0: case 1:
			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowHardness){
				redraw=1;
				vMap.toShowSpecialInfo(vrtMap::SSI_ShowHardness);
			}
			break;
		case 2: case 3: case 4: case 5:
			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowKind){
				redraw=1;
				vMap.toShowSpecialInfo(vrtMap::SSI_ShowKind);
			}
			break;
		case 6: case 7:
			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowImpassability){
				redraw=1;
				vMap.toShowSpecialInfo(vrtMap::SSI_ShowImpassability);
			}
			break;
	}
	if(redraw){
		//CWaitCursor wait;
		vMap.WorldRender();
	}

	state_radiobutton_Kind=state;
}

void CSurToolKind::OnKindButtonClicked(UINT nID)
{
	// TODO: Add your control notification handler code here

	int state=GetCheckedRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)-IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1;//
	setKindState(state);
}


bool CSurToolKind::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();

		if(state_radiobutton_Kind==0){//insert hardness
			int result=vMap.drawHardnessCircle(x, y, rad, 0);
			static bool flag_firstVisible=1;
			if(result==0 && flag_firstVisible) {
				::AfxMessageBox(TRANSLATE("Только Dam может быть неразрушаемым!"));
				flag_firstVisible=0;
			}
		}
		else if(state_radiobutton_Kind==1){ //delete hardness
			vMap.drawHardnessCircle(x, y, rad, 1);
		}
		else if(state_radiobutton_Kind==2){ //kind 0
			vMap.drawInGrid(x, y, rad, 0, GRIDAT_MASK_SURFACE_KIND);
		}
		else if(state_radiobutton_Kind==3){ //kind 1
			vMap.drawInGrid(x, y, rad, 1, GRIDAT_MASK_SURFACE_KIND);
		}
		else if(state_radiobutton_Kind==4){ //kind 2
			vMap.drawInGrid(x, y, rad, 2, GRIDAT_MASK_SURFACE_KIND);
		}
		else if(state_radiobutton_Kind==5){ //kind 3
			vMap.drawInGrid(x, y, rad, 3, GRIDAT_MASK_SURFACE_KIND);
		}
		else if(state_radiobutton_Kind==6){ //draw impassability
			vMap.drawInGrid(x, y, rad, GRIDAT_IMPASSABILITY, GRIDAT_IMPASSABILITY);
		}
		else if(state_radiobutton_Kind==7){ //erase impassability
			vMap.drawInGrid(x, y, rad, 0, GRIDAT_IMPASSABILITY);
		}

	}
	return true;
}

bool CSurToolKind::CallBack_DrawAuxData()
{
	int rad = getBrushRadius();
	universe()->circleShow()->Circle(cursorPosition(), rad, CircleColor(sColor4c(BLUE)));
	return true;
}


void CSurToolKind::OnDestroy()
{
	CSurToolBase::OnDestroy();

	// TODO: Add your message handler code here
	if(vMap.isWorldLoaded()) {
		vMap.toShowSpecialInfo(vrtMap::SSI_NoShow);
		vMap.WorldRender();
	}
}
