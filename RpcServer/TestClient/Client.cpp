#include "StdAfx.h"
#include "Client.h"

#include "MTSection.h"
#include "MainWindow.h"
#include "GameTest.h"

#include "XmlRpc\XmlRpc.h"

Client* client = 0;

using namespace kdw;
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

Client::Client(kdw::Application* app)
{
	app_ = app;

	game_ = new GameTest();

	client = this;

	window_ = new MainWindow(app_);

	rpcClient_ = 0;
	rpcAsynchClient_ = 0;

	LogHandler::setLogHandler(&logHandler);
	LogHandler::setErrorHandler(&logHandler);
	LogHandler::setVerbosity(2);

	remotePort_ = 0;
	proxyPort_ = 0;
}

Client::~Client()
{
	delete rpcClient_;
	delete rpcAsynchClient_;
	delete window_;
	delete game_;
}

void Client::setRemoteHost(const char* host, int port)
{
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

void Client::setProxy(const char* host, int port, const char* login, const char* pwd)
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

void Client::netQuant()
{
	RpcMethodCallBase* call_ = 0;
	taskLisk_.lock();
	if(!tasks_.empty()){
		call_ = tasks_[0];
		tasks_.erase(tasks_.begin());
	}
	taskLisk_.unlock();

	if(call_){
		MTAuto lock(acynchExecute_);
		call_->call();
		delete call_;
	}

	Sleep(20);
}

void Client::rpcAsynchCall(const RpcMethodCallBase* data)
{
	if(!data)
		return;

	MTAuto lock(taskLisk_);
	tasks_.push_back(data->clone());
}


void Client::mainQuant()
{
	//char buffer[512];
	//buffer[511] = 0;
	vector<string> buf;
	if(logHandler.getLog(buf)){
		//window_->addLogRecord((XBuffer() <=  buf.size() < " событий"));
		vector<string>::const_iterator it = buf.begin();
		for(; it != buf.end(); ++it){
		//	_snprintf(buffer, 511, "%s", it->c_str());
		//	OutputDebugString(buffer);
			window_->addLogRecord(it->c_str());
		}
	}

	//Sleep(20);
}

// --------------------------------------------------------------------------

void LogHandler::log(int level, const char* msg)
{ 
	if(level <= _verbosity){
		lock_.lock();

		outbuf_.push_back((XBuffer() <= level < ": " < msg).c_str());
		logEmpty_ = false;

		lock_.unlock();
	}
}

void LogHandler::error(const char* msg)
{
	lock_.lock();

	outbuf_.push_back((XBuffer() < "Error: " < msg).c_str());
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