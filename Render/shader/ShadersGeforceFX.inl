////////////////////////////GeforceFX///////////////////////////////////

void ShaderSceneTileMapGeforceFX::Select()
{
	setMatrixVS(mShadow, gb_RenderDevice3D->shadowMatViewProj()*gb_RenderDevice3D->shadowMatBias());

	SetFog();
	Vect3f l;
	gb_RenderDevice3D->camera()->GetLighting(l);
	setVectorPS(inv_light_dir, Vect4f(-l.x,-l.y,-l.z,0));
	setVectorPS(fx_offset, Vect4f(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));

	shaderPS_->Select(DETAIL_TEXTURE, Option_DetailTexture ? 1 : 0);

	__super::Select();
}

void ShaderSceneTileMapGeforceFX::GetHandle()
{
	__super::GetHandle();

	VAR_HANDLE_VS(mShadow);

	VAR_HANDLE_PS(inv_light_dir);
	VAR_HANDLE_PS(vShade);
	VAR_HANDLE_PS(fx_offset);
	VAR_INDEX_PS(DETAIL_TEXTURE);
}

void ShaderSceneTileMapGeforceFX::RestoreShader()
{
	LoadShaderVS("Minimal\\tile_map_scene_shadowFX.vsl");
	LoadShaderPS("Minimal\\tile_map_scene_shadowFX.psl");
}


