//////////////////////////////Radeon 9700///////////////////////

void ShaderSceneTileMap9700::RestoreShader()
{
	LoadShaderVS("Minimal\\tile_map_scene_shadowFX.vsl");//Потом поменять на такой же но без фога
	LoadShaderPS("Minimal\\tile_map_scene_shadow9700.psl");
}

void ShaderSceneTileMap9700::GetHandle()
{
	__super::GetHandle();

	VAR_HANDLE_VS(mShadow);

	VAR_HANDLE_PS(inv_light_dir);
	VAR_HANDLE_PS(vShade);
	VAR_INDEX_PS(DETAIL_TEXTURE);
}

void ShaderSceneTileMap9700::Select()
{
	setMatrixVS(mShadow, gb_RenderDevice3D->shadowMatViewProj()*gb_RenderDevice3D->shadowMatBias());
	SetFog();
	Vect3f l;
	gb_RenderDevice3D->camera()->GetLighting(l);
	setVectorPS(inv_light_dir, Vect4f(-l.x,-l.y,-l.z,0));

	shaderPS_->Select(DETAIL_TEXTURE, Option_DetailTexture ? 1 : 0);

    __super::Select();
}



