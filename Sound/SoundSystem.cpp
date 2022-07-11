#include "stdafx.h"
#include "Game\Universe.h"
#include "soundSystem.h"
#include "SoundInternal.h"
#include "Game\SoundApp.h"
#include "Console.h"
#include "Profiler.h"
#include "SystemUtil.h"
#include <algorithm>

#pragma message("Automatically linking with dsound.lib") 
#pragma comment(lib, "dsound.lib") 

//static HANDLE hThread=INVALID_HANDLE_VALUE;
//static int b_thread_must_stop=0;
const float DB_SIZE=10000.0f;

long CalcVolume(int vol)
{
	//9.0/(log(2.0)*8) = 1.623031921
	//double t1=9.0*log(double(vol + 1))/(log(2.0)*8) + 1.0;
	double t1=log(double(vol + 1))*1.623031921 + 1.0;
	double t2=log10(t1)*DB_SIZE;

	int v = DSBVOLUME_MIN + round(t2);
	return v;
}
long CalcPan(double vol)
{
	double sgn=vol>0?1:-1;

	vol=fabs(vol);
	if(vol>1)vol=1;
	vol=(1-vol)*255;

	//9.0/(log(2.0)*8) = 1.623031921
	int v = DSBVOLUME_MIN + round(
		log10(
		log(double(vol + 1))*1.623031921 + 1.0
		)*DB_SIZE
		);
	return round(-sgn*v);
}

struct ChannelDistance
{
	ChannelDistance(Channel* ch)
	{
		distance = 0.f;
		channel = ch;
	}

	ChannelDistance()
	{
		distance = 0.f;
		channel = 0;
	}
	float distance;
	Channel* channel;
	inline bool operator()(const ChannelDistance& s1,const ChannelDistance& s2)const
	{
		return s1.distance<s2.distance;
	}

};
/*
DWORD WINAPI SoundThreadProc(LPVOID lpParameter)
{
	SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);
	while(b_thread_must_stop==0)
	{
		{
			SoundSystem* ps=(SoundSystem*)lpParameter;
			MTAuto lock(ps->criticalSection);
			//ps->Update();
		}
		Sleep(10);
	}
	return 0;
}
*/
SoundSystem::SoundSystem()
{
	lpDirectSound_ = NULL;
	globalVolume_ = 1.0f;
	enable_ = true;
	mute3Dsounds_ = false;
	numberOfUsedSounds_ = 0;
	numberOfPlayingSounds_ = 0;
	fadeTime = 0;
	needVolumeUpdate = 0;
	globalFadeFactor_ = 0;
	globalFadeIn = true;
	prevTime_ = 0;
	gameActive_ = false;
	needFade_ = false;
	fadeAllSounds_ = true;
}

SoundSystem::~SoundSystem()
{
	Release();
}

void SoundSystem::Mute3DSounds(bool mute)
{
	//MTAuto lock(criticalSection);
	//if(mute3Dsounds_ == mute || !enable_)
	//	return;
	//mute3Dsounds_ = mute;
	StartFade(!mute,1000,false);
}

void SoundSystem::EnableSound(bool enable)
{
	if(enable == enable_)
		return;
	enable_ = enable;
	//if (!enable)
	//	StopAll();
	MuteAll(!enable_);
}
bool SoundSystem::IsEnabled()
{
	return enable_;
}

void SoundSystem::StopAll()
{
	//MTAuto lock(criticalSection);
	for (int i=0; i<sounds_.size(); i++)
	{
		sounds_[i]->StopAll();
	}
}
void SoundSystem::MuteAll(bool mute)
{
	//MTAuto lock(criticalSection);
	for (int i=0; i<sounds_.size(); i++)
	{
		if(mute)
			sounds_[i]->MuteAll(true);
		else
		if(!mute3Dsounds_||!sounds_[i]->Is3DSound())
		{
			sounds_[i]->MuteAll(false);
		}
	}
}
void SoundSystem::Release()
{
	//MTAuto lock(criticalSection);
	//b_thread_must_stop = 1;
	for (int i=0; i<sounds_.size(); i++)
	{
		SAFE_DELETE(sounds_[i]);
	}
	sounds_.clear();
	sndLibrary.Release();
	SAFE_RELEASE(pListener);
	SAFE_RELEASE(lpDirectSound_);
}

bool SoundSystem::Init(HWND nWnd)
{
	HRESULT hr;
	// Create IDirectSound using the primary sound device
	if( FAILED( hr = DirectSoundCreate8( NULL, &lpDirectSound_, NULL ) ) )
	{
		kdWarning("&SoundSystem",XBuffer(1024, 1) < "Cannot create DirectSoundCreate8"< " ErrorCode: " <= hr);
		return false;
	}

	// Set DirectSound coop level 
	if( FAILED( hr = lpDirectSound_->SetCooperativeLevel( nWnd, DSSCL_PRIORITY ) ) )
	{
		kdWarning("&SoundSystem",XBuffer(1024, 1) < "Cannot SetCooperativeLevel"< " ErrorCode: " <= hr);
		Release();
		return false;
	}

	// Get the primary buffer 
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	DSBUFFERDESC        dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat   = NULL;

	if(FAILED(hr=lpDirectSound_->CreateSoundBuffer(&dsbd,&pDSBPrimary,NULL)))
	{
		kdWarning("&SoundSystem",XBuffer(1024, 1) < "Cannot CreateSoundBuffer primary"< " ErrorCode: " <= hr);
		Release();
		return false;
	}
	struct FORMATS
	{
		int cannels;
		int herz;
		int bits;
	} formats[]=
	{
		{1,22050,8},
		{2,22050,8},
		{1,22050,16},
		{2,22050,16},
		{1,44100,16},
		{2,44100,16},
	};

	for(int i=sizeof(formats)/sizeof(FORMATS)-1;i>=0;i--)
	{
		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
		wfx.wFormatTag      = WAVE_FORMAT_PCM; 
		wfx.nChannels       = formats[i].cannels; 
		wfx.nSamplesPerSec  = formats[i].herz; 
		wfx.wBitsPerSample  = formats[i].bits; 
		wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

		hr = pDSBPrimary->SetFormat(&wfx);
		if(!FAILED(hr))
			break;
	}
	hr = pDSBPrimary->QueryInterface(IID_IDirectSound3DListener8,
		(LPVOID *)&pListener);
	SAFE_RELEASE(pDSBPrimary);
	if(FAILED(hr))
	{
		return false;
	}

	return true;
}

Sound* SoundSystem::CreateSound(const char* filename, DWORD dwCreationFlags)
{
	Sound* sound = new Sound(this);
	if (!sound->CreateSoundFromFile(filename, dwCreationFlags))
	{
		delete sound;
		return NULL;
	}

	sounds_.push_back(sound);
	return sound;
}
void SoundSystem::SetStandbyTime(float time)
{
	standbyTime_ = time;
}

void SoundSystem::Update()
{
	if(!applicationHasFocus())
		return;
	start_timer_auto();
	needVolumeUpdate = false;
	int curTime = xclock();
	float dt = float(curTime-prevTime_)*0.001f;
	if(dt>0.1)
		dt = 0.1f;
	prevTime_ = curTime;

	if(standbyTime_>0)
	{
		standbyTime_ -= dt;
		return;
	}

	if(needFade_)
	{
		globalFadeFactor_ += dt*fadeTime*(globalFadeIn?1:-1);
		if (globalFadeFactor_>1)
		{
			fadeTime = 0;
			globalFadeFactor_ = 1;
			needFade_ = false;
		}
		if(globalFadeFactor_ < 0)
		{
			fadeTime = 0;
			globalFadeFactor_ = 0;
			needFade_ = false;
		}
		needVolumeUpdate = true;
	}
	

	vector<Sound*>::iterator it;
	int total_use=0;
	int total_play=0;
	FOR_EACH(sounds_,it)
	{
		if (*it)
		{
			if((*it)->isFree())
				continue;
			(*it)->Update(dt);
			total_use += (*it)->getUsingSoundsCount();
			total_play += (*it)->getPlayingSoundsCount();
		}
	}
	numberOfPlayingSounds_ = total_play;
	numberOfUsedSounds_ = total_use;
}
void SoundSystem::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
	//MTAuto lock(criticalSection);
	lpDirectSound_->SetSpeakerConfig(dwSpeakerConfig);
}
float SoundSystem::GetGlobalVolume()
{
	//MTAuto lock(criticalSection);
	return globalVolume_;
}
void SoundSystem::StartFade(bool fadeIn,int time, bool allSounds)
{
	if(fadeIn&&!allSounds&&fadeAllSounds_)
		fadeAllSounds_ = true;
	else
		fadeAllSounds_ = allSounds;
	if(time==0)
	{
		globalFadeFactor_ = fadeIn ? 1.0f : 0.0f;
		SetGlobalVolume(globalVolume_);
		return;
	}
	fadeTime = 1000.f/(float)time;
	globalFadeIn = fadeIn;
	needFade_ = true;
}		

void SoundSystem::SetGlobalVolume(float volume)
{
	//MTAuto lock(criticalSection);
	globalVolume_ = volume;
	vector<Sound*>::iterator it;
	FOR_EACH(sounds_,it)
	{
		if (*it)
			(*it)->UpdateVolume();
	}
}
void SoundSystem::RecalculateClipDistance()
{
	start_timer_auto();
	vector<Sound*>::iterator it;
	FOR_EACH(sounds_,it)
	{
		if (*it)
			(*it)->RecalculateClipDistance();
	}
}

Sound::Sound(SoundSystem* system)
{
	system_ = system;
	buffer = NULL;
	volume_ = DSBVOLUME_MAX;
	ds3DBuffer_.dwSize = sizeof(ds3DBuffer_);
	ds3DBuffer_.flMinDistance = 100;
	ds3DBuffer_.flMaxDistance = 500000;
	ds3DBuffer_.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3DBuffer_.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
	ds3DBuffer_.vConeOrientation.x = 0;
	ds3DBuffer_.vConeOrientation.y = 0;
	ds3DBuffer_.vConeOrientation.z = 1;
	ds3DBuffer_.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
	ds3DBuffer_.vVelocity.x = 0;
	ds3DBuffer_.vVelocity.y = 0;
	ds3DBuffer_.vVelocity.z = 0;
	ds3DBuffer_.vPosition.x = 0;
	ds3DBuffer_.vPosition.y = 0;
	ds3DBuffer_.vPosition.z = 0;
	ds3DBuffer_.dwMode = DS3DMODE_NORMAL;
	lenght_ = 0;
	maxChannels_ = 25;
	numUsedChanel = 0;
	numPlayedChanel = 0;
	isFree_ = true;
	stopInFogOfWar_ = true;
	useGlobalFade_ = true;
}

Sound::~Sound()
{
	Release();
};
void Sound::StopAll()
{
	MTAuto lock(criticalSection);	
	vector<Channel*>::iterator it;
	FOR_EACH(channels_,it)
	{
		if (*it)
			(*it)->Stop(true);
	}
}
void Sound::MuteAll(bool mute)
{
	MTAuto lock(criticalSection);	
	vector<Channel*>::iterator it;
	FOR_EACH(channels_,it)
	{
		if (*it)
			(*it)->SetMute(mute);
	}
}

void Sound::UpdateVolume()
{
	MTAuto lock(criticalSection);	
	vector<Channel*>::iterator it;
	FOR_EACH(channels_,it)
	{
		if (*it)
			(*it)->UpdateVolume();
	}

}

void Sound::Release()
{
	MTAuto lock(criticalSection);
	for(int i=0; i<channels_.size(); i++)
	{
		SAFE_DELETE(channels_[i]);
	}
	channels_.clear();
	SAFE_RELEASE(buffer);
}
void Sound::SetVolume(float volume)
{
	volume_ = volume;
}
bool Sound::RestoreBuffer()
{
	HRESULT hr;
	DWORD dwStatus;
	if( FAILED( hr = buffer->GetStatus( &dwStatus ) ) )
		return false;
	if( dwStatus & DSBSTATUS_BUFFERLOST )
	{
		// Since the app could have just been activated, then
		// DirectSound may not be giving us control yet, so 
		// the restoring the buffer may fail.  
		// If it does, sleep until DirectSound gives us control.
		do 
		{
			hr = buffer->Restore();
			if( hr == DSERR_BUFFERLOST )
				Sleep( 10 );
		}
		while( hr = buffer->Restore() );
	}
	return true;
}
DWORD Sound::GetCreationFlags()
{
	DWORD creationFlags = 
		DSBCAPS_GETCURRENTPOSITION2|
		DSBCAPS_CTRLVOLUME|
		DSBCAPS_CTRLFREQUENCY;
	if (mode_ == 0)
	{
		creationFlags |= DSBCAPS_CTRLPAN |DSBCAPS_LOCDEFER;
		return creationFlags;
	}

	if (mode_&SS_3DSOUND)
	{
		creationFlags |= DSBCAPS_CTRL3D | DSBCAPS_LOCDEFER|DSBCAPS_MUTE3DATMAXDISTANCE;
		return creationFlags;
	}

	if (mode_&SS_2DSOUND)
		creationFlags |= DSBCAPS_CTRLPAN|DSBCAPS_LOCDEFER;
	return creationFlags;
}

bool Sound::CreateSoundFromFile(const char* filename, DWORD mode)
{
	mode_ = mode;
	HRESULT hr;
	LPDIRECTSOUNDBUFFER dsBuffer = NULL;
	CWaveFile waveFile;
	DWORD                dwDSBufferSize = NULL;

	VOID*   pDSLockedBuffer      = NULL; // Pointer to locked buffer memory
	DWORD   dwDSLockedBufferSize = 0;    // Size of the locked DirectSound buffer
	DWORD   dwWavDataRead        = 0;    // Amount of data read from the wav file 

	hr = waveFile.Open( filename, NULL, WAVEFILE_READ );
	if (FAILED(hr))
	{
		kdWarning("&SoundSystem",XBuffer(1024, 1) < /*TRANSLATE*/("Невозможно открыть файл : ") < filename);
		return false;
	}

	// Make the DirectSound buffer the same size as the wav file
	dwDSBufferSize = waveFile.GetSize();
	if(dwDSBufferSize  == 0)
		return false;
	lenght_ = dwDSBufferSize;
	// Create the direct sound buffer, and only request the flags needed
	// since each requires some overhead and limits if the buffer can 
	// be hardware accelerated
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize          = sizeof(DSBUFFERDESC);
	dsbd.dwFlags         = GetCreationFlags();//dwCreationFlags;
	dsbd.dwBufferBytes   = dwDSBufferSize;
	dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
	//	dsbd.guid3DAlgorithm = DS3DALG_HRTF_FULL;
	dsbd.lpwfxFormat     = waveFile.m_pwfx;

	// DirectSound is only guarenteed to play PCM data.  Other
	// formats may or may not work depending the sound card driver.
	hr = DSound()->CreateSoundBuffer( &dsbd, &dsBuffer, NULL );
	if( FAILED(hr) )
	{
		kdWarning("&SoundSystem",XBuffer(1024, 1) < /*TRANSLATE*/("Не возможно создать DirectSound буффер : ") < filename < " ErrorCode: " <= hr);
		return false;
	}

	hr = dsBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&buffer);
	dsBuffer->Release();

	if( FAILED(hr) )
		return false;


	// Make sure we have focus, and we didn't just switch in from
	// an app which had a DirectSound device
	if( FAILED( hr = RestoreBuffer() ) ) 
	{
		buffer->Release();
		return false;
	}

	// Lock the buffer down
	if( FAILED( hr = buffer->Lock( 0, dwDSBufferSize, 
		&pDSLockedBuffer, &dwDSLockedBufferSize, 
		NULL, NULL, 0L ) ) )
	{
		buffer->Release();
		return false;
	}

	// Reset the wave file to the beginning 
	waveFile.ResetFile();

	if( FAILED( hr = waveFile.Read( (BYTE*) pDSLockedBuffer,
		dwDSLockedBufferSize, 
		&dwWavDataRead ) ) )
	{
		buffer->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );
		buffer->Release();
		return false;
	}

	buffer->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );

	return true;
}

void Sound::Update(float dt)
{
	std::vector<Channel*> local_channels;
	{
		MTAuto lock(criticalSection);
		local_channels = channels_;
	}
	numUsedChanel = numPlayedChanel = 0;
	int freeCount = 0;
	std::vector<Channel*>::iterator it;
	FOR_EACH(local_channels, it)
	{
		if (*it)
		{
			if((*it)->isFree())
			{
				freeCount++;
				continue;
			}
			(*it)->Update(dt);
			if((*it)->IsPlaying())
				numPlayedChanel++;
			if((*it)->IsUsed())
				numUsedChanel++;
		}
	}
	if(freeCount == local_channels.size())
		isFree_ = true;
}

int SoundSystem::numberOfPlayingSounds()
{
	return numberOfPlayingSounds_;
}

int SoundSystem::numberOfUsedSounds()
{
	return numberOfUsedSounds_;
}

//int Sound::numberOfPlayingChannels() 
//{
//	int total = 0;
//	std::vector<Channel*>::iterator it;
//	FOR_EACH(channels_, it)
//		if((*it)->IsPlaying())
//			++total;
//
//	return total;
//}
//
//int Sound::numberOfUsedChannels() 
//{
//	int total = 0;
//	std::vector<Channel*>::iterator it;
//	FOR_EACH(channels_, it)
//		if((*it)&&(*it)->IsUsed())
//			++total;
//
//	return total;
//}

bool Sound::Is3DSound()
{
	return !(mode_&SS_2DSOUND);
}

void Sound::RecalculateClipDistance()
{
	if (mode_&SS_2DSOUND)
		return;
	if(!system_->enable_)
		return;
	std::vector<Channel*> local_channels;
	{
		MTAuto lock(criticalSection);
		local_channels = channels_;
	}
	vector<ChannelDistance> realPlayed;
	realPlayed.reserve(local_channels.size());
	//FOR_EACH(channels_,it)
	for (int i=0;i<local_channels.size(); i++)
	{
		ChannelDistance chDist(local_channels[i]);
		if (chDist.channel && chDist.channel->IsPlaying())
		{
			chDist.distance = chDist.channel->VectorToListener().norm2();
			realPlayed.push_back(chDist);
			chDist.channel->SetMute(chDist.distance > chDist.channel->maxDistance2_);
		}
	}
	ChannelDistance distCompOp;
	sort(realPlayed.begin(),realPlayed.end(), distCompOp);

	for(int i=0;i<realPlayed.size();i++)
	{
		if(i<maxChannels_)
		{
			realPlayed[i].channel->SetPaused(false);
			realPlayed[i].channel->canPlay_ = true;
		}
		else
		{
			realPlayed[i].channel->SetPaused(true);
			realPlayed[i].channel->canPlay_ = false;
		}
	}
}
Channel* Sound::CreateAndPlayChannel(bool paused)
{
	//MTAuto lock(system_->criticalSection);
	Channel* chnl = FindFreeChannel();
	if (!chnl)
		return NULL;
	chnl->Set3DParameters(ds3DBuffer_);
	chnl->SetUseGlobalFade(useGlobalFade_);
	chnl->SetStopInFogOfWar(stopInFogOfWar_);
	chnl->SetVolume(volume_);
	chnl->isUsed_ = true;
	if (paused)
	{
		chnl->SetPaused(true);
		return chnl;
	}
	chnl->Play();
	return chnl;
}
bool Sound::PlaySound(const Vect3f& pos)
{
	//MTAuto lock(system_->criticalSection);
	Channel* chnl = FindFreeChannel();
	if (!chnl)
		return false;
	chnl->Set3DParameters(ds3DBuffer_);
	chnl->SetUseGlobalFade(useGlobalFade_);
	chnl->SetStopInFogOfWar(stopInFogOfWar_);
	chnl->SetVolume(volume_);
	chnl->isUsed_ = false;
	chnl->SetLoop(false);
	chnl->SetPosition(pos);
	chnl->SetPan(0.5f);
	chnl->Play();
	return true;
}

void Sound::Set3DMinMaxDistance(float min, float max)
{
	ds3DBuffer_.flMinDistance = min;
	ds3DBuffer_.flMaxDistance = max;
}
void Sound::Get3DMinMaxDistance(float* min, float* max)
{
	if (min)
		*min = ds3DBuffer_.flMinDistance;
	if (max)
		*max = ds3DBuffer_.flMaxDistance;
}
void Sound::SetMaxChannels(int maxChannels)
{
	maxChannels_=  maxChannels;
}

Channel* Sound::FindFreeChannel()
{
	MTAuto lock(criticalSection);
	for (int i=0; i<channels_.size(); i++)
	{
		if (!channels_[i]->IsUsed() && !channels_[i]->IsPlaying())
		{
			channels_[i]->Create3dBuffer();
			return channels_[i];
		}
	}
	Channel* channel = new Channel(this);
	if (!channel->Init())
		return NULL;
	channels_.push_back(channel);
	return channel;
}
void Channel::SetPan(float pan)
{
	if (is3DSound_)
		return;
	if (pan<0)pan=0;
	if (pan>1)pan=1;
	float x = 2.f*pan-1.f;
	long pan_ = 0;
	long calcPan = CalcPan(x); 
	buffer_->GetPan(&pan_);
	if(pan_ == calcPan)
		return;
	buffer_->SetPan(CalcPan(x));
}

Channel::Channel(Sound* sound)
{
	needDelete_ = false;
	sound_ = sound;
	isUsed_ = false;
	paused_ = false;
	isLooped_ = false;
	buffer_ = NULL;
	buffer3D_ = NULL;
	is3DSound_ = sound->mode_&SS_3DSOUND;
	minDistance_ = 0.f;
	maxDistance2_ = 0.f;
	volume_ = DSBVOLUME_MAX;
	playing_ = false;
	fadeOut_ = 0;
	fadeIn_ = 0;
	isMuted_ = false;
	fadeTime_ = 0;
	isFadeIn_ = true;
	needStop_ = false;
	fadeVolumeFactor_ = 1;
	inFogOfWar_ = false;
	stopInFogOfWar_ = true;
	useGlobalFade_ = true;
	needFade_ = false;
	isFree_ = true;
	canPlay_ = true;
}

Channel::~Channel()
{
	Release();
	SAFE_RELEASE(buffer_);
	SAFE_RELEASE(buffer3D_);
}
void Channel::Release()
{
	//MTAuto lock(sndSystem.criticalSection);
	isUsed_ = false;
}

bool Channel::Init()
{
	HRESULT hr;
	LPDIRECTSOUNDBUFFER dsBuffer=NULL;
	if (buffer_)
	{
		buffer_->Release();
		buffer_ = NULL;
	}
	hr = sound_->DSound()->DuplicateSoundBuffer(sound_->buffer,&dsBuffer);
	if (SUCCEEDED(hr)) 
	{ 
		hr = dsBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) &buffer_);
		dsBuffer->Release();
	} 
	fadeVolumeFactor_ = 0;
	Create3dBuffer();
	if (FAILED(hr))
		return false;
	return true;
}
void Channel::SetPaused(bool pause)
{
	//MTAuto lock(sndSystem.criticalSection);
	if (!isUsed_ || pause == paused_)
		return;
	if(!isLooped_)
		return;
	paused_ = pause;
	if (pause)
	{
		BufferStop();
	}else
	if(isLooped_)
	{
		BufferPlay();
	}
}
void Channel::SetLoop(bool loop)
{
	//MTAuto lock(sndSystem.criticalSection);
	isLooped_ = loop;
}
void Channel::BufferPlay(bool fromZero)
{
	playing_ = true;
	if(isMuted_)
		return;
	if(!isInFogOfWar())
	{
		if (fadeIn_>0)
		{
			StartFade(true,fadeIn_);
			if(fromZero)
				fadeVolumeFactor_ =0;
		}else
			fadeVolumeFactor_ = 1;
		DWORD flag = isLooped_?DSBPLAY_LOOPING:0;
		flag |= DSBPLAY_TERMINATEBY_DISTANCE;
		buffer_->Play(0,0,flag);
	}else
		inFogOfWar_ = true;
	ChangeVolume(volume_*fadeVolumeFactor_);
}
void Channel::BufferStop(bool immediately)
{
	if (fadeOut_<=0||immediately)
	{
		buffer_->Stop();
		fadeVolumeFactor_ = 0;
	}else
	{
		StartFade(false,fadeOut_);
	}
}

void Channel::Play()
{
	if (!sound_->system_->enable_)
		return;
	//if(hThread==INVALID_HANDLE_VALUE)
	//{
	//	DWORD ThreadId;
	//	hThread=CreateThread(
	//		NULL,
	//		0,
	//		SoundThreadProc,
	//		sound_->system_,
	//		0,
	//		&ThreadId);
	//}
	//MTAuto lock(sndSystem.criticalSection);
	Apply3DParameters();
	buffer_->SetCurrentPosition(0);
	paused_ = false;
	isMuted_ = false;
	playing_ = true;
	if((VectorToListener().norm()>ds3DBuffer_.flMaxDistance)&&is3DSound_)
	{
		SetMute(true);
	}
	//int playCount = 0;

	//for(int i=0; i<sound_->channels_.size(); i++)
	//{
	//	if(sound_->channels_[i]==this)
	//		continue;
	//	if(sound_->channels_[i]->playing_&&!sound_->channels_[i]->paused_)
	//		playCount++;
	//	if(playCount >= sound_->maxChannels_)
	//	{
	//		paused_ = true;
	//		break;
	//	}
	//}
	
	sound_->RecalculateClipDistance();
	if(canPlay_)
		BufferPlay(true);
}
void Channel::StartFade(bool fadeIn, int time)
{
	fadeTime_ = 1000.0f / time;
	isFadeIn_ = fadeIn;
	needFade_ = true;
}

void Channel::Stop(bool immediately)
{
	//MTAuto lock(sndSystem.criticalSection);
	if(!playing_)
		return;
	BufferStop(immediately);
	if (fadeOut_<=0||immediately)
	{
		playing_ = false;
		paused_ = false;
		isMuted_ = false;
	}else
		needStop_ = true;
}
bool Channel::IsUsed()
{
	//MTAuto lock(sndSystem.criticalSection);
	if (!buffer_||!isUsed_)
		return false;
	return true;
}

void Channel::Set3DMinMaxDistance(float min, float max)
{
	maxDistance2_ = max*max;
	ds3DBuffer_.flMinDistance = min;
	ds3DBuffer_.flMaxDistance = max;
	//Apply3DParameters();
}
void Channel::Get3DMinMaxDistance(float* min, float* max)
{
	if (min)
		*min = ds3DBuffer_.flMinDistance;
	if (max)
		*max = ds3DBuffer_.flMaxDistance;
}
void Channel::SetPosition(const Vect3f& pos)
{
	//MTAuto lock(sndSystem.criticalSection);
	ds3DBuffer_.vPosition.x = pos.x;
	ds3DBuffer_.vPosition.y = pos.y;
	ds3DBuffer_.vPosition.z = pos.z;
	if(playing_ && is3DSound_ && buffer3D_)
		Apply3DParameters();
		//buffer3D_->SetPosition(pos.x, pos.y, pos.z, DS3D_DEFERRED);
}
void Channel::GetPosition(Vect3f& pos)
{
	pos.x = ds3DBuffer_.vPosition.x;
	pos.y = ds3DBuffer_.vPosition.y;
	pos.z = ds3DBuffer_.vPosition.z;
}

void Channel::Apply3DParameters()
{
	//MTAuto lock(sndSystem.criticalSection);
	if (is3DSound_ && buffer3D_)
		buffer3D_->SetAllParameters(&ds3DBuffer_,DS3D_DEFERRED);
}

void Channel::Set3DParameters(DS3DBUFFER& ds3DBuffer)
{
	//MTAuto lock(sndSystem.criticalSection);
	ds3DBuffer_ = ds3DBuffer;
	maxDistance2_ = ds3DBuffer.flMaxDistance*ds3DBuffer.flMaxDistance;
	//Apply3DParameters();
}
void Channel::Clear3dBuffer()
{
	//MTAuto lock(sndSystem.criticalSection);
	SAFE_RELEASE(buffer3D_);
	isFree_ = true;
}
void Channel::Create3dBuffer()
{
	if(is3DSound_)
	{
		SAFE_RELEASE(buffer3D_);
		buffer_->QueryInterface(IID_IDirectSound3DBuffer8, 
			(LPVOID *)&buffer3D_); 
	}
	isFree_ = false;
	sound_->isFree_ = false;
}
void Channel::SetMute(bool mute)
{
	if (mute == isMuted_)
		return;
	isMuted_ = mute;
	if (mute)
	{
		BufferStop();
	}else
	if(isUsed_&&isLooped_)
	{
		BufferPlay();
	}
	
}
void Channel::ChangeVolume(float volume)
{
	float fVol = sound_->system_->globalVolume_*volume;
	if(useGlobalFade_||sound_->system_->fadeAllSounds_)
		fVol *= sound_->system_->globalFadeFactor_;
	long vol = CalcVolume(int(fVol*255.0f));
	if (vol<DSBVOLUME_MIN)
		vol = DSBVOLUME_MIN;
	long curVol = 0;
	buffer_->GetVolume(&curVol);
	if (curVol == vol)
		return;
	buffer_->SetVolume(vol);
}
bool Channel::isInFogOfWar()
{
	if(sound_->system_->gameActive_&&stopInFogOfWar_&&universe()->activePlayer()->fogOfWarMap()&&is3DSound_&&
	   universe()->activePlayer()->fogOfWarMap()->getFogState(ds3DBuffer_.vPosition.x,ds3DBuffer_.vPosition.y) != FOGST_NONE)
	{
		return true;
	}
	return false;
}
void Channel::CheckFogOfWar()
{
	if(!sound_->system_->gameActive_)
		return;
	if(isInFogOfWar()&&!inFogOfWar_)
	{
		BufferStop();
		inFogOfWar_ = true;
	}
	if(!isInFogOfWar()&&inFogOfWar_)
	{
		if(isUsed_&&isLooped_)
		{
			BufferPlay(false);
		}
		inFogOfWar_ = false;
	}
}

void Channel::Update(float dt)
{
	start_timer_auto();
	if(!buffer3D_&&is3DSound_)
		return;
	if(sound_->system_->needVolumeUpdate)
		UpdateVolume();

	CheckFogOfWar();
	if(needFade_)
	{
		fadeVolumeFactor_ += (dt*fadeTime_)*(isFadeIn_?1:-1);
		if(fadeVolumeFactor_ < 0)
		{
			fadeTime_ = 0;
			needFade_ = false;
			fadeVolumeFactor_ = 0;
			buffer_->Stop();
			if(needStop_)
				playing_ = false;
			needStop_ = false;
		}
		if(fadeVolumeFactor_ > 1)
		{
			fadeTime_ = 0;
			needFade_ = false;
			fadeVolumeFactor_ = 1;
		}
		ChangeVolume(volume_*fadeVolumeFactor_ );
	}
	if (playing_&&!paused_&&!isMuted_&&!inFogOfWar_)
	{
		if (!isBufferPlaying())
			playing_ = false;
	}
	if(playing_&&(paused_||isMuted_||inFogOfWar_)&&!isLooped_)
		playing_ = false;
	if(playing_&&!isUsed_&&isLooped_&&!needFade_)
	{
		playing_ = false;
	}
	if (!playing_&&!isUsed_)
	{
		Clear3dBuffer();
	}
}
bool Channel::isBufferPlaying()
{
	DWORD status;
	buffer_->GetStatus(&status);
	return status&DSBSTATUS_PLAYING;
}

void Channel::SetVolume(float volume)
{
	//MTAuto lock(sndSystem.criticalSection);
	volume_ = volume;
	ChangeVolume(volume);
}
void Channel::UpdateVolume()
{
	SetVolume(volume_);
}
bool Channel::IsPlaying()
{
	return playing_;
}
Vect3f Channel::VectorToListener()
{
	//MTAuto lock(sndSystem.criticalSection);
	D3DVECTOR posListener;
	if (!sound_->system_->pListener)
		return Vect3f::ID;
	sound_->system_->pListener->GetPosition(&posListener);
	Vect3f pos(ds3DBuffer_.vPosition.x - posListener.x,
			   ds3DBuffer_.vPosition.y - posListener.y,
			   ds3DBuffer_.vPosition.z - posListener.z);
	return pos;
}
