// Profiler stub for the cross-platform build.
//
// The real Profiler.cpp (XLibs.Net/Profiler) is Win32/editor-coupled (windows.h,
// process.h, kdw property editor). The engine references a handful of profiling
// entry points unconditionally; these no-ops let it link with profiling
// disabled (timers record nothing).
#include <string>
#include <vector>
using namespace std;

#include "Profiler.h"

Profiler::Profiler() {}
void Profiler::quant(unsigned long) {}
void Profiler::setAutoMode(int, int, const char*, const char*, bool) {}
void Profiler::start_stop(ProfilerMode) {}

Profiler& ProfilerInterface::profiler()
{
	static Profiler theProfiler;
	return theProfiler;
}

TimerData::TimerData(const char* title) : title_(title) {}
void TimerData::start() {}
void TimerData::stop() {}

StatisticalData::StatisticalData(char* title) : title_(title) {}
void StatisticalData::add(double) {}
