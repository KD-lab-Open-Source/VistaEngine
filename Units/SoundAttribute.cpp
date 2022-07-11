
#include "StdAfx.h"
#include "SoundAttribute.h"
#include "ResourceSelector.h"
#include "PlayOgg.h"
#include "Sound.h"
#include "SoundApp.h"
#include "TypeLibraryImpl.h"
#include "../Sound/soundSystem.h"
#include "Universe.h"
#include "GlobalAttributes.h"
#include "..\Util\Serialization\XPrmArchive.h"

REGISTER_CLASS(SoundAttribute, Sound2DAttribute, "2D звук");
REGISTER_CLASS(SoundAttribute, Sound3DAttribute, "3D звук");
WRAP_LIBRARY(SoundAttributeLibrary, "SoundLibrary", "Звуки", "Scripts\\Content\\SoundLibrary", 0, true);

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
	static ResourceSelector::Options options("*.wav", "Resource\\Sounds", "Will select location of sound file");
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
	if(ar.isInput())
		for(FileNames::iterator i = fileNames_.begin(); i != fileNames_.end();){
			if(!i->fileName())
				i = fileNames_.erase(i);
			else
				++i;
		}
}

void VoiceAttribute::VoiceFile::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.ogg", getLocDataPath("Voice"), "Will select location of voice file");
	ar.serialize(ResourceSelector(fileName_, options), "fileName", "&Имя файла");

	// убирание пути локализации
	static string name;
	name = fileName_;
	int pos = name.find("\\");
	if(pos != string::npos){
		pos = name.find("\\", pos + 1);
		if(pos != string::npos){
			pos = name.find("\\", pos + 1);
			name.erase(0, pos + 1);
		}
	}		

	if(ar.isOutput() && name != ""){
		voiceFileDurations_.insert(make_pair(name.c_str(), 0.f));
	}

	duration_ = 0.f;
	if(ar.isInput() && name != "" && !isUnderEditor()){
		VoiceFileDurations::const_iterator i = voiceFileDurations_.find(name.c_str());
		duration_ = (i != voiceFileDurations_.end()) ? (*i).second : 0.f;
	}
}

void VoiceAttribute::VoiceFile::saveVoiceFileDurations()
{
	LanguagesList::iterator i;
	FOR_EACH(GlobalAttributes::instance().languagesList, i){
		recalculateVoiceFileDuration((*i).language.c_str());
		XBuffer buf;
		buf < "RESOURCE\\LOCDATA\\" < (*i).language.c_str() < "\\VOICE\\voiceDurations";
		XPrmOArchive ia(buf);
		ia.serialize(voiceFileDurations_, "voiceFileDurations", 0);
	}
}

void VoiceAttribute::VoiceFile::loadVoiceFileDuration()
{
	XBuffer buf;
	buf < getLocDataPath("Voice") < "\\voiceDurations";
	XPrmIArchive ia;
	if(ia.open(buf))
		ia.serialize(voiceFileDurations_, "voiceFileDurations", 0);
}

void VoiceAttribute::VoiceFile::recalculateVoiceFileDuration(const char* language)
{
	XBuffer buf;
	buf < "RESOURCE\\LOCDATA\\" < language < "\\";
	VoiceFileDurations::iterator i;
	FOR_EACH(voiceFileDurations_, i)
	{
		XBuffer buff;
		buff < buf < (*i).first.c_str();
		(*i).second = MpegGetLen(buff);
	}
}

const char* VoiceAttribute::VoiceFile::fileName() const 
{ 
	if(fileName_ != ""){
		const char* locFileName = setLocDataPath(fileName_.c_str());
		return locFileName;
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
		if(timeToFun_ && !funTimer_()){
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
	return !fileNames_.empty() ? fileNames_[curIndex_].duration() : 0;
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


