#pragma once
#include <mmsystem.h>
#include <dsound.h>
#include "..\Util\Timers.h"
#include "MTSection.h"

class Sound;
class Channel;

#define SS_DEFAULT 0x00000000
#define SS_3DSOUND 0x00000001
#define SS_2DSOUND 0x00000002
#define SS_MUTEBYMAXDISTANCE 0x00000004

class SoundSystem
{
	friend Sound;
	friend Channel;
public:
	SoundSystem();
	~SoundSystem();
	bool Init(HWND hWnd);
	void Release();
	Sound* CreateSound(const char* filename, DWORD mode);
	void Update();
	int numberOfPlayingSounds();
	int numberOfUsedSounds();
	void SetSpeakerConfig(DWORD dwSpeakerConfig);
	void SetGlobalVolume(float volume);
	LPDIRECTSOUND3DLISTENER8 pListener;
	LPDIRECTSOUND8 GetDirectSound(){return lpDirectSound_;}
	void EnableSound(bool enable);
	bool IsEnabled();
	float GetGlobalVolume();
	void RecalculateClipDistance();
	void Mute3DSounds(bool mute);
	void StartFade(bool fadeIn,int time=0, bool allSounds=true);
	void SetGameActive(bool active) {gameActive_ = active;}
	void SetStandbyTime(float time);
	void StopAll();

protected:
	LPDIRECTSOUND8 lpDirectSound_; 
	std::vector<Sound*> sounds_;
	float globalVolume_;
	float globalFadeFactor_;
	bool enable_;
	bool mute3Dsounds_;
	bool gameActive_;
	void MuteAll(bool mute);
	int numberOfUsedSounds_;
	int numberOfPlayingSounds_;
	float fadeTime;
	bool needVolumeUpdate;
	bool globalFadeIn;
	int prevTime_;
	float standbyTime_;
	bool needFade_;
	bool fadeAllSounds_;
};

extern SoundSystem sndSystem;

class Sound
{
	friend SoundSystem;
	friend Channel;
public:
	Sound(SoundSystem* system);
	~Sound();
	bool CreateSoundFromFile(const char* filename, DWORD mode);
	bool RestoreBuffer();
	void Release();

	//int numberOfPlayingChannels();
	//int numberOfUsedChannels();
	
	//void Play(bool paused = true, Channel** channel=NULL);
	Channel* CreateAndPlayChannel(bool paused = true);
	bool PlaySound(const Vect3f& pos);
	
	void Set3DMinMaxDistance(float min, float max);
	void Get3DMinMaxDistance(float* min, float* max);
//	void SetPosition(const Vect3f& pos);
//	void GetPosition(D3DVECTOR* pos);
	void SetVolume(float volume);
	void SetMaxChannels(int maxChannels);
	bool Is3DSound();
	bool isFree(){return isFree_;}
	void SetStopInFogOfWar(bool stop) {stopInFogOfWar_ = stop;}
	void SetUseGlobalFade(bool use){useGlobalFade_ = use;}
protected:
	SoundSystem* system_;
	LPDIRECTSOUNDBUFFER8 buffer;
	std::vector<Channel*> channels_;
	DWORD mode_;
	DS3DBUFFER ds3DBuffer_;
	float volume_;
	DWORD lenght_;
	int maxChannels_;
	LPDIRECTSOUND8 DSound(){return system_->lpDirectSound_;}
	DWORD GetCreationFlags();
	Channel* FindFreeChannel();
	void Update(float dt);
	void UpdateVolume();
	void RecalculateClipDistance();
	void StopAll();
	void MuteAll(bool mute);
	int numUsedChanel;
	int numPlayedChanel;
	int getPlayingSoundsCount(){return numPlayedChanel;}
	int getUsingSoundsCount(){return numUsedChanel;}
	bool isFree_;
	bool stopInFogOfWar_;
	bool useGlobalFade_;
	MTSection criticalSection;
};
class Channel
{
	friend SoundSystem;
	friend Sound;
public:
	Channel(Sound* sound);
	~Channel();
	bool Init();
	void Play();
	void Stop(bool immediately=false);
	bool IsUsed();
	void SetPaused(bool pause);
	void SetLoop(bool loop);
	void Release();
	void Set3DMinMaxDistance(float min, float max);
	void Get3DMinMaxDistance(float* min, float* max);
	void SetPosition(const Vect3f& pos);
	void GetPosition(Vect3f& pos);
	void SetVolume(float volume);
	void SetPan(float pan);
	void SetFadeOut(int fade){fadeOut_ = fade;}
	void SetFadeIn(int fade){fadeIn_ = fade;}
	bool IsPlaying();
	void SetMute(bool mute);
	void SetStopInFogOfWar(bool stop) {stopInFogOfWar_ = stop;}
	void SetUseGlobalFade(bool use){useGlobalFade_ = use;}
	bool isFree() const {return isFree_;}
protected:
	LPDIRECTSOUNDBUFFER8 buffer_;
	LPDIRECTSOUND3DBUFFER8 buffer3D_;
	Sound* sound_;
	bool needDelete_;
	bool isUsed_;
	bool paused_;
	bool isMuted_;
	bool isLooped_;
	bool is3DSound_;
	float minDistance_;
	float maxDistance2_;
	DS3DBUFFER ds3DBuffer_;
	float volume_;
	bool playing_;
	int fadeOut_;
	int fadeIn_;
	float fadeTime_;
	bool isFadeIn_;
	bool needStop_;
	float fadeVolumeFactor_;
	bool inFogOfWar_;
	bool stopInFogOfWar_;
	bool useGlobalFade_;
	bool needFade_;
	bool isFree_;
	bool canPlay_;
	void Set3DParameters(DS3DBUFFER& ds3DBuffer_);
	void Apply3DParameters();
	void Update(float dt);
	void UpdateVolume();
	Vect3f VectorToListener();
	void Clear3dBuffer();
	void Create3dBuffer();
	void ChangeVolume(float volume);
	void StartFade(bool fadeIn, float time);
	void BufferPlay(bool fromZero = false);
	void BufferStop(bool immediately=false);
	bool isInFogOfWar();
	void CheckFogOfWar();
	bool isBufferPlaying();
};
