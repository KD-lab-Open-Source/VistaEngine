#ifndef __UI_NET_CENTER_H__
#define __UI_NET_CENTER_H__

#include "UI_Enums.h"
#include "..\Game\GlobalStatistics.h"

class Archive;
struct sGameHostInfo;
struct ChatChanelInfo;
class MissionDescription;
enum ScoresID;

typedef unsigned __int64 ChatChannelID;

struct ShowStatisticType;
typedef vector<ShowStatisticType> ShowStatisticTypes;

typedef std::vector<std::string> ComboStrings;

class UI_NetCenter
{
	mutable MTSection lock_;
public:
	enum NetType{
		LAN,
		ONLINE
	};

	UI_NetCenter();
	~UI_NetCenter();

	/// ������ ������ ������� ����
	bool isNetGame() const;
	/// ��� ��������� ���� (host)
	bool isServer() const;

	/// ���� ��������� ����
	bool gameSelected() const;
	/// ���� ��������
	bool gameCreated() const { return gameCreated_; }

	/// ���������� ������
	void setPassword(const char* pass);
	/// ���������� ��������� ������
	void setPass2(const char* pass);
	/// �������� ������
	void resetPasswords();

	/// ������ ���������� ��������� �������
	UI_NetStatus status() const { return netStatus_; }
	/// �������� ���������� ���������� �������
	bool acyncEventWaiting() const { return netStatus_ == UI_NET_WAITING; }
	/// ���������� ������ ������� ����������� ������� (��������� ����������), �������� � ������� ����
	void commit(UI_NetStatus status);

	/// ������� ������� ����������
	void create(NetType type);
	
	/// ������� �������� ������ ���
	void refreshGameList();
	/// �������������� ��������� �������� ������ � ������� ��������
	bool updateGameList();
	/// ������� ���� �� �������� ������
	void selectGame(int idx);
	/// ������� ������ ����� � ��������� ������� ��������� ���
	int getGameList(const GameListInfoTypes& infoType, ComboStrings& strings, const sColor4c& started);
	/// ���������� ���� ��������� ������� ����
	const MissionDescription& selectedGame() const { return selectedGame_; }
	/// ��� ��������� ����
	const char* selectedGameName() const;
	
	void login();
	void logout();
	void quickStart();
	void createAccount();
	void changePassword();
	void deleteAccount();

	bool canCreateGame() const;
	/// ������� ������ ������� ����
	void createGame();
	bool canJoinGame() const;
	/// �������������� � ��������� ������� ����
	void joinGame();
	/// ���������� ������ ����
	void startGame();

	const char* natType() const;
	
	/// �������� ������� ��������
	void abortCurrentOperation();
	
	/// �������� ������� ����, ������ ������ �����
	void reset(bool flag_internalReset=false);
	/// ��������� ��������� ������ � �����
	void release();

	void teamConnect(int teemIndex);
	void teamDisconnect(int teemIndex, int cooperativeIndex);

	/// ��������� ������ ��� �������
	void setChatString(const char* stringForSend);
	/// �������� ������
	bool sendChatString(int rawIntData);
	/// ���������� ��������� ��������� ���� �� ����
	void handleChatString(const char* str);
	/// �������� ������ ��������� ���� � ������� ��������� �������
	void getChatBoard(ComboStrings &board) const;
	/// �������������������� ���
	void resetChatBoard(bool unsubscribe = true);
	void clearChatBoard();
	
	/// ������� �� ������ �������� ������ ������ ������ ���� � ������� � ����
	void refreshChatChannels();
	/// ���������������� ��� � ������� ��������
	void updateChatChannels();
	/// �������� ������ ������ ����
	int getChatChannels(ComboStrings& channels) const;
	/// �������� ��� ������� ��������� ������� ����
	void getCurrentChatChannelName(string& name) const;
	/// ������� ������� � ����
	void selectChatChannel(int channel);
	/// ����� � ��������� ������� � ����
	void enterChatChannel(ChatChannelID forceID = -1);
	/// ������ �������� ����� ��������� � ���
	bool autoSubscribeMode() const { return autoSubscribeMode_; }

	void chatSubscribeOK();
	void chatSubscribeFailed();

	void updateChatUsers();
	int getChatUsers(ComboStrings &users) const;

	void queryGlobalStatistic(bool force = false);
	void getGlobalStatisticFromBegin();
	void getAroundMeGlobalStats();
	bool canGetPrevGlobalStats() const;
	void getPrevGlobalStats();
	bool canGetNextGlobalStats() const;
	void getNextGlobalStats();
	int getGlobalStatistic(const ShowStatisticTypes& format, ComboStrings &board) const;
	void selectGlobalStaticticEntry(int idx);
	int getCurrentGlobalStatisticValue(StatisticType type);
	
	void setPausePlayerList(const ComboStrings& playerList);
	void getPausePlayerList(ComboStrings& playerList) const;
	bool isOnPause() const { return isOnPause_; }

	void updateFilter();

private:
	void clear();
	void autoSubscribeChatChannel();

	UI_NetStatus netStatus_;

	enum DelayOperationType{
		DELAY_NONE,
		CREATE_GAME,
		ONLINE_CREATE_LOGIN,
		ONLINE_NEW_LOGIN,
		ONLINE_DELETE_LOGIN,
		ONLINE_CHECK_VERSION,
		QUERY_GLOBAL_STATISTIC
	};
	DelayOperationType delayOperation_;

	typedef vector<sGameHostInfo> GameHostInfos;
	GameHostInfos netGames_;
	int selectedGameIndex_;

	sGameHostInfo& selectedGameInfo_;
	MissionDescription& selectedGame_;

	bool gameCreated_;
	bool onlineLogined_;

	string password_;
	string pass2_;

	//typedef vector<ChatChanelInfo> ChatChanelInfos; //@Hallkezz
	//ChatChanelInfos chatChanelInfos_; //@Hallkezz
	ChatChannelID subscribedChannel_;
	ChatChannelID subscribeWaitingChannel_;
	ChatChannelID selectedChannel_;
	bool autoSubscribeMode_;
	//bool autoSubscribeUnsusseful_; //@Hallkezz
	//bool lastChannelSubscribeAttemptMode_; //@Hallkezz
	ChatChannelID lastSubscribeAttempt_;
	string currentChatChannelName_;

	string currentChatString_;
	
	ComboStrings chatUsers_;
	ComboStrings chatBoard_;

	ComboStrings pausePlayerList_;
	bool isOnPause_;

	void setStatus(UI_NetStatus status);

	int currentStatisticFilterRace_;
	int currentStatisticFilterGamePopulation_;
	int firstScorePosition_;
	GlobalStatistics gsForRead_;
	GlobalStatistics globalStatistics_;
	int selectedGlobalStatisticIndex_;
	void applyNewGlobalStatistic(const GlobalStatistics& stats);
	XBuffer version_;
	void queryGameVersion();
	bool setGameVersion();
};

#endif // __UI_NET_CENTER_H__