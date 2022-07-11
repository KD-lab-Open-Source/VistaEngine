#include "StdAfx.h"

#include "P2P_interface.h"
#include "LogMsg.h"

#include <Winsock2.h>

///////////////////////////////////
#define caseR(a) case a: return #a;
const char* PNetCenter::getStrWorkMode()
{
	switch(workMode){
	caseR(PNCWM_LAN);
	caseR(PNCWM_ONLINE_GAMESPY);
	caseR(PNCWM_LAN_DW);
	caseR(PNCWM_ONLINE_DW);
	caseR(PNCWM_ONLINE_P2P);
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
	//caseR(PNC_COMMAND__STOP_GAME_AND_ASSIGN_HOST_2_MY);
	//caseR(PNC_COMMAND__STOP_GAME_AND_WAIT_ASSIGN_OTHER_HOST);
	caseR(PNC_COMMAND__STOP_GAME_AND_MIGRATION_HOST);

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
		//caseR(NetGEC_ConnectionFailed);
		//caseR(NetGEC_HostTerminatedSession);
		//caseR(NetGEC_GameDesynchronized);
		//caseR(NetGEC_GeneralError);

		//caseR(NetGEC_DWLobbyConnectionFailed);
		//caseR(NetGEC_DWLobbyConnectionFailed_MultipleLogons);
		//NetGEC_DemonWareTerminatedSession);

		//caseR(NetRC_Init_Ok);
		//caseR(NetRC_Init_Err);

		//caseR(NetRC_CreateAccount_Ok);
		//caseR(NetRC_CreateAccount_BadLicensed);
		//caseR(NetRC_CreateAccount_IllegalOrEmptyPassword_Err);
		//caseR(NetRC_CreateAccount_IllegalUserName_Err);
		//caseR(NetRC_CreateAccount_VulgarUserName_Err);
		//caseR(NetRC_CreateAccount_UserNameExist_Err);
		//caseR(NetRC_CreateAccount_MaxAccountExceeded_Err);
		//caseR(NetRC_CreateAccount_Other_Err);

		//caseR(NetRC_ChangePassword_Ok);
		//caseR(NetRC_ChangePassword_Err);

		//caseR(NetRC_DeleteAccount_Ok);
		//caseR(NetRC_DeleteAccount_Err);

		//caseR(NetRC_Configurate_Ok);
		//caseR(NetRC_Configurate_ServiceConnect_Err);
		//caseR(NetRC_Configurate_UnknownName_Err);
		//caseR(NetRC_Configurate_IncorrectPassword_Err);
		//caseR(NetRC_Configurate_AccountLocked_Err);

		//caseR(NetRC_CreateGame_Ok);
		//caseR(NetRC_CreateGame_CreateHost_Err);
		//caseR(NetRC_JoinGame_Ok);
		//caseR(NetRC_JoinGame_GameSpyConnection_Err);
		//caseR(NetRC_JoinGame_GameSpyPassword_Err);
		//caseR(NetRC_JoinGame_Connection_Err);
		//caseR(NetRC_JoinGame_GameIsRun_Err);
		//caseR(NetRC_JoinGame_GameIsFull_Err);
		//caseR(NetRC_JoinGame_GameNotEqualVersion_Err);

		//caseR(NetRC_QuickStart_Ok);
		//caseR(NetRC_QuickStart_Err);

		//caseR(NetRC_ReadStats_Ok);
		//caseR(NetRC_ReadStats_Err);

		//caseR(NetRC_WriteStats_Ok);
		//caseR(NetRC_WriteStats_Err);

		//caseR(NetRC_ReadGlobalStats_Ok);
		//caseR(NetRC_ReadGlobalStats_Err);

		//caseR(NetRC_LoadInfoFile_Ok);
		//caseR(NetRC_LoadInfoFile_Err);

		//caseR(NetRC_Subscribe2ChatChanel_Ok);
		//caseR(NetRC_Subscribe2ChatChanel_Err);

		caseR(NetMsg_PlayerDisconnected);
		caseR(NetMsg_PlayerExit);

		default: 
			LogMsg("!!!Unknown eNetMessageCode:%u\n", (unsigned int)mc );
			return ("eNetMessageCode - ???");
	}
}

//////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////

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
		}
		if(i>=len) break;
		i++;
	}
	if(counterOk) return true;
	else return false;
}

