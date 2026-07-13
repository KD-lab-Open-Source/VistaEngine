#pragma once
#include "Timers.h"
#include "MTSection.h"

/// A miniaudio voice. The backend owns it (Sound/AudioBackend.h); this header only
/// passes the handle around, so it stays a forward declaration and miniaudio.h does not
/// leak into the ~100 call sites that include this file.
struct ma_sound;

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
	bool Init();
	void Release();
	Sound* CreateSound(const char* filename, DWORD mode);
	void Update();
	int numberOfPlayingSounds();
	int numberOfUsedSounds();
	void SetGlobalVolume(float volume);
	void EnableSound(bool enable);
	bool IsEnabled();
	float GetGlobalVolume();
	void RecalculateClipDistance();
	void Mute3DSounds(bool mute);
	void StartFade(bool fadeIn,int time=0, bool allSounds=true);
	void SetGameActive(bool active);
	void SetStandbyTime(float time);
	void StopAll();

	/// The miniaudio engine (ma_engine*), for OggPlayer::initLibrary(). Typed void* so
	/// that miniaudio.h stays out of this header.
	void* GetAudioEngine();

protected:
	std::vector<Sound*> sounds_;
	float globalVolume_;
	float globalFadeFactor_;
	bool enable_;
	bool mute3Dsounds_;
	bool gameActive_;
	void MuteAll(bool mute);
	/// Pushes enable_/gameActive_ down to the device as one master mute.
	void ApplyMuteState();
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
	/// The decoded sample, loaded once. A Channel is a playing copy of it, so this one is
	/// never played itself.
	ma_sound* source_;
	std::vector<Channel*> channels_;
	DWORD mode_;
	float minDistance_;
	float maxDistance_;
	float volume_;
	int maxChannels_;
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
	/// This channel's own voice: a copy of Sound::source_, with its own playback position,
	/// volume and 3D placement.
	ma_sound* voice_;
	Sound* sound_;
	bool needDelete_;
	bool isUsed_;
	bool paused_;
	bool isMuted_;
	bool isLooped_;
	bool is3DSound_;
	/// Where the emitter is. It used to live inside the channel's DS3DBUFFER.
	Vect3f position_;
	float minDistance_;
	float maxDistance_;
	/// maxDistance_ squared, so the clip-distance pass can compare against norm2().
	float maxDistance2_;
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
	void Apply3DParameters();
	void Update(float dt);
	void UpdateVolume();
	Vect3f VectorToListener();
	void ChangeVolume(float volume);
	void StartFade(bool fadeIn, int time);
	void BufferPlay(bool fromZero = false);
	void BufferStop(bool immediately=false);
	bool isInFogOfWar();
	void CheckFogOfWar();
	bool isBufferPlaying();
};
