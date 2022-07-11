#include "StdAfxRD.h"
#include "AccessTexture.h"
#include "FileImage.h"
#include "FileUtils\FileUtils.h"

cAccessTexture::cAccessTexture()
{
	sizex_=sizey_=0;
	data_=0;
}

cAccessTexture::~cAccessTexture()
{
	delete data_;
}

bool cAccessTexture::load(const char* file_name)
{
	if(data_)
	{
		delete data_;
		data_=0;
	}

	string file_name_n = normalizePath(file_name);

	cFileImage* f=cFileImage::Create(file_name_n.c_str());
	if(f==0)
		return false;
	if(f->load(file_name_n.c_str())<0)
	{
		delete f;
		return false;
	}

	sizex_=f->GetX();
	sizey_=f->GetY();
	data_=new Color4c[sizex_*sizey_];
	f->GetTexture(data_,0,sizex_,sizey_);

	delete f;
	return true;
}

Color4c cAccessTexture::get(int x,int y)
{
	x=clamp(x,0,sizex_);
	y=clamp(y,0,sizey_);
	return data_[x+y*sizex_];
}

Color4c cAccessTexture::get1(float x,float y)
{
	int xi=round(x*sizex_);
	int yi=round(y*sizey_);
	
	xi=clamp(xi,0,sizex_);
	yi=clamp(yi,0,sizey_);
	return data_[xi+yi*sizex_];
}