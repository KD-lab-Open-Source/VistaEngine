#pragma once

#include "XMath/Colors.h"

class cAccessTexture
{
public:
	cAccessTexture();
	~cAccessTexture();

	bool load(const char* file_name);
	Color4c get(int x,int y);//clamp на границах текстуры
	Color4c get1(float x,float y);//x=0..1,y=0..1
	bool empty()const{return !data_;}
	int sizex()const{return sizex_;}
	int sizey()const{return sizey_;}
protected:
	int sizex_,sizey_;
	Color4c* data_;
};
