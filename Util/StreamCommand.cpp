#include "StdAfx.h"
#include "StreamCommand.h"

StreamDataManager uiStreamCommand;
StreamDataManager uiStreamGraph2LogicCommand;

// команды из логики для исполнения в графике, заполняется в конце логики из uiStreamCommand
StreamDataManager uiStreamLogicCommand;

// команды из графики для исполнения в логике, заполняется в конце графического кванта из uiStreamGraph2LogicCommand
StreamDataManager uiStreamGraphCommand;


void StreamDataManager::execute()
{
	xassert(lockSection_.locked() && "StreamDataManager::execute()");

	start_timer_auto();

	if(stream_.tell())
	{
		calcSize();

		char* cur = &stream_[0];
		char* end = cur + stream_.tell();
		while(cur < end)
		{
			StreamDataFunction func = *(StreamDataFunction*)cur;
			cur += 4;
			
			DataSize realSize = *(DataSize*)cur;
			cur += sizeof(DataSize);
			
			func(cur);
			
			cur += realSize;
		}
		// HINT!!! команды очищаются после исполнения!
		clear();
	}
}

StreamDataManager& StreamDataManager::set(StreamDataFunction func)
{
	xxassert(lockSection_.locked(), "put command without lock");

	calcSize();

	stream_.write(func);

	sizePosition_ = stream_.tell();
	stream_.write(DataSize(0));

	return *this;
}

StreamDataManager& StreamDataManager::operator<<(const XBuffer& xb)
{
	xxassert(lockSection_.locked(), "put command without lock");
	stream_.write(xb, xb.makeFree() ? xb.tell() : xb.size());
	return *this;
}

StreamDataManager& StreamDataManager::operator<<(StreamDataManager& s)
{
	calcSize();
	s.calcSize();
	return *this << s.stream_;
}
