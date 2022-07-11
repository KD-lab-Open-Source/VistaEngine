#include "StdAfx.h"
#include "Sound.h"
#include "SoundSystem.h"
#include "SoundApp.h"
#include "SystemUtil.h"
#include "CameraManager.h"
#include "RenderObjects.h"
#include "PlayOgg.h"
#include "terra.h"
#include "UnitAttribute.h"
#include "SoundScript.h"
#include "ResourceSelector.h"
#include "XPrmArchive.h"
#include "EditArchive.h"
#include "GameOptions.h"
#include "..\Util\Console.h"
#include "..\Util\FileUtils.h"

bool terSoundEnable = true;
bool terMusicEnable = true;
float terSoundVolume = 1;
float terMusicVolume = 1;
float terVoiceVolume = 1;
float fSoundZMultiple = 0.5f; // 0..1 коэффициэнт масштабирования громкости по оси Z

// Добавляет слэш в конце пути к дирректории если нужно
void SlashFixup(string& dir)
{
	if ((0 != dir.size()) && ('\\' != dir[dir.size()-1]))
		dir += '\\';
}

void LoadAllSound(const char* localeDataPath,const char* sound_directory)
{
	// Дирректория общих звуков (не зависящих от локализации)
	string common_dir = sound_directory;
	SlashFixup(common_dir);	
	// Дирректория локализованных звуков
	string locale_dir = localeDataPath;
	SlashFixup(locale_dir);

	string head, tail, add_str;

	for(SoundAttributeLibrary::Map::const_iterator it = SoundAttributeLibrary::instance().map().begin(); it != SoundAttributeLibrary::instance().map().end(); ++it)
		if(it->get() && !it->get()->isEmpty()) 
			it->get()->CreateSounds();
		//LoadSoundRelativeAttr(common_dir, locale_dir, it->first, it->second);
}
void ApplySoundParameters()
{
	for(SoundAttributeLibrary::Map::const_iterator it = SoundAttributeLibrary::instance().map().begin(); it != SoundAttributeLibrary::instance().map().end(); ++it)
		if(it->get() && !it->get()->isEmpty()) 
			it->get()->CreateSounds();
}

void InitSound(bool sound, bool music, HWND hwnd, const char* localeDataPath)
{
	terSoundEnable = sound;
	terMusicEnable = music;

	// инициализировать в любом случае, т.к. могут включить потом из интерфейса
	static int inited = 0;
	if(!inited){
		inited = 1;

		if(SNDInitSound(hwnd,true,false)){
			LoadAllSound(localeDataPath,"RESOURCE\\SOUNDS\\");
			MpegInitLibrary(sndSystem.GetDirectSound());
		}
	}

	terSoundVolume	= GameOptions::instance().getFloat(OPTION_SOUND_VOLUME);
	terMusicVolume	= GameOptions::instance().getFloat(OPTION_MUSIC_VOLUME);
	terVoiceVolume	= GameOptions::instance().getFloat(OPTION_VOICE_VOLUME);

	musicManager.SetVolume( terMusicVolume );
	SNDSetVolume(terSoundVolume);
	voiceManager.SetVolume(terVoiceVolume);

	snd_listener.SetZMultiple(fSoundZMultiple);

	SNDEnableSound(terSoundEnable);
}

void UpdateSound()
{
	bool enable = GameOptions::instance().getBool(OPTION_SOUND_ENABLE);
	if(enable != terSoundEnable){
		terSoundEnable = enable;
		SNDEnableSound(terSoundEnable);
	}

	enable = GameOptions::instance().getBool(OPTION_MUSIC_ENABLE);
	if(enable != terMusicEnable){
		terMusicEnable = enable;

		if(terMusicEnable)
			musicManager.Resume();
		else
			musicManager.Pause();
	}

	enable = GameOptions::instance().getBool(OPTION_VOICE_ENABLE);
	if(enable != voiceManager.enabled())
		voiceManager.setEnabled(enable);

	float volume = GameOptions::instance().getFloat(OPTION_SOUND_VOLUME);
	if(fabs(volume - terSoundVolume) > FLT_EPS){
		terSoundVolume = volume;
		SNDSetVolume(terSoundVolume);
	}

	volume = GameOptions::instance().getFloat(OPTION_MUSIC_VOLUME);
	if(fabs(volume - terMusicVolume) > FLT_EPS){
		terMusicVolume = volume;
		musicManager.SetVolume(terMusicVolume);
	}

	volume = GameOptions::instance().getFloat(OPTION_VOICE_VOLUME);
	if(fabs(volume - terVoiceVolume) > FLT_EPS){
		terVoiceVolume = volume;
		voiceManager.SetVolume(terVoiceVolume);
	}
}

void SoundQuant()
{
	start_timer_auto();
	statistics_add(NowPlayingSounds, sndSystem.numberOfPlayingSounds());	
	statistics_add(NowUsedSounds, sndSystem.numberOfUsedSounds());

	if(!terSoundEnable)
		return;

	snd_listener.SetPos(cameraManager->matrix());
	snd_listener.SetVelocity(Vect3f(0,0,0));

	snd_listener.Update();
	sndSystem.RecalculateClipDistance();
}

void FinitSound()
{
	MpegDeinitLibrary();
	SNDReleaseSound();
}

//-----------------------------
MpegSound gb_Music;
MpegSound mpegSound;
MusicManager musicManager;
VoiceManager voiceManager;

int XZipMpegOpen(void* datasource, const char* file_name)
{
	XZipStream* stream = reinterpret_cast<XZipStream*>(datasource);
	if(stream->open(file_name))
		return 1;

	return 0;
}

size_t XZipMpegRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	XZipStream* stream = reinterpret_cast<XZipStream*>(datasource);
	xassert(stream->isOpen());

	if(stream->isOpen())
		return stream->read(ptr, size * nmemb);
	return 0;
}

int XZipMpegSeek(void *datasource, __int64 offset, int dir)
{
	return -1;
//	XZipStream* stream = reinterpret_cast<XZipStream*>(datasource);
//	return stream->seek(offset, dir);
}

int XZipMpegClose(void *datasource)
{
	XZipStream* stream = reinterpret_cast<XZipStream*>(datasource);
	stream->close();
	return 0;
}

long XZipMpegTell(void *datasource)
{
	XZipStream* stream = reinterpret_cast<XZipStream*>(datasource);
	return stream->tell();
}

//MpegCallbacks zipMpegCallbacks = { XZipMpegOpen, XZipMpegRead, XZipMpegSeek, XZipMpegClose, XZipMpegTell };

VoiceManager::VoiceManager() : stream_(false)
{
	enabled_ = GameOptions::instance().getBool(OPTION_VOICE_ENABLE);
	mpeg = &mpegSound;
	soundTrack_ = "";
	canPaused_ = true;

//	MpegSound::SetCallbacks(&zipMpegCallbacks);
}

bool VoiceManager::isPlaying() const
{
	return mpeg->IsPlay() == MPEG_PLAY;
}

bool VoiceManager::isPaused() const
{
	return mpeg->IsPlay() == MPEG_PAUSE;
}

void VoiceManager::Pause()
{
	if(isPlaying() && canPaused_)
		mpeg->Pause();
}

void VoiceManager::Resume()
{
	if(isPaused())
		mpeg->Resume();
}

bool VoiceManager::validatePlayingFile(const VoiceAttribute& voice)
{
	return isPlaying() && !strcmp(voice.voiceFile(), voiceFile());
}

bool VoiceManager::Play(const char* soundTrack, bool cycled, bool canPaused, bool always)
{
	if((!enabled_ && !always) || !soundTrack || extractFileName(soundTrack) == "")
		return false;

	mpeg->Stop();

	XZipStream test(false);
	test.open(soundTrack, XZS_IN);

	if(!test.isOpen()){
		kdWarning("&VoiceManager",XBuffer(1024, 1) < /*TRANSLATE*/("Невозможно открыть файл : ") < soundTrack);
		return false;
	}

	test.close();

	soundTrack_ = soundTrack;
	canPaused_ = canPaused;

	SetVolume(terVoiceVolume);
	bool b = mpeg->OpenToPlay(soundTrack, cycled/*, &stream_*/);
	// из-за открытия файла позже в другом потоке, говорит лишь о невозможности создания потока
	xassert_s(b && "Cannot open music: ", soundTrack); 
	return b;
}

void VoiceManager::setEnabled(bool enable)
{
	if(enable != enabled_){
		enabled_ = enable;
		if(!enable)
			mpeg->Stop();
	}
}

void VoiceManager::Stop()
{
	mpeg->Stop();
}

void VoiceManager::SetVolume(float f)
{
	mpeg->SetVolume(round(255*f));
}

bool VoiceManager::FadeVolume(float time,int newVolume)
{
	return mpeg->FadeVolume(time, newVolume);
}

VoiceManager::~VoiceManager()
{
	mpeg->Stop();
}

MusicManager::MusicManager()
{
	active = false;
	mpeg = &gb_Music;
	soundTrack_ = "";
}

MusicManager::~MusicManager()
{
	CloseWorld();
	//Release();
}

void MusicManager::Release()
{
	active=false;
}

bool MusicManager::isPlaying() const
{ 
	return mpeg->IsPlay() == MPEG_PLAY; 
}

bool MusicManager::isPaused() const
{
	return mpeg->IsPlay() == MPEG_PAUSE;
}

void MusicManager::Pause() 
{
	if(isPlaying())
		mpeg->Pause();
}

bool MusicManager::FadeVolume(float time,int newVolume)
{
	return mpeg->FadeVolume(time, newVolume);
}

void MusicManager::Resume()
{
	if(isPaused())
		mpeg->Resume();
	else
		if(!isPlaying())
			Play(soundTrack_.c_str());
}

bool MusicManager::Play(const char* soundTrack)
{
	if(!soundTrack || extractFileName(soundTrack) == "")
		return false;

	soundTrack_ = soundTrack; // специально для проигрывания при смене опции вкл./выкл. саундтрек 

	if(!terMusicEnable)
		return false;

#ifndef _FINAL_VERSION_
	FILE* file=fopen(soundTrack,"r");
	if(file==NULL){
		kdWarning("&VoiceManager",XBuffer(1024, 1) < /*TRANSLATE*/("Невозможно открыть файл : ") < soundTrack);
		return false;
	}
	fclose(file);
#endif

	if(mpeg->IsPlay() == MPEG_PLAY && soundTrack_ == soundTrack) {
		return true;
	}
	
	mpeg->Stop();

	SetVolume( terMusicVolume );
	bool b=mpeg->OpenToPlay(soundTrack_.c_str(), true);
	xassert_s(b && "Cannot open music: ", soundTrack_.c_str());
	return b;
}

void MusicManager::Stop()
{
	mpeg->Stop();
	soundTrack_ = "";
}

void MusicManager::OpenWorld()
{
	if(!terMusicEnable || active)
		return;
	active=true;
}

void MusicManager::CloseWorld()
{
	if(!active)
		return;
	Release();
	mpeg->Stop();
}

void MusicManager::SetVolume(float f)
{
	mpeg->SetVolume( round(255*f) );
}

void MusicManager::Enable(int enable, bool gameActive)
{
	if(enable)
		if (gameActive) 
			OpenWorld();
		else
			Play(soundTrack_.c_str()); 
	else
		if (gameActive)
			CloseWorld();
		else
			mpeg->Stop();
}


//////////////////////////////////////////////////////////////////////////
SoundLibrary sndLibrary;
SoundLibrary::SoundLibrary()
{
}

SoundLibrary::~SoundLibrary()
{
}

bool SoundLibrary::AddSound(LPCSTR name,Sound* sound)
{
	if (!name && strlen(name)==0)
		return false;
	sounds_[name] = sound;
	return true;
}

Sound* SoundLibrary::FindSound(LPCSTR name)
{
	SoundArray::iterator it=sounds_.find(name);
	if(it==sounds_.end())
	{
		//logs("Sound not found:%s\n",name);
		return NULL;
	}
	return ((*it).second);

}


SNDSound::SNDSound()
{
	channel = NULL;
}

SNDSound::~SNDSound()
{
	Destroy();
}

void SNDSound::Destroy()
{
	if (channel)
	{
		channel->Release();
		channel = NULL;
	}
}
void SNDSound::SetPan(float pan)
{
	if (channel)
		channel->SetPan(pan);
}
void SNDSound::SetMute(bool mute)
{
	if(channel)
		channel->SetMute(mute);
}

bool SNDSound::Init(const SoundAttribute* soundAttr)
{
	Destroy();
	if (!soundAttr)
		return false;
	soundAttr_ = soundAttr;
	Sound* snd = sndLibrary.FindSound(soundAttr->GetPlaySoundName());
	if (!snd)
		return false;
	channel = snd->CreateAndPlayChannel(true);
	if (!channel)
		return false;
	channel->SetFadeIn(soundAttr->fadeIn());
	channel->SetFadeOut(soundAttr->fadeOut());
	return true;
}

bool SNDSound::Play(bool cycled/* =true */)
{
	if (!channel)
		return false;
	channel->SetLoop(cycled);
	channel->Play();
	return true;
}
bool SNDSound::Stop(bool immediately)
{
	if (!channel)
		return false;
	channel->Stop(immediately);
	return true;
}
bool SNDSound::IsPlayed()
{
	return (channel&&channel->IsPlaying());
}
void SNDSound::SetPos(const Vect3f& pos)
{
	channel->SetPosition(pos);
}
void SNDSound::SetVelocity(const Vect3f& velocity)
{

}
void SNDSound::SetVolume(float vol)
{
	if (channel)
		channel->SetVolume(vol);
}

SoundEnvironmentManager::SoundEnvironmentManager()
{

}
SoundEnvironmentManager::~SoundEnvironmentManager()
{
	multimap<string,SNDSound*>::iterator it;
	FOR_EACH(playSounds,it)
	{
		it->second->Stop();
		delete it->second;
	}
	playSounds.clear();
}

bool SoundEnvironmentManager::PlaySound(SoundReference &sndRef)
{
	const SoundAttribute* attr = sndRef.get();
	if(!sndRef->cycled())
	{
		sndRef->play();
		return true;
	}
	typedef pair <string,SNDSound*> SND_Pair;
	SNDSound* sound = new SNDSound();
	sound->Init(sndRef);
	sound->Play(true);
	playSounds.insert(SND_Pair(sndRef.c_str(),sound));
	return true;
}
bool SoundEnvironmentManager::StopSound(SoundReference &sndRef)
{
	multimap<string,SNDSound*>::iterator it;
	it = playSounds.find(sndRef.c_str());
	if(it == playSounds.end())
		return true;
	it->second->Stop();
	delete it->second;
	playSounds.erase(it);
	return true;
}
