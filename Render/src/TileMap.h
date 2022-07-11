#ifndef __TILE_MAP_H_INCLUDED__
#define __TILE_MAP_H_INCLUDED__

class cScene;

typedef vector<Vect2s> Vect2sVect;

const int TILEMAP_SHL  = 6;
const int TILEMAP_SIZE = 1<<TILEMAP_SHL;
const int TILEMAP_LOD=5;

enum eUPDATEMAP
{
	UPDATEMAP_TEXTURE=1<<0,
	UPDATEMAP_HEIGHT=1<<1,
	UPDATEMAP_REGION=1<<2,
	UPDATEMAP_ALL=UPDATEMAP_TEXTURE|UPDATEMAP_HEIGHT|UPDATEMAP_REGION,
};

//TILEMAP_LOD в соответствии с ним нужно исправить ATTRTILE_DRAWLODxxx
enum eAttributeTile
{
	ATTRTILE_DRAW_VERTEX	=   1<<0,
	ATTRTILE_DRAW_TEXTURE	=   1<<1,
	ATTRTILE_UPDATE_VERTEX	=	1<<2,	
	ATTRTILE_UPDATE_TEXTURE	=	1<<3,	
};

struct sTile : public sAttribute
{
	int bumpTileID;
	BYTE zmin,zmax;
	DWORD update_zminmmax_time;

	sTile()								
	{
		bumpTileID = -1;
		zmin=255;zmax=0;
		update_zminmmax_time=0;
	}

	inline int GetDrawVertex()		{ return GetAttribute(ATTRTILE_DRAW_VERTEX); }
	inline int GetDrawTexture()		{ return GetAttribute(ATTRTILE_DRAW_TEXTURE); }
	inline void SetDrawVertex()		{ SetAttribute(ATTRTILE_DRAW_VERTEX); }
	inline void SetDrawTexture()	{ SetAttribute(ATTRTILE_DRAW_TEXTURE); }

	inline void ClearDraw()			{ ClearAttribute(ATTRTILE_DRAW_VERTEX|ATTRTILE_DRAW_TEXTURE); }

	inline int GetUpdateVertex()			{ return GetAttribute(ATTRTILE_UPDATE_VERTEX); }
	inline int GetUpdateTexture()			{ return GetAttribute(ATTRTILE_UPDATE_TEXTURE); }
	inline void ClearUpdateVertex()	{ ClearAttribute(ATTRTILE_UPDATE_VERTEX); }
	inline void ClearUpdateTexture(){ ClearAttribute(ATTRTILE_UPDATE_TEXTURE); }
};

typedef vector<vector<Vect2s>* > CurrentRegion;
typedef void (*UpdateMapFunction)(const Vect2i& pos1, const Vect2i& pos2,void* data);

class cTileMapRender;
class Column;
class cTileMap : public cBaseGraphObject
{
	friend class cScene;

	sTile*			Tile;
	Vect2i			TileSize;		// размер одного тайла
	Vect2i			TileNumber;		// число тайлов по осям

	Vect3d			tilesize;

	cTileMapRender* pTileMapRender;
	

	int				zeroplastnumber;
	int			update_zminmmax_time;
	vector<sColor4f> zeroplast_color;

	class TerraInterface* terra;

	struct UpdateRect
	{
		Vect2i p1,p2;
		int type;
	};
	vector<UpdateRect> update_rect;
	MTSection lock_update_rect;

	struct DebugRect
	{
		Vect2i p1,p2;
		float time;
	};
	bool enable_debug_rect;
	float debug_fade_interval;
	list<DebugRect> debug_rect;
	int TileMapTypeNormal;
	int miniDetailTexRes_;

	Vect3f tile_scale;
	sColor4f diffuse;
public:
	cTileMap(cScene* pScene,TerraInterface* terra);
	virtual ~cTileMap();
	
	virtual void PreDraw(cCamera *UCamera);
	virtual void Draw(cCamera *UCamera);
	virtual void UpdateMap(const Vect2i& pos1, const Vect2i& pos2,int type=UPDATEMAP_ALL);//type - сумма eUPDATEMAP
	void UpdateMap(const Vect2i& pos, float radius,int type=UPDATEMAP_ALL);
	void Animate(float dt);
	// общие интерфейсные функции cTileMap
	const Vect2i& GetTileSize()const					{ return TileSize; }
	const Vect2i& GetTileNumber()const					{ return TileNumber; }
	sTile& GetTile(int i,int j)							{ return Tile[i+j*GetTileNumber().x]; }
	int GetZeroplastNumber()	const					{ return zeroplastnumber; }

	sColor4f GetZeroplastColor(int player)
	{
		xassert(player>=0 && player<zeroplast_color.size());
		return zeroplast_color[player];
	}

	void SetZeroplastColor(int player,const sColor4f& color)
	{
		VISASSERT(player>=0 && player<zeroplastnumber);
		zeroplast_color[player]=color;
	}

	void SetTilemapRender(cTileMapRender* p){VISASSERT(pTileMapRender==NULL);pTileMapRender=p;};
	cTileMapRender* GetTilemapRender(){return pTileMapRender;}

	TerraInterface* GetTerra(){return terra;}

	Vect2f CalcZ(cCamera *DrawNode);
	sBox6f CalcShadowReciverInSpace(cCamera *DrawNode,D3DXMATRIX matrix);

	void RegisterUpdateMap(UpdateMapFunction f,void* data);
	void UnRegisterUpdateMap(UpdateMapFunction f,void* data);
	int& GetMiniDetailRes(){return miniDetailTexRes_;}

	const Vect3f& GetTileScale()const {return tile_scale;}
	void SetTileScale(const Vect3f& s){tile_scale=s;}
	inline const sColor4f& GetDiffuse()	const 										{ return diffuse; }
	void SetDiffuse(const sColor4f& color)
	{
		VISASSERT(color.r>=0 && color.r<100.0f);
		VISASSERT(color.g>=0 && color.g<100.0f);
		VISASSERT(color.b>=0 && color.b<100.0f);
		VISASSERT(color.a>=0 && color.a<2.0f);
		diffuse=color;
	}

	virtual const MatXf& GetPosition() const {return MatXf::ID;}
protected:
	/////////////to initialize
	void SetBuffer(const Vect2i &size,int zeroplastnumber);

	////////////////////
	void CalcZMinMax(int x,int y);

	void BuildRegionPoint();
	friend void cTileMapBorderCall(void* data,Vect2f& p);

	Vect3f To3D(const Vect2f& pos);
	void DrawLines();

	struct UpdateMapData
	{
		UpdateMapFunction f;
		void* data;
		bool operator==(const UpdateMapData& d)
		{
			return f==d.f && data==d.data;
		}
	};
	vector<UpdateMapData> func_update_map;
};

#endif
