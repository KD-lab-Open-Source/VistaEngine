// D3DScene.cpp: implementation of the CD3DScene class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "effecttool.h"
#include "D3DScene.h"
#include "DrawInfo.h"
#include "EffectToolDoc.h"
#include "DrawGraph.h"

#include "..\Terra\terra.h"
#include "..\Render\inc\TerraInterface.inl"
#include "..\Render\src\postEffects.h"
#include "MainFrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

bool IsKey(int key) //VK_CONTROL
{
	return GetAsyncKeyState(key) & 0x8000;
}


const float M_PI_2 = 2*M_PI;
CD3DScene::ToolMode CD3DScene::m_ToolMode=CD3DScene::TOOL_NONE;
bool CD3DScene::bPause=false;
bool CD3DScene::bShowEmitterBox=false;
bool CD3DScene::bShowWorld=false;
bool CD3DScene::bShowGrid=true;


//////////////////////////////////////////////////////////////////////
//

CD3DCamera::CD3DCamera(float psi, float theta, float r)
{
	m_cameraPsi = psi;
	m_cameraTheta = theta;
	m_cameraR = r;
	m_cameraFocus.set(0, 0, 0);

	m_pCamera = 0;
	m_bVisible = false;
	m_bLocked = false;
	m_bMain = false;
}
CD3DCamera::~CD3DCamera()
{
	ASSERT(m_pCamera == 0);
}

void CD3DCamera::LoadCameraSettings()
{
	CString temp;

	temp = theApp.GetProfileString("Camera", "Psi", "");
	if(!temp.IsEmpty())
		m_cameraPsi = atof(temp);

	temp = theApp.GetProfileString("Camera", "Theta", "");
	if(!temp.IsEmpty())
		m_cameraTheta = atof(temp);

	temp = theApp.GetProfileString("Camera", "R", "");
	if(!temp.IsEmpty())
		m_cameraR = atof(temp);

	temp = theApp.GetProfileString("Camera", "Fx", "");
	float fx = atof(temp);
	temp = theApp.GetProfileString("Camera", "Fy", "");
	float fy = atof(temp);
	temp = theApp.GetProfileString("Camera", "Fz", "");
	float fz = atof(temp);

	m_cameraFocus.set(fx, fy, fz);
}

void CD3DCamera::SaveCameraSettings()
{
	CString temp;

	temp.Format("%f", m_cameraPsi);
	theApp.WriteProfileString("Camera", "Psi", temp);

	temp.Format("%f", m_cameraTheta);
	theApp.WriteProfileString("Camera", "Theta", temp);

	temp.Format("%f", m_cameraR);
	theApp.WriteProfileString("Camera", "R", temp);

	temp.Format("%f", m_cameraFocus.x);
	theApp.WriteProfileString("Camera", "Fx", temp);
	temp.Format("%f", m_cameraFocus.y);
	theApp.WriteProfileString("Camera", "Fy", temp);
	temp.Format("%f", m_cameraFocus.z);
	theApp.WriteProfileString("Camera", "Fz", temp);
}

const float bNonPerspFocus = 1.f/500.f;

void CD3DCamera::Resize(const CRect& rcWindow, const CRect& rcCamera,bool bPerspective)
{
	if(bPerspective)
		m_pCamera->SetAttr(ATTRCAMERA_PERSPECTIVE);
	ASSERT(m_pCamera);
	Vect2f vSize(float(rcCamera.Width())/rcWindow.Width(), float(rcCamera.Height())/rcWindow.Height());
	Vect2f vLT(float(rcCamera.left)/rcWindow.Width(), float(rcCamera.top)/rcWindow.Height());
	Vect2f vRB(float(rcCamera.right)/rcWindow.Width(), float(rcCamera.bottom)/rcWindow.Height());
	Vect2f vC = (vLT + vRB)/2;

	m_pCamera->SetFrustum(
		&vC,          // центр камеры
		&sRectangle4f(-vSize.x/2,-vSize.y/2,vSize.x/2,vSize.y/2),// видимая область камеры
		bPerspective ? &Vect2f(1.f,1.f) : &Vect2f(bNonPerspFocus, bNonPerspFocus), // фокус камеры
		&Vect2f(10.0f,5000.0f)); // ближайший и дальний z-плоскости отсечения

	m_rcCamera = rcCamera;
}
void CD3DCamera::Init(cScene* pScene, cInterfaceRenderDevice* pRenderDevice, const CRect& rcWindow, const CRect& rcCamera, CAMERA_PLANE plane, bool bPerspective)
{
	m_pCamera = pScene->CreateCamera();

	if(bPerspective)
		m_pCamera->SetAttr(ATTRCAMERA_PERSPECTIVE);

	Vect2f vSize(float(rcCamera.Width())/rcWindow.Width(), float(rcCamera.Height())/rcWindow.Height());
	Vect2f vLT(float(rcCamera.left)/rcWindow.Width(), float(rcCamera.top)/rcWindow.Height());
	Vect2f vRB(float(rcCamera.right)/rcWindow.Width(), float(rcCamera.bottom)/rcWindow.Height());
	Vect2f vC = (vLT + vRB)/2;

	m_pCamera->SetFrustum(
		&vC,          // центр камеры
		&sRectangle4f(-vSize.x/2,-vSize.y/2,vSize.x/2,vSize.y/2),// видимая область камеры
		bPerspective ? &Vect2f(1.f,1.f) : &Vect2f(bNonPerspFocus, bNonPerspFocus), // фокус камеры
		&Vect2f(10.0f,5000.0f)); // ближайший и дальний z-плоскости отсечения

	m_rcCamera = rcCamera;
	m_plane = plane;
}
void CD3DCamera::Done()
{
	if(m_bMain)
		SaveCameraSettings();
	RELEASE(m_pCamera);
}


void CD3DCamera::ViewPos(const Vect3f& v)
{
	switch(m_plane)
	{
	case CAMERA_PLANE_X:
	case CAMERA_PLANE_Y:
	{
		m_cameraFocus.x = - v.x;
		m_cameraFocus.y = - v.y;
		m_cameraFocus.z = - v.z;
		Vect2f Focus(0.002f,0.002f);
		m_pCamera->SetFrustum(0, 0, &Focus, 0);
	}
		break;
	case CAMERA_PLANE_Z:
		m_cameraPsi = 0;
		m_cameraTheta = 0;
		m_cameraR = v.z+800;
		m_cameraFocus.x = - v.x;
		m_cameraFocus.y = - v.y;
		break;
	}
}

void CD3DCamera::Rotate(float dPsi, float dTheta)
{
	if(!m_bLocked)
	{
		m_cameraPsi += dPsi;
		m_cameraTheta += dTheta;
	}
}
void CD3DCamera::Move(float dx, float dy, float dz)
{
	//if(!m_bLocked)
	{
		Vect3f camera_angle(M_PI + m_cameraTheta, 0, M_PI/2 - m_cameraPsi);

		Vect3f v(dx, dy, dz);
		v *= Mat3f(-camera_angle.z, Z_AXIS);

		m_cameraFocus += v;
	}
}
void CD3DCamera::Zoom(float delta)
{
	if(m_pCamera->GetAttr() & ATTRCAMERA_PERSPECTIVE)
	{
		if(delta > 0)
			m_cameraR *= 1.1f;
		else
			m_cameraR /= 1.1f;
	}
	else
	{
		Vect2f Focus;

		m_pCamera->GetFrustum(0, 0, &Focus, 0);

		if(delta > 0)
		{
			Focus.x *= 1.1f;
			Focus.y *= 1.1f;
		}
		else
		{
			Focus.x /= 1.1f;
			Focus.y /= 1.1f;
		}
		m_pCamera->SetFrustum(0, 0, &Focus, 0);
	}
}

void CD3DCamera::Quant()
{
	if(!m_bVisible)
		return;

	if(m_cameraPsi < 0)
		m_cameraPsi += M_PI_2;
	if(m_cameraPsi > M_PI_2)
		m_cameraPsi -= M_PI_2;

	Vect3f r;
	r.setSpherical(m_cameraPsi, M_PI+m_cameraTheta, m_cameraR);
	Vect3f camera_pos = m_cameraFocus + r; 
	Vect3f camera_angle(M_PI+m_cameraTheta, 0, M_PI/2 - m_cameraPsi);

	MatXf mat = MatXf::ID;
	mat.rot() = Mat3f(camera_angle.x, X_AXIS)*Mat3f(camera_angle.y, Y_AXIS)*Mat3f(camera_angle.z, Z_AXIS);
	::Translate(mat, camera_pos);

	m_pCamera->SetPosition(mat);
}

void CD3DCamera::GetBilboardMatrix(Mat3f& mat)
{
	mat.xrow() = m_pCamera->GetWorldI();
	mat.yrow() = m_pCamera->GetWorldJ();
	mat.zrow() = m_pCamera->GetWorldK();
	mat.xpose();
}

inline void DrawRect(cInterfaceRenderDevice* pRenderDevice, int x1, int y1, int x2, int y2, BYTE r, BYTE g, BYTE b)
{
	sColor4c c(r, g, b, 255);
	pRenderDevice->DrawLine(x1, y1, x2, y1, c);
	pRenderDevice->DrawLine(x2, y1, x2, y2, c);
	pRenderDevice->DrawLine(x2, y2, x1, y2, c);
	pRenderDevice->DrawLine(x1, y2, x1, y1, c);
}
void CD3DCamera::DrawCameraRect(cInterfaceRenderDevice* pRenderDevice, bool bActive)
{
/*
	DrawRect(pRenderDevice, m_rcCamera.left, m_rcCamera.top, m_rcCamera.right, m_rcCamera.bottom, 255, 255, 255);
	if(bActive)
		DrawRect(pRenderDevice, m_rcCamera.left+1, m_rcCamera.top+1, m_rcCamera.right-1, m_rcCamera.bottom-1, 255, 255, 0);
*/
	if(bActive)
		DrawRect(pRenderDevice, m_rcCamera.left, m_rcCamera.top, m_rcCamera.right-1, m_rcCamera.bottom-1, 255, 255, 0);
}

bool CD3DCamera::HitTest(const CPoint& pt)
{
	return m_bVisible && m_rcCamera.PtInRect(pt);
}

inline Vect3f RayPlaneIntr(const Vect3f& vRayOrg, const Vect3f& vRayDir, const Vect3f& Pn, float Pd)
{
	float d = Pn.dot(vRayDir);
	if(d == 0) //no intersection
		return Vect3f(0, 0, 0);

	float t = -(Pn.dot(vRayOrg) + Pd)/d;
	return vRayOrg + vRayDir*t;
}

void CD3DCamera::Camera2World(Vect3f& v_to, float x, float y, Vect3f* pvObjDist)
{
	Vect3f vRayOrg, vRayDir;

	m_pCamera->GetWorldRay(Vect2f(x, y), vRayOrg, vRayDir);

	float d = 0;
	if(pvObjDist) //будет работать только для CAMERA_PLANE_Z
		d = pvObjDist->z;
	
	switch(m_plane)
	{
	case CAMERA_PLANE_Z:
		v_to = RayPlaneIntr(vRayOrg, vRayDir, Vect3f(0, 0, 1), -d);
		break;

	case CAMERA_PLANE_X:
		v_to = RayPlaneIntr(vRayOrg, vRayDir, Vect3f(1, 0, 0), 0);
		break;

	case CAMERA_PLANE_Y:
		v_to = RayPlaneIntr(vRayOrg, vRayDir, Vect3f(0, 1, 0), 0);
		break;
	}
}

void CD3DCamera::GetCameraRay(Vect3f& vRay, Vect3f& vOrg, float x, float y)
{
	m_pCamera->GetWorldRay(Vect2f(x, y), vOrg, vRay);
}

//////////////////////////////////////////////////////////////////////
// 

CD3DScene::CD3DScene()
{
	m_pRenderDevice = 0;
	gb_VisGeneric = 0;
	m_pScene = 0;
	m_pLight = 0;
	m_pEffectObj = 0;
	m_pModel = 0;
	m_pModelLogic = 0;

	m_pActiveCamera = 0;

	m_pWayPointActive = 0;

	backgroundColor.set(50,100,50);

	m_pDoc = 0;
	modelPhase = 0;
	m_particle_rate = 1.0;
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
	enableOverDraw = false;
	preset = false;
	m_pPEManager = NULL;
	gb_VisGeneric->SetFloatZBufferType(2);

}

CD3DScene::~CD3DScene()
{
	
}

void CD3DScene::DoneRenderDevice()
{
	CameraListType::iterator it;
	if (m_cameras.size()>=3)
		m_cameras[2]->SaveCameraSettings();
	FOR_EACH(m_cameras, it)
		(*it)->Done();
	m_cameras.clear();
	delete m_pPEManager;
	RELEASE(m_pModel);
	RELEASE(m_pModelLogic);
	RELEASE(m_pEffectObj);	
	RELEASE(m_pLight);
	RELEASE(terMapPoint);
	RELEASE(m_pScene);
	RELEASE(m_pRenderDevice);
}

void UpdateRegionMap(int x1,int y1,int x2,int y2)
{
//	if(terMapPoint) terMapPoint->UpdateMap(Vect2i(x1,y1),Vect2i(x2,y2));
}

bool CD3DScene::LoadWorld(CString& world)
{
	gb_VisGeneric->SetTilemapDetail(false);

	int len = world.GetLength();
	int br = world.ReverseFind('\\');
	CString path = world.Left(br);
	CString name = world.Right(len - br-1);

	vMap.prepare(path);
	if (vMap.load(name))
	{
		vMap.WorldRender();
		vMap.renderQuant();
		RELEASE(terMapPoint);
		terMapPoint = m_pScene->CreateMap(new StandartTerraInterface,1);
		return true;
	}
	world = "";
	return false;
}

void CD3DScene::Resize(CRect& rcWnd)
{
	ASSERT(m_cameras.size()==4);
	CRect rcCamera;
	if(!m_pRenderDevice->ChangeSize(rcWnd.Width(), rcWnd.Height(), RENDERDEVICE_MODE_WINDOW))
		return;
	for(int i=0;i<4;++i)
	{
		if (!m_bCameraMode&&m_cameras[i]==m_pActiveCamera)
			m_cameras[i]->Resize(rcWnd, rcWnd,i==0||i==2);
		else
		{
			switch(i)
			{
			case 0:	rcCamera.SetRect(0, 0, rcWnd.Width()/2, rcWnd.Height()/2); break;
			case 1: rcCamera.SetRect(rcWnd.Width()/2, 0, rcWnd.right, rcWnd.Height()/2); break;
			case 2: rcCamera.SetRect(rcWnd.Width()/2, rcWnd.Height()/2, rcWnd.right, rcWnd.bottom); break;
			case 3: rcCamera.SetRect(0, rcWnd.Height()/2, rcWnd.Width()/2, rcWnd.bottom); break;
			}
			m_cameras[i]->Resize(rcWnd, rcCamera,i==0||i==2);
		}
	}
/*
	if(!m_pRenderDevice->ChangeSize(rcWnd.Width(), rcWnd.Height(), RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_RGB16))
		return;
	if (m_pActiveCamera!=m_cameras[0])
		rcCamera.SetRect(0, 0, rcWnd.Width()/2, rcWnd.Height()/2);
	else rcCamera = rcWnd;
	m_cameras[0]->Resize(rcWnd, rcCamera,true);

	if (m_pActiveCamera!=m_cameras[1])
		rcCamera.SetRect(rcWnd.Width()/2, 0, rcWnd.right, rcWnd.Height()/2);
	else rcCamera = rcWnd;
	m_cameras[1]->Resize(rcWnd, rcCamera,false);

	if (m_pActiveCamera!=m_cameras[2])
		rcCamera.SetRect(rcWnd.Width()/2, rcWnd.Height()/2, rcWnd.right, rcWnd.bottom);
	else rcCamera = rcWnd;
	m_cameras[2]->Resize(rcWnd, rcCamera,true);

	if (m_pActiveCamera!=m_cameras[3])
		rcCamera.SetRect(0, rcWnd.Height()/2, rcWnd.Width()/2, rcWnd.bottom);
	else rcCamera = rcWnd;
	m_cameras[3]->Resize(rcWnd, rcCamera,false);
*/	
}
void CD3DScene::SetPreset()
{
	preset = !preset;
	if (preset)
	{
		m_pScene->GetTileMap()->SetDiffuse(sColor4f(0.98468125f,0.98468125f,0.85692370f,0.47104770f));
		m_pScene->SetSun(Vect3f(0,-1,-1),sColor4f(0.94209540f,0.94209540f,0.94209540f,1.f),
			sColor4f(0.98468125f,0.98468125f,0.85692370f,1.f),sColor4f(0.98468125f,0.98468125f,0.85692370f,1.f));

	}else
	{
		m_pScene->GetTileMap()->SetDiffuse(sColor4f(0.80000001f,0.80000001f,0.80000001f,0.19999999f));
		m_pScene->SetSun(Vect3f(0,-1,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
	}
}

bool CD3DScene::InitRenderDevice(CWnd* pWnd)
{
	vMap.disableLoadContainerPMO();
	ASSERT(gb_VisGeneric == 0);

	CRect rcWnd;
	pWnd->GetClientRect(rcWnd);

	m_pRenderDevice = CreateIRenderDevice(false);
	if(!m_pRenderDevice->Initialize(rcWnd.Width(), rcWnd.Height(), RENDERDEVICE_MODE_WINDOW, pWnd->m_hWnd))
		return false;
	// создание сцены
	m_pScene=gb_VisGeneric->CreateScene(); 

	// создается источник света, иначе кромешная тьма и объектов не видно
	m_pScene->SetSun(Vect3f(0,-1,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
	//m_pScene->SetSun(Vect3f(0,0.58778530,-0.80901706),sColor4f(0.94209540,0.94209540,0.94209540,1),
	//	sColor4f(0.98468125,0.98468125,0.85692370,1),sColor4f(0.98468125,0.98468125,0.85692370,1));
	
	/*
	light_vector	{x=0.00000000 y=0.58778530 z=-0.80901706 ...}	Vect3f
		-	ambient_color	{r=0.94209540 g=0.94209540 b=0.94209540 ...}	sColor4f
		r	0.94209540	float
		g	0.94209540	float
		b	0.94209540	float
		a	255.00000	float
		-	sun_color	{r=0.98468125 g=0.98468125 b=0.85692370 ...}	sColor4f
		r	0.98468125	float
		g	0.98468125	float
		b	0.85692370	float
		a	255.00000	float
		-	specular_color	{r=0.98468125 g=0.98468125 b=0.85692370 ...}	sColor4f
		r	0.98468125	float
		g	0.98468125	float
		b	0.85692370	float
		a	255.00000	float
		*/
//	m_pEffectObj = m_pScene->CreateEffect();

	CD3DCamera* pCamera;
	CRect rcCamera;

/*	//global camera
	rcCamera = rcWnd;
	pCamera = new CD3DCamera;
	pCamera->Init(m_pScene, m_pRenderDevice, rcWnd, rcCamera, CAMERA_PLANE_Z);
	pCamera->m_bVisible = true;
	pCamera->m_bMain = true;
	pCamera->LoadCameraSettings();
	m_cameras.push_back(pCamera);
*/
	//camera LT
	rcCamera.SetRect(0, 0, rcWnd.Width()/2, rcWnd.Height()/2);
	pCamera = new CD3DCamera(M_PI/2, 0, 1024);
	pCamera->m_bLocked = true;
	pCamera->Init(m_pScene, m_pRenderDevice, rcWnd, rcCamera, CAMERA_PLANE_Z);
	m_cameras.push_back(pCamera);

	//camera RT
	rcCamera.SetRect(rcWnd.Width()/2, 0, rcWnd.right, rcWnd.Height()/2);
	pCamera = new CD3DCamera(M_PI/2, M_PI/2, 1024);
	pCamera->Init(m_pScene, m_pRenderDevice, rcWnd, rcCamera, CAMERA_PLANE_Y, false);
	pCamera->m_bLocked = true;
	m_cameras.push_back(pCamera);

	//camera RB

	rcCamera.SetRect(rcWnd.Width()/2, rcWnd.Height()/2, rcWnd.right, rcWnd.bottom);
	pCamera = new CD3DCamera;
	pCamera->Init(m_pScene, m_pRenderDevice, rcWnd, rcCamera, CAMERA_PLANE_Z);
	m_cameras.push_back(pCamera);

	//camera LB
	rcCamera.SetRect(0, rcWnd.Height()/2, rcWnd.Width()/2, rcWnd.bottom);
	pCamera = new CD3DCamera(0, M_PI/2, 1024);
	pCamera->Init(m_pScene, m_pRenderDevice, rcWnd, rcCamera, CAMERA_PLANE_X, false);
	pCamera->m_bLocked = true;
	m_cameras.push_back(pCamera);


	m_pActiveCamera = m_cameras[2];
	m_pActiveCamera->m_bVisible = true;
	if(!m_pDoc->m_WorldPath.IsEmpty()){
		// Загрузка мира
//		gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF, 3);

		terMapPoint = m_pScene->CreateMap(new StandartTerraInterface,0);
		terMapPoint->SetAttr(ATTRUNKOBJ_IGNORE);
	}
	CameraListType::iterator it;
	FOR_EACH(m_cameras,it)
		(*it)->ViewPos(GetCenter());
	if (m_cameras.size()>=3)
	m_cameras[2]->LoadCameraSettings();

	m_pPEManager = new PostEffectManager();
	m_pPEManager->Init(PE_MIRAGE);
 	return true;
}

void CD3DScene::SetActiveCamera(const CPoint& pt)
{
	CameraListType::iterator it;
	FOR_EACH(m_cameras, it)
		if((*it)->HitTest(pt))
		{
			m_pActiveCamera = *it;
			break;
		}
}

void CD3DScene::CameraDefault()
{
	m_pActiveCamera->ViewPos(GetCenter());
}
void CD3DScene::CameraRotate(float dPsi, float dTheta)
{
	ASSERT(m_pActiveCamera);

	m_pActiveCamera->Rotate(dPsi, dTheta);
}
void CD3DScene::CameraMove(float dx, float dy, float dz)
{
	ASSERT(m_pActiveCamera);

	m_pActiveCamera->Move(dx, dy, dz);
}
void CD3DScene::CameraZoom(float delta)
{
	ASSERT(m_pActiveCamera);

	m_pActiveCamera->Zoom(delta);
}

CAMERA_PLANE CD3DScene::GetCameraPlane()
{
	ASSERT(m_pActiveCamera);
	return m_pActiveCamera->m_plane;
}
const CRect& CD3DScene::GetActiveCameraRect()
{
	ASSERT(m_pActiveCamera);

	return m_pActiveCamera->m_rcCamera;
}

void CD3DScene::SetMultiviewMode(bool bMulti)
{
	int ix=0;
	for(int i=0; i<m_cameras.size(); ++i)
	{
		if (m_pActiveCamera == m_cameras[i])
			ix=i;
		else m_cameras[i]->m_bVisible = bMulti;
	}
	CRect rcWnd,rcCamera;
	m_pRenderDevice->GetClipRect((int*)&rcWnd.left, (int*)&rcWnd.top, (int*)&rcWnd.right, (int*)&rcWnd.bottom);
	if (bMulti==true)
		switch(ix)
		{
		case 0:	rcCamera.SetRect(0, 0, rcWnd.Width()/2, rcWnd.Height()/2); break;
		case 1: rcCamera.SetRect(rcWnd.Width()/2, 0, rcWnd.right, rcWnd.Height()/2); break;
		case 2: rcCamera.SetRect(rcWnd.Width()/2, rcWnd.Height()/2, rcWnd.right, rcWnd.bottom); break;
		case 3: rcCamera.SetRect(0, rcWnd.Height()/2, rcWnd.Width()/2, rcWnd.bottom); break;
		}
	else
		rcCamera = rcWnd;
	m_pActiveCamera->Resize(rcWnd, rcCamera,ix==0||ix==2);
	m_bCameraMode = bMulti;
}

void CD3DScene::Camera2World(Vect3f& v_to, float x, float y, Vect3f* pvObjDist)
{
	ASSERT(m_pActiveCamera);
	m_pActiveCamera->Camera2World(v_to, x, y, pvObjDist);
}
void CD3DScene::GetCameraRay(Vect3f& vRay, Vect3f& vOrg, float x, float y)
{
	ASSERT(m_pActiveCamera);
	m_pActiveCamera->GetCameraRay(vRay, vOrg, x, y);
}
void CD3DScene::SetActiveKeyPos(const int i)
{
	if(m_pDoc->ActiveEmitter()->IsBase())
	{
		ASSERT(i<m_pDoc->ActiveEmitter()->emitter_position().size());
		m_pWayPointActive = &m_pDoc->ActiveEmitter()->emitter_position()[i];
	}
}
void CD3DScene::SetActiveWayPoint(int nPoint)
{
	float fTime = m_pDoc->ActiveEmitter()->GenerationPointTime(nPoint);

	m_pWayPointActive = 0;

	CKeyPos::iterator i;

	FOR_EACH(m_pDoc->ActiveEmitter()->emitter_position(), i)
		if(fabs(i->time - fTime) < 0.001f)
		{
			m_pWayPointActive = &*i;
			break;
		}
}
KeyPos* CD3DScene::GetActiveWayPoint()
{
	return m_pWayPointActive;
}
KeyPos* CD3DScene::HitTestWayPoint(const Vect3f& v)
{
	KeyPos* pKey = 0;

	CKeyPos::iterator it;
	FOR_EACH(m_pDoc->ActiveEmitter()->emitter_position(), it)
		if(it->pos.distance(v) < 10)
		{
			pKey = &(*it);
			break;
		}

	return pKey;
}

cTexture* CD3DScene::CreateTexture(LPCSTR lpszName)
{
	return gb_VisGeneric->CreateTexture(lpszName);
}

bool CD3DScene::LoadModelInsideEmitter(LPCTSTR lpszModelName)
{
	strlwr((char*)lpszModelName);
/*	if (strstr(lpszModelName,".m3d"))
	{
		if (m_pDoc&&m_pDoc->ActiveEmitter()&&m_pDoc->ActiveEmitter()->data)
		{
			gb_VisGeneric->SetLinkEffectToModel(false);
			cObjectNodeRoot* model=(cObjectNodeRoot*)m_pScene->GetObjLibrary()->GetElement(lpszModelName,0);
			m_pDoc->ActiveEmitter()->data->Load3DModelPos(model);
			RELEASE(model);
			theApp.scene.InitEmitters();
			return true;
		}
	}else */
	if (strstr(lpszModelName,".3dx"))
	{
		cObject3dx* model = m_pScene->CreateObject3dx(lpszModelName);
		cObject3dx* modelLogic = m_pScene->CreateLogic3dx(lpszModelName);

		cObject3dx* p=modelLogic?modelLogic:model;
		sBox6f boundBox;
		p->GetBoundBoxUnscaled(boundBox);
		float boundScale = (one_size_model)/max(boundBox.max.z - boundBox.min.z, FLT_EPS);
		model->SetScale(boundScale);

		m_pDoc->ActiveEmitter()->GetBase()->Load3DModelPos(model);
		RELEASE(model);
		RELEASE(modelLogic)
		theApp.scene.InitEmitters();
	}
	return false;
}

void CD3DScene::LoadParticleModel(LPCTSTR lpszModelName, int mode)
{
	strlwr((char*)lpszModelName);
	if (strstr(lpszModelName,".3dx"))
	{
		if(mode){
			RELEASE(m_pModel);
			RELEASE(m_pModelLogic);
			m_pModel  = m_pScene->CreateObject3dx(lpszModelName);
			m_pModelLogic  = m_pScene->CreateLogic3dx(lpszModelName);
			m_pModel->SetSkinColor(sColor4c(255,0,0));
			_pDoc->ActiveEffect()->ModelScale(_pDoc->ActiveEffect()->GetModelScale());
		}else{
			RELEASE(m_pBackModel);
			m_pBackModel = m_pScene->CreateObject3dx(lpszModelName);
			m_pBackModel->SetSkinColor(sColor4c(255,0,0));
		}
	}
}

void CD3DScene::DrawGrid()
{
	const num=20;
	float step=10;
	float mn=-num*step,mx=num*step;

	Vect3f	fx_center=GetCenter();
	MatXf mat(Mat3f::ID,fx_center);
	if (m_pModel)
	{
		if (m_pDoc->m_nCurrentNode>0)
		{
			mat = m_pModel->GetNodePositionMat(m_pDoc->m_nCurrentNode-1);
		}
	}
	
	for(int x=-num;x<=num;x++)
	{
		Vect3f pn,px;
		//pn.set(x*step +fx_center.x,mn +fx_center.y, fx_center.z);
		//px.set(pn.x,mx +fx_center.y, fx_center.z);
		pn.set(x*step ,mn , 0);
		px.set(pn.x,mx , 0);
		mat.xformPoint(pn);
		mat.xformPoint(px);
		m_pRenderDevice->DrawLine(pn,px,sColor4c(196,196,196,255));
	}

	for(int y=-num;y<=num;y++)
	{
		Vect3f pn,px;
		//pn.set(mn +fx_center.x,y*step +fx_center.y, fx_center.z);
		//px.set(mx +fx_center.x,pn.y, fx_center.z);
		pn.set(mn ,y*step,0);
		px.set(mx ,pn.y, 0);
		
		mat.xformPoint(pn);
		mat.xformPoint(px);
		m_pRenderDevice->DrawLine(pn,px,sColor4c(196,196,196,255));
	}

	Vect3f pn,px;
	//pn.set(fx_center.x,fx_center.y, fx_center.z);
	//px.set(mx +fx_center.x,pn.y, fx_center.z);
	pn.set(0,0, 0);
	px.set(mx ,pn.y, 0);

	mat.xformPoint(pn);
	mat.xformPoint(px);
	m_pRenderDevice->DrawLine(pn,px,sColor4c(255,0,0,255));

	//pn.set(fx_center.x,fx_center.y, fx_center.z);
	//px.set(pn.x,mx +fx_center.y, fx_center.z);
	pn.set(0,0,0);
	px.set(pn.x,mx, 0);
	
	mat.xformPoint(pn);
	mat.xformPoint(px);
	m_pRenderDevice->DrawLine(pn,px,sColor4c(0,255,0,255));
}

inline KeyPos* get_pos_at_time(CKeyPos& key_pos, float fTime)
{
	CKeyPos::iterator it;
	FOR_EACH(key_pos, it)
	{
		if(fabs(fTime - it->time) < 0.001f)
			return &*it;
	}

	return 0;
}


static sColor4c clrActive(255, 0, 0, 255);
static sColor4c clrHover(255, 255, 0, 255);
static sColor4c clrInactive(0, 0, 255, 255);

void CD3DScene::DrawInfo(CD3DCamera* pCamera)
{
	if(!m_pDoc->ActiveEmitter())
		return;
	const int GPoint = m_pDoc->m_nCurrentGenerationPoint;
	float fTime = m_pDoc->ActiveEmitter()->Effect2EmitterTime(GetEffect()->GetTime());

	bool bSplineEmtr = m_pDoc->ActiveEmitter()->Class() == EMC_SPLINE;

	Vect3f fx_center=GetCenter();

//	if(m_pDoc->ActiveEmitter()->Class() != EMC_LIGHT) //if(m_bPause)
	{
		if(m_ToolMode != TOOL_NONE || bSplineEmtr) 
		{
			ASSERT(m_pDoc && m_pDoc->ActiveEmitter());

			//if(m_bPause)
			//	m_pWayPointActive = get_pos_at_time(m_pDoc->ActiveEmitter()->pos, fTime);

			Vect3f* prev_pos = 0;
			CKeyPos::iterator it;
			CKeyPos& emit_pos =  m_pDoc->ActiveEmitter()->emitter_position(); 
			int i=0;
			FOR_EACH(emit_pos, it)
			{
				if(prev_pos)
					m_pRenderDevice->DrawLine((*prev_pos) +fx_center, 
												it->pos +fx_center, sColor4c(0, 0, 255, 255));

				sColor4c* pClr = &clrInactive;
				if(m_pDoc->m_nCurrentGenerationPoint == i)
					pClr = &clrActive;

				MatXf mat(Mat3f::ID,it->pos);
				if (m_pModel && m_pDoc->m_nCurrentNode>0)
				{
					mat.premult(m_pModel->GetNodePositionMat(m_pDoc->m_nCurrentNode-1));
				}else
				mat.trans() += fx_center;
				//DrawCircle(m_pRenderDevice, MatXf(Mat3f::ID, it->pos +fx_center), 5, *pClr);
				DrawCircle(m_pRenderDevice, mat, 5, *pClr);

				prev_pos = &it->pos;
				++i;
			}
		}

		if(m_ToolMode == TOOL_DIRECTION || bSplineEmtr)
		{
			ASSERT(m_pDoc && m_pDoc->ActiveEmitter());
			MatXf pos;
			float ft = m_pDoc->ActiveEmitter()->GenerationPointTime(m_pDoc->m_nCurrentGenerationPoint);
			if (m_pDoc->ActiveEmitter()->IsBase())
			{
				m_pDoc->ActiveEmitter()->GetBase()->GetPosition(ft, pos);
				if (m_pModel && m_pDoc->m_nCurrentNode>0)
				{
					pos.premult(m_pModel->GetNodePositionMat(m_pDoc->m_nCurrentNode-1));
				}else
					pos.trans()+=fx_center;
			}else if (m_pDoc->ActiveEmitter()->IsCLight())
			{
				pos.rot().set(m_pDoc->ActiveEmitter()->rot());
				pos.trans() = m_pDoc->ActiveEmitter()->emitter_position().Get(ft)+fx_center;
			}
			DrawXYZ(m_pRenderDevice, pos);
		}
		else if(m_ToolMode == TOOL_DIRECTION_BS)
		{
			ASSERT(m_pDoc && m_pDoc->ActiveEmitter() && m_pDoc->ActiveEmitter()->IsBase());

			MatXf pos;
			m_pDoc->ActiveEmitter()->GetBeginSpeedMatrix(m_pDoc->m_nCurrentGenerationPoint, m_pDoc->m_nCurrentParticlePoint, pos);
			pos.trans()+=fx_center;
			DrawXYZ(m_pRenderDevice, pos);
		}
	}
	if(bShowEmitterBox && m_pDoc->ActiveEmitter() && m_pDoc->ActiveEmitter()->IsBase())
	{
		static sColor4c _sClr(255, 255, 255, 255);
		MatXf pos;
		float fTime = m_pDoc->ActiveEmitter()->Effect2EmitterTime(GetEffect()->GetTime());

		m_pDoc->ActiveEmitter()->GetBase()->GetPosition(fTime, pos);

		switch(m_pDoc->ActiveEmitter()->particle_position().type)
		{
		case EMP_BOX:
			{
				Vect3f sz = 50.0f*m_pDoc->ActiveEmitter()->particle_position().size;
				DrawBox(m_pRenderDevice, MatXf(pos.rot(), pos.trans()+ fx_center), sz, -sz, _sClr);
			}
			break;
		case EMP_CYLINDER:
			{
				Vect3f sz = 100.0f*m_pDoc->ActiveEmitter()->particle_position().size;
				bool cone = m_pDoc->ActiveEmitter()->cone();

				MatXf m = MatXf::ID;
				if (!m_pDoc->ActiveEmitter()->bottom())
					m.trans().z=-sz.y*0.5f;
				DrawCylinder(m_pRenderDevice, (MatXf(pos.rot(), pos.trans()+ fx_center))*m, sz.x,cone?sz.z:sz.x, sz.y, _sClr);
			}
			break;
		case EMP_SPHERE:
			{
				float r=100.0f*m_pDoc->ActiveEmitter()->particle_position().size.x;
				DrawSphere(m_pRenderDevice, MatXf(pos.rot(), pos.trans()+ fx_center), r, _sClr, _sClr, _sClr);
			}
			break;
		case EMP_LINE:
			break;
		case EMP_RING:
			{
				float r=100.0f*m_pDoc->ActiveEmitter()->particle_position().size.x;
				float ba = m_pDoc->ActiveEmitter()->particle_position().alpha_min;
				float ea = m_pDoc->ActiveEmitter()->particle_position().alpha_max;
				float bt = m_pDoc->ActiveEmitter()->particle_position().teta_min;
				float et = m_pDoc->ActiveEmitter()->particle_position().teta_max;
				DrawSector(m_pRenderDevice, MatXf(pos.rot(), pos.trans()+ fx_center), r, ba, ea, bt, et, _sClr);
				break;
			}
		//EMP_3DMODEL,
		}
	}
	if(bSplineEmtr)
	{
		MatXf _offs;
		m_pDoc->GetSplineMatrix(_offs);
		//_offs.rot() = Mat3f::ID;
		_offs.trans()+=fx_center;
		const Vect3f* prev_pos = 0;
		CKeyPosHermit::iterator it;
		int i=0;
		FOR_EACH(m_pDoc->ActiveEmitter()->p_position(), it)
		{
			Vect3f v1, v2;
			if(prev_pos)
			{
				_offs.xformPoint((*prev_pos), v1);
				_offs.xformPoint(it->pos, v2);
				m_pRenderDevice->DrawLine(v1, v2, sColor4c(0, 255, 0, 255));
			}

			MatXf mat;
			pCamera->GetBilboardMatrix(mat.rot());
			mat.trans() = _offs.xformPoint(it->pos, v1);
			sColor4c color(0, 255, 0, 255);
			if (i==m_pDoc->m_nCurrentParticlePoint && m_ToolMode == CD3DScene::TOOL_SPLINE)
				color.set(255,0,0,255);
			DrawCircle(m_pRenderDevice, mat, 3, color);

			prev_pos = &it->pos;
			++i;
		}
	}
	CEmitterData *edat = m_pDoc->ActiveEmitter();
	if (m_ToolMode == TOOL_POSITION && edat->IsLighting())
	{
		const int radius = 5;
		MatXf mat;
		sColor4c color(0, 0, 255, 255);
		sColor4c sel_color(255, 0, 0);
		DrawCircle(m_pRenderDevice, MatXf(Mat3f::ID, edat->pos_begin() +fx_center), radius, GPoint==0 ? sel_color : color);
		for(int i=0;i<edat->pos_end().size(); i++)
		{
			m_pRenderDevice->DrawLine(edat->pos_begin() +fx_center, edat->pos_end()[i] +fx_center, color);
			DrawCircle(m_pRenderDevice, MatXf(Mat3f::ID, edat->pos_end()[i] +fx_center), radius, GPoint==(i+1) ? sel_color : color);
		}
	}
}

void CD3DScene::QuantFps()
{
	fps.quant();
	((CMainFrame*)AfxGetMainWnd())->SetFps(fps.GetFPS());
}
void CD3DScene::ClearTriangleCount()
{
	if (m_pEffectObj)
		m_pEffectObj->ClearTriangleCount();
}
int CD3DScene::GetTriangleCount()
{
	if (m_pEffectObj&&m_pEffectObj->GetTriangleCount()>0)
		return m_pEffectObj->GetTriangleCount();
	return 0;
}
double CD3DScene::GetSquareTriangle()
{
	if (m_pEffectObj&&m_pEffectObj->GetSquareTriangle()>0)
		return m_pEffectObj->GetSquareTriangle();
	return 0;
}
void TestKeyDown()
{
	static bool down = false;
	if (IsKey(VK_CONTROL))
		if (IsKey('Z'))
		{
			if (!down)
				_pDoc->History().Undo();
			down = true;
		}
		else if (IsKey('X'))
		{
			if (!down)
				_pDoc->History().Redo();
			down = true;
		}else
			down = false;
		
}
void CD3DScene::Quant()
{
	TestKeyDown();
	theApp.scene.ClearTriangleCount();
	if(!m_pScene)
		return;

	if(!m_pEffectObj)
		InitEmitters();

	if(!m_pDoc->m_WorldPath.IsEmpty()){
		if(bShowWorld)
			terMapPoint->ClearAttr(ATTRUNKOBJ_IGNORE);
		else
			terMapPoint->SetAttr(ATTRUNKOBJ_IGNORE);
	}

	Vect3f fx_center=GetCenter();

	CameraListType::iterator it;
	FOR_EACH(m_cameras, it)
		(*it)->Quant();

	m_pEffectObj->SetPosition(MatXf(Mat3f::ID, fx_center));
	m_pEffectObj->SetParticleRate(m_particle_rate);
	int dt=0;
	if(!bPause)
	{
		static DWORD g_CurTime=0;
		DWORD time=timeGetTime();
		dt=time-g_CurTime;
		if(dt<0 || dt>1000)dt=0;
		
		m_pScene->SetDeltaTime((float)dt);
		g_CurTime=time;

	}else
		m_pScene->SetDeltaTime(0);
/// Background
	if(m_pBackModel)
	{
		if(m_pDoc->ActiveGroup()->m_bShow3DBack){
			m_pBackModel->ClearAttr(ATTRUNKOBJ_IGNORE);
			m_pBackModel->SetPosition(MatXf(MatXf::ID.rot(), fx_center));
		}
//		else m_pBackModel->SetAttribute(ATTRUNKOBJ_IGNORE);
	}
///
	if(m_pModel)
	{
/*
		if(m_pDoc->ActiveEmitter() && (m_pDoc->ActiveEmitter()->data && m_pDoc->ActiveEmitter()->data->particle_position.type == EMP_3DMODEL))
		{
			float fTime = m_pDoc->ActiveEmitter()->Effect2EmitterTime(GetEffect()->GetTime());

			MatXf pos;
			m_pDoc->ActiveEmitter()->data->GetPosition(fTime, pos);

			m_pModel->ClearAttribute(ATTRUNKOBJ_IGNORE);
			m_pModel->SetPosition(pos);
		}
		else
*/
		int ichain=m_pModel->GetAnimationGroupChain(0);
		cAnimationChain* chain=m_pModel->GetChain(ichain);

		float frame_period=(1.f/chain->time)*0.001f;

		modelPhase += frame_period*dt;
		if (modelPhase > 1)
			modelPhase = 0;

		{
			if(m_pDoc->ActiveEffect()->Show3Dmodel){
				m_pModel->ClearAttr(ATTRUNKOBJ_IGNORE);
				m_pModel->SetPosition(MatXf(MatXf::ID.rot(), fx_center));
			}
			//m_pModel->SetAnimationGroupPhase(0,modelPhase);
			m_pModel->SetPhase(modelPhase);
//			else m_pModel->SetAttribute(ATTRUNKOBJ_IGNORE);
		}

	}
/*
	FOR_EACH(m_cameras, it)
		if((*it)->m_bVisible)
			m_pScene->PreDraw((*it)->m_pCamera);
*/
	
	m_pRenderDevice->Fill(backgroundColor.r,backgroundColor.g,backgroundColor.b);
	m_pRenderDevice->BeginScene();
	m_pRenderDevice->SetRenderState(RS_FILLMODE, /*bWireFrame?FILL_WIREFRAME:*/ FILL_SOLID);

	FOR_EACH(m_cameras, it)
	{
		CD3DCamera* p = *it;
		if(p->m_bVisible)
		{
			m_pRenderDevice->SetDrawTransform(p->m_pCamera);
			
			if(bShowGrid)
				DrawGrid();

			m_pScene->Draw(p->m_pCamera);
			m_pPEManager->Draw();
			m_pRenderDevice->SetDrawTransform(p->m_pCamera);

			DrawInfo(*it);

			DrawCircle(m_pRenderDevice, MatXf(MatXf::ID.rot(), fx_center), .5 ,sColor4c(255, 0, 0, 255));

			m_pRenderDevice->FlushPrimitive3D();
		}
	}

	m_pRenderDevice->SetClipRect(0,0,m_pRenderDevice->GetSizeX(),m_pRenderDevice->GetSizeY());
	FOR_EACH(m_cameras, it)
	{
		CD3DCamera* p = *it;
		if(p->m_bVisible)
		{
			(*it)->DrawCameraRect(m_pRenderDevice, p == m_pActiveCamera);
		}
	}
	m_pRenderDevice->FlushPrimitive2D();

	if(!m_pEffectObj->IsLive())
		InitEmitters();

	m_pRenderDevice->EndScene();
	m_pRenderDevice->Flush();
/*
	FOR_EACH(m_cameras, it)
		if((*it)->m_bVisible)
			m_pScene->PostDraw((*it)->m_pCamera);
*/
	QuantFps();
}

void CD3DScene::InitEmitters()
{
	Vect3f fx_center=GetCenter();
	CEffectData* eff = _pDoc->ActiveEffect();
	if(eff)
	{
		eff->prepare_effect_data(_pDoc->ActiveEmitter());
//		m_pEffectObj->Init(*pDoc->ActiveEffect(), m_pModel);
		RELEASE(m_pEffectObj);

		eff->BuildRuntimeData();
		
		CEffectData scaleData(eff);
		scaleData.RelativeScale(_pDoc->ActiveEffect()->GetEffectScale());
		//_pDoc->ActiveEffect()->GetEffectScale()
		m_pEffectObj = m_pScene->CreateEffectDetached(scaleData , m_pModel);
		m_pEffectObj->Attach();
		m_pEffectObj->useOverDraw = true;
		m_pEffectObj->initOverdraw();
		if(!_pDoc->m_WorldPath.IsEmpty())
		{
			if (fx_center.x<0 || fx_center.x>=vMap.H_SIZE || fx_center.y<0 || fx_center.y>=vMap.V_SIZE)
				fx_center.set(vMap.H_SIZE/2, vMap.V_SIZE/2, 0);
			fx_center.z = vMap.GetApproxAlt(fx_center.x, fx_center.y);
		}
		else
			fx_center.z = 0;
		
		m_pEffectObj->SetPosition(MatXf(Mat3f::ID, fx_center));
		if (m_pModel&&m_pDoc->m_nCurrentNode>0)
			m_pEffectObj->LinkToNode(m_pModel,m_pDoc->m_nCurrentNode-1);
		if (_pDoc->ActiveEmitter())
		{
			m_pEffectObj->SetTime(_pDoc->ActiveEmitter()->emitter_create_time());

			if (m_pModel)
			{
				int ichain=m_pModel->GetAnimationGroupChain(0);
				cAnimationChain* chain=m_pModel->GetChain(ichain);

				modelPhase = (1.f/chain->time)*_pDoc->ActiveEmitter()->emitter_create_time();
			}else
				modelPhase = 0;

		}else
			modelPhase =0;

		for(int i=eff->EmittersSize()-1; i>=0; i--)
		{
			CEmitterData *em = eff->Emitter(i);
			m_pEffectObj->ShowEmitter(em->emitter(), em->IsActive());
		}
		m_pEffectObj->enableOverDraw = enableOverDraw;
	}else
		modelPhase =0;
}

const Vect3f& CD3DScene::GetCenter()
{
//	if(m_bCameraMode)
//		return m_pDoc->m_NullCenter;
	return m_pDoc->m_EffectCenter;
}

void CD3DScene::LoadSceneSettings()
{
	bShowGrid = theApp.GetProfileInt("Scene", "ShowGrid", 1);
	bShowWorld = theApp.GetProfileInt("Scene", "ShowWorld", 1);
}

void CD3DScene::SaveSceneSettings()
{
	theApp.WriteProfileInt("Scene", "ShowGrid", bShowGrid);
	theApp.WriteProfileInt("Scene", "ShowWorld", bShowWorld);
}

