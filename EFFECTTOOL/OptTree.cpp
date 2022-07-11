// OptTree.cpp : implementation file
//
#include "stdafx.h"
#include "EffectTool.h"
#include "OptTree.h"
#include ".\opttree.h"
#include "MainFrm.h"
#include "DlgLoadSprite.h"
#include "NoiseProperties.h"


// COptTree
const float fl_max = float(1e35);
extern CControlView* ctrv;
extern CMainFrame* mf;
IMPLEMENT_DYNCREATE(COptTree, CFormView)

CString ToStr(float p)
{
	CString s;
	s.Format("%f",p);
	char *cs = strstr(s,".");
	if (cs)
		cs[4] = 0;

	return s;
}
CString ToStr(int p)
{
	CString s;
	s.Format("%d",p);
	return s;
}

inline bool SpiralDef()
{
	return true;
}

enum E_LOCATION
{
	MANUALLY	= 0,
	SPIRAL		= 1,
};
Vect3f GetKeyPosCenter(CKeyPos& kp)
{
	int size = kp.size();
	if (!size) return Vect3f::ZERO;
	Vect3f cnt(0,0,0);
	CKeyPos::iterator it;
	FOR_EACH(kp,it)
		cnt+= it->pos;
	return cnt/=size;
}
float COptTree::SetParamSpiral(CKeyPos& kp)
{
	int size = kp.size();
	if (size<2) return 0 ;
	Vect3f cnt = GetKeyPosCenter(kp);
	float ang = acos(Vect3f(kp[0].pos.x,kp[0].pos.y,0).Normalize().dot(Vect3f(kp[1].pos.x,kp[1].pos.y,0).Normalize()));
	Write(IX_SpAngle,R2G(ang));
	float alpha = acos(Vect3f(1,0,0).dot(Vect3f(kp[0].pos.x,kp[0].pos.y,0).Normalize()));
	Write(IX_SpHeight1,kp[0].pos.z);
	Write(IX_SpHeight2,kp[size-1].pos.z);
	float r = sqrt(pow(kp[0].pos.x-cnt.x,2)+pow(kp[0].pos.y-cnt.y,2));
	Write(IX_SpRadius1,r);
	r = sqrt(pow(kp[size-1].pos.x-cnt.x,2)+pow(kp[size-1].pos.y-cnt.y,2));
	Write(IX_SpRadius2,r);
	Write(IX_SpCompress,1);
	return alpha;
}
void COptTree::Spiral(bool set_par /*= false*/)
{

	if (intV(IX_PointLoc)!=1 || !GetDocument()->ActiveEmitter())
		return;
	CKeyPos* kt = NULL;
	SpiralData::Dat* dat = NULL;
	CEmitterData *edat = GetDocument()->ActiveEmitter();
	if(!edat->IsSpl())
		return;
	if (theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE) 
	{
		kt = &edat->p_position();
		dat = &edat->spiral_data().GetData(1);
	}
	else if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
	{
		kt = &edat->emitter_position();
		dat = &edat->spiral_data().GetData(0);
	}
	else return;
	ASSERT(dat);
	CKeyPos& kp = *kt;
	int size = kp.size();
	if (size==0) return;

	float alpha = 0;
	float r = dat->r1;//floatV(IX_SpRadius1);
	float h = dat->h1;//floatV(IX_SpHeight1);
	float dalpha = dat->dalpha;//G2R(floatV(IX_SpAngle));
	float dr = (dat->r2 - dat->r1)/size;//(floatV(IX_SpRadius2)-floatV(IX_SpRadius1))/size;
//	float dh = (floatV(IX_SpHeight2)-floatV(IX_SpHeight1))/size;
	float k = 1/dat->compress;//1/floatV(IX_SpCompress);
	float dh_max = dat->h2 - dat->h1;//floatV(IX_SpHeight2)-floatV(IX_SpHeight1);
	float k_norm = pow(k,size-1)-1;
	for(int i=0;i<size;++i)
	{
		kp[i].pos.x = r*cos(alpha);
		kp[i].pos.y = -r*sin(alpha);
		float tt;
		if (k!=1) tt = (pow(k,i)-1)/k_norm;
		else tt = float(i)/(size-1);
		kp[i].pos.z = h + tt*dh_max;
		alpha+=dalpha;
		r+=dr;
	}
	for(int i=0;i<size;++i)
	{
//		kp[i].pos+=first;
		kp[i].pos=dat->mat*kp[i].pos;
	}

}

void CDataItem::SetComboStr(CString combo_text)
{
	combo_stings.clear();
	int end;
	int begin=0;
	while((end = combo_text.Find(',' , begin))!=-1)
	{
		combo_stings.push_back();
		combo_stings.back().Visible = true;
		combo_stings.back().str = combo_text.Mid(begin,end-begin);
		begin = end+1;
	}
	if (round(value)>combo_stings.size())
		value = 0;
}

void CDataItem::Init(DWORD inx, DWORD style_, float min, float max,bool isint, CString combo_text)
{
	style = style_;
	index = inx;
	is_int = isint;
	min_val = min;
	max_val = max;
	if (style & STI_COMBO)
		SetComboStr(combo_text);
}

inline DWORD CDataItem::GetStyle()
{
	return style;
}
float CDataItem::GetValue()
{
	return value;
}
inline float CDataItem::GetRealyValue()
{
	return value;
}
void CDataItem::SetValue(float nv)
{
	if (nv<min_val) 	nv = min_val;
	else if (nv>max_val)nv = max_val;
	value = nv;
}
inline void CDataItem::SetRealyValue(float nv)
{
	value = nv;
}
inline bool CDataItem::GetCheck()
{
	return check;
}
inline void CDataItem::SetCheck(bool nch)
{
	check = nch;
}

float CDataItem::Delta()
{
	if (fabs(value/100)<0.1f)
		return 0.1f;
	return fabs(value/100);
}

CString CDataItem::GetStrValue()
{
	if (GetStyle()&STI_EDIT)
		if (is_int) return ::ToStr(round(GetValue()));
		else return ::ToStr(GetValue());
	if (GetStyle()&STI_COMBO)
		return GetComboStr(round(GetValue()));
	return "";
}
void CDataItem::SetStrValue(CString& nv)
{
	if (GetStyle()&STI_EDIT)
		if (is_int) SetValue(round(atof(nv)));
		else SetValue(atof(nv));
	if (GetStyle()&STI_COMBO)
	{
		int i = combo_stings.size()-1;
		for(;i>=0;--i)
			if (combo_stings[i].str==nv)
				break;
		if ((UINT)i>=combo_stings.size())
			return;
		value = i;
	}
}

inline int CDataItem::GetComboStrCount()
{
	return combo_stings.size();
}
bool CDataItem::GetComboStrVisible(int ix)
{
	if ((UINT)ix>=combo_stings.size()){ASSERT(false);return 0;}
	return combo_stings[ix].Visible;
}
void CDataItem::SetComboStrVisible(int i, bool nb)
{
	if ((UINT)i>=combo_stings.size()){ASSERT(false);return;}
	combo_stings[i].Visible = nb;
}
const CString& CDataItem::GetComboStr(int ix)
{
	static const CString sss;
	if ((UINT)ix>=combo_stings.size()){ASSERT(false);return sss;}
	return combo_stings[ix].str;
}




COptTree::COptTree()
	: CFormView(COptTree::IDD),ined(0)
{
	B[0] = true;
	B[1] = false;
	theApp.scene.bShowWorld = 1;
	theApp.scene.bShowGrid = 1;
	curent_editing = -1;
	scaleEffect = true;
}

void COptTree::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CUSTOM1, treeList);
}

BEGIN_MESSAGE_MAP(COptTree, CFormView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_BUT_ONE, UpDateToolBar)
	ON_COMMAND(ID_BUT_TWO, UpDateToolBar)
ON_WM_TIMER()
ON_WM_LBUTTONDOWN()
//ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


// COptTree diagnostics
void COptTree::UpDateToolBar()
{
	if (ined)
	{
		PutButton(B[0]==1);
		ShowOptEmiter();
	}
}

#ifdef _DEBUG
void COptTree::AssertValid() const
{
	CFormView::AssertValid();
}

void COptTree::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


// COptTree message handlers

int COptTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;
	BOOL create=m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, 
		WS_CHILD|WS_VISIBLE|CBRS_ALIGN_TOP|CBRS_TOOLTIPS|CBRS_FLYBY);
	BOOL load=m_wndToolBar.LoadToolBar(IDR_TOOLBAR_OPT);
	m_wndToolBar.SetButtonStyle(0, TBBS_CHECKBOX);
	m_wndToolBar.SetButtonStyle(1, TBBS_CHECKBOX);
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUT_ONE, true);
	return 0;
}
void COptTree::PutButton(int ix)
{
   	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUT_ONE, B[0]=ix==0);
	m_wndToolBar.GetToolBarCtrl().CheckButton(ID_BUT_TWO, B[1]=ix==1);
}
/*
const nEMMITER_TYPE = 4;
int nVEL_TYPE = 8;
const nSPLINE_ENDING=3;
const nPOS_TYPE = 7;
const nOUT_TYPE = 3;
const nTYPE_DIRECTION = 4;
const nPOINTS_LOCATION = 2;
char* EMMITER_TYPE[nEMMITER_TYPE] = {"Базовый","Поверхность","Сплайн","Свет"};
char* VEL_TYPE[9] = {"Box","Cylinder","Sphere","Line","Normal in","Normal out",
					"Normal in out","Vertical","Normal 3D Model"};
char* SPLINE_ENDING[nSPLINE_ENDING] = {"Closed", "Free", "Cycled"};
char* YN[] = {"Нет","Да"};
char* POS_TYPE[nPOS_TYPE] = {"Box","Cylinder","Sphere","Line","Ring","3DModel","3DModelInside"};
char* OUT_TYPE[nOUT_TYPE] = {"Умножение","Сложение","Вычитание"};
char* TYPE_DIRECTION[nTYPE_DIRECTION] = {"Перемещение","Случайное","Фигура 1","Фигура 2"};
char* POINTS_LOCATION[nPOINTS_LOCATION] = {"Вручную","Спираль"};
*/
CTreeListItem* COptTree::FindItem(DWORD id,CTreeListItem* item)
{
	if (!item) item = treeList.GetRootItem();
	if (!item) return NULL;
	do
	{
		if (treeList.GetItemData(item)==id) 
			return item;
		CTreeListItem* chit = treeList.GetNextItem(item,TLGN_CHILD);
		if (chit&&(chit=FindItem(id,chit)))
			return chit;

	}while((item = treeList.GetNextItem(item,TLGN_NEXT)));
	return NULL;
}
CTreeListItem* COptTree::InsertItem(const char* cap,DWORD dat,CTreeListItem* item = TLI_ROOT)//LPSTR
{
	CTreeListItem* citem = treeList.InsertItem(cap, item);
	treeList.ShowItemCheckBox(citem,GetDataItem(dat)->GetStyle() & STI_CHECKBOX);
	treeList.SetItemData(citem,dat);
	return citem;
}
bool COptTree::SetItemText(DWORD id,CString s)
{
	CTreeListItem* item = FindItem(id);
	if (!item) return false;
	return treeList.SetItemText(item,1,s);
}
bool COptTree::SetItemСheck(DWORD id,bool v)
{
	CTreeListItem* item = FindItem(id);
	if (!item) return false;
	if (GetDataItem(id)->GetStyle()&STI_CHECKBOX)
		return treeList.SetItemCheck(item,v,0);
	if (GetDataItem(id)->GetStyle()&STI_CHECKGRAPH)
		treeList.SetItemImage(item,v);
	return true;
}
void COptTree::UpdateData()
{
	for(int i = items.size()-1; i>=0; --i)
	{
		CDataItem* it = GetDataItem(i);
		if(it->GetStyle()& (STI_EDIT | STI_COMBO))
			SetItemText(it->GetIndex(),	it->GetStrValue());
		if((it->GetStyle()& STI_CHECKBOX) || (it->GetStyle()& STI_CHECKGRAPH))
			SetItemСheck(it->GetIndex(),	  it->GetCheck());
	}

	if (_pDoc->ActiveEmitter())//temp
	{
		//if (_pDoc->ActiveEmitter()->texturesCount() == 1)
		//{
		//	CString s = _pDoc->ActiveEmitter()->texture_name().c_str();
		//	s = s.Right(s.GetLength()-s.ReverseFind('\\'));
		//	SetItemText(IX_Texture1, s);
		//}else
		//{
		//	for (int i=0; i<_pDoc->ActiveEmitter()->texturesCount(); i++)
		//	{
		//		CString s = _pDoc->ActiveEmitter()->textureNames()[i].c_str();
		//		s = s.Right(s.GetLength()-s.ReverseFind('\\'));
		//		SetItemText(IX_Texture1+i, s);
		//	}
		//}
		if (_pDoc->ActiveEmitter()->IsLighting() || _pDoc->ActiveEmitter()->IsLight())
		{
			CString s = _pDoc->ActiveEmitter()->texture_name().c_str();
			s = s.Right(s.GetLength()-s.ReverseFind('\\'));
			SetItemText(IX_Texture1, s);
		}else
		{
			for (int i=0; i<10; i++)
			{
				CString s = _pDoc->ActiveEmitter()->textureName(i).c_str();
				s = s.Right(s.GetLength()-s.ReverseFind('\\'));
				SetItemText(IX_Texture1+i, s);
			}
		}
		if (_pDoc->ActiveEmitter()->IsCLight())
		{
			CString s = _pDoc->ActiveEmitter()->texture_name().c_str();
			s = s.Right(s.GetLength()-s.ReverseFind('\\'));
			SetItemText(IX_Texture1, s);

			s = _pDoc->ActiveEmitter()->texture2_name().c_str();
			s = s.Right(s.GetLength()-s.ReverseFind('\\'));
			SetItemText(IX_Texture2, s);
		}
	}
	Spiral();
}

void COptTree::AddItem(DWORD ix, DWORD style, float min_val, float max_val, int isint)
{
	items.push_back();
	items.back().Init(ix, style, min_val, max_val, isint);
}

void COptTree::AddItem(DWORD ix, DWORD style, CString cs)
{
	ASSERT(style & STI_COMBO);
	items.push_back();
	items.back().Init(ix, style, -fl_max, fl_max, true, cs);
}


void COptTree::InitTreeList()
{
	treeList.SetStyle
		( 
		  TLC_TREELIST								// TreeList or List
//		| TLC_SHOWSELACTIVE							// show active column of selected item
		| TLC_SHOWSELFULLROWS						// show selected item in fullrow mode
		| TLC_HGRID									// show horizonal grid lines
		| TLC_VGRID									// show vertical grid lines
//		| TLC_TREELINE								// show tree line
		| TLC_ROOTLINE								// show root line
//		| TLC_BUTTON								// show expand/collapse button [+]
		| TLC_CHECKBOX								// show check box
		| TLC_NOAUTOCHECK
		| TLC_HOTTRACK								// show hover text 
//		| TLC_HEADDRAGDROP							// head drag drop
		| TLC_IMAGE									// show image
		);

	treeList.InsertColumn("0", TLF_DEFAULT_LEFT, 100);
	treeList.InsertColumn("1", TLF_DEFAULT_LEFT, 100);

	treeList.SetColumnModify(0, TLM_STATIC);
	treeList.SetColumnModify(1, TLM_REQUEST);
	treeList.setChangeItemLabelNotifyListener(this);
	treeList.setItemChangeNotifyListener(this);
	treeList.setGeneralNotifyListener(this);
	const graph_max = 200;
	const graph_min = -200;
	ImageList.Create(IDB_GRAPH_CHECKBOX, 16, 0, RGB(255, 255, 0));
	treeList.SetImageList(&ImageList);

		AddItem(IX_NONE,			STI_TITLE);
		AddItem(IX_ShowGrid,		STI_CHECKBOX);
		AddItem(IX_ShowWorld,		STI_CHECKBOX);
		AddItem(IX_Scale,			STI_EDIT, 1, 1000, true);
		AddItem(IX_Rate,			STI_EDIT, 1, 100, true);
		AddItem(IX_ShowFigure,		STI_CHECKBOX);
		AddItem(IX_SpriteBlend, 	STI_COMBO,"Умножение,Сложение,Вычитание,");
		AddItem(IX_PosType,			STI_COMBO, "Box,Cylinder,Sphere,Line,Ring,3DModel,3DModelInside,По эмитеру,");
		AddItem(IX_X, 				STI_EDIT,0,+1e10,true);
		AddItem(IX_Y, 				STI_EDIT,0,+1e10,true);
		AddItem(IX_Z, 				STI_EDIT,0,+1e10,true);
		AddItem(IX_AlphaMin, 		STI_EDIT, -360, 360, true);
		AddItem(IX_AlphaMax, 		STI_EDIT, -360, 360, true);
		AddItem(IX_ThetaMin, 		STI_EDIT, -90, 90, true);
		AddItem(IX_ThetaMax, 		STI_EDIT, -90, 90, true);
		AddItem(IX_UseLight, 		STI_CHECKBOX);
		AddItem(IX_EmitterType, 	STI_COMBO,"Базовый,Поверхность,Сплайн,Свет,Столб света,Молния,");
		AddItem(IX_PointsCount, 	STI_EDIT, 1, 100, true);
		AddItem(IX_PointLoc,		STI_COMBO,"Вручную,Спираль,");
		AddItem(IX_SpRadius1, 		STI_EDIT,0,1000,true);
		AddItem(IX_SpRadius2, 		STI_EDIT,0,1000,true);
		AddItem(IX_SpHeight1, 		STI_EDIT,0,10000,true);
		AddItem(IX_SpHeight2, 		STI_EDIT,0,10000,true);
		AddItem(IX_SpAngle,	  		STI_EDIT,-180,180,true);
		AddItem(IX_SpCompress,		STI_EDIT, 0,200);
		AddItem(IX_EmitterTimeLife,	STI_EDIT,0.1f,60);
		AddItem(IX_ParticlesCount,	STI_GRAPH, 0, 10000, true);
		AddItem(IX_Prolonged, 		STI_CHECKBOX);
		AddItem(IX_PlayCycled,		STI_CHECKBOX);
		AddItem(IX_Velocity,		STI_GRAPH,graph_min, graph_max);
		AddItem(IX_Mul,				STI_GRAPH,graph_min, graph_max);
		AddItem(IX_VelType,			STI_COMBO,"Box,Cylinder,Sphere,Line,Normal in,Normal out,Normal in out,Vertical,Normal 3D Model,Invariably,");
		AddItem(IX_SplineEnding,	STI_COMBO,"Closed,Free,Cycled,");
		AddItem(IX_SplTypeDirection,STI_COMBO,"Перемещение,Случайное,Фигура 1,Фигура 2,");
		AddItem(IX_Size,			STI_GRAPH,0, graph_max);
		AddItem(IX_Gravitation,		STI_GRAPH,graph_min, graph_max);
		AddItem(IX_AngleVel,		STI_GRAPH,graph_min, graph_max);
		AddItem(IX_Plume,			STI_CHECKBOX);
		AddItem(IX_PlInterval,		STI_EDIT, 1 , 1000);
		AddItem(IX_PlTracesCount,	STI_EDIT,1, 1000,true);
		AddItem(IX_PlTimeScale,		STI_EDIT);
		AddItem(IX_PlSizeScale,		STI_EDIT,0);
		AddItem(IX_DeltaSize, 		STI_GRAPH,0, graph_max);
		AddItem(IX_DeltaLifeTime, 	STI_GRAPH,0, graph_max);
		AddItem(IX_DeltaVel,  		STI_GRAPH,graph_min, graph_max);
		AddItem(IX_AngleChaos,		STI_CHECKBOX);
		AddItem(IX_SurAddZ,			STI_EDIT,0);
		AddItem(IX_SurAngle,		STI_EDIT, -180, 180, true);
		AddItem(IX_SurCenter,		STI_CHECKBOX);
		AddItem(IX_SurUseForceField,STI_CHECKBOX);
		AddItem(IX_SurPlanar,		STI_CHECKBOX);
		AddItem(IX_OtherEmiter,		STI_COMBO," ,");
		AddItem(IX_Filling,			STI_CHECKBOX);
		AddItem(IX_Fix_X,			STI_EDIT,1,1000,true);
		AddItem(IX_Fix_Y,			STI_EDIT,1,1000,true);
		AddItem(IX_Fix_Z,			STI_EDIT,1,1000,true);
		AddItem(IX_Fix_Pos,			STI_CHECKBOX);
		AddItem(IX_PlSmooth,		STI_CHECKBOX);
		AddItem(IX_Realtive,		STI_CHECKBOX);
		AddItem(IX_Draw_First,		STI_COMBO, "Использовать Z,Выше травы,Ниже травы,");
		AddItem(IX_U_VEL,			STI_GRAPH, graph_min, graph_max);
		AddItem(IX_V_VEL,			STI_GRAPH, graph_min, graph_max);
		AddItem(IX_HEIGHT_CL,		STI_GRAPH, graph_min, graph_max);
		AddItem(IX_COLOR_MODE,		STI_COMBO, "Умножение,Сложение,");
		AddItem(IX_NEED_WIND,		STI_CHECKBOX);
		AddItem(IX_K_WIND_MIN,			STI_EDIT);
		AddItem(IX_Generate_time	, STI_EDIT, 0.01f);
		AddItem(IX_Strip_width_begin, STI_EDIT, 0);
		AddItem(IX_strip_width_time	, STI_EDIT, 0);
		AddItem(IX_strip_length		, STI_EDIT, 0);
		AddItem(IX_fade_time		, STI_EDIT, 0);
		AddItem(IX_lighting_amplitude,STI_EDIT);
		AddItem(IX_Lighting_count	, STI_EDIT, 1, 100, true);
		AddItem(IX_Particle_LifeTime, STI_EDIT, 0);
		AddItem(IX_EmitterScale		, STI_GRAPH, 0, 10);
		AddItem(IX_Texture2			, STI_BROWSE);
		AddItem(IX_ModelScale		, STI_EDIT, 0);
		AddItem(IX_Turn				, STI_CHECKBOX);
		AddItem(IX_Plane			, STI_CHECKBOX);
		AddItem(IX_Size2			, STI_GRAPH);
		AddItem(IX_LightOnTerrain,    STI_CHECKBOX);
		AddItem(IX_LightBlending,     STI_COMBO, "Сложение, Вычитание,");
		AddItem(IX_PlaneOnWater,	  STI_CHECKBOX);
		AddItem(IX_Laser,			  STI_CHECKBOX);
		AddItem(IX_Cone,			  STI_CHECKBOX);
		AddItem(IX_SetBottom,		  STI_CHECKBOX);
		AddItem(IX_RandomFrame,		  STI_CHECKBOX);
		AddItem(IX_TextureCount,	  STI_EDIT,1,10);
		AddItem(IX_SizeByTexture,	STI_CHECKBOX);
		AddItem(IX_ScaleEffectWithModel,STI_CHECKBOX);
		for(int i=0; i<10;i++)
		{
			AddItem(IX_Texture1+i			, STI_BROWSE);
		}
		AddItem(IX_VelNoise,			STI_CHECKBOX);
		AddItem(IX_VelProp,				STI_PROP);
		AddItem(IX_DirNoise,			STI_CHECKBOX);
		AddItem(IX_DirProp,				STI_PROP);
		AddItem(IX_IsVelNoiseOther,		STI_CHECKBOX);
		AddItem(IX_VelNoiseOther,		STI_COMBO," ,");
		AddItem(IX_IsDirNoiseOther,		STI_CHECKBOX);
		AddItem(IX_DirNoiseOther,		STI_COMBO," ,");
		AddItem(IX_LightOnObjects,		STI_CHECKBOX);
		AddItem(IX_OrientedByDirection, STI_CHECKBOX);
		AddItem(IX_BlockX,				STI_CHECKBOX);
		AddItem(IX_BlockY,				STI_CHECKBOX);
		AddItem(IX_BlockZ,				STI_CHECKBOX);
		AddItem(IX_NoiseReplace,		STI_CHECKBOX);
		AddItem(IX_SetPosX,				STI_EDIT);
		AddItem(IX_SetPosY,				STI_EDIT);
		AddItem(IX_SetPosZ,				STI_EDIT);
		AddItem(IX_AlphaDir,			STI_EDIT);
		AddItem(IX_BetaDir,			STI_EDIT);
		AddItem(IX_Mirage,				STI_CHECKBOX);
		AddItem(IX_SoftSmoke,			STI_CHECKBOX);
		AddItem(IX_K_WIND_MAX,			STI_EDIT);
		AddItem(IX_IgnoreParticleRate,	STI_CHECKBOX);
		AddItem(IX_OrientedToCenter,	STI_CHECKBOX);
		AddItem(IX_OrientedToAxis,	STI_CHECKBOX);

		for(int i=0;i<items.size();++i)
		{
			ASSERT(i==items[i].GetIndex());
		}
		GetDataItem(IX_Velocity)->SetCheck(true);
		Write(IX_Scale, 100);
		SetTimer(WM_TIMER,100,NULL);
}

DWORD COptTree::onRequestCtrl(CTreeListCtrl& source,CHANGE_LABEL_NOTIFY_INFO* info)
{
	DWORD dat = source.GetItemData(info->pItem);
	if (dat<items.size())
	{
		DWORD st  = items[dat].GetStyle();
		if (st & STI_EDIT)
			return TLM_EDIT;
		if (st & STI_COMBO)
			return TLM_COMBO;
	}
	return TLM_STATIC;		
}

bool COptTree::onBeginControl(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* info)
{
	DWORD dat = source.GetItemData(info->pItem);
	if (dat<items.size())
	{
		curent_editing = dat;
		CDataItem& item = *GetDataItem(dat);
		switch (onRequestCtrl(source, info))
		{
		case TLM_COMBO:
			{
				CComboBox *cb = static_cast<CComboBox*>(info->pEditControl);
				cb->ResetContent();
				for(int i=0; i<item.GetComboStrCount(); ++i)
					if (item.GetComboStrVisible(i))
						cb->AddString(item.GetComboStr(i));
				cb->SelectString(-1,item.GetStrValue());
			}
			break;
		case TLM_EDIT:
			info->pEditControl->SetWindowText(item.GetStrValue());
			break;
		}
	}
	return false;
}
void COptTree::onEndControl(CTreeListCtrl& source,CHANGE_LABEL_NOTIFY_INFO* info)
{
	curent_editing = -1;
}
bool COptTree::onBeginLabelEdit(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* info)
{
	return onRequestCtrl(source,info)!=TLM_STATIC&&info->iCol==1;
}

	//! Уведомляет о том, что редактирование закончено
bool COptTree::onEndLabelEdit(CTreeListCtrl& source, CHANGE_LABEL_NOTIFY_INFO* info)
{
	if (info->text=="") return false;
	DWORD dat = source.GetItemData(info->pItem);
	if (dat<items.size())
	{
		CDataItem& item = items[source.GetItemData(info->pItem)];
		if (_pDoc->ActiveEmitter())
			_pDoc->History().PushEmitter();
		item.SetStrValue(info->text);
		int pos = treeList.GetScrollPos(1);
//		GetDocument()->Push();
		SaveControlsData(dat);
		treeList.SetScrollPos(1,pos, false);
		UpdateData();
	}
	return false;
}
void COptTree::onRClick(CTreeListCtrl& source, GENERAL_NOTIFY_INFO* info)
{
	if (info->item==NULL)
		return;
	if(info->iSubItem==1)
	{
		CPoint pt;
		GetCursorPos(&pt);
		DWORD style = GetDataItem(source.GetItemData(info->item))->GetStyle();
		if (style==STI_BROWSE)
		{
			DWORD dat = source.GetItemData(info->item);
			CDataItem& item = items[source.GetItemData(info->item)];
			string *name = NULL;
			if (_pDoc->ActiveEmitter()->IsLighting() || _pDoc->ActiveEmitter()->IsCLight() || _pDoc->ActiveEmitter()->IsLight())
			{
				if (dat == IX_Texture2)
					name = &_pDoc->ActiveEmitter()->texture2_name();
				else
					name = &(_pDoc->ActiveEmitter()->texture_name());
			}else
			{
				xassert(dat >= IX_Texture1);
				name  = &(_pDoc->ActiveEmitter()->textureName(dat-IX_Texture1));
			}
			//string &name = (dat == IX_Texture1)? _pDoc->ActiveEmitter()->texture_name() : _pDoc->ActiveEmitter()->texture2_name();
			if (!(*name).empty() && MessageBox("Удалить текстуру?","Внимание!!!",MB_ICONQUESTION|MB_YESNO) == IDYES)
			{
				(*name).clear();
				UpdateData();
				_pDoc->ActiveEmitter()->SetDirty(true);
				theApp.scene.InitEmitters();
			}
		}
	}
}
void COptTree::onClick(CTreeListCtrl& source, GENERAL_NOTIFY_INFO* info)
{
	static int myClick = 0;
	if (myClick)
	{
		myClick--;
		return;
	}
	if (info->item==NULL)
		return;
	if(info->iSubItem==1)
	{
		CPoint pt;
		GetCursorPos(&pt);
		DWORD style = GetDataItem(source.GetItemData(info->item))->GetStyle();
		if (style & STI_COMBO) 
		{
			CRect t;
			::GetWindowRect(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),&t);
			pt.x-=t.left;
			pt.y-=t.top;
			DWORD pp = ((DWORD)pt.y<<16)+(WORD)pt.x;
			myClick = true;
			myClick = 2;
			::PostMessage(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),WM_LBUTTONDOWN,0,(LPARAM)pp);
			::PostMessage(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),WM_LBUTTONUP,0,(LPARAM)pp);
			::PostMessage(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),WM_LBUTTONDOWN,0,(LPARAM)pp);
			::PostMessage(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),WM_LBUTTONUP,0,(LPARAM)pp);
		}
		else if (style==STI_BROWSE)
		{
			DWORD dat = source.GetItemData(info->item);
			CDataItem& item = items[source.GetItemData(info->item)];
			string *name = NULL;
			if (_pDoc->ActiveEmitter()->IsLighting() || _pDoc->ActiveEmitter()->IsCLight() || _pDoc->ActiveEmitter()->IsLight())
			{
				if (dat == IX_Texture2)
					name = &_pDoc->ActiveEmitter()->texture2_name();
				else
					name = &(_pDoc->ActiveEmitter()->texture_name());
			}else
			{
				xassert(dat >= IX_Texture1);
				name  = &(_pDoc->ActiveEmitter()->textureName(dat-IX_Texture1));
			}
			//string &name = (dat == IX_Texture1)? _pDoc->ActiveEmitter()->texture_name() : _pDoc->ActiveEmitter()->texture2_name();
			CDlgLoadSprite dlg(NULL,name->c_str());
			dlg.SetStrTexture(name->c_str());

			char cb[MAX_PATH];
			_getcwd(cb, 100);
			std::string s = theApp.GetProfileString("Paths","SpritePath",cb);
			dlg.m_ofn.lpstrInitialDir = (LPSTR)s.c_str();

			if(dlg.DoModal() == IDOK)
			{
				if (_pDoc->m_StorePath.IsEmpty())
					*name = dlg.GetStrTexture();
				else
				{
					*name = _pDoc->CopySprite(dlg.GetStrTexture());
					if (*name!="")
						*name = string(FOLDER_SPRITE)+(*name);
				}
				UpdateData();
				_pDoc->ActiveEmitter()->SetDirty(true);
				theApp.scene.InitEmitters();
				theApp.WriteProfileString("Paths","SpritePath",dlg.GetStrTexture());
			}
		}else if (style==STI_PROP)
		{
			DWORD dat = source.GetItemData(info->item);
			CDataItem& item = items[source.GetItemData(info->item)];
			if (dat == IX_VelProp)
			{
				CNoiseProperties propDlg;
				propDlg.octaves		= _pDoc->ActiveEmitter()->velOctaves();
				propDlg.m_Frequency = _pDoc->ActiveEmitter()->velFrequency();
				propDlg.m_Amplitude = _pDoc->ActiveEmitter()->velAmplitude();
				propDlg.m_Positive	= _pDoc->ActiveEmitter()->velOnlyPositive();
				if (propDlg.DoModal() == IDOK)
				{
					_pDoc->ActiveEmitter()->velOctaves()  =	propDlg.octaves;
					_pDoc->ActiveEmitter()->velFrequency()= propDlg.m_Frequency;
					_pDoc->ActiveEmitter()->velAmplitude()= propDlg.m_Amplitude;
					_pDoc->ActiveEmitter()->velOnlyPositive() = propDlg.m_Positive;
				}
				_pDoc->ActiveEmitter()->SetDirty(true);
				theApp.scene.InitEmitters();
			}else
			if (dat == IX_DirProp)
			{
				CNoiseProperties propDlg;
				propDlg.octaves		= _pDoc->ActiveEmitter()->dirOctaves();
				propDlg.m_Frequency = _pDoc->ActiveEmitter()->dirFrequency();
				propDlg.m_Amplitude = _pDoc->ActiveEmitter()->dirAmplitude();
				propDlg.m_Positive	= _pDoc->ActiveEmitter()->dirOnlyPositive();
				if (propDlg.DoModal() == IDOK)
				{
					_pDoc->ActiveEmitter()->dirOctaves()  =	propDlg.octaves;
					_pDoc->ActiveEmitter()->dirFrequency()= propDlg.m_Frequency;
					_pDoc->ActiveEmitter()->dirAmplitude()= propDlg.m_Amplitude;
					_pDoc->ActiveEmitter()->dirOnlyPositive() = propDlg.m_Positive;
				}
				_pDoc->ActiveEmitter()->SetDirty(true);
				theApp.scene.InitEmitters();
			}
		}
		ctrv->ShowKeys();
	}
	else
	{
		DWORD dat = source.GetItemData(info->item);
		if (dat>=items.size())
			return;
		CDataItem& item = items[source.GetItemData(info->item)];
		if (item.GetStyle() & STI_CHECKGRAPH)
		{
			CRect t;
			CPoint pt;
			GetCursorPos(&pt);
			::GetWindowRect(GetDlgItem(IDC_CUSTOM1)->GetSafeHwnd(),&t);
			pt.x-=t.left;
			if((UINT)pt.x>16&&(UINT)pt.x<32)
			{
				item.SetCheck(!item.GetCheck());
				treeList.SetItemImage(info->item,item.GetCheck());
			}
		}
		ctrv->ShowKeys();
	}
}
void COptTree::onItemCheckChanged(CTreeListCtrl& source, ITEM_CHANGE_INFO* info)
{
	DWORD dat = source.GetItemData(info->item);
	if (dat>=items.size())
		return;
	CDataItem* item = GetDataItem(dat);
	bool ch = treeList.GetItemCheck(info->item); 
	if (item&&item->GetCheck()!=ch)
	{
		if (_pDoc->ActiveEmitter())
			_pDoc->History().PushEmitter();
		item->SetCheck(ch);
		int pos = treeList.GetScrollPos(1);
		SaveControlsData(dat);
		treeList.SetScrollPos(1,pos, false);
	}
}
CDataItem* COptTree::GetDataItem(int ix)
{
	ASSERT((UINT)ix<items.size());
	return &items[ix];
}
inline float COptTree::floatV(int ix)
{
	CDataItem* it = GetDataItem(ix);
	if (!it){ASSERT(false); return 0;}
	return it->GetRealyValue();
}
int COptTree::intV(int ix)
{
	CDataItem* it = GetDataItem(ix);
	if (!it){ASSERT(false); return 0;}
	return round(it->GetRealyValue());
}
bool COptTree::boolV(int ix)
{
	CDataItem* it = GetDataItem(ix);
	if (!it){ASSERT(false); return 0;}
	return it->GetCheck();
}
void COptTree::Write(int ix, float* p_nv)
{
	ASSERT(p_nv);
	CDataItem* it = GetDataItem(ix);
	if (!it){ASSERT(false); return;}
	if (it->GetStyle() & (STI_EDIT|STI_COMBO))
	{
		it->SetValue(*p_nv);//Realy
		if (it->GetValue()!=*p_nv)
			*p_nv = it->GetValue();
	}
	else it->SetCheck(*p_nv);
}
void COptTree::Write(int ix, float nv)
{
	CDataItem* it = GetDataItem(ix);
	if (!it){ASSERT(false); return;}
	if (it->GetStyle() & (STI_EDIT|STI_COMBO))
		it->SetValue(nv);//Realy
	else it->SetCheck(nv);
}
void COptTree::ChangeEmitterName(CString old_name, CString new_name)
{
	CEffectData* ek = GetDocument()->ActiveEffect();
	if (!ek)
		return;
	for(int i=ek->EmittersSize()-1; i>=0;i--)
	{
		CEmitterData* em = ek->Emitter(i);
		if (em->IsBase())
			if (em->other().c_str() == old_name)
				em->other() = new_name;
	}
	UpdateData();
}
void COptTree::CheckRangeEmitter()
{
	CEffectToolDoc* pDoc = GetDocument();
	if (!pDoc) 
		return;
	CEmitterData* edat = pDoc->ActiveEmitter();
	if (edat)
	{
		int n;
		n = edat->emitter_position().size();
		int t = GetDocument()->m_nCurrentGenerationPoint;
		for(int i = n-1; i>=0; --i)
		{
			GetDocument()->m_nCurrentGenerationPoint = i;
			SetControlsData(false, i==t);
		}
		GetDocument()->m_nCurrentGenerationPoint = t;

		if (edat->IsBase())
			n = edat->p_size().size();
		else 
			n = edat->emitter_size().size();
		t = GetDocument()->m_nCurrentParticlePoint;
		for(int i = n-1; i>=0; --i)
		{
			GetDocument()->m_nCurrentParticlePoint = i;
			SetControlsData(false, i==t);
		}
		GetDocument()->m_nCurrentParticlePoint = t;
	}
}

void COptTree::SetControlsData(bool show /*= true*/, bool update /*= false*/)
{
	static bool stop = false;
	if (stop) return;
	stop = true;
	Write(IX_Rate,			100*theApp.scene.m_particle_rate);
	Write(IX_ShowFigure,	theApp.scene.bShowEmitterBox);
	Write(IX_ShowWorld,		theApp.scene.bShowWorld);
	Write(IX_ShowGrid,		theApp.scene.bShowGrid);
	Write(IX_ModelScale,	_pDoc->ActiveEffect()->GetModelScale()*one_size_model);
	Write(IX_ScaleEffectWithModel,_pDoc->ActiveEffect()->ScaleEffectWithModel());

	CEffectToolDoc* pDoc = GetDocument();
	if (!pDoc){stop = false;return;}
	CEmitterData* edat = pDoc->ActiveEmitter();
	if (!edat){stop = false;return;}
	if (edat->IsBase() || edat->IsCLight())
		Write(IX_SpriteBlend, edat->sprite_blend());

	if (!edat->IsLighting())
	{
		if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
		{
			CKeyPos& emit_pos =  edat->emitter_position();
			int GPoint= GetDocument()->m_nCurrentGenerationPoint;
			Write(IX_SetPosX,emit_pos[GPoint].pos.x);
			Write(IX_SetPosY,emit_pos[GPoint].pos.y);
			Write(IX_SetPosZ,emit_pos[GPoint].pos.z);
		}
		if(theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE&&edat->IsSpl())
		{
			KeyPos& spl_pos = edat->p_position()[GetDocument()->m_nCurrentParticlePoint];
			Write(IX_SetPosX,spl_pos.pos.x);
			Write(IX_SetPosY,spl_pos.pos.y);
			Write(IX_SetPosZ,spl_pos.pos.z);
		}
	}else if(edat->IsLighting())
	{
		if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
		{
			int GPoint= GetDocument()->m_nCurrentGenerationPoint;
			if (GPoint==0)
			{
				Write(IX_SetPosX,edat->pos_begin().x);
				Write(IX_SetPosY,edat->pos_begin().y);
				Write(IX_SetPosZ,edat->pos_begin().z);
			}else
			{
				Write(IX_SetPosX,edat->pos_end()[GPoint-1].x);
				Write(IX_SetPosY,edat->pos_end()[GPoint-1].y);
				Write(IX_SetPosZ,edat->pos_end()[GPoint-1].z);
			}
		}
	}
	if (edat->IsBase() || edat->IsCLight())
	{
		QuatF* q = NULL;
		if (edat->IsBase())
		{
			if (theApp.scene.m_ToolMode == CD3DScene::TOOL_DIRECTION_BS)
			{
				EffectBeginSpeed& bs = edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint];
				q=&bs.rotation;
			}else
			{
				float fTime = edat->GenerationPointGlobalTime(GetDocument()->m_nCurrentGenerationPoint);
				KeyRotate* r_key = edat->GetBase()->GetOrCreateRotateKey(fTime, 0);
				if (r_key)
					q = &r_key->pos;
			}
		}else
		{
			q = &edat->rot();
		}
		if(q)
		{
			float alpha = 0;
			float beta = 0;
			if ((fabs(q->z()) < FLT_EPS) && (fabs(q->y()) < FLT_EPS))
			{
				if (fabs(q->x()) < FLT_EPS)
				{
					Write(IX_AlphaDir,0.f);
					Write(IX_BetaDir,0.f);
				}else
				{
					alpha = -2.f*atan(q->x()/q->s());
					Write(IX_AlphaDir,R2G(alpha));
					Write(IX_BetaDir,90.f);
				}
			}else
			{
				if (fabs(q->z()) > FLT_EPS)
					alpha = -2.f*atan(q->x()/q->z());
				else
					alpha = 2.f*atan(q->y()/q->s());

				if (fabs(q->y()) > FLT_EPS)
					beta = -2.f*atan(q->x()/q->y());
				else
					beta = 2.f*atan(q->z()/q->s());
				Write(IX_AlphaDir,R2G(alpha));
				Write(IX_BetaDir,R2G(beta));
			}
		}

	}
	if (edat->IsLighting())
	{
		Write(IX_EmitterType,TYPE_LIGHTING);
		Write(IX_EmitterTimeLife	, edat->emitter_life_time());
		Write(IX_Generate_time		, edat->generate_time());
		Write(IX_Strip_width_begin	, edat->strip_width_begin());
		Write(IX_strip_width_time	, edat->strip_width_time());
		Write(IX_strip_length		, edat->strip_length());
		Write(IX_fade_time			, edat->fade_time());
		Write(IX_lighting_amplitude	, edat->lighting_amplitude());
		Write(IX_Lighting_count		, edat->pos_end().size());
		Write(IX_PlayCycled, edat->cycled());
	}

	if(edat->IsBase())
	{
		Write(IX_IgnoreParticleRate,edat->ignoreParticleRate());
		Write(IX_Draw_First, (int)edat->draw_first_no_zbuffer());
		Write(IX_PosType, edat->particle_position().type);

		Write(IX_X, edat->particle_position().size.x*100);
		Write(IX_Y, edat->particle_position().size.y*100);
		Write(IX_Z, edat->particle_position().size.z*100);
		Write(IX_Cone, edat->cone());
		Write(IX_SetBottom, edat->bottom());
		Write(IX_Fix_Pos, edat->particle_position().fix_pos);
		Write(IX_Fix_X, edat->particle_position().num.x);
		Write(IX_Fix_Y, edat->particle_position().num.y);
		Write(IX_Fix_Z, edat->particle_position().num.z);
		Write(IX_RandomFrame,edat->randomFrame());
		Write(IX_Mirage,edat->mirage());
		Write(IX_SoftSmoke,edat->softSmoke());
		//Write(IX_TextureCount,edat->texturesCount());

		Write(IX_Realtive,edat->relative());
		Write(IX_Filling, edat->chFill());

		Write(IX_PointsCount, edat->num_particle().size());
		//Write(IX_PointsCount, edat->emitter_position().size());
		Write(IX_EmitterTimeLife, &edat->emitter_life_time());
		Write(IX_ParticlesCount, (int)edat->num_particle()[GetDocument()->m_nCurrentGenerationPoint].f);
		Write(IX_DeltaSize, &edat->begin_size_delta()[GetDocument()->m_nCurrentGenerationPoint].f);
		Write(IX_DeltaLifeTime, &edat->life_time_delta()[GetDocument()->m_nCurrentGenerationPoint].f);
		Write(IX_Size, &edat->p_size()[GetDocument()->m_nCurrentParticlePoint].f);
		Write(IX_AngleVel, &edat->p_angle_velocity()[GetDocument()->m_nCurrentParticlePoint].f);
		Write(IX_EmitterScale, edat->emitter_scale()[GetDocument()->m_nCurrentGenerationPoint].f);
		Write(IX_VelNoise,edat->velNoise());
		Write(IX_IsVelNoiseOther,edat->IsVelNoiseOther());
		Write(IX_SizeByTexture,edat->sizeByTexture());
		if(boolV(IX_IsVelNoiseOther))
		{
			CEffectData* ek = pDoc->ActiveEffect();
			CString s = "не связан,";
			CString cur = edat->velNoiseOther().c_str();
			int ix=0,i=0,n=0;
			if (edat->IsBase()) 
			{
				for (int i=0; i<ek->EmittersSize(); i++)
				{
					CEmitterData* em = ek->Emitter(i);
					if (em->IsBase() && (em != edat) && (em->velNoise()))
					{
						s+=(em->name() + ",").c_str();
						n++;
						if (cur==em->name().c_str())// opt
						{
							//ix = i+n;
							ix = n;
						}
					}
				}
				GetDataItem(IX_VelNoiseOther)->SetComboStr(s);
				Write(IX_VelNoiseOther,ix);

			}
		}

		Write(IX_DirNoise,edat->dirNoise());
		Write(IX_IsDirNoiseOther,edat->IsDirNoiseOther());
		Write(IX_BlockX,edat->BlockX());
		Write(IX_BlockY,edat->BlockY());
		Write(IX_BlockZ,edat->BlockZ());
		Write(IX_NoiseReplace,edat->noiseReplace());
		if(boolV(IX_IsDirNoiseOther))
		{
			CEffectData* ek = pDoc->ActiveEffect();
			CString s = "не связан,";
			CString cur = edat->dirNoiseOther().c_str();
			int ix=0,i=0,n=0;
			if (edat->IsBase()) 
			{
				for (int i=0; i<ek->EmittersSize(); i++)
				{
					CEmitterData* em = ek->Emitter(i);
					if (em->IsBase() && (em != edat) && (em->dirNoise()))
					{
						s+=(em->name() + ",").c_str();
						n++;
						if (cur==em->name().c_str())// opt
						{
							//ix = i+n;
							ix = n;
						}
					}
				}
				GetDataItem(IX_DirNoiseOther)->SetComboStr(s);
				Write(IX_DirNoiseOther,ix);

			}
		}

		Write(IX_AngleChaos, edat->rotation_direction() == ETRD_RANDOM);

		Write(IX_AlphaMin, R2G(edat->particle_position().alpha_min));
		Write(IX_AlphaMax, R2G(edat->particle_position().alpha_max));
		Write(IX_ThetaMin, R2G(edat->particle_position().teta_min));
		Write(IX_ThetaMax, R2G(edat->particle_position().teta_max));

		Write(IX_Plume, edat->chPlume());
		Write(IX_PlTracesCount, edat->TraceCount());
		Write(IX_PlInterval, edat->PlumeInterval()*100);
		Write(IX_PlSmooth, edat->smooth());
		if (SpiralDef())
		{
			int ix = -1;
			if (intV(IX_PointsCount)>1&&theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
				ix = 0;
			else if (edat->IsSpl() && 
				theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE)
				ix = 1;
			if (ix!=-1)
			{
				SpiralData::Dat &sdat = edat->spiral_data().GetData(ix);
				Write(IX_SpAngle,R2G(sdat.dalpha));
				Write(IX_SpRadius1,sdat.r1);
				Write(IX_SpRadius2,sdat.r2);
				Write(IX_SpHeight1,sdat.h1);
				Write(IX_SpHeight2,sdat.h2);
				Write(IX_SpCompress,sdat.compress*100);
			}
		}
		Write(IX_PlayCycled, edat->cycled());
		Write(IX_Prolonged, edat->generate_prolonged());
//		if (base->particle_position.type==EMP_OTHER_EMITTER)
		{
			CEffectData* ek = pDoc->ActiveEffect();
			CString s = "не связан,";
			CString cur = edat->other().c_str();
			int ix=0,i=0,n=1;
			if (edat->IsBase())
				for(int i=0; i<ek->EmittersSize(); i++)
				{
					CEmitterData* em = ek->Emitter(i);
					if (em!=edat)
					{
						if (cur==em->name().c_str())// opt
						{
							ix = i+n;
						}
						s+=(em->name() + ",").c_str();
					}else
						n=0;
				}
			GetDataItem(IX_OtherEmiter)->SetComboStr(s);
			Write(IX_OtherEmiter,ix);
		}
		Write(IX_NEED_WIND, edat->need_wind());
		Write(IX_K_WIND_MIN, edat->k_wind_min());
		if(edat->k_wind_max()<0)
			edat->k_wind_max() = edat->k_wind_min();
		Write(IX_K_WIND_MAX, edat->k_wind_max());
		float plf = edat->GetParticleLifeTime();
		Write(IX_Particle_LifeTime, plf);
	}
	else if (edat->IsLight() || edat->IsCLight())
	{
//		EmitterKeyInterface* emit = pActiveEmitter->emiter();
		if (edat->IsLight())
		{
			Write(IX_EmitterType,TYPE_LIGHT);
			Write(IX_LightOnObjects,edat->toObjects());
			Write(IX_LightOnTerrain,edat->toTerrain());
			Write(IX_LightBlending,edat->light_blend()-1);
		}
		else if (edat->IsCLight())
			Write(IX_EmitterType,TYPE_COLUMN_LIGHT);
		Write(IX_PointsCount, edat->emitter_position().size());
		Write(IX_EmitterTimeLife, &edat->emitter_life_time());
		Write(IX_PlayCycled, edat->cycled());
		Write(IX_Size, &edat->emitter_size()[GetDocument()->m_nCurrentParticlePoint].f);

		if (SpiralDef() && (intV(IX_PointsCount)>1))
		{
			if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
			{
				SpiralData::Dat &sdat = edat->spiral_data().GetData(0);
				Write(IX_SpAngle,R2G(sdat.dalpha));
				Write(IX_SpRadius1,sdat.r1);
				Write(IX_SpRadius2,sdat.r2);
				Write(IX_SpHeight1,sdat.h1);
				Write(IX_SpHeight2,sdat.h2);
				Write(IX_SpCompress,sdat.compress*100);
			}
		}
		if (edat->IsCLight())
		{
			Write(IX_U_VEL, edat->u_vel()[GetDocument()->m_nCurrentParticlePoint].f);
			Write(IX_V_VEL, edat->v_vel()[GetDocument()->m_nCurrentParticlePoint].f);
			Write(IX_HEIGHT_CL, edat->height()[GetDocument()->m_nCurrentParticlePoint].f/100);
			Write(IX_COLOR_MODE, edat->color_mode());
			Write(IX_Plane,	edat->plane());
			Write(IX_Turn,  edat->turn());
			Write(IX_Laser,  edat->laser());
			Write(IX_Size2, edat->emitter_size2()[GetDocument()->m_nCurrentParticlePoint].f);
		}
//		Write(IX_Prolonged, pActiveEmitter->data_light->generate_prolonged);
	}
	if (edat->IsSpl())
	{
		Write(IX_EmitterType,TYPE_SPLINE);
		Write(IX_SplineEnding, edat->p_position().cbegin);
		Write(IX_SplineEnding, edat->p_position().cend);
		Write(IX_SplTypeDirection, edat->direction());
	}
	if (edat->IsIntZ())
	{
		Write(IX_EmitterType,(float)TYPE_INTEGRAL);
		Write(IX_DeltaVel, &edat->velocity_delta()[GetDocument()->m_nCurrentGenerationPoint].f);
		Write(IX_Gravitation, &edat->p_gravity()[GetDocument()->m_nCurrentParticlePoint].f);
		Write(IX_Velocity, &edat->p_velocity()[GetDocument()->m_nCurrentParticlePoint].f);
		Write(IX_VelType, edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint].velocity);
		GetDataItem(IX_VelType)->SetComboStrVisible(EMV_NORMAL_3D_MODEL, intV(IX_PosType) == EMP_3DMODEL_INSIDE || intV(IX_PosType) == EMP_3DMODEL);
		Write(IX_Mul, &edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint].mul);
		Write(IX_UseLight, edat->use_light());
	}
	if(edat->IsIntZ())
	{
		Write(IX_SurCenter, edat->angle_by_center());
	}
	if (edat->IsZ())
	{
		Write(IX_EmitterType,TYPE_SURFACE);
		Write(IX_SurAddZ, edat->add_z());
		Write(IX_SurUseForceField, edat->use_force_field());
		Write(IX_PlaneOnWater, edat->OnWater());
	}
	if (edat->IsBase())
	{
		Write(IX_OrientedByDirection, edat->oriented());
		Write(IX_SurPlanar, edat->planar());
		Write(IX_Turn,edat->planar_turn());
		Write(IX_SurAngle, R2G(edat->base_angle()));
		Write(IX_OrientedToCenter, edat->orientedCenter());
		Write(IX_OrientedToAxis, edat->orientedAxis());
	}
	if (show) ShowOptEmiter();
	if(update) UpdateData();
	stop = false;
} 

void COptTree::SaveControlsData(DWORD IX, bool show /*= true*/)
{
	static bool stop = false;
	if (stop) return;
	stop = true;
	theApp.scene.m_particle_rate = floatV(IX_Rate)/100;
	theApp.scene.bShowEmitterBox = boolV(IX_ShowFigure);
	theApp.scene.bShowWorld = boolV(IX_ShowWorld);
	theApp.scene.bShowGrid = boolV(IX_ShowGrid);
	_pDoc->ActiveEffect()->ScaleEffectWithModel() = boolV(IX_ScaleEffectWithModel);
	_pDoc->ActiveEffect()->ModelScale(floatV(IX_ModelScale)/one_size_model);


	CEffectToolDoc* pDoc = GetDocument();
	if (!pDoc){stop = false; return;}
	if(IX == IX_Scale && pDoc->ActiveEffect())
	{
		float scale = intV(IX_Scale)/100.0f;
		Write(IX_Scale,100);
		pDoc->ActiveEffect()->RelativeScale(scale);
		theApp.scene.InitEmitters();
		stop = false;
		return;
	}
	CEmitterData* edat = pDoc->ActiveEmitter();
	if (!edat){stop = false;theApp.scene.InitEmitters(); return;}

	switch(IX)
	{
	case IX_EmitterType:
		mf->SplineEndEdit();
		pDoc->m_nCurrentGenerationPoint = 0;
		pDoc->m_nCurrentParticlePoint = 0;
		if (!edat->IsBase())
		{ 
			CEffectData* ek = pDoc->ActiveEffect();
			for(int i = ek->EmittersSize()-1; i>=0; i--)
			{
				CEmitterData* em = ek->Emitter(i);
				if (em->IsBase() && em->other()==edat->name())
					em->other() = "";
			}
/*			vector<CEmitterData*>::iterator it;
			FOR_EACH(ek->emitters,it)
				if ((*it)->IsBase())
					if ((*it)->other() == edat->name())
						(*it)->other() = "";
*/
		}
		ctrv->m_nEmmiterType = intV(IX_EmitterType);
		ctrv->OnSelchangeComboEmmiterType();
//		SetControlsData(false,false);
		break;
	case IX_PointsCount:
		edat->SetGenerationPointCount(intV(IX_PointsCount));
		pDoc->m_nCurrentGenerationPoint = 0;
		Write(IX_SetPosX,edat->emitter_position()[0].pos.x);
		Write(IX_SetPosY,edat->emitter_position()[0].pos.y);
		Write(IX_SetPosZ,edat->emitter_position()[0].pos.z);

		break;
	case IX_EmitterTimeLife:
		edat->ChangeLifetime(floatV(IX_EmitterTimeLife));
		break;
	case IX_PointLoc:
		Spiral(true);
		break;
	case IX_Particle_LifeTime:
		edat->ChangeParticleLifeTime(floatV(IX_Particle_LifeTime));
		break;
	}
	if (!edat->IsLighting())
	{
		if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
		{
			CKeyPos& emit_pos =  edat->emitter_position();
			int GPoint= GetDocument()->m_nCurrentGenerationPoint;
			emit_pos[GPoint].pos.x = floatV(IX_SetPosX);
			emit_pos[GPoint].pos.y = floatV(IX_SetPosY);
			emit_pos[GPoint].pos.z = floatV(IX_SetPosZ);
		}
		if(theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE&&edat->IsSpl())
		{
			KeyPos& spl_pos = edat->p_position()[GetDocument()->m_nCurrentParticlePoint];
			spl_pos.pos.x = floatV(IX_SetPosX);
			spl_pos.pos.y = floatV(IX_SetPosY);
			spl_pos.pos.z = floatV(IX_SetPosZ);
		}

	}else if(edat->IsLighting())
	{
		if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
		{
			int GPoint= GetDocument()->m_nCurrentGenerationPoint;
			if (GPoint==0)
			{
				edat->pos_begin().x = floatV(IX_SetPosX);
				edat->pos_begin().y = floatV(IX_SetPosY);
				edat->pos_begin().z = floatV(IX_SetPosZ);
			}else
			{
				edat->pos_end()[GPoint-1].x = floatV(IX_SetPosX);
				edat->pos_end()[GPoint-1].y = floatV(IX_SetPosY);
				edat->pos_end()[GPoint-1].z = floatV(IX_SetPosZ);
			}
		}
	}
	if (edat->IsBase())
	{
		//		if(!theApp.scene.bPause)
		//				return;

		if (theApp.scene.m_ToolMode == CD3DScene::TOOL_DIRECTION_BS)
		{
			EffectBeginSpeed& bs = edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint];
			float alpha = G2R(floatV(IX_AlphaDir));
			float beta = G2R(floatV(IX_BetaDir));
			QuatF q1(cos(alpha/2.f),0,sin(alpha/2.f),0);
			QuatF q2(cos(beta/2.f),0,0,sin(beta/2.f));
			bs.rotation.mult(q2,q1);
		}else
		{
			float fTime = edat->GenerationPointGlobalTime(GetDocument()->m_nCurrentGenerationPoint);
			KeyRotate* r_key = edat->GetBase()->GetOrCreateRotateKey(fTime, 0);

			if(r_key)
			{
				float alpha = G2R(floatV(IX_AlphaDir));
				float beta = G2R(floatV(IX_BetaDir));
				QuatF q1(cos(alpha/2.f),0,sin(alpha/2.f),0);
				QuatF q2(cos(beta/2.f),0,0,sin(beta/2.f));
				r_key->pos.mult(q2,q1);
			}
		}
	}else if (edat->IsCLight())
	{
		float alpha = G2R(floatV(IX_AlphaDir));
		float beta = G2R(floatV(IX_BetaDir));
		QuatF q1(cos(alpha/2.f),0,sin(alpha/2.f),0);
		QuatF q2(cos(beta/2.f),0,0,sin(beta/2.f));
		edat->rot().mult(q2,q1);
	}


	if (edat->IsBase()||edat->IsCLight())
		edat->sprite_blend() = (EMITTER_BLEND)intV(IX_SpriteBlend);
	if (edat->IsLighting())
	{
		edat->generate_time() = floatV(IX_Generate_time);
		edat->strip_width_begin() = floatV(IX_Strip_width_begin);
		edat->strip_width_time() = floatV(IX_strip_width_time);
		edat->strip_length() = floatV(IX_strip_length);
		edat->fade_time() = floatV(IX_fade_time);
		edat->lighting_amplitude() = floatV(IX_lighting_amplitude);
		edat->ChangeLightingCount(intV(IX_Lighting_count));
		edat->cycled() = boolV(IX_PlayCycled);
	}

	if(edat->IsBase())
	{
		edat->ignoreParticleRate() = boolV(IX_IgnoreParticleRate);
		edat->draw_first_no_zbuffer() = intV(IX_Draw_First);
		edat->particle_position().type = (EMITTER_TYPE_POSITION)intV(IX_PosType);
		switch(intV(IX_PosType))
		{
		case EMP_3DMODEL_INSIDE:
		case EMP_3DMODEL:
			GetDataItem(IX_VelType)->SetComboStrVisible(8, true);
			mf->m_wndToolBar.GetToolBarCtrl().CheckButton(ID_TOOLS_DIR, FALSE);
			if(theApp.scene.m_ToolMode == CD3DScene::TOOL_DIRECTION)
				theApp.scene.m_ToolMode = CD3DScene::TOOL_NONE;
			pDoc->Load3DModel(MODE_FIND, TYPE_3DMODEL);
			pDoc->Load3DModel(MODE_FIND, TYPE_3DBACK);
			mf->UpdateControlBar();
			break;
		default:
			break;
		}
		edat->particle_position().size.x = floatV(IX_X)/100;
		edat->particle_position().size.y = floatV(IX_Y)/100;
		edat->particle_position().size.z = floatV(IX_Z)/100;
		edat->cone() = boolV(IX_Cone);
		edat->bottom() = boolV(IX_SetBottom);
		edat->mirage() = boolV(IX_Mirage);
		edat->softSmoke() = boolV(IX_SoftSmoke);

		edat->particle_position().fix_pos = boolV(IX_Fix_Pos);
		edat->particle_position().num.x = floatV(IX_Fix_X);
		edat->particle_position().num.y = floatV(IX_Fix_Y);
		if (boolV(IX_Filling) || edat->particle_position().type==EMP_BOX)
			edat->particle_position().num.z = floatV(IX_Fix_Z);
		else edat->particle_position().num.z = 1;

		edat->randomFrame() = boolV(IX_RandomFrame);
		//edat->texturesCount() = intV(IX_TextureCount);
		//edat->textureNames().resize(edat->texturesCount());

		edat->relative() = boolV(IX_Realtive);
		edat->chFill() = boolV(IX_Filling);

		edat->num_particle()[GetDocument()->m_nCurrentGenerationPoint].f = floatV(IX_ParticlesCount);
		edat->begin_size_delta()[GetDocument()->m_nCurrentGenerationPoint].f = floatV(IX_DeltaSize);
		edat->life_time_delta()[GetDocument()->m_nCurrentGenerationPoint].f = floatV(IX_DeltaLifeTime);
		edat->p_size()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_Size);
		edat->emitter_scale()[GetDocument()->m_nCurrentGenerationPoint].f = floatV(IX_EmitterScale);
		
		edat->sizeByTexture() = boolV(IX_SizeByTexture);
		edat->velNoise() = boolV(IX_VelNoise);
		edat->IsVelNoiseOther() = boolV(IX_IsVelNoiseOther);
		if (edat->IsVelNoiseOther())
			edat->velNoiseOther() = GetDataItem(IX_VelNoiseOther)->GetStrValue();
		else
			edat->velNoiseOther() = ""; 

		edat->dirNoise() = boolV(IX_DirNoise);
		edat->IsDirNoiseOther() = boolV(IX_IsDirNoiseOther);
		edat->BlockX() = boolV(IX_BlockX);
		edat->BlockY() = boolV(IX_BlockY);
		edat->BlockZ() = boolV(IX_BlockZ);
		edat->noiseReplace() = boolV(IX_NoiseReplace);
		if (edat->IsDirNoiseOther())
			edat->dirNoiseOther() = GetDataItem(IX_DirNoiseOther)->GetStrValue();
		else
			edat->dirNoiseOther() = ""; 

		edat->p_angle_velocity()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_AngleVel);

		edat->rotation_direction() = boolV(IX_AngleChaos) ? ETRD_RANDOM : ETRD_CW;

		edat->particle_position().alpha_min = G2R(floatV(IX_AlphaMin));
		edat->particle_position().alpha_max = G2R(floatV(IX_AlphaMax));
		edat->particle_position().teta_min = G2R(floatV(IX_ThetaMin));
		edat->particle_position().teta_max = G2R(floatV(IX_ThetaMax));

		edat->chPlume() = boolV(IX_Plume);
		edat->TraceCount() = floatV(IX_PlTracesCount);
		edat->PlumeInterval() = floatV(IX_PlInterval)/100;
		edat->smooth() = boolV(IX_PlSmooth);
		if (SpiralDef())
		{
			int ix = -1;
			if (intV(IX_PointsCount)>1&&theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
				ix = 0;
			else if (edat->IsSpl() && theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE)
				ix = 1;
			if (ix!=-1)
			{
				SpiralData::Dat &sdat = edat->spiral_data().GetData(ix);
				sdat.dalpha = G2R(floatV(IX_SpAngle));
				sdat.r1 = floatV(IX_SpRadius1);
				sdat.r2 = floatV(IX_SpRadius2);
				sdat.h1 = floatV(IX_SpHeight1);
				sdat.h2 = floatV(IX_SpHeight2);
				sdat.compress = floatV(IX_SpCompress)/100;
			}
		}
		edat->cycled() = boolV(IX_PlayCycled);
		edat->generate_prolonged() = boolV(IX_Prolonged);
		if (edat->particle_position().type==EMP_OTHER_EMITTER )
			edat->other() = GetDataItem(IX_OtherEmiter)->GetStrValue();
		else
			edat->other() = ""; 
		if (boolV(IX_NEED_WIND) && boolV(IX_Realtive))
		{
			MessageBox("Сносится ветром не может быть включен,\n пока поднято 'Жесткая привязка'", "сообщение");
			edat->need_wind() = false;
			Write(IX_NEED_WIND, edat->need_wind());
		}else edat->need_wind() = boolV(IX_NEED_WIND);
		if(floatV(IX_K_WIND_MIN)>floatV(IX_K_WIND_MAX))
			edat->k_wind_min() = floatV(IX_K_WIND_MAX);
		else
			edat->k_wind_min() = floatV(IX_K_WIND_MIN);

		edat->k_wind_max() = floatV(IX_K_WIND_MAX);
	}
	else if (edat->IsLight() || edat->IsCLight())
	{
		edat->cycled() = boolV(IX_PlayCycled);
		edat->emitter_size()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_Size);
		if(edat->IsLight())
		{
			edat->toObjects() = boolV(IX_LightOnObjects);
			edat->toTerrain() = boolV(IX_LightOnTerrain);
			edat->light_blend() = (EMITTER_BLEND)(intV(IX_LightBlending)+1);
		}
		if (SpiralDef()&&(intV(IX_PointsCount)>1&&theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION))
		{
			SpiralData::Dat &sdat = edat->spiral_data().GetData(0);
			sdat.dalpha = G2R(floatV(IX_SpAngle));
			sdat.r1 = floatV(IX_SpRadius1);
			sdat.r2 = floatV(IX_SpRadius2);
			sdat.h1 = floatV(IX_SpHeight1);
			sdat.h2 = floatV(IX_SpHeight2);
			sdat.compress = floatV(IX_SpCompress)/100;
		}
		if (edat->IsCLight())
		{
			edat->u_vel()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_U_VEL);
			edat->v_vel()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_V_VEL);
			edat->height()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_HEIGHT_CL)*100;
			edat->color_mode() = (EMITTER_BLEND)intV(IX_COLOR_MODE);
			edat->plane() = boolV(IX_Plane);
			edat->turn() = boolV(IX_Turn);
			edat->laser() = boolV(IX_Laser);
			edat->emitter_size2()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_Size2);
		}
	}
	if (edat->IsSpl())
	{
		edat->p_position().cbegin = (CKeyPosHermit::CLOSING)intV(IX_SplineEnding);
		edat->p_position().cend   = (CKeyPosHermit::CLOSING)intV(IX_SplineEnding);
		edat->direction() = (EMITTER_TYPE_DIRECTION_SPL)intV(IX_SplTypeDirection);
	}
	if (edat->IsIntZ())
	{
		edat->velocity_delta()[GetDocument()->m_nCurrentGenerationPoint].f = floatV(IX_DeltaVel);
		edat->p_gravity()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_Gravitation);
		edat->p_velocity()[GetDocument()->m_nCurrentParticlePoint].f = floatV(IX_Velocity);
		edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint].velocity = (EMITTER_TYPE_VELOCITY)intV(IX_VelType);
		edat->begin_speed()[GetDocument()->m_nCurrentParticlePoint].mul = floatV(IX_Mul);
		edat->use_light() = boolV(IX_UseLight);
		if (IX == IX_PosType && intV(IX_PosType) != EMP_3DMODEL_INSIDE && intV(IX_PosType) != EMP_3DMODEL)
		{
			std::vector<EffectBeginSpeed>::iterator it;
			FOR_EACH(edat->begin_speed(), it)
				if (it->velocity == EMV_NORMAL_3D_MODEL)
					it->velocity = EMV_BOX;
			if(intV(IX_VelType) == EMV_NORMAL_3D_MODEL)
				Write(IX_VelType, EMV_BOX);
		}
	}
	if(edat->IsIntZ())
	{
		edat->angle_by_center() = boolV(IX_SurCenter);
	}
	if (edat->IsZ())
	{
		edat->add_z() = floatV(IX_SurAddZ);
		edat->use_force_field() = boolV(IX_SurUseForceField);
		edat->OnWater() = boolV(IX_PlaneOnWater);
	}
	if(edat->IsBase())
	{
		edat->planar() = boolV(IX_SurPlanar);
		edat->oriented() = boolV(IX_OrientedByDirection);
		edat->planar_turn() = boolV(IX_Turn);
		edat->base_angle() = G2R(floatV(IX_SurAngle));
		edat->orientedCenter() = boolV(IX_OrientedToCenter);
		edat->orientedAxis() = boolV(IX_OrientedToAxis);
	}

	ctrv->ShowKeys();
	ctrv->m_graph.Update(false);
	ctrv->InitSlider();

	edat->SetDirty(true);
	theApp.scene.InitEmitters();
	if (show) ShowOptEmiter();
	stop = false;
}

void COptTree::OnInitialUpdate()
{
	__super::OnInitialUpdate();
	static bool first=1;
	if(first)
		InitTreeList();
	first = 0;
	ined=1;
}
void COptTree::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if (ined)
	{
		RECT r;
		GetClientRect(&r);
		r.top+=18;
		treeList.MoveWindow(&r);
		int w = (r.right-r.left)-18;
		treeList.SetColumnWidth(0,w/2);
		treeList.SetColumnWidth(1,w/2);
		m_wndToolBar.MoveWindow(0,0,w+18,18);
		GetDlgItem(IDC_STATIC_FPS)->MoveWindow(70,3,50,15);
		GetDlgItem(IDC_TRIANGLE_COUNT)->MoveWindow(110,3,100,15);
		GetDlgItem(IDC_TRIANGLE_SQUARE)->MoveWindow(170,3,100,15);
	}
}

/*
void COptTree::ShowOptEmiter()
{
	if (!ined) return;
	static bool show = false;
	if (show) return;
	show = true;
	int sel=-1;
	if (treeList.GetSelectedItem())
		sel = treeList.GetItemData(treeList.GetSelectedItem());
	treeList.DeleteAllItems();
	CTreeListItem* item;
	SetControlsData();
	GetDlgItem(IDC_TRIANGLE_COUNT)->ShowWindow(GetDocument()->ActiveEffect()!=NULL);
	GetDlgItem(IDC_TRIANGLE_SQUARE)->ShowWindow(GetDocument()->ActiveEffect()!=NULL);
	CEmitterData* edat = GetDocument()->ActiveEmitter();
	
	if (B[0])
	{
		item = InsertItem("Cеткa",IX_ShowGrid);
		item = InsertItem("Мир",IX_ShowWorld);
		item = InsertItem("Масштаб",IX_Scale);
		item = InsertItem("P",IX_Rate);
	}
	GetDocument()->m_EnableButForAll = (GetDocument()->m_nCurrentParticlePoint == 0) && edat;
	if(edat)
	{
		if (edat->IsBase()||edat->IsCLight() || edat->IsLight())
		{
		bool b = false;
		bool bl = false;
		bool bz = false;
		bool bFirstPoint = GetDocument()->m_nCurrentParticlePoint == 0;

		bool bX,bY,bZ;
		bX = bY = bZ = false;
		switch(intV(IX_PosType))
		{
		case EMP_BOX:
			bX = true;
			bY = true;
			bZ = true;
			break;
		case EMP_CYLINDER:
			bX = true;
			bY = true;
			break;
		case EMP_LINE:
		case EMP_RING:
		case EMP_SPHERE:
			bX = true;
			break;
		case EMP_3DMODEL:
		case EMP_3DMODEL_INSIDE:
		default:
			break;
		}
		switch(edat->Class())
		{
		case EMC_SPLINE:
			b = false;
			break;
		case EMC_INTEGRAL:
			b = true;
			break;
		case EMC_INTEGRAL_Z:
			b = true;
			bz = true;
			break;
		case EMC_COLUMN_LIGHT:
		case EMC_LIGHT:
			b = false;
			bl = true;
			break;
		}
		GetDocument()->m_EnableButForAll = !bl;
		bool b1 = false; 
		if(edat->IsBase())
		{
			b1 = edat->num_particle().size()>1;
			if (B[0])
			{
				InsertItem("Показать",IX_ShowFigure);
				InsertItem("Blending",IX_SpriteBlend);
				InsertItem("Распределение",IX_PosType);
				if (intV(IX_PosType)==EMP_OTHER_EMITTER)
					InsertItem("Связь",IX_OtherEmiter);
				else
				{
					if (bX) InsertItem("X",IX_X);
					if (bY) InsertItem("Y",IX_Y);
					if (bZ) InsertItem("Z",IX_Z);
					if (intV(IX_PosType)==EMP_RING)
					{
						InsertItem("Alpha min",IX_AlphaMin);
						InsertItem("Alpha max",IX_AlphaMax);
						InsertItem("Teta min", IX_ThetaMin);
						InsertItem("Teta max", IX_ThetaMax);
					}
					InsertItem("Заполнение", IX_Filling);
					item = InsertItem("Фиксировать", IX_Fix_Pos);
					if (boolV(IX_Fix_Pos))
					{
						InsertItem("dx",IX_Fix_X);
						InsertItem("dy",IX_Fix_Y);
						if (boolV(IX_Filling)||bZ)InsertItem("dz",IX_Fix_Z);
					}
				}
			}
		}
		if (B[1])
		{
			if (edat->IsCLight())
				InsertItem("Blending",IX_SpriteBlend);
			if (edat->IsIntZ()&& (intV(IX_PosType)==EMP_3DMODEL || intV(IX_PosType)==EMP_3DMODEL_INSIDE) )
				item = InsertItem("Освещение",IX_UseLight);
			item = InsertItem("Эмитер",IX_NONE);
			InsertItem("Тип",IX_EmitterType,item); 
			InsertItem("Кол-во точек",IX_PointsCount,item);
			if ((intV(IX_PointsCount)>1 && theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
					||((!b&&!bl) && theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE))
			{
				CTreeListItem* it = InsertItem("Расположение",IX_PointLoc,item);
				if (intV(IX_PointLoc) == SPIRAL)
				{
					InsertItem("Радиус 1",IX_SpRadius1,it);
					InsertItem("Радиус 2",IX_SpRadius2,it);
					InsertItem("Высота 1",IX_SpHeight1,it);
					InsertItem("Высота 2",IX_SpHeight2,it);
					InsertItem("Угол",	  IX_SpAngle,it);
					InsertItem("Cжатие",  IX_SpCompress,it);
					treeList.Expand(it,TLIS_EXPANDED);
				}
			}
			InsertItem("Время жизни",IX_EmitterTimeLife,item);
			if (edat->IsBase())
				InsertItem("Сквозь землю", IX_Draw_First, item);
			if (bl) 
			{
				InsertItem("Blending текстур", IX_COLOR_MODE, item); 
				CTreeListItem* it = InsertItem("Размер",IX_Size,item);
				InsertItem("U cкорость", IX_U_VEL, item);
				InsertItem("V cкорость", IX_V_VEL, item);
				InsertItem("Высота", IX_HEIGHT_CL, item);
			}
			treeList.Expand(item,TLIS_EXPANDED);
			item = InsertItem("Генерация",IX_NONE);
			if (!bl) InsertItem("Кол-во частиц",IX_ParticlesCount,item);
 			if (!bl) InsertItem("Непрерывно",IX_Prolonged,item);
			InsertItem("Зациклить",IX_PlayCycled,item);
			treeList.Expand(item,TLIS_EXPANDED);

			if (b||!bl||(!b&&!bl))
			{
				if (!b&&!bl) item = InsertItem("Скорость",IX_NONE);
				else item = InsertItem("Скорость",IX_Velocity);
				if (b)
				{
					InsertItem("Коэф. скорости",IX_Mul,item); 
					InsertItem("Распределн.",IX_VelType,item);
				}
				if (!b&&!bl) 
				{
					InsertItem("Сплайн",IX_SplineEnding,item);
					InsertItem("Движение", IX_SplTypeDirection, item);
				}
				treeList.Expand(item,TLIS_EXPANDED);
			}

			if (!bl||b)
			{
				item = InsertItem("Частица",IX_NONE);
				if (!bl) 
				{
					InsertItem("Размер",IX_Size,item);		
				}
				if (b&&!bz)	 InsertItem("Гравитация",IX_Gravitation,item);	
				if (!bl) 
				{
					InsertItem("Вращение",IX_AngleVel,item);	
					InsertItem("Время жизни", IX_Particle_LifeTime, item);
					InsertItem("Жесткая привязка",IX_Realtive,item);	
					InsertItem("Сносится ветром",IX_NEED_WIND,item);	
					if (boolV(IX_NEED_WIND))
						InsertItem("Инертность",IX_K_WIND,item);	
				}
				CTreeListItem* it = InsertItem("Шлейф",IX_Plume,item);
				if (boolV(IX_Plume))
				{
					InsertItem("Интервал",IX_PlInterval,item);
					InsertItem("Гибкость",IX_PlTracesCount,item);
					if (bz)InsertItem("Cглаживать",IX_PlSmooth, item);
//					InsertItem("Растяжение", IX_PlTimeScale,item);
//					InsertItem("Размер",IX_PlSizeScale,item);
				}

				treeList.Expand(item,TLIS_EXPANDED);
			}
			
			if (!bl||(b&&bFirstPoint)||(!bl&&bFirstPoint))	
			{
				item = InsertItem("Разброс",IX_NONE);
				if (!bl) InsertItem("Размер",IX_DeltaSize,item);
				if (!bl) InsertItem("Время",IX_DeltaLifeTime,item);
				if (b)   InsertItem("Скорость",IX_DeltaVel,item);
				if (!bl) InsertItem("Ротор",IX_AngleChaos,item);
				treeList.Expand(item,TLIS_EXPANDED);
			}

			if(bz)
			{
				item = InsertItem("Поверхность",IX_NONE);
				InsertItem("Высота",IX_SurAddZ,item);
				InsertItem("Угол",IX_SurAngle,item);
				InsertItem("Центр",IX_SurCenter,item);
				InsertItem("Поле",IX_SurUseForceField,item);
				InsertItem("Плоский",IX_SurPlanar,item);
				treeList.Expand(item,TLIS_EXPANDED);
			}
		}
		}else if (edat->IsLighting()&& B[1])
		{
//			item = InsertItem("Эмитер",IX_NONE);
			InsertItem("Тип",IX_EmitterType); 
			InsertItem("Время жизни",IX_EmitterTimeLife);
			InsertItem("Зациклить",IX_PlayCycled);
			InsertItem("Время генерации", IX_Generate_time);
			InsertItem("Начальная ширина", IX_Strip_width_begin);
			InsertItem("Время расширения", IX_strip_width_time);	
			InsertItem("Длина полоски", IX_strip_length);	
			InsertItem("Время затухания", IX_fade_time);	
			InsertItem("Амплитуда", IX_lighting_amplitude);
			InsertItem("Количество", IX_Lighting_count);
		}
	}
	UpdateData();
	if (sel!=-1)
	{
		item = FindItem(sel);
		if (item)
		{
			treeList.SelectItem(item);
			treeList.SetFocusedItem(item);
		}
	}
	show = false;
}
*/
void COptTree::ShowOptEmiter()
{
	if (!ined) return;
	static bool show = false;
	if (show) return;
	show = true;

	int sel=-1;
	if (treeList.GetSelectedItem())
		sel = treeList.GetItemData(treeList.GetSelectedItem());
	treeList.DeleteAllItems();
	CTreeListItem* item;
	SetControlsData();
//	GetDlgItem(IDC_TRIANGLE_COUNT)->ShowWindow(GetDocument()->ActiveEffect()!=NULL);
//	GetDlgItem(IDC_TRIANGLE_SQUARE)->ShowWindow(GetDocument()->ActiveEffect()!=NULL);
	CEmitterData* edat = GetDocument()->ActiveEmitter();
	
	if (B[0])
	{
		item = InsertItem("Cеткa",IX_ShowGrid);
		item = InsertItem("Мир",IX_ShowWorld);
		item = InsertItem("Масштаб",IX_Scale);
		if (theApp.scene.m_pModel)
		{
			InsertItem("Маштаб модели", IX_ModelScale);
			InsertItem("Синхронизировть с эффектом", IX_ScaleEffectWithModel);
		}
		item = InsertItem("P",IX_Rate);
	}
	GetDocument()->m_EnableButForAll = (GetDocument()->m_nCurrentParticlePoint == 0) && edat;
	if(edat)
	{
		if (edat->IsBase()||edat->IsCLight() || edat->IsLight())
		{
		bool b = false;
		bool bl = false;
		bool bz = false;
		bool bFirstPoint = GetDocument()->m_nCurrentParticlePoint == 0;

		bool bX,bY,bZ;
		bX = bY = bZ = false;
		switch(intV(IX_PosType))
		{
		case EMP_BOX:
			bX = true;
			bY = true;
			bZ = true;
			break;
		case EMP_CYLINDER:
			//bX = true;
			//bY = true;
			//bZ = true;
			break;
		case EMP_LINE:
		case EMP_RING:
		case EMP_SPHERE:
			bX = true;
			break;
		case EMP_3DMODEL:
		case EMP_3DMODEL_INSIDE:
		default:
			break;
		}
		switch(edat->Class())
		{
		case EMC_SPLINE:
			b = false;
			break;
		case EMC_INTEGRAL:
			b = true;
			break;
		case EMC_INTEGRAL_Z:
			b = true;
			bz = true;
			break;
		case EMC_COLUMN_LIGHT:
		case EMC_LIGHT:
			b = false;
			bl = true;
			break;
		}
		GetDocument()->m_EnableButForAll = !bl;
		bool b1 = false; 
		if (B[0])
		{
			if(edat->IsBase())
			{
				b1 = edat->num_particle().size()>1;
				InsertItem("Игнорировать ParticleRate",IX_IgnoreParticleRate);
				InsertItem("Показать",IX_ShowFigure);
				InsertItem("Blending",IX_SpriteBlend);
				InsertItem("Распределение",IX_PosType);
				if (intV(IX_PosType)==EMP_OTHER_EMITTER)
					InsertItem("Связь",IX_OtherEmiter);
				else
				{
					if (intV(IX_PosType) == EMP_CYLINDER)
					{
						InsertItem("Конус",IX_Cone);
						InsertItem("Радиус",IX_X);
						if (boolV(IX_Cone))
							InsertItem("Радиус2",IX_Z);
						InsertItem("Высота",IX_Y);
						InsertItem("Ставить на дно",IX_SetBottom);
					}
					if (bX) InsertItem("X",IX_X);
					if (bY) InsertItem("Y",IX_Y);
					if (bZ) InsertItem("Z",IX_Z);
					if (intV(IX_PosType)==EMP_RING)
					{
						InsertItem("Alpha min",IX_AlphaMin);
						InsertItem("Alpha max",IX_AlphaMax);
						InsertItem("Teta min", IX_ThetaMin);
						InsertItem("Teta max", IX_ThetaMax);
					}
					InsertItem("Заполнение", IX_Filling);
					item = InsertItem("Фиксировать", IX_Fix_Pos);
					if (boolV(IX_Fix_Pos))
					{
						InsertItem("dx",IX_Fix_X);
						InsertItem("dy",IX_Fix_Y);
						if (boolV(IX_Filling)||bZ)InsertItem("dz",IX_Fix_Z);
					}
				}
			}
			if (edat->IsBase())
			{
				InsertItem("Случайный кадр",IX_RandomFrame);
				//InsertItem("Количество текстур", IX_TextureCount);
				//int cnt = intV(IX_TextureCount);

				if(!boolV(IX_RandomFrame))
					InsertItem("Текстура", IX_Texture1);
				else
				{
					char buf[255];
					for(int i=0; i<10; i++)
					{
						sprintf(buf,"Текстура %d",i+1);
						InsertItem(buf, IX_Texture1+i);
					}
				}
				InsertItem("Размер по текстуре",IX_SizeByTexture);
			}else
			{
				InsertItem("Текстура", IX_Texture1);
			}
			if (edat->IsCLight())
			{
				InsertItem("Анимированная текстура", IX_Texture2);
			}
		}
		if (B[1])
		{
			if (edat->IsIntZ()&& (intV(IX_PosType)==EMP_3DMODEL || intV(IX_PosType)==EMP_3DMODEL_INSIDE) )
				item = InsertItem("Освещение",IX_UseLight);
			item = InsertItem("Эмитер",IX_NONE);
			if (edat->IsBase())
			{
				InsertItem("Мираж",IX_Mirage,item);
				InsertItem("Размывать",IX_SoftSmoke,item);
			}
			InsertItem("Тип",IX_EmitterType,item); 
			InsertItem("Кол-во точек",IX_PointsCount,item);
			if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION || theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE)
			{
				InsertItem("Позиция X",IX_SetPosX,item);
				InsertItem("Позиция Y",IX_SetPosY,item);
				InsertItem("Позиция Z",IX_SetPosZ,item);
			}
			if (theApp.scene.m_ToolMode == CD3DScene::TOOL_DIRECTION || theApp.scene.m_ToolMode == CD3DScene::TOOL_DIRECTION_BS)
			{
				InsertItem("Угол вокруг Y",IX_AlphaDir,item);
				InsertItem("Угол вокруг Z",IX_BetaDir,item);
			}
			if ((intV(IX_PointsCount)>1 && theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
					||((!b&&!bl) && theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE))
			{
				CTreeListItem* it = InsertItem("Расположение",IX_PointLoc,item);
				if (intV(IX_PointLoc) == SPIRAL)
				{
					InsertItem("Радиус 1",IX_SpRadius1,it);
					InsertItem("Радиус 2",IX_SpRadius2,it);
					InsertItem("Высота 1",IX_SpHeight1,it);
					InsertItem("Высота 2",IX_SpHeight2,it);
					InsertItem("Угол",	  IX_SpAngle,it);
					InsertItem("Cжатие",  IX_SpCompress,it);
					treeList.Expand(it,TLIS_EXPANDED);
				}
			}
			InsertItem("Время жизни",IX_EmitterTimeLife,item);
			if (bl) 
			{
				if (edat->IsCLight())
				{
					InsertItem("Blending",IX_SpriteBlend);
					InsertItem("Blending текстур", IX_COLOR_MODE, item); 
					InsertItem("Плоскость", IX_Plane, item);
					string s = "Ширина";
					if (!boolV(IX_Plane))
					{
					}	
					else
					{
						InsertItem("Лазер", IX_Laser, item);
						if(!boolV(IX_Laser))
							InsertItem("Поворачиваться", IX_Turn, item);
					}
					InsertItem("Ширина 1",IX_Size2,item);
					InsertItem("Ширина 2",IX_Size,item);
					InsertItem("Скорость текстуры по Х", IX_U_VEL, item);
					InsertItem("Скорость текстуры по Y", IX_V_VEL, item);
					InsertItem("Длинна", IX_HEIGHT_CL, item);
				}else
					InsertItem("Размер",IX_Size,item);
			}
//			treeList.Expand(item,TLIS_EXPANDED);
//			item = InsertItem("Генерация",IX_NONE);
			if (!bl) InsertItem("Кол-во частиц",IX_ParticlesCount,item);
 			if (!bl) InsertItem("Непрерывно",IX_Prolonged,item);
			InsertItem("Зациклить",IX_PlayCycled,item);
			if (edat->IsLight()) 
			{
				InsertItem("На объекты",IX_LightOnObjects,item);
				InsertItem("На Землю",IX_LightOnTerrain,item);
				InsertItem("Blending",IX_LightBlending);
			}
			if (edat->IsBase() || edat->IsCLight())
				InsertItem("Сквозь землю", IX_Draw_First, item);
			treeList.Expand(item,TLIS_EXPANDED);
			if (!bl||b)
			{
				if (!bl) 
				{
					InsertItem("Жесткая привязка",IX_Realtive,item);	
					InsertItem("Сносится ветром",IX_NEED_WIND,item);	
					if (boolV(IX_NEED_WIND))
					{
						InsertItem("Инертность min",IX_K_WIND_MIN,item);	
						InsertItem("Инертность max",IX_K_WIND_MAX,item);	
					}
				}
				CTreeListItem* it = InsertItem("Шлейф",IX_Plume,item);
				if (boolV(IX_Plume))
				{
					InsertItem("Интервал",IX_PlInterval,item);
					InsertItem("Гибкость",IX_PlTracesCount,item);
					if (bz)InsertItem("Cглаживать",IX_PlSmooth, item);
				}
			}
			if(bz)
			{
//				item = InsertItem("Поверхность",IX_NONE);
				InsertItem("Высота",IX_SurAddZ,item);
				InsertItem("Угол",IX_SurAngle,item);
				InsertItem("Центр",IX_SurCenter,item);
				InsertItem("Поле",IX_SurUseForceField,item);
				InsertItem("На воде",IX_PlaneOnWater,item);
				//treeList.Expand(item,TLIS_EXPANDED);
			}
			if (edat->IsBase())
			{
				if (!boolV(IX_OrientedByDirection))
					InsertItem("Плоский",IX_SurPlanar,item);
				if (!boolV(IX_SurPlanar))
					InsertItem("Ориентированный",IX_OrientedByDirection,item);
				else
					InsertItem("Поворачиваться",IX_Turn,item);
				InsertItem("Ориент. центр",IX_OrientedToCenter,item);
				if(boolV(IX_OrientedToCenter))
					InsertItem("Ориент. ось",IX_OrientedToAxis,item);
			}

			if (edat->IsInt())
			{
				InsertItem("Центр",IX_SurCenter,item);
				InsertItem("Масштаб",IX_EmitterScale,item);
				InsertItem("Угол",IX_SurAngle,item);
			}
			if(edat->IsBase() && !edat->IsSpl())
			{
				item = InsertItem("Шум скорости",IX_VelNoise);
				if (boolV(IX_VelNoise))
				{
					InsertItem("Из эмиттера",IX_IsVelNoiseOther,item);
					if (boolV(IX_IsVelNoiseOther))
					{
						InsertItem("Эмиттер",IX_VelNoiseOther,item);
					}
					else
						InsertItem("Настройка Шума",IX_VelProp,item);
					treeList.Expand(item,TLIS_EXPANDED);
				}
				item = InsertItem("Шум направления",IX_DirNoise);
				if (boolV(IX_DirNoise))
				{
					InsertItem("Из эмиттера",IX_IsDirNoiseOther,item);
					if (boolV(IX_IsDirNoiseOther))
					{
						InsertItem("Эмиттер",IX_DirNoiseOther,item);
					}
					else
					{
						InsertItem("Настройка Шума",IX_DirProp,item);
						InsertItem("Замещать",IX_NoiseReplace,item);
						InsertItem("Блокировать X",IX_BlockX,item);
						InsertItem("Блокировать Y",IX_BlockY,item);
						InsertItem("Блокировать Z",IX_BlockZ,item);
					}
					treeList.Expand(item,TLIS_EXPANDED);
				}
			}
			if (!bl||(b&&bFirstPoint)||(!bl&&bFirstPoint))	
			{
				item = InsertItem("Разброс",IX_NONE);
				if (!bl) InsertItem("Размер",IX_DeltaSize,item);
				if (!bl) InsertItem("Время",IX_DeltaLifeTime,item);
				if (b)   InsertItem("Скорость",IX_DeltaVel,item);
				if (!bl) InsertItem("Ротор",IX_AngleChaos,item);
				treeList.Expand(item,TLIS_EXPANDED);
			}
			if (!bl||b)
			{
				InsertItem("_____________________________________________________",IX_NONE);
				item = InsertItem("Частица",IX_NONE);
				if (!(!b&&!bl))InsertItem("Скорость",IX_Velocity, item);
				if (b)
					InsertItem("Коэф. скорости",IX_Mul,item); 
				if (!b&&!bl)
				{
					InsertItem("Сплайн",IX_SplineEnding,item);
					InsertItem("Движение", IX_SplTypeDirection, item);
				}
				if (!bl) 
					InsertItem("Размер",IX_Size,item);		
				if (b&&!bz)	 InsertItem("Гравитация",IX_Gravitation,item);	
				if (!bl) InsertItem("Вращение",IX_AngleVel,item);	

				if (b)
					InsertItem("Распределн.",IX_VelType,item);
				if (!bl) 
					InsertItem("Время жизни", IX_Particle_LifeTime, item);
				treeList.Expand(item,TLIS_EXPANDED);
			}
		}
		}else if (edat->IsLighting())
			if ( B[1])
			{
				item = InsertItem("Эмитер",IX_NONE);
				InsertItem("Тип",IX_EmitterType, item); 
				if (theApp.scene.m_ToolMode == CD3DScene::TOOL_POSITION)
				{
					InsertItem("Позиция X",IX_SetPosX,item);
					InsertItem("Позиция Y",IX_SetPosY,item);
					InsertItem("Позиция Z",IX_SetPosZ,item);
				}
				InsertItem("Время жизни",IX_EmitterTimeLife, item);
				InsertItem("Зациклить",IX_PlayCycled, item);
				InsertItem("Время генерации", IX_Generate_time, item);
				InsertItem("Начальная ширина", IX_Strip_width_begin, item);
				InsertItem("Время расширения", IX_strip_width_time, item);	
				InsertItem("Длина полоски", IX_strip_length, item);	
				InsertItem("Время затухания", IX_fade_time, item);	
				InsertItem("Амплитуда", IX_lighting_amplitude, item);
				InsertItem("Количество", IX_Lighting_count, item);
				treeList.Expand(item,TLIS_EXPANDED);
			}
			else if (B[0])
				InsertItem("Текстура", IX_Texture1);
	}
	UpdateData();
	if (sel!=-1)
	{
		item = FindItem(sel);
		if (item)
		{
			treeList.SelectItem(item);
			treeList.SetFocusedItem(item);
		}
	}
	show = false;
}


int RBNum(UINT num,USHORT n=2)
{
	UINT t = num;
	int i=1;
	while(t/=10) i++;
	if (i<n) return num;
	return (num/round(pow(10,i-n)))*pow(10,i-n);
}
			  
void COptTree::SetFps(float fps)
{
	GetDlgItem(IDC_STATIC_FPS)->SetWindowText(CString("fps ")+ToStr(round(fps)));
	GetDlgItem(IDC_TRIANGLE_COUNT)->SetWindowText(CString("N = ")+
		ToStr(RBNum(theApp.scene.GetTriangleCount()/2)));
	GetDlgItem(IDC_TRIANGLE_SQUARE)->SetWindowText(CString("S = ")+
		ToStr(RBNum(round(theApp.scene.GetSquareTriangle()))));
}
bool COptTree::ActEmitIsInt()
{
	if (GetDocument()&&GetDocument()->ActiveEmitter())
		switch(GetDocument()->ActiveEmitter()->Class())
		{
		case EMC_INTEGRAL:
		case EMC_INTEGRAL_Z:
			return true;
		}
	return false;
}
bool COptTree::SelIs(int Ix)
{
	CTreeListItem *item = treeList.GetSelectedItem();
	if (item)
		return treeList.GetItemData(item)==Ix;
	return false;
}
bool COptTree::NeedGraphSize(bool &sel)				{sel=SelIs(IX_Size);		return boolV(IX_Size);}
bool COptTree::NeedGraphGravitation(bool &sel)		{sel=SelIs(IX_Gravitation); return ActEmitIsInt() && boolV(IX_Gravitation);}
bool COptTree::NeedGraphVelocity(bool &sel)			{sel=SelIs(IX_Velocity);	return ActEmitIsInt() && boolV(IX_Velocity);}
bool COptTree::NeedGraphVelMul(bool &sel)			{sel=SelIs(IX_Mul);			return ActEmitIsInt() && boolV(IX_Mul);}
bool COptTree::NeedGraphAngleVel(bool &sel)			{sel=SelIs(IX_AngleVel);	return boolV(IX_AngleVel);}
bool COptTree::NeedGraphParticlesCount(bool &sel)	{sel=SelIs(IX_ParticlesCount); return boolV(IX_ParticlesCount);}
bool COptTree::NeedGraphDeltaVel(bool &sel)			{sel=SelIs(IX_DeltaVel);	return ActEmitIsInt() && boolV(IX_DeltaVel);}
bool COptTree::NeedGraphDeltaSize(bool &sel)		{sel=SelIs(IX_DeltaSize);	return boolV(IX_DeltaSize);}
bool COptTree::NeedGraphDeltaTimeLife(bool &sel)	{sel=SelIs(IX_DeltaLifeTime); return boolV(IX_DeltaLifeTime);}
bool COptTree::NeedGraphEmitterScale(bool &sel)		{sel=SelIs(IX_EmitterScale); return boolV(IX_EmitterScale);}
bool COptTree::NeedGraphU_Vel(bool &sel)			{sel=SelIs(IX_U_VEL);		return boolV(IX_U_VEL);}
bool COptTree::NeedGraphV_Vel(bool &sel)			{sel=SelIs(IX_V_VEL);		return boolV(IX_V_VEL);}
bool COptTree::NeedGraphSize2(bool &sel)			{sel=SelIs(IX_Size2);		return boolV(IX_Size2);}
bool COptTree::NeedGraphHeight(bool &sel)			{sel=SelIs(IX_HEIGHT_CL);	return boolV(IX_HEIGHT_CL);}
void COptTree::OnTimer(UINT nIDEvent)
{
	static int cur_ix=-1;
	static int prev_y=-1;
	static bool changed = false;
	if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		CRect rc;
		CPoint pt;
		GetDlgItem(IDC_CUSTOM1)->GetWindowRect(rc);
		GetCursorPos(&pt);
		if (!rc.PtInRect(pt))
		{
			if (curent_editing!=-1)
				treeList.EndLabelEdit(true);
		}
		if (curent_editing==-1 && rc.PtInRect(pt))
		{
			if (prev_y==-1)
			{
				CRect rci;
	//			GetDlgItem(IDC_CUSTOM1)->GetWindowRect(rc);
				CTreeListItem* itsel= treeList.GetSelectedItem();
				if(itsel)
				{
					itsel->GetState();
					treeList.GetItemRect(itsel,1,&rci,false);
					if (pt.x>rc.left+rci.left && pt.x<rc.left+rci.right)
					{

						cur_ix = treeList.GetItemData(itsel);
						if (cur_ix!=IX_Scale&&GetDataItem(cur_ix)->GetStyle()&STI_EDIT)
							prev_y = pt.y;
						else cur_ix = -1;
		/*				pt.y-=rc.top;
						treeList.GetViewRect(&rc);
						treeList.GetV
						pt.y+=rc.top;
						CTreeListItem* it = treeList.GetFirstVisibleItem();
						while(it)
						{
							treeList.GetItemRect(it,1,&rci,false);
							if (pt.y>rci.top && pt.y<rci.bottom)
							{

								cur_ix = treeList.GetItemData(it);
								if (GetDataItem(cur_ix)->GetStyle()&STI_EDIT)
									prev_y = pt.y;
								break;
							}
							it = treeList.GetNextVisibleItem(it);
						}*/
					}
				}
			}
			else if (prev_y!=pt.y)
			{
				if (!changed && _pDoc->ActiveEmitter())
					_pDoc->History().PushEmitter();
				changed = true;
				float curv = floatV(cur_ix);
				float nv = floatV(cur_ix)+(prev_y-pt.y)*GetDataItem(cur_ix)->Delta();
				Write(cur_ix,nv);
				SaveControlsData(cur_ix, false);
				UpdateData();
				prev_y = pt.y;
			}
		}
	}else if (prev_y != -1)
	{
		if(cur_ix!=-1&&changed)
		{
			int pos = treeList.GetScrollPos(1);
			SaveControlsData(cur_ix);
			treeList.SetScrollPos(1,pos, false);
		}
		prev_y = -1;
		cur_ix = -1;
		changed = false;
	}

//	__super::OnTimer(nIDEvent);
}

//Dummy for wind.cpp (only in EffectTool)
/*#include "..\..\Water\Wind\wind.h"
cMapWind* cMapWind::wind_this = NULL;
const WindNode& cMapWind::GetNode(float x, float y)
{
	static WindNode none;
	return none;
}
*/
