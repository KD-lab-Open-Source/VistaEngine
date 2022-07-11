#include "StdAfxRD.h"
#include "VisError.h"
#include "IRenderDevice.h"

cVisError VisError;

cVisError::cVisError()
{
	buf.reserve(256);
	no_message_box=false;
}

cVisError& cVisError::operator << (int a)
{
	char bNew[256];
	sprintf(bNew,"%d",a);
	buf+=bNew;
	return *this;
}

cVisError& cVisError::operator << (float a)
{
	char bNew[256];
	sprintf(bNew,"%f",a);
	buf+=bNew;
	return *this;
}

cVisError& cVisError::operator << (const char *a)
{
	if(a==0)
		return *this;
	static bool in_message_box=false;
	if(in_message_box)
		return *this;
	if(strcmp(a,VERR_END)==0)
	{// конец потока
		kdError("3d", buf.c_str());
		if(!no_message_box)
		{
			if(gb_RenderDevice && gb_RenderDevice->IsFullScreen())
				ShowWindow(gb_RenderDevice->currentRenderWindow()->GetHwnd(),SW_MINIMIZE);
			in_message_box=true;
			int ret=MessageBox(0,buf.c_str(),"cVisGeneric::ErrorMessage()",MB_ABORTRETRYIGNORE|MB_TOPMOST|MB_ICONSTOP);
			if(ret==IDIGNORE)
			{
				no_message_box=true;
			}

			if(ret==IDABORT) 
			{
				__asm { int 3 };
			}
		}

		buf.clear();
		in_message_box=false;
	}else
		buf+=a;
	return *this;
}

cVisError& cVisError::operator << (string& a)
{
	return (*this)<<a.c_str();
}


