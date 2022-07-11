#pragma once

#include "Render\3dx\Saver.h"

bool RenderFileRead(const char *fname,char *&buf,int &size);//Читает файл в буфер созданный new, размер файла - size

class CLoadDirectoryFileRender : public CLoadDirectory
{
public:
	CLoadDirectoryFileRender::CLoadDirectoryFileRender()
		:CLoadDirectory(0,0)
	{
	}

	CLoadDirectoryFileRender::~CLoadDirectoryFileRender()
	{
		delete begin;
	}

	bool Load(LPCSTR filename)
	{
		cur=0;
		char* data=0;
		bool ok=RenderFileRead(filename,data,size);
		begin=(BYTE*)data;
		return ok;
	}
};

