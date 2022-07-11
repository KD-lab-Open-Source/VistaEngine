
void VSSkinFur::SetFurDistance(float dist)
{
	Vect4f f(dist,0,0,0);
	setVectorVS(vFurDistance, f);
}

void VSSkinFur::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(vFurDistance);
}

void VSSkinFur::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_light_fur.vsl");
}
