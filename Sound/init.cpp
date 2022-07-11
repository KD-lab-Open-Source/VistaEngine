#include "stdafx.h"
#include "Sound.h"
#include "SoundInternal.h"
//#include "c3d.h"
#include "SoundSystem.h"
#include "Profiler.h"


SoundSystem sndSystem;
SND3DListener snd_listener;

void* SNDGetDirectSound()
{
	return sndSystem.GetDirectSound();
}

void SNDEnableSound(bool enable)
{
	sndSystem.EnableSound(enable);
}

bool SNDIsSoundEnabled()
{
	return sndSystem.IsEnabled();
}

bool SNDInitSound(HWND hWnd,bool bEnable3d,bool _soft3d)
{
	//SNDReleaseSound();
	if (!sndSystem.Init(hWnd))
		return false;
	SNDEnableSound(true);
	return true;
}

void SNDReleaseSound()
{
	sndSystem.Release();
}

void SNDSetFade(bool fadeIn,int time)
{
	sndSystem.StartFade(fadeIn,time);
}
void SNDStopAll()
{
	sndSystem.StopAll();
}
void SNDSetVolume(float volume)
{
	sndSystem.SetGlobalVolume(volume);
}

float SNDGetVolume()
{
	return sndSystem.GetGlobalVolume();
}

void SNDSetGameActive(bool active)
{
	sndSystem.SetGameActive(active);
}

///////////////////////SND3DListener////////////////////////
SND3DListener::SND3DListener()
:velocity(0,0,0)
{
	rotate=invrotate=Mat3f::ID;
	position.x=position.y=position.z=0;
	//mat=MatXf::ID;
	s_distance_factor=1.0;//DS3D_DEFAULTDISTANCEFACTOR
	s_doppler_factor=1.0;//DS3D_DEFAULTDOPPLERFACTOR
	s_rolloff_factor=1.0;//DS3D_DEFAULTROLLOFFFACTOR

	velocity.x=velocity.y=velocity.z=0;

	front.x=0;
	front.y=0;
	front.z=1.0;
	top.x=0;
	top.y=1.0;
	top.z=0;
	right.x=-1.0;
	right.y=0;
	right.z=0;

	zmultiple=1.0f;
}

SND3DListener::~SND3DListener()
{
}

bool SND3DListener::SetDistanceFactor(float f)
{
	s_distance_factor=f;
	return true;
}

bool SND3DListener::SetDopplerFactor(float f)
{
	s_doppler_factor=f;
	return true;
}

bool SND3DListener::SetRolloffFactor(float f)
{
	s_rolloff_factor=f;
	return true;
}

bool SND3DListener::SetPos(const MatXf& _mat)
{
	Vect3f t=_mat.trans();

	rotate=_mat.rot();
	invrotate.invert(rotate);
	position=invrotate*t;
	position=-position;

	front=invrotate*Vect3f(0,0,1.0f);;
	top=invrotate*Vect3f(0,1.0f,0);
	right=invrotate*Vect3f(-1.0,0,0);

	return true;
}

bool SND3DListener::SetVelocity(const Vect3f& _velocity)
{
	velocity=_velocity;
	return true;
}

bool SND3DListener::Update()
{
	start_timer_auto();
	HRESULT hr;
	if(sndSystem.pListener)
	{
		/*
		Сильно тормозит, поэтому пока заремил*/
		hr=sndSystem.pListener->SetPosition(position.x,position.y,position.z,DS3D_DEFERRED);
		hr=sndSystem.pListener->SetVelocity(velocity.x,velocity.y,velocity.z,DS3D_DEFERRED);
		hr=sndSystem.pListener->SetOrientation(
			front.x, front.y, front.z,
			top.x, top.y, top.z, 
			DS3D_DEFERRED);
		hr=sndSystem.pListener->CommitDeferredSettings();/**/
	}
	return true;
}

