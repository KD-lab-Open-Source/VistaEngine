#include "stdafx.h"
#include "ProfileManager.h"

#include "Serialization.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "UnitAttribute.h"
#include "GlobalAttributes.h"

GUIDcontainer Profile::MissionFilter::EMPTY;

Profile::Profile(const string& _dirName) : 
dirName(_dirName)
{
	dirIndex = atoi((dirName.substr(7)).c_str());
	init();
}

Profile::Profile() 
{
	dirIndex = 0;
	init();
}

void Profile::init()
{
	playersSlotFilter = -1;
	gameTypeFilter = -1;
	statisticFilterRace = 0;
	statisticFilterGamePopulation = 0;
	quickStartFilterRace = -1;
	quickStartFilterGamePopulation = 0;
	parametersByRace.insert(parametersByRace.end(), RaceTable::instance().size(), GlobalAttributes::instance().profileParameters);
}

void Profile::MissionFilter::serialize(Archive& ar)
{
	ar.serialize(filterDisabled, "filterDisabled", 0);
	if(!filterDisabled)
		ar.serialize(filterList, "filterList", 0);
}

STARFORCE_API void Profile::serialize(Archive& ar)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(9);

	ar.serialize(name, "name", 0);
	ar.serialize(lastCreateGameName, "lastCreateGameName", 0);
	ar.serialize(lastSaveGameName, "lastSaveGameName", 0);
	ar.serialize(lastSaveReplayName, "lastSaveReplayName", 0);
	ar.serialize(cdKey, "cdKey", 0);
	ar.serialize(lastInetName, "lastNetLogin", 0);
	ar.serialize(onlineLogins, "onlineLogins", 0);
	ar.serialize(chatChannel, "chatChannel", 0);
	ar.serialize(findMissionFilter, "findMissionFilter", 0);
	ar.serialize(quickStartMissionFilter, "quickStartMissionFilter", 0);
	ar.serialize(quickStartFilterRace, "quickStartFilterRace", 0);
	ar.serialize(quickStartFilterGamePopulation, "quickStartFilterGamePopulation", 0);
	ar.serialize(playersSlotFilter, "playersSlotFilter", 0);
	ar.serialize(gameTypeFilter, "gameTypeFilter", 0);
	ar.serialize(statisticFilterRace, "statisticFilterRace", 0);
	ar.serialize(statisticFilterGamePopulation, "statisticFilterGamePopulation", 0);
	intVariables.serialize(ar, "intVariables", 0);
	ar.serialize(parametersByRace, "parametersByRace", 0);

	SECUROM_MARKER_HIGH_SECURITY_OFF(9);
}
const string ProfileManager::rootPath("Resource\\Saves\\");

ProfileManager::ProfileManager() :
currentProfileIndex_(-1)
{
	reset();
	scanProfiles();
}

ProfileManager::~ProfileManager()
{
	saveState();
}

void ProfileManager::scanProfiles() {
	profiles_.clear();

	int maxIndex = -1;

	WIN32_FIND_DATA FindFileData;
	string Mask = rootPath;
	Mask += "Profile*";
	HANDLE hf = FindFirstFile(Mask.c_str(), &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				profiles_.push_back( Profile(FindFileData.cFileName) );
			}
		} while(FindNextFile( hf, &FindFileData ));
		FindClose( hf );
	}
	for (int i = 0, s = profiles_.size(); i < s; i++) {
		loadProfile(i);
		maxIndex = max(maxIndex, profiles_[i].dirIndex);
	}
	usedProfilesIdx_.clear();
	if (maxIndex != -1) {
		usedProfilesIdx_.resize(maxIndex + 1);
		int i, s;
		for (i = 0, s = usedProfilesIdx_.size(); i < s; i++) {
			usedProfilesIdx_[i] = false;
		}
		for (i = 0, s = profiles_.size(); i < s; i++) {
			usedProfilesIdx_[profiles_[i].dirIndex] = true;
		}
	}
	Profiles::iterator it = profiles_.begin();
	while(it != profiles_.end())
		if((*it).name.empty())
			it = profiles_.erase(it);
		else
			++it;
	readLastCurrentProfile();
}

void ProfileManager::saveState()
{
	if(currentProfileIndex() >= 0){
		saveProfile(currentProfileIndex());
		writeLastCurrentProfile();
	}
}

void ProfileManager::fixProfileDir() const
{
	CreateDirectory(rootPath.c_str(), NULL);
	if(currentProfileIndex() >= 0){
		string path = rootPath + profiles_[currentProfileIndex()].dirName;
		CreateDirectory(path.c_str(), NULL);
	}
}

int ProfileManager::updateProfile(const string& name)
{
	for(int idx = 0; idx < profiles_.size(); ++idx)
		if(profiles_[idx].name == name)
			return idx;

	int freeIdx;
	for(freeIdx = 0; freeIdx < usedProfilesIdx_.size(); ++freeIdx)
		if(!usedProfilesIdx_[freeIdx])
			break;

	char ind[18];
	sprintf(ind, "Profile%d", freeIdx);
	Profile newProfile(ind);
	newProfile.name = name;
	string path = rootPath + newProfile.dirName;

	xassert(newProfile.dirIndex == freeIdx);
	fixProfileDir();
	
	if(CreateDirectory(path.c_str(), NULL)){
		profiles_.push_back(newProfile);
		if(newProfile.dirIndex == usedProfilesIdx_.size())
			usedProfilesIdx_.push_back(true);
		else
			usedProfilesIdx_[newProfile.dirIndex] = true;
		loadProfile(profiles_.size() - 1);
		saveProfile(profiles_.size() - 1);
	}
	else {
		return -1;
		ErrH.Abort("Can't create directory: ", XERR_USER, 0, path.c_str());
	}

	return profiles_.size() - 1;
}

void ProfileManager::removeDirRecursive(const string& dir)
{
	if(dir.substr(0, 1) == "."){
		xassert(false && "À-À-À-À-À-À-À-À-À-À !!!!!");
		ErrH.Exit();
		return;
	}
	WIN32_FIND_DATA findFileData;
	string mask = dir + "*.*";
	HANDLE hf = FindFirstFile(mask.c_str(), &findFileData);
	if(hf != INVALID_HANDLE_VALUE){
		do {
			if(!strcmp(findFileData.cFileName, "."))
				continue;
			if(!strcmp(findFileData.cFileName, ".."))
				continue;
			string file = dir + findFileData.cFileName;
			if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				removeDirRecursive(file + "\\");
			else
				DeleteFile(file.c_str());
		} while(FindNextFile(hf, &findFileData));
		FindClose(hf);
	}
	RemoveDirectory(dir.c_str());
}

void ProfileManager::removeProfile(int index)
{
	xassert(index >= 0 && index < (int)profiles_.size());
	if(index < currentProfileIndex() || currentProfileIndex() == (int)profiles_.size() - 1)
		setCurrentProfile(currentProfileIndex() - 1);

	removeDirRecursive(rootPath + profiles_[index].dirName + "\\");
	
	usedProfilesIdx_[profiles_[index].dirIndex] = false;

	profiles_.erase(profiles_.begin() + index);
}

void ProfileManager::loadProfile(int index)
{
	xassert(index < (int)profiles_.size());
	if(index < 0)
		return;
	string path = getProfileDataFile(index);
	XPrmIArchive ia;
	if(ia.open(path.c_str()))
		ia.serialize(profiles_[index], "Profile", 0);
}

void ProfileManager::saveProfile(int index)
{
	xassert(index < (int)profiles_.size());
	if(index < 0)
		return;
	string path = getProfileDataFile(index);
	fixProfileDir();
	XPrmOArchive ia(path.c_str());
	ia.serialize(profiles_[index], "Profile", 0);
}

bool ProfileManager::setCurrentProfile(int index)
{
	if(index != currentProfileIndex()){
		saveProfile(currentProfileIndex());
		currentProfileIndex_ = index;
		reset();
		return true;
	}
	return false;
}

int ProfileManager::getProfileByName(const string& name) const
{
	int idx;
	for(idx = 0; idx < profiles_.size(); idx++)
		if(profiles_[idx].name == name)
			return idx;

	//if(profiles_.size() > 0)
	//	return 0;
	
	return -1;
}

STARFORCE_API void ProfileManager::serialize(Archive& ar)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(10);

	if(ar.isInput() || currentProfileIndex() >= 0){
		string lastProfileName = profile().name;
		ar.serialize(lastProfileName, "LastProfileName", 0);
		setCurrentProfile(getProfileByName(lastProfileName));
	}
	if(ar.isEdit())
		ar.serialize(profiles_, "profiles", 0);

	SECUROM_MARKER_HIGH_SECURITY_OFF(10);
}

void ProfileManager::readLastCurrentProfile()
{
	string path = getProfileRootDataFile();
	XPrmIArchive ia;
	if(ia.open(path.c_str())){
		ia.serialize(*this, "CurrentProfile", 0);
		return;
	}
	setCurrentProfile(-1);
}

void ProfileManager::writeLastCurrentProfile()
{
	string path = getProfileRootDataFile();
	fixProfileDir();
	XPrmOArchive ia(path.c_str());
	ia.serialize(*this, "CurrentProfile", 0);
}



ProfileManager::SaveType ProfileManager::convertType(GameType type) const
{
	if(type & GAME_TYPE_REEL)
		return REPLAY;
	else if(type & GAME_TYPE_BATTLE)
		return BATTLE;
	else
		return SCENARIO;
}

void ProfileManager::reset()
{
	saves_.assign(SAVE_TYPES_COUNT, MissionCache());
}

string ProfileManager::getSavePath(GameType type)
{
	string path = rootPath + profile().dirName;
	switch(convertType(type)){
		case SCENARIO:
			path += "\\Scenario\\"; 
			break;
		case BATTLE:
			path += "\\Battle\\"; 
			break;
		case REPLAY:
			path += "\\Reel\\"; 
			break;
	}
	return path;
}

const MissionDescriptions& ProfileManager::saves(GameType type)
{
	SaveType saveType = convertType(type);
	if(!saves_[saveType].cached){
		saves_[saveType].missions.readFromDir(getSavePath(type).c_str(), type);
		saves_[saveType].cached = true;
	}
	return saves_[saveType].missions;
}

void ProfileManager::newSave(const MissionDescription& md)
{
	if(!saves_[convertType(md.gameType())].cached)
		return;
	saves_[convertType(md.gameType())].missions.add(md);
}

void ProfileManager::deleteSave(const MissionDescription& md)
{
	if(!saves_[convertType(md.gameType())].cached)
		return;
	saves_[convertType(md.gameType())].missions.remove(md);
}

bool ProfileManager::isSaveExist(const char* save_name, GameType type)
{
	string str = setExtention(save_name, MissionDescription::getExtention(type));
	const MissionDescriptions& list = saves(type);
	for(MissionDescriptions::const_iterator it = list.begin(); it != list.end(); ++it){
		if(!stricmp(it->saveName(), str.c_str()))
			return true;
	}

	return false;
}

bool ProfileManager::isReplayExist(const char* replay_name, GameType type)
{
	string str = setExtention(replay_name, MissionDescription::getExtention(type));
	const MissionDescriptions& list = saves(type);
	for(MissionDescriptions::const_iterator it = list.begin(); it != list.end(); ++it){
		if(!stricmp(it->reelName(), str.c_str()))
			return true;
	}

	return false;
}

bool ProfileManager::isProfileExist(const char* profile_name) const
{
	std::string name(profile_name);

	for(int idx = 0; idx < profiles_.size(); ++idx)
		if(profiles_[idx].name == name)
			return true;

	return false;
}

void ProfileManager::newOnlineLogin() 
{
	if(find(profile().onlineLogins.begin(), profile().onlineLogins.end(), profile().lastInetName) == profile().onlineLogins.end())
		profile().onlineLogins.push_back(profile().lastInetName);
}

void ProfileManager::deleteOnlineLogin() 
{
	ComboStrings::iterator it = find(profile().onlineLogins.begin(), profile().onlineLogins.end(), profile().lastInetName);
	if(it != profile().onlineLogins.end())
		profile().onlineLogins.erase(it);
}