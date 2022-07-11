void VSCloudShadow::Select(const MatXf& world)
{
	D3DXMATRIX mat;
	cD3DRender_SetMatrix(mat,world);
	D3DXMatrixMultiply(&mat,&mat,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	SetMatrix(mWVP,&mat);
	__super::Select();
}

void VSCloudShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mWVP);
}
void VSCloudShadow::RestoreShader()
{
	LoadShader("NoMaterial\\CloudShadow.vsl");
}


void PSCloudShadow::Select(const sColor4f& ctfactor)
{
	SetVector(tfactor,(D3DXVECTOR4*)&ctfactor);

	sColor4f ctfactorm05(0.5f,0.5f,0.5f,0.5f);
	ctfactorm05-=ctfactor*0.75f;
	SetVector(tfactorm05,(D3DXVECTOR4*)&ctfactorm05);
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSCloudShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(tfactor);
	VAR_HANDLE(tfactorm05);
}

void PSCloudShadow::RestoreShader()
{
	LoadShader("NoMaterial\\CloudShadow.psl");
}

