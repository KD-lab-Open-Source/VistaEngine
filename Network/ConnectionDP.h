#ifndef __CONNECTIONDP_H__
#define __CONNECTIONDP_H__

#include "NetPlayer.h"

#define PERIMETER_DEFAULT_PORT      0x6501

const unsigned int MAX_TIME_INTERVAL_HOST_RESPOND=10000;
const unsigned int ENUMERATION_HOST_RETRY_INTERVAL=400;

////////////////////////////////////////////////////////////////////////
// XDPacket

struct XDPacket
{
	int   size;
	unsigned char* buffer;
	UNetID unid;

	__forceinline XDPacket(){
		size = 0;
		buffer = 0;
		unid.setEmpty();
	}
	__forceinline XDPacket(const UNetID& _unid, int _size, const void* cb){
		unid=_unid;
		size= _size;
		buffer = new unsigned char[size];
		memcpy(buffer, cb, size);
	}
	__forceinline ~XDPacket(){
		if(buffer)
			delete buffer;
	}
	__forceinline void set(const UNetID& _unid, int _size, const void* cb){
		unid=_unid;
		size = _size;
		if(buffer) delete buffer;
		buffer = new unsigned char[size];
		memcpy(buffer, cb, size);
	}
	void create(const UNetID& _unid, int _size){
		unid=_unid;
		size = _size;
		if(buffer) delete buffer;
		buffer = new unsigned char[size];
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// XDPConnection

void XDPInit();
void XDPClose();

//#define XDP_DPNID_PLAYER_GENERAL 0xffffffff

///typedef void (*PFNEVENTPROC)(DPNID, void* pData, XDP_Message msg);

/*
class XDPConnection
{
	//directplay server stuff
	//IDirectPlay8Server* m_pDPServer;
	DPNID               m_dpnidGroupCreating;

	//directplay client stuff
	//IDirectPlay8Client*  m_pDPClient;


	///CRITICAL_SECTION     m_csLock;
	CRITICAL_SECTION     m_csLockReSend;
	CRITICAL_SECTION     m_csLockInternalQueue;

	CRITICAL_SECTION     m_LockHostList;



	typedef list<XDPacket> PacketListType;
	typedef hash_map<DPNID, PacketListType> DPQueueType;

	DPQueueType m_internal_queue;

	void FindHost(const char* lpszHost);

public:


	//enum DPMode {
	//	DP_NOBODY=0,
	//	DP_CLIENT, 
	//	DP_SERVER
	//};

	//DPMode   m_mode;

	XDPConnection(PFNDPNMESSAGEHANDLER pfn, void* pData);
	~XDPConnection();

	bool Init();
	bool StartFindHost(const char* lpszHost);
	void StopFindHost();


	int ServerStart(const char* _name, int port);
	//int Started();

	void SetConnectionTimeout(int ms);
	int GetConnectionTimeout();

	GUID getHostGUIDInstance();

	//int Connect(int ip, int port);
	int Connect(GUID hostID);//const char* lpszHost, int port);
	int Connect(unsigned int ip);//, int port

	int Send(const char* buffer, int size, DPNID dpnid);
	int Receive(char* buffer, int size, DPNID dpnid);

	void InitPlayerQueue(DPNID dpnid);
	//void GetActivePlayers(list<DPNID>& players);
	void RemovePlayer(DPNID dpnid);

	//DPNID CreateGroup();
	//void DestroyGroup(DPNID dpnid);
	//void AddPlayerToGroup(DPNID group, DPNID player);
	//void DelPlayerFromGroup(DPNID group, DPNID player);

	void SetServerInfo(void* pb, int sz);
	int  GetServerInfo(void* pb);

	bool GetConnectionInfo(DPN_CONNECTION_INFO& info);

	void Close();

	///HRESULT DirectPlayMessageHandler(DWORD dwMessageId, PVOID pMsgBuffer);





	//void refreshGroupMember();
	//list<DPNID> gameGroupMemberList;

};
*/


#endif //__CONNECTIONDP_H__
