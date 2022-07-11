#include "StdAfx.h"

#include "MissionDescriptionNet.h"
#include "GlobalAttributes.h"
#include "crc.h"
#include "UI_Logic.h"

//unsigned int ConnectPlayerData::calcCompAndUserID(const char* _computerName, const char* _userName)
//{
//	unsigned int result=startCRC32;
//	result=crc32((const unsigned char*)_computerName, strlen(_computerName), result);
//	result=crc32((const unsigned char*)_userName, strlen(_userName), result);
//	result=~result;
//	return result;
//}

void MissionDescriptionNet::clearAllUsersData()
{
	setChanged();
	for(unsigned int i=0; i<NETWORK_PLAYERS_MAX; i++){
		usersData[i].flag_userConnected=0;
	}
	for(unsigned int i=0; i<playersAmountMax(); i++){
		if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER){
			slotsData[i].realPlayerType = REAL_PLAYER_TYPE_OPEN;
			for(int j=0; j < NETWORK_TEAM_MAX; j++){
				slotsData[i].usersIdxArr[j]=USER_IDX_NONE;
			}
		}
	}
}

void MissionDescriptionNet::clearAllPlayerStartReady(void)
{
	setChanged();
	for(unsigned int i=0; i<NETWORK_PLAYERS_MAX; i++){
		//playersData_[i].flag_playerStartReady=0;
		usersData[i].flag_playerStartReady=0;
	}
}

void MissionDescriptionNet::setPlayerStartReady(UNetID& unid)
{
	setChanged();
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++)
		if(usersData[i].unid==unid){
			usersData[i].flag_playerStartReady=true;
			return;
		}
	xassert(0 && "invalidUNID");
}

bool MissionDescriptionNet::isAllRealPlayerStartReady(void)
{
	bool result=1;
	for(unsigned int i=0; i<playersAmountMax(); i++){
		if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER)
			for(int j=0; j < NETWORK_TEAM_MAX; j++){
				int pidx=slotsData[i].usersIdxArr[j];
				if(pidx!=USER_IDX_NONE){
					xassert(pidx>=0 && pidx<NETWORK_PLAYERS_MAX);
					if(pidx>=0 && pidx<NETWORK_PLAYERS_MAX){
						result&=usersData[pidx].flag_playerStartReady;
					}
				}
			}
	}
	return result;
}

void MissionDescriptionNet::setPlayerGameLoaded(UNetID& unid, unsigned int _gameCRC)
{
	setChanged();
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++)
		if(usersData[i].unid==unid){
			usersData[i].flag_playerGameLoaded=true;
			usersData[i].clientGameCRC=_gameCRC;
			return;
		}
	xassert(0 && "invalidUNID");
}

bool MissionDescriptionNet::isAllRealPlayerGameLoaded(void)
{
	bool result=1;
	//for(unsigned int i=0; i<NETWORK_PLAYERS_MAX; i++){
	//	if(playersData_[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER)
	//		result&=playersData_[i].flag_playerStartReady;
	//}
	for(unsigned int i=0; i<playersAmountMax(); i++){
		if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER)
			for(int j=0; j < NETWORK_TEAM_MAX; j++){
				int pidx=slotsData[i].usersIdxArr[j];
				if(pidx!=USER_IDX_NONE){
					xassert(pidx>=0 && pidx<NETWORK_PLAYERS_MAX);
					if(pidx>=0 && pidx<NETWORK_PLAYERS_MAX){
						result&=usersData[pidx].flag_playerGameLoaded;
					}
				}
			}
	}
	return result;
}

void MissionDescriptionNet::clearAllPlayerGameLoaded(void)
{
	setChanged();
	for(unsigned int i=0; i<NETWORK_PLAYERS_MAX; i++){
		usersData[i].flag_playerGameLoaded=0;
	}
}

int MissionDescriptionNet::playersMaxEasily() const
{
	int cntPlayers=0;
	for(unsigned int i=0; i<playersAmountMax(); i++){
		if(slotsData[i].realPlayerType!=REAL_PLAYER_TYPE_CLOSE /*&& 
			slotsData[i].realPlayerType !=REAL_PLAYER_TYPE_AI*/)
            cntPlayers++;
	}
	if(gameType() == GAME_TYPE_MULTIPLAYER_COOPERATIVE)
		cntPlayers*=NETWORK_TEAM_MAX;
	return cntPlayers;
}
int MissionDescriptionNet::amountUsersConnected() const
{
	int cnt=0;
	for(unsigned int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(usersData[i].flag_userConnected)
			cnt++;
	}
	return cnt;
}


int MissionDescriptionNet::getUniquePlayerColor(int begColor, bool dirBack)
{
	int i,c;
	for(c=0; c<GlobalAttributes::instance().playerAllowedColorSize(); c++){
		int curColor;
		if(dirBack==0)curColor=(begColor+c)%GlobalAttributes::instance().playerAllowedColorSize();
		else curColor=(begColor-c)%GlobalAttributes::instance().playerAllowedColorSize();
		bool error=0;
		//for(i=0; i<NETWORK_PLAYERS_MAX; i++){
		//	if(playersData_[i].realPlayerType==REAL_PLAYER_TYPE_AI || playersData_[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER){
		//		if(playersData_[i].playerID!=PlayerData::PLAYER_ID_NONE && playersData_[i].cooperativeIndex==0){
		//			if(playersData_[i].colorIndex==curColor) {
		//				error=1;
		//				break;
		//			}
		//		}
		//	}
		//}
		for(i=0; i<playersAmountMax(); i++){
			if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_AI || slotsData[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER){
				if(slotsData[i].colorIndex==curColor) {
					error=1;
					break;
				}
			}
		}
		if(error==0) return curColor;
	}
	return -1;
}

int MissionDescriptionNet::getUniquePlayerClan(void)
{
	int i, c;
	for(c=1; c<=playersAmountMax(); c++){
		bool error=0;
		for(i=0; i<playersAmountMax(); i++){
			if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_AI || slotsData[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER){
				if(slotsData[i].clan==c) {
					error=1;
					break;
				}
			}
		}
		if(error==0) return c;
	}
	return -1;
}

bool MissionDescriptionNet::disconnectUser(unsigned int idxUsersData) //REAL_PLAYER_TYPE_PLAYER only
{
	bool result=false;
	setChanged(true);
	xassert(idxUsersData>=0 && idxUsersData < NETWORK_PLAYERS_MAX);
	//if(playersData_[idxPlayerData].realPlayerType==REAL_PLAYER_TYPE_PLAYER || playersData_[idxPlayerData].realPlayerType==REAL_PLAYER_TYPE_AI){
	//	playersData_[idxPlayerData].realPlayerType=REAL_PLAYER_TYPE_OPEN;
	//	playersData_[idxPlayerData].playerID=PlayerData::PLAYER_ID_NONE;
	//	playersData_[idxPlayerData].cooperativeIndex=PlayerData::COOPERATIVE_IDX_NONE;
	//}
	int pid=findSlotIdx(idxUsersData);
	if(pid!=PLAYER_ID_NONE){
		SlotData& sd=changePlayerData(pid);
		xassert(sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER);
		int cntu=0;
		for(int i=0; i<NETWORK_TEAM_MAX; i++){
			if(sd.usersIdxArr[i]!=USER_IDX_NONE){
				cntu++;
				xassert(sd.usersIdxArr[i]>=0 && sd.usersIdxArr[i]<NETWORK_PLAYERS_MAX);
				if(sd.usersIdxArr[i]==idxUsersData){
					sd.usersIdxArr[i]=USER_IDX_NONE;
					cntu--;
					result=true;
				}
			}
		}
		if(cntu==0){
			sd.realPlayerType=REAL_PLAYER_TYPE_OPEN;
		}
	}
	else {
		//no slot use(in cooperative game)
	}
	usersData[idxUsersData].flag_userConnected=false;
	//usersData[idxUsersData].dpnid=0;

	return result;
}

void MissionDescriptionNet::disconnectAI(unsigned int slotID) //REAL_PLAYER_TYPE_AI only
{
	setChanged(true);
	xassert(slotID < playersAmountMax());
	SlotData& sd=changePlayerData(slotID);
	xassert(sd.realPlayerType==REAL_PLAYER_TYPE_AI);
	sd.realPlayerType=REAL_PLAYER_TYPE_OPEN;
}


void MissionDescriptionNet::connectAI(unsigned int slotID)
{
	setChanged(true);
	xassert(slotID >=0 && slotID < min(NETWORK_PLAYERS_MAX, playersAmountMax()));
	//if(playersData_[i].playerID==PlayerData::PLAYER_ID_NONE)
	if(slotID>=0 && slotID < min(NETWORK_PLAYERS_MAX, playersAmountMax())){
		//slotsData[slotID].playerID=slotID;
		slotsData[slotID].realPlayerType=REAL_PLAYER_TYPE_AI;
		//slotsData[slotID].dpnid=0;
		int colorNewPlayer=getUniquePlayerColor();
		if(colorNewPlayer!=-1) 
			slotsData[slotID].colorIndex = colorNewPlayer;
		int newClan=getUniquePlayerClan();
		if(newClan!=-1) 
			slotsData[slotID].clan=newClan;
	}
}

int MissionDescriptionNet::getFreeSlotID()
{
	int lastFreeId=0;
	for(int curSlotID=0; curSlotID<playersAmountMax(); curSlotID++){
		bool flag_IDUse=false;
		for(int j=0; j<NETWORK_TEAM_MAX; j++){
			if(slotsData[curSlotID].usersIdxArr[j]!=USER_IDX_NONE)
				flag_IDUse=true;
		}
		if(!flag_IDUse)
			return curSlotID;
	}
	return PLAYER_ID_NONE;
}

int MissionDescriptionNet::connectNewUser(ConnectPlayerData& pd, const UNetID& unid, unsigned int _curTime)
{
	setChanged(true);
	int result=-1;

	int freeUserIdx;
	for( freeUserIdx=0; freeUserIdx<NETWORK_PLAYERS_MAX; freeUserIdx++){
		if(!usersData[freeUserIdx].flag_userConnected)
			break;
	}
	if(freeUserIdx < NETWORK_PLAYERS_MAX){ //free userData found
		int i;
		for(i=0; i<playersAmountMax(); i++){
			if(slotsData[i].realPlayerType==REAL_PLAYER_TYPE_OPEN){
				//if(playersData_[i].playerID==PlayerData::PLAYER_ID_NONE)
				//slotsData[i].playerID=i;
				for(int k=0; k<NETWORK_TEAM_MAX; k++){
					xassert(slotsData[i].usersIdxArr[k]==USER_IDX_NONE);
				}
				int colorNewPlayer=getUniquePlayerColor();
				int newClan=getUniquePlayerClan();
				slotsData[i].realPlayerType=REAL_PLAYER_TYPE_PLAYER; //for getUniquePlayerColor & getUniquePlayerClan 
				slotsData[i].usersIdxArr[0]=freeUserIdx;
				slotsData[i].race.setKey(pd.race >= 0 ? pd.race : logicRND(NETWORK_RACE_NUMBER));
				if(colorNewPlayer!=-1)  
					slotsData[i].colorIndex = colorNewPlayer;
				else 
					xassert(0 && "Невозможный цвет");
				if(newClan!=-1) 
					slotsData[i].clan=newClan;
				else 
					xassert(0 && "Невозможный клан");
				break;
			}
		}
		if(i < playersAmountMax() || gameType()== GAME_TYPE_MULTIPLAYER_COOPERATIVE){
			UserData& ud=usersData[freeUserIdx];
			ud.flag_userConnected=true;
			ud.flag_playerStartReady = false;
			ud.flag_playerGameLoaded = false;
			ud.setName(pd.playerName);
			ud.unid=unid;
			//ud.compAndUserID=pd.compAndUserID;
			ud.backGameInf2List.reserve(20000);//резерв под 20000 квантов
			result=freeUserIdx;

			ud.lagQuant=0;
			ud.lastExecuteQuant=0;
			ud.curLastQuant=0;
			ud.lastTimeBackPacket = _curTime;
			ud.confirmQuant=0;
			ud.requestPause=0;
			ud.clientPause=0;
			ud.timeRequestPause=0;
			ud.accessibleLogicQuantPeriod=0;
		}
	}
	return result;
}



bool MissionDescriptionNet::join2Command(int playerIdx, int slotID)
{
	setChanged();
	//if(gameType()== GAME_TYPE_MULTIPLAYER_COOPERATIVE){
	//	for(int i=0; i<NETWORK_COOPERATIVE_MAX; i++){
	//		PlayerData& pd=changePlayerData(slotID, i);
	//		if(pd.playerID==PlayerData::PLAYER_ID_NONE){
	//			playersData_[playerIdx].playerID=PlayerData::PLAYER_ID_NONE;
	//			if(i==0){
	//				int colorNewPlayer=getUniquePlayerColor();
	//				if(colorNewPlayer!=-1) 
	//					playersData_[playerIdx].colorIndex = colorNewPlayer;
	//				int newClan=getUniquePlayerClan();
	//				if(newClan!=-1) 
	//					playersData_[playerIdx].clan=newClan;
	//			}
	//			playersData_[playerIdx].playerID=slotID;
	//			playersData_[playerIdx].cooperativeIndex=i;
	//			return i;
	//		}
	//	}
	//}
	//return PlayerData::COOPERATIVE_IDX_NONE;

	xassert(playerIdx>=0 && playerIdx< NETWORK_PLAYERS_MAX);
	xassert(usersData[playerIdx].flag_userConnected);

	if(gameType()== GAME_TYPE_MULTIPLAYER_COOPERATIVE){
		int oldSlotID=findSlotIdx(playerIdx);
		if(oldSlotID==slotID) return false;
		SlotData& sd=changePlayerData(slotID);
		if(/*sd.playerID!=PLAYER_ID_NONE &&*/ (sd.realPlayerType == REAL_PLAYER_TYPE_OPEN || sd.realPlayerType == REAL_PLAYER_TYPE_PLAYER)) {
			for(int i=0; i<NETWORK_TEAM_MAX; i++){
				if(sd.usersIdxArr[i]==USER_IDX_NONE){
					if(i==0){
						int colorNewPlayer=getUniquePlayerColor();
						if(colorNewPlayer!=-1) 
							sd.colorIndex = colorNewPlayer;
						int newClan=getUniquePlayerClan();
						if(newClan!=-1) 
							sd.clan=newClan;
					}
					//free old slots
					if(oldSlotID!=PLAYER_ID_NONE){
						int oldCoopIdx=findCooperativeIdx(playerIdx);
						SlotData& osd=changePlayerData(oldSlotID);
						if(oldCoopIdx!=USER_IDX_NONE){
							osd.usersIdxArr[oldCoopIdx]=USER_IDX_NONE;
							packCooperativeIdx(oldSlotID);
						}
					}
					sd.usersIdxArr[i]=playerIdx;
					sd.realPlayerType=REAL_PLAYER_TYPE_PLAYER;
					return true; 
				}
			}
		}
		else {
			xassert(0 && "Incorrect join 2 command");
		}

	}
	return false;
}

bool MissionDescriptionNet::setPlayerUNID(unsigned int userIdx, UNetID& unid)
{
	setChanged();
	//xassert(idx < playersAmountMax());
	//if(idx < playersAmountMax()){
	//	if(playersData_[idx].realPlayerType==REAL_PLAYER_TYPE_PLAYER){
	//		playersData_[idx].dpnid=dpnid;
	//		return 1;
	//	}
	//}
	//return 0;
	xassert(userIdx>=0 && userIdx < NETWORK_PLAYERS_MAX);
	if(usersData[userIdx].flag_userConnected==true){
		usersData[userIdx].unid=unid;
		return 1;
	}
	return 0;
}


void MissionDescriptionNet::getAllOtherPlayerName(string& outStr)
{
	//for(int i=0; i<playersAmountMax(); i++){
	//	if( (playersData_[i].playerID!=activePlayerID()) && (playersData_[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER) ){
	//		outStr+=playersData_[i].name();
	//		outStr+="&FFFFFF\n";
	//	}
	//}
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(usersData[i].flag_userConnected && activeUserIdx_!=i){
			outStr+=usersData[i].playerNameE;
			outStr+="&FFFFFF\n";
		}
	}
}

void MissionDescriptionNet::getPlayerName(int _userIdx, string& outStr)
{
	//for(int i=0; i<playersAmountMax(); i++){
	//	if( (playersData_[i].playerID==_playerID) && (playersData_[i].realPlayerType==REAL_PLAYER_TYPE_PLAYER) ){
	//		outStr+=playersData_[i].name();
	//		outStr+='\n';
	//		break;
	//	}
	//}
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(_userIdx==i) {
			outStr+=usersData[i].playerNameE;
			outStr+='\n';
			break;
		}
	}
}

int MissionDescriptionNet::findUserIdx(const UNetID& unid)
{
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(usersData[i].flag_userConnected && usersData[i].unid==unid){
			return i;
		}
	}
	xassert(0 && "Invalid unid");
	return USER_IDX_NONE;
}

bool MissionDescriptionNet::checkSlotData4Change(const SlotData& sd, UNetID& unid, bool flag_changeAbsolutely)
{
	if(sd.realPlayerType==REAL_PLAYER_TYPE_AI){
		if(flag_changeAbsolutely)
            return true;
		else
			return false;
	}
	else{
		int idx=sd.usersIdxArr[0];
		if(idx!=USER_IDX_NONE)
			if(usersData[idx].unid==unid || flag_changeAbsolutely)
				if(sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER || sd.realPlayerType==REAL_PLAYER_TYPE_OPEN)
					return true;
		return false;
	}
}

bool MissionDescriptionNet::changePlayerRace(int slotID, Race newRace, UNetID& unid, bool flag_changeAbsolutely)
{
	if(slotID<0 || slotID >= playersAmountMax()) return false;
	SlotData& sd=changePlayerData(slotID);
	if(checkSlotData4Change(sd, unid, flag_changeAbsolutely)){
		sd.race=newRace;
		return true;
	}
	return false;
}



bool MissionDescriptionNet::changePlayerColor(int slotID, int color, bool dirBack, UNetID& unid, bool flag_changeAbsolutely)
{
	if(slotID<0 || slotID >= playersAmountMax()) return false;
	SlotData& sd=changePlayerData(slotID);

	if(checkSlotData4Change(sd, unid, flag_changeAbsolutely)){
		int colorNew=getUniquePlayerColor(color, dirBack);
		if(colorNew!=-1) {
			sd.colorIndex = colorNew;
			return true;
		}
	}
	return false;

}

bool MissionDescriptionNet::changePlayerSign(int slotID, int sign, UNetID& unid, bool flag_changeAbsolutely)
{
	if(slotID<0 || slotID >= playersAmountMax()) return false;
	SlotData& sd=changePlayerData(slotID);

	if(checkSlotData4Change(sd, unid, flag_changeAbsolutely)){
		sd.signIndex= sign;
		return true;
	}
	return false;
}

bool MissionDescriptionNet::changePlayerDifficulty(int slotID, Difficulty difficulty, UNetID& unid, bool flag_changeAbsolutely)
{
	if(slotID<0 || slotID >= playersAmountMax()) return false;
	SlotData& sd=changePlayerData(slotID);

	if(checkSlotData4Change(sd, unid, flag_changeAbsolutely)){
		sd.difficulty=difficulty;
		return true;
	}
	return false;
}

bool MissionDescriptionNet::changePlayerClan(int slotID, int clan, UNetID& unid, bool flag_changeAbsolutely)
{
	if(slotID<0 || slotID >= playersAmountMax()) return false;
	SlotData& sd=changePlayerData(slotID);

	if(checkSlotData4Change(sd, unid, flag_changeAbsolutely)){
		setChanged();
		sd.clan=clan;
		return true;
	}
	return false;


}

bool MissionDescriptionNet::changeMD(eChangedMDVal val, int v)
{
	setChanged();
	switch(val){
	case CMDV_ChangeTriggers:
		triggerFlags_=v;
        return true;
	default:
		return false;
	}
}

int MissionDescriptionNet::getAmountCooperativeUsers(int slotID)
{
	xassert(slotID>=0 && slotID < playersAmountMax());

	const SlotData& sd=playerData(slotID);
	int cnt=0;
	//if(sd.playerID!=PlayerData::PLAYER_ID_NONE){
		for(int i=0; i<NETWORK_TEAM_MAX; i++){
			if(sd.usersIdxArr[i]!=USER_IDX_NONE)
				cnt++;
		}
	//}
	return cnt;
}

void MissionDescriptionNet::updateFromNet(const MissionDescription& netMD)
{
	const MissionDescription* lmd = UI_LogicDispatcher::instance().getMissionByID(netMD.missionGUID());
	if(lmd)
		*this=*lmd;
	else{
		*this=MissionDescription(); //clear
		errorCode=ErrMD_NoMission;
	}
	XBuffer tmp(1024,true);
	netMD.writeNet(tmp);
	tmp.set(0,XS_BEG);
	this->readNet(tmp);
}
