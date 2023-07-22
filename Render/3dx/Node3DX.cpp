#include "StdAfxRD.h"
#include "node3dx.h"
#include "..\shader\shaders.h"
#include "nparticle.h"
#include "scene.h"
#include "TileMap.h"
#include "OcclusionQuery.h"
#include "..\inc\SmallCache.h"


int polygon_limitation_max_polygon=100000;
int polygon_limitation_max_polygon_simply3dx=100000;

static float AlphaMaxiumBlend=0.95f;
static float AlphaMiniumShadow=0.70f;

C3dxVisibilityGroup C3dxVisibilityGroup::BAD;
C3dxVisibilitySet C3dxVisibilitySet::BAD;
C3dxVisibilitySet C3dxVisibilitySet::ZERO(0);

/*
Оптимизация.
!! На втором и третьем LOD - уменьшать количество костей до 2 и 1
*/

Shader3dx::Shader3dx()
{
	vsSkinSceneShadow=NULL;
	psSkinSceneShadow=NULL;
	vsSkinBumpSceneShadow=NULL;
	psSkinBumpSceneShadow=NULL;
	vsSkinReflectionSceneShadow=NULL;
	psSkinReflectionSceneShadow=NULL;
	if(gb_RenderDevice3D->dtAdvanceOriginal && gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_RADEON9700)
	{
		vsSkinSceneShadow=new VSSkinSceneShadow;
		psSkinSceneShadow=new PSSkinSceneShadow;
		vsSkinBumpSceneShadow=new VSSkinBumpSceneShadow;
		psSkinBumpSceneShadow=new PSSkinBumpSceneShadow;
		vsSkinSceneShadow->Restore();
		psSkinSceneShadow->Restore();
		vsSkinBumpSceneShadow->Restore();
		psSkinBumpSceneShadow->Restore();

		vsSkinReflectionSceneShadow=new VSSkinReflectionSceneShadow;
		psSkinReflectionSceneShadow=new PSSkinReflectionSceneShadow;
		vsSkinReflectionSceneShadow->Restore();
		psSkinReflectionSceneShadow->Restore();
	}else
	if(gb_RenderDevice3D->dtAdvanceOriginal && gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_GEFORCEFX)
	{
		vsSkinSceneShadow=new VSSkinSceneShadow;
		psSkinSceneShadow=new PSSkinSceneShadowFX;
		vsSkinBumpSceneShadow=new VSSkinBumpSceneShadow;
		psSkinBumpSceneShadow=new PSSkinBumpSceneShadowFX;
		vsSkinSceneShadow->Restore();
		psSkinSceneShadow->Restore();
		vsSkinBumpSceneShadow->Restore();
		psSkinBumpSceneShadow->Restore();

		vsSkinReflectionSceneShadow=new VSSkinReflectionSceneShadow;
		psSkinReflectionSceneShadow=new PSSkinReflectionSceneShadowFX;
		vsSkinReflectionSceneShadow->Restore();
		psSkinReflectionSceneShadow->Restore();
	}

	vsSkin=new VSSkin;
	vsSkin->Restore();
	psSkin=new PSSkinNoShadow;
	psSkin->Restore();
	vsSkinBump=new VSSkinBump;
	vsSkinBump->Restore();
	psSkinBump=new PSSkinBump;
	psSkinBump->Restore();

	vsSkinNoLight=new VSSkinNoLight;
	vsSkinNoLight->Restore();
	vsSkinShadow=new VSSkinShadow;
	vsSkinShadow->Restore();
	psSkinShadow=new PSSkinShadow;
	psSkinShadow->Restore();

	psSkinShadowAlpha=new PSSkinShadowAlpha;
	psSkinShadowAlpha->Restore();

	vsSkinReflection=new VSSkinReflection;
	vsSkinReflection->Restore();
	psSkinReflection=new PSSkinReflection;
	psSkinReflection->Restore();

	vsSkinSecondOpacity = new VSSkinSecondOpacity;
	vsSkinSecondOpacity->Restore();
	psSkinSecondOpacity = new PSSkinSecondOpacity;
	psSkinSecondOpacity->Restore();

	vsSkinZBuffer = new VSSkinZBuffer();
	vsSkinZBuffer->Restore();
	psSkinZBuffer = new PSSkinZBuffer();
	psSkinZBuffer->Restore();
	psSkinZBufferAlpha = new PSSkinZBufferAlpha();
	psSkinZBufferAlpha->Restore();
}

Shader3dx::~Shader3dx()
{
	delete vsSkinSceneShadow;
	delete psSkinSceneShadow;
	delete vsSkinBumpSceneShadow;
	delete psSkinBumpSceneShadow;

	delete vsSkin;
	delete psSkin;
	delete vsSkinBump;
	delete psSkinBump;
	delete vsSkinNoLight;
	delete vsSkinShadow;
	delete psSkinShadow;
	delete psSkinShadowAlpha;
	delete vsSkinSecondOpacity;
	delete psSkinSecondOpacity;

	delete vsSkinReflection;
	delete psSkinReflection;
	delete vsSkinReflectionSceneShadow;
	delete psSkinReflectionSceneShadow;

	delete vsSkinZBuffer;
	delete psSkinZBuffer;
	delete psSkinZBufferAlpha;
}

Shader3dx* pShader3dx=NULL;

void Done3dxshader()
{
	delete pShader3dx;
	pShader3dx=NULL;
}

void Init3dxshader()
{
	Done3dxshader();
	pShader3dx=new Shader3dx;
}

bool cObject3dx::enable_use_lod=true;


class cOcclusionSilouette
{
	cOcclusionQuery occlusionQuery;
	int time_invisible;
	bool is_real_visible;
	bool is_visible_in_camera;
public:
	cOcclusionSilouette();
	~cOcclusionSilouette();
	void Begin();
	void End();
	bool IsVisible(int dt);
	void Test(cCamera* pCamera,Vect3f& pos)
	{
		is_visible_in_camera=pCamera->TestVisible(pos,0)!=VISIBLE_OUTSIDE;
		occlusionQuery.Test(pos);
	}
};

bool cObject3dx::QueryVisibleIsVisible()
{
	if(pOcclusionQuery==NULL)
		pOcclusionQuery=new cOcclusionSilouette;
	return pOcclusionQuery->IsVisible(IParent->GetDeltaTimeInt());
}

void cObject3dx::QueryVisible(cCamera* pCamera)
{
	if(pOcclusionQuery)
	{
		Vect3f pos;
		//pCamera->GetWorldK() все же не совсем то, что направление от камеры на центр объекта, но во многих случаях прокатывает.
		if(false)
		{
			Vect3f pos=position.trans()-(GetBoundRadius()*1.1f)*pCamera->GetWorldK();
			pos+=position.rot().xform((pStatic->bound_box.min+pStatic->bound_box.max)*(0.5f*position.s));
		}else
		{
			Vect3f center_pos=position.trans();
/*
			Vect3f bound_pos=position.rot().xform((pStatic->bound_box.min+pStatic->bound_box.max)*(0.5f*position.s));
			center_pos+=bound_pos;
/*/
			center_pos+=silouette_center*position.s;
/**/
			Vect3f dir=pCamera->GetPos()-center_pos;
			dir.Normalize();
			pos=center_pos+(GetBoundRadius()*1.1f)*dir;
		}

		pOcclusionQuery->Test(pCamera,pos);
	}
}

cOcclusionSilouette::cOcclusionSilouette()
{
	time_invisible=0;
	is_real_visible=true;
	is_visible_in_camera=false;
}

cOcclusionSilouette::~cOcclusionSilouette()
{
}

void cOcclusionSilouette::Begin()
{
	occlusionQuery.Begin();
}

void cOcclusionSilouette::End()
{
	occlusionQuery.End();
}

bool cOcclusionSilouette::IsVisible(int dt)
{
	bool visible=!occlusionQuery.IsInit() || occlusionQuery.IsVisible();
	visible=visible || !is_visible_in_camera;

	const int max_time=300;
	if(!visible)
	{
		time_invisible=min(time_invisible+dt,max_time);
	}else
	{
		time_invisible=max(time_invisible-dt,0);
	}

	if(time_invisible==0)
		is_real_visible=true;
	if(time_invisible==max_time)
		is_real_visible=false;
	
	return is_real_visible;
}


cNode3dx::cNode3dx()
{
	pos.Identify();
	phase=0;
	chain=0;
	index_scale=0;
	index_position=0;
	index_rotation=0;
	additional_transform=255;
}

void TestAnimationGrroup(vector<AnimationGroup>& animation_group)
{
	typedef StaticMap<int,int> NODE_MAP;
	NODE_MAP node_map;
	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=animation_group[iag];
		for(int inode=0;inode<ag.nodes.size();inode++)
		{
			int node=ag.nodes[inode];
			NODE_MAP::iterator it=node_map.find(node);
			if(it!=node_map.end())
			{
				int cur_node=it->first;
				int group=it->second;
				xassert(0);
			}
			node_map[node]=iag;
		}
	}
}

cObject3dx::cObject3dx(cStatic3dx* pStatic_,bool interpolate)
:c3dx(KIND_OBJ_3DX),cObject3dxAnimation(pStatic_)
{
	int sz=sizeof(cObject3dx);
	border_lod12_2=sqr(200);
	border_lod23_2=sqr(600);
	hideDistance = 500;

#ifdef POLYGON_LIMITATION
	polygon_limitation_num_polygon=0;
	num_out_polygons=0;
#endif
	pOcclusionQuery=NULL;
	updated=false;
	silhouette_index = 0;

	link3dx.SetParent(this);
	SetAttribute(ATTRCAMERA_REFLECTION|ATTRCAMERA_FLOAT_ZBUFFER);
	distance_alpha=1.0f;

	ambient.set(1,1,1,0);
	diffuse.set(1,1,1,0);
	specular.set(1,1,1,0);
	lerp_color.set(1,1,1,0);
	object_opacity=1;
	skin_color.set(255,255,255,0);
	position.Identify();
	isSkinColorSet_ = false;
	iLOD=0;

	iGroups.resize(pStatic->visible_sets.size());
	for(int ig=0;ig<iGroups.size();ig++)
	{
		iGroups[ig].i.igroup=0;
		iGroups[ig].p=0;
		iGroups[ig].alpha=1;
	}
	UpdateVisibilityGroups();

	material_textures.resize(pStatic->materials.size());
	for(int i=0;i<material_textures.size();i++)
	{
		MaterialTexture& m=material_textures[i];
		m.diffuse_texture=0;
		m.texture_phase=0;
		m.is_opacity_vg=false;
	}

	pAnimSecond=NULL;
	if(interpolate)//Переделать GetAllPoints
		pAnimSecond=new cObject3dxAnimationSecond(pStatic);

	if(!pStatic->is_inialized_bound_box)
	{
		pStatic->is_inialized_bound_box=true;
		if(!pStatic->is_logic)
			CalcBoundingBox();
		else
		{
			pStatic->bound_box=pStatic->logic_bound.bound;
			pStatic->radius=pStatic->bound_box.max.distance(pStatic->bound_box.min)*0.5f;
		}
	}
	isHaveSkinColorMaterial_ = false;
	for(int i =0; i<pStatic->materials.size(); i++)
	{
		if(pStatic->materials[i].is_skinned)
			isHaveSkinColorMaterial_=  true;
	}

	LoadTexture(false);
	effects.resize(pStatic->effects.size());

	SetPosition(MatXf::ID);
	Update();

	silouette_center=(pStatic->bound_box.min+pStatic->bound_box.max)*0.5f;
//	TestAnimationGrroup(pStatic->animation_group);
}
cObject3dx::cObject3dx(cObject3dx* pObj)
:c3dx(KIND_OBJ_3DX),cObject3dxAnimation(pObj)
{
#ifdef POLYGON_LIMITATION
	polygon_limitation_num_polygon=0;
	num_out_polygons=0;
#endif
	border_lod12_2=pObj->border_lod12_2;
	border_lod23_2=pObj->border_lod23_2;
	hideDistance = pObj->hideDistance;

	pOcclusionQuery=NULL;
	updated=false;
	silhouette_index = pObj->silhouette_index;
	pStatic->AddRef();
	SetAttr(pObj->GetAttr());
	ClearAttr(ATTRUNKOBJ_ATTACHED);
	ambient = pObj->ambient;
	diffuse = pObj->diffuse;
	specular = pObj->specular;
	lerp_color=pObj->lerp_color;
	object_opacity=pObj->object_opacity;
	position = pObj->position;
	iLOD = pObj->iLOD;
	iGroups = pObj->iGroups;
	isHaveSkinColorMaterial_ = pObj->isHaveSkinColorMaterial_;
	isSkinColorSet_ = pObj->isSkinColorSet_;
	material_textures = pObj->material_textures;
	vector<MaterialTexture>::iterator itmat;
	FOR_EACH(material_textures, itmat)
		if (itmat->diffuse_texture)
			itmat->diffuse_texture->AddRef();
	skin_color = pObj->skin_color;
	additional_transformations = pObj->additional_transformations;
	distance_alpha=pObj->distance_alpha;

	if (pObj->pAnimSecond)
		pAnimSecond=new cObject3dxAnimationSecond(pObj->pAnimSecond);
	else pAnimSecond = NULL;
	
	effects.resize(pStatic->effects.size());
	SetPosition(pObj->GetPosition());
	silouette_center=pObj->silouette_center;
}

cObject3dx::~cObject3dx()
{
	int i;
	for(i=0;i<material_textures.size();i++)
	if(material_textures[i].diffuse_texture)
		material_textures[i].diffuse_texture->Release();
	material_textures.clear();
	for(i=0;i<lights.size();i++)
	{
		RELEASE(lights[i]);
	}
	lights.clear();

	for(i=0;i<effects.size();i++)
	if(effects[i].pEffect)
		effects[i].pEffect->StopAndReleaseAfterEnd();
	effects.clear();
	pStatic->Release();

	delete pOcclusionQuery;
	delete pAnimSecond;

	xassert(gb_RenderDevice3D);
	xassert(GetRef()==0);
}

void cObject3dx::SetScale(float scale_)
{
	updated=false;
	if(pAnimSecond)
		pAnimSecond->updated=false;
	position.s=scale_;

//	if(GetBoundRadius()>1000)
//		console()<< cConsole::LOW << "&Balmer" << "Big object " <<pStatic->file_name << cConsole::END;

	//Фикс (кривой) для постоянно генерирующихся эффектов
	vector<EffectData>::iterator it;
	FOR_EACH(effects,it)
	{
		EffectData& d=*it;
		RELEASE(d.pEffect);
	}

	for(int i=0;i<lights.size();i++)
	{
		cStaticLights& sl=pStatic->lights[i];
		lights[i]->GetRadius()=lights[i]->GetRealRadius()=sl.atten_end*GetScale();
	}

}

void cObject3dx::SetPosition(const Se3f&  pos)
{
	MTAccess();
#ifdef _DEBUG
	xassert(fabsf(pos.rot().norm2()-1)<1.5e-2f);
#endif
	updated=false;
	if(pAnimSecond)
		pAnimSecond->updated=false;
	position.se()=pos;
}

void cObject3dx::SetPosition(const MatXf& pos)
{
	updated=false;
#ifdef _DEBUG
	float s=pos.rot().xrow().norm();
	xassert(fabsf(s-1)<1e-2f);
#endif
	if(pAnimSecond)
		pAnimSecond->updated=false;
//	xassert(0);
	position.se().set(pos);
}

const MatXf& cObject3dx::GetPosition() const
{
	static MatXf mat;
	mat.set(position.se());
//	xassert(0);
	return mat;
}

const Se3f& cObject3dx::GetPositionSe() const
{
	return position.se();
}

const Mats& cObject3dx::GetPositionMats() const
{
	return position;
}

cStaticVisibilityChainGroup* cObject3dx::GetVisibilityGroup(C3dxVisibilitySet iset)
{
	VISASSERT(iset.iset>=0 && iset.iset<pStatic->visible_sets.size());
	return iGroups[iset.iset].p;
}

C3dxVisibilityGroup cObject3dx::GetVisibilityGroupIndex(C3dxVisibilitySet iset)
{
	xassert(iset.iset>=0 && iset.iset<iGroups.size());
	return iGroups[iset.iset].i;
}


void cObject3dx::SetAttr(int attribute)
{
	if(attribute&ATTR3DX_HIDE_LIGHTS)
		updated=false;

	if(attribute&ATTRUNKOBJ_IGNORE)
	{
		for(int i=0;i<lights.size();i++)
			lights[i]->SetAttr(ATTRUNKOBJ_IGNORE);
#ifdef POLYGON_LIMITATION
		num_out_polygons=0;
#endif
	}

	__super::SetAttr(attribute);
}

void cObject3dx::ClearAttr(int attribute)
{
	if(attribute&ATTR3DX_HIDE_LIGHTS)
		updated=false;
	__super::ClearAttr(attribute);
}


void cObject3dx::Update()
{
	if(pAnimSecond)
	{
		if(updated && pAnimSecond->updated)
			return;
	}else
	{
		if(updated)
			return;
	}

	if(!GetAttribute(ATTR3DX_NOUPDATEMATRIX))
	{
/*
		UpdateMatrix(position,additional_transformations);
		if(pAnimSecond)
		{
			if(pAnimSecond->IsInterpolation())
			{
				pAnimSecond->UpdateMatrix(position,additional_transformations);
				pAnimSecond->lerp(this);
			}
		}
/*/
		if(pAnimSecond && pAnimSecond->IsInterpolation())
			UpdateAndLerp(position,additional_transformations);
		else
			UpdateMatrix(position,additional_transformations);
/**/
	}

	if(GetScene())//update lights
	{
		bool first=lights.empty() && !pStatic->lights.empty();
		if(first)
		{
			lights.resize(pStatic->lights.size());
			for(int i=0;i<lights.size();i++)
			{
				cStaticLights& sl=pStatic->lights[i];
				if(sl.pTexture)
				{
					sl.pTexture->AddRef();
					lights[i]=GetScene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_SPRITE,sl.pTexture);
				}else
				{
					cTexture* sperical=GetTexLibrary()->GetSpericalTexture();
					lights[i]=GetScene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
				}
				
				
				lights[i]->SetDiffuse(sl.color);
				lights[i]->GetRadius()=lights[i]->GetRealRadius()=sl.atten_end*GetScale();
			}
		}

		cStaticVisibilityChainGroup* pGroup=GetVisibilityGroup();
		for(int i=0;i<lights.size();i++)
		{
			cStaticLights& sl=pStatic->lights[i];
			cNode3dx& node=nodes[sl.inode];
			if(!sl.chains.empty())
			{
				cStaticLightsAnimation& chain=sl.chains[node.chain];
				sColor4f color;
				chain.color.InterpolateSlow(node.phase,(float*)&color);
				color.r=clamp(color.r,0.0f,1.0f);
				color.g=clamp(color.g,0.0f,1.0f);
				color.b=clamp(color.b,0.0f,1.0f);
				color.a=clamp(color.a,0.0f,1.0f);
				lights[i]->SetDiffuse(color);
			}

			MatXf xpos;
			node.pos.copy_right(xpos);
			if(GetAttr(ATTRUNKOBJ_ATTACHED))
				lights[i]->SetPosition(xpos);

			//Ночью должен быть lightmap!
			bool is_group_visible=pGroup->visible_nodes[sl.inode];

			lights[i]->PutAttribute(ATTRUNKOBJ_IGNORE,GetAttribute(ATTR3DX_HIDE_LIGHTS|ATTRUNKOBJ_IGNORE) || !is_group_visible);
		}

		if(first)
		{
			for(int i=0;i<lights.size();i++)
				lights[i]->Attach();
		}
	}
}

void cObject3dx::DrawLine(cCamera* pCamera)
{
	Update();
	sColor4c color(255,255,255);
	sColor4c c0(255,0,0);
	sColor4c c_invisible(0,0,255);
	int size=nodes.size();
	for(int i=1;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		cStaticNode& sn=pStatic->nodes[i];
		cNode3dx& parent=nodes[sn.iparent];

		bool visible=GetVisibilityTrack(i);
		if(visible)
			gb_RenderDevice->DrawLine(parent.pos.trans(),node.pos.trans(),sn.iparent==0?c0:color);
		else
			gb_RenderDevice->DrawLine(parent.pos.trans(),node.pos.trans(),c_invisible);
	}
}

void cObject3dx::Draw(cCamera* pCamera)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
#ifdef POLYGON_LIMITATION
	polygon_limitation_num_polygon=0;
#endif
	if(!isSkinColorSet_ && IsHaveSkinColorMaterial())
	{
		VisError << "Для модели " << pStatic->file_name.c_str() << " не назначен Skin Color.\nБудет назначен Skin Color по умолчанию" << VERR_END;
		SetSkinColor(sColor4c(255,255,255,255),NULL);
	}

//	DrawLine(pCamera);
	Update();

	if(pCamera->GetAttribute(ATTRCAMERA_SHADOWMAP))
	{
		DrawShadowAndZbuffer(pCamera,false);
		return;
	}
	if(pCamera->GetAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
	{
		if(Option_FloatZBufferType==2)
			DrawShadowAndZbuffer(pCamera,true);
		return;
	}

	sDataRenderMaterial material;
	material.lerp_texture=lerp_color;
	material.point_light=&point_light;

	{
		gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);
		gb_RenderDevice3D->SetTextureBase(4,NULL);
	}

	//Из плюсов - выводит стабильное количество полигонов вне зависимости
	//от количества подобъектов. Из минусов - для больших объектов не слишком
	//эффективен. indexed 14 mtrtis, nonindexed 26 mtris - пиковые значения на FX 5950.
	bool is_shadow=pCamera->IsShadow();

//#define DEBUG_COLORS
#ifdef DEBUG_COLORS
	sColor4f colors[]=
	{
		sColor4f(0,0,1),
		sColor4f(0,1,0),
		sColor4f(1,0,0),
		sColor4f(1,1,0),
		sColor4f(1,0,1),
		sColor4f(0,1,1),

		sColor4f(1,0.5f,0),
		sColor4f(1,0,0.5f),
		sColor4f(0,1,0.5f),
		sColor4f(0.5f,1,0),
		sColor4f(0.5f,0,1),
		sColor4f(0,0.5f,1),
	};
	int color_size=sizeof(colors)/sizeof(colors[0]);
	{
		for(int i=0;i<color_size;i++)
		{
			sColor4f& c=colors[i];
			float m=0.6f;
			c.r*=m;c.g*=m;c.b*=m;
		}
	}
#endif

	DWORD old_zfunc;
	DWORD old_zwriteble;
	DWORD old_color;


	bool draw_opacity=pCamera->GetCameraPass()==SCENENODE_OBJECTSORT;
	bool draw2passes = GetAttribute(ATTRUNKOBJ_2PASS_ZBUFFER);//(pCamera->GetCameraPass()==SCENENODE_OBJECT_2PASS)||(pCamera->GetCameraPass()==SCENENODE_ZPASS);
	if((draw_opacity || draw2passes))
	{
		old_zfunc = gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
		old_zwriteble = gb_RenderDevice3D->GetRenderState(D3DRS_ZWRITEENABLE);
		old_color = gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	}

	//!!! Не забыть сортировку по материалам.
	int size=lod.skin_group.size();
	static MatXf world[cStaticIndex::max_index];
	int world_num;
	for(int iskin_group=0;iskin_group<size;iskin_group++)
	{
		cStaticIndex& s=lod.skin_group[iskin_group];
		if(!IsVisibleMaterialGroup(s))
			continue;

		cStaticMaterial& mat=pStatic->materials[s.imaterial];
		MaterialAnim& mat_anim=materials[s.imaterial];
		cTexture* diffuse_texture=material_textures[s.imaterial].diffuse_texture;
		float texture_phase=material_textures[s.imaterial].texture_phase;
		bool is_opacity_vg=material_textures[s.imaterial].is_opacity_vg;
//*
		if(mat.no_light)
		{
			material.Ambient.interpolate3(mat.ambient,ambient,ambient.a);

			material.Diffuse.r=0;
			material.Diffuse.g=0;
			material.Diffuse.b=0;

			material.Specular.set(0,0,0,0);
		}else
		{
			material.Ambient.interpolate3(mat.ambient,ambient,ambient.a);
			material.Diffuse.interpolate3(mat.diffuse,diffuse,diffuse.a);
			material.Specular.interpolate3(mat.specular,specular,specular.a);
		}

		material.Ambient.a=
		material.Diffuse.a=mat_anim.opacity*object_opacity*distance_alpha;

/*/
		material.Ambient=mat.ambient;
		material.Diffuse=mat.diffuse;
		material.Specular=mat.specular;
/**/
		material.Tex[0]=diffuse_texture;

		gb_RenderDevice3D->SetSamplerData(0,(mat.tiling_diffuse&cStaticMaterial::TILING_U_WRAP)?sampler_wrap_anisotropic:sampler_clamp_anisotropic);


		if(mat.is_reflect_sky)
			material.Tex[1]=IParent->GetSkyCubemap();
		else
			material.Tex[1]=mat.pReflectTexture?mat.pReflectTexture:mat.pBumpTexture;
//*
		material.Diffuse.mul3(material.Diffuse,GetScene()->GetSunDiffuse());
		material.Ambient.mul3(material.Ambient,GetScene()->GetSunAmbient());
		material.Specular.mul3(material.Specular,GetScene()->GetSunSpecular());
/*/
		material.Diffuse=GetScene()->GetSunDiffuse();
		material.Ambient=GetScene()->GetSunAmbient();
		material.Specular.set(0,0,0,0);
/**/
		material.Specular.a=mat.specular_power;

#ifdef DEBUG_COLORS
		material.Ambient=
		material.Diffuse=
		material.Specular=colors[iskin_group%color_size];
		material.Tex[0]=NULL;
#endif

		eBlendMode blend=ALPHA_NONE;
		if(material.Tex[0])
		{
			cTexture* pTex0=material.Tex[0];
			//
			if(pTex0->IsAlphaTest())
			{
				blend=ALPHA_TEST;
			}
		}

		if(is_opacity_vg)
		{
			//Рисовать прозрачную и непрозрачную часть. Выставлять.
			if(draw_opacity)
				blend=ALPHA_BLEND;
		}else
		{
			if(material.Diffuse.a<AlphaMaxiumBlend || mat.is_opacity_texture)
			{
				blend=ALPHA_BLEND;
				if(!draw_opacity&&!draw2passes)
					continue;
			}else
			{
				if(draw_opacity)
					continue;
			}
		}

		switch(mat.transparency_type) {
			case cStaticMaterial::TRANSPARENCY_ADDITIVE:
				blend = ALPHA_ADDBLENDALPHA; /// dst=dst+src*alpha
				break;
					
			case cStaticMaterial::TRANSPARENCY_SUBSTRACTIVE:
				blend = ALPHA_SUBBLEND; /// dst=dst-src
				break;
		}

		gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
//*
//		if(i==0)
		{
			gb_RenderDevice3D->SetTexturePhase(0,material.Tex[0],texture_phase);
			gb_RenderDevice3D->SetTexturePhase(1,material.Tex[1],texture_phase);
		}
/*/
		gb_RenderDevice3D->SetTextureBase(0,NULL);
		gb_RenderDevice3D->SetTextureBase(1,NULL);
/**/

		VSSkin* vs=NULL;
		PSSkin* ps=NULL;

		if(IParent->GetTileMap())
		{
			gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->dtAdvance->GetLightMapObjects());
		}else
			gb_RenderDevice3D->SetTextureBase(3,NULL);

		if(mat.pSecondOpacityTexture)
		{
			vs=pShader3dx->vsSkinSecondOpacity;
			ps=pShader3dx->psSkinSecondOpacity;
			SetSecondUVTrans(true,vs,mat,mat_anim);

			gb_RenderDevice3D->SetTexture(1,mat.pSecondOpacityTexture);
		}else
		if(GetAttr(ATTRUNKOBJ_NOLIGHT))
		{
			vs=pShader3dx->vsSkinNoLight;
			ps=pShader3dx->psSkin;
			pShader3dx->psSkin->SelectLT(false,diffuse_texture);
		}else
		if(mat.pReflectTexture || mat.is_reflect_sky)
		{
			int is_cube=false;
			if(material.Tex[1])
				is_cube=material.Tex[1]->GetAttribute(TEXTURE_CUBEMAP)?1:0;
			if(is_shadow)
			{
				vs=pShader3dx->vsSkinReflectionSceneShadow;
				ps=pShader3dx->psSkinReflectionSceneShadow;
			}else
			{
				vs=pShader3dx->vsSkinReflection;
				ps=pShader3dx->psSkinReflection;
			}

			sColor4f amount;
			amount.r=mat.reflect_amount*diffuse.r;
			amount.g=mat.reflect_amount*diffuse.g;
			amount.b=mat.reflect_amount*diffuse.b;
			amount.a=0;

			ps->SetReflection(is_cube,amount);
			vs->SetReflection(is_cube);
		}else
		if(mat.pBumpTexture && Option_EnableBump)
		{
			if(is_shadow)
			{
				vs=pShader3dx->vsSkinBumpSceneShadow;
				ps=pShader3dx->psSkinBumpSceneShadow;
			}else
			{
				vs=pShader3dx->vsSkinBump;
				ps=pShader3dx->psSkinBump;
			}

			ps->SelectSpecularMap(mat.pSpecularmap,texture_phase);
		}else
		{
			if(is_shadow)
			{
				vs=pShader3dx->vsSkinSceneShadow;
				if(diffuse_texture)
				{
					ps=pShader3dx->psSkinSceneShadow;
				}else
				{
					ps=pShader3dx->psSkin;
					pShader3dx->psSkin->SelectLT(true,false);
				}
			}else
			{
				vs=pShader3dx->vsSkin;
				ps=pShader3dx->psSkin;
				pShader3dx->psSkin->SelectLT(true,diffuse_texture);
			}
		}

		SetUVTrans(vs,mat,mat_anim);

		bool reflectionz=pCamera->GetAttr(ATTRCAMERA_REFLECTION);
		vs->SetReflectionZ(reflectionz);
		ps->SetReflectionZ(reflectionz);

		{//Немного не к месту, зато быстро по скорости, для отражений.
			gb_RenderDevice3D->SetTexture(5,pCamera->GetZTexture());
			gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
		}
		if(is_shadow)
			ps->SetShadowIntensity(GetScene()->GetShadowIntensity());

		ps->SetSelfIllumination(!mat.tex_self_illumination.empty() && !GetAttribute(ATTR3DX_NO_SELFILLUMINATION));
		ps->SetMaterial(&material, mat.is_big_ambient);

		ps->Select();

		GetWorldPos(s, world, world_num);
		vs->Select(world, world_num, lod.blend_indices);
		vs->SetMaterial(&material);

		if(0)
		{
			gb_RenderDevice3D->DrawIndexedPrimitive(
				lod.vb,s.offset_vertex,s.num_vertex,
				lod.ib,s.offset_polygon,s.num_polygon);
		}else
		{
			bool draw_2pass=draw_opacity;
			if(/*material.Tex[0] && */mat.is_opacity_texture)
			{
				draw_2pass=false;
			}

			if (!draw2passes)
			{
				if(is_opacity_vg)
				{
					if(draw_2pass)
					{
						gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
						gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
						gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
					}

					DrawMaterialGroupSelectively(s,material.Diffuse,draw_opacity,vs,ps);

					if(draw_2pass)
					{
						gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
						gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
						gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
						DrawMaterialGroupSelectively(s,material.Diffuse,draw_opacity,vs,ps);
					}
				}else
				{
					if(draw_2pass)
					{
						gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
						gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
						gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
					}

					DrawMaterialGroup(s);

					if(draw_2pass)
					{
						gb_RenderDevice3D->SetRenderState(RS_ZWRITEENABLE,FALSE);
						gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
						gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
						DrawMaterialGroup(s);
					}
				}
			}else
			{
				//if (pCamera->GetCameraPass()==SCENENODE_ZPASS)
				//{
				//	gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
				//	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
				//	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
				//}else
				//{
				//	gb_RenderDevice3D->SetRenderState(RS_ZWRITEENABLE,FALSE);
				//	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
				//	gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
				//}
				DrawMaterialGroup(s);
			}
		}
	}

	if(draw_opacity || draw2passes)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,old_zwriteble);
		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
	}
	
}

void cObject3dx::SetUVTrans(VSSkinBase* vs,cStaticMaterial& mat,MaterialAnim& mat_anim)
{
	//uv transform
	if(mat.chains.empty())
	{
		vs->SetUVTrans(NULL);
	}else
	{
		cStaticMaterialAnimation& mat_chain=mat.chains[mat_anim.chain];
		if(mat_chain.uv.values.empty())
		{
			vs->SetUVTrans(NULL);
		}else
		{
			float uvmatrix[6];
			mat_chain.uv.InterpolateSlow(mat_anim.phase,uvmatrix);
			vs->SetUVTrans(uvmatrix);
		}
	}
}

void cObject3dx::SetSecondUVTrans(bool use,class VSSkinBase* vs,cStaticMaterial& mat,MaterialAnim& mat_anim)
{
	if(use)
	{
		//uv transform
		if(!mat.chains.empty()) {
			cStaticMaterialAnimation& mat_chain=mat.chains[mat_anim.chain];
			if(!mat_chain.uv_displacement.values.empty()){
				float uvmatrix_dp[6];
				mat_chain.uv_displacement.InterpolateSlow(mat_anim.phase,uvmatrix_dp);
				vs->SetSecondOpacityUVTrans(uvmatrix_dp,pStatic->is_uv2?SECOND_UV_T1:SECOND_UV_T0);
			} else {
				vs->SetSecondOpacityUVTrans(0,pStatic->is_uv2?SECOND_UV_T1:SECOND_UV_T0);
			}
		} else {
			vs->SetUVTrans(NULL);
		}
	}else
	{
		vs->SetSecondOpacityUVTrans(NULL,SECOND_UV_NONE);
	}
}

void cObject3dx::DrawMaterialGroup(cStaticIndex& s)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	for(int ivg=0;ivg<s.visible_group.size();ivg++)
	{
		cTempVisibleGroup& vg=s.visible_group[ivg];
		if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
		{
			int num_polygon=vg.num_polygon;
#ifdef POLYGON_LIMITATION
			if(polygon_limitation_num_polygon>=polygon_limitation_max_polygon)
				break;
			if(polygon_limitation_num_polygon+num_polygon>=polygon_limitation_max_polygon)
				num_polygon=polygon_limitation_max_polygon-polygon_limitation_num_polygon;
			polygon_limitation_num_polygon+=num_polygon;
			num_out_polygons+=num_polygon;
#endif
			gb_RenderDevice3D->DrawIndexedPrimitive(
				lod.vb,s.offset_vertex,s.num_vertex,
				lod.ib,vg.begin_polygon+s.offset_polygon,num_polygon);
		}
	}
}

void cObject3dx::DrawMaterialGroupSelectively(cStaticIndex& s,const sColor4f& color,bool draw_opacity,VSSkin* vs,PSSkin* ps)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	for(int ivg=0;ivg<s.visible_group.size();ivg++)
	{
		cTempVisibleGroup& vg=s.visible_group[ivg];
		sGroup& group=iGroups[vg.visible_set];
		if(vg.visible&group.p->visible_shift)
		{
			if(draw_opacity)
			{
				if(group.alpha>=AlphaMaxiumBlend)
					continue;
				sColor4f c(color);
				c.a*=group.alpha;
				vs->SetAlphaColor(&c);
				ps->SetAlphaColor(&c);
			}else
			{
				if(group.alpha<AlphaMaxiumBlend)
					continue;
			}

			//БЛИН!!! Этот подход не дает истинной гибкости!

			gb_RenderDevice3D->DrawIndexedPrimitive(
				lod.vb,s.offset_vertex,s.num_vertex,
				lod.ib,vg.begin_polygon+s.offset_polygon,vg.num_polygon);

		}
	}
}

bool cObject3dx::IsVisibleMaterialGroup(cStaticIndex& s) const
{
	for(int ivg=0;ivg<s.visible_group.size();ivg++)
	{
		cTempVisibleGroup& vg=s.visible_group[ivg];
		if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
		{
			return true;
		}
	}

	return false;
}

void cObject3dx::DrawShadowAndZbuffer(cCamera* pCamera,bool ZBuffer)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	gb_RenderDevice3D->SetTextureBase(1,NULL);

	//!!! Не забыть сортировку по материалам.
	int size=lod.skin_group.size();
	static MatXf world[cStaticIndex::max_index];
	int world_num;
	for(int iskin_group=0;iskin_group<size;iskin_group++)
	{
		cStaticIndex& s=lod.skin_group[iskin_group];
		if(!IsVisibleMaterialGroup(s))
			continue;

		cStaticMaterial& mat=pStatic->materials[s.imaterial];

		if(mat.transparency_type!=cStaticMaterial::TRANSPARENCY_FILTER)
			continue;

		MaterialAnim& mat_anim=materials[s.imaterial];
		cTexture* diffuse_texture=material_textures[s.imaterial].diffuse_texture;

		float alpha=mat_anim.opacity*object_opacity*distance_alpha;

		gb_RenderDevice3D->SetSamplerData(0,(mat.tiling_diffuse&cStaticMaterial::TILING_U_WRAP)?sampler_wrap_anisotropic:sampler_clamp_anisotropic);

		eBlendMode blend=ALPHA_NONE;
		bool is_alphatest=false;
		if(diffuse_texture)
		{
			cTexture* pTex0=diffuse_texture;
			//
			if(pTex0->IsAlphaTest() || pTex0->IsAlpha())
			{
				blend=ALPHA_TEST;
				is_alphatest=true;
			}
		}

		if(gb_RenderDevice3D->dtAdvance->GetID()!=DT_GEFORCEFX)
			blend=ALPHA_NONE;
		if(alpha<AlphaMiniumShadow)
			continue;

		gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
		if(is_alphatest)
		{
			float texture_phase=material_textures[s.imaterial].texture_phase;
			gb_RenderDevice3D->SetTexturePhase(0,diffuse_texture,texture_phase);
		}else
		{
			gb_RenderDevice3D->SetTextureBase(0,NULL);
		}

		VSSkinBase* vs=NULL;

		gb_RenderDevice3D->SetTextureBase(3,NULL);
		vs=ZBuffer?pShader3dx->vsSkinZBuffer:pShader3dx->vsSkinShadow;
		SetUVTrans(vs,mat,mat_anim);

		if(is_alphatest)
		{
			if (ZBuffer)
			{
				pShader3dx->psSkinZBufferAlpha->SetSecondOpacity(mat.pSecondOpacityTexture);
				pShader3dx->psSkinZBufferAlpha->Select();
			}else
			{
				pShader3dx->psSkinShadowAlpha->SetSecondOpacity(mat.pSecondOpacityTexture);
				pShader3dx->psSkinShadowAlpha->Select();
			}
		}else
		{
			if (ZBuffer)
			{
				pShader3dx->psSkinZBuffer->Select();
			}
			else
				pShader3dx->psSkinShadow->Select();
		}
		
		SetSecondUVTrans(mat.pSecondOpacityTexture?true:false,vs,mat,mat_anim);
		

		GetWorldPos(s,world,world_num);
		vs->Select(world,world_num,lod.blend_indices);

		DrawMaterialGroup(s);
	}

}

int cObject3dx::GetAnimationGroup(const char* name)
{
	vector<AnimationGroup>& animation_group=pStatic->animation_group;
	int size=animation_group.size();
	for(int i=0;i<size;i++)
	{
		AnimationGroup& ac=animation_group[i];
		if(ac.name==name)
			return i;
	}

//	{
//		string str="Не найдена анимационная группа. Все имена, кроме имен файлов являются чувствительными к регистрам. Файл:";
//		str+=pStatic->file_name;
//		str+="  Animation Group:";
//		str+=name;
//		xxassert(0 ,str.c_str());
//	}
	return -1;
}

int cObject3dx::GetAnimationGroupNumber()
{
	return pStatic->animation_group.size();
}

const char* cObject3dx::GetAnimationGroupName(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		string str="Bad group index. Файл:";
		str+=pStatic->file_name;
		xxassert(0 ,str.c_str());
		return NULL;
	}

	return pStatic->animation_group[igroup].name.c_str();
}

bool cObject3dx::SetVisibilityGroup(const char* name, bool silently,C3dxVisibilitySet iset)
{
	xassert(iset.iset>=0 && iset.iset<pStatic->visible_sets.size());
	vector<cStaticVisibilityChainGroup*>& vg=pStatic->visible_sets[iset.iset]->visibility_groups[0];
	int vg_size=vg.size();
	for(int igroup=0;igroup<vg_size;igroup++)
	{
		cStaticVisibilityChainGroup* cur=pStatic->visible_sets[0]->visibility_groups[0][igroup];
		string& cname=cur->name;
		if(cname==name)
		{
			SetVisibilityGroup(C3dxVisibilityGroup(igroup));
			return true;
		}
	}

	xassert(silently);
	return false;
}

void cObject3dx::SetVisibilityGroup(C3dxVisibilityGroup group,C3dxVisibilitySet iset)
{
	if(iset==C3dxVisibilitySet::BAD)
	{
		xassert(0);//Наверно не нужно такое поведение.
		int size=GetVisibilitySetNumber();
		for(int i=0;i<size;i++)
			SetVisibilityGroup(group,C3dxVisibilitySet(i));
		return;
	}

	if(iGroups.empty())
		return;

	xassert(iset.iset>=0 && iset.iset<iGroups.size());
	xassert(group.igroup >=0 && group.igroup< pStatic->visible_sets[iset.iset]->visibility_groups[0].size());
	iGroups[iset.iset].i=group;
	UpdateVisibilityGroup(iset);
}

void cObject3dx::SetVisibilitySetAlpha(float alpha,C3dxVisibilitySet iset)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	xassert(iset.iset>=0 && iset.iset<iGroups.size());
	iGroups[iset.iset].alpha=alpha;

	int num_mat=material_textures.size();
	for(int imat=0;imat<num_mat;imat++)
	{
		material_textures[imat].is_opacity_vg=false;
	}

	int skingroup_size=lod.skin_group.size();
	for(int iskin_group=0;iskin_group<skingroup_size;iskin_group++)
	{
		cStaticIndex& s=lod.skin_group[iskin_group];
		if(!IsVisibleMaterialGroup(s))
			continue;

		MaterialTexture& mat=material_textures[s.imaterial];

		for(int ivg=0;ivg<s.visible_group.size();ivg++)
		{
			cTempVisibleGroup& vg=s.visible_group[ivg];
			sGroup& group=iGroups[vg.visible_set];
			if(vg.visible&group.p->visible_shift)
			{
				if(group.alpha<AlphaMaxiumBlend)
				{
					mat.is_opacity_vg=true;
				}
			}
		}
	}
}

void cObject3dx::UpdateVisibilityGroup(C3dxVisibilitySet iset)
{
	iGroups[iset.iset].p=pStatic->visible_sets[iset.iset]->visibility_groups[iLOD][iGroups[iset.iset].i.igroup];
}

void cObject3dx::UpdateVisibilityGroups()
{
	int size=iGroups.size();
	for(int iset=0;iset<size;iset++)
		UpdateVisibilityGroup(C3dxVisibilitySet(iset));
}

void cObject3dx::CalcIsOpacity(	bool& is_opacity,bool& is_noopacity)
{
	is_opacity=false;
	is_noopacity=false;

	for(int imat=0;imat<materials.size();imat++)
	{
		MaterialAnim& m=materials[imat];
		cStaticMaterial& sm=pStatic->materials[imat];
		MaterialTexture& tex=material_textures[imat];

		bool is=(m.opacity*object_opacity*distance_alpha)<AlphaMaxiumBlend;
		is=is || sm.is_opacity_texture;
		if(is)
			is_opacity=true;
		else
			is_noopacity=true;

		if(tex.is_opacity_vg)
		{
			is_opacity=true;
		}
	}
}

void cObject3dx::PreDraw(cCamera *pCamera)
{
#ifdef POLYGON_LIMITATION
	num_out_polygons=0;
#endif
	ProcessEffect(pCamera);
	if(GetAttr(ATTRUNKOBJ_HIDE_BY_DISTANCE))
	{
		float radius=GetBoundRadius();
		float dist=pCamera->xformZ(position.trans());

		if(Option_HideSmoothly)
		{
			if(dist>hideDistance)
				return;
			float hideRange = hideDistance*0.15f;
			float fadeDist = hideDistance-hideRange;
			if(dist>fadeDist)
			{
				distance_alpha = 1.0f-(dist-fadeDist)/hideRange;
			}else
				distance_alpha=1.0f;
		}else
		{
			if(dist>hideDistance)
				return;
		}

	}else
	{
		distance_alpha = 1.0f;
	}

	if(pStatic->circle_shadow_enable_min==OST_SHADOW_REAL)
	{
		cCamera* pShadow=pCamera->FindChildCamera(ATTRCAMERA_SHADOWMAP);
		if(pShadow)
		{
			if(pShadow->TestVisible(position.trans(),pStatic->radius*position.scale()))
				pShadow->AttachNoRecursive(SCENENODE_OBJECT,this);
		}
	}

	{
		cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
		if(pReflection)
		{
			if(pReflection->TestVisible(position.trans(),pStatic->radius*position.scale()))
			{
				bool is_opacity,is_noopacity;
				CalcIsOpacity(is_opacity,is_noopacity);
				if(is_noopacity)
					pReflection->AttachNoRecursive(SCENENODE_OBJECT,this);
				if(is_opacity)
					pReflection->AttachNoRecursive(SCENENODE_OBJECTSORT,this);
			}
		}
	}

//	MatXf xpos;
//	position.copy_right(xpos);
//	pCamera->TestVisible(xpos,pStatic->bound_box.min,pStatic->bound_box.max)
	
	if(pStatic->is_lod && !GetAttribute(ATTRUNKOBJ_NO_USELOD))
	{//select lod
		Vect3f camera_pos=pCamera->GetPos();
		Vect3f unit_pos=position.trans();
		float d=pCamera->GetPos().distance2(position.trans());
		iLOD=GetLodByDistance2(d);
		UpdateVisibilityGroups();
	}

	if(!pCamera->TestVisible(position.trans(),pStatic->radius*position.scale()))
		return;

	cCamera* pFloatZBuffer=pCamera->FindChildCamera(ATTRCAMERA_FLOAT_ZBUFFER);
	if (pFloatZBuffer)
	{
		bool is_opacity,is_noopacity;
		CalcIsOpacity(is_opacity,is_noopacity);
		if(is_noopacity)
			pFloatZBuffer->AttachNoRecursive(SCENENODE_OBJECT,this);
		if(is_opacity)
			pFloatZBuffer->AttachNoRecursive(SCENENODE_OBJECTSORT,this);
	}

	if (GetAttribute(ATTRUNKOBJ_2PASS_ZBUFFER))
	{
		pCamera->AttachNoRecursive(SCENENODE_OBJECT_2PASS,this);
	}else
	{
		bool is_opacity,is_noopacity;
		CalcIsOpacity(is_opacity,is_noopacity);

		if(is_noopacity)
		{
			if(GetAttribute(ATTR3DX_UNDERWATER))
				pCamera->AttachNoRecursive(SCENENODE_UNDERWATER,this);
			else
			if(GetAttribute(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE))
				pCamera->AttachNoRecursive(SCENENODE_FLAT_SILHOUETTE,this);
			else
				pCamera->AttachNoRecursive(SCENENODE_OBJECT,this);
		}

		if(is_opacity)
		{
			pCamera->AttachNoRecursive(SCENENODE_OBJECTSORT,this);
		}
	}

	if(pStatic->circle_shadow_enable_min==c3dx::OST_SHADOW_CIRCLE)
		AddCircleShadow();

	Update();
	//updated=false;
	//if(pAnimSecond)
	//	pAnimSecond->updated=false;
}

void cObject3dx::Animate(float dt)
{

	point_light.clear();

	Update();//Все равно там внутри проверка есть.
	if(!observer.empty())
		observer.UpdateLink();

	int mat_size=material_textures.size();
	for(int imaterial=0;imaterial<mat_size;imaterial++)
	{
		MaterialTexture& ma=material_textures[imaterial];
		if(ma.diffuse_texture)
		{
			int len=ma.diffuse_texture->GetNumberFrame()*ma.diffuse_texture->GetTimePerFrame();
			if(len)
			{
				ma.texture_phase+=dt/len;
				if(ma.texture_phase>1)
					ma.texture_phase-=1;
			}
		}
	}
}

float cObject3dx::GetBoundRadius() const
{
	//sBox6f box;
	//GetBoundBox(box);
	//float r=box.max.distance(box.min)*0.5f;
	//return r;
	return pStatic->radius*position.s;
}

void cObject3dx::GetBoundBox(sBox6f& box_) const
{
	box_=pStatic->bound_box;
	box_.min*=position.s;
	box_.max*=position.s;
}

void cObject3dx::GetBoundBoxUnscaled(sBox6f& box_)
{
	box_=pStatic->bound_box;
}

void cObject3dx::CalcBoundingBox()
{
	sBox6f box;
	box.min.set(1e30f,1e30f,1e30f);
	box.max.set(-1e30f,-1e30f,-1e30f);

	TriangleInfo all;
	GetTriangleInfo(all,TIF_POSITIONS|TIF_ZERO_POS|TIF_ONE_SCALE);
	if(all.positions.empty())
	{
		box.min.set(0,0,0);
		box.max.set(0,0,0);
	}

	float radius=0;
	vector<Vect3f>::iterator it;
	FOR_EACH(all.positions,it)
	{
		Vect3f& pos=*it;
		box.AddBound(pos);
		float r=pos.norm();
		radius=max(radius,r);
	}

	pStatic->radius=radius;
	pStatic->bound_box=box;

	CalcBoundSphere();
}

void cObject3dx::GetWorldPos(cStaticIndex& s,MatXf* world,int& world_num) const
{
	world_num=s.node_index.size();
	for(int j=0;j<world_num;j++)
	{
		int inode=s.node_index[j];
		const cNode3dx& node=nodes[inode];
		cStaticNode& sn=pStatic->nodes[inode];
		MatXf xpos;
		node.pos.copy_right(xpos);
		world[j].mult(xpos,sn.inv_begin_pos);

//		position.copy_right(world[j]);//test
	}
}
void cObject3dx::GetWorldPos(cStaticIndex& s,MatXf& world, int& idx)
{
	xassert(idx < s.node_index.size());
	
	int inode=s.node_index[idx];
	cNode3dx& node=nodes[inode];
	cStaticNode& sn=pStatic->nodes[inode];
	MatXf xpos;
	node.pos.copy_right(xpos);
	world.mult(xpos,sn.inv_begin_pos);
}


void cObject3dx::GetEmitterMaterial(struct cObjMaterial& material)
{
	if(pStatic->materials.empty())
	{
		xassert(0 && "Zero materials?");
		return;
	}

	cStaticMaterial& mt = pStatic->materials.front();
	material.Ambient = mt.ambient;
	material.Diffuse = mt.diffuse;
	material.Specular = mt.specular;
	material.Power = mt.specular_power;
}

const char* cObject3dx::GetFileName() const
{
	return pStatic->file_name.c_str();
}

void cObject3dx::SetSkinColor(sColor4c skin_color_, const char* logo_name_)//emblem_name - хранит путь к эмблеме. 
{
	try {
		skin_color=skin_color_;

		LoadTexture(true,false,logo_name_);
		isSkinColorSet_ = true;
	} catch (...) {
	}
}

void cObject3dx::SetSilhouetteIndex(int index)
{
	silhouette_index = index;
}

void cObject3dx::LoadTexture(bool only_skinned,bool only_self_illumination, const char* logo_name_)
{
	int m_size=materials.size();
	xassert(m_size==pStatic->materials.size());
	bool enable_self_illum=GetAttribute(ATTR3DX_NO_SELFILLUMINATION)?false:true;

	for(int i=0;i<m_size;i++)
	{
		cStaticMaterial& mat=pStatic->materials[i];
		cTexture*& diffuse=material_textures[i].diffuse_texture;
		sColor4c color=skin_color;
		
		if(only_self_illumination)
		{
			if(mat.tex_self_illumination.empty())
				continue;
		}else
		{
			if(!(logo_name_ && logo_name_[0]))
			if(!(mat.is_skinned && only_skinned) && !(!mat.is_skinned && !only_skinned))
				continue;
		}

		sRectangle4f logo_position;
		float logo_angle = 0;
		const char* logo=NULL;
		if(pStatic->logos.GetLogoPosition(mat.tex_diffuse.c_str(),&logo_position,logo_angle))
		{
			logo=logo_name_;
		}

		RELEASE(diffuse);
		if(enable_self_illum)
			diffuse=GetTexLibrary()->GetElement3DColor(mat.tex_diffuse.c_str(),
			mat.is_skinned?mat.tex_skin.c_str():NULL,mat.is_skinned?&skin_color:NULL,
					mat.tex_self_illumination.c_str(),logo, &logo_position,logo_angle);
		else
			diffuse=GetTexLibrary()->GetElement3DColor(mat.tex_diffuse.c_str(),
					mat.is_skinned?mat.tex_skin.c_str():NULL,mat.is_skinned?&skin_color:NULL,
					NULL,logo, &logo_position,logo_angle);

		if(diffuse)
		{
			if(diffuse->GetAttribute(TEXTURE_ALPHA_BLEND) && !mat.is_opacity_texture)
			{
				diffuse->ClearAttribute(TEXTURE_ALPHA_BLEND);
				diffuse->SetAttribute(TEXTURE_ALPHA_TEST);
			}

			bool reload=false;
			if (GetAttr(ATTR3DX_NO_RESIZE_TEXTURES))
			{
				reload=diffuse->GetAttribute(TEXTURE_DISABLE_DETAIL_LEVEL)==0;
				diffuse->SetAttribute(TEXTURE_DISABLE_DETAIL_LEVEL);
			}
			if (reload)
				GetTexLibrary()->ReLoadTexture(diffuse);

			if(mat.pSecondOpacityTexture)
			{
				if(mat.pSecondOpacityTexture->GetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST))
				{
					diffuse->SetAttribute(TEXTURE_ALPHA_TEST);
				}
			}

		}
	}
}

void cObject3dx::EnableSelfIllumination(bool enable)
{
	PutAttribute(ATTR3DX_NO_SELFILLUMINATION,!enable);
	LoadTexture(false,true);
}
void cObject3dx::DisableDetailLevel()
{
	try {
		SetAttribute(ATTR3DX_NO_RESIZE_TEXTURES);
		LoadTexture(false,false,"");
	} catch (...) {
	}
}

void cObject3dx::GetLocalBorder(int *nVertex,Vect3f **Vertex,int *nIndex,short **Index)
{
	cStaticBasement& b=pStatic->basement;
	*nVertex=b.vertex.size();
	*Vertex=&b.vertex[0];
	*nIndex=b.polygons.size();
	*Index=(short*)&b.polygons[0];

}

void cObject3dx::DrawLogicBound()
{
	Update();
	sBox6f box=pStatic->logic_bound.bound;
	MatXf pos=GetPosition();
	pos.rot()*=position.s;
	gb_RenderDevice->DrawBound(pos,box.min,box.max,0,sColor4c(255,0,0,255));

	for(int inode=0;inode<GetNodeNumber();inode++)
	{
		pos=GetNodePositionMat(inode);
		if(IsLogicBound(inode))
		{
			box=GetLogicBoundScaledUntransformed(inode);
			gb_RenderDevice->DrawBound(pos,box.min,box.max,0,sColor4c(0,255,0,255));
		}
	}
}

void cObject3dx::DrawBound() const
{
	sBox6f box;
	GetBoundBox(box);
	gb_RenderDevice3D->DrawBound(GetPosition(), box.min, box.max, false, sColor4c(150, 0, 0, 155));
}

void DrawPointer(MatXf m,sColor4c color,float len,int xyz);
static void DrawLogicNode(MatXf& pos,float logic_radius, bool all)
{
	sColor4c color;
	float r=logic_radius*0.1f;
	if(all)
	{
		DrawPointer(pos,sColor4c(255,0,0),r,0);
		DrawPointer(pos,sColor4c(0,255,0),r,1);
		DrawPointer(pos,sColor4c(0,0,255),r,2);
	}else
	{
		sColor4c c(255,255,255);
		DrawPointer(pos,c,r,0);
		DrawPointer(pos,c,r,1);
		DrawPointer(pos,c,r,2);
	}
}

void cObject3dx::DrawLogic(cCamera* pCamera,int selected)
{
	Update();

	Vect3f pos=pCamera->GetMatrix()*position.trans();
	float min_size=100000/pos.z;
	float radius=max(GetBoundRadius(),min_size);

	double interval=0.4e3;
	bool blink=fmod(xclock(),interval)<interval*0.5f;
	int size=nodes.size();
	int i;
	for(i=0;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		cStaticNode& sn=pStatic->nodes[i];
		
		MatXf xpos;
		node.pos.copy_right(xpos);
		if(i==selected)
			DrawLogicNode(xpos,radius,blink);
		else
			DrawLogicNode(xpos,radius,true);
	}

	gb_RenderDevice->FlushPrimitive3D();

	if(selected>=0 && selected<size)
	{
		i=selected;
		cNode3dx& node=nodes[i];
		cStaticNode& sn=pStatic->nodes[i];
		
		MatXf xpos;
		node.pos.copy_right(xpos);
		{
			Vect3f pv,pe;
			pCamera->ConvertorWorldToViewPort(&xpos.trans(),&pv,&pe);
			if(pe.z>0)
			{
				gb_RenderDevice->OutText(round(pe.x),round(pe.y),sn.name.c_str(),sColor4f(1,1,1));
				const cNode3dx& node=nodes[i];

				if(!GetVisibilityTrack(i))
				{
					gb_RenderDevice->OutText(round(pe.x),round(pe.y+18),"hide",sColor4f(0.5f,0.5f,1));
				}
/*
				float bphase=node.phase-0.05f;
				float ephase=node.phase;
				if(bphase<0)
					bphase+=1;
				if(GetVisibilityTrackInterval(i,bphase,ephase))
					gb_RenderDevice->OutText(pe.x,pe.y+36,"ch",sColor4f(0.5f,0.5f,1));
/**/
			}
		}
	}
}

void cObject3dx::SetUserTransform(int nodeindex,const Se3f& pos)
{
	updated=false;
	Mats p;
	p.se()=pos;
	p.scale()=1;
	SetUserTransform(nodeindex,p);
}

void cObject3dx::SetUserTransform(int nodeindex,const Mats& pos)
{
	updated=false;
	c3dxAdditionalTransformation& t=GetUserTransformIndex(nodeindex);
	t.mat=pos;
	if(pAnimSecond)
	{
		cNode3dx& s=pAnimSecond->nodes[nodeindex];
		s.additional_transform=nodes[nodeindex].additional_transform;
	}
}

#define ASSERT_NODEINDEX(nodeindex) if(!(nodeindex>=0 && nodeindex<nodes.size())){string s="Bad nodeindex in: ";s+=pStatic->file_name.c_str();xxassert(0,s.c_str());}

c3dxAdditionalTransformation& cObject3dx::GetUserTransformIndex(int nodeindex)
{
	ASSERT_NODEINDEX(nodeindex);
	cNode3dx& s=nodes[nodeindex];
	if(s.IsAdditionalTransform())
	{
		BYTE add_index=s.additional_transform;
		xassert(add_index<additional_transformations.size());
		return additional_transformations[add_index];
	}

	int size=additional_transformations.size();
	xassert(size<254);
	for(int i=0;i<size;i++)
	if(additional_transformations[i].nodeindex==-1)
	{
		int found_index=i;
		s.additional_transform=found_index;
		additional_transformations[found_index].nodeindex=nodeindex;
		return additional_transformations[found_index];
	}

	s.additional_transform=size;
	c3dxAdditionalTransformation n;
	n.nodeindex=nodeindex;
	additional_transformations.push_back(n);
	return additional_transformations.back();
}

void cObject3dx::RestoreUserTransform(int nodeindex)
{
	ASSERT_NODEINDEX(nodeindex);
	cNode3dx& s=nodes[nodeindex];
	if(!s.IsAdditionalTransform())
		return;

	updated=false;
	xassert(s.additional_transform<additional_transformations.size());
	additional_transformations[s.additional_transform].nodeindex=-1;
	s.additional_transform=255;
	xassert(!s.IsAdditionalTransform());
	if(pAnimSecond)
	{
		cNode3dx& s=pAnimSecond->nodes[nodeindex];
		s.additional_transform=255;
	}
}

void cObject3dx::ProcessEffect(cCamera *pCamera)
{
	cStaticVisibilityChainGroup* pGroup=GetVisibilityGroup();

	int effect_size=effects.size();
	for(int ieffect=0;ieffect<effect_size;ieffect++)
	{
		EffectData& e=effects[ieffect];
		cStaticEffect& se=pStatic->effects[ieffect];
		cStaticNode& snode=pStatic->nodes[se.node];
		cNode3dx& node=nodes[se.node];
		cStaticNodeChain& chain=snode.chains[node.chain];

		xassert(!chain.visibility.values.empty());

		if(e.index_visibility==255)
		{
			e.index_visibility=chain.visibility.FindIndex(node.phase);
			e.prev_phase=node.phase;
		}

		bool is_group_visible=pGroup->visible_nodes[se.node];
		if(se.is_cycled)
		{
			e.index_visibility=chain.visibility.FindIndexRelative(node.phase,e.index_visibility);
			bool visible=false;
			if(is_group_visible)
				chain.visibility.Interpolate(node.phase,&visible,e.index_visibility);
			if(visible && !e.pEffect)
			{
				EffectKey* key=gb_EffectLibrary->Get((gb_VisGeneric->GetEffectPath()+se.file_name).c_str(),position.s,gb_VisGeneric->GetEffectTexturePath());
				e.pEffect=pCamera->GetScene()->CreateEffectDetached(*key,this);//,position.s);
				e.pEffect->LinkToNode(this,se.node);
				e.pEffect->Attach();
			}

			if(e.pEffect)
				e.pEffect->SetParticleRate(visible?1.0f:0);
		}else
		if(is_group_visible)
		{
			///Ищем 
			float delta_plus,delta_minus;
			if(node.phase>e.prev_phase)
			{
				delta_plus=node.phase-e.prev_phase;
				delta_minus=1-delta_plus;
			}else
			{
				delta_minus=e.prev_phase-node.phase;
				delta_plus=1-delta_minus;
			}

			bool is_up=false;
			Interpolator3dxBool::ValuesType& data=chain.visibility.values;
			int dsize=data.size();
			if(delta_plus<delta_minus)
			{
				int cur=e.index_visibility;
				bool prev_visibly=bool(data[cur].value);
				
				do
				{
					const sInterpolate3dxBool& p=data[cur];

					if(p.value && !prev_visibly)
						is_up=true;
					
					float t=(node.phase-p.tbegin)*p.inv_tsize;
					if(t>=0 && t<=1)
					{
						break;
					}

					prev_visibly=bool(p.value);

					cur++;
					if(cur>=dsize)
						cur=0;
				}while(cur!=e.index_visibility);

				e.index_visibility=cur;
				
			}else
			{
				int cur=e.index_visibility;
				bool prev_visibly=data[cur].value;
				
				do
				{
					const sInterpolate3dxBool& p=data[cur];

					if(p.value && !prev_visibly)
						is_up=true;
					
					float t=(node.phase-p.tbegin)*p.inv_tsize;
					if(t>=0 && t<=1)
					{
						break;
					}

					prev_visibly=p.value;

					cur--;
					if(cur<0)
						cur=dsize-1;
				}while(cur!=e.index_visibility);

				e.index_visibility=cur;
			}

			if(is_up)
			{
				xassert(0);
				EffectKey* key=gb_EffectLibrary->Get((gb_VisGeneric->GetEffectPath()+se.file_name).c_str(),position.s,gb_VisGeneric->GetEffectTexturePath());
				cEffect* eff=pCamera->GetScene()->CreateEffectDetached(*key,this,true);
				eff->LinkToNode(this,se.node);
				eff->Attach();
			}
		}

		e.prev_phase=node.phase;
	}
}

void cObject3dx::SetColorOld(const sColor4f *ambient_,const sColor4f *diffuse_,const sColor4f *specular_)
{
	if(ambient_)
		ambient=*ambient_;
	if(diffuse_)
	{
		diffuse=*diffuse_;
		if(ambient_)
			diffuse.a=ambient_->a;
		object_opacity=diffuse_->a;
	}
	if(specular_)
		specular=*specular_;
}

void cObject3dx::GetColorOld(sColor4f *ambient_,sColor4f *diffuse_,sColor4f *specular_) const
{
	if(ambient_)
		*ambient_=ambient;
	if(diffuse_)
	{
		*diffuse_=diffuse;
		diffuse_->a=object_opacity;
	}
	if(specular_)
		*specular_=specular;
}

void cObject3dx::SetColorMaterial(const sColor4f *ambient_,const sColor4f *diffuse_,const sColor4f *specular_)
{
	if(ambient_)
		ambient=*ambient_;
	if(diffuse_)
		diffuse=*diffuse_;
	if(specular_)
		specular=*specular_;
}

void cObject3dx::GetColorMaterial(sColor4f *ambient_,sColor4f *diffuse_,sColor4f *specular_) const
{
	if(ambient_)
		*ambient_=ambient;
	if(diffuse_)
		*diffuse_=diffuse;
	if(specular_)
		*specular_=specular;
}

bool cObject3dx::IntersectSphere(const Vect3f& p0, const Vect3f& p1) const
{
	sBox6f box;
	GetBoundBox(box);
	float radius = box.radius();
	Vect3f center(GetPosition() * box.center());

	Vect3f l=p1-p0;
	float A=l.norm2();

	Vect3f d=p0-center;
	float C=d.x*d.x+d.y*d.y+d.z*d.z-radius*radius;
	float B=d.x*l.x+d.y*l.y+d.z*l.z;
	float det=B*B-A*C;
	if(det<0)return false;
	det=sqrtf(det);
	float t1=(-B+det)/A;
	float t2=(-B-det)/A;
	if((t1>1||t1<0)&&(t2>1||t2<0)) return false;
	return true;
}

bool cObject3dx::IntersectBound(const Vect3f& p0, const Vect3f& p1) const
{
	sBox6f box;
	GetBoundBox(box);
	return box.isCrossOrInside(position.se(), p0, p1);
}

bool Intersection(const Vect3f& a,const Vect3f& b,const Vect3f& c,
		const Vect3f& p0,const Vect3f& p1,const Vect3f& pn,float& t);
bool IntersectionRay(const Vect3f& a,const Vect3f& b,const Vect3f& c,
		const Vect3f& p0,const Vect3f& pn);
bool IntersectionRaySpecial(const Vect3f& a,const Vect3f& b,const Vect3f& c);

bool cObject3dx::Intersect(const Vect3f& p0,const Vect3f& p1) const
{
	if(is_sse_instructions)
		return IntersectTriangleSSE(p0,p1);
	return IntersectTriangle(p0,p1);
}
/*
bool cObject3dx::IntersectTriangle(const Vect3f& p0,const Vect3f& p1)
{
	TriangleInfo all;
	//Медленно довольно так, потому как может выбраться куча точек, которые невидимы.
	GetTriangleInfo(all,TIF_TRIANGLES|TIF_POSITIONS);

	Vect3f pn=p1-p0;
	Vect3f pnorm=pn;
	pnorm.Normalize();
	bool intersect=false;


	int size_polygon=all.triangles.size();
	for(int i=0;i<size_polygon;i++)
	{
		sPolygon& p=all.triangles[i];
		//float t;
		//if(Intersection(point[p[0]],point[p[1]],point[p[2]],p0,p1,pn,t))

//		for(int ip=0;ip<3;ip++)
//			gb_RenderDevice3D->DrawPoint(all.positions[p[ip]],sColor4c(255,255,255,255));

		if(IntersectionRay(all.positions[p[0]],all.positions[p[1]],all.positions[p[2]],p0,pnorm))
		{
			intersect=true;
			break;
		}
	}

	return intersect;
}
*/
//Можно подумать об упрощении, если повернуть/сдвинуть фигуру так, чтобы 
//p0==0 p1==0,0,1
void cObject3dx::CalcOffsetMatrix(MatXf& offset,const Vect3f& p0,const Vect3f& p1) const
{
	Vect3f pn=p1-p0;
	Vect3f z=pn;
	z.Normalize();

	int imin=0;
	float xmin=fabsf(z[0]);
	for(int i=1;i<3;i++)
	{
		if(fabsf(z[i])<xmin)
		{
			xmin=fabsf(z[i]);
			imin=i;
		}
	}

	Vect3f x=Vect3f::ZERO;
	x[imin]=1;

	Vect3f y=x%z;
	y.Normalize();
	x=y%z;


	//offset.rot()=Mat3f(
	//x.x, y.x, z.x,
	//x.y, y.y, z.y,
	//x.z, y.z, z.z
	//);
	offset.rot()=Mat3f(//Инвертированная транспонированием.
	x.x, x.y, x.z,
	y.x, y.y, y.z,
	z.x, z.y, z.z
	);

	float det=offset.rot().det();
	xassert(det>=0.99 && det<=1.01);

	offset.trans()=offset.rot()*(-p0);

	Vect3f p0trans=offset*p0;
	Vect3f pntrans=offset.rot()*z;
//	p0trans=0,0,0
//	pntrans==0,0,1
}

//*
int cObject3dx::GetNumPolygons()
{
	pStatic->CacheBuffersToSystemMem(iLOD);
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];

	int numPolyugons=0;
	cSkinVertexSysMemI skin_vertex;
	sPolygon* pPolygon = lod.sys_ib;
	skin_vertex.SetVB(lod.sys_vb);
	for(int isg=0;isg<lod.skin_group.size();isg++)
	{
		cStaticIndex& s=lod.skin_group[isg];
		if(!IsVisibleMaterialGroup(s))
			continue;

		MaterialAnim& mat_anim=materials[s.imaterial];
		float alpha=mat_anim.opacity*object_opacity*distance_alpha;
		if(alpha<BLEND_STATE_ALPHA_REF/255.0f)
			continue;

		for(int ivg=0;ivg<s.visible_group.size();ivg++)
		{
			cTempVisibleGroup& vg=s.visible_group[ivg];
			if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
			{
				numPolyugons += vg.num_polygon;
			}
		}
	}

	return numPolyugons;
}

bool cObject3dx::IntersectTriangle(const Vect3f& p0,const Vect3f& p1) const
{
	pStatic->CacheBuffersToSystemMem(iLOD);
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	SmallCache<Vect3f,5> cache;
	bool intersect=false;

//	Vect3f pn=p1-p0;
//	Vect3f pnorm=pn;
//	pnorm.Normalize();

	MatXf offset;
	CalcOffsetMatrix(offset,p0,p1);

	int blend_weight=lod.GetBlendWeight();
	cSkinVertexSysMemI skin_vertex;
	sPolygon* pPolygon = lod.sys_ib;
	skin_vertex.SetVB(lod.sys_vb);
	float int2float=1/255.0f;

	for(int isg=0;isg<lod.skin_group.size();isg++)
	{
		cStaticIndex& s=lod.skin_group[isg];
		if(!IsVisibleMaterialGroup(s))
			continue;

		const MaterialAnim& mat_anim=materials[s.imaterial];
		float alpha=mat_anim.opacity*object_opacity*distance_alpha;
		if(alpha<BLEND_STATE_ALPHA_REF/255.0f)
			continue;

		MatXf world[cStaticIndex::max_index];
		int world_num;
		GetWorldPos(s,world,world_num);
		for(int iworld=0;iworld<world_num;iworld++)
			world[iworld]=offset*world[iworld];

		for(int ivg=0;ivg<s.visible_group.size();ivg++)
		{
			cTempVisibleGroup& vg=s.visible_group[ivg];
			if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
			{
				int begin_polygon=s.offset_polygon+vg.begin_polygon;
				int end_polygon=begin_polygon+vg.num_polygon;
				for(int ipolygon=begin_polygon;ipolygon<end_polygon;ipolygon++)
				{
					sPolygon& polygon=pPolygon[ipolygon];
					Vect3f* real_pos[3];
					for(int ivertex=0;ivertex<3;ivertex++)
					{
						int vertex=polygon[ivertex];
						Vect3f* pos=cache.get(vertex);
						if(pos==NULL)
						{
							skin_vertex.Select(vertex);

							Vect3f global_pos;
							if(lod.blend_indices==1)
							{
								int idx=skin_vertex.GetIndex()[0];
								Vect3f& pos=skin_vertex.GetPos();
								world[idx].xformPoint(pos,global_pos);
							}else
							{
								global_pos=Vect3f::ZERO;
								Vect3f& pos=skin_vertex.GetPos();
								Vect3f temp;
								for(int ibone=0;ibone<blend_weight;ibone++)
								{
									int idx=skin_vertex.GetIndex()[ibone];
									float weight=skin_vertex.GetWeight(ibone)*int2float;
									world[idx].xformPoint(pos,temp);
									temp*=weight;
									global_pos+=temp;
								}
							}

							pos=cache.add(vertex,global_pos);
//							gb_RenderDevice3D->DrawPoint(*pos,sColor4c(255,255,255,255));
						}

						real_pos[ivertex]=pos;
					}

					//if(IntersectionRay(*real_pos[0],*real_pos[1],*real_pos[2],p0,pnorm))
					//if(IntersectionRay(*real_pos[0],*real_pos[1],*real_pos[2],Vect3f(0,0,0),Vect3f(0,0,1)))
					if(IntersectionRaySpecial(*real_pos[0],*real_pos[1],*real_pos[2]))
					{
						intersect=true;
						return true;//!!!!!!Only if lock/unlock not needed.
					}
				}
			}
		}
	}

	return intersect;
}
/**/

void cObject3dx::SetLodDistance(float lod12,float lod23)
{
	xassert(lod12<lod23);
	border_lod12_2=sqr(lod12);
	border_lod23_2=sqr(lod23);
}
void cObject3dx::SetHideDistance(float distance)
{
	hideDistance = distance;
}
void cObject3dx::SetUseLod(bool enable)
{
	enable_use_lod=enable;
}

bool cObject3dx::GetUseLod()
{
	return enable_use_lod;
}


void cVisGeneric::SetUseLod(bool enable)
{
	cObject3dx::SetUseLod(enable);
}

bool cVisGeneric::GetUseLod()
{
	return cObject3dx::GetUseLod();
}

int cObject3dx::GetMaterialNum()
{
	return pStatic->materials.size();
}

int cObject3dx::GetNodeNum()
{
	return nodes.size();
}

void cObject3dx::SetShadowType(OBJECT_SHADOW_TYPE type)
{
	pStatic->circle_shadow_enable=type;
	pStatic->circle_shadow_enable_min=min(pStatic->circle_shadow_enable,gb_VisGeneric->GetMaximalShadowObject());
}

void cObject3dx::SetCircleShadowParam(float radius,float height)
{
	pStatic->circle_shadow_radius=radius;
	pStatic->circle_shadow_height=round(height);
}

void cObject3dx::AddCircleShadow()
{
	cTileMap* tilemap=IParent->GetTileMap();
	if(!tilemap)
		return;
	float c_radius=pStatic->circle_shadow_radius*position.scale();
	if(pStatic->circle_shadow_height<0)
	{
		IParent->AddCircleShadow(GetPosition().trans(),c_radius,GetScene()->GetCircleShadowIntensity());
	}else
	{
		const Vect3f& pos=GetPosition().trans();
		int z=tilemap->GetTerra()->GetZ(round(pos.x),round(pos.y));
		sColor4c mul=GetScene()->GetCircleShadowIntensity();
		int dz=z+pStatic->circle_shadow_height-round(pos.z);
		const int delta=20;
		if(dz>0)
		{
			sColor4c intensity=mul;
			if(dz<delta)
			{
				intensity.r=(dz*mul.r)/delta;
				intensity.g=(dz*mul.g)/delta;
				intensity.b=(dz*mul.b)/delta;
			}

			IParent->AddCircleShadow(pos,c_radius,intensity);
		}
	}
}

bool cObject3dx::GetVisibilityTrack(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cStaticNode& snode=pStatic->nodes[nodeindex];
	const cNode3dx& node=nodes[nodeindex];
	const cStaticNodeChain& chain=snode.chains[node.chain];
	if(chain.visibility.values.empty())
		return true;
	bool visible=false;
	int idx=chain.visibility.FindIndex(node.phase);
	chain.visibility.Interpolate(node.phase,&visible,idx);
	return visible;
}

bool cObject3dx::GetVisibilityTrackInterval(int nodeindex,float begin_phase,float end_phase) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cStaticNode& snode=pStatic->nodes[nodeindex];
	const cNode3dx& node=nodes[nodeindex];
	const cStaticNodeChain& chain=snode.chains[node.chain];
	if(chain.visibility.values.empty())
		return true;
	bool visible=false;
	const Interpolator3dxBool& v=chain.visibility;
	int idx_begin=v.FindIndex(begin_phase);
	int idx_end=v.FindIndexRelative(end_phase,idx_begin);

	bool prev=chain.visibility.values[idx_begin].value;

	if(begin_phase<=end_phase)
	{
		for(int i=idx_begin;i<=idx_end;i++)
		{
			bool cur=chain.visibility.values[i].value;
			if(!prev && cur)
				return true;
			prev=cur;
		}
	}else
	{
		for(int i=idx_begin;i<chain.visibility.values.size();i++)
		{
			bool cur=chain.visibility.values[i].value;
			if(!prev && cur)
				return true;
			prev=cur;
		}

		for(int i=0;i<=idx_end;i++)
		{
			bool cur=chain.visibility.values[i].value;
			if(!prev && cur)
				return true;
			prev=cur;
		}
	}

	return false;
}

static void NormalizeMatrix(Mat3f& m)
{
	Vect3f vx,vy,vz;
	vx=m.xcol();vx.Normalize();
	vy=m.ycol();vy.Normalize();
	vz=m.zcol();vz.Normalize();

	m.setXcol(vx);
	m.setYcol(vy);
	m.setZcol(vz);
}

void DrawPointer(MatXf m,sColor4c color,float len,int xyz)
{
	NormalizeMatrix(m.rot());
	Vect3f p,d1,d2;
	switch(xyz)
	{
	case 0:
		p.set(len,0,0);
		d1.set(0,len*0.1f,0);
		d2.set(0,0,len*0.1f);
		break;
	case 1:
		p.set(0,len,0);
		d1.set(len*0.1f,0,0);
		d2.set(0,0,len*0.1f);
		break;
	case 2:
		p.set(0,0,len);
		d1.set(0,len*0.1f,0);
		d2.set(len*0.1f,0,0);
		break;
	default:
		VISASSERT(0);
		return;
	}
	Vect3f front=p*(1-0.2f);
	
	Vect3f p0=m*Vect3f::ZERO;
	Vect3f pp=m*p;

	gb_RenderDevice->DrawLine(p0,pp,color);
	gb_RenderDevice->DrawLine(pp,m*(front+d1),color);
	gb_RenderDevice->DrawLine(pp,m*(front-d1),color);
	
	gb_RenderDevice->DrawLine(pp,m*(front+d2),color);
	gb_RenderDevice->DrawLine(pp,m*(front-d2),color);
}

//Функция пересечения треугольника с линией
bool IntersectionRay(const Vect3f& a,const Vect3f& b,const Vect3f& c,
		const Vect3f& p0,const Vect3f& pn)
{
	//a,b,c - треугольник
	//p0,pn - линия

	//pn=p1-p0;
	//n.x*(x-a.x)+n.y*(y-a.y)+n.z*(z-a.z)=0;
	//(x,y,z)=pn*t+p0
	//n.x*(pn.x*t+p0.x-a.x)+n.y*(pn.y*t+p0.y-a.y)+n.z*(pn.z*t+p0.z-a.z)=0
	//n*pn*t+n*(p0-a)=0
	//t=n*(p0-a)/(n*pn);

	Vect3f nab=(b-a)%pn,nbc=(c-b)%pn,nca=(a-c)%pn;
	float kab=dot(nab,(p0-a));
	float kbc=dot(nbc,(p0-b));
	float kca=dot(nca,(p0-c));
	return (kab<0 && kbc<0 && kca<0)||(kab>0 && kbc>0 && kca>0);
}

//p0=(0,0,0), pn=(0,0,1)
bool IntersectionRaySpecial(const Vect3f& a,const Vect3f& b,const Vect3f& c)
{
	float kab=(b.y-a.y)*a.x-(b.x-a.x)*a.y;
	float kbc=(c.y-b.y)*b.x-(c.x-b.x)*b.y;
	float kca=(a.y-c.y)*c.x-(a.x-c.x)*c.y;
	return (kab<0 && kbc<0 && kca<0)||(kab>0 && kbc>0 && kca>0);
}


void cObject3dx::SetNodePosition(int nodeindex,const Se3f& pos)
{
	ASSERT_NODEINDEX(nodeindex);
	xassert(GetAttribute(ATTR3DX_NOUPDATEMATRIX));
	cNode3dx& s=nodes[nodeindex];
	s.pos.se()=pos;
	s.pos.s=1;
}

void cObject3dx::SetNodePositionMats(int nodeindex,const Mats& pos)
{
	ASSERT_NODEINDEX(nodeindex);
	xassert(GetAttribute(ATTR3DX_NOUPDATEMATRIX));
	cNode3dx& s=nodes[nodeindex];
	s.pos=pos;
}


cObject3dxAnimation::cObject3dxAnimation(cStatic3dx* pStatic_)
:pStatic(pStatic_),updated(false)
{
	nodes.resize(pStatic->nodes.size());
	if(nodes.empty())
	{
		string str="Empty object:";
		str+=pStatic->file_name;
		xassert_s(0,str.c_str());
	}

	materials.resize(pStatic->materials.size());
	for(int i=0;i<materials.size();i++)
	{
		MaterialAnim& m=materials[i];
		m.chain=0;
		m.phase=0;
		m.opacity=pStatic->materials[i].opacity;
	}
}

cObject3dxAnimation::cObject3dxAnimation(cObject3dxAnimation* pObj)
:updated(false)
{
	pStatic = pObj->pStatic;
	nodes = pObj->nodes;
	materials = pObj->materials;
}

void cObject3dxAnimation::SetPhase(float phase)
{
/*
	updated=false;
	int size=nodes.size();
	int i;
	for(i=1;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		node.phase=phase;
	}

	for(i=0;i<materials.size();i++)
	{
		materials[i].phase=phase;
		CalcOpacity(i);
	}
/*/
	for(int igroup=0;igroup<pStatic->animation_group.size();igroup++)
		SetAnimationGroupPhase(igroup,phase);
/**/
}

void cObject3dxAnimation::CalcOpacity(int imaterial)
{
	MaterialAnim& m=materials[imaterial];
	cStaticMaterial& mat=pStatic->materials[imaterial];
	float material_phase=materials[imaterial].phase;
	int material_chain=materials[imaterial].chain;
	if(material_chain>=0 && !mat.chains.empty())
	{
		cStaticMaterialAnimation& mat_chain=mat.chains[material_chain];
		mat_chain.opacity.InterpolateSlow(material_phase,&m.opacity);
	}else
	{
		m.opacity=mat.opacity;
	}
}

void cObject3dxAnimation::SetAnimationGroupPhase(int igroup,float phase)
{
	updated=false;
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		xassert(0 && "Bad group index");
		return;
	}
	AnimationGroup& ag=pStatic->animation_group[igroup];

	int size=ag.nodes.size();
	for(int iag=0;iag<size;iag++)
	{
		int inode=ag.nodes[iag];
		cNode3dx& node=nodes[inode];
		node.phase=phase;
	}

	for(int imat=0;imat<materials.size();imat++)
	{
		cStaticMaterial& mat = pStatic->materials[imat];
		if(pStatic->materials[imat].animation_group_index==igroup)
		{
			materials[imat].phase=phase;
			CalcOpacity(imat);
		}
	}
}

float cObject3dxAnimation::GetAnimationGroupPhase(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animation_group[igroup];

	if(!ag.nodes.empty())
	{
		int inode=ag.nodes[0];
		cNode3dx& node=nodes[inode];
		return node.phase;
	}

	return 0;
}

void cObject3dxAnimation::SetAnimationGroupChain(int igroup,const char* chain_name)
{
	updated=false;
	int chain_index=FindChain(chain_name);
	if(chain_index<0)
	{
		xassert(0);
		return;
	}

	SetAnimationGroupChain(igroup,chain_index);
}

void cObject3dxAnimation::SetAnimationGroupChain(int igroup,int chain_index)
{
	updated=false;
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		string str="Неправильный номер группы анимации. Файл:";
		str+=pStatic->file_name;
		xxassert(0 ,str.c_str());
		return;
	}

	if(chain_index<0 || chain_index>=pStatic->animation_chain.size())
	{
		string str="Неправильный номер анимационной цепочки. Файл:";
		str+=pStatic->file_name;
		xxassert(0 ,str.c_str());
		return;
	}

	AnimationGroup& ag=pStatic->animation_group[igroup];

	int size=ag.nodes.size();
	for(int iag=0;iag<size;iag++)
	{
		int inode=ag.nodes[iag];
		cNode3dx& node=nodes[inode];
		node.chain=chain_index;
		node.index_scale=
		node.index_position=
		node.index_rotation=0;
	}

	for(int imat=0;imat<materials.size();imat++)
	if(pStatic->materials[imat].animation_group_index==igroup)
	{
		materials[imat].chain=chain_index;
	}
}

int cObject3dxAnimation::GetAnimationGroupChain(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animation_group[igroup];
	int size=ag.nodes.size();
	if(size==0)
		return 0;
	int inode=ag.nodes[0];
	cNode3dx& node=nodes[inode];
	return node.chain;
}

int cObject3dxAnimation::FindChain(const char* chain_name)
{
	int size=pStatic->animation_chain.size();
	for(int i=0;i<size;i++)
	{
		cAnimationChain& ac=pStatic->animation_chain[i];
		if(ac.name==chain_name)
			return i;
	}

//	{
//		string str="Не найдена цепочка. Все имена, кроме имен файлов являются чувствительными к регистрам. Файл:";
//		str+=pStatic->file_name;
//		str+="  Chain:";
//		str+=chain_name;
//		xxassert(0 ,str.c_str());
//	}
	return -1;
}

int cObject3dxAnimation::GetChainNumber()
{
	return pStatic->animation_chain.size();
}

cAnimationChain* cObject3dxAnimation::GetChain(int i)
{
	xassert(i>=0 && i<pStatic->animation_chain.size());
	return &pStatic->animation_chain[i];
}

int cObject3dxAnimation::GetChainIndex(const char* chain_name)
{
	int chain=FindChain(chain_name);
	return chain;
}

bool cObject3dxAnimation::SetChain(const char* chain_name)
{
	int chain=FindChain(chain_name);
	if(chain<0)
	{
		xassert(0);
		return false;
	}

	SetChain(chain);
	return true;
}

void cObject3dxAnimation::SetChain(int chain)
{
	updated=false;
	if(chain<0 || chain>=pStatic->animation_chain.size())
	{
		string str="Неправильный номер анимационной цепочки. Файл:";
		str+=pStatic->file_name;
		xxassert(0 ,str.c_str());
	}

	vector<cNode3dx>::iterator it;
	FOR_EACH(nodes,it)
	{
		cNode3dx& node=*it;
		node.chain=chain;
		node.index_scale=
		node.index_position=
		node.index_rotation=0;
	}

	for(int imat=0;imat<materials.size();imat++)
	{
		materials[imat].chain=chain;
	}
}

int cObject3dxAnimation::FindNode(const char* node_name) const
{
	int n_size=pStatic->nodes.size();
	for(int i=0;i<n_size;i++)
	{
		const cStaticNode& node=pStatic->nodes[i];
		if(node.name==node_name)
			return i;
	}

	return -1;
}

const Se3f& cObject3dxAnimation::GetNodePosition(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cNode3dx& s=nodes[nodeindex];
	return s.pos.se();
}

const Mats& cObject3dxAnimation::GetNodePositionMats(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cNode3dx& s=nodes[nodeindex];
	return s.pos;
}

const MatXf& cObject3dxAnimation::GetNodePositionMat(int nodeindex) const
{
	static MatXf mat;
	mat.set(GetNodePosition(nodeindex));
	return mat;
}

int cObject3dxAnimation::GetNodeNumber() const
{
	return pStatic->nodes.size();
}

int cObject3dxAnimation::GetParentNumber(int node_index) const
{
	xassert(node_index>=0 && node_index<nodes.size());
	return pStatic->nodes[node_index].iparent;
}

bool cObject3dxAnimation::CheckDependenceOfNodes(int dependent_node_index, int node_index) const
{
	xassert(node_index>=0 && node_index<nodes.size());
	xassert(dependent_node_index>=0 && dependent_node_index<nodes.size());
	int index = dependent_node_index;
	do{
		index = GetParentNumber(index);
		if(index == node_index)
			return true;
	}while(index > -1);
	return false;
}

const char* cObject3dxAnimation::GetNodeName(int node_index) const
{
	return pStatic->nodes[node_index].name.c_str();
}

void cNode3dx::CalculatePos(cStaticNodeChain& chain,Mats& pos)
{
	float xyzs[4];
	index_scale=chain.scale.FindIndexRelative(phase,index_scale);
	index_position=chain.position.FindIndexRelative(phase,index_position);
	index_rotation=chain.rotation.FindIndexRelative(phase,index_rotation);

	chain.scale.Interpolate(phase,&pos.scale(),index_scale);
	chain.position.Interpolate(phase,(float*)&pos.trans(),index_position);
	chain.rotation.Interpolate(phase,xyzs,index_rotation);
	pos.rot().set(xyzs[3],-xyzs[0],-xyzs[1],-xyzs[2]);
}

void cObject3dxAnimation::UpdateMatrix(Mats& position,vector<c3dxAdditionalTransformation>& additional_transformations)
{
	if(updated)
		return;
	updated=true;
	xassert(!nodes.empty());
	nodes[0].pos=position;

	int size=nodes.size();
	for(int i=1;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		cStaticNode& sn=pStatic->nodes[i];

		if(sn.iparent<0)//временно
			continue;

		xassert(sn.iparent>=0 && sn.iparent<size);
		cNode3dx& parent=nodes[sn.iparent];

		if(node.chain >= sn.chains.size())
			continue;

		cStaticNodeChain& chain=sn.chains[node.chain];

		Mats pos;
		node.CalculatePos(chain,pos);

		if(node.IsAdditionalTransform())
		{
		/*Если вращение справа, то.
			node.pos.rot()=additional_transformations.rot()*node.pos.rot()
		*/
			c3dxAdditionalTransformation& t=additional_transformations[node.additional_transform];
			Mats tmp=pos;
			pos.mult(tmp,t.mat);
			node.pos.mult(parent.pos,pos);
		}else
		{
			node.pos.mult(parent.pos,pos);
		}
/* //Тест начального положения объекта
		MatXf begin_pos=sn.inv_begin_pos;
		begin_pos.Invert();
		Mats ms_begin_pos;
		ms_begin_pos.set(begin_pos);
		node.pos.mult(position,ms_begin_pos);
/**/
	}

}

void cObject3dxAnimationSecond::lerp(cObject3dxAnimation* to)
{
	int size=nodes.size();
	for(int i=1;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		cNode3dx& node_out=to->nodes[i];
		cStaticNode& sn=pStatic->nodes[i];
		float k=interpolation[i];

		if(sn.iparent<0)//временно
			continue;

		Mats tmp;
		tmp.trans().interpolate(node_out.pos.trans(),node.pos.trans(),k);
		::lerp(tmp.rot(),node_out.pos.rot(), node.pos.rot(), k);
		tmp.scale()=LinearInterpolate(node_out.pos.scale(),node.pos.scale(),k);
		node_out.pos=tmp;
	}

	//Здесь материалы интерполировать
}


void cObject3dx::UpdateAndLerp(Mats& position,vector<c3dxAdditionalTransformation>& additional_transformations)
{
	if(updated && pAnimSecond->updated)
		return;
	updated=true;
	xassert(!nodes.empty());
	nodes[0].pos=position;
	pAnimSecond->nodes[0].pos=position;

	int size=nodes.size();
	for(int i=1;i<size;i++)
	{
		cNode3dx& node=nodes[i];
		cNode3dx& node1=pAnimSecond->nodes[i];
		cStaticNode& sn=pStatic->nodes[i];

		if(sn.iparent<0)//временно
			continue;

		xassert(sn.iparent>=0 && sn.iparent<size);
		cNode3dx& parent=nodes[sn.iparent];

		if(node.chain >= sn.chains.size())
			continue;
		if(node1.chain >= sn.chains.size())
			continue;

		cStaticNodeChain& chain=sn.chains[node.chain];
		cStaticNodeChain& chain1=sn.chains[node1.chain];

		Mats pos,pos1;
		node.CalculatePos(chain,pos);
		node1.CalculatePos(chain1,pos1);

		float k=pAnimSecond->interpolation[i];

		Mats lerp_pos;
		lerp_pos.trans().interpolate(pos.trans(),pos1.trans(),k);
		::lerp(lerp_pos.rot(),pos.rot(), pos1.rot(), k);
		lerp_pos.scale()=LinearInterpolate(pos.scale(),pos1.scale(),k);

		if(node.IsAdditionalTransform())
		{
			c3dxAdditionalTransformation& t=additional_transformations[node.additional_transform];
			Mats tmp=lerp_pos;
			lerp_pos.mult(tmp,t.mat);
			node.pos.mult(parent.pos,lerp_pos);
		}else
		{
			node.pos.mult(parent.pos,lerp_pos);
		}
	}
}


cObject3dxAnimationSecond::cObject3dxAnimationSecond(cStatic3dx* pStatic_)
: cObject3dxAnimation(pStatic_)
{
	interpolation.resize(pStatic->nodes.size());
	for(int i=0;i<interpolation.size();i++)
		interpolation[i]=0.0f;
}
cObject3dxAnimationSecond::cObject3dxAnimationSecond(cObject3dxAnimationSecond* pObj)
: cObject3dxAnimation(pObj)
{
	interpolation = pObj->interpolation;
}

float cObject3dxAnimationSecond::GetAnimationGroupInterpolation(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animation_group[igroup];

	if(!ag.nodes.empty())
	{
		int inode=ag.nodes[0];
		return interpolation[inode];
	}

	return 0;
}

bool cObject3dxAnimationSecond::IsInterpolation()
{
	vector<float>::iterator it;
	FOR_EACH(interpolation,it)
	{
		if(*it>1e-3f)
			return true;
	}

	return false;
}

void cObject3dxAnimationSecond::SetAnimationGroupInterpolation(int igroup,float kinterpolation)
{
	if(!(igroup>=0 && igroup<pStatic->animation_group.size()))
	{
		xassert(0 && "Bad group index");
		return;
	}
	AnimationGroup& ag=pStatic->animation_group[igroup];
	xassert(kinterpolation>=0 && kinterpolation<=1);

	int size=ag.nodes.size();
	for(int iag=0;iag<size;iag++)
	{
		int inode=ag.nodes[iag];
		interpolation[inode]=kinterpolation;
	}

}

void cObject3dx::EffectObserverLink3dx::Link(class cObject3dx* object_,int inode,bool set_scale_)
{
	if(observer)observer->BreakLink(this);

	parent_object=NULL;
	parent_node=0;
	set_scale=false;

	if(object_)
	{
		parent_object=object_;
		parent_node=inode;
		set_scale=set_scale_;

		if(parent_object)
			parent_object->AddLink(this);
	}
}

void cObject3dx::EffectObserverLink3dx::Update()
{
	const Mats& mat=parent_object->GetNodePositionMats(parent_node);
	this_object->SetPosition(mat.se());
	if(set_scale)
		this_object->SetScale(mat.scale());

}

void cObject3dx::LinkTo(cObject3dx* object,int inode,bool set_scale)
{
	link3dx.Link(object,inode,set_scale);
}

struct VertexRange
{
	cTempVisibleGroup* vg;
	cStaticIndex* s;
};

void cObject3dx::GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	if (pos.size() != norm.size())
		norm.resize(pos.size());

	int size=lod.skin_group.size();
	int num_polygons = 0;

	vector<VertexRange> sti;
	for(int i=0;i<size;i++)
	{
		cStaticIndex& s=lod.skin_group[i];
		for(int ivg=0;ivg<s.visible_group.size();ivg++)
		{
			cTempVisibleGroup& vg=s.visible_group[ivg];
			if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
			{
				VertexRange rng;
				rng.vg = &vg;
				rng.s = &s;
				sti.push_back(rng);
				num_polygons += vg.num_polygon;
			}
		}
	}

	if(num_polygons==0)
	{
		for (int i=0; i<pos.size(); i++)
		{
			pos[i] = position.trans();
			norm[i].set(0,0,1);
		}
		return;
	}

	int blend_weight=lod.GetBlendWeight();
	cSkinVertex skin_vertex(blend_weight,pStatic->bump,pStatic->is_uv2);
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
	sPolygon* pPolygon = gb_RenderDevice->LockIndexBuffer(lod.ib);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());
	float int2float=1/255.0f;
	MatXf world[cStaticIndex::max_index];
//	int world_num;

	for (int i=0; i<pos.size(); i++)
	{
		int ply = graphRnd()%num_polygons;
		int v = graphRnd()%3;

		cTempVisibleGroup* vg = NULL;
		cStaticIndex* s = NULL;
		int old_num = 0;
		int vertex = 0;
		for (int j=0; j<sti.size(); j++)
		{
			vg = sti[j].vg;
			s = sti[j].s;
			if (ply < vg->num_polygon+old_num)
			{
				vertex = *(v+&(pPolygon[vg->begin_polygon +s->offset_polygon+ ply-old_num].p1));
				break;
			}
			old_num += vg->num_polygon;
		}

		//GetWorldPos(*s,world,world_num);

		MatXf  mat_world;
		skin_vertex.Select(vertex);
		Vect3f global_pos;
		Vect3f global_norm;
		if(lod.blend_indices==1)
		{
			int idx=skin_vertex.GetIndex()[0];
			GetWorldPos(*s,mat_world,idx);

			Vect3f pos=skin_vertex.GetPos();
			global_pos=mat_world*pos;

			Vect3f nrm=skin_vertex.GetNorm();
			global_norm=mat_world.rot()*nrm;
		}else
		{
			global_pos=Vect3f::ZERO;
			global_norm=Vect3f::ZERO;
			Vect3f pos=skin_vertex.GetPos();
			Vect3f nrm=skin_vertex.GetNorm();
			for(int ibone=0;ibone<blend_weight;ibone++)
			{
				int idx=skin_vertex.GetIndex()[ibone];
				GetWorldPos(*s,mat_world,idx);
				float weight=skin_vertex.GetWeight(ibone)*int2float;

				global_pos+=(mat_world*pos)*weight;
				global_norm+=(mat_world.rot()*nrm)*weight;
			}
		}
		pos[i] = global_pos;
		norm[i] = global_norm;

	}
	gb_RenderDevice->UnlockIndexBuffer(lod.ib);
	gb_RenderDevice->UnlockVertexBuffer(lod.vb);
}

void cObject3dx::GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node)
{
	cStatic3dx::ONE_LOD& lod=pStatic->lods[0];
	all.triangles.clear();
	all.visible_points.clear();
	all.positions.clear();
	all.normals.clear();
	all.uv.clear();
	if(!lod.vb.IsInit() || !lod.ib.IsInit())
		return;

	bool zero_pos=(tif_flags&TIF_ZERO_POS)?true:false;
	bool one_scale=(tif_flags&TIF_ONE_SCALE)?true:false;
	int num_points=0;
	int size=lod.skin_group.size();
	int i;
	for(i=0;i<size;i++)
	{
		cStaticIndex& s=lod.skin_group[i];
		num_points+=s.num_vertex;
	}

	xassert(num_points<=lod.vb.GetNumberVertex());

	if(tif_flags&TIF_VISIBLE_POINTS)
		all.visible_points.resize(num_points);
	if(tif_flags&TIF_POSITIONS)
		all.positions.resize(num_points);
	if(tif_flags&TIF_NORMALS)
		all.normals.resize(num_points);
	if(tif_flags&TIF_UV)
		all.uv.resize(num_points);
	if(tif_flags&TIF_VISIBLE_POINTS)
	{
		for(i=0;i<num_points;i++)
			all.visible_points[i]=false;
	}

	all.selected_node.resize(num_points,false);

	if(tif_flags&(TIF_POSITIONS|TIF_NORMALS|TIF_UV))
	{
		Mats old_pos;
		if(zero_pos || one_scale)
		{
			old_pos=position;
			if(one_scale)
			{
				xassert(!GetAttribute(ATTR3DX_NOUPDATEMATRIX));
				SetScale(1);
			}
			if(zero_pos)
			{
				xassert(!GetAttribute(ATTR3DX_NOUPDATEMATRIX));
				SetPosition(Se3f::ID);
			}
			Update();
		}

		int pointindex=0;
		cSkinVertex skin_vertex(lod.GetBlendWeight(),pStatic->bump,pStatic->is_uv2);
		void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
		skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

		MatXf world[cStaticIndex::max_index];
		int world_num;
		for(i=0;i<size;i++)
		{
			cStaticIndex& s=lod.skin_group[i];

			GetWorldPos(s,world,world_num);

			int max_vertex=s.offset_vertex+s.num_vertex;
			int blend_weight=lod.GetBlendWeight();
			float int2float=1/255.0f;
			for(int vertex=s.offset_vertex;vertex<max_vertex;vertex++)
			{
				skin_vertex.Select(vertex);
				Vect3f global_pos;
				Vect3f global_norm;
				bool found_selected=false;
				if(lod.blend_indices==1)
				{
					int idx=skin_vertex.GetIndex()[0];
					Vect3f pos=skin_vertex.GetPos();
					global_pos=world[idx]*pos;

					Vect3f nrm=skin_vertex.GetNorm();
					global_norm=world[idx].rot()*nrm;
					int node=s.node_index[idx];
					found_selected=(node==selected_node);
				}else
				{
					global_pos=Vect3f::ZERO;
					global_norm=Vect3f::ZERO;
					Vect3f pos=skin_vertex.GetPos();
					Vect3f nrm=skin_vertex.GetNorm();
					for(int ibone=0;ibone<blend_weight;ibone++)
					{
						int idx=skin_vertex.GetIndex()[ibone];
						BYTE byte_weight=skin_vertex.GetWeight(ibone);
						float weight=byte_weight*int2float;
						global_pos+=(world[idx]*pos)*weight;
						global_norm+=(world[idx].rot()*nrm)*weight;

						if(byte_weight>0)
						{
							int node=s.node_index[idx];
							found_selected=found_selected || (node==selected_node);
						}
					}
				}

				if(tif_flags&TIF_POSITIONS)
					all.positions[pointindex]=global_pos;
				if(tif_flags&TIF_NORMALS)
				{
					global_norm.Normalize();
					all.normals[pointindex]=global_norm;
				}

				if(tif_flags&TIF_UV)
					all.uv[pointindex]=skin_vertex.GetTexel();
				all.selected_node[pointindex]=found_selected;
				pointindex++;
			}
		}

		xassert(num_points==pointindex);

		gb_RenderDevice->UnlockVertexBuffer(lod.vb);

		if(zero_pos || one_scale)
		{
			position=old_pos;
			Update();
		}
	}

	if(tif_flags&(TIF_TRIANGLES|TIF_VISIBLE_POINTS))
	{
		int size=lod.skin_group.size();
		int num_triangles=0;
		for(i=0;i<size;i++)
		{
			cStaticIndex& s=lod.skin_group[i];
			bool is_visible=false;
			for(int ivg=0;ivg<s.visible_group.size();ivg++)
			{
				cTempVisibleGroup& vg=s.visible_group[ivg];
				if(vg.visible&iGroups[vg.visible_set].p->visible_shift)
				{
					num_triangles+=vg.num_polygon;
				}
			}
		}

		if(tif_flags&TIF_TRIANGLES)
			all.triangles.resize(num_triangles);
		int offset_triangles=0;
		sPolygon *IndexPolygon=gb_RenderDevice->LockIndexBuffer(lod.ib);
		for(int igroup=0;igroup<size;igroup++)
		{
			cStaticIndex& s=lod.skin_group[igroup];
			bool is_visible=false;
			for(int ivg=0;ivg<s.visible_group.size();ivg++)
			{
				cTempVisibleGroup& vg=s.visible_group[ivg];
				if(!(vg.visible&iGroups[vg.visible_set].p->visible_shift))
					continue;

				int offset_buf=vg.begin_polygon+s.offset_polygon;

				for(int ipolygon=0;ipolygon<vg.num_polygon;ipolygon++)
				{
					sPolygon p=IndexPolygon[offset_buf+ipolygon];
					if(tif_flags&TIF_TRIANGLES)
						all.triangles[offset_triangles+ipolygon]=p;
					if(tif_flags&TIF_VISIBLE_POINTS)
					{
						for(int ip=0;ip<3;ip++)
						{
							int idx=p[ip];
							all.visible_points[idx]=true;
						}
					}
				}
				
				offset_triangles+=vg.num_polygon;
			}
		}
	/*
		for(int i=0;i<pStatic->ib_polygon;i++)
		{
			sPolygon p=IndexPolygon[i];
			if(tif_flags&TIF_TRIANGLES)
				all.triangles[i]=p;
			if(tif_flags&TIF_VISIBLE_POINTS)
			{
				for(int ip=0;ip<3;ip++)
				{
					int idx=p[ip];
					all.visible_points[idx]=true;
				}
			}
		}
	*/
		gb_RenderDevice->UnlockIndexBuffer(lod.ib);
	}
}

const char* cObject3dx::GetVisibilitySetName(C3dxVisibilitySet iset)
{
	xassert(iset.iset>=0 && iset.iset<pStatic->visible_sets.size());
	return pStatic->visible_sets[iset.iset]->name.c_str();
}

C3dxVisibilitySet cObject3dx::GetVisibilitySetIndex(const char* set_name)
{
	for(int i=0;i<GetVisibilitySetNumber();i++)
	{
		if(strcmp(GetVisibilitySetName(C3dxVisibilitySet(i)),set_name)==0)
			return C3dxVisibilitySet(i);
	}
	return C3dxVisibilitySet::BAD;
}

cStaticVisibilitySet* cObject3dx::GetVisibilitySet(C3dxVisibilitySet iset)
{
	xassert(iset.iset>=0 && iset.iset<pStatic->visible_sets.size());
	return pStatic->visible_sets[iset.iset];
}

void cObject3dx::SetLod(int ilod)
{
	SetAttribute(ATTRUNKOBJ_NO_USELOD);
	iLOD=ilod;
	UpdateVisibilityGroups();
}

void cObject3dx::SetOpacity(float opacity)
{
	object_opacity=opacity;
}

float cObject3dx::GetOpacity()const
{
	return object_opacity;
}

void cObject3dx::DrawAll(cCamera *pCamera)
{
	bool is_opacity,is_noopacity;
	CalcIsOpacity(is_opacity,is_noopacity);
	eSceneNode old_pass=pCamera->GetCameraPass();
	if(is_noopacity)
	{
		pCamera->SetCameraPass(SCENENODE_OBJECT);
		Draw(pCamera);
	}

	if(is_opacity)
	{
		pCamera->SetCameraPass(SCENENODE_OBJECTSORT);
		DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
		gb_RenderDevice3D->SetRenderState( RS_ZWRITEENABLE, FALSE );
		Draw(pCamera);
		gb_RenderDevice3D->SetRenderState( RS_ZWRITEENABLE, TRUE );
		gb_RenderDevice3D->SetRenderState( D3DRS_CULLMODE, old_cullmode );
	}
	pCamera->SetCameraPass(old_pass);
}

void cObject3dx::AddLight(cUnkLight* light)
{
	if(GetAttr(ATTRUNKOBJ_IGNORE))
		return;
	//Может пооптимальнее потом написать.
	const int maxc=2;
	if(point_light.size()>=maxc)
	{
		xassert(point_light.size()==maxc);
		float new_r=GetPosition().trans().distance2(light->GetPosition().trans());
		float max_radius=0;
		int max_i=0;
		for(int i=0;i<maxc;i++)
		{
			cUnkLight* l=point_light[i];
			float cur_radius=GetPosition().trans().distance2(l->GetPosition().trans());
			if(cur_radius>max_radius)
			{
				max_radius=cur_radius;
				max_i=i;
			}
		}

		if(new_r<max_radius)
		{
			point_light[max_i]=light;
		}
	}else
	{
		point_light.push_back(light);
	}
}

void cObject3dxAnimation::GetNodeOffset(int nodeindex,Mats& offset,int& parent_node)
{
	int size=nodes.size();
	ASSERT_NODEINDEX(nodeindex);

	cNode3dx& node=nodes[nodeindex];
	cStaticNode& sn=pStatic->nodes[nodeindex];
	parent_node=sn.iparent;

	if(sn.iparent<0)
	{
		offset.rot()=QuatF::ID;
		offset.trans()=Vect3f::ID;
		return;
	}

	xassert(sn.iparent>=0 && sn.iparent<size);
	xassert(node.chain < sn.chains.size());

	cStaticNodeChain& chain=sn.chains[node.chain];

	Mats& pos=offset;
	float xyzs[4];
	node.index_scale=chain.scale.FindIndexRelative(node.phase,node.index_scale);
	node.index_position=chain.position.FindIndexRelative(node.phase,node.index_position);
	node.index_rotation=chain.rotation.FindIndexRelative(node.phase,node.index_rotation);

	chain.scale.Interpolate(node.phase,&pos.scale(),node.index_scale);
	chain.position.Interpolate(node.phase,(float*)&pos.trans(),node.index_position);
	chain.rotation.Interpolate(node.phase,xyzs,node.index_rotation);
	pos.rot().set(xyzs[3],-xyzs[0],-xyzs[1],-xyzs[2]);
}
void cObject3dx::GetTextureNames(vector<string>& texture_names)
{
	for(int i=0; i<material_textures.size();i++)
	{
		MaterialTexture& mtex = material_textures[i];
		if(!mtex.diffuse_texture)
			continue;
		bool found = false;
		for(int j=0; j<texture_names.size();j++)
		{
			if (mtex.diffuse_texture->GetName() == texture_names[j])
			{
				found = true;
				break;
			}
		}
		if(!found)
			texture_names.push_back(mtex.diffuse_texture->GetName());
	}
}


void cObject3dx::SetTextureLerpColor(const sColor4f& lerp_color_)
{
	lerp_color=lerp_color_;
}

sColor4f cObject3dx::GetTextureLerpColor() const
{
	return lerp_color;
}

void cObject3dx::CalcBoundSphere()
{
	SetPosition(MatXf::ID);
	Update();
	pStatic->bound_spheres.clear();
	for(int inode=0;inode<nodes.size();inode++)
	{
		TriangleInfo all;
		GetTriangleInfo(all,TIF_POSITIONS|TIF_ZERO_POS|TIF_ONE_SCALE,inode);

		sBox6f box;
		box.min.set(1e30f,1e30f,1e30f);
		box.max.set(-1e30f,-1e30f,-1e30f);
		bool found=false;

		for(int ipos=0;ipos<all.positions.size();ipos++)
		if(all.selected_node[ipos])
		{
			found=true;
			box.AddBound(all.positions[ipos]);
		}

		if(found)
		{
			Vect3f center=(box.max+box.min)*0.5f;
			float radius=0;

			for(int ipos=0;ipos<all.positions.size();ipos++)
			if(all.selected_node[ipos])
			{
				Vect3f d=all.positions[ipos]-center;
				float r=d.norm();
				radius=max(radius,r);
			}

			MatXf pos;
			pos.set(GetNodePosition(inode));
			pos.invert();
			cStatic3dx::BoundSphere b;
			b.node_index=inode;
			b.position=pos*center;
			b.radius=radius;
			pStatic->bound_spheres.push_back(b);
		}
	}
}

sBox6f cObject3dx::CalcDynamicBoundBox(MatXf world_view)
{
	sBox6f box;
	box.SetInvalidBox();

	for(int i=0;i<pStatic->bound_spheres.size();i++)
	{
		cStatic3dx::BoundSphere& b=pStatic->bound_spheres[i];

		MatXf pos;
		pos.set(GetNodePosition(b.node_index));
		pos.trans()+=(pos.rot()*b.position)*position.s;
		float radius=position.s*b.radius;
		Vect3f pworld=world_view*pos.trans();

		box.min.x=min(pworld.x-radius,box.min.x);
		box.min.y=min(pworld.y-radius,box.min.y);
		box.min.z=min(pworld.z-radius,box.min.z);

		box.max.x=max(pworld.x+radius,box.max.x);
		box.max.y=max(pworld.y+radius,box.max.y);
		box.max.z=max(pworld.z+radius,box.max.z);
	}

	return box;
}

bool cObject3dx::IsLogicBound(int nodeindex)
{
	sBox6f box=GetLogicBoundScaledUntransformed(nodeindex);
	return box.xmin()<=box.xmax();
}

sBox6f cObject3dx::GetLogicBoundScaledUntransformed(int nodeindex)
{
	if(pStatic->local_logic_bounds.empty())
		return sBox6f::BAD;
	Update();
	xassert(nodeindex>=0 && nodeindex<pStatic->local_logic_bounds.size());
	sBox6f box=pStatic->local_logic_bounds[nodeindex];
	float s=GetNodePositionMats(nodeindex).scale();
	box.min*=s;
	box.max*=s;
	return box;
}
cTexture* cObject3dx::GetDiffuseTexture(int num_mat)const
{
	return material_textures[num_mat].diffuse_texture;
}

void cObject3dx::SetDiffuseTexture(int num_mat,cTexture* texture)
{
	cTexture*& diffuse=material_textures[num_mat].diffuse_texture;
	RELEASE(diffuse);
	diffuse=texture;
	if(diffuse)
		diffuse->AddRef();
}


bool cObject3dx::IsHaveSkinColorMaterial()
{
	return isHaveSkinColorMaterial_;
}
