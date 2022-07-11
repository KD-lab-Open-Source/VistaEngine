#include "stdafx.h"
#include "Static3dxBase.h"
#include "Saver.h"
#include "Render\inc\FileRead.h"
#include "Serialization\BinaryArchive.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\EnumDescriptor.h"

REGISTER_CLASS(cTempMesh3dx, cTempMesh3dx, "cTempMesh3dx");
REGISTER_CLASS(StaticVisibilityGroup, StaticVisibilityGroup, "StaticVisibilityGroup");

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(FurInfo, FurType, "FurType")
REGISTER_ENUM_ENCLOSED(FurInfo, FUR_CONST, "FUR_CONST")
REGISTER_ENUM_ENCLOSED(FurInfo, FUR_LINEAR, "FUR_LINEAR")
END_ENUM_DESCRIPTOR_ENCLOSED(FurInfo, FurType)

VisibilityGroupIndex VisibilityGroupIndex::BAD;
VisibilitySetIndex VisibilitySetIndex::BAD;
VisibilitySetIndex VisibilitySetIndex::ZERO(0);

Static3dxBase* Static3dxBase::serializedObject_;
const char* Static3dxBase::fileExtention = "3dx";

bool RenderFileRead(const char *fname,char *&buf,int &size)
{
	buf=0; size=0;
	XZipStream f(0);
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

	template<class Data>
	void loadInterpolator(Interpolator3dx<Data>& self, CLoadIterator ld);
	void loadInterpolator(Interpolator3dxBool& self, CLoadIterator ld);

	StaticChainsBlock& chainsBlock_;
}; 

Static3dxBase::Static3dxBase(bool is_logic_)
{
	version = 0;
	maxWeights = 1;
	isBoundBoxInited=false;
	boundBox.min.set(0,0,0);
	boundBox.max.set(0,0,0);
	boundRadius=0;

	is_logic=is_logic_;
	is_lod=false;
	is_old_model=false;

	circle_shadow_enable=circle_shadow_enable_min=OST_SHADOW_REAL;
	circle_shadow_height=-1;
	circle_shadow_radius=10;

	bump=false;
	isUV2=false;
	loaded = false;
	enableFur=false;
}

Static3dxBase::~Static3dxBase()
{
	nodes.clear();

}

bool Static3dxBase::loadOld(CLoadDirectory& dir)
{
	bool loaded=false;
	bool is_new_model=false;
	IF_FIND_DIR(C3DX_ADDITIONAL_LOD1)
		LoadLod(dir,tempMeshLod1_);
	IF_FIND_DIR(C3DX_ADDITIONAL_LOD2)
		LoadLod(dir,tempMeshLod2_);

	dir.rewind();
	while(CLoadData* ld=dir.next()){
		switch(ld->id){
		case C3DX_MAX_WEIGHTS:
			CLoadIterator(ld)>>maxWeights;
			break;
		case C3DX_NON_DELETE_NODE:
			is_new_model = true;
			CLoadIterator(ld)>>nonDeleteNodes;
			break;
		case C3DX_LOGIC_NODE:
			CLoadIterator(ld)>>logicNodes;
			break;
		case C3DX_LOGIC_BOUND_NODE_LIST:
			CLoadIterator(ld)>>boundNodes;
			break;

		case C3DX_LOGOS:
			logos.Load(ld);
			break;
		}

		if((is_logic && ld->id==C3DX_LOGIC_OBJECT) || (!is_logic && ld->id==C3DX_GRAPH_OBJECT)){
			CLoadDirectory ldir(ld);
			LoadInternal(ldir);
			loaded=true;
		}
	}

	is_old_model=!is_new_model;

	if(!loaded){
		if(is_logic){
			//xassertStr("Устаревший формат 3dx, необходимо переэкспортировать: " && 0, file_name);
			return false;
		}
		else{
			dir.rewind();
			LoadInternal(dir);//Поддержка старого формата, потом стереть.
		}
	}

	return true;
}

void Static3dxBase::LoadInternal(CLoadDirectory& rd)
{
	StaticChainsBlock chains_block;

	while(CLoadData* ld=rd.next())
		switch(ld->id){
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
			LoadMeshes(ld, tempMesh_);
			break;
		case C3DX_LIGHTS:
			LoadLights(ld, chains_block);
			break;
		case C3DX_LEAVES:
			LoadLeaves(ld, chains_block);
			break;
		case C3DX_MATERIAL_GROUP:
			{
				int mat_num=LoadMaterialsNum(ld);
				LoadMaterials(ld,mat_num,chains_block);
			}
			break;
		case C3DX_LOGIC_BOUND:
			{
			CLoadIterator it(ld);
			int inode;
			it>>inode;
			it>>boundBox.min;
			it>>boundBox.max;
			}
			break;
		case C3DX_LOGIC_BOUND_LOCAL:
			LoadLocalLogicBound(ld);
			break;
		}
}

void Static3dxBase::LoadChainData(CLoadDirectory rd)
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

void Static3dxBase::LoadGroup(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_ANIMATION_GROUP:
			{
				AnimationGroup ag;
				ag.Load(ld);
				animationGroups_.push_back(ag);
			}
			break;
	}
}

FurInfo::FurInfo()
{
	scale = 1;
	alpha = 1;
}

void FurInfo::serialize(Archive& ar)
{
	ar.serialize(material, "material", "material");
	ar.serialize(scale, "scale", "scale");
	ar.serialize(alpha, "alpha", "alpha");
	ar.serialize(texture, "texture", "texture");
	ar.serialize(normal, "normal", "normal");
}

void AnimationGroup::serialize(Archive& ar)
{
	ar.serialize(name, "name", "^");
	ar.serialize(nodes, "nodes", 0); //!!! 
	ar.serialize(nodesNames, "nodesNames", 0);
	ar.serialize(materialsNames, "materialsNames", 0);
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
				it>>nodesNames;
			}
			break;
		case C3DX_AG_LINK_MATERIAL:
			{
				CLoadIterator it(ld);
				it>>materialsNames;
			}
			break;
	}
}

StaticChainsBlock::StaticChainsBlock()
: positions_(0)
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
	block_.free();

	positions_ = 0;
	rotations_ = 0;
	scales_ = 0;
	bools_ = 0;
	uvs_ = 0;

	num_positions_ = num_rotations_ = num_scales_ = num_bools_ = num_uvs_ = 0;
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

	block_.alloc(num_positions_ * sizeof(SplineDataPosition) + 
		num_rotations_ * sizeof(SplineDataRotation) +
		num_scales_    * sizeof(SplineDataScale) +
		num_bools_     * sizeof(SplineDataBool) +
		num_uvs_       * sizeof(SplineDataUV));

	assert(load_data->size - 5 * (sizeof(int)) == (block_.size()));

	positions_  = reinterpret_cast<SplineDataPosition*>(block_.buffer());
	rotations_	= reinterpret_cast<SplineDataRotation*>(positions_ + num_positions_);
	scales_     = reinterpret_cast<SplineDataScale*>   (rotations_ + num_rotations_);
	bools_		= reinterpret_cast<SplineDataBool*>    (scales_ + num_scales_);
	uvs_		= reinterpret_cast<SplineDataUV*>      (bools_ + num_bools_);

	it.read(block_.buffer(), block_.size());
	assert(reinterpret_cast<unsigned char*>(uvs_ + num_uvs_) == (reinterpret_cast<unsigned char*>(block_.buffer()) + block_.size()));
}

StaticAnimationChain::StaticAnimationChain()
{
	time = 1;
	begin_frame = 0;
	end_frame = 50;
	cycled = true;
}

void StaticAnimationChain::serialize(Archive& ar)
{
	ar.serialize(name, "name", "&Имя");
	ar.serialize(time, "time", "Длительность");
	ar.serialize(begin_frame, "begin_frame", "&Начальный кадр");
	ar.serialize(end_frame, "end_frame", "Конечный кадр");
	ar.serialize(cycled, "cycled", "Зацикленная");
}

void Static3dxBase::LoadChain(CLoadDirectory rd)
{
	animationChains_.clear();
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_AC_ONE:
			{
				CLoadIterator li(ld);
				StaticAnimationChain ac;
				li>>ac.name;
				li>>ac.begin_frame;
				li>>ac.end_frame;
				li>>ac.time;
				li>>ac.cycled;
				animationChains_.push_back(ac);
			}
			break;
	}
}

void Static3dxBase::LoadChainGroup(CLoadDirectory rd)
{
	xassert(visibilitySets_.empty());
	visibilitySets_.push_back(StaticVisibilitySet());
	StaticVisibilitySet& vs = visibilitySets_.back();
	//vs.name="default";
	//vs.is_all_mesh_in_set=true;
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_ACG_ONE:
			{
				CLoadIterator li(ld);
				StaticVisibilityGroup group;
				li>>group.name;
				li>>group.meshes;
				group.is_invisible_list=true;

				vs.visibilityGroups.push_back(group);
			}
			break;
	}

	vs.DummyVisibilityGroup();
}

void Static3dxBase::LoadVisibleSets(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_AVS_ONE:
			LoadVisibleSet(ld);
			break;
	}

}

void Static3dxBase::LoadVisibleSet(CLoadDirectory rd)
{
	visibilitySets_.push_back(StaticVisibilitySet());
	StaticVisibilitySet& vs = visibilitySets_.back();
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_AVS_ONE_NAME:
			{
				CLoadIterator li(ld);
				li>>vs.name;
			}
			break;
		case C3DX_AVS_ONE_OBJECTS:
			{
				CLoadIterator li(ld);
				li>>vs.meshes;
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
							StaticVisibilityGroup group;

							li >> group.name;
							li >> group.meshes;

							vs.visibilityGroups.push_back(group);
						}
						break;
				}
			}
	}

	vs.DummyVisibilityGroup();
}

ChainConverter::ChainConverter(StaticChainsBlock& chains_block)
: chainsBlock_(chains_block)
{

}

template<class Data>
void ChainConverter::loadInterpolator(Interpolator3dx<Data>& self, CLoadIterator ld)
{
	typedef Interpolator3dx<Data> Interpolator;
	Interpolator::Values& curve = self.values;

	int size=0;
	ld>>size;
	xassert(size>0 && size<255);
	curve.resize(size);
	for(int i=0;i<size;i++)
	{
		Interpolator::Data& one=curve[i];
		int type=0;
		ld>>type;
		one.itpl=(ITPL)type;
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

		for(int j=0;j<one_size*Interpolator::Data::size;j++)
			ld>>one.a[j];
	}
}

void ChainConverter::loadInterpolator(Interpolator3dxBool& self, CLoadIterator ld)
{
	Interpolator3dxBool::Values& curve = self.values;

	int size=0;
	ld>>size;
	xassert(size>0 && size<255);
	curve.resize(size);
	for(int i=0;i<size;i++){
		SplineDataBool& one=curve[i];
		ld>>one.tbegin;
		float tsize=0;
		ld>>tsize;
		xassert(tsize>1e-5f);
		one.inv_tsize=1/tsize;

		bool value;
		ld>>value;
		one.value = value;
	}
}

void Static3dxBase::LoadCamera(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_CAMERA_HEAD:
			{
				CLoadIterator it(ld);
				it>>cameraParams.camera_node_num;
				it>>cameraParams.fov;
			}
			break;
	}
}

void Static3dxBase::LoadNodes(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_NODE:
			{
				nodes.push_back(StaticNode());
				StaticNode& node=nodes.back();
				node.LoadNode(ld, chains_block);
				if(nodes.size()>1)
					xassert(node.iparent>=0);
			}
			break;
	}
}

void StaticNodeAnimation::serialize(Archive& ar)
{
	ar.serialize(scale, "scale", "scale");
	ar.serialize(position, "position", "position");
	ar.serialize(rotation, "rotation", "rotation");
	ar.serialize(visibility, "visibility", "visibility");
}

void StaticNodeAnimation::referencePose(Mats& pos) 
{
	float xyzs[4];
	scale.Interpolate(0,&pos.scale(),0);
	position.Interpolate(0,(float*)&pos.trans(),0);
	rotation.Interpolate(0,xyzs,0);
	pos.rot().set(xyzs[3],-xyzs[0],-xyzs[1],-xyzs[2]);
}

StaticNode::StaticNode()
{
	inv_begin_pos=MatXf::ID;
}

void StaticNode::serialize(Archive& ar)
{
	ar.serialize(name, "name", "&name");
	ar.serialize(inode, "inode", "inode");
	ar.serialize(iparent, "iparent", "iparent");
	ar.serialize(chains, "chains", "chains");
	ar.serialize(inv_begin_pos, "inv_begin_pos", "inv_begin_pos");
}

void StaticNode::LoadNode(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
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

void StaticNode::LoadNodeChain(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	chains.push_back(StaticNodeAnimation());
	StaticNodeAnimation& chain=chains.back();

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
}

void Static3dxBase::LoadMeshes(CLoadDirectory rd,TempMeshes& temp_mesh)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_MESH:
			{
				TempMesh mesh=new cTempMesh3dx;
				mesh->Load(ld);
				if(!mesh->vertex_uv2.empty())
					isUV2=true;

				if(mesh->name.empty())
				{
					int inode=mesh->inode;
					mesh->name=nodes[inode].name;
				}

				temp_mesh.push_back(mesh);
			}
			break;
	}

	//DummyVisibilitySet(temp_mesh);
	//BuildProgramLod(temp_mesh); //!!!
}

void cTempBone::serialize(Archive& ar)
{
	ar.serializeArray(weight, "weight", "weight");
	ar.serializeArray(inode, "inode", "inode");
}

cTempMesh3dx::cTempMesh3dx() 
{
	inode = -1;
	imaterial = 0;
	visibilityAnimated = false;
}

void cTempMesh3dx::serialize(Archive& ar)
{
	ar.serialize(name, "name", "&name");
	ar.serialize(vertex_pos, "vertex_pos", "vertex_pos");
	ar.serialize(vertex_norm, "vertex_norm", "vertex_norm");
	ar.serialize(vertex_uv, "vertex_uv", "vertex_uv");
	ar.serialize(vertex_uv2, "vertex_uv2", "vertex_uv2");
	ar.serialize(bones, "bones", "bones");
	ar.serialize(polygons, "polygons", "polygons");

	ar.serialize(inode, "inode", "inode");
	ar.serialize(imaterial, "imaterial", "imaterial");
	ar.serialize(visibilityAnimated, "visibilityAnimated", "visibilityAnimated");
	ar.serialize(inode_array, "inode_array", "inode_array");
	//ar.serialize(visible_group, "visible_group", "visible_group");

	if(ar.isInput()){
		if(imaterial < 0)
			imaterial = 0;
		if(!vertex_uv2.empty())
			Static3dxBase::serializedObject()->isUV2 = true;

		if(name.empty())
			name = Static3dxBase::serializedObject()->nodes[inode].name;

		deleteSingularPolygon();
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
					for(ibone=0;ibone < MAX_BONES;ibone++)
					{
						it>>p.weight[ibone];
						it>>p.inode[ibone];

						float& weight = p.weight[ibone];
						weight = clamp(weight, 0.0f, 1.0f);
						sum += weight;

						if(p.inode[ibone]>=0)
							bones_inode[p.inode[ibone]]=1;
						else
							xassert(p.weight[ibone]==0);
					}

					//				xassert(sum>=0.999f && sum<1.001f);//Потом опять вставить, закомментировано для демы

					sum=1/sum;
					for(ibone=0;ibone<MAX_BONES;ibone++)
						p.weight[ibone]*=sum;

				}

				inode_array.clear();
				BonesInode::iterator itn;
				FOR_EACH(bones_inode,itn)
					inode_array.push_back(itn->first);
			}
			break;
	}

	deleteSingularPolygon();

	xassert(vertex_pos.size()==vertex_norm.size());
	xassert(vertex_uv.empty() || vertex_pos.size()==vertex_uv.size());
	xassert(vertex_uv2.empty() || vertex_pos.size()==vertex_uv2.size());
}

void cTempMesh3dx::deleteSingularPolygon()
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

int Static3dxBase::LoadMaterialsNum(CLoadDirectory rd)
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

void Static3dxBase::LoadMaterials(CLoadDirectory rd,int num_materials, StaticChainsBlock& chains_block)
{
	int cur_mat=0;
	materials.resize(num_materials);
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_MATERIAL:
			{
				materials[cur_mat].Load(ld,this, chains_block);
				cur_mat++;
			}
			break;
	}

	//LoadFurMap(path_name);
}

StaticMaterial::StaticMaterial()
{
	ambient=Color4f(0,0,0);
	diffuse=Color4f(1,1,1);
	specular=Color4f(0,0,0);
	opacity=0;
	specular_power=0;

	is_skinned=false;
	pBumpTexture=0;
	pReflectTexture=0;
	pSpecularmap=0;
	pSecondOpacityTexture=0;
	animation_group_index=-1;
	reflect_amount=1.0f;
	is_reflect_sky=false;
	is_opacity_texture=false;
	no_light=false;
	tiling_diffuse=TILING_U_WRAP|TILING_V_WRAP;
	transparencyType=TRANSPARENCY_FILTER;
	is_big_ambient=false;

	pFurmap = 0;
	fur_scale=1.0f;
	fur_alpha=0.3f;
	fur_alpha_type = FurInfo::FUR_CONST;
	 
	texturesCreated = false;
}

StaticMaterial::~StaticMaterial()
{
	xassert(!texturesCreated);
}

void StaticMaterial::serialize(Archive& ar)
{
	ar.serialize(name, "name", "&name");
	ar.serialize(ambient, "ambient", "ambient");
	ar.serialize(diffuse, "diffuse", "diffuse");
	ar.serialize(specular, "specular", "specular");
	ar.serialize(opacity, "opacity", "opacity");
	ar.serialize(specular_power, "specular_power", "specular_power");

	specular_power = max(specular_power,1.0f);

	ar.serialize(is_opacity_texture, "is_opacity_texture", "is_opacity_texture");
	ar.serialize(tex_diffuse, "tex_diffuse", "tex_diffuse"); 
	ar.serialize(tiling_diffuse, "tiling_diffuse", "tiling_diffuse");
	ar.serialize((int&)transparencyType, "|transparencyType|transparency_type", "transparencyType");
	ar.serialize(is_skinned, "is_skinned", "is_skinned");
	ar.serialize(tex_skin, "tex_skin", "tex_skin");
	ar.serialize(tex_bump, "tex_bump", "tex_bump");
	ar.serialize(tex_reflect, "tex_reflect", "tex_reflect");
	ar.serialize(reflect_amount, "reflect_amount", "reflect_amount");
	ar.serialize(is_reflect_sky, "is_reflect_sky", "is_reflect_sky");
	ar.serialize(tex_specularmap, "tex_specularmap", "tex_specularmap"); 
	ar.serialize(tex_self_illumination, "tex_self_illumination", "tex_self_illumination"); 
	ar.serialize(tex_secondopacity, "tex_secondopacity", "tex_secondopacity"); 

	ar.serialize(no_light, "no_light", "no_light");
	ar.serialize(animation_group_index, "animation_group_index", "animation_group_index");

	ar.serialize(chains, "chains", "chains");

	ar.serialize(fur_scale, "fur_scale", "fur_scale");
	ar.serialize(fur_alpha, "fur_alpha", "fur_alpha");
	ar.serialize(tex_furmap, "tex_furmap", "tex_furmap");
	ar.serialize(tex_furnormalmap, "tex_furnormalmap", "tex_furnormalmap");
	ar.serialize((int&)fur_alpha_type, "fur_alpha_type", "fur_alpha_type");

	diffuse.a=opacity;

	float min_ambient=0.65f;
	is_big_ambient=ambient.r>min_ambient || ambient.g>min_ambient || ambient.b>min_ambient;
}

void StaticMaterial::Load(CLoadDirectory rd,Static3dxBase* pStatic, StaticChainsBlock& chains_block)
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
				it>>v;ambient=Color4f(v.x,v.y,v.z);
				it>>v;diffuse=Color4f(v.x,v.y,v.z);
				it>>v;specular=Color4f(v.x,v.y,v.z);
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
				transparencyType = (TransparencyType)transparency;

				if(!name.empty()){
					switch(slot){
					case TEXMAP_DI:
						tex_diffuse=name;
						tiling_diffuse=tiling;
						break;
					case TEXMAP_FI:
						is_skinned=true;
						tex_skin=name;
						break;
					case TEXMAP_BU:
						tex_bump=name;
						break;
					case TEXMAP_RL:
						tex_reflect=name;
						reflect_amount=amount;
						break;
					case TEXMAP_OP:
						is_opacity_texture=true;
						break;
					case TEXMAP_SI:
						tex_self_illumination=name;
						break;
					case TEXMAP_SP:
						tex_specularmap=name;
						break;
					case TEXMAP_DP:
						tex_secondopacity=name;
						break;
					}
				}
			}
			break;
		case C3DX_NODE_CHAIN:
			chains.push_back(StaticMaterialAnimation());
			chains.back().Load(ld, chains_block);
			break;
		case C3DX_MATERIAL_STATIC_FURINFO:
			{
				CLoadIterator it(ld);
				it>>fur_scale;
				it>>fur_alpha;
				it>>tex_furmap;
				it>>tex_furnormalmap;
				int tmp=FurInfo::FUR_CONST;
				it>>tmp;
				fur_alpha_type=(FurInfo::FurType)tmp;
			}
			break;
	}
	diffuse.a=opacity;

	float min_ambient=0.65f;
	is_big_ambient=ambient.r>min_ambient || ambient.g>min_ambient || ambient.b>min_ambient;
}

void Static3dxBase::LoadLocalLogicBound(CLoadIterator it)
{
	int inode=0;
	sBox6f bound;
	it>>inode;
	it>>bound.min;
	it>>bound.max;

	if(localLogicBounds.empty())
	{
		xassert(!nodes.empty());
		sBox6f bound;
		localLogicBounds.resize(nodes.size(),bound);
	}

	xassert(inode>=0 && inode<localLogicBounds.size());
	localLogicBounds[inode]=bound;
}

void Static3dxBase::LoadLights(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_LIGHT:
			{
				lights.push_back(StaticLight());
				lights.back().Load(ld, chains_block);
				break;
			}
	}
}
void Static3dxBase::LoadLeaves(CLoadDirectory rd, StaticChainsBlock& chains_block)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_LEAF:
			{
				leaves.push_back(StaticLeaf());
				leaves.back().Load(ld);
				break;
			}
	}
}

StaticLeaf::StaticLeaf()
{
	inode=-1;
	size = -1;
	pTexture=0;
}

void StaticLeaf::serialize(Archive& ar)
{
	ar.serialize(inode, "inode", "inode");
	ar.serialize(color, "color", "color");
	ar.serialize(size, "size", "size");
	ar.serialize(texture, "texture", "texture");
	ar.serialize(lods, "lods", "lods");
}

void StaticLeaf::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
		switch(ld->id)
	{
		case C3DX_LEAF_HEAD:
			{
				CLoadIterator it(ld);
				it>>inode;
				Vect3f c;
				it>>c;
				color.set(c.x,c.y,c.z,1);
				int atten_start;
				it>>atten_start;
				it>>size;
				break;
			}
		case C3DX_LEAF_TEXTURE:
			{
				CLoadIterator it(ld);
				it>>texture;
			}
			break;
	}
}

StaticLight::StaticLight()
{
	inode=-1;
	atten_start=-1;
	atten_end=-1;
	pTexture=0;
}

void StaticLight::serialize(Archive& ar)
{
	ar.serialize(inode, "inode", "inode");
	ar.serialize(color, "color", "color");
	ar.serialize(atten_start, "atten_start", "atten_start");
	ar.serialize(atten_end, "atten_end", "atten_end");

	ar.serialize(chains, "chains", "chains"); // chains_block

	ar.serialize(texture, "texture", "texture");
}

void StaticLight::Load(CLoadDirectory rd, StaticChainsBlock& chains_block)
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
			chains.push_back(StaticLightAnimation());
			chains.back().Load(ld, chains_block);
			break;
		case C3DX_LIGHT_TEXTURE:
			CLoadIterator(ld)>>texture;
			break;

	}
}

void StaticMaterialAnimation::serialize(Archive& ar)
{
	ar.serialize(opacity, "opacity", "opacity");
	ar.serialize(uv, "uv", "uv");
	ar.serialize(uv_displacement, "uv_displacement", "uv_displacement");
}

void StaticMaterialAnimation::Load(CLoadDirectory rd, StaticChainsBlock& chains_block)
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

void StaticLightAnimation::serialize(Archive& ar)
{
	ar.serialize(color, "color", "color");
}

void StaticLightAnimation::Load(CLoadDirectory rd, StaticChainsBlock& chains_block)
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

StaticVisibilitySet::StaticVisibilitySet()
{
	name="default";
}

void StaticVisibilitySet::serialize(Archive& ar)
{
	ar.serialize(name, "name", "^");
	ar.serialize(visibilityGroups, "visibilityGroups", "Группы видимости");
}

void StaticVisibilitySet::DummyVisibilityGroup()
{
	if(visibilityGroups.empty()){
		StaticVisibilityGroup group;
		group.name="default";
		group.meshes=meshes;
		visibilityGroups.push_back(group);
	}
}

const char* StaticVisibilitySet::GetVisibilityGroupName(VisibilityGroupIndex vgi)
{
	xassert(vgi >= 0 && vgi < visibilityGroups.size());
	return visibilityGroups[vgi].name.c_str();
}

VisibilityGroupIndex StaticVisibilitySet::GetVisibilityGroupIndex(const char* group_name)
{
	int size=GetVisibilityGroupNumber();
	for(int i=0;i<size;i++)	{
		const char* name=GetVisibilityGroupName(VisibilityGroupIndex(i));
		if(strcmp(group_name,name)==0)
			return VisibilityGroupIndex(i);
	}
	return VisibilityGroupIndex::BAD;
}

StaticVisibilityGroup::StaticVisibilityGroup()
{
	visibility=0;
	is_invisible_list=false;
}

void StaticVisibilityGroup::serialize(Archive& ar)
{
	ar.serialize(name, "name", "^");
	ar.serialize(visibility, "visibility", 0); // !!!
	ar.serialize(meshes, "meshes", 0);
	ar.serialize(is_invisible_list, "is_invisible_list", 0); // !!!
	ar.serialize(visibleNodes, "visibleNodes", 0); // !!!
}

void StaticVisibilityGroup::Load(CLoadIterator &ld)
{
	ld>>visibility;
	ld>>name;
	ld>>is_invisible_list;
	int lod; // temp
	ld>>lod;
	ld>>meshes;
	DWORD size=0;
	ld>>size;
	visibleNodes.resize(size);
	for(int i=0;i<size;i++)
	{
		bool b;
		ld>>b;
		visibleNodes[i]=b;
	}
}

sLogo::sLogo(const char* name)
{
	enabled = false;
	TextureName = name;
	angle = 0;
	rect.set(0,0,0,0);
}

void sLogo::serialize(Archive& ar)
{
	ar.serialize(rect, "rect", "rect");
	ar.serialize(TextureName, "TextureName", "TextureName");
	ar.serialize(angle, "angle", "angle");
}

void StaticLogos::serialize(Archive& ar)
{
	ar.serialize(logos, "logos", "logos");
}

void StaticLogos::Load(CLoadDirectory it)
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

void Static3dxBase::serialize(Archive& ar)
{
	serializedObject_ = this;
	
	if(ar.isOutput() && !ar.isEdit())
		version = VERSION;
	ar.serialize(version, "version", "version");

	if(ar.inPlace())
		ar.serialize(fileName_, "fileName", "fileName"); // Имя самого себя лучше не писать

	ar.serialize(maxWeights, "maxWeights", "maxWeights");
	ar.serialize(nonDeleteNodes, "nonDeleteNodes", "nonDeleteNodes");
	ar.serialize(logicNodes, "logicNodes", "logicNodes");
	ar.serialize(boundNodes, "boundNodes", "bound_node");

	ar.serialize(animationGroups_, "animationGroups", "animationGroups");
	ar.serialize(animationChains_, "animationChains", "animationChains");
	ar.serialize(nodes, "nodes", "nodes");
	ar.serialize(visibilitySets_, "visibilitySets", "visibilitySets");
	ar.serialize(materials, "materials", "materials"); 

	ar.serialize(isBoundBoxInited, "isBoundBoxInited", "isBoundBoxInited");
	ar.serialize(boundRadius, "|boundRadius|radius", "boundRadius");
	ar.serialize(boundBox, version < 2 ? "logicBound" : "boundBox", "boundBox"); // CONVERSION 18.02.08	
	ar.serialize(localLogicBounds, "localLogicBounds", "localLogicBounds");
	ar.serialize(boundSpheres, "boundSpheres", "boundSpheres");

	ar.serialize(logos, "logos", "logos");
	ar.serialize(effects, "effects", "effects");
	ar.serialize(lights, "lights", "lights"); 
	ar.serialize(leaves, "leaves", "leaves"); 

	ar.serialize(cameraParams, "cameraParams", "cameraParams");

	ar.serialize(bump, "bump", "bump");
	ar.serialize(isUV2, "isUV2", "isUV2");
	ar.serialize(enableFur, "enableFur", "enableFur");

	ar.serialize(tempMesh_, "tempMesh", "tempMesh");
	ar.serialize(tempMeshLod1_, "tempMeshLod1", "tempMeshLod1");
	ar.serialize(tempMeshLod2_, "tempMeshLod2", "tempMeshLod2");

	serializedObject_ = 0;
}

CameraParams::CameraParams()
{
	camera_node_num = -1;
	fov = 0;
}

void CameraParams::serialize(Archive& ar)
{
	ar.serialize(camera_node_num, "camera_node_num", "camera_node_num");
	ar.serialize(fov, "fov", "fov");
}

void Static3dxBase::BoundSphere::serialize(Archive& ar)
{
	ar.serialize(node_index, "node_index", "node_index");
	ar.serialize(position, "position", "position");
	ar.serialize(radius, "radius", "radius");
}

StaticEffect::StaticEffect()
{
	node = 0;
	is_cycled = false;
}

void StaticEffect::serialize(Archive& ar)
{
	ar.serialize(node, "node", "node");
	ar.serialize(is_cycled, "is_cycled", "is_cycled");
	ar.serialize(file_name, "file_name", "file_name");
}

void cTempVisibleGroup::serialize(Archive& ar)
{
	ar.serialize(visibilitySet, "visible_set", "visible_set");
	ar.serialize(visibilities, "visibilities", "visibilities");
	ar.serialize(begin_polygon, "begin_polygon", "begin_polygon");
	ar.serialize(num_polygon, "num_polygon", "num_polygon");
}

void Static3dxBase::LoadLod(CLoadDirectory dir,TempMeshes& temp_mesh_lod)
{
	IF_FIND_DIR(C3DX_MESHES)
	{
		temp_mesh_lod.clear();
		while(CLoadData* ld=dir.next())
			switch(ld->id)
		{
			case C3DX_MESH:
				{
					TempMesh mesh=new cTempMesh3dx;
					mesh->Load(ld);
					if(!mesh->vertex_uv2.empty())
						isUV2=true;

					if(mesh->name.empty())
					{
						int inode=mesh->inode;
						mesh->name=nodes[inode].name;
					}

					temp_mesh_lod.push_back(mesh);
				}
				break;
		}
	}
}

bool Static3dxBase::load(const char* fileName)
{
	fileName_ = fileName;
	BinaryIArchive ia(0);
	if(ia.open((setExtention(fileName_.c_str(), fileExtention).c_str())))
		return ia.serialize(*this, is_logic ? "logic3dx" : "graphics3dx", 0); // В конверсиях из старого формата может не быть logic3dx

	CLoadDirectoryFileRender rd;
	if(rd.Load(fileName_.c_str()) && loadOld(rd))
		return true;
	return false;
}

Static3dxFile::Static3dxFile()
{
	graphics3dx_ = new Static3dxBase(false);
	logic3dx_ = new Static3dxBase(true);
	badLogic3dx_ = false;
}

Static3dxFile::~Static3dxFile()
{
	delete graphics3dx_;
	delete logic3dx_;
}

void Static3dxFile::load(const char* fileName)
{
	graphics3dx_->load(fileName);
	badLogic3dx_ = !logic3dx_->load(fileName);
}

void Static3dxFile::save(const char* fileName)
{
	BinaryOArchive oa(setExtention(fileName, Static3dxBase::fileExtention).c_str());
	serialize(oa);
}

void Static3dxFile::serialize(Archive& ar)
{
	ar.serialize(*graphics3dx_, "graphics3dx", "graphics3dx");
	if(!badLogic3dx_)
		ar.serialize(*logic3dx_, "logic3dx", "logic3dx");
}

void Static3dxFile::convertFile(const char* fname)
{
	Static3dxFile file;
	file.load(fname);
	file.save(fname);
}

