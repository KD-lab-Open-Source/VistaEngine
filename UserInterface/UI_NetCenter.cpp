#include "stdafx.h"
#include "UI_NetCenter.h"
#include "UI_Logic.h"
#include "GameShell.h"
#include "CommonLocText.h"
#include "..\Network\P2P_interface.h"
#include "..\Network\LogMsg.h"

#ifndef _FINAL_VERSION_
#include "Serialization.h"
#endif

const char* getColorString(const sColor4f& color);

#define readGlobalStatsAtOnce 20

UI_NetCenter::UI_NetCenter()
: selectedGameInfo_(*new sGameHostInfo)
, selectedGame_(*new MissionDescription)
{
	netStatus_ = UI_NET_OK;
	delayOperation_ = DELAY_NONE;
	selectedGameIndex_ = -1;
	selectedGlobalStatisticIndex_ = -1;
	currentStatisticFilterRace_ = -1;
	currentStatisticFilterGamePopulation_ = -1;
	firstScorePosition_ = 1;
	gameCreated_ = false;
	onlineLogined_ = false;
	isOnPause_ = false;
	subscribedChannel_ = -1;
	subscribeWaitingChannel_ = -1;
	selectedChannel_ = -1;
	autoSubscribeMode_ = false;
	lastSubscribeAttempt_ = -1;
}

UI_NetCenter::~UI_NetCenter()
{
	delete &selectedGameInfo_;
	delete &selectedGame_;
}

void UI_NetCenter::setStatus(UI_NetStatus status)
{
	LogMsg("UI_NetCenter: Current status: %s, change to: %s\n", getEnumDescriptor(UI_NetStatus()).name(netStatus_), getEnumDescriptor(UI_NetStatus()).name(status));
	netStatus_ = status;
}

void UI_NetCenter::commit(UI_NetStatus status)
{
	xassert(status != UI_NET_WAITING);
	lock_.lock();
	bool checkVersion = false;

	if(status == UI_NET_SERVER_DISCONNECT){
		LogMsg("UI_NetCenter: DISCONNECT commited\n");
		setStatus(UI_NET_ERROR);
		lock_.unlock();
		release();
		UI_LogicDispatcher::instance().networkDisconnect(true);
		return;
	}
	else if(status == UI_NET_TERMINATE_SESSION){
		LogMsg("UI_NetCenter: TERMANATE SESION commited\n");
		setStatus(UI_NET_ERROR);
		lock_.unlock();
		reset(true);
		UI_LogicDispatcher::instance().networkDisconnect(false);
		return;
	}
	else if(status == UI_NET_OK){
		switch(delayOperation_){
		case CREATE_GAME:
			gameCreated_ = true;
			resetChatBoard();
			break;
		case ONLINE_NEW_LOGIN:
			UI_LogicDispatcher::instance().profileSystem().newOnlineLogin();
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_ONLINE_LOGIN_LIST, UI_ActionDataControlCommand::RE_INIT)));
			onlineLogined_ = true;
			resetChatBoard();
			checkVersion = true;
			break;
		case ONLINE_CREATE_LOGIN:
			UI_LogicDispatcher::instance().profileSystem().newOnlineLogin();
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_ONLINE_LOGIN_LIST, UI_ActionDataControlCommand::RE_INIT)));
			break;
		case ONLINE_DELETE_LOGIN:
			UI_LogicDispatcher::instance().profileSystem().deleteOnlineLogin();
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_ONLINE_LOGIN_LIST, UI_ActionDataControlCommand::RE_INIT)));
			break;
		case QUERY_GLOBAL_STATISTIC:
			applyNewGlobalStatistic(gsForRead_);
			break;
		case ONLINE_CHECK_VERSION:
			if(!setGameVersion()){
				UI_LogicDispatcher::instance().handleNetwork(NetRC_LoadInfoFile_Err);
				lock_.unlock();
				return;
			}
			break;
		}
	}
	
	if(checkVersion) {
		delayOperation_ = ONLINE_CHECK_VERSION;
		setStatus(UI_NET_WAITING);
		lock_.unlock();
		queryGameVersion();
		return;
	}
	else {
		delayOperation_ = DELAY_NONE;
		setStatus(status);
		lock_.unlock();
	}
}

void UI_NetCenter::create(NetType type)
{
	int localDelayOperation = 0;

	{
		MTAuto lock(lock_);
		
		LogMsg("UI_NetCenter: CREATE ");
		
		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		UI_LogicDispatcher::instance().resetCurrentMission();
		switch(type){
		case LAN:
			if(!gameShell->isNetClientConfigured(PNCWM_LAN_DW)){
				LogMsg("LAN done\n");
				setStatus(UI_NET_WAITING);
				localDelayOperation = 1;
			}
			else
				LogMsg("LAN skiped, lan jast created\n");
			break;
		
		case ONLINE:
			if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
				LogMsg("ONLINE started\n");
				setStatus(UI_NET_WAITING);
				localDelayOperation = 2;
			}
			else
				LogMsg("ONLINE skiped, online jast created\n");
			break;
		
		default:
			LogMsg("ERROR, request undefined type\n");
			xassert(0);
		}
	}

	switch(localDelayOperation){
	case 1:
		gameShell->createNetClient(PNCWM_LAN_DW);
		if(PNetCenter* center = gameShell->getNetClient())
			center->Configurate(UI_LogicDispatcher::instance().currentPlayerDisplayName());
		else {
			LogMsg("LAN creation ERROR\n");
			commit(UI_NET_ERROR);
		}
		break;
	case 2:
		gameShell->createNetClient(PNCWM_ONLINE_DW);
		break;
	}
}

void UI_NetCenter::setPassword(const char* pass) 
{
	MTAuto lock(lock_);
	LogMsg("UI_NetCenter: SET PASS ""%s"" done\n", pass);

	password_ = pass;
}

void UI_NetCenter::setPass2(const char* pass) 
{
	MTAuto lock(lock_);
	LogMsg("UI_NetCenter: SET PASS2 ""%s"" done\n", pass);

	pass2_ = pass;
}

void UI_NetCenter::resetPasswords() 
{
	MTAuto lock(lock_);

	password_.clear();
	pass2_.clear();
}

void UI_NetCenter::abortCurrentOperation()
{
	{
		MTAuto lock(lock_);
		LogMsg("UI_NetCenter: ABORT ");

		if(!acyncEventWaiting()){
			LogMsg("skiped, no operation waiting\n");
			return;
		}

		LogMsg("started\n");

		clear();
		UI_LogicDispatcher::instance().resetCurrentMission();

		commit(UI_NET_ERROR);
	}

	if(PNetCenter* center = gameShell->getNetClient()){
		LogMsg("done\n");
		center->ResetAndStartFindHost();
	}
	else
		LogMsg("skiped, net system not created\n");
}

void UI_NetCenter::reset(bool flag_internalReset)
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: RESET ");

		resetPasswords();

		clear();
		UI_LogicDispatcher::instance().resetCurrentMission();
	}
	
	if(!flag_internalReset){
		if(PNetCenter* center = gameShell->getNetClient()){
			LogMsg("done\n");
			center->ResetAndStartFindHost();
		}
		else
			LogMsg("skiped, net system not created\n");
	}
	else
		LogMsg("skiped, internal reset\n");

}

class GameSorting
{
public:
	bool operator () (const sGameHostInfo& lsh, const sGameHostInfo& rsh) const {
		return !lsh.gameStatusInfo.flag_gameRun && rsh.gameStatusInfo.flag_gameRun;
	}
};

bool UI_NetCenter::updateGameList()
{
	GameHostInfos tmpHostList;
	PNetCenter* center = 0;
	if(center = gameShell->getNetClient())
		center->getGameHostList(tmpHostList);

	MTAuto lock(lock_);

	if(acyncEventWaiting())
		return false;

	if(!center){
		if(selectedGameIndex_ >= 0)
			UI_LogicDispatcher::instance().resetCurrentMission();
		selectedGameIndex_ = -1;
		netGames_.clear();
		return false;
	}

	if(gameCreated())
		return false;

	netGames_ = tmpHostList;

	//GameHostInfos::iterator it = netGames_.begin();
	//while(it != netGames_.end())
	//	if(it->gameStatusInfo.flag_gameRun)
	//		it = netGames_.erase(it);
	//	else
	//		++it;

	stable_sort(netGames_.begin(), netGames_.end(), GameSorting());

	if(selectedGameIndex_ >= 0){
		selectedGameIndex_ = -1;
		for(int i = 0; i < netGames_.size(); ++i)
			if(netGames_[i].gameHostGUID == selectedGameInfo_.gameHostGUID){
				selectedGameIndex_ = netGames_[i].gameStatusInfo.flag_gameRun ? -1 : i;
				break;
			}
		if(selectedGameIndex_ == -1)
			UI_LogicDispatcher::instance().resetCurrentMission();
	}
	
	return true;
}

int UI_NetCenter::getGameList(const GameListInfoTypes& format, ComboStrings&  board, const sColor4c& started)
{
	MTAuto lock(lock_);

	board.clear();

	if(!gameShell->getNetClient())
		return -1;

	XBuffer buf;

	GameHostInfos::const_iterator gm;
	FOR_EACH(netGames_, gm){
		buf.init();
		GameListInfoTypes::const_iterator frm;
		FOR_EACH(format, frm){
			if(gm->gameStatusInfo.flag_gameRun && started.a > 0)
				buf < getColorString(sColor4f(started));
			switch(*frm){
			case GAME_INFO_GAME_NAME:
				buf < gm->gameName.c_str();
				break;
			case GAME_INFO_HOST_NAME:
				buf < gm->hostName.c_str();
				break;
			case GAME_INFO_WORLD_NAME:
				buf < gm->gameStatusInfo.missionName();
				break;
			case GAME_INFO_PLAYERS_CURRENT:
				buf <= gm->gameStatusInfo.currrentPlayers;
				break;
			case GAME_INFO_PLAYERS_MAX:
				buf <= gm->gameStatusInfo.maximumPlayers;
				break;
			case GAME_INFO_PLAYERS_NUMBER:
				buf <= gm->gameStatusInfo.currrentPlayers < "/" <= gm->gameStatusInfo.maximumPlayers;
				break;
			case GAME_INFO_PING:
				buf <= gm->gameStatusInfo.ping;
				break;
			case GAME_INFO_GAME_TYPE:
				if(gm->gameStatusInfo.jointGameType.isUseMapSetting())
					buf < GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME);
				else
					buf < GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME);
				break;
			case GAME_INFO_START_STATUS:
				if(gm->gameStatusInfo.flag_gameRun)
					buf < GET_LOC_STR(UI_COMMON_TEXT_GAME_RUNNING);
				else
					buf < GET_LOC_STR(UI_COMMON_TEXT_GAME_NOT_RUNNING);
				break;
			case GAME_INFO_NAT_TYPE:
				switch(gm->dwNATType){
				case DWNT_Open:
					buf < GET_LOC_STR(UI_COMMON_TEXT_NAT_TYPE_OPEN);
					break;
				case DWNT_Moderate:
					buf < GET_LOC_STR(UI_COMMON_TEXT_NAT_TYPE_MODERATE);
					break;
				case DWNT_Strict:
					buf < GET_LOC_STR(UI_COMMON_TEXT_NAT_TYPE_STRICT);
					break;
				}
				break;
			case GAME_INFO_NAT_COMPATIBILITY:
				if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
					buf < GET_LOC_STR(UI_COMMON_TEXT_NAT_INCOMPATIBLE);
				}
				else
					buf < " ";
				break;
			default:
				buf < " ";
			}
			buf < "\t";
		}
		board.push_back(buf.c_str());
	}

	xassert(selectedGameIndex_ < (int)board.size());
	return selectedGameIndex_;
}

const char* UI_NetCenter::selectedGameName() const
{
	if(isNetGame()){
		if(isServer())
			return UI_LogicDispatcher::instance().currentProfile().lastCreateGameName.c_str();
		else if(gameSelected())
			return selectedGameInfo_.gameName.c_str();
	}
	return 0;
}

bool UI_NetCenter::gameSelected() const
{
	return selectedGameIndex_ >= 0 && selectedGame_.isLoaded();
}

void UI_NetCenter::selectGame(int idx)
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: SELECT GAME %i ", idx);

	if(acyncEventWaiting()){
		LogMsg("skiped, waiting acync event\n");
		return;
	}

	if(!gameShell->getNetClient()){
		LogMsg("ERROR, net client not created\n");
		return;
	}

	if(!updateGameList()){
		LogMsg("skiped, update game list failed\n");
		return;
	}
	
	selectedGameIndex_ = idx;
	
	if(selectedGameIndex_ >= netGames_.size())
		selectedGameIndex_ = netGames_.size() - 1;

	if(selectedGameIndex_ >= 0){
		selectedGameInfo_ = netGames_[selectedGameIndex_];
		if(!selectedGameInfo_.gameStatusInfo.flag_gameRun){
			if(const MissionDescription* mission = UI_LogicDispatcher::instance().getMissionByID(selectedGameInfo_.gameStatusInfo.missionGuid)){
				MissionDescription md(*mission);
				md.setGameType(
					selectedGameInfo_.gameStatusInfo.jointGameType.isCooperative()
					? GAME_TYPE_MULTIPLAYER_COOPERATIVE
					: GAME_TYPE_MULTIPLAYER);
				selectedGame_ = md;
				LogMsg("done\n");
			}
			else{
				selectedGame_.clear();
				LogMsg("failed, world by guid not found\n");
			}
		}
		else {
			selectedGame_.clear();
			LogMsg("failed, game is run\n");
		}
	}
	else {
		selectedGame_.clear();
		LogMsg("failed, mission cleared\n");
	}
}

void UI_NetCenter::login()
{
	int localDelayOperation = 0;

	{
		MTAuto lock(lock_);
		
		LogMsg("UI_NetCenter: LOGIN ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			if(!password_.empty()){
				LogMsg("started\n");
				setStatus(UI_NET_WAITING);
				delayOperation_ = ONLINE_NEW_LOGIN;
				localDelayOperation = 1;
			}
			else {
				LogMsg("ERROR, password empty\n");
				localDelayOperation = 2;
			}
		}
		else {
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
		}
	}

	switch(localDelayOperation){
	case 1:
		if(PNetCenter* center = gameShell->getNetClient())
			center->Configurate(UI_LogicDispatcher::instance().currentProfile().lastInetName.c_str(), password_.c_str());
		else {
			LogMsg("ERROR login, pNnetCenter killed\n");
			commit(UI_NET_ERROR);
		}
		break;
	case 2:
		gameShell->networkMessageHandler(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);
		break;
	}
}

void UI_NetCenter::logout()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: LOGOUT ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
			return;
		}

		onlineLogined_ = false;
	}

	LogMsg("started\n");

	resetChatBoard(false);
	
	commit(UI_NET_OK); // всегда завершается успешно

	resetPasswords();

	clear();
	UI_LogicDispatcher::instance().resetCurrentMission();

}

void UI_NetCenter::refreshGameList()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: REFRESH GAME LIST ");

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
			return;
		}

		LogMsg("started\n");
	}
}

void UI_NetCenter::quickStart()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: QUICK START ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
			return;
		}
		else {
			LogMsg("started\n");
			setStatus(UI_NET_WAITING);
			delayOperation_ = CREATE_GAME;
		}
	}

	if(PNetCenter* center = gameShell->getNetClient())
		center->QuickStartThroughDW(
				UI_LogicDispatcher::instance().currentPlayerDisplayName(),
				UI_LogicDispatcher::instance().currentProfile().quickStartFilterRace,
				GameOrder_1v1,
				UI_LogicDispatcher::instance().quickStartFilter()
		);
	else {
		LogMsg("ERROR quickStart, pNnetCenter killed\n");
		setStatus(UI_NET_ERROR);
	}
}

void UI_NetCenter::createAccount()
{
	int localDelayOperation = 0;

	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: CREATE ACCOUNT ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			if(!password_.empty() && password_ == pass2_){
				LogMsg("started\n");
				setStatus(UI_NET_WAITING);
				delayOperation_ = ONLINE_CREATE_LOGIN;
				localDelayOperation = 1;
			}
			else {
				LogMsg("ERROR, %s\n", password_.empty() ? "password empty" : "password not equal");
				localDelayOperation = 2;
			}
		}
		else {
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
		}
	}

	switch(localDelayOperation){
	case 1:
		break;
	case 2:
		gameShell->networkMessageHandler(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);
		break;
	}
}

void UI_NetCenter::changePassword()
{
	int localDelayOperation = 0;

	{
		MTAuto lock(lock_);
		
		LogMsg("UI_NetCenter: CHANGE PASSWORD ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			if(!password_.empty() && !pass2_.empty()){
				LogMsg("started\n");
				setStatus(UI_NET_WAITING);
				localDelayOperation = 1;
			}
			else {
				LogMsg("ERROR, password empty\n");
				localDelayOperation = 2;
			}
		}
		else {
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
		}
	}

	switch(localDelayOperation){
	case 1:
		break;
	case 2:
		gameShell->networkMessageHandler(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);
		break;
	}
}

void UI_NetCenter::deleteAccount()
{
	int localDelayOperation = 0;

	{
		MTAuto lock(lock_);
		
		LogMsg("UI_NetCenter: DELETE ACCOUNT ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			if(!password_.empty()){
				LogMsg("started\n");
				setStatus(UI_NET_WAITING);
				delayOperation_ = ONLINE_DELETE_LOGIN;
				localDelayOperation = 1;
			}
			else {
				LogMsg("ERROR, password empty\n");
				localDelayOperation = 2;
			}
		}
		else {
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
		}
	}

	switch(localDelayOperation){
	case 1:
		break;
	case 2:
		UI_LogicDispatcher::instance().handleNetwork(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);			
		break;
	}
}

bool UI_NetCenter::canCreateGame() const
{
	if(!gameShell->getNetClient())
		return false;

	if(acyncEventWaiting())
		return false;

	if(gameCreated())
		return false;

	if(!UI_LogicDispatcher::instance().currentMission())
		return false;

	if(UI_LogicDispatcher::instance().currentProfile().lastCreateGameName.empty())
		return false;

	const char* dn = UI_LogicDispatcher::instance().currentPlayerDisplayName();
	if(!dn || !*dn)
		return false;
	
	return true;
}

void UI_NetCenter::createGame()
{
	string gameName = UI_LogicDispatcher::instance().currentProfile().lastCreateGameName;
	string playerName = UI_LogicDispatcher::instance().currentPlayerDisplayName();
	const MissionDescription* mission = 0;
	bool delayStart = false;

	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: CREATE GAME ");

		if(!gameShell->getNetClient()){
			LogMsg("ERROR, net client not created\n");
			return;
		}

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameCreated()){
			LogMsg("skiped, game greated\n");
			return;
		}

		if(mission = UI_LogicDispatcher::instance().currentMission())
		{
			if(!gameName.empty() && !playerName.empty()){
				updateGameList();

				string originalGameName = gameName;
				bool nameExist = true;
				for(int sufix = 0; nameExist; ++sufix){
					nameExist = false;
					if(sufix > 0){
						XBuffer suf; 
						suf <= sufix;
						xxassert(suf.tell() < MAX_MULTIPALYER_GAME_NAME, "Ну ни хрена себе!");
						gameName = originalGameName.substr(0, MAX_MULTIPALYER_GAME_NAME - suf.tell());
						gameName += suf.c_str();
					}
					for(int i = 0; i < netGames_.size(); ++i)
						if(gameName == netGames_[i].gameName){
							nameExist = true;
							break;
						}
				}

				if(gameName != originalGameName)
					LogMsg(", renamed to: %s, ", gameName.c_str());
				
				LogMsg("started\n");

				setStatus(UI_NET_WAITING);

				delayOperation_ = CREATE_GAME;
				delayStart = true;
			}
			else {
				LogMsg("ERROR, empty game name or player name for creation\n");
				commit(UI_NET_ERROR);
			}
		}
		else {
			LogMsg("ERROR, not selected map for creation\n");
			commit(UI_NET_ERROR);
		}
	}

	if(delayStart)
		if(PNetCenter* center = gameShell->getNetClient())
			center->CreateGame(
					gameName.c_str(),
					*mission,
					playerName.c_str(),
					Race(),
					0
				);
		else {
			LogMsg("ERROR start CreateGame, pNnetCenter killed\n");
			commit(UI_NET_ERROR);
		}

}

bool UI_NetCenter::canJoinGame() const
{
	if(!gameShell->getNetClient())
		return false;

	if(acyncEventWaiting())
		return false;

	if(gameCreated())
		return false;

	if(!gameSelected())
		return false;

	if(selectedGameInfo_.gameStatusInfo.flag_gameRun)
		return false;
	
	return true;
}

void UI_NetCenter::joinGame()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: JOIN GAME ");
		
		if(!gameShell->getNetClient()){
			LogMsg("ERROR, net client not created\n");
			return;
		}

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameCreated()){
			LogMsg("skiped, game greated\n");
			return;
		}

		if(!gameSelected()){
			LogMsg("skiped, NOT SELECTED GAME for join\n");
			commit(UI_NET_ERROR);
			return;
		}

		LogMsg("started\n");
		setStatus(UI_NET_WAITING);
		delayOperation_ = CREATE_GAME;

	}

	if(PNetCenter* center = gameShell->getNetClient())
		center->JoinGame(
				selectedGameInfo_.gameHostGUID,
				UI_LogicDispatcher::instance().currentPlayerDisplayName(),
				Race(),
				0
			);
	else {
		LogMsg("ERROR start CreateGame, pNnetCenter killed\n");
		commit(UI_NET_ERROR);
	}
}

bool UI_NetCenter::isNetGame() const
{
	return gameShell->getNetClient();
}

bool UI_NetCenter::isServer() const
{
	if(!gameCreated())
		return true;

	if(PNetCenter* center = gameShell->getNetClient())
		return center->isHost();
	return false;
}

const char* UI_NetCenter::natType() const
{
	return "";
}

void UI_NetCenter::startGame()
{
	LogMsg("UI_NetCenter: START GAME ");

	if(PNetCenter* center = gameShell->getNetClient()){
		LogMsg("done\n");
		center->StartLoadTheGame();
	}
	else
		LogMsg("ERROR, net client not created\n");
}

void UI_NetCenter::teamConnect(int teemIndex)
{
	LogMsg("UI_NetCenter: JOIN TEEM ");
	
	if(PNetCenter* center = gameShell->getNetClient()){
		LogMsg("%d\n", teemIndex);
		center->JoinCommand(teemIndex);
	}
	else
		LogMsg("ERROR, net client not created\n");
}

void UI_NetCenter::teamDisconnect(int teemIndex, int cooperativeIndex)
{
	LogMsg("UI_NetCenter: LEAVE TEEM ");
	
	if(PNetCenter* center = gameShell->getNetClient()){
		LogMsg("(%d,%d)\n", teemIndex, cooperativeIndex);
		center->KickInCommand(teemIndex, cooperativeIndex);
	}
	else
		LogMsg("ERROR, net client not created\n");
}

void UI_NetCenter::release()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: RELEASE ");

		if(!gameShell->getNetClient()){
			LogMsg("ERROR, net client not created\n");
			return;
		}

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		LogMsg("done\n");

		onlineLogined_ = false;

		clear();

	}

	resetChatBoard(false);

	gameShell->stopNetClient();
}



void UI_NetCenter::setChatString(const char* stringForSend) 
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: SET CHAT message: %s\n", stringForSend);

	currentChatString_ = stringForSend;
}

bool UI_NetCenter::sendChatString(int rawIntData) 
{
	string forSend(UI_LogicDispatcher::instance().currentPlayerDisplayName());
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: SEND CHAT ");
		
		if(!gameShell->getNetClient()){
			LogMsg("ERROR, net client not created\n");
			return false;
		}
		else if(currentChatString_.empty()){
			LogMsg("skiped: message empty\n");
			return false;
		}
		else {
			forSend += ": ";
			forSend += currentChatString_;
			currentChatString_.clear();
			LogMsg("message: (%d, %s) ", rawIntData, forSend.c_str());
		}
	}
	
	if(PNetCenter* center = gameShell->getNetClient()){
		bool st = center->chatMessage(ChatMessage(forSend.c_str(), rawIntData));
		LogMsg(st ? "done\n" : "skiped by netClient\n");
		return st;
	}
	else {
		LogMsg("skiped\n");
	}
	
	return false;
}

void UI_NetCenter::handleChatString(const char* str) 
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: HANDLE CHAT message: %s\n", str);

	chatBoard_.push_back(str);
}

void UI_NetCenter::getChatBoard(ComboStrings &board) const
{
	MTAuto lock(lock_);

	board = chatBoard_;
}

void UI_NetCenter::clearChatBoard()
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: CLEAR CHAT done\n");

	currentChatString_.clear();
	chatBoard_.clear();
}

void UI_NetCenter::resetChatBoard(bool unsubscribe) 
{
	ChatChannelID sid = -1;
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: RESET CHAT done\n");

		currentChatString_.clear();
		chatBoard_.clear();
		chatUsers_.clear();
//		chatChanelInfos_.clear();

		selectedChannel_ = -1;
		autoSubscribeMode_ = false;
		lastSubscribeAttempt_ = -1;

		currentChatChannelName_.clear();

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW))
			return;

		if(subscribedChannel_ == -1)
			return;

		if(unsubscribe)
			sid = subscribedChannel_;
		subscribedChannel_ = -1;

		subscribeWaitingChannel_ = -1;
	}

	if(sid != -1){
		LogMsg("UI_NetCenter: USUBSCRIBE CURRENT CHANNEL done\n");
	}
}

void UI_NetCenter::updateChatUsers()
{
	/*
	vector<DWInterfaceSimple::ChatMemberInfo> infos;
	
	if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW));
	else
		return;

	MTAuto lock(lock_);

	chatUsers_.clear();

	vector<DWInterfaceSimple::ChatMemberInfo>::const_iterator it;
	FOR_EACH(infos, it)
		chatUsers_.push_back(it->name);
	*/
}

int UI_NetCenter::getChatUsers(ComboStrings &users) const
{
	MTAuto lock(lock_);

	users = chatUsers_;
	return -1;
}

void UI_NetCenter::clear()
{
	selectedGameIndex_ = -1;
	selectedGlobalStatisticIndex_ = -1;
	currentStatisticFilterRace_ = -1;
	currentStatisticFilterGamePopulation_ = -1;
	gameCreated_ = false;
	delayOperation_ = DELAY_NONE;
	globalStatistics_.clear();
	pausePlayerList_.clear();
	firstScorePosition_ = 1;
	isOnPause_ = false;
}

void UI_NetCenter::queryGlobalStatistic(bool force)
{
	int localStatisticFilterRace = 0;
	int localStatisticFilterGamePopulation = 0;
	int localScorePosition = 0;
	bool delayRequerest = false;

	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: QUERY GS ");

		if(acyncEventWaiting()){
			LogMsg("skiped, waiting acync event\n");
			return;
		}

		if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			if(currentStatisticFilterRace_ != UI_LogicDispatcher::instance().currentProfile().statisticFilterRace
				|| currentStatisticFilterGamePopulation_ != UI_LogicDispatcher::instance().currentProfile().statisticFilterGamePopulation)
			{
				firstScorePosition_ = 1;
			}
			else if(!force){
				LogMsg("skiped, filter not changed\n");	
				return;
			}
			LogMsg("started from position %d\n", firstScorePosition_);
			localStatisticFilterRace = currentStatisticFilterRace_ = UI_LogicDispatcher::instance().currentProfile().statisticFilterRace;
			localStatisticFilterGamePopulation = currentStatisticFilterGamePopulation_ = UI_LogicDispatcher::instance().currentProfile().statisticFilterGamePopulation;
			setStatus(UI_NET_WAITING);
			delayOperation_ = QUERY_GLOBAL_STATISTIC;
			localScorePosition = firstScorePosition_;
			delayRequerest = true;
		}
		else {
			LogMsg("ERROR, online client not created\n");
			commit(UI_NET_ERROR);
		}
	}

	if(delayRequerest);
}

void UI_NetCenter::getGlobalStatisticFromBegin()
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: QUERY FROM BEGIN GS page ");

	if(acyncEventWaiting()){
		LogMsg("skiped, waiting acync event\n");
		return;
	}
	LogMsg("started\n");

	firstScorePosition_ = 1;

	queryGlobalStatistic(true);
}

void UI_NetCenter::getAroundMeGlobalStats()
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: QUERY AROUND ME GS page ");

	if(acyncEventWaiting()){
		LogMsg("skiped, waiting acync event\n");
		return;
	}
	LogMsg("started\n");

	firstScorePosition_ = -1;

	queryGlobalStatistic(true);
}


bool UI_NetCenter::canGetNextGlobalStats() const
{
	MTAuto lock(lock_);

	if(acyncEventWaiting())
		return false;

	if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW))
		return false;

	if(firstScorePosition_ > 0 && globalStatistics_.empty())
		return false;

	return true;
}

void UI_NetCenter::getNextGlobalStats()
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: QUERY NEXT GS page ");

	if(acyncEventWaiting()){
		LogMsg("skiped, waiting acync event\n");
		return;
	}
	LogMsg("started\n");

	if(firstScorePosition_ < 1)
		firstScorePosition_ = 1;
	else
		firstScorePosition_ += readGlobalStatsAtOnce;

	queryGlobalStatistic(true);
}

bool UI_NetCenter::canGetPrevGlobalStats() const
{
	MTAuto lock(lock_);

	if(acyncEventWaiting())
		return false;

	if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW))
		return false;

	if(firstScorePosition_ == 1)
		return false;

	return true;
}

void UI_NetCenter::getPrevGlobalStats()
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: QUERY PREV GS page ");

	if(acyncEventWaiting()){
		LogMsg("skiped, waiting acync event\n");
		return;
	}
	LogMsg("started\n");

	firstScorePosition_ -= readGlobalStatsAtOnce;
	if(firstScorePosition_ < 1)
		firstScorePosition_ = 1;

	queryGlobalStatistic(true);
}

void UI_NetCenter::applyNewGlobalStatistic(const GlobalStatistics& stats)
{
	if(selectedGlobalStatisticIndex_ >= 0){
		string name = globalStatistics_[selectedGlobalStatisticIndex_].name;
		selectedGlobalStatisticIndex_ = -1;
		for(int idx = 0; idx < stats.size(); ++idx)
			if(stats[idx].name == name){
				selectedGlobalStatisticIndex_ = idx;
				break;
			}
	}

	globalStatistics_ = stats;

	if(!globalStatistics_.empty())
		firstScorePosition_ = globalStatistics_[0].position;

	LogMsg("UI_NetCenter: GS applyed, cur start from %d\n", firstScorePosition_);
}

void UI_NetCenter::selectGlobalStaticticEntry(int idx)
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: GS select: %i", idx);

	selectedGlobalStatisticIndex_ = idx;

	if(selectedGlobalStatisticIndex_ >= (int)globalStatistics_.size()){
		selectedGlobalStatisticIndex_ = -1;
		LogMsg(" skiped, idx > listSize\n");
	}
	else
		LogMsg(" done\n");
}

int UI_NetCenter::getGlobalStatistic(const ShowStatisticTypes& format, ComboStrings &board) const
{
	MTAuto lock(lock_);
	
	board.clear();
	XBuffer buf;

	GlobalStatistics::const_iterator it;
	FOR_EACH(globalStatistics_, it){
		buf.init();
		ShowStatisticTypes::const_iterator frm;
		FOR_EACH(format, frm){
			frm->getValue(buf, *it);
			buf < "\t";
		}
		board.push_back(buf.c_str());
	}

	xassert(selectedGlobalStatisticIndex_ < (int)board.size());
	return selectedGlobalStatisticIndex_;
}

int UI_NetCenter::getCurrentGlobalStatisticValue(StatisticType type)
{
	MTAuto lock(lock_);

	if(selectedGlobalStatisticIndex_ >= 0)
		return globalStatistics_[selectedGlobalStatisticIndex_][type];
	
	return 0;
}

void UI_NetCenter::setPausePlayerList(const ComboStrings& playerList)
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: setPausePlayerList, list %s\n", playerList.empty() ? "empty" : "not empty");

	pausePlayerList_ = playerList;
	
	isOnPause_ = !pausePlayerList_.empty();
}

void UI_NetCenter::getPausePlayerList(ComboStrings& playerList) const
{
	MTAuto lock(lock_);
	
	playerList = pausePlayerList_;
}


void UI_NetCenter::updateFilter()
{
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: UPDATE FILTER ");

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("skiped, online client not created\n");
			return;
		}
		else {
			LogMsg((XBuffer()
				< "|gt=" <= UI_LogicDispatcher::instance().currentProfile().gameTypeFilter
				< ", slot=" <= UI_LogicDispatcher::instance().currentProfile().playersSlotFilter
				< ", map={size=" <= UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().size()
				< ", first=" < (UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().empty()
				? "EMPTY"
				: (UI_LogicDispatcher::instance().getMissionByID(UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().front())
				? UI_LogicDispatcher::instance().getMissionByID(UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().front())->interfaceName()
				: "MISSION UNDEFINED"))
				< "}| done\n").c_str());
		}
	}
		
	if(PNetCenter* center = gameShell->getNetClient())
		center->setGameHostFilter(
			UI_LogicDispatcher::instance().currentProfile().gameTypeFilter,
			UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter(),
			UI_LogicDispatcher::instance().currentProfile().playersSlotFilter
		);
}

void UI_NetCenter::queryGameVersion()
{
	xassert(acyncEventWaiting() && "несинхронный запрос версии игры");

	LogMsg("UI_NetCenter: GET GAME VERSION ");

	if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
		LogMsg("skiped, online client not created\n");
		setStatus(UI_NET_ERROR);
		return;
	}
	LogMsg("started\n");
}

bool UI_NetCenter::setGameVersion()
{
	xassert(acyncEventWaiting() && "несинхронная обработка версии игры");
	
	version_ < '\0';

	LogMsg("UI_NetCenter: PARSE GAME VERSION\n<file dump begin>\n");
	LogMsg(version_.c_str());
	LogMsg("\n<file dump end>\n");

	return UI_LogicDispatcher::instance().parseGameVersion(version_.buffer());
}

class ChanelSortRule{
};

void UI_NetCenter::updateChatChannels()
{
	/*
	ChatChanelInfos infos;

	if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW) && onlineLogined_ && !gameCreated_);
	else
		return;

	sort(infos.begin(), infos.end(), ChanelSortRule());
	*/

	bool needAutoSubscribe = false;
	/*

	{
		MTAuto lock(lock_);

		chatChanelInfos_.clear();
		ChatChanelInfos::const_iterator it;
		FOR_EACH(infos, it){
			chatChanelInfos_.push_back(*it);
		}

		if(subscribedChannel_ == -1)
			if(subscribeWaitingChannel_ == -1)
				if(chatChanelInfos_.empty())
					currentChatChannelName_.clear();
				else {
					needAutoSubscribe = true;
					currentChatChannelName_ = GET_LOC_STR(UI_COMMON_TEXT_CONNECTING);
				}
			else
				currentChatChannelName_ = GET_LOC_STR(UI_COMMON_TEXT_CONNECTING);
		else {
			ChatChanelInfos::const_iterator info = find(chatChanelInfos_.begin(), chatChanelInfos_.end(), subscribedChannel_);
			if(info != chatChanelInfos_.end())
				currentChatChannelName_ = info->name;
			else
				currentChatChannelName_ = "ERROR";
		}
	}
	*/

	if(needAutoSubscribe)
		autoSubscribeChatChannel();
}

int UI_NetCenter::getChatChannels(ComboStrings& channels) const
{
	MTAuto lock(lock_);

	channels.clear();
	int selected = -1;

	XBuffer buf;
	/*
	ChatChanelInfos::const_iterator it;
	FOR_EACH(chatChanelInfos_, it){
		buf.init();
		buf < it->name.c_str() < "\t" <= it->numSubscribers < "/" <= it->maxSubscribers;
		if(it->id == selectedChannel_)
			selected = channels.size();
		channels.push_back(buf.c_str());
	}

	if(channels.empty())
		channels.push_back(GET_LOC_STR(UI_COMMON_TEXT_CONNECTING));
	*/

	return selected;
}

void UI_NetCenter::getCurrentChatChannelName(string& name) const
{
	MTAuto lock(lock_);

	name = currentChatChannelName_;
}

void UI_NetCenter::selectChatChannel(int channel)
{
	LogMsg("UI_NetCenter: SELECT CHANNEL ");
	if(channel < 0){
		LogMsg("skiped, chan num < 0\n");
		return;
	}

	MTAuto lock(lock_);

	/*
	if(channel >= chatChanelInfos_.size()){
		LogMsg("skiped, chan num > channels size\n");
		return;
	}

	LogMsg("ok, channel:");
	LogMsg(chatChanelInfos_[channel].name.c_str());
	LogMsg("\n");

	selectedChannel_ = chatChanelInfos_[channel].id;
	*/
}

void UI_NetCenter::autoSubscribeChatChannel()
{
	ChatChannelID sid = -1;

	{	
		MTAuto lock(lock_);

//		xassert(!chatChanelInfos_.empty() && subscribedChannel_ == -1 && subscribeWaitingChannel_ == -1);

		LogMsg("UI_NetCenter: AUTOSUBSCRIBE CHANNEL MODE ");

		if(!onlineLogined_){
			autoSubscribeMode_ = false;
			LogMsg("stoped, online not login\n");
			return;
		}
		else if(gameCreated_){
			autoSubscribeMode_ = false;
			LogMsg("stoped, game created\n");
			return;
		}

		if(autoSubscribeMode_){
			/*
			ChatChanelInfos::const_iterator info = find(chatChanelInfos_.begin(), chatChanelInfos_.end(), lastSubscribeAttempt_);
			++info;
			if(info != chatChanelInfos_.end()){
				LogMsg("next channel attempt.\n");
				sid = info->id;
			}
			else {
				LogMsg("finished unsusseful.\n");
				autoSubscribeMode_ = false;
			}
			*/
		}
		else {
			autoSubscribeMode_ = true;
			LogMsg("main channel attempt.\n");
			/* sid = chatChanelInfos_[0].id; */
		}

		lastSubscribeAttempt_ = sid;
	}

	if(sid != -1)
		enterChatChannel(sid);

}

void UI_NetCenter::enterChatChannel(ChatChannelID forceID)
{
	LogMsg("UI_NetCenter: ENTER CHANNEL ");

	ChatChannelID sid = -1;
	{
		MTAuto lock(lock_);

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("skiped, online client not created\n");
			return;
		}

		if(subscribeWaitingChannel_ != -1){
			LogMsg("skiped, connect in progress\n");
			return;
		}

		if(forceID != -1){
			LogMsg(XBuffer() < "(force id=" <= (unsigned int)(forceID) < ") ");
			selectedChannel_ = forceID;
		}

		if(selectedChannel_ == -1){
			LogMsg("skiped, no channel selected\n");
			return;
		}

		if(selectedChannel_ == subscribedChannel_){
			LogMsg("skiped, this channel just suscribed\n");
			return;
		}

		/*
		ChatChanelInfos::const_iterator info = find(chatChanelInfos_.begin(), chatChanelInfos_.end(), selectedChannel_);
		if(info == chatChanelInfos_.end()){
			LogMsg("skiped, no channel found with selected id\n");
			return;
		}

		subscribeWaitingChannel_ = selectedChannel_;
		sid = subscribeWaitingChannel_;
		*/
	}

	xassert(sid != -1);
	LogMsg("done.\n");
}

void UI_NetCenter::chatSubscribeOK()
{
	LogMsg("UI_NetCenter: SUBSCRIBE ");

	ChatChannelID sid = -1;

	{
		MTAuto lock(lock_);

		if(subscribeWaitingChannel_ == -1 || !onlineLogined_){
			LogMsg(" ERROR, not waiting subscribe or not logined\n");
			xassert(false && "серьезная ошибка подписывания на канал");
			return;
		}

		sid = subscribedChannel_;
		subscribedChannel_ = subscribeWaitingChannel_;
		subscribeWaitingChannel_ = -1;

		/*
		ChatChanelInfos::const_iterator info = find(chatChanelInfos_.begin(), chatChanelInfos_.end(), subscribedChannel_);
		if(info != chatChanelInfos_.end()){
			LogMsg(" OK\n");
			currentChatChannelName_ = info->name;
		}
		else {
			LogMsg(" STRANGE!!!, not found channel in list\n");
			currentChatChannelName_ = "ERROR";
		}
		*/

		autoSubscribeMode_ = false;
	}
}

void UI_NetCenter::chatSubscribeFailed()
{
	LogMsg("UI_NetCenter: SUBSCRIBE FAILED\n");

	MTAuto lock(lock_);

	subscribeWaitingChannel_ = -1;
}
