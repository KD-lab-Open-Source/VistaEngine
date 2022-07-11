void SetZBufferMat(vsSceneShader* shader,SHADER_HANDLE& mZBuffer,const Mat4f& mat);
VSWater::VSWater()
{
	technique=0;
	enableZBuffer = false;
}
void VSWater::EnableZBuffer(bool enable)
{
	if(FLOAT_ZBUFFER.is())
		shaderVS_->Select(FLOAT_ZBUFFER,enable?1:0);
	enableZBuffer = enable;
}

void VSWater::SetSpeed(Vect2f scale,Vect2f offset)
{
	Vect4f s(scale.x,scale.y,offset.x,offset.y);
	setVectorVS(uvScaleOffset, s);
}

void VSWater::SetSpeed1(Vect2f scale,Vect2f offset)
{
	Vect4f s(scale.x,scale.y,offset.x,offset.y);
	setVectorVS(uvScaleOffset1,s);
}

void VSWater::SetSpeedSky(Vect2f scale,Vect2f offset)
{
	Vect4f s(scale.x,scale.y,offset.x,offset.y);
	setVectorVS(uvScaleOffsetSky, s);
}

void VSWater::Select()
{
	SetFog();
	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);
	setVectorVS(vCameraPos, Vect4f(gb_RenderDevice3D->camera()->GetPos(), 0));
	//cVertexShader::Select();
	if(enableZBuffer)
		SetZBufferMat(this,mZBuffer,gb_RenderDevice3D->GetFloatZBufferMatViewProj());
	
	__super::Select();
}

void VSWater::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mZBuffer);
	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(uvScaleOffset);
	VAR_HANDLE_VS(uvScaleOffset1);
	VAR_HANDLE_VS(uvScaleOffsetSky);
	VAR_HANDLE_VS(vCameraPos);
	VAR_HANDLE_VS(vMirrorVP);
	VAR_INDEX_VS(FLOAT_ZBUFFER);
}

void VSWater::RestoreShader()
{
	if(technique==4)
	{
		LoadShaderVS("Water\\water_linear.vsl");
	}else
	if(technique==2)
	{
		LoadShaderVS("Water\\water_cube.vsl");
	}else
	if(technique==1)
	{
		LoadShaderVS("Water\\water_easy.vsl");
	}else
	{
		LoadShaderVS("Water\\water_easy.vsl");
//		xassert(0);
	}
}

void VSWater::SetMirrorMatrix(Camera* mirror)
{
	if(mirror==0)
	{
		xassert(0);
		return;
	}

	int map_size = mirror->GetRenderTarget() ? mirror->GetRenderTarget()->GetWidth() : 1024;
	float fOffsetX = 0.5f + (0.5f / map_size);
	float fOffsetY = 0.5f + (0.5f / map_size);
	float fBias    = 0;
	Mat4f matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
		                  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,     1,     0.0f,
						  fOffsetX, fOffsetY, fBias, 1.0f );

	setMatrixVS(vMirrorVP, mirror->matViewProj*matTexAdj);
}

PSWater::PSWater()
{
	technique=0;
	enableZBuffer = false;
	flashColor_ = Color4f::WHITE;
}

void PSWater::Select()
{
	SetFog();

	if(technique==4)
	{
		setVectorPS(vCameraPos, Vect4f(gb_RenderDevice3D->camera()->GetPos(), 0));
		Vect3f l;
		gb_RenderDevice3D->camera()->GetLighting(l);
		setVectorPS(vLightDirection, Vect4f(l, 0));
		setVectorPS(vLightColor, flashColor_);
	}

	if(enableZBuffer)
		gb_RenderDevice3D->SetTexture(4,gb_RenderDevice3D->GetFloatMap());
	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMap());

	__super::Select();
}
void PSWater::EnableZBuffer(bool enable)
{
	if(FLOAT_ZBUFFER.is())
		shaderPS_->Select(FLOAT_ZBUFFER,enable?1:0);
	enableZBuffer = enable;
}

void PSWater::RestoreShader()
{
	if(technique==4)
	{
		LoadShaderPS("Water\\water_linear.psl");
	}else
	if(technique==2)
	{
		LoadShaderPS("Water\\water_cube.psl");
	}else
	if(technique==1)
	{
		LoadShaderPS("Water\\water_easy.psl");
	}else
	{
		LoadShaderPS("Water\\water_easy.psl");
	}
	
}

void PSWater::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(vCameraPos);
	VAR_HANDLE_PS(vLightDirection);
	VAR_HANDLE_PS(vLightColor);
	VAR_HANDLE_PS(vReflectionColor);
	VAR_HANDLE_PS(vPS11Color);
	VAR_HANDLE_PS(fBrightnes);
	VAR_INDEX_PS(FLOAT_ZBUFFER);
}

void PSWater::SetTechnique(int t)
{
	technique=t;
	Restore();
}

void VSWater::SetTechnique(int t)
{
	technique=t;
	Restore();
}

void PSWater::SetPS11Color(const Color4f& color)
{
	setVectorPS(vPS11Color, color);
}

void PSWater::SetReflectionColor(const Color4f& color)
{
	float inva=1-color.a;
	Color4f c(color.r*inva,color.g*inva,color.b*inva,color.a);
	setVectorPS(vReflectionColor, c);
}
void PSWater::SetReflectionBrightnes(const float brightnes)
{
	setVectorPS(fBrightnes, Vect4f(brightnes,brightnes,brightnes,brightnes));
}

IDirect3DVolumeTexture9* CreateVolumeRand(int size)
{
	bool mipmap=false;
	IDirect3DVolumeTexture9* tex=0;
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateVolumeTexture(size,size,size,mipmap?0:1,
		0,D3DFMT_L8,D3DPOOL_MANAGED,&tex,0));
	if(!tex)
	{
		xassert(0);
		return 0;
	}

	D3DLOCKED_BOX lock_box;
	HRESULT hr=tex->LockBox(0,&lock_box,0,0);
	if(FAILED(hr))
	{
		xassert(0);
		RELEASE(tex);
		return 0;
	}

	for(int z=0;z<size;z++)
	for(int y=0;y<size;y++)
	{
		BYTE* p=lock_box.SlicePitch*z+lock_box.RowPitch*y+(BYTE*)lock_box.pBits;
		for(int x=0;x<size;x++,p++)
		{
			*p=graphRnd()&255;
		}
	}

	tex->UnlockBox(0);

	if(mipmap)
	 tex->GenerateMipSubLevels();
	return tex;
}
/////////////////////////////////WaterLava///////////////////////
ShaderSceneWaterLava::ShaderSceneWaterLava(bool convertZ)
{
	convertZ_ = convertZ;
	time=0;

	textureScaleValue_.set(0.002f, 0.003f, 0, 0);
	
	pRandomVolume=(IDirect3DBaseTexture9*)CreateVolumeRand(64);
}

void ShaderSceneWaterLava::SetTime(double time_)
{
	time=time_;//Тут бы невредно время обрезать, чтобы во float нормально интерполировалось.
	float ftime=(float)time;
	gb_RenderDevice3D->SetPixelShaderConstant(0, Vect4f(ftime,ftime,ftime,ftime));//Криво это.
	setVectorVS(fTime, Vect4f(ftime,ftime,ftime,ftime));
}

void ShaderSceneWaterLava::Select()
{
	SetFog();
	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);

	shaderVS_->Select(CONVERT_Z, convertZ_ ? 1 : 0);

	setVectorVS(textureScale, textureScaleValue_);
	
	SetFog();
	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetTextureBase(0,pRandomVolume);
	if(!gb_RenderDevice3D->IsPS20())
		gb_RenderDevice3D->SetTextureBase(2,pRandomVolume);

	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMap());
	
	__super::Select();
}

void ShaderSceneWaterLava::RestoreShader()
{
	if(gb_RenderDevice3D->IsPS20())
	{
		LoadShaderVS("Water\\water_lava.vsl");
		LoadShaderPS("Water\\water_lava.psl");
	}
	else{
		LoadShaderVS("Water\\water_lava_ps11.vsl");
		LoadShaderPS("Water\\water_lava_ps11.psl");
	}
}

void ShaderSceneWaterLava::GetHandle()
{
	__super::GetHandle();

	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(fTime);
	VAR_HANDLE_VS(textureScale);
	VAR_INDEX_VS(CONVERT_Z);

	VAR_HANDLE_PS(vLavaColor);
	VAR_HANDLE_PS(vLavaColorAmbient);
}

ShaderSceneWaterLava::~ShaderSceneWaterLava()
{
	RELEASE(pRandomVolume);
}

void ShaderSceneWaterLava::SetColors(const Color4f& lava,const Color4f& ambient)
{
	setVectorPS(vLavaColor, lava);
	setVectorPS(vLavaColorAmbient, ambient);
}

///////////////////////////WaterIce////////////////////////
ShaderSceneWaterIce::ShaderSceneWaterIce()
{
	linear = false;

	sampler_border = sampler_wrap_anisotropic;
	sampler_border.addressu = sampler_border.addressv = sampler_border.addressw = DX_TADDRESS_BORDER;
}

void ShaderSceneWaterIce::SetMirrorMatrix(Camera* mirror)
{
	if(mirror==0){
		linear=false;
		return;
	}
	linear = true;

	int map_size = mirror->GetRenderTarget() ? mirror->GetRenderTarget()->GetWidth() : 1024;
	float fOffsetX = 0.5f + (0.5f / map_size);
	float fOffsetY = 0.5f + (0.5f / map_size);
	float fBias    = 0;
	Mat4f matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
		                  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,     1,     0.0f,
						  fOffsetX, fOffsetY, fBias, 1.0f );

	setMatrixVS(vMirrorVP, mirror->matViewProj*matTexAdj);
}

void ShaderSceneWaterIce::Select()
{
	shaderVS_->Select(PS11,gb_RenderDevice3D->IsPS20()?0:1);
	SetFog();
	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);

	setVectorVS(uvScaleOffset, gb_RenderDevice3D->tilemap_inv_size);

	shaderVS_->Select(MIRROR_LINEAR,linear?1:0);
	setVectorVS(vCameraPos, Vect4f(gb_RenderDevice3D->camera()->GetPos(), 0));

	setVectorVS(fScaleBumpSnow, Vect4f(0.01f,0.003f,0.002f,0));
	
	shaderPS_->Select(PS11,gb_RenderDevice3D->IsPS20()?0:1);
	SetFog();
	gb_RenderDevice3D->SetSamplerData( 3,sampler_wrap_linear);
	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMap());
	shaderPS_->Select(MIRROR_LINEAR,linear?1:0);

	__super::Select();
}

void ShaderSceneWaterIce::RestoreShader()
{
	LoadShaderVS("Water\\water_ice.vsl");
	LoadShaderPS("Water\\water_ice.psl");
}

void ShaderSceneWaterIce::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(uvScaleOffset);
	VAR_HANDLE_VS(vCameraPos);
	VAR_HANDLE_VS(vMirrorVP);
	VAR_HANDLE_VS(fScaleBumpSnow);
	VAR_INDEX_VS(MIRROR_LINEAR);
	VAR_INDEX_VS(PS11);
	VAR_INDEX_VS(USE_ALPHA);

	VAR_HANDLE_PS(vSnowColor);
	VAR_INDEX_PSX(MIRROR_LINEAR_PS, "MIRROR_LINEAR");
	VAR_INDEX_PSX(PS11_PS, "PS11");
	VAR_INDEX_PSX(USE_ALPHA_PS, "USE_ALPHA");
}

void ShaderSceneWaterIce::SetSnowColor(const Color4f& color)
{
	Color4f colorClamped(
		clamp(color.r,0.0f,1.0f),
		clamp(color.g,0.0f,1.0f),
		clamp(color.b,0.0f,1.0f),
		color.a);
	setVectorPS(vSnowColor, colorClamped);
}

void ShaderSceneWaterIce::beginDraw(cTexture* pTexture, cTexture* pBump, cTexture* pAlpha, cTexture* pCleft, int alphaRef, int borderColor)
{
	cD3DRender* rd = gb_RenderDevice3D;
 
	Camera* camera = rd->camera();
	cScene* scene = camera->scene();
	Camera* pReflection = camera->FindChildCamera(ATTRCAMERA_REFLECTION);
	SetMirrorMatrix(pReflection);
	if(pReflection)
		rd->SetTexture(4, pReflection->GetRenderTarget());
	else
		rd->SetTexture(4, scene->GetSkyCubemap());

	alpharef_ = rd->GetRenderState(D3DRS_ALPHAREF);
	zwrite_ = rd->GetRenderState(D3DRS_ZWRITEENABLE);
	if(pAlpha){
		rd->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		rd->SetNoMaterial(ALPHA_TEST, MatXf::ID, 0, pAlpha);
	}
	else
		rd->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	rd->SetRenderState(D3DRS_ALPHAREF, alphaRef);

	rd->SetSamplerData(0, sampler_clamp_anisotropic);
	rd->SetSamplerData(1, sampler_wrap_anisotropic);
	rd->SetSamplerData(2, sampler_wrap_anisotropic);

	SetSnowColor(scene->GetPlainLitColor());
	shaderVS_->Select(USE_ALPHA, pAlpha ? 1 : 0);
	shaderPS_->Select(USE_ALPHA_PS, pAlpha ? 1 : 0);
	Select();
	rd->SetTexture(1, pTexture);
	rd->SetTexture(2, pBump);

	rd->SetSamplerData(3, sampler_clamp_linear);
	rd->SetSamplerData(5, sampler_wrap_linear);
	rd->SetTexture(5, pCleft);
	
	sampler_border.bordercolor = borderColor;
	rd->SetSamplerData(0, sampler_border);
}

void ShaderSceneWaterIce::endDraw()
{
	cD3DRender* rd = gb_RenderDevice3D;
	rd->SetRenderState(D3DRS_ZWRITEENABLE, zwrite_);
	rd->SetRenderState(D3DRS_ALPHAREF, alpharef_);
	rd->SetVertexShader(0);
	rd->SetPixelShader(0);

	for(int i=0;i<8;i++)
		rd->SetTextureBase(i,0);
}
