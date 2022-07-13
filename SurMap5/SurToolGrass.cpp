#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolGrass.h"

#include "Environment\Environment.h"
#include "Game\RenderObjects.h"
#include "Serialization\Serialization.h"
#include "Render\src\Grass.h"


int CSurToolGrass::nextGrass = 0;
IMPLEMENT_DYNAMIC(CSurToolGrass, CSurToolBase)

BEGIN_MESSAGE_MAP(CSurToolGrass, CSurToolBase)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SELECT_TEXTURE, OnBnClickedSelectTexture)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
	ON_CBN_SELCHANGE(IDC_GRASS_LIST, OnCbnSelchangeGrassList)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

CSurToolGrass::CSurToolGrass(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(),pParent)
{
	grassNumber = nextGrass;
	nextGrass++;
	for (int i=0; i<7;i++)
	{
		intensity1[i] = 0;
		intensity2[i] = 0;
		density[i] = 1;
	}
	oldSelectedGrass = 0;

}

CSurToolGrass::~CSurToolGrass()
{
	nextGrass--;
}

BOOL CSurToolGrass::OnInitDialog()
{
	if(environment){
	CSurToolBase::OnInitDialog();
	CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
	cmb->AddString("Grass1");
	cmb->AddString("Grass2");
	cmb->AddString("Grass3");
	cmb->AddString("Grass4");
	cmb->AddString("Grass5");
	cmb->AddString("Grass6");
	cmb->AddString("Grass7");
	cmb->SetCurSel(oldSelectedGrass);
	OnCbnSelchangeGrassList();
	CSliderCtrl* sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DENSITY);
	sldr->SetRange(1,16);
	sldr->SetPos(density[oldSelectedGrass]);
	sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY);
	sldr->SetRange(0,100);
	sldr->SetPos(intensity1[oldSelectedGrass]);
	sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY2);
	sldr->SetRange(0,100);
	sldr->SetPos(intensity2[oldSelectedGrass]);
	CString str;
	str.Format("%d",density[oldSelectedGrass]);
	GetDlgItem(IDC_STATIC_DENSITY)->SetWindowText(str);
	str.Format("%d",intensity1[oldSelectedGrass]);
	GetDlgItem(IDC_TOPGRASS)->SetWindowText(str);
	str.Format("%d",intensity2[oldSelectedGrass]);
	GetDlgItem(IDC_BOTTOM)->SetWindowText(str);
	environment->grass()->EnableShadow(false);
	}
	return FALSE;
}
void CSurToolGrass::OnBnClickedApply()
{
	int hr = MessageBox("¬ы действительно хотите применить заданную интенсивность ко всей выбранной траве?","ѕредупреждение!!!",MB_ICONQUESTION|MB_YESNO);
	if (hr == IDYES)
	{
		CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
		environment->grass()->SetAllGrass(cmb->GetCurSel(),intensity1[oldSelectedGrass],intensity2[oldSelectedGrass]);
	}
}

void CSurToolGrass::OnDestroy()
{
	if (environment&&environment->grass())
		environment->grass()->EnableShadow(true);
	CSurToolBase::OnDestroy();
}

bool CSurToolGrass::onDrawAuxData()
{
	drawCursorCircle();
	return true;
}
bool CSurToolGrass::onOperationOnMap(int x, int y)
{
	if(environment && environment->grass()) {
		CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
		int radius = getBrushRadius();
		bool erase = ((CButton*)GetDlgItem(IDC_ERASE))->GetCheck();
		CSliderCtrl* sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DENSITY);
		CSliderCtrl* sldrI = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY);
		CSliderCtrl* sldrI2 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY2);
		environment->grass()->SetGrass(x,y,radius,cmb->GetCurSel(),sldr->GetPos(),erase,sldrI->GetPos(),sldrI2->GetPos());
	}
	return true;
}
void CSurToolGrass::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


void CSurToolGrass::OnBnClickedSelectTexture()
{
	if(environment && environment->grass()) {
		string fname = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Textures", 
			"*.tga", "bitmap.tga", "Will select location of an file textures");

		if (fname.empty())
			return;

		CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
		environment->grass()->SetTexture(fname.c_str(),cmb->GetCurSel());
		GetDlgItem(IDC_TEXTURE_NAME)->SetWindowText(fname.c_str());
	}

}

void CSurToolGrass::OnCbnSelchangeGrassList()
{
	CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
	GetDlgItem(IDC_TEXTURE_NAME)->SetWindowText(environment->grass()->GetTextureName(cmb->GetCurSel()));
	oldSelectedGrass = cmb->GetCurSel();
	CSliderCtrl* sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DENSITY);
	CSliderCtrl* sldrI = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY);
	CSliderCtrl* sldrI2 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY2);
	sldr->SetPos(density[oldSelectedGrass]);
	sldrI->SetPos(intensity1[oldSelectedGrass]);
	sldrI2->SetPos(intensity2[oldSelectedGrass]);
	CString str;
	str.Format("%d",density[oldSelectedGrass]);
	GetDlgItem(IDC_STATIC_DENSITY)->SetWindowText(str);
	str.Format("%d",intensity1[oldSelectedGrass]);
	GetDlgItem(IDC_TOPGRASS)->SetWindowText(str);
	str.Format("%d",intensity2[oldSelectedGrass]);
	GetDlgItem(IDC_BOTTOM)->SetWindowText(str);
}
bool CSurToolGrass::onDrawPreview(int width, int height)
{
	if(environment && environment->grass() && environment->grass()->GetTexture()) {
		CComboBox* cmb = (CComboBox*)GetDlgItem(IDC_GRASS_LIST);
		const sRectangle4f& rt = ((cTextureAviScale*)environment->grass()->GetTexture())->GetFramePos(cmb->GetCurSel());
		
		drawPreviewTexture(environment->grass()->GetTexture(), width, height, rt);

		/*
		float texWidth = rt.max.x-rt.min.x;
		float texHeight = rt.max.y-rt.min.y;
		int w = width;
		int h = height;
		float f= texWidth/texHeight;
		if (width>height)
		{
			w = height*f;
		}else
		{
			h = width*f;
		}

		gb_RenderDevice->DrawSprite (w, h *0.1f,
			w*0.8f, h*0.8f, rt.min.x, rt.min.y, rt.max.x-rt.min.x, rt.max.y-rt.min.y,environment->grass()->GetTexture());
			*/
	}
	return true;
}

void CSurToolGrass::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* sldr = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DENSITY);
	CSliderCtrl* sldr1 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY);
	CSliderCtrl* sldr2 = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_INTENSITY2);
	if (pScrollBar->GetDlgCtrlID() == sldr->GetDlgCtrlID())
	{
		density[oldSelectedGrass] = sldr->GetPos();
	}else
	if (pScrollBar->GetDlgCtrlID() == sldr1->GetDlgCtrlID())
	{
		intensity1[oldSelectedGrass] = sldr1->GetPos();
	}else
	if (pScrollBar->GetDlgCtrlID() == sldr2->GetDlgCtrlID())
	{
		intensity2[oldSelectedGrass] = sldr2->GetPos();
	}
	CString str;
	str.Format("%d",sldr->GetPos());
	GetDlgItem(IDC_STATIC_DENSITY)->SetWindowText(str);
	str.Format("%d",intensity1[oldSelectedGrass]);
	GetDlgItem(IDC_TOPGRASS)->SetWindowText(str);
	str.Format("%d",intensity2[oldSelectedGrass]);
	GetDlgItem(IDC_BOTTOM)->SetWindowText(str);

	CSurToolBase::OnHScroll(nSBCode, nPos, pScrollBar);
}
