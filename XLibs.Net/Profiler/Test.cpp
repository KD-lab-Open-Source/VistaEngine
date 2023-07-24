// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#include "StaticMap.h"
#include <memory.h>
#include <xutil.h>
#include <Windows.h>
#include <process.h>
#include "Profiler\Profiler.h"
#include "MTSection.h"
#include "xmath.h"


XStream ff("log", XS_OUT);

static int getCPUID()
{
	unsigned int id;
	__asm {
		mov eax, 1
		CPUID
		shr ebx, 24
		mov id, ebx
	}
	return id;
}

static MTSection lock_;
int time_;

int totalMemoryUsed();

int aaa;
int bbb;

void foo1()
{
	MTAuto lock(lock_);
	for(int i = 0; i < 10; i++){
		start_timer_auto1(eee);
		{
		start_timer_auto();
		}
		{
			start_timer_auto();
		}

		//ff <= getCPUID() < "\t" <= xclock() < "\n";
		//new char(1000);
		//totalMemoryUsed();
		for(aaa = 0; aaa < 100000; aaa++)
			bbb = aaa;
	}
}

void foo()
{
	for(int i = 0; i < 100; i++){
		start_timer_auto();
		//	MTAuto lock(lock_);
		foo1();
		//ff <= getCPUID() < "\t" <= xclock() < "\n";
		profiler_quant();
		statistics_add(sss, random(10));
		//	Sleep(10);
	}
}

void __cdecl logic_thread( void * argument)
{
	//SetThreadAffinityMask(GetCurrentProcess(), 2);

	foo();
//	for(;;){
//		MTAuto lock(lock_);
//		ff <= getCPUID() < "\t" <= xclock() < "\n";
//		Sleep(10);
//	}
}

int main(int argc, const char* argv[])
{
	profiler_start_stop(PROFILER_ACCURATE);

	_beginthread(::logic_thread, 1000000,NULL);

	//SetThreadAffinityMask(GetCurrentProcess(), 1);

	foo();
	profiler_start_stop();

	return 0;
}

