#include "StdAfxRD.h"
#include "TileMap.h"
#include "Scene.h"
#include "ObjLibrary.h"
#include "MultiRegion.h"
#include "font.h"

cTileMap::cTileMap(cScene* pScene,TerraInterface* terra_) : cBaseGraphObject(0)
{
	tile_scale.set(1,1,1);
	TileMapTypeNormal=Option_TileMapTypeNormal;
	terra=terra_;
	SetAttr(ATTRCAMERA_REFLECTION);
	TileSize.set(TILEMAP_SIZE,TILEMAP_SIZE);
	TileNumber.set(0,0);
	Tile=0;

	tilesize.set(0,0,0);
	pTileMapRender=NULL;

	zeroplastnumber=0;
	enable_debug_rect=false;
	debug_fade_interval=500;

	float c=0.8f;
	SetDiffuse(sColor4f(c,c,c,1-c));

	gb_RenderDevice3D->tilemap_inv_size.x=1.0f/terra->SizeX();
	gb_RenderDevice3D->tilemap_inv_size.y=1.0f/terra->SizeY();
	update_zminmmax_time=1;

	SetScene(pScene);

	miniDetailTexRes_ = 8;
}
cTileMap::~cTileMap()
{
	RELEASE(terra);
	gb_RenderDevice3D->Delete(this);
	if(Tile) { delete [] Tile; Tile=0; }
}


//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cIUnkObj
//////////////////////////////////////////////////////////////////////////////////////////

void ReloadTilemapPool(cTileMapRender* pTileMapRender);

void cTileMap::PreDraw(cCamera *DrawNode)
{
	start_timer_auto();

	if(GetAttribute(ATTRUNKOBJ_IGNORE)) return;

	if(Option_TileMapTypeNormal!=TileMapTypeNormal)
	{
		TileMapTypeNormal=Option_TileMapTypeNormal;
		ReloadTilemapPool(pTileMapRender);
		gb_RenderDevice3D->RestoreShader();
	}


	BuildRegionPoint();

	DrawNode->Attach(SCENENODE_OBJECT_TILEMAP,this);
	gb_RenderDevice3D->PreDraw(this);
}

void cTileMap::Draw(cCamera *DrawNode)
{
	if(!Option_ShowType[SHOW_TILEMAP])
		return;
	start_timer_auto();

	cD3DRender *Render=gb_RenderDevice3D;
	if(DrawNode->GetAttribute(ATTRCAMERA_SHADOW))
	{
		Render->Draw(GetScene()); // рисовать источники света
	}else
	if(DrawNode->GetAttribute(ATTRCAMERA_SHADOWMAP))
	{
		if(Option_ShadowType==SHADOW_MAP_SELF)
			Render->Draw(this,ALPHA_TEST,true);
	}else
	if(DrawNode->GetAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
	{
		Render->Draw(this,ALPHA_NONE,false,true);
	}else
	{
		Render->Draw(this,ALPHA_NONE,false);
	}

	DrawLines();
}

//////////////////////////////////////////////////////////////////////////////////////////
// реализация cTileMap
//////////////////////////////////////////////////////////////////////////////////////////
void cTileMap::UpdateMap(const Vect2i& pos1,const Vect2i& pos2,int type)
{
	MTAuto enter(lock_update_rect);
	UpdateRect rc;
	rc.p1=pos1;
	rc.p2=pos2;
	rc.type=type;

	if(enable_debug_rect)
	{
		DebugRect rc;
		rc.p1=pos1;
		rc.p2=pos2;
		rc.time=debug_fade_interval;
		debug_rect.push_back(rc);
	}

	vector<UpdateMapData>::iterator it;
	if(type&UPDATEMAP_HEIGHT)
	FOR_EACH(func_update_map,it)
	{
		it->f(pos1,pos2,it->data);
	}

	//point_offset На столько точка может выступать за границы тайла (реально на 16 может для самого грубого тайла)
	const int point_offset=8;
	Vect2i sz(TileSize.x*TileNumber.x, TileSize.y*TileNumber.y);
	rc.p1.x=max(rc.p1.x-point_offset,0);
	rc.p2.x=min(rc.p2.x+point_offset,sz.x-1);
	rc.p1.y=max(rc.p1.y-point_offset,0);
	rc.p2.y=min(rc.p2.y+point_offset,sz.y-1);

	update_rect.push_back(rc);
}
void cTileMap::UpdateMap(const Vect2i& pos, float radius,int type) 
{ 
	Vect2i p1 = pos - Vect2i(radius, radius);
	Vect2i p2 = pos + Vect2i(radius, radius);
	Vect2i sz(TileSize.x*TileNumber.x, TileSize.y*TileNumber.y);
	if (p1.x<0) p1.x =0;
	if (p1.x>=sz.x) p1.x = sz.x-1;
	if (p1.y<0) p1.y =0;
	if (p1.y>=sz.y) p1.y = sz.y-1;

	if (p2.x<0) p2.x =0;
	if (p2.x>=sz.x) p2.x = sz.x-1;
	if (p2.y<0) p2.y =0;
	if (p2.y>=sz.y) p2.y = sz.y-1;
	UpdateMap(p1, p2,type); 
}

void cTileMap::SetBuffer(const Vect2i &size,int zeroplastnumber_)
{
	zeroplastnumber=zeroplastnumber_;
	xassert(zeroplastnumber>0);
	zeroplast_color.resize(zeroplastnumber);
	for(int i=0;i<zeroplastnumber;i++)
		zeroplast_color[i].set(1,1,1,1);

	if(Tile) { gb_RenderDevice3D->Delete(this); delete Tile; }
	TileNumber.set(size.x/GetTileSize().x,size.y/GetTileSize().y);
	VISASSERT(TileNumber.x*GetTileSize().x==size.x);
	VISASSERT(TileNumber.y*GetTileSize().y==size.y);

	Tile=new sTile[GetTileNumber().x*GetTileNumber().y];
	gb_RenderDevice3D->Create(this);

	UpdateMap(Vect2i(0,0), Vect2i(size.x-1,size.y-1));
}

sBox6f CalcZMinMax(const D3DXMATRIX* look)
{
	D3DXVECTOR3 in[8]=
	{
	D3DXVECTOR3(-1,	1,	0),
	D3DXVECTOR3(1,	1,	0),
	D3DXVECTOR3(-1,	-1,	0),
	D3DXVECTOR3(1,	-1,	0),

	D3DXVECTOR3(-1,	1,	1),
	D3DXVECTOR3(1,	1,	1),
	D3DXVECTOR3(-1,	-1,	1),
	D3DXVECTOR3(1,	-1,	1),
	};

	sBox6f box;
	box.SetInvalidBox();

	for(int i=0;i<8;i++)
	{
		D3DXVECTOR4 out;
		D3DXVec3Transform(&out,in+i,look);
		out.x/=out.w;
		out.y/=out.w;
		out.z/=out.w;
		out.w/=out.w;

		out.x/=out.z;
		out.y/=out.z;
		box.AddBound(Vect3f(out.x,out.y,out.z));
	}

	return box;
}

void cTileMap::CalcZMinMax(int x_tile,int y_tile)
{
/*
	BYTE zmin=255,zmax=0;
	int dx=GetTileSize().x,dy=GetTileSize().y;
	int xMap=dx*GetTileNumber().x;
	int yMap=dy*GetTileNumber().y;
	for(int y=0;y<dy;y++)
	{
		int ofs=x_start*dx+(y+y_start*dy)*xMap;
		for(int x=0;x<dx;x++,ofs++)
		{
			BYTE z=vMap_GetZ(ofs);
			if(z<zmin)
				zmin=z;
			if(z>zmax)
				zmax=z;
		}
	}
/*/

	BYTE zmin=255,zmax=0;
	terra->GetZMinMax(x_tile*GetTileSize().x,y_tile*GetTileSize().y,GetTileSize().x,GetTileSize().y,zmin,zmax);
/**/
	sTile& s=GetTile(x_tile,y_tile);
	s.zmin=zmin;
	s.zmax=zmax;
}


Vect2f cTileMap::CalcZ(cCamera *DrawNode)
{
	float tx=GetTileSize().x * GetTileScale().x;
	float ty=GetTileSize().y * GetTileScale().y;

	Vect2f z(1e20f,1e-20f);

	for(int x=0;x<TileNumber.x;x++)
	for(int y=0;y<TileNumber.y;y++)
	{
		sTile& s=GetTile(x,y);
		Vect3f c0,c1;
		c0.x=x*tx;
		c0.y=y*ty;
		c0.z=s.zmin;
		c1.x=c0.x+tx;
		c1.y=c0.y+ty;
		c1.z=s.zmax;

		if(DrawNode->TestVisible(c0,c1))//Не оптимально, лучше чераз обращение к pTestGrid
		{
			Vect3f p[8]=
			{
				Vect3f(c0.x,c0.y,c0.z),
				Vect3f(c1.x,c0.y,c0.z),
				Vect3f(c0.x,c1.y,c0.z),
				Vect3f(c1.x,c1.y,c0.z),
				Vect3f(c0.x,c0.y,c1.z),
				Vect3f(c1.x,c0.y,c1.z),
				Vect3f(c0.x,c1.y,c1.z),
				Vect3f(c1.x,c1.y,c1.z),
			};

			for(int i=0;i<8;i++)
			{
				Vect3f o=DrawNode->GetMatrix()*p[i];
				if(o.z<z.x)
					z.x=o.z;
				if(o.z>z.y)
					z.y=o.z;
			}
		}
	}

	if(z.y<z.x)
	{
		z=DrawNode->GetZPlane();
	}else
	{
		float addz=(z.y-z.x)*1e-2;
		addz=max(addz,10.0f);
		z.x-=addz;
		z.y+=addz;
		if(z.x<DrawNode->GetZPlane().x)
			z.x=DrawNode->GetZPlane().x;
		if(z.y>DrawNode->GetZPlane().y)
			z.y=DrawNode->GetZPlane().y;
	}

	return z;
}


sBox6f cTileMap::CalcShadowReciverInSpace(cCamera *DrawNode,D3DXMATRIX matrix)
{
	sBox6f box;
	box.SetInvalidBox();
	float tx=GetTileSize().x * GetTileScale().x;
	float ty=GetTileSize().y * GetTileScale().y;

	for(int x=0;x<TileNumber.x;x++)
	for(int y=0;y<TileNumber.y;y++)
	{
		sTile& s=GetTile(x,y);
		Vect3f c0,c1;
		c0.x=x*tx;
		c0.y=y*ty;
		c0.z=s.zmin;
		c1.x=c0.x+tx;
		c1.y=c0.y+ty;
		c1.z=s.zmax;

		if(DrawNode->TestVisible(c0,c1))
		{
			Vect3f p[8]=
			{
				Vect3f(c0.x,c0.y,c0.z),
				Vect3f(c1.x,c0.y,c0.z),
				Vect3f(c0.x,c1.y,c0.z),
				Vect3f(c1.x,c1.y,c0.z),
				Vect3f(c0.x,c0.y,c1.z),
				Vect3f(c1.x,c0.y,c1.z),
				Vect3f(c0.x,c1.y,c1.z),
				Vect3f(c1.x,c1.y,c1.z),
			};

			for(int i=0;i<8;i++)
			{
				Vect3f out_pos;
				D3DXVec3TransformCoord((D3DXVECTOR3*) &out_pos, (D3DXVECTOR3*)&p[i], &matrix);
				box.AddBound(out_pos);
			}
		}
	}

	return box;
}


void cTileMap::BuildRegionPoint()
{

	{
		MTAuto enter(lock_update_rect);

		int counter = 0;
		int counterRegion = 0;

		vector<UpdateRect>::iterator it;
		FOR_EACH(update_rect,it)
		{
			UpdateRect& r=*it;
			Vect2i& pos1=r.p1;
			Vect2i& pos2=r.p2;

			VISASSERT(pos1.y<=pos2.y&&pos1.x<=pos2.x);
			int dx=GetTileSize().x,dy=GetTileSize().y;
			int j1=pos1.y/dy,j2=min(pos2.y/dy,GetTileNumber().y-1);
			int i1=pos1.x/dx,i2=min(pos2.x/dx,GetTileNumber().x-1);
			for(int j=j1;j<=j2;j++)
			for(int i=i1;i<=i2;i++)
			{
				sTile& p=GetTile(i,j);
				if((r.type&UPDATEMAP_HEIGHT) && p.update_zminmmax_time!=update_zminmmax_time)
				{
					p.update_zminmmax_time=update_zminmmax_time;
					CalcZMinMax(i,j);
					counter++;
				}
				if(r.type&(UPDATEMAP_HEIGHT|UPDATEMAP_REGION))
					p.SetAttribute(ATTRTILE_UPDATE_VERTEX);
				if(r.type&UPDATEMAP_TEXTURE)
					p.SetAttribute(ATTRTILE_UPDATE_TEXTURE);
			}
			counterRegion++;
		}

		update_rect.clear();

		statistics_add(TilesUpdated, counter);
		statistics_add(RegionsUpdated, counterRegion);
	}

	update_zminmmax_time++;
}

Vect3f cTileMap::To3D(const Vect2f& pos)
{
	Vect3f p;
	p.x=pos.x;
	p.y=pos.y;

	int x = round(pos.x), y = round(pos.y);
	if(x >= 0 && x < terra->SizeX() && y >= 0 && y < terra->SizeY())
	{
		p.z=terra->GetZ(x,y);
	}else
		p.z=0;
	return p;
}

void cTileMap::Animate(float dt)
{
	if(enable_debug_rect)
	{
		for(list<DebugRect>::iterator it=debug_rect.begin();it!=debug_rect.end();)
		{
			DebugRect& r=*it;
			r.time-=dt;
			if(r.time<0)
				it=debug_rect.erase(it);
			else
				it++;
		}
	}
}

void cTileMap::DrawLines()
{
	if(enable_debug_rect)
	{
		list<DebugRect>::iterator it;
		FOR_EACH(debug_rect,it)
		{
			DebugRect& r=*it;
			sColor4c c(255,255,255,round(255*r.time/debug_fade_interval));
			Vect3f p0,p1,p2,p3;
			p0=To3D(Vect2f(r.p1.x,r.p1.y));
			p1=To3D(Vect2f(r.p2.x,r.p1.y));
			p2=To3D(Vect2f(r.p2.x,r.p2.y));
			p3=To3D(Vect2f(r.p1.x,r.p2.y));

			gb_RenderDevice->DrawLine(p0,p1,c);
			gb_RenderDevice->DrawLine(p1,p2,c);
			gb_RenderDevice->DrawLine(p2,p3,c);
			gb_RenderDevice->DrawLine(p3,p0,c);
		}
	}
}

void cTileMap::RegisterUpdateMap(UpdateMapFunction f,void* data)
{
	UpdateMapData d;
	d.f=f;
	d.data=data;
	vector<UpdateMapData>::iterator it=find(func_update_map.begin(),func_update_map.end(),d);
	bool is=it!=func_update_map.end();
	xassert(!is);
	if(is)
		return;
	func_update_map.push_back(d);
}

void cTileMap::UnRegisterUpdateMap(UpdateMapFunction f,void* data)
{
	UpdateMapData d;
	d.f=f;
	d.data=data;
	vector<UpdateMapData>::iterator it=find(func_update_map.begin(),func_update_map.end(),d);
	bool is=it!=func_update_map.end();
	xassert(is);
	if(!is)
		return;
	func_update_map.erase(it);
}
