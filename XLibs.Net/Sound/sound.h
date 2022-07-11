#pragma once
#include <dsound.h>

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }


class CSound
{
	friend CSound* DDLoadSound(LPCSTR fxname);
	friend CSound* FindSound(LPCSTR name,bool one,bool* playone);

	LPDIRECTSOUNDBUFFER buffer;
public:
	CSound();
	~CSound();
	void Play(bool looped=false);
	void Stop(){if(buffer)buffer->Stop();}
	bool IsPlaying();

	//vol=0..-10000
	void SetVolume(long vol);

	//pan=-10000..10000
	void SetPan(long pan);
};

CSound* DDLoadSound(LPCSTR name);
bool DDPlaySound(LPCSTR fname,long vol=DSBVOLUME_MAX,bool one=false,long pan=0);

HRESULT InitDirectSound(HWND g_hWnd);
void ReleaseDirectSound();

void ClearAutoSound();//Удалить все звуки, звучащие автоматически

//Get Set GlobalVolume 0 - нет звука, 1 - максимальный
float GetGlobalVolume();
void SetGlobalVolume(float g);

void SoundEnable(bool b);
bool IsSoundEnabled();

bool DDCache(LPCSTR fname);

#include "PlayMpeg.h"
