#ifndef __P2P_INTERFACEAUX_H__
#define __P2P_INTERFACEAUX_H__

#include "NetPlayer.h"
#include "FileUtils\XGUID.h" //определение XGUID
#include "ExternalTask.h"

const GUID ZERO_GUID = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};


struct sGameType {
	sGameType(){ gameType = GAME_TYPE_BATTLE; }
	sGameType(int _gameType){
		gameType=(GameType)_gameType;
		xassert((gameType&GameType_MASK) >=GAME_TYPE_SCENARIO && (gameType&GameType_MASK)<=GAME_TYPE_BATTLE);
		if(!isUseMapSetting()) { xassert(isBattle());; }
		if(isCooperative()) { xassert(isMultiPlayer());; }
		if(isMultiPlayer()) { xassert(isBattle());; }
	}
	GameType gameType;
	bool operator ==( const GameType gt){ return (gameType&GameType_MASK) == gt; }
	bool isScenario() const { return (gameType&GameType_MASK)==GAME_TYPE_SCENARIO;}
	bool isBattle() const { return (gameType&GameType_MASK)==GAME_TYPE_BATTLE; }

	bool isReel() const { return gameType & GAME_TYPE_REEL; }
	bool isUseMapSetting() const { return gameType&GameType_UseMapSettings; }
	bool isMultiPlayer() const { return gameType&GameType_Multiplayer; }
	bool isCooperative() const { xassert(isMultiPlayer()); return gameType&GameType_Cooperative; }
	void setUseMapSetting(bool val=true) { 
		if(val) gameType=(GameType)((int)gameType|GameType_UseMapSettings); 
		else gameType=(GameType)( (int)gameType & (~GameType_UseMapSettings) ); 
	}
	void setMultiPlayer() { gameType = (GameType)((int)gameType|GameType_Multiplayer); }
	void setCooperative() { xassert(isMultiPlayer()); gameType=(GameType)((int)gameType|GameType_Cooperative); }
	void reset2OnlineType() {}
};

struct sGameStatusInfo {
	sGameStatusInfo(){
		setNS(0,0, "", ZERO_GUID, sGameType());
	}
	void setNS(char _maxPlayers, int _curPlayers, const char* _missionName, const GUID& _missionGuid, 
		const sGameType& sgt, bool _flag_gameRun=false, int _ping=10){
			flag_quickStart=false;
			maximumPlayers=_maxPlayers;
			currrentPlayers=_curPlayers;
			strncpy(missionName_, _missionName, sizeof(missionName_));
			missionName_[sizeof(missionName_)-1]='\0';
			missionGuid=_missionGuid;
			gameOrder=GameOrder_Any;
			jointGameType=sgt;
			flag_gameRun=_flag_gameRun;
			ping=_ping;
	}

	void setQS(char _maxPlayers, int _curPlayers, enum eGameOrder go, __int64 _filterMap, int _ping=10){
			flag_quickStart=true;
			maximumPlayers=_maxPlayers;
			currrentPlayers=_curPlayers;
			missionName_[0]='\0';
			filterMap=_filterMap;
			gameOrder=go;
			flag_gameRun=false;
			ping=_ping;
	}

	const char* missionName() const {
		return missionName_;
	}
	bool flag_quickStart;
	char maximumPlayers;
	char currrentPlayers;
	char missionName_[32];
	union {
		GUID missionGuid;
		__int64 filterMap;
	};
	eGameOrder gameOrder;
	sGameType jointGameType;
	bool flag_gameRun;
	int ping;
};

enum DWNATType {
	DWNT_Open = 1,
	DWNT_Moderate=2,
	DWNT_Strict=3
};
inline bool isNATCompatible(DWNATType t1, DWNATType t2) { return t1+t2 < 5; }

struct sGameHostInfo {
	GUID gameHostGUID;
	string hostName;
	string portStr;
	string gameName;
	sGameStatusInfo gameStatusInfo;
	//DWNATType dwNATType;
	bool natCompatible;
	sGameHostInfo(){
		GUID guid = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
		set(guid, "", "", "", sGameStatusInfo());
	}
	sGameHostInfo(GUID _gameHostID, const char * _hostName, const char * _port, const char * _gameName, const sGameStatusInfo& gsi, bool _natCompatible=true){ // DWNATType _dwNATType=DWNT_Open
		set(_gameHostID, _hostName, _port, _gameName, gsi);
	}
	void set(GUID _gameHostGUID, const char * _hostName, const char * _port, const char * _gameName, const sGameStatusInfo& gsi, bool _natCompatible=true){ //DWNATType _dwNATType=DWNT_Open
		gameStatusInfo=gsi;
		hostName=_hostName;
		portStr=_port;
		gameName=_gameName;
		gameHostGUID=_gameHostGUID;
		//dwNATType=_dwNATType;
		natCompatible=_natCompatible;
	}
};

struct StartGameParamBase {
	enum { NO_FILTER=-1 };
	char quickStart;
	int useMapSettingsFilter;
	std::vector<GUID> missionFilter;
	__int64 filterMap;
	int maxPlayersFilter;
	char gameProtection;
	eGameOrder gameOrder;
};
class StartGameParam : public StartGameParamBase { //sGameHostFilter 
public:
	StartGameParam(MTSection& mts);
	void reset();
	void setNS(int _useMapSettingsFilter, const std::vector<XGUID>& _missionFilter, int _maxPlayersFilter, char _gameProtection);
	void setQS(eGameOrder _gameOrder, __int64 _filterMap);
	bool isConditionEntry(const sGameStatusInfo& gsi);
	StartGameParamBase getGameParam();
private:
	MTSection& mtLock;
};

struct QSStateAndCondition {
	enum { MAX_QSUSERS = 6 };
	eGameOrder gameOrder;
	struct QSUserState {
		bool flag_userConnected;
		UNetID unid;
		ConnectPlayerData connectPlayerData;
	};
	QSUserState qsUserState[MAX_QSUSERS];

	QSStateAndCondition(){ gameOrder=GameOrder_1v1; clear(); }
	void clear(){ for(int i=0; i<MAX_QSUSERS; i++) qsUserState[i].flag_userConnected=false; }
	int addPlayers(ConnectPlayerData _connectPlayerData, const UNetID& unid);	
	bool removeUser(const UNetID& unid);
	bool isConditionRealized();
	__int64 getTotalFilterMap();
};

////////////////////////////////////////////////////////////////////////
// XDPacket

struct XDPacket
{
	int   size;
	unsigned char* buffer;
	UNetID unid;

	__forceinline XDPacket(){
		size = 0;
		buffer = 0;
		unid.setEmpty();
	}
	__forceinline XDPacket(const UNetID& _unid, int _size, const void* cb){
		unid=_unid;
		size= _size;
		buffer = new unsigned char[size];
		memcpy(buffer, cb, size);
	}
	__forceinline ~XDPacket(){
		if(buffer)
			delete buffer;
	}
	__forceinline void set(const UNetID& _unid, int _size, const void* cb){
		unid=_unid;
		size = _size;
		if(buffer) delete buffer;
		buffer = new unsigned char[size];
		memcpy(buffer, cb, size);
	}
	void create(const UNetID& _unid, int _size){
		unid=_unid;
		size = _size;
		if(buffer) delete buffer;
		buffer = new unsigned char[size];
	}
};

#endif //__P2P_INTERFACEAUX_H__


