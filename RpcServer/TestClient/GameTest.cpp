#include "stdafx.h"
#include "GameTest.h"
#include "Serialization\Serialization.h"
#include "Serialization\Decorators.h"
#include "Client.h"
#include "MainWindow.h"
#include "UserInterface\XmlRpc\RpcEnums.h"
#include "UserInterface\XmlRpc\RpcTypes.h"

GameTest::GameTest()
{
	logined_ = false;
	session_.generate();
	memset(callLock_, 0, CALL_LOCK_SIZE);

	score_ = 1;
	scoreType_ = 0;
	startScores_ = 0;
}

void GameTest::serialize(Archive& ar)
{
	ar.serialize(login_, "login", "Имя");
	ar.serialize(pass_, "pass", "Пароль");

	ButtonDecorator new_user_button("Зарегистрироваться");
	ar.serialize(new_user_button, "new_user", "<");
	if(new_user_button)
		registerUser(login_.c_str(), pass_.c_str());

	ButtonDecorator login_button("Войти");
	ar.serialize(login_button, "login", "<");
	if(login_button)
		login(login_.c_str(), pass_.c_str());

	ButtonDecorator logout_button("Выйти");
	ar.serialize(logout_button, "logout", "<");
	if(logout_button)
		logout(session_);

	ar.serialize(score_, "score", "Очки");
	ar.serialize(scoreType_, "scoreType", "Тип очков");

	ButtonDecorator addScoreBySession_button("Добавить очки в сессию");
	ar.serialize(addScoreBySession_button, "addbys", "<");
	if(addScoreBySession_button)
		addScoreBySession(session_, score_, scoreType_);

	ButtonDecorator addScoreByLogin_button("Добавить очки пользователю");
	ar.serialize(addScoreByLogin_button, "addbyu", "<");
	if(addScoreByLogin_button)
		addScoreByName(login_.c_str(), score_, scoreType_);

	ar.serialize(startScores_, "startScores", "Получить начиная с позиции");
	ButtonDecorator getScores_button("Получить очки");
	ar.serialize(getScores_button, "getscores", "<");
	if(getScores_button)
		getScores(startScores_, 3);
}

void GameTest::log(const char* msg)
{
	client->getMainWindow()->addLogRecord(msg);
}

bool GameTest::setCallLock(CallLock type)
{
	if(callLock_[type])
		return false;
	callLock_[type] = true;
	return true;
}

void GameTest::releaseCallLock(CallLock type)
{
	xassert(callLock_[type]);
	callLock_[type] = false;
}

void GameTest::registerUser(const char* n, const char* p)
{
	if(!setCallLock(LOCK1))
		return;

	log((XBuffer() < "Регистрация: " < n < ", " < p).c_str());
	
	typedef RpcSimpleMethodAsynchCall<GameTest, RpcType::LoginData> MethodRegister;

	RpcType::LoginData data;
	data.login = n;
	data.pass = p;
	MethodRegister call("Register", this, &GameTest::registerUserHandler, data);

	client->rpcAsynchCall(&call);
}

void GameTest::login(const char* n, const char* p)
{
	if(!setCallLock(LOCK1))
		return;

	log((XBuffer() < "Вход: " < n < ", " < p).c_str());

	typedef RpcSimpleMethodAsynchCall<GameTest, RpcType::LoginData> MethodLogin;

	RpcType::LoginData data;
	data.login = n;
	data.pass = p;
	data.session = session_;
	MethodLogin call("Login", this, &GameTest::loginHandler, data);

	client->rpcAsynchCall(&call);
}

void GameTest::logout(const XGUID& session)
{
	if(!setCallLock(LOCK1))
		return;

	log("Выход");

	typedef RpcSimpleMethodAsynchCall<GameTest, XGUID> MethodLogout;

	MethodLogout call("Logout", this, &GameTest::logoutHandler, session);

	client->rpcAsynchCall(&call);
}

void GameTest::addScoreBySession(const XGUID& session, int score, unsigned type)
{
	if(!setCallLock(LOCK1))
		return;

	log("addScoreBySession");

	typedef RpcSimpleMethodAsynchCall<GameTest, RpcType::ScoreData> MethodAddScore;

	MethodAddScore call("AddScore", this, &GameTest::addScoreHandler,
		RpcType::ScoreData(score, type, "", &session));

	client->rpcAsynchCall(&call);
}

void GameTest::addScoreByName(const char* n, int score, unsigned type)
{
	if(!setCallLock(LOCK1))
		return;

	log("addScoreByName");

	typedef RpcSimpleMethodAsynchCall<GameTest, RpcType::ScoreData> MethodAddScore;

	MethodAddScore call("AddScore", this, &GameTest::addScoreHandler,
		RpcType::ScoreData(score, type, n));

	client->rpcAsynchCall(&call);
}

void GameTest::getScores(unsigned long start, unsigned count)
{
	if(!setCallLock(LOCK2))
		return;

	log((XBuffer() < "getScores(" <= start < ", " <= count < ")").c_str());

	typedef RpcMethodAsynchCall<GameTest, RpcType::GetScore, RpcType::ReturnScore> MethodGetScores;

	MethodGetScores call("GetScores", this, &GameTest::getScoresHandler,
		RpcType::GetScore(start, count));

	client->rpcAsynchCall(&call);
}

void GameTest::registerUserHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		log("registerUser: Регистрация прошла успешно");
		break;
	case STATUS_USER_NAME_EXIST:
		log("registerUser: Такой уже есть");
	    break;
	default:
		log("registerUserHandler: Ошибка");
	    break;
	}

	releaseCallLock(LOCK1);

}

void GameTest::loginHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		logined_ = true;
		log("login: Успешный вход");
		break;
	case STATUS_BAD_USER_OR_PASSWORD:
		log("login: Нет такого пользователя или неверный пароль");
		break;
	default:
		log("login: Ошибка");
		break;
	}

	releaseCallLock(LOCK1);

}

void GameTest::logoutHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		logined_ = false;
		log("logout: Успешный выход");
		break;
	case STATUS_DOUBLE_OR_NOT_LOGON:
		log("logout: Вход в этой сессии не произведен");
		break;
	default:
		log("logout: Ошибка");
		break;
	}

	releaseCallLock(LOCK1);
}


void GameTest::addScoreHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		logined_ = false;
		log("addScore: Успешная обработка");
		break;
	case STATUS_DOUBLE_OR_NOT_LOGON:
		log("addScore: Вход в этой сессии не произведен");
		break;
	case STATUS_BAD_USER_OR_PASSWORD:
		log("addScore: Прохое имя");
		break;
	default:
		log("logout: Ошибка");
		break;
	}

	releaseCallLock(LOCK1);
}

void GameTest::getScoresHandler(int status, const RpcType::ReturnScore* scores)
{
	if(status == STATUS_GOOD && scores){
		log("getScoresHandler: Успешный ответ на получение очков");
		unsigned rank = scores->startRank;
		RpcType::Scores::const_iterator it = scores->scores.begin();
		for(;it != scores->scores.end(); ++it){
			int allscore = 0;
			RpcType::ScoreNode::UserScores::const_iterator it2 = it->scores.begin();
			for(;it2 != it->scores.end(); ++it2){
				allscore += it2->score;
				log((XBuffer() < "Type " <= it2->type < " Score " <= it2->score).c_str());
			}
			++rank;
			log((XBuffer() < "#" <= rank < "> " < it->login.c_str() < " --> " <= allscore).c_str());
		}
	}
	else
		log("getScoresHandler: Ошибка получения");

	releaseCallLock(LOCK2);
}