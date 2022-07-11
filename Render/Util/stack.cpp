#include <my_stl.h>
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "stackwalker.h"
#include "stackwalker.cpp"
#include <string>
#include <stdlib.h>
#include "stack.h"
#ifdef _DEBUG

struct tostack
{
	int size;
	DWORD* pointers;
};

static void __stdcall rdProcessToStack(DWORD addres,DWORD param)
{
	tostack& out=*(tostack*)param;
	const num_skip=3;
	if(out.size<CreateStack::max_size+num_skip)
	{
		if(out.size>=num_skip)
			out.pointers[out.size-num_skip]=addres;
		out.size++;
	}
}

CreateStack::CreateStack()
{
	tostack s;
	s.size=0;
	s.pointers=pointers;
	memset(pointers,0,sizeof(pointers));
	ProcessStack(rdProcessToStack,(DWORD)&s);
}

void CreateStack::GetStack(std::string& out)
{
	out.clear();
	for(int i=0;i<max_size;i++)
	{
		DWORD addres=pointers[i];
		if(addres==0)
			break;
		const buf_max=256;
		char file_name[buf_max];
		int line;
		char func_name[buf_max];
		char buffer[1024];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		if(DebugInfoByAddr(addres,file_name,line,func_name,buf_max))
		{
			_splitpath(file_name,drive,dir,fname,ext);
			sprintf(buffer,"\t%s%s(%i) %s\n",fname,ext,line,func_name);
			out+=buffer;
		}
	}
}

void __stdcall rdProcessStack(DWORD addres,DWORD param)
{
	std::string& out=*(std::string*)param;
	const buf_max=256;
	char file_name[buf_max];
	int line;
	char func_name[buf_max];
	char buffer[1024];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	if(DebugInfoByAddr(addres,file_name,line,func_name,buf_max))
	{
		_splitpath(file_name,drive,dir,fname,ext);
		sprintf(buffer,"%s%s(%i) %s\n",fname,ext,line,func_name);
		out+=buffer;
	}

}

void GetStack(std::string& out)
{
	ProcessStack(rdProcessStack,(DWORD)&out);
}

#endif