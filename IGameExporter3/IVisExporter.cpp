#include "stdafx.h"
#include "IVisExporter.h"
#include "RootExport.h"
#include "stdmat.h"
#include "Serialization\SerializationFactory.h"

REGISTER_CLASS(IVisNode, IVisNode, "IVisNode");

NodeNames IVisNode::checkMap_;

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
		errorStr += "Пробелы в начале, ";
	}
	// удаление пробелов в конце
	pos = workStr.find_last_not_of(" ");
	if(pos != string::npos && pos != workStr.size()-1)
	{
		workStr = workStr.erase(pos+1);
		errorStr += "Пробелы в конце, ";
	}
	// удаление двойных пробелов
	while((pos = workStr.find("  ")) != string::npos)
	{
		workStr.replace(pos,2," ");
		errorStr = "Двойные пробелы, ";
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


// IVisMaterial

IVisMaterial::IVisMaterial(Mtl* mtl):
mtl_(mtl)
{
	if(mtl_){
		name_ = TrimString((const char*)mtl_->GetName());
		FindSubMaterials();
		FindTexMaps();
	}
}

void IVisMaterial::FindSubMaterials()
{
	for(int i=0; i<mtl_->NumSubMtls(); i++){
		Mtl* subMtl = mtl_->GetSubMtl(i);
		if(subMtl){
			IVisMaterial* mat = new IVisMaterial(subMtl);
			subMaterials_.push_back(mat);
		}
	}
}

void IVisMaterial::FindTexMaps()
{
	if(!IsStandartType()) 
		return;

	for(int i=0; i<mtl_->NumSubTexmaps(); i++){
		Texmap* texmap = mtl_->GetSubTexmap(i);
		if(texmap){
			if(!((StdMat*)mtl_)->MapEnabled(i))
				continue;
			if(texmap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0x00))
				continue;
			IVisTexmap* tm = new IVisTexmap(texmap,this,i);
			texmaps_.push_back(tm);
		}
	}
}

bool IVisMaterial::IsStandartType()
{
	return (mtl_->ClassID() == Class_ID(DMTL_CLASS_ID, 0));
}

Color IVisMaterial::GetAmbient(int t)
{
	return ((StdMat*)mtl_)->GetAmbient(t);
}

Color IVisMaterial::GetDiffuse(int t)
{
	return ((StdMat*)mtl_)->GetDiffuse(t);
}

Color IVisMaterial::GetSpecular(int t)
{
	return ((StdMat*)mtl_)->GetSpecular(t);
}

float IVisMaterial::GetSpecularLevel(int t)
{
	return ((StdMat*)mtl_)->GetShinStr(t);
}

float IVisMaterial::GetShininess(int t)
{
	return ((StdMat*)mtl_)->GetShininess(t);
}

float IVisMaterial::GetOpacity(int t)
{
	return ((StdMat*)mtl_)->GetOpacity(t);
}

int IVisMaterial::GetShading()
{
	return ((StdMat*)mtl_)->GetShading();
}

int IVisMaterial::GetTransparencyType()
{
	return ((StdMat*)mtl_)->GetTransparencyType();
}

Interval IVisMaterial::Validity(int t)
{
	return mtl_->Validity(t);
}

bool IVisMaterial::IsMultiType()
{
	return GetSubMaterialCount()>0?true:false;
}

int IVisMaterial::GetSubMaterialCount()
{
	return subMaterials_.size();
}

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

	return subMaterials_.empty()?0:subMaterials_[0];
}

int IVisMaterial::GetTexmapCount()
{
	return texmaps_.size();
}

IVisTexmap* IVisMaterial::GetTexmap(int i)
{
	if(i<texmaps_.size())
		return texmaps_[i];
	return 0;
}

void IVisMaterial::serialize(Archive& ar)
{
	if(ar.isOutput()){
		string comboList;
		AnimationGroups::const_iterator gi;
		FOR_EACH(exporter->animationGroups_, gi)
			comboList += gi->name + "|";
		animationGroupName_.setComboList(comboList.c_str());
	}
	ar.serialize(animationGroupName_, GetName(), GetName());
	if(ar.isInput()){
		AnimationGroups::iterator gi;
		FOR_EACH(exporter->animationGroups_, gi){
			gi->materialsNames.remove(GetName());
			if(gi->name == animationGroupName_.value())
				gi->materialsNames.add(GetName());
		}
	}
}

// IVisTexmap 

IVisTexmap::IVisTexmap(Texmap* texmap, IVisMaterial* material, int slot):
texmap_(texmap),
material_(material),
slot_(slot)
{
}

const char* IVisTexmap::GetBitmapFileName()
{
	return ((BitmapTex *)texmap_)->GetMapName();
}

float IVisTexmap::GetAmount()
{
	return ((StdMat*)material_->GetMtl())->GetTexmapAmt(slot_,0);
}

int IVisTexmap::GetTextureTiling()
{
	return texmap_->GetTextureTiling();
}

void IVisTexmap::GetUVTransform(Matrix3& mat)
{
	texmap_->GetUVTransform(mat);
}


// IVisNode 

IVisNode::IVisNode(INode* node, IVisNode* parent):
node_(node),
parentNode_(parent)
{
	xassert(node);
	mesh_ = 0;
	material_ = 0;
	light_ = 0;
	camera_ = 0;
	
	isAnimated_ = false;
	visibilityAnimated_ = false;
	anisotropicScale_ = false;
	export_ = false;

	isNonDelete_ = false;
	isLogic_ = false;
	isBound_ = false;

	index_ = -1;
	
	for(int i=0; i<node_->NumberOfChildren(); i++){
		IVisNode* node = new IVisNode(node_->GetChildNode(i),this);
		childNodes_.push_back(node);
	}

	if(node_->GetMtl())
		material_ = exporter->GetIMaterial(node_->GetMtl());
	
	nodeName = TrimString(node_->GetName());

	if(GetType() == MESH_TYPE)
		mesh_ = new IVisMesh(this);

	if(GetType() == LIGHT_TYPE)
		light_ = new IVisLight(this);

	if(GetType() == CAMERA_TYPE)
		camera_ = new IVisCamera(this);

	if(!parentNode_)
		checkMap_.clear();
}

bool IVisNode::isMesh() const
{
	if(isAux())
		return false;

	if(GetType() == MESH_TYPE)
		return true;
	
	return false;
}

bool IVisNode::isLight() const 
{
	if(isAux())
		return false;

	if(strstr(GetName(), "leaf"))
		return false;

	if(GetType() == LIGHT_TYPE)
		return true;
	return false;
}

bool IVisNode::isCamera() const
{
	string name = GetName();
	if(name != "Camera01")
		return false;
	if(GetType() == CAMERA_TYPE)
		return true;
	return false;
}

bool IVisNode::isLeaf() const 
{
	if(isAux())
		return false;

	if(!strstr(GetName(), "leaf"))
		return false;

	if(GetType() == LIGHT_TYPE)
		return true;
	return false;
}

void IVisNode::addBoundRecursive(sBox6f& bound, float& boundRadius)
{
	Vect3f v = convert(node_->GetNodeTM(0).GetTrans());
	bound.addPoint(v);
	boundRadius = max(boundRadius, v.norm());

	if(GetType() == MESH_TYPE){
		IVisMesh* mesh = GetMesh();
		for(int i = 0; i < mesh->GetNumberVerts(); i++){
			Vect3f v = convert(mesh->GetVertex(i));
			bound.addPoint(v); 
			boundRadius = max(boundRadius, v.norm());
		}
	}

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->addBoundRecursive(bound, boundRadius);
}

void IVisNode::cacheTransformRecursive(int time)
{
	worldTMs_.resize(exporter->toFrame(exporter->endTime()) + 2);

	int frame = exporter->toFrame(time);
    xassert(frame < worldTMs_.size());
	worldTMs_[frame] = node_->GetNodeTM(time);

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->cacheTransformRecursive(time);
}

Matrix3 IVisNode::GetWorldTM(int time)
{
	//return node_->GetNodeTM(time);
	int frame = exporter->toFrame(time);
	xassert(frame < worldTMs_.size());
	return worldTMs_[frame];
}

Matrix3 IVisNode::GetLocalTM(int t)
{
	if(!parentNode_)
		return GetWorldTM(t);
	Matrix3 matParent = parentNode_->GetWorldTM(t);
	matParent.Invert();
	return GetWorldTM(t)*matParent;
}

void IVisNode::prepareToExportRecursive()
{
	if(checkMap_.exists(GetName()))
		Msg("Error: Node %s. несколько объектов с таким именем.\n", GetName());	
	else
		checkMap_.add(GetName());

	if(!parentNode_)
		export_ = EXPORT_LOGIC | EXPORT_GRAPHICS;

	if(parentNode_ && exporter->findAnimationGroupIndex(GetName()) != exporter->findAnimationGroupIndex(parentNode_->GetName()))
		parentNode_->export_ = EXPORT_LOGIC | EXPORT_GRAPHICS;

	if(isLogic_)
		export_ |= EXPORT_LOGIC;

	if(isNonDelete_)
		export_ |= EXPORT_GRAPHICS;

	if(GetType() == LIGHT_TYPE || GetType() == CAMERA_TYPE)
		export_ = EXPORT_LOGIC | EXPORT_GRAPHICS;

	const char* effect = "effect:";
	if(!strncmp(GetName(), effect, strlen(effect)))
		export_ = EXPORT_LOGIC | EXPORT_GRAPHICS;

	if(isMesh()){
		IVisMesh* mesh = GetMesh();
		if(!mesh->GetSkin())
			export_ |= EXPORT_GRAPHICS;
		else{
			IVisSkin* skin = mesh->GetSkin();
			int num_skin_verts = skin->GetVertexCount();
			for(int ipnt = 0; ipnt < num_skin_verts; ipnt++){
				int nbones = skin->GetNumberBones(ipnt);
				if(nbones==0)
					export_ |= EXPORT_GRAPHICS;
				else{
					for(int ibone = 0; ibone < nbones; ibone++){
						IVisNode* boneNode = skin->GetIBone(ipnt, ibone);
						boneNode->export_ |= EXPORT_GRAPHICS;
						//int inode = FindNodeIndex(pnode);
						//if(inode < 0){
						//	Msg("Error: %s не прилинкованна к group center\n",pnode->GetName());
						//	return;
						//}
					}
				}
			}
		}
	}

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->prepareToExportRecursive();
}

void IVisNode::processAnimationRecursive()
{
	const StaticAnimationChains& animationChains = exporter->animationChains_;
	chainsLogic_.resize(animationChains.size());
	chainsGraphics_.resize(animationChains.size());
	for(int i = 0; i < animationChains.size(); i++){
		const StaticAnimationChain& ac = animationChains[i];
		processAnimation(chainsLogic_[i], ac.begin_frame, ac.intervalSize(), ac.cycled, EXPORT_LOGIC);
		processAnimation(chainsGraphics_[i], ac.begin_frame, ac.intervalSize(), ac.cycled, EXPORT_GRAPHICS);
	}

	if(anisotropicScale_)
		Msg("Node %s. Не поддерживается анизотропный scale\n", GetName());

	if(visibilityAnimated_)
		Msg("Node %s. Трек видимости\n", GetName());

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->processAnimationRecursive();
}

IVisNode* IVisNode::goodParent(int export) const
{
	for(IVisNode* parent = parentNode_; ; parent = parent->parentNode_){
		if(!parent)
			return 0;
		if(parent->export_ & export)
			return parent;
	}
}

IVisNode* IVisNode::root() 
{
	for(IVisNode* node = this; ; node = node->parentNode_)
		if(!node->parentNode_)
			return node;
}

void IVisNode::processAnimation(StaticNodeAnimation& chain, int interval_begin, int interval_size, bool cycled, int export)
{
	RefinerPosition ipos;
	RefinerRotation irot;
	RefinerScale iscale;
	RefinerBool ivis;
	bool anisotropicScale = false;

	for(int icurrent = 0; icurrent < interval_size; icurrent++){
		int current_max_time = exporter->toTime(icurrent + interval_begin);
		// Ранее: у root - m.NoRot();m.NoScale();, root->child - mparent.NoTrans();
		Matrix3 m = GetWorldTM(current_max_time);
		IVisNode* parent = goodParent(export);
		if(!parent){
			m.NoRot();
			m.NoScale();
		}
		else{
			Matrix3 matParentInv = parent->GetWorldTM(current_max_time);
			matParentInv.Invert();
			m = m*matParentInv;
		} 
			
		//if(!parentNode_->parentNode_){
		//	Matrix3 mparent = parentNode_->GetWorldTM(current_max_time);
		//	mparent.NoTrans();
		//	m = m*mparent;
		//}

		RightToLeft(m);

		AffineParts ap;
		decomp_affine(m, &ap);
		Quat q = ap.q;
		VectPosition pos;
		pos[0]=ap.t.x;
		pos[1]=ap.t.y;
		pos[2]=ap.t.z;

		ipos.addValue(pos);

		VectRotation rot;
		rot[0]=ap.q.x;
		rot[1]=ap.q.y;
		rot[2]=ap.q.z;
		rot[3]=ap.q.w;
		//Матрица вращения может скачком менять знак.
		//Необходимо препятствовать этому явлению.
		irot.addValue(rot);

		VectScale s;
		float mids = (ap.k.x+ap.k.y+ap.k.z)/3;
		float deltas = fabsf(mids-ap.k.x)+fabsf(mids-ap.k.y)+fabsf(mids-ap.k.z);
		if(ap.f < 0 && mids > 0)
			mids = -mids;

		if(deltas>1e-3f)
			anisotropicScale = true;

		s[0] = mids;
		iscale.addValue(s);

		bool visible = GetVisibility(current_max_time) > 0.5f;
		ivis.addValue(visible);
		if(!visible)
			visibilityAnimated_ = true;
	}

	ipos.refine(exporter->position_delta,cycled);
	ipos.export(chain.position);

	irot.refine(exporter->rotation_delta, cycled);
	irot.export(chain.rotation);

	iscale.refine(exporter->scale_delta, cycled);
	iscale.export(chain.scale);

	ivis.refine(interval_size, cycled);
	ivis.export(chain.visibility);

	if(!ipos.constant() || !irot.constant() || !iscale.constant())
		isAnimated_ = true;

	if(!iscale.constant() && anisotropicScale)
		anisotropicScale_ = true;
}

void IVisNode::exportRecursive(Static3dxBase* object, bool logic)
{
	if(isMesh() && visibilityAnimated_)
		export_ |= EXPORT_GRAPHICS;

	if(export_ & (logic ? EXPORT_LOGIC : EXPORT_GRAPHICS)){
		index_ = object->nodes.size();
		object->nodes.push_back(StaticNode());
		StaticNode& staticNode = object->nodes.back();

		staticNode.name = GetName();
		staticNode.inode = index();

		IVisNode* parent = goodParent(logic ? EXPORT_LOGIC : EXPORT_GRAPHICS);
		staticNode.iparent = parent ? parent->index() : -1;

		staticNode.chains = logic ? chainsLogic_ : chainsGraphics_;

		Matrix3 m = GetWorldTM(0); // !!!
		if(!parent){
			m.NoScale();
			m.NoRot();
		}

		RightToLeft(m);
		staticNode.inv_begin_pos = convert(m);
		staticNode.inv_begin_pos.Invert();
	}
	else
		index_ = -1;

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->exportRecursive(object, logic);
}

void IVisNode::exportLightsRecursive(Static3dxBase* object)
{
	if(isLight()){
		object->lights.push_back(StaticLight());
		object->lights.back().export(GetLight(), index());
	}

	if(isLeaf()){
		object->leaves.push_back(StaticLeaf());
		object->leaves.back().export(GetLight(), index());
	}

	if(isCamera()){
		IVisCamera* camera = GetCamera();
		if(camera){
			object->cameraParams.camera_node_num = index();
			object->cameraParams.fov = camera->GetFov();
		}
	}

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->exportLightsRecursive(object);
}

void IVisNode::exportBoundsRecursive(Static3dxBase* object)
{
	if(!strcmp(GetName(), "logic bound")){
		if(GetType() != MESH_TYPE)
			Msg("Объект с именем logic bound должен состоять из треугольников.\n");
		else{
			Matrix3 mRoot = root()->GetLocalTM(exporter->startTime());
			mRoot.NoScale();
			mRoot.NoRot();
			GetMesh()->exportBound(object->boundBox, Inverse(mRoot));
			object->boundRadius = object->boundBox.max.distance(object->boundBox.min)*0.5f;
		}
	}

	NodeNames::const_iterator i = find(object->boundNodes.begin(), object->boundNodes.end(), GetName());
	if(i != object->boundNodes.end()){
		if(GetType() != MESH_TYPE)
			Msg("Логический bound (%s) должен состоять из треугольников. \n", GetName());
		else{
			Matrix3 mInv = Inverse(GetWorldTM(exporter->startTime()));
			GetMesh()->exportBound(object->localLogicBounds[index()], mInv);
		}
	}

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->exportBoundsRecursive(object);
}

void IVisNode::exportNodesNamesRecursive(Static3dxBase* object)
{
	if(isNonDelete_)
		object->nonDeleteNodes.push_back(GetName());
	if(isLogic_)
		object->logicNodes.push_back(GetName());
	if(isBound_)
		object->boundNodes.push_back(GetName());

	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		(*ni)->exportNodesNamesRecursive(object);
}

void IVisNode::serialize(Archive& ar)
{
	if(ar.filter(SERIALIZE_NON_DELETE))
		ar.serialize(isNonDelete_, GetName(), "^");

	if(ar.filter(SERIALIZE_LOGIC))
		ar.serialize(isLogic_, GetName(), "^");

	if(ar.filter(SERIALIZE_BOUND))
		ar.serialize(isBound_, GetName(), "^");

	if(ar.filter(SERIALIZE_ANIMATION_GROUP)){
		if(ar.isOutput()){
			string comboList;
			AnimationGroups::const_iterator gi;
			FOR_EACH(exporter->animationGroups_, gi)
				comboList += gi->name + "|";
			animationGroupName_.setComboList(comboList.c_str());
		}
		if(parentNode_)
			ar.serialize(animationGroupName_, GetName(), "^");
		if(ar.isInput()){
			AnimationGroups::iterator gi;
			FOR_EACH(exporter->animationGroups_, gi){
				gi->nodesNames.remove(GetName());
				if(gi->name == animationGroupName_.value())
					gi->nodesNames.add(GetName());
			}
		}
	}

	if(ar.filter(SERIALIZE_MESHES) && isMesh() || ar.filter(SERIALIZE_LIGHTS) && isLight() || ar.filter(SERIALIZE_LEAVES) && isLeaf()){
		visibilitySet_.serializeButton(ar, GetName());
	}


	IVisNodes::iterator ni;
	FOR_EACH(childNodes_, ni)
		if(ar.filter(SERIALIZE_ANIMATION_GROUP))
			ar.serialize(**ni, (*ni)->GetName(), (*ni)->GetName());
		else if(!ar.filter(SERIALIZE_MESHES | SERIALIZE_LIGHTS | SERIALIZE_LEAVES))
			ar.serialize(**ni, (*ni)->GetName(), (*ni)->GetName());
		else
			(*ni)->serialize(ar);
}

NodeType IVisNode::GetType() const
{
	ObjectState os = node_->EvalWorldState(0); 
	return (NodeType)os.obj->SuperClassID();
}

bool IVisNode::isAux() const
{
	ObjectState os = node_->EvalWorldState(0); 
	if(os.obj && os.obj->ClassID() == BONE_OBJ_CLASSID)
		return true;
	Control *control;
	control = node_->GetTMController();

	if((control->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
		(control->ClassID() == BIPBODY_CONTROL_CLASS_ID))
		return true;

	const char* name = GetName();
	if(!strncmp(name, "Bip", 3))
		return true;
	if(!strcmp(name, "logic bound"))
		return true;
	if(!strcmp(name,"_base_"))
		return true;

	if(strstr(name,"logic node bound"))
		return true;
	if(isBound_)
		return true;


	return false;
}

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
	return 0;
}

int IVisNode::GetChildNodeCount()
{
	return childNodes_.size();
}

IVisNode* IVisNode::GetChildNode(int i)
{
	return childNodes_[i];
}

bool IVisNode::isTarget() const
{
	return node_->IsTarget();
}

float IVisNode::GetVisibility(int t)
{
	return node_->GetVisibility(t);
}

// IVisMesh

IVisMesh::IVisMesh(IVisNode* node):
node_(node)
{
	triObject_ = 0;
	mesh_ = 0;

	ObjectState os = node_->GetMAXNode()->EvalWorldState(0);
	if(!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) 
		return;

	if(os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		triObject_ = (TriObject*)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
	else 
		return;

	//BOOL needDelete = FALSE;
	//mesh_ = triObject_->GetRenderMesh(0,node_->GetMAXNode(),theNullView,needDelete);
	mesh_ = &triObject_->GetMesh();
	//mesh_->buildNormals(); // пропадали к моменту использования!!!
	//mesh_->checkNormals(TRUE);
	pivMat = node_->GetMAXNode()->GetObjectTM(0);

	skin_ = new IVisSkin(this);
}

int IVisMesh::GetNumberVerts()
{
	return mesh_->getNumVerts();
}

int IVisMesh::GetNumberFaces()
{
	return mesh_->getNumFaces();
}

bool IVisMesh::IsMapSupport(int n)
{
	return mesh_->mapSupport(n);
}

int IVisMesh::GetNumMapVerst(int n)
{
	return mesh_->getNumMapVerts(n);
}

UVVert* IVisMesh::GetMapVerts(int n)
{
	return mesh_->mapVerts(n);
}

TVFace* IVisMesh::GetMapFaces(int n)
{
	return mesh_->mapFaces(n);
}

FaceEx IVisMesh::GetFace(int n)
{
	FaceEx face;
	if(HasNegativeScale()){
		face.vert[0] = mesh_->faces[n].v[0];
		face.vert[1] = mesh_->faces[n].v[2];
		face.vert[2] = mesh_->faces[n].v[1];
	}
	else{
		face.vert[0] = mesh_->faces[n].v[0];
		face.vert[1] = mesh_->faces[n].v[1];
		face.vert[2] = mesh_->faces[n].v[2];
	}
	face.index = n;
	return face;
}

bool IVisMesh::HasNegativeScale()
{
	return pivMat.Parity() ? true : false;
}

Point3 IVisMesh::GetVertex(int n)
{
	return mesh_->verts[n]*pivMat;
}

Point3 IVisMesh::GetNormal(int faceIndex, int vertex)
{
	RVertex* rv = mesh_->getRVertPtr(vertex);
	Face* f = &mesh_->faces[faceIndex];
	DWORD smGroup = f->smGroup;
	int numNormals;
	Point3 vertexNormal;

	// Is normal specified
	// SPECIFIED_NORMAL is not currently used, but may be used in future versions.
	if(rv->rFlags & SPECIFIED_NORMAL)
		vertexNormal = rv->rn.getNormal();

	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	else 
		if((numNormals = rv->rFlags & NORCT_MASK) && smGroup) 
		{
			// If there is only one vertex is found in the rn member.
			if(numNormals == 1) 
			{
				vertexNormal = rv->rn.getNormal();
			}
			else
			{
				// If two or more vertices are there you need to step through them
				// and find the vertex with the same smoothing group as the current face.
				// You will find multiple normals in the ern member.
				for(int i = 0; i < numNormals; i++)
				{
					if(rv->ern[i].getSmGroup() & smGroup)
					{
						vertexNormal =  rv->ern[i].getNormal();
					}
				}
			}
		}
		else{
			// Get the normal from the Face if no smoothing groups are there
			vertexNormal = mesh_->getFaceNormal(faceIndex);
		}

		Matrix3 tmRotation;
		tmRotation = pivMat;
		tmRotation.NoTrans();
		tmRotation.NoScale();
		return (vertexNormal*tmRotation).Normalize();
}

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

IVisMaterial* IVisMesh::GetMaterialFromFace(int n)
{
	Face& face = mesh_->faces[n];
	MtlID id = face.getMatID();
	IVisMaterial* material = node_->GetMaterial();
	if(!material)
		return 0;
	if(material->IsMultiType()){
		if(id < material->GetSubMaterialCount())
			return material->GetSubMaterial(id);
		return material->GetSubMaterial(0);
	}
	return material;
}

IVisSkin* IVisMesh::GetSkin()
{
	if(skin_->IsSkin()||skin_->IsPhysique())
		return skin_;
	else
	{
		if(skin_->Init())
			return skin_;
	}
	return 0;
}

UVFace IVisMesh::GetMapVertex(int mapID,FaceEx &face)
{
	UVFace uvFace;
	TVFace* tvFace = GetMapFaces(mapID);
	UVVert* uvVert = GetMapVerts(mapID);
	if(tvFace && uvVert){
		if(HasNegativeScale()){
			uvFace.v[0] = uvVert[tvFace[face.index].t[0]];
			uvFace.v[2] = uvVert[tvFace[face.index].t[1]];
			uvFace.v[1] = uvVert[tvFace[face.index].t[2]];
		}
		else{
			uvFace.v[0] = uvVert[tvFace[face.index].t[0]];
			uvFace.v[1] = uvVert[tvFace[face.index].t[1]];
			uvFace.v[2] = uvVert[tvFace[face.index].t[2]];
		}
	}
	return uvFace;
}

void IVisMesh::exportBound(sBox6f& bound, const Matrix3& m)
{
	bound.invalidate();
	int num_vertex = GetNumberVerts();
	for(int i=0;i<num_vertex;i++){
		Vect3f v = convert(GetVertex(i)*m);
		RightToLeft(v);
		bound.addPoint(v);
	}
}

// IVisSkin

IVisSkin::IVisSkin(IVisMesh* mesh):
mesh_(mesh)
{
	skin_ = 0;
	phys_ = 0;
	phyContext_= 0;
	modifer_ = 0;
	isPhysique_ = false;
	isSkin_ = false;

	if(mesh_)
		Init();
}

bool IVisSkin::Init()
{
	Modifier* modifer_ = FindModifer(Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B));
	if(modifer_){
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
	}
	else{
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

	return 0;
}

int IVisSkin::GetNumberBones(int vertex)
{
	if(isSkin_)
	{
		return skinContext_->GetNumAssignedBones(vertex);
	}else
	if(isPhysique_)
	{
		IPhyVertexExport *vtxExport = phyContext_->GetVertexInterface(vertex);
		if(vtxExport)
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
		if(vertexExport)
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

IVisNode* IVisSkin::GetIBone(int vertexIndex, int boneIndex)
{
	if(isSkin_)
	{
		int n = skinContext_->GetAssignedBone(vertexIndex,boneIndex);
		INode* bone = skin_->GetBone(n);
		if(!bone)
			return 0;
		return exporter->FindNode(bone);
	}else
	if(isPhysique_)
	{
		IPhyVertexExport* vertexExport = phyContext_->GetVertexInterface(vertexIndex);
		if(vertexExport)
		{
			int vertexType = vertexExport->GetVertexType();
			if(vertexType == RIGID_BLENDED_TYPE)
			{
				IPhyBlendedRigidVertex* blended;
				blended = static_cast<IPhyBlendedRigidVertex*>(vertexExport);
				INode* bone = blended->GetNode(boneIndex);
				if(!bone)
					return 0;
				return exporter->FindNode(bone);
			}else
			if(vertexType == RIGID_TYPE)
			{
				IPhyRigidVertex* rigid;
				rigid = static_cast<IPhyRigidVertex*>(vertexExport);
				INode* bone  = rigid->GetNode();
				if(!bone)
					return 0;
				return exporter->FindNode(bone);
			}
		}
	}
	return 0;
}

int IVisSkin::GetVertexCount()
{
	if(isSkin_)
		return skinContext_->GetNumPoints();
	if(isPhysique_)
		return phyContext_->GetNumberVertices();
	return 0;
}


// IVisLight 

IVisLight::IVisLight(IVisNode* node)
{
	node_ = node;
	genLight_ = (GenLight*)node->GetMAXNode()->EvalWorldState(0).obj;
}

const char* IVisLight::GetName()
{
	return node_->GetName();
}

int IVisLight::GetType()
{
	return genLight_->Type();
}

Point3 IVisLight::GetRGBColor(int t)
{
	return genLight_->GetRGBColor(t);
}

float IVisLight::GetAttenStart(int t)
{
	return genLight_->GetAtten(t,ATTEN_START);
}

float IVisLight::GetAttenEnd(int t)
{
	return genLight_->GetAtten(t,ATTEN_END);
}

const char* IVisLight::GetBitmapName()
{
	Texmap *tex = genLight_->GetProjMap();
	if(tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
		return ((BitmapTex*)tex)->GetMapName();
	return 0;
}

float IVisLight::GetIntensity(int t)
{
	return genLight_->GetIntensity(t);
}


IVisCamera::IVisCamera(IVisNode* node)
{
	genCamera_ = (GenCamera*)node->GetMAXNode()->EvalWorldState(0).obj;
}

float IVisCamera::GetFov()
{
	Interval interval;
	CameraState camState;
	genCamera_->EvalCameraState(0, interval, &camState);
	return camState.fov;
}
