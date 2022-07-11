
void VSSkin::Select(MatXf* world,int world_num,int blend_num)
{
	if(LIGHTMAP.is())
		shaderVS_->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);

	SetFog();
	
	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		setMatrix4x3VS(mWorldM,i,w);//for skin
	}

	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);
	
	Vect4f reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	setVectorVS(vReflectionMul, reflection_mul);

	xassert(blend_num>=1 && blend_num<=4);
	shaderVS_->Select(WEIGHT,blend_num-1);

	cVertexShader::Select();
}

void VSSkin::SetWorldMatrix(MatXf* world,int world_offset,int world_num)
{
	for(int i=0;i<world_num;i++)
		setMatrix4x3VS(mWorldM, world_offset + i, world[i]);//for skin
}

void VSSkin::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_light.vsl");
}

void VSSkinNoLight::RestoreShader()
{
	__super::RestoreShader();
	VAR_INDEX_VS(NOLIGHT);
	shaderVS_->Select(NOLIGHT,1);
}

void VSSkin::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_VS(WEIGHT);
	VAR_INDEX_VS(LIGHTMAP);
	VAR_INDEX_VS(REFLECTION);
	VAR_INDEX_VS(UVTRANS);
	VAR_INDEX_VS(ZREFLECTION);
	VAR_INDEX_VS(POINT_LIGHT);

	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(mWorldM);

	VAR_HANDLE_VS(vAmbient);
	VAR_HANDLE_VS(vDiffuse);
	VAR_HANDLE_VS(vSpecular);
	VAR_HANDLE_VS(vCameraPos);
	VAR_HANDLE_VS(vLightDirection);
	VAR_HANDLE_VS(vUtrans);
	VAR_HANDLE_VS(vVtrans);

	VAR_HANDLE_VS(vReflectionMul);

	VAR_HANDLE_VS(vPointPos0);
	VAR_HANDLE_VS(vPointPos1);
	VAR_HANDLE_VS(vPointColor0);
	VAR_HANDLE_VS(vPointColor1);
	VAR_HANDLE_VS(vLightAttenuation);
}

void VSSkin::SetUVTrans(float mat[6])
{
	if(!mat)
	{
		shaderVS_->Select(UVTRANS,0);
		return;
	}

	shaderVS_->Select(UVTRANS,1);
	Vect4f u(mat[0],mat[2],mat[4],0);
	Vect4f v(mat[1],mat[3],mat[5],0);
	setVectorVS(vUtrans, u);
	setVectorVS(vVtrans, v);
}

void VSSkin::CalcLightParameters(cUnkLight* light,SHADER_HANDLE& spos,SHADER_HANDLE& scolor,Vect2f& attenuation)
{
	setVectorVS(spos, Vect4f(light->GetPos(), 1));
	setVectorVS(scolor, light->GetDiffuse());

	float radius= max(light->GetRadius(),1.0f);
	attenuation.x=1;
	attenuation.y=-attenuation.x/(radius*radius);
}

void VSSkin::SetMaterial(sDataRenderMaterial *Data)
{
	setVectorVS(vAmbient, Data->Ambient);
	setVectorVS(vDiffuse, Data->Diffuse);
	setVectorVS(vSpecular, Data->Specular);
	Vect3f p=gb_RenderDevice3D->camera()->GetPos();
	setVectorVS(vCameraPos, Vect4f(p.x,p.y,p.z,0));
	Vect3f l;
	gb_RenderDevice3D->camera()->GetLighting(l);
	setVectorVS(vLightDirection, Vect4f(l.x,l.y,l.z,0));

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
				setVectorVS(vPointPos1, Vect4f::ZERO);
				setVectorVS(vPointColor1,Vect4f::ZERO);
			}

			Vect4f attenuation(attenuation0.y,attenuation1.y,attenuation0.x,attenuation1.x);
			setVectorVS(vLightAttenuation, attenuation);

			shaderVS_->Select(POINT_LIGHT,1);
		}else
			shaderVS_->Select(POINT_LIGHT,0);
	}
}

void VSSkin::SetAlphaColor(const Color4f& color)
{
	setVectorVS(vDiffuse, color);
}

PSSkinNoShadow::PSSkinNoShadow()
{
}

void PSSkinNoShadow::SelectLT(bool light,bool texture)
{
	shaderPS_->Select(NOTEXTURE,texture?0:1);
	shaderPS_->Select(NOLIGHT,light?0:1);
}

void PSSkinNoShadow::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_light.psl");
}

void PSSkinNoShadow::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_PS(NOTEXTURE);
	VAR_INDEX_PS(NOLIGHT);
}

void PSSkinNoShadow::Select()
{
	SetFog();
	
	__super::Select();
}

void PSSkinSceneShadow::Select()
{
	SetFog();
	setVectorPS(fx_offset, Vect4f(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));
	
	__super::Select();
}

void PSSkinSceneShadow::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_light_shadow9700.psl");
}

void PSSkinSceneShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(vShade);
	VAR_HANDLE_PS(fx_offset);
}

///////////////////////////////////////bump////////////////////////////
void VSSkinBump::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_bump.vsl");
	VAR_INDEX_VS(WEIGHT);
}

void PSSkinBump::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_bump.psl");
}

void PSSkinBumpSceneShadow::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_bump_shadow9700.psl");
}

void PSSkinBump::Select()
{
	SetFog();

	__super::Select();
}

void PSSkin::SetMaterial(sDataRenderMaterial *Data,bool is_big_ambient)
{
	setVectorPS(bumpAmbient, Data->Ambient);
	setVectorPS(bumpDiffuse, Data->Diffuse);
	setVectorPS(bumpSpecular, Data->Specular);

	if(LIGHTMAP.is())
		shaderPS_->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?(is_big_ambient?1:2):0);

	xassert(LERP_TEXTURE_COLOR.is());
	if(Data->lerp_texture.a>0.001f)
	{
		Color4f lerp_pre;
		lerp_pre.r=Data->lerp_texture.r*Data->lerp_texture.a;
		lerp_pre.g=Data->lerp_texture.g*Data->lerp_texture.a;
		lerp_pre.b=Data->lerp_texture.b*Data->lerp_texture.a;
		lerp_pre.a=1-Data->lerp_texture.a;

		setVectorPS(tLerpPre, lerp_pre);
		shaderPS_->Select(LERP_TEXTURE_COLOR,1);
	}else
	{
		shaderPS_->Select(LERP_TEXTURE_COLOR,0);
	}
}

void PSSkin::SetAlphaColor(const Color4f& color)
{
	setVectorPS(bumpDiffuse, color);
}

void PSSkin::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(bumpAmbient);
	VAR_HANDLE_PS(bumpDiffuse);
	VAR_HANDLE_PS(bumpSpecular);
	VAR_HANDLE_PS(reflectionAmount);
	VAR_HANDLE_PS(tLerpPre);
	VAR_INDEX_PS(REFLECTION);
	VAR_INDEX_PS(LIGHTMAP);
	VAR_INDEX_PS(SELF_ILLUMINATION);
	VAR_INDEX_PS(SPECULARMAP);
	VAR_INDEX_PS(FILTER_SHADOW);
	VAR_INDEX_PS(ZREFLECTION);
	VAR_INDEX_PS(LERP_TEXTURE_COLOR);
}

void PSSkin::Select()
{
	if(FILTER_SHADOW.is())
		shaderPS_->Select(FILTER_SHADOW,Option_filterShadow);
	
	__super::Select();
}

void PSSkin::SetSelfIllumination(bool on)
{
	if(SELF_ILLUMINATION.is())
		shaderPS_->Select(SELF_ILLUMINATION,on?1:0);
}

void PSSkin::SetReflection(int n, const Color4f& amount)
{
	xassert(REFLECTION.is());
	shaderPS_->Select(REFLECTION,n);

	setVectorPS(reflectionAmount, amount);
}

void VSSkin::SetReflection(int n)
{
	xassert(REFLECTION.is());
	shaderVS_->Select(REFLECTION,n);
}

void PSSkinShadow::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShaderPS("Skin\\object_shadow.psl");
		INDEX_HANDLE SHADOW_9700;
		VAR_INDEX_PS(SHADOW_9700);
		shaderPS_->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
	}
}

void PSSkinShadowAlpha::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShaderPS("Skin\\object_shadow.psl");
		INDEX_HANDLE SHADOW_9700;
		VAR_INDEX_PS(SHADOW_9700);
		shaderPS_->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
		INDEX_HANDLE ALPHA;
		VAR_INDEX_PS(ALPHA);
		shaderPS_->Select(ALPHA,1);
	}
}

void PSSkinShadowAlpha::GetHandle()
{
	__super::GetHandle();
	if(shaderPS_)
		VAR_INDEX_PS(SECOND_OPACITY_TEXTURE);
}

void PSSkinShadowAlpha::SetSecondOpacity(cTexture* pTexture)
{
	if(SECOND_OPACITY_TEXTURE.is())
		shaderPS_->Select(SECOND_OPACITY_TEXTURE,pTexture?1:0);
	gb_RenderDevice3D->SetTexture(1,pTexture);
}

void VSSkinShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(mWorldM);

	VAR_HANDLE_VS(vUtrans);
	VAR_HANDLE_VS(vVtrans);
	VAR_HANDLE_VS(vSecondUtrans);
	VAR_HANDLE_VS(vSecondVtrans);
	VAR_INDEX_VS(UVTRANS);
	VAR_INDEX_VS(SECOND_UVTRANS);
	VAR_INDEX_VS(SECOND_OPACITY_TEXTURE);
}

void VSSkinShadow::RestoreShader()
{
	LoadShaderVS("Skin\\object_shadow.vsl");
	VAR_INDEX_VS(WEIGHT);

	INDEX_HANDLE TARGET;
	VAR_INDEX_VS(TARGET);
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		if(gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_RADEON9700)
			shaderVS_->Select(TARGET,1);
		else
			shaderVS_->Select(TARGET,0);
	}
}

void VSSkinShadow::SetUVTrans(float mat[6])
{
	if(!mat)
	{
		shaderVS_->Select(UVTRANS,0);
		return;
	}

	shaderVS_->Select(UVTRANS,1);
	Vect4f u(mat[0],mat[2],mat[4],0);
	Vect4f v(mat[1],mat[3],mat[5],0);
	setVectorVS(vUtrans, u);
	setVectorVS(vVtrans, v);
}

void VSSkinShadow::SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1)
{
	shaderVS_->Select(SECOND_OPACITY_TEXTURE,use_t1);
	if(!mat) {
		shaderVS_->Select(SECOND_UVTRANS,0);
	} else {
		shaderVS_->Select(SECOND_UVTRANS,1);
		Vect4f u(mat[0],mat[2],mat[4],0);
		Vect4f v(mat[1],mat[3],mat[5],0);
		setVectorVS(vSecondUtrans, u);
		setVectorVS(vSecondVtrans, v);
	}
}

void VSSkinShadow::Select(MatXf* world,int world_num,int blend_num)
{
	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		setMatrix4x3VS(mWorldM,i,w);//for skin
	}

	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);
	
	xassert(blend_num>=1 && blend_num<=4);
	shaderVS_->Select(WEIGHT,blend_num-1);
	
	cVertexShader::Select();
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
	setMatrixVS(mShadow, gb_RenderDevice3D->shadowMatViewProj()*gb_RenderDevice3D->shadowMatBias());
	__super::Select(world,world_num,blend_num);
}

void VSSkinSceneShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mShadow);
}

void VSSkinSceneShadow::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_light.vsl");
	shaderVS_->BeginStaticSelect();
	shaderVS_->StaticSelect("SHADOW",1);
	shaderVS_->EndStaticSelect();
}

void VSSkinBumpSceneShadow::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_bump.vsl");
	shaderVS_->BeginStaticSelect();
	shaderVS_->StaticSelect("SHADOW",1);
	shaderVS_->EndStaticSelect();
}

void PSSkinSceneShadowFX::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_light_shadowFX.psl");
}

void PSSkinBumpSceneShadowFX::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_bump_shadowFX.psl");
}

void PSSkinReflectionSceneShadowFX::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_reflection_shadowFX.psl");
}

void PSSkinReflectionSceneShadow::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_reflection_shadow9700.psl");
}

void VSSkinReflectionSceneShadow::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_reflection.vsl");
	shaderVS_->BeginStaticSelect();
	shaderVS_->StaticSelect("SHADOW",1);
	shaderVS_->EndStaticSelect();
}

void VSSkinReflection::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_reflection.vsl");
}

void VSSkinSecondOpacity::RestoreShader()
{
	LoadShaderVS("Skin\\object_scene_light_second_opacity.vsl");
}

void VSSkinSecondOpacity::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_VS(SECOND_UVTRANS);
	VAR_INDEX_VS(SECOND_OPACITY_TEXTURE);

	VAR_HANDLE_VS(vSecondUtrans);
	VAR_HANDLE_VS(vSecondVtrans);
}

void VSSkinSecondOpacity::SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1)
{
	shaderVS_->Select(SECOND_OPACITY_TEXTURE,(use_t1==SECOND_UV_T1)?1:0);
	if(!mat) {
		shaderVS_->Select(SECOND_UVTRANS,0);
	} else {
		shaderVS_->Select(SECOND_UVTRANS,1);
		Vect4f u(mat[0],mat[2],mat[4],0);
		Vect4f v(mat[1],mat[3],mat[5],0);
		setVectorVS(vSecondUtrans, u);
		setVectorVS(vSecondVtrans, v);
	}
}

void PSSkinSecondOpacity::GetHandle()
{
	__super::GetHandle();
}

void PSSkinSecondOpacity::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_light_second_opacity.psl");
}

void PSSkinSecondOpacity::Select()
{
//	SetFog();

	cPixelShader::Select(); 
}

void PSSkinReflection::RestoreShader()
{
	LoadShaderPS("Skin\\object_scene_reflection.psl");
}


void PSSkin::SelectSpecularMap(cTexture* pSpecularmap,float phase)
{
	xassert(SPECULARMAP.is());
	shaderPS_->Select(SPECULARMAP,pSpecularmap?1:0);
	gb_RenderDevice3D->SetTexturePhase( 4, pSpecularmap,phase);
	gb_RenderDevice3D->SetSamplerData( 4,sampler_wrap_linear);
}
