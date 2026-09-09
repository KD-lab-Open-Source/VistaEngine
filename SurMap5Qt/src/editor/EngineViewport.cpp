// EngineViewport.cpp — see header. Compiled with the engine's flags.

#include "EngineViewport.h"

// The engine's headers assume the old StdAfx preamble: `using namespace std`,
// <vector>/<string> (IRenderDevice.h names `vector`/`string` unqualified),
// `xassert` (XLibs.Net/XUtil/xutil.h) and `FOR_EACH` (Util/XTL/my_STL.h) —
// engine headers call all of them.
#include <vector>
#include <string>
#include <climits>
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
#include "Util/XMath/xmath.h"        // MatXf/Mat3f/Mat2f + X_AXIS/Y_AXIS/Z_AXIS
#include "Render/SDLRenderDevice.h"
#include "Terra/VMAP.H"              // vMap (load/create, H_SIZE/V_SIZE)
#include "Terra/TerrainType.h"       // TerrainTypeDescriptor (surface names)
#include "Environment/SourceManager.h"  // sourceManager (sources, anchors)
#include "Environment/SourceBase.h"     // SourceBase::label()
#include "Environment/Anchor.h"         // Anchor (Environment owns the type)
#include "Environment/Environment.h"    // environment (Environment tab data)
#include "Water/SkyObject.h"            // EnvironmentTime (GetCurFoneColor), cSkyObj
#include "Water/Waves.h"                // cFixedWavesContainer (fixedWaves)
#include "Game/Universe.h"               // universe(), Players, worldPlayer
#include "Game/Player.h"                 // Player::units(), worldPlayer
#include "Game/CameraManager.h"          // cameraManager, splines()
#include "Game/RenderObjects.h"          // initScene/finitScene, terScene, cameraManager
#include "Util/DebugPrm.h"               // logicTimePeriod (the universe logic quant period)
#include "Network/NetPlayer.h"           // MissionDescription
#include "Serialization/Dictionary.h"    // TranslationManager (Universe ctor calls GameOptions::setTranslate)
#include "Serialization/XPrmArchive.h"   // XPrmIArchive (.spg loading)
#include "Units/UnitAttribute.h"         // AttributeBase (libraryKey/isEnvironment)
#include "Units/UnitEnvironment.h"       // UnitEnvironment (Environment tab filter)
#include "Units/EnvironmentSimple.h"    // UnitEnvironmentSimple (Environment tab filter)
#include "Units/BaseUnit.h"              // UnitBase, UnitList
#include "Units/BaseUniverseObject.h"    // BaseUniverseObject (world bridge visit)
#include "Units/GlobalAttributes.h"      // GlobalAttributes::showHeadNames (Heads library)
#include "Units/CommandsQueue.h"         // CommandColorManager (command colors)
#include "Util/Serialization/EnumDescriptor.h" // getEnumDescriptor (command colors)
#include "Render/3dx/Node3DX.h"          // cObject3dx (model state debug)
#include "Render/3dx/Simply3dx.h"        // cSimply3dx (environment models)
#include "Render/src/NParticle.h"        // cEffect::setVisibleRange (editor shows all effects)
#include "Game/GameOptions.h"            // GameOptions::filterBaseGraphOptions (Universe ctor calls it)
#include "UserInterface/UI_Render.h"     // UI_Render::create (SurMap5/SurMap5.cpp prelude)
#include "UserInterface/UI_GlobalAttributes.h" // UI_GlobalAttributes (loadAllLibraries dep)
#include "Util/EffectContainer.h"        // EffectContainer::setTexturesPath (effect texture root)
#include "Util/ZipConfig.h"              // ZipConfig::initArchives (pak archives)
#include "Util/SystemUtil.h"             // setLogicFp (FPU precision)
#include "Util/ConsoleWindow.h"          // ConsoleWindow::instance (console listener)
#include "Util/Win32/DebugSymbolManager.h" // DebugSymbolManager::create
#include "PropertyRows.h"                   // registerBuiltinPropertyRows (LibraryEditor core)
#include "PropertyArchive.h"                // PropertyOArchive/PropertyIArchive (LibraryEditor bridge)
#include "Serialization/LibrariesManager.h" // LibrariesManager (library lookup)
#include "Serialization/LibraryWrapper.h"   // EditorLibraryInterface (library element access)

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

// WorldBridge — the engine-side implementation of the engine-free IWorldBridge
// the tools talk to. It is a port of the SelectionUtil globals +
// universe/sourceManager/cameraManager access, but over the common
// BaseUniverseObject base (exactly what the original's UniverseObjectAction
// visited). Every method maps an EditorObjectId to a BaseUniverseObject*.
class WorldBridge : public IWorldBridge
{
public:
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

	float objectRadius(EditorObjectId id) override
	{
		BaseUniverseObject* obj = reinterpret_cast<BaseUniverseObject*>(id);
		return obj ? obj->radius() : 0.f;
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
		out.clear();
		EditorLibraryInterface* lib = LibrariesManager::instance().find(libraryName.c_str());
		if(!lib)
			return;
		const std::size_t count = lib->editorSize();
		for(std::size_t i = 0; i < count; ++i){
			const char* name = lib->editorElementName((int)i);
			if(name && name[0] != '\0')
				out.push_back(name);
		}
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
	bridge_ = new (std::nothrow) WorldBridge;

	// The LibraryEditor core: register the builtin property-row types
	// (string/bool/numeric) so PropertyOArchive can build rows for them.
	// Idempotent; safe to call once at startup.
	editor::registerBuiltinPropertyRows();

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
	}
	// terScene and cameraManager are owned by initScene/finitScene — the
	// editor keeps them across loads so reloading a world reuses the
	// same scene + camera. (SurMap5/GeneralView.cpp called createScene
	// once and reInitWorld on every world load.) Drop the references;
	// the teardown order matches SurMap5/VistaEngineContext.cpp.
	delete bridge_;
	bridge_ = nullptr;
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
	const Vect2f zPlane(30.0f, std::max(12000.0f, orbit_.distance * 3.0f));
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

	// Universe dtor releases the tile map (RELEASE(tileMap)) and clears
	// the engine globals (sourceManager, environment, cameraManager
	// splines, Players, pathFinder, soundEnvironmentManager_).
	if(ownedUniverse_){
		ownedUniverse_.reset();
	}
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
			gb_VisGeneric->SetLogicQuant(universe()->quantCounter() + 2);
			universe()->Quant();
			universe()->interpolationQuant();
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
		const Vect2f zPlane(30.0f, std::max(12000.0f, orbit_.distance * 3.0f));
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

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
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
		// Pan: move the orbit centre in the camera plane (CGeneralView's
		// RMB branch, minus the terrain-height coupling).
		const float dx = float(x - dragStartX_);
		const float dy = float(y - dragStartY_);
		// The pan scales with distance like the original.
		const float scale = max(orbit_.distance, 100.f) / 800.f;
		// Camera-plane basis: psi rotates the yaw; a screen delta maps to
		// world x/y through the yaw rotation.
		const float c = cosf(orbit_.psi);
		const float s = sinf(orbit_.psi);
		orbit_.px = dragStartPx_ - (dx * c - dy * s) * scale;
		orbit_.py = dragStartPy_ - (dx * s + dy * c) * scale;
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

	const int w = gb_RenderDevice->GetSizeX();
	const int h = gb_RenderDevice->GetSizeY();
	if(w <= 0 || h <= 0)
		return false;

	// CGeneralView::CoordScr2vMap normalized with the render-device size, origin
	// at the centre (the engine's screen-space convention).
	const Vect2f posIn((float)x / (float)w - 0.5f, (float)y / (float)h - 0.5f);

	// The frustum drawFrame uses (CGeneralView::graphQuant's camera set-up).
	const Vect2f center(0.5f, 0.5f);
	const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
	const Vect2f focus(orbit_.focus, orbit_.focus);
	const Vect2f zPlane(30.0f, std::max(12000.0f, orbit_.distance * 3.0f));
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

	// The orbit matrix, as CameraManager::quant builds it:
	//   R(theta,X)*R(fi,Y)*R(pi/2-psi,Z) translated by -position.
	// Position sits on the orbit sphere around the centre.
	Vect3f position(
		orbit_.px + orbit_.distance * sinf(orbit_.theta) * cosf(orbit_.psi),
		orbit_.py + orbit_.distance * sinf(orbit_.theta) * sinf(orbit_.psi),
		orbit_.pz + orbit_.distance * cosf(orbit_.theta));

	MatXf matrix = MatXf::ID;
	matrix.rot() = Mat3f(orbit_.theta, X_AXIS) * Mat3f(orbit_.fi, Y_AXIS) * Mat3f(M_PI_2 - orbit_.psi, Z_AXIS);
	matrix *= MatXf(Mat3f::ID, -position);
	setCameraPosition(camera_, matrix);
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
				out[n++] = strdup(buf);
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
			out[n++] = strdup(buf);
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
			out[n++] = strdup(buf);
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
			out[n++] = strdup(buf);
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
				out[n++] = strdup(buf);
			}
		}
		return n;
	}

	return 0;
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

	const int w = gb_RenderDevice->GetSizeX();
	const int h = gb_RenderDevice->GetSizeY();
	if(w <= 0 || h <= 0)
		return false;

	const Vect2f posIn((float)screenX / (float)w - 0.5f, (float)screenY / (float)h - 0.5f);

	// Re-apply the frustum (see screenPointToGround) then unproject the ray.
	const Vect2f center(0.5f, 0.5f);
	const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
	const Vect2f focus(orbit_.focus, orbit_.focus);
	const Vect2f zPlane(30.0f, std::max(12000.0f, orbit_.distance * 3.0f));
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

	const float width = (float)gb_RenderDevice->GetSizeX();
	const float height = (float)gb_RenderDevice->GetSizeY();
	if(width <= 0.f || height <= 0.f)
		return false;

	// SelectionUtil::selectByScreenRectangle normalized the box corners and
	// picked every unit whose 2D screen position (ConvertorWorldToViewPort)
	// falls inside. ConvertorWorldToViewPort returns the screen point in
	// device pixels (SurMap5/SelectionUtil.cpp:worldToScreen used round(e.x)
	// directly), so compare in pixels against the original box.
	const int xA = std::min(x0, x1);
	const int xB = std::max(x0, x1);
	const int yA = std::min(y0, y1);
	const int yB = std::max(y0, y1);

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