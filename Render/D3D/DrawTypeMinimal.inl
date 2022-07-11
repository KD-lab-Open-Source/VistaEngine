//////////////////////////////DrawTypeMinimal/////////////////////
DrawTypeMinimal::DrawTypeMinimal()
{
	pVSTileMapScene=new VSMinimalTileMapScene;
	pVSTileMapScene->Restore();
	pPSTileMapScene=new PSMinimalTileMapScene;
	pPSTileMapScene->Restore();

	pTile=pBump=NULL;
}

DrawTypeMinimal::~DrawTypeMinimal()
{
	DEL(pVSTileMapScene);
	DEL(pPSTileMapScene);
}

bool DrawTypeMinimal::CreateShadowTexture(int xysize)
{
	DeleteShadowTexture();

	pLightMap=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	pLightMapObjects=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	if(!pLightMap || !pLightMapObjects)
	{
		DeleteShadowTexture();
		return false;
	}
	return true;
}

void DrawTypeMinimal::SetMaterialTilemapShadow()
{
	xassert(0);
}

void DrawTypeMinimal::NextTile(IDirect3DTextureProxy* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	pVSTileMapScene->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
	pTile=pTile_;
	pBump=pBump_;
}

void DrawTypeMinimal::DeleteShadowTexture()
{
	__super::DeleteShadowTexture();
}
