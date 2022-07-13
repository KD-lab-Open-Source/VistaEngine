#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolRoad.h"
#include "game\CameraManager.h"
#include "Render\Src\cCamera.h"

#include "Serialization\Serialization.h"
#include "Serialization\EnumDescriptor.h"

//#include "Game\RenderObjects.h"
#include "Render\src\MultiRegion.h"
#include "Render\src\TileMap.h"
//#include "Render\src\Scene.h"
#include "FileUtils\FileUtils.h"


BEGIN_ENUM_DESCRIPTOR_ENCLOSED(CSurToolRoad, eStateRoadMetod, "StateRoadMetod")
REGISTER_ENUM_ENCLOSED(CSurToolRoad, SRM_BegEndLinear, 0)
REGISTER_ENUM_ENCLOSED(CSurToolRoad, SRM_BegEndSpheric, 0)
REGISTER_ENUM_ENCLOSED(CSurToolRoad, SRM_TrackSurface, 0)
REGISTER_ENUM_ENCLOSED(CSurToolRoad, SRM_OnlyTexture, 0)
END_ENUM_DESCRIPTOR_ENCLOSED(CSurToolRoad, eStateRoadMetod)

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
	m_AlphaRoad.value=255;
	m_AlphaRoad.SetRange(0,255);
	m_AlphaRoadSide.value=255;
	m_AlphaRoadSide.SetRange(0,255);
	state_rd_roadMetod=SRM_TrackSurface;
	texturingMetod=sRoadPMO::TM_AlignWidthAndHeight;

	putMetod=sRoadPMO::PM_ByMaxHeight;
}

CSurToolRoad::~CSurToolRoad()
{
}

void CSurToolRoad::serialize(Archive& ar) 
{
	CSurToolBase::serialize(ar);
	RoadTool::serialize(ar);
	ar.serialize(m_roadWidth.value, "roadWidth", 0);
	ar.serialize(m_edgeAngle.value, "m_edgeAngle", 0);
	ar.serialize(m_sphericH.value, "m_sphericH", 0);
	ar.serialize(state_rd_roadMetod, "state_rd_roadMetod", 0);
	ar.serialize(m_AlphaRoad.value, "m_AlphaRoad", 0);
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

	m_roadWidth.Create(this, IDC_SLDR_EDT_ROADWIDTH, IDC_EDT_ROADWIDTH);
	m_edgeAngle.Create(this, IDC_SLDR_EDT_EDGEANGLE, IDC_EDT_EDGEANGLE);
	m_sphericH.Create(this, IDC_SLDR_EDT_SPHERIC_H, IDC_EDT_SPHERIC_H);
	m_AlphaRoad.Create(this, IDCSL_ALPHAROAD, IDCSLE_ALPHAROAD);
	m_AlphaRoadSide.Create(this, IDCSL_ALPHAROADSIDE, IDCSLE_ALPHAROADSIDE);

	CheckRadioButton(IDC_RD_ROADMETOD1, IDC_RD_ROADMETOD4, IDC_RD_ROADMETOD1+state_rd_roadMetod);//
	checkRadioButtonAndStateControlStatus();

	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAP);
	ew->SetWindowText(textureSetting.bitmapFileName.c_str());
	ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAPV);
	ew->SetWindowText(textureSetting.bitmapVolFileName.c_str());
	ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_TEXTURE);
	ew->SetWindowText(textureSetting.edgeBitmapFName.c_str());
	ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_VTEXTURE);
	ew->SetWindowText(textureSetting.edgeVolBitmapFName.c_str());

	CComboBox * ComBox = (CComboBox *) GetDlgItem(IDC_CMB_TEXTURING_METHOD);
	ComBox->AddString("Масштб текстуры по ширине и длинне"); //Align width аnd height
	ComBox->AddString("Масштб текстуры по ширине"); //Align only width
	ComBox->AddString("Масштб 1:1");
	ComBox->SetCurSel(texturingMetod);

	ComBox = (CComboBox *) GetDlgItem(IDC_CMB_PUT_METHOD);
	ComBox->AddString("НЕ выгрызать"); //by max H
	ComBox->AddString("Выгрызать"); //by road H
	ComBox->SetCurSel(putMetod);

	if(tileMap){
		ComBox = (CComboBox *) GetDlgItem(IDCCB_ROAD_MINIDETAIL);
		ComBox->AddString("NoChange");
		for(int i = 0; i < cTileMap::miniDetailTexturesNumber; i++)
			ComBox->AddString(extractFileName(tileMap->miniDetailTexture(i).textureName.c_str()).c_str());
		ComBox->SetCurSel(roadDetailTex);
		ComBox = (CComboBox *) GetDlgItem(IDCCB_EDGE_MINIDETAIL);
		ComBox->AddString("NoChange");
		for(int i = 0; i < cTileMap::miniDetailTexturesNumber; i++)
			ComBox->AddString(extractFileName(tileMap->miniDetailTexture(i).textureName.c_str()).c_str());
		ComBox->SetCurSel(edgeDetailTex);
	}

	correctWidthAngleRange();

	flag_init_dialog=1;

	return FALSE;
}

void CSurToolRoad::OnCbnSelchangeCmbTexturingMethod()
{
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
	textureSetting.flag_onlyTextured=state_rd_roadMetod==SRM_OnlyTexture;
	textureSetting.alphaRoad=m_AlphaRoad.GetPos();
	textureSetting.alphaRoadSide=m_AlphaRoadSide.GetPos();
	CComboBox * ComBox;
	ComBox = (CComboBox *) GetDlgItem(IDCCB_ROAD_MINIDETAIL);
	roadDetailTex = ComBox->GetCurSel();
	ComBox = (CComboBox *) GetDlgItem(IDCCB_EDGE_MINIDETAIL);
	edgeDetailTex= ComBox->GetCurSel();
	buildRoad();
}

void CSurToolRoad::OnBnClickedBtnClearroad()
{
	nodeArr.clear();
	Invalidate(FALSE);
}

void CSurToolRoad::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
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
	textureSetting.bitmapFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAP);
	ew->SetWindowText(textureSetting.bitmapFileName.c_str());
}

void CSurToolRoad::OnBnClickedBtnBrowseFilev()
{
	textureSetting.bitmapVolFileName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_BITMAPV);
	ew->SetWindowText(textureSetting.bitmapVolFileName.c_str());
}
void CSurToolRoad::OnBnClickedBtnBrowseFileEdgebitmapTexture()
{
	textureSetting.edgeBitmapFName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_TEXTURE);
	ew->SetWindowText(textureSetting.edgeBitmapFName.c_str());
}

void CSurToolRoad::OnBnClickedBtnBrowseFileEdgebitmapVtexture()
{
	textureSetting.edgeVolBitmapFName = requestResourceAndPut2InternalResource("Resource\\TerrainData\\Pictures", 
		"*.tga", "bitmap.tga", "Will select location of an file textures");
	CEdit* ew=(CEdit*)GetDlgItem(IDC_EDT_EDGEBITMAP_VTEXTURE);
	ew->SetWindowText(textureSetting.edgeVolBitmapFName.c_str());
}


bool CSurToolRoad::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
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
bool CSurToolRoad::onLMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	TypeNodeIterator result=selectNode(screenCoord);
	if(result!=nodeArr.end()){
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
bool CSurToolRoad::onLMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	curRoadOperation=CRO_None;
	curNode_=nodeArr.end();
	return true;
}
bool CSurToolRoad::onRMBDown(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	return false;
}
bool CSurToolRoad::onKeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
	if (keyCode == VK_DELETE){
		if(curRoadOperation==CRO_MoveNode){
			xassert(curNode_!=nodeArr.end());
			nodeArr.erase(curNode_);
			recalcNodeHeight();
		}
	}
	return true;
}

CSurToolRoad::TypeNodeIterator CSurToolRoad::selectNode(const Vect2i& scrCoord)
{
	if(!cameraManager) return nodeArr.end();

	const Vect2f scrCoordf(scrCoord);
	const float DISTANCE=8;
	const float DISTANCE2=DISTANCE*DISTANCE;
	float minDistance2=DISTANCE2;

	TypeNodeIterator p;
	TypeNodeIterator findingNode=nodeArr.end();
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

void drawCross(const Vect3f& wrldCoord, const Color4c& color)
{
	gb_RenderDevice->DrawLine(wrldCoord-Vect3f(-5,-5,0), wrldCoord-Vect3f(+5,+5,0), color);
	gb_RenderDevice->DrawLine(wrldCoord-Vect3f(+5,-5,0), wrldCoord-Vect3f(-5,+5,0), color);
}
bool CSurToolRoad::onDrawAuxData(void)
{
	TypeNodeIterator p;
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcNodeOrientation(p);
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++)
		recalcSpline(p);
	for(p=nodeArr.begin(); p!=nodeArr.end(); p++){
		if(p==curNode_)
			drawCross(p->pos, Color4c(255, 0, 0, 200));
		else 
			drawCross(p->pos, Color4c(255, 255, 255, 200));

		//поперечная линия
		gb_RenderDevice->DrawLine( Vect3f(p->leftPnt.x, p->leftPnt.y, p->pos.z), Vect3f(p->rightPnt.x, p->rightPnt.y, p->pos.z), Color4c(255, 255, 255, 200) );
		//Прямая соединяющая линия
		//if(p!=nodeArr.begin()){
		//	TypeNodeIterator m=p;
		//	m--;
		//	gb_RenderDevice->DrawLine( Vect3f(m->leftPnt.x, m->leftPnt.y, m->pos.z), Vect3f(p->leftPnt.x, p->leftPnt.y, p->pos.z), Color4c(255, 255, 255, 200) );
		//	gb_RenderDevice->DrawLine( Vect3f(m->rightPnt.x, m->rightPnt.y, m->pos.z), Vect3f(p->rightPnt.x, p->rightPnt.y, p->pos.z), Color4c(255, 255, 255, 200) );
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
			gb_RenderDevice->DrawLine( lPP, lNP, Color4c(255, 255, 255, 200) );
			gb_RenderDevice->DrawLine( rPP, rNP, Color4c(255, 255, 255, 200) );
			gb_RenderDevice->DrawLine( lPP, rPP, Color4c(255, 0, 0, 200) );
			gb_RenderDevice->DrawLine( p->spline2Next[i-1], p->spline2Next[i], Color4c(0, 255, 0, 200));
			Vect3f& lPPE=p->spline2NextLeftEdge[i-1];//(p->spline2NextLeftEdge[i-1].x, p->spline2NextLeftEdge[i-1].y, 0.f);
			Vect3f& rPPE=p->spline2NextRightEdge[i-1];//(p->spline2NextRightEdge[i-1].x, p->spline2NextRightEdge[i-1].y, 0.f);
			Vect3f& lNPE=p->spline2NextLeftEdge[i];//(p->spline2NextLeftEdge[i].x, p->spline2NextLeftEdge[i].y, 0.f);
			Vect3f& rNPE=p->spline2NextRightEdge[i];//(p->spline2NextRightEdge[i].x, p->spline2NextRightEdge[i].y, 0.f);
			gb_RenderDevice->DrawLine( lPPE, lNPE, Color4c(055, 055, 255, 200) );
			gb_RenderDevice->DrawLine( rPPE, rNPE, Color4c(055, 055, 255, 200) );
		}
	}
	return true;
}



