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
	isOnPause_ = false;
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
		reset();
		UI_LogicDispatcher::instance().networkDisconnect(false);
		return;
	}
	else if(status == UI_NET_OK){
		switch(delayOperation_){
		case CREATE_GAME:
			gameCreated_ = true;
			break;
		case ONLINE_NEW_LOGIN:
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
		}
	}
	
	delayOperation_ = DELAY_NONE;
	setStatus(status);
	lock_.unlock();
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
	LogMsg("UI_NetCenter: RESET PASSES done\n");

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

void UI_NetCenter::reset()
{
	{
		MTAuto lock(lock_);

		resetPasswords();

		LogMsg("UI_NetCenter: RESET ");

		clear();
		UI_LogicDispatcher::instance().resetCurrentMission();
	}
	
	if(PNetCenter* center = gameShell->getNetClient()){
		LogMsg("done\n");
		center->ResetAndStartFindHost();
	}
	else
		LogMsg("skiped, net system not created\n");
}

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

	GameHostInfos::iterator it = netGames_.begin();
	while(it != netGames_.end())
		if(it->gameStatusInfo.flag_gameRun)
			it = netGames_.erase(it);
		else
			++it;

	if(selectedGameIndex_ >= 0){
		selectedGameIndex_ = -1;
		for(int i = 0; i < netGames_.size(); ++i)
			if(netGames_[i].gameHostGUID == selectedGameInfo_.gameHostGUID){
				selectedGameIndex_ = i;
				break;
			}
		if(selectedGameIndex_ == -1)
			UI_LogicDispatcher::instance().resetCurrentMission();
	}
	
	return true;
}

int UI_NetCenter::getGameList(ComboStrings&  strings, GameListInfoType infoType)
{
	MTAuto lock(lock_);

	if(!gameShell->getNetClient())
		return -1;

	for(int i = 0; i < netGames_.size(); i++){
		switch(infoType){
		case GAME_INFO_TAB_LIST: {
			string tabs = netGames_[i].gameName;
			tabs += "\t\t";
			XBuffer buf;
			buf <= netGames_[i].gameStatusInfo.ping;
			tabs += buf.c_str();
			tabs += "\t\t";
			buf.init();
			buf <= netGames_[i].gameStatusInfo.currrentPlayers < "/" <= netGames_[i].gameStatusInfo.maximumPlayers;
			tabs += buf.c_str();
			tabs += "\t\t";
			tabs += netGames_[i].gameStatusInfo.missionName();
			tabs += "\t\t";
			tabs += (netGames_[i].gameStatusInfo.jointGameType.isUseMapSetting()
				? GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME)
				: GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME));
			tabs += "\t\t";
			tabs += netGames_[i].hostName;
			strings.push_back(tabs);
			break;
								 }
		case GAME_INFO_GAME_NAME:
			strings.push_back(netGames_[i].gameName);
			break;
		case GAME_INFO_HOST_NAME:
			strings.push_back(netGames_[i].hostName);
			break;
		case GAME_INFO_WORLD_NAME:
			strings.push_back(netGames_[i].gameStatusInfo.missionName());
			break;
		case GAME_INFO_PLAYERS_NUMBER: {
			XBuffer buf;
			buf <= netGames_[i].gameStatusInfo.currrentPlayers < "/" <= netGames_[i].gameStatusInfo.maximumPlayers;
			strings.push_back(buf.c_str());
			break;
							 }
		case GAME_INFO_PING:{
			XBuffer buf;
			buf <= netGames_[i].gameStatusInfo.ping;
			strings.push_back(buf.c_str());
			break;
				  }
		case GAME_INFO_GAME_TYPE:
			if(netGames_[i].gameStatusInfo.jointGameType.isUseMapSetting())
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME));
			else
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME));
			break;
		}
	}

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
				UI_LogicDispatcher::instance().currentProfile().quickStartMissionFilter.getFilter()
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
				delayOperation_ = ONLINE_NEW_LOGIN;
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
		gameShell->networkMessageHandler(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);			
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

		clear();
	}

	gameShell->stopNetClient();
}

void UI_NetCenter::setChatString(const char* stringForSend) 
{
	MTAuto lock(lock_);

	LogMsg("UI_NetCenter: SET CHAT message: %s\n", stringForSend);

	currentChatString_ = stringForSend;
}

void UI_NetCenter::sendChatString(int rawIntData) 
{
	string forSend(UI_LogicDispatcher::instance().currentPlayerDisplayName());
	{
		MTAuto lock(lock_);

		LogMsg("UI_NetCenter: SEND CHAT ");
		
		if(!gameShell->getNetClient()){
			LogMsg("ERROR, net client not created\n");
			return;
		}
		else if(currentChatString_.empty()){
			LogMsg("skiped: message empty\n");
			return;
		}
		else {
			forSend += ": ";
			forSend += currentChatString_;
			currentChatString_.clear();
			LogMsg("message: (%d, %s)\n", rawIntData, forSend.c_str());
		}
	}
	
	if(PNetCenter* center = gameShell->getNetClient())
		center->chatMessage(ChatMessage(forSend.c_str(), rawIntData));
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

	LogMsg("UI_NetCenter: CLEAR CHAT board\n");

	currentChatString_.clear();
	chatBoard_.clear();
	chatUsers_.clear();
}

bool UI_NetCenter::updateChatUsers()
{
	
	if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW))
		;
	else
		return false;

	MTAuto lock(lock_);

	chatUsers_.clear();

	return true;
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

	if(selectedGlobalStatisticIndex_ > 0)
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

		LogMsg((XBuffer()
			< "|gt=" <= UI_LogicDispatcher::instance().currentProfile().gameTypeFilter
			< ", slot=" <= UI_LogicDispatcher::instance().currentProfile().playersSlotFilter
			< ", map={size=" <= UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().size()
				< ", first=" < (UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().empty()
				? "EMPTY"
				: (UI_LogicDispatcher::instance().getMissionByID(UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().front())
					? UI_LogicDispatcher::instance().getMissionByID(UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter().front())->interfaceName()
					: "MISSION UNDEFINED"))
			< "}| ").c_str());

		if(!gameShell->isNetClientConfigured(PNCWM_ONLINE_DW)){
			LogMsg("skiped, online client not created\n");
			return;
		}
		else {
			LogMsg("done\n");
		}
	}
		
	if(PNetCenter* center = gameShell->getNetClient())
		center->setGameHostFilter(
			UI_LogicDispatcher::instance().currentProfile().gameTypeFilter,
			UI_LogicDispatcher::instance().currentProfile().findMissionFilter.getFilter(),
			UI_LogicDispatcher::instance().currentProfile().playersSlotFilter
		);
}
