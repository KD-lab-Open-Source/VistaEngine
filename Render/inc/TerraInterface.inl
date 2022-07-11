#ifndef __TERRA_INTERFACE_INL_INCLUDED__
#define __TERRA_INTERFACE_INL_INCLUDED__
#include "..\terra\terra.h"
#include "IncTerra.h"

class StandartTerraInterface : public TerraInterface
{
	float inv_vx_fraction;
public:
	StandartTerraInterface()
	{
		inv_vx_fraction=1.0f/float(1<<VX_FRACTION);

	}
	virtual void postInit(class cTileMap* tm){
		vMap.linking2TileMap(tm);
	}

	int SizeX(){return vMap.H_SIZE;}
	int SizeY(){return vMap.V_SIZE;}
	int GetZ(int x,int y)
	{
		return vMap.GetAltWhole(x,y);
	}

	float GetZf(int x,int y)
	{
		int ofs=x+vMap.H_SIZE*y;
		return  vMap.GetAlt(ofs)*inv_vx_fraction;
	}

	void GetZMinMax(int tile_x,int tile_y,int tile_dx,int tile_dy,BYTE& out_zmin,BYTE& out_zmax)
	{
		BYTE zmin=255,zmax=0;
		int shift=kmGrid;

		int dx=tile_dx>>shift,dy=tile_dy>>shift;

		int x_start=tile_x>>shift,y_start=tile_y>>shift;
		int x_end=x_start+dx,y_end=y_start+dy;
		for(int y=y_start;y<y_end;y++)
		{
			for(int x=x_start;x<x_end;x++)
			{
				BYTE z=vMap.getGridVoxel(x,y);
				if(z<zmin)
					zmin=z;
				if(z>zmax)
					zmax=z;
			}
		}

		out_zmin=zmin;
		out_zmax=zmax;
	}

	virtual class MultiRegion* GetRegion(){return NULL;}

	virtual void GetTileColor(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		for(int y = ystart; y < yend; y += step)
		{
			DWORD* tx=(DWORD*)Texture;
			int yy=min(max(0,y),vMap.clip_mask_y);;
			for (int x = xstart; x < xend; x += step)
			{
				int xx=min(max(0,x),vMap.clip_mask_x);
				DWORD color=vMap.getColor32(xx,yy);

				*tx = color|0xff000000;
				tx++;
			}
			Texture += pitch;
		}
		
	}

	virtual void GetTileZ(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step)
	{
		int size=pitch>>2;
		xassert((yend+step-ystart)<=size*step);
		xassert((xend+step-xstart)<=size*step);
		for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
		{
			int * tx=(int*)(Texture+ybuffer*pitch);
			int yy=min(max(0,y),vMap.clip_mask_y);;
			for (int x = xstart; x < xend; x += step,tx++)
			{
				int xx=min(max(0,x),vMap.clip_mask_x);
				int z=int(vMap.GetAlt(xx,yy))<<(8-VX_FRACTION);
				*tx=z;
			}
		}
	}
};

#endif
