#include "StdAfx.h"
#include "StreamCommand.h"

#ifndef _FINAL_VERSION_
#define CALL_STATISTIC
#endif

#ifdef CALL_STATISTIC
#include "Util\xtl\StaticMap.h"
#include "Util\Win32\DebugSymbolManager.h"
struct TimerContainer
{
	TimerContainer() : name(0), timer(0) {}
	char* name;
	TimerData* timer;
};
#endif

StreamDataManager uiStreamCommand(true);
StreamDataManager uiStreamGraph2LogicCommand(false);

// команды из логики дл€ исполнени€ в графике, заполн€етс€ в конце логики из uiStreamCommand
StreamDataManager uiStreamLogicCommand(true);

// команды из графики дл€ исполнени€ в логике, заполн€етс€ в конце графического кванта из uiStreamGraph2LogicCommand
StreamDataManager uiStreamGraphCommand(false);

#ifndef _FINAL_VERSION_
#define THREAD_TEST(isPut)	\
	if(MT_IS_LOGIC()){		\
		xxassert(logic2graph_ == (isPut) || MT_IS_GRAPH(), "ќбращение к StreamDataManager не из того потока"); \
	} else {				\
		xxassert(logic2graph_ != (isPut), "ќбращение к StreamDataManager не из того потока"); \
	}
#else
#define THREAD_TEST(isPut)
#endif

void StreamDataManager::execute()
{
	THREAD_TEST(false)

	start_timer_auto();

	if(stream_.tell())
	{
		calcSize();

		#ifdef CALL_STATISTIC
			static __declspec(thread) StaticMap<int, TimerContainer>* timerMap = 0;
			if(!timerMap)
				timerMap = new StaticMap<int, TimerContainer>;
		#endif

		char* cur = &stream_[0];
		char* end = cur + stream_.tell();
		while(cur < end)
		{
			StreamDataFunction func = *(StreamDataFunction*)cur;
			cur += 4;
			
			DataSize realSize = *(DataSize*)cur;
			cur += sizeof(DataSize);
			
			#ifdef CALL_STATISTIC
				TimerContainer& data = timerMap->operator []((int)func);
				if(data.timer == 0){
					string name;
					debugSymbolManager->getProcName(func, name);
					xassert(!name.empty());
					data.name = new char[name.size()+1];
					strcpy(data.name, name.c_str());
					data.timer = new TimerData(data.name);
				}
				data.timer->start();
			#endif
			
			func(cur);

			#ifdef CALL_STATISTIC
				data.timer->stop();
			#endif
			
			cur += realSize;
		}
		// HINT!!! команды очищаютс€ после исполнени€!
		clear();
	}
}

StreamDataManager& StreamDataManager::set(StreamDataFunction func)
{
	THREAD_TEST(true)

	calcSize();

	stream_.write(func);

	sizePosition_ = stream_.tell();
	stream_.write(DataSize(0));

	return *this;
}

StreamDataManager& StreamDataManager::operator<<(const XBuffer& xb)
{
	stream_.write(xb, xb.makeFree() ? xb.tell() : xb.size());
	return *this;
}

StreamDataManager& StreamDataManager::operator<<(StreamDataManager& s)
{
	calcSize();
	s.calcSize();
	return *this << s.stream_;
}
