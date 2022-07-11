void PSBlobsShader::Select(cTexture* pTexture, const D3DXVECTOR4& def_color, const D3DXVECTOR4& spec_color, float phase)
{
	gb_RenderDevice3D->SetTexture(0,pTexture);

	D3DXVECTOR4 pSize(1.0f/pTexture->GetWidth(),1.0f/pTexture->GetHeight(),0,0);
	D3DXVECTOR4 ph(phase,0,0,0);
		
	SetVector(pixel_size, &pSize);
	SetVector(blobs_color, &def_color);
	SetVector(specular_color, &spec_color);
	SetVector(fade_phase, &ph);

	__super::Select();
}

void PSBlobsShader::GetHandle()
{
	VAR_HANDLE(pixel_size);
	VAR_HANDLE(blobs_color);
	VAR_HANDLE(specular_color);
	VAR_HANDLE(fade_phase);
}

void PSBlobsShader::RestoreShader()
{
	LoadShader("PostProcessing\\blobs.psl");
}
