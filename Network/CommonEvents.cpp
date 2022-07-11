#include "StdAfx.h"
#include "CommonEvents.h"

//----------------------------------------------------------

terEventControlServerTime::terEventControlServerTime(XBuffer& in)
//: NetCommandBase(EVENT_ID_SERVER_TIME_CONTROL)
{
	in > scale;
}
void terEventControlServerTime::Write(XBuffer& out) const
{
	out < scale;
}

