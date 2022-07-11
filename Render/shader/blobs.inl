void PSBlobsShader::Select(cTexture* pTexture, const Vect4f& def_color, const Vect4f& spec_color, float phase)
{
	gb_RenderDevice3D->SetTexture(0,pTexture);

	Vect4f size(1.0f/pTexture->GetWidth(),1.0f/pTexture->GetHeight(),0,0);
	Vect4f ph(phase,0,0,0);
		
	setVectorPS(pixel_size, size);
	setVectorPS(blobs_color, def_color);
	setVectorPS(specular_color, spec_color);
	setVectorPS(fade_phase, ph);

	__super::Select();
}

void PSBlobsShader::GetHandle()
{
	VAR_HANDLE_PS(pixel_size);
	VAR_HANDLE_PS(blobs_color);
	VAR_HANDLE_PS(specular_color);
	VAR_HANDLE_PS(fade_phase);
}

void PSBlobsShader::RestoreShader()
{
	LoadShaderPS("PostProcessing\\blobs.psl");
}
