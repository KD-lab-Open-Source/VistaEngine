// D3DWindow.cpp : implementation file
//

#include "stdafx.h"
#include "EffectTool.h"
#include "D3DWindow.h"
#include "ControlView.h"
#include ".\d3dwindow.h"
#include "OptTree.h"
extern COptTree* tr;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CControlView* ctrv;
const float _eps = 20.0f;
const float fMouseRotateScale = 0.01f;
const float fMouseMoveScale = 1.f;

#define IS_SHIFT() (GetAsyncKeyState(VK_SHIFT) & 0x8000)
#define IS_ALT() (GetAsyncKeyState(VK_MENU) & 0x8000)
#define IS_CTRL() (GetAsyncKeyState(VK_CONTROL) & 0x8000)
#define IS_RBUT() (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
#define IS_LBUT() (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
float SqrDistRayPoint(const Vect3f& vPoint, const Vect3f& vRay, const Vect3f& vRayOrg)
{
    Vect3f kDiff = vPoint - vRayOrg;
    float fT = kDiff.dot(vRay);

    if (fT <= 0.0f)
    {
        fT = 0.0f;
    }
    else
    {
        fT /= vRay.norm2();
        kDiff -= fT*vRay;
    }

    return kDiff.norm2();
}

KeyPos* HitTestSplinePoint(CKeyPosHermit& spline, float x, float y, const MatXf& offs)
{
	const float _eps = 5.0f;

	Vect3f vRay, vOrg;
	theApp.scene.GetCameraRay(vRay, vOrg, x, y);

	CKeyPosHermit::iterator it;
	FOR_EACH(spline, it)
	{
		Vect3f abs_pos = offs*it->pos+theApp.scene.GetCenter();
		if(SqrDistRayPoint(abs_pos, vRay, vOrg) < _eps)
			return &*it;
	}

	return 0;
}
void RemoveSplinePoint(CKeyPosHermit& spline, KeyPos *p)
{
	CKeyPosHermit::iterator it;
	FOR_EACH(spline, it)
		if(&*it == p)
		{
			spline.erase(it);
			break;
		}
}
KeyPos* AddSplinePoint(CKeyPosHermit& spline)
{
	spline.push_back(KeyPos());
	spline.back().pos.set(0, 0, 0);

	return &spline.back();
}
/////////////////////////////////////////////////////////////////////////////
// CD3DWindow

IMPLEMENT_DYNCREATE(CD3DWindow, CView)

CD3DWindow::CD3DWindow()
{
	m_hCursorHand = theApp.LoadCursor(IDC_CURSOR_MOVE);
	m_ptPrevPoint = CPoint(0, 0);
	m_bLeftPressed = false;
}
CD3DWindow::~CD3DWindow()
{
}

BOOL CD3DWindow::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.lpszClass = AfxRegisterWndClass(0);
	
	return CView::PreCreateWindow(cs);
}


void CD3DWindow::SetWaypointHover(const CPoint& pt)
{
/*	if(GetDocument()->ActiveEmitter())
	{
		CRect rc;
		GetClientRect(rc);

		Vect3f v;
		theApp.scene.Camera2World(v, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);
		theApp.scene.WayPointHover(v);
	}
*/
}
void CD3DWindow::PositionKeysRotate(float dPsi, float dTheta)
{
	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	if(!pEmitter||!pEmitter->IsBase())
		return;
	ASSERT(GetDocument()->m_nCurrentGenerationPoint<pEmitter->emitter_position().size());
	
	Vect3f center(0,0,0);
	CKeyPos& pks = pEmitter->emitter_position();
	CKeyPos::iterator it;
	FOR_EACH(pks,it)
		center+=it->pos;
	center/=pks.size();
	Mat3f mtr;
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		mtr.set(dPsi,X_AXIS);
		break;
	case CAMERA_PLANE_Y:
		mtr.set(dPsi,Y_AXIS);
		break;
	case CAMERA_PLANE_Z:
		mtr.set(dPsi,Z_AXIS);
		break;
	}
	FOR_EACH(pks,it)
		it->pos = (it->pos-center)*mtr+center;
}
void CD3DWindow::SetAllPositionKey(const CPoint& pt,const CPoint& prev_pt)
{
	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	if(!pEmitter||!pEmitter->IsBase())
		return;
	CRect rc;
	GetClientRect(rc);
	CKeyPos::iterator it;
	Vect3f v1,v2;
	theApp.scene.Camera2World(v1, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);
	theApp.scene.Camera2World(v2, float(prev_pt.x)/rc.Width() - 0.5f, float(prev_pt.y)/rc.Height() - 0.5f);
	v1-=v2;
	FOR_EACH(pEmitter->emitter_position(),it)
		it->pos+=v1;
}
KeyPos* CD3DWindow::HitPositionKey(const CPoint& pt)
{
	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	if(!pEmitter || !pEmitter->IsBase())
		return NULL;
	Vect3f fxc = GetDocument()->m_EffectCenter;
	Vect3f fx_center=theApp.scene.GetCenter();
	CRect rc;
	GetClientRect(rc);
	Vect3f v;
	Vect2f pt_sc(float(pt.x)/rc.Width() - 0.5f , float(pt.y)/rc.Height() - 0.5f);
	CKeyPos& emp = pEmitter->emitter_position();
	int i;
	int cam_plane = theApp.scene.GetCameraPlane();
	for(i = emp.size()-1;i>=0;--i)
	{
		Vect3f abs_pos = fx_center+emp[i].pos;
		theApp.scene.Camera2World(v, pt_sc.x, pt_sc.y,&abs_pos/*emp[i].pos*/);
		v -= fx_center;
		switch(cam_plane)
		{
		case CAMERA_PLANE_X:
			if (abs(emp[i].pos.y - v.y)<5 && abs(emp[i].pos.z - v.z)<5)
				goto finish;			 
			break;
		case CAMERA_PLANE_Y:
			if (abs(emp[i].pos.x - v.x)<5 && abs(emp[i].pos.z - v.z)<5)
				goto finish;			 
			break;
		case CAMERA_PLANE_Z:
			if (abs(emp[i].pos.x - v.x)<5 && abs(emp[i].pos.y - v.y)<5)
				goto finish;			 
			break;
		}
	}
	return NULL;
finish:
	GetDocument()->m_nCurrentGenerationPoint = i;
	ctrv->OnChangeActivePoint(0,0);//PostMessage(WM_ACTIVE_POINT,0,0);
	return &emp[i];
}
extern void ChangePosition(CEmitterData* emi, int ix);
void CD3DWindow::SetPositionKey(const CPoint& pt, bool bNoChangePos)
{
	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	if(!pEmitter || !pEmitter->IsBase())
		return;
	theApp.scene.SetActiveCamera(pt);
//	Vect3f fxc = GetDocument()->m_EffectCenter;
	Vect3f fx_center=theApp.scene.GetCenter();
	CRect rc;
	GetClientRect(rc);
	Vect3f v;
	Vect2f pt_sc(float(pt.x)/rc.Width() - 0.5f , float(pt.y)/rc.Height() - 0.5f);
	ASSERT(GetDocument()->m_nCurrentGenerationPoint<pEmitter->emitter_position().size());
	KeyPos* pos_key = &pEmitter->emitter_position()[GetDocument()->m_nCurrentGenerationPoint];
	Vect3f abs_pos = fx_center+pos_key->pos;
	theApp.scene.Camera2World(v, pt_sc.x, pt_sc.y, &abs_pos);
	v-=fx_center;

	if(pos_key&&!bNoChangePos)
	{
		switch(theApp.scene.GetCameraPlane())
		{
		case CAMERA_PLANE_X:
			pos_key->pos.y = v.y; pos_key->pos.z = v.z;
			break;
		case CAMERA_PLANE_Y:
			pos_key->pos.x = v.x; pos_key->pos.z = v.z;
			break;
		case CAMERA_PLANE_Z:
			pos_key->pos.y = v.y; pos_key->pos.x = v.x;
			break;
		}
	}
//	ChangePosition(GetDocument()->ActiveEmitter(), GetDocument()->m_nCurrentGenerationPoint);

}

void CD3DWindow::RotateAround(vector<Vect3f>& points, Vect3f center, const Mat3f& rot)
{
	vector<Vect3f>::iterator it;
	FOR_EACH(points, it)
		(*it) = ((*it)-center)*rot+center;
}
Mat3f CD3DWindow::GetMatRot(CPoint ptDelta)
{
	float dPsi = ptDelta.x*fMouseRotateScale;
	float dTheta = -ptDelta.y*fMouseRotateScale;
	Mat3f mtr;
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		mtr.set(dPsi,X_AXIS);
		break;
	case CAMERA_PLANE_Y:
		mtr.set(dPsi,Y_AXIS);
		break;
	case CAMERA_PLANE_Z:
		mtr.set(dPsi,Z_AXIS);
		break;
	}
	return mtr;
}
Mat3f CD3DWindow::RotateAllKeyPos(CKeyPos& key_pos, float dPsi, float dTheta)
{
	if (key_pos.empty())
		return Mat3f::ID;
	Vect3f center(0,0,0);
	CKeyPos::iterator it;
//	FOR_EACH(key_pos, it)
//		center+=it->pos;
//	center/=key_pos.size();
	center = key_pos.front().pos; 
	Mat3f mtr;
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		mtr.set(dPsi,X_AXIS);
		break;
	case CAMERA_PLANE_Y:
		mtr.set(dPsi,Y_AXIS);
		break;
	case CAMERA_PLANE_Z:
		mtr.set(dPsi,Z_AXIS);
		break;
	}
	FOR_EACH(key_pos, it)
		it->pos = (it->pos-center)*mtr+center;
	return mtr;
}

Vect3f CD3DWindow::SetAllKeyPos(CKeyPos& key_pos, const CPoint& pt, const CPoint& prev_pt)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f v1,v2;
	theApp.scene.Camera2World(v1, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);
	theApp.scene.Camera2World(v2, float(prev_pt.x)/rc.Width() - 0.5f, float(prev_pt.y)/rc.Height() - 0.5f);
	v1-=v2;
	if (_pDoc->ActiveEmitter()->IsSpl())
	{
		MatXf _offs;
		GetDocument()->GetSplineMatrix(_offs);
		_offs.rot().invXform(v1);
	}
	CKeyPos::iterator it;
	FOR_EACH(key_pos,it)
		it->pos+=v1;
	return v1;
}
int CD3DWindow::HitTestPoint(const Vect3f &pos, const CPoint &pt)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f vRay, vOrg;
	theApp.scene.GetCameraRay(vRay, vOrg, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);

	Vect3f abs_pos = pos + theApp.scene.GetCenter();
	return SqrDistRayPoint(abs_pos, vRay, vOrg) < _eps;
}
int CD3DWindow::HitTestPoints(const vector<Vect3f> &points, const CPoint &pt)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f vRay, vOrg;
	theApp.scene.GetCameraRay(vRay, vOrg, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);

	vector<Vect3f>::const_iterator it;
	int i=0;
	FOR_EACH(points, it)
	{
		Vect3f abs_pos = (*it) + theApp.scene.GetCenter();
		if(SqrDistRayPoint(abs_pos, vRay, vOrg) < _eps)
			return i;
		i++;
	}
	return -1;
}
KeyPos* CD3DWindow::HitTestPoint(CKeyPos& key_pos, const CPoint& pt, const MatXf* offs /*=NULL*/)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f vRay, vOrg;
	theApp.scene.GetCameraRay(vRay, vOrg, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);

	CKeyPosHermit::iterator it;
	FOR_EACH(key_pos, it)
	{
		Vect3f abs_pos = ((offs) ? *offs*it->pos : it->pos) + theApp.scene.GetCenter();
		if(SqrDistRayPoint(abs_pos, vRay, vOrg) < _eps)
			return &*it;
	}
	return NULL;
}
Vect3f CD3DWindow::Trace(const CPoint& pt)
{
	Vect3f pos;
	CRect rc;
	GetClientRect(rc);
	theApp.scene.Camera2World(pos, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f);
	return pos;
}
void CD3DWindow::SetVect3fPos(Vect3f& key_pos, const CPoint& pt, const MatXf* offs /*=NULL*/)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f v;
	Vect3f pos = offs ? *offs*key_pos : key_pos;
	Vect3f abs_pos = pos + theApp.scene.GetCenter();
	theApp.scene.Camera2World(v, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f, &abs_pos);
	v-=theApp.scene.GetCenter();
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		pos.y = v.y; pos.z = v.z;
		break;
	case CAMERA_PLANE_Y:
		pos.x = v.x; pos.z = v.z;
		break;
	case CAMERA_PLANE_Z:
		pos.y = v.y; pos.x = v.x;
		break;
	}
	if (offs)
		offs->invXformPoint(pos);
	key_pos = pos;
}
void CD3DWindow::SetKeyPos(KeyPos* key_pos, const CPoint& pt, const MatXf* offs /*=NULL*/)
{
	if(key_pos == NULL)
		return;
	CRect rc;
	GetClientRect(rc);
	Vect3f v;
	Vect3f pos = offs ? *offs*key_pos->pos : key_pos->pos;
	Vect3f abs_pos = pos + theApp.scene.GetCenter();
	theApp.scene.Camera2World(v, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f, &abs_pos);
	v-=theApp.scene.GetCenter();
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		pos.y = v.y; pos.z = v.z;
		break;
	case CAMERA_PLANE_Y:
		pos.x = v.x; pos.z = v.z;
		break;
	case CAMERA_PLANE_Z:
		pos.y = v.y; pos.x = v.x;
		break;
	}
	if (offs)
		offs->invXformPoint(pos);
	key_pos->pos = pos;
}
/*
void CD3DWindow::SetSplineKey(const CPoint& pt)
{
	if(m_pSplinePointTrack == 0)
		return;

	MatXf _offs;
	GetDocument()->GetSplineMatrix(_offs);


	CRect rc;
	GetClientRect(rc);

	Vect3f v;
	Vect3f pos = _offs*m_pSplinePointTrack->pos;
	Vect3f abs_pos = pos + theApp.scene.GetCenter();
	theApp.scene.Camera2World(v, float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f, &abs_pos);
	v-=theApp.scene.GetCenter();
	switch(theApp.scene.GetCameraPlane())
	{
	case CAMERA_PLANE_X:
		pos.y = v.y; pos.z = v.z;
		break;
	case CAMERA_PLANE_Y:
		pos.x = v.x; pos.z = v.z;
		break;
	case CAMERA_PLANE_Z:
		pos.y = v.y; pos.x = v.x;
		break;
	}
	_offs.invXformPoint(pos);
	m_pSplinePointTrack->pos = pos;
}
*/
void RotateQuaternion(QuatF& q, const CPoint& pt_delta, CAMERA_PLANE plane)
{
	QuatF qf;
	const float mul=0.01f;

	if(plane == CAMERA_PLANE_X)
	{
		qf.mult(QuatF(pt_delta.x*mul,Vect3f(1,0,0)), q);
		q = qf;
	}
	else if(plane == CAMERA_PLANE_Y)
	{
		qf.mult(QuatF(pt_delta.x*mul,Vect3f(0,1,0)), q);
		q = qf;
	}
	else
	{	
		qf.mult(QuatF(pt_delta.y*mul,Vect3f(0,1,0)), q);
		q = qf;
		qf.mult(QuatF(pt_delta.x*mul,Vect3f(1,0,0)), q);
		q = qf;
	}
}
void CD3DWindow::SetDirectionKey(const CPoint& pt_delta)
{
	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	ASSERT(pEmitter);
	if (pEmitter->IsBase())
	{
//		if(!theApp.scene.bPause)
//				return;

		float fTime = pEmitter->GenerationPointGlobalTime(GetDocument()->m_nCurrentGenerationPoint);
		KeyRotate* r_key = pEmitter->GetBase()->GetOrCreateRotateKey(fTime, 0);

		if(r_key)
			RotateQuaternion(r_key->pos, pt_delta, theApp.scene.GetCameraPlane());
	}else if (pEmitter->IsCLight())
		RotateQuaternion(pEmitter->rot(),	pt_delta, theApp.scene.GetCameraPlane());
}
void CD3DWindow::SetBSDirectionKey(const CPoint& pt_delta)
{
	ASSERT(GetDocument()->ActiveEmitter());

	if(GetDocument()->ActiveEmitter()->IsIntZ())
	{
		EffectBeginSpeed& bs = GetDocument()->ActiveEmitter()->begin_speed()[GetDocument()->m_nCurrentParticlePoint];
		RotateQuaternion(bs.rotation, pt_delta, theApp.scene.GetCameraPlane());
	}
}
void CD3DWindow::MoveToPositionTime(float tm)
{
	float fTime = theApp.scene.GetEffect()->GetTime();

	if(fabs(fTime - tm) < 0.0001)
		return;

	if(tm < fTime)
		theApp.scene.InitEmitters();	

	theApp.scene.GetEffect()->MoveToTime(tm);
}


BEGIN_MESSAGE_MAP(CD3DWindow, CView)
	//{{AFX_MSG_MAP(CD3DWindow)
//	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_KEYDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MBUTTONDOWN()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CD3DWindow message handlers

BOOL CD3DWindow::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CD3DWindow::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ((nFlags&MK_LBUTTON) && !IS_CTRL() && !IS_SHIFT()&& !IS_ALT())
		SetEffectCenter(point);

	if (theApp.scene.m_ToolMode == CD3DScene::TOOL_TARGET)
	{
		CRect rc;
		GetClientRect(rc);
		Vect3f v,pos,dir;
		theApp.scene.m_pActiveCamera->m_pCamera->GetWorldRay(Vect2f(float(point.x)/rc.Width() - 0.5f, float(point.y)/rc.Height() - 0.5f), pos, dir);
		theApp.scene.m_pScene->TraceDir(pos,dir,&v);

		Vect3f start_pos = theApp.scene.GetEffect()->GetPosition().trans();
		vector<Vect3f> vec;
		vec.push_back(v);
		theApp.scene.GetEffect()->SetTarget(start_pos,vec);
	}

	int GPoint= GetDocument()->m_nCurrentGenerationPoint;
	CPoint ptDelta = point - m_ptPrevPoint;
	if ((nFlags & MK_RBUTTON))// && !IS_CTRL()&&!IS_SHIFT())
	{
		CAMERA_PLANE cpl = theApp.scene.GetCameraPlane();
		if((cpl == CAMERA_PLANE_X) || (cpl == CAMERA_PLANE_Y))
			theApp.scene.CameraMove(ptDelta.x*fMouseMoveScale, 0, -ptDelta.y*fMouseMoveScale);
		else
			theApp.scene.CameraMove(ptDelta.x*fMouseMoveScale, ptDelta.y*fMouseMoveScale, 0);
	}
	if(nFlags & MK_MBUTTON)
	{
//		if (!(nFlags & MK_CONTROL))
		{
			theApp.scene.CameraRotate(ptDelta.x*fMouseRotateScale, -ptDelta.y*fMouseRotateScale);
		}
	}
//	if(m_bLeftPressed)
	{
		CAMERA_PLANE cpl = theApp.scene.GetCameraPlane();

/*		if(IS_CTRL())
		{
			if((cpl == CAMERA_PLANE_X) || (cpl == CAMERA_PLANE_Y))
				theApp.scene.CameraMove(ptDelta.x*fMouseMoveScale, 0, -ptDelta.y*fMouseMoveScale);
			else
				theApp.scene.CameraMove(ptDelta.x*fMouseMoveScale, ptDelta.y*fMouseMoveScale, 0);
		}
		else
*/		{
			CEmitterData* emitter = GetDocument()->ActiveEmitter();
			if (emitter)
			{			
				CKeyPos& emit_pos =  emitter->emitter_position();
			switch(theApp.scene.m_ToolMode)
			{
			case CD3DScene::TOOL_SPLINE: 
				if(emitter->Class() == EMC_SPLINE)
				{
					if (nFlags & MK_CONTROL)
					{
						if (nFlags&MK_LBUTTON)
						{
							emitter->spiral_data().GetData(1).mat.trans()+=
								SetAllKeyPos(emitter->p_position(),point,m_ptPrevPoint);
						}
						else if (nFlags & MK_RBUTTON)
						{
							emitter->spiral_data().GetData(1).mat.rot()*=RotateAllKeyPos(emitter->p_position(),
								ptDelta.x*fMouseRotateScale, -ptDelta.y*fMouseRotateScale);
						}
					}
					else if ((nFlags & MK_LBUTTON)&&(IS_SHIFT()||IS_ALT())&& tr->EnableEditPos())
					{
						MatXf offs;
						GetDocument()->GetSplineMatrix(offs);
						ASSERT((UINT)GetDocument()->m_nCurrentParticlePoint<emitter->p_position().size());
						SetKeyPos(&emitter->p_position()[GetDocument()->m_nCurrentParticlePoint], point, &offs);
					}
				}
				break;
			case CD3DScene::TOOL_POSITION:
				if (emitter->IsBase()||emitter->IsLight()||emitter->IsCLight())
				{
					if(nFlags & MK_CONTROL)
					{
						if (nFlags & MK_RBUTTON)
						{
							Mat3f &r = emitter->spiral_data().GetData(0).mat.rot();
							r*=RotateAllKeyPos(emit_pos, ptDelta.x*fMouseRotateScale, -ptDelta.y*fMouseRotateScale);
						}
						else if (nFlags & MK_LBUTTON)
						{
							Vect3f &t = emitter->spiral_data().GetData(0).mat.trans();
							t+=SetAllKeyPos(emit_pos, point, m_ptPrevPoint);
						}
					}				
					else 
					{
						ASSERT((UINT)GetDocument()->m_nCurrentGenerationPoint<emit_pos.size());
						if ((nFlags & MK_LBUTTON)&& (IS_SHIFT() || IS_ALT())&& tr->EnableEditPos())
							SetKeyPos(&emit_pos[GPoint], point);
					}
				}else if (emitter->IsLighting())
				{
					if(nFlags & MK_CONTROL)
					{
						if (nFlags & MK_RBUTTON)
							RotateAround(emitter->pos_end(), emitter->pos_begin(), GetMatRot(ptDelta));
						else if (nFlags & MK_LBUTTON)
						{
							Vect3f dpos = Trace(point) - Trace(m_ptPrevPoint);
							vector<Vect3f>::iterator it;
							FOR_EACH(emitter->pos_end(), it)
								(*it)+=dpos;
							emitter->pos_begin()+=dpos;
						}
					}else if (nFlags & MK_LBUTTON &&IS_SHIFT())
					{
						if (GPoint==0)
							SetVect3fPos(emitter->pos_begin(), point);
						else
							SetVect3fPos(emitter->pos_end()[GPoint-1], point);
					}
				}
				break;

			case CD3DScene::TOOL_TARGET:
				{
					if (emitter->IsLighting())
					{
						if (nFlags & MK_LBUTTON &&IS_SHIFT())
						{
							//emitter->pos_end()
						}
					}
					break;
				}
			case CD3DScene::TOOL_DIRECTION:
				if (nFlags & MK_LBUTTON && IS_SHIFT())
					SetDirectionKey(ptDelta);
				break;

			case CD3DScene::TOOL_DIRECTION_BS:
				if (nFlags & MK_LBUTTON && IS_SHIFT())
					SetBSDirectionKey(ptDelta);
				break;
			}
			}
		}
	}
	tr->SetControlsData(false, true);

	SetWaypointHover(point);

	m_ptPrevPoint = point;
}

void CD3DWindow::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int &GPoint = GetDocument()->m_nCurrentGenerationPoint;
	theApp.scene.SetActiveCamera(point);

	m_ptPrevPoint = point;
	if (!IS_CTRL() && !IS_SHIFT() && !IS_ALT())
		SetEffectCenter(point);

	CEmitterData* emitter = GetDocument()->ActiveEmitter();
	if(emitter && IS_CTRL() == 0)
	{
		CKeyPos& emit_pos =  emitter->emitter_position();
		switch(theApp.scene.m_ToolMode)
		{
		case CD3DScene::TOOL_POSITION:
		{
			if (IS_SHIFT())
			if (emitter->IsBase() || emitter->IsLight()||emitter->IsCLight())
			{
				KeyPos* key  = HitTestPoint(emit_pos, point);
				if (key)
				{
					GetDocument()->m_nCurrentGenerationPoint = std::distance(emit_pos.begin(), key);
					ctrv->OnChangeActivePoint(0,0);//PostMessage(WM_ACTIVE_POINT,0,0);
	//				SetKeyPos(key, point);
				}
			}else if (emitter->IsLighting())
			{
				if (HitTestPoint(emitter->pos_begin(), point))
					GPoint = 0;
				else 
				{
					int i = HitTestPoints(emitter->pos_end(), point);
					if (i!=-1)
						GPoint = i+1;
				}
			}

//			HitPositionKey(point);
//			SetPositionKey(point);
			break;
		}
		case CD3DScene::TOOL_DIRECTION:
			//SetPositionKey(point, true);
			break;
		case CD3DScene::TOOL_SPLINE:
			if (emitter->Class() == EMC_SPLINE&&IS_SHIFT())
			{
				MatXf offs;
				GetDocument()->GetSplineMatrix(offs);
				KeyPos* t = HitTestPoint(emitter->p_position(),point,&offs);
				if (t)
				{
					GetDocument()->m_nCurrentParticlePoint = std::distance(emitter->p_position().begin(),t);
					if (GetDocument()->m_nCurrentParticlePoint>=emitter->p_position().size())
						GetDocument()->m_nCurrentParticlePoint = 0;
					ctrv->m_graph.Update(true);
				}
			}											
			
/*			if(GetDocument()->ActiveEmitter() && (GetDocument()->ActiveEmitter()->Class() == EMC_SPLINE))
			{
				CRect rc;
				GetClientRect(rc);

				MatXf offs;
				m_pSplinePointTrack = HitTestSplinePoint(GetDocument()->ActiveEmitter()->GetSpl()->p_position, 
					float(point.x)/rc.Width() - 0.5f, float(point.y)/rc.Height() - 0.5f, GetDocument()->GetSplineMatrix(offs));
*//*
				if(m_pSplinePointTrack == 0)
				{
					m_pSplinePointTrack = AddSplinePoint(GetDocument()->ActiveEmitter()->GetSpl()->p_position);
					SetSplineKey(point);
				}
*/
//			}
			break;
		}
	}

	m_bLeftPressed = true;

	SetCapture();
}
void CD3DWindow::OnLButtonUp(UINT nFlags, CPoint point) 
{
//	if (theApp.scene.m_ToolMode == CD3DScene::TOOL_SPLINE)
//	m_pSplinePointTrack = 0;

	m_bLeftPressed = false;
	ReleaseCapture();

	if(!IS_CTRL())
		theApp.scene.InitEmitters();
}

void CD3DWindow::OnRButtonDown(UINT nFlags, CPoint point) 
{
	theApp.scene.SetActiveCamera(point);
/*	if(GetDocument()->ActiveEmitter())
		if(GetDocument()->ActiveEmitter()->Class() == EMC_SPLINE)
		{
			CRect rc;
			GetClientRect(rc);

			MatXf offs;
			KeyPos* p = HitTestSplinePoint(GetDocument()->ActiveEmitter()->GetSpl()->p_position, 
				float(point.x)/rc.Width() - 0.5f, float(point.y)/rc.Height() - 0.5f, GetDocument()->GetSplineMatrix(offs));
*/
			/*
			if(p)
				RemoveSplinePoint(GetDocument()->ActiveEmitter()->GetSpl()->p_position, p);
			*/
//		}
}

BOOL CD3DWindow::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	float delta = float(zDelta)/120.0f;

	if (theApp.scene.GetCameraPlane() == CAMERA_PLANE_X || theApp.scene.GetCameraPlane() == CAMERA_PLANE_Y)
		theApp.scene.CameraZoom(delta);
	else
		theApp.scene.CameraZoom(-delta);

	return TRUE;
}

void CD3DWindow::OnSize(UINT nType, int cx, int cy) 
{
	static d=1;
	CView::OnSize(nType, cx, cy);
	if (cx<=0||cy<=0) return;
	if(d)
	{
//		theApp.scene.DoneRenderDevice();
		theApp.scene.InitRenderDevice(this);
		GetDocument()->LoadWorld();
	}
//	return;
	d=0;
	CRect rcWnd;
	GetClientRect(rcWnd);
	theApp.scene.Resize(rcWnd);
}

BOOL CD3DWindow::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if(IS_RBUT())
	{
		SetCursor(m_hCursorHand);
	}
	else
		SetCursor(theApp.LoadStandardCursor(IDC_ARROW));
	
	return TRUE;
}

#define MOVE_EFFECT_STEP 10

void CD3DWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	/*
	if(nChar == VK_DELETE)
	{
		CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
		if(pEmitter)
		{
			KeyPos* pos_key = theApp.scene.GetActiveWayPoint();
			if(pos_key)
				pEmitter->DeletePositionKey(pos_key); 
		}
	}
	*/
	switch(nChar)
	{
	//case VK_DELETE:
	//{
	//	CEmitterData* pEmitter = GetDocument()->ActiveEmitter();
	//	if(pEmitter)
	//	{
	//		KeyPos* pos_key = theApp.scene.GetActiveWayPoint();
	//		if(pos_key)
	//			pEmitter->DeletePositionKey(pos_key);
	//	}
	//}
	//	break;
/*
	case VK_LEFT:
//		if(GetDocument()->ActiveEmitter())
		{
			GetDocument()->MoveEffectCenter(-MOVE_EFFECT_STEP, 0);
		}
		break;
	case VK_RIGHT:
//		if(GetDocument()->ActiveEmitter())
		{
			GetDocument()->MoveEffectCenter(MOVE_EFFECT_STEP, 0);
		}
		break;
	case VK_UP:
//		if(GetDocument()->ActiveEmitter())
		{
			GetDocument()->MoveEffectCenter(0, -MOVE_EFFECT_STEP);
		}
		break;
	case VK_DOWN:
//		if(GetDocument()->ActiveEmitter())
		{
			GetDocument()->MoveEffectCenter(0, MOVE_EFFECT_STEP);
		}
*/
	case VK_DOWN:
		break;
	default:
		break;
	}
}

void CD3DWindow::SetEffectCenter(CPoint pt)
{
	CRect rc;
	GetClientRect(rc);
	Vect3f v,pos,dir;
	theApp.scene.m_pActiveCamera->m_pCamera->GetWorldRay(Vect2f(float(pt.x)/rc.Width() - 0.5f, float(pt.y)/rc.Height() - 0.5f), pos, dir);
	if(!GetDocument()->m_WorldPath.IsEmpty()&&theApp.scene.m_pScene->TraceDir(pos,dir,&v))
	{
		GetDocument()->m_EffectCenter = v;
	}
}

void CD3DWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnRButtonUp(nFlags, point);
}


void CD3DWindow::OnMButtonDown(UINT nFlags, CPoint point)
{
	CView::OnMButtonDown(nFlags, point);
}
