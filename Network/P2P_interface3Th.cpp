#include "StdAfx.h"

#include "P2P_interface.h"
#include "GS_interface.h"

#include "GameShell.h"
#include "Universe.h"

#include "Lmcons.h"

#include "Terra\vmap.h"

#include <algorithm>

#include "ConnectionInfo.h"

#include "LogMsg.h"


HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
	return ((PNetCenter*)pvUserContext)->DirectPlayMessageHandler_th3(dwMessageId, pMsgBuffer);
}



HRESULT PNetCenter::DirectPlayMessageHandler_th3(DWORD dwMessageId, PVOID pMsgBuffer)
{

	MTAuto _lock(m_GeneralLock);
	switch(dwMessageId) {
	case DPN_MSGID_INDICATE_CONNECT:
		{
			DPNMSG_INDICATE_CONNECT* pMsg = (DPNMSG_INDICATE_CONNECT*)pMsgBuffer;

			if(pMsg->dwUserConnectDataSize!=sizeof(sConnectInfo))
				return E_FAIL;
			sConnectInfo clientConnectInfo((unsigned char*)pMsg->pvUserConnectData, pMsg->dwUserConnectDataSize);
			if(!clientConnectInfo.checkOwnCorrect()) 
				return E_FAIL;

			static sDigitalGameVersion hostDGV(true);//создание версии игры
			static sReplyConnectInfo replyConnectInfo;
			pMsg->pvReplyData=&replyConnectInfo;
			pMsg->dwReplyDataSize=sizeof(replyConnectInfo);
			if(hostDGV!=clientConnectInfo.dgv){ //Несоответствующая версия игры
				replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_VERSION, hostDGV);
				return E_FAIL;
			}
			if(flag_StartedLoadGame) { // Игра запущена
				replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_STARTED, hostDGV);
				return E_FAIL;
			}
			if( (!gamePassword.empty()) && (!clientConnectInfo.isPasswordCorrect(gamePassword.c_str())) ){
				replyConnectInfo.set(sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD, hostDGV);
				return E_FAIL;
			}
			int resultIdx=AddClient(clientConnectInfo.perimeterConnectPlayerData, 0);
			if(resultIdx==USER_IDX_NONE) {// Игра полная
				replyConnectInfo.set(sReplyConnectInfo::CR_ERR_GAME_FULL, hostDGV);
				return E_FAIL;
			}

			pMsg->pvPlayerContext=(void*)resultIdx; //для корректного удаления в DPN_MSGID_INDICATED_CONNECT_ABORTED 
			replyConnectInfo.set(sReplyConnectInfo::CR_OK, hostDGV);
		}
		break;

	case DPN_MSGID_INDICATED_CONNECT_ABORTED:
		{
			DPNMSG_INDICATED_CONNECT_ABORTED* pMsg=(DPNMSG_INDICATED_CONNECT_ABORTED*)pMsgBuffer;

			if(isHost()){
				th3_DeleteClientByMissionDescriptionIdx((unsigned int)pMsg->pvPlayerContext);
			}
		}

		break;
    case DPN_MSGID_DESTROY_PLAYER:
		{
			PDPNMSG_DESTROY_PLAYER pDestroyPlayerMsg;
			pDestroyPlayerMsg = (PDPNMSG_DESTROY_PLAYER)pMsgBuffer;

			///NetHandlerProc(pDestroyPlayerMsg->dpnidPlayer, msg);
			//if(isHost()){
			//deleteClientByDPNID_th3(pDestroyPlayerMsg->dpnidPlayer, pDestroyPlayerMsg->dwReason);
			deleteUser_thA(UNetID(pDestroyPlayerMsg->dpnidPlayer));
			//}
			break;
		}
	case DPN_MSGID_CONNECT_COMPLETE:
		{
			DPNMSG_CONNECT_COMPLETE* pMsg = (DPNMSG_CONNECT_COMPLETE*)pMsgBuffer;

			if(pMsg->dwApplicationReplyDataSize==sizeof(sReplyConnectInfo)){
				sReplyConnectInfo hostReplyConnectInfo((unsigned char*)pMsg->pvApplicationReplyData, pMsg->dwApplicationReplyDataSize);
				if(hostReplyConnectInfo.checkOwnCorrect()){
					if( (hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_OK) && ( SUCCEEDED(pMsg->hResultCode)) ){
						//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Ok);
						runCompletedExtTask_Ok(extNetTask_Game);
						return DPN_OK;
					}
					else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_VERSION){
						//ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameNotEqualVersion_Err);
						finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameGameNotEqualVersionErr);
						return DPN_OK;
					}
					else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_STARTED){
						//ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsRun_Err);
						finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameGameIsRunErr);
						return DPN_OK;
					}
					else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_GAME_FULL){
						//ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameIsFull_Err);
						finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameGameIsFullErr);
						return DPN_OK;
					}
					else if(hostReplyConnectInfo.connectResult==sReplyConnectInfo::CR_ERR_INCORRECT_PASWORD){
						//ExecuteInterfaceCommand_thA(NetRC_JoinGame_GameSpyPassword_Err);
						finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameGameIsRunErr);
						return DPN_OK;
					}
				}
				else {
					InterlockedIncrement(&m_nClientSgnCheckError);
				}
			}
			else {
				InterlockedIncrement(&m_nClientSgnCheckError);
			}
			//ExecuteInterfaceCommand_thA(NetRC_JoinGame_Connection_Err);
			finitExtTask_Err(extNetTask_Game, ENTGame::JoinGameConnectionErr);
			return DPN_OK;
		}
		break;

    case DPN_MSGID_CREATE_PLAYER:
		{
			PDPNMSG_CREATE_PLAYER pCreatePlayerMsg;
			pCreatePlayerMsg = (PDPNMSG_CREATE_PLAYER)pMsgBuffer;


			IDirectPlay8Peer* m_pDP;
			//if(m_mode == DP_SERVER) m_pDP=m_pDPServer;
			//else if(m_mode == DP_CLIENT) m_pDP=m_pDPClient;
			m_pDP=m_pDPPeer;

			DPNID dpnid=pCreatePlayerMsg->dpnidPlayer;

			HRESULT hr;
			DWORD dwSize = 0;
			DPN_PLAYER_INFO* pdpPlayerInfo = NULL;

			// Get the peer info and extract its name 
			hr = DPNERR_CONNECTING;

			// GetPeerInfo might return DPNERR_CONNECTING when connecting, 
			// so just keep calling it if it does
			while( hr == DPNERR_CONNECTING )
				hr = m_pDP->GetPeerInfo( dpnid, pdpPlayerInfo, &dwSize, 0 );                                

			if( hr == DPNERR_BUFFERTOOSMALL ) {
				pdpPlayerInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
				if( NULL == pdpPlayerInfo ) {
					hr = E_OUTOFMEMORY;
					goto LErrorReturn;
				}

				ZeroMemory( pdpPlayerInfo, dwSize );
				pdpPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

				hr = m_pDP->GetPeerInfo( dpnid, pdpPlayerInfo, &dwSize, 0 );
				if( SUCCEEDED(hr) ) {
                    if( pdpPlayerInfo->dwPlayerFlags & DPNPLAYER_LOCAL )
                        m_localUNID=UNetID(dpnid);
					if( pdpPlayerInfo->dwPlayerFlags & DPNPLAYER_HOST )
						m_hostUNID=UNetID(dpnid);
					//Дополнительно
					if( (pdpPlayerInfo->dwPlayerFlags&DPNPLAYER_LOCAL)==0 && isHost()){//Кривоватое условие
						th3_setDPNIDInClientsDate((unsigned int)pCreatePlayerMsg->pvPlayerContext, dpnid);
					}

				}

				hr=S_OK;
			}
LErrorReturn:
			//SAFE_DELETE_ARRAY( pdpPlayerInfo );
			if(pdpPlayerInfo) delete [] pdpPlayerInfo;
			return hr;

			break;
		}
	case DPN_MSGID_CREATE_GROUP:
		{
			DPNMSG_CREATE_GROUP* pMsg = (DPNMSG_CREATE_GROUP*)pMsgBuffer;
			///m_dpnidGroupCreating = pMsg->dpnidGroup;
		}
		break;
	case DPN_MSGID_ENUM_HOSTS_QUERY:
		{
			DPNMSG_ENUM_HOSTS_QUERY* pMsg=(DPNMSG_ENUM_HOSTS_QUERY*)pMsgBuffer;
			//AutoLock!!!
			if(isHost()){
				static sGameStatusInfo cGSInf;
				sGameType gti(hostMissionDescription.gameType());
				gti.setUseMapSetting(hostMissionDescription.useMapSettings());
				cGSInf.setNS(hostMissionDescription.playersMaxEasily(), hostMissionDescription.playersAmount(), 
					hostMissionDescription.interfaceName(), hostMissionDescription.missionGUID(), 
					gti, isGameRun(), 10);
				xassert(sizeof(cGSInf) <= pMsg->dwMaxResponseDataSize);
				pMsg->pvResponseData=&cGSInf;
				pMsg->dwResponseDataSize=sizeof(cGSInf);
			}
		}
		break;
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		{

			DPNMSG_ENUM_HOSTS_RESPONSE* pMsg = (DPNMSG_ENUM_HOSTS_RESPONSE*)pMsgBuffer;

			if(pMsg->pApplicationDescription->guidApplication==guidPerimeterGame){
				vector<INTERNAL_HOST_ENUM_INFO*>::iterator p;
				for(p=internalFoundHostList.begin(); p!=internalFoundHostList.end(); p++){
					if(pMsg->pApplicationDescription->guidInstance == (*p)->pAppDesc->guidInstance){
						if(sizeof(sGameStatusInfo)==pMsg->dwResponseDataSize){
							(*p)->gameStatusInfo=*((sGameStatusInfo*)(pMsg->pvResponseData));
							(*p)->gameStatusInfo.ping=pMsg->dwRoundTripLatencyMS;
							(*p)->timeLastRespond = networkTime_th2;
						}
						else xassert(0&& "Invalid enum host responce!");
						break;
					}
				}
				if(p==internalFoundHostList.end()){ //host not found
					INTERNAL_HOST_ENUM_INFO*  pNewHost=new INTERNAL_HOST_ENUM_INFO(pMsg);
					if(sizeof(sGameStatusInfo)==pMsg->dwResponseDataSize){
						pNewHost->gameStatusInfo=*((sGameStatusInfo*)(pMsg->pvResponseData));
						pNewHost->gameStatusInfo.ping=pMsg->dwRoundTripLatencyMS;
						pNewHost->timeLastRespond = networkTime_th2;
					}
					else xassert(0&& "Invalid enum host responce!");
					internalFoundHostList.push_back(pNewHost);
				}
			}
			break;
		}

    case DPN_MSGID_TERMINATE_SESSION:
		{
			PDPNMSG_TERMINATE_SESSION pTerminateSessionMsg;
			pTerminateSessionMsg = (PDPNMSG_TERMINATE_SESSION)pMsgBuffer;

			//m_mode = DP_NOBODY;
			flag_connectedDP=0;
			flag_StartedLoadGame = false;
			//flag_StartedGame = false;
			if(pTerminateSessionMsg->hResultCode==DPNERR_HOSTTERMINATEDSESSION){
				//dropped
				//ExecuteInterfaceCommand_thA(NetGEC_HostTerminatedSession);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::HostTerminatedSession);
			}
			else if(pTerminateSessionMsg->hResultCode==DPNERR_CONNECTIONLOST){
				//ExecuteInterfaceCommand_thA(NetGEC_ConnectionFailed);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::ConnectionFailed);
			}
			else {
				//Не распознанная ситуация
				//ExecuteInterfaceCommand_thA(NetGEC_ConnectionFailed);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::ConnectionFailed);
			}
			//xassert(0 && "DP connect terminate!");
			break;
		}
	case DPN_MSGID_RECEIVE:
		{
			PDPNMSG_RECEIVE pReceiveMsg;
			pReceiveMsg = (PDPNMSG_RECEIVE)pMsgBuffer;

			{
				m_DPPacketList.push_back(XDPacket());
				m_DPPacketList.back().set(UNetID(pReceiveMsg->dpnidSender), pReceiveMsg->dwReceiveDataSize, pReceiveMsg->pReceiveData);

				//Отлов необходимой передаваемой информации
				//InOutNetComBuffer tmp(2048, true);
				//tmp.putBufferPacket(pReceiveMsg->pReceiveData, pReceiveMsg->dwReceiveDataSize);
				//if(tmp.currentNetCommandID()==NETCOM4C_CurMissionDescriptionInfo){
				//	netCommand4C_CurrentMissionDescriptionInfo ncCMD(tmp);
				//	int color1=ncCMD.missionDescription_.playersData_[0].colorIndex;
				//	int color2=ncCMD.missionDescription_.playersData_[1].colorIndex;
				//}
			}

			///XDP_Message msg=XDPMSG_DateReceive;
			///NetHandlerProc(pReceiveMsg->dpnidSender, msg);

			break;
		}
	case DPN_MSGID_DESTROY_GROUP:
		{
			DPNMSG_DESTROY_GROUP* m=(DPNMSG_DESTROY_GROUP*)pMsgBuffer;
			break;
		}

	case DPN_MSGID_HOST_MIGRATE:
        {
			if((nCState_th2!=PNC_STATE__CLIENT_LOADING_GAME) && (nCState_th2!=PNC_STATE__CLIENT_GAME)){
				//Нужно запустить abort
				//ExecuteInterfaceCommand_thA(NetGEC_HostTerminatedSession);//PNC_INTERFACE_COMMAND_CONNECTION_FAILED);
				finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::HostTerminatedSession);
				break;
			}
			LockInputPacket();

            PDPNMSG_HOST_MIGRATE pHostMigrateMsg = (PDPNMSG_HOST_MIGRATE)pMsgBuffer;


			m_hostUNID=UNetID(pHostMigrateMsg->dpnidNewHost);
			//if( m_hostUNID == m_localUNID )//Host Я
			//	ExecuteInternalCommand(PNC_COMMAND__STOP_GAME_AND_ASSIGN_HOST_2_MY, false);
			//else  // Host не Я
			//	ExecuteInternalCommand(PNC_COMMAND__STOP_GAME_AND_WAIT_ASSIGN_OTHER_HOST, false);
			ExecuteInternalCommand(PNC_COMMAND__STOP_GAME_AND_MIGRATION_HOST, false);

			///NetHandlerProc(pHostMigrateMsg->dpnidNewHost, msg);
            break;
        }
	default:
		{
			switch(dwMessageId){
			case DPN_MSGID_ADD_PLAYER_TO_GROUP	: 
				break;
			case DPN_MSGID_APPLICATION_DESC		: 
				break;
			case DPN_MSGID_ASYNC_OP_COMPLETE	: 
				break;
			case DPN_MSGID_CLIENT_INFO			: 
				break;	
			case DPN_MSGID_CONNECT_COMPLETE		: 
				break;	
			case DPN_MSGID_CREATE_GROUP			: 
				break;	
			case DPN_MSGID_CREATE_PLAYER		: 
				break;		
			case DPN_MSGID_DESTROY_GROUP		: 
				break;		
			case DPN_MSGID_DESTROY_PLAYER		: 
				break;	
			case DPN_MSGID_ENUM_HOSTS_QUERY		: 
				break;	
			case DPN_MSGID_ENUM_HOSTS_RESPONSE	: 
				break;	
			case DPN_MSGID_GROUP_INFO			: 
				break;	
			case DPN_MSGID_HOST_MIGRATE			: 
				break;	
			case DPN_MSGID_INDICATE_CONNECT			: 
				break;
			case DPN_MSGID_INDICATED_CONNECT_ABORTED: 
				break;	
			case DPN_MSGID_PEER_INFO			: 
				break;		
			case DPN_MSGID_RECEIVE				: 
				break;	
			case DPN_MSGID_REMOVE_PLAYER_FROM_GROUP	: 
				break;
			case DPN_MSGID_RETURN_BUFFER			: 
				break;	
			case DPN_MSGID_SEND_COMPLETE		: 
				break;		
			case DPN_MSGID_SERVER_INFO			: 
				break;	
			case DPN_MSGID_TERMINATE_SESSION	: 
				break;		
			// Messages added for DirectX 9
			case DPN_MSGID_CREATE_THREAD		: 
				break;		
			case DPN_MSGID_DESTROY_THREAD		: 
				break;	
			case DPN_MSGID_NAT_RESOLVER_QUERY	: 
				break;	
			}
			int v=0;
		}
		break;

	}

	return S_OK;
}

void PNetCenter::th3_setDPNIDInClientsDate(const int missionDescriptionIdx, DPNID dpnid)
{
	hostMissionDescription.setChanged();
	if(hostMissionDescription.setPlayerUNID(missionDescriptionIdx, UNetID(dpnid)))
		LogMsg("set DPNID client-OK\n");
	else
		LogMsg("set DPNID client-err\n");
}
void PNetCenter::th3_DeleteClientByMissionDescriptionIdx(const int missionDescriptionIdx)
{
	hostMissionDescription.setChanged();

	if(hostMissionDescription.disconnectUser(missionDescriptionIdx))
		LogMsg("OK\n");
	else
		LogMsg("error in missionDescription\n");
}

//void PNetCenter::deleteClientByDPNID_th3(const DPNID dpnid, DWORD dwReason)
//{
//	int uidx;
//	MissionDescriptionNet& mdch = isHost()? hostMissionDescription : clientMissionDescription;
//	uidx=mdch.findUserIdx(UNetID(dpnid));
//	xassert(uidx!=USER_IDX_NONE);
//	if(uidx==USER_IDX_NONE )
//		return;
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
//
//	if(isHost()){
//		LogMsg("Client 0x%X() disconnecting-", dpnid);
//		//if(hostMissionDescription.disconnectPlayer2PlayerDataByDPNID(dpnid))
//		if(hostMissionDescription.disconnectUser(hostMissionDescription.findUserIdx(UNetID(dpnid))))
//			LogMsg("OK\n");
//		else
//			LogMsg("error in missionDescription\n");
//	}
//	else {
//	}
//	if(flag_StartedLoadGame){
//		int idx=clientMissionDescription.findUserIdx(UNetID(dpnid));
//		xassert(idx!=USER_IDX_NONE);
//		if(idx!=USER_IDX_NONE){
//			//отсылка сообщения о том, что игрок вышел
//			if(dwReason & DPNDESTROYPLAYERREASON_NORMAL){
//				ExecuteInterfaceCommand_thA(NetMsg_PlayerExit, clientMissionDescription.usersData[idx].playerNameE);
//			}
//			else {
//				ExecuteInterfaceCommand_thA(NetMsg_PlayerDisconnected, clientMissionDescription.usersData[idx].playerNameE);
//			}
//		}
//		//Удаление игрока из clientMD
//		clientMissionDescription.disconnectUser(clientMissionDescription.findUserIdx(UNetID(dpnid)));
//	}
//
//}


void PNetCenter::LockInputPacket()
{
	flag_LockIputPacket++;
}


PNetCenter::INTERNAL_HOST_ENUM_INFO::INTERNAL_HOST_ENUM_INFO(DPNMSG_ENUM_HOSTS_RESPONSE* pDpn)
{
	timeLastRespond=0;
    pDpn->pAddressSender->Duplicate(&pHostAddr);
    pDpn->pAddressDevice->Duplicate(&pDeviceAddr);

    pAppDesc = new DPN_APPLICATION_DESC;
    ZeroMemory(pAppDesc, sizeof(DPN_APPLICATION_DESC) );
    memcpy(pAppDesc, pDpn->pApplicationDescription, sizeof(DPN_APPLICATION_DESC));
    if(pDpn->pApplicationDescription->pwszSessionName ) {
        pAppDesc->pwszSessionName = new WCHAR[ wcslen(pDpn->pApplicationDescription->pwszSessionName)+1 ];
        wcscpy(pAppDesc->pwszSessionName, pDpn->pApplicationDescription->pwszSessionName );
    }
}
PNetCenter::INTERNAL_HOST_ENUM_INFO::~INTERNAL_HOST_ENUM_INFO()
{
	if(pAppDesc)
	{
		if(pAppDesc->pwszSessionName)
			delete pAppDesc->pwszSessionName;

		delete pAppDesc;
	}
	SAFE_RELEASE(pHostAddr);
	SAFE_RELEASE(pDeviceAddr);
}


