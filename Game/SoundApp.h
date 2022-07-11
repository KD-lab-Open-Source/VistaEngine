#ifndef __SOUND_APP_H__
#define __SOUND_APP_H__

#include "StaticMap.h"
#include <map>
#include "..\Units\SoundAttribute.h"

class MpegSound;
class SoundSystem;
class SoundAttribute;

class VoiceManager
{
public:
	VoiceManager();
	~VoiceManager();

	bool Play(const char* soundTrack, bool cycled = false, bool canPaused = true, bool always = false);
	void Pause();
	void Resume();
	void Stop();
	
	void setEnabled(bool enable);
	void SetVolume(float f);
	bool FadeVolume(float time,int newVolume);
	
	bool enabled() const { return enabled_; }
	bool isPlaying() const;
	bool isPaused() const;
	
	const char* voiceFile() { return soundTrack_.c_str(); }

	bool validatePlayingFile(const VoiceAttribute& voice);

private:
	string soundTrack_;
	MpegSound* mpeg;
	XZipStream stream_;
	bool enabled_;
	bool canPaused_;
};

class MusicManager
{
public:
	MusicManager();
	~MusicManager();
	
	void Release();

	void OpenWorld();
	void CloseWorld();

	bool Play(const char *str = 0);
	void Stop();
	void Enable(int enable, bool gameActive);
	void SetVolume(float f);
	bool isPlaying() const;
	bool isPaused() const;
	void Pause();
	void Resume();
	bool FadeVolume(float time,int newVolume = 0);

private:
	string soundTrack_;
	bool active;
	MpegSound* mpeg;
};

extern MpegSound mpegSound;
extern MusicManager musicManager;
extern VoiceManager voiceManager;

class Sound;
class Channel;

class SoundLibrary
{
	struct ltstr
	{
		bool operator()(LPCSTR s1,LPCSTR s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};
	typedef StaticMap<LPCSTR, Sound*,ltstr> SoundArray;
public:
	SoundLibrary();
	~SoundLibrary();

	bool AddSound(LPCSTR name,Sound* sound);
	Sound* FindSound(LPCSTR name);
protected:
	SoundArray sounds_;
};

class SNDSound
{
public:
	SNDSound();
	~SNDSound();
	bool Init(const SoundAttribute* soundAttr);
	bool Play(bool cycled=true);
	bool Stop(bool immediately = false);
	bool IsPlayed();

	void SetPos(const Vect3f& pos);//Обязательно вызвать до Play
	void SetVelocity(const Vect3f& velocity);
	void SetVolume(float vol);//0..1 учитывает volmin и volume
	void SetPan(float pan);
	void SetMute(bool mute);

	////ScriptFrequency - установить относительную 
	//bool SetFrequency(float frequency);//0..2 - 0 - минимальная, 1 - по умолчанию

	////SetFrequency - frequency=1..44100 Гц, оригинальная - 0
	//bool SetRealFrequency(DWORD frequency);
	void Destroy();
protected:
	Channel* channel;
	const SoundAttribute* soundAttr_;
};

class SoundEnvironmentManager
{
public:
	SoundEnvironmentManager();
	~SoundEnvironmentManager();

	bool PlaySound(SoundReference &sndRef);
	bool StopSound(SoundReference &sndRef);
protected:
	multimap<string,SNDSound*> playSounds;
};

void InitSound(bool sound, bool music, HWND hwnd, const char* localeDataPath);
void UpdateSound();
void ApplySoundParameters();
void SoundQuant();
void FinitSound();

extern bool terSoundEnable;		// 0,1
extern bool terMusicEnable;		// 0,1
extern float terSoundVolume;	// 0..1
extern float terMusicVolume;	// 0..1
extern float terVoiceVolume;	// 0..1
extern SoundLibrary sndLibrary;


#endif //__SOUND_APP_H__
