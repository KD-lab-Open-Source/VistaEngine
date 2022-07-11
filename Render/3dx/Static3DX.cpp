#include "StdAfxRd.h"
#include "Static3dx.h"
#include "TexLibrary.h"
#include "D3DRender.h"
#include "scene.h"
#include "VisGeneric.h"
#include "AccessTexture.h"
#include "Serialization\XPrmArchive.h"
#include "Serialization\InPlaceArchive.h"
#include "FileUtils\FileUtils.h"

cStatic3dx::cStatic3dx(bool isLogic, const char* fname)
: Static3dxBase(isLogic),
  voxelBox(5)
{
	fileName_ = fname;
	inPlace_ = false;
}

cStatic3dx::~cStatic3dx()
{
	for(int i=0;i<lods.size();i++){
		StaticLod& l=lods[i];
		delete[] l.sys_vb;
		delete[] l.sys_ib;
	}

	releaseTextures();

	vector<cStaticSimply3dx*>::iterator iss;
	FOR_EACH(debrises,iss)
		RELEASE((*iss));
}

void cStatic3dx::GetTextureNames(TextureNames& names) const
{
	for(int i = 0; i < materials.size(); ++i){
		const StaticMaterial& sm = materials[i];
		if(!sm.tex_diffuse.empty())
			names.add(fixTextureName(sm.tex_diffuse.c_str()));
		if(!sm.tex_bump.empty())
			names.add(fixTextureName(sm.tex_bump.c_str()));
		if(!sm.tex_reflect.empty())
			names.add(fixTextureName(sm.tex_reflect.c_str()));
		if(!sm.tex_specularmap.empty())
			names.add(fixTextureName(sm.tex_specularmap.c_str()));
		if(!sm.tex_self_illumination.empty())
			names.add(fixTextureName(sm.tex_self_illumination.c_str()));
		if(!sm.tex_secondopacity.empty())
			names.add(fixTextureName(sm.tex_secondopacity.c_str()));
		if(!sm.tex_skin.empty())
			names.add(fixTextureName(sm.tex_skin.c_str()));
		if(!sm.tex_furnormalmap.empty())
			names.add(fixTextureName(sm.tex_furnormalmap.c_str()));
		if(!sm.tex_furmap.empty())
			names.add(fixTextureName(sm.tex_furmap.c_str()));
	}

	for(StaticLights::const_iterator itl=lights.begin();itl!=lights.end();++itl){
		const StaticLight& s = *itl;
		if(!s.texture.empty())
			names.add(fixTextureName(s.texture.c_str()));
	}
}

string cStatic3dx::fixTextureName(const char* name) const
{
	if(!name || !strlen(name))
		return "";
	return extractFilePath(fileName()) + "Textures\\" + extractFileName(name);
}

cTexture* cStatic3dx::LoadTexture(const char* name, char* mode)
{
	if(name==0 || name[0]==0) 
		return 0;
	bool prev_error=GetTexLibrary()->EnableError(false);
	//cTexture* p=GetTexLibrary()->GetElement(name,mode);

	cTexture* p=GetTexLibrary()->GetElement3D((extractFilePath(fileName()) + "Textures\\" + name).c_str(), mode);
	if(!p)
		errlog()<<"Texture is bad: "<<name<<"."<<VERR_END;
	GetTexLibrary()->EnableError(prev_error);
	return p;
}

bool StaticLogos::GetLogoPosition(const char* fname, sRectangle4f *rect, float& angle)
{
	if(fname[0]==0)
	{
		rect->min.set(0,0);
		rect->max.set(0,0);
		return false;
	}

	string name=extractFileName(fname);
	strupr((char*)name.c_str()); 
	for (int i = 0; i<logos.size(); i++)
	{
		if (strcmp(logos[i].TextureName.c_str(),name.c_str()) == 0)
		{
			*rect = logos[i].rect;
			angle = logos[i].angle;
			return true;
		}
	}
	rect->min.set(0,0);
	rect->max.set(0,0);
	return false;
}

void cStatic3dx::saveInPlace(const char* fileName)
{
	int numRefs = GetRef();
	for(int i = 0; i < numRefs; i++)
		DecRef();

	LodsCache cache;
	Lods::iterator iLod;
	FOR_EACH(lods, iLod){
		cache.lods.push_back(LodCache());
		iLod->prepareToSerialize(cache.lods.back());
	}

	debris.prepareToSerialize(cache.debris);

	if(!is_logic){
		string bufferName = fileName;
		bufferName += 'B';
		InPlaceOArchive oab(bufferName.c_str(), true);
		oab.serialize(cache, "LodsCache", 0);
	}

	Debrises debrisesTemp;
	debrisesTemp.swap(debrises);

	InPlaceOArchive oa(fileName, true);
	oa.serialize(*this, "cStatic3dx", 0);

	debrisesTemp.swap(debrises);

	for(int i = 0; i < numRefs; i++)
		AddRef();
}

cStatic3dx::StaticLod::StaticLod()
{
	blend_indices=1;
	sys_vb = 0;
	sys_ib = 0;
}

bool cStatic3dx::load(const char* fileName)
{
	if(!__super::load(fileName))
		return false;

	if(!is_logic)
		createTextures();
	prepareMesh();
	tempMeshLod1_.clear();
	tempMeshLod2_.clear();
	tempMesh_.clear();

	return true;
}

void cStatic3dx::createTextures()
{
	StaticMaterials::iterator mi;
	FOR_EACH(materials, mi)
		mi->createTextures(this);

	StaticLights::iterator iLight;
	FOR_EACH(lights, iLight)
		iLight->pTexture = LoadTexture(iLight->texture.c_str());

	StaticLeaves::iterator iLeaf;
	FOR_EACH(leaves, iLeaf)
		iLeaf->pTexture = LoadTexture(iLeaf->texture.c_str());
}

void cStatic3dx::releaseTextures()
{
	StaticMaterials::iterator iMaterial;
	FOR_EACH(materials, iMaterial)
		iMaterial->releaseTextures();

	StaticLights::iterator iLight;
	FOR_EACH(lights, iLight)
		RELEASE(iLight->pTexture);
	
	StaticLeaves::iterator iLeaf;
	FOR_EACH(leaves, iLeaf)
		RELEASE(iLeaf->pTexture);
}

//!!!! Если furinfo поменялось, перегенерировать кешь.
void cStatic3dx::loadFur()
{
	typedef vector<FurInfo> FurInfos;

	XPrmIArchive ia(0);
	if(ia.open(setExtention(fileName(), "fur").c_str())){
		FurInfos furInfos;
		ia.serialize(furInfos, "furInfos", "furInfos");
		FurInfos::iterator iFur;
		FOR_EACH(furInfos, iFur){
			StaticMaterials::iterator iMaterial;
			FOR_EACH(materials, iMaterial)
				if(iMaterial->name == iFur->material){
					iMaterial->loadFur(*iFur, this);
					enableFur = true;
				}
		}
	}
}

void StaticMaterial::loadFur(const FurInfo& furInfo, cStatic3dx* object)
{
	fur_scale = furInfo.scale;
	fur_alpha = furInfo.alpha;
	fur_alpha_type = furInfo.furType;

	tex_furmap  = furInfo.texture;
	pFurmap = object->LoadTexture(tex_furmap.c_str(), "NoResize");

	tex_furnormalmap = furInfo.normal;
}

void StaticMaterial::createTextures(cStatic3dx* object)
{
	texturesCreated = true;
	gb_RenderDevice3D->SetCurrentConvertDot3Mul(10.0f);//Потом читать из файла

	if(!tex_bump.empty() && gb_RenderDevice3D->IsPS20())
		pBumpTexture = object->LoadTexture(tex_bump.c_str(),"Bump");

	if(!tex_specularmap.empty() && gb_RenderDevice3D->IsPS20())
		pSpecularmap = object->LoadTexture(tex_specularmap.c_str(),"Specular");

	if(!tex_secondopacity.empty())
		pSecondOpacityTexture = object->LoadTexture(tex_secondopacity.c_str());

	if(!tex_furmap.empty())
		pFurmap = object->LoadTexture(tex_furmap.c_str(), "NoResize");

	if(!tex_reflect.empty()){
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath(tex_reflect.c_str(),drive,dir,fname,ext);
		if(stricmp(fname,"sky")==0 && cScene::IsSkyCubemap())
			is_reflect_sky = true;
		else
			pReflectTexture = object->LoadTexture(tex_reflect.c_str());
	}
}

void StaticMaterial::releaseTextures()
{
	texturesCreated = false;
	RELEASE(pSecondOpacityTexture);
	RELEASE(pBumpTexture);
	RELEASE(pReflectTexture);
	RELEASE(pSpecularmap);
	RELEASE(pFurmap);
}

void cStatic3dx::prepareMesh()
{
	DummyVisibilitySet();

	PrepareIndices();
	ParseEffect();

	if(!is_logic){
		BuildMeshes();
		CreateDebrises();
	}

	//BuildLeavesLods();
	//ClearDebrisNodes();
}

void cStatic3dx::PrepareIndices()
{
	for(int iag=0;iag<animationGroups_.size();iag++){
		AnimationGroup& ag=animationGroups_[iag];
		ag.nodes.clear();
		int i;
		for(i=0;i<ag.nodesNames.size();i++){
			string& name=ag.nodesNames[i];
			if(name=="_base_")
				continue;

			int inode;
			for(inode=0;inode<nodes.size();inode++){
				StaticNode& s=nodes[inode];
				if(name==s.name)
					break;
			}

			if(inode>=nodes.size())
				inode=0;

			ag.nodes.push_back(inode);
		}

		ag.nodesNames.clear();

		for(i=0;i<ag.materialsNames.size();i++){
			string& name=ag.materialsNames[i];
			for(int imat=0;imat<materials.size();imat++){
				StaticMaterial& sm=materials[imat];
				if(name==sm.name)
					sm.animation_group_index=iag;
			}
		}
		ag.materialsNames.clear();
	}
}

void cStatic3dx::CreateDebrises()
{
	start_timer_auto();

	if(visibilitySets_.empty())
		return;
	StaticVisibilitySet& vs = visibilitySets_[0];
	VisibilityGroupIndex vgi = vs.GetVisibilityGroupIndex("debris");
	if(vgi == VisibilityGroupIndex::BAD)
		return;

	StaticVisibilityGroup& group = vs.visibilityGroups[vgi];
	for(int i=0; i < group.visibleNodes.size(); i++)
	{
		if(group.visibleNodes[i]){
			cStaticSimply3dx* obj = new cStaticSimply3dx();
			if(!obj->BuildFromNode(this,i,vgi)){
				RELEASE(obj);
				continue;
			}
			obj->isDebris = true;
			debrises.push_back(obj);
		}
	}
}

struct SortMyMaterial
{
	cStatic3dx* data;
	SortMyMaterial(cStatic3dx* data_)
	{
		data=data_;
	}

	bool operator()(const TempMesh p1,const TempMesh p2) const
	{
		StaticMaterial& s1=data->materials[p1->imaterial];
		StaticMaterial& s2=data->materials[p2->imaterial];
		if(s1.pBumpTexture<s2.pBumpTexture)
			return true;
		if(s1.pBumpTexture>s2.pBumpTexture)
			return false;
		if(p1->imaterial<p2->imaterial)
			return true;
		if(p1->imaterial>p2->imaterial)
			return false;
		xassert(p1->visible_group.size()==1);
		xassert(p2->visible_group.size()==1);
		const cTempVisibleGroup& vg1=p1->visible_group[0];
		const cTempVisibleGroup& vg2=p2->visible_group[0];
		if(vg1.visibilitySet<vg2.visibilitySet)
			return true;
		if(vg1.visibilitySet>vg2.visibilitySet)
			return false;

		return vg1.visibilities < vg2.visibilities;
	}
};

struct SortMyNodeIndex
{
	cStatic3dx* data;
	SortMyNodeIndex(cStatic3dx* data_)
	{
		data=data_;
	}

	bool operator()(const TempMesh p1,const TempMesh p2) const
	{
		StaticMaterial& s1=data->materials[p1->imaterial];
		StaticMaterial& s2=data->materials[p2->imaterial];
		if(p1->inode<p2->inode)
			return true;
		if(p1->inode>p2->inode)
			return false;

		if(s1.pBumpTexture<s2.pBumpTexture)
			return true;
		if(s1.pBumpTexture>s2.pBumpTexture)
			return false;
		if(p1->imaterial<p2->imaterial)
			return true;
		if(p1->imaterial>p2->imaterial)
			return false;
		xassert(p1->visible_group.size()==1);
		xassert(p2->visible_group.size()==1);
		const cTempVisibleGroup& vg1=p1->visible_group[0];
		const cTempVisibleGroup& vg2=p2->visible_group[0];
		if(vg1.visibilitySet<vg2.visibilitySet)
			return true;
		if(vg1.visibilitySet>vg2.visibilitySet)
			return false;

		return vg1.visibilities < vg2.visibilities;
	}
};


void cStatic3dx::DummyVisibilitySet()
{
	if(visibilitySets_.empty()){
		visibilitySets_.push_back(StaticVisibilitySet());
		StaticVisibilitySet& set = visibilitySets_.back();
		set.name = "default";

		set.meshes.resize(tempMesh_.size());
		for(int i=0;i < tempMesh_.size();i++)
			set.meshes[i] = tempMesh_[i]->name;

		set.DummyVisibilityGroup();
	}
}

int cTempMesh3dx::CalcMaxBonesPerVertex()
{
	int max_nonzero_bones=0;
	max_nonzero_bones=0;
	for(int i=0;i<bones.size();i++)
	{
		cTempBone& p=bones[i];
		int cur_bones=0;
		for(int ibone=0;ibone<MAX_BONES;ibone++)
		{
			if(p.weight[ibone]>=1/255.0f)
				cur_bones++;
		}
		max_nonzero_bones=max(max_nonzero_bones,cur_bones);
	}

	return max_nonzero_bones;
}

void cStatic3dx::BuildMeshes()
{
	is_lod = !tempMeshLod1_.empty() && !tempMeshLod2_.empty();

	StaticVisibilitySets::iterator iSet;
	FOR_EACH(visibilitySets_, iSet){
		StaticVisibilitySet& set = *iSet;
		xassert(set.visibilityGroups.size()<=32);
		for(int igroup=0;igroup<set.visibilityGroups.size();igroup++)
			set.visibilityGroups[igroup].visibility = 1<<igroup;
	}

	if(is_lod){
		lods.resize(StaticVisibilitySet::num_lod);
		BuildMeshesLod(tempMesh_, 0, false);
		BuildMeshesLod(tempMeshLod1_, 1, false);
		BuildMeshesLod(tempMeshLod2_, 2, false);
	}
	else{
		lods.resize(1);
		BuildMeshesLod(tempMesh_, 0, false);
	}

	BuildMeshesLod(tempMesh_, 0, true);

	//Ноды относятся ко всем, а меши - к определенным группам видимости.
	//Вместо temp_invisible_object, нужно temp_visible_object.
	FOR_EACH(visibilitySets_, iSet){
		StaticVisibilitySet& set = *iSet;
		StaticVisibilityGroups::iterator iGroup;
		FOR_EACH(set.visibilityGroups, iGroup){
			StaticVisibilityGroup& group = *iGroup;
			xassert(group.visibleNodes.empty());
			group.visibleNodes.resize(nodes.size());
			for(int inode=0;inode<nodes.size();inode++){
				bool found = group.meshes.exists(nodes[inode].name);
				if(group.is_invisible_list)
					found=!found;
				group.visibleNodes[inode]=found;
			}
		}
	}
}

void cStatic3dx::BuildMeshesLod(const TempMeshes& tempMeshesIn, int ilod, bool isDebris)
{
	TempMeshes temp_mesh;

	TempMeshes::const_iterator iMesh;
	FOR_EACH(tempMeshesIn, iMesh){
		TempMesh mesh = *iMesh;
		int visible_set;
		DWORD visible;
		bool is_debris,is_no_debris;
		GetVisibleMesh(mesh->name,visible_set,visible,is_debris,is_no_debris);
		mesh->FillVisibleGroup(visible_set,visible);

		if(isDebris ? !is_debris : !is_no_debris)
			continue;
		/*
		Нужно ещё 
		читать/писать кэш
		строить debrises и simply 3dx не создавая vb под эти данные.
		*/

		temp_mesh.push_back(*iMesh);
	}

	if(temp_mesh.empty())
		return;

	if(!isDebris)
		sort(temp_mesh.begin(),temp_mesh.end(),SortMyMaterial(this));
	else
		sort(temp_mesh.begin(),temp_mesh.end(),SortMyNodeIndex(this));
	///////////////////////

	TempMeshes material_mesh;
	MergeMaterialMesh(temp_mesh,material_mesh,!isDebris);
	int max_bones=4;//К сожалению на заметили ускорения от ограничения количества костей. Нужно дальше разбираться.
	BuildBuffers(material_mesh, isDebris ? debris : lods[ilod], max_bones);
}

void cTempMesh3dx::FillVisibleGroup(int visible_set, DWORD visibilities)
{
	visible_group.resize(1);
	cTempVisibleGroup& vg = visible_group[0];
	vg.visibilitySet = visible_set;
	vg.visibilities = visibilities;
	vg.begin_polygon = 0;
	vg.num_polygon = polygons.size();
	vg.visibilityNodeIndex = visibilityAnimated ? inode : -1;
}

void cStatic3dx::GetVisibleMesh(const string& mesh_name,int& visible_set,DWORD& visible,bool& is_debris,bool& is_no_debris)
{
	visible_set=0;
	visible=0;
	is_debris=false;
	is_no_debris=false;

	StaticVisibilitySets::iterator iSet;
	FOR_EACH(visibilitySets_, iSet){
		StaticVisibilitySet& set = *iSet;

		xassert(!set.visibilityGroups.empty());

		StaticVisibilityGroups::iterator iGroup;
		FOR_EACH(set.visibilityGroups, iGroup){
			StaticVisibilityGroup& group = *iGroup;

			bool found = group.meshes.exists(mesh_name);

			if(group.is_invisible_list)
				found=!found;

			if(found){
				if(group.name=="debris")
					is_debris=true;
				else
					is_no_debris=true;
				xassert(group.visibility);
				visible |= group.visibility;
				visible_set = iSet - visibilitySets_.begin();
			}
		}
	}
}

void cStatic3dx::BuildBuffers(TempMeshes& temp_mesh,StaticLod& lod,int max_bones)
{
	if(temp_mesh.empty())
	{
		//		errlog()<<"Нет ни одного объекта"<<VERR_END;
		return;
	}

	TempMeshes out_mesh;
	BuildSkinGroupSortedNormal(temp_mesh,out_mesh);

	int vertex_size=0;
	int polygon_size=0;
	bool is_blend=false;

	TempMeshes::iterator itm;
	int max_bones_per_vertex=1;
	FOR_EACH(out_mesh,itm)
	{
		TempMesh m=*itm;
		m->OptimizeMesh();

		vertex_size+=m->vertex_pos.size();
		polygon_size+=m->polygons.size();
		int bones_per_vertex=m->CalcMaxBonesPerVertex();
		max_bones_per_vertex=max(max_bones_per_vertex,bones_per_vertex);
		if(!m->bones.empty())
			is_blend=true;

		bool is_bump=materials[m->imaterial].pBumpTexture!=0;
		if(is_bump)
			bump=true;
	}

	max_bones_per_vertex=min(max_bones,max_bones_per_vertex);

	if(vertex_size>65535)
		errlog()<<"Количество вертексов "<<vertex_size<<" превышает максимально допустимые 65535"<<VERR_END;
	xassert(polygon_size<65535);

	vector<sVertexXYZINT1> vertex(vertex_size);
	vector<sPolygon> polygons(polygon_size);

	gb_RenderDevice->CreateIndexBuffer(lod.ib,polygon_size);
	sPolygon *IndexPolygon=gb_RenderDevice->LockIndexBuffer(lod.ib);

	lod.blend_indices=max_bones_per_vertex;

	cSkinVertex skin_vertex=GetSkinVertex(lod.GetBlendWeight());

	gb_RenderDevice->CreateVertexBuffer(lod.vb,vertex_size, skin_vertex.GetDeclaration());
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);

	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

	lod.bunches.resize(out_mesh.size());
	int cur_vertex_offset=0;
	int cur_polygon_offset=0;
	for(int i=0;i<out_mesh.size();i++){
		cTempMesh3dx& tmesh=*out_mesh[i];
		StaticBunch& bunch=lod.bunches[i];
		bunch.offset_polygon=cur_polygon_offset;
		bunch.offset_vertex=cur_vertex_offset;
		bunch.num_polygon=tmesh.polygons.size();
		bunch.num_vertex=tmesh.vertex_pos.size();
		bunch.imaterial=tmesh.imaterial;
		bunch.nodeIndices=tmesh.inode_array;
		bunch.visibleGroups=tmesh.visible_group;

		for(int index=0;index<bunch.num_polygon;index++){
			sPolygon& in=tmesh.polygons[index];
			sPolygon& out=IndexPolygon[cur_polygon_offset+index];
			out.p1=in.p1+cur_vertex_offset;
			out.p2=in.p2+cur_vertex_offset;
			out.p3=in.p3+cur_vertex_offset;
		}

		for(int vertex=0;vertex<bunch.num_vertex;vertex++){
			skin_vertex.Select(cur_vertex_offset+vertex);
			skin_vertex.GetPos()=tmesh.vertex_pos[vertex];
			skin_vertex.GetNorm()=tmesh.vertex_norm[vertex];
			if(tmesh.vertex_uv.empty())
				skin_vertex.GetTexel().set(0,0);
			else
				skin_vertex.GetTexel()=tmesh.vertex_uv[vertex];

			if(isUV2)
				if(tmesh.vertex_uv2.empty())
					skin_vertex.GetTexel2().set(0,0);
				else
					skin_vertex.GetTexel2()=tmesh.vertex_uv2[vertex];

			Color4c& index=skin_vertex.GetIndex();
			index.RGBA()=0;
		}

		cur_vertex_offset+=tmesh.vertex_pos.size();
		cur_polygon_offset+=tmesh.polygons.size();
	}

	gb_RenderDevice->UnlockIndexBuffer(lod.ib);

	for(int i=0; i < out_mesh.size(); i++){
		cTempMesh3dx& tmesh=*out_mesh[i];
		StaticBunch& sinsex=lod.bunches[i];

		StaticBunch* cur_index=0;
		int j;
		for(j=0;j<lod.bunches.size();j++){
			StaticBunch& s=lod.bunches[j];
			if(s.offset_polygon<=sinsex.offset_polygon && 
			  s.offset_polygon+s.num_polygon>sinsex.offset_polygon){
				cur_index=&lod.bunches[j];
				break;
			}
		}

		vector<int> node_hash(nodes.size(), -1);
		for(j=0;j<cur_index->nodeIndices.size();j++){
			int idx=cur_index->nodeIndices[j];
			xassert(idx>=0 && idx<node_hash.size());
			xassert(node_hash[idx]==-1);
			node_hash[idx]=j;
		}

		if(!tmesh.bones.empty()){
			for(int vertex=0;vertex<sinsex.num_vertex;vertex++){
				skin_vertex.Select(sinsex.offset_vertex+vertex);
				cTempBone& tb=tmesh.bones[vertex];

				Color4c weight;
				weight.RGBA()=0;
				xassert(lod.blend_indices<=max_bones);
				int sum=0;
				int ibone;
				int weights=lod.GetBlendWeight();

				int error_weight=0;
				for(ibone=0;ibone<weights;ibone++){
					int w=int(tb.weight[ibone]*255);
					error_weight+=w;
				}
				error_weight=255-error_weight;

				for(ibone=0;ibone<weights;ibone++){
					int w=int(tb.weight[ibone]*255);
					if(ibone==0)
						w+=error_weight;
					xassert(w>=0 && w<=255);
					skin_vertex.GetWeight(ibone)=w;
					sum+=w;
				}

				xassert(sum==255 || weights==0);
				Color4c index;
				index.RGBA()=0;
				for(ibone=0;ibone<lod.blend_indices;ibone++){
					int idx=0;
					int cur_inode=tb.inode[ibone];
					if(cur_inode>=0)
						idx=node_hash[cur_inode];
					xassert(idx>=0 && idx<node_hash.size());
					index[ibone]=idx;
				}
				skin_vertex.GetIndex()=index;
			}
		}
		else{
			xassert(0);
		}
	}
	gb_RenderDevice->UnlockVertexBuffer(lod.vb);

	CalcBumpSTNorm(lod);

	//	FOR_EACH(out_mesh,itm)
	//		delete *itm;

}

void cStatic3dx::CalcBumpSTNorm(StaticLod& lod)
{
	if(!bump && !enableFur)
		return;

	cSkinVertex skin_vertex=GetSkinVertex(lod.GetBlendWeight());
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());
	vector<Vect3f> vS(lod.vb.GetNumberVertex()),
		vT(lod.vb.GetNumberVertex());

	sPolygon *Polygon=gb_RenderDevice->LockIndexBuffer(lod.ib);
	int i;
	for(i=0;i<lod.vb.GetNumberVertex();i++)
	{
		skin_vertex.Select(i);
		vS[i].set(0,0,0);
		vT[i].set(0,0,0);
		skin_vertex.GetNorm().normalize();
	}

	cSkinVertex v0=GetSkinVertex(lod.GetBlendWeight()),
		v1=GetSkinVertex(lod.GetBlendWeight()),
		v2=GetSkinVertex(lod.GetBlendWeight());
	v0.SetVB(pVertex,lod.vb.GetVertexSize());
	v1.SetVB(pVertex,lod.vb.GetVertexSize());
	v2.SetVB(pVertex,lod.vb.GetVertexSize());
	/*
	for(i=0;i<ib_polygon;i++)
	{
	sPolygon &p=Polygon[i];
	v0.Select(p.p1);
	v1.Select(p.p2);
	v2.Select(p.p3);

	if(true)//check normal
	{
	Vect3f d1=v1.GetPos()-v0.GetPos(),d2=v2.GetPos()-v0.GetPos();
	Vect3f n;
	n.cross(d2,d1);
	n.normalize();
	Vect3f& n0=v0.GetNorm();
	Vect3f& n1=v1.GetNorm();
	Vect3f& n2=v2.GetNorm();
	xassert(n.dot(n0)>0);
	xassert(n.dot(n1)>0);
	xassert(n.dot(n2)>0);
	}
	}
	*/
	for(i=0;i<lod.ib.GetNumberPolygon();i++)
	{
		Vect3f cp;
		sPolygon &p=Polygon[i];
		v0.Select(p.p1);
		v1.Select(p.p2);
		v2.Select(p.p3);

		const Vect3f& p0 = v0.GetPos();
		const Vect3f& p1 = v1.GetPos();
		const Vect3f& p2 = v2.GetPos();

		const Vect2f& w0 = v0.GetTexel();
		const Vect2f& w1 = v1.GetTexel();
		const Vect2f& w2 = v2.GetTexel();

		float x1 = p1.x - p0.x;
		float x2 = p2.x - p0.x;
		float y1 = p1.y - p0.y;
		float y2 = p2.y - p0.y;
		float z1 = p1.z - p0.z;
		float z2 = p2.z - p0.z;

		float s1 = w1.x - w0.x;
		float s2 = w2.x - w0.x;
		float t1 = w1.y - w0.y;
		float t2 = w2.y - w0.y;

		float invr=s1 * t2 - s2 * t1;
		if(fabsf(invr)>1e-8f)
		{
			float r = 1.0F / invr;
			Vect3f sdir((t2 * x1 - t1 * x2) * r, 
				(t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);
			Vect3f tdir((s1 * x2 - s2 * x1) * r, 
				(s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r);

			vS[p.p1] += sdir;
			vS[p.p2] += sdir;
			vS[p.p3] += sdir;

			vT[p.p1] += tdir;
			vT[p.p2] += tdir;
			vT[p.p3] += tdir;
		}
	}

	for(i=0;i<lod.vb.GetNumberVertex();i++)
	{
		skin_vertex.Select(i);
		Vect3f  n=skin_vertex.GetNorm();
		Vect3f& s=vS[i];
		Vect3f& t=vT[i];
		Vect3f prev_s=s;

		t=(t-n*n.dot(t)).normalize();
		s.cross(n, t);
		float dots=dot(s,prev_s);
		if(dots<0)
		{
			s=-s;
		}

		if(bump)
		{
			skin_vertex.GetBumpT()=t;
			skin_vertex.GetBumpS()=s;
		}

		if(enableFur)
		{
			Color4c& c=skin_vertex.GetFur();
			c[0]=n.z*127+127;
			c[1]=n.y*127+127;
			c[2]=n.x*127+127;
			c[3]=255;
		}
	}

	//Блин! по материалам разбивка нужна
	cAccessTexture tex;
	if(enableFur)
	{
		for(int isi=0;isi<lod.bunches.size();++isi)
		{
			StaticBunch& si=lod.bunches[isi];
			string& name=materials[si.imaterial].tex_furnormalmap;
			if(!name.empty())
			{
				if(!tex.load(name.c_str()))
				{
					errlog()<<"Cannot load "<<name<<VERR_END;
				}
			}

			int up_vertex=si.offset_vertex+si.num_vertex;

			if(tex.empty())
			{
				for(int i=si.offset_vertex;i<up_vertex;i++)
				{
					skin_vertex.Select(i);
					Color4c& c=skin_vertex.GetFur();
					Vect3f n=skin_vertex.GetNorm();
					c.r=round(n.x*127.5f+127);
					c.g=round(n.y*127.5f+127);
					c.b=round(n.z*127.5f+127);
					c.a=255;
				}
			}else
			{
				for(int i=si.offset_vertex;i<up_vertex;i++)
				{
					skin_vertex.Select(i);
					Vect3f n=skin_vertex.GetNorm();
					Vect3f& s=vS[i];
					Vect3f& t=vT[i];
					Color4c& c=skin_vertex.GetFur();
					Vect2f& uv=skin_vertex.GetTexel();
					Color4c tc=tex.get1(uv.x,uv.y);
					Vect3f ntc;
					ntc.x=tc.r/127.5f-1;
					ntc.y=tc.g/127.5f-1;
					ntc.z=tc.b/127.5f-1;
					ntc.normalize();

					Vect3f tc_global;
					//Надо из tangent space в глобальное перевести.
					tc_global.x=ntc.x*t.x+ntc.y*s.x+ntc.z*n.x;
					tc_global.y=ntc.x*t.y+ntc.y*s.y+ntc.z*n.y;
					tc_global.z=ntc.x*t.z+ntc.y*s.z+ntc.z*n.z;


					c.r=round(tc_global.x*127.5f+127);
					c.g=round(tc_global.y*127.5f+127);
					c.b=round(tc_global.z*127.5f+127);
					c.a=tc.a;
				}
			}
		}
	}

	gb_RenderDevice->UnlockVertexBuffer(lod.vb);
	gb_RenderDevice->UnlockIndexBuffer(lod.ib);
}

cVisError& cStatic3dx::errlog()
{
	return VisError<<"Error load file: "<<fileName()<<"\r\n";
}

static bool is_space(char c)
{
	return c==' ' || c==8;
}

static bool is_name_char(char c)
{
	return (c>='0' && c<='9') || 
		(c>='a' && c<='z') ||
		(c>='A' && c<='Z') || c=='_';
}

void cStatic3dx::ParseEffect()
{
	for(int inode=0;inode<nodes.size();inode++)
	{
		StaticNode& node=nodes[inode];
		static char eff[]="effect:";
		const char* cur=node.name.c_str();
		const char* end;
		if(strncmp(cur,eff,sizeof(eff)-1)!=0)
			continue;
		cur+=sizeof(eff)-1;
		while(is_space(*cur))
			cur++;
		end=cur;
		while(is_name_char(*end))
			end++;

		string file_name(cur,end-cur);

		EffectKey* key=gb_EffectLibrary->Get((gb_VisGeneric->GetEffectPath()+file_name).c_str(),1,gb_VisGeneric->GetEffectTexturePath());

		if(!key)
		{
			errlog()<<"Object: \""<<node.name.c_str()<<"\"\r\n"<<
				"Effect library path: "<<gb_VisGeneric->GetEffectPath()<<"\"\r\n"<<
				"Library not found: \""<<file_name.c_str()<<"\""<<VERR_END;
			return;
		}

		StaticEffect effect;
		effect.node=inode;
		string effect_name(cur,end-cur);
		effect.file_name=file_name;
		effect.is_cycled=key->isCycled();

		effects.push_back(effect);
	}
}

/*
1. Не правильный.
Берем все объекты, относящиеся с одному материалу.
Ищем с какой нодой больше треугольников.
Ищем, какие 2ноды с ней контачат, и выбираем те, в которых больше треугольников.
Теперь у нас 3 ноды.
Ищем с какой из оставшихся нод больше треугольников, причем отдаем предпочтение 
тем треугольникам, в которых уже есть ноды из списка.
Копируем найденные треугольники в новую ноду, и помечаем их, как пустые.

2. Добавляем все ноды первого попавшегося треугольника в список.

Эмпирически ищем еще одну ноду, которая входит в максимальное 
количество треугольников, но в для тех, в которых есть уже выбранные - большой бонус.
Добавляем ноду к списку выбранных.
Добавляем, если есть треугольники к списку выбранных.
И так по кругу, пока не будет 20 нод.

...
Тут надо в целом переделывать все связанное со Split/BuildSkinGroup/заполнением вертекс и индекс буферов.

1 - убираем split
2 - скидываем все cTempMesh3dx с одинаковыми материалами в один.
3 - применяем процедуру для разбиения по 20 к этим мешам.

Дописать !!!!!
Объекты , которые ведут себя по разному в разных группах видимости, не должны мержиться.
Пронумеровать их разными индексами и отсортировать по ним.
В выходные данные указать на то в каких группах видимости эти меши видны.

Потом переключать бОльшим количеством DIP их.
Это сэкономит сильно размеры, и 
множества видимости - это всего лишь разные if при DIP.
*/
void cStatic3dx::MergeMaterialMesh(TempMeshes& temp_mesh, TempMeshes& material_mesh, bool merge_mesh)
{
	if(temp_mesh.empty())
		return;

	TempMeshes cur_mesh;
	cur_mesh.push_back(temp_mesh[0]);

	for(int itemp=1;itemp<temp_mesh.size();itemp++){
		TempMesh p = temp_mesh[itemp];
		bool mergerable = false;
		if(merge_mesh)
			mergerable = p->imaterial == cur_mesh[0]->imaterial;
		else
			mergerable = p->inode == cur_mesh[0]->inode;
		if(!mergerable){
			TempMesh mesh = new cTempMesh3dx;
			mesh->Merge(cur_mesh,this);
			material_mesh.push_back(mesh);
			cur_mesh.clear();
		}
		cur_mesh.push_back(p);
	}

	if(!cur_mesh.empty())
	{
		TempMesh mesh=new cTempMesh3dx;
		mesh->Merge(cur_mesh,this);
		material_mesh.push_back(mesh);
		cur_mesh.clear();
	}
}

void cTempMesh3dx::Merge(TempMeshes& meshes,cStatic3dx* pStatic)
{
	int num_vertices=0;
	int num_polygons=0;
	typedef StaticMap<int,int> NODE_MAP;
	NODE_MAP node_map;
	int itemp;
	for(itemp=0;itemp<meshes.size();itemp++)
	{
		TempMesh p=meshes[itemp];
		num_vertices+=p->vertex_pos.size();
		num_polygons+=p->polygons.size();

		int sz=p->vertex_pos.size();
		xassert(sz==p->vertex_norm.size());
		xassert(sz==p->vertex_uv.size() || p->vertex_uv.size()==0);
		xassert(sz==p->vertex_uv2.size() || p->vertex_uv2.size()==0);
		xassert(sz==p->bones.size() || p->bones.size()==0);

		int n=p->inode;
		node_map[n]=1;
		for(int inode=0;inode<p->inode_array.size();inode++){
			n=p->inode_array[inode];
			node_map[n]=1;
		}
	}

	imaterial=meshes[0]->imaterial;
	inode=meshes[0]->inode;

	NODE_MAP::iterator it_map;
	FOR_EACH(node_map,it_map){
		int n=it_map->first;
		node_map[n]=inode_array.size();
		inode_array.push_back(n);
	}

	vertex_pos.resize(num_vertices);
	vertex_norm.resize(num_vertices);
	vertex_uv.resize(num_vertices);
	vertex_uv2.resize(num_vertices);
	bones.resize(num_vertices);
	polygons.resize(num_polygons);

	int offset_vertices=0;
	int offset_polygon=0;

	for(itemp=0;itemp<meshes.size();itemp++){
		TempMesh p=meshes[itemp];
		cTempBone temp_bone;
		temp_bone.weight[0]=1;
		temp_bone.weight[1]=0;
		temp_bone.weight[2]=0;
		temp_bone.weight[3]=0;

		temp_bone.inode[0]=p->inode;
		temp_bone.inode[1]=
			temp_bone.inode[2]=
			temp_bone.inode[3]=-1;

		for(int ipos=0;ipos<p->vertex_pos.size();ipos++){
			vertex_pos[ipos+offset_vertices]=p->vertex_pos[ipos];
			vertex_norm[ipos+offset_vertices]=p->vertex_norm[ipos];
			if(p->vertex_uv.empty())
				vertex_uv[ipos+offset_vertices].set(0,0);
			else
				vertex_uv[ipos+offset_vertices]=p->vertex_uv[ipos];

			if(p->vertex_uv2.empty())
				vertex_uv2[ipos+offset_vertices].set(0,0);
			else
				vertex_uv2[ipos+offset_vertices]=p->vertex_uv2[ipos];

			if(p->bones.size()){
				cTempBone& tb=p->bones[ipos];
				cTempBone& out=bones[ipos+offset_vertices];
				for(int ibone=0;ibone<MAX_BONES;ibone++){
					int bone=tb.inode[ibone];
					if(tb.weight[ibone]>0)
						out.inode[ibone]=bone;
					else
						out.inode[ibone]=-1;
					out.weight[ibone]=tb.weight[ibone];
				}
			}
			else
				bones[ipos+offset_vertices]=temp_bone;
		}

		for(int ipolygon=0;ipolygon<p->polygons.size();ipolygon++){
			sPolygon& pin=p->polygons[ipolygon];
			sPolygon& pout=polygons[ipolygon+offset_polygon];
			for(int ipnt=0;ipnt<3;ipnt++)
				pout[ipnt]=pin[ipnt]+offset_vertices;
		}

		xassert(p->visible_group.size()==1);
		cTempVisibleGroup in_vg=p->visible_group[0];
		in_vg.begin_polygon=offset_polygon;
		if(!visible_group.empty()){
			cTempVisibleGroup& out_vg=visible_group.back();
			if(out_vg.visibilitySet == in_vg.visibilitySet 
			  && out_vg.visibilities == in_vg.visibilities 
			  && out_vg.visibilityNodeIndex == in_vg.visibilityNodeIndex)
				out_vg.num_polygon += in_vg.num_polygon;
			else
				visible_group.push_back(in_vg);
		}
		else
			visible_group.push_back(in_vg);

		offset_vertices+=p->vertex_pos.size();
		offset_polygon+=p->polygons.size();
	}
}

/*
Дописать!!!
Учитывать разбиение на cTempVisibleGroup.
Перемещать треугольники внутри ноды только.
*/
void cStatic3dx::BuildSkinGroupSortedNormal(TempMeshes& temp_mesh,TempMeshes& out_mesh)
{
	TempMeshes::iterator it_mesh;
	FOR_EACH(temp_mesh,it_mesh)
	{
		TempMesh tm=*it_mesh;
		vector<char> selected_polygon(tm->polygons.size());//0 - свободный, 1 - текущий, 2 - предыдущий меш.
		vector<bool> selected_node(nodes.size());
		int ipolygon;
		for(ipolygon=0;ipolygon<tm->polygons.size();ipolygon++)
			selected_polygon[ipolygon]=0;
		int inode;
		for(inode=0;inode<selected_node.size();inode++)
			selected_node[inode]=false;


		{
			cTempVisibleGroup& vgf=tm->visible_group.front();
			cTempVisibleGroup& vgb=tm->visible_group.back();
			xassert(vgf.begin_polygon==0);
			xassert(vgb.begin_polygon+vgb.num_polygon==tm->polygons.size());
		}

		int cur_visible_group=0;
		int cur_added_polygon=0;

		int min_visible_group=0;
		int polygon_offset=0;

		bool next_mesh=true;
		while(1)
		{
			cTempVisibleGroup* cur_vg=0;

			int first_polygon=-1;
			while(cur_visible_group<tm->visible_group.size())
			{
				cur_vg=&tm->visible_group[cur_visible_group];
				//Выходим, если нет больше полигонов, иначе выбираем первый свободный.
				for(int ipolygon=cur_vg->begin_polygon;ipolygon<cur_vg->begin_polygon+cur_vg->num_polygon;ipolygon++)
				{
					if(!selected_polygon[ipolygon])
					{
						first_polygon=ipolygon;
						break;
					}
				}

				if(first_polygon==-1)
				{
					xassert(cur_added_polygon==cur_vg->num_polygon);
					cur_visible_group++;
					cur_added_polygon=0;
				}else
					break;
			}

			if(cur_visible_group<tm->visible_group.size())
				cur_vg=&tm->visible_group[cur_visible_group];
			else
				cur_vg=0;

			vector<int> node_index;
			for(inode=0;inode<selected_node.size();inode++)
			{
				if(selected_node[inode])
					node_index.push_back(inode);
			}

			int first_additional_node=0;

			if(next_mesh && first_polygon!=-1)
			{
				vector<bool> selected_node_tmp=selected_node;
				sPolygon& p=tm->polygons[first_polygon];
				for(int ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<MAX_BONES;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert(node>=0 && node<selected_node.size());
							if(!selected_node_tmp[node])
								first_additional_node++;
							selected_node_tmp[node]=true;
						}
					}
				}
			}

			//Если больше нет полигонов или индексов уже 20 штук, то
			//добавляем помеченные в меш (делаем SplitMesh)
			if(node_index.size()==StaticBunch::max_index ||
				node_index.size()+first_additional_node>StaticBunch::max_index
				|| first_polygon==-1)
			{
				xassert(node_index.size()<=StaticBunch::max_index);
				TempMesh new_mesh;
				ExtractMesh(tm,new_mesh,selected_polygon,node_index);
				if(new_mesh)
					out_mesh.push_back(new_mesh);
				for(int ipolygon=0;ipolygon<tm->polygons.size();ipolygon++)
				{
					if(selected_polygon[ipolygon]==1)
						selected_polygon[ipolygon]=2;
				}
				for(int inode=0;inode<selected_node.size();inode++)
					selected_node[inode]=false;

				for(int ivg=0;ivg<tm->visible_group.size();ivg++)
				{
					cTempVisibleGroup& vg=tm->visible_group[ivg];
					int vg_out=vg.begin_polygon+vg.num_polygon;
					int mesh_out=polygon_offset+new_mesh->polygons.size();
					if(vg_out>polygon_offset &&  vg.begin_polygon<mesh_out)
					{
						cTempVisibleGroup outvg=vg;
						if(outvg.begin_polygon<polygon_offset)
						{
							int sub=polygon_offset-outvg.begin_polygon;
							outvg.begin_polygon=polygon_offset;
							outvg.num_polygon-=sub;
							xassert(sub>0 && outvg.num_polygon>0);
						}

						if(vg.begin_polygon+vg.num_polygon>polygon_offset+new_mesh->polygons.size())
						{
							int sub=(vg.begin_polygon+vg.num_polygon)-(polygon_offset+new_mesh->polygons.size());
							outvg.num_polygon-=sub;
							xassert(sub>0 && outvg.num_polygon>0);
						}

						outvg.begin_polygon-=polygon_offset;
						xassert(outvg.begin_polygon>=0 && outvg.num_polygon>0);
						xassert(outvg.begin_polygon<new_mesh->polygons.size());
						xassert(outvg.begin_polygon+outvg.num_polygon<=new_mesh->polygons.size());
						new_mesh->visible_group.push_back(outvg);
					}
				}

				{
					cTempVisibleGroup& vgf=new_mesh->visible_group.front();
					cTempVisibleGroup& vgb=new_mesh->visible_group.back();
					xassert(vgf.begin_polygon==0);
					xassert(vgb.begin_polygon+vgb.num_polygon==new_mesh->polygons.size());
				}

				min_visible_group=cur_visible_group;
				polygon_offset+=new_mesh->polygons.size();
				next_mesh=true;
			}

			if(first_polygon==-1)
				break;

			//смотрим, на какие ноды в first_polygon
			if(next_mesh)
			{
				sPolygon& p=tm->polygons[first_polygon];
				for(int ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<MAX_BONES;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert(node>=0 && node<selected_node.size());
							selected_node[node]=true;
						}
					}
				}
				next_mesh=false;
			}

			//Ищем ноду чаще всего встречающююся
			vector<float> cur_nodes(nodes.size());
			for(inode=0;inode<cur_nodes.size();inode++)
				cur_nodes[inode]=0;

			for(int ipolygon=cur_vg->begin_polygon;ipolygon<cur_vg->begin_polygon+cur_vg->num_polygon;ipolygon++)
			{
				if(selected_polygon[ipolygon])
					continue;
				sPolygon& p=tm->polygons[ipolygon];
				float sum_used=1;
				int ipnt;
				for(ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<MAX_BONES;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert("Баг со скином" && node>=0 && node<selected_node.size());
							sum_used+=10;
						}
					}
				}

				for(ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<MAX_BONES;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert(node>=0 && node<selected_node.size());
							cur_nodes[node]+=sum_used;
						}
					}
				}
			}

			float max_node_sum=-1;
			int max_node=-1;
			for(inode=0;inode<cur_nodes.size();inode++)
			{
				if(!selected_node[inode])
					if(cur_nodes[inode]>max_node_sum)
					{
						max_node_sum=cur_nodes[inode];
						max_node=inode;
					}
			}

			if(cur_nodes.size()!=1)
			{
				if(max_node>=0 && max_node_sum>0)
				{
					xassert(max_node>=0 && max_node<selected_node.size());
					selected_node[max_node]=true;
				}
			}else
			{
				xassert(max_node==-1);
			}
			//Помечаем новые треугольники
			for(int ipolygon=cur_vg->begin_polygon;ipolygon<cur_vg->begin_polygon+cur_vg->num_polygon;ipolygon++)
			{
				if(selected_polygon[ipolygon])
					continue;
				sPolygon& p=tm->polygons[ipolygon];

				bool not_in_list=false;
				for(int ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<MAX_BONES;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert(node>=0 && node<selected_node.size());
							if(!selected_node[node])
							{
								not_in_list=true;
								break;
							}
						}
					}
					if(not_in_list)break;
				}

				if(!not_in_list)
				{
					selected_polygon[ipolygon]=1;
					cur_added_polygon++;
				}
			}
		}
	}
}

void cStatic3dx::ExtractMesh(TempMesh mesh,TempMesh& out_mesh,
							 vector<char>& selected_polygon,vector<int>& node_index)
{
	//Выбираем соответствующие полигоны, и копируем их в новый mesh
	out_mesh=new cTempMesh3dx;
	out_mesh->inode_array=node_index;
	out_mesh->inode=mesh->inode;
	out_mesh->imaterial=mesh->imaterial;

	vector<int> vertex_realloc(mesh->vertex_pos.size());
	for(int vi=0;vi<vertex_realloc.size();vi++)
		vertex_realloc[vi]=-1;

	for(int ipolygon=0;ipolygon<mesh->polygons.size();ipolygon++)
	{
		sPolygon& sp=mesh->polygons[ipolygon];
		if(selected_polygon[ipolygon]!=1)
			continue;

		sPolygon new_polygon;
		for(int ip=0;ip<3;ip++)
		{
			int ivertex=sp[ip];
			int& idx=vertex_realloc[ivertex];
			if(idx<0)
			{
				idx=out_mesh->vertex_pos.size();
				out_mesh->vertex_pos.push_back(mesh->vertex_pos[ivertex]);
				out_mesh->vertex_norm.push_back(mesh->vertex_norm[ivertex]);
				out_mesh->vertex_uv.push_back(mesh->vertex_uv[ivertex]);
				if(isUV2)
					out_mesh->vertex_uv2.push_back(mesh->vertex_uv2[ivertex]);
				out_mesh->bones.push_back(mesh->bones[ivertex]);
			}

			new_polygon[ip]=idx;
		}

		out_mesh->polygons.push_back(new_polygon);
	}

	if(out_mesh->vertex_pos.empty()){
		//delete out_mesh;
		out_mesh = 0;
	}
}

void cTempMesh3dx::OptimizeMesh()
{
	for(int ivg=0;ivg<visible_group.size();ivg++)
	{
		cTempVisibleGroup& vg=visible_group[ivg];
		//OptimizeFaces
		vector<DWORD> face_remap(vg.num_polygon);
		RDCALL(D3DXOptimizeFaces((void*)&polygons[vg.begin_polygon],
			vg.num_polygon,
			vertex_pos.size(),
			false,
			&face_remap[0]
			));

			vector<sPolygon> new_polygons(vg.num_polygon);
			for(int ipolygon=0;ipolygon<vg.num_polygon;ipolygon++)
			{
				int idx=face_remap[ipolygon];
				xassert(idx>=0 && idx<vg.num_polygon);
				new_polygons[ipolygon]=polygons[idx+vg.begin_polygon];
			}

			for(int ipolygon=0;ipolygon<vg.num_polygon;ipolygon++)
			{
				polygons[ipolygon+vg.begin_polygon]=new_polygons[ipolygon];
			}
	}

	//OptimizeVertices
	int polygons_size=polygons.size();
	int vertex_num=vertex_pos.size();
	vector<DWORD> vertex_remap(vertex_num);
	RDCALL(D3DXOptimizeVertices((void*)&polygons[0],
		polygons.size(),
		vertex_pos.size(),
		false,
		&vertex_remap[0]
		));

		vector<Vect3f> new_vertex_pos(vertex_pos.size());
		vector<Vect3f> new_vertex_norm(vertex_norm.size());
		vector<Vect2f> new_vertex_uv(vertex_uv.size());
		vector<Vect2f> new_vertex_uv2(vertex_uv2.size());
		vector<cTempBone> new_bones(bones.size());

		int ivertex;
		for(ivertex=0;ivertex<vertex_num;ivertex++)
		{
			int idx=vertex_remap[ivertex];
			xassert(idx>=0 && idx<vertex_num);

			new_vertex_pos[ivertex]=vertex_pos[idx];
			new_vertex_norm[ivertex]=vertex_norm[idx];
			if(!vertex_uv.empty())
				new_vertex_uv[ivertex]=vertex_uv[idx];
			if(!vertex_uv2.empty())
				new_vertex_uv2[ivertex]=vertex_uv2[idx];
			if(!bones.empty())
				new_bones[ivertex]=bones[idx];
		}

		vertex_pos.swap(new_vertex_pos);
		vertex_norm.swap(new_vertex_norm);
		vertex_uv.swap(new_vertex_uv);
		vertex_uv2.swap(new_vertex_uv2);
		bones.swap(new_bones);

		vector<DWORD> vertex_remap_inv(vertex_num);
		for(ivertex=0;ivertex<vertex_num;ivertex++)
		{
			vertex_remap_inv[ivertex]=0;
		}

		for(ivertex=0;ivertex<vertex_num;ivertex++)
		{
			int idx=vertex_remap[ivertex];
			xassert(idx>=0 && idx<vertex_num);
			vertex_remap_inv[idx]=ivertex;
		}

		for(int ipolygon=0;ipolygon<polygons_size;ipolygon++)
		{
			sPolygon& p=polygons[ipolygon];
			for(int i=0;i<3;i++)
			{
				int idx=vertex_remap_inv[p[i]];
				xassert(idx>=0 && idx<vertex_num);
				p[i]=idx;
			}
		}
}

void cStatic3dx::CacheBuffersToSystemMem(int ilod)
{
	StaticLod& lod=lods[ilod];
	if(lod.sys_vb || !lod.ib.IsInit())
		return;
	lod.sys_vb=new cSkinVertexSysMem[lod.vb.GetNumberVertex()];
	lod.sys_ib=new sPolygon[lod.ib.GetNumberPolygon()];

	sPolygon* pPolygon = gb_RenderDevice->LockIndexBuffer(lod.ib);
	memcpy(lod.sys_ib,pPolygon,lod.ib.GetNumberPolygon()*sizeof(sPolygon));
	gb_RenderDevice->UnlockIndexBuffer(lod.ib);

	int blend_weight=lod.GetBlendWeight();
	cSkinVertex skin_vertex=GetSkinVertex(blend_weight);
	cSkinVertexSysMemI out_vertex;
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb,true);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());
	out_vertex.SetVB(lod.sys_vb);
	for(int ivertex=0;ivertex<lod.vb.GetNumberVertex();ivertex++)
	{
		skin_vertex.Select(ivertex);
		out_vertex.Select(ivertex);
		out_vertex.GetPos()=skin_vertex.GetPos();
		out_vertex.GetIndex()=skin_vertex.GetIndex();
		for(int ibone=0;ibone<blend_weight;ibone++)
			out_vertex.GetWeight(ibone)=skin_vertex.GetWeight(ibone);
	}

	gb_RenderDevice->UnlockVertexBuffer(lod.vb);
}

void cStatic3dx::GetVBSize(int& vertex_count,int& vertex_size)
{
	vertex_count=0;
	vertex_size=0;
	for(int ilod=0;ilod<lods.size();ilod++)
	{
		cStatic3dx::StaticLod& lod=lods[ilod];
		vertex_count+= lod.vb.GetNumberVertex();
		vertex_size+= lod.vb.GetVertexSize() * lod.vb.GetNumberVertex();
	}

	vertex_count+= debris.vb.GetNumberVertex();
	vertex_size+= debris.vb.GetVertexSize() * debris.vb.GetNumberVertex();
}

void cStatic3dx::StaticLod::prepareToSerialize(LodCache& chache)
{
	chache.polygonNumber = ib.GetNumberPolygon();
	chache.vertexNumber = vb.GetNumberVertex();
	chache.vertexSize = vb.GetVertexSize();

	if(vb.IsInit()){
		chache.vbBlock.alloc(vb.GetNumberVertex()*vb.GetVertexSize());
		memcpy(chache.vbBlock.buffer(), gb_RenderDevice->LockVertexBuffer(vb), chache.vbBlock.size());
		gb_RenderDevice->UnlockVertexBuffer(vb);

		chache.ibBlock.alloc(ib.GetNumberPolygon()*sizeof(sPolygon));
		memcpy(chache.ibBlock.buffer(), gb_RenderDevice->LockIndexBuffer(ib), chache.ibBlock.size());
		gb_RenderDevice->UnlockIndexBuffer(ib);
	}
}

void cStatic3dx::StaticLod::initBuffersInPlace(const LodCache& cache, cStatic3dx* object)
{
	vb = sPtrVertexBuffer();
	ib = sPtrIndexBuffer();

	if(cache.vertexNumber > 0){
		xassert(cache.vbBlock.size() == cache.vertexNumber*cache.vertexSize);
		cSkinVertex skin_vertex = object->GetSkinVertex(GetBlendWeight());
		gb_RenderDevice->CreateVertexBuffer(vb, cache.vertexNumber, skin_vertex.GetDeclaration());
		memcpy(gb_RenderDevice->LockVertexBuffer(vb), cache.vbBlock.buffer(), cache.vbBlock.size());
		gb_RenderDevice->UnlockVertexBuffer(vb);
		xassert(vb.GetVertexSize() == cache.vertexSize);

	}

	if(cache.polygonNumber > 0){
		xassert(cache.ibBlock.size() == cache.polygonNumber*sizeof(sPolygon));
		gb_RenderDevice->CreateIndexBuffer(ib, cache.polygonNumber);
		memcpy(gb_RenderDevice->LockIndexBuffer(ib), cache.ibBlock.buffer(), cache.ibBlock.size());
		gb_RenderDevice->UnlockIndexBuffer(ib);
	}
}

void cStatic3dx::serialize(Archive& ar)
{
	Static3dxBase::serialize(ar);
	
	ar.serialize(lods, "lods", "lods");
	ar.serialize(debris, "debris", "debris");
	xassert(debrises.empty());
	ar.serialize((vector<int>&)debrises, "debrises", "debrises");
	ar.serialize(voxelBox, "voxelBox", "voxelBox");
}

void cStatic3dx::constructInPlace(const char* fileName)
{
	inPlace_ = true;

	if(!is_logic){
		LodsCache* cache = 0;
		InPlaceIArchive ia(0);
		if(ia.open((string(fileName) + 'B').c_str()))
			ia.construct(cache);
		xassert(cache);

		int i = 0;
		Lods::iterator iLod;
		FOR_EACH(lods, iLod)
			iLod->initBuffersInPlace(cache->lods[i++], this);

		debris.initBuffersInPlace(cache->debris, this);

		InPlaceIArchive::destruct(cache);

		createTextures();
		CreateDebrises();
	}
}

int cStatic3dx::Release()
{ 
	if(!inPlace_)
		return UnknownClass::Release();
	else{
		xassert(GetRef() > 0);
		if(DecRef()>0) 
			return GetRef();

		releaseTextures();

		InPlaceIArchive::destruct(this);
		return 0;
	}
}


void cStatic3dx::StaticLod::serialize(Archive& ar)
{
	ar.serialize(blend_indices, "blend_indices", "blend_indices");
	ar.serialize(bunches, "skin_group", "skin_group");
}

cStatic3dx::LodCache::LodCache()
{
	polygonNumber = 0;
	vertexNumber = 0;
	vertexSize = 0;
}

void cStatic3dx::LodCache::serialize(Archive& ar)
{
	ar.serialize(polygonNumber, "polygonNumber", "polygonNumber");
	ar.serialize(vertexNumber, "vertexNumber", "vertexNumber");
	ar.serialize(vertexSize, "vertexSize", "vertexSize");

	ar.serialize(vbBlock, "vbBlock", "vbBlock");
	ar.serialize(ibBlock, "ibBlock", "ibBlock");

	vbBlock.free();
	ibBlock.free();
}

void cStatic3dx::LodsCache::serialize(Archive& ar)
{
	ar.serialize(lods, "lods", "lods");
	ar.serialize(debris, "debris", "debris");
}

void StaticBunch::serialize(Archive& ar)
{
	ar.serialize(offset_polygon, "offset_polygon", "offset_polygon");
	ar.serialize(num_polygon, "num_polygon", "num_polygon");
	ar.serialize(offset_vertex, "offset_vertex", "offset_vertex");
	ar.serialize(num_vertex, "num_vertex", "num_vertex");
	ar.serialize(imaterial, "imaterial", "imaterial");
	ar.serialize(nodeIndices, "node_index", "node_index");
	ar.serialize(visibleGroups, "visible_group", "visible_group");
}


//Может быть несколько объектов с одним именем, но разными материалами.
