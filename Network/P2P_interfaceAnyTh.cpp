#include "StdAfx.h"

#include "P2P_interface.h"

#include "LogMsg.h"


//Запускается из 1 2 3-го потока
//Может вызываться с флагом waitExecuted только из одного потока (сейчас 1-го)
bool PNetCenter::ExecuteInternalCommand(e_PNCInternalCommand _ic, bool waitExecuted)
{
	if( WaitForSingleObject(hSecondThread, 0) == WAIT_OBJECT_0) {
		return 0;
	}

	InternalCommand ic(_ic);
	if(waitExecuted) {
		ResetEvent(hCommandExecuted);
		//ic = static_cast<e_PNCInternalCommand>(ic | InternalCommand_FlagWaitExecuted);
	}
	ic.setWaitExecuted(waitExecuted);
	{
		MTAuto _lock(m_GeneralLock);
		internalCommandList.push_back(ic);
	}
	if(waitExecuted){
		::Sleep(0);
		//if(WaitForSingleObject(hCommandExecuted, INFINITE) != WAIT_OBJECT_0) xassert(0&&"Error execute command");
		const unsigned char ha_size=2;
		HANDLE ha[ha_size];
		ha[0]=hSecondThread;
		ha[1]=hCommandExecuted;
		DWORD result=WaitForMultipleObjects(ha_size, ha, FALSE, INFINITE);
		if(result<WAIT_OBJECT_0 || result>= (WAIT_OBJECT_0+ha_size)) {
			xassert(0&&"Error execute command");
		}
	}
	return 1;
}



//Запускается из 2 3-го потока
int PNetCenter::AddClient(ConnectPlayerData& pd, const UNetID& unid)
{
	MTAuto _lock(m_GeneralLock); //В этой функции в некоторых вызовах будет вложенный

	int userIdx=USER_IDX_NONE;
	if(hostMissionDescription.gameType()==GAME_TYPE_MULTIPLAYER || hostMissionDescription.gameType()==GAME_TYPE_MULTIPLAYER_COOPERATIVE){
		userIdx=hostMissionDescription.connectNewUser(pd, unid, networkTime_th2);
	}
	hostMissionDescription.setChanged();
	if(userIdx!=USER_IDX_NONE){
		LogMsg("New client 0x%X() for game %s\n", unid.dpnid(), m_GameName.c_str());
		return userIdx;
	}
	else {
		LogMsg("client 0x%X() for game %s id denied\n", unid.dpnid(), m_GameName.c_str());
		return USER_IDX_NONE;
	}

}

////Запускается из 1-го(деструктор) и 2-го потока
//void PNetCenter::ClearClients()
//{
//	ClientMapType::iterator i;
//	FOR_EACH(m_clients, i)
//		delete *i;
//	m_clients.clear();
//}

//Запускается из 1 и 2-го потока
void PNetCenter::clearInternalFoundHostList(void) 
{
	MTAuto _lock(m_GeneralLock); //В этой функции в некоторых вызовах будет вложенный
	vector<INTERNAL_HOST_ENUM_INFO*>::iterator p;
	for(p=internalFoundHostList.begin(); p!=internalFoundHostList.end(); p++){
		delete *p;
	}
	internalFoundHostList.erase(internalFoundHostList.begin(), internalFoundHostList.end());
}




bool PNetCenter::ExecuteInterfaceCommand_thA(eNetMessageCode _nmc, const char* str)
{
	{
		interfaceCommandList.push_back(sPNCInterfaceCommand(_nmc, str));
	}
	return 1;
}

void PNetCenter::deleteUser_thA(const UNetID& unid) //Internal 2&3Th //, DWORD dwReason
{
	MTAuto _lock(m_GeneralLock);
    deleteUsersSuspended.push_back(unid);
}


