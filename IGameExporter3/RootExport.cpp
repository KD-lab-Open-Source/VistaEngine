#include "StdAfx.h"
#include "RootExport.h"
#include "Interpolate.h"
#include "render\3dx\umath.h"
#include "CS\BipedApi.h"

#include "Serialization\BinaryArchive.h"
#include "Serialization\XPrmArchive.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\Decorators.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\EnumDescriptor.h"
#include "Serialization\SerializationFactory.h"
#include "kdw/PropertyEditor.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(Exporter, Lod, "Lod")
REGISTER_ENUM_ENCLOSED(Exporter, LOD0, "Экспортировать ЛОД 0")
REGISTER_ENUM_ENCLOSED(Exporter, LOD1, "Экспортировать ЛОД 1")
REGISTER_ENUM_ENCLOSED(Exporter, LOD2, "Экспортировать ЛОД 2")
END_ENUM_DESCRIPTOR_ENCLOSED(Exporter, Lod)

Exporter* exporter;

DWORD WINAPI ProgressFunction(LPVOID arg)
{
	return 0;
}

void RightToLeft(Matrix3& m)
{
	Point3 p=m.GetRow(0);
	p.y=-p.y;
	m.SetRow(0,p);

	p=m.GetRow(1);
	p.x=-p.x;
	p.z=-p.z;
	m.SetRow(1,p);

	p=m.GetRow(2);
	p.y=-p.y;
	m.SetRow(2,p);

	p=m.GetRow(3);
	p.y=-p.y;
	m.SetRow(3,p);
}

Exporter::Exporter(Interface* maxInterface, const char* filename)
{
	exporter = this;
	interface_ = maxInterface;

	curCount_ = 0;
	maxCount_ = 0;

	textLog_ = false;
	dontExport_ = false;
	show_info_polygon = false;

	relative_position_delta = 1e-4f;
	position_delta = 1e-2f;
	rotation_delta = 2e-4f;
	scale_delta = 1e-3f;

	for(int i=0; i < interface_->GetRootNode()->NumberOfChildren(); i++){
		IVisNode* node = new IVisNode(interface_->GetRootNode()->GetChildNode(i));
		rootNodes_.push_back(node);
	}

	maxWeights = 1;
	lod_ = LOD0;

	startTime_ = interface_->GetAnimRange().Start();
	endTime_ = interface_->GetAnimRange().End();
	ticksPerFrame_ = GetTicksPerFrame();

	BuildMaterialList();

	fileName_ = filename;
	backupName_ = filename;
	backupName_ += ".bak";
	CopyFile(filename, backupName_.c_str(), false);
	
	loadData(filename);
}

Exporter::~Exporter()
{
	DeleteFile(backupName_.c_str());

	exporter = 0;
}

bool Exporter::loadData(const char* filename)
{
	load(filename);
	badLogic3dx_ = false;

	maxWeights = graphics3dx_->maxWeights;
	
	animationGroups_ = graphics3dx_->animationGroups_;
	animationChains_ = graphics3dx_->animationChains_;
	visibilitySets_ = graphics3dx_->visibilitySets_;

	StaticVisibilitySets::const_iterator si;
	FOR_EACH(graphics3dx_->visibilitySets_, si){
		StaticVisibilityGroups::const_iterator gi;
		FOR_EACH(si->visibilityGroups, gi){
			NodeNames::const_iterator ni;
			FOR_EACH(gi->meshes, ni){
				IVisNode* node = Find(ni->c_str());
				if(node)
					node->setVisibilitySet(si->name.c_str(), gi->name.c_str());
			}
		}
	}

	logos = graphics3dx_->logos.logos;
	
	NodeNames::iterator ni;
	FOR_EACH(graphics3dx_->nonDeleteNodes, ni){
		IVisNode* node = Find(ni->c_str());
		if(node)
			node->setIsNonDelete();
	}

	FOR_EACH(logic3dx_->logicNodes, ni){
		IVisNode* node = Find(ni->c_str());
		if(node)
			node->setIsLogic();
	}

	FOR_EACH(logic3dx_->boundNodes, ni){
		IVisNode* node = Find(ni->c_str());
		if(node)
			node->setIsBound();
	}

	AnimationGroups::iterator gi;
	FOR_EACH(graphics3dx_->animationGroups_, gi){
		NodeNames::iterator ni;
		FOR_EACH(gi->nodesNames, ni){
			IVisNode* node = Find(ni->c_str());
			if(node)
				node->setAnimationGroupName(gi->name.c_str());
		}
		NodeNames::iterator mi;
		FOR_EACH(gi->materialsNames, mi){
			IVisMaterial* material = FindMaterial(mi->c_str());
			if(material)
				material->setAnimationGroupName(gi->name.c_str());
		}
	}

	boundRadius_ = 0;
	bound_.invalidate();
	IVisNodes::iterator nit;
	FOR_EACH(rootNodes_, nit)
		(*nit)->addBoundRecursive(bound_, boundRadius_);

	ShowConsole(0);
	return true;
}

int Exporter::GetNumFrames()
{
	return (endTime_-startTime_)/ticksPerFrame_ + 1;
}

void Exporter::export(const char* filename)
{
#ifdef _DEBUG
	_clearfp();
	_controlfp( _controlfp(0,0) & ~(EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL | EM_INVALID),  MCW_EM); 
#endif

	startTime_ = 0;
	endTime_ = 1;
	StaticAnimationChains::iterator aci;
	FOR_EACH(animationChains_, aci){
		startTime_ = min(startTime_, toTime(aci->begin_frame));
		endTime_ = max(endTime_, toTime(aci->end_frame));
	}
	if(startTime_ < 0)
		endTime_ += -startTime_;
	endTime_ += 200*ticksPerFrame_;

	position_delta = boundRadius_*relative_position_delta;

	for(int time = startTime(); time <= endTime(); time += ticksPerFrame_){
		IVisNodes::iterator ni;
		FOR_EACH(rootNodes_, ni)
			(*ni)->cacheTransformRecursive(time);
	}

	if(!lod_){
		IVisNodes::iterator ni;
		FOR_EACH(rootNodes_, ni)
			(*ni)->prepareToExportRecursive();

		FOR_EACH(rootNodes_, ni)
			(*ni)->processAnimationRecursive();

		CheckMaterialInAnimationGroup();

		graphics3dx_->maxWeights = maxWeights;
		logic3dx_->maxWeights = maxWeights;
		
		graphics3dx_->nonDeleteNodes.clear();
		logic3dx_->logicNodes.clear();
		logic3dx_->boundNodes.clear();
		GetRootNode(0)->exportNodesNamesRecursive(graphics3dx_);
		GetRootNode(0)->exportNodesNamesRecursive(logic3dx_);
		graphics3dx_->logicNodes.clear();
		graphics3dx_->boundNodes.clear();
		logic3dx_->nonDeleteNodes.clear();
		
		graphics3dx_->logos.logos = logos;

		exportStatic3dx(true);
		exportStatic3dx(false);
	}
	else
		exportLOD(lod_);

	Static3dxFile::save(filename);

	if(textLog_)
		Static3dxFile::serialize(XPrmOArchive(setExtention(filename, "3dxText").c_str()));

#ifdef _DEBUG
	_clearfp();
	_controlfp(_controlfp(0,0) | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL | EM_INVALID,  MCW_EM); 
#endif
}

void Exporter::exportStatic3dx(bool logic)
{
	Static3dxBase* object = logic ? logic3dx_ : graphics3dx_;

	object->animationGroups_ = animationGroups_;
	object->animationChains_ = animationChains_;
	object->visibilitySets_ = visibilitySets_;

	if(dontExport_)
		return;

	object->nodes.clear();
	GetRootNode(0)->exportRecursive(object, logic);

	if(!logic)
		exportMaterials(graphics3dx_);

	if(!logic){
		graphics3dx_->tempMesh_.clear();
		GetRootNode(0)->exportMeshesRecursive(graphics3dx_->tempMesh_);
		graphics3dx_->lights.clear();
		graphics3dx_->leaves.clear();
		GetRootNode(0)->exportLightsRecursive(graphics3dx_);
	}

 	object->boundBox = bound_;
	object->boundRadius = boundRadius_;
	if(logic){
		logic3dx_->localLogicBounds.clear();
		logic3dx_->localLogicBounds.resize(logic3dx_->nodes.size(), sBox6f());
		GetRootNode(0)->exportBoundsRecursive(logic3dx_);
	}
}

void Exporter::exportLOD(int lod)
{
	int index = 0;
	StaticNodes::iterator ni;
	FOR_EACH(graphics3dx_->nodes, ni){
		IVisNode* node = Find(ni->name.c_str());
		xassert(node);
		node->setIndex(index++);
	}

	duplicate_material_names.clear();
	StaticMaterials& materials = graphics3dx_->materials;
	StaticMaterials::iterator mi;
	FOR_EACH(materials, mi)
		duplicate_material_names.push_back(mi->name);

	BuildMaterialList();

	TempMeshes& mesh = lod == 1 ? graphics3dx_->tempMeshLod1_ : graphics3dx_->tempMeshLod2_;
	mesh.clear();
	GetRootNode(0)->exportMeshesRecursive(mesh);
}

/*
Нельзя удалять также ноды, которые находятся на стыке разных анимационных групп,
чтобы не получилось, что суммируется анимация из разных групп.
Соответственно нужен формализм, где скопом определяется, какие ноды .

1. Нельзя удалять ноды, если они помечены как логические/неудаляемые.
2. Нельзя удаляить ноды у которых есть источники света/эффекты.
3. Нельзя удалять ноды, если к ним по иерархии привязанны меши либо ноды, входящии в другую анимационную группу.

Соотвественно такой алгоритм:
   1. Сначала удаляем неанимированные ноды.
   2. Вторым проходом смотрим - если к нодам не привязанно мешей, источников света либо нод, 
      которые входят в другую анимационную группу, то можно их удалить.

---------------------------------Попытка номер 2

Первым проходом удаляем все неанимированные ноды, которые не помеченны как неудаляемые/логические.
Вторым проходом - удаляем анимированные ноды, к которые 
	не расположенны на границе разных анимационных групп, не помеченны как неудаляемые/логические.
    в случае графики - не привязанны меши.

*/

IVisNode* Exporter::Find(const char* name)
{
	for(int loop = 0; loop < GetRootNodeCount();loop++)
	{
		IVisNode * pGameNode = GetRootNode(loop);
		IVisNode * out=FindRecursive(pGameNode,name);
		if(out)
			return out;
	}

	return 0;
}

IVisNode* Exporter::FindRecursive(IVisNode* pGameNode,const char* name)
{
	if(strcmp(pGameNode->GetName(),name)==0)
		return pGameNode;
	for(int count=0;count<pGameNode->GetChildNodeCount();count++)
	{
		IVisNode * pChildNode = pGameNode->GetChildNode(count);
		IVisNode * out=FindRecursive(pChildNode,name);
		if(out)
			return out;
	}

	return 0;
}

void Exporter::BuildMaterialList()
{
	materials.clear();
	int size = GetRoolMaterialCount();
	for(int i=0;i<size;i++){
		IVisMaterial* mat = GetRootMaterial(i);
		if(mat->IsMultiType()){
			for(int j=0; j<mat->GetSubMaterialCount(); j++){
				IVisMaterial *sub_mat=  mat->GetSubMaterial(j);
				materials.push_back(sub_mat);
			}
		}
		else
			materials.push_back(mat);
	}
	
	for(int i=0;i<materials.size();i++){ //duplicate material
		IVisMaterial* mat_i=materials[i];
		const char* mat_name_i=mat_i->GetName();
		for(int j=i+1;j<materials.size();j++){
			IVisMaterial* mat_j=materials[j];
			const char* mat_name_j=mat_j->GetName();
			if(strcmp(mat_name_i,mat_name_j)==0)
			{
				Msg("Error: Материал с именем %s встречается несколько раз.\n",mat_name_i);
			}
		}
	}

	if(!duplicate_material_names.empty()){
		IVisMaterials materials_correct(duplicate_material_names.size(),0);
		for(int i=0;i<materials.size();i++){
			IVisMaterial* mat=materials[i];
			const char* mat_name=mat->GetName();
			int idx;
			for(idx=0;idx<duplicate_material_names.size();idx++)
				if(duplicate_material_names[idx]==mat_name)
					break;
			if(idx<duplicate_material_names.size()){
				materials_correct[idx]=mat;
			}
			else{
				Msg("Error: Материал %s не найден в оригинальном файле.\n",mat_name);
				throw GameExporterError();
			}
		}
		materials=materials_correct;
	}

	for(int i=0;i<materials.size();i++){ // duplicate material
		IVisMaterial* mat_i=materials[i];
		if(mat_i==0)
			continue;
		const char* mat_name_i=mat_i->GetName();
		string diffname_i,fillcolorname_i;
		int num_tex = mat_i->GetTexmapCount();
		for(int t=0; t<num_tex; t++){
			if(mat_i->GetTexmap(t)->GetSlot() == ID_DI)
				diffname_i = extractFileName(mat_i->GetTexmap(t)->GetBitmapFileName());
			if(mat_i->GetTexmap(t)->GetSlot() == ID_FI)
				fillcolorname_i = extractFileName(mat_i->GetTexmap(t)->GetBitmapFileName());
		}
		for(int j=i+1;j<materials.size();j++){
			IVisMaterial* mat_j=materials[j];
			if(mat_j==0)
				continue;
			const char* mat_name_j=mat_j->GetName();
			string diffname_j,fillcolorname_j;
			int num_tex = mat_j->GetTexmapCount();
			for(int t=0; t<num_tex; t++){
				if(mat_j->GetTexmap(t)->GetSlot() == ID_DI)
					diffname_j = extractFileName(mat_j->GetTexmap(t)->GetBitmapFileName());
				if(mat_j->GetTexmap(t)->GetSlot() == ID_FI)
					fillcolorname_j = extractFileName(mat_j->GetTexmap(t)->GetBitmapFileName());
			}
			if(diffname_i == diffname_j && fillcolorname_i != fillcolorname_j)
				Msg("Error: Материалы (\"%s\", \"%s\") имеют одинаковую diffuse текстуру и разную текстру fiter color.\n",mat_name_i, mat_name_j);
		}
	}
}

void Exporter::CheckMaterialInAnimationGroup()
{
	for(int imat=0;imat<materials.size();imat++){
		IVisMaterial* mat=materials[imat];
		if(mat==0)
			continue;
		bool found=false;
		for(int iag=0; iag < animationGroups_.size(); iag++){
			AnimationGroup& ag = animationGroups_[iag];
			for(int im=0;im<ag.materialsNames.size();im++){
				if(ag.materialsNames[im] == mat->GetName())
				{
					found=true;
					break;
				}
			}
		}

		if(!found){
			Msg("Материал %s не входит ни в одну анимационную группу.\n",mat->GetName());
		}
	}
}

void Exporter::exportMaterials(Static3dxBase* object)
{
	object->materials.clear();
	int size = GetRoolMaterialCount();
	for(int i=0;i<size;i++){
		IVisMaterial *mat = GetRootMaterial(i);
		if(mat->IsMultiType()){
			for(int j = 0; j < mat->GetSubMaterialCount(); j++){
				object->materials.push_back(StaticMaterial());
				exportMaterial(object->materials.back(), mat->GetSubMaterial(j));
			}
		}
		else{
			object->materials.push_back(StaticMaterial());
			exportMaterial(object->materials.back(), mat);
		}
	}
}

int Exporter::FindMaterialIndex(IVisMaterial* mat)
{
	for(int i=0;i<materials.size();i++)
	if(materials[i]==mat)
		return i;

	return -1;
}

IVisMaterial* Exporter::FindMaterial(const char* name)
{
	for(int i=0;i<materials.size();i++)
	{
		IVisMaterial *mat=materials[i];
		if(mat==0)
			continue;
		const char* mat_name=mat->GetName();
		if(strcmp(name,mat_name)==0)
			return mat;
	}

	return 0;
}

void Exporter::ProgressStart(const char* title)
{
	curCount_=0;
	maxCount_=0;
	interface_->ProgressStart((TCHAR*)title,TRUE,ProgressFunction,0);
}

void Exporter::ProgressEnd()
{
	interface_->ProgressEnd();
}

void Exporter::ProgressUpdate(const char* title)
{
	curCount_++;
	if(title)
		interface_->ProgressUpdate(float(curCount_)/float(maxCount_)*100,FALSE,(TCHAR*)title);
	else
		interface_->ProgressUpdate(float(curCount_)/float(maxCount_)*100);
}

void Exporter::AddProgressCount(int cnt)
{
	curCount_=0;
	maxCount_=cnt;
}

IVisMaterial* Exporter::GetIMaterial(Mtl* mtl)
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

int Exporter::GetRootNodeCount()
{
	return rootNodes_.size();
}

IVisNode* Exporter::GetRootNode(int n)
{
	if(n<rootNodes_.size())
		return rootNodes_[n];
	return 0;
}

IVisNode* Exporter::FindNode(INode* node_)
{
	for(int i=0;i<rootNodes_.size(); i++)
	{
		IVisNode* node = rootNodes_[i]->FindNode(node_);
		if(node)
			return node;
	}
	return 0;
}

int Exporter::findAnimationGroupIndex(const char* nodeName) const
{
	AnimationGroups::const_iterator agi;
	FOR_EACH(animationGroups_, agi){
		NodeNames::const_iterator iNode;
		FOR_EACH(agi->nodesNames, iNode)
			if(*iNode == nodeName)
				return agi - animationGroups_.begin();
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////

class MaterialsSerializator
{
public:
	MaterialsSerializator(IVisMaterials& materials) : materials_(materials) {}

	void serialize(Archive& ar){
		IVisMaterials::iterator i;
		FOR_EACH(materials_, i)
			(*i)->serialize(ar);
	}

private:
	IVisMaterials materials_;
};

class AnimationGroupsSerializer
{
public:
	AnimationGroupsSerializer(AnimationGroups& groups) : groups_(groups) {}

	void serialize(Archive& ar) {
		ar.serialize(groups_, "groups", "Анимационные группы");
		ar.setFilter(SERIALIZE_ANIMATION_GROUP);
		ar.serialize(*exporter->GetRootNode(0), "nodes", "Узлы");
		ar.setFilter(0);
		ar.serialize(MaterialsSerializator(exporter->materials), "materials", "Материалы");
	}

private:
	AnimationGroups& groups_;
};

class AnimationChainsSerializer
{
public:
	struct MoveData
	{
		int start;
		int end;
		int shift;
		
		MoveData() : start(INT_INF), end(-INT_INF), shift(0) {}

		void serialize(Archive& ar)
		{
			ar.serialize(start, "start", "Начальный фрейм");
			ar.serialize(end, "end", "Конечный фрейм");
			ar.serialize(shift, "shift", "Сдвиг");
		}
	};

	struct LessByFrame
	{
		bool operator()(const StaticAnimationChain& c1, const StaticAnimationChain& c2)
		{
			return c1.begin_frame < c2.begin_frame;
		}
	};

	struct LessByName
	{
		bool operator()(const StaticAnimationChain& c1, const StaticAnimationChain& c2)
		{
			return c1.name < c2.name;
		}
	};

	AnimationChainsSerializer(StaticAnimationChains& chains) : chains_(chains) {}

	void serialize(Archive& ar)
	{
		ButtonDecorator sortByFrameButton("Сортировать цепочки по кадрам");
		ar.serialize(sortByFrameButton, "sortByFrameButton", "<");

		ButtonDecorator sortByNameButton("Сортировать цепочки по имени");
		ar.serialize(sortByNameButton, "sortByNameButton", "<");

		ButtonDecorator moveButton("Подвинуть цепочки");
		ar.serialize(moveButton, "moveButton", "<");

		ar.serialize(chains_, "chains", "Анимационные цепочки");

		if(sortByFrameButton)
			sort(chains_.begin(), chains_.end(), LessByFrame());

		if(sortByNameButton)
			sort(chains_.begin(), chains_.end(), LessByName());
		
		if(moveButton){
			MoveData moveData;
			StaticAnimationChains::iterator i;
			FOR_EACH(chains_, i){
				moveData.start = min(moveData.start, i->begin_frame);
				moveData.end = max(moveData.end, i->end_frame);
			}
			moveData.shift = moveData.start < 0 ? -moveData.start : 0;
			static string path = getenv("TEMP");
			static string moveCfg = path + "\\" + "KDVExporterMove.cfg";
			if(kdw::edit(Serializer(moveData), moveCfg.c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Сдвиг анимационных цепочек")){
				StaticAnimationChains::iterator i;
				FOR_EACH(chains_, i){
					if(i->begin_frame >= moveData.start && i->end_frame <= moveData.end){
						i->begin_frame += moveData.shift;
						i->end_frame += moveData.shift;
					}
				}
			}
		}
	}

private:
	StaticAnimationChains& chains_;
};

class VisibilityGroupsSerializer
{
public:
	VisibilityGroupsSerializer(StaticVisibilitySets& sets) : sets_(sets) {}

	void serialize(Archive& ar)
	{
		ar.serialize(sets_, "sets", "Независимые множества выдимости");
		ar.setFilter(SERIALIZE_MESHES);
		ar.serialize(*exporter->GetRootNode(0), "meshes", "Полигональные объекты");
		ar.setFilter(SERIALIZE_LIGHTS);
		ar.serialize(*exporter->GetRootNode(0), "lights", "Источники освещения");
		ar.setFilter(SERIALIZE_LEAVES);
		ar.serialize(*exporter->GetRootNode(0), "leaves", "Листья");
		ar.setFilter(0);
	}

private:
	StaticVisibilitySets& sets_;

};

void Exporter::serialize(Archive& ar)
{
	bool invalidTime = false;
	StaticAnimationChains::iterator ci;
	FOR_EACH(animationChains_, ci)
		if(ci->begin_frame < 0)
			invalidTime = true;
	xassert("Анимационные цепочки с отрицательным временем недопустимы. В 0-м кадре должна быть референсная поза" && !invalidTime);

	ButtonDecorator animationGroupsButton("Анимационные группы");
	ButtonDecorator animationChainsButton("Анимационные цепочки");
	ButtonDecorator visibilityGroupsButton("Группы видимости");
	ButtonDecorator nonDeleteNodesButton("Неудаляемые узлы");
	ButtonDecorator logicNodesButton("Логические узлы");
	ButtonDecorator boundNodesButton("Узлы баундов");
	ButtonDecorator emblemButton("Выставить эмблему");

	ar.serialize(animationGroupsButton, "animationGroupsButton", "<");
	ar.serialize(animationChainsButton, "animationChainsButton", "<");
	ar.serialize(visibilityGroupsButton, "visibilityGroupsButton", "<");
	ar.serialize(nonDeleteNodesButton, "nonDeleteNodesButton", "<");
	ar.serialize(logicNodesButton, "logicNodesButton", "<");
	ar.serialize(boundNodesButton, "boundNodesButton", "<");
	ar.serialize(emblemButton, "emblemButton", "<");

	ar.serialize(maxWeights, "maxWeights", "Максимальное количество весов у точки");
	ar.serialize(lod_, "lod", "<");
	ar.serialize(textLog_, "textLog", "Текстовый лог (*.3dxText)");
	ar.serialize(show_info_polygon, "show_info_polygon", "Лог полигонов");
	ar.serialize(dontExport_, "dontExport", "Не экспортировать, а только записать изменения");

	static string path = getenv("TEMP");
	static string animationGroupsCfg = path + "\\" + "KDVExporterAnimationGroups.cfg";
	static string visibilityGroupsCfg = path + "\\" + "KDVExporterVisibilityGroups.cfg";
	static string animationChainsCfg = path + "\\" + "KDVExporterAnimationChains.cfg";
	static string nodesCfg = path + "\\" + "KDVExporterNodes.cfg";

	if(animationGroupsButton)
		kdw::edit(Serializer(AnimationGroupsSerializer(animationGroups_)), animationGroupsCfg.c_str(), kdw::IMMEDIATE_UPDATE | kdw::ONLY_TRANSLATED, HWND(0), "Анимационные группы");

	if(animationChainsButton)
		kdw::edit(Serializer(AnimationChainsSerializer(animationChains_)), animationChainsCfg.c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Анимационные цепочки");
	
	if(visibilityGroupsButton)
		kdw::edit(Serializer(VisibilityGroupsSerializer(visibilitySets_)), visibilityGroupsCfg.c_str(), kdw::IMMEDIATE_UPDATE | kdw::ONLY_TRANSLATED, HWND(0), "Группы видимости");

	if(nonDeleteNodesButton)
		kdw::edit(Serializer(*GetRootNode(0), "", "", SERIALIZE_NON_DELETE), nodesCfg.c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Неудаляемые узлы");
	if(logicNodesButton)
		kdw::edit(Serializer(*GetRootNode(0), "", "", SERIALIZE_LOGIC), nodesCfg.c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Логические узлы");
	if(boundNodesButton)
		kdw::edit(Serializer(*GetRootNode(0), "", "", SERIALIZE_BOUND), nodesCfg.c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Узлы баундов");

	if(emblemButton){
		cMoveEmblem dlg;
		dlg.DoModal();
	}
}

void ExporterVisibilitySet::set(const char* setName, const char* groupName)
{
	if(name.value() != setName){
		groups.clear();
		name = setName;
	}
	groups.push_back(ComboListString("", groupName));
}

void ExporterVisibilitySet::serializeButton(Archive& ar, const char* meshName)
{
	nameBuffer_.init();
	nameBuffer_ < meshName;
	buttonBuffer_.init();
	buttonBuffer_ < name;
	Groups::iterator i;
	FOR_EACH(groups, i){
		if(i != groups.begin())
			buttonBuffer_ < ", ";
		else
			buttonBuffer_ < ": ";
		buttonBuffer_ < *i;
	}

	ButtonDecorator button(buttonBuffer_.c_str());

	ar.serialize(button, meshName, nameBuffer_);

	if(button){
		static string cfg = string(getenv("TEMP")) + "\\" + "KDVExporterExpVisSet.cfg";
		if(groups.empty())
			groups.push_back("");
		kdw::edit(Serializer(*this), cfg.c_str(), kdw::IMMEDIATE_UPDATE | kdw::COMPACT, HWND(0), nameBuffer_);

		StaticVisibilitySets::iterator si;
		FOR_EACH(exporter->visibilitySets_, si){
			StaticVisibilityGroups::iterator gi;
			FOR_EACH(si->visibilityGroups, gi){
				gi->meshes.remove(meshName);
				Groups::iterator i;
				FOR_EACH(groups, i)
					if(si->name == name.value() && gi->name == i->value())
						gi->meshes.add(meshName);
			}
		}
	}
}

void ExporterVisibilitySet::serialize(Archive& ar)
{
	if(ar.isOutput()){
		const StaticVisibilitySet* set = 0;
		string setList;
		StaticVisibilitySets::const_iterator si;
		FOR_EACH(exporter->visibilitySets_, si){
			setList += si->name + "|";
			if(si->name == name.value())
				set = &*si;
		}
		name.setComboList(setList.c_str());

		string groupList;
		if(set){
			StaticVisibilityGroups::const_iterator gi;
			FOR_EACH(set->visibilityGroups, gi)
				groupList += gi->name + "|";
		}

		Groups::iterator i;
		FOR_EACH(groups, i)
			i->setComboList(groupList.c_str());
	}
	string namePrev = name;
	ar.serialize(name, "name", "Множество");
	ar.serialize(groups, "groups", "Группы");

	if(ar.isInput() && namePrev != name.value()){
		Groups::iterator i;
		FOR_EACH(groups, i)
			*i = "";
	}
}
