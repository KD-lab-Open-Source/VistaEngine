
#include "StdAfx.h"
#include "SoundAttribute.h"
#include "Serialization\ResourceSelector.h"
#include "PlayOgg.h"
#include "Sound.h"
#include "SoundApp.h"
#include "Serialization\StringTableImpl.h"
#include "Sound\soundSystem.h"
#include "Universe.h"
#include "GlobalAttributes.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\SerializationFactory.h"
#include "FileUtils\FileUtils.h"

REGISTER_CLASS(SoundAttribute, Sound2DAttribute, "2D звук");
REGISTER_CLASS(SoundAttribute, Sound3DAttribute, "3D звук");
WRAP_LIBRARY(SoundAttributeLibrary, "SoundLibrary", "Звуки", "Scripts\\Content\\SoundLibrary", 0, LIBRARY_EDITABLE);

VoiceAttribute::VoiceFile::VoiceFileDurations VoiceAttribute::VoiceFile::voiceFileDurations_;
bool VoiceAttribute::deterministic_ = false;

void SoundLogoAttributes::serialize(Archive& ar)
{
	ar.serialize(mouseMove_,"mouseMove","Движение мыши");
	ar.serialize(fishSwim_,"fishSwim","Рыба плывет");
	ar.serialize(fishStop_,"fishStop","Рыба остановилась");
	ar.serialize(fishRise_,"fishRise","Рыба растет");
	ar.serialize(waterOut_,"waterOut","Вода убывает");
	ar.serialize(backGround_,"backGround","Фоновый звук");
}

bool SoundAttribute::SoundFile::serialize(Archive& ar, const char* name, const char* nameAlt) 
{
	static ResourceSelector::Options options("*.wav", "Resource\\Sounds", "Will select location of sound file", true, true);
	return ar.serialize(ResourceSelector(static_cast<string&>(*this), options), name, nameAlt);
}

SoundAttribute::SoundAttribute()
{
	volume_ = 1.0f;
	volumeMin_ = 0.0f;
	maxCount_ = 25;

	frequencyRnd_ = false;
	frequencyRndMin_ = 1.0f;
	frequencyRndMax_ = 1.0f;

	cycled_ = false;
	fadeIn_ = 0;
	fadeOut_ = 0;
	surfKind_ = SOUND_SURF_ALL;
	stopInFogOfWar_ = true;
	useGlobalFade_ = true;
}

void SoundAttribute::serialize(Archive& ar)
{
	ar.serialize(volume_, "volume", "громкость");
	ar.serialize(volumeMin_, "volumeMin", "минимальная громкость");
	ar.serialize(maxCount_, "maxCount", "макс. количество одновременно слышимых звуков");

	ar.serialize(frequencyRnd_, "frequencyRnd", "случайное изменение частоты");
	ar.serialize(frequencyRndMin_, "frequencyRndMin", "макс. коэфф. уменьшения частоты");
	ar.serialize(frequencyRndMax_, "frequencyRndMax", "макс. коэфф. увеличения частоты");

	ar.serialize(soundFiles_, "soundFiles", "файлы");

	ar.serialize(surfKind_, "surfKind", "Тип поверхности");
	ar.serialize(cycled_, "cycled", "зацикливать");
	ar.serialize(fadeIn_, "fadeIn", "FadeIn (милисекунд)");
	ar.serialize(fadeOut_, "fadeOut", "FadeOut (милисекунд)");
	ar.serialize(stopInFogOfWar_,"stopInFogOfWar","Выключать в тумане войны");
	ar.serialize(useGlobalFade_,"useGlobalFade","Учитывать глобальный FadeIn\\FadeOut");
}

const char* SoundAttribute::convertFileName(const char* file_name) const
{
	return file_name;
}

Sound3DAttribute::Sound3DAttribute()
{
	radius_ = 50.0f;
	radiusMax_ = 2000.0f;
	muteByMaxDistance_ = true;
}

void Sound3DAttribute::serialize(Archive& ar)
{
	SoundAttribute::serialize(ar);

	ar.serialize(radius_, "radius", "радиус объекта");
	ar.serialize(radiusMax_, "radiusMax", "максимальный радиус слышимости");
	ar.serialize(muteByMaxDistance_, "muteByMaxDistance", "Обрывать за предeлaми макс. радиуса");
}

Sound* Sound3DAttribute::CreateSound(const char* name) const
{
	Sound* sound = NULL;
	DWORD mode = SS_3DSOUND;
	//if (muteByMaxDistance_)
	mode|=SS_MUTEBYMAXDISTANCE;
	sound = sndSystem.CreateSound(name,mode);
	if (!sound)
		return NULL;
	sound->SetVolume(volume_);
	sound->Set3DMinMaxDistance(radius_,radiusMax_);
	sound->SetStopInFogOfWar(stopInFogOfWar());
	sound->SetUseGlobalFade(useGlobalFade());
	return sound;
}
void Sound3DAttribute::ApplyParameters(Sound* sound) const
{
	sound->SetVolume(volume_);
	sound->Set3DMinMaxDistance(radius_,radiusMax_);
}

const char* SoundAttribute::GetPlaySoundName() const
{
	int n = rand()%soundFiles_.size();
	return soundFiles_[n].c_str();
}

bool SoundAttribute::play(const Vect3f& position) const
{
	Sound* sound = sndLibrary.FindSound(GetPlaySoundName());
	if(sound)
		sound->PlaySound(position);
	return true;
}

Sound* SoundAttribute::CreateSound(const char* name) const
{
	Sound* sound = NULL;
	sound = sndSystem.CreateSound(name,SS_2DSOUND);
	if (!sound)
		return NULL;
	sound->SetVolume(volume_);
	sound->SetStopInFogOfWar(stopInFogOfWar());
	sound->SetUseGlobalFade(useGlobalFade());
	return sound;
}
void SoundAttribute::ApplyParameters(Sound* sound) const
{
	sound->SetVolume(volume_);
}
void SoundAttribute::CreateSounds() const
{
	for (int i=0; i<soundFiles_.size(); i++)
	{
		if(soundFiles_[i].empty())
			continue;
		Sound* sound = sndLibrary.FindSound(soundFiles_[i].c_str());
		if (!sound)
		{
			sound = CreateSound(soundFiles_[i].c_str());
			if (sound)
			{
				sound->SetMaxChannels(maxCount_);
				sndLibrary.AddSound(soundFiles_[i].c_str(),sound);
			}
		}else
		{
			ApplyParameters(sound);
		}
	}
}



VoiceAttribute::VoiceAttribute()
{ 
	randomSelect_ = true;
	timeToFun_ = 0.f;
	index_ = 0;
	curIndex_ = 0;
}

void VoiceAttribute::serialize(Archive& ar)
{
	ar.serialize(fileNames_, "fileNames", "&Файлы");
	ar.serialize(randomSelect_, "randomSelect", "Случайный выбор звука");
	if(!randomSelect_)
		ar.serialize(timeToFun_, "timeToFun", "Время при котором возможно проигрывание всей цепочки файлов"); 
	if(ar.isInput() && !ar.isEdit())
		for(FileNames::iterator i = fileNames_.begin(); i != fileNames_.end();){
			if(!i->fileName())
				i = fileNames_.erase(i);
			else
				++i;
		}
}

VoiceAttribute::VoiceFile::VoiceFile(const char* name)
{
	fileName_ = name;
	duration_ = 1.f; 
}

void VoiceAttribute::VoiceFile::serialize(Archive& ar)
{
	if(ar.isOutput())
		voiceFileDurations_[fileName_] = 1.f;

	static ResourceSelector::Options options("*.ogg", getLocDataPath("Voice\\").c_str(), "Will select location of voice file");
	ar.serialize(ResourceSelector(fileName_, options), "fileName", "&Имя файла");

	if(ar.isInput()){
		static bool loaded;
		if(!loaded){
			loaded = true;
			loadVoiceFileDuration();
		}
		fileName_ = extractFileName(fileName_.c_str());
		VoiceFileDurations::const_iterator i = voiceFileDurations_.find(fileName_);
		duration_ = (i != voiceFileDurations_.end()) ? (*i).second : 1.f;
	}
}

void VoiceAttribute::VoiceFile::saveVoiceFileDurations()
{
	AvaiableLanguages::iterator it;
	FOR_EACH(GlobalAttributes::instance().languagesList, it){
		recalculateVoiceFileDuration(*it);
		XBuffer buf;
		buf < "Resource\\LocData\\" < *it < "\\Voice\\voiceDurations";
		XPrmOArchive ia(buf);
		ia.serialize(voiceFileDurations_, "voiceFileDurations", 0);
	}
}

void VoiceAttribute::VoiceFile::loadVoiceFileDuration()
{
	XPrmIArchive ia;
	if(ia.open(getLocDataPath("Voice\\voiceDurations").c_str()))
		ia.serialize(voiceFileDurations_, "voiceFileDurations", 0);
}

void VoiceAttribute::VoiceFile::recalculateVoiceFileDuration(const char* language)
{
	VoiceFileDurations::iterator i;
	FOR_EACH(voiceFileDurations_, i){
		float time = OggPlayer::getLength((getLocDataPath("Voice\\") + (*i).first).c_str());
		(*i).second = time > FLT_EPS ? time : 1.f;
	}
}

const char* VoiceAttribute::VoiceFile::fileName() const 
{ 
	if(fileName_ != ""){
		static string name;
		name = getLocDataPath("Voice\\") + fileName_;
		return name.c_str();
	}
	else
		return 0;
}

float VoiceAttribute::VoiceFile::duration() const
{
	if(deterministic_){
		size_t pos = fileName_.rfind("\\");
		string name(fileName_, pos != string::npos ? pos + 1 : 0);
		float t = universe()->voiceFileDuration(name.c_str(), duration_ * 1.1);
		return t;
	}
	else
		return duration_ * 1.1;
}

void VoiceAttribute::getNextVoiceFile() const
{
	if(randomSelect_){
		curIndex_ = deterministic_ ? logicRND(fileNames_.size()) : effectRND(fileNames_.size());
	}
	else{
		if(timeToFun_ && !funTimer_.busy()){
			funTimer_.start(timeToFun_ * 1000);
			index_ = 0;
		}

		if(index_ >= fileNames_.size())
			index_ = 0;
		curIndex_ = index_;
		index_++;
	}
}

const char* VoiceAttribute::voiceFile() const 
{ 
	return !fileNames_.empty() ? fileNames_[curIndex_].fileName() : 0;
}

float VoiceAttribute::duration() const
{
	return !fileNames_.empty() ? fileNames_[curIndex_].duration() : 1.f; 
}

// -------------------------------------------

const SoundAttribute* SoundReferences::getSound(int surfKind) const
{
	int nsnd = -1;
	SoundReferences::const_iterator i0 = begin();
	SoundReferences::const_iterator i;
	FOR_EACH(*this, i)
	{
		if(!(*i))
			return NULL;
		if((*i)->surfKind() == 0)
			i0 = i;
		if((*i)->surfKind() == surfKind)
			return *i;
	}
	return i0 != end() ? i0->get() : 0;
}

bool SoundReferences::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(static_cast<BaseClass&>(*this), name, nameAlt);
}

SoundMarker::SoundMarker()
{
	phase_ = 0.0f;
	active_ = false;
}

void SoundMarker::serialize(Archive& ar)
{
	ar.serialize(phase_, "phase", "&Фаза");
	ar.serialize(soundReferences_,"soundReferences","Звуки");
}


