#include "StdAfxRD.h"
#include "DrawType.h"
#include "D3DRender.h"
#include "TileMap.h"
#include "VisGeneric.h"
#include "Terra\vmap.h"

DrawType::DrawType()
{
	tileMap_ = 0;

	shaderSceneTileMap_=0;
	pTile=pBump=0;
	
	pPSClearAlpha = new PSFillColor;
	pPSClearAlpha->Restore();
	vsTileZBuffer = new VSTileZBuffer();
	vsTileZBuffer->Restore();
	psSkinZBuffer = new PSSkinZBuffer();
	psSkinZBuffer->Restore();
}

DrawType::~DrawType()
{
	delete shaderSceneTileMap_;

	delete pPSClearAlpha;
	delete vsTileZBuffer;
	delete psSkinZBuffer;
}

void DrawType::BeginDrawTilemap(cTileMap *TileMap_)
{
	xassert(tileMap_==0);
	xassert(TileMap_);
	tileMap_=TileMap_;
}

void DrawType::EndDrawTilemap()
{
	xassert(tileMap_);
	tileMap_=0;
	for(int i=0;i<8;i++)
		gb_RenderDevice3D->SetTextureBase(i,0);
}

void DrawType::SetTileColor(Color4f c)
{
	const Color4f& diffuse=tileMap_->GetDiffuse();
	c.r*=diffuse.r;
	c.g*=diffuse.g;
	c.b*=diffuse.b;
	c.a=diffuse.a;
	shaderSceneTileMap_->SetColor(c);
}

void DrawType::SetReflection(bool reflection)
{
	shaderSceneTileMap_->SetReflectionZ(reflection);
}

void DrawType::BeginDraw()
{
	for(int i=0;i<6;i++)
		gb_RenderDevice3D->SetTextureBase(i,0);

	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_anisotropic);
	gb_RenderDevice3D->SetSamplerData(2,sampler_wrap_anisotropic);
}

void DrawType::SetMaterialTilemap(const Color4f& shadowIntensity, cTexture* miniDetailTexture, float miniDetailResolution)
{
	gb_RenderDevice3D->SetTexture(3, gb_RenderDevice3D->GetLightMap());
	gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_anisotropic);

	shaderSceneTileMap_->SetWorldSize(Vect2f(vMap.H_SIZE, vMap.V_SIZE));
	gb_RenderDevice3D->SetTextureBase(0, pTile);
	if(!Option_tileMapVertexLight)
		gb_RenderDevice3D->SetTextureBase(1, pBump);
	else
		gb_RenderDevice3D->SetTextureBase(1, 0);

	if(miniDetailTexture){
		gb_RenderDevice3D->SetTexture(4, miniDetailTexture);
		shaderSceneTileMap_->SetMiniTextureSize(miniDetailTexture->GetWidth(), miniDetailTexture->GetHeight(), miniDetailResolution);
	}

	shaderSceneTileMap_->SetShadowIntensity(shadowIntensity);
	shaderSceneTileMap_->Select();
}

void DrawType::SetMaterialTilemapZBuffer()
{
	vsTileZBuffer->Select();
	psSkinZBuffer->Select();
}

//////////////////////////////DrawTypeMinimal/////////////////////
DrawTypeMinimal::DrawTypeMinimal()
{
	shaderSceneTileMap_=new ShaderSceneTileMapMinimal;
	shaderSceneTileMap_->Restore();

	pTile=pBump=0;
}

DrawTypeMinimal::~DrawTypeMinimal()
{
}

void DrawTypeMinimal::SetMaterialTilemapShadow()
{
	xassert(0);
}

void DrawTypeMinimal::NextTile(IDirect3DTexture9* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	shaderSceneTileMap_->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
	pTile=pTile_;
	pBump=pBump_;
}


//////////////////////////////DrawTypeRadeon9700/////////////////////
DrawTypeRadeon9700::DrawTypeRadeon9700()
{
	pVSTileMapShadow=new VSShadow;
	pVSTileMapShadow->Restore();
	pPSTileMapShadow=new PSShadow;
	pPSTileMapShadow->Restore();

	shaderSceneTileMap_=new ShaderSceneTileMap9700;
	shaderSceneTileMap_->Restore();
}

DrawTypeRadeon9700::~DrawTypeRadeon9700()
{
	delete pVSTileMapShadow;
	delete pPSTileMapShadow;
}

void DrawTypeRadeon9700::NextTile(IDirect3DTexture9* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	shaderSceneTileMap_->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
	pTile=pTile_;
	pBump=pBump_;

	//gb_RenderDevice3D->SetSamplerData( 3,sampler_clamp_point);
	//gb_RenderDevice3D->SetTexture(3, pShadowMap);
	SetShadowMapTexture();
}
void DrawTypeRadeon9700::SetShadowMapTexture()
{
	gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_point);
	gb_RenderDevice3D->SetTexture(2, gb_RenderDevice3D->GetShadowMap());
}

void DrawTypeRadeon9700::SetMaterialTilemapShadow()
{
	pVSTileMapShadow->Select();
	pPSTileMapShadow->Select();
}

//////////////////////////////DrawTypeGeforceFX/////////////////////
DrawTypeGeforceFX::DrawTypeGeforceFX()
{
	shaderSceneTileMap_=new ShaderSceneTileMapGeforceFX;
	shaderSceneTileMap_->Restore();

	pVSTileMapShadow=new VSShadow;
	pVSTileMapShadow->Restore();
	pPSTileMapShadow=new PSShadow;
	pPSTileMapShadow->Restore();
}

DrawTypeGeforceFX::~DrawTypeGeforceFX()
{
	delete pVSTileMapShadow;
	delete pPSTileMapShadow;
}

void DrawTypeGeforceFX::BeginDrawShadow()
{
	old_colorwrite=gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
}

void DrawTypeGeforceFX::EndDrawShadow()
{
	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE, old_colorwrite);
}

void DrawTypeGeforceFX::SetMaterialTilemapShadow()
{
	pVSTileMapShadow->Select();
	pPSTileMapShadow->Select();
}

void DrawTypeGeforceFX::NextTile(IDirect3DTexture9* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTexture9* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	shaderSceneTileMap_->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
	pTile=pTile_;
	pBump=pBump_;

	{
		int ss=3;
//*
		//gb_RenderDevice3D->SetSamplerData( ss,sampler_clamp_linear);
/*/
		gb_RenderDevice3D->SetSamplerData( ss,sampler_clamp_point);
/**/
		//gb_RenderDevice3D->SetTextureBase(ss,ptZBuffer);
		//gb_RenderDevice3D->SetTexture(pShadowMap,0,ss);
		SetShadowMapTexture();
	}
}
void DrawTypeGeforceFX::SetShadowMapTexture()
{
	gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_linear);
	gb_RenderDevice3D->SetTextureBase(2, gb_RenderDevice3D->GetTZBuffer());
}



