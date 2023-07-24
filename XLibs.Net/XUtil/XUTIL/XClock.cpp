
/* ---------------------------- INCLUDE SECTION ----------------------------- */

#include "xglobal.h"
#include "MTSection.h"
#include "XMath\xmath.h"

#pragma comment(lib, "winmm.lib")

#pragma warning(disable : 4073 )
#pragma init_seg(lib)

static const unsigned int INIT_PERIOD = 1000;
static const int FIXED_POINT = 12;
static __int64 ADJUST_PERIOD = 60*1000 << FIXED_POINT;
static const int PROCESSORS_MAX = 8;

__declspec (noinline)
__int64 getRDTSC()
{
	#define RDTSC __asm _emit 0xf __asm _emit 0x31
	__int64 timeRDTS;
	__asm {
		push ebx
		push ecx
		push edx
		RDTSC
		mov dword ptr [timeRDTS],eax
		mov dword ptr [timeRDTS+4],edx
		pop edx
		pop ecx
		pop ebx
	}
	return timeRDTS;
}

int getCPUID()
{
	unsigned int id;
	__asm {
		push ebx
		push ecx
		push edx
		mov eax, 1
		CPUID
		shr ebx, 24
		mov id, ebx
		pop edx
		pop ecx
		pop ebx
	}
	return id;
}

class XClock
{
public:
	XClock(int cpuID)
	{
		cpuID_ = cpuID;
		time_ = 0;
		frequency_ = 3000000;
		counterPrev_ = 0;
		
		timeToAgjust_ = 0;
		counterToAdjust_ = 0;
		clockToAdjust_ = 0;
	}

	int time()
	{
		__int64 counter = getRDTSC();

		int cpuID = getCPUID();
		if(cpuID != cpuID_)
			return clocks_[cpuID & (PROCESSORS_MAX - 1)].time();

		if(!counterPrev_){
			counterPrev_ = counter;
			counterToAdjust_ = counter;
			clockToAdjust_ = timeGetTime();
			timeToAgjust_ = 1000;
		}
		
		__int64 dt = ((counter - counterPrev_) << FIXED_POINT)/frequency_;

		__int64 bigTime = __int64(60*60*1000) << FIXED_POINT;

		dt = clamp(dt, 0, bigTime);

		time_ += dt;
		counterPrev_ = counter;
		
		if(time_ >= timeGlobal_)
			timeGlobal_ = time_;
		else
			time_ = timeGlobal_;
		
		if(time_ > timeToAgjust_){
			unsigned int clock = timeGetTime();
			if(clock > clockToAdjust_)
				frequency_ = (counter - counterToAdjust_)/(clock - clockToAdjust_);
			timeToAgjust_ = time_ + ADJUST_PERIOD;
			clockToAdjust_ = clock;
			counterToAdjust_ = counter;
		}

		return int(time_ >> FIXED_POINT); 
	}

private:
	int cpuID_;
	__int64 time_;
	__int64 counterPrev_;
	__int64 frequency_;

	__int64 timeToAgjust_;
	__int64 counterToAdjust_;
	unsigned int clockToAdjust_;

	static __int64 timeGlobal_;
	static XClock clocks_[PROCESSORS_MAX];

	friend int xclock();
}; 

__int64 XClock::timeGlobal_ = 0;

XClock XClock::clocks_[PROCESSORS_MAX] = { XClock(0), XClock(1), XClock(2), XClock(3), XClock(4), XClock(5), XClock(6), XClock(7) };

static MTSection lock_;

int xclock()
{
	MTAuto lock(lock_);
	return XClock::clocks_[getCPUID() & (PROCESSORS_MAX - 1)].time();
} 

