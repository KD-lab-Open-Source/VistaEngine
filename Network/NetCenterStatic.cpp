#include "StdAfx.h"
#include "P2P_interface.h"
#include "UniverseX.h"
#include "UI_Logic.h"
#include "UI_NetCenter.h"

#include "Lmcons.h"

PNetCenter* PNetCenter::netCenter=0;
bool PNetCenter::stopNetCenterSuspended=false;

void PNetCenter::createNetCenter(ExternalNetTask_Init* entInit) //static
{ 
	destroyNetCenter(); 
	netCenter = new PNetCenter(entInit); 
}

void PNetCenter::stopNetCenter() 
{ 
	stopNetCenterSuspended = true;	
	if(universeX()) universeX()->stopNetCenter(); 
}

void PNetCenter::netQuant() //static
{ 
	start_timer_auto();
	UI_LogicDispatcher::instance().getNetCenter().quant();
	if(netCenter) { 
		netCenter->quant_th1(); 
		if(stopNetCenterSuspended) destroyNetCenter(); 
	} 
}

void PNetCenter::destroyNetCenter() //static
{ 
	delete netCenter; 
	netCenter=0; 
	stopNetCenterSuspended = false; 
}

///////////////////////////////////////////////////////////////
// Command line start
void PNetCenter::startMPWithoutInterface(const char* missionName)
{
	const int BUF_CN_SIZE=MAX_COMPUTERNAME_LENGTH + 1;
	DWORD cns = BUF_CN_SIZE;
	char cname[BUF_CN_SIZE];
	::GetComputerName(cname, &cns);
	const int BUF_UN_SIZE=UNLEN + 1;
	DWORD uns = BUF_UN_SIZE;
	char username[BUF_UN_SIZE];
	::GetUserName(username, &uns);

	static ExternalNetTask_Init etInit;
	if(check_command_line(COMMAND_LINE_MULTIPLAYER_HOST)){
		int numPlayers=atoi(check_command_line(COMMAND_LINE_MULTIPLAYER_HOST));
		etInit.setup(PNCWM_LAN);
		createNetCenter(&etInit);
		string playerName=string("Host_")+cname+"_"+username;
		//instance()->CreateGame("HostGame", MissionDescription(missionName, GAME_TYPE_MULTIPLAYER), playerName.c_str(), Race(), 0, 1, "");
		static ENTGame entGame;
		MissionDescription md(missionName, GAME_TYPE_MULTIPLAYER);
		entGame.setupCreateGame("HostGame", &md, playerName.c_str(), Race());
		instance()->implementingENT(&entGame);
		while(instance()->getCurrentMissionDescription().playersAmount() < numPlayers){
			PNetCenter::netQuant();
			::Sleep(40);
		}
		instance()->plyaerIsReadyOrStartLoadGame();
	}
	else if(check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT)){
		string playerName=string("Client_")+cname+"_"+username;
		string ipstr=check_command_line(COMMAND_LINE_MULTIPLAYER_CLIENT);
		etInit.setup(PNCWM_LAN);
		createNetCenter(&etInit);
		static ENTGame entGame;
		if(ipstr==""){
			instance()->ResetAndStartFindHost();
			vector<sGameHostInfo> hl;
			instance()->getGameHostList(hl);
			do{
				PNetCenter::netQuant();
				instance()->getGameHostList(hl);
			}while(hl.size()==0);
			//instance()->JoinGame(hl.begin()->gameHostGUID, playerName.c_str(), Race(), 1);
			entGame.setupJoinGame(hl.begin()->gameHostGUID, playerName.c_str(), Race());
		}
		else {
			//instance()->JoinGame(ipstr.c_str(), playerName.c_str(), Race(), 1, "");
		}
		instance()->implementingENT(&entGame);
		while(instance()->getCurrentMissionDescription().playersAmount() < 1){ //Hint-овая проверка на то, что подключились
			PNetCenter::netQuant();
			::Sleep(40);
		}
		instance()->plyaerIsReadyOrStartLoadGame();
	}
}
void PNetCenter::startDWMPWithoutInterface(const char* missionName)
{
	const char* playerName=check_command_line("playerName");
	xassert(playerName);
	const char* strPassword=check_command_line("password");
	if(strPassword==0) strPassword="";
	static ExternalNetTask_Init etInit;
	etInit.setup(PNCWM_ONLINE_DW);
	createNetCenter(&etInit);
	static ENTLogin entLogin;
	entLogin.setup(playerName, strPassword);
	instance()->implementingENT(&entLogin);
	if(check_command_line(COMMAND_LINE_DW_HOST)){
		int numPlayers=atoi(check_command_line(COMMAND_LINE_DW_HOST));
		//instance()->CreateGame("HostGame", MissionDescription(missionName, GAME_TYPE_MULTIPLAYER), playerName, Race(), 0, 1, strPassword);
		static ENTGame entGame;
		MissionDescription md(missionName, GAME_TYPE_MULTIPLAYER);
		entGame.setupCreateGame("HostGame", &md, playerName, Race());
		instance()->implementingENT(&entGame);
		while(instance()->getCurrentMissionDescription().playersAmount() < numPlayers){
			PNetCenter::netQuant();
			::Sleep(40);
		}
		instance()->plyaerIsReadyOrStartLoadGame();

	}
	else if(check_command_line(COMMAND_LINE_DW_CLIENT)){
		string ipstr=check_command_line(COMMAND_LINE_DW_CLIENT);
		instance()->ResetAndStartFindHost();
		//vector<sGameHostInfo*>& hl=getNetClient()->getGameHostList();
		//do{
		//	getNetClient()->quant_th1();
		//	hl=getNetClient()->getGameHostList();
		//}while(hl.size()==0);
		//getNetClient()->JoinGame((*hl.begin())->gameHostGUID, playerName, Race(), 1);

		static ENTGame entGame;
		if(ipstr==""){
			instance()->ResetAndStartFindHost();
			vector<sGameHostInfo> hl;
			instance()->getGameHostList(hl);
			do{
				PNetCenter::netQuant();
				instance()->getGameHostList(hl);
			}while(hl.size()==0);
			//instance()->JoinGame(hl.begin()->gameHostGUID, playerName, Race(), 1);
			entGame.setupJoinGame(hl.begin()->gameHostGUID, playerName, Race());
		}
		else {
			//instance()->JoinGame(ipstr.c_str(), playerName, Race(), 1, "");
		}
		instance()->implementingENT(&entGame);
		while(instance()->getCurrentMissionDescription().playersAmount() < 1){ //Hint-овая проверка на то, что подключились
			PNetCenter::netQuant();
			::Sleep(40);
		}
		instance()->plyaerIsReadyOrStartLoadGame();
	}
	else {
		xassert(0);;
	}
}
