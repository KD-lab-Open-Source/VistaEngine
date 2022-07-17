#ifndef __LOGMSG_H__
#define __LOGMSG_H__

#include <strsafe.h>
#include "MTSection.h"

#pragma warning(disable : 4995)

//void LogMsg(LPCTSTR fmt, ...);
class LogMsgCenter {
public:
	LogMsgCenter()  { //: memLog(1024*1024, true)
		setLogMode(false, false);
		setFileName("!General.log");
	}
	~LogMsgCenter(){
		saveLog();
	}
	void setLogMode(bool _screenON, bool _fileON){
		screenON = _screenON;
		fileON = _fileON;
		if(fileON)
			memLog.alloc(1024*1024);
		else
			memLog.free();
	}
	void setFileName(const char* fname) {
		strncpy(logFileName, fname, sizeof(logFileName));
		logFileName[sizeof(logFileName)-1] = '\0';
	}
	void init() {
		memLog.init();
	}
	void saveLog() {
		if(!fileON) return;
		XStream f(logFileName, XS_OUT);
		int size=memLog.tell();
		f.write(memLog.buffer(), size);
	}
	void msg(LPCTSTR fmt, ...){
//#ifndef _FINAL_VERSION_
		if(screenON || fileON){
			MTAuto lock(m_Lock);
			va_list val;
			va_start(val, fmt);

			DWORD written;
			TCHAR buf[256];
			//wvsprintf(buf, fmt, val);
			::StringCbVPrintf(buf, sizeof(buf), fmt, val);
			buf[sizeof(buf)-1]='\0';

			if(screenON){
				static const COORD _80x50 = {80,1500};
				static BOOL startup = (AllocConsole(), SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), _80x50));
				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlen(buf), &written, 0);
			}
			if(fileON)
                memLog < buf;
		}
//#endif
	}
	MTSection m_Lock;
	XBuffer memLog;
	bool screenON;
	bool fileON;
	char logFileName[MAX_PATH];
};

__declspec(selectany) LogMsgCenter logMsgCenter;


extern LogMsgCenter logMsgCenter;
//#ifndef _FINAL_VERSION_
#define LogMsg logMsgCenter.msg
//#else
//#define LogMsg
//#endif


#endif //__LOGMSG_H__