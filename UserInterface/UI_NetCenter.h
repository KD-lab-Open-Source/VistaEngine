#ifndef __UI_NET_CENTER_H__
#define __UI_NET_CENTER_H__

#include "UI_Enums.h"
#include "..\Game\GlobalStatistics.h"

class Archive;
struct sGameHostInfo;
class MissionDescription;
enum ScoresID;

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

	bool isNetGame() const;
	bool isServer() const;

	bool gameSelected() const;;
	bool gameCreated() const { return gameCreated_; };

	void setPassword(const char* pass);
	void setPass2(const char* pass);
	void resetPasswords();

	UI_NetStatus status() const { return netStatus_; }
	bool acyncEventWaiting() const { return netStatus_ == UI_NET_WAITING; }
	void commit(UI_NetStatus status);

	void create(NetType type);
	
	bool updateGameList();
	void selectGame(int idx);
	int getGameList(ComboStrings&  strings, GameListInfoType infoType);
	const MissionDescription& selectedGame() const { return selectedGame_; }
	const char* selectedGameName() const;
	
	void login();
	void quickStart();
	void createAccount();
	void changePassword();
	void deleteAccount();

	bool canCreateGame() const;
	void createGame();
	bool canJoinGame() const;
	void joinGame();
	void startGame();
	
	void abortCurrentOperation();
	
	void reset();
	void release();

	void teamConnect(int teemIndex);
	void teamDisconnect(int teemIndex, int cooperativeIndex);

	void setChatString(const char* stringForSend);
	void sendChatString(int rawIntData);
	void handleChatString(const char* str);
	void getChatBoard(ComboStrings &board) const;
	void clearChatBoard();
	bool updateChatUsers();
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

	UI_NetStatus netStatus_;

	enum DelayOperationType{
		DELAY_NONE,
		CREATE_GAME,
		ONLINE_NEW_LOGIN,
		ONLINE_DELETE_LOGIN,
		QUERY_GLOBAL_STATISTIC
	};
	DelayOperationType delayOperation_;

	typedef vector<sGameHostInfo> GameHostInfos;
	GameHostInfos netGames_;
	int selectedGameIndex_;

	sGameHostInfo& selectedGameInfo_;
	MissionDescription& selectedGame_;

	bool gameCreated_;

	string password_;
	string pass2_;

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
};

#endif // __UI_NET_CENTER_H__