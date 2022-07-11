#pragma once
#ifndef __VISTARPC_GAMETEST_H_INCLUDED__
#define __VISTARPC_GAMETEST_H_INCLUDED__

#include "FileUtils\XGUID.h"

class Archive;

namespace RpcType
{
	struct ReturnScore;
};

class GameTest
{
public:
	GameTest();

	void serialize(Archive& ar);

	void registerUser(const char* n, const char* p);
	void login(const char* n, const char* p);
	void logout(const XGUID& session);
	void addScoreBySession(const XGUID& session, int score, unsigned type);
	void addScoreByName(const char* n, int score, unsigned type);
	void getScores(unsigned long start, unsigned count);

public:
	void registerUserHandler(int status);
	void loginHandler(int status);
	void logoutHandler(int status);
	void addScoreHandler(int status);
	void getScoresHandler(int status, const RpcType::ReturnScore* scores);

private:
	void log(const char* msg);

	enum CallLock
	{
		LOCK1 = 0,
		LOCK2,
		CALL_LOCK_SIZE
	};
	char callLock_[CALL_LOCK_SIZE];

	bool setCallLock(CallLock type);
	void releaseCallLock(CallLock type);

	bool logined_;
	XGUID session_;

	string login_;
	string pass_;
	int score_;
	unsigned scoreType_;

	unsigned startScores_;
};

#endif //__VISTARPC_GAMETEST_H_INCLUDED__
