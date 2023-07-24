#include "StdAfx.h"
#include "SafeCast.h"
#include "GameOptions.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "ComboVectorString.h"
#include "RangedWrapper.h"
#include "TextDB.h"
#include "VideoMemoryInformation.h"

// коммент не удалять!  нужно для перевода: _VISTA_ENGINE_EXTERNAL_

bool isUnderEditor();

WRAP_LIBRARY(GameOptions, "GameOptions", "Игровые опции", (isUnderEditor() ? "Scripts\\Content\\GameOptionsEditor" : "Scripts\\Content\\GameOptions"), 0, false);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(GameOptions, KindType, "GameOptions::KindType")
REGISTER_ENUM_ENCLOSED(GameOptions, SYSTEM, "Системные опции");
REGISTER_ENUM_ENCLOSED(GameOptions, GAME, "Общеигровые опции");
REGISTER_ENUM_ENCLOSED(GameOptions, GRAPHICS, "Графические настройки");
REGISTER_ENUM_ENCLOSED(GameOptions, SOUND, "Звуковые настройки");
REGISTER_ENUM_ENCLOSED(GameOptions, CAMERA, "Опции камеры");
END_ENUM_DESCRIPTOR_ENCLOSED(GameOptions, KindType)

string getStringTokenByIndex(const char*, int);
int indexInComboListString(const char* combo_string, const char* value);
void splitComboList(ComboStrings& combo_array, const char* combo_string, char delimeter);

GameOptions::GameOptions()
{
	locDataRoot_ = "Resource\\LocData\\";
	LocLibraryWrapperBase::locDataRootPath() = locDataRoot_;

	locDataFolders_.push_back(make_pair(string("english"),	UI_COMMON_TEXT_LANG_ENGLISH));
	locDataFolders_.push_back(make_pair(string("russian"),	UI_COMMON_TEXT_LANG_RUSSIAN));
	locDataFolders_.push_back(make_pair(string("german"),	UI_COMMON_TEXT_LANG_GERMAN));
	locDataFolders_.push_back(make_pair(string("french"),	UI_COMMON_TEXT_LANG_FRENCH));
	locDataFolders_.push_back(make_pair(string("spanish"),	UI_COMMON_TEXT_LANG_SPANISH));
	locDataFolders_.push_back(make_pair(string("italian"),	UI_COMMON_TEXT_LANG_ITALIAN));
	
	resolutions_.push_back(Vect2i(640, 480));
	resolutions_.push_back(Vect2i(720, 480));
	resolutions_.push_back(Vect2i(720, 576));
	resolutions_.push_back(Vect2i(800, 600));
	resolutions_.push_back(Vect2i(848, 480));
	resolutions_.push_back(Vect2i(852, 480));
	resolutions_.push_back(Vect2i(960, 600));
	resolutions_.push_back(Vect2i(1024, 600));
	resolutions_.push_back(Vect2i(1024, 768));
	resolutions_.push_back(Vect2i(1024, 800));
	resolutions_.push_back(Vect2i(1088, 612));
	resolutions_.push_back(Vect2i(1152, 864));
	resolutions_.push_back(Vect2i(1280, 720));
	resolutions_.push_back(Vect2i(1280, 768));
	resolutions_.push_back(Vect2i(1280, 800));
	resolutions_.push_back(Vect2i(1280, 960));
	resolutions_.push_back(Vect2i(1280, 1024));
	resolutions_.push_back(Vect2i(1360, 768));
	resolutions_.push_back(Vect2i(1366, 768));
	resolutions_.push_back(Vect2i(1400, 1050));
	resolutions_.push_back(Vect2i(1440, 900));
	resolutions_.push_back(Vect2i(1600, 900));
	resolutions_.push_back(Vect2i(1600, 1024));
	resolutions_.push_back(Vect2i(1600, 1200));
	resolutions_.push_back(Vect2i(1680, 1050));
	resolutions_.push_back(Vect2i(1920, 1080));
	resolutions_.push_back(Vect2i(1920, 1200));
	resolutions_.push_back(Vect2i(1920, 1440));
	resolutions_.push_back(Vect2i(2048, 1080));
	resolutions_.push_back(Vect2i(2048, 1536));

	antialiasModes_.push_back(D3DMULTISAMPLE_NONE);
	antialiasModes_.push_back(D3DMULTISAMPLE_2_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_3_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_4_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_5_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_6_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_7_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_8_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_9_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_10_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_11_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_12_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_13_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_14_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_15_SAMPLES);
	antialiasModes_.push_back(D3DMULTISAMPLE_16_SAMPLES);

	anisatropicFiltering_.push_back(0);
	anisatropicFiltering_.push_back(2);
	anisatropicFiltering_.push_back(4);
	anisatropicFiltering_.push_back(8);
	anisatropicFiltering_.push_back(16);

	// настройки опций, должны быть заполнены для каждой опции, не сериализуются, не меняются
	
	gameOptionPrms_.clear();
	gameOptionPrms_.resize(OPTION_ENUM_SIZE);
	
	gameOptionPrms_[OPTION_LANGUAGE]				.set(GAME|INDEXED,		TYPE_LIST, 0, locDataFolders_.size()-1);
	gameOptionPrms_[OPTION_GAMMA]					.set(GRAPHICS|INSTANT,	TYPE_PHASA, 0.25, 1.75);
	gameOptionPrms_[OPTION_FULL_SCREEN]				.set(GRAPHICS|CRITICAL,	TYPE_BOOL);
	gameOptionPrms_[OPTION_SCREEN_SIZE]				.set(GRAPHICS|CRITICAL|INDEXED, TYPE_LIST, 0, resolutions_.size()-1);
	gameOptionPrms_[OPTION_ANTIALIAS]				.set(GRAPHICS|CRITICAL|INDEXED, TYPE_LIST, 0, antialiasModes_.size()-1);
	gameOptionPrms_[OPTION_ANISOTROPY]				.set(GRAPHICS|INDEXED,	TYPE_LIST, 0, anisatropicFiltering_.size()-1);
	gameOptionPrms_[OPTION_MAP_LEVEL_LOD]			.set(GRAPHICS,	TYPE_PHASA,	20, 100);
	gameOptionPrms_[OPTION_SHADOW]					.set(GRAPHICS,	TYPE_LIST,	0, 3);
	gameOptionPrms_[OPTION_TILEMAP_DETAIL]			.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_TEXTURE_DETAIL_LEVEL]	.set(GRAPHICS|INDEXED,	TYPE_LIST,	0, 2);
	gameOptionPrms_[OPTION_TILEMAP_TYPE_NORMAL]		.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_REFLECTION]				.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_PARTICLE_RATE]			.set(GRAPHICS,	TYPE_PHASA,	0.1f, 1.f);
	gameOptionPrms_[OPTION_BUMP]					.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_SILHOUETTE]				.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_SOFT_SMOKE]				.set(GRAPHICS,	TYPE_LIST,	0, 2);
	gameOptionPrms_[OPTION_BLOOM]					.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_MIRAGE]					.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_WEATHER]					.set(GRAPHICS,	TYPE_BOOL);
	gameOptionPrms_[OPTION_GRASSDENSITY]			.set(GRAPHICS,	TYPE_PHASA);

	gameOptionPrms_[OPTION_CAMERA_RESTRICTION]		.set(CAMERA,	TYPE_BOOL);
	gameOptionPrms_[OPTION_CAMERA_UNIT_FOLLOW]		.set(CAMERA,	TYPE_BOOL);
	gameOptionPrms_[OPTION_CAMERA_UNIT_DOWN_FOLLOW]	.set(CAMERA,	TYPE_BOOL);
	gameOptionPrms_[OPTION_CAMERA_UNIT_ROTATE]		.set(CAMERA,	TYPE_BOOL);
	gameOptionPrms_[OPTION_CAMERA_ZOOM_TO_CURSOR]	.set(CAMERA,	TYPE_BOOL);
	gameOptionPrms_[OPTION_CAMERA_INVERT_MOUSE]		.set(CAMERA,	TYPE_BOOL);

	gameOptionPrms_[OPTION_SOUND_ENABLE]			.set(SOUND|INSTANT, TYPE_BOOL);
	gameOptionPrms_[OPTION_SOUND_VOLUME]			.set(SOUND|INSTANT, TYPE_PHASA);
	gameOptionPrms_[OPTION_MUSIC_ENABLE]			.set(SOUND|INSTANT, TYPE_BOOL);
	gameOptionPrms_[OPTION_MUSIC_VOLUME]			.set(SOUND|INSTANT, TYPE_PHASA);
	gameOptionPrms_[OPTION_VOICE_ENABLE]			.set(SOUND|INSTANT, TYPE_BOOL);
	gameOptionPrms_[OPTION_VOICE_VOLUME]			.set(SOUND|INSTANT, TYPE_PHASA);

	gameOptionPrms_[OPTION_SHOW_HINTS]				.set(GAME,		TYPE_BOOL);
	gameOptionPrms_[OPTION_SHOW_MESSAGES]			.set(GAME,		TYPE_BOOL);
	gameOptionPrms_[OPTION_FOG_OF_WAR]				.set(SYSTEM,	TYPE_BOOL);

	gameOptionPrms_[OPTION_DEBUG_WINDOW]			.set(SYSTEM,	TYPE_BOOL);

	// значения опций по умолчанию, значения сериализуются (новые добавлять нельзя), тип остается неизменным
	
	options_.clear();

	options_.push_back(Option(OPTION_LANGUAGE, 0));

	options_.push_back(Option(OPTION_FULL_SCREEN, true));
	options_.push_back(Option(OPTION_SCREEN_SIZE, 8));

	options_.push_back(Option(OPTION_CAMERA_RESTRICTION, true));
	options_.push_back(Option(OPTION_CAMERA_ZOOM_TO_CURSOR, false));
	options_.push_back(Option(OPTION_CAMERA_UNIT_DOWN_FOLLOW, true));
	options_.push_back(Option(OPTION_CAMERA_UNIT_ROTATE, true));
	options_.push_back(Option(OPTION_CAMERA_UNIT_FOLLOW, true));
	options_.push_back(Option(OPTION_CAMERA_INVERT_MOUSE, false));

	options_.push_back(Option(OPTION_GAMMA, 0.5f)); // 0.5 при границах (0.25, 1.75) соответствует гамме = 1.0
	options_.push_back(Option(OPTION_MAP_LEVEL_LOD, 0.8f));
	options_.push_back(Option(OPTION_SHADOW, 2));
	options_.push_back(Option(OPTION_TILEMAP_DETAIL, true));
	options_.push_back(Option(OPTION_TEXTURE_DETAIL_LEVEL, 1));
	options_.push_back(Option(OPTION_TILEMAP_TYPE_NORMAL, true));
	options_.push_back(Option(OPTION_REFLECTION, false));
	options_.push_back(Option(OPTION_PARTICLE_RATE, 0.6f));
	options_.push_back(Option(OPTION_BUMP, true));
	options_.push_back(Option(OPTION_FOG_OF_WAR, true));
	options_.push_back(Option(OPTION_ANTIALIAS, 0));
	options_.push_back(Option(OPTION_ANISOTROPY, 0));
	options_.push_back(Option(OPTION_SILHOUETTE, true));
	options_.push_back(Option(OPTION_SOFT_SMOKE, 0));
	options_.push_back(Option(OPTION_BLOOM, false));
	options_.push_back(Option(OPTION_MIRAGE, true));
	options_.push_back(Option(OPTION_WEATHER, true));
	options_.push_back(Option(OPTION_GRASSDENSITY,0.4f));

	options_.push_back(Option(OPTION_SOUND_ENABLE, true));
	options_.push_back(Option(OPTION_SOUND_VOLUME, .5f));
	options_.push_back(Option(OPTION_MUSIC_ENABLE, true));
	options_.push_back(Option(OPTION_MUSIC_VOLUME, .5f));
	options_.push_back(Option(OPTION_VOICE_ENABLE, true));
	options_.push_back(Option(OPTION_VOICE_VOLUME, .5f));

	options_.push_back(Option(OPTION_SHOW_HINTS, true));
	options_.push_back(Option(OPTION_SHOW_MESSAGES, true));

	options_.push_back(Option(OPTION_DEBUG_WINDOW, false));

	commitTimer_ = 0.f;
	optionsSaved_ = options_;
	currentResolution_ = resolutions_[getOption(OPTION_SCREEN_SIZE)];
	presetSerialization_ = false;

	filterGameOptions();
	defineGlobalVars();

	loadPresets();
}

void GameOptions::setTranslate()
{
	XBuffer buf;
	buf < GET_LOC_STR(UI_COMMON_TEXT_YES) < "|" < GET_LOC_STR(UI_COMMON_TEXT_NO);
	GameOptionPrms::iterator prm;
	FOR_EACH(gameOptionPrms_, prm)
		if(prm->Vtype_ == TYPE_BOOL)
			prm->setTranslate(buf);
	
	buf.init();
	for(int i = 0; i < locDataFolders_.size(); ++i)
		buf < getLocString(locDataFolders_[i].second, getEnumName(locDataFolders_[i].second)) < "|";
	buf-=1; buf < '\0';
	gameOptionPrms_[OPTION_LANGUAGE].setTranslate(buf);

	buf.init();
	for(int i = 0; i < resolutions_.size(); ++i)
		buf <= resolutions_[i].x < "*" <= resolutions_[i].y < "|";
	buf-=1; buf < '\0';
	gameOptionPrms_[OPTION_SCREEN_SIZE].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_DISABLED);
	for(int i = 1; i < antialiasModes_.size(); ++i)
		buf < "|" <= (int)antialiasModes_[i] < "x";
	gameOptionPrms_[OPTION_ANTIALIAS].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_DISABLED);
	for(int i = 1; i < anisatropicFiltering_.size(); ++i)
		buf < "|" <= (int)anisatropicFiltering_[i] < "x";
	gameOptionPrms_[OPTION_ANISOTROPY].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_DISABLED) < "|" < GET_LOC_STR(UI_COMMON_TEXT_SHADOW_CIRCLE) < "|"
		< GET_LOC_STR(UI_COMMON_TEXT_SHADOW_BAD) < "|" < GET_LOC_STR(UI_COMMON_TEXT_SHADOW_GOOD);
	gameOptionPrms_[OPTION_SHADOW].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_TEXTURE_LOW) < "|" < GET_LOC_STR(UI_COMMON_TEXT_TEXTURE_MEDIUM) < "|" < GET_LOC_STR(UI_COMMON_TEXT_TEXTURE_GOOD);
	gameOptionPrms_[OPTION_TEXTURE_DETAIL_LEVEL].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_DISABLED) < "|" < GET_LOC_STR(UI_COMMON_TEXT_EFFECT_SMOOTH_TERRAIN) < "|" < GET_LOC_STR(UI_COMMON_TEXT_EFFECT_SMOOTH_FULL);
	gameOptionPrms_[OPTION_SOFT_SMOKE].setTranslate(buf);

	buf.init();
	buf < GET_LOC_STR(UI_COMMON_TEXT_HIGH_QA) < "|" < GET_LOC_STR(UI_COMMON_TEXT_LOW_QA);
	gameOptionPrms_[OPTION_TILEMAP_TYPE_NORMAL].setTranslate(buf);
}

UI_CommonLocText GameOptions::getTranslate() const
{
	return locDataFolders_[getInt(OPTION_LANGUAGE)].second;
}

void GameOptions::filterBaseGraphOptions()
{
	OptionPrm& prmSize = gameOptionPrms_[OPTION_SCREEN_SIZE];
	dassert(resolutions_.size() == prmSize.valid_.size());

	vector<Vect2i> SSmask;
	//gb_RenderDevice->getScreenSizes(RENDERDEVICE_MODE_STENCIL, SSmask);
	::getSupportedResolutions(0, true, true, false, SSmask);

	for(int i = 0; i < resolutions_.size(); ++i)
		prmSize.valid_[i] = find(SSmask.begin(), SSmask.end(), resolutions_[i]) != SSmask.end();

	Option* opt = getOptionObject(OPTION_SCREEN_SIZE);
	xassert(opt);
	opt->data_ = raw2filtered(OPTION_SCREEN_SIZE, opt->data_);

	currentResolution_ = resolutions_[getInt(OPTION_SCREEN_SIZE)];

	OptionPrm& prmAA = gameOptionPrms_[OPTION_ANTIALIAS];
	dassert(antialiasModes_.size() == prmAA.valid_.size());

	vector<DWORD> AAmask;
	::CheckDeviceType(0, currentResolution_.x, currentResolution_.y, true, true, false, &AAmask);
	
	for(int i = 1; i < antialiasModes_.size(); ++i)
		prmAA.valid_[i] = find(AAmask.begin(), AAmask.end(), antialiasModes_[i]) != AAmask.end();

	opt = getOptionObject(OPTION_ANTIALIAS);
	xassert(opt);
	opt->data_ = raw2filtered(OPTION_ANTIALIAS, opt->data_);

	OptionPrm& texDetail = gameOptionPrms_[OPTION_TEXTURE_DETAIL_LEVEL];
	int vidMemory = GetVideoMemory();
	if(vidMemory > 0 && vidMemory < 128*1024*1024)
		texDetail.valid_.back() = false;

	opt = getOptionObject(OPTION_TEXTURE_DETAIL_LEVEL);
	xassert(opt);
	opt->data_ = raw2filtered(OPTION_TEXTURE_DETAIL_LEVEL, opt->data_);

	optionsSaved_ = options_;

	defineGlobalVars();
}

void GameOptions::filterGameOptions()
{ // тут проверять независимые ни от чего настройки, например список языков
	OptionPrm& prmLang = gameOptionPrms_[OPTION_LANGUAGE];
	dassert(locDataFolders_.size() == prmLang.valid_.size());

	vector<string> LDmask;
	WIN32_FIND_DATA FindFileData;
	string Mask = locDataRoot_;
	Mask += "*";
	HANDLE hf = FindFirstFile(Mask.c_str(), &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				LDmask.push_back(string(strlwr(FindFileData.cFileName)));
		}while(FindNextFile(hf, &FindFileData));
		FindClose(hf);
	}

	if(LDmask.size() == 0){
		dassert(0 && "missing data for localization");
		ErrH.Abort("missing data for localization", XERR_CRITICAL);
		return;
	}

	for(int i = 0; i < locDataFolders_.size(); ++i)
		prmLang.valid_[i] = find(LDmask.begin(), LDmask.end(), locDataFolders_[i].first) != LDmask.end();

	Option* opt = getOptionObject(OPTION_LANGUAGE);
	xassert(opt);
	opt->data_ = raw2filtered(OPTION_LANGUAGE, opt->data_);
}

void GameOptions::Option::serialize(Archive& ar)
{
	if(!ar.isEdit())
		ar.serialize(type_, "type", "&Параметр");
	else if(GameOptions::instance().presetSerialization_){
		XBuffer buf(256, 1);
		int data = 0;
		int idx = 0;
		for(int i = 0; i < OPTION_ENUM_SIZE; ++i)
			if(GameOptions::instance().gameOptionPrms_[i].type() == GRAPHICS){
				buf < getEnumNameAlt((GameOptionType)i) < "|";
				if(i == type_)
					data = idx;
				idx++;
			}
		--buf; buf < '\0';
		ComboVectorString lst(buf, data);
		ar.serialize(lst, "type", "&Параметр");
		data = lst.value();
		for(int i = 0; i < OPTION_ENUM_SIZE; ++i)
			if(GameOptions::instance().gameOptionPrms_[i].type() == GRAPHICS)
				if(!data){
					type_ = (GameOptionType)i;
					break;
				}
				else
					--data;
	}

	if(type_ >= 0 && type_ < GameOptions::instance().gameOptionPrms_.size()){
		switch(GameOptions::instance().gameOptionPrms_[type_].Vtype_){
		case TYPE_PHASA:{
			float tmp = data_ / 1000.f;
			if(GameOptions::instance().presetSerialization_ || !ar.isEdit())
				ar.serialize(RangedWrapperf(tmp, 0.f, 1.f), "phase", "Значение");
			else
				ar.serialize(RangedWrapperf(tmp, 0.f, 1.f), getEnumName(type_), getEnumNameAlt(type_));
			data_ = round(tmp * 1000.0f);
			break;
						}

		case TYPE_BOOL:
			if(ar.isEdit()){
				ComboVectorString lst(GameOptions::instance().getList(type_), data_);
				ar.serialize(lst, getEnumName(type_), getEnumNameAlt(type_));
				data_ = max(0, lst.value());
			}
			else {
				bool tmp = data_ ? false : true;
				ar.serialize(tmp, "flag", "Значение");
				data_ = tmp ? 0 : 1;
			}
			break;

		case TYPE_LIST:
			if(ar.isEdit()){
				if(GameOptions::instance().presetSerialization_){
					ComboVectorString lst(GameOptions::instance().gameOptionPrms_[type_].valueList_.c_str(), data_);
					ar.serialize(lst, "number", "Значение");
					data_ = max(0, lst.value());
				}
				else {
					ComboVectorString lst(GameOptions::instance().getList(type_), data_);
					ar.serialize(lst, getEnumName(type_), getEnumNameAlt(type_));
					data_ = max(0, lst.value());
				}
			}
			else{
				if(GameOptions::instance().presetSerialization_)
					ar.serialize(data_, "number", 0);
				else {
					data_ = GameOptions::instance().filtered2raw(type_, data_);
					ar.serialize(data_, "number", 0);
					data_ = GameOptions::instance().raw2filtered(type_, data_);
					xassert(data_ >= 0);
				}
				if(ar.isOutput() && !GameOptions::instance().presetSerialization_)
					ar.serialize(GameOptions::instance().gameOptionPrms_[type_].valueList_, "comment", 0);
			}
			break;

		default:
			xxassert(false, "Супер бага");
		}
	}
}

void GameOptions::GameOptionsPreset::serialize(Archive& ar)
{
	ar.serialize(locName_, "locName", "(loc) Название пресета");
	ar.serialize(preset_, "preset", "значения");
}

void GameOptions::serializePresets(Archive& ar)
{
	presetSerialization_ = true;
	ar.serialize(presets_, "presets", "Наборы графических настроек");
}

void GameOptions::loadPresets()
{
	XPrmIArchive ar;
	if(ar.open("Scripts\\Engine\\GraphPresets"))
		serializePresets(ar);
}

void GameOptions::savePresets()
{
	XPrmOArchive ar("Scripts\\Engine\\GraphPresets");
	serializePresets(ar);
}

void GameOptions::serializeForEditor(Archive& ar, int groupsMask)
{
	presetSerialization_ = false;
	if(!(groupsMask & GAME))
		getOptionObject(OPTION_LANGUAGE)->serialize(ar);

	const EnumDescriptor& descriptor = getEnumDescriptor(SYSTEM);
	ComboStrings strings;
	splitComboList(strings, descriptor.comboListAlt());

	ComboStrings::iterator groupIt;
	FOR_EACH(strings, groupIt){
		const char* nameAlt = groupIt->c_str();
		int key = descriptor.keyByNameAlt(nameAlt);
		if(groupsMask & key){
			ar.openBlock("", getEnumNameAlt<KindType>(KindType(key))); // использовать nameAlt НЕЛЬЗЯ, т.к. openBlock не сохраняет копию строки, только const char*

			Options::iterator it;
			FOR_EACH(options_, it)
				if(gameOptionPrms_[it->type_].type() == key)
					it->serialize(ar);

			ar.closeBlock();
		}
	}

	if(ar.isInput() && ar.isEdit()){
		TextDB::instance().loadLanguage(getLanguage());
		CommonLocText::instance().update();
		setTranslate();
	}
}

void GameOptions::serialize(Archive& ar)
{
	presetSerialization_ = false;
	if(ar.isEdit()){
		if(isUnderEditor())
			serializeForEditor(ar);
		else {
			Options::iterator it;
			FOR_EACH(options_, it)
				it->serialize(ar);
		}
	}
	else if(ar.isOutput())
		ar.serialize(options_, "options_", 0);
	else{
		Options tmp;			
		ar.serialize(tmp, "options_", 0);
		setOptions(tmp);
		optionsSaved_ = options_;
	}
	
	if(ar.isEdit() && isUnderEditor())
		serializePresets(ar);
	
	if(ar.isInput())
		defineGlobalVars();
}

int GameOptions::raw2filtered(GameOptionType type, int data) const
{
	if(gameOptionPrms_[type].isIndexed()){
		const OptionPrm& opt = gameOptionPrms_[type];

		// вернуть количество разрешенных настроек вплоть до data 
		int filtered = 0;
		for(int raw = 0; raw < opt.valid_.size(); ++raw){
			if(opt.valid_[raw])
				++filtered;
			if(raw == data)
				break;
		}

		return max(filtered-1, 0);
	}

	return data;
}

int GameOptions::filtered2raw(GameOptionType type, int data) const
{
	if(gameOptionPrms_[type].isIndexed()){
		// вернуть номер N-ой разрешенной настройки
		const OptionPrm& opt = gameOptionPrms_[type];
		int raw = 0;
		for(; raw < opt.valid_.size(); ++raw)
			if(opt.valid_[raw])
				if(!data--)
					return raw;
		return 0;
	}
	return data;
}

void GameOptions::OptionPrm::set(int kindType, ValueType valueType, float min, float max, const char* lst)
{
	kindType_ = kindType;
	Vtype_ = valueType;

	xassert(Vtype_ == TYPE_PHASA || min == 0.f);
	min_ = min;
	max_ = max;
	xassert(max_ >= min_);

	if(lst)
		valueList_ = lst;

	if(isIndexed()){
		xassert(Vtype_ == TYPE_LIST);
		valid_.assign(max_ - min_ + 1, true);
	}
}

void GameOptions::OptionPrm::setTranslate(const char* lst)
{
	xassert(lst);
	valueList_ = lst;
}

void GameOptions::setOptions(const Options& opt)
{
	Options::const_iterator it;
	FOR_EACH(opt, it)
		if(Option* opt = getOptionObject(it->type_))
			opt->data_ = clamp(it->data_, 0, 1000);
}

const char* GameOptions::getUnfilteredList(GameOptionType type) const
{
	const OptionPrm& prm = gameOptionPrms_[type];
	return prm.valueList_.c_str();
}

const char* GameOptions::getPresetList() const
{
	if(presets_.empty())
		return "";

	static string out;
	out.clear();
	
	Presets::const_iterator it;
	FOR_EACH(presets_, it){
		out += it->name();
		out.push_back('|');
	}
	out.pop_back();

	return out.c_str();
}

int GameOptions::getCurrentPreset() const
{
	Presets::const_iterator pr;
	Options::const_iterator it;
	int current = 0;
	
	FOR_EACH(presets_, pr){
		FOR_EACH(pr->preset_, it)
			if(gameOptionPrms_[it->type_].Vtype_ == TYPE_PHASA ? abs(it->data_ - getOption(it->type_)) > 20 : it->data_ != filtered2raw(it->type_, getOption(it->type_)))
				break;
		if(it == pr->preset_.end())
			break;
		current++;
	}
	
	if(pr == presets_.end())
		return -1;

	return current;
}

const char* GameOptions::getList(GameOptionType type) const
{
	static string out;
	out.clear();

	const OptionPrm& prm = gameOptionPrms_[type];
	
	if(prm.isIndexed()){
		ComboStrings strings;
		splitComboList(strings, prm.valueList_.c_str(), '|');
		dassert(strings.size() == prm.valid_.size());
		string out_tmp;
		for(int i = 0; i < strings.size(); ++i)
			if(prm.valid_[i]){
				out += strings[i];
				out += "|";
			}
		if(!out.empty())
			out.pop_back();
		
		return out.c_str();
	}

	return prm.valueList_.c_str();
}

int GameOptions::getOption(GameOptionType type) const
{
	if(const Option *po = getOptionObject(type)){
		xassert(type >= 0 && type < gameOptionPrms_.size());
		if(gameOptionPrms_[type].Vtype_ != TYPE_PHASA)
			return clamp(po->data_, round(gameOptionPrms_[type].min_), round(gameOptionPrms_[type].max_));
		else
			return po->data_;
	}
	return -1;
}

void GameOptions::setOption(GameOptionType type, int data)
{
	if(Option *po = getOptionObject(type))
		po->data_ = data;

#ifndef _FINAL_VERSION_
	if(Option *po = getOptionObject(type)){
		if(gameOptionPrms_[type].Vtype_ != TYPE_LIST)
			xassert(data >= 0 && data <= 1000);
		else
			xassert(data >= gameOptionPrms_[type].min_ && data <= gameOptionPrms_[type].max_);
	}
#endif
}

bool GameOptions::getBool(GameOptionType type) const
{
	xassert(type >= 0 && type < gameOptionPrms_.size());
	xassert(gameOptionPrms_[type].Vtype_ == TYPE_BOOL);
	if(const Option *po = getOptionObject(type))
		return !po->data_;

	xxassert(false, "отсутствует значение для параметра игры");
	return false;
}

float GameOptions::getFloat(GameOptionType type) const
{
	xassert(type >= 0 && type < gameOptionPrms_.size());
	if(const Option *po = getOptionObject(type)){
		const OptionPrm& prm = gameOptionPrms_[type];
		if(prm.Vtype_ == TYPE_PHASA)
			return  prm.min_ + (prm.max_ - prm.min_) * clamp(.001f * po->data_, 0.f, 1.f);
		else {
			xassert(po->data_ >= 0 && filtered2raw(type, po->data_) <= prm.max_);
			switch(type){
			case OPTION_ANTIALIAS:
				return antialiasModes_[filtered2raw(OPTION_ANTIALIAS, po->data_)];
			case OPTION_ANISOTROPY:
				return anisatropicFiltering_[filtered2raw(OPTION_ANISOTROPY, po->data_)];
			default:
				return filtered2raw(type, po->data_);
			}
		}
	}

	xxassert(false, "отсутствует значение для параметра игры");
	return 0.f;
}

const char* GameOptions::getLanguage() const
{
	return locDataFolders_[getInt(OPTION_LANGUAGE)].first.c_str();
}

const char* GameOptions::getLocDataPath() const
{
	locDataPath_ = getLanguage();
	if(locDataPath_.empty())
		locDataPath_ = "English";
	locDataPath_ = locDataRoot_ + locDataPath_ + "\\"; 
	return locDataPath_.c_str();
}

BEGIN_ENUM_DESCRIPTOR(GameOptionType, "GameOptionType")
REGISTER_ENUM(OPTION_LANGUAGE, "язык")
REGISTER_ENUM(OPTION_GAMMA, "гамма")
REGISTER_ENUM(OPTION_FULL_SCREEN, "полный экран")
REGISTER_ENUM(OPTION_DEBUG_WINDOW, "отладочное окно")
REGISTER_ENUM(OPTION_SCREEN_SIZE, "размер экрана")
REGISTER_ENUM(OPTION_MAP_LEVEL_LOD, "уровень детализации карты")
REGISTER_ENUM(OPTION_TILEMAP_DETAIL, "использовать мелкодетальную текстуру")
REGISTER_ENUM(OPTION_TEXTURE_DETAIL_LEVEL, "детальность текстур")
REGISTER_ENUM(OPTION_SHADOW, "тени")
REGISTER_ENUM(OPTION_TILEMAP_TYPE_NORMAL, "качество освещения")
REGISTER_ENUM(OPTION_REFLECTION, "отражения")
REGISTER_ENUM(OPTION_PARTICLE_RATE, "Интенсивность частиц")
REGISTER_ENUM(OPTION_BUMP, "включить бамп")
REGISTER_ENUM(OPTION_FOG_OF_WAR, "туман войны")
REGISTER_ENUM(OPTION_ANTIALIAS, "качество антиалиасинга")
REGISTER_ENUM(OPTION_ANISOTROPY, "анизотропная фильтрация")
REGISTER_ENUM(OPTION_SILHOUETTE, "включить силуэты")
REGISTER_ENUM(OPTION_SOFT_SMOKE, "сглаживание эффектов")
REGISTER_ENUM(OPTION_CAMERA_RESTRICTION, "включить ограничение камеры")
REGISTER_ENUM(OPTION_CAMERA_UNIT_FOLLOW, "привязать камеру к юниту при прямом управлении")
REGISTER_ENUM(OPTION_CAMERA_UNIT_DOWN_FOLLOW, "привязанный юнит внизу экрана")
REGISTER_ENUM(OPTION_CAMERA_UNIT_ROTATE, "поворачивать камеру за юнитом")
REGISTER_ENUM(OPTION_CAMERA_ZOOM_TO_CURSOR, "зум в курсор")
REGISTER_ENUM(OPTION_CAMERA_INVERT_MOUSE, "инвертировать мышь в прямом управление");
REGISTER_ENUM(OPTION_SOUND_ENABLE, "включить звуки")
REGISTER_ENUM(OPTION_SOUND_VOLUME, "громкость звуков")
REGISTER_ENUM(OPTION_MUSIC_ENABLE, "включить музыку")
REGISTER_ENUM(OPTION_MUSIC_VOLUME, "громкость музыки")
REGISTER_ENUM(OPTION_VOICE_ENABLE, "включить звуковые сообщения")
REGISTER_ENUM(OPTION_VOICE_VOLUME, "громкость звуковых сообщений")
REGISTER_ENUM(OPTION_SHOW_HINTS, "показывать подсказки")
REGISTER_ENUM(OPTION_SHOW_MESSAGES, "показывать сообщения")
REGISTER_ENUM(OPTION_BLOOM,"включить эффект свечения")
REGISTER_ENUM(OPTION_GRASSDENSITY,"плотность травы")
REGISTER_ENUM(OPTION_MIRAGE,"включить эффект миража/подводного колебания")
REGISTER_ENUM(OPTION_WEATHER,"включить погодные эффекты")
END_ENUM_DESCRIPTOR(GameOptionType)
