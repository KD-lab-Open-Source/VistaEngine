////////////////////////////Minimal///////////////////////////////////

void VSMinimalTileMapScene::RestoreShader()
{
	if(Option_TileMapTypeNormal)
	{
		LoadShader("Minimal\\tile_map_scene_pixel.vsl");
	}else
	{
		LoadShader("Minimal\\tile_map_scene_vertex.vsl");
	}
}

void VSMinimalTileMapScene::GetHandle()
{
	__super::GetHandle();
}

void PSMinimalTileMapScene::Select(int i,VSTileMapScene* vs,float res)
{
	SetFog();
	if(Option_TileMapTypeNormal)
	{
		Vect3f l;
		gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
		SetVector(inv_light_dir,&D3DXVECTOR4(-l.x,-l.y,-l.z,0));
	}
	__super::Select(i,vs,res);
}

void PSMinimalTileMapScene::SetColor(const sColor4f& scolor)
{
	if(Option_TileMapTypeNormal)
	{
		SetVector(light_color,(D3DXVECTOR4*)&scolor);
	}
}

void PSMinimalTileMapScene::RestoreShader()
{
	if(Option_DetailTexture && gb_RenderDevice3D->IsPS20())
	{
		LoadShader("Minimal\\tile_map_scened.psl");
	}else
	{
		if(Option_TileMapTypeNormal && gb_RenderDevice3D->IsPS20())
		{
			LoadShader("Minimal\\tile_map_scenep.psl");
		}else
		{
			LoadShader("Minimal\\tile_map_scene.psl");
		}
	}
}

void PSMinimalTileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(VERTEX_LIGHT);

	if(VERTEX_LIGHT.is())
		shader->Select(VERTEX_LIGHT,Option_TileMapTypeNormal?0:1);
	VAR_HANDLE(light_color);
	VAR_HANDLE(inv_light_dir);
}
