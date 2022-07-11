#include "StdAfx.h"
#include "kdw\Application.h"
#include "Client.h"

#include "XMath\XMathLib.h"
#include "Serialization\SerializationLib.h"
#include "kdw\kdWidgetsLib.h"

//#define _LIB_NAME "XmlRpc"
//#include "AutomaticLink.h"

#include <process.h>

HANDLE server_finish = 0;

void __cdecl server_thread(void*)
{
	while(!server_finish)
		client->netQuant();

	SetEvent(server_finish);
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
	kdw::Application application(instance);
	
	new Client(&application);
	xassert(client);

	//client->setProxy("localhost", 3128, "Skorp", "1");
	//client->setRemoteHost("sk0rp.no-ip.org", 81);

	client->setRemoteHost("localhost", 81);

	_beginthread(::server_thread, 1000000, 0);

	application.signalIdle().connect(client, &Client::mainQuant);
	application.run();

	server_finish = CreateEvent(0, false, false, 0);
	WaitForSingleObject(server_finish, INFINITE);
	CloseHandle(server_finish);

	delete client;
}