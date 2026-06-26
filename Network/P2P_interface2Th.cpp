#include "StdAfx.h"

#include "Runtime.h"
#include "P2P_interface.h"
#include "GS_interface.h"

#include "GameShell.h"
#include "UniverseX.h"
#include "UI_Logic.h"

#include "Lmcons.h"

#include "Terra/vmap.h"

#include <algorithm>

#include "QSWorldsMgr.h"

#include "LogMsg.h"

void XDPInit();
void XDPClose();

const unsigned int MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST=30000;//10sec
const unsigned int PERIMETER_DEFAULT_PORT = 0x6501;

///////////////////////////////////////////////////////////

void PNetCenter::th2_clearInOutClientHostBuffers()
{
	in_ClientBuf.reset();
	out_ClientBuf.reset();

	in_HostBuf.reset();
	out_HostBuf.reset();
}

//Second thread
DWORD WINAPI InternalServerThread(LPVOID lpParameter)
{
	PNetCenter* pPNetCenter=(PNetCenter*)lpParameter;
	pPNetCenter->SecondThread_th2();
	return 0;
}
XBuffer BUFFER_LOG(10000,1);

void PNetCenter::internalCommandEnd_th2( bool flag_result )
{
	if(currentExecutionInternalCommand == NCmd_Null){
		LogMsg("End internal EmptyCommand!!!\n");
		xassert("End internal EmptyCommand!!!");
	}
	if( currentExecutionInternalCommand.isFlagWaitExecuted() )
		SetEvent(hCommandExecuted);
	LogMsg("@command<%s> Wait:%u -completed result-%u\n", getStrInternalCommand(currentExecutionInternalCommand), currentExecutionInternalCommand.isFlagWaitExecuted(), flag_result);
	currentExecutionInternalCommand = NCmd_Null;
}

bool PNetCenter::internalCommandQuant_th2()
{
	bool flag_end=0;
	{//LOCK
		MTAuto _lock(m_GeneralLock);
		if(internalCommandList.empty())
			return flag_end;

		///???? ���������� ���� ������� ������������ ��� Reset & end !
		if(currentExecutionInternalCommand != NCmd_Null){
			LogMsg("@Double command ! \n");
			internalCommandEnd_th2( false );
		}

		currentExecutionInternalCommand=*internalCommandList.begin();
		internalCommandList.pop_front();
	}
	LogMsg("@command<%s> Wait:%u-start\n", getStrInternalCommand(currentExecutionInternalCommand), currentExecutionInternalCommand.isFlagWaitExecuted() );
	if(nCState_th2 == NSTATE__PARKING || nCState_th2 == PNC_STATE__NONE) {
		//� �������� & None(��� ��������� ����������� � �������� ����� ����������) ���������� ��� ����� Reset 
		if(currentExecutionInternalCommand!=PNCCmd_Reset2FindHost && currentExecutionInternalCommand!=PNC_COMMAND__END){
			//SetEvent(hCommandExecuted);
			internalCommandEnd_th2( false );
			return flag_end;
		}
	}
	switch(currentExecutionInternalCommand.cmd()){
	case NCmd_Parking:
		nCState_th2 = NSTATE__PARKING;
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	case PNCCmd_Reset2FindHost:
		{
			//flag_StartedLoadGame = false;
			if(isDemonWareMode());
			else {
				if(isConnectedDP()) {
					Close();
					InitDP();//Close DirectPlay-� ��������� ������ ���������������
				}
				StartFindHostDP();
			}
			nCState_th2 = NSTATE__FIND_HOST;
		}
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	//case PNC_COMMAND__STOP_GAME_AND_ASSIGN_HOST_2_MY:
	//	{
	//		m_hostUNID=m_localUNID;
	//		nCState_th2=PNC_STATE__NEWHOST_PHASE_0;
	//	}
	//	//SetEvent(hCommandExecuted);
	//	internalCommandEnd_th2();
	//	break;
	//case PNC_COMMAND__STOP_GAME_AND_WAIT_ASSIGN_OTHER_HOST:
	//	{
	//		if(isDemonWareMode())
	//		nCState_th2=PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0;
	//	}
	//	//SetEvent(hCommandExecuted);
	//	internalCommandEnd_th2();
	//	break;
	case PNC_COMMAND__STOP_GAME_AND_MIGRATION_HOST:
		{
			if(isDemonWareMode()){
			}
			else {} //DP ��� ���������
			if( m_hostUNID == m_localUNID ) // Host �
				nCState_th2=PNC_STATE__NEWHOST_PHASE_0;
			else 
				nCState_th2=PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0;
			internalCommandEnd_th2();
		}
		break;
	case PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST:
		{
			flag_LockIputPacket=0;
			flag_SkipProcessingGameCommand=0;
			m_DPPacketList.clear();
			th2_clearInOutClientHostBuffers();
			flag_StartedLoadGame = false;
			//flag_StartedGame = false;

			nCState_th2=NSTATE__HOST_TUNING_GAME; //���������� ��� DPN_MSGID_ENUM_HOSTS_QUERY ���� ����� ������� ���������� ����

			hostMissionDescription.clearAllUsersData();//������ClearClients();
			if(isDemonWareMode()){
			}
			else { //DirectPlay
				StopFindHostDP();
				LogMsg("DP starting server...");
				if(!isConnectedDP())
					ServerStart(m_GameName.c_str(), PERIMETER_DEFAULT_PORT);//
				LogMsg("started\n");
				if(isConnectedDP())
					runCompletedExtTask_Ok(extNetTask_Game); //ExecuteInterfaceCommand_thA(NetRC_CreateGame_Ok);
				else 
					finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::CreateGameErr); //ExecuteInterfaceCommand_thA(NetRC_CreateGame_CreateHost_Err);
				if(AddClient(internalConnectPlayerData, m_localUNID)==USER_IDX_NONE){
					//ExecuteInterfaceCommand_thA(NetRC_CreateGame_CreateHost_Err);
					finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::CreateGameErr);
					//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
					nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
				}
			}
			LogMsg("New game <%s> worldSaveName=%s. for start...\n", m_GameName.c_str(), hostMissionDescription.interfaceName());
			hostMissionDescription.clearAllPlayerGameLoaded();
			hostMissionDescription.setChanged();
		}
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	case PNC_COMMAND__CLIENT_STARTING_LOAD_GAME:
		ClearDeletePlayerGameCommand();
		nCState_th2=PNC_STATE__CLIENT_LOADING_GAME;
		internalCommandEnd_th2();
		break;
	case PNC_COMMAND__CLIENT_STARTING_GAME:
		nCState_th2=PNC_STATE__CLIENT_GAME;
		internalCommandEnd_th2();
		break;
	case PNC_COMMAND__END:
		{
			flag_end=true;
		}
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	case PNC_COMMAND__ABORT_PROGRAM:
		{
			nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
		}
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	case NCmd_QuickStart:
		{
			if(isDemonWareMode()){
			}
			else {
				xassert(0&&"Quick Start mode - DemonWare only");;
			}

		}
		internalCommandEnd_th2();
		break;
	//������ ��������������� ������� 
	// ��������������� ��������
	case PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST:
		{
			flag_LockIputPacket=0;
			flag_SkipProcessingGameCommand=0;
			m_DPPacketList.clear();
			th2_clearInOutClientHostBuffers();
			flag_StartedLoadGame = false;
			//flag_StartedGame = false;
			if(nCState_th2==NSTATE__FIND_HOST){
				if(gameSpyInterface && internalIP_DP){
					StartConnect2IP(internalIP_DP);
					nCState_th2=PNC_STATE__INFINITE_CONNECTION_2_IP; 
				}
				else if(isDemonWareMode()){
				}
				else {
					xassert(0);
					nCState_th2=PNC_STATE__CONNECTION;
				}
			}
			else {
				xassert(0&&"Connecting: command order error(not find host state)");
				nCState_th2 = NSTATE__PARKING;
				internalCommandEnd_th2( false );
				//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
				finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameConnectionErr);
			}
		}
		break;
	// ��������������� ��������
	case PNC_COMMAND__END_GAME:
		{
			nCState_th2=PNC_STATE__ENDING_GAME;
			flag_StartedLoadGame = false;
			//flag_StartedGame = false;
			//if(isConnected()) {
				if(isHost()){
					//��������������� ������� ���������� ������
					netCommandNextQuant com(m_numberGameQuant, 0, 0, hostGeneralCommandCounter, 0);
					SendEventI(com, UNetID::UNID_ALL_PLAYERS, true);
				}
			//}
		}
		break;
	default:
		xassert("Unknown internal command !");
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	}

	return flag_end;
}

//log data
static int MAX_QUANT_TIME_DELTA=0;
static int MIN_QUANT_TIME_DELTA=0;
static double SUM_TIME_DELTA=0;
static unsigned int QUANT_CNT=0;
bool PNetCenter::SecondThread_th2()
{
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	networkTime_th2 = xclock();

	if(!isDemonWareMode()){
		XDPInit();
		InitDP();
		StartFindHostDP();
	}
	nCState_th2 = NSTATE__FIND_HOST;

	if(isDemonWareMode());
	//������������� ��������� - XDPConnection ������
	SetEvent(hSecondThreadInitComplete);

	bool initresult=true;
	if(workMode==PNCWM_LAN_DW);
	else if(workMode==PNCWM_ONLINE_DW);
	else if(workMode==PNCWM_ONLINE_P2P);
	//ExecuteInterfaceCommand_thA( initresult==true ? NetRC_Init_Ok : NetRC_Init_Err );
	if(initresult) finitExtTask_Ok(extNetTask_Init); //extNetTaskInit->setOk();
	else finitExtTask_Err(extNetTask_Init); //extNetTaskInit->setErr();

	bool flag_end=0;
	while(!flag_end){
		networkTime_th2 = xclock();

		if(nCState_th2 != NSTATE__PARKING && isDemonWareMode()){
		}

		MTAuto* _pLock=new MTAuto(m_GeneralLock);
		//decoding command 
		flag_end = internalCommandQuant_th2();

		if(nCState_th2 != NSTATE__PARKING) { //��� ��������� ; � ��� ��� �������������
			if(nCState_th2&PNC_State_QuickStart)
                quickStartReceiveQuant_th2();
			else if(isHost())
				th2_HostReceiveQuant();
			else if(isClient()){//Client receive quant!
				th2_ClientPredReceiveQuant();
			}
		}

		delete _pLock;

		if(flag_end) break; //��� �������� ������

		//Logic quant
		networkTime_th2 = xclock();
		if(nextQuantTime_th2==0) nextQuantTime_th2 = networkTime_th2; //tracking start game
		const int TIME_MISTAKE=0;//ms //1
		bool flag_dwEndQuantRunned=false;
		if(nextQuantTime_th2 - TIME_MISTAKE <= networkTime_th2) {
			//if(nCState_th2==NSTATE__HOST_GAME){
			//	int td=curTime-nextQuantTime_th2;
			//	MAX_QUANT_TIME_DELTA=max(td,MAX_QUANT_TIME_DELTA);
			//	MIN_QUANT_TIME_DELTA=min(td,MIN_QUANT_TIME_DELTA);
			//	SUM_TIME_DELTA+=(double)abs(td);
			//	QUANT_CNT++;
			//}
			th2_LLogicQuant();
			if(nCState_th2 != NSTATE__PARKING && isDemonWareMode()){
			}
			deleteUserQuant_th2();
			nextQuantTime_th2+=m_quantInterval;
		}
		if(!flag_dwEndQuantRunned && nCState_th2 != NSTATE__PARKING && isDemonWareMode()){
		}
		networkTime_th2 = xclock();
		if(nextQuantTime_th2 - TIME_MISTAKE > networkTime_th2){
			Sleep(nextQuantTime_th2 - TIME_MISTAKE - networkTime_th2);
		}
		//end logic quant
	}

	if(isDemonWareMode());

	if(!isDemonWareMode()){
		StopFindHostDP();
		SetConnectionTimeout(1);//��� �������� ����������
		//if(m_pConnection->Connected()) m_pConnection->Close();
		Close();
		XDPClose();
	}

	return 0;
}

void PNetCenter::sendStartLoadGame2AllPlayers_th2(const XBuffer& auxdata)
{
	//hostMissionDescription.packPlayerIDs();
	//Disconnect all not joined 2 command players
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(hostMissionDescription.usersData[i].flag_userConnected){
			if(hostMissionDescription.findSlotIdx(i)==PLAYER_ID_NONE){
				UNetID delPlayerUNID=hostMissionDescription.usersData[i].unid;
				//hostMissionDescription.disconnectUser(i);
				//RemovePlayer(delPlayerUNID); //������ �������� �� DPN_MSGID_DESTROY_PLAYER
				discardUser_th2(delPlayerUNID);
			}
		}
	}

	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(hostMissionDescription.usersData[i].flag_userConnected){
			hostMissionDescription.setActivePlayerIdx(i);
			netCommand4C_StartLoadGame nccsl(hostMissionDescription, auxdata);
			SendEventI(nccsl, hostMissionDescription.usersData[i].unid);
		}
	}
	hostMissionDescription.setActivePlayerIdx(hostMissionDescription.findUserIdx(m_hostUNID));
	LogMsg("Sent battle info\n");
}

void PNetCenter::th2_UpdateCurrentMissionDescription4C()
{
	for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
		if(hostMissionDescription.usersData[i].flag_userConnected){
			hostMissionDescription.setActivePlayerIdx(i);
			netCommand4C_CurrentMissionDescriptionInfo nccmd(hostMissionDescription);
			SendEventI(nccmd, hostMissionDescription.usersData[i].unid);
		}
	}
	hostMissionDescription.setActivePlayerIdx(hostMissionDescription.findUserIdx(m_hostUNID));
}
void PNetCenter::th2_CheckClients()
{
	int j;
	for(j=0; j<NETWORK_PLAYERS_MAX; j++){
		UserData& ud=hostMissionDescription.usersData[j];
		if(ud.flag_userConnected){
			if(!ud.flag_playerGameLoaded){
				LogMsg("Client 0x%X() is not ready. removing.\n", ud.unid.dpnid());
				//RemovePlayer(ud.unid);
				discardUser_th2(ud.unid);
				ud.flag_userConnected=false;
				ud.unid.setEmpty(); //�������������
			}
		}
	}
	
}

void PNetCenter::resetAllClients_th2()
{
	unsigned int curTime = networkTime_th2;
	int j;
	for(j=0; j<NETWORK_PLAYERS_MAX; j++){
		UserData& ud=hostMissionDescription.usersData[j];
		if(ud.flag_userConnected){
			ud.flag_playerGameLoaded=0;
			ud.lastTimeBackPacket=curTime;//���������� ��� ����������� ���������� ��������
			ud.backGameInf2List.clear();
		}
	}
}

void PNetCenter::th2_DumpClients()
{
	LogMsg("Dumping clients---------------------------\n");
	int j;
	for(j=0; j<NETWORK_PLAYERS_MAX; j++){
		UserData& ud=hostMissionDescription.usersData[j];
		if(ud.flag_userConnected){
			LogMsg("Client userIdx:%u dpnid:0x%X()\n", j, ud.unid.dpnid());
		}
	}
	LogMsg("End of clients---------------------------\n");
}

bool PNetCenter::th2_AddClientToMigratedHost(const UNetID& _unid, unsigned int _curLastQuant, unsigned int _confirmQuant)
{
	//����� ����-�� ����� ������
	int j;
	for(j=0; j<NETWORK_PLAYERS_MAX; j++){
		UserData& ud=hostMissionDescription.usersData[j];
		if(ud.flag_userConnected){
			if(ud.unid == _unid && ud.flag_playerStartReady==1) return 0; //�������� �� ����������
		}
	}


	int idxPlayerData=hostMissionDescription.findUserIdx(_unid);
	if(idxPlayerData!=-1){
		hostMissionDescription.setChanged();
		UserData& ud=hostMissionDescription.usersData[idxPlayerData];
		ud.curLastQuant=_curLastQuant;
		ud.confirmQuant=_confirmQuant;
		ud.flag_playerStartReady=1;
		LogMsg("ReJoin client 0x%X for game %s; curLastQuant:%u confirmQuant:%u\n", _unid.dpnid(), m_GameName.c_str(), ud.curLastQuant, ud.confirmQuant);
		return 1;
	}
	else {
		LogMsg("client 0x%X for game %s id denied\n", _unid.dpnid(), m_GameName.c_str());
		return 0;
	}
}


void PNetCenter::SendEventI(const NetCommandBase& event, const UNetID& unid, bool flag_guaranted)
{
	//if(isHost()){
		if(!(unid==m_localUNID)){
			out_HostBuf.putNetCommand(&event);
			out_HostBuf.send(*this, unid, flag_guaranted);
		}
		if( (unid==m_localUNID) || (unid==UNetID::UNID_ALL_PLAYERS/*m_dpnidGroupGame*/) ){
			in_ClientBuf.putNetCommand(&event);
		}
	//}

}

void PNetCenter::putNetCommand2InClientBuf_th2(NetCommandBase& event)
{
	MTAuto _lock(m_GeneralLock);
	in_ClientBuf.putNetCommand(&event);
}


void PNetCenter::ClearCommandList()
{
	list<NetCommandBase*>::iterator m;
	for(m=m_CommandList.begin(); m!=m_CommandList.end(); m++)
		delete *m;
	m_CommandList.clear();
}

void PNetCenter::th2_SaveLogByDesynchronization(vector<BackGameInformation2>& firstList, vector<BackGameInformation2>& secondList)
{
	if( (*firstList.begin()).quant_ != (*secondList.begin()).quant_ ) {
		XStream f("outnet.log", XS_OUT);
		f.write(BUFFER_LOG.buffer(), BUFFER_LOG.tell());
		f < currentVersion < "\r\n";
		f < "Number quants is not equal !!!" < "\n";
		vector<BackGameInformation2>::iterator q;
		for(q=firstList.begin(); q!=firstList.end(); q++){
			f < "HostQuant=" <= (*q).quant_ < " " <= (*q).replay_ < " " <= (*q).state_< "\n";
		}
		for(q=secondList.begin(); q!=secondList.end(); q++){
			f < "ClientQuant=" <= (*q).quant_ < " " <= (*q).replay_ < " " <= (*q).state_ < "\n";
		}
		f.close();

		XBuffer to(1024,1);
		to < "Number quants is not equal !!!" < "N1=" <= (*firstList.begin()).quant_ < " N2=" <=(*secondList.begin()).quant_;
		::MessageBox(0, to, "Error network synchronization", MB_OK|MB_ICONERROR);
		LogMsg("%s", (const char*)to);
		//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
	}
	else {
		// ��������� ��� netCommand4H_BackGameInformation2
		if( (*firstList.begin()).signature_ != (*secondList.begin()).signature_ ){
			SendEventI(netCommand4C_SaveLog((*firstList.begin()).quant_), UNetID::UNID_ALL_PLAYERS);
			XBuffer to(1024,1);
			XStream f("outnet.log", XS_OUT);
			f < currentVersion < "\r\n";
			f < "Unmatched game quants !" < " on Quant=" <= (*firstList.begin()).quant_;
			f.close();
			LogMsg("Unmatched game quants =%u; send netCommand4C_SaveLog\n" , (*firstList.begin()).quant_);

			////!to < "Unmatched game quants !" < " on Quant=" <= (*firstList.begin()).quant_;
			////!::MessageBox(0, to, "Error network synchronization", MB_OK|MB_ICONERROR);
			//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
		}
	}
	///else xassert(0 && "�������������� ���������������");
}



void PNetCenter::th2_LLogicQuant()
{
	//LogMsg("q=%d\n", m_nQuantDelay + nDbgServerLag + int(nDbgServerLagNoise*float(rand())/RAND_MAX));

	//�� ���������� ����� !
	//CAutoLock* _pLock=new CAutoLock(&m_GeneralLock);
	//delete _pLock;


	switch(nCState_th2) {
	case NSTATE__PARKING:
		break;
	case NSTATE__FIND_HOST:
		break;
	//////////////////////////////////////////////
	//Quick Start
	case NSTATE__QSTART_NON_CONNECT:
		break;
	case NSTATE__QSTART_HOSTING_CLIENTING:
		break;
	case NSTATE__QSTART_HOST:
		break;
	case NSTATE__QSTART_CLIENT:
		xassert(workMode==PNCWM_ONLINE_DW);

		break;
	//////////////////////////////////////////////
	// Simple start game
	case PNC_STATE__CONNECTION:
		if(isDemonWareMode()){
		}
		else {
			nCState_th2=PNC_STATE__CLIENT_TUNING_GAME;
			if(gameSpyInterface){ //Internet
				if(!internalIP_DP)
					internalIP_DP=gameSpyInterface->getHostIP(m_gameHostID);
			}
			if(internalIP_DP) {
				if(!Connect(internalIP_DP)) {
					nCState_th2 = NSTATE__PARKING;
					//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
					finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameConnectionErr);
				}
			}
			else { // IP==0
				if(gameSpyInterface){
					nCState_th2 = NSTATE__PARKING;
					//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
					finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameConnectionErr);
				}
				else if(!Connect(m_gameHostID)) {
					nCState_th2 = NSTATE__PARKING;
					//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
					finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameConnectionErr);
				}

			}
			StopFindHostDP();
			SetEvent(hCommandExecuted);
			internalCommandEnd_th2();
		}
		//SetConnectionTimeout(TIMEOUT_DISCONNECT);//30s//3600000
		break;
	case PNC_STATE__INFINITE_CONNECTION_2_IP:
		if(QuantConnect2IP()){
			StopFindHostDP();
			nCState_th2=PNC_STATE__CLIENT_TUNING_GAME;
			//SetEvent(hCommandExecuted);
			internalCommandEnd_th2();
		}
		break;
	case NSTATE__HOST_TUNING_GAME:
		{
			MTAuto _Lock(m_GeneralLock); //! Lock

			m_numberGameQuant= 1;//!

			if(hostMissionDescription.isUsersAmountChanged())
				hostMissionDescription.clearAllPlayerStartReady();
			if(isDemonWareMode() ){ //workMode==PNCWM_ONLINE_DW
			}
			if(hostMissionDescription.isChanged()){
				th2_UpdateCurrentMissionDescription4C();
				hostMissionDescription.clearChanged();
				//hostMissionDescription.clearAllPlayerStartReady();
			}
			if(hostMissionDescription.isAllRealPlayerStartReady()){

				LogMsg("Game Started\n");

				//LockAllPlayers
				XBuffer auxDWData;
				//if(isDemonWareMode())
				if(isDemonWareMode());
				sendStartLoadGame2AllPlayers_th2(auxDWData);

				//ReleaseAllPlayers
				LogMsg("Wait for all clients ready. \n");

				//��� �������� ������� �������� ��� ��������
				ClearDeletePlayerGameCommand();

				nCState_th2=NSTATE__HOST_LOADING_GAME;
			}
		}
		break;
	case NSTATE__HOST_LOADING_GAME:
		{
			MTAuto _Lock(m_GeneralLock); //! Lock

			bool flag_ready=1;
			bool flag_playerPresent=false; 
			unsigned int lastPlayerGameCRC=0;
			UNetID lastPlayerUNID;
			int j;
			for(j=0; j<NETWORK_PLAYERS_MAX; j++){
				UserData& ud=hostMissionDescription.usersData[j];
				if(ud.flag_userConnected){
					xassert(!ud.unid.isEmpty());
					flag_ready&=ud.flag_playerGameLoaded;
					if(flag_ready){
						if(flag_playerPresent && lastPlayerGameCRC!=ud.clientGameCRC){
							XBuffer buf;
							buf < "Game of the player " <= ud.unid.dpnid() < "does not agree to game of the player " <= lastPlayerUNID.dpnid();
							xxassert(0 , buf);
						}
						flag_playerPresent=true;
						lastPlayerGameCRC=ud.clientGameCRC;
						lastPlayerUNID=ud.unid;
					}
				}
			}
			if(flag_ready){
				th2_CheckClients();
				th2_DumpClients();

				LogMsg("Go! go! go!\n");

				resetAllClients_th2();
				nCState_th2=NSTATE__HOST_GAME;

				//Init game counter
				ClearCommandList();

				hostGeneralCommandCounter=0;
				quantConfirmation=netCommandNextQuant::NOT_QUANT_CONFIRMATION;
				m_nQuantCommandCounter=0;
				m_numberGameQuant= 1;//!
				hostPause=0;

			}
		}
		break;
	case NSTATE__HOST_GAME: 
		{
			MTAuto _Lock(m_GeneralLock); //! Lock

			int idxHost=hostMissionDescription.findUserIdx(m_hostUNID);
			xassert(idxHost!=-1);
			if(idxHost==-1) break;
			vector<BackGameInformation2> & firstList=hostMissionDescription.usersData[idxHost].backGameInf2List;

			while(!firstList.empty()) { //�������� ��� ������ ������ �� ������
				unsigned int countCompare=0;
				int k;
				for(k=0; k<NETWORK_PLAYERS_MAX; k++){
					UserData& ud=hostMissionDescription.usersData[k];
					if(!ud.flag_userConnected || ud.unid==m_hostUNID) continue; //������� ������ ������ � ����� 
					vector<BackGameInformation2>& secondList = ud.backGameInf2List;
					if(secondList.empty()) goto end_while_01;//���������� while ���� ���� �� ������� �������� ������ �������
					//if(secondList.empty()) continue; //������������ ��� ������!

					if( *firstList.begin() == *secondList.begin() ) countCompare++;
					else {
						th2_SaveLogByDesynchronization(firstList, secondList);
						//ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
						//finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::GeneralError);
						//finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::GameDesynchronized);
						nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
						return;
					}
				}
				if(countCompare+1>=hostMissionDescription.amountUsersConnected()) {
					quantConfirmation=(*firstList.begin()).quant_;
					//erase begin elements
					for(k=0; k<NETWORK_PLAYERS_MAX; k++){
						UserData& ud=hostMissionDescription.usersData[k];
						if(ud.flag_userConnected){
							vector<BackGameInformation2>& curList = ud.backGameInf2List;
							BUFFER_LOG <= (*curList.begin()).quant_ < " " <= (*curList.begin()).replay_ < " " <= (*curList.begin()).state_< "\n";
							curList.erase(curList.begin());
						}

					}
				}
			}

end_while_01:;
			string notResponceClientList;
			unsigned int maxInternalLagQuant=0;
			unsigned int maxAccessibleLogicQuantPeriod=0;
			unsigned int minClientExecutionQuant=m_numberGameQuant;
			int k;
			for(k=0; k<NETWORK_PLAYERS_MAX; k++){
				UserData& ud=hostMissionDescription.usersData[k];
				if(ud.flag_userConnected ){
					if(ud.lagQuant > maxInternalLagQuant) {//��� �������� ���� ��������
						maxInternalLagQuant=ud.lagQuant; 
					}
					if(ud.accessibleLogicQuantPeriod > maxAccessibleLogicQuantPeriod) {//��� �������� ���� ��������
						maxAccessibleLogicQuantPeriod=ud.accessibleLogicQuantPeriod; 
					}
					if(ud.lastExecuteQuant < minClientExecutionQuant) {//��� �������� ���� ��������
						minClientExecutionQuant=ud.lastExecuteQuant;
					}
					if(networkTime_th2 > (ud.lastTimeBackPacket + TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION)){
						UNetID d=ud.unid;
						int n=hostMissionDescription.findUserIdx(d);
						if(n!=-1) { notResponceClientList+=hostMissionDescription.usersData[n].playerNameE; notResponceClientList+=' '; }
					}
				}
			}

			statistics_add(maxAccessibleLogicQuantPeriod, maxAccessibleLogicQuantPeriod);

			/// �������� ��� ���� �������� !
			const unsigned int MAX_EXTERNAL_LAG_QUANT_WAIT = 8*3;
			const unsigned int MIN_LOGICAL_TIME_RESERV = 10;
			const unsigned int maxExternalLagQuant = m_numberGameQuant - minClientExecutionQuant;
			if(maxExternalLagQuant > MAX_EXTERNAL_LAG_QUANT_WAIT)
				m_quantInterval = min(m_quantInterval*(maxExternalLagQuant/2), 500);
			else if(flag_DecreaseSpeedOnSlowestPlayer){
				if(maxAccessibleLogicQuantPeriod + MIN_LOGICAL_TIME_RESERV > m_originalQuantInterval)
					m_quantInterval = min(maxAccessibleLogicQuantPeriod + MIN_LOGICAL_TIME_RESERV, 500);
				else 
					m_quantInterval = m_originalQuantInterval;
			}

			statistics_add(maxExternalLagQuant, maxExternalLagQuant);
			statistics_add(m_quantInterval, m_quantInterval);

			//Pause handler
			int usersIdxArr[NETWORK_PLAYERS_MAX];
			for(int m=0; m<NETWORK_PLAYERS_MAX; m++) usersIdxArr[m]=netCommand4C_Pause::NOT_PLAYER_IDX; //clear
			bool flag_requestPause=0;
			bool flag_changePause=0;
			int curPlayer=0;

			for(k=0; k<NETWORK_PLAYERS_MAX; k++){
				UserData& ud=hostMissionDescription.usersData[k];
				if(ud.flag_userConnected ){
					flag_requestPause|=ud.requestPause;
					flag_changePause|=(ud.requestPause!=ud.clientPause);
					ud.clientPause=ud.requestPause;
					if(ud.requestPause){
						xassert(curPlayer<NETWORK_PLAYERS_MAX);
						if(curPlayer<NETWORK_PLAYERS_MAX){
							usersIdxArr[curPlayer++]=k;
						}
					}
				}
			}
			for(k=0; k<NETWORK_PLAYERS_MAX; k++){
				UserData& ud=hostMissionDescription.usersData[k];
				if(ud.flag_userConnected ){
					if(ud.requestPause && ((networkTime_th2-ud.timeRequestPause) > MAX_TIME_PAUSE_GAME) ) {
						//RemovePlayer(ud.unid); //������ �������� �� DPN_MSGID_DESTROY_PLAYER
						discardUser_th2(ud.unid);
						break;//�� ������ ������� �� �����!
					}
				}
			}

			if(hostPause && (!flag_requestPause) ){
				//������ �����
				netCommand4C_Pause ncp(usersIdxArr, false);
				SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);
				hostPause=false;
			}
			else if(flag_changePause && flag_requestPause){
				netCommand4C_Pause ncp(usersIdxArr, true);
				SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);
				hostPause=true;
			}
			if(hostPause)
				break;

			//����������� ���� ������ �������� � ������ ������� �� ����������
			list<netCommand4G_ForcedDefeat*>::iterator p;
			for(p=m_DeletePlayerCommand.begin(); p!=m_DeletePlayerCommand.end(); p++){
				PutGameCommand2Queue_andAutoDelete(*p);
				LogMsg("ForcedDefeat for UserID-%u\n", (*p)->userID);
			}
			m_DeletePlayerCommand.clear();

			//��������� ��������� �������, ��������, ���������
			if(!m_CommandList.empty()){
				list<NetCommandBase*>::iterator p=m_CommandList.end();
				p--; //��������� �������
				if((*p)->isGameCommand()){
					(static_cast<netCommandGame*>(*p))->setFlagLastCommandInQuant();
				}
				else xassert("Error in commands list on server");
			}

			///hostGeneralCommandCounter+=m_CommandList.size(); //������ ����������� ��� PutGameCommand2Queue_andAutoDelete


			if(0){
				static bool flag_timeOut=0; //���� ��������� � ���� ������!!!
				if(!flag_timeOut) {
					if(!notResponceClientList.empty()){
						netCommandNextQuant* pcom=new netCommandNextQuant(m_numberGameQuant, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, quantConfirmation, true);
						m_CommandList.push_back(pcom);

						netCommand4C_ClientIsNotResponce* pnr=new netCommand4C_ClientIsNotResponce(notResponceClientList);
						m_CommandList.push_back(pnr);
						flag_timeOut=1;
					}
					else {
						netCommandNextQuant* pcom=new netCommandNextQuant(m_numberGameQuant, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, quantConfirmation);
						m_CommandList.push_back(pcom);
					}
				}
				else {
					if(!notResponceClientList.empty()){
						netCommand4C_ClientIsNotResponce* pnr=new netCommand4C_ClientIsNotResponce(notResponceClientList);
						m_CommandList.push_back(pnr);
					}
					else {
						netCommandNextQuant* pcom=new netCommandNextQuant(m_numberGameQuant, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, quantConfirmation);
						m_CommandList.push_back(pcom);
					}
				}
			}
			else {
				//��� ������ ��� ���
				netCommandNextQuant* pcom=new netCommandNextQuant(m_numberGameQuant, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, quantConfirmation);
				m_CommandList.push_back(pcom);
			}

			list<NetCommandBase*>::iterator i;
			FOR_EACH(m_CommandList, i) {
				if((**i).EventID==NETCOM4G_NextQuant || (**i).EventID==NETCOM4C_ClientIsNotResponce)
					SendEventI(**i, UNetID::UNID_ALL_PLAYERS, false);//�� ��������������� ��������!!!
				else 
					SendEventI(**i, UNetID::UNID_ALL_PLAYERS);
				delete *i;
			}
			m_CommandList.clear();
			m_numberGameQuant++;
		}
		break;
	case PNC_STATE__CLIENT_TUNING_GAME: 
		break;
	case PNC_STATE__CLIENT_LOADING_GAME:
		break;
	case PNC_STATE__CLIENT_GAME:
		{
		}
		break;
	case PNC_STATE__NEWHOST_PHASE_0:
		{
			//�������� ��������� ���� ������� �����
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				if(universeX()){
					if(in_ClientBuf.currentNetCommandID()!=NETCOM_None) break;
					if(universeX()->allowedRealizingQuant > universeX()->lastRealizedQuant) break;
					//�� ���� ��������� �������� �.�. ������� ������� ���������� ������ ��� ��������� ������, �� ����� ����������
					universeX()->stopGame_HostMigrate();//������� ���� ������ �������� ������
				}
				flag_SkipProcessingGameCommand=1;
				//�� ���� ������ ���� �.�. ������ ���������� th2_ClientPredReceiveQuant
				m_DPPacketList.clear();
			}

			//��������������� ������� �������, ��������������� ��� ����������� ��������� � ����� Host-�
			//���������� ��� ����, ����� �� ���������� ��������� � reJoin-�
			UnLockInputPacket();

			//����������� ���� �� ������������� ������ ?
			///////CAutoLock lock(&m_ClientInOutBuffLock);

			
			//�.�. �������� ����������� ������ ����� START_LOAD_GAME clientMissionDescription ���������
			hostMissionDescription=clientMissionDescription;
			//Update state MD 
			if(isDemonWareMode()){
			}
			//hostMissionDescription.clearAllUsersData(); //������ ClearClientData();
			hostMissionDescription.clearAllPlayerStartReady();

			beginWaitTime_th2 = networkTime_th2;

			//������� InHostBuffer � ���� ������ ��� �� �� �������������!
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				th2_clearInOutClientHostBuffers();
			}
			if(th2_AddClientToMigratedHost(m_localUNID, universeX()->getCurrentGameQuant(), universeX()->getConfirmQuant()) ){
				nCState_th2=PNC_STATE__NEWHOST_PHASE_A;
			}
			else {
				LogMsg("Migration Host - th2_AddClientToMigratedHost Error!\n");
				//ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::GeneralError);
				//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
				nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
			}

		}
		break;
	case PNC_STATE__NEWHOST_PHASE_A:
		{
			MTAuto _Lock(m_GeneralLock); //! Lock

			const unsigned int MAX_TIME_4_HOST_MIGRATE=10000;//10s

			bool result=1;
			if(hostMissionDescription.isAllRealPlayerStartReady()){
				result=1;
			}
			else {
				result=0;
				if((networkTime_th2 - beginWaitTime_th2) < MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST){
				}
				else { //��� ����� �����
					LogMsg("PNC_STATE__NEWHOST_PHASE_A - Time has expired!\n");
					if(isDemonWareMode()){
					}
				}

			}
			
			if( result==1 ) { //���� ��� ������������ � ������ ������
				unsigned int maxConfirmedQuant=0;
				///unsigned int maxQuant=0;
				///unsigned int minQuant=UINT_MAX;
				///DPNID maxQuantClientDPNID=0;
				int k;
				for(k=0; k<NETWORK_PLAYERS_MAX; k++){
					UserData& ud=hostMissionDescription.usersData[k];
					if(ud.flag_userConnected ){
						if(ud.confirmQuant > maxConfirmedQuant)
							maxConfirmedQuant=ud.confirmQuant;
						/*if(ud.curLastQuant >= maxQuant){
							maxQuant=ud.curLastQuant;
							maxQuantClientDPNID=ud.dpnidPlayer;
						}
						if(ud.curLastQuant <= minQuant){
							minQuant=ud.curLastQuant;
						}*/
					}
				}

				for(int j=0; j<NETWORK_PLAYERS_MAX; j++){
					UserData& ud=hostMissionDescription.usersData[j];
					if(ud.flag_userConnected){
						int fq = (ud.backGameInf2List.begin()!=ud.backGameInf2List.end()) ? ud.backGameInf2List.begin()->quant_ : 0;
						LogMsg("backGameInf2List user:%s size:%u firstQuant:%u\n", ud.playerNameE, ud.backGameInf2List.size(), fq);
					}
				}
				resetAllClients_th2();

				netCommand4C_sendLog2Host ncslh(maxConfirmedQuant+1);
				SendEventI(ncslh, UNetID::UNID_ALL_PLAYERS);
				LogMsg("New Host request log from %u quant\n", maxConfirmedQuant+1);

				///minQuant+=1; //������ ���������� ������ �� ��� �������
				///netCommand4C_RequestLastQuantsCommands nc(minQuant);
				///SendEventI(nc, maxQuantClientDPNID);
				unidClientWhichWeWait.setEmpty();//maxQuantClientDPNID;


				beginWaitTime_th2 = networkTime_th2;
				nCState_th2=PNC_STATE__NEWHOST_PHASE_B;
			}
		}
		break;
	case PNC_STATE__NEWHOST_PHASE_B:
		{
			//if( ((networkTime_th2 - beginWaitTime_th2) > MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST) ) {
			//}
			if( (unidClientWhichWeWait.isEmpty()) || (hostMissionDescription.findUserIdx(unidClientWhichWeWait)==-1) ){ //����� ����� �� ������� ������ ����� ������� �������
				unsigned int maxQuant=0;
				unsigned int minQuant=UINT_MAX;
				UNetID maxQuantClientUNID;
				int k;
				for(k=0; k<NETWORK_PLAYERS_MAX; k++){
					UserData& ud=hostMissionDescription.usersData[k];
					if(ud.flag_userConnected ){
						if(ud.curLastQuant >= maxQuant){
							maxQuant=ud.curLastQuant;
							maxQuantClientUNID=ud.unid;
						}
						if(ud.curLastQuant <= minQuant)
							minQuant=ud.curLastQuant;
					}
				}
				minQuant+=1; //������ ���������� ������ �� ��� �������
				netCommand4C_RequestLastQuantsCommands nc(minQuant);
				SendEventI(nc, maxQuantClientUNID);
				unidClientWhichWeWait=maxQuantClientUNID;
			}
		}
		break;
	case PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0:
		{
			//�������� ��������� ���� ������� �����
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				if(universeX()){
					if(in_ClientBuf.currentNetCommandID()!=NETCOM_None) break;
					if(universeX()->allowedRealizingQuant > universeX()->lastRealizedQuant) break;
					//�� ���� ��������� �������� �.�. ������� ������� ���������� ������ ��� ��������� ������, �� ����� ����������
					universeX()->stopGame_HostMigrate();//������� ���� ������ �������� ������
				}
				flag_SkipProcessingGameCommand=1;
				//�� ���� ������ ���� �.�. ������ ���������� th2_ClientPredReceiveQuant
				m_DPPacketList.clear();
			}

			//��������������� ������� �������, ��������������� ��� ����������� ��������� � ����� Host-�
			UnLockInputPacket();

			//������� out_ClientBuf � ���� ������ ��� �� �� �������������!
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				th2_clearInOutClientHostBuffers();
			}
			nCState_th2=PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB;
			//ClearClientData();
			//hostMissionDescription.clearAllUsersData(); //������ ClearClientData();
			beginWaitTime_th2 = networkTime_th2;
		}
		break;
	case PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB:
		{
			MTAuto _Lock(m_GeneralLock); //! Lock ��� senda

			static unsigned char bandPassFilter=0;
			if((bandPassFilter&0x7)==0){//������ 8� �����
				netCommand4H_ReJoinRequest ncrjr(universeX()->getCurrentGameQuant(), universeX()->getConfirmQuant() );
				SendEventI(ncrjr, m_hostUNID);
				///nCState_th2=PNC_STATE__CLIENT_GAME;
			}
			bandPassFilter++;

			if((networkTime_th2 - beginWaitTime_th2) < MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST){
			}
			else { //��� ����� �����
				LogMsg("There is no answer to a command netCommand4H_ReJoinRequest!\n");
				//ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::GeneralError);
				nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
			}
		}
		break;
	case PNC_STATE__NET_CENTER_CRITICAL_ERROR:
		{
			//Close(false);
			//ExecuteInternalCommand(PNC_COMMAND__END, false);
		}
		break;
	case PNC_STATE__ENDING_GAME:
		{
			if(isDemonWareMode()){
			}
			else {
				StopFindHostDP();
				Close(false);
				InitDP();//Close DirectPlay-� ��������� ������ ���������������
			}
			MTAuto _Lock(m_GeneralLock); //! Lock
			hostMissionDescription.clearAllUsersData();//������ClearClients();
			nCState_th2=PNC_STATE__NONE;
		}
		//SetEvent(hCommandExecuted);
		internalCommandEnd_th2();
		break;
	default:
		{
		}
		break;
							 
	}
	
}

void PNetCenter::quickStartReceiveQuant_th2()
{
	xassert(isDemonWareMode());
	list<XDPacket>::iterator p=m_DPPacketList.begin();
	while(p!=m_DPPacketList.end()){
		//if(p->unid==m_hostUNID){
		UNetID& unid = p->unid;
		InOutNetComBuffer tmp(2048, true);
		tmp.putBufferPacket(p->buffer, p->size);

		while(tmp.currentNetCommandID()!=NETCOM_None) {
			if(tmp.currentNetCommandID()==NETCOM4C_StartLoadGame){
				netCommand4C_StartLoadGame nc4c_sl(tmp);
				if(nCState_th2 == NSTATE__QSTART_CLIENT){
					nCState_th2 = PNC_STATE__CLIENT_TUNING_GAME;//PNC_STATE__CLIENT_LOADING_GAME;
					//ExecuteInterfaceCommand_thA(NetRC_QuickStart_Ok);
					runCompletedExtTask_Ok(extNetTask_Game);
					return;//!!! //���� th2_ClientPredReceiveQuant ��� �� ��� ��� ����������
				}
			}
			if( tmp.currentNetCommandID()==NETCOM4H_JoinRequest){
					netCommandC_JoinRequest ncjr(tmp);
					xassert(isDemonWareMode());

					sConnectInfo& clientConnectInfo=ncjr.connectInfo;
					if(clientConnectInfo.checkOwnCorrect()) {
						int resultIdx=USER_IDX_NONE;
						static sDigitalGameVersion hostDGV(true);//�������� ������ ����
						static sReplyConnectInfo replyConnectInfo;
						if( nCState_th2 != NSTATE__QSTART_NON_CONNECT 
							&& nCState_th2 != NSTATE__QSTART_HOSTING_CLIENTING 
							&& nCState_th2 != NSTATE__QSTART_HOST )
								replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
						else if( !clientConnectInfo.flag_quickStart )
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
						else if(hostDGV!=clientConnectInfo.dgv){ //����������������� ������ ����
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_VERSION, hostDGV);
						}
						else if( (!gamePassword.empty()) && (!clientConnectInfo.isPasswordCorrect(gamePassword.c_str())) ){
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD, hostDGV);
						}
						//����������� � m_qsStateAndCondition.addPlayers
						else if( clientConnectInfo.gameOrder != m_QSGameOrder)
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV); 
						else {
							resultIdx = m_qsStateAndCondition.addPlayers(clientConnectInfo.perimeterConnectPlayerData, unid);
							if(resultIdx==USER_IDX_NONE){ // ���� ������
								replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_FULL, hostDGV);
								LogMsg("QuickStart - break connection\n");
							}
							else 
								replyConnectInfo.set(sReplyConnectInfo::CR_OK, hostDGV);
						}
						netCommand4C_JoinResponse ncjrs(replyConnectInfo);
						if(replyConnectInfo.connectResult==sReplyConnectInfo::CR_OK){ //������� �� tmpConnection � �������� �������

						}
						else {//�������� ����������
							InOutNetComBuffer tmp(128, true);
							tmp.putNetCommand(&ncjrs);
							//tmp.send(*this, unid, flag_guaranted);
						}
					}
					else { // if(!clientConnectInfo.checkOwnCorrect())
						xassert(0&& "Connect info defected!");;
					}
			}
			else if( tmp.currentNetCommandID()==NETCOM4C_JoinResponse){
					netCommand4C_JoinResponse ncjrs(tmp);
					sReplyConnectInfo& hostReplyConnectInfo = ncjrs.replyConnectInfo;
					if(nCState_th2 != NSTATE__QSTART_HOSTING_CLIENTING && nCState_th2 != NSTATE__QSTART_CLIENT){
					}
					if(hostReplyConnectInfo.checkOwnCorrect()){
						if( hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_OK ){
							//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Ok);
							if(nCState_th2 == NSTATE__QSTART_HOSTING_CLIENTING){
								nCState_th2 = NSTATE__QSTART_CLIENT;
							}
						}
						else {
							nCState_th2 = NSTATE__QSTART_NON_CONNECT;
						}
					}
			}
			else
                tmp.ignoreNetCommand();
			tmp.nextNetCommand();
		}
        p=m_DPPacketList.erase(p);
	}
}

void PNetCenter::th2_ClientPredReceiveQuant()
{
	if(!out_ClientBuf.isEmpty()) out_ClientBuf.send(*this, m_hostUNID);

	if(flag_LockIputPacket) return; //return 0;
	int cnt=0;
	list<XDPacket>::iterator p=m_DPPacketList.begin();
	while(p!=m_DPPacketList.end()){
		//if(p->unid==m_hostUNID){

			//���������������� �������
			InOutNetComBuffer tmp(2048, true);
			tmp.putBufferPacket(p->buffer, p->size);

			while(tmp.currentNetCommandID()!=NETCOM_None) {
				if(tmp.currentNetCommandID()==NETCOM4C_StartLoadGame){
					netCommand4C_StartLoadGame nc4c_sl(tmp);
					if(isDemonWareMode());
					ExecuteInternalCommand(PNC_COMMAND__CLIENT_STARTING_LOAD_GAME, false);
				}
				else if(tmp.currentNetCommandID()==NETCOM4G_NextQuant){
					netCommandNextQuant nc(tmp);
					if(nCState_th2==PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB){
						nCState_th2=PNC_STATE__CLIENT_GAME;
					}
					else if(nCState_th2 == PNC_STATE__CLIENT_LOADING_GAME) {
						if(isDemonWareMode()){
						}
						LogMsg("Game Started!\n");
						nCState_th2=PNC_STATE__CLIENT_GAME;
					}
					//if(!flag_StartedGame){
					//	flag_StartedGame=true;
					//	if(isDemonWareMode()){
					//	}
					//	LogMsg("Game Started!\n");
					//	nCState_th2=PNC_STATE__CLIENT_GAME;
					//}

				}
				else if(tmp.currentNetCommandID()==NETCOM4C_JoinResponse){
					netCommand4C_JoinResponse ncjrs(tmp);
					xassert(isDemonWareMode());
					if(isDemonWareMode()){
					}
				}
				else
                    tmp.ignoreNetCommand();
				tmp.nextNetCommand();
			}
			//�������� �������
			if(in_ClientBuf.putBufferPacket(p->buffer, p->size)){
				p=m_DPPacketList.erase(p);
				cnt++;
			}
			else break;
		//}
		//else {
		//	xassert(0&&"Commands not from a host!");
		//	p=m_DPPacketList.erase(p);
		//}
	}
	///return (cnt!=0);
}

void PNetCenter::th2_HostReceiveQuant()
{
	if(flag_LockIputPacket) return; //return 0;

	UNetID unid=m_localUNID;
	do { //������ ������ ��� ����������� �������
		int k;
		for(k=0; k<NETWORK_PLAYERS_MAX; k++){
			UserData& ud=hostMissionDescription.usersData[k];
			if(!ud.flag_userConnected || unid!=ud.unid) continue;

			while(in_HostBuf.currentNetCommandID()) {
				//netLog <= in_HostBuf.currentNetCommandID() < "\n";
				switch(in_HostBuf.currentNetCommandID()) {
				case NETCOM4H_RequestPause:
					{
						netCommand4H_RequestPause nc_rp(in_HostBuf);
						ud.requestPause=nc_rp.pause;
						ud.timeRequestPause = networkTime_th2;

/*							if( (!(*p)->requestPause) && nc_rp.pause){
							int playersIDArr[NETWORK_PLAYERS_MAX];
							for(int i=0; i<NETWORK_PLAYERS_MAX; i++) playersIDArr[i]=netCommand4C_Pause::NOT_PLAYER_ID;
							playersIDArr[0]=nc_rp.playerID;
							netCommand4C_Pause ncp(playersIDArr, true);
							SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);
							(*p)->requestPause=true;
						}
						else if((*p)->requestPause && (!nc_rp.pause) ){
							int playersIDArr[NETWORK_PLAYERS_MAX];
							for(int i=0; i<NETWORK_PLAYERS_MAX; i++) playersIDArr[i]=netCommand4C_Pause::NOT_PLAYER_ID;
							netCommand4C_Pause ncp(playersIDArr, false);
							SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);
							(*p)->requestPause=false;
						}*/

					}
					break;
				case NETCOM4H_ChangeRealPlayerType:
					{
						netCommand4H_ChangeRealPlayerType ncChRT(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged(true);
						if(unid==m_hostUNID){
							xassert(ncChRT.slotID_ < min(NETWORK_PLAYERS_MAX, hostMissionDescription.playersAmountMax()));
							if( ncChRT.slotID_!=hostMissionDescription.findSlotIdx(hostMissionDescription.findUserIdx(m_hostUNID)) ){//�������� �� ��, ��� �������� �� � Host-�
								if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_PLAYER || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_WORLD || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_EMPTY){
									xassert(0 && "Error change real type!");
									ncChRT.newRealPlayerType_=REAL_PLAYER_TYPE_OPEN; //�������������� ��������
								}
								SlotData& cur_sd=hostMissionDescription.changePlayerData(ncChRT.slotID_);
								if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER) {
									if( ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_AI || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_CLOSE ){
										//������������ ������
										for(int k=0; k<NETWORK_TEAM_MAX; k++){
											int cur_userIdx=cur_sd.usersIdxArr[k];
											if(cur_userIdx!=USER_IDX_NONE){
												UNetID delPlayerUNID=hostMissionDescription.usersData[cur_userIdx].unid;
												//hostMissionDescription.disconnectUser(cur_userIdx);
												//RemovePlayer(delPlayerUNID); //������ �������� �� DPN_MSGID_DESTROY_PLAYER
												discardUser_th2(delPlayerUNID);
											}
										}
										cur_sd.realPlayerType=ncChRT.newRealPlayerType_;
									}
								}
								else if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_AI){ //���� ��� AI
									if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_OPEN || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_CLOSE){
										//��������� AI
										hostMissionDescription.disconnectAI(ncChRT.slotID_);
										cur_sd.realPlayerType=ncChRT.newRealPlayerType_;
									}
								}
								else { //���� ��� Close ��� Open
									if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_AI){ 
										hostMissionDescription.connectAI(ncChRT.slotID_);
										//cur_sd...............
									}
									else if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_OPEN || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_CLOSE){
										//hostMissionDescription.playersData_[ncChRT.idxPlayerData_].realPlayerType=ncChRT.newRealPlayerType_;
										cur_sd.realPlayerType=ncChRT.newRealPlayerType_;
									}
									else{
										xassert(0 && "Invalid hostMissionDescription");
									}
								}
							}
						}
					}
					break;
				case NETCOM4H_ChangePlayerRace:
					{
						netCommand4H_ChangePlayerRace  ncChB(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host ����� ������ � ������
						hostMissionDescription.changePlayerRace(ncChB.slotID_, ncChB.newRace_, unid, changeAbsolutely);
					}
					break;

				case NETCOM4H_ChangePlayerColor:
					{
						netCommand4H_ChangePlayerColor  ncChC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host ����� ������ � ������
						hostMissionDescription.changePlayerColor(ncChC.slotID_, ncChC.newColor_, false, unid, changeAbsolutely);
					}
					break;
				case NETCOM4H_ChangePlayerSign:
					{
						netCommand4H_ChangePlayerSign  ncChS(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host ����� ������ � ������
						hostMissionDescription.changePlayerSign(ncChS.slotID_, ncChS.sign_, unid, changeAbsolutely);
					}
					break;
				case NETCOM4H_ChangePlayerDifficulty:
					{
						netCommand4H_ChangePlayerDifficulty ncChD(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host ����� ������ � ������
						hostMissionDescription.changePlayerDifficulty(ncChD.slotID_, ncChD.difficulty_, unid, changeAbsolutely );
					}
					break;
				case NETCOM4H_ChangePlayerClan:
					{
						netCommand4H_ChangePlayerClan ncChC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host ����� ������ � ������
						hostMissionDescription.changePlayerClan(ncChC.slotID_, ncChC.clan_, unid, changeAbsolutely);
					}
					break;
				case NETCOM4H_ChangeMissionDescription:
					{
						netCommand4H_ChangeMD nc_changeMD(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						if(unid==m_hostUNID){
							hostMissionDescription.setChanged();
							hostMissionDescription.changeMD(nc_changeMD.chv, nc_changeMD.v);
						}
					}
					break;
				case NETCOM4H_Join2Command:
					{
						netCommand4H_Join2Command nc_J2C(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						if(nc_J2C.idxUserData_!=USER_IDX_NONE && hostMissionDescription.findUserIdx(unid)==nc_J2C.idxUserData_)
							hostMissionDescription.join2Command(nc_J2C.idxUserData_, nc_J2C.commandID_);
					}
					break;
				case NETCOM4H_KickInCommand:
					{
						netCommand4H_KickInCommand nc_KInC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						hostMissionDescription.setChanged(true);
						if(unid==m_hostUNID){
							xassert(nc_KInC.commandID_ < min(NETWORK_PLAYERS_MAX, hostMissionDescription.playersAmountMax()));
							if( nc_KInC.commandID_!=hostMissionDescription.findSlotIdx(hostMissionDescription.findUserIdx(m_hostUNID)) ){//�������� �� ��, ��� �������� �� � Host-�
								SlotData& cur_sd=hostMissionDescription.changePlayerData(nc_KInC.commandID_);
								if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER) {
									xassert(nc_KInC.teamIdx_>=0 && nc_KInC.teamIdx_ < NETWORK_TEAM_MAX);
									nc_KInC.teamIdx_=clamp(nc_KInC.teamIdx_, 0, NETWORK_TEAM_MAX);
									int cur_userIdx=cur_sd.usersIdxArr[nc_KInC.teamIdx_];
									if(cur_userIdx!=USER_IDX_NONE){
										UNetID delPlayerUNID=hostMissionDescription.usersData[cur_userIdx].unid;
										//hostMissionDescription.disconnectUser(cur_userIdx);
										//RemovePlayer(delPlayerUNID); //������ �������� �� DPN_MSGID_DESTROY_PLAYER
										discardUser_th2(delPlayerUNID);
									}
								}
							}
						}
					}
					break;
				case NETCOM4H_PlayerIsReadyOrStartLoadGame:
					{
						netCommand4H_PlayerIsReadyOrStartLoadGame ncslg(in_HostBuf);
						hostMissionDescription.setChanged();

						hostMissionDescription.setPlayerStartReady(unid);
						///StartGame();
					}
					break;
				case NETCOM4H_GameIsLoaded:
					{
						netCommandC_PlayerReady event(in_HostBuf);
						hostMissionDescription.setChanged();

						hostMissionDescription.setPlayerGameLoaded(unid, event.gameCRC_);
						LogMsg("Player 0x%X() (GCRC=0x%X) reported ready\n", unid, event.gameCRC_);
					}
					break;

				case NETCOM4G_UnitCommand:
					{
						netCommand4G_UnitCommand * pCommand = new netCommand4G_UnitCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM4G_UnitListCommand:
					{
						netCommand4G_UnitListCommand * pCommand = new netCommand4G_UnitListCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM4G_PlayerCommand:
					{
						netCommand4G_PlayerCommand * pCommand = new netCommand4G_PlayerCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM4G_Event:
					{
						netCommand4G_Event * pCommand = new netCommand4G_Event(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM4H_BackGameInformation:
					{
						netCommand4H_BackGameInformation* pEvent= new netCommand4H_BackGameInformation(in_HostBuf);
						//p->second->backGameInfList.push_back(pEvent);
						delete pEvent;
					}
					break;
				case NETCOM4H_BackGameInformation2:
					{
						netCommand4H_BackGameInformation2 nc(in_HostBuf);
						ud.backGameInf2List.push_back(nc.backGameInf2_);
						ud.lagQuant=nc.backGameInf2_.lagQuant_;
						ud.lastExecuteQuant=nc.backGameInf2_.quant_;
						ud.lastTimeBackPacket = networkTime_th2;
						ud.accessibleLogicQuantPeriod=nc.backGameInf2_.accessibleLogicQuantPeriod_;
						//static XStream tl("!tl.log", XS_OUT);
						//tl < "LQ=" <= nc.lagQuant_ < " Q="<= nc.quant_ < " aLQP="<= nc.accessibleLogicQuantPeriod_ < "\r\n";
					}
					break;
				case NETCOM4H_AlifePacket:
					{
						netCommand4H_AlifePacket nc(in_HostBuf);
						ud.lastTimeBackPacket = networkTime_th2;
					}
					break;
				case NETCOM4H_ResponceLastQuantsCommands:
					{
						//!!!Server!
						xassert(nCState_th2==PNC_STATE__NEWHOST_PHASE_B);
						netCommand4H_ResponceLastQuantsCommands nci(in_HostBuf);
						if(nCState_th2!=PNC_STATE__NEWHOST_PHASE_B) break;

						vector<netCommandGame*> tmpListGameCommands;

						InOutNetComBuffer in_buffer(nci.sizeCommandBuf+1, 1); //��������� ������������� ��������������!
						in_buffer.putBufferPacket(nci.pData, nci.sizeCommandBuf);

						while(in_buffer.currentNetCommandID()!=NETCOM_None) {
							NCEventID event = (NCEventID)in_buffer.currentNetCommandID();
							switch(event){
							case NETCOM4G_UnitCommand: 
								{
									netCommand4G_UnitCommand*  pnc= new netCommand4G_UnitCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM4G_UnitListCommand: 
								{
									netCommand4G_UnitListCommand*  pnc= new netCommand4G_UnitListCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM4G_PlayerCommand: 
								{
									netCommand4G_PlayerCommand*  pnc= new netCommand4G_PlayerCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM4G_ForcedDefeat:
								{
									netCommand4G_ForcedDefeat* pnc=new netCommand4G_ForcedDefeat(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM4G_Event:
								{
									netCommand4G_Event* pnc=new netCommand4G_Event(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							default:
								xassert(0&&"Incorrect commanf in RESPONCE_LAST_QUANTS_COMMANDS!");
								break;
							}
							in_buffer.nextNetCommand();
						}
						//������ �����������
						netCommand4C_ContinueGameAfterHostMigrate ncContinueGame;
						SendEventI(ncContinueGame, UNetID::UNID_ALL_PLAYERS);
						//���������� ������, ������� �� � ���� ���� ���������
						for(m_numberGameQuant=nci.beginQuantCommandTransmit; m_numberGameQuant<=nci.endQuantCommandTransmit; m_numberGameQuant++){
							m_nQuantCommandCounter=0;
							vector<netCommandGame*>::iterator p;
							for(p=tmpListGameCommands.begin(); p!=tmpListGameCommands.end(); p++){
								if((*p)->curCommandQuant_==m_numberGameQuant) {
									SendEventI(**p, UNetID::UNID_ALL_PLAYERS);
									m_nQuantCommandCounter++;
								}
							}
							//netCommandNextQuant netCommandNextQuant(m_numberGameQuant, m_nQuantCommandCounter, netCommandNextQuant::NOT_QUANT_CONFIRMATION);
							//SendEventI(netCommandNextQuant, UNetID::UNID_ALL_PLAYERS);

						}
						hostGeneralCommandCounter=nci.finGeneraCommandCounter;
						netCommandNextQuant netCommandNextQuant(nci.endQuantCommandTransmit, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, netCommandNextQuant::NOT_QUANT_CONFIRMATION);
						SendEventI(netCommandNextQuant, UNetID::UNID_ALL_PLAYERS);

						UniverseX::clearListGameCommands(tmpListGameCommands);

						//Init game counter afte MigrateHost
						//hostGeneralCommandCounter; //��� ��������� ����
						quantConfirmation=netCommandNextQuant::NOT_QUANT_CONFIRMATION;
						m_nQuantCommandCounter=0;
						m_numberGameQuant=nci.endQuantCommandTransmit+1;//!����
						///ClearDeletePlayerGameCommand();
						ClearCommandList();
						//������ ����� ���� ����
						hostPause=0;
						int playersIdxArr[NETWORK_PLAYERS_MAX];
						for(int m=0; m<NETWORK_PLAYERS_MAX; m++) playersIdxArr[m]=netCommand4C_Pause::NOT_PLAYER_IDX;
						netCommand4C_Pause ncp(playersIdxArr, false);
						SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);

						nCState_th2=NSTATE__HOST_GAME;
					}
					break;
				case NETCOM4G_ChatMessage:
					{
						netCommand4G_ChatMessage nc_ChatMessage(in_HostBuf);
						SendEventI(nc_ChatMessage, UNetID::UNID_ALL_PLAYERS);
					}
					break;
				case NETCOM4C_ServerTimeControl:
					{
						terEventControlServerTime event(in_HostBuf);

	///					m_pGame->SetGameSpeedScale(event.scale, dpnidPlayer);
					}
					break;
				case NETCOM4H_RejoinRequest:
					{
						//����������� ������ �������(������ ������ ���������� �������� �� �� NEXT_QUANT)
						netCommand4H_ReJoinRequest nc(in_HostBuf);
						th2_AddClientToMigratedHost(unid, nc.currentLastQuant, nc.confirmedQuant);
					}
					break;
				default:
					{
						xassert("Invalid netCommand to host.");
						in_HostBuf.ignoreNetCommand();
					}
					break;
				}
				//����������� �������� currentNetCommandID(�.�. ������������ ignoreNetCommand)
				in_HostBuf.nextNetCommand();
			}
			break; //for-a
		}
		//if(p==m_clients.end()) //�� ���� ������������ ������ �� ������������� DPNID
		if(k==NETWORK_PLAYERS_MAX){ //�� ���� ������������ ������ �� ������������� DPNID
			while(in_HostBuf.currentNetCommandID()) {
				if(in_HostBuf.currentNetCommandID()==NETCOM4H_JoinRequest){
					//HandleNewPlayer(dpnid);
					netCommandC_JoinRequest ncjr(in_HostBuf);
					xassert(isDemonWareMode());
					if(!isDemonWareMode()) continue;

					sConnectInfo& clientConnectInfo=ncjr.connectInfo;
					if(!clientConnectInfo.checkOwnCorrect()) {
						xassert(0&& "Connect info defected!");
						continue;
					}

					int resultIdx=USER_IDX_NONE;
					static sDigitalGameVersion hostDGV(true);//�������� ������ ����
					static sReplyConnectInfo replyConnectInfo;
					if(hostDGV!=clientConnectInfo.dgv){ //����������������� ������ ����
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_VERSION, hostDGV);
					}
					else if( clientConnectInfo.flag_quickStart )
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
					else if( nCState_th2!=NSTATE__HOST_TUNING_GAME ) { //if(flag_StartedLoadGame) { // ���� ��������
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_STARTED, hostDGV);
					}
					else if( (!gamePassword.empty()) && (!clientConnectInfo.isPasswordCorrect(gamePassword.c_str())) ){
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD, hostDGV);
					}
					else {
						resultIdx=AddClient(clientConnectInfo.perimeterConnectPlayerData, unid);
						if(resultIdx==USER_IDX_NONE)// ���� ������
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_FULL, hostDGV);
						else 
                            replyConnectInfo.set(sReplyConnectInfo::CR_OK, hostDGV);
					}
					netCommand4C_JoinResponse ncjrs(replyConnectInfo);
					if(replyConnectInfo.connectResult==sReplyConnectInfo::CR_OK){ //������� �� tmpConnection � �������� �������
					}
					else {//�������� ����������
						InOutNetComBuffer tmp(128, true);
						tmp.putNetCommand(&ncjrs);
						//tmp.send(*this, unid, true);
					}
				}
				else if(in_HostBuf.currentNetCommandID()==NETCOM4H_RejoinRequest && nCState_th2==PNC_STATE__NEWHOST_PHASE_A){
					netCommand4H_ReJoinRequest nc(in_HostBuf);
					th2_AddClientToMigratedHost(unid, nc.currentLastQuant, nc.confirmedQuant);
				}
				else if(in_HostBuf.currentNetCommandID()==NETCOM4H_ResponceLastQuantsCommands){
					netCommand4H_ResponceLastQuantsCommands nci(in_HostBuf);
				}
				else {
					xassert("Invalid netCommand to host (unknown source)");
					in_HostBuf.ignoreNetCommand();
				}

				in_HostBuf.nextNetCommand();//���������� ��������� ��������
			}
		}
	}while(PutInputPacket2NetBuffer(in_HostBuf, unid)!=0);
}


void PNetCenter::UnLockInputPacket()
{
	if(flag_LockIputPacket) flag_LockIputPacket--;
}


bool PNetCenter::PutInputPacket2NetBuffer(InOutNetComBuffer& netBuf, UNetID& returnUNID)
{
	if(flag_LockIputPacket) return 0;

	int cnt=0;
	list<XDPacket>::iterator p=m_DPPacketList.begin();
	if(p!=m_DPPacketList.end()){
		returnUNID = p->unid;
		while(p!=m_DPPacketList.end()){
			if(returnUNID==p->unid){
				if(netBuf.putBufferPacket(p->buffer, p->size)){
					p=m_DPPacketList.erase(p);
					cnt++;
				}
				else break;
			}
			else p++;
		}
	}
	return (cnt!=0);
}

/// !!! �������� ��������� !!! �������� ���������� ��������� ����� ������� !!!
void PNetCenter::PutGameCommand2Queue_andAutoDelete(netCommandGame* pCommand)
{
	pCommand->setCurCommandQuantAndCounter(m_numberGameQuant, hostGeneralCommandCounter);
	m_CommandList.push_back(pCommand);
	hostGeneralCommandCounter++;
	m_nQuantCommandCounter++;
}

void PNetCenter::ClearDeletePlayerGameCommand()
{
	list<netCommand4G_ForcedDefeat*>::iterator p;
	for(p=m_DeletePlayerCommand.begin(); p!=m_DeletePlayerCommand.end(); p++){
		delete *p;
	}
	m_DeletePlayerCommand.clear();
}

void PNetCenter::removeUserInQuickStart(const UNetID& unid)
{
	m_qsStateAndCondition.removeUser(unid);
}

void PNetCenter::discardUser_th2(const UNetID& unid)
{
	disconnectUsersSuspended.push_back(unid);


	//int uidx;
	//MissionDescriptionNet& mdch = isHost()? hostMissionDescription : clientMissionDescription;
	//uidx=mdch.findUserIdx(unid);
	//xassert(uidx!=USER_IDX_NONE);
	//if(uidx!=USER_IDX_NONE ){
	//	if(flag_StartedLoadGame){
	//		int playerID;
	//		playerID=mdch.findSlotIdx(uidx);
	//		if(playerID!=PLAYER_ID_NONE){
	//			if(mdch.getAmountCooperativeUsers(playerID) <= 1 ){
	//				netCommand4G_ForcedDefeat* pncfd=new netCommand4G_ForcedDefeat(playerID);
	//				m_DeletePlayerCommand.push_back(pncfd);
	//			}
	//		}
	//	}
	//}
	netCommand4C_DiscardUser ncdu(unid);
	SendEventI(ncdu, UNetID::UNID_ALL_PLAYERS);
}

void PNetCenter::deleteUserQuant_th2()
{
	vector<UNetID>::iterator p;
	
	for(p=disconnectUsersSuspended.begin(); p!=disconnectUsersSuspended.end(); p++){
		if(*p==m_localUNID){
			LogMsg("RemovePlayer delete this!\n");
			//ExecuteInterfaceCommand_thA(NetGEC_HostTerminatedSession); //���������� ����� �������� netCommand4C_DiscardUser
			ExecuteInternalCommand(PNC_COMMAND__END_GAME, false); //�.�. ExecuteInterfaceCommand_thA �������������� �� �����(���� ��� ������ �� ����������� ���������� �����) ���������� ������� ��������
		}
		else {
			if(isDemonWareMode());
			else
				RemovePlayerDP(*p);
		}
	}
	disconnectUsersSuspended.clear();

	MTAuto _lock(m_GeneralLock);
	for(p=deleteUsersSuspended.begin(); p!=deleteUsersSuspended.end(); p++){
		UNetID& cur_unid=*p;
		int uidx;
		MissionDescriptionNet& mdch = isHost()? hostMissionDescription : clientMissionDescription;
		uidx=mdch.findUserIdx(cur_unid);
		xassert(uidx!=USER_IDX_NONE);
		if(uidx==USER_IDX_NONE ){
			deleteUsersSuspended.erase(p);
			return;
		}
		if(flag_StartedLoadGame){
			int playerID;
			playerID=mdch.findSlotIdx(uidx);
			if(playerID!=PLAYER_ID_NONE){
				if(mdch.getAmountCooperativeUsers(playerID) <= 1 ){
					netCommand4G_ForcedDefeat* pncfd=new netCommand4G_ForcedDefeat(playerID);
					m_DeletePlayerCommand.push_back(pncfd);
				}
			}
		}

		if(isHost()){
			LogMsg("Client 0x%X() disconnecting-", cur_unid.dpnid());
			//if(hostMissionDescription.disconnectPlayer2PlayerDataByDPNID(dpnid))
			if(hostMissionDescription.disconnectUser(hostMissionDescription.findUserIdx(cur_unid)))
				LogMsg("OK\n");
			else
				LogMsg("error in missionDescription\n");
		}
		else {
		}
		if(flag_StartedLoadGame){
			int idx=clientMissionDescription.findUserIdx(cur_unid);
			xassert(idx!=USER_IDX_NONE);
			if(idx!=USER_IDX_NONE){
				//������� ��������� � ���, ��� ����� �����
				//if(dwReason & DPNDESTROYPLAYERREASON_NORMAL){
				//	ExecuteInterfaceCommand_thA(NetMsg_PlayerExit, clientMissionDescription.usersData[idx].playerNameE);
				//}
				//else {
					ExecuteInterfaceCommand_thA(NetMsg_PlayerDisconnected, clientMissionDescription.usersData[idx].playerNameE);
				//}
			}
			//�������� ������ �� clientMD
			clientMissionDescription.disconnectUser(clientMissionDescription.findUserIdx(cur_unid));
		}
	}
	deleteUsersSuspended.clear();
}

