// WinVGView.cpp : implementation of the CWinVGView class
//

#include "stdafx.h"
#include "WinVG.h"
#include "MainFrm.h"
#include "WinVGDoc.h"
#include "WinVGView.h"
#include "TabDialog.h"
#include "DirectoryTree.h"
#include <mmsystem.h>
#include <list>
#include "ModelInfo.h"
#include "ScreenShotSize.h"
#include "..\Render\3dx\Node3dx.h"
#include "DrawGraph.h"
#include "..\Render\inc\IVIsD3D.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

cScene					*gb_Scene=0;		// сцена, может содержать мир, объекты и т.д.
cCamera					*gb_Camera=0;

cBaseGraphObject		*gb_TileMap=NULL;
int						g_CurTime=0;
double					gd_CurTime=0;
CWinVGView*				pView=NULL;

int num_object_linear=1;

cPlane* pPlane=NULL;
QuatF	CameraRotation(0, Vect3f(1,0,0));

bool	g_bPressLighing=false;
bool enable_bound_spheres=false;

#include "fps.h"
#include "winvgview.h"

FPS fps;
void SetUseShadow(bool use);

/////////////////////////////////////////////////////////////////////////////
// CWinVGView

IMPLEMENT_DYNCREATE(CWinVGView, CView)

BEGIN_MESSAGE_MAP(CWinVGView, CView)
	//{{AFX_MSG_MAP(CWinVGView)
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_NUM_OBJECT2, OnViewNumObject2)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinVGView construction/destruction
#include "SunParameters.h"

SunParameters sun_parameters;

void CWinVGView::SetDirectLight(float time)
{
	sun_parameters.Set(time);
}

CWinVGView::CWinVGView()
{
	cScene::DisableSkyCubeMap();
	animationEnable = true;
	b_viewHologram=false;
	pView=this;

	bWireFrame=0;
	bMouseLDown=bMouseRDown=0;

	m_ScaleTime=1;
	Color.set(50,100,50,255);
	m_ControlSwitch=0;
	m_NumberAnimChain=0;
	m_OldScaleTime=0;
	frame_period=2000;
	FramePhase.Set(frame_period,0);
	target_pos.set(1024,1024,0);
	ResetCameraPosition();
	pLightMap=NULL;
	show_basement=false;
	enable_tilemap=false;
	light_on=true;
	pLogicObj=NULL;
	enable_zeroplane=false;
	show_sceleton=false;
	view_normal=false;

	selected_logic_node=-1;
	selected_graph_node=-1;
	pFont=NULL;

	pauseAnimation = false;
	reverseAnimation = false;
	set_fog=false;
}

CWinVGView::~CWinVGView()
{
	gb_FrameWnd->SaveRegistry();
	DoneRenderDevice();
}

/////////////////////////////////////////////////////////////////////////////
// CWinVGView drawing

void CWinVGView::SynhronizeObjAndLogic(cObject3dx *UObj,cObject3dx *ULogic)
{
}

void CWinVGView::OnDraw(CDC* pDC)
{
	if(gb_Scene && !pauseAnimation&&animationEnable)
	{
		double dt;
		int time=xclock();
		dt=((double)time-(double)g_CurTime)*m_ScaleTime;
		if(dt<0 || dt>1000)dt=0;
		double dtime=gd_CurTime+dt*(reverseAnimation?-1:1);

		int int_time=round(dtime);
		
		FramePhase.AddPhase((float)int_time);
		gb_Scene->SetDeltaTime((float)int_time);
		g_CurTime=time;
		gd_CurTime=dtime-int_time;
	}

	Draw();
}
void CWinVGView::UpdateCameraFrustum()
{
	xassert(gb_Camera);
	if (GetDocument()->IsModelCamera())
	{
		xassert(gb_RenderDevice);
		const Vect2f org_size(0.123886f, 0.166526f);//(0.206206f, 0.170271f);
		Vect2f sz1(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
		const float k = 4.0f/3;
		float k1 = sz1.x/sz1.y;
		//float half_width = org_size.x/2;
		//float half_height = (k1/k)*(org_size.y)/2;
		cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
		float fov = pObject->GetFov();
		//static Vect2f focus(1.0f/sqrt(2.f),1.0f/sqrt(2.f));
		Vect2f focus(1.f/(2*tanf(fov*0.5f)),1.f/(2*tanf(fov*0.5f)));
		static Vect2f center(0.5f,0.5f);
		//sRectangle4f rel_pos(- half_width, - half_height, half_width, half_height);
		float half_w = 512.f/sz1.x*0.5f;
		float half_h = 294.f/sz1.y*0.5f;
		sRectangle4f rel_pos(0.5f-half_w,0.5f-half_h, 0.5f+half_w, 0.5f+half_h);
		gb_Camera->SetFrustumPositionAutoCenter(rel_pos,focus.x);
		//gb_Camera->SetFrustum(&center,	NULL, &focus, &Vect2f(10.0f,100000.0f));
	}else
	if(GetDocument()->IsCamera43())
	{
		Vect2f sz1(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
		const float k = 4.0f/3;
		float k1 = sz1.x/sz1.y;
		float hw = 0.5f;
		float hh = 0.5f;
		if(k1>k)
		{	
			hw = (sz1.y/3*4)/sz1.x*0.5f;
		}else
		{
			hh = (sz1.x/4*3)/sz1.y*0.5f;
		}
		cObject3dx* pObject = pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
		float fov = pObject->GetFov();
		sRectangle4f rel_pos(0.5f-hw,0.5f-hh, 0.5f+hw, 0.5f+hh);
		Vect2f focus(1.f/(2*tanf(fov*0.5f)),1.f/(2*tanf(fov*0.5f)));
		gb_Camera->SetFrustumPositionAutoCenter(rel_pos,focus.x);
	}else
	{
		gb_Camera->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		
			&Vect2f(1.f,1.f), &Vect2f(10.0f,3000.0f));

//		gb_Camera->SetFrustrumPositionAutoCenter(sRectangle4f(0,0,1,1),0.01f);
	}
}

void CWinVGView::LightUpdate()
{
	gb_Scene->HideSelfIllumination(!light_on);
	gb_Scene->HideAllObjectLights(!light_on);
}

void CWinVGView::Draw()
{	
	CWinVGDoc* pDoc = GetDocument();
	if(gb_RenderDevice==0) return;
	if(pDoc==NULL || gb_FrameWnd==NULL) return;
	ASSERT_VALID(pDoc);

	if(pDoc->m_pDirectoryTree&& !pDoc->m_pDirectoryTree->FileDoubleClick.IsEmpty())
	{
		CString fname=pDoc->m_pDirectoryTree->FileDoubleClick;
		pDoc->m_pDirectoryTree->FileDoubleClick.Empty();
		LoadObject(fname);
	}
	
	if( !gb_Scene ) return;


	cObject3dx* UObj=GetRoot();
	vector<cObject3dx*>& hobj=pDoc->m_pHierarchyObj->GetAllObj();

	MoveBoundSperes();

	cObject3dx* logic=pDoc->m_pHierarchyObj->GetLogicObj();

	vector<cObject3dx*>::iterator it;
	FOR_EACH(hobj,it)
	{
		cObject3dx* node=*it;
		node->SetPhase(FramePhase.GetPhase());
	}

	if(logic)
	{
		logic->SetPhase(FramePhase.GetPhase());
	}
	if (pDoc->IsModelCamera()||pDoc->IsCamera43())
	{
		cObject3dx* head = UObj;
		if (head)
		{
			int ix_camera = head->FindNode("Camera01");
			int ix_light = head->FindNode("FDirect01");
			head->Update();
			if (ix_light!=-1)
			{
				Se3f me = head->GetNodePosition(ix_light);
				MatXf m(me);
				Mat3f rot(Vect3f(1,0,0), M_PI);
				m.rot()=rot*m.rot();
				m.Invert();
				gb_Scene->SetSunDirection(m.rot().zrow());
			}
			if (ix_camera!=-1)
			{
				MatXf cam(head->GetNodePosition(ix_camera));

				Mat3f rot(Vect3f(1,0,0), M_PI);
				cam.rot()=cam.rot()*rot;
				cam.Invert();

				gb_Camera->SetPosition(cam);
			}
		}
	}
	if (pDoc->NeedTimeLight())
		SetDirectLight(pDoc->time_light());

	if(GetRoot())
	{
		SetDebrisPosition();
		GetRoot()->PutAttr(ATTRUNKOBJ_IGNORE,!debrises.empty());
	}
	
//*
	gb_RenderDevice->Fill(Color.r,Color.g,Color.b,0);
	gb_RenderDevice->BeginScene();

	sColor4f fog_color(0.5f,0.5f,0);
	if(set_fog)
		gb_RenderDevice->SetGlobalFog(fog_color,Vect2f(100,300));
	else
		gb_RenderDevice->SetGlobalFog(fog_color,Vect2f(-1, -2));


	gb_RenderDevice->SetRenderState(RS_FILLMODE,bWireFrame?FILL_WIREFRAME:FILL_SOLID);

	cObject3dx *SelectObj=0;
	
	if(show_basement && UObj)
	{
		gb_RenderDevice->SetDrawTransform(gb_Camera);
		DrawLocalBorder(UObj);
	}
	gb_Scene->Draw(gb_Camera);
	if(enable_zeroplane)
		DrawZeroPlane();

	/*if(show_sceleton && UObj)
	{
		gb_RenderDevice->SetDrawTransform(gb_Camera);
		UObj->DrawLogic(gb_Camera,selected_graph_node);
	}*/

	if(show_sceleton && UObj)
	{
		gb_RenderDevice->SetDrawTransform(gb_Camera);
		vector<cObject3dx*>::iterator it;
		FOR_EACH(pDoc->m_pHierarchyObj->GetAllObj(),it)
		{
			cObject3dx* obj = *it;
			obj->DrawLogic(gb_Camera,selected_graph_node);
		}
	}


	if (logic)
	{
		if(gb_FrameWnd->m_bPressBound)
			logic->DrawLogicBound();
		if(gb_FrameWnd->m_bPressLogic)
			logic->DrawLogic(gb_Camera,selected_logic_node);
	}
	if(show_sceleton || gb_FrameWnd->m_bPressLogic)
		gb_RenderDevice->OutText(0,0,"&FF0000X&00FF00Y&0000FFZ",sColor4f(1,1,1));

	if(gb_FrameWnd->m_bPressBBox&&UObj)
	{
		cObject3dx* UObj=GetRoot();
		sBox6f Box;
		//*
		MatXf Matrix; 
		float Scale=UObj->GetScale();
		Matrix=UObj->GetPosition();
		UObj->GetBoundBox(Box);
		gb_RenderDevice->DrawBound(Matrix,Box.min,Box.max,1,sColor4c(0,0,0,0));
		/*/
		MatXf camerarot=gb_Camera->GetMatrix();
		camerarot.trans()=Vect3f::ID;
		MatXf camerarotinv=camerarot;
		camerarotinv.invert();
		Box=UObj->CalcDynamicBoundBox(camerarot);
		gb_RenderDevice->DrawBound(camerarotinv,Box.min,Box.max,1,sColor4c(0,0,0,0));
		/**/
	}


//	if(SelectObj) SelectObj->SetColor(0,0,&sColor4f(0,0,0,1));
//	if(UObj)UObj->SetColor(0,&sColor4f(1,0,0,1),0);
	DrawSpline();

	if(view_normal && UObj)
	{
		if(false)
		{
			TestObject(UObj);
		}else
		{
			TriangleInfo all;
			UObj->GetTriangleInfo(all,TIF_TRIANGLES|TIF_NORMALS|TIF_POSITIONS|TIF_ZERO_POS|TIF_ONE_SCALE);
			float scale=UObj->GetScale();

			float size=UObj->GetBoundRadius()/(16*scale);
			MatXf mat=UObj->GetPosition();
			mat.rot()*=scale;
			for(int i=0;i<all.positions.size();i++)
			{
				//gb_RenderDevice->DrawPoint(mat*point[i],sColor4c(255,255,0,255));
				Vect3f pos=mat*all.positions[i];
				Vect3f norm=mat.rot()*all.normals[i];
				norm*=size;
				gb_RenderDevice->DrawLine(pos,pos+norm,sColor4c(255,255,0,255));
			}

			gb_RenderDevice->FlushLine3D(false,true);
		}
	}


	gb_RenderDevice->EndScene();

	gb_RenderDevice->Flush();
/**/
	fps.quant();


	static int prev_time=0;
 
	if((xclock()-prev_time>200))
	{
		CString mes;
		float chainTime = 0;
		cObject3dx* p3dx=GetRoot();
		if(p3dx)
		{
			if(p3dx->GetAnimationGroupNumber()>0)
			{
				int ichain=p3dx->GetAnimationGroupChain(0);
				cAnimationChain* chain=p3dx->GetChain(ichain);
				chainTime = chain->time;
			}
		}
		mes.Format("Time: %.2f sec.",chainTime);
		gb_FrameWnd->m_wndStatusBar.SetPaneText(1, mes);
		
		float cur_fps=fps.GetFPS();
		
		mes.Format("FPS: %.2f",cur_fps);
		gb_FrameWnd->m_wndStatusBar.SetPaneText(2, mes);

		float phase=0;
		phase=FramePhase.GetPhase();//node->GetPhase();
		mes.Format("phase: %f",phase);
		gb_FrameWnd->m_wndStatusBar.SetPaneText(3, mes);

		mes.Format("%2.2f mpoly/s",cur_fps*gb_RenderDevice->GetDrawNumberPolygon()/1000000.0f);
		gb_FrameWnd->m_wndStatusBar.SetPaneText(4, mes);

		mes.Format("%i polygon",gb_RenderDevice->GetDrawNumberPolygon());
		gb_FrameWnd->m_wndStatusBar.SetPaneText(5, mes);

		prev_time=xclock();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWinVGView diagnostics

#ifdef _DEBUG
void CWinVGView::AssertValid() const
{
	CView::AssertValid();
}

void CWinVGView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWinVGDoc* CWinVGView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWinVGDoc)));
	return (CWinVGDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWinVGView message handlers

MatXf TransposeMeshScr(cObject3dx *UObject,cCamera *Camera1,Vect3f &dPos,Vect3f &dAngle)
{
	MatXf CameraMatrix=Camera1->GetPosition();
	MatXf LocalMatrix=UObject->GetPosition();
	Mat3f InvCamera;
	InvCamera.invert(CameraMatrix.rot());
	MatXf Rot;
	SetPosition(Rot,Vect3f(0,0,0),Vect3f(-dAngle.z,-dAngle.x,dAngle.y));

	LocalMatrix.set(InvCamera*Rot.rot()*CameraMatrix.rot()*LocalMatrix.rot(),
		LocalMatrix.trans()+dPos.x*CameraMatrix.rot().xrow()
		-dPos.y*CameraMatrix.rot().yrow());
	UObject->SetPosition(LocalMatrix);

	return LocalMatrix;
}

void CWinVGView::ResetCameraPosition()
{
	camera_pos.rot().set(0,1,0,0);
	camera_pos.trans().set(1024,1024,1024);
	SetCameraPosition(0,0,1);
}


void CWinVGView::SetCameraPosition(float du,float dv,float dscale)
{
	float dist=target_pos.distance(camera_pos.trans())*dscale;

	QuatF rot;
	rot.mult(camera_pos.rot(),QuatF(du,Vect3f(0,1,0)));
	camera_pos.rot().mult(rot,QuatF(dv,Vect3f(1,0,0)));

	camera_pos.trans()=target_pos+Mat3f(camera_pos.rot())*Vect3f(0,0,-dist);
	Se3f pos = camera_pos;
	MatXf m(pos);
	m.Invert();
	if(gb_Camera)gb_Camera->SetPosition(m);
}

void CWinVGView::SetEffectDirectory(const char* dir)
{
	string textures=dir+string("Textures");
	gb_VisGeneric->SetEffectLibraryPath(dir,textures.c_str());
}

int CWinVGView::InitRenderDevice(int xScr,int yScr)
{
	CreateIRenderDevice(false);
	SetEffectDirectory(gb_FrameWnd->effect_directory.c_str());

//	gb_VisGeneric->SetDrawNumberPolygon(true);
//	gb_VisGeneric->SetFavoriteLoadDDS(true);

	if(!gb_RenderDevice->Initialize(xScr,yScr,RENDERDEVICE_MODE_WINDOW
		//|RENDERDEVICE_MODE_STENCIL
		,m_hWnd))
	{
		RELEASE(gb_RenderDevice);
		AfxMessageBox("Cannot create device!",MB_OK);
		return 1;
	}
//	gb_VisGeneric->SetStrencilShadow(true);
	
	gb_VisGeneric->SetUseLod(false);

	gb_VisGeneric->SetTilemapDetail(false);
	// создание сцены
	gb_Scene=gb_VisGeneric->CreateScene(); 
	gb_Scene->DisableTileMapVisibleTest();
	// создание камеры
	gb_Camera=gb_Scene->CreateCamera();
	gb_Camera->SetAttr(ATTRCAMERA_PERSPECTIVE); // перспектива
	ResetCameraPosition();

	UpdateCameraFrustum();

	
	//gb_Scene->SetSun(Vect3f(0,-1,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
	gb_Scene->SetSun(Vect3f(0,0,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));

	if(false)
	{
		pPlane=gb_Scene->CreatePlaneObj();
		cTexture* pTexture=gb_VisGeneric->CreateTexture("TEXTURES\\028.tga");

		float sz=2000.0f;
		MatXf mat(MatXf::ID);
		mat.rot().set(Vect3f(0,1,0),0.1f);
		mat.trans().x=-sz*0.5f;
		mat.trans().y=-sz*0.5f;
		mat.trans().z=-300;

		pPlane->SetPosition(mat);
		pPlane->SetSize(Vect3f(sz,sz,0));
		pPlane->SetTexture(0,pTexture);
		pPlane->SetUV(0,0,3,3);
	}

	static bool first_load_registry=true;
	if(first_load_registry)
	{
		gb_FrameWnd->LoadRegistry();
		first_load_registry=false;
	}

//	gb_VisGeneric->SetFarDistanceLOD(15);//to use lod
//	gb_VisGeneric->SetShowType(SHOW_TILEMAP,enable_tilemap);
	InitFont();

	return 0;
}

void CWinVGView::DoneRenderDevice()
{
	DestroyDebrises();
	DeleteSpheres();
	RELEASE(pLogicObj);
	RELEASE(pFont);

	CWinVGDoc* pDoc=GetDocument();
	if(pDoc)
	{
		if(pDoc->m_pHierarchyObj) pDoc->m_pHierarchyObj->ClearRoot();
	}
	RELEASE(pLightMap);
	RELEASE(gb_TileMap);
	RELEASE(pPlane);

	RELEASE(gb_Camera);
	RELEASE(gb_Scene);
	RELEASE(gb_RenderDevice); 
}

static void SetExtension(const char *fnameOld,const char *extension,char *fnameNew)
{
	strcpy(fnameNew,fnameOld);
	for(int l=strlen(fnameNew)-1;l>=0&&fnameNew[l]!='\\';l--)
		if(fnameNew[l]=='.')
			break;
	if(l>=0&&fnameNew[l]=='.') 
		strcpy(&fnameNew[l+1],extension);
	else
		strcat(fnameNew,extension);
}

void CWinVGView::LoadObject(LPCSTR fName)
{
	DestroyDebrises();
	b_viewHologram=false;
//	gb_VisGeneric->GetLibrary()->Free();
	CWinVGDoc* pDoc=GetDocument();
	char fname[256];
	SetExtension(fName,"3dx",fname);
	_strlwr(fname);

//	target_pos=Vect3f(1024,1024,400);
	target_pos=Vect3f(1024,1024,0);
	MatXf pos=GetMatrix(target_pos,Vect3f(0,0,0));
	MatXf p1,p2;

	if(num_object_linear<2)
	{
		cObject3dx *UObj=gb_Scene->CreateObject3dx(fname);
//		UObj->SetAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);
		pDoc->m_pHierarchyObj->SetRoot(UObj,fname);
		if(UObj)
		{
			p1=UObj->GetPosition();
			UObj->SetPosition(pos);
//			UObj->SetVisibilityGroup("main");
		}

		//vector<Vect3f> point;
		//vector<sPolygon> polygon;
		//bool ok=GetAllTriangle3dx(fName,point,polygon);
	}else
	{
		//target_pos=Vect3f(1024,1024,200);
		target_pos=Vect3f(1024,1024,0);
		int num=num_object_linear/2;
		bool first=true;
		for(int x=-num;x<num;x++)
		for(int y=-num;y<num;y++)
		{
			cObject3dx *UObj=gb_Scene->CreateObject3dx(fname);
//			if(x==0)
//				UObj->SetAttr(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE);

			if(first)
				pDoc->m_pHierarchyObj->SetRoot(UObj,fname);
			else
				pDoc->m_pHierarchyObj->AddRoot(UObj);

			first=false;

			if(UObj==NULL)
				break;

			const float mul=500.0f/num_object_linear;
			UObj->SetPosition(GetMatrix(Vect3f(target_pos.x+x*mul,target_pos.y+y*mul,target_pos.z),Vect3f(0,0,0)));
//			UObj->SetColor(NULL,&sColor4f(1,1,1,1.f),NULL);

		}
	}

	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;

	FOR_EACH(obj,it)
	{ 
		cObject3dx* UObj=*it;
		UObj->SetAttr(ATTRUNKOBJ_SHADOW);
	}

	{
		_strlwr(fname);

		cObject3dx* logic=gb_Scene->CreateLogic3dx(fname);
		if(logic)
		{
			pDoc->m_pHierarchyObj->SetLogicObj(logic);
			MatXf pos=GetRoot()->GetPosition();
			logic->SetPosition(pos);
		}
	}


	UpdateObjectLight();

	{
		CString show_name=fName;
		cObject3dx* UObj=GetRoot();
		if(UObj && UObj->GetStatic()->is_old_model)
		{
			show_name+="(Old model)";
		}
		pDoc->SetPathName(show_name,FALSE);
	}

	SetScale(gb_FrameWnd->scale_normal);
	pDoc->m_pHierarchyObj->TreeUpdate();
	UpdateFramePeriod();

	SetSkinColor(gb_FrameWnd->skin_color,gb_FrameWnd->logo_image_name);

	pDoc->m_pHierarchyObj->ReselectLod();
	LightUpdate();
	gb_FrameWnd->ncdlg.ModelChange(GetRoot(),pDoc->m_pHierarchyObj->GetLogicObj());

	BuildBoundSpheres();
}

void CWinVGView::UpdateObjectLight()
{
	if(pDoc)
	{
		vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
		vector<cObject3dx*>::iterator it;

		FOR_EACH(obj,it)
		{ 
			cObject3dx* UObj=*it;
			if(g_bPressLighing) UObj->ClearAttr(ATTRUNKOBJ_NOLIGHT);
			else UObj->SetAttr(ATTRUNKOBJ_NOLIGHT);
		}
	}
}

void CWinVGView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if(cx<=0||cy<=0) return;

	dwScrX=cx;
	dwScrY=cy;
	if(gb_RenderDevice)
	{
		gb_RenderDevice->ChangeSize(cx,cy,RENDERDEVICE_MODE_WINDOW);
		gb_Camera->Update();
		SetScale(gb_FrameWnd->scale_normal);
		UpdateCameraFrustum();
	}else
		InitRenderDevice(cx,cy);
/*
	CWinVGDoc* pDoc=GetDocument();
	CString fname;
	if(pDoc&&pDoc->m_pHierarchyObj)
		fname=pDoc->m_pHierarchyObj->GetFileName();
	DoneRenderDevice();
	InitRenderDevice(dwScrX=cx,dwScrY=cy);
	if(!fname.IsEmpty()) LoadObject(fname);
*/
}

void CWinVGView::SetScale(bool normal)
{
	CWinVGDoc* pDoc=GetDocument();

	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;
	float mul=1;
	FOR_EACH(obj,it)
	{ 
		cObject3dx* UObj=*it;
		float Radius=UObj->GetBoundRadius();

//		if(num_object_linear==1)
//			UObj->SetPosition(&MatXf(Matrix.rot(),Vect3f(1024,1024,0))); 
		if(Radius<=0)
			Radius=1;
		mul=300/(num_object_linear*Radius);
		if(!normal)
			mul=1;
		UObj->SetScale(mul);
	} 

	cObject3dx* logic=pDoc->m_pHierarchyObj->GetLogicObj();
	if(logic)
	{
		logic->SetScale(mul);
	}

}

void CWinVGView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	Vect3f dPos(0,0,0),dAngleGrad(0,0,0);
	float dScale=1;
	CWinVGDoc* pDoc=GetDocument();

	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();

	if(nChar==VK_F2)
	{ 
		FramePhase.Set(0,0); 
	}
	else if(nChar==VK_F3)
	{ 
		FramePhase.Set(0,1); 
	}
	else if(nChar==VK_F4)
	{ 
		FramePhase.Set(frame_period,0); 
	}
	else if(nChar=='D')		dPos.set(  0, 1,  0);
/*	else if(nChar=='1')		
	{
		FramePhase.Set(0,0.1,0.1);
	}
	else if(nChar=='2')		
	{
		FramePhase.Set(0,0.2,0.2);
	}
	else if(nChar=='3')		
	{
		FramePhase.Set(0,0.3,0.3);
	}
	else if(nChar=='4')		
	{
		FramePhase.Set(0,0.4,0.4);
	}
	else if(nChar=='5')		
	{
		FramePhase.Set(0,0.5,0.5);
	}
	else if(nChar=='6')		
	{
		FramePhase.Set(0,0.6,0.6);
	}
	else if(nChar=='7')		
	{
		FramePhase.Set(0,0.7,0.7);
	}
	else if(nChar=='8')		
	{
		FramePhase.Set(0,0.8,0.8);
	}
	else if(nChar=='9')		
	{
		FramePhase.Set(0,0.9,0.9);
	}
	else if(nChar=='0')		
	{
		FramePhase.Set(0,0,0);
	}*/
	else if(nChar=='C')		dPos.set(  0,-1,  0);
	else if(nChar=='F')		dPos.set( 1,  0,  0);
	else if(nChar=='V')		dPos.set(-1,  0,  0);
	else if(nChar=='S')		dScale=1.03f*nRepCnt;
	else if(nChar=='X')		dScale=0.98f*nRepCnt;
	else if(nChar==VK_DOWN)	dAngleGrad.set( 0,  0,  5);
	else if(nChar==VK_UP)	dAngleGrad.set( 0,  0, -5);
	else if(nChar==VK_LEFT)	dAngleGrad.set( 5,  0, 0);
	else if(nChar==VK_RIGHT)dAngleGrad.set(-5,  0, 0);
	else if(nChar==VK_SUBTRACT)	{ m_ScaleTime*=0.9f; m_ScaleTime=max(1e-3,m_ScaleTime); }
	else if(nChar==VK_ADD)		{ m_ScaleTime*=1.1f; m_ScaleTime=min(1e3,m_ScaleTime); }
	else if(nChar=='P')
	{
		if(m_ScaleTime==0) 
			m_ScaleTime=m_OldScaleTime; 
		else 
			m_OldScaleTime=m_ScaleTime, m_ScaleTime=0;
	}
	else if(nChar=='W') bWireFrame=!bWireFrame;
	else if(nChar=='J')
	{
		
	}
	

	ObjectControl(dPos*(float)nRepCnt,dAngleGrad*(float)nRepCnt,dScale);
}
void CWinVGView::SetSunDirection(const Vect3f &dAngle)
{
	if(gb_Scene)
	{
		const float mul=0.01f;
		static float alpha=-M_PI/2,teta=-M_PI/4;
		alpha+=dAngle.x*mul;
		teta+=dAngle.z*mul;

		Vect3f d;
		d.x=cosf(teta)*cosf(alpha);
		d.y=cosf(teta)*sinf(alpha);
		d.z=sinf(teta);
		gb_Scene->SetSun(Vect3f(0,-1,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
		gb_Scene->SetSunDirection(d);
	}
}
void CWinVGView::ObjectControl(Vect3f &dPos,Vect3f &dAngle,float dScale)
{
	CWinVGDoc* pDoc=GetDocument();
	switch(m_ControlSwitch)
	{
		case 0:
			{
				vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();

				float scale=1;
				MatXf pos=MatXf::ID;
				for(int i = 0; i<obj.size(); i++)
				{
					float Scale=1;
					MatXf LocalMatrix=TransposeMeshScr(obj[i],gb_Camera,dPos,dAngle);
					Scale=obj[i]->GetScale();
					Scale*=dScale;
					if(fabsf(dScale-1.0f)>FLT_EPS)
					{
						obj[i]->SetScale(Scale);
					}

					if(i==0)
					{
						pos=obj[i]->GetPosition();
						scale=Scale;
					}
				}
				cObject3dx* logic=pDoc->m_pHierarchyObj->GetLogicObj();
				if(logic)
				{
					logic->SetPosition(pos);
					logic->SetScale(scale);
				}
			}
			break;
		case 1:
			if(gb_Camera)
			{
				const float mul=0.01f;
				float du=dAngle.x*mul,dv=dAngle.z*mul;
				SetCameraPosition(du,dv,1/dScale);
			}
			break;
		case 2:
			SetSunDirection(dAngle);
			break;
		default:
			m_ControlSwitch=0;
	}
}

void CWinVGView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if(bMouseLDown==0) bMouseLDown=1;
	PointMouseLDown=point;
	CView::OnLButtonDown(nFlags, point);
}

void CWinVGView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if(bMouseLDown) bMouseLDown=0;
	CView::OnLButtonUp(nFlags, point);
}

void CWinVGView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if(bMouseRDown==0) bMouseRDown=1;
	PointMouseRDown=point;
	CView::OnRButtonDown(nFlags, point);
}

void CWinVGView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if(bMouseRDown) bMouseRDown=0;
	CView::OnRButtonUp(nFlags, point);
}

void CWinVGView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(bMouseLDown) 
		ObjectControl(Vect3f(0,0,0),Vect3f((float)point.x-PointMouseLDown.x,0,(float)point.y-PointMouseLDown.y),1);
	if(bMouseRDown) 
		ObjectControl(Vect3f((float)point.x-PointMouseRDown.x,(float)point.y-PointMouseRDown.y,0),Vect3f(0,0,0),1);
	PointMouseLDown=point;
	PointMouseRDown=point;

	/*
		Vect2f pos(point.x/(float)gb_RenderDevice->GetSizeX()-0.5f,
				   point.y/(float)gb_RenderDevice->GetSizeY()-0.5f);
		gb_Camera->ConvertorCameraToWorld(&test_pos,&pos);
	*/
	
	CView::OnMouseMove(nFlags, point);
}

BOOL CWinVGView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	ObjectControl(Vect3f(0,0,0),Vect3f(0,0,0),1+zDelta/2000.f);
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


void CWinVGView::OnNumObject() 
{
	num_object_linear=num_object_linear==1?10:1;
	CWinVGDoc* pDoc = GetDocument();
	if(pDoc==NULL) return;

	if(pDoc->m_pHierarchyObj->GetFileName().IsEmpty())return;

	LoadObject(pDoc->m_pHierarchyObj->GetFileName());
	pDoc->m_pHierarchyObj->TreeUpdate();
}

void CWinVGView::OnUpdateNumObject(CCmdUI* pCmdUI) 
{
	//pCmdUI->SetCheck(num_object_linear!=1);
}

void CWinVGView::UpdateIgnore()
{
	CWinVGDoc* pDoc = GetDocument();
	if(pDoc==NULL || gb_FrameWnd==NULL) return;

	vector<cObject3dx*>& hobj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;

	FOR_EACH(hobj,it)
	{
		cObject3dx* UObj=*it;
		if(gb_FrameWnd->m_bPressObject)
			UObj->ClearAttr(ATTRUNKOBJ_IGNORE);
		else
			UObj->SetAttr(ATTRUNKOBJ_IGNORE);

	}

}

void CWinVGView::ModelInfo() 
{
	CWinVGDoc* pDoc = GetDocument();
	if(pDoc==NULL)return;
	cObject3dx* UObj=GetRoot();
	if(UObj==NULL)return;
/*
	CModelInfo dlg(this);	
	dlg.Init(UObj);
	dlg.DoModal();
*/
	MsgModelInfo(UObj);
}

void CWinVGView::OnHologram() 
{
	CWinVGDoc* pDoc = GetDocument();
	if(pDoc==NULL) return;

	b_viewHologram=!b_viewHologram;
	vector<cObject3dx*>& hobj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;

	if(!pLightMap)
		pLightMap=gb_VisGeneric->CreateTexture("TEXTURES\\lightmap.tga");
	
	RandomGenerator rnd;
	FOR_EACH(hobj,it)
	{
		cObject3dx* UObj=*it;
		if(b_viewHologram)
		{
//			UObj->SetTexture(pLightMap,NULL);
			sColor4f a(0,0,0,0);
			UObj->SetColorOld(&a,&SkinColor,&a);
		}else
		{
//			UObj->SetTexture(NULL,NULL);
			sColor4f a(0,0,0,0),d(1,1,1,1);
			UObj->SetColorOld(&a,&d,&a);
		}
	}

}

void CWinVGView::OnUpdateHologram(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(b_viewHologram);
}

void terCalcIntersectionRay(float x,float y,Vect3f& v0,Vect3f& v1)
{
	gb_Camera->ConvertorCameraToWorld(&Vect2f(x,y),&v1);
	if(gb_Camera->GetAttr(ATTRCAMERA_PERSPECTIVE))
	{
		MatXf matrix=gb_Camera->GetPosition();
		v0 = matrix.invXformVect(matrix.trans(),v0);
		v0.negate();
	}else
	{
		v0.x = v1.x;
		v0.y = v1.y;
		v0.z = v1.z + 10.0f;
	}

	Vect3f dir=v1-v0;
	float m=2000.0f/dir.norm();
	dir*=m;
	v1=v0+dir;	
};

void CWinVGView::TestObject(cObject3dx* UObj)
{	
	int dx=gb_RenderDevice->GetSizeX();
	int dy=gb_RenderDevice->GetSizeY();
	cObject3dx* Obj=UObj;

	int x=dx/2,y=dy/2;
	const step=30;//10;
	for(y=0;y<dy;y+=step)
	for(x=0;x<dx;x+=step)
	{
		Vect3f v0,v1;
		terCalcIntersectionRay(x/(float)dx-0.5f,y/(float)dy-0.5f,v0,v1);

		bool b=Obj->Intersect(v0,v1);
		//gb_RenderDevice->DrawPixel(x,y,b?sColor4c(255,255,0,255):sColor4c(0,0,255,255));
		gb_RenderDevice->DrawRectangle(x,y,2,2,b?sColor4c(255,255,0,255):sColor4c(0,0,255,255));
	}

}

class EmptyTerraInterface:public TerraInterface
{
public:
	int SizeX(){return 2048;}
	int SizeY(){return 2048;}
	int GetZ(int x,int y)
	{
		return 1;
	}

	float GetZf(int x,int y)
	{
		return 1;
	}

	MultiRegion* GetRegion(){return NULL;};

	virtual void GetTileColor(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		for(int y = ystart; y < yend; y += step)
		{
			DWORD* tx=(DWORD*)Texture;
			for (int x = xstart; x < xend; x += step)
			{
			/*
				DWORD c=(x*y>>4)&0xFF;
				DWORD color=0xFF000000+c+(c<<8)+(c<<16);
				*tx = color;
			/*/
				*tx=0xFF808080;
			/**/
				tx++;
			}
			Texture += pitch;
		}
		
	}

	virtual void GetTileZ(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		for(int y = ystart,ybuffer=0; y < yend+step; y += step,ybuffer++)
		{
			int * tx=(int*)(Texture+ybuffer*pitch);
			for (int x = xstart; x < xend+step; x += step,tx++)
			{
				*tx=0;
			}
		}
	}

	virtual void GetZMinMax(int tile_x,int tile_y,int tile_dx,int tile_dy,BYTE& out_zmin,BYTE& out_zmax)
	{
		out_zmin=0;
		out_zmax=255;
	}

	virtual void postInit(class cTileMap* tm)
	{
	}
};

void SetUseShadow(bool use)
{
//	dprintf(use?"use":"not use");
	if(use && !gb_TileMap)
		gb_TileMap=gb_Scene->CreateMap(new EmptyTerraInterface);

	if(gb_TileMap)
	{
		gb_TileMap->PutAttr(ATTRCAMERA_IGNORE,!use);
		gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,use?3:0);
	}
}

void CWinVGView::DrawLocalBorder(cObject3dx* root)
{
	int nVertex;
	Vect3f *Vertex;
	int nIndex;
	short *Index;
	MatXf mat=root->GetPosition();
	mat.rot().scale(root->GetScale());

	root->GetLocalBorder(&nVertex,&Vertex,&nIndex,&Index);
	if(nIndex==0)
		return;

	gb_RenderDevice->SetNoMaterial(ALPHA_NONE, mat);

	cVertexBuffer<sVertexXYZD>& buf=*gb_RenderDevice->GetBufferXYZD();

	int ntriangle=0;
	nIndex*=3;

	sVertexXYZD* v=buf.Lock();
	sColor4c c(255,255,0);

	for(int index=0;index<nIndex;index+=3)
	{
		int idx=ntriangle*3;
		for(int i=0;i<3;i++,idx++)
		{
			int cur_index=Index[index+i];
			xassert(cur_index>=0 && cur_index<nVertex);
			v[idx].pos=Vertex[cur_index];
			v[idx].diffuse=c;
		}

		ntriangle++;

		if( ntriangle*3>=buf.GetSize()-4)
		{
			buf.Unlock(ntriangle*3);
			buf.DrawPrimitive(PT_TRIANGLELIST,ntriangle);
			v=buf.Lock();
			ntriangle=0;
		}
	}

	buf.Unlock(ntriangle*3);
	if(ntriangle)
		buf.DrawPrimitive(PT_TRIANGLELIST,ntriangle);

}

void CWinVGView::OnScreenShoot()
{
	CScreenShotSize dlg;

	static screenshot_x=800;
	dlg.x=screenshot_x;
	if(dlg.DoModal()!=IDOK)
		return;
	screenshot_x=dlg.x;
	if(screenshot_x<16)
		screenshot_x=16;

	cObject3dx* UObj=GetRoot();

	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath( UObj?UObj->GetFileName():"screenshot", drive, dir, fname, ext );
	_makepath(path_buffer,drive, dir, fname, ".bmp");

	CFileDialog file(FALSE, "bmp",path_buffer , 0, 
		"*.bmp|*.bmp|All Files (*.*)|*.*||", this );
	if(file.DoModal()!=IDOK)
		return;

	int cx=screenshot_x;
	int cy=round(cx*(dwScrY/(float)dwScrX));

	if(gb_RenderDevice->ChangeSize(cx,cy,RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_ALPHA|RENDERDEVICE_MODE_RETURNERROR))
	{
		gb_Camera->Update();
		Draw();
		if(!gb_RenderDevice->SetScreenShot(file.GetPathName()))
		{
			AfxMessageBox("Cannot save screenshoot");
		}
	}else
	{
		AfxMessageBox("Cannot initialize grapics to save screen shoot");
	}

	gb_RenderDevice->ChangeSize(dwScrX,dwScrY,RENDERDEVICE_MODE_WINDOW);
	gb_Camera->Update();
	
}

void CWinVGView::UpdateFramePeriod()
{
	cObject3dx* p3dx=GetRoot();
	if(!p3dx)
		return;
	if(p3dx->GetAnimationGroupNumber()<=0)
		return;
	int ichain=p3dx->GetAnimationGroupChain(0);
	cAnimationChain* chain=p3dx->GetChain(ichain);

	frame_period=chain->time*1000;
	FramePhase.Set(frame_period,0);
}

void CWinVGView::SetSkinColor(sColor4f color,const char* logo_image_name)
{
	if(!pDoc->m_pHierarchyObj)
		return;
	if(logo_image_name && logo_image_name[0]==0)
		logo_image_name=NULL;
	SkinColor=color;
	vector<cObject3dx*>& obj=pDoc->m_pHierarchyObj->GetAllObj();
	vector<cObject3dx*>::iterator it;
	FOR_EACH(obj,it)
	{
		(*it)->SetSkinColor(SkinColor,logo_image_name);
	}
}

sColor4c Color4(int c)
{
	sColor4c in_color;
	switch(c)
	{
	case 0:
		in_color.set(255,255,255);
		break;
	case 1:
		in_color.set(255,255,0);
		break;
	case 2:
		in_color.set(0,0,255);
		break;
	case 3:
		in_color.set(128,128,128);
		break;
	case 4:
		in_color.set(0,0,128);
		break;
	case 5:
		in_color.set(128,0,0);
		break;
	case 6:
		in_color.set(0,0,0);
		break;
	default:
		assert(0);
	}
	return in_color;
}

class RotF:public DrawFunctor
{
	int index;
	int component;
	sColor4c in_color;
public:
	Interpolator3dx<4>* rot;

	float get(float t,sColor4c* color)
	{
		float out[4];
		index=rot->FindIndexRelative(t,index);
		rot->Interpolate(t,out,index);
		*color=in_color;
		return out[component];
	}

	RotF():index(0),component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};

class MoveF:public DrawFunctor
{
	int index;
	int component;
	sColor4c in_color;
public:
	Interpolator3dx<3>* rot;

	float get(float t,sColor4c* color)
	{
		float out[4];
		index=rot->FindIndexRelative(t,index);
		rot->Interpolate(t,out,index);
		*color=in_color;
		return out[component];
	}

	MoveF():index(0),component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};

double function[][4]=
{
/*
  {-0.271266, 0.075548, -0.127508, 0.951025,},//0
  {-0.303998, 0.068700, -0.122124, 0.942312,},//1
  {-0.333932, 0.058902, -0.114608, 0.933748,},//2
  {-0.358765, 0.048942, -0.106406, 0.926051,},//3
  {-0.376420, 0.041378, -0.099578, 0.920152,},//4
  {-0.384970, 0.038659, -0.096470, 0.917059,},//5
  {-0.382187, 0.041728, -0.098486, 0.917874,},//6
  {-0.369436, 0.048907, -0.104269, 0.922092,},//7
  {-0.350137, 0.058732, -0.111831, 0.928143,},//8
  {-0.327830, 0.069652, -0.119432, 0.934565,},//9
  {-0.306284, 0.079919, -0.125874, 0.940191,},//10
  {-0.281729, 0.091281, -0.132390, 0.945923,},//11
  {-0.251477, 0.104989, -0.139091, 0.952045,},//12
  {-0.221312, 0.118283, -0.144270, 0.957192,},//13
  {-0.197278, 0.128268, -0.147260, 0.960699,},//14
  {-0.185518, 0.131986, -0.148099, 0.962408,},//15
  {-0.201994, 0.130687, -0.148008, 0.959277,},//16
  {-0.242489, 0.126306, -0.146492, 0.950677,},//17
  {-0.282999, 0.117072, -0.141689, 0.941345,},//18
  {-0.300267, 0.101061, -0.134267, 0.938935,},//19
//  {-0.271267, 0.075548, -0.127505, 0.951025,},//20
*/
  {0.138233, -0.877776, 0.452722, 0.073783,},
  {0.170297, -0.876553, 0.448786, 0.035289,},
  {0.194286, -0.892868, 0.405635, -0.022362,},
  {0.207283, -0.911106, 0.345889, -0.085326,},
  {0.212016, -0.921264, 0.294803, -0.139331,},
  {0.186672, -0.914539, 0.314075, -0.173576,},
  {0.160907, -0.910050, 0.326659, -0.198020,},
  {0.141864, -0.908573, 0.336925, -0.202116,},
  {0.135418, -0.907317, 0.361789, -0.165971,},
  {0.131295, -0.908534, 0.374863, -0.129634,},
  {0.124826, -0.908458, 0.387635, -0.094135,},
  {0.090663, -0.911763, 0.399213, -0.033115,},
  {0.046053, -0.910208, 0.410714, 0.026709,},
  {0.008099, -0.903963, 0.420601, 0.076678,},
  {0.009403, -0.908279, 0.409826, 0.083569,},
  {0.024464, -0.903622, 0.419143, 0.084783,},
  {0.047625, -0.887501, 0.452007, 0.075914,},
  {0.075099, -0.862217, 0.496141, 0.069183,},
  {0.103557, -0.828589, 0.546409, 0.064438,},
  {0.127017, -0.822995, 0.549886, 0.064586,},
//  {0.138236, -0.877775, 0.452722, 0.073785,},
};

int function_size=sizeof(function)/sizeof(function[0]);

class RotFf:public DrawFunctor
{
	int component;
	sColor4c in_color;
public:

	float get(float t,sColor4c* color)
	{
		int i=round(t*function_size)%function_size;
		//*color=in_color;
		*color=Color4(i%7);
		return function[i][component];
	}

	RotFf():component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};


void CWinVGView::DrawSpline()
{
	cObject3dx* p3dx=GetRoot();
	if(!p3dx)
		return;
/*
	const char* node_name="turret";
	int inode=p3dx->FindNode(node_name);
	if(inode<0)
		return;
/*/
	int inode=selected_graph_node;
	if(inode<0 || inode>=p3dx->GetNodeNum())
		return;
	const char* node_name=p3dx->GetNodeName(inode);
/**/
	gb_RenderDevice->OutText(0,20,node_name,sColor4f(1,1,1));
	cStaticNode& node=p3dx->GetStatic()->nodes[inode];
	int ichain=p3dx->GetAnimationGroupChain(0);
	cStaticNodeChain& chain=node.chains[ichain];

	DrawGraph graph;
	//graph.SetArgumentRange(-0.5f,+0.5f,-1.2f,+1.2f);
	graph.SetCycleHalfShift();

	if(GetKeyState(VK_LSHIFT)&0x8000)
	{
		graph.SetArgumentRange(0,1,-1.2f,+1.2f);

		RotFf functor;

		sColor4c c;
		float x=functor.get(0.0f,&c);

		for(int i=0;i<4;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}else
	if(true)
	{
		Interpolator3dx<4>& rot=chain.rotation;
		graph.SetArgumentRange(0,1,-1.2f,+1.2f);

		RotF functor;
		functor.rot=&rot;

		sColor4c c;
		float x0=functor.get(0.0f,&c);
		float x1=functor.get(1.0f,&c);

		for(int i=0;i<4;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}else
	{
		graph.SetArgumentRange(0,1,-2.0e2f,+2.0e2f);
		Interpolator3dx<3>& pos=chain.position;

		MoveF functor;
		functor.rot=&pos;

		sColor4c c;
		functor.get(0.69f,&c);

		for(int i=0;i<3;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}

	graph.DrawXPosition(FramePhase.GetPhase(),sColor4c(255,255,255,255));
}

void CWinVGView::DrawZeroPlane()
{
	cObject3dx* p3dx=GetRoot();
	if(!p3dx)
		return;
	MatXf mat=p3dx->GetPosition();
  	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	cQuadBuffer<sVertexXYZDT1>* quad=gb_RenderDevice->GetQuadBufferXYZDT1();
	DWORD old_cullmode=gb_RenderDevice->GetRenderState(RS_CULLMODE);
	gb_RenderDevice->SetRenderState(RS_CULLMODE,1);
	gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, mat);
	quad->BeginDraw(mat);
	int dx=1000,dy=1000;
	sColor4c diffuse(0,255,128,128);
	sVertexXYZDT1 *v=quad->Get();
	v[0].pos.x=-dx; v[0].pos.y=-dy; v[0].pos.z=0; v[0].u1()=0; v[0].v1()=0;
	v[1].pos.x=-dx; v[1].pos.y=dy; v[1].pos.z=0; v[1].u1()=0; v[1].v1()=1;
	v[2].pos.x=dx; v[2].pos.y=-dy; v[2].pos.z=0; v[2].u1()=1; v[2].v1()=0;
	v[3].pos.x=dx; v[3].pos.y=dy; v[3].pos.z=0; v[3].u1()=1; v[3].v1()=1;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=diffuse;
	quad->EndDraw();
	gb_RenderDevice->SetRenderState(RS_CULLMODE,old_cullmode);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
}

void CWinVGView::InitFont()
{
	RELEASE(pFont);
	HRSRC hrsrc=FindResource(NULL,MAKEINTRESOURCE(IDR_FONTS1),"FONTS");

	HGLOBAL hglobal=LoadResource(NULL,hrsrc);
	LPVOID pdata=LockResource(hglobal);
	DWORD size=SizeofResource(NULL,hrsrc);

	pFont=gb_VisGeneric->CreateFontMem(pdata,size,-20);

	gb_RenderDevice->SetDefaultFont(pFont);
}

void CWinVGView::OnPause()
{
	pauseAnimation = !pauseAnimation;
}
void CWinVGView::OnUpdatePause(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pauseAnimation);
}

void CWinVGView::OnReverse()
{
	reverseAnimation = !reverseAnimation ;
}
void CWinVGView::OnUpdateReverse(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(reverseAnimation );
}

void CWinVGView::OnViewNumObject2()
{
	num_object_linear=num_object_linear==1?2:1;
	CWinVGDoc* pDoc = GetDocument();
	if(pDoc==NULL) return;

	if(pDoc->m_pHierarchyObj->GetFileName().IsEmpty())return;

	LoadObject(pDoc->m_pHierarchyObj->GetFileName());
	pDoc->m_pHierarchyObj->TreeUpdate();
}

void CWinVGView::BuildBoundSpheres()
{
	DeleteSpheres();
	if(!enable_bound_spheres)
		return;
	cObject3dx* UObj=GetRoot();
	if(!UObj)
		return;
	cStatic3dx* pStatic=UObj->GetStatic();
	bound_spheres.resize(pStatic->bound_spheres.size());
	for(int i=0;i<bound_spheres.size();i++)
	{
		cObject3dx*& p=bound_spheres[i];
		p=gb_Scene->CreateObject3dx("E:\\3dsmax8\\meshes\\Size1Sphere.3DX");
		p->SetPosition(MatXf::ID);
		sColor4f c(1,0,0,1);
		p->SetColorMaterial(&c,&c,&c);
		p->SetAttr(ATTRUNKOBJ_NOLIGHT);
	}
}

cObject3dx* CWinVGView::GetRoot()
{
	return pDoc->m_pHierarchyObj?pDoc->m_pHierarchyObj->GetRoot():NULL;
}

void CWinVGView::MoveBoundSperes()
{
	if(!enable_bound_spheres)
		return;
	cObject3dx* UObj=GetRoot();
	if(!UObj)
		return;
	cStatic3dx* pStatic=UObj->GetStatic();
	for(int i=0;i<bound_spheres.size();i++)
	{
		cStatic3dx::BoundSphere& b=pStatic->bound_spheres[i];

		MatXf pos;
		pos.set(UObj->GetNodePosition(b.node_index));
		//pos.trans()+=(pos.rot()*b.position)*UObj->GetScale();
		pos.trans()=pos*(b.position*UObj->GetScale());//То же что и предыдущая сторочка, но другими словами
		float scale=UObj->GetScale()*b.radius;
		bound_spheres[i]->SetPosition(pos);
		bound_spheres[i]->SetScale(scale);
	}
}

void CWinVGView::DeleteSpheres()
{
	vector<cObject3dx*>::iterator it;
	FOR_EACH(bound_spheres,it)
		(*it)->Release();

	bound_spheres.clear();
}

void CWinVGView::DestroyDebrises()
{
	for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it)
		(*it)->Release();
	debrises.clear();
}

void CWinVGView::OnShowDebrises()
{
	if(debrises.empty())
	{
		if(!GetRoot())
			return;
		gb_Scene->CreateDebrisesDetached(GetRoot(),debrises);
		for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it)
			gb_Scene->AttachObj(*it);
	}else
	{
		DestroyDebrises();
	}
}

void CWinVGView::SetDebrisPosition()
{
	if(debrises.empty())
		return;
	Mats pos=GetRoot()->GetPositionMats();
	for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it)
	{
		Mats m;
		m.mult(pos,(*it)->GetStatic()->debrisPos);
		(*it)->SetPosition(m.se());
		(*it)->SetScale(m.scale());
	}
}

void CWinVGView::OnUpdateShowDebrises(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(debrises.size()!=0);
}

class AttribEditorInterface & __cdecl attribEditorInterface() {return *(AttribEditorInterface*)0;} 