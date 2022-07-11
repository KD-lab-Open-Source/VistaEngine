#ifndef __LOGMSG_H__
#define __LOGMSG_H__

#include <strsafe.h>
#include "MTSection.h"

#pragma warning(disable : 4995)

//void LogMsg(LPCTSTR fmt, ...);
class LogMsgCenter {
public:
	void msg(LPCTSTR fmt, ...){
#ifndef _FINAL_VERSION_
		MTAuto lock(m_Lock);
		va_list val;
		va_start(val, fmt);

		DWORD written;
		TCHAR buf[256];
		//wvsprintf(buf, fmt, val);
		::StringCbVPrintf(buf, sizeof(buf), fmt, val);
		buf[sizeof(buf)-1]='\0';

		static const COORD _80x50 = {80,500};
		static BOOL startup = (AllocConsole(), SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), _80x50));
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlen(buf), &written, 0);
#endif
	}
	MTSection m_Lock;
};

__declspec(selectany) LogMsgCenter logMsgCenter;


extern LogMsgCenter logMsgCenter;
#ifndef _FINAL_VERSION_
#define LogMsg logMsgCenter.msg
#else
#define LogMsg
#endif


#endif //__LOGMSG_H__