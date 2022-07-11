void VSCloudShadow::Select(const MatXf& world)
{
	setMatrixVS(mWVP, Mat4f(world)*gb_RenderDevice3D->camera()->matViewProj);

	__super::Select();
}

void VSCloudShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mWVP);
}

void VSCloudShadow::RestoreShader()
{
	LoadShaderVS("NoMaterial\\CloudShadow.vsl");
}

void PSCloudShadow::Select(const Color4f& ctfactor)
{
	setVectorPS(tfactor, ctfactor);

	Color4f ctfactorm05(0.5f,0.5f,0.5f,0.5f);
	ctfactorm05-=ctfactor*0.75f;
	setVectorPS(tfactorm05, ctfactorm05);
	
	__super::Select();
}

void PSCloudShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(tfactor);
	VAR_HANDLE_PS(tfactorm05);
}

void PSCloudShadow::RestoreShader()
{
	LoadShaderPS("NoMaterial\\CloudShadow.psl");
}

