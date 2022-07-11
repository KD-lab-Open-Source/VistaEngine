#include "stdafx.h"
#include "RpcClient.h"
#include <process.h>
#include "MTSection.h"
#include "Network\LogMsg.h"
#include "XmlRpc\XmlRpc.h"

#define _LIB_NAME "XmlRpc"
#include "AutomaticLink.h"

RpcClient* RpcClient::gbXmlRpcClient = 0;

using namespace XmlRpc;

class LogHandler : public XmlRpcLogHandler, public XmlRpcErrorHandler
{
public:
	LogHandler() : logEmpty_(true) {}

	void log(int level, const char* msg);
	void error(const char* msg);

	bool getLog(vector<string>& out);

private:
	MTSection lock_;
	vector<string> outbuf_;
	bool logEmpty_;
} logHandler;

// --------------------------------------------------------------------------

RpcClient::RpcClient()
{
	gbXmlRpcClient = this;

	netThreadID = 0;
	netThreadFinish = 0;

	rpcClient_ = 0;
	rpcAsynchClient_ = 0;

	LogHandler::setLogHandler(&logHandler);
	LogHandler::setErrorHandler(&logHandler);
	LogHandler::setVerbosity(2);

	remotePort_ = 0;
	proxyPort_ = 0;
}

RpcClient::~RpcClient()
{
	delete rpcClient_;
	delete rpcAsynchClient_;
}

void __cdecl rpcClient_thread(void* client)
{
	RpcClient* rpcClient = reinterpret_cast<RpcClient*>(client);

	while(!rpcClient->netThreadFinish)
		rpcClient->netQuant();

	SetEvent(rpcClient->netThreadFinish);
}

void RpcClient::startNetThread()
{
	MTL();
	if(!netThreadID)
		netThreadID = (HANDLE)_beginthread(::rpcClient_thread, 1000000, this);
}

void RpcClient::stopNetThread()
{
	MTL();
	if(netThreadID){
		xassert(netThreadFinish == 0);
		netThreadFinish = CreateEvent(0, false, false, 0);
		WaitForSingleObject(netThreadFinish, INFINITE);
		CloseHandle(netThreadFinish);
		netThreadFinish = 0;
		netThreadID = 0;
		tasks_.clear();
	}
}

void RpcClient::netQuant()
{
	taskLisk_.lock();

	if(tasks_.empty()){
		taskLisk_.unlock();
		Sleep(10);
		return;
	}

	RpcMethodCallBase* call_ = tasks_[0];
	tasks_.erase(tasks_.begin());
	
	taskLisk_.unlock();

	if(call_){
		MTAuto lock(acynchExecute_);
		call_->call();
		delete call_;
	}

}

void RpcClient::Quant()
{
	MTL();

	vector<string> buf;
	if(logHandler.getLog(buf)){
		vector<string>::const_iterator it = buf.begin();
		for(; it != buf.end(); ++it)
			LogMsg(it->c_str());
	}
}

void RpcClient::setRemoteHost(const char* host, int port)
{
	MTL();
	MTAuto lock(acynchExecute_);

	remoteHost_ = host;
	remotePort_ = port;

	delete rpcClient_;
	delete rpcAsynchClient_;

	if(proxyHost_.empty()){
		rpcClient_ = new XmlRpc::XmlRpcClient(remoteHost_.c_str(), remotePort_);
		rpcAsynchClient_ = new XmlRpc::XmlRpcClient(remoteHost_.c_str(), remotePort_);
	}
	else {
		XBuffer addr;
		addr < "http://" < remoteHost_.c_str() < ":" <= remotePort_ < "/RPC2 ";

		rpcClient_ = new XmlRpc::XmlRpcClient(proxyHost_.c_str(), proxyPort_, addr.c_str());
		rpcClient_->setAuthorization(proxyLogin_.c_str(), proxyPassword_.c_str());

		rpcAsynchClient_ = new XmlRpc::XmlRpcClient(proxyHost_.c_str(), proxyPort_, addr.c_str());
		rpcAsynchClient_->setAuthorization(proxyLogin_.c_str(), proxyPassword_.c_str());
	}
}

void RpcClient::setProxy(const char* host, int port, const char* login, const char* pwd)
{
	if(host)
		proxyHost_ = host;
	else
		proxyHost_.clear();

	proxyPort_ = port;

	if(login)
		proxyLogin_ = login;
	else
		proxyLogin_.clear();

	if(pwd)
		proxyPassword_ = pwd;
	else
		proxyPassword_.clear();
}

void RpcClient::rpcAsynchCall(const RpcMethodCallBase* data)
{
	if(!data)
		return;

	xassert(rpcAsynchClient_);

	MTAuto lock(taskLisk_);
	tasks_.push_back(data->clone());
}

// --------------------------------------------------------------------------

void LogHandler::log(int level, const char* msg)
{ 
	if(level <= _verbosity){
		lock_.lock();

		outbuf_.push_back((XBuffer() < "XmlRpc(" <= level < "): " < msg < "\n").c_str());
		logEmpty_ = false;

		lock_.unlock();
	}
}

void LogHandler::error(const char* msg)
{
	lock_.lock();

	outbuf_.push_back((XBuffer() < "XmlRpc-Error: " < msg < "\n").c_str());
	logEmpty_ = false;

	lock_.unlock();
}

bool LogHandler::getLog(vector<string>& out)
{
	if(logEmpty_)
		return false;

	lock_.lock();

	out = outbuf_;
	outbuf_.clear();
	logEmpty_ = true;

	lock_.unlock();

	return !out.empty();
}