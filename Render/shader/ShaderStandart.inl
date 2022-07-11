
void SetZBufferMat(vsSceneShader* shader,SHADER_HANDLE& mZBuffer,const D3DXMATRIX& matZ)
{
	D3DXMATRIX mat;
	Vect2f offset = gb_RenderDevice3D->GetInvFloatZBufferSize();
	float fOffsetX = 0.5f + (0.5f *offset.x);
	float fOffsetY = 0.5f + (0.5f *offset.y);
	D3DXMATRIX matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
						  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,	  1.0f,  0.0f,
						  fOffsetX, fOffsetY, 0.0f,  1.0f );
	D3DXMatrixMultiply(&mat,&matZ,&matTexAdj);
	shader->SetMatrix(mZBuffer,&mat);
}



void VSStandart::Select(const MatXf& world)
{
	SetFog();
	D3DXMATRIX mat;
	cD3DRender_SetMatrix(mat,world);
	SetMatrix(mWorld,&mat);
	D3DXMatrixMultiply(&mat,&mat,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	SetMatrix(mWVP,&mat);
	if (useFloatZBuffer)
		SetZBufferMat(this,mZBuffer,gb_RenderDevice3D->GetFloatZBufferMatViewProj());
	D3DXVECTOR4 reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	SetVector(vReflectionMul,&reflection_mul);
	__super::Select();
}
void VSStandart::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mWVP);
	VAR_HANDLE(mWorld);
	VAR_HANDLE(mZBuffer);
	VAR_HANDLE(vReflectionMul);
	VAR_INDEX(FIX_FOG_ADD_BLEND);
	VAR_INDEX(COLOR_OPERATION);
	VAR_INDEX(FLOAT_ZBUFFER);
	VAR_INDEX(ZREFLECTION);
}
void VSStandart::RestoreShader()
{
	LoadShader("NoMaterial\\standart.vsl");
}
void VSStandart::SetFixFogAddBlend(bool fix)
{
	shader->Select(FIX_FOG_ADD_BLEND,fix?1:0);
}

void PSSolidColor::Select()
{
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSSolidColor::RestoreShader()
{
	LoadShader("NoMaterial\\SolidColor.psl");
}
void PSStandart::Select()
{
	SetFog();
	gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_linear);
	gb_RenderDevice3D->SetTexture(2,gb_RenderDevice3D->dtAdvance->GetLightMap());
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSStandart::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(COLOR_OPERATION);
	VAR_INDEX(FLOAT_ZBUFFER);
	VAR_INDEX(ZREFLECTION);
}

void PSStandart::RestoreShader()
{
	LoadShader("NoMaterial\\standart.psl");
}

void PSStandart::SetColorOperation(int op)
{
	shader->Select(COLOR_OPERATION,op);
}
void PSStandart::EnableFloatZBuffer(bool enable)
{
	shader->Select(FLOAT_ZBUFFER,enable?1:0);
}
void VSStandart::SetColorOperation(int op)
{
	shader->Select(COLOR_OPERATION,op?1:0);
}
void VSStandart::EnableFloatZBuffer(bool enable)
{
	shader->Select(FLOAT_ZBUFFER,enable?1:0);
	useFloatZBuffer = enable;
}
