#include "stdafx.h"
#include "IVisExporter.h"
#include "stdmat.h"

static IVisExporter visExporter;
NullView theNullView;

int maxCount=0;
int curCount=0;

IVisExporter* GetVisExporter()
{
	return &visExporter;
}
DWORD WINAPI ProgressFunction(LPVOID arg)
{
	return 0;
}

string TrimString(string str)
{
	string workStr = str;
	string errorStr;
	int pos;
	// удаление пробелов вначале
	pos = workStr.find_first_not_of(" ");
	if(pos != string::npos && pos != 0)
	{
		workStr = workStr.substr(pos);
		errorStr += "ѕробелы в начале, ";
	}
	// удаление пробелов в конце
	pos = workStr.find_last_not_of(" ");
	if(pos != string::npos && pos != workStr.size()-1)
	{
		workStr = workStr.erase(pos+1);
		errorStr += "ѕробелы в конце, ";
	}
	// удаление двойных пробелов
	while((pos = workStr.find("  ")) != string::npos)
	{
		workStr.replace(pos,2," ");
		errorStr = "ƒвойные пробелы, ";
	}
	if(!errorStr.empty())
	{
		errorStr += "в строке: \"";
		errorStr += str;
		errorStr += "\"\n";
		Msg(errorStr.c_str());
	}
	return workStr;
}


//==================================================================================================================
///// IVisExporter /////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisExporter::IVisExporter()
{
	totalNodes_ = 0;
}
//==================================================================================================================
IVisExporter::~IVisExporter()
{
	ClearAll();
}
//==================================================================================================================
void IVisExporter::ClearRootNodes()
{
	for(int i=0; i<rootNodes_.size(); i++)
		delete rootNodes_[i];
	rootNodes_.clear();
}
//==================================================================================================================
void IVisExporter::ClearMaterials()
{
	for(int i=0; i<materials_.size(); i++)
		delete materials_[i];
	materials_.clear();
}
//==================================================================================================================
void IVisExporter::ClearAll()
{
	ClearMaterials();
	ClearRootNodes();
}
//==================================================================================================================
bool IVisExporter::Initialize(Interface* in)
{
	interface_ = in;
	curCount = 0;
	maxCount = 0;
	ClearAll();
	for(int i=0; i<interface_->GetRootNode()->NumberOfChildren(); i++)
	{
		IVisNode* node = new IVisNode(interface_->GetRootNode()->GetChildNode(i));
		rootNodes_.push_back(node);
	}
	return true;
}
//==================================================================================================================
void IVisExporter::ProgressStart(const char* title)
{
	curCount=0;
	maxCount=0;
	interface_->ProgressStart((TCHAR*)title,TRUE,ProgressFunction,0);
}
//==================================================================================================================
void IVisExporter::ProgressEnd()
{
	interface_->ProgressEnd();
}
//==================================================================================================================
void IVisExporter::ProgressUpdate(const char* title)
{
	curCount++;
	if(title)
		interface_->ProgressUpdate(float(curCount)/float(maxCount)*100,FALSE,(TCHAR*)title);
	else
		interface_->ProgressUpdate(float(curCount)/float(maxCount)*100);
}
//==================================================================================================================
void IVisExporter::AddProgressCount(int cnt)
{
	curCount=0;
	maxCount=cnt;
}
//==================================================================================================================
TimeValue IVisExporter::GetStartTime()
{
	Interval i = interface_->GetAnimRange();
	return i.Start();
}
//==================================================================================================================
TimeValue IVisExporter::GetEndTime()
{
	Interval i = interface_->GetAnimRange();
	return i.End();
}
//==================================================================================================================
TimeValue IVisExporter::GetTicksPerFrame()
{
	return ::GetTicksPerFrame();
}
//==================================================================================================================
IVisMaterial* IVisExporter::GetIMaterial(Mtl* mtl)
{
	for(int i=0;i<materials_.size();i++)
	{
		if(materials_[i]->GetMtl() == mtl)
			return materials_[i];
	}
	IVisMaterial* material = new IVisMaterial(mtl);
	materials_.push_back(material);
	return material;
}
//==================================================================================================================
int IVisExporter::GetRootNodeCount()
{
	return rootNodes_.size();
}
//==================================================================================================================
IVisNode* IVisExporter::GetRootNode(int n)
{
	if(n<rootNodes_.size())
		return rootNodes_[n];
	return NULL;
}
IVisNode* IVisExporter::FindNode(INode* node_)
{
	for(int i=0;i<rootNodes_.size(); i++)
	{
		IVisNode* node = rootNodes_[i]->FindNode(node_);
		if(node)
			return node;
	}
	return NULL;
}

void IVisExporter::SetTime(int max_time)
{
	interface_->SetTime(max_time,FALSE);
}


//==================================================================================================================
///// IVisMaterial /////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisMaterial::IVisMaterial(Mtl* mtl):
mtl_(mtl)
{
	if(mtl_)
	{
		name_ = TrimString((const char*)mtl_->GetName());
		FindSubMaterials();
		FindTexMaps();
	}
}
//==================================================================================================================
IVisMaterial::~IVisMaterial()
{
	for (int i=0; i<subMaterials_.size(); i++)
	{
		delete subMaterials_[i];
	}
	for (int i=0; i<texmaps_.size(); i++)
	{
		delete texmaps_[i];
	}
	subMaterials_.clear();
	texmaps_.clear();
}
//==================================================================================================================
void IVisMaterial::FindSubMaterials()
{
	for(int i=0; i<mtl_->NumSubMtls(); i++)
	{
		Mtl* subMtl = mtl_->GetSubMtl(i);
		if(subMtl)
		{
			IVisMaterial* mat = new IVisMaterial(subMtl);
			subMaterials_.push_back(mat);
		}
	}
}
//==================================================================================================================
void IVisMaterial::FindTexMaps()
{
	for(int i=0; i<mtl_->NumSubTexmaps(); i++)
	{
		Texmap* texmap = mtl_->GetSubTexmap(i);
		
		if(texmap)
		{
			if (mtl_->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) 
			{
				if (!((StdMat*)mtl_)->MapEnabled(i))
					continue;
				if (texmap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0x00))
					continue;
				IVisTexmap* tm = new IVisTexmap(texmap,this,i);
				texmaps_.push_back(tm);
			}
		}
	}
}
//==================================================================================================================
bool IVisMaterial::IsStandartType()
{
	return (mtl_->ClassID() == Class_ID(DMTL_CLASS_ID, 0));
}
//==================================================================================================================
Color IVisMaterial::GetAmbient(TimeValue t)
{
	return ((StdMat*)mtl_)->GetAmbient(t);
}
//==================================================================================================================
Color IVisMaterial::GetDiffuse(TimeValue t)
{
	return ((StdMat*)mtl_)->GetDiffuse(t);
}
//==================================================================================================================
Color IVisMaterial::GetSpecular(TimeValue t)
{
	return ((StdMat*)mtl_)->GetSpecular(t);
}
//==================================================================================================================
float IVisMaterial::GetSpecularLevel(TimeValue t)
{
	return ((StdMat*)mtl_)->GetShinStr(t);
}
//==================================================================================================================
float IVisMaterial::GetShininess(TimeValue t)
{
	return ((StdMat*)mtl_)->GetShininess(t);
}
//==================================================================================================================
float IVisMaterial::GetOpacity(TimeValue t)
{
	return ((StdMat*)mtl_)->GetOpacity(t);
}
//==================================================================================================================
int IVisMaterial::GetShading()
{
	return ((StdMat*)mtl_)->GetShading();
}
//==================================================================================================================
int IVisMaterial::GetTransparencyType()
{
	return ((StdMat*)mtl_)->GetTransparencyType();
}
//==================================================================================================================
Interval IVisMaterial::Validity(TimeValue t)
{
	return mtl_->Validity(t);
}
//==================================================================================================================
bool IVisMaterial::IsMultiType()
{
	return GetSubMaterialCount()>0?true:false;
}
//==================================================================================================================
int IVisMaterial::GetSubMaterialCount()
{
	return subMaterials_.size();
}
//==================================================================================================================
IVisMaterial* IVisMaterial::GetSubMaterial(int mtlid)
{
	map<int,int>::iterator it=matid2sub.find(mtlid);
	if(it==matid2sub.end())
	{
		TSTR str=mtl_->GetSubMtlSlotName(mtlid);
		int id=0;
		if(str[0]=='(' && str[1]>='1' && str[1]<='9' && str[2]==')')
		{
			id=str[1]-'1';
		}

		matid2sub[mtlid]=id;
		it=matid2sub.find(mtlid);
	}

	int subid=it->second;
	if(subid<subMaterials_.size())
		return subMaterials_[subid];

	return subMaterials_.empty()?NULL:subMaterials_[0];
}
//==================================================================================================================
int IVisMaterial::GetTexmapCount()
{
	return texmaps_.size();
}
//==================================================================================================================
IVisTexmap* IVisMaterial::GetTexmap(int i)
{
	if(i<texmaps_.size())
		return texmaps_[i];
	return NULL;
}
//==================================================================================================================
///// IVisTexmap ////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisTexmap::IVisTexmap(Texmap* texmap, IVisMaterial* material, int slot):
texmap_(texmap),
material_(material),
slot_(slot)
{

}
//==================================================================================================================
IVisTexmap::~IVisTexmap()
{

}
//==================================================================================================================
const char* IVisTexmap::GetBitmapFileName()
{
	return ((BitmapTex *)texmap_)->GetMapName();
}
//==================================================================================================================
float IVisTexmap::GetAmount()
{
	return ((StdMat*)material_->GetMtl())->GetTexmapAmt(slot_,0);
}
//==================================================================================================================
int IVisTexmap::GetTextureTiling()
{
	return texmap_->GetTextureTiling();
}
//==================================================================================================================
void IVisTexmap::GetUVTransform(Matrix3& mat)
{
	texmap_->GetUVTransform(mat);
}
//==================================================================================================================
///// IVisNode //////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisNode::IVisNode(INode* node, IVisNode* parent):
node_(node),
parentNode_(parent)
{
	mesh_ = NULL;
	material_ = NULL;
	light_ = NULL;
	camera_ = NULL;
	if(node_)
	{
		FindChildNodes();
		if(node_->GetMtl())
		{
			material_ = GetVisExporter()->GetIMaterial(node_->GetMtl());
		}
		nodeName = TrimString(node->GetName());
	}
}
//==================================================================================================================
IVisNode::~IVisNode()
{
	for(int i=0; i<childNodes_.size(); i++)
		delete childNodes_[i];
	if(mesh_) delete mesh_;
	if(light_) delete light_;
	if(camera_) delete camera_;
}
//==================================================================================================================
void IVisNode::FindChildNodes()
{
	for(int i=0; i<node_->NumberOfChildren(); i++)
	{
		IVisNode* node = new IVisNode(node_->GetChildNode(i),this);
		childNodes_.push_back(node);
	}
}
//==================================================================================================================
const char* IVisNode::GetName()
{
	return nodeName.c_str();
}
//==================================================================================================================
Matrix3 IVisNode::GetWorldTM(TimeValue t)
{
	return node_->GetNodeTM(t);
}
//==================================================================================================================
Matrix3 IVisNode::GetLocalTM(TimeValue t)
{
	if(!parentNode_)
		return GetWorldTM(t);
	Matrix3 matParent = parentNode_->GetWorldTM(t);
	matParent.Invert();
	return GetWorldTM(t)*matParent;
}
//==================================================================================================================
NodeType IVisNode::GetType()
{
	ObjectState os = node_->EvalWorldState(0); 
	return (NodeType)os.obj->SuperClassID();
}
//==================================================================================================================
bool IVisNode::IsBone()
{
	ObjectState os = node_->EvalWorldState(0); 
	if(os.obj&&os.obj->ClassID() == BONE_OBJ_CLASSID)
		return true;
	Control *control;
	control = node_->GetTMController();

	if ((control->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
		(control->ClassID() == BIPBODY_CONTROL_CLASS_ID))
		return true;

	return false;
}
//==================================================================================================================
IVisMesh* IVisNode::GetMesh()
{
	if(GetType() == MESH_TYPE)
	{
		if(!mesh_)
			mesh_ = new IVisMesh(this);
		return mesh_;
	}
	return NULL;
}
//==================================================================================================================
IVisNode* IVisNode::FindNode(INode* node)
{
	if (node_ == node)
		return this;
	for(int i=0;i<childNodes_.size(); i++)
	{
		IVisNode* nd = childNodes_[i]->FindNode(node);
		if(nd)
			return nd;
	}
	return NULL;
}
//==================================================================================================================
int IVisNode::GetChildNodeCount()
{
	return childNodes_.size();
}
//==================================================================================================================
IVisNode* IVisNode::GetChildNode(int i)
{
	return childNodes_[i];
}
//==================================================================================================================
bool IVisNode::IsTarget()
{
	return node_->IsTarget();
}
//==================================================================================================================
float IVisNode::GetVisibility(TimeValue& t)
{
	return node_->GetVisibility(t);
}
//==================================================================================================================
IVisLight* IVisNode::GetLight(TimeValue t)
{
	if(GetType() == LIGHT_TYPE)
	{
		if(!light_)
			light_ = new IVisLight(this,t);
		return light_;
	}
	return NULL;
}
//==================================================================================================================
IVisCamera* IVisNode::GetCamera()
{
	if(GetType() == CAMERA_TYPE)
	{
		if(!camera_)
			camera_ = new IVisCamera(this);
		return camera_;
	}
	return NULL;
}
//==================================================================================================================
///// IVisMesh //////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisMesh::IVisMesh(IVisNode* node):
node_(node)
{
	triObject_ = NULL;
	mesh_ = NULL;
	needDelete_ = false;
	if(node)
		Init();
	skin_ = new IVisSkin(this);
}
//==================================================================================================================
IVisMesh::~IVisMesh()
{

}
//==================================================================================================================
void IVisMesh::Init()
{
	ObjectState os = node_->GetMAXNode()->EvalWorldState(0);
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) {
		return;
	}
	if (os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
	{ 
		triObject_ = (TriObject *) os.obj->ConvertToType(0, 
			Class_ID(TRIOBJ_CLASS_ID, 0));
		if (os.obj != triObject_) 
			needDelete_ = true;
	}
	else 
		return;

	BOOL needDelete = FALSE;
	//mesh_ = triObject_->GetRenderMesh(0,node_->GetMAXNode(),theNullView,needDelete);
	mesh_ = &triObject_->GetMesh();
	mesh_->buildNormals();
	mesh_->checkNormals(TRUE);
	pivMat = node_->GetMAXNode()->GetObjectTM(0);
}
//==================================================================================================================
int IVisMesh::GetNumberVerts()
{
	return mesh_->getNumVerts();
}
//==================================================================================================================
int IVisMesh::GetNumberFaces()
{
	return mesh_->getNumFaces();
}
//==================================================================================================================
bool IVisMesh::IsMapSupport(int n)
{
	return mesh_->mapSupport(n);
}
//==================================================================================================================
int IVisMesh::GetNumMapVerst(int n)
{
	return mesh_->getNumMapVerts(n);
}
//==================================================================================================================
UVVert* IVisMesh::GetMapVerts(int n)
{
	return mesh_->mapVerts(n);
}
//==================================================================================================================
TVFace* IVisMesh::GetMapFaces(int n)
{
	return mesh_->mapFaces(n);
}
//==================================================================================================================
FaceEx IVisMesh::GetFace(int n)
{
	FaceEx face;
	if(HasNegativeScale())
	{
		face.vert[0] = mesh_->faces[n].v[0];
		face.vert[1] = mesh_->faces[n].v[2];
		face.vert[2] = mesh_->faces[n].v[1];
	}else
	{
		face.vert[0] = mesh_->faces[n].v[0];
		face.vert[1] = mesh_->faces[n].v[1];
		face.vert[2] = mesh_->faces[n].v[2];
	}
	face.index = n;
	return face;
}
//==================================================================================================================
bool IVisMesh::HasNegativeScale()
{
	return pivMat.Parity() ? true : false;
}
//==================================================================================================================
Point3 IVisMesh::GetVertex(int n)
{
	return mesh_->verts[n]*pivMat;
}
//==================================================================================================================
Point3 IVisMesh::GetNormal(int faceIndex, int vertex)
{
	RVertex* rv = mesh_->getRVertPtr(vertex);
	Face* f = &mesh_->faces[faceIndex];
	DWORD smGroup = f->smGroup;
	int numNormals;
	Point3 vertexNormal;

	// Is normal specified
	// SPECIFIED_NORMAL is not currently used, but may be used in future versions.
	if (rv->rFlags & SPECIFIED_NORMAL)
	{
		vertexNormal = rv->rn.getNormal();
	}
	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	else 
		if ((numNormals = rv->rFlags & NORCT_MASK) && smGroup) 
		{
			// If there is only one vertex is found in the rn member.
			if (numNormals == 1) 
			{
				vertexNormal = rv->rn.getNormal();
			}
			else
			{
				// If two or more vertices are there you need to step through them
				// and find the vertex with the same smoothing group as the current face.
				// You will find multiple normals in the ern member.
				for (int i = 0; i < numNormals; i++)
				{
					if (rv->ern[i].getSmGroup() & smGroup)
					{
						vertexNormal =  rv->ern[i].getNormal();
					}
				}
			}
		}
		else
		{
			// Get the normal from the Face if no smoothing groups are there
			vertexNormal = mesh_->getFaceNormal(faceIndex);
		}

		Matrix3 tmRotation;
		tmRotation = pivMat;
		tmRotation.NoTrans();
		tmRotation.NoScale();
		return (vertexNormal*tmRotation).Normalize();
}
//==================================================================================================================
Point3 IVisMesh::GetFaceNormal(int n)
{
	Point3 maxNorm = mesh_->getFaceNormal(n);
	Face& face = mesh_->faces[n];
	Point3 pos0=GetVertex(face.v[0]);
	Point3 pos1=GetVertex(face.v[1]);
	Point3 pos2=GetVertex(face.v[2]);
	Point3 norm =(pos0-pos1)^(pos1-pos2);//CROSS PRODUCT
	norm=norm.Normalize();
	if(maxNorm%norm<0)// DOT PRODUCT
		maxNorm=-maxNorm;
	return maxNorm;
}
//==================================================================================================================
IVisMaterial* IVisMesh::GetMaterialFromFace(int n)
{
	Face& face = mesh_->faces[n];
	MtlID id = face.getMatID();
	IVisMaterial* material = node_->GetMaterial();
	if(!material)
		return NULL;
	if(material->IsMultiType())
	{
		if(id < material->GetSubMaterialCount())
			return material->GetSubMaterial(id);
		return material->GetSubMaterial(0);
	}
	return material;
}
//==================================================================================================================
IVisSkin* IVisMesh::GetSkin()
{
	if(skin_->IsSkin()||skin_->IsPhysique())
		return skin_;
	else
	{
		if(skin_->Init())
			return skin_;
	}
	return NULL;
}
//==================================================================================================================
UVFace IVisMesh::GetMapVertex(int mapID,FaceEx &face)
{
	UVFace uvFace;
	TVFace* tvFace = GetMapFaces(mapID);
	UVVert* uvVert = GetMapVerts(mapID);
	if(tvFace&&uvVert)
	{
		if(HasNegativeScale())
		{
			uvFace.v[0] = uvVert[tvFace[face.index].t[0]];
			uvFace.v[2] = uvVert[tvFace[face.index].t[1]];
			uvFace.v[1] = uvVert[tvFace[face.index].t[2]];
		}else
		{
			uvFace.v[0] = uvVert[tvFace[face.index].t[0]];
			uvFace.v[1] = uvVert[tvFace[face.index].t[1]];
			uvFace.v[2] = uvVert[tvFace[face.index].t[2]];
		}
	}
	return uvFace;
}
//==================================================================================================================
///// IVisSkin //////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisSkin::IVisSkin(IVisMesh* mesh):
mesh_(mesh)
{
	skin_ = NULL;
	phys_ = NULL;
	phyContext_= NULL;
	modifer_ = NULL;
	isPhysique_ = false;
	isSkin_ = false;

	if(mesh_)
		Init();
}
//==================================================================================================================
IVisSkin::~IVisSkin()
{

}
//==================================================================================================================
bool IVisSkin::Init()
{
	Modifier* modifer_ = FindModifer(Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B));
	if(modifer_)
	{
		phys_ = (IPhysiqueExport *)modifer_->GetInterface(I_PHYINTERFACE);
		if(!phys_)
			return false;
		phyContext_ = (IPhyContextExport *)phys_->GetContextInterface(mesh_->GetNode()->GetMAXNode());
		if(!phyContext_)
			return false;
		phyContext_->ConvertToRigid(true);
		phyContext_->AllowBlending(true);

		isPhysique_ = true;
		isSkin_ = false;
	}else
	{
		modifer_ = FindModifer(SKIN_CLASSID);
		if(!modifer_)
			return false;
		skin_ = (ISkin*) modifer_->GetInterface(I_SKIN);
		if(!skin_)
			return false;

		skinContext_ = skin_->GetContextInterface(mesh_->GetNode()->GetMAXNode());
		if(!skinContext_)
			return false;

		isPhysique_ = false;
		isSkin_ = true;
	}
	
	return true;

}
//==================================================================================================================
Modifier* IVisSkin::FindModifer(Class_ID& classID)
{
	// get the object reference of the node
	Object *pObject = mesh_->GetNode()->GetMAXNode()->GetObjectRef();
	if(pObject == 0) return 0;

	// loop through all derived objects
	while(pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject *pDerivedObject;
		pDerivedObject = static_cast<IDerivedObject *>(pObject);

		// loop through all modifiers
		int stackId;
		for(stackId = 0; stackId < pDerivedObject->NumModifiers(); stackId++)
		{
			Modifier *pModifier;
			pModifier = pDerivedObject->GetModifier(stackId);

			// check if we found the physique modifier
			if(pModifier->ClassID() == classID) return pModifier;
		}

		// continue with next derived object
		pObject = pDerivedObject->GetObjRef();
	}

	return NULL;
}
//==================================================================================================================
int IVisSkin::GetNumberBones(int vertex)
{
	if(isSkin_)
	{
		return skinContext_->GetNumAssignedBones(vertex);
	}else
	if(isPhysique_)
	{
		IPhyVertexExport *vtxExport = phyContext_->GetVertexInterface(vertex);
		if (vtxExport)
		{
			if(vtxExport->GetVertexType() & BLENDED_TYPE)
			{
				IPhyBlendedRigidVertex *vtxBlend = (IPhyBlendedRigidVertex *)vtxExport;
				int numbones = vtxBlend->GetNumberNodes();
				phyContext_->ReleaseVertexInterface(vtxExport);
				return numbones;
			}else
				return 1;
		}
	}
	return 0;
}
//==================================================================================================================
float IVisSkin::GetWeight(int vertexIndex, int boneIndex)
{
	if(isSkin_)
	{
		const int numBones = skinContext_->GetNumAssignedBones(vertexIndex);
		if(boneIndex<numBones)
		{
			float weight = skinContext_->GetBoneWeight(vertexIndex,boneIndex);
			return weight;
		}
	}else
	if(isPhysique_)
	{
		IPhyVertexExport* vertexExport = phyContext_->GetVertexInterface(vertexIndex);
		if (vertexExport)
		{
			int vertexType = vertexExport->GetVertexType();
			if(vertexType == RIGID_BLENDED_TYPE)
			{
				IPhyBlendedRigidVertex* blended;
				blended = static_cast<IPhyBlendedRigidVertex*>(vertexExport);
				return blended->GetWeight(boneIndex);
			}else
				return 1.f;
		}
	}
	return 0.f;
}
//==================================================================================================================
IVisNode* IVisSkin::GetIBone(int vertexIndex, int boneIndex)
{
	if(isSkin_)
	{
		int n = skinContext_->GetAssignedBone(vertexIndex,boneIndex);
		INode* bone = skin_->GetBone(n);
		if(!bone)
			return NULL;
		return GetVisExporter()->FindNode(bone);
	}else
	if(isPhysique_)
	{
		IPhyVertexExport* vertexExport = phyContext_->GetVertexInterface(vertexIndex);
		if (vertexExport)
		{
			int vertexType = vertexExport->GetVertexType();
			if(vertexType == RIGID_BLENDED_TYPE)
			{
				IPhyBlendedRigidVertex* blended;
				blended = static_cast<IPhyBlendedRigidVertex*>(vertexExport);
				INode* bone = blended->GetNode(boneIndex);
				if(!bone)
					return NULL;
				return GetVisExporter()->FindNode(bone);
			}else
			if(vertexType == RIGID_TYPE)
			{
				IPhyRigidVertex* rigid;
				rigid = static_cast<IPhyRigidVertex*>(vertexExport);
				INode* bone  = rigid->GetNode();
				if(!bone)
					return NULL;
				return GetVisExporter()->FindNode(bone);
			}
		}
	}
	return NULL;
}
//==================================================================================================================
int IVisSkin::GetVertexCount()
{
	if(isSkin_)
		return skinContext_->GetNumPoints();
	if(isPhysique_)
		return phyContext_->GetNumberVertices();
	return 0;
}
//==================================================================================================================
///// IVisLight ////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisLight::IVisLight(IVisNode* node,TimeValue& t)
{
	node_ = node;
	genLight_ = (GenLight*)node->GetMAXNode()->EvalWorldState(t).obj;
}
//==================================================================================================================
IVisLight::~IVisLight()
{

}
//==================================================================================================================
int IVisLight::GetType()
{
	return genLight_->Type();
}
//==================================================================================================================
Point3 IVisLight::GetRGBColor(TimeValue t)
{
	return genLight_->GetRGBColor(t);
}
//==================================================================================================================
float IVisLight::GetAttenStart(TimeValue t)
{
	return genLight_->GetAtten(t,ATTEN_START);
}
//==================================================================================================================
float IVisLight::GetAttenEnd(TimeValue t)
{
	return genLight_->GetAtten(t,ATTEN_END);
}
//==================================================================================================================
const char* IVisLight::GetBitmapName()
{
	Texmap *tex=genLight_->GetProjMap();
	if(tex)
	if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
	{
			return ((BitmapTex*)tex)->GetMapName();
	}
	return NULL;
}
//==================================================================================================================
float IVisLight::GetIntensity(TimeValue t)
{
	return genLight_->GetIntensity(t);
}
//==================================================================================================================
///// IVisLight ////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================================================================================================
IVisCamera::IVisCamera(IVisNode* node)
{
	node_ = node;
	genCamera_ = (GenCamera*)node->GetMAXNode()->EvalWorldState(0).obj;
}
//==================================================================================================================
IVisCamera::~IVisCamera()
{

}
//==================================================================================================================
float IVisCamera::GetFov()
{
	Interval interval;
	CameraState camState;
	genCamera_->EvalCameraState(0, interval, &camState);
	return camState.fov;
}
