#include "StdAfx.h"
#include "Serialization.h"
#include "SurMapOptions.h"
#include "..\Environment\Environment.h"
#include "ConsoleWindow.h"
#include "Dictionary.h"

#include "..\UserInterface\UI_Types.h"
#include "GameOptions.h"

// will be removed:
#include "XPrmArchive.h"
#include "EditArchive.h"
// ^^^

// коммент не удалять!  нужно для перевода: _VISTA_ENGINE_EXTERNAL_

const char* SurMapOptions::configFile = ".\\UserInterface.cfg";
SurMapOptions surMapOptions;

SurMapOptions::SurMapOptions()
: showPathFinding_(false)
, showSources_(true)
, showCameras_(true)
, hideWorldModels_(false)
{
	showFog_ = false;

	lastToolzerRadius=1;
	lastToolzerForm=0;

	gridSpacing_ = 128;
	enableGrid_ = false;
	gridColor_.set(0, 255, 0, 128);

	//restrictCamera_ = false;
	enableCameraBorder_ = false;
	cameraBorderColor_.set(0, 150, 0, 192);
	cameraBorderMinimapColor_.set(0, 150, 0, 192);
}

SurMapOptions::~SurMapOptions()
{
}

std::string SurMapOptions::getLastDirResource(const char* internalResourcePath) 
{
    std::vector<char> temp(internalResourcePath, internalResourcePath + strlen (internalResourcePath) + 2);
	strlwr(&temp[0]); std::string loweredPath(temp.begin(), temp.end());  

	Map::iterator it;
	for(it=last_dirs_.begin(); it!=last_dirs_.end(); it++){
		if(stricmp(it->first.c_str(), loweredPath.c_str()) == 0)
			break;
	}
	if(it == last_dirs_.end())
		return internalResourcePath;
	else
        return it->second;
}

void SurMapOptions::setLastDirResource(const char* internalResourcePath, const char* selectedDir) 
{
    std::vector<char> temp(internalResourcePath, internalResourcePath + strlen (internalResourcePath) + 2);
	strlwr(&temp [0]); std::string loweredPath(temp.begin(), temp.end());  

    // last_dirs_ [loweredPath] = selectedDir;
	Map::iterator it;
	for(it=last_dirs_.begin(); it!=last_dirs_.end(); it++){
		if(stricmp(it->first.c_str(), loweredPath.c_str()) == 0)
			break;
	}
	if(it == last_dirs_.end()) {
		last_dirs_.push_back(std::pair<std::string, std::string>(loweredPath, selectedDir));
	}
	else {
		it->second = selectedDir;
	}
}

void SurMapOptions::serialize(Archive& ar) 
{
	ar.serialize(last_dirs_, "last_dirs_", 0);
	ar.serialize(dlgBarState, "dlgBarState", 0);
	ar.serialize(dlgBarStateExt, "dlgBarStateExt", 0);
	ar.serialize(lastToolzerRadius, "lastToolzerRadius", 0);
	ar.serialize(lastToolzerForm, "lastToolzerForm", 0);
	ar.serialize(showPathFinding_, "showPathFinding", 0);
	ar.serialize(showPathFindingReferenceUnit_, "showPathFindingReferenceUnit", 0);

	ar.serialize(hideWorldModels_, "hideWorldModels", 0);
	ar.serialize(showSources_, "showSources", 0);

	if(ar.isEdit())
		GameOptions::instance().serializeForEditor(ar, GameOptions::GRAPHICS | GameOptions::SOUND | GameOptions::GAME | GameOptions::CAMERA);

	ar.openBlock("", "Отображение камер/ограниений камеры");
	ar.serialize(enableCameraBorder_, "enableCameraBorder", "Показывать ограничение камеры на мире");
	ar.serialize(cameraBorderColor_, "cameraBorderColor", "Цвет рамки камеры на мире");
	ar.serialize(cameraBorderMinimapColor_, "cameraBorderMinimapColor", "Цвет рамки камеры на миникарте");
	ar.closeBlock();

	ar.openBlock("", "Сетка");
	ar.serialize(enableGrid_, "enableGrid", "Включить");
	ar.serialize(gridSpacing_, "gridSpacing", "Периодичность");
	ar.serialize(gridColor_, "gridColor", "Цвет");
	ar.closeBlock();

	ar.serialize(showFog_, "showFog", "Отображать туман (в редакторе)");

	if(!ar.isEdit()){
		ConsoleWindow::Options consoleOptions = ConsoleWindow::instance().options();
		ar.serialize(consoleOptions, "consoleOptions", 0);
		ConsoleWindow::instance().setOptions(consoleOptions);
	}

	if(ar.isInput())
		GameOptions::instance().userApply(true);
	if(ar.isInput())
		apply();
}

void SurMapOptions::save()
{
	XPrmOArchive archive(configFile);
    serialize(archive);
}

void SurMapOptions::load()
{
	XPrmIArchive archive;
    if(archive.open(configFile)){
        serialize(archive);
    }
}

void SurMapOptions::apply()
{
	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());
	if(environment)
		environment->setFogTempDisabled(!showFog_);
}
