#ifndef __SOUND_APP_H__
#define __SOUND_APP_H__

#include "XTL/StaticMap.h"
#include <map>
#include "Units/SoundAttribute.h"

class OggPlayer;
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
	/// The window gained or lost focus. The narration is streamed on miniaudio's own thread
	/// while the text it belongs to is advanced by the frame loop, which stops dead when the
	/// window goes to the background -- so the two only stay together if the voice stops too.
	/// Composed with Pause()/Resume() rather than overwriting them: either one holds the voice.
	void SetApplicationActive(bool active);
	void Stop();
	
	void setEnabled(bool enable);
	void SetVolume(float f);
	/// new_volume - относительная громкость, [0, 1]
	bool FadeVolume(float time, float newVolume);
	
	bool enabled() const { return enabled_; }
	bool isPlaying() const;
	bool isPaused() const;
	
	const char* voiceFile() { return soundTrack_.c_str(); }

	bool validatePlayingFile(const VoiceAttribute& voice);

private:
	/// Push gamePaused_ and applicationActive_ down to the player as the one pause it has.
	void applyPause();

	string soundTrack_;
	OggPlayer* mpeg;
	bool enabled_;
	bool canPaused_;
	bool gamePaused_;
	bool applicationActive_;
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
	bool FadeVolume(float time, float newVolume = 0);

private:
	string soundTrack_;
	bool active;
	OggPlayer* mpeg;
};

extern OggPlayer mpegSound;
extern MusicManager musicManager;
VoiceManager& voiceManager();

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
	void Release();
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

	bool playSound(SoundReference &sndRef);
	bool stopSound(SoundReference &sndRef);
protected:
	multimap<string,SNDSound*> playSounds;
};

void InitSound(bool sound, bool music, const char* localeDataPath);
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
