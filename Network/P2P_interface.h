#ifndef __P2P_INTERFACE_H__
#define __P2P_INTERFACE_H__

#include "ConnectionDP.h"
#include "EventBufferDP.h"
#include "CommonEvents.h"
#include "Starforce.h"

#include "MissionDescriptionNet.h"
#include "GUIDSerialize.h" //определение XGUID

#define _DBG_COMMAND_LIST

const int NORMAL_QUANT_INTERVAL=100;

// {DF006380-BF70-4397-9A18-51133CEEE3B6}
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

bool checkInetAddress(const char* ipStr);

DWORD WINAPI InternalServerThread(LPVOID lpParameter);
HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);

extern LPCTSTR lpszSignatureRQ;
extern LPCTSTR lpszSignatureRPL;

extern const GUID guidPerimeterGame;


enum eGameOrder {
	GameOrder_1v1=2,
	GameOrder_2v2=4,
	GameOrder_3v3=6,
	GameOrder_Any = 0,
	GameOrder_NoFilter=-1
};

struct sGameType {
	sGameType(){ gameType = GAME_TYPE_BATTLE; }
	sGameType(int _gameType){
		gameType=(GameType)_gameType;
		xassert((gameType&GameType_MASK) >=GAME_TYPE_SCENARIO && (gameType&GameType_MASK)<=GAME_TYPE_BATTLE);
		if(!isUseMapSetting()) { xassert(isBattle());; }
		if(isCooperative()) { xassert(isMultiPlayer());; }
		if(isMultiPlayer()) { xassert(isBattle());; }
	}
	GameType gameType;
	bool operator ==( const GameType gt){ return (gameType&GameType_MASK) == gt; }
	bool isScenario() const { return (gameType&GameType_MASK)==GAME_TYPE_SCENARIO;}
	bool isBattle() const { return (gameType&GameType_MASK)==GAME_TYPE_BATTLE; }

	bool isReel() const { return gameType & GAME_TYPE_REEL; }
	//bool isRandomScenario() const { return gameType&GameType_RandomScenario; }
	bool isUseMapSetting() const { return gameType&GameType_UseMapSettings; }
	bool isMultiPlayer() const { return gameType&GameType_Multiplayer; }
	bool isCooperative() const { xassert(isMultiPlayer()); return gameType&GameType_Cooperative; }
	//void setRandomScenario() { gameType=(GameType)((int)gameType|GameType_RandomScenario); }
	void setUseMapSetting(bool val=true) { 
		if(val) gameType=(GameType)((int)gameType|GameType_UseMapSettings); 
		else gameType=(GameType)( (int)gameType & (~GameType_UseMapSettings) ); 
	}
	void setMultiPlayer() { gameType = (GameType)((int)gameType|GameType_Multiplayer); }
	void setCooperative() { xassert(isMultiPlayer()); gameType=(GameType)((int)gameType|GameType_Cooperative); }
	void reset2OnlineType() {}
};

const GUID ZERO_GUID = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

struct sGameStatusInfo {
	sGameStatusInfo(){
		//set(0,0, false, "", ZERO_GUID, sGameType(), false);
		setNS(0,0, "", ZERO_GUID, sGameType());
	}
	void setNS(char _maxPlayers, int _curPlayers, const char* _missionName, const GUID& _missionGuid, 
		const sGameType& sgt, bool _flag_gameRun=false, int _ping=10){
			flag_quickStart=false;
			maximumPlayers=_maxPlayers;
			currrentPlayers=_curPlayers;
			strncpy(missionName_, _missionName, sizeof(missionName_));
			missionName_[sizeof(missionName_)-1]='\0';
			missionGuid=_missionGuid;
			gameOrder=GameOrder_Any;
			jointGameType=sgt;
			flag_gameRun=_flag_gameRun;
			ping=_ping;
	}

	void setQS(char _maxPlayers, int _curPlayers, eGameOrder go, __int64 _filterMap, int _ping=10){
			flag_quickStart=true;
			maximumPlayers=_maxPlayers;
			currrentPlayers=_curPlayers;
			//strncpy(missionName_, _missionName, sizeof(missionName_));
			//missionName_[sizeof(missionName_)-1]='\0';
			missionName_[0]='\0';
			//missionGuid=_missionGuid;
			filterMap=_filterMap;
			//jointGameType=sGameType();
			gameOrder=go;
			flag_gameRun=false;
			ping=_ping;
	}

	const char* missionName() const {
		return missionName_;
	}
	bool flag_quickStart;
	char maximumPlayers;
	char currrentPlayers;
	char missionName_[32];
	union {
		GUID missionGuid;
		__int64 filterMap;
	};
	eGameOrder gameOrder;
	sGameType jointGameType;
	bool flag_gameRun;
	int ping;
};


class bdMatchMakingInfo;
struct sGameHostInfo {
	GUID gameHostGUID;
	string hostName;
	string portStr;
	string gameName;
	sGameStatusInfo gameStatusInfo;
	sGameHostInfo(){
		GUID guid = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
		set(guid, "", "", "", sGameStatusInfo());
	}
	sGameHostInfo(GUID _gameHostID, const char * _hostName, const char * _port, const char * _gameName, const sGameStatusInfo& gsi){
		set(_gameHostID, _hostName, _port, _gameName, gsi);
	}
	void set(GUID _gameHostGUID, const char * _hostName, const char * _port, const char * _gameName, const sGameStatusInfo& gsi){
		gameStatusInfo=gsi;
		hostName=_hostName;
		portStr=_port;
		gameName=_gameName;
		gameHostGUID=_gameHostGUID;
	}
};

struct StartGameParamBase {
	enum { NO_FILTER=-1 };
	char quickStart;
	int useMapSettingsFilter;
	std::vector<GUID> missionFilter;
	__int64 filterMap;
	int maxPlayersFilter;
	char gameProtection;
	//char gameOrder;
	eGameOrder gameOrder;
};
class StartGameParam : public StartGameParamBase { //sGameHostFilter 
public:
	StartGameParam(MTSection& mts);
	void reset();
	void setNS(int _useMapSettingsFilter, const std::vector<XGUID>& _missionFilter, int _maxPlayersFilter, char _gameProtection);
	void setQS(eGameOrder _gameOrder, __int64 _filterMap);
	bool isConditionEntry(const sGameStatusInfo& gsi);
	StartGameParamBase getGameParam();
private:
	MTSection& mtLock;
};

struct QSStateAndCondition {
	enum { MAX_QSUSERS = 6 };
	eGameOrder gameOrder;
	struct QSUserState {
		bool flag_userConnected;
		UNetID unid;
		//int race;
		//__int64 lowFilterMap;
		//__int64 highFilterMap;
		ConnectPlayerData connectPlayerData;
	};
	QSUserState qsUserState[MAX_QSUSERS];

	QSStateAndCondition(){ gameOrder=GameOrder_1v1; clear(); }
	void clear(){ for(int i=0; i<MAX_QSUSERS; i++) qsUserState[i].flag_userConnected=false; }
	int addPlayers(ConnectPlayerData _connectPlayerData, const UNetID& unid);	
	bool removeUser(const UNetID& unid);
	bool isConditionRealized();
	__int64 getTotalFilterMap();
};


enum eNetMessageCode {
	NetMessageCode_NULL=0,
	NetGEC_ConnectionFailed,
	NetGEC_HostTerminatedSession,
	NetGEC_GameDesynchronized,
	NetGEC_GeneralError,

	NetGEC_DWLobbyConnectionFailed,
	NetGEC_DWLobbyConnectionFailed_MultipleLogons,
	//NetGEC_DemonWareTerminatedSession,

	NetRC_Init_Ok,
	NetRC_Init_Err,

	NetRC_CreateAccount_Ok,
	NetRC_CreateAccount_BadLicensed,
	NetRC_CreateAccount_IllegalOrEmptyPassword_Err,
	NetRC_CreateAccount_IllegalOrVulgarUserName_Err,
	NetRC_CreateAccount_UserNameExist_Err,
	NetRC_CreateAccount_MaxAccountExceeded_Err,
	NetRC_CreateAccount_Other_Err,

	NetRC_ChangePassword_Ok,
	NetRC_ChangePassword_Err,

	NetRC_DeleteAccount_Ok,
	NetRC_DeleteAccount_Err,

	NetRC_Configurate_Ok,
	NetRC_Configurate_ServiceConnect_Err,
	NetRC_Configurate_UnknownName_Err,
	NetRC_Configurate_IncorrectPassword_Err,
	NetRC_Configurate_AccountLocked_Err,

	NetRC_CreateGame_Ok,
	NetRC_CreateGame_CreateHost_Err,
	NetRC_JoinGame_Ok,
	NetRC_JoinGame_GameSpyConnection_Err,
	NetRC_JoinGame_GameSpyPassword_Err,
	NetRC_JoinGame_Connection_Err,
	NetRC_JoinGame_GameIsRun_Err,
	NetRC_JoinGame_GameIsFull_Err,
	NetRC_JoinGame_GameNotEqualVersion_Err,

	NetRC_QuickStart_Ok,
	NetRC_QuickStart_Err,

	NetRC_ReadStats_Ok,
	NetRC_ReadStats_Empty,
	NetRC_ReadStats_Err,

	NetRC_WriteStats_Ok,
	NetRC_WriteStats_Err,

	NetRC_ReadGlobalStats_Ok,
	NetRC_ReadGlobalStats_Err,

	NetMsg_PlayerDisconnected,
	NetMsg_PlayerExit
};
struct sPNCInterfaceCommand {
	//e_PNCInterfaceCommands icID;
	eNetMessageCode nmc;
	string textInfo;

	sPNCInterfaceCommand(){
		nmc=NetMessageCode_NULL;
	}
	sPNCInterfaceCommand(eNetMessageCode _nmc, const char* str=0){
		nmc=_nmc;
		if(str) textInfo=str;
	}
	sPNCInterfaceCommand(const sPNCInterfaceCommand& donor){
		nmc=donor.nmc;
		textInfo=donor.textInfo;
	}
};

enum e_PNCInternalCommand {
	// Not Using!
	InternalCommand_FlagWaitExecuted = 0x80,
	InternalCommand_MASK = 0x7F,

	//Using
	NCmd_Null = 0,

	PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST = 1,
	PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST = 2,
	NCmd_QuickStart = 3,
	//Client back commands
	PNC_COMMAND__CLIENT_STARTING_LOAD_GAME = 5,
	PNC_COMMAND__CLIENT_STARTING_GAME = 6,
	//Special command
	PNC_COMMAND__STOP_GAME_AND_ASSIGN_HOST_2_MY = 10,
	PNC_COMMAND__STOP_GAME_AND_WAIT_ASSIGN_OTHER_HOST = 11,

	PNC_COMMAND__END = 20,
	PNC_COMMAND__ABORT_PROGRAM = 21,
	
	PNC_COMMAND__END_GAME = 22,
	//New Command
	PNCCmd_Reset2FindHost = 30,
	NCmd_Parking = 31
};
struct InternalCommand {
	explicit InternalCommand() { cc=NCmd_Null; }
	explicit InternalCommand(e_PNCInternalCommand _cc){ cc = _cc; }
	explicit InternalCommand(e_PNCInternalCommand _cc, bool flagWaitExecuted){ cc = _cc; setWaitExecuted(flagWaitExecuted); }
	bool isFlagWaitExecuted(){ return cc & InternalCommand_FlagWaitExecuted; }
	void setWaitExecuted(bool flag) { cc = static_cast<e_PNCInternalCommand>(flag ? (cc|InternalCommand_FlagWaitExecuted) : (cc&(~InternalCommand_FlagWaitExecuted))); }
	InternalCommand& operator = (const e_PNCInternalCommand _cc) { cc=_cc; return *this; }
	bool operator == (const e_PNCInternalCommand _cc) const { return (cc&InternalCommand_MASK) == (_cc&InternalCommand_MASK); }
	bool operator != (const e_PNCInternalCommand _cc) const { return (cc&InternalCommand_MASK) != (_cc&InternalCommand_MASK); }
	e_PNCInternalCommand cmd() { return static_cast<e_PNCInternalCommand>(cc & InternalCommand_MASK); }
private:
	e_PNCInternalCommand cc;
};

#define PNC_State_Host (0x80)
#define PNC_State_Client (0x40)
#define PNC_State_GameRun (0x10)
#define PNC_State_QuickStart (0x20)
enum e_PNCState{
	PNC_STATE__NONE=0,
	NSTATE__FIND_HOST	= 1,
	NSTATE__PARKING		= 2,
	PNC_STATE__NET_CENTER_CRITICAL_ERROR = 3,
	// Состояние завершения
	PNC_STATE__ENDING_GAME	= 4,

	NSTATE__QSTART_NON_CONNECT =		PNC_State_QuickStart | 5,
	NSTATE__QSTART_HOSTING_CLIENTING =	PNC_State_QuickStart | 6,
	NSTATE__QSTART_HOST =				PNC_State_QuickStart | 7,
	NSTATE__QSTART_CLIENT =				PNC_State_QuickStart | 8,

	PNC_STATE__CONNECTION				= PNC_State_Client | 10,
	PNC_STATE__INFINITE_CONNECTION_2_IP	= PNC_State_Client | 11,
	PNC_STATE__CLIENT_TUNING_GAME		= PNC_State_Client | 12,
	PNC_STATE__CLIENT_LOADING_GAME		= PNC_State_Client | PNC_State_GameRun |13,
	PNC_STATE__CLIENT_GAME				= PNC_State_Client | PNC_State_GameRun |14,

	PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_0	= PNC_State_Client | PNC_State_GameRun |15,
	PNC_STATE__CLIENT_RESTORE_GAME_AFTE_CHANGE_HOST_PHASE_AB= PNC_State_Client | PNC_State_GameRun |16,

	//special state

	NSTATE__HOST_TUNING_GAME	= PNC_State_Host |20,
	NSTATE__HOST_LOADING_GAME	= PNC_State_Host | PNC_State_GameRun |21,
	NSTATE__HOST_GAME			= PNC_State_Host | PNC_State_GameRun |22,

	PNC_STATE__NEWHOST_PHASE_0	= PNC_State_Host | PNC_State_GameRun |23,
	PNC_STATE__NEWHOST_PHASE_A	= PNC_State_Host | PNC_State_GameRun |24,
	PNC_STATE__NEWHOST_PHASE_B	= PNC_State_Host | PNC_State_GameRun |25

};


class GameSpyInterface;
class DWInterface;

enum e_PNCWorkMode{
	PNCWM_LAN,
	PNCWM_LAN_DW,
	PNCWM_ONLINE_GAMESPY,
	PNCWM_ONLINE_DW,
	PNCWM_ONLINE_P2P,
};

class PNetCenter {
public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// EXTERNAL INTERFACE
	const char* getStrWorkMode();
	const char* getStrState();
	const char* getStrInternalCommand(InternalCommand curInternalCommand);
	const char* getStrNetMessageCode(eNetMessageCode mc);
	e_PNCState getState(){ return nCState_th2; }


	STARFORCE_API PNetCenter(e_PNCWorkMode _workMode);
	void Configurate(const char* playerName=0, const char* password = 0, const char* InternetAddress=0);
	~PNetCenter();

	void getGameHostList(vector<sGameHostInfo>& ghl);
	void CreateGame(const char* gameName, const MissionDescription& md, const char* playerName, Race race, unsigned int color, float gameSpeed=1.0f, const char* password="");
	void JoinGame(GUID _gameHostID, const char* playerName, Race race, unsigned int color);
	void JoinGame(const char* strIP, const char* playerName, Race race, unsigned int color, const char* password="");
	void JoinCommand(int commandID);//cooperative only!
	void KickInCommand(int commandID, int cooperativeIdx); //cooperative only!
	void QuickStartThroughDW(const char* playerName, int race, eGameOrder gameOrder, const std::vector<XGUID>& missionFilter);

	void FinishGame(void);
	void ResetAndStartFindHost(void);

	MissionDescription& getCurrentMissionDescription(void){ return curMD; };
	e_PNCWorkMode getWorkMode() const { return workMode; }
	bool isDemonWareMode() const { return workMode==PNCWM_LAN_DW || workMode==PNCWM_ONLINE_DW; }

	void ServerTimeControl(float scale);
	void changePlayerRace(int slotID, Race newRace);
	void changePlayerColor(int slotID, int newColorIndex);
	void changePlayerSign(int slotID, int newSign);
	void changeRealPlayerType(int slotID, RealPlayerType newRealPlayerType);
	void changePlayerDifficulty(int slotID, Difficulty difficulty);
	void changePlayerClan(int slotID, int clan);
	//void changeMap(const char* missionName);
	void changeMissionDescription(MissionDescriptionNet::eChangedMDVal val, int v);

	bool StartLoadTheGame(void);
	void GameIsReady(void);

	//Pause client
	bool setPause(bool pause);
	bool isPause();

	void SendEvent(const NetCommandBase* event);
	void SendEventSync(const NetCommandBase* event);

	//Chat
	void chatMessage(const class ChatMessage& chatMsg);
	void setGameHostFilter(int gameTypeFilter, std::vector<XGUID> missionFilter, int maxPlayersFilter);
	// END EXTERNAL INTERFACE
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//GameSpy Interface
	void reEnumPlayers(int IP); //usin only GS_interface
	//Info for GameSpy
	const char* getMissionName();
	const char* getGameName();
	const char* getGameVer();
	int getNumPlayers();
	int getMaxPlayers();
	int getHostPort();
	enum e_perimeterGameState{
		PGS_OPEN_WAITING, PGS_CLOSE_WAITING, PGS_CLOSED_PLAYING
	};
	e_perimeterGameState getGameState(void){
		switch(nCState_th2){
		case NSTATE__HOST_TUNING_GAME:	return PGS_OPEN_WAITING;
		case NSTATE__HOST_GAME:			return PGS_CLOSED_PLAYING;
		default:						return PGS_CLOSED_PLAYING;
		}
	}
	bool isPassword(void){
		if(gamePassword.empty()) return false;
		else return true;
	}
	//End GameSpy Interface
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	bool isHost(void){
		if(nCState_th2 & PNC_State_Host) return 1;
		else return 0;
	}
	bool isClient(void){
		if(nCState_th2 & PNC_State_Client) return 1;
		else return 0;
	}


	void quant_th1();
	HRESULT DirectPlayMessageHandler_th3(DWORD dwMessageId, PVOID pMsgBuffer); // Internal, for dirextX thread
	bool ExecuteInterfaceCommand_thA(eNetMessageCode _nmc, const char* str=0);
	int Send(const char* buffer, int size, const UNetID& dpnid, bool flag_guaranted=1);

	InOutNetComBuffer  in_ClientBuf;
	InOutNetComBuffer  out_ClientBuf;

	InOutNetComBuffer  in_HostBuf;
	InOutNetComBuffer  out_HostBuf;

	UNetID	m_hostUNID; //for info only
	UNetID	m_localUNID; //for info only

protected:
	void resetAllVariable_th1();
	void refreshLanGameHostList_th1();
	bool SecondThread_th2(void); //Internal - start 2Th

	//Internal function & date
	MTSection m_GeneralLock;
	HANDLE hSecondThread;
	list<InternalCommand> internalCommandList;
	InternalCommand currentExecutionInternalCommand;
	HANDLE hSecondThreadInitComplete;
	HANDLE hCommandExecuted;
	list<sPNCInterfaceCommand> interfaceCommandList;

	e_PNCWorkMode workMode; // th safe

	e_PNCState nCState_th2;
	unsigned long nextQuantTime_th2;

	//unsigned char m_amountPlayersInHost; //info only
	//unsigned char m_amountPlayerInDP;  //info only

	MissionDescriptionNet hostMissionDescription;
	MissionDescriptionNet clientMissionDescription; //Only 1

	string m_GameName;
	bool flag_StartedLoadGame;
	//bool flag_StartedGame;

	//for command PNCC_START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST
	ConnectPlayerData internalConnectPlayerData;
	GUID m_gameHostID; //for PNC_COMMAND__CONNECT_2_HOST_AND_STOP_FIND_HOST in joinGame

	list<NetCommandBase*> m_CommandList;
	list<netCommand4G_ForcedDefeat*> m_DeletePlayerCommand;
	void ClearCommandList();
	void ClearDeletePlayerGameCommand();

	void th2_UpdateBattleData(); //Internal 2Th
	void th2_UpdateCurrentMissionDescription4C(); //Internal 2Th
	void th2_CheckClients(); //Internal 2Th
	void th2_DumpClients();  //Internal 2Th
	void resetAllClients_th2(); //Internal 2Th

	//Host !!!
	void SendEventI(NetCommandBase& event, const UNetID& unid, bool flag_guaranted=1); //Internal 2Th
	void PutGameCommand2Queue_andAutoDelete(netCommandGame* pCommand); //Internal 2Th (Для 3Th пока неиспользуется)
	void putNetCommand2InClientBuf_th2(NetCommandBase& event);// Используется DW для того чтоб положить чат команду клиенту


	int AddClient(ConnectPlayerData& pd, const UNetID& unid);//Internal 2&3Th

	void th3_setDPNIDInClientsDate(const int missionDescriptionIdx, DPNID dpnid); //Internal 3Th
	void th3_DeleteClientByMissionDescriptionIdx(const int missionDescriptionIdx);//Internal 3Th
	//void deleteClientByDPNID_th3(const DPNID dpnid, DWORD dwReason); //Internal 3Th
	vector<UNetID> deleteUsersSuspended;
	void deleteUser_thA(const UNetID& unid); //Internal 2&3Th //, DWORD dwReason
	void deleteUserQuant_th2(); //Internal 2Th

	void th2_LLogicQuant(); //Internal 2Th //Основной обработчик команд
	void th2_SaveLogByDesynchronization(vector<BackGameInformation2>& firstList, vector<BackGameInformation2>& secondList);


	bool flag_SkipProcessingGameCommand;//Нужно при миграции Host-а
	unsigned int flag_LockIputPacket;

	void LockInputPacket(void);
	void UnLockInputPacket(void);

	list<XDPacket> m_DPPacketList;
	bool PutInputPacket2NetBuffer(InOutNetComBuffer& netBuf, UNetID& returnUNID);
	void th2_clearInOutClientHostBuffers();

	bool ExecuteInternalCommand(e_PNCInternalCommand ic, bool waitExecuted);
	void th1_HandlerInputNetCommand();//Internal 1Th


	//Second Thread
	bool internalCommandQuant_th2(); //Internal 2Th
	void internalCommandEnd_th2( bool flag_result=true );
	bool th2_AddClientToMigratedHost(const UNetID& _unid, unsigned int _curLastQuant, unsigned int _confirmQuant);  //Internal 2Th

	MissionDescriptionNet curMD; //1t
	void th2_HostReceiveQuant();
	void th2_ClientPredReceiveQuant();
	void quickStartReceiveQuant_th2();


	unsigned int m_quantInterval;
	unsigned int m_originalQuantInterval;

	unsigned int beginWaitTime_th2; //Используеться для измерения времени при миграции

	unsigned int lastTimeServerPacket_th1;

	unsigned int TIMEOUT_CLIENT_OR_SERVER_RECEIVE_INFORMATION; //const
	unsigned int TIMEOUT_DISCONNECT; //const
	unsigned int MAX_TIME_PAUSE_GAME; //const

	bool isGameRun(void){
		if(nCState_th2 & PNC_State_GameRun) return 1;
		else return 0;
	}

	GameSpyInterface* gameSpyInterface;

	//DP Section
    //DPNHANDLE m_hEnumAsyncOp;
    vector<DPNHANDLE> m_hEnumAsyncOp_Arr;
	IDirectPlay8Peer*	m_pDPPeer;

	unsigned long internalIP_DP;
	long m_nClientSgnCheckError;
	bool flag_connectedDP;
	bool flag_NetworkSimulation;
	DWORD m_dwPort;
	bool flag_EnableHostMigrate;
	bool flag_NoUseDPNSVR;
	int m_DPSigningLevel;//0-none 1-fast signed 2-full signed

	//DP interface
	bool InitDP(void);
	int ServerStart(const char* _name, int port);
	GUID getHostGUIDInstance();
	void SetConnectionTimeout(int ms);
	int GetConnectionTimeout(void);
	void RemovePlayer(const UNetID& unid);
	void SetServerInfo(void* pb, int sz);
	int GetServerInfo(void* pb);
	bool GetConnectionInfo(DPN_CONNECTION_INFO& info);
	void Close(bool flag_immediatle=1);
	void TerminateSession();
	int Connect(GUID _hostID); //const char* lpszHost, int port)
	int Connect(unsigned int ip);//, int port
	void StartConnect2IP(unsigned int ip);
	bool QuantConnect2IP();

	bool isConnectedDP();
	bool FindHost(const char* lpszHost);
	bool StartFindHostDP(const char* lpszHost="");
	void StopFindHostDP(void);

	struct INTERNAL_HOST_ENUM_INFO {
		DPN_APPLICATION_DESC* pAppDesc;
		IDirectPlay8Address*  pHostAddr;
		IDirectPlay8Address*  pDeviceAddr;
		sGameStatusInfo gameStatusInfo;
		unsigned int timeLastRespond;

		INTERNAL_HOST_ENUM_INFO(DPNMSG_ENUM_HOSTS_RESPONSE* pDpn);
		~INTERNAL_HOST_ENUM_INFO();
	};
	vector<INTERNAL_HOST_ENUM_INFO*> internalFoundHostList;
	void clearInternalFoundHostList(void);

	vector<string> needHostList;
	vector<sGameHostInfo> gameHostListDP;


	//Host Date
	unsigned int m_numberGameQuant; //Кванты на хосте Кванты считаются с 1-цы!
	int m_nQuantCommandCounter;
	unsigned long hostGeneralCommandCounter;
	unsigned int quantConfirmation;
	UNetID unidClientWhichWeWait; //unid игрока которому хост при миграции посылает команду прислать игровые комманды; нужен чтобы в случае выхода переслать комманду другому

	//Info for GameSpy
	string gamePassword;

	//Network settings
	bool flag_DecreaseSpeedOnSlowestPlayer;

	bool hostPause;
	bool clientPause;
	bool clientInPacketPause;

	unsigned int networkTime_th1;
	unsigned int networkTime_th2;

	StartGameParam startGameParam;
	//for QS
	eGameOrder m_QSGameOrder; //убрать!
	QSStateAndCondition m_qsStateAndCondition;
	void removeUserInQuickStart(const UNetID& unid);

	friend DWORD WINAPI InternalServerThread(LPVOID lpParameter);
	friend DWInterface;
	friend StartGameParam;
};



#endif //__P2P_INTERFACE_H__
