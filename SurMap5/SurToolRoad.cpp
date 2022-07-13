#include "stdafx.h"
#include "SurMap5.h"
#include ".\SurToolRoad.h"
#include "iVisGeneric.h"
#include "..\game\CameraManager.h"


// CSurToolRoad dialog
const int MIN_ROAD_WIDTH=2;
const int MAX_ROAD_WIDTH=500;
const int MIN_SPHERIC_H=5;
const int MAX_SPHERIC_H=250;
const int MIN_EDGE_ANGLE=0;
const int MAX_EDGE_ANGLE=60;
const int MIN_EDGE_WIDTH=0;
const int MAX_EDGE_WIDTH=300;
IMPLEMENT_DYNAMIC(CSurToolRoad, CSurToolBase)
CSurToolRoad::CSurToolRoad(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)
{
	curRoadOperation=CRO_None;
	m_roadWidth.value=30;
	m_roadWidth.SetRange(MIN_ROAD_WIDTH,MAX_ROAD_WIDTH);
	m_edgeAngle.value=10;
	m_edgeAngle.SetRange(MIN_EDGE_ANGLE, MAX_EDGE_ANGLE);
	m_sphericH.value=50;
	m_sphericH.SetRange(MIN_SPHERIC_H,MAX_SPHERIC_H);
	state_rd_roadMetod=SRM_TrackSurface;
	texturingMetod=sRoadPMO::TM_AlignWidthAndHeight;

	putMetod=sRoadPMO::PM_ByMaxHeight;
}

CSurToolRoad::~CSurToolRoad()
{
}

void CSurToolRoad::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSurToolRoad, CSurToolBase)
	ON_BN_CLICKED(IDC_BTN_PUTTRACK, OnBnClickedBtnPuttrack)
	ON_BN_CLICKED(IDC_BTN_CLEARROAD, OnBnClickedBtnClearroad)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE, OnBnClickedBtnBrowseFile)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILEV, OnBnClickedBtnBrowseFilev)
	ON_CONTROL_RANGE (BN_CLICKED, IDC_RD_ROADMETOD1, IDC_RD_ROADMETOD4, OnRoadMetodButtonClicked)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE_EDGEBITMAP_TEXTURE, OnBnClickedBtnBrowseFileEdgebitmapTexture)
	ON_BN_CLICKED(IDC_BTN_BROWSE_FILE_EDGEBITMAP_VTEXTURE, OnBnClickedBtnBrowseFileEdgebitmapVtexture)
	ON_CBN_SELCHANGE(IDC_CMB_TEXTURING_METHOD, OnCbnSelchangeCmbTexturingMethod)
	ON_CBN_SELCHANGE(IDC_CMB_PUT_METHOD, OnCbnSelchangeCmbPutMethod)
END_MESSAGE_MAP()


void CSurToolRoad::checkRadioButtonAndStateControlStatus()
{
	state_rd_roadMetod=static_cast<eStateRoadMetod>(GetCheckedRadioButton(IDC_RD_ROADMETOD1, IDC_RD_ROADMETOD4)-IDC_RD_ROADMETOD1);//
	m_sphericH.ShowControl(state_rd_roadMetod==SRM_BegEndSpheric); //spheric
}

BOOL CSurToolRoad::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	// TODO:  Add extra initialization here
	m_roadWidth.Create(this, IDC_SLDR_EDT_ROADWIDTH, IDC_EDT_ROADWIDTH);
	m_edgeAngle.Create(this, IDC_SLDR_EDT_EDGEANGLE, IDC_EDT_EDGEANGLE);
	m_sphericH.Create(this, IDC_SLDR_EDT_SPHERIC_H, IDC_EDT_SPHERIC_H);

	CheckRadioButton(IDC_RD_ROADMETOD1, IDC_RD_ROADMETOD4, IDC_RD_ROADMETOD1+state_rd_roadMetod);//
	checkRadioButtonAndStateControlStatus();

	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAP);
	ew->SetWindowText(bitmapFileName.c_str());
	ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAPV);
	ew->SetWindowText(bitmapVolFileName.c_str());

	CComboBox * ComBox = (CComboBox *) GetDlgItem(IDC_CMB_TEXTURING_METHOD);
	ComBox->AddString("Align width аnd height");
	ComBox->AddString("Align only width");
	ComBox->AddString("1 to 1");
	ComBox->SetCurSel(texturingMetod);

	ComBox = (CComboBox *) GetDlgItem(IDC_CMB_PUT_METHOD);
	ComBox->AddString("by max H");
	ComBox->AddString("by road H");


	ComBox->SetCurSel(putMetod);
	correctWidthAngleRange();

	flag_init_dialog=1;

	return FALSE;
}

void CSurToolRoad::OnCbnSelchangeCmbTexturingMethod()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		CComboBox* comBox;
		comBox=(CComboBox*)GetDlgItem(IDC_CMB_TEXTURING_METHOD);
		int texmethod=comBox->GetCurSel();
		xassert(texmethod>=sRoadPMO::TM_AlignWidthAndHeight && texmethod <=sRoadPMO::TM_1to1);
		texturingMetod=(sRoadPMO::eTexturingMetod)texmethod;
	}
}
void CSurToolRoad::correctWidthAngleRange()
{
	switch(putMetod){
	case sRoadPMO::PM_ByMaxHeight:
		m_edgeAngle.SetRange(MIN_EDGE_ANGLE, MAX_EDGE_ANGLE);
		break;
	case sRoadPMO::PM_ByRoadHeight:
		m_edgeAngle.SetRange(MIN_EDGE_WIDTH, MAX_EDGE_WIDTH);
		break;
	default:
		xassert(0&&"Unforeseen value");
	}
	Invalidate(FALSE);
}

void CSurToolRoad::OnCbnSelchangeCmbPutMethod()
{
	// TODO: Add your control notification handler code here
	if(flag_init_dialog) {
		CComboBox* comBox;
		comBox=(CComboBox*)GetDlgItem(IDC_CMB_PUT_METHOD);
		int puttingMethod=comBox->GetCurSel();
		xassert(puttingMethod>=sRoadPMO::PM_ByMaxHeight && puttingMethod <=sRoadPMO::PM_ByRoadHeight);
		putMetod=(sRoadPMO::ePutMetod)puttingMethod;
		correctWidthAngleRange();
	}
}

// CSurToolRoad message handlers
void CSurToolRoad::OnBnClickedBtnPuttrack()
{
	// TODO: Add your control notification handler code here
	buildRoad(state_rd_roadMetod==SRM_OnlyTexture);
}

void CSurToolRoad::OnBnClickedBtnClearroad()
{
	// TODO: Add your control notification handler code here
	nodeArr.clear();
	Invalidate(FALSE);
}

void CSurToolRoad::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	CSliderCtrl * slR;
	CSliderCtrl * slR2;
	//
	slR=(CSliderCtrl*)GetDlgItem(IDC_SLDR_EDT_ROADWIDTH);
	slR2=(CSliderCtrl*)GetDlgItem(IDC_SLDR_EDT_EDGEANGLE);
	if(pScrollBar==(CScrollBar*)slR || pScrollBar==(CScrollBar*)slR2){
		int withd=m_roadWidth.GetPos();
		int angle=m_edgeAngle.GetPos();
		changeAllRoadWidthAndAngle(withd, (float)angle*M_PI/180.f);
		Invalidate(FALSE);
	}
	slR=(CSliderCtrl*)GetDlgItem(IDC_SLDR_EDT_SPHERIC_H);
	if(pScrollBar == (CScrollBar*)slR){
		recalcHeightNodeParabolic(m_sphericH.value);
		Invalidate(FALSE);
	}


	CSurToolBase::OnHScroll(nSBCode, nPos, pScrollBar);

}

void CSurToolRoad::OnRoadMetodButtonClicked(UINT nID)
{
	checkRadioButtonAndStateControlStatus();
}


void CSurToolRoad::OnBnClickedBtnBrowseFile()
{
	// TODO: Add your control notification handler code here
	bitmapFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAP);
	ew->SetWindowText(bitmapFileName.c_str());
}

void CSurToolRoad::OnBnClickedBtnBrowseFilev()
{
	// TODO: Add your control notification handler code here
	bitmapVolFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAPV);
	ew->SetWindowText(bitmapVolFileName.c_str());
}
void CSurToolRoad::OnBnClickedBtnBrowseFileEdgebitmapTexture()
{
	// TODO: Add your control notification handler code here
	edgeBitmapFName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_TEXTURE);
	ew->SetWindowText(edgeBitmapFName.c_str());
}

void CSurToolRoad::OnBnClickedBtnBrowseFileEdgebitmapVtexture()
{
	// TODO: Add your control notification handler code here
	edgeVolBitmapFName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_VTEXTURE);
	ew->SetWindowText(edgeVolBitmapFName.c_str());
}


bool CSurToolRoad::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	const bool RESULT=true;
	if(curRoadOperation==CRO_None){
		return RESULT;
	}
	else if(curRoadOperation==CRO_MoveNode){
		curNode_->setPos(worldCoord);
		recalcNodeHeight();
	}
	return RESULT;
}
bool CSurToolRoad::CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	TypeNodeIterator result=selectNode(screenCoord);
	if(result!=0){
		curRoadOperation=CRO_MoveNode;
		curNode_=result;
	}
	else if(selectVerge(screenCoord)){
	}
	else {
		(addNode(worldCoord, screenCoord, m_roadWidth.value, (float)m_edgeAngle.value*M_PI/180.));
		recalcNodeHeight();
	}
	//else {
	//	curRoadOperation=CRO_None;
	//	curNode_=0;
	//}
	return true;
}
bool CSurToolRoad::CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	curRoadOperation=CRO_None;
	curNode_=0;
	return true;
}
bool CSurToolRoad::CallBack_RMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	return false;
}
bool CSurToolRoad::CallBack_KeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
	if (keyCode == VK_DELETE){
		if(curRoadOperation==CRO_MoveNode){
			xassert(curNode_!=0);
			nodeArr.erase(curNode_);
			recalcNodeHeight();
		}
	}
	return true;
}

CSurToolRoad::TypeNodeIterator CSurToolRoad::selectNode(const Vect2i& scrCoord)
{
	if(!cameraManager) return 0;

	const Vect2f scrCoordf(scrCoord);
	const float DISTANCE=8;
	const float DISTANCE2=DISTANCE*DISTANCE;
	float minDistance2=DISTANCE2;

	TypeNodeIterator p;
	TypeNodeIterator findingNode=0;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		Vect3f e,w;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&p->pos,&w,&e);
		Vect2f pntOnScr(e.x, e.y);
		float curDistance=pntOnScr.distance2(scrCoordf);
		if(minDistance2>curDistance){
			findingNode=p;
			minDistance2=curDistance;
		}
	}
	return findingNode;
}

bool CSurToolRoad::selectVerge(const Vect2i& scrCoord)
{
	return 0;
}
void CSurToolRoad::recalcNodeHeight()
{
	if(state_rd_roadMetod==SRM_BegEndLinear)
		recalcHeightNodeLinear();
	else if(state_rd_roadMetod==SRM_BegEndSpheric)
		recalcHeightNodeParabolic(m_sphericH.value);
}

void drawCross(const Vect3f& wrldCoord, const sColor4c& color)
{
	gb_RenderDevice->DrawLine(wrldCoord-Vect3f(-5,-5,0), wrldCoord-Vect3f(+5,+5,0), color);
	gb_RenderDevice->DrawLine(wrldCoord-Vect3f(+5,-5,0), wrldCoord-Vect3f(-5,+5,0), color);
}
bool CSurToolRoad::CallBack_DrawAuxData(void)
{
	TypeNodeIterator p;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcNodeOrientation(p);
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcSpline(p);
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		if(p==curNode_)
			drawCross(p->pos, sColor4c(255, 0, 0, 200));
		else 
			drawCross(p->pos, sColor4c(255, 255, 255, 200));

		//поперечная линия
		gb_RenderDevice->DrawLine( Vect3f(p->leftPnt.x, p->leftPnt.y, p->pos.z), Vect3f(p->rightPnt.x, p->rightPnt.y, p->pos.z), sColor4c(255, 255, 255, 200) );
		//Прямая соединяющая линия
		//if(p!=nodeArr.begin()){
		//	TypeNodeIterator m=p;
		//	m--;
		//	gb_RenderDevice->DrawLine( Vect3f(m->leftPnt.x, m->leftPnt.y, m->pos.z), Vect3f(p->leftPnt.x, p->leftPnt.y, p->pos.z), sColor4c(255, 255, 255, 200) );
		//	gb_RenderDevice->DrawLine( Vect3f(m->rightPnt.x, m->rightPnt.y, m->pos.z), Vect3f(p->rightPnt.x, p->rightPnt.y, p->pos.z), sColor4c(255, 255, 255, 200) );
		//}
		//сплайн
		int splineSize=p->spline2NextLeft.size();
		xassert(splineSize==p->spline2NextRight.size() && splineSize==p->spline2NextLeftEdge.size() &&
			splineSize==p->spline2NextRightEdge.size());
		int i;
		for(i=1; i<splineSize; i++){
			Vect3f& lPP=p->spline2NextLeft[i-1];
			Vect3f& rPP=p->spline2NextRight[i-1];
			Vect3f& lNP=p->spline2NextLeft[i];
			Vect3f& rNP=p->spline2NextRight[i];
			gb_RenderDevice->DrawLine( lPP, lNP, sColor4c(255, 255, 255, 200) );
			gb_RenderDevice->DrawLine( rPP, rNP, sColor4c(255, 255, 255, 200) );
			gb_RenderDevice->DrawLine( lPP, rPP, sColor4c(255, 0, 0, 200) );
			gb_RenderDevice->DrawLine( p->spline2Next[i-1], p->spline2Next[i], sColor4c(0, 255, 0, 200));
			Vect3f& lPPE=p->spline2NextLeftEdge[i-1];//(p->spline2NextLeftEdge[i-1].x, p->spline2NextLeftEdge[i-1].y, 0.f);
			Vect3f& rPPE=p->spline2NextRightEdge[i-1];//(p->spline2NextRightEdge[i-1].x, p->spline2NextRightEdge[i-1].y, 0.f);
			Vect3f& lNPE=p->spline2NextLeftEdge[i];//(p->spline2NextLeftEdge[i].x, p->spline2NextLeftEdge[i].y, 0.f);
			Vect3f& rNPE=p->spline2NextRightEdge[i];//(p->spline2NextRightEdge[i].x, p->spline2NextRightEdge[i].y, 0.f);
			gb_RenderDevice->DrawLine( lPPE, lNPE, sColor4c(055, 055, 255, 200) );
			gb_RenderDevice->DrawLine( rPPE, rNPE, sColor4c(055, 055, 255, 200) );
		}
	}
	return true;
}



