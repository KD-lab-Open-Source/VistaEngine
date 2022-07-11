#include "StdAfx.h"
#include "ht.h"
#include "GameShell.h"
#include <process.h>
#include "StreamInterpolation.h"
#include "BaseUnit.h"
#include "universeX.h"
#include "SoundApp.h"
#include <malloc.h>

const DWORD bad_thread_id=0xFFFFFFFF;

bool applicationIsGo();

HTManager* HTManager::self;

HTManager::HTManager(bool ht)
{
	logic_thread_id=bad_thread_id;
	setUseHT(ht);

	self=this;
	init_logic=false;
	end_logic=NULL;

	init();
}

HTManager::~HTManager()
{
	tls_is_graph=MT_LOGIC_THREAD|MT_GRAPH_THREAD;
	done();
	self=NULL;
}

void __cdecl logic_thread( void * argument)
{
	HTManager::instance()->logic_thread();
}

void HTManager::setUseHT(bool use_ht_)
{
	if(PossibilityHT())
		use_ht=use_ht_;
	else
		use_ht=false;
}

void HTManager::GameStart(const MissionDescription& mission)
{
	tls_is_graph = MT_GRAPH_THREAD|MT_LOGIC_THREAD;
	gameShell->GameStart(mission);
	tls_is_graph = MT_GRAPH_THREAD;
	
	// SetThreadAffinityMask(GetCurrentThread(), 1);
	
	if(use_ht){
		xassert(logic_thread_id==bad_thread_id);
		logic_thread_id=_beginthread(::logic_thread, 1000000,NULL);
	}
}

void HTManager::GameClose()
{
	MTG();
	if(use_ht && logic_thread_id!=bad_thread_id)
	{
		end_logic=CreateEvent(NULL,FALSE,FALSE,NULL);

		DWORD ret=WaitForSingleObject(end_logic,INFINITE);
		xassert(ret==WAIT_OBJECT_0);
		
		CloseHandle(end_logic);
		end_logic=NULL;
		logic_thread_id=bad_thread_id;
	}

	tls_is_graph=MT_LOGIC_THREAD|MT_GRAPH_THREAD;
	gameShell->GameClose();
}

void HTManager::logic_thread()
{
	_alloca(4096+128);
	
	SetThreadAffinityMask(GetCurrentThread(), 2);

	CoInitialize(NULL);
	init_logic = true;
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
	init_logic = false;

	tls_is_graph = MT_LOGIC_THREAD;

	while(end_logic == NULL){
		start_timer_auto();

		while(!applicationIsGo())
			Sleep(10);

		gameShell->NetQuant();
		gameShell->logicQuantHT();
	}

	SetEvent(end_logic);
}

bool HTManager::Quant()
{
	if(use_ht){
		if(init_logic)
			Sleep(100);

		if(logic_thread_id == bad_thread_id)
			gameShell->NetQuant();

		//tls_is_graph = universe() ? MT_GRAPH_THREAD : MT_GRAPH_THREAD | MT_LOGIC_THREAD;
		gameShell->GraphQuant();
	}
	else{
		tls_is_graph = MT_LOGIC_THREAD;//Для MTG, MTL ассертов.

		gameShell->NetQuant();
		gameShell->logicQuantST();

		tls_is_graph = MT_GRAPH_THREAD;

		gameShell->GraphQuant();
	}
	
	return gameShell->GameContinue;
}

bool HTManager::PossibilityHT()
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	return sys_info.dwNumberOfProcessors>=2;
}


