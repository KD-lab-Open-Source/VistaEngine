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
#include <map>

#include <vector>
#include <string>
#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define IS_SPECIAL(F)  ((FLT_AS_DW(F) & 0x7f800000L)==0x7f800000L)

std::string TrimString(std::string str);
class IVisMaterial;
class IVisTexmap;
class IVisNode;
class IVisMesh;
class IVisSkin;
class IVisLight;
class IVisCamera;
enum NodeType
{
	MESH_TYPE	= GEOMOBJECT_CLASS_ID,
	LIGHT_TYPE	= LIGHT_CLASS_ID,
	CAMERA_TYPE	= CAMERA_CLASS_ID,
	HELPER_TYPE = HELPER_CLASS_ID,
};

class NullView : public View
{
public:
	NullView()
	{
		worldToView.IdentityMatrix();
		screenW = 640.0f;
		screenH = 480.0f;
	}
	virtual ~NullView(){};

public:
	Point2 ViewToScreen(Point3 p)
	{
		return Point2(p.x,p.y);
	}
};
extern NullView theNullView;
struct FaceEx
{
	DWORD vert[3];
	int index;
};

struct UVFace
{
	UVVert v[3];
};
class IVisExporter
{
friend IVisNode;
public:
	IVisExporter();
	~IVisExporter();

	bool Initialize(Interface* i);
	Interface* GetInterface(){return interface_;}
	int GetRoolMaterialCount() {return materials_.size();}
	IVisMaterial* GetRootMaterial(int i) {return materials_[i];}
	int GetRootNodeCount();
	IVisNode* GetRootNode(int n);
	IVisNode* FindNode(INode* node);
	TimeValue GetStartTime();
	TimeValue GetEndTime();
	TimeValue GetTicksPerFrame();
	void ProgressStart(const char* title=NULL);
	void ProgressEnd();
	void ProgressUpdate(const char* title=NULL);
	void AddProgressCount(int cnt);

	void SetTime(int max_time);//Для мешей затычка.
protected:
	IVisMaterial* GetIMaterial(Mtl* mtl);
	void ClearRootNodes();
	void ClearMaterials();
	void ClearAll();
	Interface* interface_;
	std::vector<IVisMaterial*> materials_;
	std::vector<IVisNode*> rootNodes_;
	int totalNodes_;

};

class IVisMaterial
{
public:
	IVisMaterial(Mtl* mtl=NULL);
	~IVisMaterial();
	const char* GetName(){return name_.c_str();}
	Color GetAmbient(TimeValue t=0);
	Color GetDiffuse(TimeValue t=0);
	Color GetSpecular(TimeValue t=0);
	float GetSpecularLevel(TimeValue t=0);
	float GetShininess(TimeValue t=0);
	float GetOpacity(TimeValue t=0);
	int GetShading();
	int GetTransparencyType();
	
	bool IsMultiType();
	int GetSubMaterialCount();
	IVisMaterial* GetSubMaterial(int i);
	
	int GetTexmapCount();
	IVisTexmap* GetTexmap(int i);

	Mtl* GetMtl(){return mtl_;}
	Interval Validity(TimeValue t);

	bool IsStandartType();
protected:
	void FindSubMaterials();
	void FindTexMaps();
	Mtl* mtl_;
	std::string name_;
	std::vector<IVisMaterial*> subMaterials_;
	std::vector<IVisTexmap*> texmaps_;
	std::map<int,int> matid2sub;
};

// Поддерживаются пока только Bitmap
class IVisTexmap
{
public:
	IVisTexmap(Texmap* texmap=NULL, IVisMaterial* material=NULL, int slot =-1);
	~IVisTexmap();
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

class IVisNode
{
public:
	IVisNode(INode* node=NULL, IVisNode* parent=NULL);
	~IVisNode();
	const char* GetName();
	Matrix3 GetWorldTM(TimeValue t);
	Matrix3 GetLocalTM(TimeValue t);
	IVisNode* GetParentNode(){return parentNode_;}
	NodeType GetType();
	IVisMesh* GetMesh();
	IVisLight* GetLight(TimeValue t);
	IVisCamera* GetCamera();
	INode* GetMAXNode(){return node_;}
	IVisMaterial* GetMaterial(){return material_;}
	IVisNode* FindNode(INode* node);
	int GetChildNodeCount();
	IVisNode* GetChildNode(int i);
	bool IsTarget();
	bool IsBone();
	float GetVisibility(TimeValue& t);
protected:
	void FindChildNodes();
	INode* node_;
	std::vector<IVisNode*> childNodes_;
	IVisNode* parentNode_;
	IVisMesh* mesh_;
	IVisLight* light_;
	IVisCamera* camera_;
	IVisMaterial* material_;
	std::string nodeName;
};
class IVisMesh
{
public:
	IVisMesh(IVisNode* node);
	~IVisMesh();
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
protected:
	void Init();
	bool HasNegativeScale();
	IVisNode* node_;
	TriObject* triObject_;
	Mesh* mesh_;
	bool needDelete_;
	IVisSkin* skin_;
	Matrix3 pivMat;
};

class IVisSkin
{
public:
	IVisSkin(IVisMesh* mesh);
	~IVisSkin();
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

class IVisLight
{
public:
	IVisLight(IVisNode* node, TimeValue& t);
	~IVisLight();
	int GetType();
	Point3 GetRGBColor(TimeValue t=0);
	float GetAttenStart(TimeValue t=0);
	float GetAttenEnd(TimeValue t=0);
	float GetIntensity(TimeValue t=0);
	const char* GetBitmapName();
protected:
	IVisNode* node_;
	GenLight* genLight_;
};

class IVisCamera
{
public:
	IVisCamera(IVisNode*node);
	~IVisCamera();
	float GetFov();
protected:
	IVisNode* node_;
	GenCamera* genCamera_;
};
IVisExporter* GetVisExporter();
extern int maxCount;
extern int curCount;


#endif //__IVISEXPORTER_H_