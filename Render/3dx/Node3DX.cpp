#include "StdAfxRD.h"
#include "node3dx.h"
#include "Render\shader\shaders.h"
#include "nparticle.h"
#include "scene.h"
#include "TileMap.h"
#include "D3DRender.h"
#include "VisGeneric.h"
#include "cCamera.h"
#include "OcclusionQuery.h"
#include "XMath\SafeMath.h"
#include "Terra\vmap.h"

float AlphaMaxiumBlend=0.95f;
float AlphaMiniumShadow=0.0f;

/*
Оптимизация.
!! На втором и третьем LOD - уменьшать количество костей до 2 и 1
*/

Shader3dx::Shader3dx()
{
	vsSkinSceneShadow=0;
	psSkinSceneShadow=0;
	vsSkinBumpSceneShadow=0;
	psSkinBumpSceneShadow=0;
	vsSkinReflectionSceneShadow=0;
	psSkinReflectionSceneShadow=0;
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
	}
	else if(gb_RenderDevice3D->dtAdvanceOriginal && gb_RenderDevice3D->dtAdvanceOriginal->GetID()==DT_GEFORCEFX){
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

	vsSkinFur=new VSSkinFur;
	vsSkinFur->Restore();
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

	delete vsSkinFur;
}

Shader3dx* pShader3dx=0;

void Done3dxshader()
{
	delete pShader3dx;
	pShader3dx=0;
}

void Init3dxshader()
{
	Done3dxshader();
	pShader3dx=new Shader3dx;
}

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
	void Test(Camera* camera,Vect3f& pos)
	{
		is_visible_in_camera=camera->TestVisible(pos,0)!=VISIBLE_OUTSIDE;
		occlusionQuery.Test(pos);
	}
};

bool cObject3dx::QueryVisibleIsVisible()
{
	if(pOcclusionQuery==0)
		pOcclusionQuery=new cOcclusionSilouette;
	return pOcclusionQuery->IsVisible(scene_->GetDeltaTimeInt());
}

void cObject3dx::QueryVisible(Camera* camera)
{
	if(pOcclusionQuery)
	{
		Vect3f pos;
		//camera->GetWorldK() все же не совсем то, что направление от камеры на центр объекта, но во многих случаях прокатывает.
		if(false)
		{
			Vect3f pos=position.trans()-(GetBoundRadius()*1.1f)*camera->GetWorldK();
			pos+=position.rot().xform((pStatic->boundBox.min+pStatic->boundBox.max)*(0.5f*position.scale()));
		}else
		{
			Vect3f center_pos=position.trans();
/*
			Vect3f bound_pos=position.rot().xform((pStatic->bound_box.min+pStatic->bound_box.max)*(0.5f*position.s));
			center_pos+=bound_pos;
/*/
			center_pos+=silouette_center*position.scale();
/**/
			Vect3f dir=camera->GetPos()-center_pos;
			dir.normalize();
			pos=center_pos+(GetBoundRadius()*1.1f)*dir;
		}

		pOcclusionQuery->Test(camera,pos);
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
	pos = Mats::ID;
	phase=0;
	chainIndex=0;
	index_scale=0;
	index_position=0;
	index_rotation=0;
	additional_transform=255;
}

cObject3dx::cObject3dx(cStatic3dx* pStatic_, bool interpolate)
:c3dx(KIND_OBJ_3DX),cObject3dxAnimation(pStatic_)
{
	border_lod12_2=sqr(200.0f);
	border_lod23_2=sqr(600.0f);
	hideDistance = 500.0f;

	num_out_polygons=0;
	pOcclusionQuery=0;
	updated_=false;
	treeUpdated_ = false;
	isTree_ = false; // Временно, потом переделать!!!!!!!!
	silhouette_index = 0;

	link3dx.SetParent(this);
	setAttribute(ATTRCAMERA_REFLECTION|ATTRCAMERA_FLOAT_ZBUFFER);
	distance_alpha=1.0f;

	ambient.set(1,1,1,0);
	diffuse.set(1,1,1,0);
	specular.set(1,1,1,0);
	lerp_color.set(1,1,1,0);
	object_opacity=1;
	skin_color.set(255,255,255,0);
	position = Mats::ID;
	isSkinColorSet_ = false;
	iLOD=0;
	leaves_ = 0;
	unvisible_ = false;
	furScalePhase_ = 1.0f;
	furAlphaPhase_ = 1.0f;

	visibilityGroups_.resize(pStatic->visibilitySets_.size());
	UpdateVisibilityGroups();

	material_textures.resize(pStatic->materials.size());

	pAnimSecond = 0;
	if(interpolate)//Переделать GetAllPoints
		pAnimSecond = new cObject3dxAnimationSecond(pStatic);

	if(!pStatic->isBoundBoxInited){
		pStatic->isBoundBoxInited=true;
		if(!pStatic->is_logic)
			CalcBoundingBox();
		else
			pStatic->boundRadius=pStatic->boundBox.max.distance(pStatic->boundBox.min)*0.5f;
	}

	if(!pStatic->is_logic && !pStatic->voxelBox.valid())
		pStatic->voxelBox.create(this);

	silouette_center = pStatic->boundBox.center();

	isHaveSkinColorMaterial_ = false;
	for(int i =0; i<pStatic->materials.size(); i++)
		if(pStatic->materials[i].is_skinned)
			isHaveSkinColorMaterial_=  true;
	
	LoadTexture(false);
	effects.resize(pStatic->effects.size());

	SetPosition(MatXf::ID);
	Update();
}

cObject3dx::cObject3dx(cObject3dx* pObj)
:c3dx(KIND_OBJ_3DX),cObject3dxAnimation(pObj)
{
	num_out_polygons=0;
	border_lod12_2=pObj->border_lod12_2;
	border_lod23_2=pObj->border_lod23_2;
	hideDistance = pObj->hideDistance;

	pOcclusionQuery=0;
	updated_=false;
	treeUpdated_ = false;
	silhouette_index = pObj->silhouette_index;
	pStatic->AddRef();
	setAttribute(pObj->getAttribute());
	clearAttribute(ATTRUNKOBJ_ATTACHED);
	ambient = pObj->ambient;
	diffuse = pObj->diffuse;
	specular = pObj->specular;
	lerp_color=pObj->lerp_color;
	object_opacity=pObj->object_opacity;
	position = pObj->position;
	iLOD = pObj->iLOD;
	visibilityGroups_ = pObj->visibilityGroups_;
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
	else pAnimSecond = 0;
	
	effects.resize(pStatic->effects.size());
	SetPosition(pObj->GetPosition());
	silouette_center=pObj->silouette_center;
	leaves_ = 0;
	furScalePhase_ = pObj->furScalePhase_;
	furAlphaPhase_ = pObj->furAlphaPhase_;
	unvisible_ = false;
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
	RELEASE(leaves_);

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
	updated_=false;
	treeUpdated_ = false;
	position.scale() = scale_;

//	if(GetBoundRadius()>1000)
//		console()<< cConsole::LOW << "&Balmer" << "Big object " <<pStatic->file_name << cConsole::END;

	//Фикс (кривой) для постоянно генерирующихся эффектов
	vector<EffectData>::iterator it;
	FOR_EACH(effects,it)
	{
		EffectData& d=*it;
		RELEASE(d.pEffect);
	}

	for(int i=0;i<lights.size();i++){
		StaticLight& sl=pStatic->lights[i];
		lights[i]->SetRadius(sl.atten_end*GetScale());
	}
}

void cObject3dx::SetPosition(const Se3f&  pos)
{
	MTAccess();
#ifdef _DEBUG
	xassert(fabsf(pos.rot().norm2()-1)<1.5e-2f);
#endif
	updated_=false;
	treeUpdated_ = false;
	position.se()=pos;
}

void cObject3dx::SetPosition(const MatXf& pos)
{
	updated_=false;
	treeUpdated_ = false;
#ifdef _DEBUG
	float s=pos.rot().xrow().norm();
	xassert(fabsf(s-1)<1e-2f);
#endif
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

StaticVisibilityGroup* cObject3dx::GetVisibilityGroup(VisibilitySetIndex iset)
{
	xassert(iset >= 0 && iset < pStatic->visibilitySets_.size());
	return visibilityGroups_[iset].visibilityGroup;
}

VisibilityGroupIndex cObject3dx::GetVisibilityGroupIndex(VisibilitySetIndex iset)
{
	xassert(iset >= 0 && iset < visibilityGroups_.size());
	return visibilityGroups_[iset].visibilityGroupIndex;
}


void cObject3dx::setAttribute(int attribute)
{
	if(attribute&ATTR3DX_HIDE_LIGHTS)
		updated_=false;

	if(attribute&ATTRUNKOBJ_IGNORE)
	{
		for(int i=0;i<lights.size();i++)
			lights[i]->setAttribute(ATTRUNKOBJ_IGNORE);
		num_out_polygons=0;
	}

	__super::setAttribute(attribute);
}

void cObject3dx::clearAttribute(int attribute)
{
	if(attribute&ATTR3DX_HIDE_LIGHTS)
		updated_=false;
	__super::clearAttribute(attribute);
}


void cObject3dx::Update()
{
	if(updated_)
		return;

	if(isTree_ && treeUpdated_)
		return;
	treeUpdated_ = true;

	if(!getAttribute(ATTR3DX_NOUPDATEMATRIX)){
		if(pAnimSecond && pAnimSecond->IsInterpolation())
			UpdateAndLerp();
		else{
			UpdateMatrix(position,additional_transformations);
			RestoreSecondAnimationUserTransform();
		}
	}

	if(scene()){
		//update leaves
		bool first=!leaves_ && !pStatic->leaves.empty();
		if(first){
			leaves_ = new Leaves();
			for(int i=0; i<pStatic->leaves.size(); i++)
				leaves_->AddLeaf();
			scene()->AttachObj(leaves_);
			if(pStatic->leaves.size() > 0)
				isTree_ = true;
			leaves_->SetPosition(GetPosition());
		}
		if(leaves_){
			leaves_->SetLod(iLOD);
			leaves_->SetDistanceLod(border_lod12_2,border_lod23_2);
			leaves_->SetPosition(GetPosition());
			for(int i=0; i<leaves_->GetLeavesCount(); i++)
			{
				StaticLeaf& sl=pStatic->leaves[i];
				cNode3dx& node=nodes_[sl.inode];
				Leaf* leaf = leaves_->GetLeaf(i);
				MatXf xpos;
				node.pos.copy_right(xpos);
				leaf->pos = xpos;
				leaf->size = sl.size*GetScale();
				leaf->color = Color4f(1,1,1,1);
				leaf->inode = sl.inode;
				leaf->texture = sl.pTexture;
			}
			leaves_->CalcBound();
			if(first){
				if(pStatic->leaves.size()>0)
				{
					leaves_->SetTexture(pStatic->leaves[0].pTexture);
				}
				//leaves_->CalcLeafColor();
				for(int i=0; i<StaticVisibilitySet::num_lod; i++)
				{
					leaves_->CalcLods(pStatic->visibilitySets_[0].visibilityGroups[0].visibleNodes,i);
				}
			}
		}
		
		//update lights
		first=lights.empty() && !pStatic->lights.empty();
		if(first)
		{
			lights.resize(pStatic->lights.size());
			for(int i=0;i<lights.size();i++)
			{
				StaticLight& sl=pStatic->lights[i];
				if(sl.pTexture)
				{
					sl.pTexture->AddRef();
					lights[i] = scene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_SPRITE,sl.pTexture);
				}else
				{
					cTexture* sperical=GetTexLibrary()->GetSpericalTexture();
					lights[i] = scene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_TERRAIN,sperical);
				}
				
				
				lights[i]->SetDiffuse(sl.color);
				lights[i]->SetRadius(sl.atten_end*GetScale());
			}
		}

		for(int i=0;i<lights.size();i++){
			StaticLight& sl=pStatic->lights[i];
			cNode3dx& node=nodes_[sl.inode];
			if(!sl.chains.empty()){
				StaticLightAnimation& chain=sl.chains[node.chainIndex];
				Color4f color;
				chain.color.InterpolateSlow(node.phase,(float*)&color);
				color.r=clamp(color.r,0.0f,1.0f);
				color.g=clamp(color.g,0.0f,1.0f);
				color.b=clamp(color.b,0.0f,1.0f);
				color.a=clamp(color.a,0.0f,1.0f);
				lights[i]->SetDiffuse(color);
			}

			MatXf xpos;
			node.pos.copy_right(xpos);
			if(getAttribute(ATTRUNKOBJ_ATTACHED))
				lights[i]->SetPosition(xpos);

			bool is_group_visible = false;
			vector<VisibilityGroup>::iterator iGroup;
			FOR_EACH(visibilityGroups_, iGroup)
				is_group_visible = is_group_visible || iGroup->visibilityGroup->visibleNodes[sl.inode];

			lights[i]->putAttribute(ATTRUNKOBJ_IGNORE, getAttribute(ATTR3DX_HIDE_LIGHTS|ATTRUNKOBJ_IGNORE) || !is_group_visible || unvisible_);
		}

		if(first){
			for(int i=0;i<lights.size();i++)
				lights[i]->Attach();
		}
	}
}

void cObject3dx::DrawLine(Camera* camera)
{
	Update();
	Color4c color(255,255,255);
	Color4c c0(255,0,0);
	Color4c c_invisible(0,0,255);
	int size=nodes_.size();
	for(int i=1;i<size;i++)
	{
		cNode3dx& node=nodes_[i];
		StaticNode& sn=pStatic->nodes[i];
		cNode3dx& parent=nodes_[sn.iparent];

		bool visible=GetVisibilityTrack(i);
		if(visible)
			gb_RenderDevice->DrawLine(parent.pos.trans(),node.pos.trans(),sn.iparent==0?c0:color);
		else
			gb_RenderDevice->DrawLine(parent.pos.trans(),node.pos.trans(),c_invisible);
	}
}

void cObject3dx::Draw(Camera* camera)
{
	start_timer_auto();

	if(debugShowSwitch.objects)
		return;

	if(!isSkinColorSet_ && IsHaveSkinColorMaterial()){
		VisError << "Для модели " << pStatic->fileName() << " не назначен Skin Color.\nБудет назначен Skin Color по умолчанию" << VERR_END;
		SetSkinColor(Color4c(255,255,255,255),0);
	}

	Update();

	if(camera->getAttribute(ATTRCAMERA_SHADOWMAP)){
		DrawShadowAndZbuffer(camera,false);
		return;
	}
	if(camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER)){
		if(Option_FloatZBufferType==2)
			DrawShadowAndZbuffer(camera,true);
		return;
	}

	sDataRenderMaterial material;
	material.lerp_texture=lerp_color;
	material.point_light=&point_light;

	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);
	gb_RenderDevice3D->SetTextureBase(4,0);

	//Из плюсов - выводит стабильное количество полигонов вне зависимости
	//от количества подобъектов. Из минусов - для больших объектов не слишком
	//эффективен. indexed 14 mtrtis, nonindexed 26 mtris - пиковые значения на FX 5950.
	bool is_shadow=camera->IsShadow();

	DWORD old_zfunc;
	DWORD old_zwriteble;
	DWORD old_color;

	bool draw_opacity=camera->GetCameraPass()==SCENENODE_OBJECTSORT;
	bool draw2passes = getAttribute(ATTRUNKOBJ_2PASS_ZBUFFER);//(camera->GetCameraPass()==SCENENODE_OBJECT_2PASS)||(camera->GetCameraPass()==SCENENODE_ZPASS);
	if((draw_opacity || draw2passes)){
		old_zfunc = gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
		old_zwriteble = gb_RenderDevice3D->GetRenderState(D3DRS_ZWRITEENABLE);
		old_color = gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
	}

	//!!! Не забыть сортировку по материалам.
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	int size=lod.bunches.size();
	for(int iBunch=0;iBunch<size;iBunch++){
		StaticBunch& bunch = lod.bunches[iBunch];
		if(!isVisibleMaterialGroup(bunch))
			continue;

		StaticMaterial& mat=pStatic->materials[bunch.imaterial];
		MaterialAnim& mat_anim=materials[bunch.imaterial];
		cTexture* diffuse_texture=material_textures[bunch.imaterial].diffuse_texture;
		float texture_phase=material_textures[bunch.imaterial].texture_phase;
		bool is_opacity_vg=material_textures[bunch.imaterial].is_opacity_vg;

		if(mat.no_light){
			material.Ambient.interpolate3(mat.ambient,ambient,ambient.a);

			material.Diffuse.r=0;
			material.Diffuse.g=0;
			material.Diffuse.b=0;

			material.Specular.set(0,0,0,0);
		}
		else{
			//material.Ambient.interpolate3(mat.ambient,ambient,ambient.a);
			//material.Diffuse.interpolate3(mat.diffuse,diffuse,diffuse.a);
			material.Ambient.interpolate3(Color4f::WHITE, ambient, ambient.a);
			material.Diffuse.interpolate3(Color4f::WHITE, diffuse, diffuse.a);
			material.Specular.interpolate3(mat.specular,specular,specular.a);
		}

		material.Ambient.a = material.Diffuse.a = mat_anim.opacity*object_opacity*distance_alpha;

		material.Tex[0]=diffuse_texture;

		gb_RenderDevice3D->SetSamplerData(0,(mat.tiling_diffuse&StaticMaterial::TILING_U_WRAP)?sampler_wrap_anisotropic:sampler_clamp_anisotropic);


		if(mat.is_reflect_sky)
			material.Tex[1] = scene()->GetSkyCubemap();
		else
			material.Tex[1] = mat.pReflectTexture ? mat.pReflectTexture : mat.pBumpTexture;

		if(!mat.no_light){
			material.Diffuse.mul3(material.Diffuse, scene()->GetSunDiffuse());
			material.Ambient.mul3(material.Ambient, scene()->GetSunAmbient());
			material.Specular.mul3(material.Specular, scene()->GetSunSpecular());
		}
		material.Specular.a=mat.specular_power;

		eBlendMode blend=ALPHA_NONE;
		if(material.Tex[0])
		{
			cTexture* pTex0=material.Tex[0];
			//
			if(pTex0->isAlphaTest())
			{
				blend=ALPHA_TEST;
			}
		}

		if(is_opacity_vg){
			//Рисовать прозрачную и непрозрачную часть. Выставлять.
			if(draw_opacity)
				blend=ALPHA_BLEND;
		}
		else{
			if(material.Diffuse.a<AlphaMaxiumBlend || mat.is_opacity_texture){
				blend=ALPHA_BLEND;
				if(!draw_opacity&&!draw2passes)
					continue;
			}
			else if(draw_opacity)
				continue;
		}

		switch(mat.transparencyType) {
			case StaticMaterial::TRANSPARENCY_ADDITIVE:
				blend = ALPHA_ADDBLENDALPHA; /// dst=dst+src*alpha
				break;
					
			case StaticMaterial::TRANSPARENCY_SUBSTRACTIVE:
				blend = ALPHA_SUBBLEND; /// dst=dst-src
				break;
		}

		gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
		gb_RenderDevice3D->SetTexturePhase(0,material.Tex[0],texture_phase);
		gb_RenderDevice3D->SetTexturePhase(1,material.Tex[1],texture_phase);

		VSSkin* vs=0;
		PSSkin* ps=0;

		if(scene()->GetTileMap()){
			gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(3, gb_RenderDevice3D->GetLightMapObjects());
		}
		else
			gb_RenderDevice3D->SetTextureBase(3,0);

		if(mat.pSecondOpacityTexture){
			vs=pShader3dx->vsSkinSecondOpacity;
			ps=pShader3dx->psSkinSecondOpacity;
			SetSecondUVTrans(true,vs,mat,mat_anim);

			gb_RenderDevice3D->SetTexture(1,mat.pSecondOpacityTexture);
		}
		else if(getAttribute(ATTRUNKOBJ_NOLIGHT)){
			vs=pShader3dx->vsSkinNoLight;
			ps=pShader3dx->psSkin;
			pShader3dx->psSkin->SelectLT(false,diffuse_texture);
		}
		else if(mat.pReflectTexture || mat.is_reflect_sky){
			int is_cube=false;
			if(material.Tex[1])
				is_cube=material.Tex[1]->getAttribute(TEXTURE_CUBEMAP)?1:0;
			if(is_shadow){
				vs=pShader3dx->vsSkinReflectionSceneShadow;
				ps=pShader3dx->psSkinReflectionSceneShadow;
			}
			else{
				vs=pShader3dx->vsSkinReflection;
				ps=pShader3dx->psSkinReflection;
			}

			Color4f amount;
			amount.r=mat.reflect_amount*diffuse.r;
			amount.g=mat.reflect_amount*diffuse.g;
			amount.b=mat.reflect_amount*diffuse.b;
			amount.a=0;

			ps->SetReflection(is_cube,amount);
			vs->SetReflection(is_cube);
		}
		else if(mat.pBumpTexture && Option_EnableBump){
			if(is_shadow){
				vs=pShader3dx->vsSkinBumpSceneShadow;
				ps=pShader3dx->psSkinBumpSceneShadow;
			}
			else{
				vs=pShader3dx->vsSkinBump;
				ps=pShader3dx->psSkinBump;
			}

			ps->SelectSpecularMap(mat.pSpecularmap,texture_phase);
		}
		else{
			if(is_shadow){
				vs=pShader3dx->vsSkinSceneShadow;
				if(diffuse_texture){
					ps=pShader3dx->psSkinSceneShadow;
				}
				else{
					ps=pShader3dx->psSkin;
					pShader3dx->psSkin->SelectLT(true,false);
				}
			}
			else{
				vs=pShader3dx->vsSkin;
				ps=pShader3dx->psSkin;
				pShader3dx->psSkin->SelectLT(true,diffuse_texture);
			}
		}

		SetUVTrans(vs,mat,mat_anim);

		bool reflectionz=camera->getAttribute(ATTRCAMERA_REFLECTION);
		vs->SetReflectionZ(reflectionz);
		ps->SetReflectionZ(reflectionz);

		//Немного не к месту, зато быстро по скорости, для отражений.
		gb_RenderDevice3D->SetTexture(5,camera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
		
		if(is_shadow)
			ps->SetShadowIntensity(scene()->GetShadowIntensity());

		ps->SetSelfIllumination(!mat.tex_self_illumination.empty() && !getAttribute(ATTR3DX_NO_SELFILLUMINATION));
		ps->SetMaterial(&material, mat.is_big_ambient);

		ps->Select();

		static MatXf world[StaticBunch::max_index];
		int world_num;
		GetWorldPoses(bunch, world, world_num);

		vs->Select(world, world_num, lod.blend_indices);
		vs->SetMaterial(&material);

		bool draw_2pass = draw_opacity;
		if(mat.is_opacity_texture)
			draw_2pass = false;

		if(!draw2passes){
			if(is_opacity_vg){
				if(draw_2pass){
					gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
					gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
					gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
				}

				DrawMaterialGroupSelectively(bunch,material.Diffuse,draw_opacity,vs,ps);

				if(draw_2pass){
					gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
					gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
					gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
					DrawMaterialGroupSelectively(bunch,material.Diffuse,draw_opacity,vs,ps);
				}
			}
			else{
				if(draw_2pass){
					gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
					gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
					gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
				}

				DrawMaterialGroup(bunch);

				if(draw_2pass){
					gb_RenderDevice3D->SetRenderState(RS_ZWRITEENABLE,FALSE);
					gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
					gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
					DrawMaterialGroup(bunch);
				}
			}
		}
		else
			DrawMaterialGroup(bunch);
	}

	if(draw_opacity || draw2passes){
		gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,old_zwriteble);
		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
	}

	if(pStatic->enableFur && camera->GetCameraPass()==SCENENODE_OBJECTSORT)
		DrawFur(camera);
}

void cObject3dx::SetUVTrans(VSSkinBase* vs,StaticMaterial& mat,MaterialAnim& mat_anim)
{
	//uv transform
	if(mat.chains.empty())
	{
		vs->SetUVTrans(0);
	}else
	{
		StaticMaterialAnimation& mat_chain=mat.chains[mat_anim.chain];
		if(mat_chain.uv.values.empty())
		{
			vs->SetUVTrans(0);
		}else
		{
			float uvmatrix[6];
			mat_chain.uv.InterpolateSlow(mat_anim.phase,uvmatrix);
			vs->SetUVTrans(uvmatrix);
		}
	}
}

void cObject3dx::SetSecondUVTrans(bool use,class VSSkinBase* vs,StaticMaterial& mat,MaterialAnim& mat_anim)
{
	if(use)
	{
		//uv transform
		if(!mat.chains.empty()) {
			StaticMaterialAnimation& mat_chain=mat.chains[mat_anim.chain];
			if(!mat_chain.uv_displacement.values.empty()){
				float uvmatrix_dp[6];
				mat_chain.uv_displacement.InterpolateSlow(mat_anim.phase,uvmatrix_dp);
				vs->SetSecondOpacityUVTrans(uvmatrix_dp,pStatic->isUV2?SECOND_UV_T1:SECOND_UV_T0);
			} else {
				vs->SetSecondOpacityUVTrans(0,pStatic->isUV2?SECOND_UV_T1:SECOND_UV_T0);
			}
		} else {
			vs->SetUVTrans(0);
		}
	}else
	{
		vs->SetSecondOpacityUVTrans(0,SECOND_UV_NONE);
	}
}

void cObject3dx::DrawMaterialGroup(StaticBunch& bunch)
{
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++){
		cTempVisibleGroup& vg = bunch.visibleGroups[ivg];
		if(isVisible(vg)){
			int num_polygon = vg.num_polygon;
			num_out_polygons += num_polygon;
			gb_RenderDevice3D->DrawIndexedPrimitive(lod.vb,bunch.offset_vertex,bunch.num_vertex,lod.ib,vg.begin_polygon + bunch.offset_polygon,num_polygon);
		}
	}
}

void cObject3dx::DrawMaterialGroupSelectively(StaticBunch& bunch,const Color4f& color,bool draw_opacity,VSSkin* vs,PSSkin* ps)
{
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++){
		cTempVisibleGroup& vg=bunch.visibleGroups[ivg];
		VisibilityGroup& group=visibilityGroups_[vg.visibilitySet];
		if(isVisible(vg)){
			if(draw_opacity){
				if(group.alpha>=AlphaMaxiumBlend)
					continue;
				Color4f c(color);
				c.a*=group.alpha;
				vs->SetAlphaColor(c);
				ps->SetAlphaColor(c);
			}
			else if(group.alpha<AlphaMaxiumBlend)
				continue;

			//БЛИН!!! Этот подход не дает истинной гибкости!

			gb_RenderDevice3D->DrawIndexedPrimitive(
				lod.vb,bunch.offset_vertex,bunch.num_vertex,
				lod.ib,vg.begin_polygon+bunch.offset_polygon,vg.num_polygon);

		}
	}
}

bool cObject3dx::isVisible(const cTempVisibleGroup& vg) const 
{ 
	if(!(vg.visibilities & visibilityGroups_[vg.visibilitySet].visibilityGroup->visibility))
		return false;
	if(vg.visibilityNodeIndex == -1)
		return true;
	return GetVisibilityTrack(vg.visibilityNodeIndex); 
}

bool cObject3dx::isVisibleMaterialGroup(StaticBunch& bunch) const
{
	for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++)
		if(isVisible(bunch.visibleGroups[ivg]))
			return true;

	return false;
}

void cObject3dx::DrawShadowAndZbuffer(Camera* camera,bool ZBuffer)
{
	gb_RenderDevice3D->SetTextureBase(1,0);

	//!!! Не забыть сортировку по материалам.
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	int size=lod.bunches.size();
	for(int iBunch=0;iBunch<size;iBunch++){
		StaticBunch& bunch = lod.bunches[iBunch];
		if(!isVisibleMaterialGroup(bunch))
			continue;

		StaticMaterial& mat=pStatic->materials[bunch.imaterial];

		if(mat.transparencyType!=StaticMaterial::TRANSPARENCY_FILTER)
			continue;

		MaterialAnim& mat_anim=materials[bunch.imaterial];
		cTexture* diffuse_texture=material_textures[bunch.imaterial].diffuse_texture;

		float alpha=mat_anim.opacity*object_opacity*distance_alpha;

		gb_RenderDevice3D->SetSamplerData(0,(mat.tiling_diffuse&StaticMaterial::TILING_U_WRAP)?sampler_wrap_anisotropic:sampler_clamp_anisotropic);

		eBlendMode blend=ALPHA_NONE;
		bool is_alphatest=false;
		if(diffuse_texture){
			cTexture* pTex0=diffuse_texture;
			if(pTex0->isAlphaTest() || pTex0->isAlpha()){
				blend=ALPHA_TEST;
				is_alphatest=true;
			}
		}

		if(gb_RenderDevice3D->dtAdvance->GetID()!=DT_GEFORCEFX)
			blend=ALPHA_NONE;
		if(alpha<AlphaMiniumShadow)
			continue;

		gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
		if(is_alphatest){
			float texture_phase=material_textures[bunch.imaterial].texture_phase;
			gb_RenderDevice3D->SetTexturePhase(0,diffuse_texture,texture_phase);
		}
		else
			gb_RenderDevice3D->SetTextureBase(0,0);

		VSSkinBase* vs=0;

		gb_RenderDevice3D->SetTextureBase(3,0);
		vs=ZBuffer?pShader3dx->vsSkinZBuffer:pShader3dx->vsSkinShadow;
		SetUVTrans(vs,mat,mat_anim);

		if(is_alphatest){
			if (ZBuffer){
				pShader3dx->psSkinZBufferAlpha->SetSecondOpacity(mat.pSecondOpacityTexture);
				pShader3dx->psSkinZBufferAlpha->Select();
			}
			else{
				pShader3dx->psSkinShadowAlpha->SetSecondOpacity(mat.pSecondOpacityTexture);
				pShader3dx->psSkinShadowAlpha->Select();
			}
		}
		else{
			if (ZBuffer)
				pShader3dx->psSkinZBuffer->Select();
			else
				pShader3dx->psSkinShadow->Select();
		}
		
		SetSecondUVTrans(mat.pSecondOpacityTexture?true:false,vs,mat,mat_anim);
		

		static MatXf world[StaticBunch::max_index];
		int world_num;
		GetWorldPoses(bunch,world,world_num);

		vs->Select(world,world_num,lod.blend_indices);

		DrawMaterialGroup(bunch);
	}
}

/*Scripts\Resource\balmer\furmap.dds
Хочется - 
 1 разного цвета									- это понятно текстурка.
 2 разной высоты шерсти в разных местах				- grayscale текстурка.
 3 направления отличного от направления нормали.	- непонятно, можно отдельной геометрией, а можно текстуркой.
 4 шевеления шерсти????? Хм, хм.					- нафиг надо, лучше про вторую нормаль подумать, да.

 //Так - есть 2 текстуры.
 Первая текстура - цвет и прозрачность для furmap.
 Вторая текстура RGB - нормаль в tangent space. A - высота шерсти относительная.
 Все эти параметры задаются в текстовом файлике имя_модели.furinfo
*/

void cObject3dx::DrawFur(Camera* camera)
{
	sDataRenderMaterial material;
	material.lerp_texture=lerp_color;
	material.point_light=&point_light;

	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);
	gb_RenderDevice3D->SetTextureBase(4,0);

	bool is_shadow=camera->IsShadow();

	//!!! Не забыть сортировку по материалам.
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	int size=lod.bunches.size();
	for(int iBunch=0;iBunch<size;iBunch++){
		StaticBunch& bunch = lod.bunches[iBunch];
		if(!isVisibleMaterialGroup(bunch))
			continue;

		StaticMaterial& mat=pStatic->materials[bunch.imaterial];
		if(mat.pFurmap==0)
			continue;

		MaterialAnim& mat_anim=materials[bunch.imaterial];
		cTexture* diffuse_texture=material_textures[bunch.imaterial].diffuse_texture;
		float texture_phase=material_textures[bunch.imaterial].texture_phase;
		bool is_opacity_vg=material_textures[bunch.imaterial].is_opacity_vg;

		if(mat.no_light){
			material.Ambient.interpolate3(mat.ambient,ambient,ambient.a);

			material.Diffuse.r=0;
			material.Diffuse.g=0;
			material.Diffuse.b=0;

			material.Specular.set(0,0,0,0);
		}
		else{
			material.Ambient.interpolate3(Color4f::WHITE,ambient,ambient.a);
			material.Diffuse.interpolate3(Color4f::WHITE,diffuse,diffuse.a);
			material.Specular.interpolate3(mat.specular,specular,specular.a);
		}

		material.Ambient.a=
		material.Diffuse.a=mat_anim.opacity*object_opacity*distance_alpha*mat.fur_alpha*furAlphaPhase_;

		material.Tex[0]=diffuse_texture;

		gb_RenderDevice3D->SetSamplerData(0,(mat.tiling_diffuse&StaticMaterial::TILING_U_WRAP)?sampler_wrap_anisotropic:sampler_clamp_anisotropic);


		if(mat.is_reflect_sky)
			material.Tex[1]=scene()->GetSkyCubemap();
		else
			material.Tex[1]=mat.pReflectTexture?mat.pReflectTexture:mat.pBumpTexture;

		material.Diffuse.mul3(material.Diffuse,scene()->GetSunDiffuse());
		material.Ambient.mul3(material.Ambient,scene()->GetSunAmbient());
		material.Specular.mul3(material.Specular,scene()->GetSunSpecular());
		material.Specular.a=mat.specular_power;

		gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_BLEND);

		gb_RenderDevice3D->SetTexturePhase(0,mat.pFurmap,texture_phase);
		gb_RenderDevice3D->SetTexturePhase(1,material.Tex[1],texture_phase);

		VSSkin* vs=0;
		PSSkin* ps=0;

		if(scene()->GetTileMap()){
			gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMapObjects());
		}
		else
			gb_RenderDevice3D->SetTextureBase(3,0);

		vs=pShader3dx->vsSkinFur;
		ps=pShader3dx->psSkin;
		pShader3dx->psSkin->SelectLT(true,diffuse_texture);

		SetUVTrans(vs,mat,mat_anim);

		bool reflectionz=camera->getAttribute(ATTRCAMERA_REFLECTION);
		vs->SetReflectionZ(reflectionz);
		ps->SetReflectionZ(reflectionz);

		//Немного не к месту, зато быстро по скорости, для отражений.
		gb_RenderDevice3D->SetTexture(5,camera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
		
		if(is_shadow)
			ps->SetShadowIntensity(scene()->GetShadowIntensity());

		ps->SetSelfIllumination(!mat.tex_self_illumination.empty() && !getAttribute(ATTR3DX_NO_SELFILLUMINATION));
		ps->SetMaterial(&material, mat.is_big_ambient);

		ps->Select();

		static MatXf world[StaticBunch::max_index];
		int world_num;
		GetWorldPoses(bunch, world, world_num);

		vs->Select(world, world_num, lod.blend_indices);
		vs->SetMaterial(&material);

/*
	Нужно ещё попробовать - 
	1. изменять прозрачность по разным законам.
	2. изменять цвет так же взависимости от слоя.
*/
		const max_ifur=10;
		float basea=material.Diffuse.a;
		for(int ifur=1;ifur<=max_ifur;ifur++){
			if(mat.fur_alpha_type==FurInfo::FUR_LINEAR){
				material.Ambient.a=
				material.Diffuse.a=(basea*(max_ifur+1-ifur))/max_ifur;
				ps->SetMaterial(&material, mat.is_big_ambient);
				vs->SetMaterial(&material);
			}

			pShader3dx->vsSkinFur->SetFurDistance(furScalePhase_*ifur*mat.fur_scale/max_ifur);
			DrawMaterialGroup(bunch);
		}
	}
}

int cObject3dx::GetAnimationGroup(const char* name)
{
	vector<AnimationGroup>& animation_group=pStatic->animationGroups_;
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
	return pStatic->animationGroups_.size();
}

const char* cObject3dx::GetAnimationGroupName(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		string str="Bad group index. Файл:";
		str+=pStatic->fileName();
		xxassert(0 ,str.c_str());
		return 0;
	}

	return pStatic->animationGroups_[igroup].name.c_str();
}

bool cObject3dx::SetVisibilityGroup(const char* name, bool silently,VisibilitySetIndex iset)
{
	xassert(iset >= 0 && iset < pStatic->visibilitySets_.size());
	StaticVisibilityGroups& vg=pStatic->visibilitySets_[iset].visibilityGroups;
	int vg_size=vg.size();
	for(int igroup=0;igroup<vg_size;igroup++){
		StaticVisibilityGroup& group = pStatic->visibilitySets_[0].visibilityGroups[igroup];
		if(group.name == name){
			SetVisibilityGroup(VisibilityGroupIndex(igroup));
			return true;
		}
	}

	xassert(silently);
	return false;
}

void cObject3dx::SetVisibilityGroup(VisibilityGroupIndex group, VisibilitySetIndex iset)
{
	if(iset==VisibilitySetIndex::BAD)
	{
		xassert(0);//Наверно не нужно такое поведение.
		int size=GetVisibilitySetNumber();
		for(int i=0;i<size;i++)
			SetVisibilityGroup(group,VisibilitySetIndex(i));
		return;
	}

	if(visibilityGroups_.empty())
		return;

	xassert(iset >= 0 && iset < visibilityGroups_.size());
	xassert(group >=0 && group < pStatic->visibilitySets_[iset].visibilityGroups.size());
	visibilityGroups_[iset].visibilityGroupIndex = group;
	UpdateVisibilityGroup(iset);
}

void cObject3dx::UpdateVisibilityGroup(VisibilitySetIndex iset)
{
	// !!! не нужна
	visibilityGroups_[iset].visibilityGroup = &pStatic->visibilitySets_[iset].visibilityGroups[visibilityGroups_[iset].visibilityGroupIndex];
}

void cObject3dx::UpdateVisibilityGroups()
{
	int size=visibilityGroups_.size();
	for(int iset=0;iset<size;iset++)
		UpdateVisibilityGroup(VisibilitySetIndex(iset));
}

void cObject3dx::CalcIsOpacity(	bool& is_opacity,bool& is_noopacity)
{
	is_opacity=false;
	is_noopacity=false;

	is_opacity = is_opacity || pStatic->enableFur;

	for(int imat=0;imat<materials.size();imat++)
	{
		MaterialAnim& m=materials[imat];
		StaticMaterial& sm=pStatic->materials[imat];
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

void cObject3dx::PreDraw(Camera* camera)
{
	num_out_polygons=0;
	ProcessEffect(camera);
	if(getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE)){
		float radius=GetBoundRadius();
		float dist=camera->xformZ(position.trans());

		if(dist>hideDistance){
			unvisible_ = true;
			return;
		}

		if(Option_HideSmoothly){
			float hideRange = hideDistance*0.15f;
			float fadeDist = hideDistance-hideRange;
			if(dist>fadeDist)
				distance_alpha = 1.0f-(dist-fadeDist)/hideRange;
			else
				distance_alpha = 1.0f;
		}
	}
	else
		distance_alpha = 1.0f;

	if(pStatic->circle_shadow_enable_min==OST_SHADOW_REAL)
	{
		Camera* pShadow=camera->FindChildCamera(ATTRCAMERA_SHADOWMAP);
		if(pShadow)
		{
			if(pShadow->TestVisible(position.trans(),pStatic->boundRadius*position.scale()))
				pShadow->AttachNoRecursive(SCENENODE_OBJECT,this);
		}
	}

	{
		Camera* pReflection=camera->FindChildCamera(ATTRCAMERA_REFLECTION);
		if(pReflection)
		{
			if(pReflection->TestVisible(position.trans(),pStatic->boundRadius*position.scale()))
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

	if(pStatic->is_lod && !getAttribute(ATTRUNKOBJ_NO_USELOD))
	{//select lod
		Vect3f camera_pos=camera->GetPos();
		Vect3f unit_pos=position.trans();
		float d2 = camera->GetPos().distance2(position.trans());
		iLOD = d2 < border_lod12_2 ? 0 : (d2 < border_lod23_2 ? 1 : 2);
		UpdateVisibilityGroups();
	}

	if(!camera->TestVisible(position.trans(),pStatic->boundRadius*position.scale()))
		return;

	unvisible_ = distance_alpha < 0.25f;

	Camera* pFloatZBuffer=camera->FindChildCamera(ATTRCAMERA_FLOAT_ZBUFFER);
	if (pFloatZBuffer)
	{
		bool is_opacity,is_noopacity;
		CalcIsOpacity(is_opacity,is_noopacity);
		if(is_noopacity)
			pFloatZBuffer->AttachNoRecursive(SCENENODE_OBJECT,this);
		if(is_opacity)
			pFloatZBuffer->AttachNoRecursive(SCENENODE_OBJECTSORT,this);
	}

	if (getAttribute(ATTRUNKOBJ_2PASS_ZBUFFER))
	{
		camera->AttachNoRecursive(SCENENODE_OBJECT_2PASS,this);
	}else
	{
		bool is_opacity,is_noopacity;
		CalcIsOpacity(is_opacity,is_noopacity);

		if(is_noopacity)
		{
			if(getAttribute(ATTR3DX_UNDERWATER))
				camera->AttachNoRecursive(SCENENODE_UNDERWATER,this);
			else
			if(getAttribute(ATTRUNKOBJ_SHOW_FLAT_SILHOUETTE))
				camera->AttachNoRecursive(SCENENODE_FLAT_SILHOUETTE,this);
			else
				camera->AttachNoRecursive(SCENENODE_OBJECT,this);
		}

		if(is_opacity)
		{
			camera->AttachNoRecursive(SCENENODE_OBJECTSORT,this);
		}
	}

	if(pStatic->circle_shadow_enable_min==OST_SHADOW_CIRCLE)
		AddCircleShadow();

	Update();
	//updated=false;
	//if(pAnimSecond)
	//	pAnimSecond->updated=false;
}

void cObject3dx::Animate(float dt)
{
	start_timer_auto();

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
			int len=ma.diffuse_texture->frameNumber()*ma.diffuse_texture->GetTimePerFrame();
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
	return pStatic->boundRadius*position.scale();
}

void cObject3dx::GetBoundBox(sBox6f& box_) const
{
	box_=pStatic->boundBox;
	box_.min*=position.scale();
	box_.max*=position.scale();
}

void cObject3dx::GetBoundBoxUnscaled(sBox6f& box_)
{
	box_=pStatic->boundBox;
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
		box.addPoint(pos);
		float r=pos.norm();
		radius=max(radius,r);
	}

	pStatic->boundRadius=radius;
	pStatic->boundBox=box;

	CalcBoundSphere();
}

void cObject3dx::GetWorldPoses(const StaticBunch& bunch,MatXf* world,int& world_num) const
{
	world_num = bunch.nodeIndices.size();
	for(int j=0;j<world_num;j++)
		GetWorldPos(bunch, world[j], j);
}

void cObject3dx::GetWorldPos(const StaticBunch& bunch,MatXf& world, int idx) const
{
	xassert(idx < bunch.nodeIndices.size());
	
	int inode = bunch.nodeIndices[idx];
	const cNode3dx& node = nodes_[inode];
	StaticNode& sn = pStatic->nodes[inode];
	MatXf xpos;
	node.pos.copy_right(xpos);
	world.mult(xpos,sn.inv_begin_pos);
}

void cObject3dx::GetEmitterMaterial(cObjMaterial& material)
{
	if(pStatic->materials.empty())
	{
		xassert(0 && "Zero materials?");
		return;
	}

	StaticMaterial& mt = pStatic->materials.front();
	material.Ambient = mt.ambient;
	material.Diffuse = mt.diffuse;
	material.Specular = mt.specular;
	material.Power = mt.specular_power;
}

const char* cObject3dx::GetFileName() const
{
	return pStatic->fileName();
}

void cObject3dx::SetSkinColor(Color4c skin_color_, const char* logo_name_)//emblem_name - хранит путь к эмблеме. 
{
	skin_color=skin_color_;

	LoadTexture(true,false,logo_name_);
	isSkinColorSet_ = true;
}

void cObject3dx::SetSilhouetteIndex(int index)
{
	silhouette_index = index;
}

void cObject3dx::LoadTexture(bool only_skinned,bool only_self_illumination, const char* logo_name_)
{
	int m_size=materials.size();
	xassert(m_size==pStatic->materials.size());
	bool enable_self_illum=getAttribute(ATTR3DX_NO_SELFILLUMINATION)?false:true;

	for(int i=0;i<m_size;i++)
	{
		StaticMaterial& mat=pStatic->materials[i];
		cTexture*& diffuse=material_textures[i].diffuse_texture;
		Color4c color=skin_color;
		
		if(only_self_illumination)
		{
			if(mat.tex_self_illumination.empty())
				continue;
		}else
		{
			if(!logo_name_ || !logo_name_[0])
				if(mat.is_skinned != only_skinned)
					continue;
		}

		sRectangle4f logo_position;
		float logo_angle = 0;
		const char* logo=0;
		if(pStatic->logos.GetLogoPosition(mat.tex_diffuse.c_str(),&logo_position,logo_angle))
		{
			logo=logo_name_;
		}

		RELEASE(diffuse);
		if(enable_self_illum)
			diffuse=GetTexLibrary()->GetElement3DColor(pStatic->fixTextureName(mat.tex_diffuse.c_str()).c_str(),
			mat.is_skinned ? pStatic->fixTextureName(mat.tex_skin.c_str()).c_str() : 0, mat.is_skinned? &skin_color : 0,
					pStatic->fixTextureName(mat.tex_self_illumination.c_str()).c_str(), pStatic->fixTextureName(logo).c_str(), logo_position, logo_angle);
		else
			diffuse=GetTexLibrary()->GetElement3DColor(pStatic->fixTextureName(mat.tex_diffuse.c_str()).c_str(),
					mat.is_skinned ? pStatic->fixTextureName(mat.tex_skin.c_str()).c_str() : 0, mat.is_skinned ? &skin_color : 0,
					0, pStatic->fixTextureName(logo).c_str(), logo_position, logo_angle);

		if(diffuse)
		{
			if(diffuse->getAttribute(TEXTURE_ALPHA_BLEND) && !mat.is_opacity_texture)
			{
				diffuse->clearAttribute(TEXTURE_ALPHA_BLEND);
				diffuse->setAttribute(TEXTURE_ALPHA_TEST);
			}

			bool reload=false;
			if (getAttribute(ATTR3DX_NO_RESIZE_TEXTURES))
			{
				reload=diffuse->getAttribute(TEXTURE_DISABLE_DETAIL_LEVEL)==0;
				diffuse->setAttribute(TEXTURE_DISABLE_DETAIL_LEVEL);
			}
			if (reload)
				GetTexLibrary()->ReLoadTexture(diffuse);

			if(mat.pSecondOpacityTexture)
			{
				if(mat.pSecondOpacityTexture->getAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST))
				{
					diffuse->setAttribute(TEXTURE_ALPHA_TEST);
				}
			}

		}
	}
}

void cObject3dx::EnableSelfIllumination(bool enable)
{
	putAttribute(ATTR3DX_NO_SELFILLUMINATION,!enable);
	LoadTexture(false,true);
}

void cObject3dx::DisableDetailLevel()
{
	setAttribute(ATTR3DX_NO_RESIZE_TEXTURES);
	LoadTexture(false,false,"");
}

void cObject3dx::DrawLogicBound()
{
	Update();
	sBox6f box=pStatic->boundBox;
	MatXf pos=GetPosition();
	pos.rot()*=position.scale();
	gb_RenderDevice->DrawBound(pos,box.min,box.max,true,Color4c::RED);

	for(int inode=0;inode<GetNodeNumber();inode++){
		pos=GetNodePositionMat(inode);
		if(IsLogicBound(inode)){
			box=GetLogicBoundScaledUntransformed(inode);
			gb_RenderDevice->DrawBound(pos,box.min,box.max,true,Color4c::GREEN);
		}
	}
}

void cObject3dx::DrawBound() const
{
	sBox6f box;
	GetBoundBox(box);
	gb_RenderDevice3D->DrawBound(GetPosition(), box.min, box.max, true, Color4c(150, 0, 0, 155));
}

void cObject3dx::drawBoundSpheres() const
{
	Static3dxBase::BoundSpheres::const_iterator i;
	FOR_EACH(pStatic->boundSpheres, i){
		const cStatic3dx::BoundSphere& sphere = *i;
		MatXf pos;
		pos.set(GetNodePosition(sphere.node_index));
		pos.trans() += (pos.rot()*sphere.position)*position.scale();
		float radius = position.scale()*sphere.radius;
		//Vect3f pworld = world_view*pos.trans();
		gb_RenderDevice3D->drawCircle(pos.trans(), sphere.radius*position.scale(), Color4c::RED);
	}
}

void DrawPointer(MatXf m,Color4c color,float len,int xyz);
static void DrawLogicNode(MatXf& pos,float logic_radius, bool all)
{
	Color4c color;
	float r=logic_radius*0.1f;
	if(all)
	{
		DrawPointer(pos,Color4c(255,0,0),r,0);
		DrawPointer(pos,Color4c(0,255,0),r,1);
		DrawPointer(pos,Color4c(0,0,255),r,2);
	}else
	{
		Color4c c(255,255,255);
		DrawPointer(pos,c,r,0);
		DrawPointer(pos,c,r,1);
		DrawPointer(pos,c,r,2);
	}
}

void cObject3dx::DrawLogic(Camera* camera,int selected)
{
	Update();

	Vect3f pos=camera->GetMatrix()*position.trans();
	float min_size=100000/pos.z;
	float radius=max(GetBoundRadius(),min_size);

	double interval=0.4e3;
	bool blink=fmodFast(xclock(),interval)<interval*0.5f;
	int size=nodes_.size();
	int i;
	for(i=0;i<size;i++)
	{
		cNode3dx& node=nodes_[i];
		StaticNode& sn=pStatic->nodes[i];
		
		MatXf xpos;
		node.pos.copy_right(xpos);
		if(i==selected)
			DrawLogicNode(xpos,radius,blink);
		else
			DrawLogicNode(xpos,radius,true);
	}

	gb_RenderDevice->FlushPrimitive3D();

	if(selected>=0 && selected<size){
		i=selected;
		cNode3dx& node=nodes_[i];
		StaticNode& sn=pStatic->nodes[i];
		
		MatXf xpos;
		node.pos.copy_right(xpos);
		Vect3f pv,pe;
		camera->ConvertorWorldToViewPort(&xpos.trans(),&pv,&pe);
		if(pe.z>0){
			gb_RenderDevice->OutText(round(pe.x),round(pe.y),sn.name.c_str(),Color4f(1,1,1));
			const cNode3dx& node=nodes_[i];

			if(!GetVisibilityTrack(i))
				gb_RenderDevice->OutText(round(pe.x),round(pe.y+18),"hide",Color4f(0.5f,0.5f,1));
		}
	}
}

#define ASSERT_NODEINDEX(nodeindex) if(!(nodeindex>=0 && nodeindex<nodes_.size())){string s="Bad nodeindex in: ";s+=pStatic->fileName();xxassert(0,s.c_str());}

void cObject3dx::SetUserTransform(int nodeindex,const Se3f& pos)
{
	updated_=false;
	Mats p;
	p.se()=pos;
	p.se().trans().scale(1.0f / nodes_[nodeindex].pos.scale());
	p.scale() = 1;
	ASSERT_NODEINDEX(nodeindex);
	GetUserTransformIndex(nodes_[nodeindex]).mat = p;
}

c3dxAdditionalTransformation& cObject3dx::GetUserTransformIndex(cNode3dx& s)
{
	if(s.IsAdditionalTransform())
	{
		BYTE add_index=s.additional_transform;
		xassert(add_index<additional_transformations.size());
		return additional_transformations[add_index];
	}

	int size=additional_transformations.size();
	xassert(size<254);
	for(int i=0;i<size;i++)
	if(!additional_transformations[i].used)
	{
		int found_index=i;
		s.additional_transform=found_index;
		additional_transformations[found_index].used = true;
		return additional_transformations[found_index];
	}

	s.additional_transform=size;
	c3dxAdditionalTransformation n;
	n.used = true;
	additional_transformations.push_back(n);
	return additional_transformations.back();
}

void cObject3dx::RestoreUserTransform(int nodeindex)
{
	ASSERT_NODEINDEX(nodeindex);
	cNode3dx& s=nodes_[nodeindex];
	if(!s.IsAdditionalTransform())
		return;

	updated_=false;
	xassert(s.additional_transform<additional_transformations.size());
	additional_transformations[s.additional_transform].used = false;
	s.additional_transform=255;
}

void cObject3dx::CopyUserTransformToSecondAnimation(int nodeindex)
{
	if(!pAnimSecond){
		RestoreUserTransform(nodeindex);
		return;
	}

	ASSERT_NODEINDEX(nodeindex);
	cNode3dx& s = nodes_[nodeindex];
	if(!s.IsAdditionalTransform())
		return;

	updated_ = false;
	xassert(s.additional_transform < additional_transformations.size());
	cNode3dx& s1 = pAnimSecond->nodes_[nodeindex];
	s1.additional_transform = s.additional_transform;
	s.additional_transform = 255;

	cNode3dx& s2 = pAnimSecond->nodes_.front();
	if(!s2.IsAdditionalTransform())
        GetUserTransformIndex(s2).mat = position;
}

void cObject3dx::RestoreSecondAnimationUserTransform()
{	
	if(!pAnimSecond || !pAnimSecond->nodes_.front().IsAdditionalTransform())
		return;

	vector<cNode3dx>::iterator it;
	FOR_EACH(pAnimSecond->nodes_, it){
		cNode3dx& s = *it;
		if(s.IsAdditionalTransform()){
			updated_ = false;
			additional_transformations[s.additional_transform].used = false;
			s.additional_transform = 255;
		}
	}
}

bool cObject3dx::HasUserTransform(int node_index) const
{
	const cNode3dx& s=nodes_[node_index];
	return s.IsAdditionalTransform();
}

void cObject3dx::ProcessEffect(Camera* camera)
{
	StaticVisibilityGroup* pGroup=GetVisibilityGroup();

	int effect_size=effects.size();
	for(int ieffect=0;ieffect<effect_size;ieffect++){
		EffectData& e=effects[ieffect];
		StaticEffect& se=pStatic->effects[ieffect];
		StaticNode& snode=pStatic->nodes[se.node];
		cNode3dx& node=nodes_[se.node];
		StaticNodeAnimation& chain=snode.chains[node.chainIndex];

		xassert(!chain.visibility.values.empty());

		if(e.index_visibility==255){
			e.index_visibility=chain.visibility.FindIndex(node.phase);
			e.prev_phase=node.phase;
		}

		bool is_group_visible=pGroup->visibleNodes[se.node];
		if(se.is_cycled){
			e.index_visibility=chain.visibility.FindIndexRelative(node.phase,e.index_visibility);
			bool visible=false;
			if(is_group_visible)
				chain.visibility.Interpolate(node.phase,&visible,e.index_visibility);
			if(visible && !e.pEffect){
				EffectKey* key=gb_EffectLibrary->Get((gb_VisGeneric->GetEffectPath()+se.file_name).c_str(),position.scale(),gb_VisGeneric->GetEffectTexturePath());
				e.pEffect=camera->scene()->CreateEffectDetached(*key,this);//,position.s);
				e.pEffect->LinkToNode(this,se.node);
				e.pEffect->Attach();
			}

			if(e.pEffect)
				e.pEffect->SetParticleRate(visible?1.0f:0);
		}
		else if(is_group_visible){
			///Ищем 
			float delta_plus,delta_minus;
			if(node.phase>e.prev_phase){
				delta_plus=node.phase-e.prev_phase;
				delta_minus=1-delta_plus;
			}
			else{
				delta_minus=e.prev_phase-node.phase;
				delta_plus=1-delta_minus;
			}

			bool is_up=false;
			Interpolator3dxBool::Values& data=chain.visibility.values;
			int dsize=data.size();
			if(delta_plus<delta_minus){
				int cur=e.index_visibility;
				bool prev_visibly=bool(data[cur].value);
				
				do{
					const SplineDataBool& p=data[cur];

					if(p.value && !prev_visibly)
						is_up=true;
					
					float t=(node.phase-p.tbegin)*p.inv_tsize;
					if(t>=0 && t<=1)
						break;

					prev_visibly=bool(p.value);

					cur++;
					if(cur>=dsize)
						cur=0;
				} while(cur!=e.index_visibility);

				e.index_visibility=cur;
				
			}
			else {
				int cur=e.index_visibility;
				bool prev_visibly=data[cur].value;
				
				do {
					const SplineDataBool& p=data[cur];

					if(p.value && !prev_visibly)
						is_up=true;
					
					float t=(node.phase-p.tbegin)*p.inv_tsize;
					if(t>=0 && t<=1)
						break;

					prev_visibly=p.value;

					cur--;
					if(cur<0)
						cur=dsize-1;
				} while(cur!=e.index_visibility);

				e.index_visibility=cur;
			}

			if(is_up){
				xassert(0);
				EffectKey* key=gb_EffectLibrary->Get((gb_VisGeneric->GetEffectPath()+se.file_name).c_str(),position.scale(),gb_VisGeneric->GetEffectTexturePath());
				cEffect* eff=camera->scene()->CreateEffectDetached(*key,this,true);
				eff->LinkToNode(this,se.node);
				eff->Attach();
			}
		}

		e.prev_phase=node.phase;
	}
}

void cObject3dx::SetColorOld(const Color4f *ambient_,const Color4f *diffuse_,const Color4f *specular_)
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

void cObject3dx::GetColorOld(Color4f *ambient_,Color4f *diffuse_,Color4f *specular_) const
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

void cObject3dx::SetColorMaterial(const Color4f *ambient_,const Color4f *diffuse_,const Color4f *specular_)
{
	if(ambient_)
		ambient=*ambient_;
	if(diffuse_)
		diffuse=*diffuse_;
	if(specular_)
		specular=*specular_;
}

void cObject3dx::GetColorMaterial(Color4f *ambient_,Color4f *diffuse_,Color4f *specular_) const
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

int cObject3dx::GetNumPolygons()
{
	pStatic->CacheBuffersToSystemMem(iLOD);
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];

	int numPolyugons=0;
	cSkinVertexSysMemI skin_vertex;
	sPolygon* pPolygon = lod.sys_ib;
	skin_vertex.SetVB(lod.sys_vb);
	for(int isg=0;isg<lod.bunches.size();isg++)
	{
		StaticBunch& bunch = lod.bunches[isg];
		if(!isVisibleMaterialGroup(bunch))
			continue;

		MaterialAnim& mat_anim=materials[bunch.imaterial];
		float alpha=mat_anim.opacity*object_opacity*distance_alpha;
		if(alpha<BLEND_STATE_ALPHA_REF/255.0f)
			continue;

		for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++)
		{
			cTempVisibleGroup& vg=bunch.visibleGroups[ivg];
			if(isVisible(vg))
				numPolyugons += vg.num_polygon;
		}
	}

	return numPolyugons;
}

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

int cObject3dx::GetMaterialNum()
{
	return pStatic->materials.size();
}

int cObject3dx::GetNodeNum()
{
	return nodes_.size();
}

void cObject3dx::SetShadowType(ObjectShadowType type)
{
	pStatic->circle_shadow_enable=type;
	pStatic->circle_shadow_enable_min=min(pStatic->circle_shadow_enable,gb_VisGeneric->GetMaximalShadowObject());
}

void cObject3dx::SetCircleShadowParam(float radius,float height)
{
	pStatic->circle_shadow_radius=radius;
	pStatic->circle_shadow_height=round(height);
}

ObjectShadowType cObject3dx::getShadowType()
{
	return pStatic->circle_shadow_enable;
}

void cObject3dx::getCircleShadowParam(float& radius, float& height)
{
	radius = pStatic->circle_shadow_radius;
	height = pStatic->circle_shadow_height;
}

void cObject3dx::AddCircleShadow()
{
	cTileMap* tilemap=scene()->GetTileMap();
	if(!tilemap)
		return;
	float c_radius=pStatic->circle_shadow_radius*position.scale();
	if(pStatic->circle_shadow_height<0)
	{
		scene()->AddCircleShadow(GetPosition().trans(),c_radius,scene()->GetCircleShadowIntensity());
	}else
	{
		const Vect3f& pos=GetPosition().trans();
		int z = vMap.getZ(round(pos.x),round(pos.y));
		Color4c mul=scene()->GetCircleShadowIntensity();
		int dz=z+pStatic->circle_shadow_height-round(pos.z);
		const int delta=20;
		if(dz>0)
		{
			Color4c intensity=mul;
			if(dz<delta)
			{
				intensity.r=(dz*mul.r)/delta;
				intensity.g=(dz*mul.g)/delta;
				intensity.b=(dz*mul.b)/delta;
			}

			scene()->AddCircleShadow(pos,c_radius,intensity);
		}
	}
}

bool cObject3dx::GetVisibilityTrack(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const StaticNode& snode=pStatic->nodes[nodeindex];
	const cNode3dx& node=nodes_[nodeindex];
	const StaticNodeAnimation& chain=snode.chains[node.chainIndex];
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
	const StaticNode& snode=pStatic->nodes[nodeindex];
	const cNode3dx& node=nodes_[nodeindex];
	const StaticNodeAnimation& chain=snode.chains[node.chainIndex];
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
	vx=m.xcol();vx.normalize();
	vy=m.ycol();vy.normalize();
	vz=m.zcol();vz.normalize();

	m.setXcol(vx);
	m.setYcol(vy);
	m.setZcol(vz);
}

void DrawPointer(MatXf m,Color4c color,float len,int xyz)
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
		xassert(0);
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

void cObject3dx::SetNodePosition(int nodeindex,const Se3f& pos)
{
	ASSERT_NODEINDEX(nodeindex);
	xassert(getAttribute(ATTR3DX_NOUPDATEMATRIX));
	cNode3dx& s=nodes_[nodeindex];
	s.pos = Mats(pos, 1);
}

void cObject3dx::SetNodePositionMats(int nodeindex,const Mats& pos)
{
	ASSERT_NODEINDEX(nodeindex);
	xassert(getAttribute(ATTR3DX_NOUPDATEMATRIX));
	cNode3dx& s=nodes_[nodeindex];
	s.pos=pos;
}


cObject3dxAnimation::cObject3dxAnimation(cStatic3dx* pStatic_)
:pStatic(pStatic_),updated_(false)
{
	nodes_.resize(pStatic->nodes.size());
	if(nodes_.empty()){
		string str="Empty object:";
		str+=pStatic->fileName();
		xassertStr(0,str.c_str());
	}

	materials.resize(pStatic->materials.size());
	for(int i=0;i<materials.size();i++){
		MaterialAnim& m=materials[i];
		m.chain=0;
		m.phase=0;
		m.opacity=pStatic->materials[i].opacity;
	}
	treeUpdated_ = false;
}

cObject3dxAnimation::cObject3dxAnimation(cObject3dxAnimation* pObj)
:updated_(false)
{
	pStatic = pObj->pStatic;
	nodes_ = pObj->nodes_;
	materials = pObj->materials;
	treeUpdated_ = false;
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
	for(int igroup=0;igroup<pStatic->animationGroups_.size();igroup++)
		SetAnimationGroupPhase(igroup,phase);
/**/
}

void cObject3dxAnimation::CalcOpacity(int imaterial)
{
	MaterialAnim& m=materials[imaterial];
	StaticMaterial& mat=pStatic->materials[imaterial];
	float material_phase=materials[imaterial].phase;
	int material_chain=materials[imaterial].chain;
	if(material_chain>=0 && !mat.chains.empty())
	{
		StaticMaterialAnimation& mat_chain=mat.chains[material_chain];
		mat_chain.opacity.InterpolateSlow(material_phase,&m.opacity);
	}else
	{
		m.opacity=mat.opacity;
	}
}

void cObject3dxAnimation::SetAnimationGroupPhase(int igroup,float phase)
{
	updated_=false;
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		xassert(0 && "Bad group index");
		return;
	}
	AnimationGroup& ag=pStatic->animationGroups_[igroup];

	int size=ag.nodes.size();
	for(int iag=0;iag<size;iag++)
	{
		int inode=ag.nodes[iag];
		cNode3dx& node=nodes_[inode];
		node.phase=phase;
	}

	for(int imat=0;imat<materials.size();imat++)
	{
		StaticMaterial& mat = pStatic->materials[imat];
		if(pStatic->materials[imat].animation_group_index==igroup)
		{
			materials[imat].phase=phase;
			CalcOpacity(imat);
		}
	}
}

float cObject3dxAnimation::GetAnimationGroupPhase(int igroup)
{
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animationGroups_[igroup];

	if(!ag.nodes.empty())
	{
		int inode=ag.nodes[0];
		cNode3dx& node=nodes_[inode];
		return node.phase;
	}

	return 0;
}

void cObject3dxAnimation::SetAnimationGroupChain(int igroup,const char* chain_name)
{
	updated_=false;
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
	updated_=false;
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		string str="Неправильный номер группы анимации. Файл:";
		str+=pStatic->fileName();
		xxassert(0 ,str.c_str());
		return;
	}

	if(chain_index<0 || chain_index>=pStatic->animationChains_.size())
	{
		string str="Неправильный номер анимационной цепочки. Файл:";
		str+=pStatic->fileName();
		xxassert(0 ,str.c_str());
		return;
	}

	AnimationGroup& ag=pStatic->animationGroups_[igroup];

	int size=ag.nodes.size();
	for(int iag=0;iag<size;iag++)
	{
		int inode=ag.nodes[iag];
		cNode3dx& node=nodes_[inode];
		node.chainIndex=chain_index;
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
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animationGroups_[igroup];
	int size=ag.nodes.size();
	if(size==0)
		return 0;
	int inode=ag.nodes[0];
	cNode3dx& node=nodes_[inode];
	return node.chainIndex;
}

int cObject3dxAnimation::FindChain(const char* chain_name)
{
	int size=pStatic->animationChains_.size();
	for(int i=0;i<size;i++)
	{
		StaticAnimationChain& ac=pStatic->animationChains_[i];
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
	return pStatic->animationChains_.size();
}

StaticAnimationChain* cObject3dxAnimation::GetChain(int i)
{
	xassert(i>=0 && i<pStatic->animationChains_.size());
	return &pStatic->animationChains_[i];
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
	updated_=false;
	if(chain<0 || chain>=pStatic->animationChains_.size())
	{
		string str="Неправильный номер анимационной цепочки. Файл:";
		str+=pStatic->fileName();
		xxassert(0 ,str.c_str());
	}

	vector<cNode3dx>::iterator it;
	FOR_EACH(nodes_,it)
	{
		cNode3dx& node=*it;
		node.chainIndex=chain;
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
		const StaticNode& node=pStatic->nodes[i];
		if(node.name==node_name)
			return i;
	}

	return -1;
}

const Se3f& cObject3dxAnimation::GetNodePosition(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cNode3dx& s=nodes_[nodeindex];
	return s.pos.se();
}

const Mats& cObject3dxAnimation::GetNodePositionMats(int nodeindex) const
{
	ASSERT_NODEINDEX(nodeindex);
	const cNode3dx& s=nodes_[nodeindex];
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
	xassert(node_index>=0 && node_index<nodes_.size());
	return pStatic->nodes[node_index].iparent;
}

bool cObject3dxAnimation::CheckDependenceOfNodes(int dependent_node_index, int node_index) const
{
	xassert(node_index>=0 && node_index<nodes_.size());
	xassert(dependent_node_index>=0 && dependent_node_index<nodes_.size());
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

void cNode3dx::calculatePos(StaticNodeAnimations& animations, Mats& pos)
{
	xassert(chainIndex < animations.size());

	StaticNodeAnimation& ani = animations[chainIndex];
	float xyzs[4];
	index_scale=ani.scale.FindIndexRelative(phase,index_scale);
	index_position=ani.position.FindIndexRelative(phase,index_position);
	index_rotation=ani.rotation.FindIndexRelative(phase,index_rotation);

	ani.scale.Interpolate(phase,&pos.scale(),index_scale);
	ani.position.Interpolate(phase,(float*)&pos.trans(),index_position);
	ani.rotation.Interpolate(phase,xyzs,index_rotation);
	pos.rot().set(xyzs[3],-xyzs[0],-xyzs[1],-xyzs[2]);
}

void cObject3dxAnimation::UpdateMatrix(Mats& position,vector<c3dxAdditionalTransformation>& additional_transformations)
{
	if(updated_)
		return;
	updated_=true;
	xassert(!nodes_.empty());
	nodes_[0].pos=position;

	int size=nodes_.size();
	for(int i=1;i<size;i++){
		cNode3dx& node = nodes_[i];
		StaticNode& staticNode = pStatic->nodes[i];

		if(staticNode.iparent < 0)//временно
			continue;

		xassert(staticNode.iparent>=0 && staticNode.iparent<size);
		cNode3dx& parent=nodes_[staticNode.iparent];

		if(node.chainIndex >= staticNode.chains.size())
			continue;

		Mats pos;
		node.calculatePos(staticNode.chains, pos);

		if(node.IsAdditionalTransform()){
			c3dxAdditionalTransformation& t=additional_transformations[node.additional_transform];
			Mats tmp=pos;
			pos.mult(tmp,t.mat);
			node.pos.mult(parent.pos,pos);
		}
		else{
			node.pos.mult(parent.pos,pos);
		}
	}
}

void cObject3dx::UpdateAndLerp()
{
	if(updated_)
		return;
	updated_=true;
	xassert(!nodes_.empty());
	nodes_.front().pos=position;
	cNode3dx& node = pAnimSecond->nodes_.front();
	if(node.IsAdditionalTransform()){
		c3dxAdditionalTransformation& t=additional_transformations[node.additional_transform];
		node.pos = t.mat;
	}else
		node.pos = position;

	int size=nodes_.size();
	for(int i=1;i<size;i++){
		cNode3dx& node=nodes_[i];
		cNode3dx& node1=pAnimSecond->nodes_[i];
		StaticNode& sn=pStatic->nodes[i];

		if(sn.iparent<0)//временно
			continue;

		xassert(sn.iparent>=0 && sn.iparent<size);
		cNode3dx& parent=nodes_[sn.iparent];

		if(node.chainIndex >= sn.chains.size())
			continue;
		if(node1.chainIndex >= sn.chains.size())
			continue;

		Mats pos,pos1;
		node.calculatePos(sn.chains, pos);
		node1.calculatePos(sn.chains, pos1);

		if(node1.IsAdditionalTransform()){
			c3dxAdditionalTransformation& t=additional_transformations[node1.additional_transform];
			Mats tmp=pos1;
			pos1.mult(tmp,t.mat);
		}

		float k=pAnimSecond->interpolation[i];

		Mats lerp_pos;
		lerp_pos.interpolate(pos, pos1, k);

		if(node.IsAdditionalTransform()){
			c3dxAdditionalTransformation& t=additional_transformations[node.additional_transform];
			Mats tmp=lerp_pos;
			lerp_pos.mult(tmp,t.mat);
		}
		
		node.pos.mult(parent.pos,lerp_pos);
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
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		xassert(0 && "Bad group index");
		return 0;
	}
	AnimationGroup& ag=pStatic->animationGroups_[igroup];

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
	if(!(igroup>=0 && igroup<pStatic->animationGroups_.size()))
	{
		xassert(0 && "Bad group index");
		return;
	}
	AnimationGroup& ag=pStatic->animationGroups_[igroup];
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

	parent_object=0;
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
	StaticBunch* s;
};

void cObject3dx::GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm)
{
	cStatic3dx::StaticLod& lod=pStatic->lods[iLOD];
	if (pos.size() != norm.size())
		norm.resize(pos.size());

	int size=lod.bunches.size();
	int num_polygons = 0;

	vector<VertexRange> sti;
	for(int i=0;i<size;i++){
		StaticBunch& bunch = lod.bunches[i];
		for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++){
			cTempVisibleGroup& vg=bunch.visibleGroups[ivg];
			if(isVisible(vg))
			{
				VertexRange rng;
				rng.vg = &vg;
				rng.s = &bunch;
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
	cSkinVertex skin_vertex=pStatic->GetSkinVertex(blend_weight);
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
	sPolygon* pPolygon = gb_RenderDevice->LockIndexBuffer(lod.ib);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());
	float int2float=1/255.0f;
	MatXf world[StaticBunch::max_index];
//	int world_num;

	for (int i=0; i<pos.size(); i++)
	{
		int ply = graphRnd()%num_polygons;
		int v = graphRnd()%3;

		cTempVisibleGroup* vg = 0;
		StaticBunch* s = 0;
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
	cStatic3dx::StaticLod& lod=pStatic->lods[0];
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
	int size=lod.bunches.size();
	int i;
	for(i=0;i<size;i++)
	{
		StaticBunch& s=lod.bunches[i];
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
				xassert(!getAttribute(ATTR3DX_NOUPDATEMATRIX));
				SetScale(1);
			}
			if(zero_pos)
			{
				xassert(!getAttribute(ATTR3DX_NOUPDATEMATRIX));
				SetPosition(Se3f::ID);
			}
		}
		Update();

		int pointindex=0;
		cSkinVertex skin_vertex=pStatic->GetSkinVertex(lod.GetBlendWeight());
		void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
		skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

		for(i=0;i<size;i++)
		{
			StaticBunch& bunch = lod.bunches[i];

			MatXf world[StaticBunch::max_index];
			int world_num;
			GetWorldPoses(bunch,world,world_num);

			int max_vertex=bunch.offset_vertex+bunch.num_vertex;
			int blend_weight=lod.GetBlendWeight();
			float int2float=1/255.0f;
			for(int vertex=bunch.offset_vertex;vertex<max_vertex;vertex++)
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
					int node=bunch.nodeIndices[idx];
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
							int node=bunch.nodeIndices[idx];
							found_selected=found_selected || (node==selected_node);
						}
					}
				}

				if(tif_flags&TIF_POSITIONS)
				all.positions[pointindex]=global_pos;
				if(tif_flags&TIF_NORMALS)
				{
					global_norm.normalize();
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

		if(zero_pos || one_scale){
			position=old_pos;
			Update();
		}
	}

	if(tif_flags&(TIF_TRIANGLES|TIF_VISIBLE_POINTS)){
		int size=lod.bunches.size();
		int num_triangles=0;
		for(i=0;i<size;i++){
			StaticBunch& bunch=lod.bunches[i];
			bool is_visible=false;
			for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++){
				cTempVisibleGroup& vg=bunch.visibleGroups[ivg];
				if(isVisible(vg))
					num_triangles+=vg.num_polygon;
			}
		}

		if(tif_flags&TIF_TRIANGLES)
			all.triangles.resize(num_triangles);
		int offset_triangles=0;
		sPolygon *IndexPolygon=gb_RenderDevice->LockIndexBuffer(lod.ib);
		for(int igroup=0;igroup<size;igroup++)
		{
			StaticBunch& bunch=lod.bunches[igroup];
			bool is_visible=false;
			for(int ivg=0;ivg<bunch.visibleGroups.size();ivg++)
			{
				cTempVisibleGroup& vg=bunch.visibleGroups[ivg];
				if(!(isVisible(vg)))
					continue;

				int offset_buf=vg.begin_polygon+bunch.offset_polygon;

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
		gb_RenderDevice->UnlockIndexBuffer(lod.ib);
	}
}

const char* cObject3dx::GetVisibilitySetName(VisibilitySetIndex iset)
{
	xassert(iset >= 0 && iset < pStatic->visibilitySets_.size());
	return pStatic->visibilitySets_[iset].name.c_str();
}

VisibilitySetIndex cObject3dx::GetVisibilitySetIndex(const char* set_name)
{
	for(int i=0;i<GetVisibilitySetNumber();i++)
		if(strcmp(GetVisibilitySetName(VisibilitySetIndex(i)),set_name)==0)
			return VisibilitySetIndex(i);
	return VisibilitySetIndex::BAD;
}

StaticVisibilitySet& cObject3dx::GetVisibilitySet(VisibilitySetIndex iset)
{
	xassert(iset >=0 && iset < pStatic->visibilitySets_.size());
	return pStatic->visibilitySets_[iset];
}

void cObject3dx::SetLod(int ilod)
{
	setAttribute(ATTRUNKOBJ_NO_USELOD);
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

void cObject3dx::DrawAll(Camera* camera)
{
	bool is_opacity,is_noopacity;
	CalcIsOpacity(is_opacity,is_noopacity);
	SceneNode old_pass=camera->GetCameraPass();
	if(is_noopacity)
	{
		camera->SetCameraPass(SCENENODE_OBJECT);
		Draw(camera);
	}

	if(is_opacity)
	{
		camera->SetCameraPass(SCENENODE_OBJECTSORT);
		DWORD old_cullmode=gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);
		gb_RenderDevice3D->SetRenderState( RS_ZWRITEENABLE, FALSE );
		Draw(camera);
		gb_RenderDevice3D->SetRenderState( RS_ZWRITEENABLE, TRUE );
		gb_RenderDevice3D->SetRenderState( D3DRS_CULLMODE, old_cullmode );
	}
	camera->SetCameraPass(old_pass);
}

void cObject3dx::AddLight(cUnkLight* light)
{
	if(getAttribute(ATTRUNKOBJ_IGNORE))
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
	ASSERT_NODEINDEX(nodeindex);

	cNode3dx& node=nodes_[nodeindex];
	StaticNode& staticNode = pStatic->nodes[nodeindex];
	parent_node = staticNode.iparent;

	if(staticNode.iparent<0){
		offset = Mats::ID;
		return;
	}

	node.calculatePos(staticNode.chains, offset);
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
			if (mtex.diffuse_texture->name() == texture_names[j])
			{
				found = true;
				break;
			}
		}
		if(!found)
			texture_names.push_back(mtex.diffuse_texture->name());
	}
}


void cObject3dx::SetTextureLerpColor(const Color4f& lerp_color_)
{
	lerp_color=lerp_color_;
}

Color4f cObject3dx::GetTextureLerpColor() const
{
	return lerp_color;
}

void cObject3dx::CalcBoundSphere()
{
	SetPosition(MatXf::ID);
	Update();
	pStatic->boundSpheres.clear();
	for(int inode=0;inode<nodes_.size();inode++)
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
			box.addPoint(all.positions[ipos]);
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
			pStatic->boundSpheres.push_back(b);
		}
	}
}

sBox6f cObject3dx::CalcDynamicBoundBox(const MatXf& world_view)
{
	sBox6f box;

	for(int i=0;i<pStatic->boundSpheres.size();i++)
	{
		cStatic3dx::BoundSphere& b=pStatic->boundSpheres[i];

		MatXf pos;
		pos.set(GetNodePosition(b.node_index));
		pos.trans()+=(pos.rot()*b.position)*position.scale();
		float radius=position.scale()*b.radius;
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
	if(pStatic->localLogicBounds.empty())
		return sBox6f();
	Update();
	xassert(nodeindex>=0 && nodeindex<pStatic->localLogicBounds.size());
	sBox6f box=pStatic->localLogicBounds[nodeindex];
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

void cObject3dx::SetFurScalePhase(float phase)
{
	furScalePhase_ = phase;
}

void cObject3dx::SetFurAlphaPhase(float phase)
{
	furAlphaPhase_ = phase;
}
