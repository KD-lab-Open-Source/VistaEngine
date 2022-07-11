#pragma once
#include "..\inc\IVisGeneric.h"

class VSGrass;
class PSGrassShadow;
class PSGrassShadowFX;
class PSGrass;
struct Bush
{
	Bush()
	{
		needDelete = false;
		vbOffset=0;
	}
	sShort4 pos;
	short shift;
	sColor4c normal;
	short material;
	float windPower;
	bool needDelete;
	int vbOffset;
};

struct GrassTile
{
	GrassTile()
	{
		drawTileID = -1;
		initVertex = false;
		recreate = false;
		dtime = 0;
	}
	vector<Bush> bushes_;
	Vect2f pos;
	int drawTileID;
	bool initVertex;
	float dtime;
	bool recreate;
};

struct DrawTile
{
	sPtrVertexBuffer vertexBuffer;
	sPtrIndexBuffer forwardIndex;
	sPtrIndexBuffer backwardIndex;
	Vect2i tilePos;
	short bladeCount;

public:
	DrawTile();
	~DrawTile();

	void Init(int xpos, int ypos);
	void InitVertex(int bladeCount);
	void InitIndex(int bladeCount);
	void DeleteVertex();
	void DeleteIndex();

	BYTE* LockVB();
	void UnlockVB();
	sPolygon* LockForwardIB();
	sPolygon* LockBackwardIB();
	void UnlockForwardIB();
	void UnlockBackwardIB();

	static IDirect3DVertexDeclaration9* GetVertexDeclaration() {return shortVertexGrass::declaration;}
};

struct MaterialBushes
{
	vector<Bush*> bushes_;
	short textureNumber;
};
struct SortedTile{
	GrassTile* tile;
	float distance;
	bool alphaBlend;
};
struct UpdateTile
{
	GrassTile* tile;
	vector<Bush*> bushes;
};
struct TilesSortByRadius
{
	inline bool operator()(const SortedTile& o1,const SortedTile& o2)
	{
		return o1.distance>o2.distance;
	}
};

class GrassMap:public cBaseGraphObject
{
public:
	GrassMap();
	~GrassMap();
	
	void Init(cScene* pScene);

	const Vect2i& getTileNumber() {return tileNumber_;}
	GrassTile& getTile(int x, int y);
	void Animate(float dt);
	void PreDraw(cCamera *UCamera);
	void Draw(cCamera *UCamera);
	virtual const MatXf& GetPosition() const {return MatXf::ID;};
	void SaveMap();
	void SetGrass(float x, float y, float radius, int texture,int count, bool clear,int intensity,int intensity2);
	void SetAllGrass(int texture, int intensity,int intensity2);
	void serialize(Archive& ar);
	void SetTexture(const char* name, int num);
	const char* GetTextureName(int num);
	cTexture* GetTexture() {return texture_;}
	void EnableShadow(bool enable){enbaleShadow_ = enable;}
	void UpdateMap(const Vect2i& pos1, const Vect2i& pos2);
	void DeleteGrass(Vect3f pos, float radius);
	void DownGrass(Vect3f pos, float radius);
	void Enable(bool enable) {enable_ = enable;}
	int GetSpecialSortIndex()const{return -4;}
	DWORD GetVertexSize();
	void SetDensity(float density);
protected:
	void GenerateGrass();
	void GenerateGrass(Vect2i lock_min, Vect2i lock_size);
	void CreateTexture();
	void DrawGrass(eBlendMode mode,cCamera *UCamera);
	void ClearGrass();
	bool TestVisible(GrassTile& tile,cCamera* pCamera);
	void InitTextures();
	void UpdateVB();
	void BuildGrass(Vect2i min, Vect2i size);
	int drawTileValid(int id);
	void drawTileFree(int id);
	int drawTileAlloc(int bladeCount);
	void drawTileFreeAll();
	void drawTileDeath();
	void CalcVertex(GrassTile& tile);
	void UpdateTilesAfterWrapTime();

	enum{
		tileShift = 6,
		tileSize = 1<<tileShift,
		grassMapShift = 3,
		textureCountShift = 5,
	};
	cTileMap *terMapPoint;
	vector<DrawTile*> drawTiles;
	vector<int> deadDrawTiles;
	GrassTile* grassMap_;
	Vect2i tileNumber_;
	Vect2i grassMapSize_;
	cTexture* textureMap_;
	cTexture* texture_;
	vector<Bush> bushes_;
	short time;
	int textureCount_;
	string grassMapName_;
	vector<string> textureNames_;
	bool enbaleShadow_;
	float time_;
	float hideDistance_;
	float hideDistance2_;
	float invHideDistance2_;
	vector<SortedTile> sortedTile_;
	struct SortedBush
	{
		Bush *bush;
		float distance;
	};
	vector<SortedBush> sortedBush_;
	struct BushSortByRadius
	{
		inline bool operator()(const SortedBush& o1,const SortedBush& o2)
		{
			return o1.distance>o2.distance;
		}
	};
	float dtime_;
	bool enable_;
	VSGrass* vsGrass;
	PSGrassShadow* psGrassShadow;
	PSGrass* psGrass;
	PSSkinZBufferAlpha* psSkinZBuffer;
	bool oldLighting;
	vector<UpdateTile> updatedTiles_;
	float density_;
};
