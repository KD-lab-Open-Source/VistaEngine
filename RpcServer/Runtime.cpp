#include "StdAfx.h"
#include "Runtime.h"

#include "MTSection.h"
#include "MainWindow.h"
#include "ShowLog.h"

#include "XmlRpc\XmlRpc.h"

#include "Methods.h"

Runtime* runtime = 0;

using namespace kdw;
using namespace XmlRpc;
using namespace RpcMethod;

template<class ParamType>
class RpcMethodParamCall : public RpcMethodParam<ParamType>
{
public:
	typedef int (Runtime::*pFunc)(const ParamType& data);
	RpcMethodParamCall(const char* methodName, pFunc h) : RpcMethodParam<ParamType>(methodName), handler_(h) {}
	int call(Archive& out) { return (runtime->*handler_)(data_); }
private:
	pFunc handler_;
};

class LogHandler : public XmlRpcLogHandler, public XmlRpcErrorHandler
{
public:
	LogHandler() : logEmpty_(true) {}

	void log(int level, const char* msg);
	void error(const char* msg);

	bool getLog(vector<string>& out, int max = -1);

private:
	MTSection lock_;
	vector<string> outbuf_;
	bool logEmpty_;
} logHandler;

// --------------------------------------------------------------------------

Runtime::Runtime(kdw::Application* app)
{
	app_ = app;
	window_ = new MainWindow(app_);

	initTables();

	LogHandler::setLogHandler(&logHandler);
	LogHandler::setErrorHandler(&logHandler);
	LogHandler::setVerbosity(2);

	server_ = new XmlRpcServer;

	server_->bindAndListen(81);
	server_->enableIntrospection(true);

	RpcMethodBase::setServer(server_);

	serverProcedures_.push_back(new VectorSum());
	serverProcedures_.push_back(new RpcMethodParamCall<RpcType::LoginData>("Register", &Runtime::registerUser));
	serverProcedures_.push_back(new RpcMethodParamCall<RpcType::LoginData>("Login", &Runtime::login));
	serverProcedures_.push_back(new RpcMethodParamCall<XGUID>("Logout", &Runtime::logout));
	serverProcedures_.push_back(new RpcMethodParamCall<RpcType::ScoreData>("AddScore", &Runtime::addScores));
	serverProcedures_.push_back(new GetScoresMethod());
}

Runtime::~Runtime()
{
	ServerProcedures::iterator met = serverProcedures_.begin();
	for(; met != serverProcedures_.end(); ++met)
		delete *met;

	delete server_;

	finitTables();

	delete window_;
}

void Runtime::addLog(int prio, const char* msg)
{
	if(prio < 0)
		logHandler.error(msg);
	else
		logHandler.log(prio, msg);
}

void Runtime::serverQuant()
{
	server_->work(1.);
	Sleep(10);
}

void Runtime::mainQuant()
{
	vector<string> buf;
	if(logHandler.getLog(buf, 50)){
		vector<string>::const_iterator it = buf.begin();
		for(; it != buf.end(); ++it)
			window_->view()->addRecord(it->c_str());
	}

	window_->view()->setStatus((XBuffer() < "Соединений: " <= (int)server_->dispatcher().sourceCount()));

	Sleep(10);
}

// --------------------------------------------------------------------------

void LogHandler::log(int level, const char* msg)
{ 
	if(level <= _verbosity){
		MTAuto lock(lock_);

		outbuf_.push_back((XBuffer() <= level < ": " < msg).c_str());
		logEmpty_ = false;
	}
}

void LogHandler::error(const char* msg)
{
	MTAuto lock(lock_);

	outbuf_.push_back((XBuffer() < "Error: " < msg).c_str());
	logEmpty_ = false;
}

bool LogHandler::getLog(vector<string>& out, int max)
{
	if(logEmpty_)
		return false;

	MTAuto lock(lock_);

	if(max <= 0 || max >= outbuf_.size()){
		out = outbuf_;
		outbuf_.clear();
		logEmpty_ = true;
	}
	else {
		out.clear();
		out.insert(out.begin(), outbuf_.begin(), outbuf_.begin() + max);
		outbuf_.erase(outbuf_.begin(), outbuf_.begin() + max);
	}

	return !out.empty();
}