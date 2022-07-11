#ifndef __SOUND_ATTRIBUTE_H__
#define __SOUND_ATTRIBUTE_H__

#include "TypeLibrary.h"
#include "Timers.h"

long CalcVolume(int vol);
class Archive;
struct SNDSoundParam;
class SoundAttribute;
class SNDSound;
class Sound;

long CalcVolume(int vol);

enum SoundSurfKind{
	SOUND_SURF_ALL = 0,
	SOUND_SURF_KIND1 = 1,
	SOUND_SURF_KIND2 = 2,
	SOUND_SURF_KIND3 = 3,
	SOUND_SURF_KIND4 = 4,
};

typedef StringTable<StringTableBasePolymorphic<SoundAttribute> > SoundAttributeLibrary;
typedef StringTablePolymorphicReference<SoundAttribute, false> SoundReference;

/// Базовые параметры звука.
class SoundAttribute : public PolymorphicBase
{
public:
	SoundAttribute();
	virtual ~SoundAttribute(){ }

	virtual bool is3D() const = 0;
	virtual bool isRaceDependent() const = 0;
	virtual bool isLanguageDependent() const = 0;

	//bool isEmpty() const { return soundFiles_.empty(); }
	bool isEmpty() const { return soundFiles_.empty(); }

	virtual void serialize(Archive& ar);

	//virtual bool convert(SNDSoundParam& param) const;

	// пока просто возвращает то же самое имя
	// TODO: учёт зависимости от языка и расы - 
	// разные языки лежат в разных директориях,
	// для разных рас добавка к имени файла
	const char* convertFileName(const char* file_name) const;

	bool play(const Vect3f& position = Vect3f::ZERO) const;

	virtual Sound* CreateSound(const char* name) const;
	void CreateSounds() const;
	virtual void ApplyParameters(Sound* sound) const;
	const char* GetPlaySoundName() const;

	bool cycled() const { return cycled_; }
	int fadeOut()const {return fadeOut_;}
	int fadeIn()const {return fadeIn_;}
	int surfKind()const {return (int)surfKind_;}
	bool stopInFogOfWar()const {return stopInFogOfWar_;}
	bool useGlobalFade()const {return useGlobalFade_;}

protected:

	/// громкость звука
	float volume_;
	/// минимальная громкость звука
	float volumeMin_;

	/// Максимальное количество одновременно звучащих звуков.
	/**
	 В случае 2D при попытке загрузить больше этого количества звук не запускается.
	 В случае 3D:
	    если == 1, запускается только 1 звук, последующие не запускаются.
	    если > 1, запускается все звуки, но слышны только ближайшие.
	*/
	int maxCount_;

	/// Случайное изменение частоты при старте звука.
	bool frequencyRnd_;
	/// Во сколько раз частота звука может уменьшаться.
	float frequencyRndMin_;
	/// Во сколько раз частота звука может увеличиваться.
	float frequencyRndMax_;

	/// звуковые файлы, 
	/**
	 Если их несколько, то при старте звука случайно выбирается какой-то из них.
	*/
	struct SoundFile : string
	{
		SoundFile(const char* name = "") : string(name) {}
		bool serialize (Archive& ar, const char* name, const char* nameAlt);
	};

	std::vector<SoundFile> soundFiles_;

	bool cycled_;
	int fadeIn_;
	int fadeOut_;
	bool stopInFogOfWar_;
	bool useGlobalFade_;
	SoundSurfKind surfKind_;

	friend SNDSound;
};

/// Параметры обычного звука.
class Sound2DAttribute : public SoundAttribute
{
public:
	bool is3D() const { return false; }
	bool isRaceDependent() const { return false; }
	bool isLanguageDependent() const { return false; }
};

/// Параметры трёхмерного звука.
class Sound3DAttribute : public SoundAttribute
{
	friend SNDSound;
public:
	Sound3DAttribute();

	void serialize(Archive& ar);

	bool is3D() const { return true; }
	bool isRaceDependent() const { return false; }
	bool isLanguageDependent() const { return false; }

	bool modulateFrequency() const { return modulateFrequency_; }

	//bool convert(SNDSoundParam& param) const;

	virtual Sound* CreateSound(const char* name) const;
	virtual void ApplyParameters(Sound* sound) const;

private:

	/// Величина объекта, с этого расстояния громкость звука начинает убывать (По умолчанию: 50)
	float radius_; 
	/// Максимальное расстояние до объекта, после которого звук перестаёт быть слышимым (По умолчанию: oo)
	float radiusMax_;

	/// Менять частоту звука (в зависимости от скорости источника и т.п.)
	bool modulateFrequency_;
	bool muteByMaxDistance_;

};

/// Параметры голосового сообщения.
class VoiceAttribute : public ShareHandleBase
{
public:
	VoiceAttribute();

	struct VoiceFile 
    {
		VoiceFile(const char* name = "") 
		{
			fileName_ = name;
			duration_ = 0.f; 
		}

		void serialize (Archive& ar);

		const char* fileName() const;
		float duration() const; // сомножитель из-за запуска звука в отдельном потоке

		float duration_;
		string fileName_;
	
		static void saveVoiceFileDurations();
		static void loadVoiceFileDuration();
		static void recalculateVoiceFileDuration(const char* language);

		typedef StaticMap<string, float> VoiceFileDurations;
		static VoiceFileDurations voiceFileDurations_;
	};

	void getNextVoiceFile() const; 
	const char* voiceFile() const; 
	float duration() const;

	bool isEmpty() const { return fileNames_.empty(); }
	
	void serialize(Archive& ar);

	static void setDeterministic(bool deterministic) { deterministic_ = deterministic; }

private:
	typedef vector<VoiceFile> FileNames;
	FileNames fileNames_;
	bool randomSelect_;
	float timeToFun_;
	mutable int index_; 
	mutable int curIndex_; // номер текущего файла
	mutable DurationTimer funTimer_;

	static bool deterministic_;
};

struct SoundLogoAttributes
{
	void serialize(Archive& ar);
	SoundReference mouseMove_;
	SoundReference fishSwim_;
	SoundReference fishStop_;
	SoundReference fishRise_;
	SoundReference waterOut_;
	SoundReference backGround_;
};

//-------------------------------------------------------------------
class SoundReferences : public vector<SoundReference> 
{
	typedef vector<SoundReference> BaseClass;
public:
	const SoundAttribute* getSound(int surfKind) const;
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

struct SoundMarker
{
public:
	SoundMarker();
	void serialize(Archive& ar);

	SoundReferences soundReferences_;
	float phase_;
	bool active_;
};
typedef vector<SoundMarker> SoundMarkers;

#endif /* __SOUND_ATTRIBUTE_H__ */
