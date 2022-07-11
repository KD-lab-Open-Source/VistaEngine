#ifndef __UI_NET_CENTER_H__
#define __UI_NET_CENTER_H__

#include "UI_Enums.h"
#include "Game\GlobalStatistics.h"
#include "Network\ExternalTask.h"
class Archive;
struct sGameHostInfo;
struct ChatChanelInfo;
class MissionDescription;
enum ScoresID;

class WBuffer;

typedef unsigned __int64 ChatChannelID;

struct ShowStatisticType;
typedef vector<ShowStatisticType> ShowStatisticTypes;

typedef std::vector<std::string> ComboStrings;
typedef vector<wstring> ComboWStrings;

class UI_NetCenter
{
	mutable MTSection lock_;
public:
	enum NetType{
		LAN,
		ONLINE,
		DIRECT
	};

	UI_NetCenter();
	~UI_NetCenter();

	/// Создан клиент сетевой игры
	bool isNetGame() const;
	/// Это создатель игры (host)
	bool isServer() const;

	/// Есть выбранная игра
	bool gameSelected() const;
	/// Игра запущена
	bool gameCreated() const { return gameCreated_; }

	/// Установить пароль
	void setPassword(const char* pass);
	/// Установить повторный пароль
	void setPass2(const char* pass);
	/// Обнулить пароли
	void resetPasswords();

	/// Статус выполнения последней команды
	//UI_NetStatus status() const { return netStatus_; }
	UI_NetStatus status() const;
	/// Ожидание выполнения предыдущей команды
	//bool acyncEventWaiting() const { return netStatus_ == UI_NET_WAITING; }
	bool acyncEventWaiting() const;
	/// Установить статус текущей выполняемой команды (завершить выполнение), сообщить о разрыве сети
	//void commit(UI_NetStatus status);

	/// Создать сетевую подсистему
	void create(NetType type);
	
	/// Команда обновить список игр
	void refreshGameList();
	/// Синхронизирует состояние рабочего списка с сетевым клиентом
	bool updateGameList();
	/// Выбрать игру из рабочего списка
	void selectGame(int idx);
	/// вернуть список строк с описанием текущий доступных игр
	int getGameList(const GameListInfoTypes& infoType, ComboWStrings& strings, const Color4c& started);
	/// Дескриптор мира выбранной сетевой игры
	const MissionDescription& selectedGame() const { return selectedGame_; }
	/// Имя выбранной игры
	const wchar_t* selectedGameName(WBuffer& buf) const;
	/// адрес текущего сервера
	const wchar_t* currentServerAddress(WBuffer& buf) const;
	
	void login();
	void logout();
	void quickStart();
	void createAccount();
	void changePassword();
	void deleteAccount();

	bool canCreateGame() const;
	/// Создать сервер сетевой игры
	void createGame();
	bool canJoinGame() const;
	/// Присоединиться к выбранной сетевой игре
	void joinGame();
	bool canJoinDirectGame() const;
	/// Присоединиться к указанному серверу
	void joinDirectGame();
	/// Готовность начать игру
	void startGame();

	const wchar_t* natType() const;
	
	/// Прервать текущую операцию
	void abortCurrentOperation();
	
	/// Оборвать текущую игру, начать искать новую
	void reset(bool flag_internalReset=false);
	/// Полностью завершить работу с сетью
	void release();

	void teamConnect(int teemIndex);
	void teamDisconnect(int teemIndex, int cooperativeIndex);

	/// Выставить строку для отсылки
	void setChatString(const wchar_t* stringForSend);
	/// Отослать строку
	bool sendChatString(int rawIntData);
	/// Обработчик получения сообщения чата по сети
	void handleChatString(const wchar_t* str);
	/// Получить список сообщений чата с момента последней очистки
	void getChatBoard(ComboWStrings &board) const;
	/// Переинициализировать чат
	void resetChatBoard(bool unsubscribe = true);
	void clearChatBoard();
	
	/// Команда на сервер обновить список список комнат чата и игроков в чате
	void refreshChatChannels();
	/// Синхронизировать чат с сетевым клиентом
	void updateChatChannels();
	/// Получить список комнат чата
	int getChatChannels(ComboWStrings& channels) const;
	/// Получить имя текущей выбранной комнаты чата
	void getCurrentChatChannelName(wstring& name) const;
	/// Выбрать комнату в чате
	void selectChatChannel(int channel);
	/// Войти в выбранную комнату в чате
	void enterChatChannel(ChatChannelID forceID = -1);
	/// Сейчас работает режим автовхода в чат
	bool autoSubscribeMode() const { return autoSubscribeMode_; }

	void chatSubscribeOK();
	void chatSubscribeFailed();

	void updateChatUsers();
	int getChatUsers(ComboWStrings& users) const;

	void queryGlobalStatistic(bool force = false);
	void getGlobalStatisticFromBegin();
	void getAroundMeGlobalStats();
	bool canGetPrevGlobalStats() const;
	void getPrevGlobalStats();
	bool canGetNextGlobalStats() const;
	void getNextGlobalStats();
	int getGlobalStatistic(const ShowStatisticTypes& format, ComboWStrings &board) const;
	void selectGlobalStaticticEntry(int idx);
	int getCurrentGlobalStatisticValue(StatisticType type);
	
	void setPausePlayerList(const ComboWStrings& playerList);
	void getPausePlayerList(ComboWStrings& playerList) const;
	bool isOnPause() const { return isOnPause_; }

	void updateFilter();

	void quant();
private:
	void clear();
	void autoSubscribeChatChannel();

	//UI_NetStatus netStatus_;
	bool flag_lastNetCommandOk;

	enum DelayOperationType{
	//	DELAY_NONE,
	//	CREATE_GAME,
	//	ONLINE_CREATE_LOGIN,
	//	ONLINE_NEW_LOGIN,
	//	ONLINE_DELETE_LOGIN,
	//	ONLINE_CHECK_VERSION,
	//	QUERY_GLOBAL_STATISTIC
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

	typedef vector<ChatChanelInfo> ChatChanelInfos;
	ChatChanelInfos chatChanelInfos_;
	ChatChannelID subscribedChannel_;
	ChatChannelID subscribeWaitingChannel_;
	ChatChannelID selectedChannel_;
	bool autoSubscribeMode_;
	bool autoSubscribeUnsusseful_;
	bool lastChannelSubscribeAttemptMode_;
	ChatChannelID lastSubscribeAttempt_;
	wstring currentChatChannelName_;

	wstring currentChatString_;
	
	ComboWStrings chatUsers_;
	ComboWStrings chatBoard_;

	ComboWStrings pausePlayerList_;
	bool isOnPause_;

	//void setStatus(UI_NetStatus status);

	int currentStatisticFilterRace_;
	int currentStatisticFilterGamePopulation_;
	int firstScorePosition_;
	//GlobalStatistics gsForRead_;
	GlobalStatistics globalStatistics_;
	int selectedGlobalStatisticIndex_;
	void applyNewGlobalStatistic(const GlobalStatistics& stats);
	XBuffer version_;
	void queryGameVersion();
	bool setGameVersion();

	ExternalNetTask_Init extNetTask_Init;
	ENTCreateAccount extNetTask_CreateAccount;
	ENTDeleteAccount extNetTask_DeleteAccount;
	ENTChangePassword extNetTask_ChangePassword;
	ENTLogin extNetTask_Login;
	ENTDownloadInfoFile extNetTask_DownloadInfoFile;
	ENTReadGlobalStats extNetTask_ReadGlobalStats;
	ENTSubscribe2ChatChannel extNetTask_Subscribe2ChatChannel;
	ENTGame extNetTask_Game;
};

#endif // __UI_NET_CENTER_H__