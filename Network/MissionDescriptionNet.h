#ifndef __MISSIONDESCRIPTIONNET_H__
#define __MISSIONDESCRIPTIONNET_H__

#include "NetPlayer.h"

const int MAX_LENGHT_PLAYER_NAME=32;

struct ConnectPlayerData {
	char playerName[MAX_LENGHT_PLAYER_NAME+1];
	//int compAndUserID;
	int race;
	__int64 lowFilterMap;
	__int64 highFilterMap;

	void set(const char* name, int _race=0, __int64 _lowFilterMap=0, __int64 _highFilterMap=0){
		strncpy(playerName, name, sizeof(playerName));
		playerName[MAX_LENGHT_PLAYER_NAME]='\0'; //!!!
		race=_race;
		lowFilterMap=_lowFilterMap;
		highFilterMap=_highFilterMap;
		//setCompAndUserID(_computerName, _userName);
	}
	//void setCompAndUserID(const char* _computerName, const char* _userName){
	//	compAndUserID = calcCompAndUserID(_computerName, _userName);
	//}
	//unsigned int calcCompAndUserID(const char* _computerName, const char* _userName);
};


class MissionDescriptionNet : public MissionDescription
{
public:
	void clearAllUsersData();
	void clearAllPlayerStartReady(void);
	void setPlayerStartReady(UNetID& unid);
	void setPlayerGameLoaded(UNetID& unid, unsigned int _gameCRC);

	bool isAllRealPlayerStartReady(void);
	bool isAllRealPlayerGameLoaded();

	void clearAllPlayerGameLoaded(void);

	int playersMaxEasily() const;
	int amountUsersConnected() const;

	int getUniquePlayerColor(int begColor=0, bool dirBack=0);
	int getUniquePlayerClan();

	bool disconnectUser(unsigned int idxUsersData); //REAL_PLAYER_TYPE_PLAYER
	void disconnectAI(unsigned int slotID); //REAL_PLAYER_TYPE_AI only
	void connectAI(unsigned int slotID);
	int connectNewUser(ConnectPlayerData& pd, const UNetID& unid, unsigned int _curTime);
	bool join2Command(int playerIdx, int slotID);
	int getFreeSlotID();
	int getAmountCooperativeUsers(int slotID);

	bool setPlayerUNID(unsigned int idx, UNetID& unid);

	void getAllOtherPlayerName(string& outStr);
	void getPlayerName(int _userIdx, string& outStr);

	int findUserIdx(const UNetID& unid);

	bool checkSlotData4Change(const SlotData& sd, UNetID& unid, bool flag_changeAbsolutely);

	bool changePlayerRace(int slotID, Race newRace, UNetID& unid, bool flag_changeAbsolutely);
	bool changePlayerColor(int slotID, int color, bool dirBack, UNetID& unid, bool flag_changeAbsolutely);
	bool changePlayerSign(int slotID, int sign, UNetID& unid, bool flag_changeAbsolutely);
	bool changePlayerDifficulty(int slotID, Difficulty difficulty, UNetID& unid, bool flag_changeAbsolutely);
	bool changePlayerClan(int slotID, int clan, UNetID& unid, bool flag_changeAbsolutely);

	void updateFromNet(const MissionDescription& netMD);

	bool changeMD(eChangedMDVal val, int v);

	MissionDescriptionNet& operator = (const MissionDescription& md){
		static_cast<MissionDescription&>(*this)=md;
		return *this;
	}
};


#endif //__MISSIONDESCRIPTIONNET_H__
