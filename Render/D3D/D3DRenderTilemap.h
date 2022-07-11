#pragma once

#include "Render\src\TileMap.h"
#include "Render\D3D\PoolManager.h"

struct sBumpTile;

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

class sTilemapTexturePool
{
	IDirect3DTexture9* texture;
	int tileWidth, tileHeight;
	int tileRealWidth, tileRealHeight;
	int freePages;
	int *freePagesList;

	int texture_width,texture_height;
	vector<Vect2i> Pages;//x-xmin, y-ymin
	float ustep,vstep;
	D3DFORMAT format;
public:
	vector<sBumpTile*> tileRenderList;//—юда записываютс€ тайлы, которые рендер€т нормальное/отраженное изображение.

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
	IDirect3DTexture9* GetTexture(){return texture;}

	void GetTextureMemory(int &total,int& free);
protected:
	void CalcPagesPos();
};

class cTexPools
{
public:
	vector<sTilemapTexturePool*> bumpTexPools;

	vector<sBumpTile*> shadowTiles;//—юда записываютс€ тайлы, которые рендер€т тень.

	cTexPools(){};
	~cTexPools(){clear();}

	void GetTilemapTextureMemory(int& total,int& free);
	void clear();
	void FindFreeTexture(TexPoolsHandle& h,int tex_width,int tex_height,D3DFORMAT format);
	Vect2f getUVStart(TexPoolsHandle& h);
	Vect2f getUVStep(TexPoolsHandle& h);

	D3DFORMAT getTexFormat(TexPoolsHandle& h);

	void freePage(TexPoolsHandle& h);
	D3DLOCKED_RECT *LockPage(TexPoolsHandle& h);
	void UnlockPage(TexPoolsHandle& h);

	void AddToRenderList(TexPoolsHandle& h,sBumpTile* tile);
	void ClearRenderList();
};


struct sPlayerIB
{
	IndexPoolPage index;
	int nindex;
	int material;
};

struct VectDelta : public Vect2i
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

	unsigned char zavr;
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

	void CalcTexture(cTileMap *TileMap);
	void CalcVertex(cTileMap *TileMap);

	inline Vect2f GetUVStart(){return tex_uvStart;};
	inline Vect2f GetUVStep(){return uv_step;};

	Vect2f uv_base,uv_step;
	Vect2f uv_base_bump;

	static IDirect3DVertexDeclaration9* GetVertexDeclaration() {return shortVertexXYZD::declaration;}

protected:
	int FixLine(VectDelta* points,int ddv,cTileMap *TileMap);

	inline float SetVertexZ(int x,int y);
	void DeleteIndex();

	bool CollapsePoint(Vect2i& pround,int dx,int dy);
};

class cTileMapRender : public ManagedResource
{
public:
	cTileMapRender(cTileMap *pTileMap);
	~cTileMapRender();

	void deleteManagedResource();
	void restoreManagedResource();
	void dumpManagedResource(XBuffer& buffer);

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

	void PreDraw(Camera* camera);
	void DrawBump(Camera* camera,eBlendMode MatMode,bool shadow,bool zbuffer);

	sBumpTile* GetTile(int k,int n)
	{
		xassert(k>=0 && k<tileMap_->tileNumber().x);
		xassert(n>=0 && n<tileMap_->tileNumber().y);
		int bumpTileID = tileMap_->GetTile(k, n).bumpTileID;
		if(bumpTileID>=0)
			return bumpTiles[bumpTileID];
		return 0;
	}
	
	VectDelta* GetDeltaBuffer() { return delta_buffer; }
	vector<vector<sPolygon> >& GetIndexBuffer() { return index_buffer; }

	void UpdateLine(int dy,int dx);
	void SetTilesToRender(Camera* pNormalCamera);

	VertexPoolManager* vertexPool() { return &vertexPool_; }
	IndexPoolManager* indexPool() { return &indexPool_; }
	cTexPools* texturePool();

private:
	cTileMap* tileMap_;
	vector<sBumpTile*> bumpTiles;
	LPDIRECT3DINDEXBUFFER9 tilemapIB;
	VertexPoolManager vertexPool_;
	IndexPoolManager indexPool_;
	int index_offset[TILEMAP_LOD];
	int index_size[TILEMAP_LOD];

	//BYTE* visMap;
	char* vis_lod;

	VectDelta* delta_buffer;
	vector<vector<sPolygon> > index_buffer;

	static int bumpGeoScale[TILEMAP_LOD];
	static int bumpTexScale[TILEMAP_LOD];
};

extern cTileMapRender* tileMapRender;
