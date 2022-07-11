#ifndef __EXTERNALTASK_H__
#define __EXTERNALTASK_H__

#include "CommonLocText.h"
#include "Units\AttributeReference.h"
#include "FileUtils\XGUID.h" //определение XGUID
#include "GlobalStatistics.h"

#define caseR(a) case a: return #a;

enum e_PNCWorkMode{
	PNCWM_LAN,
	PNCWM_LAN_DW,
	PNCWM_ONLINE_GAMESPY,
	PNCWM_ONLINE_DW,
	PNCWM_ONLINE_P2P,
};

enum eGameOrder {
	GameOrder_1v1=2,
	GameOrder_2v2=4,
	GameOrder_3v3=6,
	GameOrder_Any = 0,
	GameOrder_NoFilter=-1
};

enum ScoresID {
	//SCORES = (1u),
	
	SCORESA0 = (3u),
	SCORESA1 = (6u),
	SCORESA2 = (9u),
	SCORESA3 = (12u),

	SCORESE0 = (2u),
	SCORESE1 = (5u),
	SCORESE2 = (8u),
	SCORESE3 = (11u),

	SCORESR0 = (1u),
	SCORESR1 = (4u),
	SCORESR2 = (7u),
	SCORESR3 = (10u)
};

// flag_end сбрасывается первым устанавливается последним! для избежания CriticalSection
class ExternalNetTaskBase {
public:
	ExternalNetTaskBase  (){
		state=StFinal_EndHandled;
		//flag_end=false; flag_run=false;
        flag_absoluteError=false; flag_ok=false;
		flag_setuped=false;
	}
protected:
	void start() {
		xassert(flag_setuped);
		xassert(state==StFinal_EndHandled);
		state=StRun;
		//flag_end=false; flag_run=true;
		flag_absoluteError=false; flag_ok=false;
		flag_setuped=false;
	} 
public:
	enum ENTState {
        StRun=0x1,
		StRunCompleted=0x3,
		StRunHandled=0x7,
		StEnd=0xF,
		StFinal_EndHandled=0x1F
	};
	ENTState state;
	//bool flag_run;
	//bool flag_end;
	bool flag_ok;
	bool flag_absoluteError;
	bool flag_setuped;

	void setOk(){ xassert(((int)state>=(int)StRun) && ((int)state<(int)StFinal_EndHandled)); if(!flag_absoluteError) flag_ok=true; setState(StEnd);} //xassert(flag_run); flag_end=true; 
	void setRunCompletedOk(){ xassert(((int)state>=(int)StRun) && ((int)state<(int)StFinal_EndHandled)); if(!flag_absoluteError) flag_ok=true; setState(StRunCompleted);}  //xassert(flag_run);
	void setErr(){ xassert(((int)state>=(int)StRun) && ((int)state<(int)StFinal_EndHandled)); flag_absoluteError=true; flag_ok=false; setState(StEnd); } //xassert(flag_run); flag_end=true;
	void finalize() { xassert(state==StEnd); setState(StFinal_EndHandled);} //xassert(flag_end); xassert(flag_run); flag_run=false;
	bool isOk() const { return !flag_absoluteError; }
	bool isErr() const { return flag_absoluteError; }
	//bool isEnd() const { return state==StEnd; } //return flag_end;
	bool isRun() const {  return (int)state < (int)StFinal_EndHandled; } //return flag_run;
	bool isWait() const { return ((int)state <= (int)StRunCompleted) || state==StEnd; }
	bool isRunAndEnd() const { return state==StEnd;  } //return flag_run && flag_end;
	bool isRunCompleted() const { return state==StRunCompleted; }
	void setRunHandled() { setState(StRunHandled); }
	bool isSetupped() { return flag_setuped; } 
	//bool isEndOk() { return !flag_start && flag_End && isOk(); }
	virtual enum UI_CommonLocText getErrorCode()=0;
	virtual const char* getErrorText()=0;
private:
	void setState(ENTState st) {
		state = (ENTState)(state | st);
	}
	friend class PNetCenter;
	friend class UI_NetCenter;
};


class ExternalNetTask_Init : public ExternalNetTaskBase {
public:
	e_PNCWorkMode workMode;
	ExternalNetTask_Init() { workMode=PNCWM_LAN_DW; }
	void setup(e_PNCWorkMode _workMode) { workMode=_workMode; flag_setuped=true; }

	UI_CommonLocText getErrorCode(){ //virtual
		return UI_COMMON_TEXT_ERROR_CANT_CONNECT; 
	};
	const char* getErrorText(){ return "Init error"; } //virtual 
};

class ENTCreateAccount : public ExternalNetTaskBase {
public:
	ENTCreateAccount() { errorCode=ErrCode::NoError; }
	void setup(const char* _userName, const char* _password, const char* _licenseCode){
		userName=_userName; password=_password; licenseCode=_licenseCode;
		flag_setuped=true;
	}
	enum ErrCode {
		NoError,
		BadLicensed,
		IllegalOrEmptyPassword,
		IllegalUserName,
		VulgarUserName,
		UserNameExist,
		MaxAccountExceeded,
		Other,
	};
	ErrCode errorCode;
	void setErr(ErrCode ec=ErrCode::Other){ errorCode=ec; if(errorCode!=ErrCode::NoError)__super::setErr(); }

	const char* getErrorText(){  //virtual 
		switch(errorCode){ caseR(NoError); caseR(BadLicensed); caseR(IllegalOrEmptyPassword); 
		caseR(IllegalUserName); caseR(VulgarUserName); caseR(UserNameExist); caseR(MaxAccountExceeded);
		caseR(Other); default: return "???"; }
	}

	UI_CommonLocText getErrorCode(){ //virtual 
		switch(errorCode){
		case NoError:				return UI_COMMON_TEXT_LAST_ENUM;
		case BadLicensed:			return UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_BAD_LIC;
		case IllegalOrEmptyPassword:return UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
		case IllegalUserName:		return UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_NAME;
		case VulgarUserName:		return UI_COMMON_TEXT_ERROR_ACCOUNT_VULGAR_NAME;
		case UserNameExist:			return UI_COMMON_TEXT_ERROR_ACCOUNT_NAME_EXIST;
		case MaxAccountExceeded:	return UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_MAX;
		default:
		case Other:					return UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE;
		}
	}
	string userName;
	string password;
	string licenseCode;
protected:
};

class ENTDeleteAccount : public ExternalNetTaskBase {
public:
	void setup(const char* _userName, const char* _password){
		userName=_userName;	password=_password;
		flag_setuped=true;
	}
	enum ErrCode {
		NoError,
		IllegalOrEmptyPassword,
		DeleteAccountErr
	};
	ErrCode errorCode;
	void setErr(ErrCode ec=ErrCode::DeleteAccountErr){ errorCode=ec; if(errorCode!=ErrCode::NoError)__super::setErr(); }
	const char* getErrorText(){  //virtual 
		switch(errorCode){ caseR(NoError); caseR(IllegalOrEmptyPassword); caseR(DeleteAccountErr); default: return "???"; }
	}
	UI_CommonLocText getErrorCode(){ //virtual 
		switch(errorCode){
		case NoError:				return UI_COMMON_TEXT_LAST_ENUM;
		case IllegalOrEmptyPassword:return UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
		default:
		case DeleteAccountErr:		return UI_COMMON_TEXT_ERROR_ACCOUNT_DELETE;
		}
	}
	string userName;
	string password;
};

class ENTChangePassword : public ExternalNetTaskBase {
public:
	void setup(const char* _userName, const char* _licenseCode, const char* _oldPassword, const char* _newPassword ){
		userName=_userName;			
		if(_licenseCode) licenseCode=_licenseCode;
		if(_oldPassword) oldPassword=_oldPassword;	
		newPassword=_newPassword;
		flag_setuped=true;
	}
	enum ErrCode {
		NoError,
		IllegalOrEmptyPassword,
		ChangePasswordErr
	};
	ErrCode errorCode;
	void setErr(ErrCode ec=ErrCode::ChangePasswordErr){ errorCode=ec; if(errorCode!=ErrCode::NoError)__super::setErr(); }
	const char* getErrorText(){  //virtual 
		switch(errorCode){ caseR(NoError); caseR(IllegalOrEmptyPassword); caseR(ChangePasswordErr); default: return "???"; }
	}
	UI_CommonLocText getErrorCode(){ //virtual 
		switch(errorCode){
		case NoError:				return UI_COMMON_TEXT_LAST_ENUM;
		case IllegalOrEmptyPassword:return UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
		default:
		case ChangePasswordErr:		return UI_COMMON_TEXT_ERROR_ACCOUNT_CHANGE_PASSWORD;
		}
	}
	string userName;
	string licenseCode;
	string oldPassword;
	string newPassword;
};

class ENTLogin : public ExternalNetTaskBase {
public:
	ENTLogin() { errorCode=ErrCode::NoError; }
	void setup(const char* _userName, const char* _password){
		userName=_userName; password=_password;
		flag_setuped=true;
	}
	enum ErrCode {
		NoError,
		IllegalOrEmptyPassword,
		ServiceConnectLost,
		UnknownName,
		IncorrectPassword,
		AccountLocked,

		LobbyConnectionFailed,
		LobbyConnectionFailedMultipleLogons,
	};
	ErrCode errorCode;
	void setErr(ErrCode ec=ErrCode::ServiceConnectLost){ errorCode=ec; if(errorCode!=ErrCode::NoError)__super::setErr(); }

	const char* getErrorText(){  //virtual 
		switch(errorCode){ caseR(NoError); caseR(IllegalOrEmptyPassword); caseR(ServiceConnectLost);
		caseR(UnknownName); caseR(IncorrectPassword); caseR(AccountLocked);
		caseR(LobbyConnectionFailed); caseR(LobbyConnectionFailedMultipleLogons); default: return"???";
		}
	}

	UI_CommonLocText getErrorCode(){ //virtual 
		switch(errorCode){
		case NoError:				return UI_COMMON_TEXT_LAST_ENUM;
		case IllegalOrEmptyPassword:return UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
		case ServiceConnectLost:	return UI_COMMON_TEXT_ERROR_CONNECTION;
		case UnknownName:			return UI_COMMON_TEXT_ERROR_ACCOUNT_UNKNOWN_NAME;
		case IncorrectPassword:		return UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;
		case AccountLocked:			return UI_COMMON_TEXT_ERROR_ACCOUNT_LOCKED;

		case LobbyConnectionFailed:		return UI_COMMON_TEXT_ERROR_DISCONNECT;
		case LobbyConnectionFailedMultipleLogons:		return UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON;
		default:					
			xassert(0&& "Invalid ErrCode enum");
			return UI_COMMON_TEXT_LAST_ENUM;
		}
	}
	string userName;
	string password;
protected:
};

class ENTDownloadInfoFile: public ExternalNetTaskBase {
public:
	XBuffer* buf4File;
	ENTDownloadInfoFile() {buf4File=0;}
	void setup(XBuffer* _buf=0) { 
		buf4File=_buf; 
		xassert(buf4File); 
		if(!buf4File){
			__super::start(); 
			setErr();//выставление ошибкм в случае не указанного буфера
		}
		flag_setuped=true;
	}
	const char* getErrorText(){  //virtual 
		return "DownloadInfoFile Error";
	}
	UI_CommonLocText getErrorCode(){ //virtual
		return UI_COMMON_TEXT_ERROR_CONNECTION; 
	};
};
class ENTReadGlobalStats: public ExternalNetTaskBase {
public:
	ENTReadGlobalStats() {}
	void setup( ScoresID _scoresID, int _firstRank, int _maxResults ) { 
		scoresID=_scoresID;
		firstRank=_firstRank;
		maxResults=_maxResults;
		userName.clear();
		flag_setuped=true;
	}
	const char* getErrorText(){  //virtual 
		return "ReadGlobalStats Error";
	}
	UI_CommonLocText getErrorCode(){ //virtual
		return UI_COMMON_TEXT_LAST_ENUM;  //UI_COMMON_TEXT_ERROR_UNKNOWN
	};
	ScoresID scoresID;
	int firstRank;
	int maxResults;
	string userName;
	GlobalStatistics resultGlobalStatistics;
};

class ENTSubscribe2ChatChannel: public ExternalNetTaskBase {
public:
	unsigned __int64 chanelId;
	ENTSubscribe2ChatChannel() {}

	void setup( unsigned __int64 _chanelId ){
		chanelId = _chanelId;
		flag_setuped=true;
	}
	const char* getErrorText(){  //virtual 
		return "Subscribe2ChatChannel Error";
	}
	UI_CommonLocText getErrorCode(){ //virtual
		return UI_COMMON_TEXT_LAST_ENUM;
	};
};

class ENTGame: public ExternalNetTaskBase {
public:
	ENTGame() { gameOperation=GO_CreateGame;}
	enum GameOperation {
        GO_CreateGame,
		GO_QSGame,
		GO_JoinGame,
		GO_JoinGameP2P
	};
	GameOperation gameOperation;
	void setupCreateGame(const char* _gameName, const class MissionDescription* md, const char* _playerName, Race _race){
		gameOperation=GO_CreateGame;
		gameName=_gameName;
		missionDescription=md;
		playerName=_playerName;
		race=_race;
		flag_setuped=true;
	}
	void setupQSGame(const char* _playerName, int _race, eGameOrder _gameOrder, const std::vector<XGUID>& _missionFilter){
		gameOperation=GO_QSGame;
		playerName=_playerName;
		qsRace=_race;
		gameOrder=_gameOrder;
		missionFilter=_missionFilter;
		flag_setuped=true;
	}
	void setupJoinGame(GUID _gameHostID, const char* _playerName, Race _race){
		gameOperation=GO_JoinGame;
		gameHostID=_gameHostID;
		playerName=_playerName;
		race=_race;
		flag_setuped=true;
	}
	void setupJoinGame(const char* _gameHostIPAndPort, const char* _playerName, Race _race){
		gameOperation=GO_JoinGameP2P;
		gameHostIPAndPort=_gameHostIPAndPort;
		playerName=_playerName;
		race=_race;
		flag_setuped=true;
	}

	enum ErrCode {
		NoError,
		CreateGameErr,
		JoinGameConnectionErr,
		JoinGameGameIsRunErr,
		JoinGameGameIsFullErr,
		JoinGameGameNotEqualVersionErr,
		JoinGameGamePasswordErr,

		ConnectionFailed,
		HostTerminatedSession,
		GameDesynchronized,
		GeneralError,
		Reset
	};
	ErrCode errorCode;
	void setErr(ErrCode ec=ErrCode::ConnectionFailed){ errorCode=ec; if(errorCode!=ErrCode::NoError)__super::setErr(); }

	const char* getErrorText(){  //virtual 
		switch(errorCode){ caseR(NoError); caseR(CreateGameErr); 
		caseR(JoinGameConnectionErr); caseR(JoinGameGameIsRunErr); caseR(JoinGameGameIsFullErr); 
		caseR(JoinGameGameNotEqualVersionErr); caseR(JoinGameGamePasswordErr);
		caseR(ConnectionFailed); caseR(HostTerminatedSession); caseR(GameDesynchronized); caseR(GeneralError);
		caseR(Reset);
		default: return "???";
		}
	}
	UI_CommonLocText getErrorCode(){ //virtual 
		switch(errorCode){
		case NoError:				return UI_COMMON_TEXT_LAST_ENUM;
		case CreateGameErr:			return UI_COMMON_TEXT_ERROR_CREATE_GAME;
		case JoinGameConnectionErr:	return UI_COMMON_TEXT_ERROR_CONNECTION_GAME;
		case JoinGameGameIsRunErr:	return UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_RUN;
		case JoinGameGameIsFullErr:	return UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_FULL;
		case JoinGameGameNotEqualVersionErr: return UI_COMMON_TEXT_ERROR_INCORRECT_VERSION;
		case JoinGameGamePasswordErr: return UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;

		case ConnectionFailed:		return UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		case HostTerminatedSession:	return UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		case GameDesynchronized:	return UI_COMMON_TEXT_ERROR_DESINCH;
		case GeneralError:			return UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		case Reset:					return UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		default:
			xassert(0&& "Invalid ErrCode enum");
			return UI_COMMON_TEXT_LAST_ENUM;
		}
	}

	string gameName;
	const MissionDescription* missionDescription;
	string playerName;
	Race race;
	int qsRace;
	eGameOrder gameOrder;
	std::vector<XGUID> missionFilter;
	GUID gameHostID;
	string gameHostIPAndPort;
};



//////////////////////////////////////////////////
template <class T, class M> void finitExtTask_Err(T*& et, M errCode){
	
	xassert(et);
	if(et){ et->setErr(errCode); et=0; }
}
template <class T> void finitExtTask_Err(T*& et){
	xassert(et);
	if(et){ et->setErr(); et=0; }
}
template <class T> void finitExtTask_Ok(T*& et){
	xassert(et);
	if(et){ et->setOk(); et=0; }
}

template <class T> void runCompletedExtTask_Ok(T*& et){
	xassert(et);
	if(et){ et->setRunCompletedOk();}
}

#undef caseR

#endif //__EXTERNALTASK_H__
