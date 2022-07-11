#ifndef __STATIC_3DX_BASE_H__
#define __STATIC_3DX_BASE_H__

#include "Interpolator3DX.h"
#include "Render\3dx\Umath.h"
#include "XMath\Colors.h"
#include "XMath\Mats.h"
#include "XMath\Rectangle4f.h"
#include "XMath\Box6f.h"
#include "XTL\UniqueVector.h"

struct cTempMesh3dx;
typedef ShareHandle<cTempMesh3dx> TempMesh;
typedef vector<TempMesh> TempMeshes;
typedef UniqueVector<string> NodeNames;
typedef UniqueVector<string> TextureNames;

class cTexture;
class cStatic3dx;
class Static3dxBase;
class cStaticSimply3dx;

class IVisMaterial;
class Exporter;
class IVisMesh;
class IVisLight;

enum ObjectShadowType
{
	OST_SHADOW_NONE,
	OST_SHADOW_CIRCLE,
	OST_SHADOW_REAL
};

struct StaticNodeAnimation
{
	Interpolator3dxScale    scale;
	Interpolator3dxPosition position;
	Interpolator3dxRotation rotation;
	Interpolator3dxBool     visibility;

	void serialize(Archive& ar);
	void referencePose(Mats& pos);
};
typedef vector<StaticNodeAnimation> StaticNodeAnimations;

struct StaticNode
{
	string name;
	int inode;
	int iparent;
	StaticNodeAnimations chains;
	MatXf inv_begin_pos;

	StaticNode();
	void serialize(Archive& ar);

	void LoadNode(CLoadDirectory ld, StaticChainsBlock& chains_block);
	void LoadNodeChain(CLoadDirectory ld, StaticChainsBlock& chains_block);
};
typedef vector<StaticNode> StaticNodes;

struct StaticAnimationChain
{
	string name;
	float time;
	int begin_frame;
	int end_frame;
	bool cycled; //Нужно для корректной интерполяции

	StaticAnimationChain();
	void serialize(Archive& ar);
	int intervalSize() const { return end_frame - begin_frame; } //  + 1
};
typedef vector<StaticAnimationChain> StaticAnimationChains;

struct cTempVisibleGroup
{
	int visibilitySet;//К какому множеству принадлежит.
	DWORD visibilities;//Видимость в соответствующих группах видимости. Битовыми флажками. Не более 32 групп видимости.
	int begin_polygon;
	int num_polygon;
	int visibilityNodeIndex;

	void serialize(Archive& ar);
};
typedef vector<cTempVisibleGroup> TempVisibleGroups;

struct AnimationGroup
{
	string name;
	vector<int> nodes;
	NodeNames nodesNames;
	NodeNames materialsNames;

	void serialize(Archive& ar);
	void Load(CLoadDirectory rd);
};
typedef vector<AnimationGroup> AnimationGroups;

struct FurInfo
{
	enum FurType
	{
		FUR_CONST,
		FUR_LINEAR
	};

	string material; //Имя материала
	float scale; //Длинна шерсти
	float alpha; //Прозрачность слоя
	FurType furType; //const or linear
	string texture; //Текстура шерсти
	string normal; //RGB - Направление шерсти в tangent space. A - длинна шерсти.

	FurInfo();
	void serialize(Archive& ar);
};

struct StaticMaterialAnimation
{
	Interpolator3dxScale opacity;
	Interpolator3dxUV uv;
	Interpolator3dxUV uv_displacement;

	void serialize(Archive& ar);
	void Load(CLoadDirectory rd, StaticChainsBlock& chains_block);
};
typedef vector<StaticMaterialAnimation> StaticMaterialAnimations;

struct StaticMaterial
{
	enum TextureMapType
	{	/* texture map in 3dSMAX */
		TEXMAP_AM					=	0,   // ambient
		TEXMAP_DI					=	1,   // diffuse						tex_diffuse
		TEXMAP_SP					=	2,   // specular					tex_specularmap (для бампа только)
		TEXMAP_SH					=	3,   // shininess
		TEXMAP_SS					=	4,   // shininess strength
		TEXMAP_SI					=	5,   // self-illumination			tex_self_illumination
		TEXMAP_OP					=	6,   // opacity						opacity
		TEXMAP_FI					=	7,   // filter color				tex_skin
		TEXMAP_BU					=	8,   // bump						tex_bump
		TEXMAP_RL					=	9,   // reflection					tex_reflect
		TEXMAP_RR					=	10,  // refraction 
		TEXMAP_DP					=	11,  // displacement				tex_secondopacity
	};
	enum TextureTiling
	{
		TILING_U_WRAP=   (1<<0),
		TILING_V_WRAP=   (1<<1),
		//		TILING_U_MIRROR= (1<<2),
		//		TILING_V_MIRROR= (1<<3),
	};
	enum TransparencyType
	{
		TRANSPARENCY_SUBSTRACTIVE = 0,
		TRANSPARENCY_ADDITIVE     = 1,
		TRANSPARENCY_FILTER		  = 2,
	};

	string name;

	Color4f ambient;
	Color4f diffuse;
	Color4f specular;
	float opacity;
	float specular_power;

	bool is_opacity_texture;
	string tex_diffuse;
	int tiling_diffuse;
	TransparencyType transparencyType;
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
	bool texturesCreated;

	StaticMaterialAnimations chains;

	cTexture* pFurmap;
	float fur_scale;
	float fur_alpha;
	string tex_furmap;
	string tex_furnormalmap;
	FurInfo::FurType fur_alpha_type;

	StaticMaterial();
	~StaticMaterial();
	void serialize(Archive& ar);

	void Load(CLoadDirectory rd, Static3dxBase* pStatic, StaticChainsBlock& chains_block);

	void loadFur(const FurInfo& furInfo, cStatic3dx* object);
	void createTextures(cStatic3dx* object);
	void releaseTextures();
};
typedef vector<StaticMaterial> StaticMaterials;

enum { MAX_BONES = 4 };

struct cTempBone
{
	float weight[MAX_BONES];
	int inode[MAX_BONES];

	void serialize(Archive& ar);
};

struct cTempMesh3dx : ShareHandleBase
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
	bool visibilityAnimated;

	vector<int> inode_array;//На какие ноды ссылается скиннинг в этом mesh
	vector<cTempVisibleGroup> visible_group;

	cTempMesh3dx();

	void serialize(Archive& ar);

	void Load(CLoadDirectory rd);
	void FillVisibleGroup(int visible_set, DWORD visibilities);

	int CalcMaxBonesPerVertex();

	void Merge(TempMeshes& meshes,class cStatic3dx* pStatic);
	void OptimizeMesh();

	void deleteSingularPolygon();
	bool export(IVisMesh* pobject, IVisMaterial* mat, int inode);
};

struct sLogo
{
	sLogo(const char* name = "");
	void serialize(Archive& ar);

	sRectangle4f rect;
	string TextureName;
	float angle;
	bool enabled; // temp
};

struct StaticLogos
{
	vector<sLogo> logos;

	void serialize(Archive& ar);
	void Load(CLoadDirectory it);
	bool GetLogoPosition(const char* fname, sRectangle4f* rect, float& angle);
};

struct StaticVisibilityGroup
{
	string name;
	DWORD visibility;

	typedef vector<char> VisibleNodes;
	VisibleNodes visibleNodes;

	NodeNames meshes; // temp
	bool is_invisible_list;//For old format

	StaticVisibilityGroup();
	~StaticVisibilityGroup() {}
	void serialize(Archive& ar);

	void Load(CLoadIterator &ld);
};
typedef vector<StaticVisibilityGroup> StaticVisibilityGroups;

struct VisibilitySetIndex
{
	explicit VisibilitySetIndex(int iset = -1) : iset_(iset) {}

	operator int() const { return iset_; }

	static VisibilitySetIndex BAD;
	static VisibilitySetIndex ZERO;

private:
	int iset_;
};

struct VisibilityGroupIndex
{
	explicit VisibilityGroupIndex(int igroup = -1) : igroup_(igroup) {}
	
	operator int() const { return igroup_; }

	static VisibilityGroupIndex BAD;

private:
	int igroup_;
};

struct StaticVisibilitySet
{
	enum 
	{
		num_lod=3
	};

	string name;
	NodeNames meshes; // CONVERSION
	StaticVisibilityGroups visibilityGroups;//Не менее одной группы

	StaticVisibilitySet();

	void serialize(Archive& ar);
	void DummyVisibilityGroup();

	int GetVisibilityGroupNumber(){return visibilityGroups.size();};
	const char* GetVisibilityGroupName(VisibilityGroupIndex group);
	VisibilityGroupIndex GetVisibilityGroupIndex(const char* group_name);
};
typedef vector<StaticVisibilitySet> StaticVisibilitySets;

struct StaticEffect
{
	int node;
	bool is_cycled;
	string file_name;

	StaticEffect();
	void serialize(Archive& ar);
};
typedef vector<StaticEffect> StaticEffects;

struct StaticLightAnimation
{
	Interpolator3dxRotation color; //!!!

	void serialize(Archive& ar);
	void Load(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void export(IVisLight* pobject, int interval_begin, int interval_size, bool cycled);
};
typedef vector<StaticLightAnimation> StaticLightAnimations;

struct StaticLight
{
	int inode;
	Color4f color;
	float atten_start;
	float atten_end;
	StaticLightAnimations chains;
	string texture;
	cTexture* pTexture;

	StaticLight();

	void serialize(Archive& ar);

	void Load(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void export(IVisLight* pobject, int inode);
};
typedef vector<StaticLight> StaticLights;

struct StaticLeaf
{
	int inode;
	Color4f color;
	float size;
	string texture;
	cTexture* pTexture;
	vector<int> lods;

	StaticLeaf();
	void serialize(Archive& ar);
	void Load(CLoadDirectory rd);
	void export(IVisLight* pobject, int inode_current);
};
typedef vector<StaticLeaf> StaticLeaves;

struct CameraParams
{
	int camera_node_num;
	float fov;

	CameraParams();
	void serialize(Archive& ar);
};


class Static3dxBase
{
public:
	Static3dxBase(bool is_logic_);
	~Static3dxBase();

	bool load(const char* fileName);
	void serialize(Archive& ar);
	bool loadOld(CLoadDirectory& rd);

	const char* fileName() const { return fileName_.c_str(); }

	enum { VERSION = 2 };
	int version;
	int maxWeights;
	NodeNames nonDeleteNodes;
	NodeNames logicNodes;
	NodeNames boundNodes;

	StaticNodes nodes;
	AnimationGroups animationGroups_;
	StaticAnimationChains animationChains_;
	StaticVisibilitySets visibilitySets_;//Не менее одного 
	StaticMaterials materials;

	bool isBoundBoxInited;
	sBox6f boundBox;
	float boundRadius;
	vector<sBox6f> localLogicBounds;//==nodes.size();

	StaticLogos logos;
	StaticEffects effects;
	StaticLights lights;
	StaticLeaves leaves;

	bool is_lod;
	bool is_logic;
	bool is_old_model;
	bool loaded;

	ObjectShadowType circle_shadow_enable;
	ObjectShadowType circle_shadow_enable_min;
	int circle_shadow_height;
	float circle_shadow_radius;

	struct BoundSphere
	{
		int node_index;
		Vect3f position;
		float radius;
		void serialize(Archive& ar);
	};

	typedef vector<BoundSphere> BoundSpheres;
	BoundSpheres boundSpheres;

	bool				bump;
	bool				isUV2;
	bool				enableFur; //not saved to chache
	CameraParams		cameraParams;

	bool GetLoaded(){return loaded;}
	void SetLoaded(){loaded = true;}

	static Static3dxBase* serializedObject() { return serializedObject_; }
	static const char* fileExtention;

protected:
	string fileName_;
	TempMeshes tempMesh_;
	TempMeshes tempMeshLod1_;
	TempMeshes tempMeshLod2_;

	static Static3dxBase* serializedObject_;

	void LoadInternal(CLoadDirectory& rd);

	void LoadChainData(CLoadDirectory rd);
	void LoadGroup(CLoadDirectory rd);
	void LoadChain(CLoadDirectory rd);
	void LoadChainGroup(CLoadDirectory rd);
	void LoadVisibleSets(CLoadDirectory rd);
	void LoadVisibleSet(CLoadDirectory rd);

	void LoadNodes(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void LoadCamera(CLoadDirectory rd);

	void LoadMeshes(CLoadDirectory rd,TempMeshes& temp_mesh);
	void LoadLod(CLoadDirectory rd,TempMeshes& temp_mesh_lod);
	void LoadLights(CLoadDirectory rd, StaticChainsBlock& chains_block);
	void LoadLeaves(CLoadDirectory rd, StaticChainsBlock& chains_block);

	int LoadMaterialsNum(CLoadDirectory rd);
	void LoadMaterials(CLoadDirectory rd,int num_materials, StaticChainsBlock& chains_block);
	void LoadLocalLogicBound(CLoadIterator it);

	friend Exporter;
};

class Static3dxFile
{
public:
	Static3dxFile();
	~Static3dxFile();

	void load(const char* fileName);
	void save(const char* fileName);
	void serialize(Archive& ar);

	static void convertFile(const char* fname);

protected:
	Static3dxBase* graphics3dx_;
	Static3dxBase* logic3dx_;
	bool badLogic3dx_;
};

inline void operator>>(CLoadIterator& it,Static3dxBase::BoundSphere& v)
{
	it>>v.node_index;
	it>>v.position;
	it>>v.radius;
}

inline Saver& operator<<(Saver& s,Static3dxBase::BoundSphere& v){
	s<<v.node_index;
	s<<v.position;
	s<<v.radius;
	return s;
}

#endif //__STATIC_3DX_BASE_H__
