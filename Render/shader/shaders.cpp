#include "StdAfxRD.h"
#include "D3DRender.h"
#include "shaders.h"
#include "UnkLight.h"
#include "scene.h"
#include "cCamera.h"
#include "VisGeneric.h"

#define VAR_HANDLE_VS(x) x = shaderVS_->GetConstHandle(#x);
#define VAR_INDEX_VS(x) x = shaderVS_->GetIndexHandle(#x)

#define VAR_HANDLE_PS(x) x = shaderPS_->GetConstHandle(#x);
#define VAR_INDEX_PS(x) x = shaderPS_->GetIndexHandle(#x)
// При совпадении имен vs и ps:
#define VAR_HANDLE_PSX(x, xName) x = shaderPS_->GetConstHandle(xName);
#define VAR_INDEX_PSX(x, xName) x = shaderPS_->GetIndexHandle(xName) 

/*
О HLSL:
 Для ускорения 
	вместо
		pConstantTable->SetFloat
	используется
		gb_RenderDevice3D->SetVertexShaderConstant

  Константы по умолчанию не устанапливаются!!!
  зато устанавливаются static константы.
  
*/

vector<cShader*> cShader::all_shader;

void cD3DRender::SetVertexShaderConstant(int start,const Mat4f& mat)
{
	RDCALL(D3DDevice_->SetVertexShaderConstantF(start, mat,4));
}

void cD3DRender::SetVertexShaderConstant(int start,const Vect4f& vect)
{
	RDCALL(D3DDevice_->SetVertexShaderConstantF(start, (const float*)&vect, 1));
}

void cD3DRender::SetPixelShaderConstant(int start,const Vect4f& vect)
{
	RDCALL(D3DDevice_->SetPixelShaderConstantF(start, (const float*)&vect, 1));
}

///////////////////////////////////////////////////

cShader::cShader()
{
	MTG();
	all_shader.push_back(this);
	
	shaderVS_ = 0;
	shaderPS_ = 0;
}

cShader::~cShader()
{
	Delete();

	vector<cShader*>::iterator it=
	find(all_shader.begin(),all_shader.end(),this);
	if(it!=all_shader.end())
		all_shader.erase(it);
	else
		xassert(0);
}

void cShader::Delete()
{
	RELEASE(shaderVS_);
	RELEASE(shaderPS_);
}

void cShader::Restore()
{
	Delete();
	RestoreShader();
	GetHandle();
}

void cShader::Select()
{
	if(shaderVS_)
		gb_RenderDevice3D->SetVertexShader(shaderVS_->GetVertexShader());

	if(shaderPS_)
		gb_RenderDevice3D->SetPixelShader(shaderPS_->GetPixelShader());
}

void cShader::LoadShaderVS(const char* name)
{
	shaderVS_ = gb_RenderDevice3D->GetShaderLib()->Get(name);
	xassertStr(shaderVS_, name);
}

void cShader::LoadShaderPS(const char* name)
{
	shaderPS_ =gb_RenderDevice3D->GetShaderLib()->Get(name);
	xassertStr(shaderPS_, name);
}

inline void cShader::setMatrixVS(const SHADER_HANDLE& h,const Mat4f& mat)
{
	if(h.num_register)
	{
		Mat4f matTrans(mat);
		matTrans.transpose();
		RDCALL(gb_RenderDevice3D->D3DDevice_->
			SetVertexShaderConstantF(h.begin_register,matTrans,h.num_register));
	}
}

inline void cShader::setMatrix4x4VS(const SHADER_HANDLE& h,int index,const Mat4f& mat)
{
	const int size=4;
	if(h.num_register)
	{
		Mat4f matTrans(mat);
		matTrans.transpose();
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->D3DDevice_->
			SetVertexShaderConstantF(h.begin_register+index*size,matTrans,size));
	}
}

inline void cShader::setMatrix4x3VS(const SHADER_HANDLE& h,int index,const Mat4f& mat)
{
	const int size=3;
	if(h.num_register)
	{
		Mat4f matTrans(mat);
		matTrans.transpose();
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->D3DDevice_->
			SetVertexShaderConstantF(h.begin_register+index*size,matTrans,size));
	}
}

void cShader::setMatrix4x3VS(const SHADER_HANDLE& h,int index,const MatXf& mat)
{
	const int size=3;
	if(h.num_register)
	{
		Mat4f mat4(mat);
		mat4.transpose();
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->D3DDevice_->
			SetVertexShaderConstantF(h.begin_register+index*size,mat4,size));
	}
}

inline void cShader::setVectorVS(const SHADER_HANDLE& h,const Vect4f& vect)
{
	if(h.num_register)
	{
		gb_RenderDevice3D->SetVertexShaderConstant(h.begin_register,vect);
//		pConstantTable->SetVector(gb_RenderDevice3D->lpD3DDevice,h,vect);
	}
}

inline void cShader::setVectorPS(const SHADER_HANDLE& h,const Vect4f& vect)
{
	if(h.num_register)
		gb_RenderDevice3D->SetPixelShaderConstant(h.begin_register,vect);
}

inline void cShader::setVectorPS(const SHADER_HANDLE& h,const Vect4f* vects,const int count)
{
	if(h.num_register)
		RDCALL(gb_RenderDevice3D->D3DDevice_->SetPixelShaderConstantF(h.begin_register,(const float*)vects,count));
}

inline void cShader::setFloatVS(const SHADER_HANDLE& h,const float vect)
{
	if(h.num_register)
		gb_RenderDevice3D->SetVertexShaderConstant(h.begin_register,Vect4f(vect,vect,vect,vect));
}

////////////////////////////////////////////////////////

void VSShadow::Select()
{
	setMatrixVS(mWVP, gb_RenderDevice3D->camera()->matViewProj);

	cVertexShader::Select();
}

void VSShadow::RestoreShader()
{
	LoadShaderVS("Minimal\\tile_map_shadow.vsl");
	INDEX_HANDLE SHADOW_9700;
	VAR_INDEX_VS(SHADOW_9700);
	shaderVS_->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
}

void VSShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mWVP);
}

void PSShadow::RestoreShader()
{
	LoadShaderPS("Minimal\\tile_map_shadow.psl");
	INDEX_HANDLE SHADOW_9700;
	VAR_INDEX_PS(SHADOW_9700);
	shaderPS_->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
}

void PSShadow::GetHandle()
{
	__super::GetHandle();
}

void PSShowMap::RestoreShader()
{
	LoadShaderPS("Minimal\\ShowMap9700.psl");
}

void PSShowAlpha::RestoreShader()
{
	LoadShaderPS("Minimal\\ShowAlpha.psl");
}


void VSChaos::Select(float umin,float vmin,float umin2,float vmin2,
					 float umin_b0,float vmin_b0,float umin_b1,float vmin_b1)
{
	SetFog();
	setMatrixVS(mWVP,gb_RenderDevice3D->camera()->matViewProj);
	setVectorVS(mUV, Vect4f(umin,vmin,umin2,vmin2));
	setVectorVS(mUVBump, Vect4f(umin_b0,vmin_b0,umin_b1,vmin_b1));
	setMatrixVS(mView,gb_RenderDevice3D->camera()->matView);

	cVertexShader::Select();
}

void VSChaos::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(mWVP);
	VAR_HANDLE_VS(mUV);
	VAR_HANDLE_VS(mUVBump);
	VAR_HANDLE_VS(mView);
	VAR_HANDLE_VS(vFog);
}

void VSChaos::RestoreShader()
{
	LoadShaderVS("chaos\\chaos.vsl");
	INDEX_HANDLE VERTEX_FOG;
	VAR_INDEX_VS(VERTEX_FOG);
	shaderVS_->Select(VERTEX_FOG,gb_RenderDevice3D->bSupportTableFog?0:1);
}

void PSChaos::RestoreShader()
{
	LoadShaderPS("Chaos\\chaos.psl");
}


void VSChaos::SetFog()
{
	setVectorVS(vFog, gb_RenderDevice3D->GetVertexFogParam());
}

void ShaderSceneTileMap::GetHandle()
{
	__super::GetHandle();

	VAR_HANDLE_VS(mWVP);
	VAR_HANDLE_VS(vLightDirection);
	VAR_HANDLE_VS(UV);
	VAR_HANDLE_VS(UVbump);
	VAR_HANDLE_VS(vColor);
	VAR_HANDLE_VS(mulMiniTexture);
	VAR_HANDLE_VS(vReflectionMul);
	VAR_INDEX_VS(ZREFLECTION);
	VAR_INDEX_VS(VERTEX_LIGHT);

	VAR_INDEX_PSX(ZREFLECTION_PS, "ZREFLECTION");
	VAR_INDEX_PS(FILTER_SHADOW);
	VAR_INDEX_PSX(VERTEX_LIGHT_PS, "VERTEX_LIGHT");
	VAR_HANDLE_PS(light_color);
}

void ShaderSceneTileMap::SetReflectionZ(bool reflection)
{
	if(ZREFLECTION.is())
		shaderVS_->Select(ZREFLECTION,reflection?1:0);
	if(ZREFLECTION_PS.is())
		shaderPS_->Select(ZREFLECTION_PS,reflection?1:0);
}

void ShaderSceneTileMap::Select()
{
	SetFog();
	setMatrixVS(mWVP, gb_RenderDevice3D->camera()->matViewProj);
	
	if(FILTER_SHADOW.is())
		shaderPS_->Select(FILTER_SHADOW,Option_filterShadow);

	if(VERTEX_LIGHT.is())
		shaderVS_->Select(VERTEX_LIGHT, Option_tileMapVertexLight);
	if(VERTEX_LIGHT_PS.is())
		shaderPS_->Select(VERTEX_LIGHT_PS, Option_tileMapVertexLight);

	__super::Select();
}

void ShaderSceneTileMap::SetUV(Vect2f& uv_base,Vect2f& uv_step,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	Vect4f uv(uv_base.x,uv_base.y,uv_step.x,uv_step.y);
	setVectorVS(UV, uv);
	Vect4f uv_bump(uv_base_bump.x,uv_base_bump.y,uv_step_bump.x,uv_step_bump.y);
	setVectorVS(UVbump, uv_bump);
}

void ShaderSceneTileMap::SetWorldSize(Vect2f sz)
{
	setVectorVS(fPlanarNode, gb_RenderDevice3D->planarTransform());
	
	Vect4f reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	setVectorVS(vReflectionMul, reflection_mul);

	Vect3f l;
	gb_RenderDevice3D->camera()->scene()->GetLightingShadow(l);
	setVectorVS(vLightDirection, Vect4f(l.x,l.y,l.z,0));
}

void ShaderSceneTileMap::SetColor(const Color4f& color)
{
	setVectorVS(vColor, color);
	setVectorPS(light_color, color);
}

void vsSceneShader::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_VS(vFog);
	VAR_HANDLE_VS(mView);
	VAR_HANDLE_VS(fPlanarNode);

	VAR_INDEX_VS(FOG_OF_WAR);
}

void vsSceneShader::SetFog()
{
	setMatrixVS(mView, gb_RenderDevice3D->camera()->matView);
	setVectorVS(vFog, gb_RenderDevice3D->GetVertexFogParam());

	setVectorVS(fPlanarNode, gb_RenderDevice3D->planarTransform());
	shaderVS_->Select(FOG_OF_WAR,gb_RenderDevice3D->GetFogOfWar()?1:0);
}

psSceneShader::psSceneShader()
{
}

void psSceneShader::GetHandle()
{
	VAR_HANDLE_PS(vFogOfWar);
	VAR_HANDLE_PS(vShade);
	VAR_INDEX_PS(FOG_OF_WAR);
}

void psSceneShader::SetFog()
{
	if(gb_RenderDevice3D->GetFogOfWar()){
		setVectorPS(vFogOfWar, gb_RenderDevice3D->fog_of_war_color);
		shaderPS_->Select(FOG_OF_WAR,1);
	}
	else
		shaderPS_->Select(FOG_OF_WAR,0);
}

void psSceneShader::SetShadowIntensity(const Color4f& f)
{
	//f=f*f; //??????????????????????
	setVectorPS(vShade, f);
}

void ShaderScene::GetHandle()
{
	__super::GetHandle();
	
	VAR_HANDLE_VS(vFog);
	VAR_HANDLE_VS(mView);
	VAR_HANDLE_VS(fPlanarNode);

	VAR_INDEX_VS(FOG_OF_WAR);

	VAR_HANDLE_PS(vFogOfWar);
	VAR_HANDLE_PS(vShade);
	VAR_INDEX_PSX(FOG_OF_WAR_PS, "FOG_OF_WAR");
}

void ShaderScene::SetFog()
{
	setMatrixVS(mView, gb_RenderDevice3D->camera()->matView);
	setVectorVS(vFog, gb_RenderDevice3D->GetVertexFogParam());

	setVectorVS(fPlanarNode, gb_RenderDevice3D->planarTransform());
	shaderVS_->Select(FOG_OF_WAR,gb_RenderDevice3D->GetFogOfWar()?1:0);

	if(gb_RenderDevice3D->GetFogOfWar()){
		setVectorPS(vFogOfWar, gb_RenderDevice3D->fog_of_war_color);
		shaderPS_->Select(FOG_OF_WAR_PS,1);
	}
	else
		shaderPS_->Select(FOG_OF_WAR_PS,0);
}

void ShaderScene::SetShadowIntensity(const Color4f& f)
{
	//f=f*f; //??????????????????????
	setVectorPS(vShade, f);
}

void ShaderSceneTileMap::SetMiniTextureSize(int dx,int dy, float res)
{
	Vect4f mul(res/dx,res/dy,0,0);
	setVectorVS(mulMiniTexture, mul);
}

void VSGrass::Select(float time_,float hideDistance_, Color4f sunDiffuse_, bool toZBuffer)
{
	Vect3f p=gb_RenderDevice3D->camera()->GetPos();
	setVectorVS(vCameraPos, Vect4f(p.x,p.y,p.z,0));
	Vect3f l;
	gb_RenderDevice3D->camera()->GetLighting(l);
	setVectorVS(vLightDirection, Vect4f(l.x,l.y,l.z,0));
	setVectorVS(time, Vect4f(time_,time_,time_,0));
	setVectorVS(hideDistance, Vect4f(hideDistance_,hideDistance_,hideDistance_,0));
	setVectorVS(SunDiffuse, Vect4f(sunDiffuse_.r,sunDiffuse_.g,sunDiffuse_.b,sunDiffuse_.a));
	gb_RenderDevice3D->dtAdvance->SetShadowMapTexture();
	setMatrixVS(mShadow, gb_RenderDevice3D->shadowMatViewProj()*gb_RenderDevice3D->shadowMatBias());

	if(LIGHTMAP.is())
		shaderVS_->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);
	if(OLD_LIGHTING.is())
		shaderVS_->Select(OLD_LIGHTING,oldLighting);
	if(ZBUFFER.is())
		shaderVS_->Select(ZBUFFER,toZBuffer);

	SetFog();
	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);
	Mat4f mat(gb_RenderDevice3D->camera()->GetMatrix());
	mat.transpose();
	setMatrixVS(mWorld,mat);
	__super::Select();
}

void VSGrass::RestoreShader()
{
	LoadShaderVS("grass\\grass.vsl");
	shaderVS_->BeginStaticSelect();
	shaderVS_->StaticSelect("SHADOW",1);
	shaderVS_->EndStaticSelect();
}

void VSGrass::SetOldLighting(bool enble)
{
	oldLighting = enble;
}

void VSGrass::GetHandle()
{
	VAR_HANDLE_VS(time);
	VAR_HANDLE_VS(hideDistance);
	VAR_HANDLE_VS(SunDiffuse);
	VAR_HANDLE_VS(vCameraPos);
	VAR_HANDLE_VS(vLightDirection);
	VAR_HANDLE_VS(mShadow);
	VAR_INDEX_VS(LIGHTMAP);
	VAR_INDEX_VS(OLD_LIGHTING);
	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(mWorld);
	VAR_INDEX_VS(ZBUFFER);
	VAR_HANDLE_VS(vDofParams);
	__super::GetHandle();
}

void PSGrass::Select()
{
	if(LIGHTMAP.is())
		shaderPS_->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);
	SetFog();

	__super::Select();
}

void PSGrass::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_PS(LIGHTMAP);
}

void PSGrass::RestoreShader()
{
	LoadShaderPS("grass\\grass.psl");
}

void PSGrassShadow::Select()
{
	shaderPS_->Select(FILTER_SHADOW,Option_filterShadow);
	setVectorPS(fx_offset, Vect4f(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));
	__super::Select();
}

void PSGrassShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(vShade);
	VAR_HANDLE_PS(fx_offset);
	VAR_INDEX_PS(FILTER_SHADOW);
}

void PSGrassShadow::RestoreShader()
{
	LoadShaderPS("grass\\grass_shadow9700.psl");
}
void PSGrassShadowFX::RestoreShader()
{
	LoadShaderPS("grass\\grass_shadowFX.psl");
}

void PSMiniMap::Select(const Color4f& waterColor_,const Color4f& terraColor_)
{
	setVectorPS(waterColor, Vect4f(waterColor_.r,waterColor_.g,waterColor_.b,waterColor_.a));
	setVectorPS(terraColor, Vect4f(terraColor_.r,terraColor_.g,terraColor_.b,terraColor_.a));
	setVectorPS(additionAlpha, Vect4f(additionAlpha_, additionAlpha_, additionAlpha_, additionAlpha_));

	__super::Select();
}

void PSMiniMap::RestoreShader()
{
	LoadShaderPS("miniMap.psl");
}

void PSMiniMap::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(waterColor);
	VAR_HANDLE_PS(terraColor);
	VAR_HANDLE_PS(additionAlpha);
	VAR_INDEX_PS(USE_WATER);
	VAR_INDEX_PS(ADDITION_TEXTURE);
	VAR_INDEX_PS(USE_TERRA_COLOR);
	VAR_INDEX_PS(USE_BORDER);
}

void PSMiniMapBorder::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_PS(USE_TEXTURE);
	VAR_INDEX_PS(USE_BORDER);
}

void PSMiniMapBorder::RestoreShader()
{
	LoadShaderPS("miniMapBorder.psl");
}

void PSZBuffer::RestoreShader()
{
	LoadShaderPS("ZBuffer\\ZBuffer.psl");
}

void PSZBuffer::Select(bool floatZBufer)
{
	if (FLOATZBUFFER.is())
		shaderPS_->Select(FLOATZBUFFER,floatZBufer?1:0);
	__super::Select();
}

void PSZBuffer::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX_PS(FLOATZBUFFER);
}
void PSSkinZBuffer::RestoreShader()
{
	LoadShaderPS("ZBuffer\\object_zbuffer.psl");
}
void PSSkinZBufferAlpha::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShaderPS("ZBuffer\\object_zbuffer.psl");
		INDEX_HANDLE ALPHA;
		VAR_INDEX_PS(ALPHA);
		shaderPS_->Select(ALPHA,1);
	}
}

void VSTileZBuffer::Select()
{
	SetFog();
	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);
	__super::Select();
}
void VSTileZBuffer::RestoreShader()
{
	LoadShaderVS("ZBuffer\\tilemap_zbuffer.vsl");
}

void VSTileZBuffer::GetHandle()
{
	VAR_HANDLE_VS(mVP);
	__super::GetHandle();

}

VSZBuffer::VSZBuffer()
{
	DofParams.x = 100;
	DofParams.y = 1000;
}

void VSZBuffer::Select(MatXf* world,int world_num,int blend_num,bool object)
{
	SetFog();

	setMatrixVS(mVP,gb_RenderDevice3D->camera()->matViewProj);

	setVectorVS(vDofParams, Vect4f(DofParams.x,DofParams.y,0,0));

	if (object)
	{
		for(int i=0;i<world_num;i++)
		{
			MatXf& w=world[i];
			setMatrix4x3VS(mWorldM,i,w);//for skin
		}
		xassert(blend_num>=1 && blend_num<=4);
		shaderVS_->Select(WEIGHT,blend_num-1);
		shaderVS_->Select(OBJECT,1);
	}
	else
		shaderVS_->Select(OBJECT,0);

	__super::Select();
}
void VSZBuffer::SetDofParams(Vect2f params)
{
	DofParams = params;
}

void VSZBuffer::SetWorldMatrix(MatXf* world,int world_offset,int world_num)
{
	for(int i=0;i<world_num;i++)
	{
		MatXf& w=world[i];
		setMatrix4x3VS(mWorldM,i+world_offset,w);//for skin
	}
}
void VSZBuffer::RestoreShader()
{
	LoadShaderVS("ZBuffer\\ZBuffer.vsl");
}

void VSZBuffer::GetHandle()
{
	VAR_HANDLE_VS(mVP);
	VAR_HANDLE_VS(mWorldM);
	VAR_HANDLE_VS(vDofParams);
	VAR_INDEX_VS(WEIGHT);
	VAR_INDEX_VS(OBJECT);
	__super::GetHandle();
}

void VSSkinZBuffer::RestoreShader()
{
	LoadShaderVS("ZBuffer\\object_zbuffer.vsl");
	VAR_INDEX_VS(WEIGHT);
}

void PSFillColor::Select(Color4f color)
{
	setVectorPS(vColor, Vect4f(color.r,color.g,color.b,color.a));
	__super::Select();
}
void PSFillColor::Select(float r, float g, float b, float a)
{
	setVectorPS(vColor, Vect4f(r,g,b,a));
	__super::Select();
}

void PSFillColor::RestoreShader()
{
	LoadShaderPS("ZBuffer\\ClearAlpha.psl");
}

void PSFillColor::GetHandle()
{
	VAR_HANDLE_PS(vColor);
	__super::GetHandle();
}

void VSDipCost::GetHandle()
{
	VAR_HANDLE_VS(pos);
	__super::GetHandle();
}
void VSDipCost::RestoreShader()
{
	LoadShaderVS("dipcost.vsl");
}

void PSDipCost::GetHandle()
{
	__super::GetHandle();
}

void PSDipCost::RestoreShader()
{
	LoadShaderPS("dipcost.psl");
}

#include "PostProcessing.inl"
#include "ShadersRadeon9700.inl"
#include "ShadersGeforceFX.inl"

#include "ShaderSkin.inl"
#include "ShaderWater.inl"
#include "ShadersMinimal.inl"
#include "ShaderStandart.inl"
#include "CloudShadow.inl"
#include "blobs.inl"
#include "fur.inl"

/*
Про многоообразие шейдеров.
1) Команда Select и сопутствующие установки констант.
2) Шейдеры побитые дефайнами - количество костей, бамп, дополнительные источники света, туман и т.д.
3) Шейдеры под разное железо. Реальная разница - только в установках shadow map.
4) По возможности биндить к одним и тем-же регистрам.
5) Установка текстур и сэмплеров - внутри шейдеров. Но возможно для оптимизации отдельной функцией, 
     что-бы не устанавливать несколько раз.

 Интерфейс шейдерного хранилища: 
  с програмной стороны - на входе название шейдера и дефайны, 
						 на выходе шейдер и биндинг констант.

  при компиляции - большой текстовый файл, с перечислением, файлов и какие константы какие значения принимают.

 Использование - установки в флагах/цифрах преобразуются в шейдер. 
                 Биндинг констант в пределах одного шейдера чисто статический.
		 Флаги - многомерный массив, индексы в котором так-же биндятся.

 При компиляции - shadername.ext /Ddefine[=off,define1,define2]
 Утилита должна понимать, что файл или его инклюды изменились.
 
 ////////////////////

*/
