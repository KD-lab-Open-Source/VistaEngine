#include "StdAfx.h"

#include "Runtime.h"
#include "P2P_interface.h"
#include "GS_interface.h"
//#include "DW_interface.h"
//#include "DW_interfaceFull.h"

#include "GameShell.h"
#include "UniverseX.h"
#include "UI_Logic.h"

#include "Lmcons.h"

#include "..\terra\terra.h"

#include <algorithm>

#include "QSWorldsMgr.h"

#include "LogMsg.h"

const unsigned int MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST=30000;//10sec


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

		///???? Выполнение двух комманд допускаеться для Reset & end !
		if(currentExecutionInternalCommand != NCmd_Null){
			LogMsg("@Double command ! \n");
			internalCommandEnd_th2( false );
		}

		currentExecutionInternalCommand=*internalCommandList.begin();
		internalCommandList.pop_front();
	}
	LogMsg("@command<%s> Wait:%u-start\n", getStrInternalCommand(currentExecutionInternalCommand), currentExecutionInternalCommand.isFlagWaitExecuted() );
	if(nCState_th2 == NSTATE__PARKING || nCState_th2 == PNC_STATE__NONE) {
		//В паркинге & None(для избежания дисконнекта и миграции после завершения) скипуеться все кроме Reset 
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
			if(isDemonWareMode())
				/*demonwareFull()->reset2FindHost()*/;
			else {
				if(isConnectedDP()) {
					Close();
					InitDP();//Close DirectPlay-я выполняет полную деинициализацию
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
 //               m_hostUNID=demonwareFull()->getConnectionUNID();
	//		nCState_th2=PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0;
	//	}
	//	//SetEvent(hCommandExecuted);
	//	internalCommandEnd_th2();
	//	break;
	case PNC_COMMAND__STOP_GAME_AND_MIGRATION_HOST:
		{
			if(isDemonWareMode()){
				/*if(demonwareFull()->isHost())
					m_hostUNID=m_localUNID;
				else 
					m_hostUNID=demonwareFull()->getConnectionUNID()*/;
			}
			else {} //DP сам назначает
			if( m_hostUNID == m_localUNID ) // Host Я
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

			nCState_th2=NSTATE__HOST_TUNING_GAME; //Необходимо для DPN_MSGID_ENUM_HOSTS_QUERY чтоб сразу выдавал правильную инфу

			hostMissionDescription.clearAllUsersData();//вместоClearClients();
			if(isDemonWareMode()){
				//m_hostUNID=m_localUNID=demonwareFull()->getMyUNID();
				if(AddClient(internalConnectPlayerData, m_localUNID)==USER_IDX_NONE){
					//xassert(0&&"Error connect host 2 missionDescription");
					//ErrH.Abort("Network: General error 2!");
					ExecuteInterfaceCommand_thA(NetRC_CreateGame_CreateHost_Err);
					//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
					nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
				}
				else {
					//demonwareFull()->createHostAndGameT2(m_GameName.c_str(), &hostMissionDescription);
					hostMissionDescription.resetUsersAmountChanged();
					//ExecuteInterfaceCommand_thA(NetRC_CreateGame_Ok); //createHostAndGameT2 сам отправляет сообщения
				}
			}
			else { //DirectPlay
				StopFindHostDP();
				LogMsg("DP starting server...");
				if(!isConnectedDP())
					ServerStart(m_GameName.c_str(), PERIMETER_DEFAULT_PORT);//
				LogMsg("started\n");
				if(isConnectedDP()) 
					ExecuteInterfaceCommand_thA(NetRC_CreateGame_Ok);
				else 
					ExecuteInterfaceCommand_thA(NetRC_CreateGame_CreateHost_Err);
				if(AddClient(internalConnectPlayerData, m_localUNID)==USER_IDX_NONE){
					//xassert(0&&"Error connect host 2 missionDescription");
					//ErrH.Abort("Network: General error 2!");
					ExecuteInterfaceCommand_thA(NetRC_CreateGame_CreateHost_Err);
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
				nCState_th2=NSTATE__QSTART_NON_CONNECT;
				//demonwareFull()->reset2FindHost();
				//demonwareFull()->quickStart();
			}
			else {
				xassert(0&&"Quick Start mode - DemonWare only");;
			}

		}
		internalCommandEnd_th2();
		break;
	//Секция продолжительных комманд 
	// Продолжительная комманда
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
					/*if(demonwareFull()->joinDW(m_gameHostID)){
						nCState_th2=PNC_STATE__CONNECTION;
					}
					else {*/
						nCState_th2 = NSTATE__PARKING;
						internalCommandEnd_th2( false );
						ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
						//ExecuteInternalCommand(PNCCmd_Reset2FindHost, false);
					}
				//}
				else 
					nCState_th2=PNC_STATE__CONNECTION;
			}
			else {
				xassert(0&&"Connecting: command order error(not find host state)");
				nCState_th2 = NSTATE__PARKING;
				internalCommandEnd_th2( false );
				ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
			}
		}
		break;
	// Продолжительная комманда
	case PNC_COMMAND__END_GAME:
		{
			nCState_th2=PNC_STATE__ENDING_GAME;
			flag_StartedLoadGame = false;
			//flag_StartedGame = false;
			//if(isConnected()) {
				if(isHost()){
					//Гарантированная отсылка последнего кванта
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
bool PNetCenter::SecondThread_th2(void)
{
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	networkTime_th2 = xclock();

	if(!isDemonWareMode()){
		XDPInit();
		InitDP();
		StartFindHostDP();
	}
	nCState_th2 = NSTATE__FIND_HOST;

	if(isDemonWareMode())
		//DWInterfaceSimple::init(this);
	//Инициализация завершена - XDPConnection создан
	SetEvent(hSecondThreadInitComplete);

	if(workMode==PNCWM_LAN_DW)
		/*demonware()->init2(false)*/;
	else if(isDemonWareMode())
		/*demonware()->init2(true)*/; //demonware сам посылает NetRC_Init_Ok или NetRC_Init_Err 
	else 
		ExecuteInterfaceCommand_thA(NetRC_Init_Ok);

	bool flag_end=0;
	while(!flag_end){
		networkTime_th2 = xclock();

		if(nCState_th2 != NSTATE__PARKING && isDemonWareMode()){
			/*demonware()->quantNetBegint()*/;
		}

		MTAuto* _pLock=new MTAuto(m_GeneralLock);
		//decoding command 
		flag_end = internalCommandQuant_th2();

		if(nCState_th2 != NSTATE__PARKING) { //Для понимания ; а так нет необходимости

			if(nCState_th2&PNC_State_QuickStart)
                quickStartReceiveQuant_th2();
			else if(isHost())
				th2_HostReceiveQuant();
			else if(isClient()){//Client receive quant!
				th2_ClientPredReceiveQuant();
			}
		}

		delete _pLock;

		if(flag_end) break; //для быстрого выхода

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
				/*demonware()->quantNetEnd()*/;
				flag_dwEndQuantRunned=true;
			}
			deleteUserQuant_th2();
			nextQuantTime_th2+=m_quantInterval;
		}
		if(!flag_dwEndQuantRunned && nCState_th2 != NSTATE__PARKING && isDemonWareMode()){
			/*demonware()->quantNetEnd()*/;
		}
		networkTime_th2 = xclock();
		if(nextQuantTime_th2 - TIME_MISTAKE > networkTime_th2){
			Sleep(nextQuantTime_th2 - TIME_MISTAKE - networkTime_th2);
		}
		//end logic quant
	}

	if(isDemonWareMode())
		//DWInterfaceSimple::finit();

	if(!isDemonWareMode()){
		StopFindHostDP();
		SetConnectionTimeout(1);//Для быстрого завершения
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
				//RemovePlayer(delPlayerUNID); //Полное удаление по DPN_MSGID_DESTROY_PLAYER
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
				ud.unid.setEmpty(); //перестраховка
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
			ud.lastTimeBackPacket=curTime;//Необходимо для корректного начального таймаута
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
	//поиск есть-ли такой клиент
	int j;
	for(j=0; j<NETWORK_PLAYERS_MAX; j++){
		UserData& ud=hostMissionDescription.usersData[j];
		if(ud.flag_userConnected){
			if(ud.unid == _unid && ud.flag_playerStartReady==1) return 0; //повторно не подключать
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


void PNetCenter::SendEventI(NetCommandBase& event, const UNetID& unid, bool flag_guaranted)
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
		LogMsg("%s", to);
		//ExecuteInternalCommand(PNC_COMMAND__ABORT_PROGRAM, false);
	}
	else {
		// Сравнение для netCommand4H_BackGameInformation2
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
	///else xassert(0 && "Нераспознанная десинхронизация");
}



void PNetCenter::th2_LLogicQuant()
{
	//LogMsg("q=%d\n", m_nQuantDelay + nDbgServerLag + int(nDbgServerLagNoise*float(rand())/RAND_MAX));

	//Не залоченный поток !
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
		xassert(workMode==PNCWM_ONLINE_DW);
		{
			m_qsStateAndCondition.gameOrder=m_QSGameOrder;
			m_qsStateAndCondition.clear();
			//int quickStartUsrIDX=m_qsStateAndCondition.addPlayers(internalConnectPlayerData, demonwareFull()->getMyUNID());
			//xassert(quickStartUsrIDX!=USER_IDX_NONE);
			/*if(quickStartUsrIDX==USER_IDX_NONE){
				ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
				nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
			}*/

			//m_localUNID=demonwareFull()->getMyUNID();
			m_gameHostID = ZERO_GUID;

			//MTAuto lock(demonwareFull()->m_DWGeneralLock);
			//vector<sGameHostInfoDW>& ghl=demonwareFull()->gameHostList;
			//vector<sGameHostInfoDW>::iterator p;
			//for(p=ghl.begin(); p!=ghl.end(); p++){
			//	if( UNetID(p->dwGameInfo.m_hostAddr.getHash()) < /*!=*/ m_localUNID 
			//		&& startGameParam.isConditionEntry( p->gameStatusInfo ) )
			//			if( p->qsStatus == QSS_None ){
			//				m_gameHostID=p->gameHostGUID;
			//				break;
			//			}
			//}
		}
		if(m_gameHostID!=ZERO_GUID){
			/*if(demonwareFull()->joinDW(m_gameHostID)){
				nCState_th2=NSTATE__QSTART_HOSTING_CLIENTING;
				//demonwareFull()->setQSStatusHost(m_gameHostID, QSS_Connecting);
				hostMissionDescription.clearAllPlayerStartReady();
			}
			else {
				demonwareFull()->setQSStatusHost(m_gameHostID, QSS_Break);
			}*/
		}
		break;
	case NSTATE__QSTART_HOSTING_CLIENTING:
		{
			xassert(workMode==PNCWM_ONLINE_DW);
			//bdConnection::Status status=demonwareFull()->getConnectionStatus();
			/*if(status==bdConnection::BD_CONNECTED){
				static sConnectInfo connectInfo;
				static sDigitalGameVersion dgv(true);
				connectInfo.set(internalConnectPlayerData, gamePassword.c_str(), dgv, true, GameOrder_1v1);
				netCommandC_JoinRequest nc_jrq(connectInfo);
				//m_hostUNID=demonwareFull()->getConnectionUNID();
				//m_localUNID=demonwareFull()->getMyUNID();
				SendEventI(nc_jrq, m_hostUNID);
				nCState_th2=NSTATE__QSTART_CLIENT;
			}
			else if(status!=bdConnection::BD_CONNECTING){ //err
				//demonwareFull()->setQSStatusHost(m_gameHostID, QSS_Break);
				nCState_th2 = NSTATE__QSTART_NON_CONNECT;
			}
			else*/ {;} // не меняем состояние и продолжаем опрашивать
		}
		break;
	case NSTATE__QSTART_HOST:
		xassert(workMode==PNCWM_ONLINE_DW);
		{
			//m_hostUNID = m_localUNID = demonwareFull()->getMyUNID();
			if(m_qsStateAndCondition.isConditionRealized()){

				XGUID missionGUID;
				__int64 lowFilterMap = m_qsStateAndCondition.getTotalFilterMap();
				qsWorldsMgr.getRandomMissionGuid(lowFilterMap, missionGUID);

				const MissionDescription* mission = UI_LogicDispatcher::instance().getMissionByID(missionGUID);
				if(!mission){
					ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
					nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
					LogMsg("Quick start error - No Mission\n");
					break; //Error
				}
				LogMsg("QS - mission - %s", mission->saveName());

                hostMissionDescription=*mission;
				hostMissionDescription.setGameType(GAME_TYPE_MULTIPLAYER);
				hostMissionDescription.setUseMapSettings(false);
				hostMissionDescription.clearAllUsersData();

				for(int k=0; k<QSStateAndCondition::MAX_QSUSERS; k++){
					QSStateAndCondition::QSUserState& qsus = m_qsStateAndCondition.qsUserState[k];
					if(qsus.flag_userConnected){
						/*if(k==0 && qsus.unid!=demonwareFull()->getMyUNID()){
							xassert(0&&"Invalid m_qsStateAndCondition");
							ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
							nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
							LogMsg("Quick start error - AddClient1\n");
							break; //Error
						}*/
						if(AddClient(qsus.connectPlayerData, qsus.unid, true)==USER_IDX_NONE){
							ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
							nCState_th2=PNC_STATE__NET_CENTER_CRITICAL_ERROR;
							LogMsg("Quick start error - AddClient2\n");
							break; //Error
						}
					}
				}
				LogMsg("Quick Start Game Started\n");
				//if(isDemonWareMode())
				//	demonwareFull()->sendAllUserAddrTable_EnableMigration_RestrictNewConnection2Host(&hostMissionDescription);
				XBuffer auxDWData;
				if(isDemonWareMode())
					//demonwareFull()->getAllUserAddrTable_EnableMigration_RestrictNewConnection2Host("Quick start", &hostMissionDescription, auxDWData);
				sendStartLoadGame2AllPlayers_th2(auxDWData);
				LogMsg("Wait for all clients ready. \n");
				//Для удаления игроков отпавших при загрузке
				ClearDeletePlayerGameCommand();
				nCState_th2=NSTATE__HOST_LOADING_GAME;
				ExecuteInterfaceCommand_thA(NetRC_QuickStart_Ok);
			}
			//int population = hostMissionDescription.gamePopulation();
			//if( (m_QSGameOrder==GameOrder_1v1 && population==1) 
			//	&& (m_QSGameOrder==GameOrder_2v2 && population==2)
			//	&& (m_QSGameOrder==GameOrder_3v3 && population==3) ){
			//		LogMsg("Quick Start Game Started\n");
			//		sendStartLoadGame2AllPlayers_th2();
			//		if(isDemonWareMode())
			//			demonwareFull()->sendAllUserAddrTable_EnableMigration_RestrictNewConnection2Host(&hostMissionDescription);
			//		LogMsg("Wait for all clients ready. \n");
			//		//Для удаления игроков отпавших при загрузке
			//		ClearDeletePlayerGameCommand();
			//		nCState_th2=NSTATE__HOST_LOADING_GAME;
			//}
			//else if(gameOrderStatus == MissionDescription::GOS_Error)
            //    ExecuteInternalCommand(NCmd_QuickStart, false);
		}
		break;
	case NSTATE__QSTART_CLIENT:
		xassert(workMode==PNCWM_ONLINE_DW);

		break;
	//////////////////////////////////////////////
	// Simple start game
	case PNC_STATE__CONNECTION:
		if(isDemonWareMode()){
			//bdConnection::Status status=demonwareFull()->getConnectionStatus();
			/*if(status==bdConnection::BD_CONNECTED){
				static sConnectInfo connectInfo;
				static sDigitalGameVersion dgv(true);
				connectInfo.set(internalConnectPlayerData, gamePassword.c_str(), dgv, false, GameOrder_Any);
				netCommandC_JoinRequest nc_jrq(connectInfo);
				//m_hostUNID=demonwareFull()->getConnectionUNID();
				//m_localUNID=demonwareFull()->getMyUNID();
				SendEventI(nc_jrq, m_hostUNID);
				//StopFindHostDP();
				nCState_th2=PNC_STATE__CLIENT_TUNING_GAME;
				//SetEvent(hCommandExecuted);
				internalCommandEnd_th2();
			}
			else if(status!=bdConnection::BD_CONNECTING){ //err
				internalCommandEnd_th2( false );
				nCState_th2 = NSTATE__PARKING;
				ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
				//ExecuteInternalCommand(PNCCmd_Reset2FindHost, false);
			}
			else*/ {;} // не меняем состояние и продолжаем опрашивать
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
					ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
					//ExecuteInternalCommand(PNCCmd_Reset2FindHost, false);
					//ErrH.Abort("Unable to find multiplayer server");
				}
			}
			else { // IP==0
				if(gameSpyInterface){
					nCState_th2 = NSTATE__PARKING;
					ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
				}
				else if(!Connect(m_gameHostID)) {
					nCState_th2 = NSTATE__PARKING;
					ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
					//ExecuteInternalCommand(PNCCmd_Reset2FindHost, false);
					//ErrH.Abort("Unable to find multiplayer server");
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
				if(hostMissionDescription.isUsersAmountChanged()){
					//demonwareFull()->updateHostAndGameInfo(m_GameName.c_str(), &hostMissionDescription);
					hostMissionDescription.resetUsersAmountChanged();
				}
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
				//	demonwareFull()->sendAllUserAddrTable_EnableMigration_RestrictNewConnection2Host(&hostMissionDescription);
				if(isDemonWareMode())
					//demonwareFull()->getAllUserAddrTable_EnableMigration_RestrictNewConnection2Host(m_GameName.c_str(), &hostMissionDescription, auxDWData);
				sendStartLoadGame2AllPlayers_th2(auxDWData);

				//ReleaseAllPlayers
				LogMsg("Wait for all clients ready. \n");

				//Для удаления игроков отпавших при загрузке
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

			while(!firstList.empty()) { //проверка что первый список не пустой
				unsigned int countCompare=0;
				int k;
				for(k=0; k<NETWORK_PLAYERS_MAX; k++){
					UserData& ud=hostMissionDescription.usersData[k];
					if(!ud.flag_userConnected || ud.unid==m_hostUNID) continue; //Пропуск пустых слотов и хоста 
					vector<BackGameInformation2>& secondList = ud.backGameInf2List;
					if(secondList.empty()) goto end_while_01;//завершение while если один из списков кончится раньше первого
					//if(secondList.empty()) continue; //сравниваются все списки!

					if( *firstList.begin() == *secondList.begin() ) countCompare++;
					else {
						th2_SaveLogByDesynchronization(firstList, secondList);
						ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
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
					if(ud.lagQuant > maxInternalLagQuant) {//Для подгонки всех клиентов
						maxInternalLagQuant=ud.lagQuant; 
					}
					if(ud.accessibleLogicQuantPeriod > maxAccessibleLogicQuantPeriod) {//Для подгонки всех клиентов
						maxAccessibleLogicQuantPeriod=ud.accessibleLogicQuantPeriod; 
					}
					if(ud.lastExecuteQuant < minClientExecutionQuant) {//Для подгонки всех клиентов
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

			/// Подгонка под всех клиентов !
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
						//RemovePlayer(ud.unid); //Полное удаление по DPN_MSGID_DESTROY_PLAYER
						discardUser_th2(ud.unid);
						break;//по одному клиенту за квант!
					}
				}
			}

			if(hostPause && (!flag_requestPause) ){
				//Отмена паузы
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

			//перенесение всех команд удаления в список комманд на выполнение
			list<netCommand4G_ForcedDefeat*>::iterator p;
			for(p=m_DeletePlayerCommand.begin(); p!=m_DeletePlayerCommand.end(); p++){
				PutGameCommand2Queue_andAutoDelete(*p);
				LogMsg("ForcedDefeat for UserID-%u\n", (*p)->userID);
			}
			m_DeletePlayerCommand.clear();

			//Установка последней команде, признака, последняя
			if(!m_CommandList.empty()){
				list<NetCommandBase*>::iterator p=m_CommandList.end();
				p--; //последняя команда
				if((*p)->isGameCommand()){
					(static_cast<netCommandGame*>(*p))->setFlagLastCommandInQuant();
				}
				else xassert("Error in commands list on server");
			}

			///hostGeneralCommandCounter+=m_CommandList.size(); //сейчас добавляется при PutGameCommand2Queue_andAutoDelete


			if(0){
				static bool flag_timeOut=0; //Надо перенести в тело класса!!!
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
				//Или сверху или это
				netCommandNextQuant* pcom=new netCommandNextQuant(m_numberGameQuant, m_quantInterval, m_nQuantCommandCounter, hostGeneralCommandCounter, quantConfirmation);
				m_CommandList.push_back(pcom);
			}

			list<NetCommandBase*>::iterator i;
			FOR_EACH(m_CommandList, i) {
				if((**i).EventID==NETCOM_ID_NEXT_QUANT || (**i).EventID==NETCOM_4C_ID_CLIENT_IS_NOT_RESPONCE)
					SendEventI(**i, UNetID::UNID_ALL_PLAYERS, false);//Не гарантированная доставка!!!
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
			//Ожидание обработки всех комманд игрой
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				if(universeX()){
					if(in_ClientBuf.currentNetCommandID()!=NETCOM_ID_NONE) break;
					if(universeX()->allowedRealizingQuant > universeX()->lastRealizedQuant) break;
					//по идее безопасно вызывать т.к. верхнее условие выполнится только при прошедшем кванте, но лучше переделать
					universeX()->stopGame_HostMigrate();//Очистка всех команд текущего кванта
				}
				flag_SkipProcessingGameCommand=1;
				//По идее список пуст т.к. только выполнился th2_ClientPredReceiveQuant
				m_DPPacketList.clear();
			}

			//Разблокирование входных пакетов, заблокированных при поступлении сообщения о смене Host-а
			//Необходимо для того, чтобы не пропустить сообщение о reJoin-е
			UnLockInputPacket();

			//Уничтожение всех не выполнившихся команд ?
			///////CAutoLock lock(&m_ClientInOutBuffLock);

			
			//Т.к. миграция разрешается только после START_LOAD_GAME clientMissionDescription корректен
			hostMissionDescription=clientMissionDescription;
			//Update state MD 
			if(isDemonWareMode()){
				for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
					/*if(hostMissionDescription.usersData[i].flag_userConnected && 
						(!demonwareFull()->checkUserState(hostMissionDescription.usersData[i].unid)) ){
							//deleteClientByDPNID_th3(hostMissionDescription.usersData[i].unid.dpnid(), DPNDESTROYPLAYERREASON_NORMAL);
							deleteUser_thA(hostMissionDescription.usersData[i].unid);
						}*/
				}
				//demonwareFull()->closeNotConnectedConnection();
			}
			//hostMissionDescription.clearAllUsersData(); //вместо ClearClientData();
			hostMissionDescription.clearAllPlayerStartReady();

			beginWaitTime_th2 = networkTime_th2;

			//очистка InHostBuffer в него пришли уже не те подтверждения!
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				th2_clearInOutClientHostBuffers();
			}
			if(th2_AddClientToMigratedHost(m_localUNID, universeX()->getCurrentGameQuant(), universeX()->getConfirmQuant()) ){
				nCState_th2=PNC_STATE__NEWHOST_PHASE_A;
			}
			else {
				LogMsg("Migration Host - th2_AddClientToMigratedHost Error!\n");
				ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
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
				else { //все время вышло
					LogMsg("PNC_STATE__NEWHOST_PHASE_A - Time has expired!\n");
					if(isDemonWareMode()){
						for(int i=0; i<NETWORK_PLAYERS_MAX; i++){
							if(hostMissionDescription.usersData[i].flag_userConnected && 
								(!hostMissionDescription.usersData[i].flag_playerStartReady) ){
									LogMsg("User 0x%X() is not rejoin - removing.\n", hostMissionDescription.usersData[i].unid.dpnid());
                                    //demonwareFull()->connectionBreak(hostMissionDescription.usersData[i].unid);
								}
						}
					}
				}

			}
			
			if( result==1 ) { //если все существующие в группе игроки
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

				///minQuant+=1; //Запрос следующего кванта за мин квантом
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
			if( (unidClientWhichWeWait.isEmpty()) || (hostMissionDescription.findUserIdx(unidClientWhichWeWait)==-1) ){ //игрок выбыл до прихода нужных хосту игровых комманд
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
				minQuant+=1; //Запрос следующего кванта за мин квантом
				netCommand4C_RequestLastQuantsCommands nc(minQuant);
				SendEventI(nc, maxQuantClientUNID);
				unidClientWhichWeWait=maxQuantClientUNID;
			}
		}
		break;
	case PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0:
		{
			//Ожидание обработки всех комманд игрой
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				if(universeX()){
					if(in_ClientBuf.currentNetCommandID()!=NETCOM_ID_NONE) break;
					if(universeX()->allowedRealizingQuant > universeX()->lastRealizedQuant) break;
					//по идее безопасно вызывать т.к. верхнее условие выполнится только при прошедшем кванте, но лучше переделать
					universeX()->stopGame_HostMigrate();//Очистка всех команд текущего кванта
				}
				flag_SkipProcessingGameCommand=1;
				//По идее список пуст т.к. только выполнился th2_ClientPredReceiveQuant
				m_DPPacketList.clear();
			}

			//Разблокирование входных пакетов, заблокированных при поступлении сообщения о смене Host-а
			UnLockInputPacket();

			//очистка out_ClientBuf в него пришли уже не те подтверждения!
			{
				MTAuto _Lock(m_GeneralLock); //! Lock
				th2_clearInOutClientHostBuffers();
			}
			nCState_th2=PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB;
			//ClearClientData();
			//hostMissionDescription.clearAllUsersData(); //вместо ClearClientData();
			beginWaitTime_th2 = networkTime_th2;
		}
		break;
	case PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB:
		{
			MTAuto _Lock(m_GeneralLock); //! Lock для senda

			static unsigned char bandPassFilter=0;
			if((bandPassFilter&0x7)==0){//каждый 8й квант
				netCommand4H_ReJoinRequest ncrjr(universeX()->getCurrentGameQuant(), universeX()->getConfirmQuant() );
				SendEventI(ncrjr, m_hostUNID);
				///nCState_th2=PNC_STATE__CLIENT_GAME;
			}
			bandPassFilter++;

			if((networkTime_th2 - beginWaitTime_th2) < MAX_TIME_WAIT_RESTORE_GAME_AFTER_MIGRATE_HOST){
			}
			else { //все время вышло
				LogMsg("There is no answer to a command netCommand4H_ReJoinRequest!\n");
				ExecuteInterfaceCommand_thA(NetGEC_GeneralError);
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
				/*demonwareFull()->reset2FindHost()*/;
			}
			else {
				StopFindHostDP();
				Close(false);
				InitDP();//Close DirectPlay-я выполняет полную деинициализацию
			}
			MTAuto _Lock(m_GeneralLock); //! Lock
			hostMissionDescription.clearAllUsersData();//вместоClearClients();
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

		while(tmp.currentNetCommandID()!=NETCOM_ID_NONE) {
			if(tmp.currentNetCommandID()==NETCOM_4C_ID_START_LOAD_GAME){
				netCommand4C_StartLoadGame nc4c_sl(tmp);
				if(nCState_th2 == NSTATE__QSTART_CLIENT){
                    //ExecuteInternalCommand(PNC_COMMAND__CLIENT_STARTING_LOAD_GAME, false);
					nCState_th2 = PNC_STATE__CLIENT_TUNING_GAME;//PNC_STATE__CLIENT_LOADING_GAME;
					ExecuteInterfaceCommand_thA(NetRC_QuickStart_Ok);
					return;//!!! //Чтоб th2_ClientPredReceiveQuant мог ее еще раз обработать
				}
			}
			if( tmp.currentNetCommandID()==NETCOM_4H_ID_JOIN_REQUEST){
					netCommandC_JoinRequest ncjr(tmp);
					xassert(isDemonWareMode());

					sConnectInfo& clientConnectInfo=ncjr.connectInfo;
					if(clientConnectInfo.checkOwnCorrect()) {
						int resultIdx=USER_IDX_NONE;
						static sDigitalGameVersion hostDGV(true);//создание версии игры
						static sReplyConnectInfo replyConnectInfo;
						if( nCState_th2 != NSTATE__QSTART_NON_CONNECT 
							&& nCState_th2 != NSTATE__QSTART_HOSTING_CLIENTING 
							&& nCState_th2 != NSTATE__QSTART_HOST )
								replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
						else if( !clientConnectInfo.flag_quickStart )
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
						else if(hostDGV!=clientConnectInfo.dgv){ //Несоответствующая версия игры
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_VERSION, hostDGV);
						}
						else if( (!gamePassword.empty()) && (!clientConnectInfo.isPasswordCorrect(gamePassword.c_str())) ){
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD, hostDGV);
						}
						//Объедининть с m_qsStateAndCondition.addPlayers
						else if( clientConnectInfo.gameOrder != m_QSGameOrder)
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV); 
						else {
							resultIdx = m_qsStateAndCondition.addPlayers(clientConnectInfo.perimeterConnectPlayerData, unid);
							if(resultIdx==USER_IDX_NONE){ // Игра полная
								replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_FULL, hostDGV);
								LogMsg("QuickStart - break connection\n");
							}
							else 
								replyConnectInfo.set(sReplyConnectInfo::CR_OK, hostDGV);
						}
						netCommand4C_JoinResponse ncjrs(replyConnectInfo);
						if(replyConnectInfo.connectResult==sReplyConnectInfo::CR_OK){ //перенос из tmpConnection в основную таблицу
							/*if(!demonwareFull()->connectionApproved(unid, resultIdx))
								removeUserInQuickStart(unid);
							else{*/
								SendEventI(ncjrs, unid, true);
								if(nCState_th2 == NSTATE__QSTART_NON_CONNECT || nCState_th2 == NSTATE__QSTART_HOSTING_CLIENTING){
									nCState_th2 = NSTATE__QSTART_HOST;
									/*demonwareFull()->switch2QSHost()*/;
								}
							//}

						}
						else {//закрытие соединения
							InOutNetComBuffer tmp(128, true);
							tmp.putNetCommand(&ncjrs);
							//tmp.send(*this, unid, flag_guaranted);
							/*demonwareFull()->tmpConnectionBreak(unid, tmp.buffer(), tmp.filled_size)*/;
						}
					}
					else { // if(!clientConnectInfo.checkOwnCorrect())
						xassert(0&& "Connect info defected!");;
					}
			}
			else if( tmp.currentNetCommandID()==NETCOM_4C_ID_JOIN_RESPONSE){
					netCommand4C_JoinResponse ncjrs(tmp);
					sReplyConnectInfo& hostReplyConnectInfo = ncjrs.replyConnectInfo;
					if(nCState_th2 != NSTATE__QSTART_HOSTING_CLIENTING && nCState_th2 != NSTATE__QSTART_CLIENT){
						/*demonwareFull()->connectionBreak(unid)*/;
					}
					if(hostReplyConnectInfo.checkOwnCorrect()){
						if( hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_OK ){
							//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Ok);
							if(nCState_th2 == NSTATE__QSTART_HOSTING_CLIENTING){
								nCState_th2 = NSTATE__QSTART_CLIENT;
							}
							//demonwareFull()->switch2QSClient();
						}
						else {
							//demonwareFull()->setQSStatusHost(m_gameHostID, QSS_Break);
							//demonwareFull()->connectionBreak(unid);
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

			//отфильтровывание команды
			InOutNetComBuffer tmp(2048, true);
			tmp.putBufferPacket(p->buffer, p->size);

			while(tmp.currentNetCommandID()!=NETCOM_ID_NONE) {
				if(tmp.currentNetCommandID()==NETCOM_4C_ID_START_LOAD_GAME){
					netCommand4C_StartLoadGame nc4c_sl(tmp);
					if(isDemonWareMode())
                        //demonwareFull()->setAllUserAddrTable_connectUsers4HostMigrate(XBuffer(nc4c_sl.auxBuf, nc4c_sl.dataSizeInAuxBuf));
					ExecuteInternalCommand(PNC_COMMAND__CLIENT_STARTING_LOAD_GAME, false);
				}
				else if(tmp.currentNetCommandID()==NETCOM_ID_NEXT_QUANT){
					netCommandNextQuant nc(tmp);
					if(nCState_th2==PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB){
						nCState_th2=PNC_STATE__CLIENT_GAME;
					}
					else if(nCState_th2 == PNC_STATE__CLIENT_LOADING_GAME) {
						if(isDemonWareMode()){
							/*demonwareFull()->startSupplementConnecttion()*/;
						}
						LogMsg("Game Started!\n");
						nCState_th2=PNC_STATE__CLIENT_GAME;
					}
					//if(!flag_StartedGame){
					//	flag_StartedGame=true;
					//	if(isDemonWareMode()){
					//		demonwareFull()->startSupplementConnecttion();
					//	}
					//	LogMsg("Game Started!\n");
					//	nCState_th2=PNC_STATE__CLIENT_GAME;
					//}

				}
				else if(tmp.currentNetCommandID()==NETCOM_4C_ID_JOIN_RESPONSE){
					netCommand4C_JoinResponse ncjrs(tmp);
					xassert(isDemonWareMode());
					if(isDemonWareMode()){
						sReplyConnectInfo& hostReplyConnectInfo = ncjrs.replyConnectInfo;
						if(hostReplyConnectInfo.checkOwnCorrect()){
							if( hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_OK )
								ExecuteInterfaceCommand_thA(NetRC_JoinGame_Ok);
							else {
								if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_VERSION)
									ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameNotEqualVersion_Err);

								else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_STARTED)
									ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsRun_Err);
								else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_FULL)
									ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsFull_Err);
								else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD)
									ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameSpyPassword_Err);
								else 
									ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);

								nCState_th2 = NSTATE__PARKING;
								//ExecuteInternalCommand(PNCCmd_Reset2FindHost, false);
							}
						}
					}
				}
				else
                    tmp.ignoreNetCommand();
				tmp.nextNetCommand();
			}
			//комманды клиенту
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
	do { //Первый проход для внутреннего клиента
		int k;
		for(k=0; k<NETWORK_PLAYERS_MAX; k++){
			UserData& ud=hostMissionDescription.usersData[k];
			if(!ud.flag_userConnected || unid!=ud.unid) continue;

			while(in_HostBuf.currentNetCommandID()) {
				//netLog <= in_HostBuf.currentNetCommandID() < "\n";
				switch(in_HostBuf.currentNetCommandID()) {
				case NETCOM_4H_ID_REQUEST_PAUSE:
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
				case NETCOM_4H_ID_CHANGE_REAL_PLAYER_TYPE:
					{
						netCommand4H_ChangeRealPlayerType ncChRT(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged(true);
						if(unid==m_hostUNID){
							xassert(ncChRT.slotID_ < min(NETWORK_PLAYERS_MAX, hostMissionDescription.playersAmountMax()));
							if( ncChRT.slotID_!=hostMissionDescription.findSlotIdx(hostMissionDescription.findUserIdx(m_hostUNID)) ){//Проверка на то, что меняется не у Host-а
								if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_PLAYER || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_WORLD || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_EMPTY){
									xassert(0 && "Error change real type!");
									ncChRT.newRealPlayerType_=REAL_PLAYER_TYPE_OPEN; //Дополнительная проверка
								}
								SlotData& cur_sd=hostMissionDescription.changePlayerData(ncChRT.slotID_);
								if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER) {
									if( ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_AI || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_CLOSE ){
										//Отбрасывание игрока
										for(int k=0; k<NETWORK_TEAM_MAX; k++){
											int cur_userIdx=cur_sd.usersIdxArr[k];
											if(cur_userIdx!=USER_IDX_NONE){
												UNetID delPlayerUNID=hostMissionDescription.usersData[cur_userIdx].unid;
												//hostMissionDescription.disconnectUser(cur_userIdx);
												//RemovePlayer(delPlayerUNID); //Полное удаление по DPN_MSGID_DESTROY_PLAYER
												discardUser_th2(delPlayerUNID);
											}
										}
										cur_sd.realPlayerType=ncChRT.newRealPlayerType_;
									}
								}
								else if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_AI){ //Если был AI
									if(ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_OPEN || ncChRT.newRealPlayerType_==REAL_PLAYER_TYPE_CLOSE){
										//Закрывать AI
										hostMissionDescription.disconnectAI(ncChRT.slotID_);
										cur_sd.realPlayerType=ncChRT.newRealPlayerType_;
									}
								}
								else { //Если был Close Или Open
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
				case NETCOM_4H_ID_CHANGE_PLAYER_RACE:
					{
						netCommand4H_ChangePlayerRace  ncChB(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host может менять у любого
						hostMissionDescription.changePlayerRace(ncChB.slotID_, ncChB.newRace_, unid, changeAbsolutely);
					}
					break;

				case NETCOM_4H_ID_CHANGE_PLAYER_COLOR:
					{
						netCommand4H_ChangePlayerColor  ncChC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host может менять у любого
						hostMissionDescription.changePlayerColor(ncChC.slotID_, ncChC.newColor_, false, unid, changeAbsolutely);
					}
					break;
				case NETCOM_4H_ID_CHANGE_PLAYER_SIGN:
					{
						netCommand4H_ChangePlayerSign  ncChS(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host может менять у любого
						hostMissionDescription.changePlayerSign(ncChS.slotID_, ncChS.sign_, unid, changeAbsolutely);
					}
					break;
				case NETCOM_4H_ID_CHANGE_PLAYER_DIFFICULTY:
					{
						netCommand4H_ChangePlayerDifficulty ncChD(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host может менять у любого
						hostMissionDescription.changePlayerDifficulty(ncChD.slotID_, ncChD.difficulty_, unid, changeAbsolutely );
					}
					break;
				case NETCOM_4H_ID_CHANGE_PLAYER_CLAN:
					{
						netCommand4H_ChangePlayerClan ncChC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;

						hostMissionDescription.setChanged();
						bool changeAbsolutely = (unid==m_hostUNID); //Host может менять у любого
						hostMissionDescription.changePlayerClan(ncChC.slotID_, ncChC.clan_, unid, changeAbsolutely);
					}
					break;
				case NETCOM_4H_ID_CHANGE_MISSION_DESCRIPTION:
					{
						netCommand4H_ChangeMD nc_changeMD(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						if(unid==m_hostUNID){
							hostMissionDescription.setChanged();
							hostMissionDescription.changeMD(nc_changeMD.chv, nc_changeMD.v);
						}
					}
					break;
				case NETCOM_4H_ID_JOIN_2_COMMAND:
					{
						netCommand4H_Join2Command nc_J2C(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						if(nc_J2C.idxUserData_!=USER_IDX_NONE && hostMissionDescription.findUserIdx(unid)==nc_J2C.idxUserData_)
							hostMissionDescription.join2Command(nc_J2C.idxUserData_, nc_J2C.commandID_);
					}
					break;
				case NETCOM_4H_ID_KICK_IN_COMMAND:
					{
						netCommand4H_KickInCommand nc_KInC(in_HostBuf);
						if(nCState_th2!=NSTATE__HOST_TUNING_GAME) break;
						hostMissionDescription.setChanged(true);
						if(unid==m_hostUNID){
							xassert(nc_KInC.commandID_ < min(NETWORK_PLAYERS_MAX, hostMissionDescription.playersAmountMax()));
							if( nc_KInC.commandID_!=hostMissionDescription.findSlotIdx(hostMissionDescription.findUserIdx(m_hostUNID)) ){//Проверка на то, что меняется не у Host-а
								SlotData& cur_sd=hostMissionDescription.changePlayerData(nc_KInC.commandID_);
								if(cur_sd.realPlayerType==REAL_PLAYER_TYPE_PLAYER) {
									xassert(nc_KInC.teamIdx_>=0 && nc_KInC.teamIdx_ < NETWORK_TEAM_MAX);
									nc_KInC.teamIdx_=clamp(nc_KInC.teamIdx_, 0, NETWORK_TEAM_MAX);
									int cur_userIdx=cur_sd.usersIdxArr[nc_KInC.teamIdx_];
									if(cur_userIdx!=USER_IDX_NONE){
										UNetID delPlayerUNID=hostMissionDescription.usersData[cur_userIdx].unid;
										//hostMissionDescription.disconnectUser(cur_userIdx);
										//RemovePlayer(delPlayerUNID); //Полное удаление по DPN_MSGID_DESTROY_PLAYER
										discardUser_th2(delPlayerUNID);
									}
								}
							}
						}
					}
					break;
				case NETCOM_4H_ID_START_LOAD_GAME:
					{
						netCommand4H_StartLoadGame ncslg(in_HostBuf);
						hostMissionDescription.setChanged();

						hostMissionDescription.setPlayerStartReady(unid);
						///StartGame();
					}
					break;
				case NETCOMC_ID_PLAYER_READY:
					{
						netCommandC_PlayerReady event(in_HostBuf);
						hostMissionDescription.setChanged();

						hostMissionDescription.setPlayerGameLoaded(unid, event.gameCRC_);

						//(*p)->m_flag_Ready=1;
						//(*p)->clientGameCRC=event.gameCRC_;

						LogMsg("Player 0x%X() (GCRC=0x%X) reported ready\n", unid, event.gameCRC_);
						///SetEvent(m_hReady);
					}
					break;

				case NETCOM_4G_ID_UNIT_COMMAND:
					{
						netCommand4G_UnitCommand * pCommand = new netCommand4G_UnitCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM_4G_ID_UNIT_LIST_COMMAND:
					{
						netCommand4G_UnitListCommand * pCommand = new netCommand4G_UnitListCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM_4G_ID_PLAYER_COMMAND:
					{
						netCommand4G_PlayerCommand * pCommand = new netCommand4G_PlayerCommand(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM_4G_ID_EVENT:
					{
						netCommand4G_Event * pCommand = new netCommand4G_Event(in_HostBuf);
						PutGameCommand2Queue_andAutoDelete(pCommand);
					}
					break;
				case NETCOM_4H_ID_BACK_GAME_INFORMATION:
					{
						netCommand4H_BackGameInformation* pEvent= new netCommand4H_BackGameInformation(in_HostBuf);
						//p->second->backGameInfList.push_back(pEvent);
						delete pEvent;
					}
					break;
				case NETCOM_4H_ID_BACK_GAME_INFORMATION_2:
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
				case NETCOM_4H_ID_ALIFE_PACKET:
					{
						netCommand4H_AlifePacket nc(in_HostBuf);
						ud.lastTimeBackPacket = networkTime_th2;
					}
					break;
				case NETCOM_4H_ID_RESPONCE_LAST_QUANTS_COMMANDS:
					{
						//!!!Server!
						xassert(nCState_th2==PNC_STATE__NEWHOST_PHASE_B);
						netCommand4H_ResponceLastQuantsCommands nci(in_HostBuf);
						if(nCState_th2!=PNC_STATE__NEWHOST_PHASE_B) break;

						vector<netCommandGame*> tmpListGameCommands;

						InOutNetComBuffer in_buffer(nci.sizeCommandBuf+1, 1); //проверить необходимость автоувелечения!
						in_buffer.putBufferPacket(nci.pData, nci.sizeCommandBuf);

						while(in_buffer.currentNetCommandID()!=NETCOM_ID_NONE) {
							NCEventID event = (NCEventID)in_buffer.currentNetCommandID();
							switch(event){
							case NETCOM_4G_ID_UNIT_COMMAND: 
								{
									netCommand4G_UnitCommand*  pnc= new netCommand4G_UnitCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM_4G_ID_UNIT_LIST_COMMAND: 
								{
									netCommand4G_UnitListCommand*  pnc= new netCommand4G_UnitListCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM_4G_ID_PLAYER_COMMAND: 
								{
									netCommand4G_PlayerCommand*  pnc= new netCommand4G_PlayerCommand(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM_4G_ID_FORCED_DEFEAT:
								{
									netCommand4G_ForcedDefeat* pnc=new netCommand4G_ForcedDefeat(in_buffer);
									xassert(pnc->curCommandQuant_ < 0xcdcd0000);
									tmpListGameCommands.push_back(pnc);
								}
								break;
							case NETCOM_4G_ID_EVENT:
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
						//Запуск продолжения
						netCommand4C_ContinueGameAfterHostMigrate ncContinueGame;
						SendEventI(ncContinueGame, UNetID::UNID_ALL_PLAYERS);
						//Выполнение команд, которые не у всех были выполнены
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
						//hostGeneralCommandCounter; //уже уставится выше
						quantConfirmation=netCommandNextQuant::NOT_QUANT_CONFIRMATION;
						m_nQuantCommandCounter=0;
						m_numberGameQuant=nci.endQuantCommandTransmit+1;//!выше
						///ClearDeletePlayerGameCommand();
						ClearCommandList();
						//отмена паузы если была
						hostPause=0;
						int playersIdxArr[NETWORK_PLAYERS_MAX];
						for(int m=0; m<NETWORK_PLAYERS_MAX; m++) playersIdxArr[m]=netCommand4C_Pause::NOT_PLAYER_IDX;
						netCommand4C_Pause ncp(playersIdxArr, false);
						SendEventI(ncp, UNetID::UNID_ALL_PLAYERS);

						nCState_th2=NSTATE__HOST_GAME;
					}
					break;
				case NETCOM_4G_ID_CHAT_MESSAGE:
					{
						netCommand4G_ChatMessage nc_ChatMessage(in_HostBuf);
						SendEventI(nc_ChatMessage, UNetID::UNID_ALL_PLAYERS);
					}
					break;
				case EVENT_ID_SERVER_TIME_CONTROL:
					{
						terEventControlServerTime event(in_HostBuf);

	///					m_pGame->SetGameSpeedScale(event.scale, dpnidPlayer);
					}
					break;
				case NETCOM_4H_ID_REJOIN_REQUEST:
					{
						//Опустошение лишней команды(сейчас клиент продолжает посылать ее до NEXT_QUANT)
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
				//Запрещается вызывать currentNetCommandID(т.к. используется ignoreNetCommand)
				in_HostBuf.nextNetCommand();
			}
			break; //for-a
		}
		//if(p==m_clients.end()) //Ни один существующий клиент не соответствует DPNID
		if(k==NETWORK_PLAYERS_MAX){ //Ни один существующий клиент не соответствует DPNID
			while(in_HostBuf.currentNetCommandID()) {
				if(in_HostBuf.currentNetCommandID()==NETCOM_4H_ID_JOIN_REQUEST){
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
					static sDigitalGameVersion hostDGV(true);//создание версии игры
					static sReplyConnectInfo replyConnectInfo;
					if(hostDGV!=clientConnectInfo.dgv){ //Несоответствующая версия игры
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_VERSION, hostDGV);
					}
					else if( clientConnectInfo.flag_quickStart )
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_QS_ERROR, hostDGV);
					else if( nCState_th2!=NSTATE__HOST_TUNING_GAME ) { //if(flag_StartedLoadGame) { // Игра запущена
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_STARTED, hostDGV);
					}
					else if( (!gamePassword.empty()) && (!clientConnectInfo.isPasswordCorrect(gamePassword.c_str())) ){
						replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD, hostDGV);
					}
					else {
						resultIdx=AddClient(clientConnectInfo.perimeterConnectPlayerData, unid);
						if(resultIdx==USER_IDX_NONE)// Игра полная
							replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_FULL, hostDGV);
						else 
                            replyConnectInfo.set(sReplyConnectInfo::CR_OK, hostDGV);
					}
					netCommand4C_JoinResponse ncjrs(replyConnectInfo);
					if(replyConnectInfo.connectResult==sReplyConnectInfo::CR_OK){ //перенос из tmpConnection в основную таблицу
						/*if(!demonwareFull()->connectionApproved(unid, resultIdx)){
							LogMsg("connectionApproved error\n");
                            //deleteClientByDPNID_th3(unid.dpnid(), DPNDESTROYPLAYERREASON_NORMAL);
							deleteUser_thA(unid);
						}
						else */
                            SendEventI(ncjrs, unid, true);
					}
					else {//закрытие соединения
						InOutNetComBuffer tmp(128, true);
						tmp.putNetCommand(&ncjrs);
						//tmp.send(*this, unid, true);
						//demonwareFull()->tmpConnectionBreak(unid, tmp.buffer(), tmp.filled_size);
					}
				}
				else if(in_HostBuf.currentNetCommandID()==NETCOM_4H_ID_REJOIN_REQUEST && nCState_th2==PNC_STATE__NEWHOST_PHASE_A){
					netCommand4H_ReJoinRequest nc(in_HostBuf);
					th2_AddClientToMigratedHost(unid, nc.currentLastQuant, nc.confirmedQuant);
				}
				else if(in_HostBuf.currentNetCommandID()==NETCOM_4H_ID_RESPONCE_LAST_QUANTS_COMMANDS){
					netCommand4H_ResponceLastQuantsCommands nci(in_HostBuf);
				}
				else {
					xassert("Invalid netCommand to host (unknown source)");
					in_HostBuf.ignoreNetCommand();
				}

				in_HostBuf.nextNetCommand();//Завершение обработки комманды
			}
		}
	}while(PutInputPacket2NetBuffer(in_HostBuf, unid)!=0);
}


void PNetCenter::UnLockInputPacket(void)
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

/// !!! педается указатель !!! удаление происходит автоматом после отсылки !!!
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
			//ExecuteInterfaceCommand_thA(NetGEC_HostTerminatedSession); //Отсылается через комманду netCommand4C_DiscardUser
			ExecuteInternalCommand(PNC_COMMAND__END_GAME, false); //т.к. ExecuteInterfaceCommand_thA обрабатывается не сразу(пока нет фокуса не выполняется логический поток) необходимо послать комманду
		}
		else {
			if(isDemonWareMode())
				/*demonwareFull()->removePlayer(*p)*/;
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
				//отсылка сообщения о том, что игрок вышел
				//if(dwReason & DPNDESTROYPLAYERREASON_NORMAL){
				//	ExecuteInterfaceCommand_thA(NetMsg_PlayerExit, clientMissionDescription.usersData[idx].playerNameE);
				//}
				//else {
					ExecuteInterfaceCommand_thA(NetMsg_PlayerDisconnected, clientMissionDescription.usersData[idx].playerNameE);
				//}
			}
			//Удаление игрока из clientMD
			clientMissionDescription.disconnectUser(clientMissionDescription.findUserIdx(cur_unid));
		}
	}
	deleteUsersSuspended.clear();
}

