#include "StdAfx.h"
#include "kdw\Application.h"
#include "XTL\sigslot.h"
#include <process.h>

#include "Runtime.h"
#include "MySQLpp\TableBase.h"

#include "XMath\XMathLib.h"
#include "Serialization\SerializationLib.h"
#include "kdw\kdWidgetsLib.h"
#include "MySQLpp\MySQLppLib.h"


HANDLE server_finish = 0;

void __cdecl server_thread(void*)
{
	while(!server_finish)
		runtime->serverQuant();

	SetEvent(server_finish);
}

struct MyConnector : public sigslot::has_slots
{
	void runtimeMainQuant(){ runtime->mainQuant(); }
	void runtimeOnQuit(){ runtime->onQuit(); }
};

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
	TableBase::setupConnection("localhost", "root", "1", "vista");

	kdw::Application application(instance);

	runtime = new Runtime(&application);

	MyConnector* connector = new MyConnector();
	application.signalIdle().connect(connector, &MyConnector::runtimeMainQuant);
	application.signalQuit().connect(connector, &MyConnector::runtimeOnQuit);

	_beginthread(::server_thread, 1000000, 0);

	application.run();

	server_finish = CreateEvent(0, false, false, 0);
	WaitForSingleObject(server_finish, INFINITE);
	CloseHandle(server_finish);

	delete connector;
	delete runtime;
}