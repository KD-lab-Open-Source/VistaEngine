#include "stdafx.h"
#include "RenderObjects.h"
#include "..\Render\Client\WinVideo.h"
#include "..\\Sound\\sound.h"
#include "ShowHead.h"
#include "DebugUtil.h"
#include "SoundApp.h"
#include "..\Units\GlobalAttributes.h"

Singleton<ShowHead> showHead;

ShowHead::ShowHead()
: rect(0, 0, 0, 0)
, head(NULL)
, camera(NULL)
, scene(NULL)
{
	release();
}

ShowHead::~ShowHead()
{
	dassert(!head && !scene && !camera);
}

void ShowHead::init()
{
	scene = gb_VisGeneric->CreateScene();
	xassert(scene);
	scene->SetSun(Vect3f(0,-1,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
	camera = scene->CreateCamera();
	xassert(camera);
	camera->SetAttr(ATTRCAMERA_PERSPECTIVE|ATTRCAMERA_CLEARZBUFFER);
	SetWndPos(sRectangle4f(0,0,1,1));
	//	camera->SetFrustum(&Vect2f(0.5,0.5),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),&Vect2f(1.0f/sqrt(2.f),1.0f/sqrt(2.f)),&Vect2f(10.0f,3000.0f));
	camera->SetPosition(MatXf(Mat3f(-M_PI/2, X_AXIS),Vect3f(0,-100,1000)));
}

void ShowHead::release()
{
	RELEASE(head);
	RELEASE(camera);
	RELEASE(scene);

	needDraw = false;
	phase = 0;
	//pause_chain = "silence";
	ix_main_group = -1;

	ix_camera = -1;
	ix_light = -1;
	id_curren_chain = -1;
	id_prev_chain = -1;
	playing=  false;
	cycled_ = false;
	focus_.set(1,1);
	phasePrev = 0;
	phaseInterpolation = 0;

}

void ShowHead::SetWndPos(const sRectangle4f& pos)
{
	if (rect.min.eq(pos.min) && rect.max.eq(pos.max))
		return;
	rect = pos;
	//Vect2f center((pos.xmax()+pos.xmin())/2,(pos.ymax()+pos.ymin())/2);
	//sRectangle4f rel_pos(pos.xmin() - center.x,pos.ymin() - center.y, pos.xmax() - center.x,pos.ymax() - center.y);
	if (camera)
		//camera->SetFrustum(&center,	&rel_pos, &focus_, &Vect2f(10.0f,100000.0f));
		camera->SetFrustumPositionAutoCenter(rect,focus_.x);
}
void ShowHead::resetHead()
{
	RELEASE(head);
	model_name = "";
}
bool ShowHead::LoadHead(const char* model_name, const char* main_chain, sColor4c skin_color)
{
	playing = false;
	if(!model_name)
	{
		resetHead();
		return false;
	}

	if (this->model_name != model_name)
	{
		resetHead();
		head = scene->CreateObject3dx(model_name, NULL, GlobalAttributes::instance().enableAnimationInterpolation);
		this->model_name = model_name;
		if (!head)
		{
			this->model_name = "";
			return false;
		}
		if(head->GetFov()!=0)
		{
			float fov = head->GetFov();
			focus_.set(1.0f/(2*tan(fov*0.5f)),1.0f/(2*tan(fov*0.5f)));
		}else
			focus_.set(1.0f/sqrt(2.f),1.0f/sqrt(2.f));
		rect.set(0,0,0,0);
		id_prev_chain = -1;
	}
	head->SetSkinColor(skin_color);
	ix_main_group = head->GetAnimationGroup("main");
	if (ix_main_group)
	{
		xassert(0&& "Не найдена анимационная группа main");
		RELEASE(head);
		return false;
	}
	id_curren_chain = head->GetChainIndex(main_chain);
	if (id_curren_chain ==-1)
	{
		current_chain_len = 0;
		xassert(0&&"не найдена анимационная цепочка");
		RELEASE(head);
		return false;
	}else
	{
		cAnimationChain* chain = head->GetChain(id_curren_chain);
		xassert(chain);
		prev_chain_len = current_chain_len;
		current_chain_len = 1.0f/chain->time;
	}
	//mode = ShowHead_Silence;
	ix_camera = head->FindNode("Camera01");
	ix_light = head->FindNode("FDirect01");
	MoveCamera();
	return true;
}
bool ShowHead::Play(bool cycled)
{
	cycled_ = cycled;
	if (!head)
	{
		return false;
	}
	if(id_curren_chain == -1)
		return false;
	if(id_prev_chain == -1 || id_prev_chain==id_curren_chain)
	{
		head->SetAnimationGroupChain(ix_main_group, id_curren_chain);
		head->SetPhase(0);
		phase = 0;
		interpolated_ = false;
	}else
	{
		if(GlobalAttributes::instance().enableAnimationInterpolation)
		{
			head->GetInterpolation()->SetAnimationGroupChain(ix_main_group, id_prev_chain);
			head->SetAnimationGroupChain(ix_main_group, id_curren_chain);
			phasePrev = phase;
			head->GetInterpolation()->SetPhase(phasePrev);
			head->GetInterpolation()->SetAnimationGroupInterpolation(ix_main_group,1.f);
			head->SetPhase(0);
			phase = 0;
			phaseInterpolation = 1;
			interpolated_ = true;
		}
	}
	id_prev_chain = id_curren_chain;
	//head->SetAnimationGroupChain(ix_main_group, id_main_chain);
	//head->SetAnimationGroupPhase(ix_main_group, 0);
	//phase = 0;
	playing = true;
	return true;
}
bool ShowHead::IsPlay()
{
	//return mp.IsPlay() == MPEG_PLAY;
	return playing;
}

void ShowHead::quant(float dt)
{
	if (head)
	{
  		scene->SetDeltaTime(dt*1000);
		phase+=dt*current_chain_len;
		if(phase>1)
			if(cycled_)
				phase-=1;
			else
				phase = 1;
		if(interpolated_)
		{
			phasePrev+=dt*prev_chain_len;
			phaseInterpolation-=dt;
			if(phasePrev>1)
				if(cycled_)
					phasePrev-=1;
				else
					phasePrev = 1;
			if(phaseInterpolation<0)
			{
				phaseInterpolation =0;
				interpolated_ = false;
			}
			head->GetInterpolation()->SetPhase(phasePrev);
			head->GetInterpolation()->SetAnimationGroupInterpolation(ix_main_group,phaseInterpolation);
		}
		head->SetPhase(phase);

		MoveCamera();
	}
}

void ShowHead::draw()
{
	if(!NeedDraw()||!head)
		return;
	scene->Draw(camera);
	ToggleDraw(false);
}

void ShowHead::MoveCamera()
{
	if (ix_camera!=-1)
	{
		MatXf cam(head->GetNodePosition(ix_camera));

		Mat3f rot(Vect3f(1,0,0), M_PI);
		cam.rot()=cam.rot()*rot;
		cam.Invert();

		camera->SetPosition(cam);
	}
	if (ix_light!=-1)
	{
		MatXf m;
		m.set(head->GetNodePosition(ix_light));
		scene->SetSunDirection(m.rot().zrow());
	}
}
