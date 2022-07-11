#include "StdAfx.h"

#include "Render/src/Scene.h"
#include "Render/src/TileMap.h"
#include "Render/inc/IRenderDevice.h"
#include "Environment/Environment.h"
#include "Terra/Vmap.h"

#include "EffectDocument.h"
#include "Serialization/SerializationFactory.h"

#include "kdw/Tool.h"
#include "kdw/Sandbox.h"
#include "kdw/Navigator.h"
#include "kdw/PopupMenu.h"
#include "kdw/FileDialog.h"
#include "Render/src/NParticleID.h"
#include "Render\Src\VisGeneric.h"
#include "FileUtils/FileUtils.h"
#include "Serialization/GenericFileSelector.h"

#include "kdw/PropertyOArchive.h"
#include "kdw/PropertyTreeModel.h"

extern RENDER_API cInterfaceRenderDevice* gb_RenderDevice;
extern RENDER_API cVisGeneric*            gb_VisGeneric;

EffectDocument* globalDocument = 0;

// --------------------------------------------------------------------------- 

NodeEffectRoot::NodeEffectRoot(EffectDocument* document)
: document_(document)
{
	
}

void NodeEffectRoot::onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget)
{
	root.add(TRANSLATE("Новый эффект")).connect(this, &NodeEffectRoot::onMenuNewEffect);
	root.add(TRANSLATE("Октрыть эффект"), widget).connect(this, &NodeEffectRoot::onMenuOpenEffect);
}

void NodeEffectRoot::onMenuNewEffect()
{
	globalDocument->freeze(this);
	NodeEffect* effect = globalDocument->add();
	NodeEmitter* emitter = effect->addEmitter();
	globalDocument->unfreeze(this);
}

void NodeEffectRoot::onMenuOpenEffect(kdw::Widget* widget)
{
	const char* masks[] = {
		"Effect (.effect)", "*.effect",
		0
	};

	kdw::FileDialog dialog(widget, false, masks, "Resource\\FX");
	if(dialog.showModal()){
		globalDocument->freeze(this);
		NodeEffect* nodeEffect = globalDocument->add(dialog.fileName());
		globalDocument->unfreeze(this);
	}
}

// --------------------------------------------------------------------------- 

namespace kdw{ class ToolNavigator; }

EffectDocument::EffectDocument()
: worldsDirectory_("Resource\\Worlds")
, randomSeed_(xclock())
, effectOrigin_(Vect3f::ZERO)
, effectPose_(Se3f::ID)
, effectNodePose_(Se3f::ID)
, particleRate_(1.0f)
, worldCenter_(0, 0)
{
	setRoot(new NodeEffectRoot(this));
	if(!gb_VisGeneric){
		gb_RenderDevice = CreateIRenderDevice(false); // тут создается gbVisGeneric
		gb_RenderDevice->Initialize(100, 100, RENDERDEVICE_MODE_WINDOW, 0, 0, GetDesktopWindow());
	}
	scene_ = gb_VisGeneric->CreateScene();
	sandboxRenderer_ = new kdw::SandboxRenderer(sandbox_, gb_RenderDevice);
	effect_ = 0;

	createFlatTerrain();
	createEnvironment();

	setBaseTool(kdw::ToolManager::instance().get(typeid(kdw::ToolSelect).name()));
	updateEffectPose();
}

EffectDocument::~EffectDocument()
{
	delete ::environment;
	::environment = 0;
    RELEASE(scene_);
}

void EffectDocument::createEnvironment()
{
	delete ::environment;
	::environment = 0;
	::environment = new Environment(scene_, scene_->GetTileMap(), false, false, false);
}

void EffectDocument::setWorldName(const char* spgFileName)
{
	if(strcmp(worldName_.c_str(), spgFileName) != 0){
		if(loadWorld(spgFileName))
			worldName_ = spgFileName;	
	}
}

void EffectDocument::createFlatTerrain()
{
	worldCenter_.set(128, 128);
	vMap.H_SIZE_POWER = vrtMapCreationParam::SIZE_256;
    vMap.V_SIZE_POWER = vrtMapCreationParam::SIZE_256;
	vMap.createWorldMetod = vrtMapCreationParam::FullPlain;
	vMap.initialHeight = 0;
	vMap.create();
	vMap.WorldRender();
	scene()->CreateMap();
}


NodeEffectRoot* EffectDocument::createRoot()
{
	return new NodeEffectRoot(this);
}

/*
void EffectDocument::setSelectedNode(kdw::ModelNode* node)
{
	if(node == 0 || node == root()){
		activeEffect_ = 0;
		activeEmitter_ = 0;
		activeCurve_ = 0;
	}
	__super::setSelectedNode(node);
}
*/


cScene* EffectDocument::scene()
{
	return scene_;
}

cEffect* EffectDocument::effect()
{
	return effect_;
}

void EffectDocument::quant(float deltaTime)
{
	scene_->SetDeltaTime(deltaTime * 1000.0f);
	if(timeChanger_){
		setTime(timeToSet_, timeChanger_);
		timeChanger_ = 0;
	}
	if(effect_){
		effect_->SetParticleRate(1.0f);
		if(rewinded()/* || !effect_->isAlive()*/){
			++randomSeed_;
			createEffect();
		}
	}	
}

void EffectDocument::queueSetTime(float time, kdw::ModelObserver* changer)
{
	timeToSet_ = time;
	timeChanger_ = changer;
}

void EffectDocument::setTime(float time, kdw::ModelObserver* changer)
{
	if(effect_){
		if(fabsf(time - this->time()) > FLT_COMPARE_TOLERANCE){
			if(time < this->time()){
				createEffect();
			}
			else{
				effect_->MoveToTime(time);
				if(activeEmitter())
					effect_->MoveToTime(activeEmitter()->get()->emitter_create_time + time);
				else
					effect_->MoveToTime(time);
			}
		}
	}
	kdw::ModelTimed::setTime(time, changer);
}

void EffectDocument::serialize(Archive& ar)
{
	if(ar.filter(kdw::SERIALIZE_STATE)){
		std::string world = worldName();
		ar.serialize(world, "worldName", "Путь к миру");
		ar.serialize(effectOrigin_, "effectOrigin", "Центр эффекта");
		if(ar.isInput())
			setWorldName(world.c_str());

		if(environment){
			std::string environmentPreset = environment->presetName();
			ar.serialize(environmentPreset, "environmentPreset", "Пресет окружения");
			//if(ar.isInput())
			//	environment->setPresetName(environmentPreset.c_str());
		}
	}
	
}

/*
void EffectDocument::onSelectionChanged()
{
	float t = time();
	createEffect();
	setTime(t);
	__super::onSelectionChanged();
	signalTimeChanged().emit();
}
*/

void EffectDocument::onCurveChanged(bool significantChange)
{
	signalCurveChanged_.emit(significantChange);
}

void EffectDocument::clear()
{
}

void EffectDocument::setActiveEffect(NodeEffect* nodeEffect)
{
	if(activeEffect_ != nodeEffect){
		activeEffect_ = nodeEffect;
		setActiveEmitter(0);
		activeCurve_ = 0;

		createEffect();
	}
}

void EffectDocument::setActiveEmitter(NodeEmitter* nodeEmitter)
{
	if(activeEmitter_ != nodeEmitter){
		activeEmitter_ = nodeEmitter;
		activeCurve_ = 0;
		if(activeEffect_ && nodeEmitter){
			//activeEffect_->setSoloEmitter(nodeEmitter->get());
		}
		createEffect();
	}
}

void EffectDocument::setActiveCurve(NodeCurve* nodeCurve)
{
	activeCurve_ = nodeCurve;
}

void EffectDocument::setParticleRate(float particleRate)
{
	particleRate_ = particleRate;
}

void EffectDocument::createEffect()
{
	RELEASE(effect_);
	static int counter = 0;
	XBuffer buf;
	buf < "Effect recreated (" <= counter < ")\n";
	OutputDebugString(buf);
	++counter;
	if(activeEffect_){
		graphRnd.set(randomSeed_);
		float duration = 0.0f;
		if(activeEmitter_){
			duration = activeEmitter_->get()->getParticleLongestLifeTime();
		}
		else{
			int count = activeEffect_->childrenCount();
			for(int i = 0; i < count; ++i){
				duration = max(duration, activeEffect_->child(i)->get()->getParticleLongestLifeTime());
			}
		}
		setDuration(duration);

		if(effect_ = scene_->CreateEffectDetached(*activeEffect_->get(), 0)){
			effect_->Attach();
			//effect_->useOverDraw = true;
			//effect_->initOverdraw();
			effect_->SetPosition(MatXf(effectPose_));

			bool allVisible = activeEmitter_ == 0;
			for(int i = activeEffect_->childrenCount() - 1; i >= 0; i--){
				NodeEmitter* emitter = activeEffect_->child(i);
				effect_->ShowEmitter(emitter->get(), (emitter == activeEmitter_) ? true : allVisible);
			}
			if(activeEmitter_){
				effect_->SetTime(activeEmitter_->get()->emitter_create_time);
				effect_->MoveToTime(activeEmitter_->get()->emitter_create_time + time());
			}
			else{
				effect_->SetTime(0.0f);
				effect_->MoveToTime(time());
			}
			//effect_->enableOverDraw = true;
		}
		else 
			xassert(0 && "Unable to create effect");
		//signalTimeChanged_.emit();
	}
}

void EffectDocument::updateEffectPosition()
{
	if(effect_){
		effect_->SetPosition(MatXf(effectPose_));
		effect_->SetParticleRate(particleRate_);
	}
}

void EffectDocument::updateEffectPose()
{
	effectPose_ = effectNodePose_;
	effectPose_.trans() += Vect3f(worldCenter_);
	effectPose_.trans() += effectOrigin_;
}

void EffectDocument::onEffectOriginChanged(bool recreate)
{
	updateEffectPose();
	if(effect_){
		if(recreate)
			createEffect();
		else
			updateEffectPosition();
	}
	signalEffectCenterChanged().emit();
}

bool EffectDocument::loadWorld(const char* fileName)
{
	if(strlen(fileName)){
		gb_VisGeneric->SetTilemapDetail(false);

		std::string path = ::extractFilePath(fileName);
		std::string name = ::extractFileName(fileName);
		std::string base = ::extractFileBase(fileName);
		
		vMap.setWorldsDir(path.c_str());
		if(vMap.load(base.c_str())){
			vMap.WorldRender();
			if(cTileMap* tileMap = scene()->GetTileMap()){
				RELEASE(tileMap);
				scene()->UpdateLists(0);
			}
			scene()->CreateMap(true);
			worldCenter_.set(int(vMap.H_SIZE / 2), int(vMap.V_SIZE / 2));
			updateEffectPose();
			signalEffectCenterChanged().emit();
			createEnvironment();
			return true;
		}
	}
	else{
		createFlatTerrain();
		createEnvironment();
	}

	return false;
}

void EffectDocument::setEffectOrigin(const Vect3f& origin)
{
	effectOrigin_ = origin;
}

void EffectDocument::setEffectNodePose(const Se3f& pose)
{
	effectNodePose_ = pose;
	updateEffectPose();	
}

void EffectDocument::onRenderDeviceInitialize()
{
	if(!scene()->GetTileMap())
		createFlatTerrain();
}


void EffectDocument::recreateEffect()
{
	createEffect();
	setTime(time_, this);
}

NodeEffect* EffectDocument::add(EffectKey* effectKey)
{
	if(!effectKey)
		effectKey = new EffectKey();
	NodeEffect* nodeEffect = new NodeEffect(effectKey, "");
	std::string name = "NewEffect";
	effectKey->name = name;
	int index = 1;
	while(root()->child(name.c_str()) != 0){
		XBuffer buf;
		buf < name.c_str() < "(" <= index++ < ")";
        name = buf;
	}
	nodeEffect->setName(name.c_str());
	root()->add(nodeEffect);
	return nodeEffect;
}

NodeEffect* EffectDocument::add(const char* fileName)
{
	if(!::isFileExists(fileName))
		return 0;

	std::string fileBase = ::extractFileBase(fileName);
	std::string texturesPath = ::extractFilePath(fileName);
	texturesPath += "\\Textures";

	CLoadDirectoryFile directory;
	if(!directory.Load(fileName))
		return 0;

	NodeEffect* result = 0;

	while(CLoadData* ld = directory.next()){
		if(ld->id == IDS_EFFECTKEY){
			EffectKey* effectKey = new EffectKey;
			effectKey->filename = fileName;
			effectKey->Load(ld);
			effectKey->changeTexturePath(texturesPath.c_str());
			effectKey->preloadTexture();

			NodeEffect* nodeEffect = new NodeEffect(effectKey, fileBase.c_str());
			std::string name = effectKey->name;
			int index = 1;
			while(root()->child(name.c_str()) != 0){
				XBuffer buf;
				buf < effectKey->name.c_str() < "(" <= index++ < ")";
                name = buf;
			}
			nodeEffect->setName(name.c_str());
			root()->add(nodeEffect);
			result = nodeEffect;
		}
	}
	return result;
}

// ---------------------------------------------------------------------------

NodeEffect::NodeEffect(EffectKey* key, const char* fileName)
: effectKey_(key)
, fileName_(fileName)
, model_(0)
, modelLogic_(0)
, modelVisibilitySetIndex_(0)
, modelAnimationChainIndex_(0)
, modelNodeIndex_(0)
{
	xassert(effectKey_);
	globalDocument->signalTimeChanged().connect(this, &NodeEffect::onTimeChanged);

	rebuild();
}

NodeEffect::~NodeEffect()
{
	RELEASE(model_);
}

static ComboListString modelVisibilitySetsComboList(cObject3dx* model, int selectedIndex)
{
	std::string comboList;
	std::string value;
	int setIndex = 0;
	StaticVisibilitySet& set = model->GetVisibilitySet(VisibilitySetIndex(setIndex));
	int count = int(set.visibilityGroups.size());
	for(int i = 0; i < count; ++i){
		StaticVisibilityGroup* chainGroup = &set.visibilityGroups[i];
		if(i)
			comboList += "|";
		else
			value = chainGroup->name;
		comboList += chainGroup->name;
		if(i == selectedIndex)
			value = chainGroup->name;
	}
	return ComboListString(comboList.c_str(), value.c_str());
}

static ComboListString animationChainsComboList(cObject3dx* model, int selectedIndex)
{
	std::string comboList;
	std::string value;
	int count = model->GetChainNumber();
	for(int i = 0; i < count; ++i){
		StaticAnimationChain* chain = model->GetChain(i);
		if(i != 0)
			comboList += "|";
		else
			value = chain->name;
		comboList += chain->name;
		if(i == selectedIndex)
			value = chain->name;
	}
	return ComboListString(comboList.c_str(), value.c_str());
}

static ComboListString modelNodes(cObject3dx* model, int selectedIndex)
{
	std::string comboList;
	std::string value;
	int nodeCount = model->GetNodeNumber();
	for(int i=0; i<nodeCount; i++){
		const char* nodeName = model->GetNodeName(i);
		if(i != 0)
			comboList += "|";
		comboList += nodeName;
		if(i == selectedIndex)
			value = nodeName;
	}
	return ComboListString(comboList.c_str(), value.c_str());
}

void NodeEffect::serialize(Archive& ar)
{
	static GenericFileSelector::Options options("*.3dx", "Resource\\Models", TRANSLATE("Модель для эффекта"));
	std::string oldModel = modelFileName_;
	ar.serialize(GenericFileSelector(modelFileName_, options), "modelFileName", "3Д Модель");

	if(ar.isInput() && oldModel != modelFileName_)
		loadModel(modelFileName_.c_str());


	cObject3dx* model = model_;

	if(model){
		ComboListString visibilitySet(modelVisibilitySetsComboList(model, modelVisibilitySetIndex_));
		ar.serialize(visibilitySet, "visibilitySet", "Группа видимости");
		if(ar.isInput()){
			modelVisibilitySetIndex_ = indexInComboListString(visibilitySet.comboList(), visibilitySet.value().c_str());
			setModelVisibilityGroupIndex(modelVisibilitySetIndex_);
		}

		ComboListString animationChain(animationChainsComboList(model, modelAnimationChainIndex_));
		ar.serialize(animationChain, "animationChain", "Цепочка анимации");
		if(ar.isInput()){
			modelAnimationChainIndex_ = indexInComboListString(animationChain.comboList(), animationChain.value().c_str());
			setModelAnimationChainIndex(modelAnimationChainIndex_);
		}

		ComboListString node(modelNodes(model, modelNodeIndex_));
		ar.serialize(node, "node", "Узел привязки");
		if(ar.isInput()){
			modelNodeIndex_ = indexInComboListString(node.comboList(), node.value().c_str());
			setModelNodeIndex(modelNodeIndex_);
		}
	}
}


void NodeEffect::rebuild()
{
	clear();
	setName(effectKey_->name.c_str());
	EffectKey::EmitterKeys::iterator it;
	FOR_EACH(effectKey_->emitterKeys, it){
		add(new NodeEmitter(*it));
	}
}

void NodeEffect::set(EffectKey* effect)
{
	globalDocument->freeze(this);
	if(globalDocument->activeEffect() == this){
		globalDocument->setActiveEmitter(0);
	}
	if(effectKey_){
		delete effectKey_;
		effectKey_ = 0;
	}
	effectKey_ = new EffectKey();
	*effectKey_ = *effect;
	rebuild();
	globalDocument->unfreeze(this);
}

Serializer NodeEffect::serializeable()
{
	return Serializer(*this);
}

void NodeEffect::onSelect()
{
	globalDocument->setActiveEffect(this);
	globalDocument->setActiveEmitter(0);
}

void NodeEffect::onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget)
{
	root.add(TRANSLATE("Создать эмиттер")).connect(this, &NodeEffect::onMenuAddEmitter);
	root.addSeparator();
	root.add(TRANSLATE("Сохранить"), widget).connect(this, &NodeEffect::onMenuSave);
	root.add(TRANSLATE("Сохранить как..."), widget).connect(this, &NodeEffect::onMenuSaveAs);
  //root.add(TRANSLATE("Открыть в проводнике")).connect(this, &NodeEffect::onMenuShowInExplorer);
	root.addSeparator();
	root.add(TRANSLATE("Выгрузить эффект")).connect(this, &NodeEffect::onMenuUnload);
}

NodeEmitter* NodeEffect::addEmitter()
{
	EmitterKeyInterface* newEmitterKey = new EmitterKeyInt();
	effectKey_->emitterKeys.push_back(newEmitterKey);
	
	int index = 0;
	while(true){
		XBuffer buf;
		buf < "Emitter " <= index++;
		newEmitterKey->name = static_cast<const char*>(buf);
		if(child(static_cast<const char*>(buf)) == 0)
			break;
	}
	NodeEmitter* nodeEmitter = new NodeEmitter(newEmitterKey);
	add(nodeEmitter);
	return nodeEmitter;
}

void NodeEffect::remove(NodeEmitter* emitter)
{
	if(globalDocument->activeEmitter() == emitter)
		globalDocument->setActiveEmitter(0);
	EffectKey::EmitterKeys::iterator it;
	FOR_EACH(effectKey_->emitterKeys, it){
		if(*it == emitter->get()){
			effectKey_->emitterKeys.erase(it);
			break;
		}
	}
	__super::remove(emitter);
}

float NodeEffect::calculateModelScale(cObject3dx* model)
{
	const float one_size_model=40.0f;
	bool scaleByModel = false;
	sBox6f boundBox;
	model->GetBoundBox(boundBox);
	float height = boundBox.zmax() - boundBox.zmin();
	float effectScale = scaleByModel ? height / one_size_model : 1.0f;
	return 1.0f / max(FLT_EPS, effectScale);
}

void NodeEffect::updateModelPosition()
{
	if(model_){
		MatXf pos = model_->GetPosition();
		pos.trans() = globalDocument->effectOrigin() + Vect3f(globalDocument->worldCenter());
		model_->SetPosition(pos);
	}	
	if(modelLogic_){
		MatXf pos = model_->GetPosition();
		pos.trans() = globalDocument->effectOrigin() + Vect3f(globalDocument->worldCenter());
		modelLogic_->SetPosition(pos);
	}	
}

void NodeEffect::setModelVisibilityGroupIndex(int index)
{
	modelVisibilitySetIndex_ = index;
	if(model_){
		int numVisibilitySets = model_->GetVisibilitySetNumber();

		for(int setIndex = 0; setIndex < numVisibilitySets; ++setIndex)
			model_->SetVisibilityGroup(VisibilityGroupIndex(index), VisibilitySetIndex(setIndex));
	}
	if(modelLogic_){
		int numVisibilitySets = modelLogic_->GetVisibilitySetNumber();

		for(int setIndex = 0; setIndex < numVisibilitySets; ++setIndex)
			modelLogic_->SetVisibilityGroup(VisibilityGroupIndex(index), VisibilitySetIndex(setIndex));
	}
}

void NodeEffect::setModelAnimationChainIndex(int index)
{
	index = max(0, min(model_->GetChainNumber() - 1, index));
	modelAnimationChainIndex_ = index;
	for(int i = 0; i < model_->GetAnimationGroupNumber(); ++i)
		model_->SetAnimationGroupChain(i, index);
}


void NodeEffect::setModelNodeIndex(int index)
{
	modelNodeIndex_ = index;
	updateEffectNodePose();
}

void NodeEffect::updateEffectNodePose()
{
	if(model_){
		Se3f nodePosition(model_->GetNodePosition(modelNodeIndex_));
		nodePosition.trans() -= globalDocument->effectOrigin() + Vect3f(globalDocument->worldCenter());
		globalDocument->setEffectNodePose(nodePosition);
		globalDocument->onEffectOriginChanged(false);
	}
	else{
		globalDocument->setEffectNodePose(Se3f::ID);
		globalDocument->onEffectOriginChanged(false);
	}
}

void NodeEffect::loadModel(const char* fileName)
{
	modelFileName_ = fileName;
	RELEASE(model_);
	RELEASE(modelLogic_);
	cScene* scene = globalDocument->scene();
	if(strlen(fileName) > 0){
		model_  = scene->CreateObject3dx(fileName);
		xassert(model_);
		modelLogic_  = scene->CreateLogic3dx(fileName);
		model_->SetSkinColor(Color4c(255, 0, 0));
		float scale = calculateModelScale(modelLogic_ ? modelLogic_ : model_);
		model_->SetScale(scale);
		if(modelLogic_)
			modelLogic_->SetScale(scale);
		setModelAnimationChainIndex(0);
		setModelVisibilityGroupIndex(0);
		setModelNodeIndex(0);
	}
	updateEffectNodePose();
}

void NodeEffect::onTimeChanged(ModelObserver* changer)
{
	if(changer != this){
		float time = globalDocument->time();
		if(model_){
			float chainLength = model_->GetChain(modelAnimationChainIndex_)->time;
			while(time > chainLength)
				time -= chainLength;

			model_->SetPhase(time / chainLength);
			if(modelLogic_)
				modelLogic_->SetPhase(time / chainLength);
			updateModelPosition();
		}
		
		if(globalDocument->activeEffect() == this && model_){
			xassert(modelNodeIndex_ >= 0 && modelNodeIndex_ < model_->GetNodeNumber());
			updateEffectNodePose();
		}
	}
}

void NodeEffect::onMenuAddEmitter()
{
	globalDocument->freeze(this);
	globalDocument->history().pushEffect();
	NodeEmitter* emitter = addEmitter();
	//globalDocument->setSelectedNode(emitter);
	emitter->setSelected(true);
	setSelected(false);
	globalDocument->unfreeze(this);
}

void NodeEffect::onMenuSave(kdw::Widget* widget)
{
	if(!fileName_.empty())
		saveOld(fileName_.c_str());
	else
		onMenuSaveAs(widget);
}


void NodeEffect::onMenuSaveAs(kdw::Widget* widget)
{
	const char* filter[] = { "Effects (.effect)", "*.effect", 0 };
	kdw::FileDialog dialog(widget, true, filter);
	if(dialog.showModal()){
		fileName_ = dialog.fileName();
		saveOld(fileName_.c_str());
	}    
}

void NodeEffect::onMenuUnload()
{
	globalDocument->freeze(this);
	int index = parent()->childIndex(this);
	globalDocument->history().pushRemoveEffect(index);

	NodeEffectRoot* root = safe_cast<NodeEffectRoot*>(parent());
	root->remove(this);
	globalDocument->unfreeze(this);
}

struct Texs{ 
	string tex1, tex2;
	string textures[10];
};

static const char* FOLDER_TEXTURES = "Textures";

std::string copyTexture(const char* sourceFile, const char* destinationDirectory)
{
	xassert(sourceFile);
	xassert(destinationDirectory);
	std::string fileName(extractFileName(sourceFile));
	std::string to = destinationDirectory;
	if(!SetCurrentDirectory(to.c_str())){
		CreateDirectory(to.c_str(), 0);
		SetCurrentDirectory(to.c_str());
	}
	if(to[to.size() - 1] != '\\')
		to += "\\";
	to += fileName;

	CopyFile(sourceFile, to.c_str(), false);
	return fileName;
}

void NodeEffect::saveOld(const char* fileName)
{
	CurrentDirectorySaver dirSaver;

	FileSaver saver;
	saver.Init(fileName);
	saver.SetData(EXPORT_TO_GAME);

	std::string export_path = ::extractFilePath(fileName);
	std::string Texture_path = ".";

	typedef vector<string> TextureNames;

	std::string newTexturesPath(export_path);
	newTexturesPath += "\\";
	newTexturesPath += FOLDER_TEXTURES;

	vector<TextureNames> texturesByEmitter;

	for(int emitterIndex = 0; emitterIndex < childrenCount(); emitterIndex++){
		NodeEmitter* nodeEmitter = child(emitterIndex);
		EmitterKeyInterface* emitter = nodeEmitter->get();

		TextureNames textures;
		emitter->GetTextureNames(textures, false);
		texturesByEmitter.push_back(textures);

		TextureNames::iterator it;
		FOR_EACH(textures, it){
			if(!it->empty()){
				std::string from;
				if(saved())
					from = Texture_path + "\\" + *it;
				else
					from = *it;
				*it = std::string(FOLDER_TEXTURES) + from;
				copyTexture(from.c_str(), newTexturesPath.c_str());
			}
		}
		emitter->setTextureNames(textures);
		nodeEmitter->buildKey();
	}


	get()->Save(saver);

	for(int emitterIndex = 0; emitterIndex < childrenCount(); ++emitterIndex){
		NodeEmitter* nodeEmitter = child(emitterIndex);
		EmitterKeyInterface* emitter = nodeEmitter->get();
		emitter->setTextureNames(texturesByEmitter[emitterIndex]);
	}
}


// ---------------------------------------------------------------------------

NodeEmitter::NodeEmitter(EmitterKeyInterface* emitterKey)
: emitterKey_(emitterKey)
, selectedGenerationPoint_(0)
, selectedPoint_(0)
, toyEmitterPosition_(0)

{
	createToys();
	update();
	updateToys();

	globalDocument->signalCurveChanged().connect(this, &NodeEmitter::onCurveChanged);
}

void NodeEmitter::onCurveChanged(bool significantChange)
{
	if(selected_){
		onDeselect();
		createToys();
		onSelect();
	}
	
	if(emitterKey_->spline()){
		kdw::Toys::iterator it = toySplinePoints_.begin();
		int index = selectedPoint_;
		xassert(index < toySplinePoints_.size());
		std::advance(it, index);
		globalDocument->sandbox()->deselectAll();
		globalDocument->sandbox()->select(*it, true);
	}	
}


NodeEmitter::~NodeEmitter()
{
	toySplinePoints_.clear();
}

void NodeEmitter::createToys()
{
	toyEffectPosition_ = new kdw::Toy(/*Se3f(QuatF::ID, globalDocument->effectOrigin())*/globalDocument->effectPose(), Vect3f::ID * 5.0f, kdw::TOY_SHAPE_GRID_XY);

	toyEmitterPosition_.clear();
	KeysPos::iterator it;
	kdw::Toy* lastToy = 0;
	FOR_EACH(emitterKey_->emitter_position, it){
		KeyPos& pos = *it;
		kdw::Toy* toy = new kdw::Toy(Se3f(QuatF::ID, pos.pos), Vect3f(4.0f, 4.0f, 4.0f), kdw::TOY_SHAPE_CIRCLE_XY);
		toy->signalBeforeMove().connect(this, &NodeEmitter::onToyEmitterPositionBeforeMove);
		toy->signalMove().connect(this, &NodeEmitter::onToyEmitterPositionMove);
		toy->signalMoved().connect(this, &NodeEmitter::onToyEmitterPositionMoved);
		if(lastToy)
			lastToy->setLink(new kdw::ToyLink(lastToy, toy));
		toyEmitterPosition_.push_back(toy);
		toyEffectPosition_->attach(toy);
		lastToy = toy;
	}
	toySplinePoints_.clear();

	lastToy = 0;
	if(KeysPosHermit* spline = emitterKey_->spline()){
		KeysPosHermit::iterator it;
		FOR_EACH(*spline, it){
			KeyPos& pos = *it;
			kdw::Toy* toy = new kdw::Toy(Se3f(QuatF::ID, pos.pos), Vect3f(1.0f, 1.0f, 1.0f));
			toy->signalBeforeMove().connect(this, &NodeEmitter::onToyEmitterPositionBeforeMove);
			toy->signalMove().connect(this, &NodeEmitter::onToySplinePointMove);
			toy->signalActivate().connect(this, &NodeEmitter::onToySplinePointActivate);
			if(lastToy)
				lastToy->setLink(new kdw::ToyLink(lastToy, toy));
			toySplinePoints_.push_back(toy);
			lastToy = toy;
			toyEffectPosition_->attach(toy);
		}
	}
}

void NodeEmitter::onToyEmitterPositionBeforeMove(kdw::Toy* toy)
{
	globalDocument->history().pushEmitter();
}

void NodeEmitter::onToyEmitterPositionMoved(kdw::Toy* toy)
{
	globalDocument->recreateEffect();
}

void NodeEmitter::onToyEmitterPositionMove(kdw::Toy* toy)
{
	kdw::Toys::iterator toyIt = toyEmitterPosition_.begin();
	KeysPosHermit::iterator it;
	FOR_EACH(emitterKey_->emitter_position, it){
		xassert(toyIt != toyEmitterPosition_.end());
		KeyPos& pos = *it;
		if(toy == *toyIt){
			pos.pos = toy->pose().trans();
			break;
		}
		++toyIt;
	}

}

void NodeEmitter::onToySplinePointActivate(kdw::Toy* toy)
{
	kdw::Toys::iterator toyIt = toySplinePoints_.begin();
	xassert(emitterKey_->spline());
	int index = 0;
	KeysPosHermit::iterator it;
	FOR_EACH(*emitterKey_->spline(), it){
		xassert(toyIt != toySplinePoints_.end());
		KeyPos& pos = *it;
		if(toy == *toyIt){
			setSelectedPoint(index);
			globalDocument->onCurveChanged(false);
			break;
		}
		++toyIt;
		++index;
	}
}

void NodeEmitter::onToySplinePointBeforeMove(kdw::Toy* toy)
{
	globalDocument->history().pushEmitter();
}

void NodeEmitter::onToySplinePointMove(kdw::Toy* toy)
{
	KeysPos::iterator it;
	kdw::Toys::iterator toyIt = toySplinePoints_.begin();
	xassert(emitterKey_->spline());
	FOR_EACH(*emitterKey_->spline(), it){
		xassert(toyIt != toySplinePoints_.end());
		KeyPos& pos = *it;
		if(toy == *toyIt){
			pos.pos = toy->pose().trans();
			buildKey();
			break;
		}
		++toyIt;
	}
}

void NodeEmitter::onEffectCenterChanged()
{
	updateToys();
}

void NodeEmitter::set(EmitterKeyInterface* emitter)
{
    emitterKey_ = emitter;

	onDeselect();
	update();
	createToys();
	onSelect();

}

void NodeEmitter::updateToys()
{
	if(toyEffectPosition_)
		toyEffectPosition_->setPose(globalDocument->effectPose());
	//toyEffectPosition_->setPose(Se3f(QuatF::ID, globalDocument->effectOrigin()));
}

void NodeEmitter::update()
{
	clear();
	xassert(emitterKey_);
	setName(emitterKey_->name.c_str());


	buildKey();

	{
		CurveCollector collector;

		kdw::PropertyTreeModel model;
		kdw::PropertyOArchive oa(&model);
		emitterKey_->serialize(oa);

		CurveCollector::Curves::iterator it;
		FOR_EACH(collector.curves(), it){
			CurveWrapperBase* wrapper = *it;
			add(new NodeCurve(wrapper->clone(), strcmp(wrapper->name(), "p_velocity") == 0));
		}
	}
}

Serializer NodeEmitter::serializeable()
{
	return Serializer(*this, "", "");
}


void NodeEmitter::setSelectedGenerationPoint(int index)
{
	selectedGenerationPoint_ = clamp(index, 0, get()->generationPointCount() - 1); 
	globalDocument->onCurveChanged(false);
}

void NodeEmitter::setSelectedPoint(int index)
{
	selectedPoint_ = index;
}

void NodeEmitter::buildKey()
{
	get()->BuildKey();
	get()->BuildRuntimeData();
}

void NodeEmitter::serialize(Archive& ar)
{
	typedef SerializationFactory<EmitterKeyInterface, FactoryArg0<EmitterKeyInterface> > Factory;
	Factory& factory = Factory::instance();
	ComboListString comboList = factory.comboListAlt();
	std::string type = factory.nameAlt(typeid(*get()).name());
	comboList.value() = type;
	ar.serialize(comboList, "type", "Тип");
	if(ar.isInput() && comboList.value() != type){
		bool wasActive = globalDocument->activeEmitter() == this;
		globalDocument->setActiveEmitter(0);

		NodeEffect* nodeEffect = safe_cast<NodeEffect*>(parent());
		EffectKey::EmitterKeys& emitters = nodeEffect->get()->emitterKeys;
		for(int i = 0; i < emitters.size(); ++i){
			ShareHandle<EmitterKeyInterface>& emitter = emitters[i];
			if(emitter == emitterKey_){
				emitter = factory.find(factory.nameByNameAlt(comboList.value().c_str())).create();
				emitter->name = emitterKey_->name;
				emitterKey_ = emitter;
				break;
			}
		}

		if(wasActive){
			globalDocument->setActiveEmitter(this);
			onDeselect();
		}
		update();
		if(wasActive){
			createToys();
			onSelect();
		}
	}
	std::string name = emitterKey_->name;
	ar.serialize(name, "name", "Имя");
	if(ar.isInput() && name != emitterKey_->name)
		emitterKey_->name = name;
	emitterKey_->serialize(ar);
}

const char* NodeEmitter::name() const
{
	return emitterKey_->name.c_str();
}


void NodeEmitter::onSelect()
{
	NodeEffect* nodeEffect = safe_cast<NodeEffect*>(parent());
	globalDocument->setActiveEffect(nodeEffect);
	globalDocument->setActiveEmitter(this);
	globalDocument->signalEffectCenterChanged().connect(this, &NodeEmitter::onEffectCenterChanged);

	globalDocument->sandbox()->root()->attach(toyEffectPosition_);
	__super::onSelect();
}


void NodeEmitter::onDeselect()
{
	__super::onDeselect();
	kdw::Sandbox* sandbox = globalDocument->sandbox();
	if(toyEffectPosition_->attached())
		sandbox->root()->detach(toyEffectPosition_);
	globalDocument->signalEffectCenterChanged().disconnect(this);
	//sandbox->root()->detach(toySplinePoints_);

}

void NodeEmitter::onAdd()
{
}

void NodeEmitter::onRemove()
{
}

void NodeEmitter::onContextMenu(kdw::PopupMenuItem& root, kdw::Widget* widget)
{
	root.add("Удалить эмиттер").connect(this, &NodeEmitter::onMenuRemoveEmitter);
}

void NodeEmitter::onMenuRemoveEmitter()
{
	globalDocument->freeze(this);
	globalDocument->history().pushEffect();
	NodeEffect* nodeEffect = safe_cast<NodeEffect*>(parent());

	nodeEffect->remove(this);
	globalDocument->unfreeze(this);
}

NodeCurve* NodeEmitter::curveByName(const char* name)
{
	return safe_cast<NodeCurve*>(child(name));
}

namespace kdw{
	REGISTER_CLASS(Space, NavigatorSpace, "Навигатор")
}

// ---------------------------------------------------------------------------

NodeCurve::NodeCurve(CurveWrapperBase* wrapper, bool visible)
: wrapper_(wrapper)
, visible_(visible)
{
    setName(wrapper->name());
}

void NodeCurve::onSelect()
{
	globalDocument->setActiveCurve(this);
}

void NodeCurve::setVisible(bool visible)
{
	visible_ = visible;
}

NodeEmitter* NodeCurve::parent()
{
	return safe_cast<NodeEmitter*>(parent_);
}

const NodeEmitter* NodeCurve::parent() const
{
	return safe_cast<const NodeEmitter*>(parent_);
}

int NodeCurve::selectedPoint() const
{
	if(wrapper_->keyPerGeneration())
		return parent()->selectedGenerationPoint();
	else
		return parent()->selectedPoint();
}

void NodeCurve::setSelectedPoint(int index)
{
	if(wrapper_->keyPerGeneration())
		parent()->setSelectedGenerationPoint(index);
	else{
		parent()->setSelectedPoint(index);
	}
}

float NodeCurve::currentValue() const
{
	return wrapper_->value(selectedPoint());
}

void NodeCurve::setCurrentValue(float value)
{
	wrapper_->setValue(selectedPoint(), value);
}

// ---------------------------------------------------------------------------


class HistoryItemEmitter : public HistoryItem{
public:
	HistoryItemEmitter(EmitterKeyInterface* emitter, int effectIndex, int emitterIndex);
	void act();
	HistoryItem* createRedo();
protected:
	ShareHandle<EmitterKeyInterface> emitter_;
	int effectIndex_;
	int emitterIndex_;
};

HistoryItemEmitter::HistoryItemEmitter(EmitterKeyInterface* emitter, int effectIndex, int emitterIndex)
: emitter_(emitter)
, effectIndex_(effectIndex)
, emitterIndex_(emitterIndex)
{
    
}

void HistoryItemEmitter::act()
{
	NodeEffect* nodeEffect = safe_cast<NodeEffect*>(globalDocument->root()->child(effectIndex_));
	NodeEmitter* nodeEmitter = nodeEffect->child(emitterIndex_);
	nodeEmitter->set(emitter_);
}

HistoryItem* HistoryItemEmitter::createRedo()
{
	NodeEmitter* node = safe_cast<NodeEmitter*>(globalDocument->root()->child(effectIndex_)->child(emitterIndex_));
	EmitterKeyInterface* emitter = node->get();
	return new HistoryItemEmitter(emitter, effectIndex_, emitterIndex_);
}

// ---------------------------------------------------------------------------

class HistoryItemEffect : public HistoryItem{
public:
	HistoryItemEffect(EffectKey* effectKey, int effectIndex);
	~HistoryItemEffect();
	void act();
	HistoryItem* createRedo();
protected:
	EffectKey* effectKey_;
	int effectIndex_;
};

HistoryItemEffect::HistoryItemEffect(EffectKey* effectKey, int effectIndex)
: effectKey_(effectKey)
, effectIndex_(effectIndex)
{

}

HistoryItemEffect::~HistoryItemEffect()
{
	delete effectKey_;
	effectKey_ = 0;
}

void HistoryItemEffect::act()
{
	NodeEffect* nodeEffect = safe_cast<NodeEffect*>(globalDocument->root()->child(effectIndex_));
	nodeEffect->set(effectKey_);
}

HistoryItem* HistoryItemEffect::createRedo()
{
	NodeEffect* node = safe_cast<NodeEffect*>(globalDocument->root()->child(effectIndex_));
	xassert(node);
	xassert(node->get());
	EffectKey* effect = new EffectKey();
	*effect = *node->get();
	return new HistoryItemEffect(effect, effectIndex_);
}


// ---------------------------------------------------------------------------

class HistoryItemNewEffect : public HistoryItem{
public:
	HistoryItemNewEffect(int effectIndex);
	HistoryItemNewEffect();
	~HistoryItemNewEffect();
	void act();
	HistoryItem* createRedo();
protected:
	int effectIndex_;
};

HistoryItemNewEffect::HistoryItemNewEffect(int effectIndex)
: effectIndex_(effectIndex)
{

}

HistoryItemNewEffect::HistoryItemNewEffect()
: effectIndex_(-1)
{

}

HistoryItemNewEffect::~HistoryItemNewEffect()
{
}

HistoryItem* HistoryItemNewEffect::createRedo()
{
	return new HistoryItemNewEffect();
}

void HistoryItemNewEffect::act()
{
	globalDocument->freeze(this);
	if(effectIndex_ >= 0){ // undo
		int count = globalDocument->root()->childrenCount();
		NodeEffect* nodeEffect = safe_cast<NodeEffect*>(globalDocument->root()->child(count - 1));
		globalDocument->root()->remove(nodeEffect);
	}
	else{ // redo
		NodeEffect* effect = globalDocument->add();
		NodeEmitter* emitter = effect->addEmitter();
	}
	globalDocument->unfreeze(this);
}

// ---------------------------------------------------------------------------

class HistoryItemRemoveEffect : public HistoryItem{
public:
	HistoryItemRemoveEffect(int effectIndex, const EffectKey* effect);
	HistoryItemRemoveEffect(int effectIndex);
	~HistoryItemRemoveEffect();
	void act();
	HistoryItem* createRedo();
protected:
	int effectIndex_;
	EffectKey* effect_;
};

HistoryItemRemoveEffect::HistoryItemRemoveEffect(int effectIndex, const EffectKey* effect)
: effectIndex_(effectIndex)
, effect_(0)
{
	effect_ = new EffectKey;
	*effect_ = *effect;

}

HistoryItemRemoveEffect::HistoryItemRemoveEffect(int effectIndex)
: effectIndex_(effectIndex)
, effect_(0)
{

}

HistoryItemRemoveEffect::~HistoryItemRemoveEffect()
{
}

void HistoryItemRemoveEffect::act()
{
	globalDocument->freeze(this);
	if(effect_){ // undo
		EffectKey* effectKey = new EffectKey();
		*effectKey = *effect_;
		kdw::ModelNode* beforeNode = effectIndex_ == (globalDocument->root()->childrenCount())
			                        ? 0 : globalDocument->root()->child(effectIndex_);
		NodeEffect* effect = safe_cast<NodeEffect*>(globalDocument->root()->add(new NodeEffect(effectKey, effectKey->filename.c_str()), beforeNode));
		effect->setSelected(true);
	}
	else{ // redo
		kdw::ModelNode* node = globalDocument->root()->child(effectIndex_);
		globalDocument->root()->remove(node);
	}
	globalDocument->unfreeze(this);
}

HistoryItem* HistoryItemRemoveEffect::createRedo()
{
	return new HistoryItemRemoveEffect(effectIndex_);
}

// ---------------------------------------------------------------------------

HistoryManager::HistoryManager()
{

}

void HistoryManager::push(HistoryItem* item)
{
	historyItems_.push_back(item);
	if(historyItems_.size() > HISTORY_STEPS)
		historyItems_.erase(historyItems_.begin());
	signalChanged_.emit();
}

void HistoryManager::pushNewEffect(int effectIndex)
{
	push(new HistoryItemNewEffect(effectIndex));
}

void HistoryManager::pushRemoveEffect(int effectIndex)
{
	NodeEffect* node = safe_cast<NodeEffect*>(globalDocument->root()->child(effectIndex));
	push(new HistoryItemRemoveEffect(effectIndex, node->get()));
}

void HistoryManager::pushEffect()
{
	if(globalDocument->activeEffect()){
		EffectKey* effect = new EffectKey();
		*effect = *globalDocument->activeEffect()->get();
		int effectIndex = globalDocument->root()->childIndex(globalDocument->activeEffect());
		HistoryItem* item = new HistoryItemEffect(effect, effectIndex);
		push(item);
	}
	redoItems_.clear();
	signalChanged_.emit();
}

void HistoryManager::pushEmitter()
{
	if(globalDocument->activeEmitter()){
		EmitterKeyInterface* emitter = globalDocument->activeEmitter()->get()->Clone();
		int effectIndex = globalDocument->root()->childIndex(globalDocument->activeEffect());
		int emitterIndex = globalDocument->activeEffect()->childIndex(globalDocument->activeEmitter());
		HistoryItem* item = new HistoryItemEmitter(emitter, effectIndex, emitterIndex);
		push(item);
	}
	redoItems_.clear();
	signalChanged_.emit();
}

void HistoryManager::undo()
{
	xassert(!historyItems_.empty());
	HistoryItem* undoItem = historyItems_.back();

	RedoItem redoItem;
	redoItem.undo = undoItem;
	redoItem.redo = undoItem->createRedo();
	xassert(redoItem.redo);
	redoItems_.push_back(redoItem);

	undoItem->act();
	historyItems_.pop_back();
	signalChanged_.emit();
}

void HistoryManager::redo()
{
	xassert(!redoItems_.empty());
	RedoItem redoItem = redoItems_.back();
	historyItems_.push_back(redoItem.undo);
	redoItem.redo->act();
	redoItems_.pop_back();
	signalChanged_.emit();
}

bool HistoryManager::canBeUndone()
{
	return !historyItems_.empty();
}

bool HistoryManager::canBeRedone()
{
	return !redoItems_.empty();
}
