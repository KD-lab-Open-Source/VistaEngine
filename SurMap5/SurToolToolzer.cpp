#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolToolzer.h"
#include "SurToolAux.h"
#include "..\Terra\vmap.inl"

// CSurToolToolzer dialog
const int MIN_TOOLZER_DELTA_H=0;
const int MAX_TOOLZER_DELTA_H=(256<<VX_FRACTION)-1;

const int MIN_FILTER_H=0;
const int MAX_FILTER_H=MAX_VX_HEIGHT;//_WHOLE;
int CSurToolToolzer::filterMinHValue=0;
int CSurToolToolzer::filterMaxHValue=MAX_VX_HEIGHT;//_WHOLE;
bool CSurToolToolzer::flag_EnableFilterH=false;

IMPLEMENT_DYNAMIC(CSurToolToolzer, CSurToolBase)
CSurToolToolzer::CSurToolToolzer(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	m_DeltaH.value=10<<VX_FRACTION;
	m_RadiusToolzer.value=20;
	m_Roughness.value=90;
	state_radio_button_DigPut=0;
	state_level_ZL_or_level_h=0;

	idxCurToolzerType=0;

	m_DeltaH.SetRange(MIN_TOOLZER_DELTA_H, MAX_TOOLZER_DELTA_H);
	m_DeltaH.value=0;

	m_FilterMinH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMaxH.SetRange(MIN_FILTER_H, MAX_FILTER_H);
	m_FilterMinH.value=filterMinHValue;
	m_FilterMaxH.value=filterMaxHValue;

}

CSurToolToolzer::~CSurToolToolzer()
{
}

void CSurToolToolzer::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(m_DeltaH.value, "m_DeltaH", 0);
	ar.serialize(state_radio_button_DigPut, "state_radio_button_DigPut", 0);
	ar.serialize(idxCurToolzerType, "idxCurToolzerType", 0);
	ar.serialize(m_Roughness.value, "m_Roughness", 0);
}

void CSurToolToolzer::staticSerialize(Archive& ar)
{
	ar.serialize(filterMinHValue, "filterMinHValue", 0);
	ar.serialize(filterMaxHValue, "filterMaxHValue", 0);
	ar.serialize(flag_EnableFilterH, "flag_EnableFilterH", 0);
}


void CSurToolToolzer::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolToolzer, CSurToolBase)
//	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_RADIO_DIGPUT1, OnBnClickedRadioDigput1)
	ON_BN_CLICKED(IDC_RADIO_DIGPUT2, OnBnClickedRadioDigput2)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_H_FILTER, OnBnClickedCheckEnableHFilter)
	ON_CBN_SELCHANGE(IDC_CBOXEX_TOOLZERTYPE, OnCbnSelchangeCboxexToolzertype)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CSurToolToolzer message handlers

BOOL CSurToolToolzer::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	m_RadiusToolzer.Create(this, IDC_SLIDER_EDITRADIUS, IDC_EDITRADIUS);
	m_DeltaH.Create(this, IDC_SLIDER_EDITDELTAH, IDC_EDITDELTAH);

	//m_LevelingH.Create(this, IDC_SLDR_EDT_LEVELING_H, IDC_EDT_LEVELING_H);
	m_Roughness.Create(this, IDC_SLDR_ROUGHNESS, IDC_EDT_ROUGHNESS);


	CheckRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2, IDC_RADIO_DIGPUT1+state_radio_button_DigPut);//
	state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;//

	//CheckDlgButton(IDC_LIMIT_HEIGHT_CHECK, state_free_or_level_h);
	//state_free_or_level_h = ((CButton*)(GetDlgItem(IDC_LIMIT_HEIGHT_CHECK)))->GetCheck();


	m_FilterMinH.Create(this, IDC_SLD_MINH, IDC_EDT_MINH);
	m_FilterMaxH.Create(this, IDC_SLD_MAXH, IDC_EDT_MAXH);
	m_FilterMinH.SetPos(filterMinHValue);
	m_FilterMaxH.SetPos(filterMaxHValue);

	//»нициализаци€ чек-бокса Enable 
	CButton* chB = (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	chB->SetCheck(flag_EnableFilterH);


	CComboBox * ComBox = (CComboBox *) GetDlgItem(IDC_CBOXEX_TOOLZERTYPE);
	ComBox->AddString("Toolzer");
	ComBox->AddString("Exp");
	ComBox->AddString("PNoise");
	ComBox->AddString("MPD");
	ComBox->SetCurSel(idxCurToolzerType);


	if(!flag_TREE_BAR_EXTENED_MODE){
		GetDlgItem(IDC_RADIO_DIGPUT1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RADIO_DIGPUT2)->ShowWindow(SW_HIDE);

		m_RadiusToolzer.ShowControl(false);
		//m_DeltaH.ShowControl(false);
		m_Roughness.ShowControl(false);

		GetDlgItem(IDC_CBOXEX_TOOLZERTYPE)->ShowWindow(SW_HIDE);
	}

	layout_.init(this);
	//layout_.add(1, 1, 1, 0, IDC_SLDR_EDT_LEVELING_H);
	layout_.add(1, 1, 1, 0, IDC_SLIDER_EDITDELTAH);
	layout_.add(1, 1, 1, 0, IDC_SLD_MINH);
	layout_.add(1, 1, 1, 0, IDC_SLD_MAXH);

	flag_init_dialog=1;
	//OnLimitHeightChecked();
	OnBnClickedCheckEnableHFilter();
	return FALSE;
}


void CSurToolToolzer::OnSize(UINT type, int cx, int cy)
{
	layout_.onSize(cx, cy);
	__super::OnSize(type, cx, cy);
}


BOOL CSurToolToolzer::Create(UINT nIDTemplate, CWnd* pParentWnd)
{
	return CSurToolBase::Create(nIDTemplate, pParentWnd);
}

bool CSurToolToolzer::onOperationOnMap(int x, int y)
{
	int dh=m_DeltaH.value;
	if(state_radio_button_DigPut==0) dh=-dh;
	else dh=dh;
	
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();
		e_BrushForm bf = BRUSHFORM_CIRCLE;
		m_RadiusToolzer.value=rad;

		int minfh=0, maxfh=MAX_VX_HEIGHT;
		if(flag_EnableFilterH){
			minfh=m_FilterMinH.value;//<<VX_FRACTION;
			maxfh=m_FilterMaxH.value;//<<VX_FRACTION;
		}

		float roughness=(float)m_Roughness.value/100.f;
		switch(bf){
		case BRUSHFORM_CIRCLE:
			switch(idxCurToolzerType){
			case 0:
				vMap.deltaZone(sToolzerPMO(x, y, m_RadiusToolzer.value, 9, dh, 0, 0, minfh, maxfh));
				break;
			case 1:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodExp(2,dh, 0), minfh, maxfh);
				break;
			case 2:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodPN(2,dh, 0), minfh, maxfh);
				break;
			case 3:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodMPD(2,dh, 0, roughness), minfh, maxfh);
				break;
			}
            break;
		case BRUSHFORM_SQUARE:
			//vMap.squareDeltaZone(x, y, m_RadiusToolzer.value, 10, dh, 0, 0);
			switch(idxCurToolzerType){
			case 0:
				vMap.squareDeltaZone(sSquareToolzerPMO(x, y, m_RadiusToolzer.value, 1, dh, 0, 0, minfh, maxfh));
				break;
			case 1:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodExp(3,dh, 0), minfh, maxfh);
				break;
			case 2:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodPN(3,dh, 0), minfh, maxfh);
				break;
			case 3:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodMPD(3,dh, 0, roughness), minfh, maxfh);
				break;
			}
			break;
		}
		//if(state_radio_button_CircleQuadrate==0) 
		//	vMap.deltaZone(x, y, m_RadiusToolzer.value, m_ToolzerFrontForm, dh, m_ToolzerAllocForm, 0);
		//else 
		//	vMap.squareDeltaZone(x, y, m_RadiusToolzer.value, m_ToolzerFrontForm, dh, m_ToolzerAllocForm, 0);
		///vMap.deltaZone(x, y, m_RadiusToolzer.value, 10, dh, 0, 0);
	}
	return true;
}

bool CSurToolToolzer::onDrawAuxData(void)
{
	drawCursorCircle();
	return true;
}

void CSurToolToolzer::OnCbnSelchangeCboxexToolzertype()
{
	if(flag_init_dialog) {
		CComboBox* comBox;
		comBox=(CComboBox*)GetDlgItem(IDC_CBOXEX_TOOLZERTYPE);
		idxCurToolzerType=comBox->GetCurSel();
	}
}


void CSurToolToolzer::OnBnClickedRadioDigput1()
{
	if(flag_init_dialog) state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;
}

void CSurToolToolzer::OnBnClickedRadioDigput2()
{
	if(flag_init_dialog) state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;
}

//void CSurToolToolzer::OnLimitHeightChecked()
//{
//	if(flag_init_dialog){
//		state_free_or_level_h = ((CButton*)(GetDlgItem(IDC_LIMIT_HEIGHT_CHECK)))->GetCheck();
//		BOOL enable = state_free_or_level_h == 0 ? 0 : 1;
//
//		m_LevelingH.editWin.EnableWindow(enable);
//		m_LevelingH.EnableWindow(enable);
//	}
//}

void CSurToolToolzer::OnDestroy()
{
	CSurToolBase::OnDestroy();

	if (vMap.isWorldLoaded ()) {
		//vMap.initGrid();
		vMap.recalcArea2Grid(0,0,vMap.H_SIZE-1,vMap.V_SIZE-1);
	}
}

void CSurToolToolzer::OnBnClickedCheckEnableHFilter()
{
	CButton* chB= (CButton*)GetDlgItem(IDC_CHECK_ENABLE_H_FILTER);
	flag_EnableFilterH=chB->GetCheck();
	m_FilterMaxH.EnableWindow(flag_EnableFilterH);
	m_FilterMinH.EnableWindow(flag_EnableFilterH);
}

void CSurToolToolzer::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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
