#include "stdafx.h"
#include "SoundTrack.h"
#include "Serialization\ResourceSelector.h"
#include "DebugUtil.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"
#include "Actions.h"
#include "SoundApp.h"
#include "Serialization\StringTableImpl.h"
#include "Sound\SoundSystem.h"
#include "Serialization\SerializationFactory.h"

WRAP_LIBRARY(SoundTrackTable, "SoundTrackTable", "Звуковые треки", "Scripts\\Content\\SoundTrackTable", 0, 0);

bool SoundTrack::Name::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	static ResourceSelector::Options soundTrackOptions("*.ogg", "Resource\\Music");
	return ar.serialize(ResourceSelector(fileName_, soundTrackOptions), name, nameAlt);
}

void SoundTrack::serialize(Archive& ar) 
{
	StringTableBase::serialize(ar);

	ar.serialize(fileNames, "fileNames", "Имена музыкальных треков");
	ar.serialize(randomChoice, "randomChoice", "Случайный выбор трека");
}

const char* SoundTrack::fileName() const
{
	if(fileNames.empty())
		return "";
	return fileNames[index];
}

void SoundTrack::nextFileName() const
{
	if(fileNames.size())
		index = (randomChoice ? effectRND() : index++) % fileNames.size();
}

class ActionStopSoundTrack : public Action
{
public:
	
	enum Type {
		STOP_IMMEDIATELY,
		STOP_FADE_OUT_VOLUME
	};

	ActionStopSoundTrack()
	{
		type = STOP_IMMEDIATELY;
		fadeTime = 5.f;
	}
	~ActionStopSoundTrack()
	{
		if(durationTimer.busy())
			musicManager.Stop();
	}

	void serialize(Archive& ar) {
		__super::serialize(ar);
		ar.serialize(type, "type", "Тип отключения");
		ar.serialize(fadeTime, "fadeTime", "Время затихания звука(сек.)");
		ar.serialize(durationTimer, "durationTimer", 0);
	}

	void activate(){
		if(aiPlayer().active()){
			switch(type){
				case STOP_IMMEDIATELY:
					musicManager.Stop();
					break;
				case STOP_FADE_OUT_VOLUME:
					if(musicManager.isPlaying()){
						musicManager.FadeVolume(fadeTime);
						durationTimer.start(round(fadeTime*1000.f));
					}
					break;
			}
		}
	}

	bool workedOut(){
		if(aiPlayer().active()){
			if(type == STOP_IMMEDIATELY)
				return true;
			if(!durationTimer.busy()){
				musicManager.Stop();
				return true;
			}
		}
		return false;
	}

private:
	Type type;
	float fadeTime;
	LogicTimer durationTimer;
};

class ActionMusicVolume : public Action
{
public:
	enum ActionType {
		SET_VOLUME_IMMEDIATELY,
		SET_VOLUME_FADE,
		SET_VOLUME_RESTORE,
		SET_VOLUME_RESTORE_FADE
	};

	ActionMusicVolume()
	{
		actionType_ = SET_VOLUME_IMMEDIATELY;
		fadeTime_ = 0.f;
		volume_ = 0.5f;
	}

	void serialize(Archive& ar)
	{
		__super::serialize(ar);

		ar.serialize(actionType_, "actionType", "Что сделать");
		ar.serialize(volume_, "volume", "Громкость(0..1)");
		ar.serialize(fadeTime_, "fadeTime", "Время фейда");

		ar.serialize(timer_, "timer", 0);
	}

    void activate() 
	{
		switch(actionType_){
			case SET_VOLUME_IMMEDIATELY:
				musicManager.SetVolume(volume_);
				break;
			case SET_VOLUME_FADE:
				musicManager.FadeVolume(fadeTime_, 1.f);
				break;
			case SET_VOLUME_RESTORE:
				musicManager.SetVolume(terMusicVolume);
				break;
			case SET_VOLUME_RESTORE_FADE:
				musicManager.FadeVolume(fadeTime_, 1.f);
				break;
		}
		if(actionType_ == SET_VOLUME_FADE || actionType_ == SET_VOLUME_RESTORE_FADE)
			timer_.start(round(fadeTime_*1000));
	}

	bool workedOut()
	{   
		if(!timer_.busy()){
			if(actionType_ == SET_VOLUME_FADE)
				musicManager.SetVolume(volume_);
			if(actionType_ == SET_VOLUME_RESTORE_FADE)
				musicManager.SetVolume(terMusicVolume);

			return true;
		}

		return false;
	}

private:
	float volume_;
	ActionType actionType_;
	float fadeTime_;
	LogicTimer timer_;
};

struct ActionMute3DSound : Action
{
	SwitchMode switchMode_;
	ActionMute3DSound()
	{
		switchMode_ = OFF;
	}

	void serialize(Archive& ar)
	{
		__super::serialize(ar);
		ar.serialize(switchMode_, "switchMode", "Действие");
	}

	void activate()
	{
		sndSystem.Mute3DSounds(switchMode_ == ON ? false : true);
	}
};

struct ActionPlaySoundTrack : Action
{
	SoundTrackReference soundTrack;

	void activate() {
		if(aiPlayer().active()){
			musicManager.Play(soundTrack->fileName());
			soundTrack->nextFileName();
		}
	}

	void serialize(Archive& ar) {
		__super::serialize(ar);
		ar.serialize(soundTrack, "soundTrack", "Музыкальный трек");
	}
};

STARFORCE_API void initActionsSound()
{
SECUROM_MARKER_HIGH_SECURITY_ON(6);
REGISTER_CLASS(Action, ActionPlaySoundTrack, "Интерфейс\\Включить музыкальный трек (Неповторяемое!!!)")
REGISTER_CLASS(Action, ActionStopSoundTrack, "Интерфейс\\Выключить музыкальный трек (Неповторяемое!!!)")
SECUROM_MARKER_HIGH_SECURITY_OFF(6);
}

REGISTER_CLASS(Action, ActionMusicVolume, "Интерфейс\\Громкость музыки")
REGISTER_CLASS(Action, ActionMute3DSound, "Интерфейс\\Включить/выключить 3D звуки")

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionMusicVolume, ActionType, "ActionType")
REGISTER_ENUM_ENCLOSED(ActionMusicVolume, SET_VOLUME_IMMEDIATELY, "Установить немедленно")
REGISTER_ENUM_ENCLOSED(ActionMusicVolume, SET_VOLUME_FADE, "Установить с фейдом")
REGISTER_ENUM_ENCLOSED(ActionMusicVolume, SET_VOLUME_RESTORE, "Восстановить немедленно")
REGISTER_ENUM_ENCLOSED(ActionMusicVolume, SET_VOLUME_RESTORE_FADE, "Восстановить с фейдом")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionMusicVolume, ActionType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ActionStopSoundTrack, Type, "Type")
REGISTER_ENUM_ENCLOSED(ActionStopSoundTrack, STOP_IMMEDIATELY, "Немедленно")
REGISTER_ENUM_ENCLOSED(ActionStopSoundTrack, STOP_FADE_OUT_VOLUME, "С фейдом")
END_ENUM_DESCRIPTOR_ENCLOSED(ActionStopSoundTrack, Type)
