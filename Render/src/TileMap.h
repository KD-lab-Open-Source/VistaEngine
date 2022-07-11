#ifndef __TILE_MAP_H_INCLUDED__
#define __TILE_MAP_H_INCLUDED__

#include "Render\Inc\IVisGenericInternal.h"
#include "Render\Inc\IRenderDevice.h"
#include "Terra\UpdateMapClient.h"
#include "Starforce.h"

class cScene;
class Mat4f;

const int TILEMAP_SHL  = 6;
const int TILEMAP_SIZE = 1<<TILEMAP_SHL;
const int TILEMAP_LOD=5;

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
	BYTE zmin, zmax, zavr;
	DWORD update_zminmmax_time;

	sTile()								
	{
		bumpTileID = -1;
		zmin=255;zmax=0;
		update_zminmmax_time=0;
	}
};

typedef vector<vector<Vect2s>* > CurrentRegion;
typedef void (*UpdateMapFunction)(const Vect2i& pos1, const Vect2i& pos2,void* data);

class cTileMapRender;

class RENDER_API cTileMap : public BaseGraphObject, UpdateMapClient
{
public:
	enum {
		miniDetailTexturesNumber = 16,
		placementZoneMaterialNumber = 8,
		multiRegionLayersNumber = miniDetailTexturesNumber + placementZoneMaterialNumber + 1
	};

	enum ShaderType {
		LAVA, 
		ICE
	};

	cTileMap(cScene* pScene, bool trueColorEnable = true);
	virtual ~cTileMap();

	STARFORCE_API void serialize(Archive& ar);

	void updateMap(const Vect2i& pos1, const Vect2i& pos2, UpdateMapType type=UPDATEMAP_ALL);

	virtual void PreDraw(Camera* camera);
	virtual void Draw(Camera* camera);
	void Animate(float dt);
	// общие интерфейсные функции cTileMap
	const Vect2i& tileSize()const					{ return tileSize_; }
	const Vect2i& tileNumber()const					{ return tileNumber_; }
	sTile& GetTile(int i,int j)							{ return tiles_[i+j*tileNumber().x]; }

	int zMax() const { return zMax_; }

	float getZ(int x,int y) const;
	void getNormal(int x, int y, int step, Vect3f& normal) const;

	const Color4f& GetZeroplastColor(int player) const 
	{
		xassert(player >= 0 && player < miniDetailTexturesNumber);
		return zeroplast_color[player];
	}

	cTileMapRender* GetTilemapRender() { return tileMapRender_; }

	Vect2f CalcZ(Camera* camera);
	sBox6f CalcShadowReciverInSpace(Camera* camera,const Mat4f& matrix);

	const Vect3f& GetTileScale()const {return tile_scale;}
	void SetTileScale(const Vect3f& s){tile_scale=s;}
	const Color4f& GetDiffuse()	const { return diffuse; }
	void SetDiffuse(const Color4f& color)
	{
		xassert(color.r>=0 && color.r<100.0f);
		xassert(color.g>=0 && color.g<100.0f);
		xassert(color.b>=0 && color.b<100.0f);
		xassert(color.a>=0 && color.a<2.0f);
		diffuse=color;
	}

	bool isTrueColorEnable(){ return trueColorEnable_; }

	struct MiniDetailTexture
	{
		string textureName;
		UnknownHandle<cTexture> texture;

		MiniDetailTexture();
		void serialize(Archive& ar);
		void setTexture(const char* name) { textureName = name; setTexture(); }
		void setTexture();

		static int resolution;
	};

	int miniDetailTextureResolution() const { return 1 << miniDetailTextureResolutionPower_; }
	int miniDetailTextureResolutionPower() const { return miniDetailTextureResolutionPower_; }
	MiniDetailTexture& miniDetailTexture(int i) {	return miniDetailTextures_[i]; }

	struct PlacementZoneMaterial
	{
		string textureName;
		UnknownHandle<cTexture> texture;

		ShaderType shaderType;

		Color4c minimapColor;
		Color4f lavaColor;
		Color4f colorAmbient;
		float speed;
		float textureScale;
		float volumeTextureScale;

		string textureBumpName;
		UnknownHandle<cTexture> textureBump;

		string textureCleftName;
		UnknownHandle<cTexture> textureCleft;

		PlacementZoneMaterial();
		void serialize(Archive& ar);
	};

	const PlacementZoneMaterial& placementZoneMaterial(int index) const { return placementZoneMaterials_[index]; }

	bool setMaterial(int material, eBlendMode MatMode); // returns is Ice

protected:
	sTile*			tiles_;
	Vect2i			tileSize_;		// размер одного тайла
	Vect2i			tileNumber_;		// число тайлов по осям
	int zMax_;

	cTileMapRender* tileMapRender_;

	int			update_zminmmax_time;
	Color4f zeroplast_color[miniDetailTexturesNumber];

	struct UpdateRect
	{
		Vect2i p1,p2;
		int type;
	};
	vector<UpdateRect> update_rect;
	MTSection lock_update_rect;

	short* tileBordersX_; // [tileX][y]
	short* tileBordersY_; // [tileY][x]
	Vect2i tileBordersShr_;
	Vect2i tileBordersSize_;
	float heightFractionInv_;

	struct DebugRect
	{
		Vect2i p1,p2;
		float time;
	};
	bool enable_debug_rect;
	float debug_fade_interval;
	list<DebugRect> debug_rect;

	Vect3f tile_scale;
	Color4f diffuse;
	bool trueColorEnable_;

	MiniDetailTexture miniDetailTextures_[miniDetailTexturesNumber];
	int miniDetailTextureResolutionPower_;

	PlacementZoneMaterial placementZoneMaterials_[placementZoneMaterialNumber];
	float animationTime_;

	class ShaderSceneWaterLava* lavaShader_;
	class ShaderSceneWaterIce* iceShader_;

	////////////////////
	void updateTileZ(int x,int y);

	void BuildRegionPoint();
	friend void cTileMapBorderCall(void* data,Vect2f& p);

	Vect3f To3D(const Vect2f& pos);
	void DrawLines();

	friend class cScene;
};

extern cTileMap* tileMap;

#endif
