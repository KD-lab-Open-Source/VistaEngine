#ifndef _UI_GAME_OPTIONS_H
#define _UI_GAME_OPTIONS_H

#include "LibraryWrapper.h"
#include "Starforce.h"
#include "xmath.h"
#include "d3d9types.h"
#include "..\UserInterface\CommonLocText.h"

enum GameOptionType
{
	OPTION_LANGUAGE = 0,
	OPTION_GAMMA, // 0.5 ... 2.5
	OPTION_FULL_SCREEN, // bool
	OPTION_DEBUG_WINDOW, // bool
	OPTION_SCREEN_SIZE, // predefine list
	OPTION_MAP_LEVEL_LOD, // 00..100, def 50
	OPTION_SHADOW, // выкл, круглые, плохие, хорошие
	OPTION_TILEMAP_DETAIL, // использовать мелкодетальную текстуру, bool
	OPTION_TEXTURE_DETAIL_LEVEL, // Low|Medium|Good
	OPTION_TILEMAP_TYPE_NORMAL, // качественное освещение, bool
	OPTION_REFLECTION,
	OPTION_PARTICLE_RATE, //0..1
	OPTION_BUMP, // включить bumpMapping, bool
	OPTION_FOG_OF_WAR, // включить туман войны, bool
	OPTION_ANTIALIAS, // качество антиалиасинга, predefine list
	OPTION_ANISOTROPY, // анизатропная фильтрация, predefine list
	OPTION_SILHOUETTE, // включить силуэты
	OPTION_SOFT_SMOKE, // сглаживание эффектов
	OPTION_BLOOM, // включить эффект свечения
	OPTION_MIRAGE,//включить эффект миража/подводного колебания
	OPTION_GRASSDENSITY, // плотность травы, при 0 - трава выключена
	OPTION_CAMERA_RESTRICTION, // включить ограничение камеры
	OPTION_CAMERA_UNIT_FOLLOW, // привязать камеру к юниту
	OPTION_CAMERA_UNIT_DOWN_FOLLOW, // привязанный юнит внизу экрана
	OPTION_CAMERA_UNIT_ROTATE, // поворачивать камеру за юнитом
	OPTION_CAMERA_ZOOM_TO_CURSOR, // зум в курсор
	OPTION_CAMERA_INVERT_MOUSE, // инвертировать мышь в прямом управление
	OPTION_SOUND_ENABLE,
	OPTION_SOUND_VOLUME,
	OPTION_MUSIC_ENABLE,
	OPTION_MUSIC_VOLUME,
	OPTION_VOICE_ENABLE,
	OPTION_VOICE_VOLUME,
	OPTION_SHOW_HINTS,
	OPTION_SHOW_MESSAGES,
	OPTION_WEATHER, //вкл/выкл погодных эффектов
	OPTION_ENUM_SIZE
};

class Archive;


class GameOptions : public LibraryWrapper<GameOptions> 
{
public:
	enum KindType{
		SYSTEM		= 0,
		GAME		= 1,
		GRAPHICS	= 2,
		SOUND		= 3,
		CAMERA		= 4,
		_MASK		= 0xFFFF,
		CRITICAL	= 1 << 16,
		INDEXED		= 1 << 17,
		INSTANT		= 1 << 18
	};
private:
	enum ValueType{
		TYPE_PHASA,
		TYPE_BOOL, // первое значение из списка- истина, любое другое ложь
		TYPE_LIST,
		TYPE_NONE
	};
	
	typedef vector<bool> ValidIndexes;

	class OptionPrm
	{
		int kindType_;
	public:
		ValidIndexes valid_;
		ValueType Vtype_;
		float min_, max_;
		string valueList_;

		OptionPrm() : kindType_(SYSTEM), Vtype_(TYPE_NONE), min_(0.f), max_(1.f) {}
		
		void set(int kindType, ValueType valueType, float min = 0, float max = 1, const char* lst = 0);
		void setTranslate(const char* lst);

		KindType type() const { return (KindType)(kindType_ & _MASK); }
		bool isCritical() const { return kindType_ & CRITICAL; }
		bool isIndexed() const { return kindType_ & INDEXED; }
		bool needInstantApply() const { return kindType_ & INSTANT; }
	};
	typedef vector<OptionPrm> GameOptionPrms;

	
	struct Option
	{
		Option() : type_(OPTION_ENUM_SIZE), data_(0) {}
		
		Option(GameOptionType type, bool flag)		: type_(type), data_(!flag) {}
		Option(GameOptionType type, int dat)		: type_(type), data_(clamp(dat, 0, 100)) {}
		Option(GameOptionType type, float phase)	: type_(type), data_(clamp(round(1000 * phase), 0, 1000)) {}

		bool operator == (GameOptionType type) const { return type_ == type; }
		
		GameOptionType type_;
		int data_;

		void serialize(Archive& ar);
	};
	typedef vector<Option> Options;

	int raw2filtered(GameOptionType type, int data) const;
	int filtered2raw(GameOptionType type, int data) const;

public:
	GameOptions();
	
	STARFORCE_API void serialize(Archive& ar);
	
	void serializePresets(Archive& ar);
	void loadPresets();
	void savePresets();
	
	void serializeForEditor(Archive& ar, int groupsMask = GRAPHICS | SOUND);

	/// получение реальных значений параметров
	bool getBool(GameOptionType type) const;
	float getFloat(GameOptionType type) const;
	int getInt(GameOptionType type) const{ return round(getFloat(type)); }

	const Vect2i& getScreenSize() const { return currentResolution_; }
	const char* getLanguage() const;
	const char* getLocDataPath() const;

	// акцессеры только для UI 
	/// получить строку разделенную | значений для выбора
	const char* getList(GameOptionType type) const;
	/// получить строку с названиями пресетов разделенную |
	const char* getPresetList() const;
	/// получить comboList всех возможных(включая недоступные) значения списка
	const char* getUnfilteredList(GameOptionType type) const;
	/// номер пресета из списка с которым совпадают текущие опции
	int getCurrentPreset() const;
	/// акцессер для получения значения параметра, float'ы домножены на 1000
	int getOption(GameOptionType type) const;
	/// установка значения параметра. float'ы домножить на 1000
	void setOption(GameOptionType type, int data);
	/// загрузка предустановленных значений из вектора пресетов
	void loadPresets(int number);
	/// надо ли применять изменения параметра сразу
	bool needInstantApply(GameOptionType type) const { return gameOptionPrms_[type].needInstantApply(); }
	/// надо применить временные настройки
	void setPartialOptionsApply();


	/// применить значения параметров к текущей игре
	void userApply(bool silent = false);


	void revertChanges();
	enum { COMMIT_TIME = 15	};
	void commitQuant(float dt);
	bool needCommit() const { return commitTimer_ > FLT_EPS; }
	bool needRollBack() const { return commitTimer_ < -FLT_EPS; }
	float commitTimeAmount() const { return max(commitTimer_, 0.f); }
	void commitSettings();
	void restoreSettings();

	void graphSetup();
	void gameSetup();
	void environmentSetup();

	void setTranslate();
	UI_CommonLocText getTranslate() const;
	void filterGameOptions();
	void filterBaseGraphOptions();
	void filterGraphOptions();

private:
	bool presetSerialization_;
	
	GameOptionPrms gameOptionPrms_;
	Options options_;
	
	Vect2i currentResolution_;
	
	Options optionsSaved_;
	float commitTimer_;

	typedef vector<Vect2i> Resolutions;
    Resolutions resolutions_;

	typedef vector<D3DMULTISAMPLE_TYPE> AntialiasModes;
	AntialiasModes antialiasModes_;

	string locDataRoot_;
	mutable string locDataPath_;
	// пара: (папка в LocData, название языка)
	typedef vector<pair<string, UI_CommonLocText> > LocDataFolders;
	LocDataFolders locDataFolders_;

	typedef vector<int> AnisatropicFilteringModes;
	AnisatropicFilteringModes anisatropicFiltering_;

	struct GameOptionsPreset{
		LocString locName_;
		Options preset_;
		
		const char* name() const { return locName_.c_str(); }
		void serialize(Archive& ar);
	};
	typedef std::vector<GameOptionsPreset> Presets;
	Presets presets_;
	
	void defineGlobalVars();

	Option* getOptionObject(GameOptionType type){
		GameOptions::Options::iterator it;
		if((it = std::find(options_.begin(),options_.end(), type)) != options_.end())
			return &*it;
		return 0;
	}
	
	const Option* const getOptionObject(GameOptionType type) const{
		GameOptions::Options::const_iterator it;
		if((it = std::find(options_.begin(),options_.end(), type)) != options_.end())
			return &*it;
		return 0;
	}
	
	void setOptions(const Options& opt);
};

#endif //_UI_GAME_OPTIONS_H
