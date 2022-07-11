#include "stdafx.h"
#include "UI_RpcClient.h"
#include "Serialization\Serialization.h"
#include "Serialization\Decorators.h"
#include "RpcClient.h"
#include "RpcTypes.h"
#include "Network\LogMsg.h" 

using namespace RpcType;

typedef RpcSimpleMethodAsynchCall<UI_RpcClient, LoginData> MethodRegister;
typedef RpcSimpleMethodAsynchCall<UI_RpcClient, LoginData> MethodLogin;
typedef RpcSimpleMethodAsynchCall<UI_RpcClient, XGUID> MethodLogout;

UI_RpcClient* UI_RpcClient::inst_ = 0;

UI_RpcClient::UI_RpcClient()
{
	created_ = false;
	session_.generate();
	for(int idx = 0; idx < CALL_LOCK_SIZE; ++idx)
		callLock_[idx] = 0;
}

bool UI_RpcClient::setCallLock(CallLock type)
{
	if(!created_)
		return false;

	if(InterlockedExchange(&callLock_[type], 1))
		return false;
	return true;
}

void UI_RpcClient::releaseCallLock(CallLock type)
{
	if(!created_)
		return;

	xassert(callLock_[type] != 0);
	callLock_[type] = 0;
}

void UI_RpcClient::create()
{
	MTL();

	if(created_)
		return;
	LogMsg("UI_RpcClient::create\n");

	RpcClient::instance()->setRemoteHost("node", 81);
	RpcClient::instance()->startNetThread();

	created_ = true;
}

void UI_RpcClient::release()
{
	MTL();

	if(!created_)
		return;
	LogMsg("UI_RpcClient::release\n");

	RpcClient::instance()->stopNetThread();

	created_ = false;
}

void UI_RpcClient::registerUser(const char* n, const char* p)
{
	if(!setCallLock(LOCK1))
		return;
	LogMsg((XBuffer() < "UI_RpcClient:: Регистрация: " < n < ", " < p < "\n").c_str());

	RpcClient::instance()->rpcAsynchCall(&MethodRegister("Register", this, &UI_RpcClient::registerUserHandler, LoginData(n, p)));
}

void UI_RpcClient::login(const char* n, const char* p)
{
	if(!setCallLock(LOCK1))
		return;
	LogMsg((XBuffer() < "UI_RpcClient:: Вход: " < n < ", " < p < "\n").c_str());

	RpcClient::instance()->rpcAsynchCall(&MethodLogin("Login", this, &UI_RpcClient::loginHandler, LoginData(n, p, &session_)));
}

void UI_RpcClient::logout()
{
	if(!setCallLock(LOCK1))
		return;
	LogMsg("UI_RpcClient:: Выход\n");

	RpcClient::instance()->rpcAsynchCall(&MethodLogout("Logout", this, &UI_RpcClient::logoutHandler, session_));
}

void UI_RpcClient::registerUserHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		LogMsg("UI_RpcClient::registerUserHandler: Регистрация прошла успешно\n");
		break;
	case STATUS_USER_NAME_EXIST:
		LogMsg("UI_RpcClient::registerUserHandler: Такой уже есть\n");
		break;
	default:
		LogMsg("UI_RpcClient::registerUserHandler: Ошибка\n");
		break;
	}

	releaseCallLock(LOCK1);

}

void UI_RpcClient::loginHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		LogMsg("UI_RpcClient::loginHandler: Успешный вход\n");
		break;
	case STATUS_BAD_USER_OR_PASSWORD:
		LogMsg("UI_RpcClient::loginHandler: Нет такого пользователя или неверный пароль\n");
		break;
	default:
		LogMsg("UI_RpcClient::loginHandler: Ошибка\n");
		break;
	}

	releaseCallLock(LOCK1);

}

void UI_RpcClient::logoutHandler(int status)
{
	switch(status){
	case STATUS_GOOD:
		LogMsg("UI_RpcClient::logoutHandler: Успешный выход\n");
		break;
	case STATUS_DOUBLE_OR_NOT_LOGON:
		LogMsg("UI_RpcClient::logoutHandler: Вход в этой сессии не произведен\n");
		break;
	default:
		LogMsg("UI_RpcClient::logoutHandler: Ошибка\n");
		break;
	}

	releaseCallLock(LOCK1);
}