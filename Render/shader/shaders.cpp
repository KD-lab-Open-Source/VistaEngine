#include "StdAfxRD.h"
#include "D3DRender.h"
#include "shaders.h"
#include "UnkLight.h"
#include "scene.h"

#define VAR_HANDLE(x) GetVariableByName(x,#x);
#define VAR_INDEX(x) x=shader->GetIndexHandle(#x)

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

void cD3DRender::SetVertexShaderConstant(int start,const D3DXMATRIX *pMat)
{
	RDCALL(lpD3DDevice->SetVertexShaderConstantF(start,(float*)pMat,4));
}

void cD3DRender::SetVertexShaderConstant(int start,const D3DXVECTOR4 *pVect)
{
	RDCALL(lpD3DDevice->SetVertexShaderConstantF(start,(float*)pVect,1));
}

void cD3DRender::SetPixelShaderConstant(int start,const D3DXVECTOR4 *pVect)
{
	RDCALL(lpD3DDevice->SetPixelShaderConstantF(start,(float*)pVect,1));
}

cShader::cShader()
{
	MTG();
	all_shader.push_back(this);
}

cShader::~cShader()
{
	vector<cShader*>::iterator it=
	find(all_shader.begin(),all_shader.end(),this);
	if(it!=all_shader.end())
		all_shader.erase(it);
	else
		VISASSERT(0);
}

cVertexShader::cVertexShader()
{
	pShaderInfo=NULL;
	ShaderInfoSize=0;
	shader=NULL;
}

cVertexShader::~cVertexShader()
{
	Delete();
}

void cVertexShader::Select()
{
	gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
}

void cVertexShader::GetHandle()
{
}


void cVertexShader::Delete()
{
	RELEASE(shader);
}

void cVertexShader::Restore()
{
	Delete();
	RestoreShader();
	GetHandle();
}

void cVertexShader::LoadShader(const char* name)
{
	shader=gb_RenderDevice3D->GetShaderLib()->Get(name);
	xassert_s(shader,name);
}

void cPixelShader::LoadShader(const char* name)
{
	shader=gb_RenderDevice3D->GetShaderLib()->Get(name);
	xassert_s(shader,name);
}

inline void cVertexShader::SetMatrix(const SHADER_HANDLE& h,const D3DXMATRIX* mat)
{
	if(h.num_register)
	{
		D3DXMATRIX out;
		D3DXMatrixTranspose(&out,mat);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->
			SetVertexShaderConstantF(h.begin_register,(float*)&out,h.num_register));
	}
}

inline void cVertexShader::SetMatrix4x4(const SHADER_HANDLE& h,int index,const D3DXMATRIX* mat)
{
	const int size=4;
	if(h.num_register)
	{
		D3DXMATRIX out;
		D3DXMatrixTranspose(&out,mat);
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->
			SetVertexShaderConstantF(h.begin_register+index*size,(float*)&out,size));
	}
}

inline void cVertexShader::SetMatrix4x3(const SHADER_HANDLE& h,int index,const D3DXMATRIX* mat)
{
	const int size=3;
	if(h.num_register)
	{
		D3DXMATRIX out;
		D3DXMatrixTranspose(&out,mat);
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->
			SetVertexShaderConstantF(h.begin_register+index*size,(float*)&out,size));
	}
}

void cVertexShader::SetMatrix4x3(const SHADER_HANDLE& h,int index,const MatXf& mat)
{
	const int size=3;
	if(h.num_register)
	{
		D3DXMATRIX out;
		cD3DRender_SetMatrixTranspose4x3(out,mat);
		xassert(index*size<=h.num_register);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->
			SetVertexShaderConstantF(h.begin_register+index*size,(float*)&out,size));
	}
}

inline void cVertexShader::SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* vect)
{
	if(h.num_register)
	{
		gb_RenderDevice3D->SetVertexShaderConstant(h.begin_register,vect);
//		pConstantTable->SetVector(gb_RenderDevice3D->lpD3DDevice,h,vect);
	}
}

inline void cPixelShader::SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* vect)
{
	if(h.num_register)
	{
		gb_RenderDevice3D->SetPixelShaderConstant(h.begin_register,vect);
	}
}
inline void cPixelShader::SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* vect,const int count)
{
	if(h.num_register)
	{
		RDCALL(gb_RenderDevice3D->lpD3DDevice->SetPixelShaderConstantF(h.begin_register,(float*)vect,count));
	}
}

inline void cVertexShader::SetFloat(const SHADER_HANDLE& h,const float vect)
{
	if(h.num_register)
	{
		gb_RenderDevice3D->SetVertexShaderConstant(h.begin_register,&D3DXVECTOR4(vect,vect,vect,vect));
	}
}

void cPixelShader::GetVariableByName(SHADER_HANDLE& sh,const char* name)
{
	sh=shader->GetConstHandle(name);
}

void cVertexShader::GetVariableByName(SHADER_HANDLE& sh,const char* name)
{
	sh=shader->GetConstHandle(name);
}

/////////////////////////////////cPixelShader/////////////////////////////
cPixelShader::cPixelShader()
{
	shader=NULL;
}

cPixelShader::~cPixelShader()
{
	Delete();
}

void cPixelShader::Select()
{
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void cPixelShader::Delete()
{
	RELEASE(shader);
}

void cPixelShader::Restore()
{
	Delete();
	RestoreShader();
	GetHandle();
}

void cPixelShader::GetHandle()
{
}

////////////////////////////////////////////////////////

void VSShadow::Select(const MatXf& world)
{
	D3DXMATRIX mat;
	cD3DRender_SetMatrix(mat,world);
	D3DXMatrixMultiply(&mat,&mat,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	SetMatrix(mWVP,&mat);

	cVertexShader::Select();
}

void VSShadow::RestoreShader()
{
	LoadShader("Minimal\\tile_map_shadow.vsl");
	INDEX_HANDLE SHADOW_9700;
	VAR_INDEX(SHADOW_9700);
	shader->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
}

void VSShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mWVP);
}

void PSShadow::RestoreShader()
{
	LoadShader("Minimal\\tile_map_shadow.psl");
	INDEX_HANDLE SHADOW_9700;
	VAR_INDEX(SHADOW_9700);
	shader->Select(SHADOW_9700,gb_RenderDevice3D->dtAdvanceID==DT_RADEON9700?1:0);
}



void PSShowMap::RestoreShader()
{
	LoadShader("Minimal\\ShowMap9700.psl");
}

void PSShowAlpha::RestoreShader()
{
	LoadShader("Minimal\\ShowAlpha.psl");
}


void VSChaos::Select(float umin,float vmin,float umin2,float vmin2,
					 float umin_b0,float vmin_b0,float umin_b1,float vmin_b1)
{
	SetFog();
	SetMatrix(mWVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	SetVector(mUV,&D3DXVECTOR4(umin,vmin,umin2,vmin2));
	SetVector(mUVBump,&D3DXVECTOR4(umin_b0,vmin_b0,umin_b1,vmin_b1));
	SetMatrix(mView,gb_RenderDevice3D->GetDrawNode()->matView);

	cVertexShader::Select();
}

void VSChaos::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mWVP);
	VAR_HANDLE(mUV);
	VAR_HANDLE(mUVBump);
	VAR_HANDLE(mView);
	VAR_HANDLE(vFog);
}

void VSChaos::RestoreShader()
{
	LoadShader("chaos\\chaos.vsl");
	INDEX_HANDLE VERTEX_FOG;
	VAR_INDEX(VERTEX_FOG);
	shader->Select(VERTEX_FOG,gb_RenderDevice3D->bSupportTableFog?0:1);
}

void PSChaos::RestoreShader()
{
	LoadShader("Chaos\\chaos.psl");
}


void VSChaos::SetFog()
{
	SetVector(vFog,&gb_RenderDevice3D->GetVertexFogParam());
}

void VSTileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(mWVP);
	VAR_HANDLE(vLightDirection);
	VAR_HANDLE(UV);
	VAR_HANDLE(UVbump);
	VAR_HANDLE(vColor);
	VAR_HANDLE(mulMiniTexture);
	VAR_HANDLE(vReflectionMul);
	VAR_INDEX(ZREFLECTION);
}

void VSTileMapScene::SetReflectionZ(bool reflection)
{
	if(ZREFLECTION.is())
		shader->Select(ZREFLECTION,reflection?1:0);
}

void PSTileMapScene::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(ZREFLECTION);
}

void PSTileMapScene::SetReflectionZ(bool reflection)
{
	if(ZREFLECTION.is())
		shader->Select(ZREFLECTION,reflection?1:0);
}

void VSTileMapScene::Select()
{
	SetFog();
	SetMatrix(mWVP,(D3DXMATRIX*)gb_RenderDevice3D->GetDrawNode()->matViewProj);
	//cVertexShader::Select();
	gb_RenderDevice3D->SetVertexShader(shader->GetVertexShader());
}

void VSTileMapScene::SetUV(Vect2f& uv_base,Vect2f& uv_step,Vect2f& uv_base_bump,Vect2f& uv_step_bump)
{
	D3DXVECTOR4 uv(uv_base.x,uv_base.y,uv_step.x,uv_step.y);
	SetVector(UV,&uv);
	D3DXVECTOR4 uv_bump(uv_base_bump.x,uv_base_bump.y,uv_step_bump.x,uv_step_bump.y);
	SetVector(UVbump,&uv_bump);
}

void VSTileMapScene::SetWorldSize(Vect2f sz)
{
	SetVector(fPlanarNode,&gb_RenderDevice3D->planar_node_size);
	
	D3DXVECTOR4 reflection_mul(gb_RenderDevice3D->tilemap_inv_size.x,gb_RenderDevice3D->tilemap_inv_size.y,1,0);
	SetVector(vReflectionMul,&reflection_mul);

	Vect3f l;
	gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
	SetVector(vLightDirection,&D3DXVECTOR4(l.x,l.y,l.z,0));
}

void VSTileMapScene::SetColor(const sColor4f& scolor)
{
	SetVector(vColor,(D3DXVECTOR4*)&scolor);
}

void SetShadowMatrix(cVertexShader* shader,SHADER_HANDLE& mShadow,const D3DXMATRIX& matlight)
{
	float bias=0;//=0.005f)
	//Для шейдера Radeon D3DRS_SLOPESCALEDEPTHBIAS не будет иметь эффекта.
	if(Option_ShadowTSM)
	{
		//bias=1e-6f;
		bias=1e-4f;
		//bias=0;
	}else
	{
		if(gb_RenderDevice3D->dtAdvance && gb_RenderDevice3D->dtAdvance->GetID()==DT_RADEON9700)
		{
			bias=0.004f;
			//bias=0;
		}else
		{
			bias=0.001f;
			//bias=0;
		}
	}

	D3DXMATRIX mat;
	float inv_shadow_map_size=gb_RenderDevice3D->GetInvShadowMapSize();
//* 4
	float fOffsetX = 0.5f + (0.5f *inv_shadow_map_size);
	float fOffsetY = 0.5f + (0.5f *inv_shadow_map_size);
/*/ //16
	float fOffsetX = 0.5f + (0.0f / shadow_map_size);
	float fOffsetY = 0.5f + (0.0f / shadow_map_size);
/**/
	float fBias    = -bias;
	D3DXMATRIX matTexAdj( 0.5f,     0.0f,     0.0f,  0.0f,
		                  0.0f,    -0.5f,     0.0f,  0.0f,
						  0.0f,     0.0f,	  1,     0.0f,
						  fOffsetX, fOffsetY, fBias, 1.0f );


	D3DXMatrixMultiply(&mat, &matlight, &matTexAdj);

	shader->SetMatrix(mShadow,&mat);
}

void vsSceneShader::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vFog);
	VAR_HANDLE(mView);
	VAR_HANDLE(fPlanarNode);

	VAR_INDEX(FOG_OF_WAR);
}

void vsSceneShader::SetFog()
{
	SetMatrix(mView,gb_RenderDevice3D->GetDrawNode()->matView);
	SetVector(vFog,&gb_RenderDevice3D->GetVertexFogParam());

	SetVector(fPlanarNode,&gb_RenderDevice3D->planar_node_size);
	shader->Select(FOG_OF_WAR,gb_RenderDevice3D->GetFogOfWar()?1:0);
}

psSceneShader::psSceneShader()
{
}

void psSceneShader::GetHandle()
{
	VAR_HANDLE(vFogOfWar);
	VAR_HANDLE(vShade);
	VAR_INDEX(FOG_OF_WAR);
}

void psSceneShader::SetFog()
{
	if(gb_RenderDevice3D->GetFogOfWar())
	{
		SetVector(vFogOfWar,&gb_RenderDevice3D->fog_of_war_color);
		shader->Select(FOG_OF_WAR,1);
	}else
	{
		shader->Select(FOG_OF_WAR,0);
	}
}

void psSceneShader::SetShadowIntensity(const sColor4f& f)
{
	//f=f*f; //??????????????????????
	SetVector(vShade,(D3DXVECTOR4*)&f);
}


void ContainerMiniTextures::SetTexture(int i, const char* fn)
{
	xassert((UINT)i<pTextures.size());
	if(i>=pTextures.size())//временно, для конвертации объектов.
		return;
	minitexture_file_name[i] = fn;
	RELEASE(pTextures[i]);
	pTextures[i] = NULL;
	//pTextures[i] = GetTexLibrary()->GetElement(minitexture_file_name[i].c_str());
	pTextures[i] = GetTexLibrary()->GetElement3D(minitexture_file_name[i].c_str());
	pTextures[i]->SetAttribute(TEXTURE_DISABLE_DETAIL_LEVEL);
	GetTexLibrary()->ReLoadTexture(pTextures[i]);
}
void ContainerMiniTextures::Resize(int sz)
{
	for(int i=0; i<pTextures.size(); i++)
		RELEASE(pTextures[i]);
	minitexture_file_name.resize(sz, string("Scripts\\Resource\\balmer\\noise.tga"));
	pTextures.resize(sz);
}
cTexture* ContainerMiniTextures::Texture(int i)
{
	if(i==-1)
	{
		xassert(0);
		return NULL;
	}

	xassert((UINT)i<pTextures.size());

	if(pTextures[i]==0 && !minitexture_file_name[i].empty())
	{
		//pTextures[i] = GetTexLibrary()->GetElement(minitexture_file_name[i].c_str());
		pTextures[i] = GetTexLibrary()->GetElement3D(minitexture_file_name[i].c_str());
		if(pTextures[i]==0)
			minitexture_file_name[i].clear();
	}

	cTexture *ret = pTextures[i];
	return ret;
}
ContainerMiniTextures::~ContainerMiniTextures()
{
	for(int i=0; i<pTextures.size(); i++)
		RELEASE(pTextures[i]);
}

PSTileMapScene::PSTileMapScene()
{
}

PSTileMapScene::~PSTileMapScene()
{
}

void VSTileMapScene::SetMiniTextureSize(int dx,int dy, float res)
{
	D3DXVECTOR4 mul(res/dx,res/dy,0,0);
	SetVector(mulMiniTexture,&mul);
}

void PSTileMapScene::Select(int matarial_num,VSTileMapScene* vs,float res)
{
	if(Option_DetailTexture)
	{
		cScene *scene = gb_RenderDevice3D->GetDrawNode()->GetScene();
		cTexture* pTexture=scene->GetTilemapDetailTextures().Texture(matarial_num);
		gb_RenderDevice3D->SetTexture(4,pTexture);
		if(pTexture)
			vs->SetMiniTextureSize(pTexture->GetWidth(),pTexture->GetHeight(),res);
	}

	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void VSGrass::Select(float time_,float hideDistance_, sColor4f sunDiffuse_, bool toZBuffer)
{
	Vect3f p=gb_RenderDevice3D->GetDrawNode()->GetPos();
	SetVector(vCameraPos,&D3DXVECTOR4(p.x,p.y,p.z,0));
	Vect3f l;
	gb_RenderDevice3D->GetDrawNode()->GetLighting(l);
	SetVector(vLightDirection,&D3DXVECTOR4(l.x,l.y,l.z,0));
	SetVector(time,&D3DXVECTOR4(time_,time_,time_,0));
	SetVector(hideDistance,&D3DXVECTOR4(hideDistance_,hideDistance_,hideDistance_,0));
	SetVector(SunDiffuse,&D3DXVECTOR4(sunDiffuse_.r,sunDiffuse_.g,sunDiffuse_.b,sunDiffuse_.a));
	gb_RenderDevice3D->dtAdvance->SetShadowMapTexture();
	SetShadowMatrix(this,mShadow,gb_RenderDevice3D->GetShadowMatViewProj());
	if(LIGHTMAP.is())
		shader->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);
	if(OLD_LIGHTING.is())
		shader->Select(OLD_LIGHTING,oldLighting);
	if (ZBUFFER.is())
		shader->Select(ZBUFFER,toZBuffer);

	SetFog();
	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	D3DXMATRIX mat;
	cD3DRender_SetMatrixTranspose4x3(mat,gb_RenderDevice3D->GetDrawNode()->GetMatrix());
	SetMatrix(mWorld,&mat);
	__super::Select();
}

void VSGrass::RestoreShader()
{
	LoadShader("grass\\grass.vsl");
	shader->BeginStaticSelect();
	shader->StaticSelect("SHADOW",1);
	shader->EndStaticSelect();
}
void VSGrass::SetOldLighting(bool enble)
{
	oldLighting = enble;
}

void VSGrass::GetHandle()
{
	VAR_HANDLE(time);
	VAR_HANDLE(hideDistance);
	VAR_HANDLE(SunDiffuse);
	VAR_HANDLE(vCameraPos);
	VAR_HANDLE(vLightDirection);
	VAR_HANDLE(mShadow);
	VAR_INDEX(LIGHTMAP);
	VAR_INDEX(OLD_LIGHTING);
	VAR_HANDLE(mVP);
	VAR_HANDLE(mWorld);
	VAR_INDEX(ZBUFFER);
	VAR_HANDLE(vDofParams);
	__super::GetHandle();
}
void PSGrass::Select()
{
	if(LIGHTMAP.is())
		shader->Select(LIGHTMAP,gb_RenderDevice3D->GetTexture(3)?1:0);
	SetFog();
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSGrass::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(LIGHTMAP);
}

void PSGrass::RestoreShader()
{
	LoadShader("grass\\grass.psl");
}
void PSGrassShadow::Select()
{
	SelectShadowQuality();
	SetVector(fx_offset,&D3DXVECTOR4(gb_RenderDevice3D->GetInvShadowMapSize()*0.5f,0,0,0));
	__super::Select();
}
void PSGrassShadow::SelectShadowQuality()
{
	if(c2x2.is())
		shader->Select(c2x2,1);
}

void PSGrassShadow::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(vShade);
	VAR_HANDLE(fx_offset);
	VAR_INDEX(c2x2);
}

void PSGrassShadow::RestoreShader()
{
	LoadShader("grass\\grass_shadow9700.psl");
}
void PSGrassShadowFX::RestoreShader()
{
	LoadShader("grass\\grass_shadowFX.psl");
}

void PSMiniMap::Select(const sColor4f& waterColor_,const sColor4f& terraColor_)
{
	SetVector(waterColor,&D3DXVECTOR4(waterColor_.r,waterColor_.g,waterColor_.b,waterColor_.a));
	SetVector(terraColor,&D3DXVECTOR4(terraColor_.r,terraColor_.g,terraColor_.b,terraColor_.a));
	SetVector(fogAlpha,&D3DXVECTOR4(fogAlpha_,fogAlpha_,fogAlpha_,fogAlpha_));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSMiniMap::RestoreShader()
{
	LoadShader("miniMap.psl");
}
void PSMiniMap::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(waterColor);
	VAR_HANDLE(terraColor);
	VAR_HANDLE(fogAlpha);
	VAR_INDEX(USE_WATER);
	VAR_INDEX(USE_FOGOFWAR);
	VAR_INDEX(USE_TERRA_COLOR);
}

void PSZBuffer::RestoreShader()
{
	LoadShader("ZBuffer\\ZBuffer.psl");
}
void PSZBuffer::Select(bool floatZBufer)
{
	if (FLOATZBUFFER.is())
		shader->Select(FLOATZBUFFER,floatZBufer?1:0);
	__super::Select();
}
void PSZBuffer::GetHandle()
{
	__super::GetHandle();
	VAR_INDEX(FLOATZBUFFER);
}
void PSSkinZBuffer::RestoreShader()
{
	LoadShader("ZBuffer\\object_zbuffer.psl");
}
void PSSkinZBufferAlpha::RestoreShader()
{
	if(gb_RenderDevice3D->dtAdvanceOriginal)
	{
		LoadShader("ZBuffer\\object_zbuffer.psl");
		INDEX_HANDLE ALPHA;
		VAR_INDEX(ALPHA);
		shader->Select(ALPHA,1);
	}
}
void VSTileZBuffer::Select()
{
	SetFog();
	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);
	__super::Select();
}
void VSTileZBuffer::RestoreShader()
{
	LoadShader("ZBuffer\\tilemap_zbuffer.vsl");
}
void VSTileZBuffer::GetHandle()
{
	VAR_HANDLE(mVP);
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
	D3DXMATRIX mat;


	SetMatrix(mVP,gb_RenderDevice3D->GetDrawNode()->matViewProj);

	SetVector(vDofParams, &D3DXVECTOR4(DofParams.x,DofParams.y,0,0));

	if (object)
	{
		for(int i=0;i<world_num;i++)
		{
			MatXf& w=world[i];
			SetMatrix4x3(mWorldM,i,w);//for skin
		}
		xassert(blend_num>=1 && blend_num<=4);
		shader->Select(WEIGHT,blend_num-1);
		shader->Select(OBJECT,1);
	}
	else
		shader->Select(OBJECT,0);

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
		SetMatrix4x3(mWorldM,i+world_offset,w);//for skin
	}
}
void VSZBuffer::RestoreShader()
{
	LoadShader("ZBuffer\\ZBuffer.vsl");
}
void VSZBuffer::GetHandle()
{
	VAR_HANDLE(mVP);
	VAR_HANDLE(mWorldM);
	VAR_HANDLE(vDofParams);
	VAR_INDEX(WEIGHT);
	VAR_INDEX(OBJECT);
	__super::GetHandle();
}
void VSSkinZBuffer::RestoreShader()
{
	LoadShader("ZBuffer\\object_zbuffer.vsl");
	VAR_INDEX(WEIGHT);

}
void PSFillColor::Select(sColor4f color)
{
	SetVector(vColor, &D3DXVECTOR4(color.r,color.g,color.b,color.a));
	__super::Select();
}
void PSFillColor::Select(float r, float g, float b, float a)
{
	SetVector(vColor, &D3DXVECTOR4(r,g,b,a));
	__super::Select();
}

void PSFillColor::RestoreShader()
{
	LoadShader("ZBuffer\\ClearAlpha.psl");
}
void PSFillColor::GetHandle()
{
	VAR_HANDLE(vColor);
	__super::GetHandle();
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
