
void SetZBufferMat(vsSceneShader* shader,SHADER_HANDLE& mZBuffer,const Mat4f& matZ)
{
	Vect2f offset = gb_RenderDevice3D->GetInvFloatZBufferSize();
	float fOffsetX = 0.5f + (0.5f *offset.x);
	float fOffsetY = 0.5f + (0.5f *offset.y);
	Mat4f matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
						  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,	  1.0f,  0.0f,
						  fOffsetX, fOffsetY, 0.0f,  1.0f );
	shader->setMatrixVS(mZBuffer, matZ*matTexAdj);
}



void VSStandart::Select(const MatXf& world)
{
	SetFog();
	setMatrixVS(mWorld, Mat4f(world));
	setMatrixVS(mWVP, Mat4f(world)*gb_RenderDevice3D->camera()->matViewProj);
	if (useFloatZBuffer)
		SetZBufferMat(this,mZBuffer,gb_RenderDevice3D->GetFloatZBufferMatViewProj());
	Vect4f reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	setVectorVS(vReflectionMul, reflection_mul);
	__super::Select();
}
void VSStandart::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mWVP);
	VAR_HANDLE_VS(mWorld);
	VAR_HANDLE_VS(mZBuffer);
	VAR_HANDLE_VS(vReflectionMul);
	VAR_INDEX_VS(FIX_FOG_ADD_BLEND);
	VAR_INDEX_VS(COLOR_OPERATION);
	VAR_INDEX_VS(FLOAT_ZBUFFER);
	VAR_INDEX_VS(ZREFLECTION);
}
void VSStandart::RestoreShader()
{
	LoadShaderVS("NoMaterial\\standart.vsl");
}
void VSStandart::SetFixFogAddBlend(bool fix)
{
	shaderVS_->Select(FIX_FOG_ADD_BLEND,fix?1:0);
}

void PSSolidColor::RestoreShader()
{
	LoadShaderPS("NoMaterial\\SolidColor.psl");
}

void PSStandart::Select()
{
	SetFog();
	gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_linear);
	gb_RenderDevice3D->SetTexture(2,gb_RenderDevice3D->GetLightMap());

	__super::Select();
}

void PSStandart::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_PS(COLOR_OPERATION);
	VAR_INDEX_PS(FLOAT_ZBUFFER);
	VAR_INDEX_PS(ZREFLECTION);
}

void PSStandart::RestoreShader()
{
	LoadShaderPS("NoMaterial\\standart.psl");
}

void PSStandart::SetColorOperation(int op)
{
	shaderPS_->Select(COLOR_OPERATION,op);
}
void PSStandart::EnableFloatZBuffer(bool enable)
{
	shaderPS_->Select(FLOAT_ZBUFFER,enable?1:0);
}
void VSStandart::SetColorOperation(int op)
{
	shaderVS_->Select(COLOR_OPERATION,op?1:0);
}
void VSStandart::EnableFloatZBuffer(bool enable)
{
	shaderVS_->Select(FLOAT_ZBUFFER,enable?1:0);
	useFloatZBuffer = enable;
}
