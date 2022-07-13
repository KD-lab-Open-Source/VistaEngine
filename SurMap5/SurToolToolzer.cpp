#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolToolzer.h"
#include "SurToolAux.h"

// CSurToolToolzer dialog
const int MIN_TOOLZER_DELTA_H=0;
const int MAX_TOOLZER_DELTA_H=(256<<VX_FRACTION)-1;
const int MIN_TOOLZER_LEVELING_H=0;
const int MAX_TOOLZER_LEVELING_H=256;

IMPLEMENT_DYNAMIC(CSurToolToolzer, CSurToolBase)
CSurToolToolzer::CSurToolToolzer(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	m_DeltaH.value=10<<VX_FRACTION;
	m_RadiusToolzer.value=20;
	m_Roughness.value=90;
	state_radio_button_DigPut=0;
	state_level_ZL_or_level_h=0;
	state_free_or_level_h=0;

	idxCurToolzerType=0;

	m_LevelingH.SetRange(MIN_TOOLZER_LEVELING_H,MAX_TOOLZER_LEVELING_H);
	m_DeltaH.SetRange(MIN_TOOLZER_DELTA_H, MAX_TOOLZER_DELTA_H);
	m_DeltaH.value=0;

}

CSurToolToolzer::~CSurToolToolzer()
{
}

void CSurToolToolzer::serialize(Archive& ar) 
{
	__super::serialize(ar);
	ar.serialize(m_LevelingH.value, "m_LevelingH", 0);
	ar.serialize(m_DeltaH.value, "m_DeltaH", 0);
	ar.serialize(state_radio_button_DigPut, "state_radio_button_DigPut", 0);
	if( ar.isInput() && (!ar.serialize(state_free_or_level_h, "state_free_or_level_h", 0)) ){
        ar.serialize(state_level_ZL_or_level_h, "state_level_ZL_or_level_h", 0);
		state_free_or_level_h=state_level_ZL_or_level_h;
	}
	if(ar.isOutput())
		ar.serialize(state_free_or_level_h, "state_free_or_level_h", 0);
	ar.serialize(idxCurToolzerType, "idxCurToolzerType", 0);
	ar.serialize(m_Roughness.value, "m_Roughness", 0);
}

void CSurToolToolzer::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolToolzer, CSurToolBase)
//	ON_WM_CREATE()
ON_BN_CLICKED(IDC_RADIO_DIGPUT1, OnBnClickedRadioDigput1)
ON_BN_CLICKED(IDC_RADIO_DIGPUT2, OnBnClickedRadioDigput2)
ON_BN_CLICKED(IDC_RDB_LEVELING_1, OnBnClickedRdbLeveling1)
ON_BN_CLICKED(IDC_RDB_LEVELING_2, OnBnClickedRdbLeveling2)
ON_CBN_SELCHANGE(IDC_CBOXEX_TOOLZERTYPE, OnCbnSelchangeCboxexToolzertype)
ON_WM_DESTROY()
END_MESSAGE_MAP()


// CSurToolToolzer message handlers

BOOL CSurToolToolzer::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_RadiusToolzer.Create(this, IDC_SLIDER_EDITRADIUS, IDC_EDITRADIUS);
	m_DeltaH.Create(this, IDC_SLIDER_EDITDELTAH, IDC_EDITDELTAH);

	m_LevelingH.Create(this, IDC_SLDR_EDT_LEVELING_H, IDC_EDT_LEVELING_H);
	m_Roughness.Create(this, IDC_SLDR_ROUGHNESS, IDC_EDT_ROUGHNESS);


	CheckRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2, IDC_RADIO_DIGPUT1+state_radio_button_DigPut);//
	state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;//

	CheckRadioButton(IDC_RDB_LEVELING_1, IDC_RDB_LEVELING_2, IDC_RDB_LEVELING_1+state_free_or_level_h);//
	state_free_or_level_h=GetCheckedRadioButton(IDC_RDB_LEVELING_1, IDC_RDB_LEVELING_2)-IDC_RDB_LEVELING_1;//
	if(state_free_or_level_h==0) m_LevelingH.ShowControl(false);
	else m_LevelingH.ShowControl(true);

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


	flag_init_dialog=1;
	return FALSE;
}




BOOL CSurToolToolzer::Create(UINT nIDTemplate, CWnd* pParentWnd)
{
	// TODO: Add your specialized code here and/or call the base class

	return CSurToolBase::Create(nIDTemplate, pParentWnd);
}

bool CSurToolToolzer::CallBack_OperationOnMap(int x, int y)
{
	int dh=m_DeltaH.value;
	if(state_radio_button_DigPut==0) dh=-dh;
	else dh=dh;
	
	if(vMap.isWorldLoaded()) {
		int rad = getBrushRadius();
		e_BrushForm bf = BRUSHFORM_CIRCLE;
		m_RadiusToolzer.value=rad;

		short levelingH;
		if(state_free_or_level_h==0){ 
			if(dh>0) levelingH=MAX_VX_HEIGHT;
			else if(dh<0) levelingH=0;
			else levelingH=0; //В этом случае все равно какое значение
		}
		else
			levelingH=m_LevelingH.value<<VX_FRACTION;
		if(levelingH>MAX_VX_HEIGHT) //для того, чтобы при 256 было 255.31
			levelingH=MAX_VX_HEIGHT;

		float roughness=(float)m_Roughness.value/100.f;
		switch(bf){
		case BRUSHFORM_CIRCLE:
			switch(idxCurToolzerType){
			case 0:
				vMap.deltaZone(sToolzerPMO(x, y, m_RadiusToolzer.value, 1, dh, 0, 0, levelingH));
				break;
			case 1:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodExp(2,dh, 0), levelingH);
				break;
			case 2:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodPN(2,dh, 0), levelingH);
				break;
			case 3:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodMPD(2,dh, 0, roughness), levelingH);
				break;
			}
            break;
		case BRUSHFORM_SQUARE:
			//vMap.squareDeltaZone(x, y, m_RadiusToolzer.value, 10, dh, 0, 0);
			switch(idxCurToolzerType){
			case 0:
				vMap.squareDeltaZone(sSquareToolzerPMO(x, y, m_RadiusToolzer.value, 1, dh, 0, 0, levelingH));
				break;
			case 1:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodExp(3,dh, 0), levelingH);
				break;
			case 2:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodPN(3,dh, 0), levelingH);
				break;
			case 3:
				vMap.drawBitMetod(x, y, rad, sBitGenMetodMPD(3,dh, 0, roughness), levelingH);
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

bool CSurToolToolzer::CallBack_DrawAuxData(void)
{
	drawCursorCircle();
	return true;
}

void CSurToolToolzer::OnCbnSelchangeCboxexToolzertype()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		CComboBox* comBox;
		comBox=(CComboBox*)GetDlgItem(IDC_CBOXEX_TOOLZERTYPE);
		idxCurToolzerType=comBox->GetCurSel();
	}
}


void CSurToolToolzer::OnBnClickedRadioDigput1()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;
}

void CSurToolToolzer::OnBnClickedRadioDigput2()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) state_radio_button_DigPut=GetCheckedRadioButton(IDC_RADIO_DIGPUT1, IDC_RADIO_DIGPUT2)-IDC_RADIO_DIGPUT1;
}

void CSurToolToolzer::OnBnClickedRdbLeveling1()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		state_free_or_level_h=GetCheckedRadioButton(IDC_RDB_LEVELING_1, IDC_RDB_LEVELING_2)-IDC_RDB_LEVELING_1;//
		if(state_free_or_level_h==0) m_LevelingH.ShowControl(false);
		else m_LevelingH.ShowControl(true);
	}
}

void CSurToolToolzer::OnBnClickedRdbLeveling2()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		state_free_or_level_h=GetCheckedRadioButton(IDC_RDB_LEVELING_1, IDC_RDB_LEVELING_2)-IDC_RDB_LEVELING_1;//
		if(state_free_or_level_h==0) m_LevelingH.ShowControl(false);
		else m_LevelingH.ShowControl(true);
	}
}



void CSurToolToolzer::OnDestroy()
{
	CSurToolBase::OnDestroy();

	if (vMap.isWorldLoaded ()) {
		//vMap.initGrid();
		vMap.recalcArea2Grid(0,0,vMap.H_SIZE-1,vMap.V_SIZE-1);
	}
}

