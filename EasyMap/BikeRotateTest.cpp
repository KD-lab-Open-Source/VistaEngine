#include "StdAfx.h"
#include "BikeRotateTest.h"


extern bool LoadTGA(const char* filename,int& dx,int& dy,unsigned char*& buf,
			 int& byte_per_pixel);

static BYTE* in_buffer=NULL;
static int in_dx,in_dy;
cTexture* texture=NULL;
void BikeInitTest()
{
	int byte_per_pixel;
	LoadTGA("resource\\FX\\Textures\\001.tga",in_dx,in_dy,in_buffer,byte_per_pixel);
//	LoadTGA("resource\\FX\\Textures\\003.tga",in_dx,in_dy,in_buffer,byte_per_pixel);
	xassert(byte_per_pixel==4);
	texture=GetTexLibrary()->CreateTexture(512,256,true);
}

void BikeReleaseTest()
{
	RELEASE(texture);
	delete in_buffer;
	in_buffer=NULL;
}

void BikeDrawTest()
{
	int pitch;
	BYTE* data=texture->LockTexture(pitch);
	for(int y=0;y<texture->GetHeight();y++)
	{
		DWORD* p=(DWORD*)(data+y*pitch);
		for(int x=0;x<texture->GetWidth();x++,p++)
		{
			*p=0xFF000000;
		}
	}

	static float angle=0;
	BikeRotateBilinearAndApply(data,texture->GetWidth(),texture->GetHeight(),pitch,
								in_buffer,in_dx,in_dy,in_dx*4,
								256,256,angle);
	angle+=0.01f;

	texture->UnlockTexture();
	gb_RenderDevice->DrawSprite(0,0,texture->GetWidth(),texture->GetHeight(),0,0,1,1,texture);
}