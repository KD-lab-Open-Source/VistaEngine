#include "StdAfx.h"

class PathInitializer
{
public:
	PathInitializer(const char* path){
		if(path)
			SetCurrentDirectory(path);
	}
};

const char* getPathFromCommandLine()
{
	if(__argc > 1){
		return __argv[1];
	}
	else
		return ".";
}

#pragma warning(disable: 4073)
#pragma init_seg(lib)
static PathInitializer pathInitializer(getPathFromCommandLine());