//////////////////////////////DrawTypeRadeon9700/////////////////////
DrawTypeRadeon9700::DrawTypeRadeon9700()
{
	pShadowMap=NULL;

	pVSTileMapShadow=new VSShadow;
	pVSTileMapShadow->Restore();
	pPSTileMapShadow=new PSShadow;
	pPSTileMapShadow->Restore();

	pVSTileMapScene=new VS9700TileMapScene;
	pVSTileMapScene->Restore();
	pPSTileMapScene=new PS9700TileMapScene4x4;
	pPSTileMapScene->Restore();
}

DrawTypeRadeon9700::~DrawTypeRadeon9700()
{
	DEL(pVSTileMapShadow);
	DEL(pPSTileMapShadow);
	DEL(pVSTileMapScene);
	DEL(pPSTileMapScene);
}

bool DrawTypeRadeon9700::CreateShadowTexture(int xysize)
{
	DeleteShadowTexture();
	pShadowMap=GetTexLibrary()->CreateRenderTexture(xysize,xysize,TEXTURE_RENDER_SHADOW_9700,false);
	if(!pShadowMap)
	{
		DeleteShadowTexture();
		return false;
	}

	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateDepthStencilSurface(xysize, xysize, 
		D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &pZBuffer, NULL);
	if(FAILED(hr))
	{
		DeleteShadowTexture();
		return false;
	}


	pLightMap=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	pLightMapObjects=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	if(!pLightMap || !pLightMapObjects)
	{
		DeleteShadowTexture();
		return false;
	}

	return true;
}

void DrawTypeRadeon9700::NextTile(IDirect3DTextureProxy* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	pVSTileMapScene->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
	pTile=pTile_;
	pBump=pBump_;

	//gb_RenderDevice3D->SetSamplerData( 3,sampler_clamp_point);
	//gb_RenderDevice3D->SetTexture(3, pShadowMap);
	SetShadowMapTexture();
}
void DrawTypeRadeon9700::SetShadowMapTexture()
{
	gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_point);
	gb_RenderDevice3D->SetTexture(2,pShadowMap);
}

void DrawTypeRadeon9700::SetMaterialTilemapShadow()
{
	pVSTileMapShadow->Select();
	pPSTileMapShadow->Select();
}
