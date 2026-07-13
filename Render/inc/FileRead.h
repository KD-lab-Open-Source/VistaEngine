#pragma once

#include "Render/3dx/Saver.h"

bool RenderFileRead(const char *fname,char *&buf,int &size);//Читает файл в буфер созданный new, размер файла - size

class CLoadDirectoryFileRender : public CLoadDirectory
{
public:
	// Unqualified: naming the class again inside its own definition
	// (CLoadDirectoryFileRender::CLoadDirectoryFileRender) was an old MSVC extension,
	// and a conforming MSVC now reads it as an explicit override (C3241/C3254).
	CLoadDirectoryFileRender()
		:CLoadDirectory(0,0)
	{
	}

	~CLoadDirectoryFileRender()
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

