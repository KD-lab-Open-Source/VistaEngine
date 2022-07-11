#include "StdAfx.h"
#include "Sound.h"
#include "SoundInternal.h"
#include "C3D.h"

using namespace SND;
#define RDCALL(x) {HRESULT hr=x;ASSERT(SUCCEEDED(hr));}


HardSound3D::HardSound3D()
{
	pBuffer3d=NULL;
}

HardSound3D::~HardSound3D()
{
	SAFE_RELEASE(pBuffer3d);
}

bool HardSound3D::Init(LPDIRECTSOUNDBUFFER ptr)
{
	if(!VirtualSound3D::Init(ptr))
		return false;
	HRESULT hr = pSound->QueryInterface(IID_IDirectSound3DBuffer8, 
                (LPVOID *)&pBuffer3d); 
	if(FAILED(hr))
		return false;
	return true;
}

float HardSound3D::GetMaxDistance()
{
	float dist;
	RDCALL(pBuffer3d->GetMaxDistance(&dist));
	return dist;
}

float HardSound3D::GetMinDistance()
{
	float dist;
	RDCALL(pBuffer3d->GetMinDistance(&dist));
	return dist;
}

bool HardSound3D::SetMaxDistance(float dist)
{
	RDCALL(pBuffer3d->SetMaxDistance(dist,DS3D_DEFERRED));
	return true;
}

bool HardSound3D::SetMinDistance(float dist)
{
	RDCALL(pBuffer3d->SetMinDistance(dist,DS3D_DEFERRED));
	return true;
}

Vect3f HardSound3D::GetPosition()
{
	return position;
}

bool HardSound3D::SetPosition(const Vect3f& pos_)
{
	position=pos_;
	RDCALL(pBuffer3d->SetPosition(position.x, position.y, position.z,DS3D_DEFERRED));
	return true;
}

Vect3f HardSound3D::GetVelocity()
{
	return velocity;
}

bool HardSound3D::SetVelocity(const Vect3f& vel_)
{
	velocity=vel_;
	RDCALL(pBuffer3d->SetVelocity(velocity.x, velocity.y, velocity.z,DS3D_DEFERRED));
	return true;
}

bool HardSound3D::SetVolume(float volume)
{
	HRESULT hr;
	long vol=ToDirectVolumef(volume);
	FAIL(hr=pSound->SetVolume(vol));
	return true;
}

void HardSound3D::RecalculatePos()
{
	//Vect3f pos=position-snd_listener.position;
	//pos.z*=snd_listener.zmultiple;
	//pos=snd_listener.rotate*pos;
	//float flDistSqrd=pos.norm2();

	//float volume_scale;
	//float flDist = sqrtf( flDistSqrd );
	//volume_scale = (1.0f/flDist-1.0f/max_distance)/(1.0f/min_distance-1.0f/max_distance);
	//float real_volume = set_volume*volume_scale;
	//long vol=ToDirectVolumef(real_volume);
	//FAIL(pSound->SetVolume(vol));

	Mute(is_muted || is_paused);
}

void HardSound3D::RecalculateVolume()
{
	RecalculatePos();
}
