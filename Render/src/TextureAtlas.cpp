#include <my_stl.h>
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
//#include "StdAfxRD.h"
#include <vector>
#include <algorithm>
using namespace std;
#include "XMath\xmath.h"
#include <xutil.h>
#include "Render\3dx\umath.h"
#include "TextureAtlas.h"

cTextureAtlas::cTextureAtlas()
{
	texture=0;
}

cTextureAtlas::~cTextureAtlas()
{
	delete[] texture;
}

int cTextureAtlas::GetMaxMip()
{
	int num_mip=16;
	for(int i=0;i<texture_data.size();i++)
	{
		Tex& t=texture_data[i];
		int numx=ReturnBit(t.pow2_size.x);
		int numy=ReturnBit(t.pow2_size.y);
		if(t.pow2_size.x>0)
			num_mip=min(num_mip,numx);
		if(t.pow2_size.y>0)
			num_mip=min(num_mip,numy);
	}

	xassert(num_mip<10);
	return num_mip;
}

struct cTextureAtlasSort
{
	vector<cTextureAtlas::Tex>& texture_data;
	cTextureAtlasSort(vector<cTextureAtlas::Tex>& texture_data_):texture_data(texture_data_){};
	bool operator()(int i0,int i1)
	{
		cTextureAtlas::Tex& t0=texture_data[i0];
		cTextureAtlas::Tex& t1=texture_data[i1];
		int size0=t0.pow2_size.x*t0.pow2_size.y;
		int size1=t1.pow2_size.x*t1.pow2_size.y;
		return size0>size1;
	}
};

bool cTextureAtlas::Init(vector<Vect2i>& texture_in,bool line)
{
	texture_data.resize(texture_in.size());
	sort_by_size.resize(texture_in.size());
	for(int itex=0;itex<texture_in.size();itex++)
	{
		Tex& t=texture_data[itex];
		t.size=texture_in[itex];
		if(t.size.x==0 || t.size.y==0)
		{
			t.pow2_size.x=t.pow2_size.y=0;
		}else
		{
			t.pow2_size.x=Power2up(t.size.x);
			t.pow2_size.y=Power2up(t.size.y);
		}

		if(!line)
			t.pow2_size.x=t.pow2_size.y=max(t.pow2_size.x,t.pow2_size.y);
		t.pos.set(0,0);
		sort_by_size[itex]=itex;
	}

	cTextureAtlasSort sort_data(texture_data);
	stable_sort(sort_by_size.begin(),sort_by_size.end(),sort_data);

	if(line)
	{
		texture_size.x = 0;
		texture_size.y = texture_data[sort_by_size[0]].pow2_size.y;
		for(int i=0; i<sort_by_size.size();i++)
		{
			Tex& t = texture_data[sort_by_size[i]];
			t.pos.set(texture_size.x,0);
			texture_size.x += t.pow2_size.x;
		}
		texture_size.x = Power2up(texture_size.x);
		texture_size.y = Power2up(texture_size.y);
	}else
	{

		Vect2i sizes[]={
			Vect2i(64,64),
				Vect2i(128,64),
				Vect2i(128,128),
				Vect2i(256,128),
				Vect2i(256,256),
				Vect2i(512,256),
				Vect2i(512,512),
				Vect2i(1024,512),
				Vect2i(1024,1024),
		};
		const int sizes_size= sizeof(sizes)/sizeof(sizes[0]);

		int i;
		for(i=0;i<sizes_size;i++)
		{
			texture_size=sizes[i];
			if(TryCompact())
				break;
		}

		if(i==sizes_size)
		{
			texture_size.set(0,0);
			return false;
		}

	}
	texture=new DWORD[texture_size.x*texture_size.y];
	memset(texture,0,texture_size.x*texture_size.y*4);
	return true;
}

bool cTextureAtlas::TryCompact()
{
	//Сначала заполняется большими текстурками, потом определяется, какие из мелких могут запихнуться в оставшееся место.
	//Пусть будет рекурсивный алгоритм разбиения на 2!
	int cur=0;
	if(texture_size.x<texture_size.y)
	{
		for(int y=0;y<texture_size.y;y+=texture_size.x)
			CompactRecursive(cur,0,y,texture_size.x);
	}else
	{
		for(int x=0;x<texture_size.x;x+=texture_size.y)
			CompactRecursive(cur,x,0,texture_size.y);
	}

	while(cur<sort_by_size.size())
	{
		Tex& t=texture_data[sort_by_size[cur]];
		if(t.pow2_size.x==0 || t.pow2_size.y==0)
		{
			t.pos.set(0,0);
			cur++;
		}else
			break;
	}

	return cur==sort_by_size.size();
}

void cTextureAtlas::CompactRecursive(int& cur,int position_x,int position_y,int size)
{
	if(cur>=sort_by_size.size())
		return;
	Tex& t=texture_data[sort_by_size[cur]];

	if(t.pow2_size.x>size || t.pow2_size.y>size)
		return;

	if(t.pow2_size.x==size && t.pow2_size.y==size)
	{
		t.pos.set(position_x,position_y);
		cur++;
		return;
	}

	int sz2=size/2;
	CompactRecursive(cur,position_x,position_y,sz2);
	CompactRecursive(cur,position_x+sz2,position_y,sz2);
	CompactRecursive(cur,position_x,position_y+sz2,sz2);
	CompactRecursive(cur,position_x+sz2,position_y+sz2,sz2);
}
Vect2i cTextureAtlas::GetSize(int index)
{
	Tex& t=texture_data[index];
	return t.size;
}

sRectangle4f cTextureAtlas::GetUV(int index)
{
	Tex& t=texture_data[index];
	sRectangle4f rect;
	rect.min.set(t.pos.x/(float)texture_size.x,t.pos.y/(float)texture_size.y);
	rect.max.set((t.pos.x+t.size.x)/(float)texture_size.x,(t.pos.y+t.size.y)/(float)texture_size.y);
	return rect;
}

void cTextureAtlas::FillTexture(int index,DWORD* data,int dx,int dy)
{
	Tex& t=texture_data[index];
	xassert(t.size.x==dx && t.size.y==dy);
	if(dx==0 || dy==0)
		return;
	for(int y=0;y<dy;y++)
	{
		DWORD* pout=texture+(t.pos.x+(t.pos.y+y)*texture_size.x);
		DWORD* pin=data+(y*dx);
		memcpy(pout,pin,dx*4);
	}
}
