#include "StdAfxRd.h"
#include "Static3dx.h"
#include "Node3dx.h"
#include "NParticle.h"
#include "VisError.h"
#include <algorithm>
#include "scene.h"

extern DebugType<int>		Option_DeleteLod;

enum eTextureMapType
{ /* texture map in 3dSMAX */
	TEXMAP_AM					=	0,   // ambient
	TEXMAP_DI					=	1,   // diffuse
	TEXMAP_SP					=	2,   // specular
	TEXMAP_SH					=	3,   // shininess
	TEXMAP_SS					=	4,   // shininess strength
	TEXMAP_SI					=	5,   // self-illumination
	TEXMAP_OP					=	6,   // opacity
	TEXMAP_FI					=	7,   // filter color
	TEXMAP_BU					=	8,   // bump 
	TEXMAP_RL					=	9,   // reflection
	TEXMAP_RR					=	10,  // refraction 
	TEXMAP_DP					=	11,  // displacement 
};

bool RenderFileReadXstream(const char *fname,char *&buf,int &size)
{
	start_timer_auto();
	buf=0; size=0;
	start_timer(21);
	XStream f(0);
	stop_timer(21);
	if(!f.open(fname, XS_IN))
		return false; 

	size=f.size();
	buf=new char[size];
	int read_size=f.read(buf,size);
	f.close();
	if(read_size!=size)
	{
		delete buf;
		buf=0;
		size=0;
		return false;
	}
	return true;
}

bool RenderFileReadXZipStream(const char *fname,char *&buf,int &size)
{
	start_timer_auto();
	buf=0; size=0;
	start_timer(1);
	XZipStream f(0);
	stop_timer(1);
	if(!f.open(fname, XS_IN))
		return false; 

	size=f.size();
	buf=new char[size];
	int read_size=f.read(buf,size);
	f.close();
	if(read_size!=size)
	{
		delete buf;
		buf=0;
		size=0;
		return false;
	}
	return true;
}

class ChainConverter{
public:
	ChainConverter(StaticChainsBlock& chains_block);

    template<int Size>
    void loadInterpolator(Interpolator3dx<Size>& self, CLoadIterator ld);
    void loadInterpolator(Interpolator3dxBool& self, CLoadIterator ld);

    template<class InterpolatorType>
	void fixUp(InterpolatorType& interpolator);

    void mergeDown();

	StaticChainsBlock& chainsBlock_;

    int add(const Interpolator3dxPosition::darray& array){ return add(positions_, array); }
    int add(const Interpolator3dxRotation::darray& array){ return add(rotations_, array); }
    int add(const Interpolator3dxScale::darray& array)   { return add(scales_, array); }
    int add(const Interpolator3dxBool::darray& array)    { return add(bools_, array); }
    int add(const Interpolator3dxUV::darray& array)      { return add(uvs_, array); }
private:
	template<class Arrays, class Array>
	int add(Arrays& arrays, const Array& array){
		int result = 0;
		Arrays::iterator it;
		for(it = arrays.begin(); it != arrays.end(); ++it)
			result += it->size();
		arrays.push_back(array);
		return result;
	}

	typedef std::list<Interpolator3dxPosition::darray> Positions;
    typedef std::list<Interpolator3dxRotation::darray> Rotations;
    typedef std::list<Interpolator3dxScale::darray>    Scales;
    typedef std::list<Interpolator3dxBool::darray>     Bools;
    typedef std::list<Interpolator3dxUV::darray>       UVs;

	Positions positions_;
	Rotations rotations_;
	Scales    scales_;
	Bools     bools_;
	UVs       uvs_;
}; 

cStaticVisibilityChainGroup::cStaticVisibilityChainGroup()
{
	visible_shift=0;
	lod=-1;
	is_invisible_list=false;
}

cStatic3dx::cStatic3dx(bool is_logic_,const char* file_name_)
:file_name(file_name_)
{
	is_inialized_bound_box=false;
	bound_box.min.set(0,0,0);
	bound_box.max.set(0,0,0);
	radius=0;

	is_logic=is_logic_;
	is_lod=false;
	is_old_model=false;

	circle_shadow_enable=circle_shadow_enable_min=c3dx::OST_SHADOW_REAL;
	circle_shadow_height=-1;
	circle_shadow_radius=10;

	bump=false;
	is_uv2=false;
	loaded = false;

}

cStatic3dx::~cStatic3dx()
{
//	if(!is_logic)dprintf("D %s\n",file_name.c_str());
	for(int i=0;i<lods.size();i++)
	{
		ONE_LOD& l=lods[i];
		delete[] l.sys_vb;
		delete[] l.sys_ib;
	}

	vector<cStaticVisibilitySet*>::iterator its;
	FOR_EACH(visible_sets,its)
	{
		cStaticVisibilitySet* p=*its;
		delete p;
	}
	visible_sets.clear();

	vector<cStaticLights>::iterator itlight;
	FOR_EACH(lights,itlight)
	{
		cStaticLights& light=*itlight;
		RELEASE(light.pTexture);
	}
	lights.clear();

	vector<cStaticSimply3dx*>::iterator iss;
	FOR_EACH(debrises,iss)
	{
		RELEASE((*iss));
	}
	debrises.clear();
}

void cStatic3dx::GetTextureNames(vector<string>& names) const
{
	StaticMap<string, int> txt;
	StaticMap<string, int>::iterator it;

#define ADD(x) if(!x.empty())txt[x]=1
	for(int i = 0; i < materials.size(); ++i){
		const cStaticMaterial& sm = materials[i];
		ADD(sm.tex_diffuse);
		ADD(sm.tex_bump);
		ADD(sm.tex_reflect);
		ADD(sm.tex_specularmap);
		ADD(sm.tex_self_illumination);
		ADD(sm.tex_secondopacity);
		ADD(sm.tex_skin);
	}

	for(vector<cStaticLights>::const_iterator itl=lights.begin();itl!=lights.end();++itl)
	{
		const cStaticLights& s=*itl;
		if(s.pTexture)
		{
			string name=s.pTexture->GetName();
			ADD(name);
		}
	}

#undef ADD
	FOR_EACH(txt, it){
		const string& s = it->first;
		names.push_back(s);
	}
}


bool cStatic3dx::Load(CLoadDirectory& dir)
{
	start_timer_auto();

	bool loaded=false;
	bool is_new_model=false;
	IF_FIND_DIR(C3DX_ADDITIONAL_LOD1)
		LoadLod(dir,temp_mesh_lod1);
	IF_FIND_DIR(C3DX_ADDITIONAL_LOD2)
		LoadLod(dir,temp_mesh_lod2);

	dir.rewind();
	while(CLoadData* ld=dir.next())
	{
		if(ld->id==C3DX_NON_DELETE_NODE)
		{
			is_new_model=true;
		}

		if((is_logic && ld->id==C3DX_LOGIC_OBJECT) || 
			(!is_logic && ld->id==C3DX_GRAPH_OBJECT)
			)
		{
			CLoadDirectory ldir(ld);
			LoadInternal(ldir);
			loaded=true;
		}
		if(ld->id==C3DX_LOGOS)
		{
			logos.Load(ld);
		}

	}

	is_old_model=!is_new_model;

	if(!loaded)
	{
		if(is_logic)
		{
			//errlog()<<"Cannot load logic object"<<VERR_END;
			return false;
		}else
		{
			dir.rewind();
			LoadInternal(dir);//Поддержка старого формата, потом стереть.
		}
	}

	for(int i=0;i<temp_mesh_lod1.size();i++)
		delete temp_mesh_lod1[i];
	temp_mesh_lod1.clear();
	for(int i=0;i<temp_mesh_lod2.size();i++)
		delete temp_mesh_lod2[i];
	temp_mesh_lod2.clear();

	return true;
}

void cStatic3dx::LoadInternal(CLoadDirectory& rd)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(file_name.c_str(),drive,dir,fname,ext);
	string path_name=drive;
	path_name+=dir;
	path_name+="TEXTURES\\";

	vector<cTempMesh3dx*> temp_mesh;
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_HEAD:
		LoadChainData(ld);
		break;
	case C3DX_STATIC_CHAINS_BLOCK:
		chains_block.load(ld);
		break;
	case C3DX_NODES:
		LoadNodes(ld, chains_block);
		break;
	case C3DX_CAMERA:
		LoadCamera(ld);
		break;
	case C3DX_MESHES:
		LoadMeshes(ld,temp_mesh);
		break;
	case C3DX_LIGHTS:
		LoadLights(ld,path_name.c_str(), chains_block);
		break;
	case C3DX_MATERIAL_GROUP:
		{
			int mat_num=LoadMaterialsNum(ld);
			LoadMaterials(ld,path_name.c_str(),mat_num);
		}
		break;
	case C3DX_BASEMENT:
		basement.Load(ld);
		break;
	case C3DX_LOGIC_BOUND:
		logic_bound.Load(ld);
		break;
	case C3DX_LOGIC_BOUND_LOCAL:
		LoadLocalLogicBound(ld);
		break;
	}

	if(chains_block.isEmpty()){
		chains_block.converter().mergeDown();
		fixUpNodes(chains_block);
		fixUpMaterials(chains_block);
		fixUpLights(chains_block);
	}

	int iag=0;

	for(int iag=0;iag<animation_group.size();iag++)
	{
		AnimationGroup& ag=animation_group[iag];
		ag.nodes.clear();
		int i;
		for(i=0;i<ag.temp_nodes_name.size();i++)
		{
			string& name=ag.temp_nodes_name[i];
			if(name=="_base_")
			{
				continue;
			}

			int inode;
			for(inode=0;inode<nodes.size();inode++)
			{
				cStaticNode& s=nodes[inode];
				if(name==s.name)
					break;
			}

			if(inode>=nodes.size())
				inode=0;
			
			ag.nodes.push_back(inode);
		}

		ag.temp_nodes_name.clear();

		for(i=0;i<ag.temp_materials_name.size();i++)
		{
			string& name=ag.temp_materials_name[i];
			for(int imat=0;imat<materials.size();imat++)
			{
				cStaticMaterial& sm=materials[imat];
				if(name==sm.name)
					sm.animation_group_index=iag;
			}
		}
		ag.temp_materials_name.clear();
	}

	ParseEffect();


	vector<cStaticVisibilitySet*>::iterator its;
	FOR_EACH(visible_sets,its)
	{
		if((*its)->BuildVisibilityGroupsLod())
			is_lod=true;
	}

	if(!is_logic)
		BuildMeshes(temp_mesh);;
	CreateDebrises();
	//ClearDebrisNodes();
}

void cStatic3dx::CreateDebrises()
{
	if(is_logic)
		return;
	start_timer_auto();
	if(visible_sets.empty())
		return;

	cStaticVisibilitySet* vs = visible_sets[0];
	C3dxVisibilityGroup vgi = vs->GetVisibilityGroupIndex("debris");
	if (vgi==C3dxVisibilityGroup::BAD)
		return;
	cStaticVisibilityChainGroup* vg =  vs->visibility_groups[0][vgi.igroup];
	for(int i=0; i<vg->visible_nodes.size();i++)
	{
		if(vg->visible_nodes[i])
		{
			cStaticSimply3dx* obj = new cStaticSimply3dx();
			if(!obj->BuildFromNode(this,i,vgi))
			{
				RELEASE(obj);
				continue;
			}
			obj->isDebris = true;
			debrises.push_back(obj);
		}
	}

}

void cStatic3dx::ClearDebrisNodes()
{
	if(is_logic)
		return;
	vector<int> debrisNodes;
	cStaticVisibilitySet* vs = visible_sets[0];
	C3dxVisibilityGroup vgi = vs->GetVisibilityGroupIndex("debris");
	if(vgi==C3dxVisibilityGroup::BAD)
		return;
	cStaticVisibilityChainGroup* vg =  vs->visibility_groups[0][vgi.igroup];
	for(int i=0; i<vg->visible_nodes.size();i++)
	{
		if(vg->visible_nodes[i])
		{
			debrisNodes.push_back(i);
		}
	}
	vector<cStaticNode> newNodes;
	vector<int> oldToNewNodes;
	oldToNewNodes.resize(nodes.size());
	newNodes.resize(nodes.size()-debrisNodes.size());
	int newNodeIndex=0;
	for(int i=0; i<oldToNewNodes.size(); i++)
	{
		bool found=false;
		for(int j=0; j<debrisNodes.size(); j++)
		{
			if(i==debrisNodes[j])
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			oldToNewNodes[i] = newNodeIndex++;
		}else
		{
			oldToNewNodes[i]=-1;
		}
	}
	for(int i=0; i<nodes.size(); i++)
	{
		if(oldToNewNodes[i]<0)
			continue;
		cStaticNode node = nodes[i];
		node.inode = oldToNewNodes[i];
		if(node.iparent>=0)
			node.iparent = oldToNewNodes[node.iparent];
		newNodes[node.inode] = node;
	}
	//nodes.resize(newNodes.size());
	//nodes = newNodes;
}

void cStatic3dx::LoadChainData(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_GROUPS:
		LoadGroup(ld);
		break;
	case C3DX_ANIMATION_CHAIN:
		LoadChain(ld);
		break;
	case C3DX_ANIMATION_CHAIN_GROUP:
		LoadChainGroup(ld);
		break;
	case C3DX_ANIMATION_VISIBLE_SETS:
		LoadVisibleSets(ld);
		break;
	}
}

void cStatic3dx::LoadGroup(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ANIMATION_GROUP:
		{
			AnimationGroup ag;
			ag.Load(ld);
			animation_group.push_back(ag);
		}
		break;
	}
}
void AnimationGroup::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AG_HEAD:
		{
			CLoadIterator it(ld);
			it>>name;
		}
		break;
	case C3DX_AG_LINK:
		{
			CLoadIterator it(ld);
			it>>temp_nodes_name;
		}
		break;
	case C3DX_AG_LINK_MATERIAL:
		{
			CLoadIterator it(ld);
			it>>temp_materials_name;
		}
		break;
	}
}

StaticChainsBlock::StaticChainsBlock()
: block_(0)
, block_size_(0)

, positions_(0)
, rotations_(0)
, scales_(0)
, bools_(0)
, uvs_(0)

, num_positions_(0)
, num_rotations_(0)
, num_scales_(0)
, num_bools_(0)
, num_uvs_(0)
{
	converter_=new ChainConverter(*this);
}

ChainConverter& StaticChainsBlock::converter()
{
	return *converter_;
}

void StaticChainsBlock::free()
{
    if(block_){
        ::free(block_);
        block_ = 0;
        block_size_ = 0;

		positions_ = 0;
		rotations_ = 0;
        scales_ = 0;
		bools_ = 0;
		uvs_ = 0;

        num_positions_ = num_rotations_ = num_scales_ = num_bools_ = num_uvs_ = 0;
    }
}

StaticChainsBlock::~StaticChainsBlock()
{
	delete converter_;
    free();
}

void StaticChainsBlock::load(CLoadData* load_data)
{
	CLoadIterator it(load_data);
	free();

	num_scales_ = num_positions_ = num_rotations_ = num_bools_ = num_uvs_ = 0;

	it >> num_positions_;
	it >> num_rotations_;
	it >> num_scales_;
	it >> num_bools_;
    it >> num_uvs_;

	block_size_ = num_positions_ * sizeof(sInterpolate3dxPosition) + 
				  num_rotations_ * sizeof(sInterpolate3dxRotation) +
				  num_scales_    * sizeof(sInterpolate3dxScale) +
				  num_bools_     * sizeof(sInterpolate3dxBool) +
                  num_uvs_       * sizeof(sInterpolate3dxUV);

	assert(load_data->size - 5 * (sizeof(int)) == (block_size_));
	block_ = ::malloc(block_size_);

	assert(block_ != 0 && "Unable to allocate memory for StaticChainsBlock");

	positions_  = reinterpret_cast<sInterpolate3dxPosition*>(block_);
	rotations_	= reinterpret_cast<sInterpolate3dxRotation*>(positions_ + num_positions_);
	scales_     = reinterpret_cast<sInterpolate3dxScale*>   (rotations_ + num_rotations_);
	bools_		= reinterpret_cast<sInterpolate3dxBool*>    (scales_ + num_scales_);
	uvs_		= reinterpret_cast<sInterpolate3dxUV*>      (bools_ + num_bools_);
	
	it.read(block_, block_size_);
	assert(reinterpret_cast<unsigned char*>(uvs_ + num_uvs_) == (reinterpret_cast<unsigned char*>(block_) + block_size_));
}

void StaticChainsBlock::save(Saver& saver) const
{
    saver << num_positions_;
	saver << num_rotations_;
	saver << num_scales_;
	saver << num_bools_;
	saver << num_uvs_;

    saver.write(block_, block_size_);
}


void cStatic3dx::LoadChain(CLoadDirectory rd)
{
	animation_chain.clear();
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AC_ONE:
		{
			int begin_frame,end_frame;
			CLoadIterator li(ld);
			cAnimationChain ac;
			li>>ac.name;
			li>>begin_frame;
			li>>end_frame;
			li>>ac.time;
			animation_chain.push_back(ac);
		}
		break;
	}
}

void cStatic3dx::LoadChainGroup(CLoadDirectory rd)
{
	xassert(visible_sets.empty());
	cStaticVisibilitySet* pset=new cStaticVisibilitySet;
	pset->name="default";
	pset->is_all_mesh_in_set=true;
	visible_sets.push_back(pset);
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_ACG_ONE:
		{
			CLoadIterator li(ld);
			cStaticVisibilityChainGroup* p=new cStaticVisibilityChainGroup;
			li>>p->name;
			li>>p->temp_visible_object;
			p->is_invisible_list=true;

			pset->raw_visibility_groups.push_back(p);
		}
		break;
	}

	pset->DummyVisibilityGroup();
}

void cStatic3dx::LoadVisibleSets(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AVS_ONE:
		LoadVisibleSet(ld);
		break;
	}

}

void cStatic3dx::LoadVisibleSet(CLoadDirectory rd)
{
	cStaticVisibilitySet* pset=new cStaticVisibilitySet;
	visible_sets.push_back(pset);
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_AVS_ONE_NAME:
		{
			CLoadIterator li(ld);
			li>>pset->name;
		}
		break;
	case C3DX_AVS_ONE_OBJECTS:
		{
			CLoadIterator li(ld);
			li>>pset->mesh_in_set;
		}
		break;
	case C3DX_AVS_ONE_VISIBLE_GROUPS:
		{
			CLoadDirectory rdd(ld);
			while(CLoadData* ldd=rdd.next())
			switch(ldd->id)
			{
			case C3DX_AVS_ONE_VISIBLE_GROUP:
				{
					CLoadIterator li(ldd);
					cStaticVisibilityChainGroup* p=new cStaticVisibilityChainGroup;

					li >> p->name;
					li >> p->temp_visible_object;

					pset->raw_visibility_groups.push_back(p);
				}
				break;
			}
		}
	}

	pset->DummyVisibilityGroup();
}


ChainConverter::ChainConverter(StaticChainsBlock& chains_block)
: chainsBlock_(chains_block)
{

}

namespace {
template<class ArrayType, class DArrayType>
void copyChains(ArrayType* dest, DArrayType& src)
{
    int i = 0;
    DArrayType::iterator it;
    DArrayType::value_type::iterator jt;

    FOR_EACH(src, it)
        FOR_EACH(*it, jt)
            *(dest + i++) = *jt;
}

template<class DArrayType>
int totalSize(DArrayType& array){
    int size = 0;
    DArrayType::iterator it;
    FOR_EACH(array, it)
      size += it->size();
    return size;
}

}

void ChainConverter::mergeDown()
{
    chainsBlock_.free();

    chainsBlock_.num_positions_ = totalSize(positions_);
    chainsBlock_.num_rotations_ = totalSize(rotations_);
    chainsBlock_.num_scales_    = totalSize(scales_);
    chainsBlock_.num_bools_     = totalSize(bools_);
    chainsBlock_.num_uvs_       = totalSize(uvs_);

    chainsBlock_.block_size_ =
      chainsBlock_.num_positions_ * sizeof(sInterpolate3dxPosition) + 
      chainsBlock_.num_rotations_ * sizeof(sInterpolate3dxRotation) +
      chainsBlock_.num_scales_    * sizeof(sInterpolate3dxScale) +
      chainsBlock_.num_bools_     * sizeof(sInterpolate3dxBool) +
      chainsBlock_.num_uvs_       * sizeof(sInterpolate3dxUV);

    chainsBlock_.block_ = ::malloc(chainsBlock_.block_size_);

    assert(chainsBlock_.block_ != 0 && "Unable to allocate memory for StaticChainsBlock");

    chainsBlock_.positions_  = reinterpret_cast<sInterpolate3dxPosition*>(chainsBlock_.block_);
    chainsBlock_.rotations_	 = reinterpret_cast<sInterpolate3dxRotation*>(chainsBlock_.positions_ + chainsBlock_.num_positions_);
    chainsBlock_.scales_     = reinterpret_cast<sInterpolate3dxScale*>   (chainsBlock_.rotations_ + chainsBlock_.num_rotations_);
    chainsBlock_.bools_		 = reinterpret_cast<sInterpolate3dxBool*>    (chainsBlock_.scales_    + chainsBlock_.num_scales_);
    chainsBlock_.uvs_		 = reinterpret_cast<sInterpolate3dxUV*>      (chainsBlock_.bools_     + chainsBlock_.num_bools_);

    assert(reinterpret_cast<unsigned char*>(chainsBlock_.uvs_ + chainsBlock_.num_uvs_)
		   == (reinterpret_cast<unsigned char*>(chainsBlock_.block_) + chainsBlock_.block_size_));

    copyChains(chainsBlock_.positions_, positions_);
    copyChains(chainsBlock_.rotations_, rotations_);
    copyChains(chainsBlock_.scales_,    scales_);
    copyChains(chainsBlock_.bools_,     bools_);
    copyChains(chainsBlock_.uvs_,       uvs_);

    positions_.clear();
    rotations_.clear();
    scales_.clear();
    bools_.clear();
    uvs_.clear();
}

template<class InterpolatorType>
void ChainConverter::fixUp(InterpolatorType& self)
{
    if(self.values.isJustIndex()){
        int index = self.values.index();
        int count = self.values.size();
        chainsBlock_.setValues(self, index, count);
    }
}

template<int Size>
void ChainConverter::loadInterpolator(Interpolator3dx<Size>& self, CLoadIterator ld)
{
    Interpolator3dx<Size>::darray curve;

	int size=0;
	ld>>size;
	xassert(size>0 && size<255);
	curve.resize(size);
	for(int i=0;i<size;i++)
	{
		sInterpolate3dx<Size>& one=curve[i];
		int type=0;
		ld>>type;
		one.type=(ITPL)type;
		float time_size=0;
		ld>>one.tbegin;
		ld>>time_size;
		xassert(time_size>1e-5f);
		one.inv_tsize=1/time_size;

		int one_size=0;
		switch(type)
		{
		case ITPL_CONSTANT:
			one_size=1;
			break;
		case ITPL_LINEAR:
			one_size=2;
			break;
		case ITPL_SPLINE:
			one_size=4;
			break;
		default:
			assert(0);
		}

		for(int j=0;j<one_size*Size;j++)
			ld>>one.a[j];
	}
    int index = add(curve);
    self.values.setIndex(index, size);
    
    //self.values = BlockPointer<sInterpolate3dx<Size> >(curve);
}

void ChainConverter::loadInterpolator(Interpolator3dxBool& self, CLoadIterator ld)
{
    Interpolator3dxBool::darray curve;

	int size=0;
	ld>>size;
	xassert(size>0 && size<255);
	curve.resize(size);
	for(int i=0;i<size;i++){
		sInterpolate3dxBool& one=curve[i];
		ld>>one.tbegin;
		float tsize=0;
		ld>>tsize;
		xassert(tsize>1e-5f);
		one.inv_tsize=1/tsize;

		bool value;
		ld>>value;
		one.value = value;
	}
    int index = add(curve);
    self.values.setIndex(index, size);
	//self.values = BlockPointer<sInterpolate3dxBool>(curve);
}
void cStatic3dx::LoadCamera(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_CAMERA_HEAD:
			{
				CLoadIterator it(ld);
				it>>camera_params.camera_node_num;
				it>>camera_params.fov;
			}
			break;
	}
}

void cStatic3dx::LoadNodes(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_NODE:
		{
			nodes.push_back(cStaticNode());
			cStaticNode& node=nodes.back();
			node.LoadNode(ld, chains_block);
			if(nodes.size()>1)
				xassert(node.iparent>=0);
		}
		break;
	}
}
void cStaticNode::LoadNode(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	inv_begin_pos=MatXf::ID;
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_NODE_HEAD:
		{
			CLoadIterator it(ld);
			it>>name;
			it>>inode;
			it>>iparent;
			xassert(iparent<inode);
		}
		break;
	case C3DX_NODE_CHAIN:
		LoadNodeChain(ld, chains_block);
		break;
	case C3DX_NODE_INIT_MATRIX:
		{
			CLoadIterator it(ld);
			it>>inv_begin_pos;
			inv_begin_pos.Invert();
		}
		break;
	}
	
}


void cStaticNode::LoadNodeChain(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	chains.push_back(cStaticNodeChain());
	cStaticNodeChain& chain=chains.back();

	int index = -1;

	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_NODE_CHAIN_HEAD:
		break;

	// CONVERSION:
	case C3DX_NODE_POSITION:
		chains_block.converter().loadInterpolator(chain.position, ld);
		break;
	case C3DX_NODE_ROTATION:
		chains_block.converter().loadInterpolator(chain.rotation, ld);
		break;
	case C3DX_NODE_SCALE:
		chains_block.converter().loadInterpolator(chain.scale, ld);
		break;
	case C3DX_NODE_VISIBILITY:
		chains_block.converter().loadInterpolator(chain.visibility, ld);
		break;
	// ^^^ CONVERSION

	case C3DX_NODE_POSITION_INDEX:
		chain.position.LoadIndex(ld, chains_block);
		break;
	case C3DX_NODE_ROTATION_INDEX:
		chain.rotation.LoadIndex(ld, chains_block);
		break;
	case C3DX_NODE_SCALE_INDEX:
		chain.scale.LoadIndex(ld, chains_block);
		break;
	case C3DX_NODE_VISIBILITY_INDEX:
		chain.visibility.LoadIndex(ld, chains_block);
		break;

	}

//	dprintf("chain_size=%i %i %i %i\n",chain.position.data.size(),chain.rotation.data.size(),chain.scale.data.size(),chain.visibility.data.size());
}
void cStaticNode::SaveNode(Saver &s, StaticChainsBlock& chain_block)
{
	s.push(C3DX_NODE_HEAD);
	s<<name;
	s<<inode;
	s<<iparent;
	s.pop();

	s.push(C3DX_NODE_INIT_MATRIX);
	inv_begin_pos.Invert();
	s<<inv_begin_pos;
	inv_begin_pos.Invert();
	s.pop();

	for (int i=0; i<chains.size(); i++) 
	{
		s.push(C3DX_NODE_CHAIN);
		SaveNodeChain(s,chains[i], chain_block);
		s.pop();
	}

}
void cStaticNode::SaveNodeChain(Saver& s, cStaticNodeChain& chain, StaticChainsBlock& chains_block)
{
	if(chain.position.values.size() > 0){
		s.push(C3DX_NODE_POSITION_INDEX);
        chain.position.SaveIndex(s, chains_block);
		s.pop();
	}
	if(chain.rotation.values.size() > 0){
		s.push(C3DX_NODE_ROTATION_INDEX);
        chain.rotation.SaveIndex(s, chains_block);
		s.pop();
	}
	if(chain.scale.values.size() > 0){
		s.push(C3DX_NODE_SCALE_INDEX);
        chain.scale.SaveIndex(s, chains_block);
		s.pop();
	}
	if(chain.visibility.values.size() > 0){
		s.push(C3DX_NODE_VISIBILITY_INDEX);
        chain.visibility.SaveIndex(s, chains_block);
		s.pop();
	}
	/*
	if (chain.position.data_.size()>0){
		s.push(C3DX_NODE_POSITION);
		chain.position.Save(s);
		s.pop();
	}
	if (chain.rotation.data_.size()>0)
    {
		s.push(C3DX_NODE_ROTATION);
		chain.rotation.Save(s);
		s.pop();
	}
	if (chain.scale.data_.size()>0)
    {
		s.push(C3DX_NODE_SCALE);
		chain.scale.Save(s);
		s.pop();
	}
	if (chain.visibility.data_.size()>0)
    {
		s.push(C3DX_NODE_VISIBILITY);
		chain.visibility.Save(s);
		s.pop();
	}
	*/
}

struct SortMyMaterial
{
	cStatic3dx* data;
	SortMyMaterial(cStatic3dx* data_)
	{
		data=data_;
	}

	bool operator()(const cTempMesh3dx* p1,const cTempMesh3dx* p2) const
	{
		cStaticMaterial& s1=data->materials[p1->imaterial];
		cStaticMaterial& s2=data->materials[p2->imaterial];
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
		if(vg1.visible_set<vg2.visible_set)
			return true;
		if(vg1.visible_set>vg2.visible_set)
			return false;

		return vg1.visible<vg2.visible;
	}
};

struct SortMyNodeIndex
{
	cStatic3dx* data;
	SortMyNodeIndex(cStatic3dx* data_)
	{
		data=data_;
	}

	bool operator()(const cTempMesh3dx* p1,const cTempMesh3dx* p2) const
	{
		cStaticMaterial& s1=data->materials[p1->imaterial];
		cStaticMaterial& s2=data->materials[p2->imaterial];
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
		if(vg1.visible_set<vg2.visible_set)
			return true;
		if(vg1.visible_set>vg2.visible_set)
			return false;

		return vg1.visible<vg2.visible;
	}
};

void cStaticVisibilitySet::DummyVisibilityGroup()
{
	if(raw_visibility_groups.empty())
	{
		cStaticVisibilityChainGroup* cg=new cStaticVisibilityChainGroup;
		cg->name="default";

		if(is_all_mesh_in_set)
			cg->is_invisible_list=true;
		else
			cg->temp_visible_object=mesh_in_set;

		raw_visibility_groups.push_back(cg);
	}
}

void cStatic3dx::DummyVisibilitySet(vector<cTempMesh3dx*>& temp_mesh_names)
{
	if(visible_sets.empty())
	{
		cStaticVisibilitySet* pset=new cStaticVisibilitySet;
		pset->name="default";

		pset->mesh_in_set.resize(temp_mesh_names.size());
		for(int i=0;i<temp_mesh_names.size();i++)
		{
			pset->mesh_in_set[i]=temp_mesh_names[i]->name;
		}

		
		pset->DummyVisibilityGroup();
		visible_sets.push_back(pset);
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
		for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
		{
			if(p.weight[ibone]>=1/255.0f)
				cur_bones++;
		}
		max_nonzero_bones=max(max_nonzero_bones,cur_bones);
	}

	return max_nonzero_bones;
}

void cStatic3dx::LoadMeshes(CLoadDirectory rd,vector<cTempMesh3dx*>& temp_mesh)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MESH:
		{
			cTempMesh3dx* mesh=new cTempMesh3dx;
			mesh->Load(ld);
			if(!mesh->vertex_uv2.empty())
				is_uv2=true;

			if(mesh->name.empty())
			{
				int inode=mesh->inode;
				mesh->name=nodes[inode].name;
			}

			temp_mesh.push_back(mesh);
		}
		break;
	}

	DummyVisibilitySet(temp_mesh);

	{
		bool is_lod=false;
		for(int ivs=0;ivs<visible_sets.size();ivs++)
		{
			cStaticVisibilitySet* pvs=visible_sets[ivs];
			if(pvs->IsLod())
			{
				is_lod=true;
				break;
			}
		}

		if(!is_lod && !temp_mesh_lod1.empty() && !temp_mesh_lod2.empty())
			BuildProgramLod(temp_mesh);
	}
}

void cStatic3dx::CalculateVisibleShift()
{
	for(int iset=0;iset<visible_sets.size();iset++)
	{
		cStaticVisibilitySet* set=visible_sets[iset];
		if(set->IsLod())
		{
			for(int ilod=0;ilod<cStaticVisibilitySet::num_lod;ilod++)
			{
				vector<cStaticVisibilityChainGroup*>& vgroup=set->visibility_groups[ilod];
				xassert(vgroup.size()<=32);
				for(int igroup=0;igroup<vgroup.size();igroup++)
				{
					cStaticVisibilityChainGroup* group=vgroup[igroup];
					group->visible_shift=1<<igroup;
				}
			}
		}else
		{
			xassert(set->raw_visibility_groups.size()<=32);
			for(int igroup=0;igroup<set->raw_visibility_groups.size();igroup++)
			{
				cStaticVisibilityChainGroup* group=set->raw_visibility_groups[igroup];
				group->visible_shift=1<<igroup;
			}
		}
	}
}

void cStatic3dx::BuildMeshes(vector<cTempMesh3dx*>& temp_mesh)
{
	CalculateVisibleShift();
	if(is_lod)
	{
		lods.resize(cStaticVisibilitySet::num_lod);
		for(int ilod=0;ilod<lods.size();ilod++)
		{
			BuildMeshesLod(temp_mesh,ilod,false);
		}
	}else
	{
		lods.resize(1);
		BuildMeshesLod(temp_mesh,0,false);
	}

	BuildMeshesLod(temp_mesh,0,true);

	for(int imesh=0;imesh<temp_mesh.size();imesh++)
	{
		delete temp_mesh[imesh];
	}

	//Ноды относятся ко всем, а меши - к определенным группам видимости.
	//Вместо temp_invisible_object, нужно temp_visible_object.
	for(int iset=0;iset<visible_sets.size();iset++)
	{
		cStaticVisibilitySet* pset=visible_sets[iset];
		for(int ivis=0;ivis<pset->raw_visibility_groups.size();ivis++)
		{
			cStaticVisibilityChainGroup* cur=pset->raw_visibility_groups[ivis];

			xassert(cur->visible_nodes.empty());
			cur->visible_nodes.resize(nodes.size());
			for(int inode=0;inode<nodes.size();inode++)
			{
				string& node_name=nodes[inode].name;
				vector<string>::iterator itt;
				bool found=false;
				FOR_EACH(cur->temp_visible_object,itt)
				{
					if(node_name==*itt)
					{
						found=true;
						break;
					}
				}

				if(cur->is_invisible_list)
					found=!found;

				cur->visible_nodes[inode]=found;
			}
		}
	}

	for(int iset=0;iset<visible_sets.size();iset++)
	{
		cStaticVisibilitySet* pset=visible_sets[iset];
		for(int ivis=0;ivis<pset->raw_visibility_groups.size();ivis++)
		{
			cStaticVisibilityChainGroup* cur=pset->raw_visibility_groups[ivis];
			cur->temp_visible_object.clear();
		}
	}
}

void cStatic3dx::BuildMeshesLod(vector<cTempMesh3dx*>& temp_mesh_in,int ilod,bool debris)
{
	vector<cTempMesh3dx*> temp_mesh;

	for(vector<cTempMesh3dx*>::iterator itm=temp_mesh_in.begin();itm!=temp_mesh_in.end();++itm)
	{
		cTempMesh3dx* mesh=*itm;
		int visible_set;
		DWORD visible,lod_mask;
		bool is_debris,is_no_debris;
		GetVisibleMesh(mesh->name,visible_set,visible,lod_mask,is_debris,is_no_debris);
		mesh->FillVisibleGroup(visible_set,visible);

		if(debris?!is_debris:!is_no_debris)
			continue;
/*
	Нужно ещё 
		читать/писать кэш
		строить debrises и simply 3dx не создавая vb под эти данные.
*/

		if(lod_mask==0 || (lod_mask&(1<<ilod)) )
		{
			temp_mesh.push_back(*itm);
		}
	}

	if(debris)
		BuildMeshes(temp_mesh,this->debris,false);
	else
		BuildMeshes(temp_mesh,lods[ilod],true);
}

void cStatic3dx::BuildMeshes(vector<cTempMesh3dx*>& temp_mesh,ONE_LOD& lod,bool merge_mesh)
{
	if(temp_mesh.empty())
		return;

	if(merge_mesh)
		sort(temp_mesh.begin(),temp_mesh.end(),SortMyMaterial(this));
	else
		sort(temp_mesh.begin(),temp_mesh.end(),SortMyNodeIndex(this));
	///////////////////////

	vector<cTempMesh3dx*> material_mesh;
	MergeMaterialMesh(temp_mesh,material_mesh,merge_mesh);
	BuildBuffers(material_mesh,lod);

	for(int imat=0;imat<material_mesh.size();imat++)
	{
		delete material_mesh[imat];
	}
}

void cTempMesh3dx::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MESH_HEAD:
		{
			CLoadIterator it(ld);
			inode=-1;
			it>>inode;
			imaterial=-1;
			it>>imaterial;
			xassert(imaterial>=0);
			if(imaterial<0)
				imaterial=0;
		}
		break;
	case C3DX_MESH_NAME:
		{
			CLoadIterator it(ld);
			it>>name;
		}
		break;
	case C3DX_MESH_VERTEX:
		{
			CLoadIterator it(ld);
			it>>vertex_pos;
		}
		break;
	case C3DX_MESH_NORMAL:
		{
			CLoadIterator it(ld);
			it>>vertex_norm;
		}
		break;
	case C3DX_MESH_UV:
		{
			CLoadIterator it(ld);
			it>>vertex_uv;
		}
		break;
	case C3DX_MESH_UV2:
		{
			CLoadIterator it(ld);
			it>>vertex_uv2;
		}
		break;
	case C3DX_MESH_FACES:
		{
			CLoadIterator it(ld);
			it>>polygons;
		}
		break;
	case C3DX_MESH_SKIN:
//		break;
		{
			CLoadIterator it(ld);
			int size=0;
			it>>size;
			typedef StaticMap<int,int> BonesInode;
			BonesInode bones_inode;
			bones.resize(size);

			for(int i=0;i<size;i++)
			{
				cTempBone& p=bones[i];
				float sum=0;	
				int ibone;
				for(ibone=0;ibone<cTempBone::max_bones;ibone++)
				{
					it>>p.weight[ibone];
					it>>p.inode[ibone];
/*
					if(ibone==0)
					{
						p.weight[ibone]=1;
						p.inode[ibone]=0;
					}else
					{
						p.weight[ibone]=0;
						p.inode[ibone]=0;
					}
*/
					if(p.weight[ibone]<0)
						p.weight[ibone]=0;
					if(p.weight[ibone]>=1)
						p.weight[ibone]=1;
					float w=p.weight[ibone];
					xassert(w>=0 && w<=1.0f);
					sum+=p.weight[ibone];

					if(p.inode[ibone]>=0)
						bones_inode[p.inode[ibone]]=1;
					else
						xassert(p.weight[ibone]==0);
				}

//				xassert(sum>=0.999f && sum<1.001f);//Потом опять вставить, закомментировано для демы

				sum=1/sum;
				for(ibone=0;ibone<cTempBone::max_bones;ibone++)
				{
					p.weight[ibone]*=sum;
				}

			}

			inode_array.clear();
			BonesInode::iterator itn;
			FOR_EACH(bones_inode,itn)
			{
				inode_array.push_back(itn->first);
			}
		}
		break;
	}

	DeleteSingularPolygon();

	xassert(vertex_pos.size()==vertex_norm.size());
	if(!vertex_uv.empty())
	{
		xassert(vertex_pos.size()==vertex_uv.size());
	}
	if(!vertex_uv2.empty())
	{
		xassert(vertex_pos.size()==vertex_uv2.size());
	}
}

void cTempMesh3dx::DeleteSingularPolygon()
{
	float tolerance=1e-15f;
	for(int i=0;i<polygons.size();i++)
	{
		sPolygon& p=polygons[i];
		Vect3f &p0=vertex_pos[p[0]];
		Vect3f &p1=vertex_pos[p[1]];
		Vect3f &p2=vertex_pos[p[2]];
		if(p0.eq(p1,tolerance) || p0.eq(p2,tolerance) || p1.eq(p2,tolerance))
		{
			polygons.erase(polygons.begin()+i);
			i--;
		}
	}
}

void cTempMesh3dx::FillVisibleGroup(int visible_set,DWORD visible)
{
	visible_group.resize(1);
	cTempVisibleGroup& vg=visible_group[0];
	vg.visible_set=visible_set;
	vg.visible=visible;
	vg.begin_polygon=0;
	vg.num_polygon=polygons.size();
}

void cStatic3dx::GetVisibleMesh(const string& mesh_name,int& visible_set,DWORD& visible,DWORD& lod_mask,
								bool& is_debris,bool& is_no_debris)
{
	visible_set=0;
	visible=0;
	lod_mask=0;
	is_debris=false;
	is_no_debris=false;
	cStaticVisibilitySet* pset=NULL;
	bool found_set=false;
	for(int iset=0;iset<visible_sets.size();iset++)
	{
		pset=visible_sets[iset];
		vector<string>::iterator itm;
		if(pset->is_all_mesh_in_set)
		{
			found_set=true;
		}else
		{
			FOR_EACH(pset->mesh_in_set,itm)
			if(*itm==mesh_name)
			{
				found_set=true;
				break;
			}
		}

		if(found_set)
		{
			visible_set=iset;
			break;
		}
	}

	if(!found_set)
		return;

	xassert(!pset->raw_visibility_groups.empty());
	for(int ivis=0;ivis<pset->raw_visibility_groups.size();ivis++)
	{
		cStaticVisibilityChainGroup* cur=pset->raw_visibility_groups[ivis];

		bool found=false;
		vector<string>::iterator itt;
		FOR_EACH(cur->temp_visible_object,itt)
		{
			if(mesh_name==*itt)
			{
				found=true;
				break;
			}
		}

		if(cur->is_invisible_list)
			found=!found;

		if(found)
		{
			if(cur->name=="debris")
				is_debris=true;
			else
				is_no_debris=true;
			xassert(cur->visible_shift);
			visible|=cur->visible_shift;
			if(cur->lod!=-1)
				lod_mask|=1<<cur->lod;
		}
	}

}


int cStatic3dx::LoadMaterialsNum(CLoadDirectory rd)
{
	int num_material=0;
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MATERIAL:
		num_material++;
		break;
	}

	return num_material;
}

void cStatic3dx::LoadMaterials(CLoadDirectory rd,const char* path_name,int num_materials)
{
	int cur_mat=0;
	materials.resize(num_materials);
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MATERIAL:
		{
			materials[cur_mat].Load(ld,path_name,this, chains_block);
			cur_mat++;
		}
		break;
	}
}

cStaticMaterial::cStaticMaterial()
{
	ambient=sColor4f(0,0,0);
	diffuse=sColor4f(1,1,1);
	specular=sColor4f(0,0,0);
	opacity=0;
	specular_power=0;

	is_skinned=false;
	pBumpTexture=NULL;
	pReflectTexture=NULL;
	pSpecularmap=NULL;
	pSecondOpacityTexture=NULL;
	animation_group_index=-1;
	reflect_amount=1.0f;
	is_reflect_sky=false;
	is_opacity_texture=false;
	no_light=false;
	tiling_diffuse=TILING_U_WRAP|TILING_V_WRAP;
	transparency_type=TRANSPARENCY_FILTER;
	is_big_ambient=false;
}

cStaticMaterial::~cStaticMaterial()
{
	RELEASE(pSecondOpacityTexture);
	RELEASE(pBumpTexture);
	RELEASE(pReflectTexture);
	RELEASE(pSpecularmap);
}
void cStaticMaterial::Save(Saver &s, const StaticChainsBlock& chains_block)
{
	Vect3f v;
	DWORD slot=-1;
	s.push(C3DX_MATERIAL_HEAD);
	s<<name;
	s.pop();

	s.push(C3DX_MATERIAL_STATIC);
	v.set(ambient.r,ambient.g,ambient.b);
	s<<v;
	v.set(diffuse.r,diffuse.g,diffuse.b);
	s<<v;
	v.set(specular.r,specular.g,specular.b);
	s<<v;
	s<<opacity;
	s<<specular_power;
	s<<no_light;
	s<<animation_group_index;
	s.pop();

	string sname;
	if (!tex_diffuse.empty())
	{
		slot = TEXMAP_DI;
		sname = tex_diffuse;
		SaveTextureMap(s,sname,slot);
	}
	if (!tex_bump.empty())
	{
		slot = TEXMAP_BU;
		sname = tex_bump;
		SaveTextureMap(s,sname,slot);
	}
	if (!tex_reflect.empty())
	{
		slot = TEXMAP_RL;
		sname = tex_reflect;
		SaveTextureMap(s,sname,slot);
	}
	if (!tex_self_illumination.empty())
	{
		slot = TEXMAP_SI;
		sname = tex_self_illumination;
		SaveTextureMap(s,sname,slot);
	}
	if (!tex_specularmap.empty())
	{
		slot = TEXMAP_SP;
		sname = tex_specularmap;
		SaveTextureMap(s,sname,slot);
	}
	if (!tex_secondopacity.empty())
	{
		slot = TEXMAP_DP;
		sname = tex_secondopacity;
		SaveTextureMap(s,sname,slot);
	}
	if (is_skinned)
	{
		slot = TEXMAP_FI;
		sname=tex_skin;
		SaveTextureMap(s,sname,slot);
	}
	if (is_opacity_texture)
	{
		slot = TEXMAP_OP;
		SaveTextureMap(s,sname,slot);
	}
	for (int i=0; i<chains.size();i++)
	{
		s.push(C3DX_NODE_CHAIN);
		chains[i].Save(s, chains_block);
		s.pop();
	}
}
void cStaticMaterial::SaveTextureMap(Saver& s, string &name, int slot)
{
	s.push(C3DX_MATERIAL_TEXTUREMAP);
	s<<slot;
	name.erase(0,name.find_last_of("\\")+1);
	s<<name;
	s<<reflect_amount;
	s<<tiling_diffuse;
	s<<transparency_type;
	s.pop();

}

void cStaticMaterial::Load(CLoadDirectory rd,const char* path_name,cStatic3dx* pStatic, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MATERIAL_HEAD:
		{
			CLoadIterator it(ld);
			it>>name;
		}
		break;
	case C3DX_MATERIAL_STATIC:
		{
			CLoadIterator it(ld);
			Vect3f v;
			it>>v;ambient=sColor4f(v.x,v.y,v.z);
			it>>v;diffuse=sColor4f(v.x,v.y,v.z);
			it>>v;specular=sColor4f(v.x,v.y,v.z);
			it>>opacity;
			it>>specular_power;
			specular_power=max(specular_power,1.0f);
			it>>no_light;
			it>>animation_group_index;
		}
		break;
	case C3DX_MATERIAL_TEXTUREMAP:
		{
			CLoadIterator it(ld);
			DWORD slot=-1;
			string name;
			float amount=1;
			int tiling=TILING_U_WRAP|TILING_V_WRAP;
			int transparency=TRANSPARENCY_FILTER;
			it>>slot;
			it>>name;
			it>>amount;
			it>>tiling;
			it>>transparency;
			transparency_type = transparency;

			if(!name.empty())
			{
				string full_name=path_name;
				full_name+=name;
				switch(slot)
				{
				case TEXMAP_DI:
					tex_diffuse=full_name;
					tiling_diffuse=tiling;
					break;
				case TEXMAP_FI:
					is_skinned=true;
					tex_skin=full_name;
					break;
				case TEXMAP_BU:
					tex_bump=full_name;
					break;
				case TEXMAP_RL:
					tex_reflect=full_name;
					reflect_amount=amount;
					break;
				case TEXMAP_OP:
					is_opacity_texture=true;
					break;
				case TEXMAP_SI:
					tex_self_illumination=full_name;
					break;
				case TEXMAP_SP:
					tex_specularmap=full_name;
					break;
				case TEXMAP_DP:
					tex_secondopacity=full_name;
					break;
				}
			}
		}
		break;
	case C3DX_NODE_CHAIN:
		chains.push_back(cStaticMaterialAnimation());
		chains.back().Load(ld, chains_block);
		break;
	}
	diffuse.a=opacity;
	gb_RenderDevice3D->SetCurrentConvertDot3Mul(10.0f);//Потом читать из файла

	if(!tex_bump.empty())
	{
		if(gb_RenderDevice3D->IsPS20())
			pBumpTexture=pStatic->LoadTexture(tex_bump.c_str(),"Bump");
	}

	if(!tex_specularmap.empty())
	{
		if(gb_RenderDevice3D->IsPS20())
			pSpecularmap=pStatic->LoadTexture(tex_specularmap.c_str(),"Specular");
	}

	if(!tex_secondopacity.empty())
	{
		pSecondOpacityTexture=pStatic->LoadTexture(tex_secondopacity.c_str());
	}

	if(!tex_reflect.empty())
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath(tex_reflect.c_str(),drive,dir,fname,ext);
		if(stricmp(fname,"sky")==0 && cScene::IsSkyCubemap())
		{
			is_reflect_sky=true;
		}else
		{
			pReflectTexture=pStatic->LoadTexture(tex_reflect.c_str());
		}
	}

	float min_ambient=0.65f;
	is_big_ambient=ambient.r>min_ambient || ambient.g>min_ambient || ambient.b>min_ambient;
}

void cStatic3dx::BuildBuffers(vector<cTempMesh3dx*>& temp_mesh,ONE_LOD& lod)
{
	if(temp_mesh.empty())
	{
//		errlog()<<"Нет ни одного объекта"<<VERR_END;
		return;
	}

	vector<cTempMesh3dx*> out_mesh;
	BuildSkinGroupSortedNormal(temp_mesh,out_mesh);

	int vertex_size=0;
	int polygon_size=0;
	bool is_blend=false;

	bool first_pass=true;
	vector<cTempMesh3dx*>::iterator itm;
	int max_bones_per_vertex=1;
	FOR_EACH(out_mesh,itm)
	{
		cTempMesh3dx* m=*itm;
		m->OptimizeMesh();

		vertex_size+=m->vertex_pos.size();
		polygon_size+=m->polygons.size();
		int bones_per_vertex=m->CalcMaxBonesPerVertex();
		max_bones_per_vertex=max(max_bones_per_vertex,bones_per_vertex);
		if(!m->bones.empty())
			is_blend=true;

		bool is_bump=materials[m->imaterial].pBumpTexture!=NULL;
		//if(first_pass && is_bump)
		if(is_bump)
			bump=true;

//		xassert(chain_group->bump==is_bump);

		first_pass=false;
	}

	if(vertex_size>65535)
		errlog()<<"Количество вертексов "<<vertex_size<<" превышает максимально допустимые 65535"<<VERR_END;
	xassert(polygon_size<65535);

	vector<sVertexXYZINT1> vertex(vertex_size);
	vector<sPolygon> polygons(polygon_size);

	gb_RenderDevice->CreateIndexBuffer(lod.ib,polygon_size);
	sPolygon *IndexPolygon=gb_RenderDevice->LockIndexBuffer(lod.ib);

	lod.blend_indices=max_bones_per_vertex;

	cSkinVertex skin_vertex(lod.GetBlendWeight(),bump,is_uv2);

	gb_RenderDevice->CreateVertexBuffer(lod.vb,vertex_size, skin_vertex.GetDeclaration());
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);

	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

	lod.skin_group.resize(out_mesh.size());
	int cur_vertex_offset=0;
	int cur_polygon_offset=0;
	for(int i=0;i<out_mesh.size();i++)
	{
		cTempMesh3dx& tmesh=*out_mesh[i];
		cStaticIndex& sinsex=lod.skin_group[i];
		sinsex.offset_polygon=cur_polygon_offset;
		sinsex.offset_vertex=cur_vertex_offset;
		sinsex.num_polygon=tmesh.polygons.size();
		sinsex.num_vertex=tmesh.vertex_pos.size();
		sinsex.imaterial=tmesh.imaterial;
		sinsex.node_index=tmesh.inode_array;
		sinsex.visible_group=tmesh.visible_group;

		for(int index=0;index<sinsex.num_polygon;index++)
		{
			sPolygon& in=tmesh.polygons[index];
			sPolygon& out=IndexPolygon[cur_polygon_offset+index];
			out.p1=in.p1+cur_vertex_offset;
			out.p2=in.p2+cur_vertex_offset;
			out.p3=in.p3+cur_vertex_offset;
		}

		for(int vertex=0;vertex<sinsex.num_vertex;vertex++)
		{
			skin_vertex.Select(cur_vertex_offset+vertex);
			skin_vertex.GetPos()=tmesh.vertex_pos[vertex];
			skin_vertex.GetNorm()=tmesh.vertex_norm[vertex];
			if(tmesh.vertex_uv.empty())
			{
				skin_vertex.GetTexel().set(0,0);
			}else
			{
				skin_vertex.GetTexel()=tmesh.vertex_uv[vertex];
			}

			if(is_uv2)
			{
				if(tmesh.vertex_uv2.empty())
				{
					skin_vertex.GetTexel2().set(0,0);
				}else
				{
					skin_vertex.GetTexel2()=tmesh.vertex_uv2[vertex];
				}
			}

			sColor4c& index=skin_vertex.GetIndex();
			index.RGBA()=0;
		}

		cur_vertex_offset+=tmesh.vertex_pos.size();
		cur_polygon_offset+=tmesh.polygons.size();
	}

	gb_RenderDevice->UnlockIndexBuffer(lod.ib);

	for(int i=0;i<out_mesh.size();i++)
	{
		cTempMesh3dx& tmesh=*out_mesh[i];
		cStaticIndex& sinsex=lod.skin_group[i];

		cStaticIndex* cur_index=NULL;
		int j;
		for(j=0;j<lod.skin_group.size();j++)
		{
			cStaticIndex& s=lod.skin_group[j];
			if(s.offset_polygon<=sinsex.offset_polygon && 
				s.offset_polygon+s.num_polygon>sinsex.offset_polygon)
			{
				cur_index=&lod.skin_group[j];
				break;
			}
		}

		vector<int> node_hash(nodes.size());
		for(j=0;j<node_hash.size();j++)
			node_hash[j]=-1;
		for(j=0;j<cur_index->node_index.size();j++)
		{
			int idx=cur_index->node_index[j];
			xassert(idx>=0 && idx<node_hash.size());
			xassert(node_hash[idx]==-1);
			node_hash[idx]=j;
		}

		if(!tmesh.bones.empty())
		{
			for(int vertex=0;vertex<sinsex.num_vertex;vertex++)
			{
				skin_vertex.Select(sinsex.offset_vertex+vertex);
				cTempBone& tb=tmesh.bones[vertex];

				sColor4c weight;
				weight.RGBA()=0;
				xassert(lod.blend_indices<=cTempBone::max_bones);
				int sum=0;
				int ibone;
				int weights=lod.GetBlendWeight();

				int error_weight=0;
				for(ibone=0;ibone<weights;ibone++)
				{
					int w=int(tb.weight[ibone]*255);
					error_weight+=w;
				}
				error_weight=255-error_weight;

				for(ibone=0;ibone<weights;ibone++)
				{
					int w=int(tb.weight[ibone]*255);
					if(ibone==0)
						w+=error_weight;
//					if(ibone==chain_group->blend_indices-1)
//					{
//						int w=255-sum;
//					}
					xassert(w>=0 && w<=255);
					skin_vertex.GetWeight(ibone)=w;
					sum+=w;
				}

				xassert(sum==255 || weights==0);
				sColor4c index;
				index.RGBA()=0;
				for(ibone=0;ibone<lod.blend_indices;ibone++)
				{
					int idx=0;
					int cur_inode=tb.inode[ibone];
					if(cur_inode>=0)
						idx=node_hash[cur_inode];
					//xassert(idx>=0 && idx<cStaticIndex::max_index);
					xassert(idx>=0 && idx<node_hash.size());
					index[ibone]=idx;
				}
				skin_vertex.GetIndex()=index;
			}
		}else
		{
			xassert(0);
			/*
			for(int vertex=0;vertex<sinsex.num_vertex;vertex++)
			{
				skin_vertex.Select(sinsex.offset_vertex+vertex);
				int idx=node_hash[sinsex.inode];
				xassert(idx>=0 && idx<cStaticIndex::max_index);
				sColor4c& ix=skin_vertex.GetIndex();
				ix[0]=idx;
				ix[1]=0;
				ix[2]=0;
				ix[3]=0;

				if(chain_group->GetBlendWeight()>0)
				{
					skin_vertex.GetWeight(0)=255;
					skin_vertex.GetWeight(1)=0;
					skin_vertex.GetWeight(2)=0;
					skin_vertex.GetWeight(3)=0;
				}
			}
			*/
		}
	}
	gb_RenderDevice->UnlockVertexBuffer(lod.vb);

	CalcBumpSTNorm(lod);

//	vector<cTempMesh3dx*>::iterator itm;
	FOR_EACH(out_mesh,itm)
		delete *itm;

}


void cStatic3dx::CalcBumpSTNorm(ONE_LOD& lod)
{
	if(!bump)
		return;

	cSkinVertex skin_vertex(lod.GetBlendWeight(),bump,is_uv2);
	void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);
	skin_vertex.SetVB(pVertex,lod.vb.GetVertexSize());

	sPolygon *Polygon=gb_RenderDevice->LockIndexBuffer(lod.ib);
	int i;
	for(i=0;i<lod.vb.GetNumberVertex();i++)
	{
		skin_vertex.Select(i);
		skin_vertex.GetBumpS().set(0,0,0);
		skin_vertex.GetBumpT().set(0,0,0);
  		skin_vertex.GetNorm().Normalize();
	}
	
	cSkinVertex v0(lod.GetBlendWeight(),bump,is_uv2),
				v1(lod.GetBlendWeight(),bump,is_uv2),
				v2(lod.GetBlendWeight(),bump,is_uv2);
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
			n.Normalize();
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

			v0.GetBumpS() += sdir;
			v1.GetBumpS() += sdir;
			v2.GetBumpS() += sdir;

			v0.GetBumpT() += tdir;
			v1.GetBumpT() += tdir;
			v2.GetBumpT() += tdir;
		}
	}

	int num_bad=0;
	for(i=0;i<lod.vb.GetNumberVertex();i++)
	{
		skin_vertex.Select(i);
		Vect3f s,t,n;
		n=skin_vertex.GetNorm();
		s=skin_vertex.GetBumpS();
		t=skin_vertex.GetBumpT();
/*
		//Не перепутали местами?
  		skin_vertex.GetBumpS()=(t-n*n.dot(t)).Normalize();
		skin_vertex.GetBumpT().cross(n, skin_vertex.GetBumpS());
/*/
  		skin_vertex.GetBumpT()=(t-n*n.dot(t)).Normalize();
		skin_vertex.GetBumpS().cross(n, skin_vertex.GetBumpT());
		float dots=dot(skin_vertex.GetBumpS(),s);
		if(dots<0)
		{
			skin_vertex.GetBumpS()=-skin_vertex.GetBumpS();
		}

//		float dots=dot(skin_vertex.GetBumpS(),t);
//		if(dots<0)
//		{
//			skin_vertex.GetBumpS()=-skin_vertex.GetBumpS();
//		}
/**/
/*
		s=skin_vertex.GetBumpS();
		t=skin_vertex.GetBumpT();
		Mat3f m(n.x,n.y,n.z,
			    s.x,s.y,s.z,
				t.x,t.y,t.z);
		float det=m.det();
		if(det<0.9f)
		{
			int k=0;
			num_bad++;
		}

		if(det<0)
		{
			int k=0;
		}
/**/
	}

	gb_RenderDevice->UnlockVertexBuffer(lod.vb);
	gb_RenderDevice->UnlockIndexBuffer(lod.ib);
}


void cStaticBasement::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_BASEMENT_VERTEX:
		{
			CLoadIterator it(ld);
			it>>vertex;
		}
		break;
	case C3DX_BASEMENT_FACES:
		{
			CLoadIterator it(ld);
			it>>polygons;
		}
		break;
	}
}

void cStaticLogicBound::Load(CLoadIterator it)
{
	it>>inode;
	it>>bound.min;
	it>>bound.max;
}

void cStatic3dx::LoadLocalLogicBound(CLoadIterator it)
{
	cStaticLogicBound s;
	int inode=0;
	sBox6f bound;
	bound.SetInvalidBox();
	it>>inode;
	it>>bound.min;
	it>>bound.max;

	if(local_logic_bounds.empty())
	{
		xassert(!nodes.empty());
		sBox6f bound;
		bound.SetInvalidBox();
		local_logic_bounds.resize(nodes.size(),bound);
	}

	xassert(inode>=0 && inode<local_logic_bounds.size());
	local_logic_bounds[inode]=bound;
}

void cStatic3dx::SaveLocalLogicBound(Saver& s)
{
	for(int inode=0;inode<local_logic_bounds.size();inode++)
	{
		sBox6f& b=local_logic_bounds[inode];
		if(b.min.x<b.max.x)
		{
			s.push(C3DX_LOGIC_BOUND_LOCAL);
			s<<inode;
			s<<b.min;
			s<<b.max;
			s.pop();
		}
	}
}
cVisError& cStatic3dx::errlog()
{
	return VisError<<"Error load file: "<<file_name.c_str()<<"\r\n";
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
		cStaticNode& node=nodes[inode];
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

		cStaticEffect effect;
		effect.node=inode;
		string effect_name(cur,end-cur);
		effect.file_name=file_name;
		effect.is_cycled=key->IsCycled();

		effects.push_back(effect);
	}
}

void MaxToD3D(MatXf &in)
{
	MatXf out;
	out.rot()[0][0]=in.rot()[0][0];
	out.rot()[0][1]=in.rot()[0][1];
	out.rot()[0][2]=in.rot()[0][2];

	out.rot()[1][0]=in.rot()[2][0];
	out.rot()[1][1]=in.rot()[2][1];
	out.rot()[1][2]=in.rot()[2][2];
	
	out.rot()[2][0]=-in.rot()[1][0];
	out.rot()[2][1]=-in.rot()[1][1];
	out.rot()[2][2]=-in.rot()[1][2];

	out.trans().x= in.trans().x;
	out.trans().y= in.trans().z;
	out.trans().z= -in.trans().y;

	in=out;
}

void cStatic3dx::LoadLights(CLoadDirectory rd,const char* path_name, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_LIGHT:
		{
			lights.push_back(cStaticLights());
			lights.back().Load(ld,path_name,this, chains_block);
			break;
		}
	}
}

void cStaticLights::Load(CLoadDirectory rd,const char* path_name,cStatic3dx* pStatic, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_LIGHT_HEAD:
		{
			CLoadIterator it(ld);
			it>>inode;
			Vect3f c;
			it>>c;
			color.set(c.x,c.y,c.z,1);
			it>>atten_start;
			it>>atten_end;
			break;
		}
	case C3DX_NODE_CHAIN:
		chains.push_back(cStaticLightsAnimation());
		chains.back().Load(ld, chains_block);
		break;
	case C3DX_LIGHT_TEXTURE:
		{
			CLoadIterator it(ld);
			string texture_name;
			it>>texture_name;
			string full_name=path_name;
			full_name+=texture_name;
			RELEASE(pTexture);
			pTexture=pStatic->LoadTexture(full_name.c_str());
		}
		break;

	}
}
void cStaticLights::Save(Saver& s, StaticChainsBlock& chains_block)
{
	Vect3f v;
	s.push(C3DX_LIGHT_HEAD);
	s<<inode;
	v.set(color.r,color.g,color.b);
	s<<v;
	s<<atten_start;
	s<<atten_end;
	s.pop();

	for (int i=0; i<chains.size(); i++)
	{
		s.push(C3DX_NODE_CHAIN);
		chains[i].Save(s, chains_block);
		s.pop();
	}

	if (!pTexture)
		return;
    string sname(pTexture->GetName());
	if (sname.empty())
		return;
	s.push(C3DX_LIGHT_TEXTURE);
	sname.erase(0,sname.find_last_of("\\"));
	s<<sname;
	s.pop();
}

void cStaticMaterialAnimation::Load(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_MATERIAL_ANIM_OPACITY:
		chains_block.converter().loadInterpolator(opacity, ld);
		break;
	case C3DX_MATERIAL_ANIM_UV:
		chains_block.converter().loadInterpolator(uv, ld);
		break;
	case C3DX_MATERIAL_ANIM_UV_DISPLACEMENT:
		chains_block.converter().loadInterpolator(uv_displacement, ld);
		break;
	
	case C3DX_MATERIAL_ANIM_OPACITY_INDEX:
		opacity.LoadIndex(ld, chains_block);
		break;
	case C3DX_MATERIAL_ANIM_UV_INDEX:
		uv.LoadIndex(ld, chains_block);
		break;
	case C3DX_MATERIAL_ANIM_UV_DISPLACEMENT_INDEX:
		uv_displacement.LoadIndex(ld, chains_block);
		break;
	}
}
void cStaticMaterialAnimation::Save(Saver &s, const StaticChainsBlock& chains_block)
{
	if (opacity.values.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_OPACITY_INDEX);
		opacity.SaveIndex(s, chains_block);
		s.pop();
	}
	if (uv.values.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_UV_INDEX);
		uv.SaveIndex(s, chains_block);
		s.pop();
	}
	if (uv_displacement.values.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_UV_DISPLACEMENT_INDEX);
		uv_displacement.SaveIndex(s, chains_block);
		s.pop();
	}

	/*
	if (opacity.data_.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_OPACITY);
		opacity.Save(s);
		s.pop();
	}
	if (uv.data_.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_UV);
		uv.Save(s);
		s.pop();
	}
	if (uv_displacement.data_.size()>0)
	{
		s.push(C3DX_MATERIAL_ANIM_UV_DISPLACEMENT);
		uv_displacement.Save(s);
		s.pop();
	}
	*/
}

void cStaticLightsAnimation::Load(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case C3DX_LIGHT_ANIM_COLOR:
		chains_block.converter().loadInterpolator(color, ld);
		break;
	case C3DX_LIGHT_ANIM_COLOR_INDEX:
		color.LoadIndex(ld, chains_block);
		break;
	}
}
void cStaticLightsAnimation::Save(Saver &s, StaticChainsBlock& chains_block)
{
	
	s.push(C3DX_LIGHT_ANIM_COLOR_INDEX);
	color.SaveIndex(s, chains_block);
	s.pop();
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
void cStatic3dx::MergeMaterialMesh(vector<cTempMesh3dx*>& temp_mesh,vector<cTempMesh3dx*>& material_mesh,bool merge_mesh)
{
	if(temp_mesh.empty())
		return;

	vector<cTempMesh3dx*> cur_mesh;
	cur_mesh.push_back(temp_mesh[0]);

	for(int itemp=1;itemp<temp_mesh.size();itemp++)
	{
		cTempMesh3dx* p=temp_mesh[itemp];
		bool mergerable=false;
		if(merge_mesh)
		{
			mergerable=p->imaterial==cur_mesh[0]->imaterial;
		}else
		{
			mergerable=p->inode==cur_mesh[0]->inode;
		}
		if(!mergerable)
		{
			cTempMesh3dx* mesh=new cTempMesh3dx;
			mesh->Merge(cur_mesh,this);
			material_mesh.push_back(mesh);
			cur_mesh.clear();
		}
		cur_mesh.push_back(p);
	}

	if(!cur_mesh.empty())
	{
		cTempMesh3dx* mesh=new cTempMesh3dx;
		mesh->Merge(cur_mesh,this);
		material_mesh.push_back(mesh);
		cur_mesh.clear();
	}
}

void cTempMesh3dx::Merge(vector<cTempMesh3dx*>& meshes,cStatic3dx* pStatic)
{
	int num_vertices=0;
	int num_polygons=0;
	typedef StaticMap<int,int> NODE_MAP;
	NODE_MAP node_map;
	int itemp;
	for(itemp=0;itemp<meshes.size();itemp++)
	{
		cTempMesh3dx* p=meshes[itemp];
		num_vertices+=p->vertex_pos.size();
		num_polygons+=p->polygons.size();

		int sz=p->vertex_pos.size();
		xassert(sz==p->vertex_norm.size());
		xassert(sz==p->vertex_uv.size() || p->vertex_uv.size()==0);
		xassert(sz==p->vertex_uv2.size() || p->vertex_uv2.size()==0);
		xassert(sz==p->bones.size() || p->bones.size()==0);

		int n=p->inode;
		node_map[n]=1;
		for(int inode=0;inode<p->inode_array.size();inode++)
		{
			n=p->inode_array[inode];
			node_map[n]=1;
		}
	}

	imaterial=meshes[0]->imaterial;
	inode=meshes[0]->inode;

	NODE_MAP::iterator it_map;
	FOR_EACH(node_map,it_map)
	{
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

	for(itemp=0;itemp<meshes.size();itemp++)
	{
		cTempMesh3dx* p=meshes[itemp];


		cTempBone temp_bone;
		temp_bone.weight[0]=1;
		temp_bone.weight[1]=0;
		temp_bone.weight[2]=0;
		temp_bone.weight[3]=0;

		temp_bone.inode[0]=p->inode;
		temp_bone.inode[1]=
		temp_bone.inode[2]=
		temp_bone.inode[3]=-1;

		for(int ipos=0;ipos<p->vertex_pos.size();ipos++)
		{
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

			if(p->bones.size())
			{
				cTempBone& tb=p->bones[ipos];
				cTempBone& out=bones[ipos+offset_vertices];
				for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
				{
					int bone=tb.inode[ibone];
					if(tb.weight[ibone]>0)
						out.inode[ibone]=bone;
					else
						out.inode[ibone]=-1;
					out.weight[ibone]=tb.weight[ibone];
				}
			}else
			{
				bones[ipos+offset_vertices]=temp_bone;
			}
		}

		for(int ipolygon=0;ipolygon<p->polygons.size();ipolygon++)
		{
			sPolygon& pin=p->polygons[ipolygon];
			sPolygon& pout=polygons[ipolygon+offset_polygon];
			for(int ipnt=0;ipnt<3;ipnt++)
			{
				pout[ipnt]=pin[ipnt]+offset_vertices;
			}
		}

		xassert(p->visible_group.size()==1);
		cTempVisibleGroup in_vg=p->visible_group[0];
		in_vg.begin_polygon=offset_polygon;
		if(!visible_group.empty())
		{
			cTempVisibleGroup& out_vg=visible_group.back();
			if(out_vg.visible_set==in_vg.visible_set &&
			   out_vg.visible==in_vg.visible)
			{
				out_vg.num_polygon+=in_vg.num_polygon;
			}else
			{
				visible_group.push_back(in_vg);
			}
		}else
		{
			visible_group.push_back(in_vg);
		}

		offset_vertices+=p->vertex_pos.size();
		offset_polygon+=p->polygons.size();
	}
}

/*
 Дописать!!!
 Учитывать разбиение на cTempVisibleGroup.
 Перемещать треугольники внутри ноды только.
*/
void cStatic3dx::BuildSkinGroupSortedNormal(vector<cTempMesh3dx*>& temp_mesh,vector<cTempMesh3dx*>& out_mesh)
{
	vector<cTempMesh3dx*>::iterator it_mesh;
	FOR_EACH(temp_mesh,it_mesh)
	{
		cTempMesh3dx* tm=*it_mesh;
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
			cTempVisibleGroup* cur_vg=NULL;
			
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
				cur_vg=NULL;

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
					for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
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
			if(node_index.size()==cStaticIndex::max_index ||
				node_index.size()+first_additional_node>cStaticIndex::max_index
				|| first_polygon==-1)
			{
				xassert(node_index.size()<=cStaticIndex::max_index);
				cTempMesh3dx* new_mesh;
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
					for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
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
					for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
					{
						if(bone.weight[ibone]>0)
						{
							int node=bone.inode[ibone];
							xassert(node>=0 && node<selected_node.size());
							sum_used+=10;
						}
					}
				}

				for(ipnt=0;ipnt<3;ipnt++)
				{
					WORD pnt=p[ipnt];
					xassert(pnt<tm->bones.size());
					cTempBone& bone=tm->bones[pnt];
					for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
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
					for(int ibone=0;ibone<cTempBone::max_bones;ibone++)
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

void cStatic3dx::ExtractMesh(cTempMesh3dx* mesh,cTempMesh3dx*& out_mesh,
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
				if(is_uv2)
					out_mesh->vertex_uv2.push_back(mesh->vertex_uv2[ivertex]);
				out_mesh->bones.push_back(mesh->bones[ivertex]);
			}

			new_polygon[ip]=idx;
		}

		out_mesh->polygons.push_back(new_polygon);
	}

	if(out_mesh->vertex_pos.empty())
	{
		delete out_mesh;
		out_mesh=NULL;
	}
}

cTexture* cStatic3dx::LoadTexture(const char* name,char* mode)
{
	if(name==0||name[0]==0) return NULL;
	bool prev_error=GetTexLibrary()->EnableError(false);
	//cTexture* p=GetTexLibrary()->GetElement(name,mode);
	cTexture* p=GetTexLibrary()->GetElement3D(name,mode);
	if(!p)
	{
		errlog()<<"Texture is bad: "<<name<<"."<<VERR_END;
	}
	GetTexLibrary()->EnableError(prev_error);
	return p;
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

static int GetLodSuffix(const char* str,int* new_size)
{
	const int postfix_len=5;
	int lod=-1;
	int str_len=strlen(str);
	if(str_len>=postfix_len)
	{
		if(str[str_len-5]=='l' && 
		str[str_len-4]=='o' &&
		str[str_len-3]=='d' &&
		str[str_len-2]=='0')
		{
			if(str[str_len-1]=='1')
				lod=0;
			else
			if(str[str_len-1]=='2')
				lod=1;
			else
			if(str[str_len-1]=='3')
				lod=2;
		}
	}

	if(new_size)
		*new_size=lod!=-1?str_len-postfix_len:0;

	return lod;
}

bool cStaticVisibilitySet::IsLod()
{
	if(!visibility_groups[0].empty())
	{
		return visibility_groups[0][0]!=visibility_groups[1][0] || visibility_groups[0][0]!=visibility_groups[2][0];
	}
	vector<cStaticVisibilityChainGroup*>::iterator itv;
	int mask_lod=0;
	FOR_EACH(raw_visibility_groups,itv)
	{
		const int postfix_len=5;
		cStaticVisibilityChainGroup* cur=*itv;
		const char* str=cur->name.c_str();
		int lod=GetLodSuffix(str,NULL);
		if(lod!=-1)
			mask_lod|=1<<lod;
	}

	return mask_lod==7;
}

bool cStaticVisibilitySet::BuildVisibilityGroupsLod()
{
	bool is_lod=false;
	if(!cObject3dx::GetUseLod() && false)
	{
		for(int ilod=0;ilod<num_lod;ilod++)
		{
			visibility_groups[ilod]=raw_visibility_groups;
		}
		return false;
	}

	if(!visibility_groups[0].empty())
	{
		is_lod=visibility_groups[0][0]!=visibility_groups[1][0] || visibility_groups[0][0]!=visibility_groups[2][0];
		if(is_lod && Option_DeleteLod)
		{
			for(int igroup=0;igroup<visibility_groups[0].size();igroup++)
				if(Option_DeleteLod==1)
					visibility_groups[0][igroup]=visibility_groups[1][igroup];
				else
					visibility_groups[0][igroup]=visibility_groups[1][igroup]=visibility_groups[2][igroup];
		}
		return is_lod;
	}

	is_lod=IsLod();

	vector<cStaticVisibilityChainGroup*>::iterator itv;
	FOR_EACH(raw_visibility_groups,itv)
	{
		cStaticVisibilityChainGroup* cur=*itv;
		const char* str=cur->name.c_str();
		int new_size;
		int lod=GetLodSuffix(str,&new_size);

		if(lod!=-1 && is_lod)
		{
			cur->name.resize(new_size);
			cur->lod=lod;
		}

		bool found=false;
		for(int igroup=0;igroup<visibility_groups[0].size();igroup++)
		{
			if(visibility_groups[0][igroup]->name==cur->name)
			{
				found=true;
				if(lod==-1)
				{
					for(int ilod=0;ilod<num_lod;ilod++)
					{
						if(visibility_groups[ilod][igroup]->lod!=ilod)
						{
							visibility_groups[ilod][igroup]=cur;
						}
					}
				}else
				{
					visibility_groups[lod][igroup]=cur;
				}
			}
		}

		if(!found)
		{
			for(int ilod=0;ilod<num_lod;ilod++)
			{
				visibility_groups[ilod].push_back(cur);
			}
		}
	}

	for(int igroup=0;igroup<visibility_groups[0].size();igroup++)
	{
		string& name=visibility_groups[0][igroup]->name;
		for(int ilod=1;ilod<num_lod;ilod++)
		{
			bool is_ok_nmame=visibility_groups[ilod][igroup]->name==name;
			xassert(is_ok_nmame);
		}

		if(is_lod && Option_DeleteLod)
		{
			if(Option_DeleteLod==1)
				visibility_groups[0][igroup]=visibility_groups[1][igroup];
			else
				visibility_groups[0][igroup]=visibility_groups[1][igroup]=visibility_groups[2][igroup];
		}
	}


	return is_lod;
}

cStaticVisibilitySet::cStaticVisibilitySet()
{
	is_all_mesh_in_set=false;
}

cStaticVisibilitySet::~cStaticVisibilitySet()
{
	vector<cStaticVisibilityChainGroup*>::iterator it;
	FOR_EACH(raw_visibility_groups,it)
	{
		cStaticVisibilityChainGroup* p=*it;
		delete p;
	}
	raw_visibility_groups.clear();
}

const char* cStaticVisibilitySet::GetVisibilityGroupName(C3dxVisibilityGroup vgn)
{
	xassert(vgn.igroup>=0 && vgn.igroup<visibility_groups[0].size());
	return visibility_groups[0][vgn.igroup]->name.c_str();
}

C3dxVisibilityGroup cStaticVisibilitySet::GetVisibilityGroupIndex(const char* group_name)
{
	int size=GetVisibilityGroupNumber();
	for(int i=0;i<size;i++)
	{
		const char* name=GetVisibilityGroupName(C3dxVisibilityGroup(i));
		if(strcmp(group_name,name)==0)
		{
			return C3dxVisibilityGroup(i);
		}
	}

	return C3dxVisibilityGroup::BAD;
}
void cStaticVisibilityChainGroup::Save(Saver &s)
{
	s<<visible_shift;
	s<<name;
	s<<is_invisible_list;
	s<<lod;
	s<<temp_visible_object;
	DWORD size = (DWORD)visible_nodes.size();
	vector<bool>::iterator it;
	s<<size;
	FOR_EACH(visible_nodes,it)
		s<<*it;
}
void cStaticVisibilityChainGroup::Load(CLoadIterator &ld)
{
	ld>>visible_shift;
	ld>>name;
	ld>>is_invisible_list;
	ld>>lod;
	ld>>temp_visible_object;
	DWORD size=0;
	ld>>size;
	visible_nodes.resize(size);
	for(int i=0;i<size;i++)
	{
		bool b;
		ld>>b;
		visible_nodes[i]=b;
	}
}

void cStaticLogos::Load(CLoadDirectory it)
{
	while(CLoadData* ld=it.next())
	switch(ld->id)
	{
	case C3DX_LOGO_COORD:
		CLoadIterator li(ld);
		sLogo logo;
		li>>logo.TextureName;
		li>>logo.rect.min;
		li>>logo.rect.max;
		li>>logo.angle;
		logos.push_back(logo);
		break;
	}
}

bool cStaticLogos :: GetLogoPosition(const char* fname, sRectangle4f *rect, float& angle)
{
	if(fname[0]==0)
	{
		rect->min.set(0,0);
		rect->max.set(0,0);
		return false;
	}

	string name=GetFileName(fname);
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

void cStatic3dx::SaveOneLod(Saver& saver,ONE_LOD& lod)
{
	//skin groups
	saver.push(C3DX_SKIN_GROUPS);
	for(int i=0; i<lod.skin_group.size(); i++)
	{
		cStaticIndex& index = lod.skin_group[i];
		saver.push(C3DX_SKIN_GROUP);
		saver<<index.imaterial;
		saver<<index.node_index;
		saver<<index.num_polygon;
		saver<<index.num_vertex;
		saver<<index.offset_polygon;
		saver<<index.offset_vertex;
		saver<<index.visible_group.size();
		for (int j=0; j<index.visible_group.size(); j++)
		{
			cTempVisibleGroup& visGrp = index.visible_group[j];
			saver<<visGrp.begin_polygon;
			saver<<visGrp.num_polygon;
			saver<<visGrp.visible;
			saver<<visGrp.visible_set;
		}
		saver.pop();
	}
	saver.pop();

	// buffers
	saver.push(C3DX_BUFFERS);
	saver.push(C3DX_BUFFERS_HEAD);
	saver<<(lod.ib.IsInit()?lod.ib.GetNumberPolygon():0);
	saver<<(lod.vb.IsInit()?lod.vb.GetNumberVertex():0);
	saver<<lod.blend_indices;
	saver<<bump;
	saver<<is_uv2;
	saver.pop();
	void *pBuff;
	if (lod.vb.IsInit())
	{
		saver.push(C3DX_BUFFER_VERTEX);
		int buffer_size=lod.vb.GetNumberVertex()*lod.vb.GetVertexSize();
		saver<<buffer_size;
		pBuff=gb_RenderDevice->LockVertexBuffer(lod.vb);
		saver.write(pBuff,buffer_size);
		gb_RenderDevice->UnlockVertexBuffer(lod.vb);
		saver.pop();

		saver.push(C3DX_BUFFER_INDEX);
		saver<<(lod.ib.GetNumberPolygon()*sizeof(sPolygon));
		pBuff=gb_RenderDevice->LockIndexBuffer(lod.ib);
		saver.write(pBuff,lod.ib.GetNumberPolygon()*sizeof(sPolygon));
		gb_RenderDevice->UnlockIndexBuffer(lod.ib);
		saver.pop();
	}
	saver.pop();
}

void cStatic3dx::LoadOneLod(CLoadDirectory& dir,ONE_LOD& lod)
{
	IF_FIND_DIR(C3DX_SKIN_GROUPS)
	{
		while(CLoadData* l1 = dir.next())
		if (l1->id == C3DX_SKIN_GROUP)
		{
			CLoadIterator rd(l1);
			lod.skin_group.push_back(cStaticIndex());
			cStaticIndex& index = lod.skin_group.back();
			rd>>index.imaterial;
			rd>>index.node_index;
			rd>>index.num_polygon;
			rd>>index.num_vertex;
			rd>>index.offset_polygon;
			rd>>index.offset_vertex;
			int size = 0;
			rd>>size;
			index.visible_group.resize(size);
			for (int j=0; j<size; j++)
			{
				cTempVisibleGroup& visGrp = index.visible_group[j];
				rd>>visGrp.begin_polygon;
				rd>>visGrp.num_polygon;
				rd>>visGrp.visible;
				rd>>visGrp.visible_set;
			}
		}
	}

	IF_FIND_DIR(C3DX_BUFFERS)
	{
		int ib_polygon=0,vb_size=0;
		IF_FIND_DATA(C3DX_BUFFERS_HEAD)
		{
			rd>>ib_polygon;
			rd>>vb_size;
			rd>>lod.blend_indices;
			rd>>bump;
			rd>>is_uv2;
		}
		IF_FIND_DATA(C3DX_BUFFER_VERTEX)
		{
			DWORD size = 0;
			rd>>size;
			if (size > 0)
			{
				cSkinVertex skin_vertex(lod.GetBlendWeight(),bump,is_uv2);
				gb_RenderDevice->CreateVertexBuffer(lod.vb,vb_size, skin_vertex.GetDeclaration());
				xassert(lod.vb.GetNumberVertex()*lod.vb.GetVertexSize()==size);
				void *pVertex=gb_RenderDevice->LockVertexBuffer(lod.vb);
				bool ok=rd.read(pVertex,size);
				xassert(ok);
				gb_RenderDevice->UnlockVertexBuffer(lod.vb);

			}
		}
		IF_FIND_DATA(C3DX_BUFFER_INDEX)
		{
			DWORD size = 0;
			rd>>size;
			if (size > 0)
			{
				xassert(size==ib_polygon*sizeof(sPolygon));
				gb_RenderDevice->CreateIndexBuffer(lod.ib,ib_polygon);
				void *pBuf=(void*)gb_RenderDevice->LockIndexBuffer(lod.ib);
				bool ok=rd.read(pBuf,size);
				xassert(ok);
				gb_RenderDevice->UnlockIndexBuffer(lod.ib);
			}
		}
	}
}

void cStatic3dx::saveCacheData(const char* name)
{
//*
	FileSaver saver;

	if(!saver.Init(name))
	{
		VisError<<"Cannot save "<<name<<VERR_END;
		return;
	}

	saver.push(C3DX_STATIC_CHAINS_BLOCK);
    chains_block.save(saver);
	saver.pop();

	// nodes
	saver.push(C3DX_NODES);

	for(int i=0; i<nodes.size(); i++)
	{
		cStaticNode& node = nodes[i];
		saver.push(C3DX_NODE);
		node.SaveNode(saver, chains_block);
		saver.pop();
	}
	saver.pop();
	
	// animation_chain
	saver.push(C3DX_ANIMATION_CHAIN);
	for (int i=0; i<animation_chain.size(); i++)
	{
		cAnimationChain &chain = animation_chain[i];
		saver.push(C3DX_AC_ONE);
		saver<<chain.name;
		saver<<chain.time;
		saver.pop();
	}
	saver.pop();

	// animation groups
	saver.push(C3DX_ANIMATION_GROUPS);
	for (int i=0; i<animation_group.size(); i++)
	{
		saver.push(C3DX_ANIMATION_GROUP);
		saver<<animation_group[i].name;
		saver<<animation_group[i].nodes;
		saver<<animation_group[i].temp_nodes_name;
		saver<<animation_group[i].temp_materials_name;
		saver.pop();
	}
	saver.pop();

	// visible_sets
	saver.push(C3DX_ANIMATION_VISIBLE_SETS);
	for(int i=0; i<visible_sets.size(); i++)
	{
		saver.push(C3DX_AVS_ONE);
		saver.push(C3DX_AVS_ONE_HEAD);
		cStaticVisibilitySet* set = visible_sets[i];
		saver<<set->name;
		saver<<set->is_all_mesh_in_set;
		saver << set->mesh_in_set;
		saver.pop();

		//saver<<set->raw_visibility_groups.size();
		int j;
		for (j=0; j<set->raw_visibility_groups.size(); j++)
		{	
			cStaticVisibilityChainGroup* raw = set->raw_visibility_groups[j];
			saver.push(C3DX_AVS_ONE_RAW);
			raw->Save(saver);
			saver.pop();
		}

		saver.push(C3DX_AVS_ONE_LODS);
		for (j=0; j<set->num_lod; j++)
		{
			saver<<set->visibility_groups[j].size();
			for (int k=0; k<set->visibility_groups[j].size(); k++)
			{	
				cStaticVisibilityChainGroup* vg = set->visibility_groups[j][k];
				int found=-1;
				for (int m=0; m<set->raw_visibility_groups.size(); m++)
				{	
					if (vg==set->raw_visibility_groups[m])
					{
						found=m;
						break;
					}
				}
				saver<<found;
			}
		}
		saver.pop();

		saver.pop();
	}
	saver.pop();

	// materials
	saver.push(C3DX_MATERIAL_GROUP);
	for (int i=0; i<materials.size(); i++)
	{
		saver.push(C3DX_MATERIAL);
		materials[i].Save(saver, chains_block);
		saver.pop();
	}
	saver.pop();

	//logos
	saver.push(C3DX_LOGOS);
	for (int i=0; i<logos.logos.size(); i++)
	{
		cStaticLogos::sLogo &logo = logos.logos[i];
		saver.push(C3DX_LOGO_COORD);
		saver<<logo.TextureName;
		saver<<logo.rect.min;
		saver<<logo.rect.max;
		saver<<logo.angle;
		saver.pop();
	}
	saver.pop();
	//effects
	saver.push(C3DX_EFFECTS);
	saver<<effects.size();
	for (int i=0; i<effects.size(); i++)
	{
		
		cStaticEffect& effect = effects[i];
		saver<<effect.node;
		//saver<<effect.is_cycled;
		//saver.pop();
		//saver.push(C3DX_EFFECT_KEY);
		//if (effect.effect_key)
		//	effect.effect_key->Save(saver);
		//saver.pop();
	}
	saver.pop();

	//lights
	saver.push(C3DX_LIGHTS);
	for (int i=0; i<lights.size(); i++)
	{
		saver.push(C3DX_LIGHT);
		lights[i].Save(saver, chains_block);
		saver.pop();
	}
	saver.pop();

	saver.push(C3DX_CAMERA);
		saver.push(C3DX_CAMERA_HEAD);
		saver<<camera_params.camera_node_num;
		saver<<camera_params.fov;
		saver.pop();
	saver.pop();

	// other info
	saver.push(C3DX_OTHER_INFO);

	saver<<is_lod;

	saver<<basement.vertex;
	saver<<basement.polygons;

	saver<<logic_bound.bound.min;
	saver<<logic_bound.bound.max;

	saver<<is_logic;

	saver<<is_old_model;

	//Не писать эти параметры
	//saver<<circle_shadow_enable;
	//saver<<circle_shadow_radius;
	//saver<<circle_shadow_height;
	

	saver<<is_inialized_bound_box;
	saver<<bound_box.min;
	saver<<bound_box.max;
	saver<<radius;
	saver.pop();

	saver.push('lods');
		saver.push('head');
		saver<<(DWORD)lods.size();
		saver.pop();
		for(int i=0;i<lods.size();i++)
		{
			saver.push(i);
			SaveOneLod(saver,lods[i]);
			saver.pop();
		}
		saver.push('debr');
		SaveOneLod(saver,debris);
		saver.pop();
	saver.pop();


	saver.push(C3DX_BOUND_SPHERE);
 	saver<<bound_spheres;
	saver.pop();

	SaveLocalLogicBound(saver);
/**/
}

bool cStatic3dx::loadCacheData(const char* name)
{
	start_timer_auto();
//*
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(file_name.c_str(),drive,dir,fname,ext);
	string path_name=drive;
	path_name+=dir;
	path_name+="TEXTURES\\";

	CLoadDirectoryFileRender s;
	if (!s.Load(name))
		return false;
	int size = 0;
	CLoadData* l1;
	while(CLoadData* ld=s.next())
	switch(ld->id) {
	case C3DX_STATIC_CHAINS_BLOCK:
		chains_block.load(ld);
		break;
	case C3DX_NODES:
		if(chains_block.isEmpty())
			return false;

		LoadNodes(ld, chains_block);
		break;
	case C3DX_ANIMATION_CHAIN:
		{
			CLoadDirectory rd(ld);
			while(l1 = rd.next())
				if (l1->id == C3DX_AC_ONE)
				{
					CLoadIterator li(l1);
					animation_chain.push_back(cAnimationChain());
					cAnimationChain & chain = animation_chain.back();
					li>>chain.name;
					li>>chain.time;
				}

		}
		break;
	case C3DX_ANIMATION_GROUPS:
		{
			CLoadDirectory rd(ld);
			while(l1 = rd.next())
				if (l1->id == C3DX_ANIMATION_GROUP)
				{
					CLoadIterator li(l1);
					animation_group.push_back(AnimationGroup());
					AnimationGroup & group = animation_group.back();
					li>>group.name;
					li>>group.nodes;
					li>>group.temp_nodes_name;
					li>>group.temp_materials_name;
				}

		}
		break;
	case C3DX_ANIMATION_VISIBLE_SETS:
		{
			CLoadDirectory rd(ld);
			while(l1 = rd.next())
			{
				if (l1->id == C3DX_AVS_ONE)
				{
					//CLoadIterator li(l1);
					cStaticVisibilitySet* set = new cStaticVisibilitySet();

					CLoadDirectory rd2(l1);
					CLoadData* l2;
					while(l2 = rd2.next())
					{
						if(l2->id == C3DX_AVS_ONE_HEAD)
						{
							CLoadIterator li(l2);
							visible_sets.push_back(set);
							li>>set->name;
							li>>set->is_all_mesh_in_set;
							li>>set->mesh_in_set;
						}

						if(l2->id == C3DX_AVS_ONE_RAW)
						{
							CLoadIterator li(l2);
							cStaticVisibilityChainGroup* raw = new cStaticVisibilityChainGroup();
							raw->Load(li);
							set->raw_visibility_groups.push_back(raw);
						}
						if(l2->id == C3DX_AVS_ONE_LODS)
						{
							CLoadIterator li(l2);
							for (int j=0; j<set->num_lod; j++)
							{
								li>>size;
								set->visibility_groups[j].resize(size);
								for (int k=0; k<size; k++)
								{	
									int num;
									li>>num;
									if(num>=0)
									{
										xassert(num<set->raw_visibility_groups.size());
										set->visibility_groups[j][k] = set->raw_visibility_groups[num];
									}
								}
							}

						}
					}
				}

			}
		}
		break;
	case C3DX_MATERIAL_GROUP:
		{
			CLoadDirectory rd(ld);
			int cur_mat = 0;
			int mat_num=LoadMaterialsNum(ld);
			materials.resize(mat_num);
			while(l1 = rd.next())
				if (l1->id == C3DX_MATERIAL)
				{
					materials[cur_mat].Load(l1,path_name.c_str(),this, chains_block);
					cur_mat++;
				}

		}
		break;
	case C3DX_LOGOS:
		{
			CLoadDirectory rd(ld);
			while(l1 = rd.next())
				if (l1->id == C3DX_LOGO_COORD)
				{
					CLoadIterator li(l1);
					logos.logos.push_back(cStaticLogos::sLogo());
					cStaticLogos::sLogo& logo = logos.logos.back();
					li>>logo.TextureName;
					li>>logo.rect.min;
					li>>logo.rect.max;
					li>>logo.angle;
				}

		}
		break;
	case 'lods':
		{
			CLoadDirectory dir(ld);
			IF_FIND_DATA('head')
			{
				DWORD size=0;
				rd>>size;
				lods.resize(size);
			}

			for(int i=0;i<lods.size();i++)
			{
				IF_FIND_DIR(i)
					LoadOneLod(dir,lods[i]);
			}

			IF_FIND_DIR('debr')
				LoadOneLod(dir,debris);
		}
		break;
	case C3DX_LIGHTS:
		{
			LoadLights(ld,path_name.c_str(), chains_block);
		}
		break;
	case C3DX_CAMERA:
		{
			CLoadDirectory rd(ld);
			while(l1 = rd.next())
			if (l1->id == C3DX_CAMERA_HEAD)
			{
				CLoadIterator li(l1);
				li>>camera_params.camera_node_num;
				li>>camera_params.fov;
			}
		}
		break;
	case C3DX_OTHER_INFO:
		{
			CLoadIterator li(ld);
			li>>is_lod;

			li>>basement.vertex;
			li>>basement.polygons;

			li>>logic_bound.bound.min;
			li>>logic_bound.bound.max;

			li>>is_logic;

			li>>is_old_model;

			//Не писать эти параметры
			//li>>circle_shadow_enable;
			//li>>circle_shadow_radius;
			//li>>circle_shadow_height;

			li>>is_inialized_bound_box;
			li>>bound_box.min;
			li>>bound_box.max;
			li>>radius;
		}
		break;
	case C3DX_BOUND_SPHERE:
		{
			CLoadIterator li(ld);
			li>>bound_spheres;
		}
		break;
	case C3DX_LOGIC_BOUND_LOCAL:
		LoadLocalLogicBound(ld);
		break;
	}

	ParseEffect();
	CreateDebrises();
	//ClearDebrisNodes();
/**/
	return true;
}

void cStatic3dx::fixUpNodes(StaticChainsBlock& chains_block)
{
    for(int i = 0; i < nodes.size(); ++i){
        cStaticNode& node = nodes[i];
        for(int j = 0; j < node.chains.size(); ++j){
            cStaticNodeChain& chain = node.chains[j];
            chains_block.converter().fixUp(chain.position);
            chains_block.converter().fixUp(chain.rotation);
            chains_block.converter().fixUp(chain.scale);
            chains_block.converter().fixUp(chain.visibility);
        }
    }
}

void cStatic3dx::fixUpMaterials(StaticChainsBlock& chains_block)
{
    for(int i = 0; i < materials.size(); ++i){
        cStaticMaterial& material = materials[i];
        for(int j = 0; j < material.chains.size(); ++j){
            cStaticMaterialAnimation& chain = material.chains[j];
            chains_block.converter().fixUp(chain.opacity);
            chains_block.converter().fixUp(chain.uv);
            chains_block.converter().fixUp(chain.uv_displacement);
        }
    }
}

void cStatic3dx::fixUpLights(StaticChainsBlock& chains_block)
{
    for(int i = 0; i < lights.size(); ++i){
        cStaticLights& light = lights[i];
        for(int j = 0; j < light.chains.size(); ++j){
            cStaticLightsAnimation& chain = light.chains[j];
            chains_block.converter().fixUp(chain.color);
        }
    }
}

void cStatic3dx::CacheBuffersToSystemMem(int ilod)
{
	ONE_LOD& lod=lods[ilod];
	if(lod.sys_vb)
		return;
	lod.sys_vb=new cSkinVertexSysMem[lod.vb.GetNumberVertex()];
	lod.sys_ib=new sPolygon[lod.ib.GetNumberPolygon()];

	sPolygon* pPolygon = gb_RenderDevice->LockIndexBuffer(lod.ib);
	memcpy(lod.sys_ib,pPolygon,lod.ib.GetNumberPolygon()*sizeof(sPolygon));
	gb_RenderDevice->UnlockIndexBuffer(lod.ib);

	int blend_weight=lod.GetBlendWeight();
	cSkinVertex skin_vertex(blend_weight,bump,is_uv2);
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
		cStatic3dx::ONE_LOD& lod=lods[ilod];
		vertex_count+= lod.vb.GetNumberVertex();
		vertex_size+= lod.vb.GetVertexSize() * lod.vb.GetNumberVertex();
	}

	vertex_count+= debris.vb.GetNumberVertex();
	vertex_size+= debris.vb.GetVertexSize() * debris.vb.GetNumberVertex();
}
