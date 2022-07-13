#include "stdafx.h"
#include "NormalizeTGA.h"
#include "..\Render\inc\umath.h"
#include "..\Render\inc\hsv.h"
#include <d3dx9.h>

inline
float bicos(const float p00, const float p01, const float p10, const float p11, float cx,float cy)
{
	float p0=CosInterpolate(p00,p01,cx);
	float p1=CosInterpolate(p10,p11,cx);
	float p=CosInterpolate(p0,p1,cy);
	return p;
}

float NormalizeTga::GetBrightInterpolate(int x,int y)
{
	x=(x+sizex-tile_size/2)%sizex;
	y=(y+sizey-tile_size/2)%sizey;
	x=max(x,0);
	y=max(y,0);
	int xx=(x/tile_size),yy=(y/tile_size);
	float b00=GetBrightTile(xx,yy);
	float b01=GetBrightTile(xx+1,yy);
	float b10=GetBrightTile(xx,yy+1);
	float b11=GetBrightTile(xx+1,yy+1);

	float fx=(x-xx*tile_size)/(float)tile_size;
	float fy=(y-yy*tile_size)/(float)tile_size;
	return bicos(b00,b01,b10,b11, fx,fy);
}

bool NormalizeTga::Normalize(sColor4c* data,int sizex_,int sizey_,int tile_size_)
{
	in_data=data;
	sizex=sizex_;
	sizey=sizey_;
	tile_size=tile_size_;
	tile_size=min(min(tile_size,sizex),sizey);

	if(!IsPositivePower2(sizex))
		return false;
	if(!IsPositivePower2(sizey))
		return false;
	if(!IsPositivePower2(tile_size))
		return false;
	if((sizex%tile_size!=0)||(sizey%tile_size!=0))
		return false;

	bright_sizex=sizex/tile_size;
	bright_sizey=sizey/tile_size;
	bright_data=new float[bright_sizex*bright_sizey];

	//calc bright
	for(int y=0;y<sizey;y+=tile_size)
	for(int x=0;x<sizex;x+=tile_size)
	{
		int ix,iy;
		int sum_r=0,sum_g=0,sum_b=0;
		float& bd=bright_data[(x/tile_size)+(y/tile_size)*bright_sizex];
		bd=0;
		for(iy=0;iy<tile_size;iy++)
		for(ix=0;ix<tile_size;ix++)
		{
			sColor4c c=in_data[(y+iy)*sizex+x+ix];
			float h,s,v;
			RGBtoHSV(sColor4c(c.r,c.g,c.b),h,s,v);
			bd+=v;
		}

		bd/=tile_size*tile_size;
	}

	//apply bright
	for(int y=0;y<sizey;y+=tile_size)
	for(int x=0;x<sizex;x+=tile_size)
	{
		int ix,iy;
		int sum_r=0,sum_g=0,sum_b=0;
		int xx=(x/tile_size),yy=(y/tile_size);

		for(iy=0;iy<tile_size;iy++)
		for(ix=0;ix<tile_size;ix++)
		{
			sColor4c& c=in_data[(y+iy)*sizex+x+ix];
			float fout=GetBrightInterpolate(x+ix,y+iy);

			fout=max(fout,0.05f);
			float mul=0.5f/fout;
			float h,s,v;
			RGBtoHSV(sColor4c(c.r,c.g,c.b),h,s,v);
			c=HSVtoRGB(h,s,clamp(v*mul,0,1));
//			c.r=clamp(round(c.r*mul),0,255);
//			c.g=clamp(round(c.g*mul),0,255);
//			c.b=clamp(round(c.b*mul),0,255);
//			c.r=c.g=c.b=round(fout*255);
		}
	}

	delete bright_data;

	return true;
}

bool NormalizeTga::SaveDDS(struct IDirect3DDevice9* pDevice,const char* filename)
{
	IDirect3DTexture9* pTexture=NULL;
	HRESULT hr=pDevice->CreateTexture(sizex,sizey,0,0,D3DFMT_DXT1,D3DPOOL_SCRATCH,&pTexture,NULL);
	if(FAILED(hr))
		return false;

	DWORD levels=pTexture->GetLevelCount();
	sColor4c* out_data=new sColor4c[sizex*sizey];

	int num_bit=ReturnBit(tile_size);

	for(int i=0;i<levels;i++)
	{
		float smul=1;

		smul=(num_bit-i)/(float)num_bit;
		smul=clamp(smul,0,1);

		for(int y=0;y<sizey;y++)
		for(int x=0;x<sizex;x++)
		{
			sColor4c& c=in_data[y*sizex+x];
			sColor4c& cout=out_data[y*sizex+x];
			float h,s,v;
			RGBtoHSV(c,h,s,v);
			cout=HSVtoRGB(h,s*smul,v);
		}

		LPDIRECT3DSURFACE9 lpSurface = NULL;
		hr=pTexture->GetSurfaceLevel( i, &lpSurface );
		xassert(SUCCEEDED(hr));

		RECT rc;
		rc.left=0;
		rc.top=0;
		rc.right=sizex;
		rc.bottom=sizey;

		hr=D3DXLoadSurfaceFromMemory(lpSurface,NULL,NULL,out_data,D3DFMT_A8R8G8B8,4*sizex,NULL,&rc,D3DX_FILTER_TRIANGLE,0);
		xassert(SUCCEEDED(hr));
	}

	hr=D3DXSaveTextureToFile(filename,D3DXIFF_DDS,pTexture,NULL);
	delete out_data;
	return SUCCEEDED(hr);
}
