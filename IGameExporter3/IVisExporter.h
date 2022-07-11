#ifndef __IVISEXPORTER_H_
#define __IVISEXPORTER_H_

#include "Max.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "iskin.h"
#include "plugapi.h"
#include "cs/Phyexp.h"
#include "cs/bipexp.h"

#include "Render\3dx\Static3dxBase.h"

class IVisMaterial;
class IVisTexmap;
class IVisNode;
class IVisMesh;
class IVisSkin;
class IVisLight;
class IVisCamera;
struct ExporterVisibilitySet;

typedef vector<ShareHandle<IVisNode> > IVisNodes;
typedef vector<ShareHandle<IVisMaterial> > IVisMaterials;

enum NodeType
{
	MESH_TYPE	= GEOMOBJECT_CLASS_ID,
	LIGHT_TYPE	= LIGHT_CLASS_ID,
	CAMERA_TYPE	= CAMERA_CLASS_ID,
	HELPER_TYPE = HELPER_CLASS_ID,
};

struct FaceEx
{
	DWORD vert[3];
	int index;
};

struct UVFace
{
	UVVert v[3];
};

class IVisMaterial : public ShareHandleBase
{
public:
	IVisMaterial(Mtl* mtl=0);
	const char* GetName(){return name_.c_str();}
	Color GetAmbient(int t=0);
	Color GetDiffuse(int t=0);
	Color GetSpecular(int t=0);
	float GetSpecularLevel(int t=0);
	float GetShininess(int t=0);
	float GetOpacity(int t=0);
	int GetShading();
	int GetTransparencyType();
	
	bool IsMultiType();
	int GetSubMaterialCount();
	IVisMaterial* GetSubMaterial(int i);
	
	int GetTexmapCount();
	IVisTexmap* GetTexmap(int i);

	Mtl* GetMtl(){return mtl_;}
	Interval Validity(int t);

	bool IsStandartType();

	void serialize(Archive& ar);
	void setAnimationGroupName(const char* name) { animationGroupName_ = name; }

protected:
	void FindSubMaterials();
	void FindTexMaps();
	Mtl* mtl_;
	string name_;
	IVisMaterials subMaterials_;
	vector<ShareHandle<IVisTexmap> > texmaps_;
	map<int,int> matid2sub;
	ComboListString animationGroupName_;
};

// Поддерживаются пока только Bitmap
class IVisTexmap : public ShareHandleBase
{
public:
	IVisTexmap(Texmap* texmap=0, IVisMaterial* material=0, int slot =-1);
	const char* GetBitmapFileName();
	float GetAmount();
	int GetTextureTiling();
	void GetUVTransform(Matrix3& mat);
	int GetSlot(){return slot_;}

protected:
	Texmap* texmap_;
	IVisMaterial* material_;
	int slot_;
};

struct ExporterVisibilitySet 
{
	void set(const char* setName, const char* groupName);
	void serializeButton(Archive& ar, const char* meshName);
	void serialize(Archive& ar);

	ComboListString name;
	XBuffer buttonBuffer_;
	XBuffer nameBuffer_;
	typedef vector<ComboListString> Groups;
	Groups groups;
};

enum {
	SERIALIZE_NON_DELETE = 1,
	SERIALIZE_LOGIC = 2, 
	SERIALIZE_BOUND = 4, 
	SERIALIZE_ANIMATION_GROUP = 8,
	SERIALIZE_MESHES = 16,
	SERIALIZE_LIGHTS = 32,
	SERIALIZE_LEAVES = 64
};

class IVisNode : public ShareHandleBase
{
public:
	IVisNode(INode* node=0, IVisNode* parent=0);

	const char* GetName() const { return nodeName.c_str(); }
	NodeType GetType() const;
	
	Matrix3 GetWorldTM(int t);
	Matrix3 GetLocalTM(int t);

	INode* GetMAXNode(){return node_;}

	IVisMesh* GetMesh() { return mesh_; }
	IVisLight* GetLight() { return light_; }
	IVisCamera* GetCamera() { return camera_; }
	IVisMaterial* GetMaterial(){return material_;}
	
	IVisNode* FindNode(INode* node);
	int GetChildNodeCount();
	IVisNode* GetChildNode(int i);
	
	bool isTarget() const;
	bool isAux() const; // Bone, bound, bips
	bool isMesh() const;
	bool isLight() const;
	bool isCamera() const;
	bool isLeaf() const;
	bool isAnimated() const { return isAnimated_; }

	float GetVisibility(int t);

	void addBoundRecursive(sBox6f& bound, float& boundRadius);
	void cacheTransformRecursive(int time);
	void prepareToExportRecursive();
	void processAnimationRecursive();
	void processAnimation(StaticNodeAnimation& chain, int interval_begin, int interval_size, bool cycled, int export);
	void exportRecursive(Static3dxBase* object, bool logic);
	void exportMeshesRecursive(TempMeshes& meshes);
	void exportLightsRecursive(Static3dxBase* object);
	void exportBoundsRecursive(Static3dxBase* object);

	int index(bool silent = false) const { xassert(silent || index_ != -1); return index_; }
	void setIndex(int index) { index_ = index; }

	IVisNode* goodParent(int export) const;
	IVisNode* root();

	void serialize(Archive& ar);

	void setVisibilitySet(const char* setName, const char* groupName) { visibilitySet_.set(setName, groupName); }
	void setIsNonDelete() { isNonDelete_ = true; }
	void setIsLogic() { isLogic_ = true; }
	void setIsBound() { isBound_ = true; }
	void setAnimationGroupName(const char* name) { animationGroupName_ = name; }
	void exportNodesNamesRecursive(Static3dxBase* object);

protected:
	INode* node_;
	IVisNodes childNodes_;
	IVisNode* parentNode_;
	ShareHandle<IVisMesh> mesh_;
	ShareHandle<IVisLight> light_;
	ShareHandle<IVisCamera> camera_;
	IVisMaterial* material_;
	string nodeName;
	vector<Matrix3> worldTMs_;
	StaticNodeAnimations chainsLogic_;
	StaticNodeAnimations chainsGraphics_;
	ExporterVisibilitySet visibilitySet_;
	
	bool isAnimated_;
	bool visibilityAnimated_;
	bool anisotropicScale_;
	bool isNonDelete_;
	bool isLogic_;
	bool isBound_;
	ComboListString animationGroupName_;

	enum {
		EXPORT_LOGIC = 1,
		EXPORT_GRAPHICS = 2
	};
	int export_;
	int index_;

	static NodeNames checkMap_;
};

class IVisMesh : public ShareHandleBase
{
public:
	IVisMesh(IVisNode* node);
	int GetNumberVerts();
	int GetNumberFaces();
	bool IsMapSupport(int n);
	int GetNumMapVerst(int n);
	UVVert* GetMapVerts(int n);
	TVFace* GetMapFaces(int n);
	FaceEx GetFace(int n);
	Point3 GetVertex(int n);
	Point3 GetNormal(int faceIndex, int vertex);
	Point3 GetFaceNormal(int n);
	IVisMaterial* GetMaterialFromFace(int n);
	IVisNode* GetNode(){return node_;}
	Mesh* GetMAXMesh(){return mesh_;}
	IVisSkin* GetSkin();
	UVFace GetMapVertex(int mapID,FaceEx &face);

	void exportBound(sBox6f& bound, const Matrix3& m);

protected:
	bool HasNegativeScale();
	IVisNode* node_;
	TriObject* triObject_;
	Mesh* mesh_;
	IVisSkin* skin_;
	Matrix3 pivMat;
};

class IVisSkin
{
public:
	IVisSkin(IVisMesh* mesh);
	int GetNumberBones(int vertex);
	bool Init();
	float GetWeight(int vertexIndex, int boneIndex);
	IVisNode* GetIBone(int vertexIndex, int boneIndex);
	bool IsPhysique(){return isPhysique_;}
	bool IsSkin(){return isSkin_;}
	int GetVertexCount();

protected:
	Modifier* FindModifer(Class_ID& classID);
	IVisMesh* mesh_;
	ISkin* skin_;
	ISkinContextData* skinContext_;
	IPhysiqueExport* phys_;
	IPhyContextExport *phyContext_;
	Modifier* modifer_;
	bool isPhysique_;
	bool isSkin_;
};

class IVisLight : public ShareHandleBase
{
public:
	IVisLight(IVisNode* node);
	const char* GetName();
	int GetType();
	Point3 GetRGBColor(int t=0);
	float GetAttenStart(int t=0);
	float GetAttenEnd(int t=0);
	float GetIntensity(int t=0);
	const char* GetBitmapName();

protected:
	IVisNode* node_;
	GenLight* genLight_;
};

class IVisCamera : public ShareHandleBase
{
public:
	IVisCamera(IVisNode* node);
	float GetFov();

protected:
	GenCamera* genCamera_;
};

#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define IS_SPECIAL(F)  ((FLT_AS_DW(F) & 0x7f800000L)==0x7f800000L)

string TrimString(string str);


#endif //__IVISEXPORTER_H_
