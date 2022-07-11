#include "StdAfxRD.h"
#include "Simply3dx.h"
#include "Static3dx.h"
#include "Scene.h"
#include "Render\Shader\Shaders.h"
#include "D3DRender.h"
#include "cCamera.h"
#include "VisGeneric.h"
#include "Terra\vmap.h"

/*

cSimply3dx - предназначен для мелких объектов, которых может быть много на мире.
А именно - кусты, деревья, мусор.

Ограничения cSimply3dx:
Нет анимации.
Выставляется только прозрачность объекта.
Нет дерева нод, все ноды второго уровня должны быть привязанны к group_center.
Нельзя привязывать объекты/спецэффекты.
*/

cSimply3dx::cSimply3dx(cStaticSimply3dx* pStatic_)
:c3dx(KIND_SIMPLY3DX),pStatic(pStatic_)
{
	position.se()=Se3f::ID;
	position.scale()=1;
	positionX_ = MatXf::ID;
	opacity=1;
	distance_alpha=1;
	hideDistance = 500;
	node_position.resize(pStatic->node_offset.size());
	for(int i=0;i<node_position.size();i++)
	{
		node_position[i]=MatXf::ID;
	}
	
	iLOD=0;
	xassert(pStatic->node_offset.size()<=32);
	user_node_positon=0;

	putAttribute(ATTRSIMPLY3DX_OPACITY,pStatic->is_opacity_texture);
	setAttribute(ATTRCAMERA_FLOAT_ZBUFFER);
	CalcOpacityFlag();
}

cSimply3dx::~cSimply3dx()
{
	RELEASE(pStatic);
}

void cSimply3dx::CalcOpacityFlag()
{
	putAttribute(ATTRSIMPLY3DX_OPACITY,pStatic->is_opacity_texture || pStatic->diffuse.a*opacity*distance_alpha<AlphaMaxiumBlend);
}

void cSimply3dx::SetOpacity(float opacity_)
{
	MTAccess();
	opacity=opacity_;
	CalcOpacityFlag();
}

void cSimply3dx::SetPosition(const MatXf& mat)
{
	MTAccess();
#ifdef _DEBUG
	float s=mat.rot().xrow().norm();
	xassert(fabsf(s-1)<1e-2f);
#endif

	position.se().set(mat);
	Update();
}

void cSimply3dx::SetPosition(const Se3f& pos)
{
	MTAccess();
#ifdef _DEBUG
	xassert(fabsf(pos.rot().norm2()-1) < 5e-2f);
#endif

	position.se() = pos;
	Update();
}

void cSimply3dx::Update()
{
	DWORD node_mask=1;
	position.copy_right(positionX_);
	for(int i=0;i<node_position.size();i++)
	{
		if(!(user_node_positon&node_mask))
			node_position[i].mult(positionX_, pStatic->node_offset[i]);
		node_mask=node_mask<<1;
	}
}

__forceinline float cSimply3dx::GetBoundRadiusInline() const
{
	return pStatic->radius*position.scale();
}

float cSimply3dx::GetBoundRadius() const
{
	return GetBoundRadiusInline();
}

void cSimply3dx::GetBoundBox(sBox6f& box_) const
{
	box_=pStatic->bound_box;
	box_.min*=position.scale();
	box_.max*=position.scale();
}

void cSimply3dx::GetBoundBoxUnscaled(sBox6f& box_)
{
	box_=pStatic->bound_box;
}

void cSimply3dx::SetScale(float scale_)
{
	position.scale()=scale_;
	Update();
}

float cSimply3dx::GetScale()const
{
	return position.scale();
}


bool cSimply3dx::IsDraw2Pass()
{
	return getAttribute(ATTRSIMPLY3DX_OPACITY) && !pStatic->is_opacity_texture;
}

void cSimply3dx::SelectMaterial(Camera* camera)
{
	VSSkin* vs=0;
	PSSkin* ps=0;
	sDataRenderMaterial material;
	material.lerp_texture.set(0,0,0,0);
	material.Ambient = Color4f::WHITE; //pStatic->ambient;
	material.Diffuse = Color4f::WHITE; //pStatic->diffuse;
	material.Diffuse.a=pStatic->diffuse.a*opacity*distance_alpha;
	material.Specular=pStatic->specular;
	material.Tex[0]=pStatic->pDiffuse;
	material.Tex[1]=0;
	material.point_light=0;
	
	cScene* pScene=pStatic->scene();
	material.Diffuse.mul3(material.Diffuse,pScene->GetSunDiffuse());
	material.Ambient.mul3(material.Ambient,pScene->GetSunAmbient());
	material.Specular.mul3(material.Specular,pScene->GetSunSpecular());

	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_anisotropic);

	float texture_phase=0;
	eBlendMode blend;
	if(getAttribute(ATTRSIMPLY3DX_OPACITY))
		blend=ALPHA_BLEND;
	else
	if(material.Tex[0] && material.Tex[0]->getAttribute(TEXTURE_ALPHA_TEST|TEXTURE_ALPHA_BLEND))
		blend=ALPHA_TEST;
	else
		blend=ALPHA_NONE;

	{
		int ref;
		if(IsDraw2Pass())
		{
			ref=round(BLEND_STATE_ALPHA_REF*material.Diffuse.a);
		}else
		{
			ref=blend==ALPHA_TEST?BLEND_STATE_ALPHA_REF:0;
		}

		gb_RenderDevice3D->SetRenderState(D3DRS_ALPHAREF,ref);
		gb_RenderDevice3D->SetBlendState(blend);
	}

	gb_RenderDevice3D->SetTexturePhase(0,material.Tex[0],texture_phase);
	gb_RenderDevice3D->SetTexturePhase(1,material.Tex[1],texture_phase);

	if(getAttribute(ATTRUNKOBJ_NOLIGHT))
	{
		vs=pShader3dx->vsSkinNoLight;
		ps=pShader3dx->psSkin;
		pShader3dx->psSkin->SelectLT(false,material.Tex[0] != 0);
	}else
	{
		if(camera->IsShadow())
		{
			vs=pShader3dx->vsSkinSceneShadow;
			if(material.Tex[0])
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
			pShader3dx->psSkin->SelectLT(true,material.Tex[0] != 0);
		}
	}

	vs->SetUVTrans(0);
	if(scene()->GetTileMap())
	{
		gb_RenderDevice3D->SetSamplerData(3,sampler_wrap_linear);
		gb_RenderDevice3D->SetTexture(3,gb_RenderDevice3D->GetLightMapObjects());
	}else
		gb_RenderDevice3D->SetTextureBase(3,0);

	bool reflectionz = camera->getAttribute(ATTRCAMERA_REFLECTION) != 0;
	vs->SetReflectionZ(reflectionz);
	ps->SetReflectionZ(reflectionz);

	{//Немного не к месту, зато быстро по скорости, для отражений.
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
		gb_RenderDevice3D->SetTexture(5,camera->GetZTexture());
	}

	if(camera->IsShadow())
		ps->SetShadowIntensity(scene()->GetShadowIntensity());
	ps->SetSelfIllumination(false);
	ps->SetMaterial(&material,pStatic->is_big_ambient);
	ps->Select();

	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	vs->Select(&node_position[0],node_position.size(),lod.blend_indices);
	vs->SetMaterial(&material);

}

void cSimply3dx::SelectShadowMaterial()
{
	gb_RenderDevice3D->SetTextureBase(1,0);

	eBlendMode blend=ALPHA_NONE;
	bool is_alphatest=false;
	cTexture* pDiffuse=pStatic->pDiffuse;
	if(pDiffuse)
	{
		//
		if(pDiffuse->isAlphaTest() || pDiffuse->isAlpha())
		{
			blend=ALPHA_TEST;
			is_alphatest=true;
		}
	}

	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_anisotropic);

	if(gb_RenderDevice3D->dtAdvance->GetID()!=DT_GEFORCEFX)
		blend=ALPHA_NONE;

	gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
	if(is_alphatest)
	{
		gb_RenderDevice3D->SetTexture(0,pDiffuse);
	}else
	{
		gb_RenderDevice3D->SetTextureBase(0,0);
	}
	VSSkinBase* vs=0;

	gb_RenderDevice3D->SetTextureBase(3,0);
	vs=pShader3dx->vsSkinShadow;
	if(is_alphatest)
	{
		pShader3dx->psSkinShadowAlpha->SetSecondOpacity(0);
		pShader3dx->psSkinShadowAlpha->Select();
	}else
		pShader3dx->psSkinShadow->Select();
	
	vs->SetUVTrans(0);
	vs->SetSecondOpacityUVTrans(0,SECOND_UV_NONE);
	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	vs->Select(&node_position[0],node_position.size(),lod.blend_indices);
}
void cSimply3dx::SelectZBufferMaterial()
{
	gb_RenderDevice3D->SetTextureBase(1,0);

	eBlendMode blend=ALPHA_NONE;
	bool is_alphatest=false;
	cTexture* pDiffuse=pStatic->pDiffuse;
	if(pDiffuse)
	{
		//
		if(pDiffuse->isAlphaTest() || pDiffuse->isAlpha())
		{
			blend=ALPHA_TEST;
			is_alphatest=true;
		}
	}

	gb_RenderDevice3D->SetBlendStateAlphaRef(blend);
	if(is_alphatest)
	{
		gb_RenderDevice3D->SetTexture(0,pDiffuse);
	}else
	{
		gb_RenderDevice3D->SetTextureBase(0,0);
	}
	gb_RenderDevice3D->SetTextureBase(3,0);
	if(is_alphatest)
	{
		pShader3dx->psSkinZBufferAlpha->SetSecondOpacity(0);
		pShader3dx->psSkinZBufferAlpha->Select();
	}
	else
	{
		pShader3dx->psSkinZBuffer->Select();
	}

	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	pShader3dx->vsSkinZBuffer->SetUVTrans(0);
	pShader3dx->vsSkinZBuffer->SetSecondOpacityUVTrans(0,SECOND_UV_NONE);
	pShader3dx->vsSkinZBuffer->Select(&node_position[0],node_position.size(),lod.blend_indices);
}

void cSimply3dx::SelectMatrix(int offset_matrix)
{
	pShader3dx->vsSkinNoLight->SetWorldMatrix(&node_position[0],offset_matrix,node_position.size());
}

void cSimply3dx::Draw(Camera* camera)
{
	start_timer_auto();
	
	if(debugShowSwitch.simplyObjects)
		return;

	SelectMaterial(camera);
	bool draw_2pass=IsDraw2Pass();

	DWORD old_zfunc;
	DWORD old_zwriteble;
	DWORD old_color;
	DWORD old_cull;

	if(draw_2pass)
	{
		old_zfunc = gb_RenderDevice3D->GetRenderState(D3DRS_ZFUNC);
		old_zwriteble = gb_RenderDevice3D->GetRenderState(D3DRS_ZWRITEENABLE);
		old_color = gb_RenderDevice3D->GetRenderState(D3DRS_COLORWRITEENABLE);
		old_cull = gb_RenderDevice3D->GetRenderState(D3DRS_CULLMODE);

		gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,0);
//		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
		gb_RenderDevice->SetRenderState( RS_CULLMODE, -1 );
	}

	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[iLOD];
	pStatic->DrawModels(1,lod);

	if(draw_2pass)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_EQUAL);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
		pStatic->DrawModels(1,lod);

		gb_RenderDevice3D->SetRenderState(D3DRS_ZWRITEENABLE,old_zwriteble);
		gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,old_zfunc);
		gb_RenderDevice3D->SetRenderState(D3DRS_COLORWRITEENABLE,old_color);
		gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE,old_cull);
	}
}

void cSimply3dx::PreDraw(Camera* camera)
{
}

void cSimply3dx::clearAttribute(int attribute)
{
	if(attribute & ATTRUNKOBJ_HIDE_BY_DISTANCE)
		distance_alpha = 1.0f;

	__super::clearAttribute(attribute);
}

bool cSimply3dx::CalcDistanceAlpha(Camera* camera, bool alpha)
{
	if(getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE))
	{
		float radius=GetBoundRadius();
		//float dist=camera->GetPos().distance(node_position[0].trans());
		float dist=camera->xformZ(node_position[0].trans());

		if(Option_HideSmoothly&&alpha)
		{
			if(dist>hideDistance)
				return false;
			float hideRange = hideDistance*0.15f;
			float fadeDist = hideDistance-hideRange;
			if(dist>fadeDist)
			{
				distance_alpha = 1.0f-(dist-fadeDist)/hideRange;
			}else
				distance_alpha=1.0f;
			CalcOpacityFlag();
		}else
		{
			if(dist>hideDistance)
				return false;
		}

	}

	return true;
}

void cSimply3dx::SetShadowType(ObjectShadowType type)
{
	pStatic->circle_shadow_enable=type;
	pStatic->circle_shadow_enable_min=min(pStatic->circle_shadow_enable,gb_VisGeneric->GetMaximalShadowObject());
}

void cSimply3dx::SetCircleShadowParam(float radius,float height)
{
	pStatic->circle_shadow_radius=radius;
	pStatic->circle_shadow_height=round(height);
}

ObjectShadowType cSimply3dx::getShadowType()
{
	return pStatic->circle_shadow_enable;
}

void cSimply3dx::getCircleShadowParam(float& radius, float& height)
{
	radius = pStatic->circle_shadow_radius;
	height = pStatic->circle_shadow_height;
}

cStaticSimply3dx::cStaticSimply3dx()
:BaseGraphObject(KIND_STATICSIMPLY3DX)
{
	num_out_polygons=0;
	num_out_objects=0;
	radius=0;
	pActiveSceneList=0;
	pDiffuse=0;

	ambient.set(1,1,1,1);
	diffuse.set(1,1,1,1);
	specular.set(1,1,1,1);
	is_opacity_texture=false;

	circle_shadow_enable=circle_shadow_enable_min=OST_SHADOW_REAL;
	circle_shadow_height=-1;
	is_big_ambient=false;
	circle_shadow_radius=10;

	bump=false;
	is_uv2=false;
	loaded = false;
	isDebris = false;
	enable_fur = false;
	num_material = 0;
	border_lod12_2=sqr(200);
	border_lod23_2=sqr(600);
}

cStaticSimply3dx::~cStaticSimply3dx()
{
	RELEASE(pDiffuse);
}

void cStaticSimply3dx::AddCircleShadow(Vect3f Simply3dxPos,float circle_shadow_radius)
{
	cTileMap* tilemap=scene()->GetTileMap();
	if(!tilemap)
		return;
	if(circle_shadow_height<0)
	{
		scene()->AddCircleShadow(Simply3dxPos,circle_shadow_radius,scene()->GetCircleShadowIntensity());
	}else
	{
		const Vect3f& pos=Simply3dxPos;
		int z = vMap.getZ(round(pos.x),round(pos.y));
		Color4c mul=scene()->GetCircleShadowIntensity();//
		int dz=z+circle_shadow_height-round(pos.z);
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

			scene()->AddCircleShadow(pos,circle_shadow_radius,intensity);
		}
	}
}

void cStaticSimply3dx::DrawModels(int num_models,ONE_LOD& lod)
{
	int polygons = num_models*lod.ib_polygon_one_models;
	gb_RenderDevice3D->DrawIndexedPrimitive(lod.vb,lod.vb_begin,lod.vb.GetNumberVertex(),lod.ib,lod.ib_begin,polygons);
	num_out_objects+=num_models;
	num_out_polygons+=polygons;
}

/*
  Для того, чтобы загрузить нужную группу видимости, необходимо.
  1. Определить какую cStaticIndex рассматривать.
  2. Распарсить cTempVisibleGroup и взять нужные треугольники.
  3. Пересобрать vertex buffer и индексы поменять.

  1 - пихаем нужные полигоны в новый буфер.
  2 - смотрим, какие вертексы используются.
  3 - создаем новый вертекс буфер
  4 - фиксим индекс буфер
  5 - сортируем.

  Несколько LOD.
     Пусть ноды для всех LODов будут одинаковыми.
	 Будут просто 3 куска vb, которые переключаются.
	 Задача - как можно быстрее отсортировать.
	 вариант - первым проходом определяем сколько каких, вторым проходом в нужный кусок пихаем в том же буфере.

	 Можно сделать 3 буфера для каждого лода.
	 Заранее определить какие кости будут.
	 И сортировать в зависимости от лода.

  Есть несколько vb,ib,node_index нужно слить в один и перенумеровать node_index
*/

// Рассчитывает какие новые индексы должны быть у компактифицированного вертекс буфера.
//  polygons_new[i][j]=old_to_new[polygons[i][j]]
//  vertex_new[k]=vertex[new_to_old[k]]
//  new_to_old.size() - размер нового вертекс буфера.
void CompactVertexBuffer(sPolygon* polygons,int polygon_num,
						 vector<int>& old_to_new,vector<int>& new_to_old)
{
	int num_index=0;
	for(int ipolygon=0;ipolygon<polygon_num;ipolygon++)
	{
		sPolygon& p=polygons[ipolygon];
		for(int i=0;i<3;i++)
			num_index=max(num_index,p[i]+1);
	}

	vector<bool> rename_vertex(num_index,false);
	for(int ipolygon=0;ipolygon<polygon_num;ipolygon++)
	{
		sPolygon& p=polygons[ipolygon];
		for(int i=0;i<3;i++)
		{
			WORD index=p[i];
			xassert(index<rename_vertex.size());
			rename_vertex[index]=true;
		}
	}

	int num_vertex=0;
	for(int ivertex=0;ivertex<num_index;ivertex++)
	if(rename_vertex[ivertex])
	{
		num_vertex++;
	}

	new_to_old.resize(num_vertex);
	old_to_new.resize(num_index);
	int cur_vertex=0;

	for(int ivertex=0;ivertex<num_index;ivertex++)
	{
		if(rename_vertex[ivertex])
		{
			new_to_old[cur_vertex]=ivertex;
			old_to_new[ivertex]=cur_vertex;
			cur_vertex++;
		}else
		{
			old_to_new[ivertex]=-1;
		}
	}
}

bool cStaticSimply3dx::BuildAllBuffers(vector<PsiVisible>& groups,cStatic3dx* pStatic,vector<int>& old_to_new_bone_index)
{
	lods.resize(groups.size());
	vector<TemporatyData> data(groups.size());

	vector<int> used_nodes(pStatic->nodes.size(),0);

	for(int idata=0;idata<groups.size();idata++)
	{
		cStatic3dx::StaticLod& static_lod=pStatic->lods[idata];
		cSkinVertex skin_vertex_in=pStatic->GetSkinVertex(static_lod.GetBlendWeight()),
					skin_vertex_out=skin_vertex_in;
		void *pVertexIn=gb_RenderDevice->LockVertexBuffer(static_lod.vb);
		skin_vertex_in.SetVB(pVertexIn,static_lod.vb.GetVertexSize());

		const StaticBunch& psi=*groups[idata].psi;
		DWORD visibility = groups[idata].visibility;
		TemporatyData& temp=data[idata];
		ONE_LOD& lod=lods[idata];
		lod.blend_indices=static_lod.blend_indices;


		int num_polygon=0;
		for(int ivg=0;ivg<psi.visibleGroups.size();ivg++)
		{
			const cTempVisibleGroup& vg=psi.visibleGroups[ivg];
			if(vg.visibilities & visibility)
			{
				num_polygon+=vg.num_polygon;
			}
		}

		lod.ib_polygon_one_models=num_polygon;
		temp.polygons.resize(num_polygon);
		sPolygon *IndexPolygonIn=gb_RenderDevice->LockIndexBuffer(static_lod.ib);

		int cur_polygon=0;
		for(int ivg=0;ivg<psi.visibleGroups.size();ivg++)
		{
			const cTempVisibleGroup& vg=psi.visibleGroups[ivg];
			if(vg.visibilities & visibility)
			{
				int index=vg.begin_polygon+psi.offset_polygon;
				for(int ipolygon=0;ipolygon<vg.num_polygon;ipolygon++)
				{
					temp.polygons[cur_polygon]=IndexPolygonIn[index];
					cur_polygon++;
					index++;
				}
			}
		}

		gb_RenderDevice->UnlockIndexBuffer(static_lod.ib);
		xassert(cur_polygon==num_polygon);

		CompactVertexBuffer(&temp.polygons[0],temp.polygons.size(),temp.old_to_new,temp.new_to_old);

		for(int ipolygon=0;ipolygon<num_polygon;ipolygon++)
		{
			sPolygon& p=temp.polygons[ipolygon];
			for(int j=0;j<3;j++)
			{
				WORD in=p[j];
				xassert(in<temp.old_to_new.size());
				p[j]=temp.old_to_new[in];
			}
		}

		lod.vb_vertex_one_models=temp.new_to_old.size();
		for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
		{
			int in_offset=temp.new_to_old[nvertex];
			skin_vertex_in.Select(in_offset);

			Color4c index=skin_vertex_in.GetIndex();
			for(int iblend=0;iblend<lod.blend_indices;iblend++)
			{
				int idx=index[iblend];
				xassert(idx<psi.nodeIndices.size());
				int global_idx=psi.nodeIndices[idx];
				xassert(global_idx>=0 && global_idx<used_nodes.size());
				used_nodes[global_idx]=1;
			}
		}

		gb_RenderDevice->UnlockVertexBuffer(static_lod.vb);
	}

	old_to_new_bone_index.clear();
	old_to_new_bone_index.resize(used_nodes.size(),-1);

	int num_actual_nodes=0;
	for(int inode=0;inode<used_nodes.size();inode++)
	{
		if(used_nodes[inode])
		{
			old_to_new_bone_index[inode]=num_actual_nodes;
			num_actual_nodes++;
		}
	}
	if(num_actual_nodes>=StaticBunch::max_index)
	{
		xxassert(0,"!!!При создании Simply3dx используется недопустимое количество нод, max = 20");
		return false;
	}
	int one_index=num_actual_nodes;
	node_offset.resize(one_index);
	node_name.resize(one_index);

	for(int idata=0;idata<groups.size();idata++)
	{
		cStatic3dx::StaticLod& static_lod=pStatic->lods[idata];
		cSkinVertex skin_vertex_in=pStatic->GetSkinVertex(static_lod.GetBlendWeight()),
					skin_vertex_out=skin_vertex_in;

		void *pVertexIn=gb_RenderDevice->LockVertexBuffer(static_lod.vb);
		skin_vertex_in.SetVB(pVertexIn,static_lod.vb.GetVertexSize());
		const StaticBunch& psi=*groups[idata].psi;
		DWORD visibility=groups[idata].visibility;
		TemporatyData& temp=data[idata];
		ONE_LOD& lod=lods[idata];

		{
			lod.num_repeat_models=StaticBunch::max_index/one_index;
			int num_repeat_models_by_polygon=max(2000/lod.ib_polygon_one_models,1);
			if(num_repeat_models_by_polygon<lod.num_repeat_models)
				lod.num_repeat_models=num_repeat_models_by_polygon;
		}

		int ib_polygon=lod.ib_polygon_one_models*lod.num_repeat_models;
		int vb_size=lod.vb_vertex_one_models*lod.num_repeat_models;

		gb_RenderDevice->CreateVertexBuffer(lod.vb,vb_size, skin_vertex_out.GetDeclaration());

		void *pVertexOut=gb_RenderDevice->LockVertexBuffer(lod.vb);
		skin_vertex_out.SetVB(pVertexOut,lod.vb.GetVertexSize());


		for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
		{
			int in_offset=temp.new_to_old[nvertex];
			skin_vertex_in.Select(in_offset);
			skin_vertex_out.Select(nvertex);
			memcpy(&skin_vertex_out.GetPos(),&skin_vertex_in.GetPos(),lod.vb.GetVertexSize());

			Color4c index=skin_vertex_in.GetIndex();
			for(int i=0;i<lod.blend_indices;i++)
			{
				int idx=index[i];
				xassert(idx<psi.nodeIndices.size());
				int global_idx=psi.nodeIndices[idx];
				xassert(global_idx<old_to_new_bone_index.size());
				idx=old_to_new_bone_index[global_idx];
				xassert(idx!=-1);
				index[i]=idx;
			}

			skin_vertex_out.GetIndex()=index;
		}

		skin_vertex_in.SetVB(pVertexOut,lod.vb.GetVertexSize());

		for(int nmodel=1;nmodel<lod.num_repeat_models;nmodel++)
		{
			for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
			{
				skin_vertex_in.Select(nvertex);
				skin_vertex_out.Select(nmodel*lod.vb_vertex_one_models+nvertex);
				Color4c index=skin_vertex_in.GetIndex();
				memcpy(&skin_vertex_out.GetPos(),&skin_vertex_in.GetPos(),lod.vb.GetVertexSize());
				xassert(index.r<StaticBunch::max_index); index.r+=nmodel*one_index;
				xassert(index.g<StaticBunch::max_index); index.g+=nmodel*one_index; 
				xassert(index.b<StaticBunch::max_index); index.b+=nmodel*one_index; 
				xassert(index.a<StaticBunch::max_index); index.a+=nmodel*one_index; 

				skin_vertex_out.GetIndex()=index;
			}
		}

		gb_RenderDevice->UnlockVertexBuffer(lod.vb);

		gb_RenderDevice->CreateIndexBuffer(lod.ib,ib_polygon);
		sPolygon *IndexPolygonOut=gb_RenderDevice->LockIndexBuffer(lod.ib);

		for(int nmodel=0;nmodel<lod.num_repeat_models;nmodel++)
		{
			for(int nindex=0;nindex<lod.ib_polygon_one_models;nindex++)
			{
				sPolygon in=temp.polygons[nindex];
				sPolygon *out=IndexPolygonOut+nindex+nmodel*lod.ib_polygon_one_models;

				for(int i=0;i<3;i++)
					in[i]=in[i]+nmodel*lod.vb_vertex_one_models;
				*out=in;
			}
		}

		gb_RenderDevice->UnlockIndexBuffer(lod.ib);
		gb_RenderDevice->UnlockVertexBuffer(static_lod.vb);
	}
	return true;
}

bool cStaticSimply3dx::BuildLods(cStatic3dx* pStatic,const char* visible_group)
{
	file_name=pStatic->fileName();
	bump=pStatic->bump;

	if(visible_group)
		visibleGroupName = visible_group;
	else
		visibleGroupName.clear();

	StaticVisibilitySet& vs = pStatic->visibilitySets_[0];

	int num_visible_group=-1;
	if(visible_group && visible_group[0]){
		StaticVisibilityGroups& vg=vs.visibilityGroups;
		for(int i=0;i<vg.size();i++){
			StaticVisibilityGroup& group = vg[i];
			if(group.name == visible_group){
				num_visible_group = i;
				break;
			}
		}
	}
	else
		num_visible_group=0;

	if(num_visible_group==-1){
		pStatic->errlog()<<"Visible group not found: "<<visible_group<<VERR_END;
		return false;
	}

	xassert(StaticVisibilitySet::num_lod==3);
	vector<PsiVisible> groups;
	
	int num_actual_lod = pStatic->is_lod ? 3 : 1;

	for(int i_lod=0;i_lod<num_actual_lod;i_lod++)
	{
		cStatic3dx::StaticLod& static_lod=pStatic->lods[i_lod];
		PsiVisible p;
		StaticVisibilityGroup& visibility_group=vs.visibilityGroups[num_visible_group];
		p.visibility=visibility_group.visibility;

		int isi;
		for(isi=0;isi<static_lod.bunches.size();isi++)
		{
			StaticBunch& s=static_lod.bunches[isi];
			bool found=false;
			for(int ivg=0;ivg<s.visibleGroups.size();ivg++)
			{
				if(s.visibleGroups[ivg].visibilities & p.visibility)
				{
					found=true;
					break;
				}
			}

			if(found)
				break;
		}

		if(isi==static_lod.bunches.size())
		{
			xassert(0 && "Не нашли полигонов в группе видимости");
			return false;
		}

		p.psi=&static_lod.bunches[isi];
		groups.push_back(p);
	}

	StaticMaterial& sm=pStatic->materials[groups[0].psi->imaterial];
	if(!sm.tex_diffuse.empty())
		pDiffuse=pStatic->LoadTexture(sm.tex_diffuse.c_str());
	ambient=sm.ambient;
	diffuse=sm.diffuse;
	specular=sm.specular;
	specular.a=sm.specular_power;
	is_opacity_texture=sm.is_opacity_texture;
	is_big_ambient=sm.is_big_ambient;

	vector<int> old_to_new_bone_index;
	if(!BuildAllBuffers(groups,pStatic,old_to_new_bone_index))
		return false;

	node_offset[0]=MatXf::ID;
	xassert(old_to_new_bone_index.size()==pStatic->nodes.size());
	for(int node_index=0;node_index<pStatic->nodes.size();node_index++)
	{
		int new_node_index=old_to_new_bone_index[node_index];
		if(new_node_index==-1)
			continue;
		StaticNode& sn=pStatic->nodes[node_index];
		if(node_index==0)
		{
			node_offset[new_node_index]=sn.inv_begin_pos;
			continue;
		}

		MatXf& m=node_offset[new_node_index];
		Mats pos;
		sn.chains[0].referencePose(pos);
		pos.copy_right(m);
		m=m*sn.inv_begin_pos;
		node_name[new_node_index]=sn.name;
	}

	CalcBoundBox();
	return true;
}

bool cStaticSimply3dx::BuildFromNode(cStatic3dx* pStatic,int iNode, VisibilityGroupIndex igroup)
{
	bump=pStatic->bump;
	is_uv2=pStatic->isUV2;
	enable_fur=pStatic->enableFur;

	StaticNode& sn=pStatic->nodes[iNode];
	file_name = pStatic->fileName();
	file_name += " ";
	file_name += sn.name;

	if(sn.name == "group center")
		return false;

	xassert(StaticVisibilitySet::num_lod==3);
	vector<PsiVisible> groups;
	PsiVisible p;
	StaticVisibilityGroup& visibility_group = pStatic->visibilitySets_[0].visibilityGroups[igroup];
	p.visibility = visibility_group.visibility;

	cStatic3dx::StaticLod& lod = pStatic->debris;
	StaticBunches::const_iterator iBunch;
	bool found = false;
	FOR_EACH(lod.bunches, iBunch){
		const StaticBunch& bunch = *iBunch;;
		TempVisibleGroups::const_iterator ivg;
		FOR_EACH(bunch.visibleGroups, ivg)
			if(ivg->visibilities & p.visibility && 
			  find(bunch.nodeIndices.begin(), bunch.nodeIndices.end(), iNode) != bunch.nodeIndices.end()){
				found = true;
				p.psi = &bunch;
				break;
			}
	}

	if(!found){
#ifndef _FINAL_VERSION_
		string error="Не нашли полигонов в группе видимости: ";
		error+=file_name;
		xassertStr(0,error.c_str());
#endif _FINAL_VERSION_
		return false;
	}

	groups.push_back(p);

	BuildBuffersOneNode(groups,pStatic,iNode);
	StaticMaterial& sm=pStatic->materials[groups[0].psi->imaterial];
	ambient=sm.ambient;
	diffuse=sm.diffuse;
	specular=sm.specular;
	specular.a=sm.specular_power;
	is_opacity_texture=sm.is_opacity_texture;
	is_big_ambient=sm.is_big_ambient;
//----

	node_offset.resize(1);
	node_name.resize(1);

	node_offset[0] = sn.inv_begin_pos;
	sn.chains[0].referencePose(debrisPos);

	node_name[0]=sn.name;

	CalcBoundBox();

//---
	return true;
}
void cStaticSimply3dx::BuildBuffersOneNode(vector<PsiVisible>& groups,cStatic3dx* pStatic,int nNode)
{
	if(true)
	{
		xassert(groups.size()==1);
		lods.resize(groups.size());
		cStatic3dx::StaticLod& static_lod=pStatic->debris;
		ONE_LOD& lod=lods[0];
		lod.ib.CopyAddRef(static_lod.ib);
		lod.vb.CopyAddRef(static_lod.vb);
		lod.num_repeat_models=1;

		bool found=false;
		for(int i=0;i<static_lod.bunches.size();i++)
		{
			StaticBunch& cur=static_lod.bunches[i];
			if(cur.nodeIndices[0]==nNode)
			{
				xassert(cur.nodeIndices.size()==1);
				xassert(cur.visibleGroups.size()==1);
				found=true;

				cTempVisibleGroup& vg=cur.visibleGroups[0];
				lod.ib_begin=cur.offset_polygon+vg.begin_polygon;
				lod.vb_begin=cur.offset_vertex;
				lod.ib_polygon_one_models=cur.num_polygon;
				lod.vb_vertex_one_models=cur.num_vertex;

				num_material = cur.imaterial;
			}
		}

		lod.blend_indices=static_lod.blend_indices;

		xassert(found);
	}else
	{
		xassert(groups.size()==1);
		num_material = groups[0].psi->imaterial;
		lods.resize(groups.size());
		
		{
			int idata=0;
			cStatic3dx::StaticLod& static_lod=pStatic->debris;
			ONE_LOD& lod=lods[idata];
			lod.blend_indices=static_lod.blend_indices;
			vector<int> nodeVertexIndex;
			vector<int> nodeVertexOldToNew;
			vector<sPolygon> nodePolygonIndex;

			const StaticBunch& s=*groups[idata].psi;

			cSkinVertex skin_vertex_in=pStatic->GetSkinVertex(static_lod.GetBlendWeight());
			cSkinVertex skin_vertex_out=skin_vertex_in;
			void *pVertex=gb_RenderDevice->LockVertexBuffer(static_lod.vb,true);
			skin_vertex_in.SetVB(pVertex,static_lod.vb.GetVertexSize());

			nodeVertexOldToNew.resize(s.num_vertex,-1);
			for(int i=s.offset_vertex;i<s.offset_vertex+s.num_vertex;i++)
			{
				skin_vertex_in.Select(i);
				BYTE idx=skin_vertex_in.GetIndex()[0];//<<Особенно здесь криво
				xassert(idx<s.nodeIndices.size());
				int node=s.nodeIndices[idx];
				if(node==nNode)
				{
					nodeVertexIndex.push_back(i);
					nodeVertexOldToNew[i-s.offset_vertex] = nodeVertexIndex.size()-1;
				}
			}
			sPolygon *IndexPolygonIn=gb_RenderDevice->LockIndexBuffer(static_lod.ib);
			/**/
			for(int i=s.offset_polygon;i<s.offset_polygon+s.num_polygon; i++)
			{
				int found=0;
				sPolygon plg;
				if(nodeVertexOldToNew[IndexPolygonIn[i].p1-s.offset_vertex] != -1 &&
				nodeVertexOldToNew[IndexPolygonIn[i].p2-s.offset_vertex] != -1 &&
				nodeVertexOldToNew[IndexPolygonIn[i].p3-s.offset_vertex] != -1)
				{
					plg.p1 = nodeVertexOldToNew[IndexPolygonIn[i].p1-s.offset_vertex];
					plg.p2 = nodeVertexOldToNew[IndexPolygonIn[i].p2-s.offset_vertex];
					plg.p3 = nodeVertexOldToNew[IndexPolygonIn[i].p3-s.offset_vertex];
					nodePolygonIndex.push_back(plg);
				}
			}
			/**/
			gb_RenderDevice->UnlockIndexBuffer(static_lod.ib);

			lod.ib_polygon_one_models = nodePolygonIndex.size();
			lod.vb_vertex_one_models = nodeVertexIndex.size();
			lod.num_repeat_models=1;

			int ib_polygon=lod.ib_polygon_one_models*lod.num_repeat_models;
			int vb_size=lod.vb_vertex_one_models*lod.num_repeat_models;

			gb_RenderDevice->CreateVertexBuffer(lod.vb,vb_size, skin_vertex_out.GetDeclaration());
			void *pVertexOut=gb_RenderDevice->LockVertexBuffer(lod.vb);
			skin_vertex_out.SetVB(pVertexOut,lod.vb.GetVertexSize());

			for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
			{
				int in_offset=nodeVertexIndex[nvertex];
				skin_vertex_in.Select(in_offset);
				skin_vertex_out.Select(nvertex);
				memcpy(&skin_vertex_out.GetPos(),&skin_vertex_in.GetPos(),lod.vb.GetVertexSize());

				Color4c index=skin_vertex_in.GetIndex();
				for(int i=0;i<lod.blend_indices;i++)
				{
					index[i]=0;
				}

				skin_vertex_out.GetIndex()=index;
			}

			skin_vertex_in.SetVB(pVertexOut,lod.vb.GetVertexSize());
			for(int nmodel=1;nmodel<lod.num_repeat_models;nmodel++)
			{
				for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
				{
					skin_vertex_in.Select(nvertex);
					skin_vertex_out.Select(nmodel*lod.vb_vertex_one_models+nvertex);
					Color4c index=skin_vertex_in.GetIndex();
					memcpy(&skin_vertex_out.GetPos(),&skin_vertex_in.GetPos(),lod.vb.GetVertexSize());
					xassert(index.r<StaticBunch::max_index); index.r+=nmodel;
					xassert(index.g<StaticBunch::max_index); index.g+=nmodel; 
					xassert(index.b<StaticBunch::max_index); index.b+=nmodel; 
					xassert(index.a<StaticBunch::max_index); index.a+=nmodel; 

					skin_vertex_out.GetIndex()=index;
				}
			}

			gb_RenderDevice->UnlockVertexBuffer(lod.vb);
			gb_RenderDevice->UnlockVertexBuffer(static_lod.vb);
			gb_RenderDevice->CreateIndexBuffer(lod.ib,ib_polygon);
			sPolygon *IndexPolygonOut=gb_RenderDevice->LockIndexBuffer(lod.ib);
			for(int nmodel=0;nmodel<lod.num_repeat_models;nmodel++)
			{
				for(int nindex=0;nindex<lod.ib_polygon_one_models;nindex++)
				{
					sPolygon in=nodePolygonIndex[nindex];
					sPolygon *out=IndexPolygonOut+nindex+nmodel*lod.ib_polygon_one_models;

					for(int i=0;i<3;i++)
						in[i]=in[i]+nmodel*lod.vb_vertex_one_models;
					*out=in;
				}
			}
			gb_RenderDevice->UnlockIndexBuffer(lod.ib);
		}
	}
}

void cStaticSimply3dx::CalcBoundBox()
{
	bound_box.invalidate();
	radius=0;

	vector<ONE_LOD>::iterator itlod;
	FOR_EACH(lods,itlod)
	{
		ONE_LOD& lod=*itlod;
		cSkinVertex skin_vertex=GetSkinVertex(lod.GetBlendWeight());
		void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);
		skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

		float int2float=1/255.0f;
		MatXf* world=&node_offset[0];
		int blend_weight=lod.GetBlendWeight();

		for(int nvertex=0;nvertex<lod.vb_vertex_one_models;nvertex++)
		{
			skin_vertex.Select(nvertex);
			Vect3f global_pos;
			Vect3f global_norm;
			if(lod.blend_indices==1)
			{
				int idx=skin_vertex.GetIndex()[0];
				if(idx >= node_offset.size())
					idx = node_offset.size() - 1; // !!! fix
				Vect3f pos=skin_vertex.GetPos();
				global_pos=world[idx]*pos;

				Vect3f nrm=skin_vertex.GetNorm();
				global_norm=world[idx].rot()*nrm;
			}else
			{
				global_pos=Vect3f::ZERO;
				global_norm=Vect3f::ZERO;
				Vect3f pos=skin_vertex.GetPos();
				Vect3f nrm=skin_vertex.GetNorm();
				for(int ibone=0;ibone<blend_weight;ibone++)
				{
					int idx=skin_vertex.GetIndex()[ibone];
					if(idx >= node_offset.size())
						idx = node_offset.size() - 1; // !!! fix
					float weight=skin_vertex.GetWeight(ibone)*int2float;
					global_pos+=(world[idx]*pos)*weight;
					global_norm+=(world[idx].rot()*nrm)*weight;
				}
			}

			bound_box.addPoint(global_pos);
			float r=global_pos.norm();
			radius=max(radius,r);
		}

		gb_RenderDevice->UnlockVertexBuffer(lod.vb);
	}
}

void SortByLod(cSimply3dx** object,int num_visible_object,Camera* camera,cStaticSimply3dx* pStatic)
{
	if(num_visible_object==0)
		return;

	int num_by_lod[cStaticSimply3dx::num_lod];
	for(int i=0;i<cStaticSimply3dx::num_lod;i++)
		num_by_lod[i]=0;

	for(int i=0;i<num_visible_object;i++)
	{
		cSimply3dx* p=object[i];
		p->iLOD=pStatic->GetLodByDistance2(camera->GetPos().distance2(p->position.trans()));
		num_by_lod[p->iLOD]++;
	}

	int offset_by_lod[cStaticSimply3dx::num_lod];
	int end_offset_by_lod[cStaticSimply3dx::num_lod];
	int i_by_lod[cStaticSimply3dx::num_lod];
	i_by_lod[0]=offset_by_lod[0]=0;
	i_by_lod[1]=offset_by_lod[1]=offset_by_lod[0]+num_by_lod[0];
	i_by_lod[2]=offset_by_lod[2]=offset_by_lod[1]+num_by_lod[1];
	end_offset_by_lod[0]=offset_by_lod[1];
	end_offset_by_lod[1]=offset_by_lod[2];
	end_offset_by_lod[2]=offset_by_lod[2]+num_by_lod[2];

	//vector<int> in(num_visible_object);
	//for(int i=0;i<num_visible_object;i++)
	//	in[i]=object[i]->iLOD;

	xassert(num_visible_object==end_offset_by_lod[2]);
	for(int ilod=0;ilod<cStaticSimply3dx::num_lod;ilod++)
	{
		int begin=i_by_lod[ilod],end=end_offset_by_lod[ilod];
		for(int i=begin;i<end;)
		{
			int lod=object[i]->iLOD;
			int& offset=i_by_lod[lod];
			if(lod!=ilod)
			{
				while(object[offset]->iLOD==lod)
					offset++;
				swap(object[i],object[offset]);
			}
			offset++;

			if(object[i]->iLOD==ilod)
				i++;
		}
	}

	{//Куча проверок.
		for(int ilod=0;ilod<StaticVisibilitySet::num_lod;ilod++)
			xassert(offset_by_lod[ilod]<=end_offset_by_lod[ilod]);
		for(int i=offset_by_lod[0];i<offset_by_lod[1];i++)
			xassert(object[i]->iLOD==0);
		for(int i=offset_by_lod[1];i<offset_by_lod[2];i++)
			xassert(object[i]->iLOD==1);
		for(int i=offset_by_lod[2];i<num_visible_object;i++)
			xassert(object[i]->iLOD==2);
	}
}

void cStaticSimply3dx::PreDraw(Camera* camera)
{
	num_out_polygons=0;
	num_out_objects=0;
	num_visible_object=0;
	num_visible_opacity_object=0;
	//Проверка на видимость.
	//Складываем видимые объекты вначало списка.
	vector<cSimply3dx*>& active_list=*pActiveSceneList;
	bool is_lod=lods.size()==StaticVisibilitySet::num_lod;

	int size=active_list.size();
	int i;
	for(i=0;i<size;i++)
	{
		cSimply3dx* p=active_list[i];
		if(camera->TestVisible(p->GetCenterObjectInline(),p->GetBoundRadiusInline()))
//		Vect3f center=p->GetPosition().trans();
//		if(camera->TestVisible(round(center.x),round(center.y)))
		{
			if(!p->CalcDistanceAlpha(camera))
				continue;
			if(p->getAttribute(ATTRUNKOBJ_IGNORE))
				continue;
			swap(active_list[i],active_list[num_visible_object]);

			if(p->getAttribute(ATTRSIMPLY3DX_OPACITY))//В начале списка полупрозрачные объекты.
			{
				swap(active_list[num_visible_object],active_list[num_visible_opacity_object]);
				num_visible_opacity_object++;
			}
				
			num_visible_object++;
		}
	}

	if(is_lod && !active_list.empty() && num_visible_object-num_visible_opacity_object>0)
		SortByLod(&active_list[num_visible_opacity_object],num_visible_object-num_visible_opacity_object,camera,this);

	if(num_visible_object)
	{
		camera->AttachNoRecursive(SCENENODE_OBJECT,this);
		Camera* pReflection=camera->FindChildCamera(ATTRCAMERA_REFLECTION);
		if(pReflection)
		{
			pReflection->AttachNoRecursive(SCENENODE_OBJECT,this);
		}
	}

	int list_size = active_list.size();
	for(i=0;i<list_size;i++)
	{
		cSimply3dx* p=active_list[i];
		if(circle_shadow_enable_min==OST_SHADOW_CIRCLE)
			AddCircleShadow(p->position.trans(),circle_shadow_radius*p->position.scale());
	}

	for(i=0;i<num_visible_opacity_object;i++)
	{
		cSimply3dx* p=active_list[i];
//		xassert(p->getAttribute(ATTRSIMPLY3DX_OPACITY));
		//if(p->IsDraw2Pass())
		//	camera->AttachNoRecursive(SCENENODE_OBJECT,p);
		//else
			camera->AttachNoRecursive(SCENENODE_OBJECTSORT,p);
	}

	//Видимость теней - отдельный список.
	Camera* pShadow=camera->FindChildCamera(ATTRCAMERA_SHADOWMAP);
	if(pShadow)
	{
		shadow_visible_list.clear();
		for(int i=0;i<size;i++)
		{
			cSimply3dx* p=active_list[i];
			if(!p->CalcDistanceAlpha(camera,false))
				continue;
			if(p->getAttribute(ATTRUNKOBJ_IGNORE))
				continue;
			//if(circle_shadow_enable_min==OST_SHADOW_REAL && !p->getAttribute(ATTRSIMPLY3DX_OPACITY))
			if(circle_shadow_enable_min == OST_SHADOW_REAL)
				if(pShadow->TestVisible(p->GetCenterObjectInline(),p->GetBoundRadiusInline()))
					shadow_visible_list.push_back(p);
		}
		
		if(is_lod && !shadow_visible_list.empty())
			SortByLod(shadow_visible_list.empty()?0:&shadow_visible_list[0],shadow_visible_list.size(),camera,this);

		if(!shadow_visible_list.empty())
			pShadow->AttachNoRecursive(SCENENODE_OBJECT,this);
	}

	Camera* pZBufferCamera = camera->FindChildCamera(ATTRCAMERA_FLOAT_ZBUFFER);
	if(pZBufferCamera)
	{
		zBufferVisibleList.clear();
		for(int i=0;i<size;i++)
		{
			cSimply3dx* p=active_list[i];
			if(!p->CalcDistanceAlpha(camera,false))
				continue;
			if(p->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
				if(pZBufferCamera->TestVisible(p->GetCenterObjectInline(),p->GetBoundRadiusInline()))
				{
					zBufferVisibleList.push_back(p);
				}
		}
		if(is_lod)
			SortByLod(zBufferVisibleList.empty()?0:&zBufferVisibleList[0],zBufferVisibleList.size(),camera,this);
		if(!zBufferVisibleList.empty())
			pZBufferCamera->AttachNoRecursive(SCENENODE_OBJECT,this);
	}
}

void cStaticSimply3dx::AddZMinZMaxShadowReciver(MatXf& camera_matrix,Vect2f& all)
{//После PreDraw естественно только работает нормально.

	for(vector<cSimply3dx*>::iterator it=shadow_visible_list.begin();it!=shadow_visible_list.end();++it)
	{
		cSimply3dx* p=*it;
        Vect3f& pos=p->position.trans();
        float radius=p->GetBoundRadiusInline();
        Vect3f out_pos;

        camera_matrix.xformPoint(pos,out_pos);
        all.x=min(all.x,out_pos.z-radius);
        all.y=max(all.x,out_pos.z+radius);
	}
}

void cStaticSimply3dx::DrawObjects(Camera* camera,cSimply3dx** objects,int num_object)
{
/*Тупой Draw
	for(int i=0;i<num_object;i++)
	{
		objects[i]->Draw(camera);
	}
/*/
	//bool zbuffer = camera->getAttribute(ATTRCAMERA_ZBUFFER);//Довольно пренеприятная сточенция.
	//bool floatZBuffer =camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER);
	int num_matrix=node_offset.size();

	int cur_object=0;

	while(cur_object<num_object)
	{
		int num_models=0;
		int ilod=objects[cur_object]->iLOD;
		ONE_LOD& lod=lods[ilod];
		for(;cur_object<num_object && objects[cur_object]->iLOD==ilod;cur_object++)
		{
	//		xassert(!objects[i]->getAttribute(ATTRSIMPLY3DX_OPACITY));
			objects[cur_object]->SelectMatrix(num_models*num_matrix);
			num_models++;
			
			if(num_models==lod.num_repeat_models)
			{
				DrawModels(num_models,lod);
				num_models=0;
			}
		}

		if(num_models)
			DrawModels(num_models,lod);
	}
/**/
}

void cStaticSimply3dx::Draw(Camera* camera)
{
	if(debugShowSwitch.simplyObjects)
		return;

	if(camera->getAttribute(ATTRCAMERA_SHADOWMAP))
	{
		DrawShadow(camera);
		return;
	}
	if(camera->getAttribute(ATTRCAMERA_FLOAT_ZBUFFER))
	{
		if(Option_FloatZBufferType==2)
			DrawZBuffer(camera);
		return;
	}
	vector<cSimply3dx*>& active_list=*pActiveSceneList;
	xassert(num_visible_object<=active_list.size());

	int num_object=num_visible_object-num_visible_opacity_object;
	if(num_object<=0)
		return;
	active_list[num_visible_opacity_object]->SelectMaterial(camera);
	DrawObjects(camera,&active_list[num_visible_opacity_object],num_object);
}

void cStaticSimply3dx::DrawShadow(Camera* camera)
{
	if(shadow_visible_list.empty())
		return;
	shadow_visible_list[0]->SelectShadowMaterial();
	DrawObjects(camera, shadow_visible_list.empty()?0:&shadow_visible_list[0],shadow_visible_list.size());
}
void cStaticSimply3dx::DrawZBuffer(Camera* camera)
{
	if(zBufferVisibleList.empty())
		return;
	zBufferVisibleList[0]->SelectZBufferMaterial();
	DrawObjects(camera, zBufferVisibleList.empty()?0:&zBufferVisibleList[0],zBufferVisibleList.size());
}

int cSimply3dx::FindNode(const char* node_name) const
{
	int n_size=pStatic->node_name.size();
	for(int i=0;i<n_size;i++)
	{
		if(pStatic->node_name[i]==node_name)
			return i;
	}

	return -1;
}

const MatXf& cSimply3dx::GetNodePosition(int nodeindex) const
{
	xassert(nodeindex>=0 && nodeindex<node_position.size());
	return node_position[nodeindex];
}

void cSimply3dx::SetNodePosition(int nodeindex,const MatXf& pos)
{
	xassert(nodeindex>=0 && nodeindex<node_position.size());
	node_position[nodeindex]=pos;

	user_node_positon|=(1ul<<nodeindex);
}

void cSimply3dx::SetNodePosition(int nodeindex,const Se3f& pos)
{
	MatXf m_pos;
	m_pos.set(pos);
	SetNodePosition(nodeindex,m_pos);
}

void cSimply3dx::SetNodePositionMats(int nodeindex,const Mats& pos)
{
	MatXf m;
	pos.copy_right(m);
	SetNodePosition(nodeindex,m);
}

void cSimply3dx::UseDefaultNodePosition(int nodeindex)
{
	xassert(nodeindex>=0 && nodeindex<node_position.size());
	user_node_positon&=~(1ul<<nodeindex);
}

const MatXf& cSimply3dx::GetNodeInitialOffset(int nodeindex) const
{
	xassert(nodeindex>=0 && nodeindex<pStatic->node_offset.size());
	return pStatic->node_offset[nodeindex];
}

void cSimply3dx::GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node)
{
	xassert(selected_node==-1);
	all.triangles.clear();
	all.visible_points.clear();
	all.positions.clear();
	all.normals.clear();
	all.uv.clear();
	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[0];
	if(!lod.vb.IsInit() || !lod.ib.IsInit())
		return;

	bool zero_pos=(tif_flags&TIF_ZERO_POS)?true:false;
	bool one_scale=(tif_flags&TIF_ONE_SCALE)?true:false;
	int num_points=lod.vb_vertex_one_models;

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
		for(int i=0;i<num_points;i++)
			all.visible_points[i]=false;
	}

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
				SetPosition(MatXf::ID);
			}
			Update();
		}

		int pointindex=0;
		cSkinVertex skin_vertex=pStatic->GetSkinVertex(lod.GetBlendWeight());
		void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);
		skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

		MatXf* world=&node_position[0];
		int world_num=node_position.size();

		int blend_weight=lod.GetBlendWeight();
		float int2float=1/255.0f;
		for(int vertex=lod.vb_begin;vertex<lod.vb_begin+num_points;vertex++)
		{
			skin_vertex.Select(vertex);
			Vect3f global_pos;
			Vect3f global_norm;
			if(lod.blend_indices==1)
			{
				int idx=skin_vertex.GetIndex()[0];
				Vect3f pos=skin_vertex.GetPos();
				global_pos=world[idx]*pos;

				Vect3f nrm=skin_vertex.GetNorm();
				global_norm=world[idx].rot()*nrm;
			}else
			{
				global_pos=Vect3f::ZERO;
				global_norm=Vect3f::ZERO;
				Vect3f pos=skin_vertex.GetPos();
				Vect3f nrm=skin_vertex.GetNorm();
				for(int ibone=0;ibone<blend_weight;ibone++)
				{
					int idx=skin_vertex.GetIndex()[ibone];
					float weight=skin_vertex.GetWeight(ibone)*int2float;
					global_pos+=(world[idx]*pos)*weight;
					global_norm+=(world[idx].rot()*nrm)*weight;
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
			pointindex++;
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
		int num_triangles=lod.ib_polygon_one_models;

		if(tif_flags&TIF_TRIANGLES)
			all.triangles.resize(num_triangles);
		int offset_triangles=0;
		sPolygon *IndexPolygon=gb_RenderDevice->LockIndexBuffer(lod.ib);

		bool is_visible=false;

		int offset_buf=0;

		for(int ipolygon=lod.ib_begin;ipolygon<lod.ib_begin+num_triangles;ipolygon++)
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
		
		gb_RenderDevice->UnlockIndexBuffer(lod.ib);
	}
}

void cSimply3dx::GetEmitterMaterial(cObjMaterial& material)
{
	material.Ambient = pStatic->ambient;
	material.Diffuse = pStatic->diffuse;
	material.Specular = pStatic->specular;
	material.Power = pStatic->specular.a;
}

void cSimply3dx::GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm)
{

	if (pos.size() != norm.size())
		norm.resize(pos.size());

	cStaticSimply3dx::ONE_LOD& lod=pStatic->lods[0];
	int num_points=lod.vb_vertex_one_models;

	int blend_weight=lod.GetBlendWeight();
	cSkinVertex skin_vertex=pStatic->GetSkinVertex(blend_weight);
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());
	float int2float=1/255.0f;

	for (int i=0; i<pos.size(); i++)
	{
		int vertex = graphRnd()%num_points;
		vertex+=lod.vb_begin;

		MatXf* world=&node_position[0];
		skin_vertex.Select(vertex);
		Vect3f global_pos;
		Vect3f global_norm;
		if(lod.blend_indices==1)
		{
			int idx=skin_vertex.GetIndex()[0];
			MatXf&  mat_world=world[idx];
			
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
				MatXf&  mat_world=world[idx];
				float weight=skin_vertex.GetWeight(ibone)*int2float;

				global_pos+=(mat_world*pos)*weight;
				global_norm+=(mat_world.rot()*nrm)*weight;
			}
		}
		pos[i] = global_pos;
		norm[i] = global_norm;

	}
	gb_RenderDevice->UnlockVertexBuffer(lod.vb);
}

int cSimply3dx::GetNodeNumber() const
{
	return node_position.size();
}

const char* cSimply3dx::GetNodeName(int node_index) const
{
	xassert(node_index>=0 && node_index<pStatic->node_name.size());
	return pStatic->node_name[node_index].c_str();
}

const Mats& cSimply3dx::GetNodePositionMats(int nodeindex) const
{
	static Mats m;
	m = GetNodePosition(nodeindex);
	return m;
}

const char* cSimply3dx::GetFileName() const
{
	return pStatic->file_name.c_str();
}
cTexture* cSimply3dx::GetDiffuseTexture(int num_mat)const
{
	return pStatic->pDiffuse;
}
void cSimply3dx::SetDiffuseTexture(cTexture* texture)
{
	RELEASE(pStatic->pDiffuse);
	if(texture)texture->AddRef();
	pStatic->pDiffuse = texture;
}

void cStaticSimply3dx::SetLodDistance(float lod12,float lod23)
{
	xassert(lod12<lod23);
	border_lod12_2=sqr(lod12);
	border_lod23_2=sqr(lod23);
}
void cStaticSimply3dx::SetHideDistance(float distance)
{
	hideDistance = distance;
}

void cSimply3dx::SetLodDistance(float lod12,float lod23)
{
	pStatic->SetLodDistance(lod12,lod23);
}

void cSimply3dx::SetHideDistance(float distance)
{
	hideDistance = distance;
}
