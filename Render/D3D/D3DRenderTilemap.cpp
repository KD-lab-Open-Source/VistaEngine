#include "StdAfxRD.h"
#include "IncTerra.h"
#include "TileMap.h"
#include "Scene.h"
#include "CChaos.h"
#include "..\src\MultiRegion.h"

#include "clip\ClippingMeshFixed.h"

#define BUMP_IDXTYPE   unsigned short
#define BUMP_IDXSIZE   sizeof(BUMP_IDXTYPE)

static int bumpGeoScale[TILEMAP_LOD] = { 1, 2, 3, 4, 5 };
static int bumpTexScale[TILEMAP_LOD] = { 0, 0, 1, 2, 3 };

const int texture_border=2;

unsigned int ColorByNormalRGBA(Vect3f n);
unsigned short ColorByNormalWORD(Vect3f n);
unsigned int ColorByNormalDWORD(Vect3f n);

static CMesh cmesh_to_minimize_alloc;

struct TexPoolsHandle
{
	int pool;
	int page;
	TexPoolsHandle()
	{
		pool=-1;
		page=-1;
	}

	bool IsInitialized()
	{
		return pool>=0;
	}
};

struct sBumpTile;

class sTilemapTexturePool
{
	IDirect3DTextureProxy* texture;
	int tileWidth, tileHeight;
	int tileRealWidth, tileRealHeight;
	int freePages;
	int *freePagesList;

	int texture_width,texture_height;
	vector<Vect2i> Pages;//x-xmin, y-ymin
	float ustep,vstep;
	D3DFORMAT format;
public:
	vector<sBumpTile*> tileRenderList;//Сюда записываются тайлы, которые рендерят нормальное/отраженное изображение.

	sTilemapTexturePool(int width, int height,D3DFORMAT format);
	~sTilemapTexturePool();
	int allocPage();
	void freePage(int page);
	D3DLOCKED_RECT *lockPage(int page);
	void unlockPage(int page);
	Vect2f getUVStart(int page);
	inline Vect2f getUVStep(){return Vect2f(ustep,vstep);};
	D3DFORMAT getFormat()const{return format;}

	inline int GetTileWidth(){return tileWidth;}
	inline int GetTileHeight(){return tileHeight;}
	inline int IsFree(){return freePages;}
	inline IDirect3DTextureProxy* GetTexture(){return texture;}

	void GetTextureMemory(int &total,int& free);
protected:
	void CalcPagesPos();
};

class cTexPools
{
public:
	vector<sTilemapTexturePool*> bumpTexPools;

	vector<struct sBumpTile*> tileRenderList;//Сюда записываются тайлы, которые рендерят тень.

	cTexPools(){};
	~cTexPools(){clear();}

	void GetTilemapTextureMemory(int& total,int& free);
	void clear();
	void FindFreeTexture(TexPoolsHandle& h,int tex_width,int tex_height,D3DFORMAT format);
	Vect2f getUVStart(TexPoolsHandle& h);
	Vect2f getUVStep(TexPoolsHandle& h);

	void freePage(TexPoolsHandle& h);
	D3DLOCKED_RECT *LockPage(TexPoolsHandle& h);
	void UnlockPage(TexPoolsHandle& h);

	void AddToRenderList(TexPoolsHandle& h,struct sBumpTile* tile);
	void ClearRenderList();
};

static cTexPools TexPools;

struct sPlayerIB
{
	IndexPoolPage index;
	int nindex;
	int player;
};

struct VectDelta:public Vect2i
{
	int delta2;
	Vect2i delta;
	char player;

	enum
	{
		FIX_RIGHT=1,
		FIX_BOTTOM=2,
		FIX_FIXED=4,
		FIX_FIXEMPTY=8,
	};
	char fix;

	void copy_no_fix(const VectDelta& p)
	{
		x=p.x;
		y=p.y;
		delta2=p.delta2;
		delta=p.delta;
		player=p.player;
	}
};

enum U_E
{
	U_LEFT=0,
	U_RIGHT=1,
	U_TOP=2,
	U_BOTTOM=3,
	U_LEFT_TOP=4,
	U_RIGHT_TOP=5,
	U_LEFT_BOTTOM=6,
	U_RIGHT_BOTTOM=7,
	U_ALL=8,
};

struct sBumpTile
{
	//Only to read
	VertexPoolPage vtx;
	vector<sPlayerIB> index;
	Vect2i tile_pos;

	TexPoolsHandle tex;
	TexPoolsHandle normal;

	char border_lod[U_ALL];
	char lod_vertex;
	char lod_texture;
	bool init_vertex;
	bool init_texture;
protected:
	Vect2f tex_uvStart;
	Vect2f tex_uvStartBump;
public:
	sBumpTile(int xpos,int ypos);
	~sBumpTile();

	void InitVertex(cTileMap *TileMap, int lod_vertex);
	void InitTexture(cTileMap *TileMap,int lod_texture);
	void DeleteVertex();
	void DeleteTexture();

	bool isTexture(){return tex.pool!=-1;}

	D3DLOCKED_RECT *LockTex();
	BYTE *LockVB();
	void UnlockTex();
	void UnlockVB();
	void CalcTexture(cTileMap *TileMap);
	void CalcVertex(cTileMap *TileMap);

	inline Vect2f GetUVStart(){return tex_uvStart;};
	inline Vect2f GetUVStep(){return uv_step;};

	Vect2f uv_base,uv_step;
	Vect2f uv_base_bump;

	static IDirect3DVertexDeclaration9* GetVertexDeclaration() {return Option_TileMapTypeNormal?shortVertexXYZ::declaration:shortVertexXYZD::declaration;}
protected:

	int FixLine(VectDelta* points,int ddv,cTileMap *TileMap);

	inline float SetVertexZ(TerraInterface* terra,int x,int y);
	void DeleteIndex();

	void GetNormalCenter(cTileMap *TileMap,int x,int y,int step,Vect3f& out);

	bool CollapsePoint(Vect2i& pround,int dx,int dy);
	void TestPoint(Vect2i pos);
};

#include "CollapsePoint.inl"

class cTileMapRender
{
	cTileMap *TileMap;
	vector<sBumpTile*> bumpTiles;
	vector<int> bumpDyingTiles;
	LPDIRECT3DINDEXBUFFER9 tilemapIB;
	int index_offset[TILEMAP_LOD];
	int index_size[TILEMAP_LOD];

	BYTE* visMap;
	char* vis_lod;

	char* update_stat;
	bool update_in_frame;

	void SaveUpdateStat();

	VectDelta* delta_buffer;
	vector<vector<sPolygon> > index_buffer;
public:
	void IncUpdate(sBumpTile* pbump);

	cTileMapRender(cTileMap *pTileMap);
	~cTileMapRender();

	void ClearTilemapPool();
	void RestoreTilemapPool();

	int bumpNumIndex(int lod)
	{
		int offset=lod-bumpGeoScale[0];
		xassert(offset>=0 && offset<TILEMAP_LOD);
		return index_size[offset];
	}

	int bumpIndexOffset(int lod)
	{
		return index_offset[lod-bumpGeoScale[0]];
	}

	int bumpNumVertices(int lod);
	void bumpCreateIB(sPolygon* ib, int lod);
	int bumpTileValid(int id);
	int bumpTileAlloc(int lod,int xpos,int ypos);
	void bumpTileFree(int id);
	void bumpTilesDeath();

	void PreDraw(cCamera* DrawNode);
	void DrawBump(cCamera* DrawNode,eBlendMode MatMode,bool shadow,bool zbuffer);

	sBumpTile* GetTile(int k,int n)
	{
		VISASSERT(k>=0 && k<TileMap->GetTileNumber().x);
		VISASSERT(n>=0 && n<TileMap->GetTileNumber().y);
		int bumpTileID = TileMap->GetTile(k, n).bumpTileID;
		if(bumpTileID>=0)
			return bumpTiles[bumpTileID];
		return NULL;
	}
	
	VectDelta* GetDeltaBuffer(){return delta_buffer;};
	vector<vector<sPolygon> >& GetIndexBuffer(){return index_buffer;};

	void UpdateLine(int dy,int dx);
	void SetTilesToRender(cCamera* pNormalCamera,bool shadow);
};

void cTexPools::GetTilemapTextureMemory(int& total,int& free)
{
	vector<sTilemapTexturePool*>::iterator it;
	total=free=0;
	FOR_EACH(bumpTexPools,it)
	{
		int cur_total,cur_free;
		(*it)->GetTextureMemory(cur_total,cur_free);
		total+=cur_total;
		free+=cur_free;
	}
}

void cTexPools::clear()
{
	for(int i=0;i<bumpTexPools.size();i++)
		delete bumpTexPools[i];
	bumpTexPools.clear();
}

void cTexPools::FindFreeTexture(TexPoolsHandle& h,int tex_width,int tex_height,D3DFORMAT format)
{
	int i;
	for (i = 0; i < bumpTexPools.size(); i++)
	{
		sTilemapTexturePool* p=bumpTexPools[i];
		if (p && p->IsFree()
			&& p->GetTileWidth() == tex_width
			&& p->GetTileHeight() == tex_height &&
			p->getFormat()==format)
				break;
	}

	if (i >= bumpTexPools.size())
	{
		for (i = 0; i < bumpTexPools.size(); i++)
			if (bumpTexPools[i] == NULL) break;
		if (i >= bumpTexPools.size())
		{
			i = bumpTexPools.size();
			bumpTexPools.push_back(NULL);
		}
		bumpTexPools[i] = new sTilemapTexturePool(tex_width, tex_height,format);
	}

	h.pool = i;
	h.page = bumpTexPools[i]->allocPage();
}

Vect2f cTexPools::getUVStart(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->getUVStart(h.page);
}

Vect2f cTexPools::getUVStep(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->getUVStep();
}

void cTexPools::freePage(TexPoolsHandle& h)
{
	if(!h.IsInitialized())
		return;
	bumpTexPools[h.pool]->freePage(h.page);
	h.pool=-1;
	h.page=-1;
}

D3DLOCKED_RECT *cTexPools::LockPage(TexPoolsHandle& h)
{
	return bumpTexPools[h.pool]->lockPage(h.page);
}

void cTexPools::UnlockPage(TexPoolsHandle& h)
{
	bumpTexPools[h.pool]->unlockPage(h.page);
}

void cTexPools::AddToRenderList(TexPoolsHandle& h,struct sBumpTile* tile)
{
	bumpTexPools[h.pool]->tileRenderList.push_back(tile);
}

void cTexPools::ClearRenderList()
{
	for (int i = 0; i < bumpTexPools.size(); i++)
		bumpTexPools[i]->tileRenderList.clear();
	tileRenderList.clear();
}
//
// **************** TILEMAP - SHARED ****************
// 
static void fillVisPoly(BYTE *buf,Vect2f* vert,int vert_size,int VISMAP_W,int VISMAP_H)
{
	MTG(); 
	//Временно для редактора xassert(!MT_IS_LOGIC());
	if(vert_size==0)return;
	const int VISMAP_W_MAX=128,VISMAP_H_MAX=128;
	VISASSERT(VISMAP_W<=VISMAP_W_MAX && VISMAP_H<=VISMAP_H_MAX);
	float lx[VISMAP_W_MAX], rx[VISMAP_H_MAX];
	int i, y, ytop, ybot;

	// find top/bottom y
	ytop = floor(vert[0].y);
	ybot = ceil(vert[0].y);
	for(i=1;i<vert_size;i++)
	{
		float y=vert[i].y;
		if (y < ytop) ytop = floor(y);
		if (y > ybot) ybot = ceil(y);
	}

	for (i = 0; i < VISMAP_H; i++)
	{
		lx[i] = VISMAP_W-1;
		rx[i] = 0;
	}

	// render edges
	for (i = 0; i < vert_size; i++)
	{
		int i2=(i+1>=vert_size)?0:i+1;
		float x1, x2, y1, y2, t;
		x1=vert[i].x;  y1=vert[i].y;
		x2=vert[i2].x; y2=vert[i2].y;

		if (y1 > y2) { t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }

		int iy1 = (int)y1, iy2 = (int)y2;
		if(iy1>iy2)continue;

		float dy = (y2 == y1) ? 1 : (y2 - y1);
		float dy1 =1/dy;
		for (y = max(iy1, 0); y <= min(iy2, VISMAP_H-1); y++)
		{
			float ix1 = x1 + (y-y1) * (x2-x1) * dy1;
			float ix2 = x1 + (y-y1+1) * (x2-x1) * dy1;
			if (y == iy1) ix1 = x1;
			if (y == iy2) ix2 = x2;
			lx[y] = min(min(lx[y], ix1), ix2);
			rx[y] = max(max(rx[y], ix1), ix2);
		}
	}

	// fill the buffer
	for (y = max(0, ytop); y <= min(ybot, VISMAP_H-1); y++)
	{
		if (lx[y] > rx[y]) continue;
		int x1 = (int)max((float)floor(lx[y]), 0.0f);
		int x2 = (int)min((float)ceil(rx[y]), (float)VISMAP_W);
		if(x1>=x2)continue;
		memset(buf + y*VISMAP_W + x1, 1, x2-x1);
	}
}
void calcCMesh(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,CMesh& cmesh)
{
	float dx=TileNumber.x*TileSize.x;
	float dy=TileNumber.y*TileSize.y;

	cmesh.CreateABB(Vect3f(0,0,0),Vect3f(dx,dy,256.0f));

	for(int i=0;i<DrawNode->GetNumberPlaneClip3d();i++)
	{
		sPlane4f& plane=DrawNode->GetPlaneClip3d(i);
		cmesh.Clip(plane);
	}
}

void drawCMesh(CMesh& cmesh)
{
	for(int i=0;i<cmesh.E.size();i++)
	if(cmesh.E[i].visible)
	{
		int iv0=cmesh.E[i].vertex[0];
		int iv1=cmesh.E[i].vertex[1];
		Vect3f v0,v1;
		v0=cmesh.V[iv0].point;
		v1=cmesh.V[iv1].point;
		gb_RenderDevice->DrawLine(v0,v1,sColor4c(0,0,0,255));
	}
}
/*
void calcVisMap(cCamera *DrawNode, CMesh& cmesh, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear)
{
	APolygons poly;
	cmesh.BuildPolygon(poly);
	if(clear)
		memset(visMap, 0, TileNumber.x*TileNumber.y);

	for(int i=0;i<poly.faces.size();i++)
	{
		vector<int>& inp=poly.faces[i];
		vector<Vect2f> points(inp.size());
		for(int j=0;j<inp.size();j++)
		{
			Vect3f& p=poly.points[inp[j]];
			points[j].x=p.x/TileSize.x;
			points[j].y=p.y/TileSize.y;
		}
		
		fillVisPoly(visMap, &points[0],points.size(),TileNumber.x,TileNumber.y);
	}

	if(false)
		drawCMesh(cmesh);
}
/*/
void calcVisMap(cCamera *DrawNode, CMesh& cmesh, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear)
{
	static APolygons poly;
	cmesh.BuildPolygon(poly);
	if(clear)
		memset(visMap, 0, TileNumber.x*TileNumber.y);

	for(int i=0;i<poly.faces_flat.size();)
	{
		int inp_size=poly.faces_flat[i];
		int* inp=&poly.faces_flat[i+1];
		i+=inp_size+1;
		static vector<Vect2f> points;
		points.resize(inp_size);
		for(int j=0;j<inp_size;j++)
		{
			Vect3f& p=poly.points[inp[j]];
			points[j].x=p.x/TileSize.x;
			points[j].y=p.y/TileSize.y;
		}
		
		fillVisPoly(visMap, &points[0],points.size(),TileNumber.x,TileNumber.y);
	}

	if(false)
		drawCMesh(cmesh);
}
/**/

sBox6f calcBoundInDirection(CMesh& cmesh,const Mat3f& m)
{
	sBox6f box;
	box.SetInvalidBox();
	for(int i=0;i<cmesh.E.size();i++)
	if(cmesh.E[i].visible)
	{
		for(int j=0;j<2;j++)
		{
			int iv=cmesh.E[i].vertex[j];
			Vect3f v;
			v=m*cmesh.V[iv].point;
			box.AddBound(v);
		}
	}
	return box;
}

void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,BYTE* visMap,bool clear)
{
	CMesh& cmesh=cmesh_to_minimize_alloc;
	calcCMesh(DrawNode,TileNumber,TileSize,cmesh);
	calcVisMap(DrawNode,cmesh,TileNumber,TileSize,visMap,clear);
}

void calcVisMap(cCamera *DrawNode, cTileMap *TileMap,BYTE* visMap,bool clear)
{
	calcVisMap(DrawNode, TileMap->GetTileNumber(),TileMap->GetTileSize(),visMap,clear);
}

void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,const Mat3f& direction,sBox6f& box)
{
	CMesh& cmesh=cmesh_to_minimize_alloc;
	calcCMesh(DrawNode,TileNumber,TileSize,cmesh);
	box=calcBoundInDirection(cmesh,direction);
}

sBox6f calcBoundInDirection(CMesh& cmesh,const D3DXMATRIX& m)
{
	sBox6f box;
	box.SetInvalidBox();
	for(int i=0;i<cmesh.E.size();i++)
	if(cmesh.E[i].visible)
	{
		for(int j=0;j<2;j++)
		{
			int iv=cmesh.E[i].vertex[j];
			Vect3f v;
			D3DXVec3TransformCoord((D3DXVECTOR3*) &v, (D3DXVECTOR3*)&cmesh.V[iv].point, &m);
			box.AddBound(v);
		}
	}
	return box;
}

void calcVisMap(cCamera *DrawNode, Vect2i TileNumber,Vect2i TileSize,const D3DXMATRIX& direction,sBox6f& box)
{
	CMesh& cmesh=cmesh_to_minimize_alloc;
	calcCMesh(DrawNode,TileNumber,TileSize,cmesh);
	box=calcBoundInDirection(cmesh,direction);
}


int cD3DRender::Create(class cTileMap *TileMap)
{
	cTileMapRender* p=new cTileMapRender(TileMap);
	TileMap->SetTilemapRender(p);
	p->RestoreTilemapPool();
	tilemaps.push_back(p);
	return 0;
}

void cD3DRender::GetTilemapTextureMemory(int& total,int& free)
{
	TexPools.GetTilemapTextureMemory(total,free);
}

void sTilemapTexturePool::GetTextureMemory(int &total,int& free)
{
	int byte_per_pixel=GetTextureFormatSize(format)/8;
	total=texture_width*texture_height*byte_per_pixel;
	int used=(Pages.size()-freePages)*tileRealWidth*tileRealHeight*byte_per_pixel;
	free=total-used;
}

int cD3DRender::Delete(class cTileMap *TileMap)
{
	cTileMapRender* p=TileMap->GetTilemapRender();
	if(p==NULL)
		return true;
	
	p->ClearTilemapPool();
	for(int i=0;i<tilemaps.size();i++)
	{
		if(tilemaps[i]==p)
		{
			tilemaps.erase(tilemaps.begin()+i);
			delete p;
			return true;
		}
	}

	VISASSERT(0);
	return false;
}

void cD3DRender::Draw(cTileMap *TileMap,eBlendMode MatMode,bool shadow,bool zbuffer)
{
	TileMap->GetTilemapRender()->DrawBump(DrawNode,MatMode,shadow,zbuffer);
}

void cD3DRender::ClearTilemapPool()
{
	vector<cTileMapRender*>::iterator it;
	FOR_EACH(tilemaps,it)
		(*it)->ClearTilemapPool();
}

void cD3DRender::RestoreTilemapPool()
{
	vector<cTileMapRender*>::iterator it;
	FOR_EACH(tilemaps,it)
		(*it)->RestoreTilemapPool();
}

/////////////////////cTileMapRender////////////////////

cTileMapRender::cTileMapRender(cTileMap *pTileMap)
{
	TileMap=pTileMap;
	tilemapIB=NULL;

	int dxy=TileMap->GetTileNumber().x*TileMap->GetTileNumber().y;
	visMap=new BYTE[dxy];
	vis_lod=new char[dxy];
	for(int i=0;i<dxy;i++)
		vis_lod[i]=-1;

	update_stat=NULL;
//	update_stat=new char[dxy*TILEMAP_LOD];
	update_in_frame=false;

	int ddv=TILEMAP_SIZE+1;
	delta_buffer=new VectDelta[ddv*ddv];

}

cTileMapRender::~cTileMapRender()
{
	RELEASE(tilemapIB);
	delete visMap;
	delete vis_lod;

	delete update_stat;
	delete delta_buffer;

	vector<sBumpTile*>::iterator it;
	FOR_EACH(bumpTiles,it)
		delete *it;
	bumpTiles.clear();
}

void cTileMapRender::ClearTilemapPool()
{
	for(int y=0;y<TileMap->GetTileNumber().y;y++)
	for(int x=0;x<TileMap->GetTileNumber().x;x++)
	{
		sTile &Tile = TileMap->GetTile(x, y);
		int &bumpTileID = Tile.bumpTileID;
		bumpTileID = -1;
	}

	bumpDyingTiles.clear();
	for(int i=0;i<bumpTiles.size();i++)
	{
		delete bumpTiles[i];
	}
	bumpTiles.clear();

	RELEASE(tilemapIB);

	TexPools.clear();
}

void cTileMapRender::RestoreTilemapPool()
{
	if(tilemapIB)
		return;
	int cur_offset=0;
	Vect2i TileSize=TileMap->GetTileSize();

	int iLod;
	for (iLod = 0; iLod < TILEMAP_LOD; iLod++)
	{
		int xs = TileSize.x >> bumpGeoScale[iLod];
		int ys = TileSize.y >> bumpGeoScale[iLod];

		index_offset[iLod]=cur_offset;
		index_size[iLod]=6*xs*ys;//?????? может 2 а не 6
		cur_offset+=index_size[iLod];
	}

	sPolygon *pIndex = 0;
	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateIndexBuffer(
		cur_offset * BUMP_IDXSIZE, D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT,
		&tilemapIB,NULL));
	RDCALL(tilemapIB->Lock(0, 0, (void**)&pIndex,
		D3DLOCK_NOSYSLOCK));
	for (iLod = 0; iLod < TILEMAP_LOD; iLod++)
	{
		int lod_vertex=bumpGeoScale[iLod];
		bumpCreateIB(pIndex+bumpIndexOffset(lod_vertex)/3, lod_vertex);
	}
	tilemapIB->Unlock();
}

void cTileMapRender::bumpCreateIB(sPolygon *ib, int lod)
{
	Vect2i TileSize=TileMap->GetTileSize();
	VISASSERT(TileSize.x==TileSize.y);

	int dd = (TileSize.x >> lod);
	int ddv=dd+1;
	for(int y=0;y<dd;y++)
	for(int x=0;x<dd;x++)
	{
		int base=x+y*ddv;
		ib++->set(base,base+ddv,base+1);
		ib++->set(base+ddv,base+ddv+1,base+1);
	}

}

void cTileMapRender::PreDraw(cCamera* DrawNode)
{
	for(int y=0;y<TileMap->GetTileNumber().y;y++)
	for(int x=0;x<TileMap->GetTileNumber().x;x++)
	{
		sTile &Tile = TileMap->GetTile(x, y);
		int &bumpTileID = Tile.bumpTileID;
		if(!Tile.GetAttribute(ATTRTILE_DRAW_VERTEX))
		{
			bumpTileFree(bumpTileID);
			bumpTileID = -1;
		}

		Tile.ClearAttribute(ATTRTILE_DRAW_VERTEX);
	}

	if(update_stat)
	{
		SaveUpdateStat();
		update_in_frame=false;
		int dxy=TileMap->GetTileNumber().x*TileMap->GetTileNumber().y;
		memset(update_stat,0,dxy*TILEMAP_LOD);
	}

	bumpTilesDeath();
}

int cTileMapRender::bumpNumVertices(int lod)
{
	int xs = (TileMap->GetTileSize().x >> lod);
	int ys = (TileMap->GetTileSize().y >> lod);

	return (xs+1)*(ys+1);
}

int cTileMapRender::bumpTileValid(int id)
{
	return ((id >= 0) && (id < bumpTiles.size()) && (bumpTiles[id]));
}

int cTileMapRender::bumpTileAlloc(int lod,int xpos,int ypos)
{
	int i;
	for (i = 0; i < bumpTiles.size(); i++)
		if (!bumpTiles[i]) break;
	if (i == bumpTiles.size()) bumpTiles.push_back(NULL);
	bumpTiles[i] = new sBumpTile(xpos, ypos);
	bumpTiles[i]->InitVertex(TileMap, bumpGeoScale[lod]);
	return i;
}

void cTileMapRender::bumpTileFree(int id)
{
	if (bumpTileValid(id))
		bumpDyingTiles.push_back(id);
}

void cTileMapRender::bumpTilesDeath()
{
	for (int i = 0; i < bumpDyingTiles.size(); i++)
	{
		int tile=bumpDyingTiles[i];
		//if (++bumpTiles[tile]->age > 4)
		{
			delete bumpTiles[tile];
			bumpTiles[tile] = NULL;
			bumpDyingTiles.erase(bumpDyingTiles.begin() + i);
			i--;
		}
	}
}


///////////////////////////sTilemapTexturePool/////////////////////////////////
sTilemapTexturePool::sTilemapTexturePool(int width, int height,D3DFORMAT format_)
{
	format=format_;
	texture_width=texture_height=512;
	tileWidth = width;
	tileHeight = height;

	tileRealWidth=tileWidth+texture_border*2;
	tileRealHeight=tileHeight+texture_border*2;
	ustep=1.0f/texture_height;
	vstep=1.0f/texture_width;

	CalcPagesPos();

	freePagesList = new int[Pages.size()];
	freePages = Pages.size();
	for (int i = 0; i < Pages.size(); i++)
		freePagesList[i] = i;

	IDirect3DTexture9* pTex=NULL;
	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateTexture(
		texture_width, texture_height, 1,
		0, format, D3DPOOL_MANAGED, &pTex,NULL));
	texture=CREATEIDirect3DTextureProxy(pTex);
	//D3DFMT_A8R8G8B8
/* Этот кусок теоретически лучше, но надо проверить на Geforce 2
	RDCALL(renderer->lpD3DDevice->CreateTexture(
		texture_width, texture_height, 1,
		D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture,NULL));
*/
}

sTilemapTexturePool::~sTilemapTexturePool()
{
	delete[] freePagesList;
	texture->Release();
}

void sTilemapTexturePool::CalcPagesPos()
{
	for(int y=0;y<=texture_height-tileRealHeight;y+=tileRealHeight)
	{
		for(int x=0;x<=texture_width-tileRealHeight;x+=tileRealWidth)
		{
			Vect2f page;
			page.x=x;
			page.y=y;
			Pages.push_back(page);
		}
	}
}

int sTilemapTexturePool::allocPage()
{
	if (!freePages) return -1;
	freePages--;
	return freePagesList[freePages];
}

void sTilemapTexturePool::freePage(int page)
{
	xassert(page>=0 && page<Pages.size());
	freePagesList[freePages] = page;
	freePages++;
}

D3DLOCKED_RECT *sTilemapTexturePool::lockPage(int page)
{
	static D3DLOCKED_RECT lockedRect;
	Vect2i pos=Pages[page];
	RECT rect;

	rect.left   = pos.x;
	rect.top    = pos.y;
	rect.right  = rect.left+ tileRealWidth;
	rect.bottom = rect.top + tileRealHeight;
	RDCALL(texture->LockRect(0, &lockedRect, &rect, 0));
	return &lockedRect;
}

void sTilemapTexturePool::unlockPage(int page)
{
	RDCALL(texture->UnlockRect(0));
}

Vect2f sTilemapTexturePool::getUVStart(int page)
{
	VISASSERT(page>=0 && page<Pages.size());
	Vect2i pos=Pages[page];
	return Vect2f((pos.x+texture_border)*ustep,(pos.y+texture_border)*vstep);
}

//////////////////////////sBumpTile//////////////////////////////////////
float sBumpTile::SetVertexZ(TerraInterface* terra,int x,int y)
{
	float zi=terra->GetZf(x,y);
	if(cChaos::CurrentChaos() && zi<1)
		zi=-15;
	return zi;
}

sBumpTile::sBumpTile(int xpos,int ypos)
{
	tile_pos.set(xpos,ypos);
	for(int i=0;i<U_ALL;i++)
		border_lod[i]=0;
	lod_vertex=lod_texture=0;

	init_vertex=false;
	init_texture=false;

	tex_uvStart.set(0,0);
	tex_uvStartBump.set(0,0);
	uv_base.set(0,0);
	uv_step.set(0,0);
	uv_base_bump.set(0,0);
}

sBumpTile::~sBumpTile()
{
	DeleteVertex();
	DeleteTexture();
}

void sBumpTile::InitVertex(cTileMap *TileMap, int lod_vertex_)
{
	DeleteVertex();
	lod_vertex=lod_vertex_;
	for(int i=0;i<U_ALL;i++)
		border_lod[i]=lod_vertex;

	int numVertex = TileMap->GetTilemapRender()->bumpNumVertices(lod_vertex);

	gb_RenderDevice3D->GetVertexPool()->CreatePage(vtx,VertexPoolParameter(numVertex,GetVertexDeclaration()));
	init_vertex=false;
}

void sBumpTile::InitTexture(cTileMap *TileMap,int lod_texture_)
{
	DeleteTexture();
	lod_texture=lod_texture_;
	int w = TileMap->GetTileSize().x >> lod_texture;
	int h = TileMap->GetTileSize().y >> lod_texture;

	TexPools.FindFreeTexture(tex,w,h,D3DFMT_A8R8G8B8);
	if(Option_TileMapTypeNormal)
 		TexPools.FindFreeTexture(normal,w,h,D3DFMT_V8U8);

	tex_uvStart=TexPools.getUVStart(tex);
	tex_uvStartBump.set(0,0);
	if(normal.IsInitialized())
		tex_uvStartBump=TexPools.getUVStart(normal);
	Vect2f uvStep=TexPools.getUVStep(tex);
	uv_step.x = uvStep.x/(1<<lod_texture);
	uv_step.y = uvStep.y/(1<<lod_texture);

	int xStart = tile_pos.x * TileMap->GetTileSize().x;
	int yStart = tile_pos.y * TileMap->GetTileSize().y;

	uv_base.x=tex_uvStart.x-xStart*uv_step.x;
	uv_base.y=tex_uvStart.y-yStart*uv_step.y;
	uv_base_bump.x=tex_uvStartBump.x-xStart*uv_step.x;
	uv_base_bump.y=tex_uvStartBump.y-yStart*uv_step.y;

	init_texture=false;
}

void sBumpTile::DeleteVertex()
{
	gb_RenderDevice3D->GetVertexPool()->DeletePage(vtx);
	DeleteIndex();
}

void sBumpTile::DeleteTexture()
{
	TexPools.freePage(tex);
	TexPools.freePage(normal);
}

void sBumpTile::DeleteIndex()
{
	vector<sPlayerIB>::iterator it;
	FOR_EACH(index,it)
	if(it->index.page>=0)
	{
		gb_RenderDevice3D->GetIndexPool()->DeletePage(it->index);
	}
	index.clear();
}

D3DLOCKED_RECT *sBumpTile::LockTex()
{
	return TexPools.LockPage(tex);
}

BYTE *sBumpTile::LockVB()
{
	return (BYTE*)gb_RenderDevice3D->GetVertexPool()->LockPage(vtx);
}

void sBumpTile::UnlockTex()
{
	return TexPools.UnlockPage(tex);
}

void sBumpTile::UnlockVB()
{
	gb_RenderDevice3D->GetVertexPool()->UnlockPage(vtx);
}

inline int IUCLAMP(int val,int clamp)
{ 
	if(val<0)return 0;
	if(val>=clamp)return clamp-1;
	return val;
}

sColor4c tile_color[]=
{
	sColor4c(255,0,0),
	sColor4c(0,255,0),
	sColor4c(0,0,255),
	sColor4c(128,0,0),
	sColor4c(0,128,0),
	sColor4c(0,0,128),
};

void GetTileColorX(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step,DWORD color)
{
	int by=0;
	for(int y = ystart; y < yend; y += step)
	{
		int bx=by;
		DWORD* tx=(DWORD*)Texture;
		for (int x = xstart; x < xend; x += step)
		{
			*tx = (bx&3)==0?color:0;
			bx++;
			tx++;
		}
		Texture += pitch;

		//by++;
	}
	
}

void GetNormalArrayShort(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int shift)
{
	int step=1<<shift;
	const int maxbuffer=TILEMAP_SIZE+texture_border*2+2;
	static int buffer[maxbuffer*maxbuffer];
	int clip_mask_x=TileMap->SizeX()-1;
	int clip_mask_y=TileMap->SizeY()-1;
	TileMap->GetTileZ((char*)buffer,maxbuffer*4,xstart,ystart,xend+step,yend+step,step);
	//step -> 63
	//6-shift-8
	int bump_shift=2+shift;

	for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
	{
		int * pz=buffer+ybuffer*maxbuffer;
		char* tx=texture;
		for (int x = xstart; x < xend; x += step)
		{
			int z0=*pz;
			int dzx=*pz-pz[1];
			int dzy=*pz-pz[maxbuffer];
			dzx=max(min(dzx>>bump_shift,127),-128);
			dzy=max(min(dzy>>bump_shift,127),-128);

			tx[0] =dzx;
			tx[1] =dzy;
			tx+=2;
			pz++;
		}
		texture += pitch;
	}
}


void GetNormalArray(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int shift)
{
	int step=1<<shift;
	const int maxbuffer=TILEMAP_SIZE+texture_border*2+1;
	static float buffer[maxbuffer*maxbuffer];
	int clip_mask_x=TileMap->SizeX()-1;
	int clip_mask_y=TileMap->SizeY()-1;

	for(int y = ystart,ybuffer=0; y < yend+step; y += step,ybuffer++)
	{
		float * tx=buffer+ybuffer*maxbuffer;
		int yy=min(max(0,y),clip_mask_y);;
		for (int x = xstart; x < xend+step; x += step,tx++)
		{
			int xx=min(max(0,x),clip_mask_x);
			float z=TileMap->GetZf(xx,yy);
			*tx=z;
		}
	}

	for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++)
	{
		float * pz=buffer+ybuffer*maxbuffer;
		WORD* tx=(WORD*)texture;
		for (int x = xstart; x < xend; x += step)
		{
			float z0=*pz;
			float dzx=pz[1]-*pz;
			float dzy=pz[maxbuffer]-*pz;
			Vect3f n(-dzx,-dzy,step);
			FastNormalize(n);

			*tx = ColorByNormalWORD(n);
			tx++;
			pz++;
		}
		texture += pitch;
	}
}

void GetNormal(TerraInterface *terra,int x,int y,int step,Vect3f& out)
{
	int xMap = terra->SizeX();
	int yMap = terra->SizeY();

	float z0=terra->GetZf(IUCLAMP(x, xMap),IUCLAMP(y, yMap));
	float zx=terra->GetZf(IUCLAMP(x+step, xMap),IUCLAMP(y, yMap));
	float zy=terra->GetZf(IUCLAMP(x, xMap),IUCLAMP(y+step, yMap));
	float dzx=zx-z0;
	float dzy=zy-z0;

	//Vect3f vx(step,0,dzx);
	//Vect3f vy(0,step,dzy);
	//Vect3f n=vx%vy;
	//x=vx.y*vy.z-vx.z*vy.y;=-dzx*step
	//y=vx.z*vy.x-vx.x*vy.z;=-step*dzy
	//z=vx.x*vy.y-vx.y*vy.x;=step*step

	Vect3f n(-dzx,-dzy,step);

	//n.normalize();
	FastNormalize(n);
	out=n;
}

void GetNormalArrayOld(TerraInterface *TileMap,char* texture,int pitch,int xstart,int ystart,int xend,int yend,int shift)
{
	int step=1<<shift;
    for(int y = ystart; y < yend; y += step)
	{
		WORD* tx=(WORD*)texture;
		for (int x = xstart; x < xend; x += step)
		{
			Vect3f n;
			GetNormal(TileMap,x,y,step,n);
			*tx = ColorByNormalWORD(n);
			tx++;
		}
		texture += pitch;
	}
}

void sBumpTile::CalcTexture(cTileMap *TileMap)
{
	init_texture=true;
	int tilex = TileMap->GetTileSize().x;
	int tiley = TileMap->GetTileSize().y;
	int xStart = tile_pos.x * tilex;
	int yStart = tile_pos.y * tiley;
	int xFinish = xStart + tilex;
	int yFinish = yStart + tiley;

	D3DLOCKED_RECT *texRect = LockTex();
	int dd = 1 << lod_texture;

	TerraInterface* terra=TileMap->GetTerra();
//*
	terra->GetTileColor((char*)texRect->pBits,texRect->Pitch,
		xStart -texture_border*dd, yStart -texture_border*dd,
		xFinish+texture_border*dd, yFinish+texture_border*dd,dd);
/*/
	xassert(LOD>=0 && LOD<6);
	GetTileColorX((char*)texRect->pBits,texRect->Pitch,
		xStart -texture_border*dd, yStart -texture_border*dd,
		xFinish+texture_border*dd, yFinish+texture_border*dd,dd,tile_color[LOD].RGBA());
/**/

	UnlockTex();

	if(normal.IsInitialized())
	{//Get normal 
		D3DLOCKED_RECT *texRect=TexPools.LockPage(normal);
		int xstart=xStart -texture_border*dd;
		int xend=xFinish+texture_border*dd;
		int ystart=yStart -texture_border*dd;
		int yend=yFinish+texture_border*dd;
		int step=dd;
		char* texture=(char*)texRect->pBits;
		int pitch=texRect->Pitch;
/*
		GetNormalArray(TileMap->GetTerra(),texture,pitch,xstart,ystart,xend,yend,lod_texture);
/*/
		GetNormalArrayShort(TileMap->GetTerra(),texture,pitch,xstart,ystart,xend,yend,lod_texture);
/**/
		TexPools.UnlockPage(normal);
	}
}


//////////////////////////////////////////////////////////////////////////////
void cD3DRender::PreDraw(cTileMap *TileMap)
{
	TileMap->GetTilemapRender()->PreDraw(DrawNode);
}

void cTileMapRender::UpdateLine(int dy,int dx)
{
	TileMap->GetTerra()->LockColumn();
	for (int y = 0; y < dy; y++)
	for (int x = 0; x < dx; x++)
	{
		sTile &Tile = TileMap->GetTile(x, y);
		int bumpTileID = Tile.bumpTileID;
		if(bumpTileID<0)continue;
		sBumpTile *bumpTile = bumpTiles[bumpTileID];
		bool update_line=false;

		for(int i=0;i<U_ALL;i++)
		{
			int xx=x+u_to_pos[i].x,yy=y+u_to_pos[i].y;
			if(xx>=0 && xx<dx && yy>=0 && yy<dy)
			{
				char lod=vis_lod[xx+yy*dx];
				char& cur_lod=bumpTile->border_lod[i];
				if(cur_lod!=lod && lod!=-1)
				{
					cur_lod=lod;
					update_line=true;
				}
			}
		}

		if((!bumpTile->init_vertex) || Tile.GetUpdateVertex() || update_line)
		{
			bumpTile->CalcVertex(TileMap);
			Tile.ClearUpdateVertex();
		}
	}
	TileMap->GetTerra()->UnlockColumn();
}


void cTileMapRender::SetTilesToRender(cCamera* pNormalCamera,bool shadow)
{
	cTileMapRender* tr=TileMap->GetTilemapRender();
	Vect3f coord(0,0,0);

	TexPools.ClearRenderList();

	// create/update tiles' vertices/textures
	int y,x;
	int dy=TileMap->GetTileNumber().y,dx=TileMap->GetTileNumber().x;
	float lod_focus=Option_MapLevel * pNormalCamera->GetFocusViewPort().x;
	float DistLevelDetail[TILEMAP_LOD] = { // was: 1,2,4,6
		0.5f * lod_focus,
		1.5f * lod_focus,
		3.0f * lod_focus,
		6.0f * lod_focus,
		12.0f* lod_focus
	};

	Vect3f dcoord(
		TileMap->GetTileSize().x * TileMap->GetTileScale().x,
		TileMap->GetTileSize().y * TileMap->GetTileScale().y,
		255 * TileMap->GetTileScale().z);

	for (y = 0; y < dy; y++, coord.x = 0, coord.y += dcoord.y)
	for (x = 0; x < dx; x++, coord.x += dcoord.x)
//*
	if(visMap[x+y*dx])
/*/
	if (DrawNode->TestVisible(coord, coord+dcoord))
/**/
	{
		// process visible tile
		sTile &Tile = TileMap->GetTile(x, y);
		int &bumpTileID = Tile.bumpTileID;

		// calc LOD считается всегда по отношению к прямой камере для 
		// избежания случая 2 разных LOD в одно время 
		float dist = pNormalCamera->GetPos().distance(coord + dcoord/2);
		int iLod;
		for (iLod = 0; iLod < TILEMAP_LOD; iLod++)
			if (dist < DistLevelDetail[iLod]) break;
		if (iLod >= TILEMAP_LOD) iLod = TILEMAP_LOD-1;
		int lod_vertex=bumpGeoScale[iLod];
		vis_lod[x+y*dx]=lod_vertex;

		// create/update render tile
		if (tr->bumpTileValid(bumpTileID)
			&& tr->bumpTiles[bumpTileID]->lod_vertex != lod_vertex && !shadow)
		{
			//*
			sBumpTile *bumpTile = bumpTiles[bumpTileID];
			bumpTile->InitVertex(TileMap, lod_vertex);
			if(bumpTile->lod_texture!=bumpTexScale[iLod])
				bumpTile->InitTexture(TileMap, bumpTexScale[iLod]);

			/*/
			// LOD changed, free old tile and allocate new
			bumpTileFree(bumpTileID);
			bumpTileID = bumpTileAlloc(iLod,x,y);
			/**/
		} else if (!bumpTileValid(bumpTileID))
		{
			// no tile assigned, allocate one
			bumpTileID = bumpTileAlloc(iLod,x,y);
		}

		sBumpTile *bumpTile = bumpTiles[bumpTileID];

		// set flags, add tile to list
		Tile.SetDrawVertex();
		if(!shadow)
		{
			if(!bumpTile->isTexture())
			{
				bumpTile->InitTexture(TileMap, bumpTexScale[iLod]);
			}

			Tile.SetDrawTexture();
			if((!bumpTile->init_texture) || Tile.GetUpdateTexture())
			{
				bumpTile->CalcTexture(TileMap);
				Tile.ClearUpdateTexture();
			}

			TexPools.AddToRenderList(bumpTile->tex,bumpTile);
		}else
		{
			TexPools.tileRenderList.push_back(bumpTile);
		}

	}else
		vis_lod[x+y*dx]=-1;
}

void cTileMapRender::DrawBump(cCamera* DrawNode,eBlendMode MatMode,bool shadow,bool zbuffer)
{
	//cCamera* pShadowMapCamera=DrawNode->FindChildCamera(ATTRCAMERA_SHADOWMAP);
	cCamera* pNormalCamera=DrawNode->GetRoot();
	gb_RenderDevice3D->SetNoMaterial(MatMode,MatXf::ID);
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_anisotropic);

	gb_RenderDevice3D->SetVertexDeclaration(sBumpTile::GetVertexDeclaration());

	calcVisMap(DrawNode, TileMap, visMap, true);

	gb_RenderDevice3D->dtAdvance->SetReflection(DrawNode->GetAttr(ATTRCAMERA_REFLECTION));
	gb_RenderDevice3D->dtAdvance->BeginDrawTilemap(TileMap);
	//bool zbuffer = DrawNode->GetAttr(ATTRCAMERA_ZBUFFER);
	if(shadow)
	{
		gb_RenderDevice3D->dtAdvance->SetMaterialTilemapShadow();
	}else
	if(zbuffer)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_FOGENABLE,FALSE);
		gb_RenderDevice3D->dtAdvance->SetMaterialTilemapZBuffer();
	}else
	{
//		gb_RenderDevice3D->dtAdvance->SetMaterialTilemap();
	}


	SetTilesToRender(pNormalCamera,shadow);
	UpdateLine(TileMap->GetTileNumber().y,TileMap->GetTileNumber().x);

	int st_VBSw = 0, st_TexSw = 0, st_Poly = 0;

	int lastPool = -1, nTiles = 0;
	int i;
	for (i = -1; i < (int)TexPools.bumpTexPools.size(); i++)
	{
		sTilemapTexturePool* curpool=NULL;
		if(i>=0)
			curpool=TexPools.bumpTexPools[i];
		vector<sBumpTile*>& tileRenderList=i==-1?TexPools.tileRenderList:curpool->tileRenderList;

		vector<sBumpTile*>::iterator it_tile;
		FOR_EACH(tileRenderList,it_tile)
		{
			sBumpTile *bumpTile = *it_tile;
			if(shadow||zbuffer)
			{
			}else
			{
				//gb_RenderDevice3D->dtAdvance->NextTile(curpool->GetTexture(),bumpTile->uv_base,bumpTile->uv_step);
				IDirect3DTextureProxy* pBump=NULL;
				if(bumpTile->normal.IsInitialized())
					pBump=TexPools.bumpTexPools[bumpTile->normal.pool]->GetTexture();

				gb_RenderDevice3D->dtAdvance->NextTile(curpool->GetTexture(),bumpTile->uv_base,bumpTile->uv_step,
														pBump,		   bumpTile->uv_base_bump,bumpTile->uv_step);
			}

			int lod_vertex = bumpTile->lod_vertex;
			int nVertex = bumpNumVertices(lod_vertex);

			if (lastPool != bumpTile->vtx.pool)
			{
				gb_RenderDevice3D->GetVertexPool()->Select(bumpTile->vtx);
				lastPool = bumpTile->vtx.pool;
				st_VBSw++;
			}

			int nPolygon=0;
			DWORD pageSize=gb_RenderDevice3D->GetVertexPool()->GetPageSize(bumpTile->vtx);
			DWORD base_vertex_index=pageSize * bumpTile->vtx.page;

			for(int i=0;i<bumpTile->index.size();i++)
			{
				sPlayerIB& pib=bumpTile->index[i];

				if(!shadow&&!zbuffer)//не оптимально
				{
					sColor4f color=TileMap->GetZeroplastColor(pib.player);
					if(MatMode!=ALPHA_BLEND)
						color.a=1;
					gb_RenderDevice3D->dtAdvance->SetTileColor(color);
					xassert(gb_RenderDevice3D->dtAdvance->GetTileMapDebug());
					gb_RenderDevice3D->dtAdvance->SetMaterialTilemap(pib.player);
				}

				if(pib.nindex==-1)
				{
					int nIndex = bumpNumIndex(lod_vertex);
					int nIndexOffset=bumpIndexOffset(lod_vertex);

					gb_RenderDevice3D->SetIndices(tilemapIB);

					nPolygon=nIndex/3;
					RDCALL(gb_RenderDevice3D->lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
						base_vertex_index,
						0, nVertex, nIndexOffset, nPolygon));
				}else
				{
					int nIndex = pib.nindex;
					int nIndexOffset=gb_RenderDevice3D->GetIndexPool()->GetBaseIndex(pib.index);

					gb_RenderDevice3D->GetIndexPool()->Select(pib.index);

					nPolygon=nIndex/3;
					RDCALL(gb_RenderDevice3D->lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
						base_vertex_index,
						0, nVertex, nIndexOffset, nPolygon));
				}
			}
	
			gb_RenderDevice3D->NumberTilemapPolygon += nPolygon;
			gb_RenderDevice3D->NumDrawObject++;
			nTiles++;
			st_Poly += nPolygon;
			
			if(false)
			if(pNormalCamera==DrawNode)
			{
				int x=bumpTile->tile_pos.x;
				int y=bumpTile->tile_pos.y;
				int xs = TileMap->GetTileSize().x;
				int ys = TileMap->GetTileSize().y;

				int z=TileMap->GetTile(x,y).zmax;

				Vect3f v00(xs*x    ,ys*y    ,z),
					   v01(xs*(x+1),ys*y    ,z),
					   v10(xs*x    ,ys*(y+1),z),
					   v11(xs*(x+1),ys*(y+1),z);

				sColor4c color(0,0,255,255);
				gb_RenderDevice->DrawLine(v00,v01,color);
				gb_RenderDevice->DrawLine(v01,v11,color);
				gb_RenderDevice->DrawLine(v11,v10,color);
				gb_RenderDevice->DrawLine(v10,v00,color);
			}
		}
	}

	// restore state
	gb_RenderDevice3D->dtAdvance->EndDrawTilemap();

	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);

	gb_RenderDevice3D->SetVertexShader(NULL);
	gb_RenderDevice3D->SetPixelShader(NULL);

	for(i=0;i<6;i++)
		gb_RenderDevice3D->SetTextureBase(i,NULL);
	//gb_RenderDevice3D->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
/*
char msg[256];
int st_mem = 0;
for (i = 0; i < bumpTexPools.size(); i++)
	st_mem += bumpTexPools[i]->tileWidth * BUMP_TEXPOOL_H * 4;
for (i = 0; i < bumpVtxPools.size(); i++)
	st_mem += bumpVtxPools[i]->totalPages * bumpVtxPools[i]->pageSize * bumpVtxPools[i]->vertexSize;
sprintf(msg, "tiles=%d vbsw=%d texsw=%d poly=%d texpools=%d mem=%d",
	nTiles, st_VBSw, st_TexSw, st_Poly, bumpTexPools.size(), st_mem);
OutText(0, 80, msg);
/**/
	if(false)//shadow)//pNormalCamera==DrawNode)
	if(pNormalCamera==DrawNode)
	{
		int dx=TileMap->GetTileNumber().x,dy=TileMap->GetTileNumber().y;
		int minx=9,miny=888 - 256;

		int r,g,b,a=80;
		r=0;
		g=b=255;

		int mul=dx>32?2:4;

		for(int y=0;y<dy;y++)
		for(int x=0;x<dx;x++)
		{
			if(visMap[y*dx+x])
			{
				int xx=x*mul+minx,yy=y*mul+miny;
				gb_RenderDevice3D->DrawRectangle(xx,yy,mul,mul,sColor4c(r,g,b,a));
			}
		}
	}

	if(false)//shadow)
	{
		CMesh cmesh;
		calcCMesh(pNormalCamera,TileMap->GetTileNumber(),TileMap->GetTileSize(),cmesh);
		drawCMesh(cmesh);
		gb_RenderDevice3D->FlushPrimitive3D();
	}
}

/*
 Ищет граничные точки у полигона в квадрате xmin,ymin,xmin+dx,ymin+dy.

 В Interval xr - включается в интервал. 
 Линия считается "толстой", то есть от точки y до y+1
*/
/*
void EdgeDetection(Column& column,int xmin,int ymin,int dx,int dy,vector<Vect2i>& edge_point)
{
#define xrr(i) (i->xr+1)
	int ymax=ymin+dy;
	int xmax=xmin+dx;

	if(ymin<0)
		ymin=0;
	if(ymax>column.size())
		ymax=column.size();

	for(int y=ymin;y<ymax;y++)
	{
		CellLine& line=column[y];
		CellLine& linem=(y-1>=0)?column[y-1]:line;
		CellLine& linep=(y+1<column.size())?column[y+1]:line;
	
		//Двигаемся по линии вперед, и смотрим
		//Если сверху и снизу все закрыто - значит не граничная.
		CellLine::iterator it,itm,itp;
		it=line.begin();
		itm=linem.begin();
		itp=linep.begin();
		while(it!=line.end() && xrr(it)<xmin)
			it++;
		while(itp!=linep.end() && xrr(itp)<xmin)
			itp++;
		while(itm!=linem.end() && xrr(itm)<xmin)
			itm++;

		for(int x=xmin;x<xmax;x++)
		{
			while(it!=line.end() && xrr(it)<x)
				it++;
			while(itp!=linep.end() && xrr(itp)<x)
				itp++;
			while(itm!=linem.end() && xrr(itm)<x)
				itm++;

			if(it==line.end())
				break;

			Interval& i=*it;
			if(i.xl<=x)//Есть точка
			{
				bool add_up=false,add_down=false;
				if(itm==linem.end())
					add_up=true;
				else
				if(i.xl==x || xrr(it)==x)
				{
					add_up=true;
				}else
				{
					Interval &im=*itm;
					if(im.xl>x)
						add_up=true;
				}

				if(itp==linep.end())
					add_down=true;
				else
				{
					Interval &ip=*itp;
					if(ip.xl>x)
						add_down=true;
				}

				if(add_up || add_down)
					edge_point.push_back(Vect2i(x,y));

//				if(add_down)
//					edge_point.push_back(Vect2i(x,y+1));
			}
		}
	}
#undef xrr
}
*/
void EdgeDetection(MultiRegion& region,int xmin,int ymin,int dx,int dy,vector<Vect2i>& edge_point)
{
#define xrr(i) ((i+1)->x0)
	int ymax=ymin+dy;
	int xmax=xmin+dx;

	MultiRegion::Lines& lines=region.lines();
	if(ymin<0)
		ymin=0;
	if(ymax>region.height())
		ymax=region.height();

	for(int y=ymin;y<ymax;y++)
	{
		MultiRegion::Line& line=lines[y];
		MultiRegion::Line& linem=(y-1>=0)?lines[y-1]:line;
		MultiRegion::Line& linep=(y+1<lines.size())?lines[y+1]:line;
	
		//Двигаемся по линии вперед, и смотрим
		//Если сверху и снизу все закрыто - значит не граничная.
		MultiRegion::Line::iterator it,itm,itp;
		it=line.begin();
		itm=linem.begin();
		itp=linep.begin();
		MultiRegion::Line::iterator it_end,itm_end,itp_end;
		it_end=line.end();
		itm_end=linem.end();
		itp_end=linep.end();

		unsigned char type=0,typem=0,typep=0;

		for(int x=xmin;x<xmax;x++)
		{
			while(it!=it_end && it->x0<x)
			{
				type=it->type;
				it++;
			}

			while(itp!=itp_end && itp->x0<x)
			{
				typep=itp->type;
				itp++;
			}

			while(itm!=itm_end && itm->x0<x)
			{
				typem=itm->type;
				itm++;
			}

			if(it==it_end)
				break;

			MultiRegion::Interval& i=*it;
			if(type)//Есть точка
			{
				bool add_up=false,add_down=false;
				if(i.x0==x)
				{
					add_up=true;
				}else
				{
					if(typem!=type)
						add_up=true;
				}

				if(typep!=type)
					add_down=true;

				if(add_up)
					edge_point.push_back(Vect2i(x,y));

//				if(add_down)
//					edge_point.push_back(Vect2i(x,y+1));
			}
		}
	}
#undef xrr
}

vector<Vect3f> sBumpTile_test_list;
void sBumpTile::TestPoint(Vect2i pos)
{
	sBumpTile_test_list.clear();
	int step=lod_vertex;

	Vect2i pos_offs(pos.x-tile_pos.x*TILEMAP_SIZE,pos.y-tile_pos.y*TILEMAP_SIZE);
	Vect2i pos_lod(pos_offs.x>>step,pos_offs.y>>step);

	int dx=1<<step;

	int xmin=pos_offs.x-2*dx,xmax=pos_offs.x+2*dx;
	int ymin=pos_offs.y-2*dx,ymax=pos_offs.y+2*dx;

	for(int y=ymin;y<=ymax;y++)
	for(int x=xmin;x<=xmax;x++)
	{

		Vect2i pround;
		if(CollapsePoint(pround,x,y))
		{
			if(pround.x==pos_lod.x && pround.y==pos_lod.y)
			{
				Vect3f v1(x+tile_pos.x*TILEMAP_SIZE,y+tile_pos.y*TILEMAP_SIZE,87);
				sBumpTile_test_list.push_back(v1);
			}
		}
	}
}

void sBumpTile::CalcVertex(cTileMap *TileMap)
{
	init_vertex=true;
	MultiRegion* columns=TileMap->GetTerra()->GetRegion();
	Vect2i pos=tile_pos;

	TileMap->GetTilemapRender()->IncUpdate(this);

	int tilenumber=TileMap->GetZeroplastNumber();
	int step=lod_vertex;

	char dd=TILEMAP_SIZE>>step;
	int xstep=1<<step;
	int xstep2=1<<(step-1);
	int xstep4=1<<(step-2);

	int minx=pos.x*TILEMAP_SIZE;
	int miny=pos.y*TILEMAP_SIZE;
	int maxx=minx+TILEMAP_SIZE;
	int maxy=miny+TILEMAP_SIZE;

	int ddv=dd+1;
	VectDelta* points=TileMap->GetTilemapRender()->GetDeltaBuffer();

	vector<vector<sPolygon> >& index=TileMap->GetTilemapRender()->GetIndexBuffer();

	{
		index.resize(tilenumber+1);
		for(int i=0;i<index.size();i++)
			index[i].clear();
	}

	int x,y;

	for(y=0;y<ddv;y++)
	for(x=0;x<ddv;x++)
	{
		VectDelta& p=points[x+y*ddv];
		p.set(x*xstep+minx,y*xstep+miny);
		p.delta.set(0,0);
		p.delta2=INT_MAX;
		p.player=0;
		p.fix=0;

		int xx=min(p.x,maxx-1);
		int yy=min(p.y,maxy-1);
		if(columns)
		{
			int player=columns->filled(xx,yy)-1;
			xassert(p.player==0);
			xassert(player>=0 && player<index.size());
			p.player=player;
		}
	}

	int fix_out=FixLine(points,ddv,TileMap);

//	1280,1536
// x=19,20
//	if(tile_pos.x==20 && tile_pos.y==23)
//		TestPoint(Vect2i(1280,1536));

//if(columns[player]->filled(xx,yy)) //Эта операция должна ускориться с переходом на мультирегион.
//EdgeDetection - Ускорится, потому как будет в 2 раза меньше точек.
//Добавление в регион с одной стороны ускорится, потому как будет меньше регионов, с другой стороны замедлится, 
// потому как регионы будут более сложной формы, но в среднем должна ускориться.


	int preceeded_point=0;

	if(columns)
	{
		int ymin=tile_pos.y*TILEMAP_SIZE;
		int xmin=tile_pos.x*TILEMAP_SIZE;
		vector<Vect2i> edge_point;
		EdgeDetection(*columns,xmin-xstep,ymin-xstep,TILEMAP_SIZE+xstep*2,TILEMAP_SIZE+xstep*2,edge_point);

		for(vector<Vect2i>::iterator it=edge_point.begin();it!=edge_point.end();++it)
		{
			Vect2i p=*it;
			preceeded_point++;

			Vect2i pround;
			VectDelta* pnt=NULL;

			int dx=p.x-minx,dy=p.y-miny;
			if(!CollapsePoint(pround,dx,dy))
				continue;

			pnt=&points[pround.x+pround.y*ddv];
			if(pnt->fix&VectDelta::FIX_FIXED)
			{
				xassert(0);
				pround.x&=~1;
				pround.y&=~1;
				pnt=&points[pround.x+pround.y*ddv];
				VISASSERT(pnt->fix&(VectDelta::FIX_RIGHT|VectDelta::FIX_BOTTOM));
			}

			int delta2=sqr(pnt->x-p.x)+sqr(pnt->y-p.y);
			if(delta2<pnt->delta2)
			{
				pnt->delta2=delta2;
				pnt->delta=p-*pnt;
				pnt->player=-1;
				if(pnt->fix)
				{
					if(pnt->fix&VectDelta::FIX_RIGHT)
					{
						VectDelta& p=points[pround.x+1+pround.y*ddv];
						VISASSERT(pnt->x==p.x && pnt->y==p.y);
						VISASSERT(p.fix==VectDelta::FIX_FIXED);
						p.copy_no_fix(*pnt);
					}
					if(pnt->fix&VectDelta::FIX_BOTTOM)
					{
						VectDelta& p=points[pround.x+(pround.y+1)*ddv];
						VISASSERT(pnt->x==p.x && pnt->y==p.y);
						VISASSERT(p.fix==VectDelta::FIX_FIXED);
						p.copy_no_fix(*pnt);
					}
				}
			}
		}
	}

	sPolygon poly;
	for(y=0;y<dd;y++)
	for(x=0;x<dd;x++)
	{
		int base=x+y*ddv;
		VectDelta *p00,*p01,*p10,*p11;
		p00=points+base;
		p01=points+(base+1);
		p10=points+(base+ddv);
		p11=points+(base+ddv+1);

		bool no;
		char cur_player_add;

#define X(p) \
		{ \
			if(cur_player_add<0) \
				cur_player_add=p->player; \
			else \
				if(p->player>=0 && cur_player_add!=p->player)\
					no=true;\
		}
		
		if(p01->player==p10->player || p01->player<0 || p10->player<0)
		{
			cur_player_add=-1;
			no=false;
			X(p00);
			X(p01);
			X(p10);

			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p01->x+p01->delta.x+p10->x+p10->delta.x)/3;
				int y=(p00->y+p00->delta.y+p01->y+p01->delta.y+p10->y+p10->delta.y)/3;
				cur_player_add=0;
				if(columns)
				{
					cur_player_add=columns->filled(x,y)-1;
					xassert(cur_player_add>=0 && cur_player_add<index.size());
				}
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv,base+1);
			index[cur_player_add].push_back(poly);

			cur_player_add=-1;
			no=false;
			X(p01);
			X(p10);
			X(p11);

			if(cur_player_add<0 || no)
			{
				int x=(p01->x+p01->delta.x+p10->x+p10->delta.x+p11->x+p11->delta.x)/3;
				int y=(p01->y+p01->delta.y+p10->y+p10->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				if(columns)
				{
					cur_player_add=columns->filled(x,y)-1;
					xassert(cur_player_add>=0 && cur_player_add<index.size());
				}
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base+ddv,base+ddv+1,base+1);
			index[cur_player_add].push_back(poly);
		}else
		{
			cur_player_add=-1;
			no=false;
			X(p00);
			X(p01);
			X(p11);


			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p01->x+p01->delta.x+p11->x+p11->delta.x)/3;
				int y=(p00->y+p00->delta.y+p01->y+p01->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				if(columns)
				{
					cur_player_add=columns->filled(x,y)-1;
					xassert(cur_player_add>=0 && cur_player_add<index.size());
				}
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv+1,base+1);
			index[cur_player_add].push_back(poly);

			cur_player_add=-1;
			no=false;
			X(p00);
			X(p10);
			X(p11);

			if(cur_player_add<0 || no)
			{
				int x=(p00->x+p00->delta.x+p10->x+p10->delta.x+p11->x+p11->delta.x)/3;
				int y=(p00->y+p00->delta.y+p10->y+p10->delta.y+p11->y+p11->delta.y)/3;
				cur_player_add=0;
				if(columns)
				{
					cur_player_add=columns->filled(x,y)-1;
					xassert(cur_player_add>=0 && cur_player_add<index.size());
				}
			}

			xassert(cur_player_add>=0 && cur_player_add<=tilenumber);
			poly.set(base,base+ddv,base+ddv+1);
			index[cur_player_add].push_back(poly);
		}
	}
#undef X

	/////////////////////set vertex buffer
	int is = TileMap->GetTileSize().x; // tile size in vMap units
	int js = TileMap->GetTileSize().y;
	int xMap = is * TileMap->GetTileNumber().x; // vMap size
	int yMap = js * TileMap->GetTileNumber().y;
	int xStart = tile_pos.x * is;
	int yStart = tile_pos.y * js;
	int xFinish = xStart + is;
	int yFinish = yStart + js;

	uv_base.x=tex_uvStart.x-xStart*uv_step.x;
	uv_base.y=tex_uvStart.y-yStart*uv_step.y;
	uv_base_bump.x=tex_uvStartBump.x-xStart*uv_step.x;
	uv_base_bump.y=tex_uvStartBump.y-yStart*uv_step.y;


	if(Option_TileMapTypeNormal)
	{
		shortVertexXYZ *vb,*vbbegin;
		vbbegin=vb= (shortVertexXYZ*)LockVB();

		TerraInterface* terra=TileMap->GetTerra();

		for(y=0;y<ddv;y++)
		for(x=0;x<ddv;x++)
		{
			int i=x+y*ddv;
			VectDelta& p=points[i];
			int xx=p.x+p.delta.x;
			int yy=p.y+p.delta.y;

			vb->pos.x = xx;
			vb->pos.y = yy;
			vb->pos.z=	round(SetVertexZ(terra,IUCLAMP(xx, xMap),IUCLAMP(yy, yMap))*64);
			vb->pos.w=1;
			vb++;
		}

		UnlockVB();
	}else
	{
		shortVertexXYZD *vb,*vbbegin;
		vbbegin=vb= (shortVertexXYZD*)LockVB();

		TerraInterface* terra=TileMap->GetTerra();

		for(y=0;y<ddv;y++)
		for(x=0;x<ddv;x++)
		{
			int i=x+y*ddv;
			VectDelta& p=points[i];
			int xx=p.x+p.delta.x;
			int yy=p.y+p.delta.y;

			vb->pos.x = xx;
			vb->pos.y = yy;
			vb->pos.z=	round(SetVertexZ(terra,IUCLAMP(xx, xMap),IUCLAMP(yy, yMap))*64);
			vb->pos.w=1;

			Vect3f normal;
			GetNormalCenter(TileMap,xx,yy,xstep,normal);
			vb->diffuse.RGBA()=ColorByNormalRGBA(normal);

			vb++;
		}

		UnlockVB();
	}

	////////////////////set index buffer
	DeleteIndex();

	int num_non_empty=0;
	int one_player=0;
	for(int i=0;i<index.size();i++)
	{
		if(index[i].size()>0)
		{
			num_non_empty++;
			one_player=i;
		}
	}

	if(num_non_empty<=1)
	{
		sBumpTile::index.resize(1);
		sBumpTile::index[0].player=one_player;
		sBumpTile::index[0].nindex=-1;
	}else
	{
		int sum_index=0;
		sBumpTile::index.resize(num_non_empty);
		int cur=0;
		for(int i=0;i<index.size();i++)
		if(index[i].size()>0)
		{
			int num=index[i].size()*3;
			sum_index+=num;
			int num2=Power2up(num);
			gb_RenderDevice3D->GetIndexPool()->CreatePage(sBumpTile::index[cur].index,num2);

			sPolygon* p=gb_RenderDevice3D->GetIndexPool()->LockPage(sBumpTile::index[cur].index);
			memcpy(p,&(index[i][0]),num*sizeof(WORD));
			gb_RenderDevice3D->GetIndexPool()->UnlockPage(sBumpTile::index[cur].index);

			sBumpTile::index[cur].nindex=num;
			sBumpTile::index[cur].player=i;
			cur++;
		}

		int lod_index=TileMap->GetTilemapRender()->bumpNumIndex(lod_vertex);
		VISASSERT(lod_index==sum_index);
	}
}

int sBumpTile::FixLine(VectDelta* points,int ddv,cTileMap *TileMap)
{
	int fix_out=0;
	int dmax=ddv*ddv;
	int dv=ddv-1;
	for(int side=0;side<4;side++)
	{
		if(border_lod[side]<=lod_vertex)
			continue;
//		VISASSERT(border_lod[side]==LOD+1);

		fix_out|=1<<side;
		
		VectDelta* cur;
		int delta;
		char fix=0;
		switch(side)
		{
		case U_LEFT:
			cur=points;
			delta=ddv;
			fix=VectDelta::FIX_BOTTOM;
			break;
		case U_RIGHT:
			cur=points+ddv-1;
			delta=ddv;
			fix=VectDelta::FIX_BOTTOM;
			points[ddv*ddv-1].fix|=VectDelta::FIX_FIXEMPTY;
			break;
		case U_TOP:
			cur=points;
			delta=1;
			fix=VectDelta::FIX_RIGHT;
			break;
		case U_BOTTOM:
			cur=points+ddv*(ddv-1);
			delta=1;
			fix=VectDelta::FIX_RIGHT;
			points[ddv*ddv-1].fix|=VectDelta::FIX_FIXEMPTY;
			break;
		}

		for(int i=0;i<dv;i+=2,cur+=2*delta)
		{
//			VISASSERT(cur-points<dmax);
			VectDelta* p=cur+delta;
//			VISASSERT(p-points<dmax);
			p->copy_no_fix(*cur);
			//*p=*cur;
			VISASSERT(p->fix==0);
			cur->fix|=fix;
			p->fix|=VectDelta::FIX_FIXED;
		}
	}

	return fix_out;
}

void cTileMapRender::IncUpdate(sBumpTile* pbump)
{
	if(update_stat)
	{
		int dx=TileMap->GetTileNumber().x;
		int dxy=dx*TileMap->GetTileNumber().y;
		update_in_frame=true;
		update_stat[(pbump->lod_vertex-bumpGeoScale[0])*dxy+pbump->tile_pos.y*dx+pbump->tile_pos.x]++;
	}
}


bool SaveTga(const char* filename,int width,int height,unsigned char* buf,int byte_per_pixel);
void cTileMapRender::SaveUpdateStat()
{
	static int tga_num=-1;
	tga_num++;
	if(!update_in_frame)return;
	int dx=TileMap->GetTileNumber().x,dy=TileMap->GetTileNumber().y;
	int dxy=dx*dy;

	BYTE* buf=new BYTE[dxy*4];
	memset(buf,0,dxy*4);
	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		BYTE* color=&buf[(y*dx+x)*4];
		int num_update=0;
		int lod=-1;
		bool change_lod=false;
		for(int i=0;i<TILEMAP_LOD;i++)
		{
			char u=update_stat[i*dxy+y*dx+x];
			num_update+=u;
			if(u)
			{
				if(lod==-1)
					lod=i;
				else
					change_lod=true;
			}
		}

		if(change_lod)
			color[0]=128;
		color[1]=min(num_update,4)*63;
	}

	char fname[100];
	sprintf(fname,"tga\\%05i.tga",tga_num);
	SaveTga(fname,dx,dy,buf,4);
	delete buf;

}

void sBumpTile::GetNormalCenter(cTileMap *TileMap,int x,int y,int step,Vect3f& out)
{
	int xMap = TileMap->GetTileSize().x * TileMap->GetTileNumber().x;
	int yMap = TileMap->GetTileSize().y * TileMap->GetTileNumber().y;
	TerraInterface* terra=TileMap->GetTerra();
	step=step>>1;

	float zx=SetVertexZ(terra,IUCLAMP(x+step, xMap),IUCLAMP(y, yMap));
	float zxm=SetVertexZ(terra,IUCLAMP(x-step, xMap),IUCLAMP(y, yMap));
	float zy=SetVertexZ(terra,IUCLAMP(x, xMap),IUCLAMP(y+step, yMap));
	float zym=SetVertexZ(terra,IUCLAMP(x, xMap),IUCLAMP(y-step, yMap));
	float dzx=zx-zxm;
	float dzy=zy-zym;
	Vect3f n(-dzx,-dzy,step*2);
	FastNormalize(n);
	out=n;
}

void ReloadTilemapPool(cTileMapRender* pTileMapRender)
{
	pTileMapRender->ClearTilemapPool();
	pTileMapRender->RestoreTilemapPool();
}
/*
1) Убрать Fixed pipeline.
2) Написать шейдер для Geforce FX c с освещением. С тенями и без.
   4 текстуры - diffuse, light map, bump map, shadow map.
   миниум - diffuse, bump map.
3) Распространить результаты на Radeon 9800, Geforce 3, Radeon 8500.

Варианты вывода.
Два варианта освещением на pixel shader и vertex shader.

Минимальный - без теней, с освещением на vertex shader. 
              Будет работать более менее быстро на всём заявленном оборудовании.

Средний без теней - освещение + light map.

Максимальный - освещение на pixel shader + тени + light map.
               Разные варианты для Geforce FX и Radeon 9800.
			   4 и 16 сэмплов.

Варианты под Geforce 3 и Radeon 8500 делать потом.


Настройки (влияющие на выбор шейдера)-
   Shadow map - on,off
   Normal detail - vertex shader, pixel shader
   Shadow map quality - 2x2, 4x4
*/
