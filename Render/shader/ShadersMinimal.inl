////////////////////////////Minimal///////////////////////////////////

void ShaderSceneTileMapMinimal::RestoreShader()
{
	if(Option_tileMapVertexLight)
		LoadShaderVS("Minimal\\tile_map_scene_vertex.vsl");
	else
		LoadShaderVS("Minimal\\tile_map_scene_pixel.vsl");

	if(Option_DetailTexture && gb_RenderDevice3D->IsPS20())
		LoadShaderPS("Minimal\\tile_map_scened.psl");
	else{
		if(Option_tileMapVertexLight)
			LoadShaderPS("Minimal\\tile_map_scene.psl");
		else
			LoadShaderPS("Minimal\\tile_map_scenep.psl");
	}
}

void ShaderSceneTileMapMinimal::GetHandle()
{
	__super::GetHandle();

	VAR_HANDLE_PS(inv_light_dir);
}

void ShaderSceneTileMapMinimal::Select()
{
	SetFog();
	if(!Option_tileMapVertexLight){
		Vect3f l;
		gb_RenderDevice3D->camera()->GetLighting(l);
		setVectorPS(inv_light_dir, Vect4f(-l.x,-l.y,-l.z,0));
	}
	__super::Select();
}


