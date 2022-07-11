////////////////////////////GeforceFX///////////////////////////////////

void VSGeforceFXTileMapScene::Select()
{
	SetShadowMatrix(this,mShadow,gb_RenderDevice3D->GetShadowMatViewProj());
	__super::Select();
}

void VSGeforceFXTileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mShadow);

	VAR_INDEX(VERTEX_LIGHT);
	shader->Select(VERTEX_LIGHT,Option_TileMapTypeNormal?0:1);
}

void VSGeforceFXTileMapScene::RestoreShader()
{
	LoadShader("Minimal\\tile_map_scene_shadowFX.vsl");
}

PSGeforceFXTileMapScene::PSGeforceFXTileMapScene()
{
}

void PSGeforceFXTileMapScene::SetColor(const sColor4f& color)
{
	if(Option_TileMapTypeNormal)
	{
		SetVector(light_color,(D3DXVECTOR4*)&color);
	}
}

void PSGeforceFXTileMapScene::Select(int i,VSTileMapScene* vs,float res)
{
	SetFog();
	Vect3f l;
	gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
	SetVector(inv_light_dir,&D3DXVECTOR4(-l.x,-l.y,-l.z,0));

	SetVector(fx_offset,&D3DXVECTOR4(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));
	
	__super::Select(i,vs,res);
}

void PSGeforceFXTileMapScene::RestoreShader()
{
	LoadShader("Minimal\\tile_map_scene_shadowFX.psl");
	shader->BeginStaticSelect();
	shader->StaticSelect("c2x2",Option_ShadowMapSelf4x4?0:1);
	shader->StaticSelect("VERTEX_LIGHT",Option_TileMapTypeNormal?0:1);
	shader->StaticSelect("DETAIL_TEXTURE",Option_DetailTexture?1:0);
	shader->EndStaticSelect();
}

void PSGeforceFXTileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(light_color);
	VAR_HANDLE(inv_light_dir);
	VAR_HANDLE(vShade);
	VAR_HANDLE(fx_offset);
}
