#ifndef __VISERROR_H__
#define __VISERROR_H__

#include "Console.h"
#include "Render\Inc\rd.h"

RENDER_API int RDWriteLog(HRESULT err,char *exp,char *file,int line);

#define RDCALL(exp)									{ HRESULT hr=exp; if(hr){ RDWriteLog(hr,#exp,__FILE__,__LINE__); xassert(0 && #exp); } }


#define VERR_END			"###"

class cVisError
{
public:
	cVisError();
	cVisError& operator << (int a);
	cVisError& operator << (float a);
	cVisError& operator << (const char *a);
	cVisError& operator << (string& a);
private:
	string buf;
	bool no_message_box;
};
extern cVisError VisError;


RENDER_API void dprintf(char *format, ...);

#endif //__VISERROR_H__
