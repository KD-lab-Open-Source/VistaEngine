#ifndef __DRAW_TYPE_H_INCLUDED__
#define __DRAW_TYPE_H_INCLUDED__

#include "XMath\Colors.h"
#include "Render\src\Texture.h"
#include "Render\Shader\Shaders.h"


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
	void BeginDraw();
	virtual void BeginDrawShadow()=0;//Сейчас вызываются не там где нужно, и не имеют особого смысла!
	virtual void EndDrawShadow()=0;

	virtual void SetMaterialTilemap(const Color4f& shadowIntensity, cTexture* miniDetailTexture, float miniDetailResolution);
	virtual void SetMaterialTilemapShadow()=0;
	virtual void SetShadowMapTexture(){}
	void SetMaterialTilemapZBuffer();

	void BeginDrawTilemap(cTileMap *TileMap_);
	void EndDrawTilemap();

	void SetTileColor(Color4f color);//r,g,b=[0..2], a=ignored etc
	void SetReflection(bool reflection);

	virtual void NextTile(IDirect3DTexture9* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump)=0;

	PSFillColor* pPSClearAlpha;

protected:
	cTileMap*		tileMap_;
	ShaderSceneTileMap*  shaderSceneTileMap_;
	VSTileZBuffer* vsTileZBuffer;
	PSSkinZBuffer* psSkinZBuffer;

	IDirect3DTexture9* pTile;
	IDirect3DTexture9* pBump;
};

class DrawTypeMinimal : public DrawType
{
public:
	DrawTypeMinimal();
	~DrawTypeMinimal();

	eDrawID GetID(){return DT_MINIMAL;}
	void BeginDrawShadow(){xassert(0);}
	void EndDrawShadow(){xassert(0);}

	void SetMaterialTilemapShadow();

	void NextTile(IDirect3DTexture9* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
};

class DrawTypeRadeon9700 : public DrawType
{
	VSShadow* pVSTileMapShadow;
	PSShadow* pPSTileMapShadow;

public:
	DrawTypeRadeon9700();
	~DrawTypeRadeon9700();

	eDrawID GetID(){return DT_RADEON9700;};
	void BeginDrawShadow(){BeginDraw();}
	void EndDrawShadow(){}

	void SetMaterialTilemapShadow();

	void NextTile(IDirect3DTexture9* pTile,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	void SetShadowMapTexture();
};

class DrawTypeGeforceFX : public DrawType
{
	VSShadow* pVSTileMapShadow;
	PSShadow* pPSTileMapShadow;
public:
	DrawTypeGeforceFX();
	~DrawTypeGeforceFX();

	eDrawID GetID(){return DT_GEFORCEFX;}
	void BeginDrawShadow();
	void EndDrawShadow();

	void SetMaterialTilemapShadow();

	void NextTile(IDirect3DTexture9* pTile,Vect2f& uv_base,Vect2f& uv_step,
				  IDirect3DTexture9* pBump,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	
	void SetShadowMapTexture();

protected:
	DWORD old_colorwrite;
};

#endif
