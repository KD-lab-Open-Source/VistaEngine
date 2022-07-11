#ifndef __P2P_INTERFACE_H__
#define __P2P_INTERFACE_H__

#include "EventBufferDP.h"
#include "NetCommands.h"
#include "Starforce.h"

#include "MissionDescriptionNet.h"
#include "FileUtils\XGUID.h" //определение XGUID

#include "ExternalTask.h"
#include "P2P_interfaceAux.h"

const int NORMAL_QUANT_INTERVAL=100;

// {DF006380-BF70-4397-9A18-51133CEEE3B6}
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


enum eNetMessageCode {
	NetMessageCode_NULL=0,
	//NetGEC_ConnectionFailed,
	//NetGEC_HostTerminatedSession,
	//NetGEC_GameDesynchronized,
	//NetGEC_GeneralError,

	//NetGEC_DWLobbyConnectionFailed,
	//NetGEC_DWLobbyConnectionFailed_MultipleLogons,

	//NetRC_Init_Ok,
	//NetRC_Init_Err,

	//NetRC_CreateAccount_Ok,
	//NetRC_CreateAccount_BadLicensed,
	//NetRC_CreateAccount_IllegalOrEmptyPassword_Err,
	//NetRC_CreateAccount_IllegalUserName_Err,
	//NetRC_CreateAccount_VulgarUserName_Err,
	//NetRC_CreateAccount_UserNameExist_Err,
	//NetRC_CreateAccount_MaxAccountExceeded_Err,
	//NetRC_CreateAccount_Other_Err,

	//NetRC_ChangePassword_Ok,
	//NetRC_ChangePassword_Err,

	//NetRC_DeleteAccount_Ok,
	//NetRC_DeleteAccount_Err,

	//NetRC_Configurate_Ok,
	//NetRC_Configurate_ServiceConnect_Err,
	//NetRC_Configurate_UnknownName_Err,
	//NetRC_Configurate_IncorrectPassword_Err,
	//NetRC_Configurate_AccountLocked_Err,

	//NetRC_CreateGame_Ok,
	//NetRC_CreateGame_CreateHost_Err,
	//NetRC_JoinGame_Ok,
	//NetRC_JoinGame_GameSpyConnection_Err,
	//NetRC_JoinGame_GameSpyPassword_Err,
	//NetRC_JoinGame_Connection_Err,
	//NetRC_JoinGame_GameIsRun_Err,
	//NetRC_JoinGame_GameIsFull_Err,
	//NetRC_JoinGame_GameNotEqualVersion_Err,

	//NetRC_QuickStart_Ok,
	//NetRC_QuickStart_Err,

	//NetRC_ReadStats_Ok,
	//NetRC_ReadStats_Err,

	//NetRC_WriteStats_Ok,
	//NetRC_WriteStats_Err,

	//NetRC_ReadGlobalStats_Ok,
	//NetRC_ReadGlobalStats_Err,

	//NetRC_LoadInfoFile_Ok,
	//NetRC_LoadInfoFile_Err,

	//NetRC_Subscribe2ChatChanel_Ok,
	//NetRC_Subscribe2ChatChanel_Err,

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
	PNC_COMMAND__STOP_GAME_AND_MIGRATION_HOST = 10,

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


enum e_PNCState {
	// Non using !
	PNC_State_Host =	0x80,
	PNC_State_Client =	0x40,
	PNC_State_GameRun = 0x10,
	PNC_State_QuickStart = 0x20,

	//Using
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

struct ChatChanelInfo {
	unsigned __int64 id;
	string name;
	int numSubscribers;
	int maxSubscribers;
};
struct ChatMemberInfo {
	__int64 id;
	string name;
};

class PNetCenter {
public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// EXTERNAL INTERFACE
	e_PNCState getState(){ return nCState_th2; }

	static void startMPWithoutInterface(const char* missionName);
	static void startDWMPWithoutInterface(const char* missionName);
	STARFORCE_API PNetCenter(ExternalNetTask_Init* entInit);//e_PNCWorkMode _workMode
	//void login2Server(const char* playerName=0, const char* password = 0, const char* InternetAddress=0);
	~PNetCenter();

	void getGameHostList(vector<sGameHostInfo>& ghl); 
	void JoinCommand(int commandID);//cooperative only!
	void KickInCommand(int commandID, int cooperativeIdx); //cooperative only!

	void FinishGame();
	void ResetAndStartFindHost();

	void implementingENT(ENTCreateAccount* _entCreateAccount);
	void implementingENT(ENTDeleteAccount * _entDeleteAccount);
	void implementingENT(ENTChangePassword* _entchangePasswoed);
	void implementingENT(ENTLogin* _entLogin);
	void logout();
	void implementingENT(ENTDownloadInfoFile* _entDownloadInfoFile);
	void implementingENT(ENTReadGlobalStats* _entReadGlobalStats);
	void implementingENT(ENTSubscribe2ChatChannel* _entSubscribe2ChatChannel);
	void unsub2ChatChannel(unsigned __int64 chanelId);

	void implementingENT(ENTGame* _entGame);
	void readStats(ScoresID scoresID, Scores& scores, const char* userName);
	void addStats(ScoresID scoresID, Scores& scores);


	MissionDescription& getCurrentMissionDescription(){ return curMD; };
	//e_PNCWorkMode getWorkMode() const { return workMode; }
	bool isDemonWareMode() const { return workMode==PNCWM_LAN_DW || workMode==PNCWM_ONLINE_DW || workMode==PNCWM_ONLINE_P2P; }

	void ServerTimeControl(float scale);
	void changePlayerRace(int slotID, Race newRace);
	void changePlayerColor(int slotID, int newColorIndex);
	void changePlayerSign(int slotID, int newSign);
	void changeRealPlayerType(int slotID, RealPlayerType newRealPlayerType);
	void changePlayerDifficulty(int slotID, Difficulty difficulty);
	void changePlayerClan(int slotID, int clan);
	//void changeMap(const char* missionName);
	void changeMissionDescription(MissionDescriptionNet::eChangedMDVal val, int v);

	bool plyaerIsReadyOrStartLoadGame();
	void setGameIsReady();

	//Pause client
	bool setPause(bool pause);
	bool isPause(){ return clientPause; }

	void SendEvent(const NetCommandBase* event);
	void SendEventSync(const NetCommandBase* event);

	//Chat
	bool chatMessage(const class ChatMessage& chatMsg);
	void setGameHostFilter(int gameTypeFilter, std::vector<XGUID> missionFilter, int maxPlayersFilter);
	void immediatelyRefreshGameHostList();

	//virtual void setGameHostFilter(struct sGameHostFilter& ghf)=0;
	void getChatChanelList(vector<ChatChanelInfo>& ccl);
	static const char* mainChannel() { return "GeneralA"; }
	void getChatMembers(vector<ChatMemberInfo>& cml);

	const char* getMyPublicIP();
	// END EXTERNAL INTERFACE
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Info
	const char* getStrWorkMode();
	const char* getStrState();
	const char* getStrInternalCommand(InternalCommand curInternalCommand);
	const char* getStrNetMessageCode(eNetMessageCode mc);

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
	e_perimeterGameState getGameState(){
		switch(nCState_th2){
		case NSTATE__HOST_TUNING_GAME:	return PGS_OPEN_WAITING;
		case NSTATE__HOST_GAME:			return PGS_CLOSED_PLAYING;
		default:						return PGS_CLOSED_PLAYING;
		}
	}
	bool isPassword(){
		if(gamePassword.empty()) return false;
		else return true;
	}
	//End GameSpy Interface
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	bool isHost(){ return (nCState_th2 & PNC_State_Host)!=0 ? true : false; }
	bool isClient(){ return (nCState_th2 & PNC_State_Client)!=0 ? true : false; }

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

	void setGameDesynchronized(){ if(extNetTask_Game) finitExtTask_Err(extNetTask_Game, ENTGame::ErrCode::GameDesynchronized);} //для хоста десинхронизация высталяется раньше
	static void createNetCenter(ExternalNetTask_Init* entInit);// { destroyNetCenter(); netCenter = new PNetCenter(entInit); }
	void stopNetCenter();// { stopNetCenterSuspended = true;	if(universeX()) universeX()->stopNetCenter(); }
	static bool isNCCreated() { return netCenter!=0; }
	static PNetCenter* instance() { xassert(netCenter); return netCenter; }
	static bool isNCConfigured(e_PNCWorkMode _workMode){ if(isNCCreated() && instance()->workMode==_workMode ) return true; return false;  }

	static void netQuant();// { if(netCenter) { netCenter->quant_th1(); if(stopNetCenterSuspended)destroyNetCenter(); } }
	static void destroyNetCenter();// { delete netCenter; netCenter=0; stopNetCenterSuspended = false; }
protected:
	static PNetCenter* netCenter;
	static bool stopNetCenterSuspended;

	class ExternalNetTask_Init* extNetTask_Init;
	class ENTGame* extNetTask_Game;

	void resetAllVariable_th1();
	void refreshLanGameHostList_th1();
	bool SecondThread_th2(); //Internal - start 2Th

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
	string gameHostIPAndPort;

	list<NetCommandBase*> m_CommandList;
	list<netCommand4G_ForcedDefeat*> m_DeletePlayerCommand;
	void ClearCommandList();
	void ClearDeletePlayerGameCommand();

	void sendStartLoadGame2AllPlayers_th2(const XBuffer& auxdata); //Internal 2Th
	void th2_UpdateCurrentMissionDescription4C(); //Internal 2Th
	void th2_CheckClients(); //Internal 2Th
	void th2_DumpClients();  //Internal 2Th
	void resetAllClients_th2(); //Internal 2Th

	//Host !!!
	void SendEventI(NetCommandBase& event, const UNetID& unid, bool flag_guaranted=1); //Internal 2Th
	void PutGameCommand2Queue_andAutoDelete(netCommandGame* pCommand); //Internal 2Th (Для 3Th пока неиспользуется)
	void putNetCommand2InClientBuf_th2(NetCommandBase& event);// Используется DW для того чтоб положить чат команду клиенту


	int AddClient(ConnectPlayerData& pd, const UNetID& unid, bool flag_quickStart=false);//Internal 2&3Th

	void th3_setDPNIDInClientsDate(const int missionDescriptionIdx, DPNID dpnid); //Internal 3Th
	void th3_DeleteClientByMissionDescriptionIdx(const int missionDescriptionIdx);//Internal 3Th
	//void deleteClientByDPNID_th3(const DPNID dpnid, DWORD dwReason); //Internal 3Th
	vector<UNetID> disconnectUsersSuspended;
	vector<UNetID> deleteUsersSuspended;
	void discardUser_th2(const UNetID& unid);
	void deleteUser_thA(const UNetID& unid); //Internal 2&3Th //, DWORD dwReason
	void deleteUserQuant_th2(); //Internal 2Th

	void th2_LLogicQuant(); //Internal 2Th //Основной обработчик команд
	void th2_SaveLogByDesynchronization(vector<BackGameInformation2>& firstList, vector<BackGameInformation2>& secondList);


	bool flag_SkipProcessingGameCommand;//Нужно при миграции Host-а
	unsigned int flag_LockIputPacket;

	void LockInputPacket();
	void UnLockInputPacket();

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

	bool isGameRun(){
		if(nCState_th2 & PNC_State_GameRun) return 1;
		else return 0;
	}

	class GameSpyInterface* gameSpyInterface;

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
	bool InitDP();
	int ServerStart(const char* _name, int port);
	GUID getHostGUIDInstance();
	void SetConnectionTimeout(int ms);
	int GetConnectionTimeout();
	void RemovePlayerDP(const UNetID& unid);
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
	void StopFindHostDP();

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
	void clearInternalFoundHostList();

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

private:
	void createGame(const char* gameName, const MissionDescription* md, const char* playerName, Race race, float gameSpeed=1.0f, const char* password="");
	void joinGame(GUID _gameHostID, const char* playerName, Race race);
	void joinGameP2P(const char* _gameHostIPAndPort, const char* playerName, Race race);
	//void JoinGame(const char* strIP, const char* playerName, Race race, unsigned int color, const char* password="");
	void quickStartThroughDW(const char* playerName, int race, eGameOrder gameOrder, const std::vector<XGUID>& missionFilter);

	friend DWORD WINAPI InternalServerThread(LPVOID lpParameter);
	friend class DWInterface;
	friend StartGameParam;
	friend class DWSessionManager;// прямое обращение к extNetTaskCreateGame
};

bool checkInetAddress(const char* ipStr);

DWORD WINAPI InternalServerThread(LPVOID lpParameter);
HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);

extern const GUID guidPerimeterGame;

static const char* COMMAND_LINE_MULTIPLAYER_HOST="mphost";
static const char* COMMAND_LINE_MULTIPLAYER_CLIENT="mpclient";
static const char* COMMAND_LINE_PLAY_REEL="reel";
static const char* COMMAND_LINE_DW_HOST="dwhost";
static const char* COMMAND_LINE_DW_CLIENT="dwclient";

#endif //__P2P_INTERFACE_H__
