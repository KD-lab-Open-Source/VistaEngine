
#define hash hash_
#define slist slist_

//#include "Peer2\peer\peer.h"

#undef hash
#undef slist

struct sGamePlayerInfo {
	int vvv;
};

class PNetCenter;
class GameSpyInterface {
public:
	GameSpyInterface(PNetCenter* pPNetCenter){};
	~GameSpyInterface(){};

	enum e_connectResult{
		CR_Ok, CR_ConnectErr, CR_NickErr
	};
	e_connectResult Connect(const char* playerName){ return CR_ConnectErr; };

	PNetCenter* m_pPNetCenter;
//	void FillPlayerList(RoomType roomType);
//	SBServer GetCurrentServer();

	////PEER m_peer; //необходимый элемент связи!

	int m_count;
//	CString m_selectedNick;
//	RoomType m_selectedRoom;

	bool quant(){ return false; };
	vector<sGameHostInfo> gameHostList;

	list<sGamePlayerInfo*> gamePlayerList;
	void clearGamePlayerList(){
		list<sGamePlayerInfo*>::iterator p;
		for(p=gamePlayerList.begin(); p!=gamePlayerList.end(); p++) { delete *p; }
		gamePlayerList.erase(gamePlayerList.begin(), gamePlayerList.end());
	}

	static bool IsCreated() { return instance_; }
	static GameSpyInterface* instance() { xassert(instance_); return instance_; }
	static GameSpyInterface* instance_;

	unsigned int uniqueID;
	unsigned int getUniqueID() { return uniqueID++; }

	bool CreateStagingRoom(const char* gameStagingRoomName, const char* password=""){ return false; };
	bool JoinStagingRoom(GUID ID){ return false; };	
	unsigned int getHostIP(GUID ID){ return 0;};

	bool serverListingComplete;

	enum e_JoinStagingRoomResult {
		JSRR_Processing, JSRR_Ok, JSRR_PasswordError, JSRR_Error };
	e_JoinStagingRoomResult result_joinStagingRoom;
	e_JoinStagingRoomResult JoinStagingRoom(unsigned long ip, const char* password=""){ return JSRR_Error; };

	void StartGame(){};
};
