#pragma once
#ifndef __VISTARPC_RUNTIME_H_INCLUDED__
#define __VISTARPC_RUNTIME_H_INCLUDED__

namespace kdw
{
	class Application;
};

namespace XmlRpc
{
	class XmlRpcServer;
};

namespace RpcType
{
	struct LoginData;
	struct ScoreData;
	struct ScoreNode;
	typedef vector<ScoreNode> Scores;
};

namespace RpcMethod
{
	class RpcMethodBase;
};

class XGUID;

class MainWindow;
class TableUser;
class TableScore;

class Runtime
{
public:
	Runtime(kdw::Application* app);
	virtual ~Runtime();

	void onQuit() {}

	void serverQuant();
	void mainQuant();

	void addLog(int prio, const char* msg);

	TableUser* users() { return users_; }
	TableScore* scores() { return scores_; }

	int registerUser(const RpcType::LoginData&);
	int login(const RpcType::LoginData&);
	int logout(const XGUID&);

	int addScores(const RpcType::ScoreData&);
	int getScores(unsigned long start, unsigned quantity, RpcType::Scores& out);

private:
	kdw::Application* app_;
	MainWindow* window_;

	void initTables();
	void finitTables();

	TableUser* users_;
	TableScore* scores_;

	XmlRpc::XmlRpcServer* server_;

	typedef vector<RpcMethod::RpcMethodBase*> ServerProcedures;
	ServerProcedures serverProcedures_;
};

extern Runtime* runtime;

#endif //__VISTARPC_RUNTIME_H_INCLUDED__
