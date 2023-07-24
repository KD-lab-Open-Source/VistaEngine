#include "xglobal.h"
//#include <ostream.h>

void xtSysFinit(){}
void xtClearMessageQueue(){}

void win32_break(char* error,char* msg)
{
//	cerr << "--------------------------------\n";
//	cerr << error << "\n";
//	cerr << msg << "\n";
//	cerr  << "--------------------------------" << endl;
//	_ASSERT(FALSE) ;
}

const char* check_command_line(const char* switch_str)
{
	for(int i = 1; i < __argc; i ++){
		const char* s = strstr(__argv[i], switch_str);
		if(s){
			for(const char* p = __argv[i]; p < s; p++)
				if(*p != '-' && *p != '/')
					goto cont;
			return s += strlen(switch_str);
			}
		cont:;
		}
	return 0;
}



