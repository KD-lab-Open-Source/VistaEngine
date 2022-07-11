#ifndef __DRAW_TYPE_H_INCLUDED__
#define __DRAW_TYPE_H_INCLUDED__
class cTileMap;
enum eDrawID
{
	DT_UNKNOWN=0,
	DT_RADEON9700,
	DT_GEFORCEFX,
	DT_MINIMAL,
};

class DrawType
{
public:
	DrawType();
	virtual ~DrawType();

	virtual eDrawID GetID()=0;
	virtual void BeginDraw();
	virtual void BeginDrawShadow()=0;//Сейчас вызываются не там где нужно, и не имеют особого смысла!
	virtual void EndDrawShadow()=0;

	virtual bool CreateShadowTexture(int xysize)=0;
	bool CreateFloatTexture(int width, int height);
	virtual void DeleteShadowTexture();
	virtual cTexture* GetShadowMap()							{return pShadowMap;}
	virtual LPDIRECT3DSURFACE9 GetZBuffer()						{return pZBuffer;}
	virtual cTexture* GetLightMap()								{return pLightMap;}
	virtual cTexture* GetLightMapObjects()						{return pLightMapObjects;}
	cTexture* GetFloatMap();	
	LPDIRECT3DSURFACE9 GetFloatZBuffer()						{return pFloatZBufferSurface;}

	virtual void SetMaterialTilemap(int i);
	virtual void SetMaterialTilemapShadow()=0;
	virtual void SetShadowMapTexture(){};
	virtual void SetMaterialTilemapZBuffer();

	virtual void BeginDrawTilemap(cTileMap *TileMap_);
	virtual void EndDrawTilemap();
	cTileMap*	GetTileMapDebug(){return TileMap;}
	void CreateMirageMap(int x, int y, bool recreate=false);
	LPDIRECT3DSURFACE9 GetMirageMap(){return pMirageBuffer;}

	virtual IDirect3DTexture9* GetTZBuffer(){return NULL;}

	virtual void SetTileColor(sColor4f color);//r,g,b=[0..2], a=ignored etc
	virtual void SetReflection(bool reflection);

	virtual void NextTile(IDirect3DTextureProxy* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump)=0;

	virtual cTexture* GetAccessibleZBuffer() {return NULL;};

	PSFillColor* pPSClearAlpha;
protected:
	cTexture*		pShadowMap;
	//cTexture*		pMirageMap;
	LPDIRECT3DSURFACE9 pMirageBuffer;
	LPDIRECT3DSURFACE9	pZBuffer;
	cTexture*		pFloatZBuffer;
	LPDIRECT3DSURFACE9 pFloatZBufferSurface;
	cTexture*		pLightMap;
	cTexture*		pLightMapObjects;
	cTileMap*		TileMap;
	VSTileMapScene*  pVSTileMapScene;	
	PSTileMapScene*  pPSTileMapScene;
	VSTileZBuffer* vsTileZBuffer;
	PSSkinZBuffer* psSkinZBuffer;

	IDirect3DTextureProxy* pTile;
	IDirect3DTextureProxy* pBump;
};

class DrawTypeMinimal:public DrawType
{
public:
	DrawTypeMinimal();
	~DrawTypeMinimal();

	virtual eDrawID GetID(){return DT_MINIMAL;};
	virtual void BeginDrawShadow(){xassert(0);};
	virtual void EndDrawShadow(){xassert(0);};

	virtual bool CreateShadowTexture(int xysize);
	virtual void SetMaterialTilemapShadow();
	virtual void DeleteShadowTexture();
	virtual IDirect3DTexture9* GetTZBuffer(){return NULL;}

	void NextTile(IDirect3DTextureProxy* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
protected:
};

class DrawTypeRadeon9700:public DrawType
{
	VSShadow* pVSTileMapShadow;
	cPixelShader* pPSTileMapShadow;

public:
	DrawTypeRadeon9700();
	~DrawTypeRadeon9700();

	virtual eDrawID GetID(){return DT_RADEON9700;};
	virtual void BeginDrawShadow(){BeginDraw();};
	virtual void EndDrawShadow(){};

	virtual bool CreateShadowTexture(int xysize);
	virtual void SetMaterialTilemapShadow();

	virtual void NextTile(IDirect3DTextureProxy* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	virtual void SetShadowMapTexture();

protected:
};

class DrawTypeGeforceFX:public DrawType
{
	IDirect3DTexture9* ptZBuffer;
	cTexture* pAccessibleZBuffer;

	VSShadow* pVSTileMapShadow;
	cPixelShader* pPSTileMapShadow;
public:
	DrawTypeGeforceFX();
	~DrawTypeGeforceFX();

	virtual eDrawID GetID(){return DT_GEFORCEFX;};
	virtual void BeginDrawShadow();
	virtual void EndDrawShadow();

	virtual bool CreateShadowTexture(int xysize);
	virtual void SetMaterialTilemapShadow();

	virtual void DeleteShadowTexture();
	virtual IDirect3DTexture9* GetTZBuffer(){return ptZBuffer;}
	void NextTile(IDirect3DTextureProxy* pTile,Vect2f& uv_base,Vect2f& uv_step,
				  IDirect3DTextureProxy* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	virtual void SetShadowMapTexture();

	cTexture* GetAccessibleZBuffer() {return pAccessibleZBuffer;};
protected:
	DWORD old_colorwrite;
};

#endif
