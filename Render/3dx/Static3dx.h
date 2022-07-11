#ifndef __STATIC3DX_H_INCLUDED__
#define __STATIC3DX_H_INCLUDED__

#include "IRenderDevice.h"
#include "Interpolator3DX.h"

struct cStaticNodeChain{
	Interpolator3dxScale    scale;
	Interpolator3dxPosition position;
	Interpolator3dxRotation rotation;
	Interpolator3dxBool     visibility;
};

class ChainConverter;
struct cStaticNode
{
	string name;
	int inode;
	int iparent;
	vector<cStaticNodeChain> chains;
	MatXf inv_begin_pos;

	void LoadNode(CLoadDirectory ld, StaticChainsBlock& chains_block);
	void LoadNodeChain(CLoadDirectory ld, StaticChainsBlock& chains_block);

	void SaveNode(Saver &s, StaticChainsBlock& chains_block);
	void SaveNodeChain(Saver &s, cStaticNodeChain& chain, StaticChainsBlock& chains_block);

};

struct cAnimationChain
{
	string name;
	float time;
};

struct cTempVisibleGroup
{
	int visible_set;//К какому множеству принадлежит.
	DWORD visible;//Видимость в соответствующих группах видимости. Битовыми флажками. Не более 32 групп видимости.
	int begin_polygon;
	int num_polygon;
};

struct cStaticIndex
{
	int		offset_polygon;
	int		num_polygon;

	int		offset_vertex;
	int		num_vertex;

	int		imaterial;

	enum
	{
		max_index=20,//Максимальное количество матриц
	};
	//node_index с какими матрицами связанны текущие полигоны,
	//не более max_index
	vector<int> node_index;
	vector<cTempVisibleGroup> visible_group;
};

struct AnimationGroup
{
	string name;
	vector<int> nodes;
	vector<string> temp_nodes_name;
	vector<string> temp_materials_name;

	void Load(CLoadDirectory rd);
};

struct cStaticMaterialAnimation
{
	Interpolator3dx<1> opacity;
	Interpolator3dx<6> uv;
	Interpolator3dx<6> uv_displacement;

	void Load(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void Save(Saver &s, const StaticChainsBlock& chains_block);
};

class cStatic3dx;
class cStaticSimply3dx;

struct cStaticMaterial
{
	enum TEXTURE_TILING
	{
		TILING_U_WRAP=   (1<<0),
		TILING_V_WRAP=   (1<<1),
//		TILING_U_MIRROR= (1<<2),
//		TILING_V_MIRROR= (1<<3),
	};
	enum TRANSPARENCY_TYPE
	{
		TRANSPARENCY_SUBSTRACTIVE = 0,
		TRANSPARENCY_ADDITIVE     = 1,
		TRANSPARENCY_FILTER		  = 2,
	};
	string name;

	sColor4f ambient;
	sColor4f diffuse;
	sColor4f specular;
	float opacity;
	float specular_power;

	bool is_opacity_texture;
	string tex_diffuse;
	int tiling_diffuse;//TEXTURE_TILING
	int transparency_type;
	bool is_skinned;
	string tex_skin;
	string tex_bump;
	cTexture* pBumpTexture;
	string tex_reflect;
	cTexture* pReflectTexture;
	float reflect_amount;
	bool is_reflect_sky;

	cTexture* pSpecularmap;
	string tex_specularmap;

	string tex_self_illumination;

	cTexture* pSecondOpacityTexture;
	string  tex_secondopacity;

	int animation_group_index;
	bool no_light;
	bool is_big_ambient;

	vector<cStaticMaterialAnimation> chains;

	cStaticMaterial();
	~cStaticMaterial();
	void Load(CLoadDirectory rd,const char* path_name, cStatic3dx* pStatic, StaticChainsBlock& chains_block);
	void Save(Saver &s, const StaticChainsBlock& chains_block);
	void SaveTextureMap(Saver& s, string &name, int slot);
};

struct cTempBone
{
	enum
	{
		max_bones=4,
	};

	float weight[max_bones];
	int inode[max_bones];
};


struct cTempMesh3dx
{
	string name;
	vector<Vect3f> vertex_pos;
	vector<Vect3f> vertex_norm;
	vector<Vect2f> vertex_uv;
	vector<Vect2f> vertex_uv2;
	vector<cTempBone> bones;

	vector<sPolygon> polygons;

	int inode;
	int imaterial;

	vector<int> inode_array;//На какие ноды ссылается скиннинг в этом mesh
	vector<cTempVisibleGroup> visible_group;

	void Load(CLoadDirectory rd);
	void FillVisibleGroup(int visible_set,DWORD visible);

	int CalcMaxBonesPerVertex();

	void Merge(vector<cTempMesh3dx*>& meshes,class cStatic3dx* pStatic);
	void OptimizeMesh();

	void DeleteSingularPolygon();
};

struct cStaticBasement
{
	vector<sPolygon> polygons;
	vector<Vect3f> vertex;

	void Load(CLoadDirectory rd);
};

struct cStaticLogicBound
{
protected:
	int inode;
public:
	sBox6f bound;

	cStaticLogicBound()
	{
		inode=0;
		bound.min=Vect3f::ZERO;
		bound.max=Vect3f::ZERO;
	}

	void Load(CLoadIterator it);
};

struct cStaticLogos
{
	struct sLogo
	{
		sLogo()
		{
			angle = 0;
		}
		sRectangle4f rect;
		string TextureName;
		float angle;
	};
	vector<sLogo> logos;
	void Load(CLoadDirectory it);
	bool GetLogoPosition(const char* fname, sRectangle4f* rect, float& angle);
};

struct cStaticVisibilityChainGroup
{
	DWORD visible_shift;
	string name;

	vector<string>		temp_visible_object;
	bool is_invisible_list;//For old format
	
	vector<bool>		visible_nodes;
	int lod;

	cStaticVisibilityChainGroup();
	void Save(Saver &s);
	void Load(CLoadIterator &ld);
};

struct C3dxVisibilitySet
{
	int iset;
	C3dxVisibilitySet():iset(-1){}
	explicit C3dxVisibilitySet(int iset_):iset(iset_){}

	bool operator ==(C3dxVisibilitySet s2){return iset==s2.iset;}

	static C3dxVisibilitySet BAD;
	static C3dxVisibilitySet ZERO;
};

struct C3dxVisibilityGroup
{
	int igroup;
	C3dxVisibilityGroup():igroup(-1){}
	explicit C3dxVisibilityGroup(int igroup_):igroup(igroup_){}
	bool operator ==(C3dxVisibilityGroup g2){return igroup==g2.igroup;}
	bool operator !=(C3dxVisibilityGroup g2){return igroup!=g2.igroup;}
	static C3dxVisibilityGroup BAD;
};

struct cStaticVisibilitySet
{
	string name;
	enum 
	{
		num_lod=3,
	};

	vector<string>		mesh_in_set;
	bool is_all_mesh_in_set;//Для совместимости со старым форматом.
	vector<cStaticVisibilityChainGroup*> raw_visibility_groups;//Не менее одной группы
	vector<cStaticVisibilityChainGroup*> visibility_groups[num_lod];//3 loda забиты у любого объекта.

	cStaticVisibilitySet();
	~cStaticVisibilitySet();
	void DummyVisibilityGroup();
	bool BuildVisibilityGroupsLod();
	bool IsLod();

	int GetVisibilityGroupNumber(){return visibility_groups[0].size();};
	const char* GetVisibilityGroupName(C3dxVisibilityGroup group);
	C3dxVisibilityGroup GetVisibilityGroupIndex(const char* group_name);
};

struct cStaticEffect
{
	int node;
	bool is_cycled;
	string file_name;
};

class ChainsBlock;
struct cStaticLightsAnimation
{
	Interpolator3dx<4> color;

	void Load(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void Save(Saver &s, StaticChainsBlock& chains_block);
};

struct cStaticLights
{
	int inode;
	sColor4f color;
	float atten_start;
	float atten_end;
	vector<cStaticLightsAnimation> chains;
	cTexture* pTexture;

	cStaticLights()
	{
		inode=-1;
		atten_start=-1;
		atten_end=-1;
		pTexture=NULL;
	}

	void Load(CLoadDirectory rd,const char* path_name,cStatic3dx* pStatic, StaticChainsBlock& chains_block);
	void Save(Saver& s, StaticChainsBlock& chains_block);
};

struct cSkinVertexSysMem
{
	Vect3f pos;
	sColor4c index;
	BYTE weight[4];
};
struct CameraParams
{
	CameraParams()
	{
		camera_node_num = -1;
		fov = 0;
	}
	int camera_node_num;
	float fov;
};
class cSkinVertexSysMemI
{
public:
	cSkinVertexSysMem* begin;
	cSkinVertexSysMem* cur;
public:
	cSkinVertexSysMemI():cur(0),begin(0){};
	void SetVB(cSkinVertexSysMem* p_){begin=p_;}
	void Select(int n){cur=begin+n;}
	Vect3f& GetPos(){return cur->pos;}
	BYTE& GetWeight(int idx){return cur->weight[idx];};
	sColor4c& GetIndex(){return cur->index;}
};

//Класс, хранящий в себе неизменные данные для всех 3dx объектов.
//Для Шуры специально - ничего писать сюда извне НЕЛЬЗЯ.
class cStatic3dx:public cUnknownClass
{
	bool loaded;
public:
	cStatic3dx(bool is_logic_,const char* file_name_);
	~cStatic3dx();

	bool Load(CLoadDirectory& rd);
	string file_name;

	StaticChainsBlock chains_block;

	vector<cStaticNode> nodes;
	vector<cAnimationChain> animation_chain;

	bool is_lod;

	vector<cStaticVisibilitySet*> visible_sets;//Не менее одного 
	vector<cStaticMaterial> materials;
	vector<AnimationGroup> animation_group;

	cStaticBasement basement;
	cStaticLogicBound logic_bound;
	vector<sBox6f> local_logic_bounds;//==nodes.size();
	cStaticLogos logos;
	bool is_logic;
	vector<cStaticEffect> effects;
	vector<cStaticLights> lights;
	vector<cStaticSimply3dx*> debrises;

	class cVisError& errlog();

	cTexture* LoadTexture(const char* name,char* mode=NULL);//То же что и GetTexLibrary()->GetElement, но с более развернутым сообщением об ошибке.
	
	void GetTextureNames(vector<string>& names) const;

	bool is_old_model;

	c3dx::OBJECT_SHADOW_TYPE circle_shadow_enable;
	c3dx::OBJECT_SHADOW_TYPE circle_shadow_enable_min;
	int circle_shadow_height;
	float circle_shadow_radius;

	bool is_inialized_bound_box;
	sBox6f bound_box;
	float radius;

	struct BoundSphere
	{
		int node_index;
		Vect3f position;
		float radius;
	};

	vector<BoundSphere> bound_spheres;

	struct ONE_LOD
	{
		sPtrIndexBuffer		ib;
		sPtrVertexBuffer	vb;
		int					blend_indices;//количество костей в vb
		vector<cStaticIndex> skin_group;

		cSkinVertexSysMem* sys_vb;
		sPolygon* sys_ib;

		ONE_LOD()
		{
			blend_indices=1;
			sys_vb=NULL;
			sys_ib=NULL;
		}
		int GetBlendWeight()
		{
			if(blend_indices==1)
				return 0;
			return blend_indices;
		}
	};

	vector<ONE_LOD> lods;//1 или 3 лода.

	ONE_LOD	debris;

	bool				bump;
	bool				is_uv2;
	CameraParams		camera_params;

	int GetBlendWeight(int lod)
	{
		return lods[lod].GetBlendWeight();
	}
	bool loadCacheData(const char* name);
	void saveCacheData(const char* name);
	bool GetLoaded(){return loaded;}
	void SetLoaded(){loaded = true;}

	void CacheBuffersToSystemMem(int ilod);
	void GetVBSize(int& vertex_count,int& vertex_size);
protected:
	void CalculateVisibleShift();

	void SaveOneLod(Saver& saver,ONE_LOD& lod);
	void LoadOneLod(CLoadDirectory& dir,ONE_LOD& lod);
    void fixUpNodes(StaticChainsBlock& chains_block);
    void fixUpMaterials(StaticChainsBlock& chains_block);
    void fixUpLights(StaticChainsBlock& chains_block);

	void LoadInternal(CLoadDirectory& rd);

	void LoadChainData(CLoadDirectory rd);
	void LoadGroup(CLoadDirectory rd);
	void LoadChain(CLoadDirectory rd);
	void LoadChainGroup(CLoadDirectory rd);
	void LoadVisibleSets(CLoadDirectory rd);
	void LoadVisibleSet(CLoadDirectory rd);

	void LoadNodes(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void LoadCamera(CLoadDirectory rd);

	void LoadMeshes(CLoadDirectory rd,vector<cTempMesh3dx*>& temp_mesh);
	void BuildMeshes(vector<cTempMesh3dx*>& temp_mesh);
	void BuildMeshesLod(vector<cTempMesh3dx*>& temp_mesh,int ilod,bool debris);
	void BuildMeshes(vector<cTempMesh3dx*>& temp_mesh,ONE_LOD& lod,bool merge_mesh);
	void LoadLights(CLoadDirectory rd,const char* path_name, StaticChainsBlock& chains_block);

	int LoadMaterialsNum(CLoadDirectory rd);
	void LoadMaterials(CLoadDirectory rd,const char* path_name,int num_materials);
	void LoadLocalLogicBound(CLoadIterator it);
	void SaveLocalLogicBound(Saver& s);

	void BuildBuffers(vector<cTempMesh3dx*>& temp_mesh,ONE_LOD& lod);
	void DummyVisibilitySet(vector<cTempMesh3dx*>& temp_mesh_names);
	void ParseEffect();

	void MergeMaterialMesh(vector<cTempMesh3dx*>& temp_mesh,vector<cTempMesh3dx*>& material_mesh,bool merge_mesh);

	void GetVisibleMesh(const string& mesh_name,int& visible_set,DWORD& visible,DWORD& lod_mask,
				bool& is_debris,bool& is_no_debris);

	void BuildSkinGroupSortedNormal(vector<cTempMesh3dx*>& temp_mesh,vector<cTempMesh3dx*>& out_mesh);
	void ExtractMesh(cTempMesh3dx* mesh,cTempMesh3dx*& extracted_mesh,
					  vector<char>& selected_polygon,vector<int>& node_index);

	void CalcBumpSTNorm(ONE_LOD& lod);//Нормаль равна SxT вектору.

	void CreateDebrises();
	void ClearDebrisNodes();

	void BuildProgramLod(vector<cTempMesh3dx*>& temp_mesh);
	void BuildProgramLod(cTempMesh3dx* lod0,cTempMesh3dx*& lod1,cTempMesh3dx*& lod2);
	vector<cTempMesh3dx*> FindBuildProgramLod(const char* mesh_name,vector<cTempMesh3dx*>& temp_mesh,int lod,cStaticVisibilitySet* pvs);

	vector<cTempMesh3dx*> temp_mesh_lod1,temp_mesh_lod2;
	void LoadLod(CLoadDirectory rd,vector<cTempMesh3dx*>& temp_mesh_lod);
	cTempMesh3dx* RealLoadedMesh(cTempMesh3dx* lod0,int lod);
};

struct Shader3dx
{
	class VSSkin* vsSkinSceneShadow;
	class PSSkin* psSkinSceneShadow;
	class VSSkin* vsSkinBumpSceneShadow;
	class PSSkin* psSkinBumpSceneShadow;

	class VSSkin* vsSkin;
	class PSSkinNoShadow* psSkin;
	
	class VSSkin* vsSkinBump;
	class PSSkin* psSkinBump;
	class VSSkinNoLight* vsSkinNoLight;

	class VSSkinShadow* vsSkinShadow;
	class PSSkinShadow* psSkinShadow;
	class PSSkinShadowAlpha* psSkinShadowAlpha;

	class VSSkin* vsSkinReflectionSceneShadow;
	class PSSkin* psSkinReflectionSceneShadow;
	class VSSkin* vsSkinReflection;
	class PSSkinNoShadow* psSkinReflection;

	class VSSkinSecondOpacity* vsSkinSecondOpacity;
	class PSSkinSecondOpacity* psSkinSecondOpacity;

	class VSSkinZBuffer* vsSkinZBuffer;
	class PSSkinZBuffer* psSkinZBuffer;
	class PSSkinZBufferAlpha* psSkinZBufferAlpha;
	
	Shader3dx();
	~Shader3dx();
};

inline
void operator>>(CLoadIterator& it,cStatic3dx::BoundSphere& v)
{
	it>>v.node_index;
	it>>v.position;
	it>>v.radius;
}

inline
Saver& operator<<(Saver& s,cStatic3dx::BoundSphere& v){
	s<<v.node_index;
	s<<v.position;
	s<<v.radius;
	return s;
}
extern Shader3dx* pShader3dx;

#endif
