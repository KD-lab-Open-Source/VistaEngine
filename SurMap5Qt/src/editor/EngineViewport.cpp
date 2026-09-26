// EngineViewport.cpp — see header. Compiled with the engine's flags.

#include "EngineViewport.h"

// The engine's headers assume the old StdAfx preamble: `using namespace std`,
// <vector>/<string> (IRenderDevice.h names `vector`/`string` unqualified),
// `xassert` (XLibs.Net/XUtil/xutil.h) and `FOR_EACH` (Util/XTL/my_STL.h) —
// engine headers call all of them.
#include <vector>
#include <string>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cmath>
using namespace std;
#include "xutil.h"
#include "my_STL.h"

// Engine headers — safe here because EditorEngine compiles with EngineIncludes.
#include "Render/inc/IRenderDevice.h"
#include "Render/inc/Unknown.h"     // RELEASE()
#include "Render/src/VisGeneric.h"   // gb_VisGeneric, CreateScene
#include "Render/src/Scene.h"        // cScene (CreateCamera, Draw)
#include "Render/src/cCamera.h"      // Camera (SetFrustum, SetPosition)
#include "Render/src/TileMap.h"      // cTileMap (the terrain's tile map)
#include "Render/src/TexLibrary.h"   // GetTexLibrary (texture statistics)
#include "Render/src/Texture.h"      // cTexture (GetName, CalcTextureSize)
#include "Render/src/FT_Font.h"      // FT::fontManager (editor UI text / drawText)
#include "Util/XMath/xmath.h"        // MatXf/Mat3f/Mat2f + X_AXIS/Y_AXIS/Z_AXIS
#include "Render/SDLRenderDevice.h"
#include "Render/SDLWorldQuadRenderer.h"  // TEMP FX debug (оверрайды quad-рендерера)
#include "Terra/VMAP.H"              // vMap (load/create, H_SIZE/V_SIZE)
#include "Terra/worldFileDispatcher.h" // bitmapDispatcher (the texture brush)
#include "Terra/TerrainType.h"       // TerrainTypeDescriptor (surface names)
#include "Environment/SourceManager.h"  // sourceManager (sources, anchors)
#include "Environment/SourceBase.h"     // SourceBase::label()
#include "Environment/Anchor.h"         // Anchor (Environment owns the type)
#include "Environment/Environment.h"    // environment (Environment tab data)
#include "Water/SkyObject.h"            // EnvironmentTime (GetCurFoneColor), cSkyObj
#include "Water/Waves.h"                // cFixedWavesContainer (fixedWaves)
#include "Game/Universe.h"               // universe(), Players, worldPlayer
#include "Water/CircleManager.h"         // circleManager()->addCircle (selection circle)
#include "Units/CircleManagerParam.h"    // CircleManagerParam (selection circle color)
#include "Game/Player.h"                 // Player::units(), worldPlayer
#include "Game/CameraManager.h"          // cameraManager, splines()
#include "Game/RenderObjects.h"          // initScene/finitScene, terScene, cameraManager
#include "Util/DebugPrm.h"               // logicTimePeriod (the universe logic quant period)
#include "Network/NetPlayer.h"           // MissionDescription
#include "Serialization/Dictionary.h"    // TranslationManager (Universe ctor calls GameOptions::setTranslate)
#include "Serialization/XPrmArchive.h"   // XPrmIArchive (.spg loading)
#include "Units/UnitAttribute.h"         // AttributeBase (libraryKey/isEnvironment)
#include "Units/AttributeReference.h"   // AttributeLibrary (unit placement)
#include "Units/UnitEnvironment.h"       // UnitEnvironment (Environment tab filter)
#include "Units/EnvironmentSimple.h"    // UnitEnvironmentSimple (Environment tab filter)
#include "Units/BaseUnit.h"              // UnitBase, UnitList
#include "Units/BaseUniverseObject.h"    // BaseUniverseObject (world bridge visit)
#include "Units/IronLegion.h"            // UnitLegionary (placement: squad join)
#include "Units/Squad.h"                 // UnitSquad (placement: addUnit)
#include "Physics/RigidBodyBase.h"       // RigidBodyBase::awake (SelectionUtil::awakePhysics)
#include "Util/XTL/SafeCast.h"           // safe_cast (placement: legionary/squad)
#include "Util/ObjectSpreader.h"         // ObjectSpreader (environment spread cluster)
#include "Util/FileUtils/FileUtils.h"    // DirIterator (environment model list)
#include "Units/WeaponAttribute.h"       // EnvironmentType, isEnvironmentSimple
#include "Units/GlobalAttributes.h"      // GlobalAttributes::showHeadNames (Heads library)
#include "Units/CommandsQueue.h"         // CommandColorManager (command colors)
#include "Util/Serialization/EnumDescriptor.h" // getEnumDescriptor (command colors)
#include "Render/3dx/Node3DX.h"          // cObject3dx (model state debug)
#include "Render/3dx/Simply3dx.h"        // cSimply3dx (environment models)
#include "Render/src/NParticle.h"        // cEffect::setVisibleRange (editor shows all effects)
#include "Render/src/CurveWrapper.h"     // CurveWrapperBase/CurveCollector (Effects Editor curves)
#include "Render/src/NParticleID.h"      // IDS_EFFECTKEY (Effects Editor .effect load)
#include "Render/3dx/Saver.h"            // CLoadDirectoryFile/FileSaver (Effects Editor .effect I/O)
#include "Game/GameOptions.h"            // GameOptions::filterBaseGraphOptions (Universe ctor calls it)
#include "UserInterface/UI_Render.h"     // UI_Render::create (SurMap5/SurMap5.cpp prelude)
#include "UserInterface/UI_GlobalAttributes.h" // UI_GlobalAttributes (loadAllLibraries dep)
#include "UserInterface/Controls.h"      // ControlManager (Game Scenario's Controls block)
#include "Util/EffectContainer.h"        // EffectContainer::setTexturesPath (effect texture root)
#include "Util/ZipConfig.h"              // ZipConfig::initArchives (pak archives)
#include "Util/SystemUtil.h"             // setLogicFp (FPU precision)
#include "Util/ConsoleWindow.h"          // ConsoleWindow::instance (console listener)
#include "Util/Win32/DebugSymbolManager.h" // DebugSymbolManager::create
#include "PropertyRows.h"                   // registerBuiltinPropertyRows (LibraryEditor core)
#include "PropertyRowsEngine.h"             // registerEnginePropertyRows (typed rows)
#include "PropertyArchive.h"                // PropertyOArchive/PropertyIArchive (LibraryEditor bridge)
#include "Serialization/LibrariesManager.h" // LibrariesManager (library lookup)
#include "Serialization/LibraryWrapper.h"   // EditorLibraryInterface (library element access)
#include "TriggerEditor/TriggerExport.h"    // TriggerChain/Trigger/Condition/Action (trigger editor session)
#include "Serialization/SerializationFactory.h" // FactorySelector<Action/Condition> (trigger palettes)
#include "Serialization/BinaryArchive.h"    // BinaryOArchive/BinaryIArchive (trigger undo history)
#include "Util/TextDB.h"                    // TextDB::saveLanguage (trigger save, as OnEditTriggers)
#include "Util/EditorVisual.h"              // editorVisual (before/afterQuant, как в CGeneralView::graphQuant)
#include "UserInterface/UserInterface.h"    // UI_Dispatcher (конструируется в Universe ctor)

// SDL_Init(SDL_INIT_VIDEO) normally happens in PlatformWindow::create; the Qt
// editor never calls it (Qt owns the windows), so the GPU device would fail
// with "Video subsystem not initialized". Initialize video here, once.
// SDL_MAIN_HANDLED must precede any SDL header so SDL_main.h does not claim
// main() (this is an executable with its own Qt main).
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace {
// kMouseMove2Angle from CGeneralView::WindowProc — 0.25 deg per pixel.
const float kMouseMove2Angle = 0.25f * 3.14159265f / 180.f;

// SetCameraPosition from Game/CameraManager.cpp — the axis flips that turn the
// orbit matrix into the engine's camera-space convention.
void setCameraPosition(Camera* camera, const MatXf& matrix)
{
	MatXf ml = MatXf::ID;
	ml.rot()[2][2] = -1;
	MatXf mr = MatXf::ID;
	mr.rot()[1][1] = -1;
	camera->SetPosition(mr * ml * matrix);
}

// Heap-allocated SourceManager so the global `sourceManager` is set on
// construction and cleared on destruction. The Qt editor does not run
// Game/Universe (VISTA_EDITOR_NO_UNIVERSE), so this is the only place
// the SourceManager global gets populated. When Universe is enabled,
// Universe's ctor overwrites the global with its own SourceManager
// (it deletes the old one on Universe destruction).
//
// SourceManager lives outside vMap's universe data: the editor's
// world.cls does not persist sources, and vMap::load never restores
// them, so the live list is always empty here. The Objects Manager
// shows the engine's view, which is "Sources: 0, Anchors: 0" for
// the Qt editor's world-only load — SurMap5 (MFC) had a separate
// SourcesSerialization hook in CSurMap5Doc::Serialize that the Qt
// port does not yet wire.
struct SourceManagerHolder {
	SourceManager mgr;
};
SourceManagerHolder g_sourceManagerHolder;

// CSurToolAnchor used kdw::makeName to keep anchor labels unique. That helper
// lives in the heavy Util/kdw/LibraryTab translation unit, so this local copy
// does the same: if `base` is already one of the '|'-separated `reserved`
// labels, return "<base> N" with the first free N.
static std::string uniqueLabel(const std::string& reserved, const std::string& base)
{
	const std::string name = base.empty() ? "unnamed" : base;
	// A '|'-delimited exact match, or "<name> <digits>" followed by '|'/end.
	auto taken = [&reserved](const std::string& candidate){
		size_t pos = 0;
		while(pos <= reserved.size()){
			size_t end = reserved.find('|', pos);
			if(end == std::string::npos)
				end = reserved.size();
			if(reserved.compare(pos, end - pos, candidate) == 0)
				return true;
			pos = end + 1;
		}
		return false;
	};
	if(!taken(name))
		return name;
	for(int i = 2; i < 100000; ++i){
		std::string candidate = name + " " + std::to_string(i);
		if(!taken(candidate))
			return candidate;
	}
	return name;
}

// Universe is built directly on the calling thread, exactly as the original
// SurMap5's CGeneralView::reInitWorld did (`new Universe(*currentMission_, ...)`
// with no SEH, no worker thread). The recursive parameter-formula cycle in the
// .prm files is already contained in Units/Parameters.cpp (thread-local depth
// cap + xassert), so the ctor no longer blows the stack.
//
// The one prerequisite is loadAllLibraries() (Units/UnitAttribute.cpp) before
// the first `new Universe`: Universe::setActivePlayer -> UI_Dispatcher::
// instance().clearTexts() constructs the UI_Dispatcher singleton, whose ctor
// deserializes Scripts\Content\UI_Attributes. That deserialization references
// the other UI_* libraries (UI_SpriteLibrary, UI_FontLibrary, UI_GlobalAttributes,
// ...) which must already be loaded — loadAllLibraries() loads them in the
// right order, which is why SurMap5/GeneralView.cpp:135 calls it in
// initRenderDevice, long before any world load. See init().
}

// TriggerSession — the engine-side TriggerChain behind the Qt trigger editor.
// A port of TriggerView's TriggerChain& + history_ (HISTORY_STEPS = 20 over
// BinaryOArchive snapshots). Only one session is open at a time; the Qt side
// addresses triggers/links by index, never by name (cp1251 vs UTF-8).
struct TriggerSession
{
	enum { HISTORY_STEPS = 20 };
	typedef vector<ShareHandle<BinaryOArchive> > History;

	TriggerChain chain;
	bool open = false;
	History history;
	int undoIndex = 0;

	void close()
	{
		open = false;
		history.clear();
		undoIndex = 0;
		chain = TriggerChain();
	}

	bool openFile(const char* path)
	{
		close();
		if(!path || !path[0])
			return false;
		// TriggerChain::load keeps the fresh START chain when the file does
		// not exist yet (the SelectTriggerDialog New path), and sets name.
		chain.load(path);
		open = true;
		saveStep();
		return true;
	}

	bool save()
	{
		if(!open)
			return false;
		// CMainFrame::OnEditTriggers: chain.save + TextDB languages.
		chain.save();
		TextDB::instance().saveLanguage();
		return true;
	}

	void saveStep()
	{
		// TriggerView::saveStep: skip duplicate snapshots, cap the history.
		ShareHandle<BinaryOArchive> oa = new BinaryOArchive();
		oa->serialize(chain, "triggerChain", 0);
		if(!history.empty() && *history[undoIndex] == *oa)
			return;
		if(!history.empty() && undoIndex != (int)history.size() - 1)
			history.erase(history.begin() + undoIndex + 1, history.end());
		if((int)history.size() > HISTORY_STEPS)
			history.erase(history.begin());
		history.push_back(oa);
		undoIndex = (int)history.size() - 1;
	}

	bool canUndo() const { return open && undoIndex > 0; }
	bool canRedo() const { return open && !history.empty() && undoIndex < (int)history.size() - 1; }

	bool undo()
	{
		if(!canUndo())
			return false;
		BinaryIArchive ia(*history[--undoIndex]);
		ia.serialize(chain, "triggerChain", 0);
		return true;
	}

	bool redo()
	{
		if(!canRedo())
			return false;
		BinaryIArchive ia(*history[++undoIndex]);
		ia.serialize(chain, "triggerChain", 0);
		return true;
	}

	bool valid(int i) const { return open && i >= 0 && i < (int)chain.triggers.size(); }
};

// Chain-property wrapper (TriggerView::TriggerChainPropertySerializer):
// serializeProperties is not a free Serializer, so wrap it for the bridge.
struct TriggerChainPropsSerializer
{
	TriggerChain* chain = nullptr;
	void serialize(Archive& ar) { if(chain) chain->serializeProperties(ar); }
};

// --- Scenario serializers (CMainFrame::MapSerializer / GameSerializer) ---
// Map scenario: mission + universe/map params + environment + camera manager
// + the exported players + the world's trigger-chain names. The original kept
// mission_/players_/worldTriggers_ by reference, so an edit wrote straight into
// them; here mission_ is the live MissionDescription while players_ and
// worldTriggers_ are the bridge's copies, imported back by mapScenarioSetTree.
struct MapScenarioSerializer
{
	MissionDescription& mission_;
	PlayerDataVect& players_;
	TriggerChainNames& worldTriggers_;
	MapScenarioSerializer(MissionDescription& mission, PlayerDataVect& players,
	                      TriggerChainNames& triggers)
		: mission_(mission), players_(players), worldTriggers_(triggers) {}

	void serialize(Archive& ar)
	{
		ar.setFilter(SERIALIZE_PRESET_DATA);
		mission_.serialize(ar);
		if(universe())
			universe()->serialize(ar);

		static_cast<vrtMap&>(vMap).serializeParameters(ar);

		if(environment){
			ar.setFilter(SERIALIZE_WORLD_DATA);
			ar.serialize(*environment, "environment", "Параметры окружения");
		}

		if(cameraManager)
			ar.serialize(*cameraManager, "cameraManager", "Менеджер камер");

		ar.serialize(players_, "players", "Игроки");
		ar.serialize(worldTriggers_, "worldTriggers", "Триггеры мира");
	}
};

// Game scenario: global attributes + game-option presets + UI globals + the
// control manager + the environment's global data.
struct GameScenarioSerializer
{
	void serialize(Archive& ar)
	{
		GlobalAttributes::instance().serializeGameScenario(ar);
		GameOptions::instance().serializePresets(ar);
		UI_GlobalAttributes::instance().serialize(ar);
		if(ar.openBlock("Controls", "Управление")){
			ControlManager::instance().serialize(ar);
			ar.closeBlock();
		}
		if(environment){
			ar.setFilter(SERIALIZE_GLOBAL_DATA);
			environment->serialize(ar);
		}
	}
};

// UIEditor's File > Save (Units/UnitAttribute.cpp) — declared in
// UIEditor/UIEditor_Utils.h, which the Qt editor must not include (MFC).
void saveInterfaceLibraries();

// EnvironmentType from the dialog combo index (SurTool3DM
// convertIdx2EnvironmentType: idx 0 == PHANTOM, else 1 << (idx-1)).
static EnvironmentType environmentTypeFromIndex(int idx)
{
	return EnvironmentType(idx ? 1 << (idx - 1) : 0);
}

// SurTool3DM's CircleInRadius: keep a spread circle while it stays inside the
// brush area.
struct EnvCircleInRadius
{
	explicit EnvCircleInRadius(float radius) : radius_(radius) {}
	bool operator()(const ObjectSpreader::Circle& circle) const
	{
		return circle.position.norm() + circle.radius < radius_;
	}
	float radius_;
};

// Returns the selection's single object (and the count via countOut), matching
// CSurToolSelect's attach path (it edited a single selection).
static BaseUniverseObject* singleSelectedObject(IWorldBridge& bridge, int* countOut = nullptr)
{
	struct Visitor : IEditorObjectVisitor
	{
		BaseUniverseObject* first = nullptr;
		int count = 0;
		void visit(EditorObjectId id) override
		{
			if(count == 0)
				first = reinterpret_cast<BaseUniverseObject*>(id);
			++count;
		}
	} visitor;
	bridge.forEachSelected(visitor);
	if(countOut)
		*countOut = visitor.count;
	return visitor.count == 1 ? visitor.first : nullptr;
}

// Collector of every selected universe object (the multi-selection common
// editor walks them all).
static void collectSelectedObjects(IWorldBridge& bridge, std::vector<BaseUniverseObject*>& out)
{
	struct Visitor : IEditorObjectVisitor
	{
		std::vector<BaseUniverseObject*>* out = nullptr;
		void visit(EditorObjectId id) override
		{
			if(BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id))
				out->push_back(obj);
		}
	} visitor;
	visitor.out = &out;
	bridge.forEachSelected(visitor);
}

// Deep-copy a property row (PropertyRow has no clone): containers recurse,
// leaves are recreated through the factory and their raw value string copied.
static editor::PropertyRow* clonePropertyRow(const editor::PropertyRow* src)
{
	if(!src)
		return nullptr;
	if(src->isContainer()){
		auto* dst = new editor::PropertyRowContainer(src->name().c_str(), src->nameAlt().c_str(),
		                                             src->typeName().c_str());
		for(const editor::PropertyRow* child : src->children())
			dst->addChild(clonePropertyRow(child));
		return dst;
	}
	editor::PropertyRow* dst = editor::PropertyRowFactory::instance().create(
		src->typeName(), src->name().c_str(), src->nameAlt().c_str(), nullptr);
	if(!dst){
		dst = new editor::PropertyRow(src->name().c_str(), src->nameAlt().c_str(), src->typeName().c_str());
	} else {
		dst->setValueFromString(src->valueAsString());
		// The factory marked the empty initial value as UTF-8; restore the
		// source encoding so cp1251 text edits round-trip.
		if(auto* stringRow = dynamic_cast<editor::PropertyRowString*>(dst))
			stringRow->setSourceCp1251(!editor::isValidUtf8(src->valueAsString()));
	}
	return dst;
}

static editor::PropertyRow* findChildByName(editor::PropertyRow* parent, const std::string& name)
{
	if(!parent)
		return nullptr;
	for(editor::PropertyRow* child : parent->children())
		if(child->name() == name)
			return child;
	return nullptr;
}

// Keep only the fields common to `common` and `other` (CAttribEditorCtrl::
// showMix): recurse containers, drop fields missing from either side, flag the
// leaves whose values differ as mixed.
static void intersectPropertyRows(editor::PropertyRow* common, const editor::PropertyRow* other)
{
	std::vector<editor::PropertyRow*> drop;
	for(editor::PropertyRow* child : common->children()){
		editor::PropertyRow* o = findChildByName(const_cast<editor::PropertyRow*>(other), child->name());
		if(!o || o->isContainer() != child->isContainer()){
			drop.push_back(child);
			continue;
		}
		if(child->isContainer()){
			intersectPropertyRows(child, o);
		} else {
			if(child->kind() != o->kind()){
				drop.push_back(child);
				continue;
			}
			if(child->valueAsString() != o->valueAsString())
				child->setMixed(true);
		}
	}
	for(editor::PropertyRow* child : drop)
		common->removeChild(child);
}

// Copy the touched rows of `common` into `dst` at the same path (the
// multi-selection write-back only applies what the user actually edited).
static void applyTouchedRows(editor::PropertyRow* dst, const editor::PropertyRow* common)
{
	if(!dst || !common)
		return;
	for(const editor::PropertyRow* child : common->children()){
		editor::PropertyRow* d = findChildByName(dst, child->name());
		if(!d)
			continue;
		if(child->isContainer())
			applyTouchedRows(d, child);
		else if(child->touched())
			d->setValueFromString(child->valueAsString());
	}
}

// WorldBridge — the engine-side implementation of the engine-free IWorldBridge
// the tools talk to. It is a port of the SelectionUtil globals +
// universe/sourceManager/cameraManager access, but over the common
// BaseUniverseObject base (exactly what the original's UniverseObjectAction
// visited). Every method maps an EditorObjectId to a BaseUniverseObject*.
class WorldBridge : public IWorldBridge
{
public:
	explicit WorldBridge(EngineViewport* owner) : owner_(owner) {}

	EditorObjectId hoverAt(float screenX, float screenY) override { (void)screenX; (void)screenY; return IWorldBridge::kNoObject; }
	void selectInRect(int x0, int y0, int x1, int y1, bool add) override { (void)x0; (void)y0; (void)x1; (void)y1; (void)add; }
	bool screenPointToGround(int sx, int sy, ToolVec3& out) override { (void)sx; (void)sy; (void)out; return false; }

	// forEachSelected: walk units → sources → anchors → camera splines, as
	// SelectionUtil::forEachUniverseObject did.
	void forEachSelected(IEditorObjectVisitor& visitor) override
	{
		if(universe()){
			PlayerVect::const_iterator pi;
			FOR_EACH(universe()->Players, pi){
				const UnitList& units = (*pi)->units();
				UnitList::const_iterator it;
				FOR_EACH(units, it){
					UnitBase* u = *it;   // UnitSerializer::operator UnitBase* (const)
					if(u && u->selected())
						visitor.visit(reinterpret_cast<EditorObjectId>(u));
				}
			}
		}
		if(sourceManager){
			const SourceManager::Sources& sources = sourceManager->sources();
			for(size_t i = 0; i < sources.size(); ++i)
				if(sources[i] && sources[i]->selected())
					visitor.visit(reinterpret_cast<EditorObjectId>(sources[i].get()));
			const SourceManager::Anchors& anchors = sourceManager->anchors();
			for(size_t i = 0; i < anchors.size(); ++i)
				if(anchors[i] && anchors[i]->selected())
					visitor.visit(reinterpret_cast<EditorObjectId>(anchors[i].get()));
		}
		if(cameraManager){
			const CameraSplines& splines = cameraManager->splines();
			for(size_t i = 0; i < splines.size(); ++i)
				if(splines[i] && splines[i]->selected())
					visitor.visit(reinterpret_cast<EditorObjectId>(splines[i].get()));
		}
	}

	void deselectAll() override
	{
		if(sourceManager)
			sourceManager->deselectAll();
		if(universe())
			universe()->deselectAll();
		if(cameraManager){
			CameraSplines::const_iterator it;
			FOR_EACH(cameraManager->splines(), it)
				(*it)->setSelected(false);
		}
	}

	void deleteSelected() override
	{
		if(sourceManager)
			sourceManager->deleteSelected();
		if(universe())
			universe()->deleteSelected();
		if(cameraManager)
			cameraManager->deleteSelected();
	}

	EditorPose objectPose(EditorObjectId id) override
	{
		EditorPose pose;
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		if(!obj)
			return pose;
		const Se3f& p = obj->pose();
		const QuatF& q = p.rot();
		const Vect3f& t = p.trans();
		pose.ow = q.s(); pose.ox = q.x(); pose.oy = q.y(); pose.oz = q.z();
		pose.pos = ToolVec3{ t.x, t.y, t.z };
		return pose;
	}

	void setObjectPose(EditorObjectId id, const EditorPose& pose, bool init) override
	{
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		if(!obj)
			return;
		Se3f p(QuatF(pose.ow, pose.ox, pose.oy, pose.oz),
		       Vect3f(pose.pos.x, pose.pos.y, pose.pos.z));
		obj->setPose(p, init);
	}

	// SelectionUtil::awakePhysics: only units/environment carry a rigid body.
	void awakePhysics(EditorObjectId id) override
	{
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		if(!obj)
			return;
		const UniverseObjectClass objectClass = obj->objectClass();
		if(objectClass == UNIVERSE_OBJECT_UNIT
		   || objectClass == UNIVERSE_OBJECT_ENVIRONMENT){
			UnitBase* unit = dynamic_cast<UnitBase*>(obj);
			if(unit && unit->rigidBody())
				unit->rigidBody()->awake();
		}
	}

	float objectRadius(EditorObjectId id) override
	{
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		return obj ? obj->radius() : 0.f;
	}

	// --- Selected object properties (CSurToolSelect's attrib editor) ---
	// The original attached the single selection's serializer
	// (SerializerUniverseObject(UnitLink) for source/unit/environment, else
	// Serializer(object)); the same PropertyOArchive path the library editor
	// uses. Multiple/zero selections have no single tree.
	editor::PropertyRow* selectedObjectTree(bool editOnly) override
	{
		(void)editOnly;   // no edit-only conditional fields on live objects
		BaseUniverseObject* obj = singleSelectedObject(*this);
		if(!obj)
			return nullptr;
		editor::PropertyOArchive oa;
		Serializer se(*obj);
		se.serialize(oa);
		return oa.root();
	}

	bool selectedObjectSetTree(editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		BaseUniverseObject* obj = singleSelectedObject(*this);
		if(!obj)
			return false;
		editor::PropertyIArchive ia(root);
		Serializer se(*obj);
		se.serialize(ia);
		return true;
	}

	void selectedObjectCounts(int& units, int& environment, int& sources,
	                          int& cameras, int& anchors) override
	{
		units = environment = sources = cameras = anchors = 0;
		struct CountVisitor : IEditorObjectVisitor
		{
			int u = 0, e = 0, s = 0, c = 0, a = 0;
			void visit(EditorObjectId id) override
			{
				BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
				if(!obj)
					return;
				switch(obj->objectClass()){
				case UNIVERSE_OBJECT_ENVIRONMENT: ++e; break;
				case UNIVERSE_OBJECT_UNIT:        ++u; break;
				case UNIVERSE_OBJECT_SOURCE:      ++s; break;
				case UNIVERSE_OBJECT_CAMERA_SPLINE: ++c; break;
				case UNIVERSE_OBJECT_ANCHOR:      ++a; break;
				default: break;
				}
			}
		} visitor;
		forEachSelected(visitor);
		units = visitor.u;
		environment = visitor.e;
		sources = visitor.s;
		cameras = visitor.c;
		anchors = visitor.a;
	}

	// Multi-selection common tree (the original's mixIn + showMix).
	editor::PropertyRow* selectedObjectsCommonTree() override
	{
		std::vector<BaseUniverseObject*> objs;
		collectSelectedObjects(*this, objs);
		if(objs.size() < 2)
			return nullptr;
		std::vector<editor::PropertyRow*> trees;
		trees.reserve(objs.size());
		for(BaseUniverseObject* obj : objs){
			editor::PropertyOArchive oa;
			Serializer se(*obj);
			se.serialize(oa);
			trees.push_back(oa.root());
		}
		editor::PropertyRow* common = clonePropertyRow(trees[0]);
		for(size_t i = 1; i < trees.size(); ++i)
			intersectPropertyRows(common, trees[i]);
		for(editor::PropertyRow* tree : trees)
			delete tree;
		return common;
	}

	bool selectedObjectsSetCommonTree(editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		std::vector<BaseUniverseObject*> objs;
		collectSelectedObjects(*this, objs);
		for(BaseUniverseObject* obj : objs){
			editor::PropertyOArchive oa;
			Serializer se(*obj);
			se.serialize(oa);
			editor::PropertyRow* tree = oa.root();
			applyTouchedRows(tree, root);
			editor::PropertyIArchive ia(tree);
			Serializer seOut(*obj);
			seOut.serialize(ia);
			delete tree;
		}
		return true;
	}

	// --- Scenario editors (CMainFrame::OnEditMap / OnEditGameScenario) ---

	editor::PropertyRow* mapScenarioTree() override
	{
		if(!mission_ || !universe())
			return nullptr;
		scenarioPlayers_.clear();
		universe()->exportPlayers(scenarioPlayers_);
		universe()->worldPlayer()->getPlayerData(scenarioWorldPlayer_);
		MapScenarioSerializer serializer(*mission_, scenarioPlayers_,
		                                scenarioWorldPlayer_.triggerChainNames);
		editor::PropertyOArchive oa;
		Serializer se(serializer, "mapSerializer", "Сценарий карты");
		if(!se.serialize(oa))
			return nullptr;
		return oa.root();
	}

	bool mapScenarioSetTree(editor::PropertyRow* root) override
	{
		if(!root || !mission_ || !universe())
			return false;
		MapScenarioSerializer serializer(*mission_, scenarioPlayers_,
		                                scenarioWorldPlayer_.triggerChainNames);
		editor::PropertyIArchive ia(root);
		Serializer se(serializer, "mapSerializer", "Сценарий карты");
		se.serialize(ia);
		// OnEditMap's post-edit sync: import the (edited) players back and
		// refresh the silhouette colors.
		universe()->importPlayers(scenarioPlayers_);
		universe()->worldPlayer()->setPlayerData(scenarioWorldPlayer_);
		setSilhouetteColors();
		return true;
	}

	bool mapScenarioSave() override
	{
		// OnEditMap: GlobalAttributes::saveLibrary + TextDB language + the
		// world save (OnFileSave -> vMap.save).
		GlobalAttributes::instance().saveLibrary();
		TextDB::instance().saveLanguage();
		if(!vMap.isWorldLoaded())
			return false;
		vMap.save(vMap.getWorldName().c_str());
		return true;
	}

	editor::PropertyRow* gameScenarioTree() override
	{
		if(!universe())
			return nullptr;
		GameScenarioSerializer serializer;
		editor::PropertyOArchive oa;
		Serializer se(serializer, "gameSerializer", "Сценарий игры");
		if(!se.serialize(oa))
			return nullptr;
		return oa.root();
	}

	bool gameScenarioSetTree(editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		GameScenarioSerializer serializer;
		editor::PropertyIArchive ia(root);
		Serializer se(serializer, "gameSerializer", "Сценарий игры");
		return se.serialize(ia);
	}

	bool gameScenarioSave() override
	{
		// OnEditGameScenario: saveAllLibraries().
		saveAllLibraries();
		return true;
	}

	// The live mission the map-scenario serializer edits (EngineViewport owns
	// it; loadWorld/doneWorld keep this in sync).
	void setMission(MissionDescription* mission) { mission_ = mission; }

	// --- UI Editor (UIEditor port) ---

	bool uiTree(std::vector<UiTreeNode>& out) override
	{
		if(!uiEnsureLibraries())
			return false;
		uiNodes_.clear();
		out.clear();
		UI_Dispatcher& dispatcher = UI_Dispatcher::instance();
		for(UI_Dispatcher::ScreenContainer::iterator it = dispatcher.screens().begin();
		    it != dispatcher.screens().end(); ++it)
			addUiScreenTree(*it, -1, out);
		return true;
	}

	editor::PropertyRow* uiNodeTree(int nodeId, bool editOnly) override
	{
		(void)editOnly;
		if(nodeId < 0 || nodeId >= (int)uiNodes_.size())
			return nullptr;
		const UiNodeRef& ref = uiNodes_[nodeId];
		editor::PropertyOArchive oa;
		if(ref.kind == kUiScreen && ref.screen){
			Serializer se(*ref.screen);
			se.serialize(oa);
		}
		else if(ref.kind == kUiState && ref.state){
			Serializer se(*ref.state);
			se.serialize(oa);
		}
		else if(ref.control){
			Serializer se(*ref.control);
			se.serialize(oa);
		}
		else
			return nullptr;
		return oa.root();
	}

	bool uiNodeSetTree(int nodeId, editor::PropertyRow* root) override
	{
		if(!root || nodeId < 0 || nodeId >= (int)uiNodes_.size())
			return false;
		const UiNodeRef& ref = uiNodes_[nodeId];
		editor::PropertyIArchive ia(root);
		if(ref.kind == kUiScreen && ref.screen){
			Serializer se(*ref.screen);
			se.serialize(ia);
		}
		else if(ref.kind == kUiState && ref.state){
			Serializer se(*ref.state);
			se.serialize(ia);
		}
		else if(ref.control){
			Serializer se(*ref.control);
			se.serialize(ia);
		}
		else
			return false;
		return true;
	}

	bool uiSave() override
	{
		if(!uiEnsureLibraries())
			return false;
		// UIEditor's File > Save: saveInterfaceLibraries (UI_Dispatcher + the
		// related UI_* libraries).
		saveInterfaceLibraries();
		return true;
	}

	// --- Effects Editor (EffectEditor port) ---

	bool effectOpen(const std::string& fileName) override
	{
		effectClose();
		CLoadDirectoryFile directory;
		if(!directory.Load(fileName.c_str()))
			return false;
		EffectKey* key = nullptr;
		while(CLoadData* ld = directory.next()){
			if(ld->id == IDS_EFFECTKEY){
				key = new EffectKey;
				key->filename = fileName;
				key->Load(ld);
				std::string texturesPath = extractFilePath(fileName.c_str());
				texturesPath += "\\Textures";
				key->changeTexturePath(texturesPath.c_str());
				// EffectDocument::add preloaded the frame textures so the
				// preview has them; guard because it touches the device.
				try{
					key->preloadTexture();
				}
				catch(...){
				}
			}
		}
		if(!key)
			return false;
		effectKey_ = key;
		effectFileName_ = fileName;
		return true;
	}

	void effectClose() override
	{
		if(owner_)
			owner_->stopEffectPreview();
		effectNodes_.clear();
		effectEmitters_.clear();
		effectCurveStore_.clear();
		if(effectKey_){
			delete effectKey_;
			effectKey_ = nullptr;
		}
		effectFileName_.clear();
	}

	bool effectTree(std::vector<EffectTreeNode>& out) override
	{
		out.clear();
		effectNodes_.clear();
		effectEmitters_.clear();
		effectCurveStore_.clear();
		if(!effectKey_)
			return false;
		const int rootId = addEffectNode(kEffectRoot, -1, effectKey_->name.c_str(),
		                                 "EffectKey", effectKey_, nullptr, nullptr, out);
		for(size_t i = 0; i < effectKey_->emitterKeys.size(); ++i){
			EmitterKeyInterface* emitter = effectKey_->emitterKeys[i].get();
			if(!emitter)
				continue;
			effectEmitters_.push_back(emitter);
			const int emitterId = addEffectNode(kEffectEmitter, rootId, emitter->name.c_str(),
			                                    typeid(*emitter).name(), nullptr, emitter,
			                                    nullptr, out);
			// Collect the emitter's curve wrappers (EffectDocument's
			// NodeEmitter::update ran the same serialize with a
			// CurveCollector); keep the clones alive so their pointers stay
			// valid for the tree.
			CurveCollector collector;
			editor::PropertyOArchive oa;
			Serializer se(*emitter);
			se.serialize(oa);
			CurveCollector::Curves& curves = collector.curves();
			for(size_t c = 0; c < curves.size(); ++c){
				effectCurveStore_.push_back(curves[c]);
				CurveWrapperBase* curve = curves[c].get();
				if(!curve)
					continue;
				addEffectNode(kEffectCurve, emitterId, curve->name(), "Curve",
				              nullptr, emitter, curve, out);
			}
		}
		return true;
	}

	editor::PropertyRow* effectNodeTree(int nodeId, bool editOnly) override
	{
		(void)editOnly;
		if(nodeId < 0 || nodeId >= (int)effectNodes_.size())
			return nullptr;
		const EffectNodeRef& ref = effectNodes_[nodeId];
		editor::PropertyOArchive oa;
		if(ref.kind == kEffectRoot && ref.root){
			Serializer se(*ref.root);
			se.serialize(oa);
		}
		else if(ref.kind == kEffectEmitter && ref.emitter){
			Serializer se(*ref.emitter);
			se.serialize(oa);
		}
		else
			return nullptr;   // curves are edited by the curve editor
		return oa.root();
	}

	bool effectNodeSetTree(int nodeId, editor::PropertyRow* root) override
	{
		if(!root || nodeId < 0 || nodeId >= (int)effectNodes_.size())
			return false;
		const EffectNodeRef& ref = effectNodes_[nodeId];
		editor::PropertyIArchive ia(root);
		if(ref.kind == kEffectRoot && ref.root){
			Serializer se(*ref.root);
			se.serialize(ia);
			return true;
		}
		if(ref.kind == kEffectEmitter && ref.emitter){
			Serializer se(*ref.emitter);
			se.serialize(ia);
			// EffectDocument::NodeEmitter re-BuildKey()'d after a property
			// change; the same keeps the emitter's runtime key in sync.
			ref.emitter->BuildKey();
			return true;
		}
		return false;
	}

	bool effectSave() override
	{
		if(!effectKey_ || effectFileName_.empty())
			return false;
		FileSaver saver;
		if(!saver.Init(effectFileName_.c_str()))
			return false;
		saver.SetData(EXPORT_TO_GAME);
		effectKey_->Save(saver);
		return true;
	}

	bool effectSaveAs(const std::string& fileName) override
	{
		effectFileName_ = fileName;
		return effectSave();
	}

	std::string effectFileName() const override { return effectFileName_; }

	int effectCurveKeyCount(int curveNodeId) override
	{
		if(curveNodeId < 0 || curveNodeId >= (int)effectNodes_.size())
			return 0;
		CurveWrapperBase* curve = effectNodes_[curveNodeId].curve;
		return curve ? (int)curve->size() : 0;
	}

	bool effectCurveKey(int curveNodeId, int index, float& time, float& value) override
	{
		if(curveNodeId < 0 || curveNodeId >= (int)effectNodes_.size())
			return false;
		CurveWrapperBase* curve = effectNodes_[curveNodeId].curve;
		if(!curve || index < 0 || index >= (int)curve->size())
			return false;
		time = curve->time(index);
		value = curve->value(index);
		return true;
	}

	bool effectCurveSetKey(int curveNodeId, int index, float time, float value) override
	{
		if(curveNodeId < 0 || curveNodeId >= (int)effectNodes_.size())
			return false;
		const EffectNodeRef& ref = effectNodes_[curveNodeId];
		if(!ref.curve || index < 0 || index >= (int)ref.curve->size())
			return false;
		ref.curve->setPoint(index, time, value);
		// The clone shares the emitter's key array; refresh its runtime key.
		if(ref.emitter)
			ref.emitter->BuildKey();
		return true;
	}

	bool effectPreview(bool on) override
	{
		if(!owner_)
			return false;
		if(on)
			return owner_->startEffectPreview(effectKey_);
		owner_->stopEffectPreview();
		return true;
	}

	bool effectSetPreviewTime(float time) override
	{
		return owner_ ? owner_->setEffectPreviewTime(time) : false;
	}

	// --- UI Editor tree mutations (UIEditor's Create/Erase actions) ---

	bool uiControlTypes(std::vector<std::string>& out) override
	{
		if(!uiEnsureLibraries())
			return false;
		out.clear();
		const ComboStrings& names =
			FactorySelector<UI_ControlBase>::Factory::instance().comboStringsAlt();
		for(size_t i = 0; i < names.size(); ++i)
			out.push_back(names[i]);
		return true;
	}

	bool uiAddControl(int containerNodeId, int typeIndex) override
	{
		if(containerNodeId < 0 || containerNodeId >= (int)uiNodes_.size())
			return false;
		const UiNodeRef& ref = uiNodes_[containerNodeId];
		UI_ControlContainer* container = ref.screen
			? static_cast<UI_ControlContainer*>(ref.screen)
			: static_cast<UI_ControlContainer*>(ref.control);
		if(!container)
			return false;
		typedef FactorySelector<UI_ControlBase>::Factory Factory;
		Factory& factory = Factory::instance();
		const ComboStrings& names = factory.comboStringsAlt();
		if(typeIndex < 0 || typeIndex >= (int)names.size())
			return false;
		UI_ControlBase* control = factory.createByIndex(typeIndex);
		if(!control)
			return false;
		// CreateControlAction::act: name from the factory, one "Default" state.
		control->setName(names[typeIndex].c_str());
		control->states().push_back(UI_ControlState("Default", true));
		container->addControl(control);
		control->setState(0);
		return true;
	}

	bool uiAddState(int controlNodeId) override
	{
		if(controlNodeId < 0 || controlNodeId >= (int)uiNodes_.size())
			return false;
		UI_ControlBase* control = uiNodes_[controlNodeId].control;
		if(!control)
			return false;
		control->states().push_back(UI_ControlState());
		control->init();
		return true;
	}

	bool uiDeleteNode(int nodeId) override
	{
		if(nodeId < 0 || nodeId >= (int)uiNodes_.size())
			return false;
		const UiNodeRef& ref = uiNodes_[nodeId];
		if(ref.kind == kUiScreen && ref.screen)
			return UI_Dispatcher::instance().removeScreen(ref.screen->name());
		if(ref.kind == kUiState && ref.control && ref.state){
			UI_ControlBase::StateContainer& states = ref.control->states();
			for(size_t i = 0; i < states.size(); ++i){
				if(&states[i] == ref.state){
					states.erase(states.begin() + i);
					ref.control->init();
					return true;
				}
			}
			return false;
		}
		if(ref.control && ref.ownerContainer){
			ref.ownerContainer->removeControl(ref.control);
			return true;
		}
		return false;
	}

	bool uiPreview(int screenNodeId, bool on) override
	{
		if(!owner_)
			return false;
		if(!on){
			owner_->stopUiPreview();
			return true;
		}
		UI_Screen* screen = nullptr;
		if(screenNodeId >= 0 && screenNodeId < (int)uiNodes_.size())
			screen = uiNodes_[screenNodeId].screen;
		return owner_->startUiPreview(screen);
	}

	void setObjectRadius(EditorObjectId id, float radius) override
	{
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		if(obj)
			obj->setRadius(radius);
	}

	float terrainHeight(float x, float y) override
	{
		if(!vMap.isWorldLoaded())
			return 0.f;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		if(xi < 0 || yi < 0 || xi >= (int)vMap.H_SIZE || yi >= (int)vMap.V_SIZE)
			return 0.f;
		return vMap.getZf(xi, yi);
	}

	bool applyGeoNet(float x, float y, float brushRadius,
	                 int height, int noise, int mesh) override
	{
		// SurToolGeoNet::onOperationOnMap -> geoGeneration(sGeoPMO(...)).
		// The brush is a square of side 2*radius; the generation parameters
		// mirror the original's call (powerCellSize=8, powerShift=1,
		// noiseLevel=100, borderForm=3, inverse=0).
		if(!vMap.isWorldLoaded())
			return false;
		const int rad = std::max(1, (int)brushRadius);
		sGeoPMO pmo((int)x, (int)y, rad * 2, rad * 2,
		            height, 8, 1, mesh, noise, 3, 0);
		geoGeneration(pmo);
		return true;
	}

	bool applyToolzer(float x, float y, float brushRadius,
	                  int deltaH, int smooth, int minH, int maxH) override
	{
		// SurToolToolzer::onOperationOnMap -> vMap.deltaZone(sToolzerPMO(...)).
		// The circle brush (BRUSHFORM_CIRCLE, idxCurToolzerType 0): smth=9,
		// smode/eql 0, and the height filter (0..MAX_VX_HEIGHT when disabled).
		if(!vMap.isWorldLoaded())
			return false;
		int rad = std::max(1, (int)brushRadius);
		if(rad > MAX_RADIUS_CIRCLEARR)
			rad = MAX_RADIUS_CIRCLEARR;
		if(minH == 0 && maxH == 0)
			maxH = MAX_VX_HEIGHT;
		sToolzerPMO pmo((int)x, (int)y, rad, 9, deltaH, 0, 0,
		                (short)minH, (short)maxH);
		vMap.deltaZone(pmo);
		return true;
	}

	bool applySurKind(float x, float y, float brushRadius,
	                  int kind, int minH, int maxH) override
	{
		// SurToolKind::onOperationOnMap -> vMap.drawInGrid(x, y, rad, kind,
		// GRIDAT_MASK_SURFACE_KIND, minfh, maxfh).
		if(!vMap.isWorldLoaded())
			return false;
		const int rad = std::max(1, (int)brushRadius);
		if(minH == 0 && maxH == 0)
			maxH = MAX_VX_HEIGHT;
		vMap.drawInGrid((int)x, (int)y, rad, (unsigned short)kind,
		                GRIDAT_MASK_SURFACE_KIND, (short)minH, (short)maxH);
		return true;
	}

	bool setShowSurKind(bool on) override
	{
		// SurToolKind showed the surface-kind tint while active and cleared it
		// on destroy (vMap.toShowSurKind + WorldRender both ways).
		if(!vMap.isWorldLoaded())
			return false;
		vMap.toShowSurKind(on);
		vMap.WorldRender();
		return true;
	}

	bool applyTexturePaint(float x, float y, float brushRadius,
	                       const std::string& texturePath,
	                       int centerAlpha, int kColor,
	                       int saturation, int brightness,
	                       int r, int g, int b,
	                       int minH, int maxH) override
	{
		// SurToolColorPic::onOperationOnMap -> vMap.drawBitmapCircle with a
		// ColorModificator (txColor + the K/S/B sliders / 100).
		if(!vMap.isWorldLoaded())
			return false;
		// getBitmap registers the file and loads it on first use; getUID then
		// yields the index drawBitmapCircle addresses.
		if(!bitmapDispatcher.getBitmap(texturePath.c_str()))
			return false;
		const int uid = bitmapDispatcher.getUID(texturePath.c_str());
		const int rad = std::max(1, (int)brushRadius);
		if(minH == 0 && maxH == 0)
			maxH = MAX_VX_HEIGHT;
		ColorModificator cmod(Color4c((unsigned char)r, (unsigned char)g, (unsigned char)b),
		                      (float)kColor / 100.f, (float)saturation / 100.f,
		                      (float)brightness / 100.f);
		vMap.drawBitmapCircle((int)x, (int)y, rad, (unsigned char)centerAlpha, uid,
		                      (short)minH, (short)maxH, cmod);
		return true;
	}

	bool putTextureToAllWorld(const std::string& texturePath,
	                          int kColor, int saturation, int brightness,
	                          int r, int g, int b,
	                          int minH, int maxH) override
	{
		// SurToolColorPic::OnBnClicked_Put2World -> vMap.putBitmap2AllWorld.
		if(!vMap.isWorldLoaded())
			return false;
		if(!bitmapDispatcher.getBitmap(texturePath.c_str()))
			return false;
		const int uid = bitmapDispatcher.getUID(texturePath.c_str());
		if(minH == 0 && maxH == 0)
			maxH = MAX_VX_HEIGHT;
		ColorModificator cmod(Color4c((unsigned char)r, (unsigned char)g, (unsigned char)b),
		                      (float)kColor / 100.f, (float)saturation / 100.f,
		                      (float)brightness / 100.f);
		vMap.putBitmap2AllWorld(uid, (short)minH, (short)maxH, cmod);
		return true;
	}

	void unitAttributeNames(std::vector<std::string>& out) override
	{
		// SurToolPlayerFolder built its unit tree from
		// AttributeLibrary::instance().map() (each element is a UnitAttribute,
		// wrapping an AttributeBase). Names are display only; placement
		// addresses the index, since names are cp1251 and must not round-trip
		// through Qt.
		out.clear();
		const AttributeLibrary::Map& map = AttributeLibrary::instance().map();
		for(size_t i = 0; i < map.size(); ++i){
			const AttributeBase* attr = map[i].get();
			const char* key = attr ? attr->libraryKey() : nullptr;
			out.push_back(key ? key : "");
		}
	}

	bool unitAttributePlaceable(int index) override
	{
		// SurToolPlayerFolder's world-player folder: isBuilding() ||
		// isLegionary(), !internal. Items/resources/effects are not placed as
		// units.
		const AttributeLibrary::Map& map = AttributeLibrary::instance().map();
		if(index < 0 || index >= (int)map.size())
			return false;
		const AttributeBase* attr = map[index].get();
		if(!attr || attr->internal)
			return false;
		return attr->isBuilding() || attr->isLegionary();
	}

	EditorObjectId placeUnit(int libraryIndex, float x, float y, bool select) override
	{
		// SurToolUnit::createUnit: Player::buildUnit(attribute) + setPose. The
		// unit lands at the terrain height, as CSurToolUnit used To3D.
		if(!vMap.isWorldLoaded() || !universe())
			return kNoObject;
		Player* wp = universe()->worldPlayer();
		if(!wp)
			return kNoObject;
		const AttributeLibrary::Map& map = AttributeLibrary::instance().map();
		if(libraryIndex < 0 || libraryIndex >= (int)map.size())
			return kNoObject;
		const AttributeBase* attr = map[libraryIndex].get();
		if(!attr)
			return kNoObject;
		UnitBase* unit = wp->buildUnit(AttributeReference(attr));
		if(!unit)
			return kNoObject;
		// SurToolUnit::createUnit: a legionary is never standalone — it lives in
		// a UnitSquad, and UnitLegionary::Quant kills a living legionary whose
		// squad() is null (IronLegion.cpp). Build the squad the attribute points
		// at, put it at the same spot, then add the legionary to it. Without
		// this the placed unit blinked and vanished on the first logic quant.
		if(unit->attr().isLegionary()){
			UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
			UnitSquad* squad = safe_cast<UnitSquad*>(wp->buildUnit(&*legionary->attr().squad));
			if(squad)
				squad->setPose(Se3f(QuatF::ID, Vect3f(x, y, 0)), true);
			squad->addUnit(legionary);
		}
		float z = 0.f;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		unit->setPose(Se3f(QuatF::ID, Vect3f(x, y, z)), true);
		if(select)
			unit->setSelected(true);
		return (EditorObjectId)unit;
	}

	// CSurToolUnit's cursor preview (unitOnMouse_): a real auxiliary unit of
	// the picked attribute, reposed under the cursor.
	bool previewUnit(int libraryIndex, float x, float y,
	                 float angle, float angleDelta) override
	{
		killPreviewUnit();
		if(libraryIndex < 0)
			return true;
		if(!vMap.isWorldLoaded() || !universe())
			return false;
		Player* wp = universe()->worldPlayer();
		if(!wp)
			return false;
		const AttributeLibrary::Map& map = AttributeLibrary::instance().map();
		if(libraryIndex >= (int)map.size())
			return false;
		const AttributeBase* attr = map[libraryIndex].get();
		if(!attr)
			return false;
		UnitBase* unit = wp->buildUnit(AttributeReference(attr));
		if(!unit)
			return false;
		unit->setAuxiliary(true);
		previewUnit_ = unit;
		previewUnitLibrary_ = libraryIndex;
		unitSeed_ = rand();
		// A legionary needs its squad (UnitLegionary::Quant kills a living
		// legionary whose squad() is null), exactly like placeUnit.
		if(unit->attr().isLegionary()){
			UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
			UnitSquad* squad = safe_cast<UnitSquad*>(wp->buildUnit(&*legionary->attr().squad));
			if(squad){
				squad->setAuxiliary(true);
				squad->setPose(Se3f(QuatF::ID, Vect3f(x, y, 0)), true);
				squad->addUnit(legionary);
			}
		}
		return movePreviewUnit(x, y, angle, angleDelta);
	}

	bool movePreviewUnit(float x, float y, float angle, float angleDelta) override
	{
		if(!previewUnit_)
			return false;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		float z = 0.f;
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		unitRandom_.set(unitSeed_);
		const float a = angle + unitRandom_.frnd(angleDelta);
		const Se3f pose(QuatF(a * (M_PI / 180.0f), Vect3f::K), Vect3f(x, y, z));
		previewUnit_->setPose(pose, false);
		if(previewUnit_->rigidBody())
			previewUnit_->rigidBody()->setPose(pose);
		return true;
	}

	void killPreviewUnit() override
	{
		if(previewUnit_){
			previewUnit_->Kill();
			previewUnit_ = nullptr;
		}
		previewUnitLibrary_ = -1;
	}

	// --- Source / Anchor placement (CSurToolSource / CSurToolAnchor) ---

	void sourceNames(std::vector<std::string>& out) override
	{
		out.clear();
		const SourcesLibrary::Map& map = SourcesLibrary::instance().map();
		for(size_t i = 0; i < map.size(); ++i){
			const SourceBase* src = map[i].get();
			// label_ is the instance label and is empty for a library element;
			// the element's name is its library key (SourceBase::serialize sets
			// libraryKey_ from SourceReference(this)). Fall back to the source
			// type's display name when the key is empty.
			std::string name;
			if(src){
				name = src->libraryKey();
				if(name.empty())
					name = SourceBase::getDisplayName(src->type());
			}
			out.push_back(name);
		}
	}

	editor::PropertyRow* sourceElementTree(int index, bool editOnly) override
	{
		const SourcesLibrary::Map& map = SourcesLibrary::instance().map();
		if(index < 0 || index >= (int)map.size())
			return nullptr;
		SourceBase* src = map[index].get();
		if(!src)
			return nullptr;
		// The original's attribEditor().attachSerializer(Serializer(*source)):
		// same PropertyOArchive path the library editor uses.
		editor::PropertyOArchive oa;
		Serializer se(*src);
		(void)editOnly;   // SourceBase has no edit-only conditional fields
		se.serialize(oa);
		return oa.root();
	}

	bool sourceElementSetTree(int index, editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		const SourcesLibrary::Map& map = SourcesLibrary::instance().map();
		if(index < 0 || index >= (int)map.size())
			return false;
		SourceBase* src = map[index].get();
		if(!src)
			return false;
		editor::PropertyIArchive ia(root);
		Serializer se(*src);
		se.serialize(ia);
		return true;
	}

	bool previewSource(int index) override
	{
		if(!sourceManager)
			return false;
		if(previewSource_){
			previewSource_->kill();
			previewSource_ = 0;
		}
		if(index < 0 || !vMap.isWorldLoaded())
			return true;
		const SourcesLibrary::Map& map = SourcesLibrary::instance().map();
		if(index >= (int)map.size())
			return false;
		const SourceBase* original = map[index].get();
		if(!original)
			return false;
		// CSurToolSource::OnInitDialog: sourceOnMouse_ = addSource(original).
		previewSource_ = sourceManager->addSource(original);
		if(previewSource_)
			previewSource_->setActivity(true);
		return previewSource_ != 0;
	}

	bool movePreviewSource(float x, float y) override
	{
		if(!previewSource_)
			return false;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		float z = 0.f;
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		previewSource_->setPose(Se3f(previewSource_->orientation(), Vect3f(x, y, z)), true);
		return true;
	}

	EditorObjectId placeSource(int index, float x, float y) override
	{
		if(!sourceManager || !vMap.isWorldLoaded())
			return kNoObject;
		const SourcesLibrary::Map& map = SourcesLibrary::instance().map();
		if(index < 0 || index >= (int)map.size())
			return kNoObject;
		const SourceBase* original = map[index].get();
		if(!original)
			return kNoObject;
		SourceBase* src = sourceManager->addSource(original);
		if(!src)
			return kNoObject;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		float z = 0.f;
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		src->setPose(Se3f(QuatF::ID, Vect3f(x, y, z)), true);
		return (EditorObjectId)src;
	}

	editor::PropertyRow* anchorTree(bool editOnly) override
	{
		editor::PropertyOArchive oa;
		Serializer se(editableAnchor_);
		(void)editOnly;
		se.serialize(oa);
		return oa.root();
	}

	bool anchorSetTree(editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		editor::PropertyIArchive ia(root);
		Serializer se(editableAnchor_);
		se.serialize(ia);
		return true;
	}

	bool previewAnchor(bool create) override
	{
		if(!sourceManager)
			return false;
		if(previewAnchor_){
			sourceManager->removeAnchor(previewAnchor_);
			previewAnchor_ = 0;
		}
		if(!create || !vMap.isWorldLoaded())
			return true;
		// CSurToolAnchor::OnInitDialog: anchorOnMouse_ =
		// sourceManager->addAnchor(originalAnchor()).
		previewAnchor_ = sourceManager->addAnchor(&editableAnchor_);
		return previewAnchor_ != 0;
	}

	bool movePreviewAnchor(float x, float y) override
	{
		if(!previewAnchor_)
			return false;
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		float z = 0.f;
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		previewAnchor_->setPose(Se3f(previewAnchor_->orientation(), Vect3f(x, y, z)), true);
		return true;
	}

	EditorObjectId placeAnchor(float x, float y) override
	{
		if(!sourceManager || !vMap.isWorldLoaded())
			return kNoObject;
		Anchor* anchor = sourceManager->addAnchor(&editableAnchor_);
		if(!anchor)
			return kNoObject;
		// CSurToolAnchor::onOperationOnMap generated a unique label from the
		// existing labels (kdw::makeName) before placing. Same idea, local: the
		// label becomes "<base> N" until it is not taken.
		std::string reserved;
		const SourceManager::Anchors& anchors = sourceManager->anchors();
		for(size_t i = 0; i < anchors.size(); ++i){
			if(anchors[i].get() == anchor)
				continue;
			if(!reserved.empty())
				reserved += "|";
			reserved += anchors[i]->label();
		}
		const std::string label = uniqueLabel(reserved, editableAnchor_.label());
		anchor->setLabel(label.c_str());
		const int xi = (int)roundf(x), yi = (int)roundf(y);
		float z = 0.f;
		if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
			z = vMap.getZf(xi, yi);
		anchor->setPose(Se3f(QuatF::ID, Vect3f(x, y, z)), true);
		return (EditorObjectId)anchor;
	}

	// --- Environment placement (CSurToolEnvironment / CSurTool3DM) ---

	void environmentTypeNames(std::vector<std::string>& out) override
	{
		// The dialog's attributes combo (SurTool3DM::initControls):
		// getEnumNameAlt(EnvironmentType(nItem ? 1 << nItem-1 : 0)).
		out.clear();
		for(int i = 0; i < ENVIRONMENT_TYPE_MAX; ++i){
			const char* n = getEnumNameAlt(environmentTypeFromIndex(i));
			out.push_back(n ? n : "");
		}
	}

	void environmentModelNames(std::vector<std::string>& out) override
	{
		// CSurToolEnvironment browsed Resource\TerrainData\Models (plus the
		// TerTools craters). The mesh cache also holds unit models
		// (Resource\Models\*) and UI/sky models — those are not environment
		// objects and must not appear in this list (they were being placed as
		// environment with the wrong size). Rebuild the reference path from the
		// cache file name: cLib3dx keyed the cache by cutPathToResource(name)
		// with '\' -> '_', so the directory prefixes map back to separators.
		out.clear();
		for(DirIterator it("cacheData\\Models\\*.3dxG"); it; ++it){
			if(!it.isFile())
				continue;
			std::string name = it.c_str();
			// Windows' wildcard also matches .3dxGB/.3dxGL (the logic/burnt
			// variants); keep only the render-model cache.
			if(name.size() < 5 || name.compare(name.size() - 5, 5, ".3dxG") != 0)
				continue;
			name.erase(name.size() - 1);   // strip the cache's trailing "G", keep ".3dx"
			std::string ref;
			if(name.rfind("resource_terraindata_models_", 0) == 0)
				ref = "Resource\\TerrainData\\Models\\" + name.substr(28);
			else if(name.rfind("resource_terraindata_tertools_", 0) == 0)
				ref = "Resource\\TerrainData\\TerTools\\" + name.substr(30);
			else
				continue;   // unit / UI / sky models — not environment objects
			out.push_back(ref);
		}
		std::sort(out.begin(), out.end());
	}

	bool updateEnvironmentPreview(const EnvironmentParams& params,
	                              float x, float y, bool rebuild) override
	{
		envLastX_ = x;
		envLastY_ = y;
		// Only rebuild when the panel says so (model/spread/radius change,
		// activate, after placing) or when there is no preview yet; a plain
		// mouse move just repositions, so the model survives to render.
		if(rebuild || (envPreviewObjs_.empty() && envPreviewSimple_.empty()))
			envBuildPreview(params);
		envPositionPreview(params);
		return true;
	}

	void killEnvironmentPreview() override
	{
		envKillPreview();
	}

	int placeEnvironment(const EnvironmentParams& params, float x, float y) override
	{
		// CSurToolEnvironment::onOperationOnMap.
		if(!vMap.isWorldLoaded() || !universe())
			return 0;
		Player* wp = universe()->worldPlayer();
		if(!wp || params.model.empty())
			return 0;
		envRandom_.set(envSeed_);
		int placed = 0;
		if(params.spread){
			const ObjectSpreader::CirclesList& circles = envSpreader_.circles();
			for(size_t i = 0; i < circles.size(); ++i){
				const Vect2f pos(x + circles[i].position.x, y + circles[i].position.y);
				if(!envCanPlace(wp, pos, circles[i].radius))
					continue;
				if(envBuildUnit(wp, params, pos))
					++placed;
			}
		} else {
			if(envBuildUnit(wp, params, Vect2f(x, y)))
				++placed;
		}
		// The original reseeded the RNG after placing (ReloadM3D +
		// UpdateShapeModel); the tool's refresh then rebuilds the preview.
		envSeed_ = rand();
		return placed;
	}

	bool worldRender() override
	{
		if(!vMap.isWorldLoaded())
			return false;
		vMap.WorldRender();
		return true;
	}

	void cameraNames(std::vector<std::string>& out) override
	{
		out.clear();
		if(!cameraManager)
			return;
		const CameraSplines& splines = cameraManager->splines();
		for(size_t i = 0; i < splines.size(); ++i)
			if(splines[i])
				out.push_back(splines[i]->name());
	}

	bool createCamera(const std::string& name) override
	{
		if(!cameraManager || name.empty())
			return false;
		// The original's CCameraDlg created a spline with the given name and
		// switched to CREATE_POINTS mode; here we just register an empty
		// spline (points are added by the camera tool later).
		CameraSpline* spline = new CameraSpline;
		spline->setName(name.c_str());
		cameraManager->addSpline(spline);
		return true;
	}

	bool deleteCamera(const std::string& name) override
	{
		if(!cameraManager)
			return false;
		CameraSpline* spline = cameraManager->findSpline(name.c_str());
		if(!spline)
			return false;
		cameraManager->deleteSpline(spline);
		return true;
	}

	bool playCamera(const std::string& name) override
	{
		if(!cameraManager)
			return false;
		CameraSpline* spline = cameraManager->findSpline(name.c_str());
		if(!spline)
			return false;
		// CCameraDlg::OnBnClickedButton3: loadPath(name, false) +
		// startReplayPath(stepDuration, 1).
		cameraManager->loadPath(*spline, false);
		cameraManager->startReplayPath(spline->stepDuration(), 1);
		return true;
	}

	void waveNames(std::vector<std::string>& out) override
	{
		out.clear();
		if(!environment || !environment->fixedWaves())
			return;
		cFixedWavesContainer* waves = environment->fixedWaves();
		for(int i = 0; i < waves->GetCount(); ++i)
			if(cFixedWaves* w = waves->GetWave(i))
				out.push_back(w->name());
	}

	bool createWave(const std::string& name) override
	{
		if(!environment || !environment->fixedWaves() || name.empty())
			return false;
		cFixedWaves* wave = environment->fixedWaves()->AddWaves();
		if(!wave)
			return false;
		wave->name() = name;
		return true;
	}

	bool removeWave(const std::string& name) override
	{
		if(!environment || !environment->fixedWaves())
			return false;
		cFixedWavesContainer* waves = environment->fixedWaves();
		for(int i = 0; i < waves->GetCount(); ++i){
			cFixedWaves* w = waves->GetWave(i);
			if(w && w->name() == name)
				return waves->DeleteWaves(w);
		}
		return false;
	}

	bool applyWave(const std::string& name, float distance, float speed,
	               float sizeMin, float sizeMax, float generationTime,
	               bool invert) override
	{
		if(!environment || !environment->fixedWaves())
			return false;
		cFixedWavesContainer* waves = environment->fixedWaves();
		for(int i = 0; i < waves->GetCount(); ++i){
			cFixedWaves* w = waves->GetWave(i);
			if(!w || w->name() != name)
				continue;
			// CWaveDlg::OnBnClickedApply: write the properties + rebuild.
			w->distance() = distance;
			w->speed() = speed / 10.f;
			w->generationTime() = generationTime;
			w->invertation() = invert ? -1 : 1;
			w->sizeMin() = sizeMin;
			w->sizeMax() = sizeMax;
			w->CreateSegments();
			return true;
		}
		return false;
	}

	float timeOfDay() override
	{
		if(!environment || !environment->environmentTime())
			return -1.f;
		return environment->environmentTime()->GetTime();
	}

	bool setTimeOfDay(float hours) override
	{
		if(!environment || !environment->environmentTime())
			return false;
		environment->environmentTime()->SetTime(hours);
		return true;
	}

	void headNames(std::vector<std::string>& out) override
	{
		out.clear();
		// GlobalAttributes::showHeadNames is a public field (vector of
		// ShowHeadName, each holding a file name).
		const ShowHeadNames& heads = GlobalAttributes::instance().showHeadNames;
		for(size_t i = 0; i < heads.size(); ++i)
			out.push_back(heads[i].fileName_);
	}

	bool setHeadNames(const std::vector<std::string>& names) override
	{
		// Replace the head list and persist it (GlobalAttributes::saveLibrary).
		ShowHeadNames& heads = GlobalAttributes::instance().showHeadNames;
		heads.clear();
		for(const std::string& n : names)
			heads.push_back(ShowHeadName(n.c_str()));
		GlobalAttributes::instance().saveLibrary();
		return true;
	}

	void terrainTypeNames(std::vector<std::string>& names,
	                      std::vector<unsigned>& colors) override
	{
		names.clear();
		colors.clear();
		// TerrainTypeDescriptor: names via nameAlt(1<<i), colors via getColors().
		TerrainTypeDescriptor& desc = TerrainTypeDescriptor::instance();
		const Color4c* cols = desc.getColors();
		for(int i = 0; i < TERRAIN_TYPES_NUMBER; ++i){
			const char* n = desc.nameAlt(1 << i);
			names.push_back(n ? n : "");
			if(cols){
				const Color4c& c = cols[i];
				colors.push_back(((unsigned)c.r << 16) | ((unsigned)c.g << 8) | (unsigned)c.b);
			}
			else
				colors.push_back(0);
		}
	}

	bool setTerrainTypeNames(const std::vector<std::string>& names,
	                         const std::vector<unsigned>& colors) override
	{
		// TerrainTypeDescriptor's name/color arrays are private; the public
		// write path is serialize + saveLibrary. Rebuild via a temporary
		// archive is not practical here, so this is a no-op for now (the
		// dialog shows the names read-only).
		(void)names; (void)colors;
		return false;
	}

	void commandColors(std::vector<int>& ids,
	                   std::vector<unsigned>& colors) override
	{
		// CommandColorManager::colors_ is private with no setter; reading the
		// per-command colors requires the CommandID enum. Expose the ids and
		// their colors via getColor where the enum is reachable.
		ids.clear();
		colors.clear();
		const EnumDescriptor& desc = getEnumDescriptor(CommandID(0));
		const ComboStrings& strings = desc.comboStrings();
		for(size_t i = 0; i < strings.size(); ++i){
			const int key = desc.keyByName(strings[i].c_str());
			const Color3c& c = CommandColorManager::instance().getColor(CommandID(key));
			ids.push_back(key);
			colors.push_back(((unsigned)c.r << 16) | ((unsigned)c.g << 8) | (unsigned)c.b);
		}
	}

	bool setCommandColor(int id, unsigned color) override
	{
		// CommandColorManager::colors_ is private with no setter; writing a
		// color would need engine changes. No-op for now.
		(void)id; (void)color;
		return false;
	}

	// --- Generic library editor (LibraryEditorDialog) ---

	void libraryElementNames(const std::string& libraryName,
	                         std::vector<std::string>& out) override
	{
		// One entry per element index (empty when unnamed): positions
		// address the engine, so nothing is skipped here — the tree hides
		// empty names itself.
		out.clear();
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return;
		const std::size_t count = lib->editorSize();
		for(std::size_t i = 0; i < count; ++i){
			const char* name = lib->editorElementName((int)i);
			out.push_back(name ? name : "");
		}
	}

	void libraryElementGroups(const std::string& libraryName,
	                          std::vector<std::string>& out) override
	{
		// Parallel to libraryElementNames (one entry per element index).
		out.clear();
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return;
		const std::size_t count = lib->editorSize();
		for(std::size_t i = 0; i < count; ++i)
			out.push_back(lib->editorElementGroup((int)i));
	}

	std::string libraryGroupsComboList(const std::string& libraryName) override
	{
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return std::string();
		const char* list = lib->editorGroupsComboList();
		return list ? list : std::string();
	}

	editor::PropertyRow* libraryElementTree(const std::string& libraryName,
	                                       int elementIndex,
	                                       bool editOnly) override
	{
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return nullptr;
		if(elementIndex < 0 || elementIndex >= (int)lib->editorSize())
			return nullptr;
		Serializer se = lib->editorElementSerializer(elementIndex, "", "", editOnly);
		if(!se)
			return nullptr;
		editor::PropertyOArchive oa;
		se.serialize(oa);
		return oa.root();
	}

	bool libraryElementSetTree(const std::string& libraryName,
	                           int elementIndex,
	                           editor::PropertyRow* root) override
	{
		if(!root)
			return false;
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return false;
		if(elementIndex < 0 || elementIndex >= (int)lib->editorSize())
			return false;
		Serializer se = lib->editorElementSerializer(elementIndex, "", "", false);
		if(!se)
			return false;
		editor::PropertyIArchive ia(root);
		se.serialize(ia);
		return true;
	}

	bool librarySave(const std::string& libraryName) override
	{
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return false;
		lib->saveLibrary();
		return true;
	}

	// --- Trigger editor (TriggerEditor port) ---

	bool triggerSessionOpen(const std::string& filePath) override
	{
		return triggerSession_.openFile(filePath.c_str());
	}

	bool triggerSessionSave() override
	{
		return triggerSession_.save();
	}

	void triggerSessionClose() override
	{
		triggerSession_.close();
	}

	bool triggerSessionOpenNow() override
	{
		return triggerSession_.open;
	}

	std::string triggerChainName() override
	{
		return triggerSession_.open ? triggerSession_.chain.name : std::string();
	}

	void triggerList(std::vector<TriggerInfo>& out) override
	{
		out.clear();
		if(!triggerSession_.open)
			return;
		TriggerList& triggers = triggerSession_.chain.triggers;
		for(size_t i = 0; i < triggers.size(); ++i){
			Trigger& t = triggers[i];
			TriggerInfo info;
			info.name = t.name() ? t.name() : "";
			info.cellX = t.cellIndex().x;
			info.cellY = t.cellIndex().y;
			const Color4c& c = t.color();
			info.colorRGBA = ((unsigned)c.a << 24) | ((unsigned)c.r << 16) |
			                     ((unsigned)c.g << 8) | (unsigned)c.b;
			info.state = (int)t.state();
			if(t.condition){
				if(Condition* cond = t.condition.get())
					info.conditionType = FactorySelector<Condition>::Factory::instance().find(cond).name();
			}
			if(t.action){
				if(Action* act = t.action.get())
					info.actionType = FactorySelector<Action>::Factory::instance().find(act).name();
			}
			out.push_back(info);
		}
	}

	void triggerLinkList(std::vector<TriggerLinkInfo>& out) override
	{
		out.clear();
		if(!triggerSession_.open)
			return;
		TriggerChain& chain = triggerSession_.chain;
		const Vect2f grid = Trigger::gridSize();
		for(size_t i = 0; i < chain.triggers.size(); ++i){
			Trigger& parent = chain.triggers[i];
			OutcomingLinksList::iterator li;
			FOR_EACH(parent.outcomingLinks(), li){
				int childIndex = -1;
				if(li->child)
					childIndex = chain.triggerIndex(*li->child);
				if(childIndex < 0 && li->triggerName()){
					if(Trigger* f = chain.find(li->triggerName()))
						childIndex = chain.triggerIndex(*f);
				}
				if(childIndex < 0)
					continue;
				Trigger& child = chain.triggers[(size_t)childIndex];
				TriggerLinkInfo info;
				info.parent = (int)i;
				info.child = childIndex;
				info.colorType = li->colorType();
				info.autoRestarted = li->autoRestarted();
				info.active = li->active();
				// Offsets are private; recover them from the link points.
				const Vect2f pp = li->parentPoint();
				const Vect2f cp = li->childPoint();
				const Vect2f plt = parent.leftTop();
				const Vect2f clt = child.leftTop();
				info.parentOffsetX = (int)(pp.x - (plt.x + grid.x * 0.5f));
				info.parentOffsetY = (int)(pp.y - (plt.y + grid.y * 0.5f));
				info.childOffsetX = (int)(cp.x - (clt.x + grid.x * 0.5f));
				info.childOffsetY = (int)(cp.y - (clt.y + grid.y * 0.5f));
				out.push_back(info);
			}
		}
	}

	int triggerCreate(int actionTypeIndex, const std::string& nameHint,
	                  int cellX, int cellY) override
	{
		if(!triggerSession_.open)
			return -1;
		TriggerChain& chain = triggerSession_.chain;
		// TriggerView::createTrigger: unique name + action from the palette.
		std::string name = chain.uniqueName(nameHint.empty() ? "Trigger" : nameHint.c_str());
		Trigger trigger;
		trigger.setName(name.c_str());
		{
			typedef FactorySelector<Action>::Factory Factory;
			if(actionTypeIndex >= 0 && actionTypeIndex < Factory::instance().size()){
				if(Action* a = Factory::instance().createByIndex(actionTypeIndex))
					trigger.action = a;
			}
		}
		trigger.setCellIndex(Vect2i(cellX, cellY));
		trigger.setColor(Color4c(128, 255, 128));
		trigger.setSelected(false);
		chain.triggers.push_back(trigger);
		chain.buildLinks();
		triggerSession_.saveStep();
		return (int)chain.triggers.size() - 1;
	}

	bool triggerDelete(int triggerIndex) override
	{
		// Index 0 is START (TriggerChain::initialize guarantees it); the
		// original never deleted it via the graph, so protect it here.
		if(!triggerSession_.valid(triggerIndex) || triggerIndex == 0)
			return false;
		triggerSession_.chain.removeTrigger(triggerIndex);
		triggerSession_.saveStep();
		return true;
	}

	bool triggerRename(int triggerIndex, const std::string& newName) override
	{
		if(!triggerSession_.valid(triggerIndex) || newName.empty())
			return false;
		TriggerChain& chain = triggerSession_.chain;
		if(Trigger* f = chain.find(newName.c_str())){
			if(chain.triggerIndex(*f) != triggerIndex)
				return false;
		}
		const char* oldName = chain.triggers[(size_t)triggerIndex].name();
		chain.renameTrigger(oldName ? oldName : "", newName.c_str());
		triggerSession_.saveStep();
		return true;
	}

	bool triggerSetCell(int triggerIndex, int cellX, int cellY) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return false;
		triggerSession_.chain.triggers[(size_t)triggerIndex].setCellIndex(Vect2i(cellX, cellY));
		triggerSession_.saveStep();
		return true;
	}

	bool triggerCreateLink(int parentIndex, int childIndex,
	                       int colorType, bool autoRestarted) override
	{
		if(!triggerSession_.valid(parentIndex) || !triggerSession_.valid(childIndex))
			return false;
		if(parentIndex == childIndex)
			return false;
		TriggerChain& chain = triggerSession_.chain;
		Trigger& parent = chain.triggers[(size_t)parentIndex];
		Trigger& child = chain.triggers[(size_t)childIndex];
		OutcomingLinksList::iterator li;
		FOR_EACH(parent.outcomingLinks(), li)
			if(li->child == &child)
				return false;
		// TriggerView::createLink: push + name + buildLinks, then style.
		parent.outcomingLinks().push_back(TriggerLink());
		TriggerLink& link = parent.outcomingLinks().back();
		link.setTriggerName(child.name());
		chain.buildLinks();
		if(colorType >= 0 && colorType < STRATEGY_COLOR_MAX)
			link.setColorType(colorType);
		link.setAutoRestarted(autoRestarted);
		triggerSession_.saveStep();
		return true;
	}

	bool triggerDeleteLink(int parentIndex, int childIndex) override
	{
		if(!triggerSession_.valid(parentIndex) || !triggerSession_.valid(childIndex))
			return false;
		TriggerChain& chain = triggerSession_.chain;
		Trigger& parent = chain.triggers[(size_t)parentIndex];
		Trigger& child = chain.triggers[(size_t)childIndex];
		OutcomingLinksList::iterator li;
		FOR_EACH(parent.outcomingLinks(), li){
			if(li->child == &child || (li->triggerName() && child.name() &&
			    !strcmp(li->triggerName(), child.name()))){
				parent.outcomingLinks().erase(li);
				chain.buildLinks();
				triggerSession_.saveStep();
				return true;
			}
		}
		return false;
	}

	editor::PropertyRow* triggerConditionTree(int triggerIndex) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return nullptr;
		ShareHandle<Condition>& cond = triggerSession_.chain.triggers[(size_t)triggerIndex].condition;
		Serializer se(cond, "condition", "Condition");
		editor::PropertyOArchive oa;
		se.serialize(oa);
		return oa.root();
	}

	editor::PropertyRow* triggerActionTree(int triggerIndex) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return nullptr;
		ShareHandle<Action>& act = triggerSession_.chain.triggers[(size_t)triggerIndex].action;
		Serializer se(act, "action", "Action");
		editor::PropertyOArchive oa;
		se.serialize(oa);
		return oa.root();
	}

	bool triggerConditionSetTree(int triggerIndex, editor::PropertyRow* root) override
	{
		if(!triggerSession_.valid(triggerIndex) || !root)
			return false;
		// Write directly into the pointed-to object, not the ShareHandle:
		// PropertyIArchive::openPointer cannot preserve polymorphic
		// pointers (it returns NULL_POINTER, so serializePolymorphic input
		// would delete the condition). Type changes go through
		// triggerSetConditionType; here only field values are written.
		Condition* cond = triggerSession_.chain.triggers[(size_t)triggerIndex].condition.get();
		if(!cond)
			return false;
		Serializer se(*cond, "condition", "Condition");
		editor::PropertyIArchive ia(root);
		se.serialize(ia);
		triggerSession_.saveStep();
		return true;
	}

	bool triggerActionSetTree(int triggerIndex, editor::PropertyRow* root) override
	{
		if(!triggerSession_.valid(triggerIndex) || !root)
			return false;
		// Same as above: direct-object write, type changes via
		// triggerSetActionType.
		Action* act = triggerSession_.chain.triggers[(size_t)triggerIndex].action.get();
		if(!act)
			return false;
		Serializer se(*act, "action", "Action");
		editor::PropertyIArchive ia(root);
		se.serialize(ia);
		triggerSession_.saveStep();
		return true;
	}

	editor::PropertyRow* triggerTree(int triggerIndex) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return nullptr;
		Trigger& trigger = triggerSession_.chain.triggers[(size_t)triggerIndex];
		Serializer se(trigger, "trigger", "Trigger");
		editor::PropertyOArchive oa;
		se.serialize(oa);
		return oa.root();
	}

	bool triggerSetTree(int triggerIndex, editor::PropertyRow* root) override
	{
		if(!triggerSession_.valid(triggerIndex) || !root)
			return false;
		TriggerChain& chain = triggerSession_.chain;
		Trigger& trigger = chain.triggers[(size_t)triggerIndex];
		const char* oldName = trigger.name();
		const std::string oldNameCopy = oldName ? oldName : "";
		Serializer se(trigger, "trigger", "Trigger");
		editor::PropertyIArchive ia(root);
		se.serialize(ia);
		// A renamed trigger must rewire link names (Trigger::setName only
		// fixes incoming links of the live object; the serialized write
		// bypasses it, so rename explicitly like onPropertyChanged did).
		const char* newName = trigger.name();
		if(newName && oldNameCopy != newName)
			chain.renameTrigger(oldNameCopy.c_str(), newName);
		chain.buildLinks();
		triggerSession_.saveStep();
		return true;
	}

	editor::PropertyRow* triggerChainTree() override
	{
		if(!triggerSession_.open)
			return nullptr;
		TriggerChainPropsSerializer wrap;
		wrap.chain = &triggerSession_.chain;
		Serializer se(wrap, "chain", "TriggerChain");
		editor::PropertyOArchive oa;
		se.serialize(oa);
		return oa.root();
	}

	bool triggerChainSetTree(editor::PropertyRow* root) override
	{
		if(!triggerSession_.open || !root)
			return false;
		TriggerChainPropsSerializer wrap;
		wrap.chain = &triggerSession_.chain;
		Serializer se(wrap, "chain", "TriggerChain");
		editor::PropertyIArchive ia(root);
		se.serialize(ia);
		triggerSession_.saveStep();
		return true;
	}

	void triggerActionTypes(std::vector<std::string>& names,
	                        std::vector<std::string>& namesAlt) override
	{
		names.clear();
		namesAlt.clear();
		typedef FactorySelector<Action>::Factory Factory;
		const ComboStrings& combo = Factory::instance().comboStrings();
		const ComboStrings& comboAlt = Factory::instance().comboStringsAlt();
		for(size_t i = 0; i < combo.size(); ++i)
			names.push_back(combo[i]);
		for(size_t i = 0; i < comboAlt.size(); ++i)
			namesAlt.push_back(comboAlt[i]);
	}

	void triggerConditionTypes(std::vector<std::string>& names,
	                           std::vector<std::string>& namesAlt) override
	{
		names.clear();
		namesAlt.clear();
		typedef FactorySelector<Condition>::Factory Factory;
		const ComboStrings& combo = Factory::instance().comboStrings();
		const ComboStrings& comboAlt = Factory::instance().comboStringsAlt();
		for(size_t i = 0; i < combo.size(); ++i)
			names.push_back(combo[i]);
		for(size_t i = 0; i < comboAlt.size(); ++i)
			namesAlt.push_back(comboAlt[i]);
	}

	bool triggerSetActionType(int triggerIndex, int typeIndex) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return false;
		typedef FactorySelector<Action>::Factory Factory;
		if(typeIndex < 0 || typeIndex >= Factory::instance().size())
			return false;
		Action* a = Factory::instance().createByIndex(typeIndex);
		if(!a)
			return false;
		triggerSession_.chain.triggers[(size_t)triggerIndex].action = a;
		triggerSession_.saveStep();
		return true;
	}

	bool triggerSetConditionType(int triggerIndex, int typeIndex) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return false;
		// Negative index clears the condition (the Clear button path).
		if(typeIndex < 0){
			triggerSession_.chain.triggers[(size_t)triggerIndex].condition = (Condition*)nullptr;
			triggerSession_.saveStep();
			return true;
		}
		typedef FactorySelector<Condition>::Factory Factory;
		if(typeIndex < 0 || typeIndex >= Factory::instance().size())
			return false;
		Condition* c = Factory::instance().createByIndex(typeIndex);
		if(!c)
			return false;
		triggerSession_.chain.triggers[(size_t)triggerIndex].condition = c;
		triggerSession_.saveStep();
		return true;
	}

	bool triggerSetConditionInverted(int triggerIndex, bool inverted) override
	{
		if(!triggerSession_.valid(triggerIndex))
			return false;
		Condition* c = triggerSession_.chain.triggers[(size_t)triggerIndex].condition.get();
		if(!c)
			return false;
		c->setInverted(inverted);
		triggerSession_.saveStep();
		return true;
	}

	bool triggerCanUndo() override { return triggerSession_.canUndo(); }
	bool triggerCanRedo() override { return triggerSession_.canRedo(); }
	bool triggerUndo() override { return triggerSession_.undo(); }
	bool triggerRedo() override { return triggerSession_.redo(); }

	void triggerLogRecords(std::vector<TriggerLogRecord>& out) override
	{
		out.clear();
		if(!triggerSession_.open)
			return;
		const TriggerChain::TriggerEventList& log = triggerSession_.chain.logData();
		for(size_t i = 0; i < log.size(); ++i){
			TriggerLogRecord rec;
			rec.event = log[i].event;
			rec.triggerName = log[i].triggerName;
			rec.state = (int)log[i].state;
			out.push_back(rec);
		}
	}

private:
	// --- UI Editor helpers (UIEditor port) ---

	// One UI node's engine object (indexed by UiTreeNode::id).
	struct UiNodeRef
	{
		int kind = kUiScreen;
		UI_Screen* screen = nullptr;
		UI_ControlBase* control = nullptr;
		UI_ControlState* state = nullptr;
		UI_ControlContainer* ownerContainer = nullptr;
	};

	bool uiEnsureLibraries()
	{
		if(uiLibrariesInited_)
			return true;
		if(!owner_ || !owner_->ensureUiLibraries())
			return false;
		uiLibrariesInited_ = true;
		return true;
	}

	int addUiNode(int kind, int parentId, const char* name, const char* type,
	              UI_Screen* screen, UI_ControlBase* control, UI_ControlState* state,
	              std::vector<UiTreeNode>& out)
	{
		UiNodeRef ref;
		ref.kind = kind;
		ref.screen = screen ? screen : uiCurrentScreen_;
		ref.control = control;
		ref.state = state;
		const int id = (int)uiNodes_.size();
		uiNodes_.push_back(ref);
		UiTreeNode node;
		node.id = id;
		node.parentId = parentId;
		node.kind = kind;
		node.name = name ? name : "";
		node.type = type ? type : "";
		out.push_back(node);
		return id;
	}

	// Recurse a container's controls (UIEditor's UITreeObjectControl child
	// enumeration), adding a state node per control state first.
	void addUiContainerTree(UI_ControlContainer& container, int parentId,
	                        std::vector<UiTreeNode>& out)
	{
		const UI_ControlContainer::ControlList& list = container.controlList();
		for(size_t i = 0; i < list.size(); ++i){
			UI_ControlBase* control = list[i].get();
			if(!control)
				continue;
			const int id = addUiNode(kUiControl, parentId, control->name(),
			                         typeid(*control).name(), nullptr, control, nullptr, out);
			uiNodes_[id].ownerContainer = &container;
			UI_ControlBase::StateContainer& states = control->states();
			for(size_t j = 0; j < states.size(); ++j)
				addUiNode(kUiState, id, states[j].name(), "UI_ControlState",
				          nullptr, control, &states[j], out);
			addUiContainerTree(*control, id, out);
		}
	}

	void addUiScreenTree(UI_Screen& screen, int parentId, std::vector<UiTreeNode>& out)
	{
		uiCurrentScreen_ = &screen;
		const int id = addUiNode(kUiScreen, parentId, screen.name(), "UI_Screen",
		                         &screen, nullptr, nullptr, out);
		addUiContainerTree(screen, id, out);
		uiCurrentScreen_ = nullptr;
	}

	// --- Effects Editor helpers (EffectEditor port) ---

	struct EffectNodeRef
	{
		int kind = kEffectRoot;
		EffectKey* root = nullptr;
		EmitterKeyInterface* emitter = nullptr;
		CurveWrapperBase* curve = nullptr;
	};

	int addEffectNode(int kind, int parentId, const char* name, const char* type,
	                  EffectKey* root, EmitterKeyInterface* emitter, CurveWrapperBase* curve,
	                  std::vector<EffectTreeNode>& out)
	{
		EffectNodeRef ref;
		ref.kind = kind;
		ref.root = root;
		ref.emitter = emitter;
		ref.curve = curve;
		const int id = (int)effectNodes_.size();
		effectNodes_.push_back(ref);
		EffectTreeNode node;
		node.id = id;
		node.parentId = parentId;
		node.kind = kind;
		node.name = name ? name : "";
		node.type = type ? type : "";
		out.push_back(node);
		return id;
	}

	void envKillPreview()
	{
		for(size_t i = 0; i < envPreviewObjs_.size(); ++i)
			RELEASE(envPreviewObjs_[i]);
		envPreviewObjs_.clear();
		for(size_t i = 0; i < envPreviewSimple_.size(); ++i)
			RELEASE(envPreviewSimple_[i]);
		envPreviewSimple_.clear();
	}

	// A simple environment (tree/bush/stone/...) is placed as UnitEnvironmentSimple
	// whose model is a cSimply3dx; buildings use cObject3dx. The preview must use
	// the same kind as placement, or it renders as nothing.
	bool envPreviewIsSimple(const EnvironmentParams& params)
	{
		return isEnvironmentSimple(environmentTypeFromIndex(params.typeIndex));
	}

	// SurTool3DM::getScale: scaleSlider +/- scaleDeltaSlider percent.
	float envScale(const EnvironmentParams& params)
	{
		float sv = params.scale;
		float sd = params.scaleDelta;
		return (sv * (100.0f + envRandom_.frnd(sd)) / 100.0f) / 100.0f;
	}

	// SurTool3DM::calculateObjectPose: terrain height + normal alignment.
	Se3f envObjectPose(const EnvironmentParams& params, const Vect2f& position, float radius)
	{
		Vect3f normal;
		Vect3f pos(position.x, position.y, 0.0f);
		const Vect2i center((int)roundf(position.x), (int)roundf(position.y));
		pos.z = vMap.analyzeArea(center, (int)roundf(radius), normal);
		float angle = params.angle + envRandom_.frnd(params.angleDelta);
		Se3f result(QuatF(angle * (M_PI / 180.0f), Vect3f::K), pos);
		if(!params.vertical){
			Vect3f cross = Vect3f::K % normal;
			float len = cross.norm();
			if(len > 1e-5f)
				result.rot().premult(QuatF(acosf(dot(Vect3f::K, normal) / (normal.norm() + 1e-5f)), cross));
		}
		return result;
	}

	void envBuildPreview(const EnvironmentParams& params)
	{
		envKillPreview();
		if(!terScene || params.model.empty() || !vMap.isWorldLoaded())
			return;
		envSeed_ = rand();
		envRandom_.set(envSeed_);
		const bool simple = envPreviewIsSimple(params);
		int count = 1;
		if(params.spread){
			float scale = max(5.0f, params.spreadRadius);
			float d = params.spreadRadiusDelta / 100.0f;
			envSpreader_.setSeed(envSeed_);
			envSpreader_.setRadius(Rangef(scale - scale * d, scale + scale * d));
			envSpreader_.fill(EnvCircleInRadius(params.brushRadius));
			count = (int)envSpreader_.circles().size();
		}
		for(int i = 0; i < count; ++i){
			if(simple){
				if(cSimply3dx* m = terScene->CreateSimply3dx(params.model.c_str()))
					envPreviewSimple_.push_back(m);
			} else {
				if(cObject3dx* m = terScene->CreateObject3dx(params.model.c_str()))
					envPreviewObjs_.push_back(m);
			}
		}
	}

	void envPositionPreview(const EnvironmentParams& params)
	{
		const bool simple = envPreviewIsSimple(params);
		const size_t total = simple ? envPreviewSimple_.size() : envPreviewObjs_.size();
		if(total == 0)
			return;
		envRandom_.set(envSeed_);
		const Vect2f center(envLastX_, envLastY_);
		const ObjectSpreader::CirclesList& circles = envSpreader_.circles();
		const bool spread = params.spread && !circles.empty();
		for(size_t i = 0; i < total; ++i){
			const Vect2f pos = spread
				? Vect2f(center.x + circles[i].position.x, center.y + circles[i].position.y)
				: center;
			if(simple){
				cSimply3dx* m = envPreviewSimple_[i];
				m->SetScale(1.0f);
				m->SetScale(envScale(params));
				float radius = m->GetBoundRadius();
				if(!spread)
					radius = clamp(radius, 0.001f, min(float(vMap.H_SIZE) / 2.5f, float(vMap.V_SIZE) / 2.5f));
				m->SetPosition(envObjectPose(params, pos, radius));
			} else {
				cObject3dx* m = envPreviewObjs_[i];
				m->SetScale(1.0f);
				m->SetScale(envScale(params));
				float radius = m->GetBoundRadius();
				if(!spread)
					radius = clamp(radius, 0.001f, min(float(vMap.H_SIZE) / 2.5f, float(vMap.V_SIZE) / 2.5f));
				m->SetPosition(envObjectPose(params, pos, radius));
			}
		}
	}

	// CSurToolEnvironment::loadPreset: overlay the type's saved preset, if any
	// (Scripts\TreeControlSetups\EnvironmentPreset_<type>).
	void envLoadPreset(UnitEnvironment* unit)
	{
		if(!unit)
			return;
		XBuffer buf(256, 1);
		buf < "Scripts\\TreeControlSetups\\EnvironmentPreset_"
			< getEnumDescriptor(unit->environmentType()).name(unit->environmentType());
		XPrmIArchive ia;
		if(!ia.open(buf))
			return;
		std::string model = unit->modelName();
		float radius = unit->radius();
		Se3f pose = unit->pose();
		ia.serialize(*unit, "unit", 0);
		unit->setModel(model.c_str());
		unit->setRadius(radius);
		unit->setPose(pose, true);
		if(unit->rigidBody())
			unit->rigidBody()->awake();
	}

	// CSurToolEnvironment::onOperationOnMap single-unit branch.
	UnitEnvironment* envBuildUnit(Player* wp, const EnvironmentParams& params, const Vect2f& position)
	{
		EnvironmentType type = environmentTypeFromIndex(params.typeIndex);
		UnitBase* base = wp->buildUnit(AuxAttributeReference(
			isEnvironmentSimple(type) ? AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE : AUX_ATTRIBUTE_ENVIRONMENT));
		UnitEnvironment* unit = safe_cast<UnitEnvironment*>(base);
		if(!unit)
			return nullptr;
		unit->setEnvirontmentType(type);
		unit->setModel(params.model.c_str());
		float logicRadius = envScale(params) * unit->radius();
		unit->setRadius(logicRadius);
		unit->setPose(envObjectPose(params, position, logicRadius), true);
		unit->mapUpdate(unit->position2D().x - unit->radius(), unit->position2D().x + unit->radius(),
		                unit->position2D().y - unit->radius(), unit->position2D().y + unit->radius());
		envLoadPreset(unit);
		return unit;
	}

	// The spread branch skipped a circle if an environment already overlaps
	// (SurTool3DM::onOperationOnMap can_be_placed).
	bool envCanPlace(Player* wp, const Vect2f& pos, float radius)
	{
		const UnitList& units = wp->units();
		for(UnitList::const_iterator it = units.begin(); it != units.end(); ++it){
			UnitBase* base = *it;
			UnitEnvironment* unit = dynamic_cast<UnitEnvironment*>(base);
			if(unit && unit->environmentType() != ENVIRONMENT_PHANTOM){
				const Vect3f& t = unit->pose().trans();
				const Vect2f up(t.x, t.y);
				if((up - pos).norm() < (unit->radius() + radius) * 0.8f)
					return false;
			}
		}
		return true;
	}

	TriggerSession triggerSession_;

	// Scenario editor state (CMainFrame::OnEditMap / OnEditGameScenario):
	// the live mission (set by EngineViewport::loadWorld) and the exported
	// player/world-player copies the MapSerializer edits.
	MissionDescription* mission_ = nullptr;
	PlayerDataVect scenarioPlayers_;
	PlayerDataEdit scenarioWorldPlayer_;

	// UI Editor (UIEditor port): the engine viewport (for the lazy library
	// load) and the node-id → engine object cache uiTree rebuilds.
	EngineViewport* owner_ = nullptr;
	bool uiLibrariesInited_ = false;
	std::vector<UiNodeRef> uiNodes_;
	UI_Screen* uiCurrentScreen_ = nullptr;   // the screen uiTree is walking

	// Effects Editor (EffectEditor port): the loaded EffectKey, its file, the
	// node cache and the collected curve clones (kept alive while the tree
	// references them).
	EffectKey* effectKey_ = nullptr;
	std::string effectFileName_;
	std::vector<EmitterKeyInterface*> effectEmitters_;
	std::vector<ShareHandle<CurveWrapperBase> > effectCurveStore_;
	std::vector<EffectNodeRef> effectNodes_;

	// CSurToolSource / CSurToolAnchor: the live preview object that follows the
	// cursor (the original's sourceOnMouse_ / anchorOnMouse_). Owned by the
	// sourceManager once added; the -1 previewSource()/previewAnchor(false)
	// calls kill/remove them, and the editor clears them on tool change.
	SourceBase* previewSource_ = nullptr;
	Anchor* previewAnchor_ = nullptr;
	// CSurToolUnit's cursor preview unit (unitOnMouse_) and its RNG/seed.
	UnitBase* previewUnit_ = nullptr;
	int previewUnitLibrary_ = -1;
	RandomGenerator unitRandom_;
	int unitSeed_ = 1;
	// CSurToolAnchor's editable Anchor instance (anchor_): the property tree
	// edits this, and placeAnchor stamps a fresh copy of it. doNotRegister=true
	// so it stays out of sourceManager until a preview/place is asked for.
	Anchor editableAnchor_{ true };

	// CSurToolEnvironment's live preview models (visualObjects), the spread
	// layout they follow, and the RNG seeded per reload (random_/seed_).
	// Simple environments preview as cSimply3dx, buildings as cObject3dx —
	// the same kinds placement creates.
	std::vector<cObject3dx*> envPreviewObjs_;
	std::vector<cSimply3dx*> envPreviewSimple_;
	ObjectSpreader envSpreader_;
	RandomGenerator envRandom_;
	int envSeed_ = 12345;
	float envLastX_ = 0.f;
	float envLastY_ = 0.f;
};

EngineViewport::EngineViewport() = default;

EngineViewport::~EngineViewport()
{
	done();
}

bool EngineViewport::init(int width, int height)
{
	fprintf(stderr, "EngineViewport: [init] enter w=%d h=%d inited=%d\n", width, height, (int)inited_); fflush(stderr);
	if(inited_)
		return true;
	if(!nativeWindow_){
		fprintf(stderr, "EngineViewport: [init] no native window, abort\n"); fflush(stderr);
		return false;
	}

	// Universe's ctor (Game/Universe.cpp:185) calls GameOptions::environmentSetup
	// which reads setTranslate — that walks the localizations directory
	// ("Scripts\\Engine\\Translations\\<lang>") and ErrH.Abort's when the
	// directory is empty or missing the per-language folder. The original
	// SurMap5 (SurMap5/SurMap5.cpp:155) and Configurator/Configurator.cpp:45
	// both call this trio before any engine work. The editor needs the same
	// prelude so GameOptions can read its language list.
	TranslationManager::instance().setTranslationsDir("Scripts\\Engine\\Translations");
	TranslationManager::instance().setDefaultLanguage("english");

	// UI_Render::create() (SurMap5/SurMap5.cpp:152, before GameOptions and
	// any library load) instantiates the UI_Render singleton. loadAllLibraries
	// -> UI_GlobalAttributes::instance() serializes into UI_Render::instance(),
	// which xassert(self_)s until create() ran. Cheap (one static new), so
	// this stays in init() even though loadAllLibraries moved to loadWorld.
	UI_Render::create();
	TranslationManager::instance().setLanguage("english");

	// SDL_INIT_VIDEO is normally PlatformWindow::create's job; the Qt editor
	// has no SDL window, so initialize the subsystem the GPU device needs.
	if(!SDL_WasInit(SDL_INIT_VIDEO)){
		if(!SDL_Init(SDL_INIT_VIDEO)){
			fprintf(stderr, "EngineViewport: SDL_Init(VIDEO) failed: %s\n", SDL_GetError());
			return false;
		}
	}

	// The engine's one entry point into the renderer (Render/RenderStub.cpp):
	// builds gb_VisGeneric + the SDL GPU device.
	if(!gb_RenderDevice)
		CreateIRenderDevice(false);
	if(!gb_RenderDevice)
		return false;

	// With no global SDL window (Qt owns the windows), the device is created
	// without a swapchain; createRenderWindow() below wraps this viewport's
	// native handle and becomes the (global) render window.
	if(!gb_RenderDevice->inited() &&
	   !gb_RenderDevice->Initialize(width, height, RENDERDEVICE_MODE_WINDOW, nullptr, 0, nullptr))
		return false;

	// [SurMap5Qt] initRenderObjects (Game/RenderObjects.cpp:99-101) created the
	// engine's default UI font and set it on the device. The Qt port calls
	// initScene() but not initRenderObjects(), so DefaultFont/CurrentFont stay
	// null and every gb_RenderDevice->OutText is a no-op — that is why the
	// editor drew no red source/anchor labels (EditorVisual::drawText). Create
	// the same font here.
	if(!editorFont_){
		editorFont_ = FT::fontManager().createFont(default_font_name.c_str(),
			round(18.f / 768.f * float(gb_RenderDevice->GetSizeY())));
		if(editorFont_)
			gb_RenderDevice->SetDefaultFont(editorFont_);
	}

	// [SurMap5Qt] The shipped content is cache-only: world models exist as
	// CacheData\Models\*.3dxG and textures as CacheData\Textures\*, with no
	// raw Resource\TerrainData\Models\*.3dx beside them. The original editor
	// enabled both caches in CGeneralView::initRenderDevice through
	// initRenderObjects (Game/RenderObjects.cpp:70); the Qt port only calls
	// initScene(), so Option_UseMeshCache stays false and cLib3dx::GetElement
	// tries to open the missing raw .3dx — every environment/unit model fails,
	// the units Kill() themselves at load and never reach the Objects Manager.
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);

	// [SurMap5Qt] CSurMap5App::InitInstance set the effect texture root
	// (SurMap5/SurMap5.cpp:166) before any world load. Effect files in
	// Resource\FX name their frame textures relative to that root; without it
	// EffectContainer::getEffect passes no texture path, the library looks the
	// textures up as bare names, and the complex-texture path
	// (cTexLibrary::GetElement3DComplex) throws std::out_of_range on a name
	// that has no '\' separator. Must run before the first effect key loads
	// (unit setPose -> startPermanentEffects during .spg load).
	EffectContainer::setTexturesPath("Resource\\FX\\Textures");
	// [SurMap5Qt] initRenderObjects (Game/RenderObjects.cpp:72) also set the
	// VisGeneric effect library path (used by Node3DX/Static3DX model effects
	// via GetEffectTexturePath). The Qt port calls initScene(), not
	// initRenderObjects(), so set it here too — otherwise model-attached
	// effects resolve textures as bare names ("Texture is bad: 038a.tga").
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX", "RESOURCE\\FX\\TEXTURES");
	// [SurMap5Qt] Старый редактор (CGeneralView::initRenderDevice ->
	// initRenderObjects + GameOptions::gameSetup, затем surMapOptions.load()
	// -> serializeForEditor -> userApply) применял СОХРАНЁННЫЕ опции:
	// particle rate, soft smoke, тени, bump, анизотропия, гамма, отражения.
	// Без загрузки UserInterface.cfg Option_ остаются дефолтными
	// (тени SHADOW=2 с 2048 shadow map каждый кадр!) и поведение отличается
	// от старого редактора. Только GRAPHICS-подмножество и только graphSetup:
	// полный userApply/gameSetup тянет звук (InitSound), смену разрешения
	// (updateResolution) и RestoreDeviceForce, которых в Qt-окне быть
	// не должно; gameSetup зовёт UI-диспетчер, которого в редакторе нет.
	{
		XPrmIArchive ia;
		if(ia.open("UserInterface.cfg")){
			try{
				GameOptions::instance().serializeForEditor(
					ia, GameOptions::GRAPHICS | GameOptions::GAME | GameOptions::CAMERA);
				fprintf(stderr, "EngineViewport: [init] UserInterface.cfg options loaded\n");
			}
			catch(...){
				fprintf(stderr, "EngineViewport: [init] UserInterface.cfg load failed, defaults kept\n");
			}
		}
		else
			fprintf(stderr, "EngineViewport: [init] no UserInterface.cfg, defaults kept\n");
		fflush(stderr);
	}
	GameOptions::instance().graphSetup();

	// [SurMap5Qt] The rest of CSurMap5App::InitInstance's engine prelude
	// (SurMap5/SurMap5.cpp:145-167), in the same order: the pak archives, the
	// console listener, the FPU precision and the debug-symbol manager. These
	// are engine-side and must run before any world load (ZipConfig mounts the
	// .pak files the world assets live in).
	ZipConfig::initArchives();
	Console::instance().registerListener(&ConsoleWindow::instance());
	setLogicFp();
	DebugSymbolManager::create();

	renderWindow_ = gb_RenderDevice->createRenderWindow((HWND)nativeWindow_);
	if(!renderWindow_)
		return false;
	gb_RenderDevice->selectRenderWindow(renderWindow_);

	// initScene() (Game/RenderObjects.cpp) creates the engine-wide terScene
	// + cameraManager. The original SurMap5 called initRenderObjects /
	// initScene in CGeneralView::initRenderDevice + createScene; the Qt
	// editor keeps the render device + render window Qt-side, but borrows
	// terScene + cameraManager because the Game/Universe ctor attaches
	// tileMap and objects to terScene and reads the editor camera from
	// cameraManager->GetCamera(). Without this, Universe has nowhere to
	// put its tile map and the Objects Manager tabs have no unit/spline
	// containers to walk.
	if(!terScene)
		initScene();
	if(!terScene || !cameraManager)
		return false;
	scene_ = terScene;
	camera_ = cameraManager->GetCamera();
	if(!camera_)
		return false;

	applyCamera();
	// The tools' world bridge lives for the viewport's lifetime (the engine
	// globals it reads — universe/sourceManager/cameraManager/vMap — exist
	// from initScene on and are torn down in done).
	bridge_ = new (std::nothrow) WorldBridge(this);

	// The LibraryEditor core: register the builtin property-row types
	// (string/bool/numeric) so PropertyOArchive can build rows for them.
	// Idempotent; safe to call once at startup.
	editor::registerBuiltinPropertyRows();
	// The engine-typed rows (colors, combos, ranged wrappers, file
	// selectors) — the kdw in-place editors' data counterparts.
	editor::registerEnginePropertyRows();

	inited_ = true;
	fprintf(stderr, "EngineViewport: [init] SUCCESS\n"); fflush(stderr);
	return true;
}

void EngineViewport::done()
{
	if(!inited_)
		return;
	doneWorld();
	if(gb_RenderDevice){
		gb_RenderDevice->selectRenderWindow(0);
		if(renderWindow_)
			gb_RenderDevice->DeleteRenderWindow(renderWindow_);
		// Release the editor's UI font the way finitRenderObjects did
		// (Game/RenderObjects.cpp:141-143).
		if(editorFont_){
			gb_RenderDevice->SetDefaultFont(NULL);
			gb_RenderDevice->SetFont(NULL);
			FT::fontManager().releaseFont(editorFont_);
			editorFont_ = nullptr;
		}
	}
	// terScene and cameraManager are owned by initScene/finitScene — the
	// editor keeps them across loads so reloading a world reuses the
	// same scene + camera. (SurMap5/GeneralView.cpp called createScene
	// once and reInitWorld on every world load.) Drop the references;
	// the teardown order matches SurMap5/VistaEngineContext.cpp.
	stopEffectPreview();
	delete bridge_;
	bridge_ = nullptr;
	effectPreview_ = nullptr;   // the scene owns it; tear-down drops it with scene_
	scene_ = nullptr;
	camera_ = nullptr;
	renderWindow_ = nullptr;
	inited_ = false;
}

bool EngineViewport::loadWorld(const char* worldsDir, const char* worldName)
{
	fprintf(stderr, "EngineViewport: [loadWorld] enter dir=%s name=%s\n", worldsDir, worldName); fflush(stderr);
	if(!inited_ || !scene_){
		fprintf(stderr, "EngineViewport: [loadWorld] not inited or no scene, abort\n"); fflush(stderr);
		return false;
	}

	doneWorld();
	fprintf(stderr, "EngineViewport: [loadWorld] doneWorld() ok, calling vMap.load\n"); fflush(stderr);

	// CMainFrame::OnFileOpen: vMap.load(world, true) + reInitWorld. vMap::load
	// reads world.cls from <worldsDir>\<worldName>\ and builds the terrain
	// buffers.
	vMap.setWorldsDir(worldsDir);
	if(!vMap.load(worldName, /*flag_useTryColorBuffer=*/true)){
		fprintf(stderr, "EngineViewport: [loadWorld] vMap.load FAILED\n"); fflush(stderr);
		return false;
	}
	fprintf(stderr, "EngineViewport: [loadWorld] vMap.load ok, creating Universe\n"); fflush(stderr);

	// CGeneralView::reInitWorld (SurMap5/GeneralView.cpp:227) builds the
	// game state on top of vMap: MissionDescription (loaded from
	// <worldsDir>\<worldName>.spg if present, else empty), then
	// `new Universe(mission, ia)`. Universe owns the terScene's tile map,
	// the sourceManager, the cameraManager's splines and the Players
	// (Units live there). The Objects Manager walks all of these.
	//
	// loadAllLibraries() (Units/UnitAttribute.cpp) — the SurMap5
	// initRenderDevice prelude (GeneralView.cpp:135), run once before any
	// world load. Universe::setActivePlayer constructs the UI_Dispatcher
	// singleton on first use; its ctor deserializes Scripts\Content\
	// UI_Attributes, which references the UI_* libraries that
	// loadAllLibraries loads first. Without the prelude that
	// deserialization faults. Deliberately NOT in init(): init() runs from
	// RenderViewWidget::paintEvent, and a heavy library load there dies
	// with STATUS_FATAL_USER_CALLBACK_EXCEPTION; loadWorld runs from a
	// normal event (File > Open, restore timer), the same context the
	// original MFC editor used.
	//
	// VISTA_EDITOR_NO_UNIVERSE skips the Universe build: the Qt editor
	// can still show the terrain and the user can pick a world to open
	// without the recursive ParameterValue in Units/Parameters.cpp:282
	// killing the editor on a .prm file with a cyclic formula. The
	// Objects Manager falls back to sourceManager-only data (Sources,
	// Anchors).
	if(!getenv("VISTA_EDITOR_NO_UNIVERSE")){
		if(!librariesLoaded_){
			fprintf(stderr, "EngineViewport: [loadWorld] loadAllLibraries...\n"); fflush(stderr);
			try{
				loadAllLibraries();
				librariesLoaded_ = true;
				fprintf(stderr, "EngineViewport: [loadWorld] loadAllLibraries done\n"); fflush(stderr);
			}
			catch(const std::exception& e){
				fprintf(stderr, "EngineViewport: [loadWorld] loadAllLibraries threw: %s\n", e.what()); fflush(stderr);
			}
			catch(...){
				fprintf(stderr, "EngineViewport: [loadWorld] loadAllLibraries threw (non-std)\n"); fflush(stderr);
			}
		}
		MissionDescription* mission = new MissionDescription();
		mission->setByWorldName(worldName);

		std::string spgPath = std::string(worldsDir) + "\\" + worldName + ".spg";
		XPrmIArchive ia;
		const bool haveMission = ia.open(spgPath.c_str());
		if(haveMission){
			*mission = MissionDescription(spgPath.c_str());
		}

		// The editor's Universe: an empty mission (no player buildings, no
		// mission units) is enough to populate sourceManager, the camera
		// splines saved in <world>.spg.bin and the world's anchor list.
		// Environment ctor runs without a mission's water/fog/temperature
		// flags and falls back to defaults — the Qt editor never invokes
		// Environment::graphQuant, so its render state does not need to
		// match the original Game's. The recursive ParameterValue cycle is
		// contained in Units/Parameters.cpp (thread-local depth cap).
		Universe* uni = new Universe(*mission, haveMission ? &ia : 0);
		ownedMission_.reset(mission);
		if(bridge_)
			static_cast<WorldBridge*>(bridge_)->setMission(mission);
		ownedUniverse_.reset(uni);
		if(!isUnderEditor())
			uni->relaxLoading();

		// [SurMap5Qt] Single-threaded editor: run logic and graphics on one
		// thread (tick's Quant + drawFrame's render both on the GUI thread).
		// The original SurMap5 did the same (SurMap5/GeneralView.cpp:238,
		// straight after `new Universe`) — useHT_ arms a busy-wait in
		// Universe::clearDeletedUnits for the graph thread to catch up, which
		// on one thread would spin forever the moment a unit is deleted.
		uni->setUseHT(false);

		// [SurMap5Qt] Ship the pose/visibility commands the Universe ctor queued.
		// Unit setPose(initPose=true) during .spg deserialization wrote into the
		// global streamLogicCommand / streamLogicInterpolator (RealUnit.cpp:368);
		// they only reach the graph objects after interpolationQuant moves them
		// into universe()->streamCommand/streamInterpolator and drawFrame's
		// process() applies them. Without this the models keep their creation
		// pose (the origin) and the units are invisible over the terrain. The
		// original CMainFrame::universeQuant ran Quant + interpolationQuant on
		// every logic tick; a single call after load covers the static editor
		// world (drawFrame keeps draining the universe streams each frame).
		uni->interpolationQuant();

		// [SurMap5Qt] The editor shows every effect regardless of distance. The
		// game's Environment serializes effectHideByDistance_=true with a
		// near/far range (Environment.cpp:112-114, 50..1200) and cEffect::PreDraw
		// then scales GetParticleRateReal by distance_rate = 1-(d-near)/(far-near),
		// which hits 0 past far_distance — so any effect further than ~1200 world
		// units from the camera emits nothing. An editor camera routinely sits
		// thousands of units out (the orbit view), so every unit's fountains,
		// beams and glows silently stop emitting. The original editor had no such
		// LOD: disable the distance check, exactly as HIDE_BY_DISTANCE is cleared
		// per-frame above. Environment::serialize re-arms it on every .spg load,
		// so this must run after the Universe ctor (which loads the environment).
		cEffect::setVisibleRange(false, 0.0f, 0.0f);
	}

	// reInitWorld's camera reset: centre the orbit on the map, then let
	// the camera keep its height (createScene used the map centre too).
	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 256.0f;
	orbit_.distance = 20000.f;
	orbit_.psi = 0.f;
	orbit_.theta = 0.f;
	applyCamera();
	const Vect2f center(0.5f, 0.5f);
	const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
	const Vect2f focus(orbit_.focus, orbit_.focus);
	float zMin = 0.f, zMax = 0.f; editorZPlane(zMin, zMax);
	const Vect2f zPlane(zMin, zMax);
	camera_->SetFrustum(&center, &clip, &focus, &zPlane);
	Vect3f rayPoint, rayDirection;
	camera_->GetWorldRay(Vect2f(0.f, 0.f), rayPoint, rayDirection);
	const Vect3f eye = camera_->GetPos();
	fprintf(stderr, "EngineViewport: loaded camera eye=(%.1f,%.1f,%.1f) center=(%.1f,%.1f,%.1f) ray=(%.3f,%.3f,%.3f)\n",
	        eye.x, eye.y, eye.z, orbit_.px, orbit_.py, orbit_.pz,
	        rayDirection.x, rayDirection.y, rayDirection.z);

	worldLoaded_ = true;
	fprintf(stderr, "EngineViewport: [loadWorld] SUCCESS world=%s\n", worldName); fflush(stderr);
	return true;
}

void EngineViewport::doneWorld()
{
	fprintf(stderr, "EngineViewport: [doneWorld] enter worldLoaded=%d\n", (int)worldLoaded_); fflush(stderr);
	if(!worldLoaded_){
		fprintf(stderr, "EngineViewport: [doneWorld] nothing to do\n"); fflush(stderr);
		return;
	}

	// The tools' cursor previews (a scene model / an auxiliary unit) reference
	// the world being torn down; drop them before the scene and universe go.
	if(bridge_){
		bridge_->killEnvironmentPreview();
		bridge_->killPreviewUnit();
	}

	// Universe dtor releases the tile map (RELEASE(tileMap)) and clears
	// the engine globals (sourceManager, environment, cameraManager
	// splines, Players, pathFinder, soundEnvironmentManager_).
	if(ownedUniverse_){
		ownedUniverse_.reset();
	}
	if(bridge_)
		static_cast<WorldBridge*>(bridge_)->setMission(nullptr);
	ownedMission_.reset();

	// [SurMap5Qt] Rebuild the scene between worlds. The original editor's
	// OnFileOpen called CGeneralView::createScene() before every world load
	// (SurMap5/MainFrame.cpp:1029), and createScene is doneScene() +
	// initScene() — the whole terScene/cameraManager pair is torn down and
	// re-created, so the second world's Universe starts from an empty scene.
	// The Qt port kept terScene alive across loads, so every re-loaded world
	// piled a second cTileMap (assert "tileMap_==0" in cScene::CreateMap), a
	// second cWater, grassMap, cloudShadow ... over the first world's corpses
	// — and the first world's cWater, freed with its Environment, left the
	// global `water` pointer null while the new world's units were already
	// quantsing effects against it (EffectController.cpp:298 -> water->isLava
	// on null). Delete and re-create the scene the same way the original
	// editor did.
	finitScene();
	initScene();

	scene_ = terScene;
	camera_ = cameraManager ? cameraManager->GetCamera() : nullptr;

	vMap.releaseWorld();
	worldLoaded_ = false;
	applyCamera();
	fprintf(stderr, "EngineViewport: [doneWorld] done (scene rebuilt)\n"); fflush(stderr);
}

bool EngineViewport::createWorld(const char* worldsDir, const char* worldName)
{
	if(!inited_ || !scene_)
		return false;

	doneWorld();

	// CMainFrame::OnFileNew: vMap.create(name) makes a default-size world (the
	// original let the user pick size/relief via WorldCreationParam; the
	// default vrtMapCreationParam is 256x256 flat — vMap::create uses it).
	vMap.setWorldsDir(worldsDir);
	vMap.create(worldName);
	if(!vMap.isWorldLoaded())
		return false;

	// vMap.create builds the world in memory only; persist it so Open World
	// (vMap.load) finds it on the next run. vMap.save creates the world's
	// directory but not the worlds dir itself — CreateDirectory is not
	// recursive, so make both first (CMainFrame's OnFileNew did the same via
	// CreateDirectory before the world existed).
	::CreateDirectory(worldsDir, 0);
	std::string worldDir = worldsDir;
	worldDir += "\\";
	worldDir += worldName;
	::CreateDirectory(worldDir.c_str(), 0);

	vMap.save(worldName);

	// Same Universe bootstrap as loadWorld, but without an .spg: a fresh
	// world has no mission script, so the Universe runs with an empty
	// MissionDescription and ia = 0 (the Universe ctor's `if(!ia)
	// environment->loadPreset()` branch — no .spg = no world objects).
	// VISTA_EDITOR_NO_UNIVERSE skips this, see loadWorld.
	if(!getenv("VISTA_EDITOR_NO_UNIVERSE")){
		if(!librariesLoaded_){
			fprintf(stderr, "EngineViewport: [createWorld] loadAllLibraries...\n"); fflush(stderr);
			try{
				loadAllLibraries();
				librariesLoaded_ = true;
				fprintf(stderr, "EngineViewport: [createWorld] loadAllLibraries done\n"); fflush(stderr);
			}
			catch(const std::exception& e){
				fprintf(stderr, "EngineViewport: [createWorld] loadAllLibraries threw: %s\n", e.what()); fflush(stderr);
			}
			catch(...){
				fprintf(stderr, "EngineViewport: [createWorld] loadAllLibraries threw (non-std)\n"); fflush(stderr);
			}
		}
		MissionDescription* mission = new MissionDescription();
		mission->setByWorldName(worldName);
		Universe* uni = new Universe(*mission, 0);
		ownedMission_.reset(mission);
		if(bridge_)
			static_cast<WorldBridge*>(bridge_)->setMission(mission);
		ownedUniverse_.reset(uni);
		if(!isUnderEditor())
			uni->relaxLoading();
		// [SurMap5Qt] Single-threaded editor, as in loadWorld — see there.
		uni->setUseHT(false);
	}

	// Same terrain setup as loadWorld.
	scene_->CreateMap(true);

	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 128.0f;
	orbit_.distance = 5000.f;
	orbit_.psi = 0.f;
	orbit_.theta = 0.f;
	applyCamera();

	worldLoaded_ = true;
	return true;
}

bool EngineViewport::ensureUiLibraries()
{
	if(librariesLoaded_)
		return true;
	// The same prelude loadWorld runs before `new Universe`: the UI_* +
	// attribute libraries. UIEditor did this at startup; here the UI editor
	// runs it on first use.
	try{
		loadAllLibraries();
		librariesLoaded_ = true;
	}
	catch(const std::exception& e){
		fprintf(stderr, "EngineViewport: [ensureUiLibraries] loadAllLibraries threw: %s\n", e.what());
		return false;
	}
	catch(...){
		fprintf(stderr, "EngineViewport: [ensureUiLibraries] loadAllLibraries threw (non-std)\n");
		return false;
	}
	return true;
}

bool EngineViewport::startEffectPreview(EffectKey* effectKey)
{
	stopEffectPreview();
	if(!effectKey || !scene_ || !gb_RenderDevice)
		return false;
	// EffectDocument::createEffect: a detached effect attached to the scene,
	// positioned at the map centre (origin when no world is loaded) and with
	// every emitter visible.
	cEffect* effect = scene_->CreateEffectDetached(*effectKey, 0, false);
	if(!effect)
		return false;
	effect->Attach();
	// Place it at the camera's orbit centre so the main 3D view is looking at
	// it (the map centre is off-screen whenever the user has panned).
	const float x = orbit_.px;
	const float y = orbit_.py;
	const float z = orbit_.pz + 50.f;
	effect->SetPosition(MatXf(Se3f(QuatF::ID, Vect3f(x, y, z))));
	for(size_t i = 0; i < effectKey->emitterKeys.size(); ++i){
		if(EmitterKeyInterface* emitter = effectKey->emitterKeys[i].get())
			effect->ShowEmitter(emitter, true);
	}
	effect->SetTime(0.f);
	effect->MoveToTime(effectPreviewTime_);
	// EffectDocument::quant set the particle rate every frame; without it the
	// detached effect emits nothing.
	effect->SetParticleRate(1.0f);
	effectPreview_ = effect;
	return true;
}

void EngineViewport::stopEffectPreview()
{
	if(effectPreview_){
		RELEASE(effectPreview_);   // detached effects are RELEASE()d (EffectDocument::createEffect)
		effectPreview_ = nullptr;
	}
}

bool EngineViewport::setEffectPreviewTime(float time)
{
	effectPreviewTime_ = time;
	if(effectPreview_)
		effectPreview_->MoveToTime(time);
	return effectPreview_ != nullptr;
}

bool EngineViewport::startUiPreview(UI_Screen* screen)
{
	if(!gb_RenderDevice)
		return false;
	try{
		if(!uiPreviewInited_){
			// The UIEditor prelude: UI_Render::init + UI_Dispatcher::init
			// (UI_LogicDispatcher::init + UI_BackgroundScene::init).
			UI_Render::instance().init();
			UI_Dispatcher::instance().init();
			uiPreviewInited_ = true;
		}
		if(screen){
			// Do NOT go through UI_Dispatcher::selectScreen: it runs the
			// screen's logic init/activate, which dereferences game state the
			// editor has none of and crashes. A preview only needs the graph
			// side (preLoad loads the sprites the screen draws with).
			screen->preLoad();
		}
		uiPreviewScreen_ = screen;
		const int w = gb_RenderDevice->GetSizeX();
		const int h = gb_RenderDevice->GetSizeY();
		UI_Render::instance().setWindowPosition(Recti(0, 0, w, h));
		UI_Render::instance().updateRenderSize();
		uiPreview_ = true;
	}
	catch(const std::exception& e){
		fprintf(stderr, "EngineViewport: [startUiPreview] threw: %s\n", e.what());
		return false;
	}
	catch(...){
		fprintf(stderr, "EngineViewport: [startUiPreview] threw (non-std)\n");
		return false;
	}
	return true;
}

void EngineViewport::stopUiPreview()
{
	uiPreview_ = false;
	uiPreviewScreen_ = nullptr;
}

bool EngineViewport::saveWorld(const char* worldName)
{
	if(!worldLoaded_ || !worldName)
		return false;
	// CMainFrame::save (SurMap5/MainFrame.cpp:1076): vMap.save(worldName) —
	// writes world.cls + the caches into <worldsDir>\<worldName>\ (vMap::save
	// creates the directory if missing; the worlds dir itself must exist —
	// createWorld/loadWorld made it).
	vMap.save(worldName);
	return true;
}

bool EngineViewport::saveWorld()
{
	// OnFileSave: vMap.getWorldName() is the name passed to load/create.
	return saveWorld(vMap.getWorldName().c_str());
}

bool EngineViewport::canUndo() const
{
	return worldLoaded_ && vMap.UndoDispatcher_IsUndoExist();
}

bool EngineViewport::canRedo() const
{
	return worldLoaded_ && vMap.UndoDispatcher_IsRedoExist();
}

bool EngineViewport::undo()
{
	if(!canUndo())
		return false;
	vMap.UndoDispatcher_Undo();
	return true;
}

bool EngineViewport::redo()
{
	if(!canRedo())
		return false;
	vMap.UndoDispatcher_Redo();
	return true;
}

bool EngineViewport::autoLace(int laceHeightVoxels, float angleRadians)
{
	// CMainFrame::OnEditRollingborder (SurMap5/MainFrame.cpp:2844):
	// vMap.autoLace(borderHeight*VOXEL_MULTIPLIER, boderAngle*M_PI/180.).
	if(!worldLoaded_)
		return false;
	vMap.autoLace(laceHeightVoxels, angleRadians);
	return true;
}

bool EngineViewport::rebuildWorld()
{
	// CMainFrame::OnEditRebuildworld (SurMap5/MainFrame.cpp:1564):
	// vMap.rebuild() + view_->reInitWorld(). Rebuild re-derives the terrain
	// from the source raster; reInitWorld rebuilds the render-side tile map.
	if(!worldLoaded_)
		return false;
	vMap.rebuild();
	reinitWorld();
	return true;
}

bool EngineViewport::updateSurface()
{
	// CGeneralView::updateSurface (SurMap5/GeneralView.cpp:942):
	// vMap.recalcArea2Grid(0,0,H-1,V-1) + regRender(whole map, Height|Texture|
	// Region) — the Update Surface menu command.
	if(!worldLoaded_)
		return false;
	vMap.recalcArea2Grid(0, 0, vMap.H_SIZE - 1, vMap.V_SIZE - 1);
	const char typeChanges = static_cast<char>(vrtMap::TypeCh_Height |
	                                           vrtMap::TypeCh_Texture |
	                                           vrtMap::TypeCh_Region);
	vMap.regRender(0, 0, vMap.H_SIZE - 1, vMap.V_SIZE - 1, typeChanges);
	return true;
}

int EngineViewport::toggleTryColorDamTexture()
{
	// OnDebugShowpalettetexture (SurMap5/MainFrame.cpp:2006): flip the
	// try-color dam texture view and re-render the world.
	if(!worldLoaded_)
		return -1;
	const bool show = !vMap.isShowTryColorDamTexture();
	vMap.toShowTryColorDamTexture(show);
	vMap.WorldRender();
	return show ? 1 : 0;
}

void EngineViewport::orbitCamera(float& distance, float& theta) const
{
	// GlobalAttributes::setCameraCoordinate persisted only distance + theta;
	// the orbit centre stays the map centre (reInitWorld resets it there).
	distance = orbit_.distance;
	theta = orbit_.theta;
}

void EngineViewport::setOrbitCamera(float distance, float theta)
{
	// The inverse of orbitCamera: restore the persisted default onto the
	// orbit. The centre is not saved (the original's camera create re-centred
	// on the map), so it stays where reInitWorld put it.
	orbit_.distance = distance;
	orbit_.theta = theta;
	applyCamera();
}

void EngineViewport::cameraState(CameraState& state) const
{
	state.centerX = orbit_.px;
	state.centerY = orbit_.py;
	state.centerZ = orbit_.pz;
	state.distance = orbit_.distance;
	state.yaw = orbit_.psi;
	state.pitch = orbit_.theta;
	state.roll = orbit_.fi;
}

void EngineViewport::setCameraState(const CameraState& state)
{
	orbit_.px = state.centerX;
	orbit_.py = state.centerY;
	orbit_.pz = state.centerZ;
	orbit_.distance = max(state.distance, 2.0f);
	orbit_.psi = state.yaw;
	orbit_.theta = state.pitch;
	orbit_.fi = state.roll;
	applyCamera();
}

void EngineViewport::fitCameraToWorld()
{
	if(!worldLoaded_)
		return;

	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 256.0f;
	const float halfDiagonal = 0.5f * std::sqrt(vMap.H_SIZE * vMap.H_SIZE +
	                                             vMap.V_SIZE * vMap.V_SIZE);
	orbit_.distance = std::max(20000.0f, halfDiagonal * 8.0f);
	orbit_.psi = -0.785398163f;
	orbit_.theta = 0.65f;
	orbit_.fi = 0.0f;
	applyCamera();
}

void EngineViewport::resetEditorCamera()
{
	if(!worldLoaded_)
		return;

	// CGeneralView::createScene: psi 90 deg, theta 0, distance 512, centre of
	// the map at 128. The Qt orbit model maps psi/theta onto a sphere around the
	// centre; a pure overhead (theta 0) would still be beyond the 500 hide
	// distance, so keep a working editor tilt (~30 deg) — close enough to the
	// original's low default that HIDE_BY_DISTANCE units stay visible.
	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 256.0f;
	orbit_.distance = 700.f;
	orbit_.psi = 0.785398163f;      // 45 deg, a quarter turn from the +X axis
	orbit_.theta = 0.5f;            // ~29 deg tilt from vertical
	orbit_.fi = 0.0f;
	applyCamera();
}

void EngineViewport::setGridVisible(bool visible)
{
	// OnViewShowGrid toggled surMapOptions.enableGrid_, which drawGrid
	// (GeneralView.cpp:915) read. The engine-side flag gates the same call.
	gridVisible_ = visible;
}

// drawGrid — CGeneralView::drawGrid (GeneralView.cpp): the editor grid over
// the terrain, vMap-sized, at the ground level.
void EngineViewport::drawGrid()
{
	if(!worldLoaded_)
		return;
	if(!gb_RenderDevice || !camera_)
		return;
	if(!gridVisible_)
		return;

	const int gridStep = 64;
	const int segmentLength = 16;
	// The editor's grid colour (surMapOptions.gridColor_ default).
	const Color4c color(96, 96, 96, 255);

	// drawLineTerrain drew a segmented line following the terrain height.
	// Simplified: a line strip at the terrain height read from vMap.
	// (TODO(sdl-port): full terrain-following grid once the editor options
	// land; this is the flat-map approximation that keeps Phase 3b simple.)
	for(int x = 0; x <= (int)vMap.H_SIZE; x += gridStep){
		for(int y = 0; y < (int)vMap.V_SIZE; y += segmentLength){
			const Vect3f a(x, y, vMap.getZf(x, y));
			const Vect3f b(x, y + segmentLength, vMap.getZf(x, y + segmentLength));
			gb_RenderDevice->DrawLine(a, b, color);
		}
	}
	for(int y = 0; y <= (int)vMap.V_SIZE; y += gridStep){
		for(int x = 0; x < (int)vMap.H_SIZE; x += segmentLength){
			const Vect3f a(x, y, vMap.getZf(x, y));
			const Vect3f b(x + segmentLength, y, vMap.getZf(x + segmentLength, y));
			gb_RenderDevice->DrawLine(a, b, color);
		}
	}
}

void EngineViewport::resize()
{
	if(renderWindow_)
		renderWindow_->ChangeSize();
}

void EngineViewport::tick(float dt)
{
	// The orbit state is applied immediately on input (like CGeneralView's
	// WindowProc, which sets cameraManager->setCoordinate per event), so there
	// is nothing to integrate per-frame yet. Phase 3b (world + animation) will
	// advance the scene here.
	applyCamera();

	// CMainFrame::universeQuant (SurMap5/MainFrame.cpp:684) ran the world's
	// logic at the logic time period: when its syncroTimer ticked it called
	// universe()->Quant() + interpolationQuant(). Quant walks the players and
	// quants every unit (RealUnit::quant -> chainControllers_.quant — the
	// skeletal animation phases, turret turns, effects), advances the
	// environment (environment->logicQuant: water AnimateLogic, the time of
	// day) and the source manager. Without it the units hold whatever pose the
	// load left them in and nothing animates. In the editor the same clock
	// applies: logicTimePeriod ms of real time accumulate, then one Quant +
	// interpolationQuant ships the new poses into streamCommand, which
	// drawFrame's drain applies to the graph models next frame.
	if(ownedUniverse_ && environment){
		const double now = (double)xclock();
		if(lastLogicMs_ == 0.0)
			lastLogicMs_ = now;   // first tick: start the clock, don't burst
		logicAccumMs_ += now - lastLogicMs_;
		lastLogicMs_ = now;
		if(logicAccumMs_ >= logicTimePeriod){
			logicAccumMs_ = 0.0;
			// Как в CMainFrame::universeQuant (SurMap5/MainFrame.cpp:684):
			// Console::quant до Quant, Console::graphQuant после.
			// UI_Dispatcher::logicQuant здесь нет: в редакторе gameShell==0,
			// и он падает на UI_LogicDispatcher::isGameActive()->gameShell.
			Console::instance().quant();
			gb_VisGeneric->SetLogicQuant(universe()->quantCounter() + 2);
			universe()->Quant();
			universe()->interpolationQuant();
			Console::instance().graphQuant();
		}
	}
}

void EngineViewport::drawFrame()
{
	if(!inited_ || !gb_RenderDevice || !camera_)
		return;

	// CGeneralView::OnPaint (SurMap5/GeneralView.cpp:385) measured the real time
	// between frames (capped at 100 ms) and fed it to Animate() ->
	// terScene->SetDeltaTime before graphQuant. That delta is what cScene::Draw's
	// Animate() steps the world's animations with (water waves, cloud drift,
	// texture phases), so a static editor still moves. dt is in milliseconds
	// here — SetDeltaTime's unit (cScene::SetDeltaTime "в миллисекундах"),
	// and GetDeltaTime returns it as-is.
	const double frameMs = [this]{
		static double s_prevMs = 0.0;
		const double now = (double)xclock();
		double dt = (s_prevMs > 0.0) ? (now - s_prevMs) : 0.0;
		s_prevMs = now;
		return std::min(100.0, dt);
	}();
	if(scene_)
		scene_->SetDeltaTime((float)frameMs);

	// CGeneralView::graphQuant (SurMap5/GeneralView.cpp:265) drained the
	// universe's command streams before drawing. Units don't move their model
	// directly: setPose writes a command into streamLogicCommand (or the
	// interpolator), Universe::interpolationQuant moves it into
	// universe()->streamCommand / streamInterpolator, and only process() here
	// applies it to the graph objects (fCommandSetPose -> BaseGraphObject::
	// SetPosition, fSe3fInterpolation -> interpolate). Without this drain the
	// models stay at the pose they were created with — the origin — no matter
	// where the unit logically stands. streamCommand carries the pose/visibility
	// snap commands, streamInterpolator the (factor 0 = snapped) motion.
	//
	// The graph quant counter must advance here too: Universe::clearDeletedUnits
	// (reached from Quant in tick) busy-waits while
	// GetGraphLogicQuant() < quantCounter() - 8 when useHT_ is on, and only
	// SetGraphLogicQuant releases it. The original set it in graphQuant
	// (SurMap5/GeneralView.cpp:277, GameShell.cpp:560); without it the editor's
	// first Quant hangs forever the moment a unit is queued for deletion.
	if(ownedUniverse_){
		gb_VisGeneric->SetGraphLogicQuant(universe()->quantCounter());
		universe()->streamCommand.process(0.0f);
		universe()->streamCommand.clear();
		universe()->streamInterpolator.process(0.0f);
		universe()->streamInterpolator.clear();

		// [SurMap5Qt] The editor shows every object regardless of distance. The
		// game sets hideDistance=500 (Units/GlobalAttributes.cpp, Render/3dx/
		// Node3DX.cpp:264) and flags models ATTRUNKOBJ_HIDE_BY_DISTANCE when the
		// unit attribute wants it; cObject3dx::PreDraw then culls anything whose
		// view-space depth exceeds hideDistance. An editor camera routinely sits
		// further than that from a unit (even 700 away), so the world looks
		// empty. The original editor got away with it only because its camera
		// stayed close to the action; the Qt editor's orbit view does not. Clear
		// the attribute and raise the per-model limit every frame — cheap walk,
		// and the editor has no reason to honour the game's LOD hide.
		//
		// Walk every unit, auxiliary and dead included: decoration (trees,
		// vegetation, props) is often auxiliary() on the world player, and the
		// units killed at load (missing effects etc.) still hold models worth
		// seeing in an editor. A dead unit's model is hidden by HIDE_BY_EDITOR
		// elsewhere; hide-by-distance must not be the reason a whole class of
		// objects vanishes at the editor's typical viewing range.
		const float kEditorHideDistance = 1e7f;
		PlayerVect::const_iterator pi;
		FOR_EACH(universe()->Players, pi){
			const UnitList& units = (*pi)->units();
			UnitList::const_iterator it;
			FOR_EACH(units, it){
				UnitBase* u = *it;
				if(!u)
					continue;
				if(cObject3dx* m = dynamic_cast<cObject3dx*>(u->model())){
					if(m->getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE)){
						m->clearAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
						m->SetHideDistance(kEditorHideDistance);
					}
					// [SurMap5Qt] The game's UnitReal::dayQuant flags models
					// ATTR3DX_HIDE_LIGHTS by day (RealUnit.cpp:318) — the day-time
					// look hides each model's glow sprites; cObject3dx::Update then
					// puts ATTRLIGHT_IGNORE on every sprite light of the model
					// (Node3DX.cpp:608) and UnkLight::PreDraw drops them all. The
					// editor has no day/night cycle driving it, the Environment runs
					// on its defaults, and the result is that no unit glows at all —
					// what the original editor showed. The editor shows the lights:
					// clear the flag every frame, like HIDE_BY_DISTANCE above.
					if(m->getAttribute(ATTR3DX_HIDE_LIGHTS))
						m->clearAttribute(ATTR3DX_HIDE_LIGHTS);
				}
				// UnitEnvironmentSimple keeps its model in modelSimple_ (a
				// cSimply3dx), not the virtual model() slot.
				if(UnitEnvironmentSimple* es = dynamic_cast<UnitEnvironmentSimple*>(u)){
					if(cSimply3dx* ms = es->modelSimple()){
						if(ms->getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE)){
							ms->clearAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
							ms->SetHideDistance(kEditorHideDistance);
						}
					}
				}
			}
		}

		// Not every world object lives in a player's unit list: decoration that
		// the .spg attaches straight to the scene grid (some vegetation, props)
		// never passes through the loop above. Sweep the scene's own 3dx
		// objects for the same hide-by-distance flag so nothing stays invisible
		// purely because the editor camera is further than the game's 500.
		{
			std::vector<cObject3dx*> sceneObjs;
			scene_->GetAllObject3dx(sceneObjs);
			for(size_t i = 0; i < sceneObjs.size(); ++i){
				cObject3dx* m = sceneObjs[i];
				if(!m)
					continue;
				if(m->getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE)){
					m->clearAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
					m->SetHideDistance(kEditorHideDistance);
				}
			}
		}

		// The same for the scene's cSimply3dx objects (buildings, vegetation,
		// props that render through cStaticSimply3dx). They are culled by
		// cSimply3dx::CalcDistanceAlpha against hideDistance (default 500) in
		// cStaticSimply3dx::PreDraw — a separate path from cObject3dx::PreDraw,
		// so the cObject3dx sweep above never reaches them. An editor camera
		// routinely sits further than 500 from the centre of the world, so
		// without this every simply-3dx object in the middle of the map stays
		// invisible while the cObject3dx ones at the edges show.
		{
			vector<ListSimply3dx>& lists = scene_->GetAllSimply3dxList();
			for(size_t li = 0; li < lists.size(); ++li){
				vector<cSimply3dx*>& objs = lists[li].objects;
				for(size_t oi = 0; oi < objs.size(); ++oi){
					cSimply3dx* ms = objs[oi];
					if(!ms)
						continue;
					if(ms->getAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE)){
						ms->clearAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
						ms->SetHideDistance(kEditorHideDistance);
					}
				}
			}
		}

		}

	// The editor viewport's clear. CGeneralView used the environment's fone
	// colour for the clear so the horizon behind the world and the water's
	// reflected-sky tint match the time of day; with no world loaded (no
	// Environment yet), a neutral slate.
	if(environment)
	{
		Color4c fone = environment->environmentTime()->GetCurFoneColor();
		gb_RenderDevice->Fill(fone.r, fone.g, fone.b, 255);
	}
	else
		gb_RenderDevice->Fill(32, 48, 64, 255);
	gb_RenderDevice->BeginScene();

	// [SurMap5Qt] Как в CGeneralView::graphQuant (SurMap5/GeneralView.cpp:293):
	// editorVisual().beforeQuant() до отрисовки мира.
	editorVisual().beforeQuant();

	// [SurMap5Qt] Старого вызова environmentTime()->Draw() здесь нет,
	// хотя CGeneralView::graphQuant его делал: это D3D-only путь
	// (cRenderCubemap::Draw -> gb_RenderDevice3D->SetRenderState, девайса
	// нет в SDL-порте — вылет). См. TODO(sdl-port) над
	// EnvironmentTime::Draw и Documents/Render-PORTING.md #9: у кубмапа
	// не было потребителя даже на D3D. Небо рисуется через DrawEnviroment
	// внутри environment->graphQuant ниже.

	// CGeneralView::graphQuant called environment->graphQuant(dt, camera) right
	// after BeginScene and before terScene->Draw: it opens the frame with the
	// sky (EnvironmentTime::DrawEnviroment -> cSkyObj::DrawSkyAndAnimate — the
	// sun/moon and cloud models in their own sky scene), sets the distance fog
	// plane from the time of day, feeds the water's reflected-sky colour and
	// arms the post-effect capture when an effect will draw. The Qt port never
	// called it, so the sky never drew and the fog never reached the terrain or
	// the objects. dt here is in seconds, the graph side's unit (0.001 * the
	// milliseconds scene_->SetDeltaTime got above, as CGeneralView did).
	//
	// Environment exists only while a world is loaded (Universe's ctor creates
	// it, its dtor destroys it); environmentTime() exists from the Environment
	// ctor on. Nothing below touches a world object, so a bare check suffices.
	if(environment){
		const float dt = 0.001f * (float)frameMs;
		environment->graphQuant(dt, camera_);
	}

	// CGeneralView::graphQuant and GameShell::Show both set ATTRCAMERA_CLEARZBUFFER
	// on the camera right after graphQuant and before terScene->Draw, with the
	// comment "Потому как в небе могут рисоваться планеты в z buffer" -- the sky
	// draws first (EnvironmentTime::DrawEnviroment -> cSkyObj::DrawSkyAndAnimate)
	// in a frustum of its own (1e3..1e5) and leaves its depth behind. clearZBuffer
	// (Camera::ClearZBuffer -> cSDLRenderDevice::clearZBuffer) lets the next pass
	// own the depth clear again, exactly as if nothing had been drawn, so the
	// terrain and objects sort against a clean depth buffer instead of the sky's.
	camera_->setAttribute(ATTRCAMERA_CLEARZBUFFER);

	// The camera renders whatever the scene holds: the terrain tile map when a
	// world is loaded, nothing otherwise. cScene::Draw is the entry point, NOT
	// Camera::DrawScene alone: it calls tileMap_->PreDraw first, which attaches
	// the tile map to the camera's SCENENODE_OBJECT_TILEMAP slot that
	// DrawTilemapObject then draws from. Calling DrawScene directly skips that
	// attach and renders no terrain (only the grid, drawn after).
	{
		const Vect2f center(0.5f, 0.5f);
		const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
		const Vect2f focus(orbit_.focus, orbit_.focus);
		float zMin = 0.f, zMax = 0.f; editorZPlane(zMin, zMax);
		const Vect2f zPlane(zMin, zMax);
		camera_->SetFrustum(&center, &clip, &focus, &zPlane);

		scene_->Draw(camera_);
	}

	// CGeneralView::graphQuant drew the grid after terScene->Draw(), then
	// composited the post-effect stack (environment->drawPostEffects). Monochrome
	// and the under-water effect are ported to SDL (Render-PORTING.md #6a); the
	// composite is a no-op on frames where no effect recorded anything.
	drawGrid();
	if(environment){
		const float dt = 0.001f * (float)frameMs;
		environment->drawPostEffects(dt, camera_);
	}

	// CGeneralView::graphQuant then ran universe()->graphQuant(dt) — under the
	// editor it walks the players and calls showEditor() on every unit, which
	// applies the HIDE_BY_EDITOR visibility (UnitBase::showEditor). EditorVisual
	// is always-visible in the Qt port for now, so this is the hook the View
	// filters will drive later. dt is the real frame delta in seconds, exactly
	// as the game (GameShell::Show) and the original editor (CGeneralView::graphQuant)
	// passed it — the graph side uses it to advance per-frame animation.
	if(ownedUniverse_)
		universe()->graphQuant(0.001f * (float)frameMs);

	// CGeneralView::graphQuant (SurMap5/GeneralView.cpp:314) then drew the
	// editor aux layers: cameraManager->showEditor() (camera splines),
	// sourceManager->showEditor() (sources/anchors), environment->showEditor(),
	// the current tool's onDrawAuxData(), and editorVisual().afterQuant().
	// Without these the 3D view shows no selection circles, no camera paths,
	// no source marks, no tool gizmos.
	// Старого вызова UI_Dispatcher::quant здесь нет: в редакторе gameShell==0
	// (создаётся только в игре), и quant падает на
	// UI_LogicDispatcher::isGameActive()->gameShell->GameActive.
	if(cameraManager)
		cameraManager->showEditor();
	if(sourceManager)
		sourceManager->showEditor();
	if(environment)
		environment->showEditor();
	drawToolAux();

	// [SurMap5Qt] Как в CGeneralView::graphQuant (SurMap5/GeneralView.cpp:325):
	// editorVisual().afterQuant() после aux-слоёв.
	editorVisual().afterQuant();

	// UI Editor preview: draw the selected UI screen over the 3D frame (the
	// same engine render device the original UIEditor used). Drawn directly
	// through the screen's redraw (UI_Dispatcher::selectScreen would run the
	// screen's logic activation, which the editor cannot). The overlay goes on
	// top of the aux layers and below the tool rubber band.
	if(uiPreview_ && uiPreviewScreen_){
		try{
			const int w = gb_RenderDevice->GetSizeX();
			const int h = gb_RenderDevice->GetSizeY();
			UI_Render::instance().setWindowPosition(Recti(0, 0, w, h));
			UI_Render::instance().updateRenderSize();
			uiPreviewScreen_->redraw();
		}
		catch(...){
			uiPreview_ = false;
		}
	}

	// The Select tool's rubber band (CSurToolSelect::onDrawAuxData drew it via
	// DrawRectangle after the 3D scene). DrawRectangle goes through the UI
	// renderer, so it lands on top of the frame in the same present. The box
	// arrives in widget-local pixels; scale into device pixels (see
	// selectObjectsInRect).
	if(selBoxVisible_){
		const float devW = (float)gb_RenderDevice->GetSizeX();
		const float devH = (float)gb_RenderDevice->GetSizeY();
		if(devW > 0.f && devH > 0.f && widgetW_ > 0 && widgetH_ > 0){
			const float kx = devW / (float)widgetW_;
			const float ky = devH / (float)widgetH_;
			const int x = (int)((float)std::min(selBoxX0_, selBoxX1_) * kx);
			const int y = (int)((float)std::min(selBoxY0_, selBoxY1_) * ky);
			const int dx = (int)((float)abs(selBoxX1_ - selBoxX0_) * kx);
			const int dy = (int)((float)abs(selBoxY1_ - selBoxY0_) * ky);
			gb_RenderDevice->DrawRectangle(x, y, dx, dy, Color4c(0, 255, 0, 255), true);
		}
	}

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
}

// --- Editor aux drawing (3D gizmos) ----------------------------------------

// Port of the tool aux drawing (CSurToolTransform::drawAxis/drawCircle,
// SurMap5/SurToolTransform.cpp:183-239): the selection centre + radius drive
// three axis lines and a selection circle, drawn in world space through
// gb_RenderDevice (DrawLine) and circleManager (addCircle) — exactly like the
// original. The Qt tools are engine-free and draw in screen space (never
// called); this engine-side pass draws the 3D gizmos the original showed.
void EngineViewport::drawToolAux()
{
	if(!ownedUniverse_ || !universe())
		return;

	// Selection centre + radius: AABB of the selected units' positions
	// (CSurToolTransform::selectionCenter_/selectionRadius_).
	Vect3f center = Vect3f::ZERO;
	float radius = 100.0f;
	int count = 0;
	float minX = 0, maxX = 0, minY = 0, maxY = 0, firstRadius = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi){
		const UnitList& units = (*pi)->units();
		UnitList::const_iterator it;
		FOR_EACH(units, it){
			UnitBase* u = *it;
			if(!u || !u->selected())
				continue;
			const Vect3f& p = u->position();
			const float r = u->radius();
			if(count == 0){
				minX = maxX = p.x;
				minY = maxY = p.y;
				firstRadius = r;
			}
			else{
				minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
				minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
			}
			++count;
		}
	}
	if(count == 0)
		return;
	center.set((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, 0);
	const float dx = maxX - minX, dy = maxY - minY;
	const float diag = sqrtf(dx * dx + dy * dy);
	radius = diag > 1e-6f ? diag * 0.5f : firstRadius;

	// drawAxis: three lines from the centre (X red, Y green, Z blue).
	gb_RenderDevice->DrawLine(center, center + Vect3f(0.f, 0.f, radius), Color4c(0, 0, 255, 255));
	gb_RenderDevice->DrawLine(center, center + Vect3f(radius, 0.f, 0.f), Color4c(255, 0, 0, 255));
	gb_RenderDevice->DrawLine(center, center + Vect3f(0.f, radius, 0.f), Color4c(0, 255, 0, 255));

	// drawCircle: selection circle in the XY plane at the centre.
	// CSurToolTransform::drawCircle built it from 36 DrawLine segments;
	// circleManager()->addCircle goes through the world-quad pass which the
	// editor frame never flushes at the right point, so draw the ring
	// directly like the axes above (visible immediately, no extra pass).
	{
		const int segs = 36;
		const float step = 2.0f * 3.14159265f / (float)segs;
		Vect3f prev(center.x + radius, center.y, center.z);
		for(int i = 1; i <= segs; ++i){
			const float a = step * (float)i;
			const Vect3f cur(center.x + cosf(a) * radius, center.y + sinf(a) * radius, center.z);
			gb_RenderDevice->DrawLine(prev, cur, Color4c(255, 255, 255, 255));
			prev = cur;
		}
	}
}

// --- Input ---------------------------------------------------------------

void EngineViewport::mouseWheel(int wheelDelta, int modifiers)
{
	// Port of CGeneralView::WindowProc WM_MOUSEWHEEL.
	//   wheelDelta > 0 = wheel up (zoom in); SHIFT moves focus; ALT rolls fi.
	const float sign = wheelDelta > 0 ? 1.f : -1.f;
	const bool shift = (modifiers & 1) != 0;
	const bool ctrl  = (modifiers & 2) != 0;
	const bool alt   = (modifiers & 4) != 0;

	if(shift){
		if(alt)
			orbit_.focus = clamp(orbit_.focus + sign * 0.04f, 0.1f, 10.0f);
		else
			orbit_.pz += -sign * (ctrl ? 0.0025f : 0.01f) * orbit_.distance;
	}
	else{
		if(alt)
			orbit_.fi -= sign * 0.04f;
		else
			orbit_.distance = max(orbit_.distance, 2.0f) * (1.0f - ((ctrl ? 0.01f : 0.2f) * sign));
	}
	applyCamera();
}

void EngineViewport::mouseButton(int button, bool pressed, int x, int y)
{
	// button: 1=left, 2=middle, 4=right — CGeneralView's capture model.
	if(button == 2){
		if(pressed){
			mouseMiddle_ = true;
			dragStartX_ = x; dragStartY_ = y;
			dragStartPsi_ = orbit_.psi;
			dragStartTheta_ = orbit_.theta;
		}
		else
			mouseMiddle_ = false;
	}
	else if(button == 4){
		if(pressed){
			mouseRight_ = true;
			dragStartX_ = x; dragStartY_ = y;
			dragStartPx_ = orbit_.px; dragStartPy_ = orbit_.py; dragStartPz_ = orbit_.pz;
		}
		else
			mouseRight_ = false;
	}
	else if(button == 1){
		mouseLeft_ = pressed;
		if(pressed){
			dragStartX_ = x; dragStartY_ = y;
		}
	}
}

void EngineViewport::mouseMove(int x, int y)
{
	if(mouseMiddle_){
		const float dx = float(x - dragStartX_);
		const float dy = float(y - dragStartY_);
		orbit_.psi = dragStartPsi_ + dx * kMouseMove2Angle;
		orbit_.theta = dragStartTheta_ - dy * kMouseMove2Angle;
		applyCamera();
	}
	else if(mouseRight_){
		// Pan: move the orbit centre in the camera plane. Port of CGeneralView's
		// WM_MOUSEMOVE RMB branch (SurMap5/GeneralView.cpp:546):
		//   - the anchor advances every event (BegMousePos = CurMousePos), so
		//     the delta is incremental, not from the drag start;
		//   - the screen delta rotates by Mat2f(pi/2 + psi);
		//   - the centre moves by += delta (not -=);
		//   - pz follows the terrain height under the new centre (To3D).
		const float dx = float(x - dragStartX_);
		const float dy = float(y - dragStartY_);
		dragStartX_ = x;
		dragStartY_ = y;
		Vect2f delta(dx, dy);
		delta *= Mat2f(M_PI_2 + orbit_.psi);
		delta *= max(orbit_.distance, 100.f) / 800.f;
		orbit_.px += delta.x;
		orbit_.py += delta.y;
		// To3D: the terrain height under the new centre (vMap.getZf, clamped
		// to the map like cTileMap::To3D).
		if(vMap.isWorldLoaded()){
			const int xi = (int)roundf(orbit_.px), yi = (int)roundf(orbit_.py);
			if(xi >= 0 && yi >= 0 && xi < (int)vMap.H_SIZE && yi < (int)vMap.V_SIZE)
				orbit_.pz = max(0.0f, vMap.getZf(xi, yi));
		}
		applyCamera();
	}
}

// --- Picking --------------------------------------------------------------

// Port of CGeneralView::CoordScr2vMap (SurMap5/GeneralView.cpp:405): the mouse
// pixel becomes a ray out of the camera, and the first terrain hit is the
// world point. GetWorldRay needs the camera's frustum (SetFrustum) to have
// run -- drawFrame does it each frame, but mouse events arrive between frames,
// so re-apply the same frustum here before unprojecting.
bool EngineViewport::screenPointToGround(int x, int y, float& outX, float& outY, float& outZ)
{
	if(!inited_ || !camera_ || !scene_ || !gb_RenderDevice || !worldLoaded_)
		return false;

	// Normalize against the WIDGET size, not the render device's swapchain
	// size: GetSizeX()/GetSizeY() report the swapchain drawable (device pixels,
	// devicePixelRatio times larger on HiDPI), while x,y arrive in widget-local
	// pixels. Normalizing widget pixels by the swapchain size shrinks the ray
	// toward the centre, so tools edit the wrong spot. The original had no such
	// mismatch (the D3D backbuffer was 1:1 with the window).
	const int w = widgetW_;
	const int h = widgetH_;
	if(w <= 0 || h <= 0)
		return false;

	// CGeneralView::CoordScr2vMap normalized with the render-device size, origin
	// at the centre (the engine's screen-space convention).
	const Vect2f posIn((float)x / (float)w - 0.5f, (float)y / (float)h - 0.5f);

	// The frustum drawFrame uses (CGeneralView::graphQuant's camera set-up).
	const Vect2f center(0.5f, 0.5f);
	const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
	const Vect2f focus(orbit_.focus, orbit_.focus);
	float zMin = 0.f, zMax = 0.f; editorZPlane(zMin, zMax);
	const Vect2f zPlane(zMin, zMax);
	camera_->SetFrustum(&center, &clip, &focus, &zPlane);

	Vect3f pos, dir;
	camera_->GetWorldRay(posIn, pos, dir);

	Vect3f trace;
	if(scene_->TraceDir(pos, dir, &trace)){
		outX = trace.x; outY = trace.y; outZ = trace.z;
		return true;
	}
	return false;
}

// --- Camera --------------------------------------------------------------

void EngineViewport::applyCamera()
{
	if(!camera_)
		return;

	// The orbit matrix, as CameraManager::update builds it:
	//   R(theta,X)*R(fi,Y)*R(pi/2-psi,Z) translated by -position.
	// Position sits on the orbit sphere around the centre.
	//
	// INTENTIONALLY not cameraManager->quant(): the original editor's
	// CameraQuant -> quant(0,0,dt) rebuilds the whole camera from
	// cameraManager->coordinate() and runs its velocity/restriction/clamp
	// machinery, which fights this editor's event-driven orbit (input writes the
	// orbit, quant then clamps distance/theta out from under it and the camera
	// stops responding). The port keeps the orbit authoritative and applies the
	// matrix directly, as it did before the experiment. If quant is ever wanted
	// back, it has to become the single input path (write coordinate() on every
	// event, never touch orbit_), not a per-frame re-derivation.
	Vect3f position(
		orbit_.px + orbit_.distance * sinf(orbit_.theta) * cosf(orbit_.psi),
		orbit_.py + orbit_.distance * sinf(orbit_.theta) * sinf(orbit_.psi),
		orbit_.pz + orbit_.distance * cosf(orbit_.theta));

	MatXf matrix = MatXf::ID;
	matrix.rot() = Mat3f(orbit_.theta, X_AXIS) * Mat3f(orbit_.fi, Y_AXIS) * Mat3f(M_PI_2 - orbit_.psi, Z_AXIS);
	matrix *= MatXf(Mat3f::ID, -position);
	setCameraPosition(camera_, matrix);
}

// The camera's near/far planes, as the original editor computed them:
// CameraManager::SetFrustumEditor -> calcZMinMax(), the environment's game
// frustum (defaults 30..4000), extended only when the orbit is far enough that
// the map would otherwise clip, with zNear scaled to bound the far/near ratio.
// (The quant() experiment that would have got this from SetFrustumGame is
// reverted -- it fought the editor's orbit input.)
void EngineViewport::editorZPlane(float& zMin, float& zMax) const
{
	// calcZMinMax(): the environment's frustum, blended by the camera tilt.
	float zn = 30.0f;
	float zf = 4000.0f;
	if(environment){
		zn = environment->GetGameFrustrumZMin();
		const float angle = orbit_.theta;
		float c = fabsf(angle) / (float)M_PI_2;
		c = clamp(c, 0.0f, 1.0f);
		zf = environment->GetGameFrustrumZMaxHorizontal() * c +
		     environment->GetGameFrustrumZMaxVertical() * (1.0f - c);
	}
	if(zn < 1.0f)
		zn = 1.0f;

	// The whole map (plus the orbit centre) must stay inside the far plane, or
	// zooming out clips the terrain. The original camera never left the map's
	// neighbourhood, the Qt orbit does.
	if(vMap.isWorldLoaded()){
		const float diagonal = sqrtf((float)vMap.H_SIZE * (float)vMap.H_SIZE +
		                             (float)vMap.V_SIZE * (float)vMap.V_SIZE);
		zf = std::max(zf, orbit_.distance + diagonal);
	}
	zf = std::max(zf, 1000.0f);

	// Bound the ratio: a perspective depth buffer's usable precision lives in
	// zNear/zFar. 100 keeps the terrain and the ground-level effects ~20x more
	// distinguishable than the old 2000 did.
	const float maxRatio = 100.0f;
	zn = std::max(zn, zf / maxRatio);

	zMin = zn;
	zMax = zf;
}

// --- World data (U4 dialogs) ---------------------------------------------

const char* EngineViewport::worldName() const
{
	return worldLoaded_ ? vMap.getWorldName().c_str() : "";
}

// CMainFrame's view_->reInitWorld after a terrain mutation: the tile map is
// dropped (its buffers reference the old vMap) and re-created from the current
// vMap state. doneWorld() would release the world too; here only the scene is
// rebuilt.
bool EngineViewport::reinitWorld()
{
	if(!inited_ || !scene_ || !worldLoaded_)
		return false;

	if(scene_)
		RELEASE(scene_);
	scene_ = gb_VisGeneric->CreateScene();
	camera_ = scene_ ? scene_->CreateCamera() : nullptr;
	if(!scene_ || !camera_)
		return false;

	scene_->CreateMap(true);
	applyCamera();
	return true;
}

bool EngineViewport::mapSize(int& hSize, int& vSize) const
{
	if(!worldLoaded_)
		return false;
	hSize = (int)vMap.H_SIZE;
	vSize = (int)vMap.V_SIZE;
	return true;
}

bool EngineViewport::mapCreationParams(int& hSizePower, int& vSizePower,
                                       int& createWorldMetod, int& initialHeight) const
{
	if(!worldLoaded_)
		return false;
	hSizePower = (int)vMap.H_SIZE_POWER;
	vSizePower = (int)vMap.V_SIZE_POWER;
	createWorldMetod = (int)vMap.createWorldMetod;
	initialHeight = (int)vMap.initialHeight;
	return true;
}

// Port of world2Histogram (SurMap5/DlgChangeTotalWorldHeight.cpp): bin every
// vertex's voxel height over 256 buckets, then sqrt-scale the columns so the
// dialog's bars stay readable.
bool EngineViewport::worldHeightHistogram(int out[256], int& minVx, int& maxVx)
{
	if(!worldLoaded_)
		return false;

	// Terra/terra.h: MAX_VX_HEIGHT = (1<<(VX_FRACTION+9))-1 (0x3fff).
	const int kMaxVxHeight = MAX_VX_HEIGHT;

	int histArr[256] = {0};
	int mn = INT_MAX;
	int mx = INT_MIN;
	for(unsigned i = 0; i < vMap.V_SIZE; i++){
		for(unsigned j = 0; j < vMap.H_SIZE; j++){
			const int h = (int)vMap.getAlt((int)j, (int)i);
			if(mn > h) mn = h;
			if(mx < h) mx = h;
			histArr[h * 256 / (kMaxVxHeight + 1)]++;
		}
	}

	int maxVal = 0;
	for(int i = 0; i < 256; i++)
		if(histArr[i] > maxVal) maxVal = histArr[i];

	// sqrt(maxVal) normalizes the columns; clamp to the whole-voxel max so a
	// full-height world still fits (the original clamped to MAX_VX_HEIGHT_WHOLE
	// and never let a column drop below 4px).
	const float maxValF = sqrtf((float)maxVal);
	for(int i = 0; i < 256; i++){
		int s = 0;
		if(histArr[i]){
			s = (int)roundf(sqrtf((float)histArr[i]) / maxValF * 255.f);
			if(s > MAX_VX_HEIGHT_WHOLE) s = MAX_VX_HEIGHT_WHOLE;
			if(s < 4) s = 4;
		}
		out[i] = s;
	}
	minVx = mn;
	maxVx = mx;
	return true;
}

float EngineViewport::changeTotalWorldParam(int deltaVx, float kScale, const Editor::MapChangeParams& params)
{
	if(!worldLoaded_)
		return 0.f;

	// Build the engine's vrtMapChangeParam (vrtMapCreationParam base + the
	// move/resize flags) from the engine-free struct.
	vrtMapChangeParam p;
	static_cast<vrtMapCreationParam&>(p).H_SIZE_POWER = (vrtMapCreationParam::SIZE_POWER)params.hSizePower;
	static_cast<vrtMapCreationParam&>(p).V_SIZE_POWER = (vrtMapCreationParam::SIZE_POWER)params.vSizePower;
	p.createWorldMetod = (vrtMapCreationParam::eCreateWorldMetod)params.createWorldMetod;
	p.initialHeight = params.initialHeight;
	p.flag_resizeWorld2NewBorder = params.flag_resizeWorld2NewBorder;
	p.oldWorldBegCoordX = params.oldWorldBegCoordX;
	p.oldWorldBegCoordY = params.oldWorldBegCoordY;
	p.kScaleModels = params.kScaleModels;

	return vMap.changeTotalWorldParam(deltaVx, kScale, p);
}

int EngineViewport::textureStatistics(const TextureStat*& out, int& totalSize)
{
	out = nullptr;
	totalSize = 0;

	// GetTexLibrary (Render/src/TexLibrary.h) is a global that exists as soon
	// as the render device is up. The texture names point into the library, so
	// the returned array is only valid for the caller's immediate use.
	cTexLibrary* texLib = GetTexLibrary();
	if(!texLib)
		return 0;

	const int count = texLib->GetNumberTexture();
	if(count <= 0)
		return 0;

	static std::vector<TextureStat> stats;
	stats.resize(count);
	for(int i = 0; i < count; i++){
		cTexture* tex = texLib->GetTexture(i);
		stats[i].name = tex ? tex->name() : "";
		stats[i].size = tex ? tex->CalcTextureSize() : 0;
		totalSize += stats[i].size;
	}

	out = stats.data();
	return count;
}

// --- Minimap (U6) ----------------------------------------------------------

bool EngineViewport::minimapSize(int& sizex, int& sizey) const
{
	if(!worldLoaded_)
		return false;
	sizex = (int)vMap.H_SIZE / 16;
	sizey = (int)vMap.V_SIZE / 16;
	if(sizex < 1) sizex = 1;
	if(sizey < 1) sizey = 1;
	return true;
}

// Port of vrtMap::saveMiniMap (Terra/VMAP.CPP:917): average every stepXVM x
// stepYVM block of the world into one RGB pixel, exactly the way the original
// built map.tga — minus the TGA file write, so the Qt minimap panel gets the
// pixels directly.
bool EngineViewport::minimapPixels(unsigned long* out, int sizex, int sizey)
{
	if(!worldLoaded_ || !out)
		return false;
	if(sizex < 1 || sizey < 1)
		return false;

	const int stepXVM = (int)vMap.H_SIZE / sizex;
	const int stepYVM = (int)vMap.V_SIZE / sizey;
	if(stepXVM < 1 || stepYVM < 1)
		return false;
	const int stepPoints = stepXVM * stepYVM;

	int cnt = 0;
	for(unsigned i = 0; i < vMap.V_SIZE; i += (unsigned)stepYVM){
		for(unsigned j = 0; j < vMap.H_SIZE; j += (unsigned)stepXVM){
			int r = 0, g = 0, b = 0;
			for(int k = 0; k < stepYVM; k++){
				for(int m = 0; m < stepXVM; m++){
					const int color = vMap.getColor32((int)j + m, (int)i + k);
					r += (color >> 16) & 0xFF;
					g += (color >> 8) & 0xFF;
					b += color & 0xFF;
				}
			}
			out[cnt++] = 0xFF000000u |
				((unsigned)(r / stepPoints) << 16) |
				((unsigned)(g / stepPoints) << 8) |
				(unsigned)(b / stepPoints);
		}
	}
	return true;
}

bool EngineViewport::saveMiniMapToFile()
{
	if(!worldLoaded_)
		return false;
	vMap.saveMiniMap((int)vMap.H_SIZE / 16, (int)vMap.V_SIZE / 16);
	return true;
}

bool EngineViewport::cameraCenter(float& x, float& y) const
{
	if(!worldLoaded_)
		return false;
	x = orbit_.px;
	y = orbit_.py;
	return true;
}

void EngineViewport::setCameraCenter(float x, float y)
{
	if(!worldLoaded_)
		return;
	orbit_.px = x;
	orbit_.py = y;
	applyCamera();
}

// --- Status bar (U8) ------------------------------------------------------

// CGeneralView::UpdateStatusBar (SurMap5/GeneralView.cpp:687) filled the
// status panes from the world point under the mouse: the surface-kind name
// (TerrainTypeDescriptor::nameAlt of 1<<getSurKind), the exact voxel height
// (getAlt), the approximate grid height (getApproxAlt) and the water height
// (environment->water()->GetZ). The Qt editor has no Environment, so the
// water pane is left at 0.
bool EngineViewport::terrainInfoAt(float x, float y, char* surfName, int surfNameSize,
                                   int& altVox, int& approxAlt, int& waterZ) const
{
	if(!worldLoaded_)
		return false;
	const int xi = (int)roundf(x);
	const int yi = (int)roundf(y);
	if(xi < 0 || yi < 0 || xi >= (int)vMap.H_SIZE || yi >= (int)vMap.V_SIZE)
		return false;

	const unsigned char kind = vMap.getSurKind(xi, yi);
	const char* name = TerrainTypeDescriptor::instance().nameAlt(1 << kind);
	if(surfName && surfNameSize > 0){
		snprintf(surfName, surfNameSize, "%s", name ? name : "");
		surfName[surfNameSize - 1] = 0;
	}
	altVox = (int)vMap.getAlt(xi, yi);
	approxAlt = vMap.getApproxAlt(xi, yi);
	waterZ = 0;   // no Environment in the Qt editor yet
	return true;
}
// --- Object list (Objects Manager) ---

// Mirrors CObjectsManagerTree::rebuild (SurMap5/ObjectsManagerTree.cpp:78).
// Each tab walks the engine's live container: sourceManager for Sources
// and Anchors, universe()->worldPlayer()->units_ filtered by attribute
// for Environment/Units, cameraManager->splines() for Cameras.
int EngineViewport::objectList(ObjectTab tab, char** out, int maxCount)
{
	objectPositions_.clear();
	objectIds_.clear();
	objectSelected_.clear();
	if(!out || maxCount <= 0)
		return 0;
	if(!sourceManager)
		return 0;

	if(tab == ObjectTab::Sources){
		// TAB_SOURCES — flushNewSources, then walk getTypeSources per type
		// and emit "<DisplayName(type)> #N - <label>". The original
		// grouped by SourceType (LightSource, WaterSource, ...); here
		// we keep a flat list but still prefix with the type so the
		// editor stays compatible with the per-type display names.
		sourceManager->flushNewSources();
		int n = 0;
		for(int typeIdx = 0; typeIdx < SOURCE_MAX && n < maxCount; ++typeIdx){
			const SourceType type = (SourceType)typeIdx;
			SourceManager::Sources byType;
			sourceManager->getTypeSources(type, byType);
			const std::string typeName = SourceBase::getDisplayName(type);
			int index = 0;
			for(int i = 0; i < (int)byType.size() && n < maxCount; ++i){
				SourceBase* src = byType[i].get();
				if(!src || !src->isAlive())
					continue;
				const char* label = src->label();
				char buf[256];
				if(label && *label)
					snprintf(buf, sizeof(buf), "%s #%d - %s", typeName.c_str(), index++, label);
				else
					snprintf(buf, sizeof(buf), "%s #%d", typeName.c_str(), index++);
				out[n] = strdup(buf);
				objectPositions_.push_back({ src->position2D().x, src->position2D().y });
				objectIds_.push_back((EditorObjectId)src);
				objectSelected_.push_back(src->selected() ? 1 : 0);
				++n;
			}
		}
		return n;
	}
	if(tab == ObjectTab::Anchors){
		// TAB_ANCHORS — sourceManager->anchors() filtered by visibility
		// (the on-mouse anchor is hidden in the original).
		const SourceManager::Anchors& anchors = sourceManager->anchors();
		int n = 0;
		int index = 0;
		for(; n < maxCount && index < (int)anchors.size(); ++index){
			Anchor* a = anchors[index].get();
			if(!a)
				continue;
			const char* label = a->label();
			char buf[256];
			snprintf(buf, sizeof(buf), "Anchor #%d - %s", n, label ? label : "");
			out[n] = strdup(buf);
			objectPositions_.push_back({ a->position2D().x, a->position2D().y });
			objectIds_.push_back((EditorObjectId)a);
			objectSelected_.push_back(a->selected() ? 1 : 0);
			++n;
		}
		return n;
	}
	if(tab == ObjectTab::Cameras){
		// TAB_CAMERA — cameraManager->splines(). CameraSpline::name()
		// is the splines's display label.
		if(!cameraManager)
			return 0;
		const CameraSplines& splines = cameraManager->splines();
		int n = 0;
		for(int i = 0; i < (int)splines.size() && n < maxCount; ++i){
			ShareHandle<CameraSpline> sp = splines[i];
			if(!sp)
				continue;
			const char* name = sp->name();
			char buf[256];
			snprintf(buf, sizeof(buf), "Camera #%d - %s", n, (name && *name) ? name : "(unnamed)");
			out[n] = strdup(buf);
			objectPositions_.push_back({ sp->position2D().x, sp->position2D().y });
			objectIds_.push_back((EditorObjectId)sp.get());
			objectSelected_.push_back(sp->selected() ? 1 : 0);
			++n;
		}
		return n;
	}

	// Environment and Units: walk universe()->Players[i]->units_() —
	// the original did the same (SurMap5/ObjectsManagerTree.cpp:107-141).
	if(!universe())
		return 0;

	if(tab == ObjectTab::Environment){
		// TAB_ENVIRONMENT — only UnitEnvironment / UnitEnvironmentSimple
		// units (the original dynamic_cast-filtered the world's units
		// by these two types). modelName() is the asset path; the
		// dialog shows the basename.
		Player* wp = universe()->worldPlayer();
		if(!wp)
			return 0;
		UnitList& units = const_cast<UnitList&>(wp->units());
		int n = 0;
		for(int i = 0; i < (int)units.size() && n < maxCount; ++i){
			UnitBase*& unit = units[i].unit();
			if(!unit || unit->auxiliary() || !unit->alive())
				continue;
			UnitEnvironment* uenv = dynamic_cast<UnitEnvironment*>(unit);
			UnitEnvironmentSimple* uenvs = uenv ? nullptr : dynamic_cast<UnitEnvironmentSimple*>(unit);
			if(!uenv && !uenvs)
				continue;
			const char* model = uenv ? uenv->modelName() : uenvs->modelName();
			std::string name = model ? model : "";
			const auto pos = name.rfind('\\');
			if(pos != std::string::npos)
				name = name.substr(pos + 1);
			char buf[256];
			snprintf(buf, sizeof(buf), "Environment #%d - %s", n, name.c_str());
			out[n] = strdup(buf);
			objectPositions_.push_back({ unit->position2D().x, unit->position2D().y });
			objectIds_.push_back((EditorObjectId)unit);
			objectSelected_.push_back(unit->selected() ? 1 : 0);
			++n;
		}
		return n;
	}
	if(tab == ObjectTab::Units){
		// TAB_UNITS — every non-internal, non-auxiliary, alive unit
		// across all Players. attr().libraryKey() is the unit's
		// display label (the same string the original used).
		const PlayerVect& players = universe()->Players;
		int n = 0;
		for(int p = 0; p < (int)players.size() && n < maxCount; ++p){
			Player* player = players[p];
			if(!player)
				continue;
			UnitList& units = const_cast<UnitList&>(player->units());
			for(int i = 0; i < (int)units.size() && n < maxCount; ++i){
				UnitBase*& unit = units[i].unit();
				if(!unit)
					continue;
				if(unit->attr().internal || unit->auxiliary() || !unit->alive())
					continue;
				const char* key = unit->attr().libraryKey();
				char buf[256];
				snprintf(buf, sizeof(buf), "Unit #%d - %s", n, key ? key : "(no key)");
				out[n] = strdup(buf);
				objectPositions_.push_back({ unit->position2D().x, unit->position2D().y });
				objectIds_.push_back((EditorObjectId)unit);
				objectSelected_.push_back(unit->selected() ? 1 : 0);
				++n;
			}
		}
		return n;
	}

	return 0;
}

bool EngineViewport::objectPosition(ObjectTab tab, int index, float& x, float& y)
{
	EditorObjectId id = IWorldBridge::kNoObject;
	bool selected = false;
	if(!objectEntry(tab, index, id, x, y, selected))
		return false;
	return true;
}

// The index-th entry of the last objectList walk. objectList records the
// per-entry data (position, handle, selection) as it emits; re-walk the tab up
// to include `index`.
bool EngineViewport::objectEntry(ObjectTab tab, int index, EditorObjectId& id,
                                 float& x, float& y, bool& selected)
{
	if(index < 0)
		return false;
	std::vector<char*> labels((size_t)index + 1, nullptr);
	const int n = objectList(tab, labels.data(), index + 1);
	for(int i = 0; i < n; ++i)
		free(labels[i]);
	if(index >= (int)objectPositions_.size())
		return false;
	x = objectPositions_[(size_t)index].first;
	y = objectPositions_[(size_t)index].second;
	id = objectIds_[(size_t)index];
	selected = objectSelected_[(size_t)index] != 0;
	return true;
}

bool EngineViewport::objectSelected(ObjectTab tab, int index)
{
	EditorObjectId id = IWorldBridge::kNoObject;
	float x = 0.f, y = 0.f;
	bool selected = false;
	if(!objectEntry(tab, index, id, x, y, selected))
		return false;
	return selected;
}

bool EngineViewport::setObjectSelected(ObjectTab tab, int index, bool selected)
{
	EditorObjectId id = IWorldBridge::kNoObject;
	float x = 0.f, y = 0.f;
	bool wasSelected = false;
	if(!objectEntry(tab, index, id, x, y, wasSelected))
		return false;
	if(BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id))
		obj->setSelected(selected);
	return true;
}

// --- Object selection (SurMap5/SelectionUtil.cpp) ---

int EngineViewport::objectCount(ObjectTab tab)
{
	// Cheap count-only walk: reuse objectList with a sink.
	int count = 0;
	auto consume = [](char** out, int maxCount){
		for(int i = 0; i < maxCount; ++i){
			if(!out[i])
				break;
			free(out[i]);
		}
	};
	char* labels[64] = {};
	int n = 0;
	do {
		n = objectList(tab, labels, 64);
		consume(labels, n);
		count += n;
	} while(n == 64);
	return count;
}

int EngineViewport::selectedObjectsCount()
{
	if(!ownedUniverse_ || !sourceManager)
		return 0;
	int count = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi){
		const UnitList& units = (*pi)->units();
		UnitList::const_iterator it;
		FOR_EACH(units, it){
			UnitBase* unit = *it;
			if(unit && !unit->auxiliary() && unit->alive() && unit->selected())
				++count;
		}
	}
	return count;
}

void EngineViewport::deselectAllObjects()
{
	if(!ownedUniverse_ || !sourceManager)
		return;
	sourceManager->deselectAll();
	universe()->deselectAll();
	if(cameraManager){
		CameraSplines::const_iterator it;
		FOR_EACH(cameraManager->splines(), it)
			(*it)->setSelected(false);
	}
}

bool EngineViewport::selectObjectAt(int screenX, int screenY, int mode)
{
	if(!inited_ || !camera_ || !ownedUniverse_ || !gb_RenderDevice || !worldLoaded_)
		return false;

	// Widget size, not the swapchain size (see screenPointToGround).
	const int w = widgetW_;
	const int h = widgetH_;
	if(w <= 0 || h <= 0)
		return false;

	const Vect2f posIn((float)screenX / (float)w - 0.5f, (float)screenY / (float)h - 0.5f);

	// Re-apply the frustum (see screenPointToGround) then unproject the ray.
	const Vect2f center(0.5f, 0.5f);
	const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
	const Vect2f focus(orbit_.focus, orbit_.focus);
	float zMin = 0.f, zMax = 0.f; editorZPlane(zMin, zMax);
	const Vect2f zPlane(zMin, zMax);
	camera_->SetFrustum(&center, &clip, &focus, &zPlane);

	Vect3f v0, dir;
	camera_->GetWorldRay(posIn, v0, dir);
	const Vect3f v1 = v0 + dir * 50000.0f;
	const Vect3f v01 = v1 - v0;

	// unitHoverAll: the nearest alive non-auxiliary unit whose intersect() the
	// ray hits, measured by distance to the ray origin.
	float distMin = FLT_MAX;
	UnitBase* unitMin = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi){
		const UnitList& units = (*pi)->units();
		UnitList::const_iterator it;
		FOR_EACH(units, it){
			UnitBase* unit = *it;
			Vect3f hit;
			if(unit && !unit->auxiliary() && unit->alive() && unit->intersect(v0, v1, hit)){
				const float d = unit->position().distance2(v0);
				if(d < distMin){
					distMin = d;
					unitMin = unit;
				}
			}
		}
	}
	(void)v01;
	if(!unitMin)
		return false;

	// CSurToolSelect::onLMBUp: shift = add, ctrl = toggle, plain = replace.
	if(mode == 1){            // toggle (ctrl)
		unitMin->setSelected(!unitMin->selected());
	}
	else if(mode == 2){       // add (shift)
		if(!unitMin->selected())
			unitMin->setSelected(true);
	}
	else{                     // replace
		deselectAllObjects();
		unitMin->setSelected(true);
	}
	return true;
}

bool EngineViewport::selectObjectsInRect(int x0, int y0, int x1, int y1)
{
	if(!ownedUniverse_ || !gb_RenderDevice || !worldLoaded_)
		return false;
	if(x0 > x1) std::swap(x0, x1);
	if(y0 > y1) std::swap(y0, y1);

	// SelectionUtil::selectByScreenRectangle normalized the box corners and
	// picked every unit whose 2D screen position (ConvertorWorldToViewPort)
	// falls inside. ConvertorWorldToViewPort goes through matViewProjScr,
	// which Camera::UpdateViewport builds from the swapchain size
	// (GetSizeX()/GetSizeY() — device pixels), while the box arrives in
	// widget-local pixels. Scale the box into device pixels so both sides
	// match (1:1 when devicePixelRatio == 1).
	const float devW = (float)gb_RenderDevice->GetSizeX();
	const float devH = (float)gb_RenderDevice->GetSizeY();
	if(devW <= 0.f || devH <= 0.f || widgetW_ <= 0 || widgetH_ <= 0)
		return false;
	const float kx = devW / (float)widgetW_;
	const float ky = devH / (float)widgetH_;
	const float xA = (float)std::min(x0, x1) * kx;
	const float xB = (float)std::max(x0, x1) * kx;
	const float yA = (float)std::min(y0, y1) * ky;
	const float yB = (float)std::max(y0, y1) * ky;

	bool changed = false;
	PlayerVect::const_iterator pi;
	FOR_EACH(universe()->Players, pi){
		const UnitList& units = (*pi)->units();
		UnitList::const_iterator it;
		FOR_EACH(units, it){
			UnitBase* unit = *it;
			if(!unit || unit->auxiliary() || !unit->alive())
				continue;
			// position() is the unit's pose translation, a world Vect3f.
			const Vect3f& world = unit->position();
			Vect3f view, screen;
			camera_->ConvertorWorldToViewPort(&world, &view, &screen);
			const bool inside = screen.x >= (float)xA && screen.x <= (float)xB
			                 && screen.y >= (float)yA && screen.y <= (float)yB;
			if(inside && !unit->selected()){
				unit->setSelected(true);
				changed = true;
			}
			else if(!inside && unit->selected()){
				unit->setSelected(false);
				changed = true;
			}
		}
	}
	return changed;
}

void EngineViewport::deleteSelectedObjects()
{
	if(!ownedUniverse_ || !sourceManager)
		return;
	sourceManager->deleteSelected();
	universe()->deleteSelected();
	if(cameraManager)
		cameraManager->deleteSelected();
}

