#pragma once
#ifndef __VISTARPC_CLIENT_H_INCLUDED__
#define __VISTARPC_CLIENT_H_INCLUDED__

#include "XTL\sigslot.h"
#include "Serialization\BinaryArchive.h"
#include "XmlRpc\XmlRpc.h"
#include "UserInterface\XmlRpc\RpcTypes.h"
#include "MTSection.h"

#define RPC_EXECUTE_TIMEOUT 5.

namespace kdw
{
	class Application;
};

class MainWindow;
class GameTest;

class RpcMethodCallBase
{
public:
	RpcMethodCallBase(const char* n) : name_(n) {}
	virtual RpcMethodCallBase* clone() const = 0;
	virtual bool call() = 0;
protected:
	string name_;
};

template<class ListenerType, class ParamType, class ReturnType>
class RpcMethodAsynchCall : public RpcMethodCallBase
{
public:
	typedef void (ListenerType::*pFunc)(int status, const ReturnType*);
	RpcMethodAsynchCall(const char* name, ListenerType* l, pFunc f, const ParamType& d)
		: RpcMethodCallBase(name)
		, listener_(l)
		, func_(f)
		, callData_(d)
	{} 
	
	RpcMethodCallBase* clone() const { return new RpcMethodAsynchCall(*this); }
	bool call() {
		int status;
		if(client->doRemoteCall(name_.c_str(), callData_, status, retData_, true)){
			(listener_->*func_)(status, &retData_);
			return true;
		}
		(listener_->*func_)(-1, 0);
		return false;
	}

private:
	ListenerType* listener_;
	pFunc func_;
	ParamType callData_;
	ReturnType retData_;
};

template<class ListenerType, class ParamType>
class RpcSimpleMethodAsynchCall : public RpcMethodCallBase
{
public:
	typedef void (ListenerType::*pFunc)(int status);
	RpcSimpleMethodAsynchCall(const char* name, ListenerType* l, pFunc f, const ParamType& d)
		: RpcMethodCallBase(name)
		, listener_(l)
		, func_(f)
		, callData_(d)
	{} 

	RpcMethodCallBase* clone() const { return new RpcSimpleMethodAsynchCall(*this); }
	bool call() {
		int status;
		if(client->doRemoteCall(name_.c_str(), callData_, status, true)){
			(listener_->*func_)(status);
			return true;
		}
		(listener_->*func_)(-1);
		return false;
	}

private:
	ListenerType* listener_;
	pFunc func_;
	ParamType callData_;
};

class Client : public sigslot::has_slots
{
public:
	Client(kdw::Application* app);
	virtual ~Client();

	void setRemoteHost(const char* host, int port); 
	void setProxy(const char* host, int port, const char* login = 0, const char* pwd = 0);

	void netQuant();
	void mainQuant();

	template<class ParamType, class ReturnType>
	bool doRemoteCall(const char* name, const ParamType& data, int& status, ReturnType& ret, bool asynch = false);
	template<class ParamType>
	bool doRemoteCall(const char* name, const ParamType& data, int& status, bool asynch = false);

	void rpcAsynchCall(const RpcMethodCallBase* data);

	MainWindow* getMainWindow() { return window_; }
	GameTest* game() { return game_; }

private:
	MTSection acynchExecute_;
	MTSection taskLisk_;

	kdw::Application* app_;
	MainWindow* window_;
	GameTest* game_;

	XmlRpc::XmlRpcClient* rpcClient_;
	XmlRpc::XmlRpcClient* rpcAsynchClient_;

	string proxyHost_;
	int proxyPort_;

	string proxyLogin_;
	string proxyPassword_;

	string remoteHost_;
	int remotePort_;

	typedef vector<RpcMethodCallBase*> CallTasks;
	CallTasks tasks_;
};

template<class ParamType, class ReturnType>
bool Client::doRemoteCall(const char* name, const ParamType& par, int& status, ReturnType& ret, bool asynch)
{
	XmlRpc::XmlRpcClient* client = asynch ? rpcAsynchClient_ : rpcClient_;
	xassert(client);

	BinaryOArchive oa;
	oa.serialize(par, "root", 0);

	XmlRpc::XmlRpcValue result, params(oa.data(), oa.size());

	//encodeData(params);

	if(client->execute(name, params, result, RPC_EXECUTE_TIMEOUT)
		&& !client->isFault())
	{
		const XmlRpc::XmlRpcValue::BinaryData& data = result;
		//decodeData(data);

		BinaryIArchive ia((char*)&data[0], data.size());
		ia.serialize(ret, "root", 0);
		ia.serialize(status, "status", 0);

		return true;
	}

	return false;
}

template<class ParamType>
bool Client::doRemoteCall(const char* name, const ParamType& par, int& status, bool asynch)
{
	XmlRpc::XmlRpcClient* client = asynch ? rpcAsynchClient_ : rpcClient_;
	xassert(client);

	BinaryOArchive oa;
	oa.serialize(par, "root", 0);

	XmlRpc::XmlRpcValue result, params(oa.data(), oa.size());

	//encodeData(params);

	if(client->execute(name, params, result, RPC_EXECUTE_TIMEOUT)
		&& !client->isFault())
	{
		const XmlRpc::XmlRpcValue::BinaryData& data = result;
		//decodeData(data);

		BinaryIArchive ia((char*)&data[0], data.size());
		ia.serialize(status, "status", 0);

		return true;
	}

	return false;
}

extern Client* client;

#endif //__VISTARPC_CLIENT_H_INCLUDED__
