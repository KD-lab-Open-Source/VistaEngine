//////////////////////////////Radeon 9700///////////////////////

void VS9700TileMapScene::RestoreShader()
{
	LoadShader("Minimal\\tile_map_scene_shadowFX.vsl");//Потом поменять на такой же но без фога
}

void VS9700TileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mShadow);

	INDEX_HANDLE VERTEX_LIGHT;
	VAR_INDEX(VERTEX_LIGHT);
	shader->Select(VERTEX_LIGHT,Option_TileMapTypeNormal?0:1);
}

void VS9700TileMapScene::Select()
{
	SetShadowMatrix(this,mShadow,gb_RenderDevice3D->GetShadowMatViewProj());
	__super::Select();
}

void PS9700TileMapScene4x4::RestoreShader()
{
	LoadShader("Minimal\\tile_map_scene_shadow9700.psl");
	shader->BeginStaticSelect();
	shader->StaticSelect("c2x2",Option_ShadowMapSelf4x4?0:1);
	shader->StaticSelect("VERTEX_LIGHT",Option_TileMapTypeNormal?0:1);
	shader->StaticSelect("DETAIL_TEXTURE",Option_DetailTexture?1:0);
	shader->EndStaticSelect();
}

void PS9700TileMapScene4x4::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(light_color);
	VAR_HANDLE(inv_light_dir);
	VAR_HANDLE(vShade);
}

void PS9700TileMapScene4x4::SetColor(const sColor4f& color)
{
	if(Option_TileMapTypeNormal)
	{
		SetVector(light_color,(D3DXVECTOR4*)&color);
	}
}

void PS9700TileMapScene4x4::Select(int i,VSTileMapScene* vs,float res)
{
	SetFog();
	Vect3f l;
	gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
	SetVector(inv_light_dir,&D3DXVECTOR4(-l.x,-l.y,-l.z,0));
	__super::Select(i,vs,res);
}

