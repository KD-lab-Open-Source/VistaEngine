#include "StdAfx.h"

#include "P2P_interface.h"
#include "GS_interface.h"

#include "GameShell.h"
#include "UniverseX.h"
#include "UI_Logic.h"

#include "Lmcons.h"

#include "..\terra\terra.h"

#include <algorithm>

#include "dxerr9.h"

#include "QSWorldsMgr.h"

#include "LogMsg.h"


const UNetID UNetID::UNID_ALL_PLAYERS(0,0);

char* terCurrentServerName; //Из ini файла

#define caseR(a) case a: return #a;
const char* PNetCenter::getStrWorkMode()
{
	switch(workMode){
	caseR(PNCWM_LAN);
	caseR(PNCWM_ONLINE_GAMESPY);
	caseR(PNCWM_ONLINE_P2P);
	caseR(PNCWM_LAN_DW);
	caseR(PNCWM_ONLINE_DW);
	default:
		return("PNCWM_???");
	}
}

const char* PNetCenter::getStrState()
{
	switch(nCState_th2){
	caseR(PNC_STATE__NONE);
	caseR(NSTATE__FIND_HOST);
	caseR(NSTATE__PARKING);
	caseR(PNC_STATE__NET_CENTER_CRITICAL_ERROR);
	caseR(PNC_STATE__ENDING_GAME);

	caseR(NSTATE__QSTART_NON_CONNECT);
	caseR(NSTATE__QSTART_HOSTING_CLIENTING);
	caseR(NSTATE__QSTART_HOST);
	caseR(NSTATE__QSTART_CLIENT);

	caseR(PNC_STATE__CONNECTION);
	caseR(PNC_STATE__INFINITE_CONNECTION_2_IP);
	caseR(PNC_STATE__CLIENT_TUNING_GAME);
	caseR(PNC_STATE__CLIENT_LOADING_GAME);
	caseR(PNC_STATE__CLIENT_GAME);

	caseR(PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0);
	caseR(PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB);

	caseR(NSTATE__HOST_TUNING_GAME);
	caseR(NSTATE__HOST_LOADING_GAME);
	caseR(NSTATE__HOST_GAME);

	caseR(PNC_STATE__NEWHOST_PHASE_0);
	caseR(PNC_STATE__NEWHOST_PHASE_A);
	caseR(PNC_STATE__NEWHOST_PHASE_B);
	default:
		return("PNC_???");
	}
}

const char* PNetCenter::getStrInternalCommand(InternalCommand curInternalCommand)
{
	switch(curInternalCommand.cmd()){
	caseR(NCmd_Null);
	caseR(PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST);
	caseR(PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST);
	caseR(NCmd_QuickStart);
//Client back commands
	caseR(PNC_COMMAND__CLIENT_STARTING_LOAD_GAME);
	caseR(PNC_COMMAND__CLIENT_STARTING_GAME);
//Special command
	caseR(PNC_COMMAND__STOP_GAME_AND_ASSIGN_HOST_2_MY);
	caseR(PNC_COMMAND__STOP_GAME_AND_WAIT_ASSIGN_OTHER_HOST);

	caseR(PNC_COMMAND__END);
	caseR(PNC_COMMAND__ABORT_PROGRAM);

	caseR(PNC_COMMAND__END_GAME);
	caseR(PNCCmd_Reset2FindHost);
	caseR(NCmd_Parking);
	default: return ("PNC_COMMAND_???");
	}
}
const char* PNetCenter::getStrNetMessageCode(eNetMessageCode mc)
{
	switch(mc){
		caseR(NetMessageCode_NULL);
		caseR(NetGEC_ConnectionFailed);
		caseR(NetGEC_HostTerminatedSession);
		caseR(NetGEC_GameDesynchronized);
		caseR(NetGEC_GeneralError);

		caseR(NetGEC_DWLobbyConnectionFailed);
		caseR(NetGEC_DWLobbyConnectionFailed_MultipleLogons);
		//NetGEC_DemonWareTerminatedSession);

		caseR(NetRC_Init_Ok);
		caseR(NetRC_Init_Err);

		caseR(NetRC_CreateAccount_Ok);
		caseR(NetRC_CreateAccount_BadLicensed);
		caseR(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);
		caseR(NetRC_CreateAccount_IllegalOrVulgarUserName_Err);
		caseR(NetRC_CreateAccount_UserNameExist_Err);
		caseR(NetRC_CreateAccount_MaxAccountExceeded_Err);
		caseR(NetRC_CreateAccount_Other_Err);

		caseR(NetRC_ChangePassword_Ok);
		caseR(NetRC_ChangePassword_Err);

		caseR(NetRC_DeleteAccount_Ok);
		caseR(NetRC_DeleteAccount_Err);

		caseR(NetRC_Configurate_Ok);
		caseR(NetRC_Configurate_ServiceConnect_Err);
		caseR(NetRC_Configurate_UnknownName_Err);
		caseR(NetRC_Configurate_IncorrectPassword_Err);
		caseR(NetRC_Configurate_AccountLocked_Err);

		caseR(NetRC_CreateGame_Ok);
		caseR(NetRC_CreateGame_CreateHost_Err);
		caseR(NetRC_JoinGame_Ok);
		caseR(NetRC_JoinGame_GameSpyConnection_Err);
		caseR(NetRC_JoinGame_GameSpyPassword_Err);
		caseR(NetRC_JoinGame_Connection_Err);
		caseR(NetRC_JoinGame_GameIsRun_Err);
		caseR(NetRC_JoinGame_GameIsFull_Err);
		caseR(NetRC_JoinGame_GameNotEqualVersion_Err);

		caseR(NetRC_QuickStart_Ok);
		caseR(NetRC_QuickStart_Err);

		caseR(NetRC_ReadStats_Ok);
		caseR(NetRC_ReadStats_Empty);
		caseR(NetRC_ReadStats_Err);

		caseR(NetRC_WriteStats_Ok);
		caseR(NetRC_WriteStats_Err);

		caseR(NetRC_ReadGlobalStats_Ok);
		caseR(NetRC_ReadGlobalStats_Err);

		caseR(NetMsg_PlayerDisconnected);
		caseR(NetMsg_PlayerExit);

		default: return ("eNetMessageCode - ???");
	}
}
/*
struct arun{
	int l;
	arun(){
		char buf[256];
		checkInetAddress(buf);
	}
};
arun a;
*/
bool checkInetAddress(const char* ipStr)
{
/*	CoInitializeEx(0, COINIT_MULTITHREADED);

	IDirectPlay8Address *pdpAddress;
	IDirectPlay8AddressIP *pdpAddressIP;
 
	HRESULT hResult;
	hResult = CoCreateInstance (CLSID_DirectPlay8Address,
							  NULL, 
							  CLSCTX_INPROC_SERVER,
							  IID_IDirectPlay8Address, 
							  (LPVOID *) &pdpAddress);
	hResult = pdpAddress->QueryInterface (IID_IDirectPlay8AddressIP,
										(LPVOID *) &pdpAddressIP);

    WCHAR* wszHostName = new WCHAR[strlen(ipStr) + 1];
	MultiByteToWideChar(CP_ACP, 0, ipStr, -1, wszHostName, strlen(ipStr) + 1);

	hResult = pdpAddressIP->BuildAddress (wszHostName,0);
	bool result;
	if(hResult==S_OK)result=true;
	else false;
	pdpAddressIP->Release();
	pdpAddress->Release();

	CoUninitialize();

	return result;*/

//	unsigned long ip=inet_addr(strIP);
//	if(ip!=INADDR_NONE) return true; //internalIP_DP=ip;
//	return false;
	int len=strlen(ipStr);
	if(len>=MAX_PATH) return false;
	char buf[MAX_PATH];
	int counterOk=0;
	int i=0;
	for(;;){
		int k=0;
		while(i<len && !isspace(ipStr[i]) && (ipStr[i]!=';') && (ipStr[i]!=',') ){
			buf[k++]=ipStr[i++];
		}
		buf[k++]=0;
		if(strlen(buf)){
			unsigned long ip=inet_addr(buf);
			if(ip!=INADDR_NONE) counterOk++;
		}
		if(i>=len) break;
		i++;
	}
	if(counterOk) return true;
	else return false;
}


STARFORCE_API PNetCenter::PNetCenter(e_PNCWorkMode _workMode)
	: in_ClientBuf(1000000, 1), out_ClientBuf(1000000, 1), in_HostBuf(1000000, 1), out_HostBuf(1000000, 1),
	startGameParam(m_GeneralLock)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(1);

	LogMsg("---Creating PNetCenter---\n");

	//инитятся один раз!
	workMode=_workMode;
	nextQuantTime_th2=0;
	TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION=5000;//5s
	IniManager("Network.ini").getInt("General","TimeOutClientOrServerReceive", (int&)TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION);
	TIMEOUT_DISCONNECT=600000;
	IniManager("Network.ini").getInt("General","TimeOutDisconnect", (int&)TIMEOUT_DISCONNECT);
	MAX_TIME_PAUSE_GAME=40000;
	IniManager("Network.ini").getInt("General","MaxTimePauseGame", (int&)MAX_TIME_PAUSE_GAME);

	m_nClientSgnCheckError = 0; //only DP

	flag_NetworkSimulation=false;
	if(IniManager("Network.ini").getInt("General", "NetworkSimulator")) flag_NetworkSimulation=true;
	m_dwPort = 0;
	m_dwPort=IniManager("Network.ini").getInt("General", "Port");
	flag_EnableHostMigrate=false;
	if(IniManager("Network.ini").getInt("General", "HostMigrate")) flag_EnableHostMigrate=true;
	flag_NoUseDPNSVR=false;
	if(IniManager("Network.ini").getInt("General", "NoUseDPNSVR")) flag_NoUseDPNSVR=true;
	m_DPSigningLevel=0;
	m_DPSigningLevel=IniManager("Network.ini").getInt("General", "DPSigningLevel");
	flag_DecreaseSpeedOnSlowestPlayer=false;
	if(IniManager("Network.ini").getInt("General", "DecreaseSpeedOnSlowestPlayer")) flag_DecreaseSpeedOnSlowestPlayer=true;

	deleteUsersSuspended.reserve(NETWORK_PLAYERS_MAX);

	resetAllVariable_th1();

	// DP Init
	//m_hEnumAsyncOp=0;
	m_hEnumAsyncOp_Arr.clear();
	m_pDPPeer=0;

	nCState_th2=PNC_STATE__NONE;

	gameSpyInterface=0;

	//Start work Thread
	hSecondThreadInitComplete=CreateEvent(0, true, false, 0);
	hCommandExecuted=CreateEvent(0, true, false, 0);

	//hSecondThread=INVALID_HANDLE_VALUE;

	DWORD ThreadId;
	hSecondThread=CreateThread( NULL, 0, InternalServerThread, this, /*NULL,*/ 0, &ThreadId);

	if(WaitForSingleObject(hSecondThreadInitComplete, INFINITE) != WAIT_OBJECT_0) {
		xassert(0&&"NetCenter:Error second thread init");
		ErrH.Abort("Network: General error 1!");
	}
	//::SetThreadPriority(hSecondThread, THREAD_PRIORITY_HIGHEST);


	switch(workMode){
	case PNCWM_LAN:
		break;
	case PNCWM_ONLINE_GAMESPY:
		gameSpyInterface=new GameSpyInterface(this);
		break;
	case PNCWM_LAN_DW:
		break;
	case PNCWM_ONLINE_DW:
		break;
	case PNCWM_ONLINE_P2P:
		break;
	}

	SECUROM_MARKER_HIGH_SECURITY_OFF(1);
}


void PNetCenter::Configurate(const char* playerName, const char* _password, const char* InternetAddress)
{
	//if(_password) gamePassword=_password;
	//else gamePassword="";
	gamePassword="";

	needHostList.clear();

	//e_NetCenterConstructorReturnCode result=NCC_RC_OK;
	eNetMessageCode result=NetRC_Configurate_Ok;

	switch(workMode){
	case PNCWM_LAN:
		gameShell->networkMessageHandler(result);
		break;
	case PNCWM_ONLINE_GAMESPY:
		{
			GameSpyInterface::e_connectResult gsResult;
			if(playerName==0)
				gsResult=gameSpyInterface->Connect("Player");
			else 
				gsResult=gameSpyInterface->Connect(playerName);

			if(gsResult!=GameSpyInterface::CR_Ok){
				if(gsResult==GameSpyInterface::CR_ConnectErr) result=NetRC_Configurate_ServiceConnect_Err;
				else if(gsResult==GameSpyInterface::CR_NickErr) result=NetRC_Configurate_UnknownName_Err;
				else result=NetRC_Configurate_ServiceConnect_Err;
			}
			else result=NetRC_Configurate_Ok;

			gameShell->networkMessageHandler(result);
		}
		break;
	case PNCWM_LAN_DW:
		gameShell->networkMessageHandler(result);
		break;
	case PNCWM_ONLINE_DW:
		{
			xassert(playerName);
			xassert(_password);
			//demonware()->createAccount(playerName, _password);

		}
		break;
	case PNCWM_ONLINE_P2P:
		if(InternetAddress!=0){
			int len=strlen(InternetAddress);
			if(len>=MAX_PATH) break;
			char buf[MAX_PATH];
			int i=0;
			for(;;){
				int k=0;
				while(i<len && !isspace(InternetAddress[i]) && (InternetAddress[i]!=';') && (InternetAddress[i]!=',') ){
					buf[k++]=InternetAddress[i++];
				}
				buf[k++]=0;
				if(strlen(buf)){
					needHostList.push_back(buf);
				}
				if(i>=len) break;
				i++;
			}
		}
		gameShell->networkMessageHandler(result);
		break;
	}

	if(result==NetRC_Configurate_Ok) LogMsg("Configurate PNetCenter-Ok\n");
	else LogMsg("Configurate PNetCenter-Error\n");
}

PNetCenter::~PNetCenter()
{
	LogMsg("---Deleting PNetCenter- start\n");
	ExecuteInternalCommand(PNC_COMMAND__END, true);
	const unsigned int TIMEOUT=15000;// ms
	if( WaitForSingleObject(hSecondThread, TIMEOUT) != WAIT_OBJECT_0) {
		xassert(0&&"Net Thread terminated!!!");
		TerminateThread(hSecondThread, 0);
		LogMsg("---Net Thread terminated!!!-\n");
	}

	ClearDeletePlayerGameCommand();

	hostMissionDescription.clearAllUsersData();//вместо	ClearClients();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	m_DPPacketList.clear();

	///CloseHandle(hServerReady);
	///CloseHandle(hStartServer);

	CloseHandle(hSecondThreadInitComplete);
	CloseHandle(hCommandExecuted);

	if(gameSpyInterface) delete gameSpyInterface;

    
	LogMsg("---Deleting PNetCenter complete\n");
}

const char* PNetCenter::getMissionName()
{
	if(isHost()) return hostMissionDescription.interfaceName();
	else return ("");
}
const char* PNetCenter::getGameName()
{
	if(isHost()) return m_GameName.c_str();
	else return ("");
}
const char* PNetCenter::getGameVer()
{
	return SIMPLE_GAME_CURRENT_VERSION;
}
int PNetCenter::getNumPlayers()
{
	if(isHost()) return hostMissionDescription.playersAmount();
	else return 0;
}
int PNetCenter::getMaxPlayers()
{
	if(isHost()) return hostMissionDescription.playersMaxEasily();//playersAmountMax();
	else return 0;
}
int PNetCenter::getHostPort()
{
	if(isHost()) return m_dwPort;
	else return 0;
}

void PNetCenter::QuickStartThroughDW(const char* playerName, int race, eGameOrder gameOrder, const std::vector<XGUID>& missionFilter)
{
	xassert(workMode==PNCWM_ONLINE_DW);

	//std::vector<XGUID>& missionFilter=_missionFilter;
	//missionFilter.clear(); //DEBUG!!!
	//int noFilter = sGameHostFilter::NO_FILTER;
	//gameHostFilter.set(noFilter, missionFilter, noFilter, noFilter, true, gameOrder);
	//if(isDemonWareMode())
	//	demonware()->setGameHostFilter(gameHostFilter);

	//MissionDescription md("RESOURCE\\Worlds\\test_world_tutorial", GAME_TYPE_MULTIPLAYER);
	//hostMissionDescription=md;

	m_originalQuantInterval=round((float)NORMAL_QUANT_INTERVAL);// /gameSpeed);
	m_quantInterval=m_originalQuantInterval;

	__int64 lowFilterMap = qsWorldsMgr.createFilter(missionFilter);
	startGameParam.setQS(gameOrder, lowFilterMap);
	m_GameName = "QuickStart";
	internalConnectPlayerData.set(playerName, race, lowFilterMap);

	m_QSGameOrder = gameOrder;//GameOrder_1v1;
	ExecuteInternalCommand(NCmd_QuickStart, false);
	//ExecuteInterfaceCommand_thA(NetRC_QuickStart_Ok);
}

void PNetCenter::CreateGame(const char* gameName, const MissionDescription& md, const char* playerName, Race race, unsigned int color, float gameSpeed, const char* password)
{
	clientPause=false;
	clientInPacketPause=false;


	m_originalQuantInterval=round((float)NORMAL_QUANT_INTERVAL/gameSpeed);
	m_quantInterval=m_originalQuantInterval;


	xassert( md.gameType() == GAME_TYPE_MULTIPLAYER || md.gameType() == GAME_TYPE_MULTIPLAYER_COOPERATIVE);

	hostMissionDescription=md; //Argument PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST
	m_GameName = gameName;
	internalConnectPlayerData.set(playerName);
	//internalPlayerData.setCompAndUserID(cname, username);
	ExecuteInternalCommand(PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST, true);

	//GameSpy тоже нужен hostMissionDescription и GameName
	if(gameSpyInterface)gameSpyInterface->CreateStagingRoom(gameName, password);
	return;
}

void PNetCenter::JoinGame(GUID _gameHostID, const char* playerName, Race race, unsigned int color)
{
	LogMsg("Join 2 server on ID-\n");
	clientPause=false;
	clientInPacketPause=false;

	//Argument  PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST
	internalConnectPlayerData.set(playerName);
	m_gameHostID=_gameHostID;
	ExecuteInternalCommand(PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST, false/*true*/);
}

void PNetCenter::JoinGame(const char* strIP, const char* playerName, Race race, unsigned int color, const char* password)
{
	LogMsg("Join 2 server on IP-\n");
	clientPause=false;
	clientInPacketPause=false;

	unsigned long ip=inet_addr(strIP);
	if(ip!=INADDR_NONE) 
		internalIP_DP=ip;
	else {
		LogMsg("invalid IP\n");
		//gameShell->callBack_JoinGameReturnCode(GameShell::JG_RC_CONNECTION_ERR);
		gameShell->networkMessageHandler(NetRC_JoinGame_Connection_Err);
		return;// JGRC_ERR_CONNECTION;
	}
	//GameSpy
	if(gameSpyInterface){
		GameSpyInterface::e_JoinStagingRoomResult result=gameSpyInterface->JoinStagingRoom(ip, password);
		switch (result){
		case GameSpyInterface::JSRR_Ok:
			LogMsg("GameSpy ok\n");
			break;
		case GameSpyInterface::JSRR_PasswordError:
			LogMsg("GameSpy password error\n");
			//gameShell->callBack_JoinGameReturnCode(GameShell::JG_RC_GAMESPY_PASSWORD_ERR);
			gameShell->networkMessageHandler(NetRC_JoinGame_GameSpyPassword_Err);
			return; break;
		default: //GameSpyInterface::JSRR_Error:
			LogMsg("GameSpy error\n");
			//gameShell->callBack_JoinGameReturnCode(GameShell::JG_RC_GAMESPY_CONNECTION_ERR);
			gameShell->networkMessageHandler(NetRC_JoinGame_GameSpyConnection_Err);
			return; break;
		}
	}


	//Argument  PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST
	internalConnectPlayerData.set(playerName);
	//internalPlayerData.setCompAndUserID(cname, username);
	//internalIP установлен ранее
	ExecuteInternalCommand(PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST, false /*true*/);
}

// first thread function !
void PNetCenter::SendEvent(const NetCommandBase* event)
{
	SendEventSync(event);
	///out_buffer.send(m_connection);
}
void PNetCenter::SendEventSync(const NetCommandBase* event)
{
	MTAuto lock(m_GeneralLock);

	if(isHost()){
		in_HostBuf.putNetCommand(event);
	}
	else if(isClient()){ //Client
		out_ClientBuf.putNetCommand(event);
	}
}


//First thread !


void PNetCenter::th1_HandlerInputNetCommand()
{
	while(in_ClientBuf.currentNetCommandID()!=NETCOM_ID_NONE) {
		NCEventID event = (NCEventID)in_ClientBuf.currentNetCommandID();

		lastTimeServerPacket_th1 = networkTime_th1;
		switch(event){
		case NETCOM_4C_ID_REQUEST_LAST_QUANTS_COMMANDS:
			{
				netCommand4C_RequestLastQuantsCommands nc(in_ClientBuf);

				if(universeX()){
					//По идее вызов корректный т.к. reJoin не пошлется пока игра не остановлена(stopGame_HostMigrate)
					universeX()->sendListGameCommand2Host(nc.beginQunat_);
				}
			}
			break;
		case NETCOM_4C_ID_JOIN_RESPONSE:
			{

				netCommand4C_JoinResponse ncjrs(in_ClientBuf);
				xassert(isDemonWareMode());
				if(!isDemonWareMode()) break;

				//sReplyConnectInfo& hostReplyConnectInfo = ncjrs.replyConnectInfo;
				//if(hostReplyConnectInfo.checkOwnCorrect()){
				//	if( hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_OK )
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_Ok);
				//	else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_VERSION)
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameNotEqualVersion_Err);
				//	else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_STARTED)
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsRun_Err);
				//	else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_FULL)
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsFull_Err);
				//	else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD)
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameSpyPassword_Err);
				//	else 
				//		ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
				//}
			}
			break;
		case NETCOM_4C_ID_START_LOAD_GAME:
			{
				//GameSpy
				if(gameSpyInterface){
					if(isHost())
						gameSpyInterface->StartGame();
				}
				netCommand4C_StartLoadGame nc4c_sl(in_ClientBuf);
				//clientMissionDescription=nc4c_sl.missionDescription_;
				clientMissionDescription.updateFromNet(nc4c_sl.missionDescription_);

				flag_StartedLoadGame = true;
				gameShell->startMissionSuspended(clientMissionDescription);
			}
			break;
		case NETCOM_4C_ID_PAUSE:
			{
				netCommand4C_Pause ncp(in_ClientBuf);
				if(ncp.pause){
					string s;
					for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
						if(ncp.usersIdxArr[i]!=netCommand4C_Pause::NOT_PLAYER_IDX) {
							clientMissionDescription.getPlayerName(ncp.usersIdxArr[i], s);
						}
					}
					vector<string> playerList;
					playerList.push_back(s);
					gameShell->showConnectFailedInGame(playerList);
					clientPause=true;
					clientInPacketPause=false;
				}
				else {
					gameShell->hideConnectFailedInGame();
					clientPause=false;
				}

			}
			break;
		case NETCOM_4C_ID_CUR_MISSION_DESCRIPTION_INFO: 
			{
				netCommand4C_CurrentMissionDescriptionInfo ncCMD(in_ClientBuf);
				//curMD=ncCMD.missionDescription_;
				curMD.updateFromNet(ncCMD.missionDescription_);
				// !!!
				UI_LogicDispatcher::instance().setCurrentMission(curMD);
			}
			break;
		case NETCOM_4C_ID_DISPLAY_DISTRINC_AREAS:
			{
				netCommand4C_DisplayDistrincAreas event(in_ClientBuf);
				vMap.displayChAreas(event.pDAData_, event.DASize_);
			}
			break;
		case NETCOM_4C_ID_CONTINUE_GAME_AFTER_HOST_MIGRATE:
			{
				netCommand4C_ContinueGameAfterHostMigrate nc(in_ClientBuf);
				flag_SkipProcessingGameCommand=0; //Возобновление после миграции Hosta
			}
			break;
		case NETCOM_4C_ID_SEND_LOG_2_HOST:
			{
				// !!! передается HiperSpace
				xassert(flag_SkipProcessingGameCommand);
				if(universeX()) 
					universeX()->ReceiveEvent(event, in_ClientBuf);
			}
			break;
		case NETCOM_4C_ID_ALIFE_PACKET:
			{
				netCommand4C_AlifePacket nc(in_ClientBuf);
			}
			break;
		case NETCOM_4C_ID_CLIENT_IS_NOT_RESPONCE:
			{
				netCommand4C_ClientIsNotResponce nc(in_ClientBuf);
			}
			break;
		case NETCOM_4G_ID_CHAT_MESSAGE:
			{
				netCommand4G_ChatMessage nc_ChatMessage(in_ClientBuf);
				gameShell->addStringToChatWindow(nc_ChatMessage.chatMsg);
			}
			break;
		default: 
			{
				// !!! передается HiperSpace
				if(0) {
					SetConnectionTimeout(30000);
					int c=GetConnectionTimeout();
				}
				if(flag_SkipProcessingGameCommand) in_ClientBuf.ignoreNetCommand(); //Нужно при миграции Host-а
				else {
					if(gameShell->GameActive){
//						if(in_ClientBuf.getQuantAmount()>=1){ //Отсылать на выполнение только ПОЛНОСТЬЮ законченный квант!!!
							if(universeX()->ReceiveEvent(event, in_ClientBuf)==false) {
								in_ClientBuf.backNetCommand();
								goto loc_end_quant;
							}
//						}
					}
					else {
						in_ClientBuf.ignoreNetCommand();
					}
				}
			}
			break;
		}
		in_ClientBuf.nextNetCommand();
	}
loc_end_quant:
	;
}

void PNetCenter::quant_th1()
{
	//in_buffer.receive(m_connection, XDP_DPNID_PLAYER_GENERAL);

	if(isDemonWareMode()){
	}
	networkTime_th1=xclock();

	//test REMOVE!!!
	//vector<DWInterfaceSimple::ChatMemberInfo> chml;
	//demonware()->getChatMembers(chml);
	//static int timeprevList=0;
	//if(networkTime_th1 > timeprevList + 20000){
	//	timeprevList = networkTime_th1;
	//	vector<DWInterfaceSimple::ChatMemberInfo>::iterator p;
	//	for(p=chml.begin(); p!=chml.end(); p++){
	//		LogMsg("-user-%s\n", p->name.c_str());
	//	}
	//}


	sPNCInterfaceCommand curInterfaceCommand(NetMessageCode_NULL);
	{
		MTAuto p_Lock(m_GeneralLock);
		if(!interfaceCommandList.empty()){
			////Сейчас сделано так, что нельзя поместить команду если другая выполняется
			curInterfaceCommand=*interfaceCommandList.begin();
			interfaceCommandList.pop_front();
		}

		if(isDemonWareMode()){
		}
		else if(gameSpyInterface) gameSpyInterface->quant();
		else refreshLanGameHostList_th1();

		th1_HandlerInputNetCommand();
	}

//	void playerDisconnected(string& playerName, bool disconnectOrExit);

	switch(curInterfaceCommand.nmc){
	case NetMessageCode_NULL:
		break;
	case NetRC_Init_Ok:
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_Init_Err:
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_CreateGame_Ok:
		LogMsg("NetRC_CreateGame_Ok\n");
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_CreateGame_CreateHost_Err:
		LogMsg("NetRC_CreateGame_CreateHost_Err\n");
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetMsg_PlayerDisconnected:
		LogMsg("player disconnected:%s\n", curInterfaceCommand.textInfo.c_str());
		gameShell->playerDisconnected(curInterfaceCommand.textInfo, true);
		break;
	case NetMsg_PlayerExit:
		LogMsg("player exit:%s\n", curInterfaceCommand.textInfo.c_str());
		gameShell->playerDisconnected(curInterfaceCommand.textInfo, false);
		break;
	case NetGEC_ConnectionFailed:
		LogMsg("connection failed\n");
		//gameShell->abnormalNetCenterTermination();
		ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetGEC_HostTerminatedSession:
		LogMsg("connection dropped\n");
		//gameShell->abortLobby(GameShell::CLIENT_DROPPED);
		ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	//case NetGEC_HostTerminatedSession:
	//	LogMsg("host terminated game\n");
	//	gameShell->networkMessageHandler(NetGEC_HostTerminatedSession);
	//	ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
	//	break;
	case NetRC_JoinGame_Ok:
		LogMsg("-joined game Ok\n");
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_JoinGame_GameNotEqualVersion_Err:
		LogMsg("-joined game Err(Incorrect Version)\n");
		ResetAndStartFindHost();
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_JoinGame_GameIsRun_Err:
		LogMsg("-joined game Err(game alredy started)\n");
		ResetAndStartFindHost();
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_JoinGame_GameIsFull_Err:
		LogMsg("-joined game Err(game is full)\n");
		ResetAndStartFindHost();
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_JoinGame_GameSpyPassword_Err:
		LogMsg("-joined game Err(incorrect password)\n");
		ResetAndStartFindHost();
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	case NetRC_JoinGame_Connection_Err:
		LogMsg("-joined game Err(connection error)\n");
		ResetAndStartFindHost();
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;

	case NetGEC_GameDesynchronized:
		//xassert(0&& "Host stoping, game ending");
		LogMsg("Desynchronized. Critical error, game terminated!\n");
		ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		::MessageBox(0, "Unique!!!; outnet.log saved", "Error network synchronization", MB_OK|MB_ICONERROR);
		break;

	case NetGEC_GeneralError:
		LogMsg("General error, game terminated!\n");
		ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
		gameShell->networkMessageHandler(curInterfaceCommand.nmc);
		break;
	}

	/// Pause
	if( (!clientPause) && nCState_th2!=PNC_STATE__NONE){
		if( gameShell->GameActive /*&& (!isHost())*/ ){
			if(clientInPacketPause){
				if(networkTime_th1 <= lastTimeServerPacket_th1+TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION){
					gameShell->hideConnectFailedInGame();
					clientInPacketPause=false;
				}
			}
			else if(networkTime_th1 > lastTimeServerPacket_th1+TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION){
				string s;
				clientMissionDescription.getAllOtherPlayerName(s);
				vector<string> playerList;
				playerList.push_back(s);
				gameShell->showConnectFailedInGame(playerList);
				clientInPacketPause=true;
			}
		}
	}

}


//first thread !

void PNetCenter::JoinCommand(int commandID) //cooperative only!
{
	int idxUsersData=curMD.activeUserIdx_;
	netCommand4H_Join2Command nc_J2C(idxUsersData, commandID);
	SendEvent(&nc_J2C);

}
void PNetCenter::KickInCommand(int commandID, int cooperativeIdx) //cooperative only!
{
	netCommand4H_KickInCommand nc_KInC(commandID, cooperativeIdx);
	SendEvent(&nc_KInC);
}

void PNetCenter::ServerTimeControl(float scale)
{
	terEventControlServerTime event(scale);
	SendEvent(&event);
}

void PNetCenter::changePlayerRace(int slotID, Race newRace)
{
	netCommand4H_ChangePlayerRace nc_ChPB(slotID, newRace);
	SendEvent(&nc_ChPB);
}

void PNetCenter::changePlayerColor(int slotID, int newColorIndex)
{
	netCommand4H_ChangePlayerColor nc_ChPC(slotID, newColorIndex);
	SendEvent(&nc_ChPC);
}

void PNetCenter::changePlayerSign(int slotID, int newSign)
{
	netCommand4H_ChangePlayerSign nc_ChPS(slotID, newSign);
	SendEvent(&nc_ChPS);
}

void PNetCenter::changeRealPlayerType(int slotID, RealPlayerType newRealPlayerType)
{
	netCommand4H_ChangeRealPlayerType nc_ChRPT(slotID, newRealPlayerType);
	SendEvent(&nc_ChRPT);
}

void PNetCenter::changePlayerDifficulty(int slotID, Difficulty difficulty)
{
	netCommand4H_ChangePlayerDifficulty nc_ChD(slotID, difficulty);
	SendEvent(&nc_ChD);
}
void PNetCenter::changePlayerClan(int slotID, int clan)
{
	netCommand4H_ChangePlayerClan nc_ChC(slotID, clan);
	SendEvent(&nc_ChC);
}

void PNetCenter::chatMessage(const class ChatMessage& chatMsg)
{
	if(getState()==NSTATE__FIND_HOST && workMode==PNCWM_ONLINE_DW){
	}
	else {
		netCommand4G_ChatMessage nc_ChatMessage(chatMsg);
		SendEvent(&nc_ChatMessage);
	}
}

void PNetCenter::changeMissionDescription(MissionDescriptionNet::eChangedMDVal val, int v)
{
	netCommand4H_ChangeMD nc_chMD(val, v);
	SendEvent(&nc_chMD);
}



//void PNetCenter::pauseQuant(bool flag_gameIsGo)
//{
//
//}

bool PNetCenter::setPause(bool pause)
{
	if(flag_StartedLoadGame){
		if(pause){
			netCommand4H_RequestPause nc_rp(clientMissionDescription.activePlayerID(), true);
			SendEvent(&nc_rp);
		}
		else {
			netCommand4H_RequestPause nc_rp(clientMissionDescription.activePlayerID(), false);
			SendEvent(&nc_rp);
		}
		return 1;
	}
	return 0;
}



bool PNetCenter::StartLoadTheGame()
{
	netCommand4H_StartLoadGame ncslg;
//	if(isConnected()) {
		SendEventSync(&ncslg);
//	}
	return 1;
}

void PNetCenter::GameIsReady(void)
{
	netCommandC_PlayerReady event2(vMap.getWorldCRC());
	SendEventSync(&event2);
	LogMsg("gameLoadComplete\n");
}


void PNetCenter::setGameHostFilter(int gameTypeFilter, std::vector<XGUID> missionFilter, int maxPlayersFilter)
{
	xassert(getState()==NSTATE__FIND_HOST);
	if(getState()!=NSTATE__FIND_HOST) return;
	//gameHostFilter.setNS(gameTypeFilter, missionFilter, maxPlayersFilter, sGameHostFilter::NO_FILTER);
	startGameParam.setNS(gameTypeFilter, missionFilter, maxPlayersFilter, StartGameParamBase::NO_FILTER);

	//if(isDemonWareMode())
	//	demonware()->setGameHostFilter(gameHostFilter);
	//else { //not реализованна
	//}
}

void  PNetCenter::getGameHostList(vector<sGameHostInfo>& ghl)
{
	if(gameSpyInterface){
		ghl=gameSpyInterface->gameHostList;
	}
	else if(isDemonWareMode()){
	}
	else //LAN
        ghl=gameHostListDP;
}


static const GUID zeroGUID= { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

void PNetCenter::refreshLanGameHostList_th1()
{
	//clearGameHostList();
	gameHostListDP.clear();
	char txtBufHostName[MAX_PATH];
	char txtBufGameName[MAX_PATH];
	char txtBufPort[20];
	//char textBuffer[MAX_PATH];
	int curTime = networkTime_th1;
	vector<string>::iterator k;
	for(k=needHostList.begin(); k!=needHostList.end(); k++){
		GUID curguid=zeroGUID;
		curguid.Data1=distance(needHostList.begin(), k);
		gameHostListDP.push_back(sGameHostInfo( curguid, k->c_str(), "", "", sGameStatusInfo()));
	}
	vector<INTERNAL_HOST_ENUM_INFO*>::iterator p;
	for(p=internalFoundHostList.begin(); p!=internalFoundHostList.end();){
		if((curTime - (*p)->timeLastRespond) > MAX_TIME_INTERVAL_HOST_RESPOND ) {
			delete *p;
			p=internalFoundHostList.erase(p);
		}
		else {
			IDirectPlay8Address*  pa=(*p)->pHostAddr;
			HRESULT hResult;

/*			DWORD sizeTxtBuf=MAX_PATH;
			WCHAR txtBuf[MAX_PATH];
			DWORD sizeTxtBuf2=MAX_PATH;
			char txtBuf2[MAX_PATH];
			DWORD data;

			DWORD numComponent;
			hResult = pa->GetNumComponents(&numComponent);
			int i;
			const TCHAR * arr;
			for(i=0; i<numComponent; i++){
				DWORD sizeTxtBuf=MAX_PATH;
				DWORD sizeTxtBuf2=MAX_PATH;
				hResult=pa->GetComponentByIndex(i, txtBuf, &sizeTxtBuf, txtBuf2, &sizeTxtBuf2, &data);
				arr=DXGetErrorString9(hResult);
			}*/

			char * pBuf;
			DWORD bufSize;
			DWORD dataType;
			pBuf=NULL; bufSize=0;
			hResult=pa->GetComponentByName(DPNA_KEY_HOSTNAME, pBuf, &bufSize, &dataType);
			xassert(hResult==DPNERR_BUFFERTOOSMALL);
			pBuf=new char[bufSize];
			hResult=pa->GetComponentByName(DPNA_KEY_HOSTNAME, pBuf, &bufSize, &dataType);
			if( FAILED(hResult) ){
				DXTRACE_ERR_MSGBOX( TEXT("PNetCenter::refreshLanGameHostList_th1-GetComponentByName"), hResult );
			}
			xassert(dataType==DPNA_DATATYPE_STRING);
			int nResult = WideCharToMultiByte( CP_ACP, 0, (WCHAR*)pBuf, -1, txtBufHostName, MAX_PATH, NULL, NULL );
			txtBufHostName[MAX_PATH-1]=0;
			delete pBuf;

			pBuf=NULL; bufSize=0;
			hResult=pa->GetComponentByName(DPNA_KEY_PORT, pBuf, &bufSize, &dataType);
			xassert(hResult==DPNERR_BUFFERTOOSMALL);
			pBuf=new char[bufSize];
			hResult=pa->GetComponentByName(DPNA_KEY_PORT, pBuf, &bufSize, &dataType);
			xassert(dataType==DPNA_DATATYPE_DWORD);
			if( FAILED(hResult) ){
				DXTRACE_ERR_MSGBOX( TEXT("PNetCenter::refreshLanGameHostList_th1-GetComponentByName"), hResult );
			}
			DWORD port=*((DWORD*)pBuf);
			delete pBuf;

			itoa(port,txtBufPort, 10);
			
			nResult = WideCharToMultiByte( CP_ACP, 0, (*p)->pAppDesc->pwszSessionName, -1, txtBufGameName, MAX_PATH, NULL, NULL );
			txtBufGameName[MAX_PATH-1]=0;

			//поиск среди первых необходимых hostName-ов
			//vector<sGameHostInfo*>::iterator m;
			int m;
			for(m=0; m<needHostList.size(); m++){
				unsigned long ipSrc=inet_addr(gameHostListDP[m].hostName.c_str());
				if(ipSrc==INADDR_NONE) continue;
				unsigned long ip2=inet_addr(txtBufHostName);
				if(ip2==INADDR_NONE) continue;
				if(ipSrc==ip2) break;
			}
			if(m!=needHostList.size()){ //адрес нашелся - m индекс
				gameHostListDP[m].set( (*p)->pAppDesc->guidInstance, txtBufHostName, txtBufPort, txtBufGameName, (*p)->gameStatusInfo);
			}
			else { //вставляем
				gameHostListDP.push_back(sGameHostInfo( (*p)->pAppDesc->guidInstance, txtBufHostName, txtBufPort, txtBufGameName, (*p)->gameStatusInfo));//sGameStatusInfo(4,1, false, 10, 1)
			}

			p++;
		}
	}
}




/////////////////////////////////////////////////////////////////
void PNetCenter::FinishGame(void)
{
	if(universeX())
		universeX()->stopMultiPlayer();
	ExecuteInternalCommand(PNC_COMMAND__END_GAME, true);
}

void PNetCenter::ResetAndStartFindHost(void)
{
	if(universeX())
		universeX()->stopMultiPlayer();

	ExecuteInternalCommand(NCmd_Parking, true);
	resetAllVariable_th1();
	ExecuteInternalCommand(PNCCmd_Reset2FindHost, true); 
	//сброс прошел !
}
void PNetCenter::resetAllVariable_th1()
{
	in_ClientBuf.reset();
	out_ClientBuf.reset();
	in_HostBuf.reset();
	out_HostBuf.reset();

	m_hostUNID.setEmpty();
	m_localUNID.setEmpty();

	//internalCommandList.clear()
	//interfaceCommandList.clear()
	hostMissionDescription.clearAllUsersData();
	clientMissionDescription.clearAllUsersData();
	//internalConnectPlayerData //нет необходимости - имеет смысл только в пределах комманды создания игры
	m_GameName.clear(); //нет необходимости 
	flag_StartedLoadGame = false;
	//flag_StartedGame=false;
	ClearCommandList(); //нет необходимости
	ClearDeletePlayerGameCommand(); //нет необходимости
	//m_gameHostID=0;

	deleteUsersSuspended.clear();

	flag_SkipProcessingGameCommand=0;
	flag_LockIputPacket=0;

	curMD.clearAllUsersData();
	m_originalQuantInterval=m_quantInterval=NORMAL_QUANT_INTERVAL;
	//beginWaitTime_th2=0;

	networkTime_th1 = xclock();
	lastTimeServerPacket_th1 = networkTime_th1;

	internalIP_DP=0;
	flag_connectedDP=0;
	//clearInternalFoundHostList();
	needHostList.clear();

	m_numberGameQuant = 1; //нет необходимости - сбрасываеться при загрузке
	m_nQuantCommandCounter = 0; //нет необходимости - сбрасываеться при загрузке
	hostGeneralCommandCounter=0;
	quantConfirmation=netCommandNextQuant::NOT_QUANT_CONFIRMATION;
	unidClientWhichWeWait.setEmpty(); //нет необходимости 

	gamePassword="";

	hostPause=0;
	clientPause=0;
	clientInPacketPause=0;

	//gameHostFilter=sGameHostFilter(); //reset filters
	currentExecutionInternalCommand = NCmd_Null;
}

// !!!!!!!!!!!!!!! Вызывает много вопросов !!!!!!!!!!!!!!!!!!!
// ONLY GAME SPY!
#define IP1(x) (x & 0xff)
#define IP2(x) ((x>>8) & 0xff)
#define IP3(x) ((x>>16) & 0xff)
#define IP4(x) ((x>>24) & 0xff)
void PNetCenter::reEnumPlayers(int ip)
{
	MTAuto _Lock(m_GeneralLock); //! Lock 

	StopFindHostDP();
	char ip_string[17];
	memset(ip_string, 0, sizeof(ip_string));
	sprintf(ip_string, "%d.%d.%d.%d", IP1(ip), IP2(ip), IP3(ip), IP4(ip));

	{
		clearInternalFoundHostList();
	}
	StartFindHostDP(ip_string);
}

///////////////////////////////////

int QSStateAndCondition::addPlayers(ConnectPlayerData _connectPlayerData, const UNetID& unid)
{
	__int64 lowFilterMap = _connectPlayerData.lowFilterMap;
	__int64 highFilterMap= _connectPlayerData.highFilterMap;
	int cntUser=0;
	for(int i=0; i<MAX_QSUSERS; i++) 
		if(qsUserState[i].flag_userConnected) cntUser++;
	if( cntUser >= (int) gameOrder ) // подразумевается ==
		return USER_IDX_NONE;
	for(int i=0; i<MAX_QSUSERS; i++){
		QSUserState& qsus = qsUserState[i];
		if(qsus.flag_userConnected){
			lowFilterMap &= qsus.connectPlayerData.lowFilterMap;
			highFilterMap &= qsus.connectPlayerData.highFilterMap;
		}
	}
	if(lowFilterMap == 0 && highFilterMap==0 )
		return USER_IDX_NONE;
	for(int i=0; i<MAX_QSUSERS; i++){
		QSUserState& qsus = qsUserState[i];
		if(!qsus.flag_userConnected){
			qsus.flag_userConnected=true;
			qsus.unid=unid;
			qsus.connectPlayerData=_connectPlayerData;
			return i;
		}
	}
	return USER_IDX_NONE;
}
bool QSStateAndCondition::removeUser(const UNetID& unid)
{
	int cnt=0;
	for(int i=0; i<MAX_QSUSERS; i++) 
		if(qsUserState[i].flag_userConnected && unid==qsUserState[i].unid){
			qsUserState[i].flag_userConnected = false;
			cnt++;
		}

	xassert(cnt==1);
	return (cnt>0);
}
bool QSStateAndCondition::isConditionRealized()
{
	int cntUser=0;
	for(int i=0; i<MAX_QSUSERS; i++) 
		if(qsUserState[i].flag_userConnected) cntUser++;
	if( cntUser == (int) gameOrder )
		return true;
	return false;
}

__int64 QSStateAndCondition::getTotalFilterMap()
{
	__int64 outLowFilterMap = 0xFFffFFffFFffFFff;
	for(int i=0; i<MAX_QSUSERS; i++){
		QSUserState& qsus = qsUserState[i];
		if(qsus.flag_userConnected){
			outLowFilterMap &= qsus.connectPlayerData.lowFilterMap;
		}
	}
	return outLowFilterMap;
}
