#ifndef _USERSINGLEPROFILE_H
#define _USERSINGLEPROFILE_H

#include "Network\NetPlayer.h"
#include "Units\AttributeReference.h"
#include "xtl\StaticMap.h"
#include "xtl\UniqueVector.h"
#include "FileUtils\XGUID.h"
#include "Parameters.h"
#include "Starforce.h"
#include "Serialization/ComboStrings.h"

class Arhive;

typedef StaticMap<string, int> IntVariables;

struct Profile {
	Profile();
	Profile(const string& _dirName);
	void init();
	STARFORCE_API void serialize(Archive& ar);

	string dirName;
	int dirIndex;
	
	wstring name;
	string cdKey;
	wstring chatChannel;
	
	IntVariables intVariables;
	typedef vector<ParameterSet> ParametersByRace;
	ParametersByRace parametersByRace;

	string lastSaveGameName;
	string lastSaveReplayName;
	
	string lastInetName;
	ComboStrings onlineLogins;

	wstring lastInetDirectAddress;
	string lastCreateGameName;
	
	struct MissionFilter {
		MissionFilter() : filterDisabled(true) {}
		bool filterDisabled;
		GUIDcontainer filterList;
		void serialize(Archive& ar);

		static GUIDcontainer EMPTY;
		const GUIDcontainer& getFilter() const {
			return filterDisabled ? EMPTY : filterList;
		}
	};
	
	MissionFilter findMissionFilter;
	MissionFilter quickStartMissionFilter;

	UniqueVector<XGUID> passedMissions;

	int playersSlotFilter;
	int gameTypeFilter;
	
	int statisticFilterRace;
	int statisticFilterGamePopulation;

	int quickStartFilterRace;
	int quickStartFilterGamePopulation;

	Difficulty difficulty;
};

typedef vector<Profile> Profiles;

class ProfileManager {
public:
	ProfileManager();
	~ProfileManager();

	STARFORCE_API void serialize(Archive& ar);

	int currentProfileIndex() const { return currentProfileIndex_; }
	const Profiles& profilesVector() const { return profiles_; }

	int updateProfile(const wstring& name);
	void removeProfile(int index);

	bool setCurrentProfile(int index);

	Profile& profile() {
		xassert(currentProfileIndex() < (int)profiles_.size());
		static Profile emptyProfile;
		return currentProfileIndex() >= 0 ?
			profiles_[currentProfileIndex()] :
			emptyProfile;
	}

	int getProfileByName(const wstring& name) const;

	void saveState();
	void fixProfileDir() const;

	string getSavePath(GameType type);
	const MissionDescriptions& saves(GameType type);
	void newSave(const MissionDescription& md);
	void deleteSave(const MissionDescription& md);

	bool isSaveExist(const char* save_name, GameType type);
	bool isReplayExist(const char* replay_name, GameType type);
	bool isProfileExist(const wchar_t* profile_name) const;

	void newOnlineLogin();
	void deleteOnlineLogin();

private:
	const string rootPath;

	string getProfileRootDataFile() const { return rootPath + "profile"; }
	string getProfileDataFile(int index) const { return rootPath + profiles_[index].dirName + "\\data"; }

	void scanProfiles();

	void readLastCurrentProfile();
	void writeLastCurrentProfile();

	void removeDirRecursive(const string& dir);
	
	void loadProfile(int index);
	void saveProfile(int index);

	int currentProfileIndex_;

	vector<bool> usedProfilesIdx_;
	Profiles profiles_;

	enum SaveType{
		SCENARIO = 0,
		BATTLE,
		REPLAY,
		SAVE_TYPES_COUNT
	};
	SaveType convertType(GameType type) const;

	struct MissionCache{
		bool cached;
		MissionDescriptions missions;
		MissionCache() : cached(false) {}
	};
	vector<MissionCache> saves_;
	void reset();
};

#endif //_USERSINGLEPROFILE_H
