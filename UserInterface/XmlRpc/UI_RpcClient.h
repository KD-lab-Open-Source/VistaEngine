#ifndef __VISTARPC_UI_RPC_CLIENT_H_INCLUDED__
#define __VISTARPC_UI_RPC_CLIENT_H_INCLUDED__

#include "FileUtils\XGUID.h"
//#include "MTSection.h"

class Archive;

class UI_RpcClient
{
public:
	UI_RpcClient();

	static UI_RpcClient* instance() { return inst_ ? inst_ : (inst_ = new UI_RpcClient());	}

	void create();
	void release();

	void registerUser(const char* n, const char* p);
	void login(const char* n, const char* p);
	void logout();

public:
	void registerUserHandler(int status);
	void loginHandler(int status);
	void logoutHandler(int status);

private:
	static UI_RpcClient* inst_;
	enum CallLock
	{
		LOCK1 = 0,
		CALL_LOCK_SIZE
	};
	volatile long callLock_[CALL_LOCK_SIZE];

	bool setCallLock(CallLock type);
	void releaseCallLock(CallLock type);

	XGUID session_;

	bool created_;
};

#endif //__VISTARPC_UI_RPC_CLIENT_H_INCLUDED__
