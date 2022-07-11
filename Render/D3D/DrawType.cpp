#include "StdAfxRD.h"

#include "DrawType.h"
#include "Scene.h"
#include "TileMap.h"

#define DEL(p) {if(p)delete p;p=NULL;}

DrawType::DrawType():pShadowMap(0),pZBuffer(0),pLightMap(0),TileMap(0),pLightMapObjects(0)
{
	pVSTileMapScene=NULL;
	pPSTileMapScene=NULL;
	pTile=pBump=NULL;
	
	pPSClearAlpha = new PSFillColor;
	pPSClearAlpha->Restore();
	pMirageBuffer = NULL;
	pFloatZBuffer = NULL;
	pFloatZBufferSurface =  NULL;
	vsTileZBuffer = new VSTileZBuffer();
	vsTileZBuffer->Restore();
	psSkinZBuffer = new PSSkinZBuffer();
	psSkinZBuffer->Restore();

}

DrawType::~DrawType()
{
	DeleteShadowTexture();
	DEL(pVSTileMapScene);
	DEL(pPSTileMapScene);

	DEL(pPSClearAlpha);
	DEL(vsTileZBuffer);
	DEL(psSkinZBuffer);

}

void DrawType::DeleteShadowTexture()
{
	RELEASE(pShadowMap);
	RELEASE(pZBuffer);
	RELEASE(pLightMap);
	RELEASE(pLightMapObjects);
	RELEASE(pMirageBuffer);
	RELEASE(pFloatZBuffer);
	RELEASE(pFloatZBufferSurface);
}
bool DrawType::CreateFloatTexture(int width, int height)
{
	if (gb_RenderDevice3D->TexFmtData[SURFMT_R32F]==D3DFMT_UNKNOWN)
		return false;
	pFloatZBuffer = GetTexLibrary()->CreateRenderTexture(width,height,TEXTURE_R32F);
	if (!pFloatZBuffer)
		return false;

	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->	CreateDepthStencilSurface(width, height, 
		D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, TRUE, &pFloatZBufferSurface, NULL);
	if(FAILED(hr))
	{
		xassert(0);
		RELEASE(pFloatZBufferSurface);
		return false;
	}
	return true;
}
cTexture* DrawType::GetFloatMap()
{
	return pFloatZBuffer;
}
void DrawType::CreateMirageMap(int x, int y, bool recreate)
{
	//if (!pMirageMap||recreate)
	//{
	//	RELEASE(pMirageMap);
	//	pMirageMap = GetTexLibrary()->CreateRenderTexture(x,y,TEXTURE_RENDER32);
	//}
	if (!pMirageBuffer)
	{
		RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateRenderTarget(x,y,
			gb_RenderDevice3D->d3dpp.BackBufferFormat,
			gb_RenderDevice3D->d3dpp.MultiSampleType,
			gb_RenderDevice3D->d3dpp.MultiSampleQuality,FALSE,&pMirageBuffer,NULL));
	}
}

void DrawType::BeginDrawTilemap(cTileMap *TileMap_)
{
	xassert(TileMap==NULL);
	xassert(TileMap_);
	TileMap=TileMap_;
}

void DrawType::EndDrawTilemap()
{
	xassert(TileMap);
	TileMap=NULL;
	for(int i=0;i<8;i++)
		gb_RenderDevice3D->SetTextureBase(i,NULL);
}

void DrawType::SetTileColor(sColor4f c)
{
	const sColor4f& diffuse=TileMap->GetDiffuse();
	c.r*=diffuse.r;
	c.g*=diffuse.g;
	c.b*=diffuse.b;
	c.a=diffuse.a;
	pVSTileMapScene->SetColor(c);
	pPSTileMapScene->SetColor(c);
}

void DrawType::SetReflection(bool reflection)
{
	pVSTileMapScene->SetReflectionZ(reflection);
	pPSTileMapScene->SetReflectionZ(reflection);
}

void DrawType::BeginDraw()
{
	for(int i=0;i<6;i++)
		gb_RenderDevice3D->SetTextureBase(i,NULL);

	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_anisotropic);
	gb_RenderDevice3D->SetSamplerData(2,sampler_wrap_anisotropic);
}

void DrawType::SetMaterialTilemap(int matarial_num)
{
	gb_RenderDevice3D->SetTexture(3, pLightMap);
	gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);

	TerraInterface* terra=TileMap->GetTerra();
	pVSTileMapScene->SetWorldSize(Vect2f(terra->SizeX(),terra->SizeY()));
	gb_RenderDevice3D->SetTextureBase(0, GETIDirect3DTexture(pTile));
	if(Option_TileMapTypeNormal)
		gb_RenderDevice3D->SetTextureBase(1, GETIDirect3DTexture(pBump));

	pVSTileMapScene->Select();
	pPSTileMapScene->SetShadowIntensity(TileMap->GetScene()->GetShadowIntensity());
	pPSTileMapScene->Select(matarial_num,pVSTileMapScene,(float)TileMap->GetMiniDetailRes());
}
void DrawType::SetMaterialTilemapZBuffer()
{
	vsTileZBuffer->Select();
	psSkinZBuffer->Select();
}

#include "DrawTypeRadeon9700.inl"
#include "DrawTypeGeforceFX.inl"
#include "DrawTypeMinimal.inl"
