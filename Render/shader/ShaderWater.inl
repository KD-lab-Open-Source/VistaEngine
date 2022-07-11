void SetZBufferMat(vsSceneShader* shader,SHADER_HANDLE& mZBuffer,const D3DXMATRIX& mat);
VSWater::VSWater()
{
	technique=0;
	enableZBuffer = false;
}
void VSWater::EnableZBuffer(bool enable)
{
	if(FLOAT_ZBUFFER.is())
		shader->Select(FLOAT_ZBUFFER,enable?1:0);
	enableZBuffer = enable;
}

void VSWater::SetSpeed(Vect2f scale,Vect2f offset)
{
	D3DXVECTOR4 s(scale.x,scale.y,offset.x,offset.y);
	SetVector(uvScaleOffset,&s);
}

void VSWater::SetSpeed1(Vect2f scale,Vect2f offset)
{
	D3DXVECTOR4 s(scale.x,scale.y,offset.x,offset.y);
	SetVector(uvScaleOffset1,&s);
}

void VSWater::SetSpeedSky(Vect2f scale,Vect2f offset)
{
	D3DXVECTOR4 s(scale.x,scale.y,offset.x,offset.y);
	SetVector(uvScaleOffsetSky,&s);
}
void VSWater::Select()
{
	SetFog();
	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	Vect3f p=gb_RenderDevice3D->GetDrawNode()->GetPos();
	SetVector(vCameraPos,&D3DXVECTOR4(p.x,p.y,p.z,0));
	//cVertexShader::Select();
	if(enableZBuffer)
		SetZBufferMat(this,mZBuffer,gb_RenderDevice3D->GetFloatZBufferMatViewProj());
	gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
}

void VSWater::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mZBuffer);
	VAR_HANDLE(mVP);
	VAR_HANDLE(uvScaleOffset);
	VAR_HANDLE(uvScaleOffset1);
	VAR_HANDLE(uvScaleOffsetSky);
	VAR_HANDLE(vCameraPos);
	VAR_HANDLE(vMirrorVP);
	VAR_INDEX(FLOAT_ZBUFFER);
}

void VSWater::RestoreShader()
{
	if(technique==4)
	{
		LoadShader("Water\\water_linear.vsl");
	}else
	if(technique==2)
	{
		LoadShader("Water\\water_cube.vsl");
	}else
	if(technique==1)
	{
		LoadShader("Water\\water_easy.vsl");
	}else
	{
		LoadShader("Water\\water_easy.vsl");
//		xassert(0);
	}
}

void VSWater::SetMirrorMatrix(cCamera* mirror)
{
	if(mirror==NULL)
	{
		xassert(0);
		return;
	}
	CMatrix& matViewProj=mirror->matViewProj;
	int map_size=mirror->GetRenderTarget()?mirror->GetRenderTarget()->GetWidth():1024;
	D3DXMATRIX mat;
	float fOffsetX = 0.5f + (0.5f / map_size);
	float fOffsetY = 0.5f + (0.5f / map_size);

	float fBias    = 0;

	D3DXMATRIX matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
		                  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,     1,     0.0f,
						  fOffsetX, fOffsetY, fBias, 1.0f );
/*
	float f=2;
	D3DXMATRIX mat_div( f,     0.0f,     0.0f,  0.0f,
		                  0.0f,   f,     0.0f,  0.0f,
						  0.0f,   0.0f,     1,     0.0f,
						  0.0f, 0.0f, 0.0f, 1.0f );
	D3DXMATRIX mat_tmp;
	D3DXMatrixMultiply(&mat_tmp, matViewProj, &mat_div);
	D3DXMatrixMultiply(&mat, matViewProj, &mat_tmp);
/*/
	D3DXMatrixMultiply(&mat, matViewProj, &matTexAdj);
/**/
//	D3DXMatrixMultiply(&mat, mirror->matView, &matTexAdj);

	SetMatrix(vMirrorVP,&mat);
}

PSWater::PSWater()
{
	technique=0;
	enableZBuffer = false;

}

void PSWater::Select()
{
	SetFog();

	if(technique==4)
	{
		Vect3f p=gb_RenderDevice3D->GetDrawNode()->GetPos();
		SetVector(vCameraPos,&D3DXVECTOR4(p.x,p.y,p.z,0));
		Vect3f l;
		gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
		SetVector(vLightDirection,&D3DXVECTOR4(l.x,l.y,l.z,0));

		const sColor4f& c=gb_RenderDevice3D->GetDrawNode()->GetScene()->GetSunDiffuse();
		SetVector(vLightColor,(D3DXVECTOR4*)&c);
	}

	if(enableZBuffer)
		gb_RenderDevice3D->SetTexture(4,gb_RenderDevice3D->dtFixed->GetFloatMap());
	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->dtAdvance->GetLightMap());
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
//	__super::Select();
}
void PSWater::EnableZBuffer(bool enable)
{
	if(FLOAT_ZBUFFER.is())
		shader->Select(FLOAT_ZBUFFER,enable?1:0);
	enableZBuffer = enable;
}

void PSWater::RestoreShader()
{
	if(technique==4)
	{
		LoadShader("Water\\water_linear.psl");
	}else
	if(technique==2)
	{
		LoadShader("Water\\water_cube.psl");
	}else
	if(technique==1)
	{
		LoadShader("Water\\water_easy.psl");
	}else
	{
		LoadShader("Water\\water_easy.psl");
	}
	
}

void PSWater::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vCameraPos);
	VAR_HANDLE(vLightDirection);
	VAR_HANDLE(vLightColor);
	VAR_HANDLE(vReflectionColor);
	VAR_HANDLE(vPS11Color);
	VAR_HANDLE(fBrightnes);
	VAR_INDEX(FLOAT_ZBUFFER);
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

void PSWater::SetPS11Color(const sColor4f& color)
{
	SetVector(vPS11Color,(D3DXVECTOR4*)&color);
}

void PSWater::SetReflectionColor(const sColor4f& color)
{
	float inva=1-color.a;
	sColor4f c(color.r*inva,color.g*inva,color.b*inva,color.a);
	SetVector(vReflectionColor,(D3DXVECTOR4*)&c);
}
void PSWater::SetReflectionBrightnes(const float brightnes)
{
	SetVector(fBrightnes,&D3DXVECTOR4(brightnes,brightnes,brightnes,brightnes));
}

IDirect3DVolumeTexture9* CreateVolumeRand(int size)
{
	bool mipmap=false;
	IDirect3DVolumeTexture9* tex=NULL;
	RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateVolumeTexture(size,size,size,mipmap?0:1,
		0,D3DFMT_L8,D3DPOOL_MANAGED,&tex,NULL));
	if(!tex)
	{
		xassert(0);
		return NULL;
	}

	D3DLOCKED_BOX lock_box;
	HRESULT hr=tex->LockBox(0,&lock_box,NULL,0);
	if(FAILED(hr))
	{
		xassert(0);
		RELEASE(tex);
		return NULL;
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
VSWaterLava::VSWaterLava()
{
	time=0;
	pGround=NULL;
}

VSWaterLava::~VSWaterLava()
{
	RELEASE(pGround);
}

void VSWaterLava::SetTime(double time_)
{
	time=time_;//Тут бы невредно время обрезать, чтобы во float нормально интерполировалось.
	float ftime=(float)time;
	gb_RenderDevice3D->SetPixelShaderConstant(0,&D3DXVECTOR4(ftime,ftime,ftime,ftime));//Криво это.
	SetVector(fTime,&D3DXVECTOR4(ftime,ftime,ftime,ftime));
}

string VSWaterLava::lava_name="Scripts\\Resource\\balmer\\lava.tga";
void SetLavaTexture(const char* file_name)
{
	VSWaterLava::SetLavaTexture(file_name);
}

void VSWaterLava::SetLavaTexture(const char* file_name)
{
	lava_name=file_name;
	vector<cShader*>::iterator it;
	FOR_EACH(all_shader,it)
	{
		VSWaterLava* p=dynamic_cast<VSWaterLava*>(*it);
		if(p)
		{
			RELEASE(p->pGround);
		}
	}
}

void VSWaterLava::Select()
{
	if(!pGround)
	{
		//pGround=GetTexLibrary()->GetElement(lava_name.c_str());
		pGround=GetTexLibrary()->GetElement3D(lava_name.c_str());
	}

	gb_RenderDevice3D->SetTexture(1,pGround);
	SetFog();
	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
}

void VSWaterLava::RestoreShader()
{
	if(gb_RenderDevice3D->IsPS20())
	{
		LoadShader("Water\\water_lava.vsl");
	}else
	{
		LoadShader("Water\\water_lava_ps11.vsl");
	}
}

void VSWaterLava::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mVP);
	VAR_HANDLE(fTime);
}

PSWaterLava::PSWaterLava()
{
	pRandomVolume=(IDirect3DBaseTexture9*)CreateVolumeRand(64);
}

PSWaterLava::~PSWaterLava()
{
	RELEASE(pRandomVolume);
}

void PSWaterLava::Select()
{
	SetFog();
	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetTextureBase(0,pRandomVolume);
	if(!gb_RenderDevice3D->IsPS20())
		gb_RenderDevice3D->SetTextureBase(2,pRandomVolume);

	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->dtAdvance->GetLightMap());
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSWaterLava::SetColors(const sColor4f& lava,const sColor4f& ambient)
{
	SetVector(vLavaColor,(D3DXVECTOR4*)&lava);
	SetVector(vLavaColorAmbient,(D3DXVECTOR4*)&ambient);
}

void PSWaterLava::RestoreShader()
{
	if(gb_RenderDevice3D->IsPS20())
	{
		LoadShader("Water\\water_lava.psl");
	}else
	{
		LoadShader("Water\\water_lava_ps11.psl");
	}
}

void PSWaterLava::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vLavaColor);
	VAR_HANDLE(vLavaColorAmbient);
}

///////////////////////////WaterIce////////////////////////
VSWaterIce::VSWaterIce()
{
	linear=false;
}

void VSWaterIce::SetMirrorMatrix(cCamera* mirror)
{
	if(mirror==NULL)
	{
		linear=false;
		return;
	}
	linear=true;

	CMatrix& matViewProj=mirror->matViewProj;
	int map_size=mirror->GetRenderTarget()?mirror->GetRenderTarget()->GetWidth():1024;
	D3DXMATRIX mat;
	float fOffsetX = 0.5f + (0.5f / map_size);
	float fOffsetY = 0.5f + (0.5f / map_size);

	float fBias    = 0;
	D3DXMATRIX matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
		                  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,     1,     0.0f,
						  fOffsetX, fOffsetY, fBias, 1.0f );


	D3DXMatrixMultiply(&mat, matViewProj, &matTexAdj);
//	D3DXMatrixMultiply(&mat, mirror->matView, &matTexAdj);

	SetMatrix(vMirrorVP,&mat);
}
void PSWaterIce::SetMirrorMatrix(cCamera* mirror)
{
	if(mirror==NULL)
	{
		linear=false;
		return;
	}
	linear=true;
}

void VSWaterIce::Select()
{
	shader->Select(PS11,gb_RenderDevice3D->IsPS20()?0:1);
	SetFog();
	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);

	SetVector(uvScaleOffset,&gb_RenderDevice3D->tilemap_inv_size);

	shader->Select(MIRROR_LINEAR,linear?1:0);
	Vect3f p=gb_RenderDevice3D->GetDrawNode()->GetPos();
	SetVector(vCameraPos,&D3DXVECTOR4(p.x,p.y,p.z,0));

	SetVector(fScaleBumpSnow,&D3DXVECTOR4(0.01f,0.003f,0.002f,0));
	
	__super::Select();
}

void VSWaterIce::RestoreShader()
{
	LoadShader("Water\\water_ice.vsl");
}

void VSWaterIce::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mVP);
	VAR_HANDLE(uvScaleOffset);
	VAR_HANDLE(vCameraPos);
	VAR_HANDLE(vMirrorVP);
	VAR_HANDLE(fScaleBumpSnow);
	VAR_INDEX(MIRROR_LINEAR);
	VAR_INDEX(PS11);
}

PSWaterIce::PSWaterIce()
{
	linear=false;
}

void PSWaterIce::Select()
{
	shader->Select(PS11,gb_RenderDevice3D->IsPS20()?0:1);
	SetFog();
	gb_RenderDevice3D->SetSamplerData( 3,sampler_wrap_linear);
	gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->dtAdvance->GetLightMap());
	shader->Select(MIRROR_LINEAR,linear?1:0);
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSWaterIce::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vSnowColor);
	VAR_INDEX(MIRROR_LINEAR);
	VAR_INDEX(PS11);
}

void PSWaterIce::RestoreShader()
{
	LoadShader("Water\\water_ice.psl");
}

void PSWaterIce::SetSnowColor(sColor4f color)
{
	color.r=clamp(color.r,0.0f,1.0f);
	color.g=clamp(color.g,0.0f,1.0f);
	color.b=clamp(color.b,0.0f,1.0f);
	SetVector(vSnowColor,(D3DXVECTOR4*)&color);
}
