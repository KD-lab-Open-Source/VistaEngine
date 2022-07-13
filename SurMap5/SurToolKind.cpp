// SurToolHardness.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolKind.h"

#include "SurToolAux.h"
#include "Serialization\Serialization.h"

#include "Game\Universe.h"
#include "Water\CircleManager.h"
#include "Terra\TerrainType.h"


// CSurToolHardness dialog
int CSurToolKind::filterMinHValue=0;
int CSurToolKind::filterMaxHValue=MAX_VX_HEIGHT;//_WHOLE;
bool CSurToolKind::flag_EnableFilterH=false;
IMPLEMENT_DYNAMIC(CSurToolKind, CSurToolBase)
CSurToolKind::CSurToolKind(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	state_radiobutton_Kind=0;
	m_FilterMinH.SetRange(0, MAX_VX_HEIGHT);//_WHOLE
	m_FilterMaxH.SetRange(0, MAX_VX_HEIGHT);//_WHOLE
	m_FilterMinH.value=filterMinHValue;
	m_FilterMaxH.value=filterMaxHValue;
}

CSurToolKind::~CSurToolKind()
{
}

void CSurToolKind::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(state_radiobutton_Kind, "state_radiobutton_Kind", 0);
}

void CSurToolKind::staticSerialize(Archive& ar)
{
	ar.serialize(filterMinHValue, "filterMinHValue", 0);
	ar.serialize(filterMaxHValue, "filterMaxHValue", 0);
	ar.serialize(flag_EnableFilterH, "flag_EnableFilterH", 0);
}

void CSurToolKind::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolKind, CSurToolBase)
	ON_WM_HSCROLL()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY16, OnKindButtonClicked)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_ENABLE_H_FILTER, OnBnClickedCheckEnableHFilter)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CSurToolHardness message handlers

BOOL CSurToolKind::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	//if(!flag_TREE_BAR_EXTENED_MODE){
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY2)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY3)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY4)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY5)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY6)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY7)->ShowWindow(SW_HIDE);
	//	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)->ShowWindow(SW_HIDE);
	//}

	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE0));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY2)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE1));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY3)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE2));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY4)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE3));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY5)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE4));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY6)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE5));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY7)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE6));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE7));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY9)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE8));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY10)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE9));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY11)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE10));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY12)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE11));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY13)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE12));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY14)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE13));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY15)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE14));
	GetDlgItem(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY16)->SetWindowText(TerrainTypeDescriptor::instance().nameAlt(TERRAIN_TYPE15));

	CheckRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY16, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1+state_radiobutton_Kind);//
	//state_radiobutton_Kind=GetCheckedRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY8)-IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1;//
	//setKindState(state_radiobutton_Kind);
	vMap.toShowSurKind(true);
	vMap.WorldRender();
	refrawAux();

	m_FilterMinH.Create(this, IDC_SLD_MINH, IDC_EDT_MINH);
	m_FilterMaxH.Create(this, IDC_SLD_MAXH, IDC_EDT_MAXH);
	m_FilterMinH.SetPos(filterMinHValue);
	m_FilterMaxH.SetPos(filterMaxHValue);

	//»нициализаци€ чек-бокса Enable 
	CButton* chB = (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	chB->SetCheck(flag_EnableFilterH);

	layout_.init(this);
	int i;
	for(i=0; i<8; i++)
		layout_.add(1, 0, 1, 0, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1+i);
	for(i=8; i<16; i++)
		layout_.add(1, 0, 1, 0, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1+i);
	layout_.add(1, 0, 1, 0, IDC_COLORTERRAINTYPE);

	return FALSE;
}

void CSurToolKind::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	layout_.onSize (cx, cy);
	refrawAux();
	Invalidate();
}

//void CSurToolKind::setKindState(int state)
//{
//	bool redraw=0;
//	switch(state){
//		case 0: case 1:
//			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowHardnessImpassability){
//				redraw=1;
//				vMap.toShowSpecialInfo(vrtMap::SSI_ShowHardnessImpassability);
//			}
//			break;
//		case 2: case 3: case 4: case 5:
//			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowKind){
//				redraw=1;
//				vMap.toShowSpecialInfo(vrtMap::SSI_ShowKind);
//			}
//			break;
//		case 6: case 7:
//			if(vMap.IsShowSpecialInfo()!=vrtMap::SSI_ShowHardnessImpassability){
//				redraw=1;
//				vMap.toShowSpecialInfo(vrtMap::SSI_ShowHardnessImpassability);
//			}
//			break;
//	}
//	if(redraw){
//		//CWaitCursor wait;
//		vMap.WorldRender();
//	}
//
//	state_radiobutton_Kind=state;
//}

void CSurToolKind::OnKindButtonClicked(UINT nID)
{
	state_radiobutton_Kind=GetCheckedRadioButton(IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1, IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY16)-IDC_RDB_INSDELHARDNESS_SETSURKIND_INSDELIMPASSABILITY1;//
	refrawAux();
	//setKindState(state);
}


bool CSurToolKind::onOperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded()) {
		int minfh=0, maxfh=MAX_VX_HEIGHT;
		if(flag_EnableFilterH){
			minfh=m_FilterMinH.value;//<<VX_FRACTION;
			maxfh=m_FilterMaxH.value;//<<VX_FRACTION;
		}
		int rad = getBrushRadius();

		//if(state_radiobutton_Kind==0){//insert hardness
		//	int result=vMap.drawHardnessCircle(x, y, rad, 0, minfh, maxfh);
		//	static bool flag_firstVisible=1;
		//	if(result==0 && flag_firstVisible) {
		//		::AfxMessageBox(TRANSLATE("“олько Dam может быть неразрушаемым!"));
		//		flag_firstVisible=0;
		//	}
		//}
		//else if(state_radiobutton_Kind==1){ //delete hardness
		//	vMap.drawHardnessCircle(x, y, rad, 1, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==2){ //kind 0
		//	vMap.drawInGrid(x, y, rad, 0, GRIDAT_MASK_SURFACE_KIND, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==3){ //kind 1
		//	vMap.drawInGrid(x, y, rad, 1, GRIDAT_MASK_SURFACE_KIND, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==4){ //kind 2
		//	vMap.drawInGrid(x, y, rad, 2, GRIDAT_MASK_SURFACE_KIND, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==5){ //kind 3
		//	vMap.drawInGrid(x, y, rad, 3, GRIDAT_MASK_SURFACE_KIND, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==6){ //draw impassability
		//	vMap.drawInGrid(x, y, rad, GRIDAT_IMPASSABILITY, GRIDAT_IMPASSABILITY, minfh, maxfh);
		//}
		//else if(state_radiobutton_Kind==7){ //erase impassability
		//	vMap.drawInGrid(x, y, rad, 0, GRIDAT_IMPASSABILITY, minfh, maxfh);
		//}
		vMap.drawInGrid(x, y, rad, state_radiobutton_Kind, GRIDAT_MASK_SURFACE_KIND, minfh, maxfh);

	}
	return true;
}

bool CSurToolKind::onDrawAuxData()
{
	int rad = getBrushRadius();
	universe()->circleManager()->addCircle(cursorPosition(), rad, CircleManagerParam(Color4c::BLUE));
	return true;
}


void CSurToolKind::OnDestroy()
{
	CSurToolBase::OnDestroy();

	if(vMap.isWorldLoaded()) {
		//vMap.toShowSpecialInfo(vrtMap::SSI_NoShow);
		vMap.toShowSurKind(false);
		vMap.WorldRender();
	}
}

void CSurToolKind::OnBnClickedCheckEnableHFilter()
{
	CButton* chB= (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	flag_EnableFilterH=chB->GetCheck();
}

void CSurToolKind::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ctrlID = pScrollBar->GetDlgCtrlID();

	static char fl_Recursion=0;
	if(fl_Recursion==0){//предотвращает рекурсию
		fl_Recursion=1;
		switch(ctrlID){
		case IDC_SLD_MINH:
			if(m_FilterMinH.value > m_FilterMaxH.value )
				m_FilterMaxH.SetPos(m_FilterMinH.value);
			//Invalidate(FALSE);
			filterMinHValue=m_FilterMinH.value;
			break;
		case IDC_SLD_MAXH:
			if(m_FilterMaxH.value < m_FilterMinH.value )
				m_FilterMinH.SetPos(m_FilterMaxH.value);
			//Invalidate(FALSE);
			filterMaxHValue=m_FilterMaxH.value;
			break;
		}
		fl_Recursion=0;
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSurToolKind::refrawAux()
{
	CStatic * wbmp=(CStatic *)GetDlgItem(IDC_COLORTERRAINTYPE);
	if(wbmp){
		CRect RR;
		wbmp->GetClientRect(&RR);
		wbmp->MapWindowPoints(this, &RR);
		//CDC MemDC;
		CDC* dc = GetDC();
		//MemDC.CreateCompatibleDC(dc);
		//m_bmpTerColor.CreateCompatibleBitmap(dc, RR.Width(), RR.Height());
		//CBitmap* oldBmp=MemDC.SelectObject(&m_bmpTerColor);
		Color4c tc= TerrainTypeDescriptor::instance().getColors()[state_radiobutton_Kind];
		COLORREF color(RGB(tc.r, tc.g, tc.b));
		dc->FillSolidRect(RR, color);
		//dc->BitBlt(RR.left, RR.top, RR.Width(), RR.Height(), &MemDC, 0, 0, SRCCOPY);
		//MemDC.SelectObject(oldBmp);
		//m_bmpTerColor.DeleteObject();
	}
}
