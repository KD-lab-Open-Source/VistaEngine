//////////////////////////////DrawTypeGeforceFX/////////////////////
DrawTypeGeforceFX::DrawTypeGeforceFX()
{
	ptZBuffer=NULL;
	pAccessibleZBuffer=NULL;

	pVSTileMapScene=new VSGeforceFXTileMapScene;
	pVSTileMapScene->Restore();
	pPSTileMapScene=new PSGeforceFXTileMapScene;
	pPSTileMapScene->Restore();

	pVSTileMapShadow=new VSShadow;
	pVSTileMapShadow->Restore();
	pPSTileMapShadow=new PSShadow;
	pPSTileMapShadow->Restore();
}

DrawTypeGeforceFX::~DrawTypeGeforceFX()
{
	RELEASE(pAccessibleZBuffer);
	RELEASE(ptZBuffer);

	DEL(pVSTileMapScene);
	DEL(pPSTileMapScene);
	DEL(pVSTileMapShadow);
	DEL(pPSTileMapShadow);
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

bool DrawTypeGeforceFX::CreateShadowTexture(int xysize)
{
	DeleteShadowTexture();

	pShadowMap=GetTexLibrary()->CreateRenderTexture(xysize,xysize,TEXTURE_RENDER16,false);
	if(!pShadowMap)
	{
		DeleteShadowTexture();
		return false;
	}

	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateTexture(xysize, xysize, 1, D3DUSAGE_DEPTHSTENCIL, 
		D3DFMT_D16, 
		//D3DFMT_D24X8, 
		D3DPOOL_DEFAULT, &ptZBuffer,0);
	if(FAILED(hr))
	{
		DeleteShadowTexture();
		return false;
	}

	RDCALL(ptZBuffer->GetSurfaceLevel(0,&pZBuffer));

	pLightMap=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	pLightMapObjects=GetTexLibrary()->CreateRenderTexture(256,256,TEXTURE_RENDER32,false);
	if(!pLightMap || !pLightMapObjects)
	{
		DeleteShadowTexture();
		return false;
	}


	if(Option_AccessibleZBuffer)
	{
		pAccessibleZBuffer=GetTexLibrary()->CreateRenderTexture(gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY(),
			//TEXTURE_RENDER_SHADOW_9700,
			TEXTURE_RENDER32,
			false);
		if(!pAccessibleZBuffer)
			return false;
	}

	return true;
}

void DrawTypeGeforceFX::SetMaterialTilemapShadow()
{
	pVSTileMapShadow->Select();
	pPSTileMapShadow->Select();
}

void DrawTypeGeforceFX::NextTile(IDirect3DTextureProxy* pTile_,Vect2f& uv_base,Vect2f& uv_step,
						  IDirect3DTextureProxy* pBump_,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	pVSTileMapScene->SetUV(uv_base,uv_step,uv_base_bump,uv_step_bump);
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
	gb_RenderDevice3D->SetTextureBase(2,ptZBuffer);
}

void DrawTypeGeforceFX::DeleteShadowTexture()
{
	__super::DeleteShadowTexture();
	RELEASE(ptZBuffer);
	RELEASE(pAccessibleZBuffer);
}
