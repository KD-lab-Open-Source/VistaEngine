
void VSSkin::Select(MatXf* world,int world_num,int blend_num)
{
	if(LIGHTMAP.is())
		shader->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);

	SetFog();
	D3DXMATRIX mat;

	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		SetMatrix4x3(mWorldM,i,w);//for skin
	}

	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	
	D3DXVECTOR4 reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	SetVector(vReflectionMul,&reflection_mul);

	xassert(blend_num>=1 && blend_num<=4);
	shader->Select(WEIGHT,blend_num-1);

	LPDIRECT3DVERTEXSHADER9 pShader=shader->GetVertexShader();
	gb_RenderDevice3D->SetVertexShader(pShader);
}

void VSSkin::SetWorldMatrix(MatXf* world,int world_offset,int world_num)
{
	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		SetMatrix4x3(mWorldM,i+world_offset,w);//for skin
	}
}

void VSSkin::RestoreShader()
{
	LoadShader("Skin\\object_scene_light.vsl");
}

void VSSkinNoLight::RestoreShader()
{
	__super::RestoreShader();
	VAR_INDEX(NOLIGHT);
	shader->Select(NOLIGHT,1);
}

void VSSkin::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(WEIGHT);
	VAR_INDEX(LIGHTMAP);
	VAR_INDEX(REFLECTION);
	VAR_INDEX(UVTRANS);
	VAR_INDEX(ZREFLECTION);
	VAR_INDEX(POINT_LIGHT);

	VAR_HANDLE(mVP);
	VAR_HANDLE(mWorldM);

	VAR_HANDLE(vAmbient);
	VAR_HANDLE(vDiffuse);
	VAR_HANDLE(vSpecular);
	VAR_HANDLE(vCameraPos);
	VAR_HANDLE(vLightDirection);
	VAR_HANDLE(vUtrans);
	VAR_HANDLE(vVtrans);

	VAR_HANDLE(vReflectionMul);

	VAR_HANDLE(vPointPos0);
	VAR_HANDLE(vPointPos1);
	VAR_HANDLE(vPointColor0);
	VAR_HANDLE(vPointColor1);
	VAR_HANDLE(vLightAttenuation);
}

void VSSkin::SetUVTrans(float mat[6])
{
	if(!mat)
	{
		shader->Select(UVTRANS,0);
		return;
	}

	shader->Select(UVTRANS,1);
	D3DXVECTOR4 u(mat[0],mat[2],mat[4],0);
	D3DXVECTOR4 v(mat[1],mat[3],mat[5],0);
	SetVector(vUtrans,&u);
	SetVector(vVtrans,&v);
}

void VSSkin::CalcLightParameters(cUnkLight* light,SHADER_HANDLE& spos,SHADER_HANDLE& scolor,Vect2f& attenuation)
{
	sColor4f color=light->GetDiffuse();;
	Vect3f pos=light->GetPos();
	D3DXVECTOR4 position(pos.x,pos.y,pos.z,1);
	SetVector(spos,&position);
	SetVector(scolor,(D3DXVECTOR4*)&color);

	float radius= max(light->GetRadius(),1.0f);
	attenuation.x=1;
	attenuation.y=-attenuation.x/(radius*radius);
}

void VSSkin::SetMaterial(sDataRenderMaterial *Data)
{
	SetVector(vAmbient,(D3DXVECTOR4*)&Data->Ambient);
	SetVector(vDiffuse,(D3DXVECTOR4*)&Data->Diffuse);
	SetVector(vSpecular,(D3DXVECTOR4*)&Data->Specular);
	Vect3f p=gb_RenderDevice3D->GetDrawNode()->GetPos();
	SetVector(vCameraPos,&D3DXVECTOR4(p.x,p.y,p.z,0));
	Vect3f l;
	gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
	SetVector(vLightDirection,&D3DXVECTOR4(l.x,l.y,l.z,0));

	//
	if(POINT_LIGHT.is())
	{
		if(Data->point_light && !Data->point_light->empty())
		{
			vector<class cUnkLight*>& point_light=*Data->point_light;
			Vect2f attenuation0,attenuation1(0,0);
			CalcLightParameters(point_light[0],vPointPos0,vPointColor0,attenuation0);
			if(point_light.size()>1)
			{
				CalcLightParameters(point_light[1],vPointPos1,vPointColor1,attenuation1);
			}else
			{
				D3DXVECTOR4 empty(0,0,0,0);
				SetVector(vPointPos1,&empty);
				SetVector(vPointColor1,&empty);
			}

			D3DXVECTOR4 attenuation(attenuation0.y,attenuation1.y,attenuation0.x,attenuation1.x);
			SetVector(vLightAttenuation,&attenuation);

			shader->Select(POINT_LIGHT,1);
		}else
			shader->Select(POINT_LIGHT,0);
	}
}

void VSSkin::SetAlphaColor(sColor4f* color)
{
	SetVector(vDiffuse,(D3DXVECTOR4*)color);
}

PSSkinNoShadow::PSSkinNoShadow()
{
}

void PSSkinNoShadow::SelectLT(bool light,bool texture)
{
	shader->Select(NOTEXTURE,texture?0:1);
	shader->Select(NOLIGHT,light?0:1);
}

void PSSkinNoShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_light.psl");
}

void PSSkinNoShadow::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(NOTEXTURE);
	VAR_INDEX(NOLIGHT);
}

void PSSkinNoShadow::Select()
{
	SetFog();
	SelectShadowQuality();
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSSkinSceneShadow::Select()
{
	SetFog();
	SelectShadowQuality();
	SetVector(fx_offset,&D3DXVECTOR4(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSSkinSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_light_shadow9700.psl");
}

void PSSkinSceneShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vShade);
	VAR_HANDLE(fx_offset);
}

///////////////////////////////////////bump////////////////////////////
void VSSkinBump::RestoreShader()
{
	LoadShader("Skin\\object_scene_bump.vsl");
	VAR_INDEX(WEIGHT);
}

void PSSkinBump::RestoreShader()
{
	LoadShader("Skin\\object_scene_bump.psl");
}

void PSSkinBumpSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_bump_shadow9700.psl");
}

void PSSkinBump::Select()
{
	SetFog();
	SelectShadowQuality();
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSSkin::SetMaterial(sDataRenderMaterial *Data,bool is_big_ambient)
{
	SetVector(bumpAmbient,(D3DXVECTOR4*)&Data->Ambient);
	SetVector(bumpDiffuse,(D3DXVECTOR4*)&Data->Diffuse);
	SetVector(bumpSpecular,(D3DXVECTOR4*)&Data->Specular);

	if(LIGHTMAP.is())
		shader->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?(is_big_ambient?1:2):0);

	xassert(LERP_TEXTURE_COLOR.is());
	if(Data->lerp_texture.a>0.001f)
	{
		sColor4f lerp_pre;
		lerp_pre.r=Data->lerp_texture.r*Data->lerp_texture.a;
		lerp_pre.g=Data->lerp_texture.g*Data->lerp_texture.a;
		lerp_pre.b=Data->lerp_texture.b*Data->lerp_texture.a;
		lerp_pre.a=1-Data->lerp_texture.a;

		SetVector(tLerpPre,(D3DXVECTOR4*)&lerp_pre);
		shader->Select(LERP_TEXTURE_COLOR,1);
	}else
	{
		shader->Select(LERP_TEXTURE_COLOR,0);
	}
}

void PSSkin::SetAlphaColor(sColor4f* color)
{
	SetVector(bumpDiffuse,(D3DXVECTOR4*)color);
}

void PSSkin::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(bumpAmbient);
	VAR_HANDLE(bumpDiffuse);
	VAR_HANDLE(bumpSpecular);
	VAR_HANDLE(reflectionAmount);
	VAR_HANDLE(tLerpPre);
	VAR_INDEX(REFLECTION);
	VAR_INDEX(LIGHTMAP);
	VAR_INDEX(SELF_ILLUMINATION);
	VAR_INDEX(SPECULARMAP);
	VAR_INDEX(c2x2);
	VAR_INDEX(ZREFLECTION);
	VAR_INDEX(LERP_TEXTURE_COLOR);
}

void PSSkin::SelectShadowQuality()
{
	if(c2x2.is())
		shader->Select(c2x2,Option_ShadowMapSelf4x4?0:1);
}

void PSSkin::SetSelfIllumination(bool on)
{
	if(SELF_ILLUMINATION.is())
		shader->Select(SELF_ILLUMINATION,on?1:0);
}

void PSSkin::SetReflection(int n,sColor4f& amount)
{
	xassert(REFLECTION.is());
	shader->Select(REFLECTION,n);

	SetVector(reflectionAmount,(D3DXVECTOR4*)&amount);
}

void VSSkin::SetReflection(int n)
{
	xassert(REFLECTION.is());
	shader->Select(REFLECTION,n);
}

void PSSkinShadow::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShader("Skin\\object_shadow.psl");
		INDEX_HANDLE SHADOW_9700;
		VAR_INDEX(SHADOW_9700);
		shader->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
	}
}

void PSSkinShadowAlpha::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShader("Skin\\object_shadow.psl");
		INDEX_HANDLE SHADOW_9700;
		VAR_INDEX(SHADOW_9700);
		shader->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
		INDEX_HANDLE ALPHA;
		VAR_INDEX(ALPHA);
		shader->Select(ALPHA,1);
	}
}

void PSSkinShadowAlpha::GetHandle()
{
	__super::GetHandle();
	if(shader)
	{
		VAR_INDEX(SECOND_OPACITY_TEXTURE);
	}
}

void PSSkinShadowAlpha::SetSecondOpacity(cTexture* pTexture)
{
	if(SECOND_OPACITY_TEXTURE.is())
		shader->Select(SECOND_OPACITY_TEXTURE,pTexture?1:0);
	gb_RenderDevice3D->SetTexture(1,pTexture);
}

void VSSkinShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mVP);
	VAR_HANDLE(mWorldM);

	VAR_HANDLE(vUtrans);
	VAR_HANDLE(vVtrans);
	VAR_HANDLE(vSecondUtrans);
	VAR_HANDLE(vSecondVtrans);
	VAR_INDEX(UVTRANS);
	VAR_INDEX(SECOND_UVTRANS);
	VAR_INDEX(SECOND_OPACITY_TEXTURE);
}

void VSSkinShadow::RestoreShader()
{
	LoadShader("Skin\\object_shadow.vsl");
	VAR_INDEX(WEIGHT);

	INDEX_HANDLE TARGET;
	VAR_INDEX(TARGET);
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		if(gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_RADEON9700)
			shader->Select(TARGET,1);
		else
			shader->Select(TARGET,0);
	}
}

void VSSkinShadow::SetUVTrans(float mat[6])
{
	if(!mat)
	{
		shader->Select(UVTRANS,0);
		return;
	}

	shader->Select(UVTRANS,1);
	D3DXVECTOR4 u(mat[0],mat[2],mat[4],0);
	D3DXVECTOR4 v(mat[1],mat[3],mat[5],0);
	SetVector(vUtrans,&u);
	SetVector(vVtrans,&v);
}

void VSSkinShadow::SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1)
{
	shader->Select(SECOND_OPACITY_TEXTURE,use_t1);
	if(!mat) {
		shader->Select(SECOND_UVTRANS,0);
	} else {
		shader->Select(SECOND_UVTRANS,1);
		D3DXVECTOR4 u(mat[0],mat[2],mat[4],0);
		D3DXVECTOR4 v(mat[1],mat[3],mat[5],0);
		SetVector(vSecondUtrans,&u);
		SetVector(vSecondVtrans,&v);
	}
}

void VSSkinShadow::Select(MatXf* world,int world_num,int blend_num)
{
	D3DXMATRIX mat;

	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		SetMatrix4x3(mWorldM,i,w);//for skin
	}

	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	
	xassert(blend_num>=1 && blend_num<=4);
	shader->Select(WEIGHT,blend_num-1);
	gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
}

void VSSkinSceneShadow::Select(MatXf* world,int world_num,int blend_num)
{
	//{//Этому куску здесь не место, когда будет оптимизироваться, перенесётся в начало блока отрисовки объектов.
	//	if(gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_RADEON9700)
	//	{
	//		gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_point);
	//		gb_RenderDevice3D->SetTexture(2,gb_RenderDevice3D->dtAdvance->GetShadowMap());
	//	}else
	//	{
	//		gb_RenderDevice3D->SetSamplerData(2,sampler_clamp_linear);
	//		gb_RenderDevice3D->SetTextureBase(2,gb_RenderDevice3D->dtAdvance->GetTZBuffer());
	//	}
	//}
	gb_RenderDevice3D->dtAdvance->SetShadowMapTexture();
	SetShadowMatrix(this,mShadow,gb_RenderDevice3D->GetShadowMatViewProj());
	__super::Select(world,world_num,blend_num);
}

void VSSkinSceneShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mShadow);
}

void VSSkinSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_light.vsl");
	shader->BeginStaticSelect();
	shader->StaticSelect("SHADOW",1);
	shader->EndStaticSelect();
}

void VSSkinBumpSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_bump.vsl");
	shader->BeginStaticSelect();
	shader->StaticSelect("SHADOW",1);
	shader->EndStaticSelect();
}

void PSSkinSceneShadowFX::RestoreShader()
{
	LoadShader("Skin\\object_scene_light_shadowFX.psl");
}

void PSSkinBumpSceneShadowFX::RestoreShader()
{
	LoadShader("Skin\\object_scene_bump_shadowFX.psl");
}

void PSSkinReflectionSceneShadowFX::RestoreShader()
{
	LoadShader("Skin\\object_scene_reflection_shadowFX.psl");
}

void PSSkinReflectionSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_reflection_shadow9700.psl");
}

void VSSkinReflectionSceneShadow::RestoreShader()
{
	LoadShader("Skin\\object_scene_reflection.vsl");
	shader->BeginStaticSelect();
	shader->StaticSelect("SHADOW",1);
	shader->EndStaticSelect();
}

void VSSkinReflection::RestoreShader()
{
	LoadShader("Skin\\object_scene_reflection.vsl");
}

void VSSkinSecondOpacity::RestoreShader()
{
	LoadShader("Skin\\object_scene_light_second_opacity.vsl");
}

void VSSkinSecondOpacity::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(SECOND_UVTRANS);
	VAR_INDEX(SECOND_OPACITY_TEXTURE);

	VAR_HANDLE(vSecondUtrans);
	VAR_HANDLE(vSecondVtrans);
}

void VSSkinSecondOpacity::SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1)
{
	shader->Select(SECOND_OPACITY_TEXTURE,(use_t1==SECOND_UV_T1)?1:0);
	if(!mat) {
		shader->Select(SECOND_UVTRANS,0);
	} else {
		shader->Select(SECOND_UVTRANS,1);
		D3DXVECTOR4 u(mat[0],mat[2],mat[4],0);
		D3DXVECTOR4 v(mat[1],mat[3],mat[5],0);
		SetVector(vSecondUtrans,&u);
		SetVector(vSecondVtrans,&v);
	}
}

void PSSkinSecondOpacity::GetHandle()
{
	__super::GetHandle();
}

void PSSkinSecondOpacity::RestoreShader()
{
	LoadShader("Skin\\object_scene_light_second_opacity.psl");
}

void PSSkinSecondOpacity::Select()
{
//	SetFog();
	SelectShadowQuality();
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSSkinReflection::RestoreShader()
{
	LoadShader("Skin\\object_scene_reflection.psl");
}


void PSSkin::SelectSpecularMap(cTexture* pSpecularmap,float phase)
{
	xassert(SPECULARMAP.is());
	shader->Select(SPECULARMAP,pSpecularmap?1:0);
	gb_RenderDevice3D->SetTexturePhase( 4, pSpecularmap,phase);
	gb_RenderDevice3D->SetSamplerData( 4,sampler_wrap_linear);
}
