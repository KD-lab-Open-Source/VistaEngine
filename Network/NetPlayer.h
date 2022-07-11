#ifndef __PERIMETER_PLAYER_H__
#define __PERIMETER_PLAYER_H__

#include "FileUtils\FileTime.h"
#include "Units\AttributeReference.h"
#include "LocString.h"
#include "FileUtils\XGUID.h"

class Archive;
class WBuffer;

enum RealPlayerType {
	REAL_PLAYER_TYPE_CLOSE = 1,
	REAL_PLAYER_TYPE_OPEN = 2,
	REAL_PLAYER_TYPE_PLAYER = 4,
	REAL_PLAYER_TYPE_AI = 8,
	REAL_PLAYER_TYPE_WORLD = 16,
	REAL_PLAYER_TYPE_EMPTY = REAL_PLAYER_TYPE_CLOSE | REAL_PLAYER_TYPE_OPEN
};

enum {
	NETWORK_SLOTS_MAX = 6, 
	NETWORK_PLAYERS_MAX = 12, 
	NETWORK_TEAM_MAX = 3, 
	NETWORK_RACE_NUMBER = 3, 
	PERIMETER_CLIENT_DESCR_SIZE = 64,
	PERIMETER_CONTROL_NAME_SIZE = 64,
	MAX_MULTIPALYER_GAME_NAME = 19, // синхронизить с GAME_NAME_SIZE в GameInfo.h , BD_MAX_SESSIONNAME_LENGTH в CustomMatchMakingInfo.h и классом bdfindByGameTypeAndModeResultRow

	PLAYER_ID_NONE = -1,
	USER_IDX_NONE=-1
};

struct UNetID{
	union { 
		unsigned long dpnid_;
		unsigned long ip;
	};
	unsigned long port;

	UNetID(){ setEmpty(); }
	UNetID(DWORD _dpnid){
		dpnid_=_dpnid;
		port=0;
	}
	UNetID(unsigned long _ip, unsigned short _port){
		ip=_ip;
		port=_port;
	}
	unsigned long dpnid() const { xassert(port==0); return dpnid_; }
	void setEmpty(){ ip=-1; port=-1; }
	bool isEmpty() { return (port==-1);} 
	void setAllPlayers(){ *this=UNID_ALL_PLAYERS; }
	bool isAllPlayers() const { return ip==0; }
	bool operator == (const UNetID& secop) const { return (ip==secop.ip) && (port==secop.port); }
	bool operator != (const UNetID& secop) const { return (ip!=secop.ip) || (port!=secop.port); }
	bool operator < (const UNetID& secop) const { return (ip < secop.ip); }

	static const UNetID UNID_ALL_PLAYERS;
};

struct SlotData {
	int shuffleIndex;
	RealPlayerType realPlayerType;
	Race race;
	int colorIndex;
	int silhouetteColorIndex;
	int underwaterColorIndex;
	int signIndex;
	int clan;
	Difficulty difficulty;

	char usersIdxArr[NETWORK_TEAM_MAX];
	bool hasFree() const;

	SlotData(int playerIDIn = PLAYER_ID_NONE, RealPlayerType realPlayerTypeIn = REAL_PLAYER_TYPE_CLOSE);

	void readNet(XBuffer& in);
	void writeNet(XBuffer& out) const;
	void logVar() const;

	void serialize(Archive& ar);
};

struct BackGameInformation2 {
	unsigned int lagQuant_;
	unsigned int quant_;
	unsigned int signature_;
	unsigned int replay_;
	unsigned int state_;
	unsigned int accessibleLogicQuantPeriod_;
	bool operator == (const BackGameInformation2 &secop) const {
		//lagQuant не сравнивается !
		return ( (quant_ == secop.quant_) && 
			(signature_== secop.signature_) );
	}
};

struct UserData {
	UserData();
	bool flag_userConnected;
	bool flag_playerStartReady;
	bool flag_playerGameLoaded;
	//int compAndUserID;
	char playerNameE[32];
	UNetID unid;
	// not transfer network
	unsigned int clientGameCRC;
	unsigned int lastTimeBackPacket;
	vector<BackGameInformation2> backGameInf2List;

	bool requestPause;
	bool clientPause;
	unsigned int timeRequestPause;

	unsigned int lagQuant;
	unsigned int lastExecuteQuant;
	unsigned int curLastQuant;
	unsigned int confirmQuant;
	unsigned int accessibleLogicQuantPeriod;


	void setName(const char* name) { strncpy(playerNameE, name, sizeof(playerNameE)-1); playerNameE[sizeof(playerNameE)-1]='\0'; }
	void readNet(XBuffer& in);
	void writeNet(XBuffer& out) const;
	void serialize(Archive& ar);
};

struct PlayerData : public SlotData {
	enum  {
		PLAYER_ID_NONE = -1,
		COOPERATIVE_IDX_NONE = -1
	};
	
	int playerID;
	int teamIndex;

	bool flag_playerStartReady;
	bool flag_playerGameLoaded;
	//int compAndUserID;
	UNetID unid;
	char playerName[64];

	PlayerData(int playerIDIn = PLAYER_ID_NONE, RealPlayerType realPlayerTypeIn = REAL_PLAYER_TYPE_CLOSE);

	PlayerData& operator = (const SlotData& inData);


	const char* name() const { return playerName; } 
	void setName(const char* name) { strncpy(playerName, name, 63); }

	void readNet(XBuffer& in);
	void writeNet(XBuffer& out) const;

	void serialize(Archive& ar);
};


enum GameType {
	// Not Using!
	GameType_Multiplayer=0x80,
	GameType_Cooperative=0x40,
	GameType_UseMapSettings=0x20,
	GameType_MASK=0x0F,
	//Using
	GAME_TYPE_SCENARIO	= 0,
	GAME_TYPE_BATTLE	= 0x1,
	GAME_TYPE_REEL		= 0x10,
	GAME_TYPE_MULTIPLAYER			  = GAME_TYPE_BATTLE | GameType_Multiplayer,
	GAME_TYPE_MULTIPLAYER_COOPERATIVE = GAME_TYPE_BATTLE | GameType_Multiplayer | GameType_Cooperative,
};

class MissionDescription 
{
public:
	enum eChangedMDVal{
		CMDV_ChangeTriggers
	};

	MissionDescription(const char* fname = 0, GameType gameType = GAME_TYPE_SCENARIO);
	void setGameType(GameType gameType, bool force = false);
	void setByWorldName(const char* worldName);
	void loadReplayInfoInFile(const char* fname);
	void loadReplayInfoInBuf(XBuffer& buffer);

	void saveReplay(XBuffer& buffer);

	void restart();
	void resetToSave(bool userSave);
	void deleteSave() const;
	
	bool isLoaded() const { return !worldName_.empty(); }
	void clear() { worldName_.clear(); }
	string saveData(const char* extention) const; 
	string worldData(const char* fileName) const;

	void setSaveName(const char* name);
	void setInterfaceName(const char* name);

	void setDifficulty(const Difficulty& dif);

	void readNet(XBuffer& in);
	void writeNet(XBuffer& out) const;

	int revision() const { return revision_; }
	const char* worldName() const { return worldName_.c_str(); } // Имя мира без пути
	const char* saveName() const { return saveName_.c_str(); } // Путь + имя spg
	const char* interfaceName() const { return interfaceName_.c_str(); } // Имя сейва или реплея без расширения и пути
	const GUID& missionGUID() const { return missionGUID_; }
	const wchar_t* missionDescription() const { return missionDescription_.c_str(); }
	const char* reelName() const { return reelName_.c_str(); }
	const FileTime& saveTime() { return saveTime_; }

	Vect2f worldSize() const { return Vect2f(worldSize_); } //{ return Vect2f(sizeX, sizeY); }
	GameType gameType() const { return gameType_; }
	bool isMultiPlayer() const { return (gameType_ & GameType_Multiplayer) && !(gameType_ & GAME_TYPE_REEL);	}
	bool scenario() const { return (gameType_ & GameType_MASK) == GAME_TYPE_SCENARIO; }
	bool battle() const { return (gameType_ & GameType_MASK) == GAME_TYPE_BATTLE; }
	bool isReplay() const { return gameType_ & GAME_TYPE_REEL; }
	bool valid() const;
	int gamePopulation() const;

	bool useMapSettings() const { return useMapSettings_ || userSave_; }
	void setUseMapSettings(bool useMapSettings) { useMapSettings_ = useMapSettings; }

	int triggerFlags() const { return triggerFlags_; }
	void setTriggerFlags(int triggerFlags) { triggerFlags_ = triggerFlags; }

	void shufflePlayers();

	const SlotData& playerData(int playerID) const;
	SlotData& changePlayerData(int playerID);
	int findSlotIdx(int userIdx) const;	
	int findCooperativeIdx(int userIdx) const;
	int findPlayerIndex(int slotIdx) const; 
	const PlayerData& constructPlayerData(int playerID, int cooperativeIndex = 0, int newPlayerID=PLAYER_ID_NONE) const;
	const UserData& getUserData(int playerID, int cooperativeIndex = 0) const;

	//void setActivePlayer(int playerID);
	void switchActiveUser2OtherSlot(int slotIdx);
	void setActivePlayerIdx(int _activeUserIdx);

	int playersAmountMax() const { return playersAmountMax_; }
	int auxPlayersAmount() const { return auxPlayersAmount_; }
	int playersAmount() const;

	bool userSave() const { return userSave_; }
	void setBattle(bool battle) { isBattle_ = battle; }
	bool isBattle() const { return isBattle_; }
	bool isCorrect() const { return errorCode==ErrMD_None; }

	void packPlayerIDs();
	void packCooperativeIdx(int slotId);
	void packCooperativeIdx();

	void serialize(Archive& ar);

	int activePlayerID() const { int api=findSlotIdx(activeUserIdx_); if(api==PLAYER_ID_NONE) api=playersAmountMax();/*HINT!*/  api=clamp(api, 0, playersAmountMax()); return api; }
	int activeCooperativeIndex() const { int aci=findCooperativeIdx(activeUserIdx_); /*xassert(aci>=0 && aci<NETWORK_COOPERATIVE_MAX);*/ aci=clamp(aci,0,NETWORK_TEAM_MAX-1); return aci; }
	int activeUserIdx() const { return activeUserIdx_; }
	const wchar_t* getPlayerName(WBuffer& name, int slotID, int cooperativeIdx) const;
	void setPlayerName(int slotID, int cooperativeIdx, const wchar_t* str);
	
	const Vect2s& startLocation(int index) const { return startLocations_[index]; }

	bool isChanged() const { return flag_missionDescriptionUpdate; }
	void setChanged(bool _userAmountChanged=false) { flag_missionDescriptionUpdate=true; if(_userAmountChanged) flag_usersAmountChanged=true; }
	void clearChanged() { flag_missionDescriptionUpdate=0; }
	void resetUsersAmountChanged() { flag_usersAmountChanged=false; }
	bool isUsersAmountChanged(){ return flag_usersAmountChanged; }

	int findFreeUserIdx();
	void clearSlotAndUserData();
	void insertPlayerData(const struct PlayerDataEdit& data, bool flag_active);

	bool isGamePaused() { return gamePaused; }
	void setGamePaused(bool pause) { gamePaused = pause; }
	void logVar() const;

	static const char* getExtention(GameType type);
	static const wchar_t* getMapSizeName(float size);

	const wchar_t* getMapSizeName() const;

	void getDebugInfo(XBuffer& out) const;

	int globalTime; 
	float gameSpeed;
	bool gamePaused;
	bool enableInterface;
	bool enablePause;

	bool flag_missionDescriptionUpdate;
	bool flag_usersAmountChanged;
	bool is_fog_of_war;
	bool is_water;
	bool is_temperature;
	bool silhouettesEnabled;
	bool saveMiniMap;
	Vect2s worldSize_;

protected:
	SlotData slotsData[NETWORK_PLAYERS_MAX+1]; //Hint!
	UserData usersData[NETWORK_PLAYERS_MAX];

	int activeUserIdx_;
	int triggerFlags_;
	enum eErrorCode{
		ErrMD_None=0,
		ErrMD_IncorrectReplay,
		ErrMD_NoMission
	};
	eErrorCode errorCode;

private:
	Vect2s startLocations_[NETWORK_PLAYERS_MAX];

	int revision_;
	LocString missionDescription_;
	GameType gameType_;
	int playersAmountMax_;
	int auxPlayersAmount_;
	bool useMapSettings_;
	bool userSave_;
	bool isBattle_;

	FileTime saveTime_;

	string worldName_;
	string saveName_;
	string interfaceName_;
	string reelName_;
	unsigned int serverRnd_;

	XGUID worldGUID_;
	XGUID missionGUID_;

	friend class PNetCenter;
	friend class DWInterface;
};

class MissionDescriptions : public vector<MissionDescription>
{
public:
	void readFromDir(const char* path, GameType gameType);
	void readUserWorldsFromDir(const char* path);
	void add(const MissionDescription& mission);
	void remove(const MissionDescription& mission);
	const MissionDescription* find(const char* missionName) const;
	const MissionDescription* find(const wchar_t* missionName) const;
	const MissionDescription* find(const GUID& id) const;
};

struct ReplayHeader
{
	ReplayHeader();
	void read(XBuffer& in);
	void write(XBuffer& out);

	bool valid() const;

	GUID ID;
	unsigned long version;
	unsigned long universeSignature;
	unsigned long librarySignature;
	unsigned long worldSignature;
	unsigned long endQuant;

	static const GUID FilePlayReelID;
	static const int FilePlayReelVersion;
};

#endif
