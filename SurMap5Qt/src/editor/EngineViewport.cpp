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
#include "Render/3dx/Node3DX.h"          // cObject3dx (model state debug)
#include "Render/3dx/Simply3dx.h"        // cSimply3dx (environment models)
#include "Game/GameOptions.h"            // GameOptions::filterBaseGraphOptions (Universe ctor calls it)
#include "UserInterface/UI_Render.h"     // UI_Render::create (SurMap5/SurMap5.cpp prelude)
#include "UserInterface/UI_GlobalAttributes.h" // UI_GlobalAttributes (loadAllLibraries dep)
#include "Util/EffectContainer.h"        // EffectContainer::setTexturesPath (effect texture root)

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

		// [SurMap5Qt debug] one-shot: where the models are vs what the camera
		// sees, after the stream drain and the hide-distance clear above.
		static int dbgOnce = 0;
		if(dbgOnce < 3){
			++dbgOnce;
			// [SurMap5Qt debug] walk the SCENE's 3dx objects (not just player units):
			// buildings/vegetation may live outside the player lists. Show position,
			// distance from the camera, visibility and screen projection so a class of
			// objects that PreDraw rejects (or the camera never reaches) shows up.
			std::vector<cObject3dx*> sceneObjs;
			scene_->GetAllObject3dx(sceneObjs);
			int shown = 0;
			for(size_t i = 0; i < sceneObjs.size() && shown < 12; ++i){
				cObject3dx* m = sceneObjs[i];
				if(!m)
					continue;
				const MatXf& mp = m->GetPosition();
				const Vect3f ceye = camera_->GetPos();
				const float d2 = ceye.distance2(mp.trans());
				eTestVisible vis = camera_->TestVisible(mp, Vect3f(-1,-1,-1), Vect3f(1,1,1));
				Vect3f pv, pe;
				const Vect3f origin = mp.trans();
				camera_->ConvertorWorldToViewPort(&origin, &pv, &pe);
				const Vect2f& zplane = camera_->GetZPlane();
				const float terrainZ = vMap.getZf((int)mp.trans().x, (int)mp.trans().y);
				fprintf(stderr,
					"[dbg3d] mpos=(%.0f,%.0f,%.0f) terrZ=%.0f d2=%.0f vis=%d scale=%.2f | cam=(%.0f,%.0f,%.0f) | scr=(%.0f,%.0f) viewZ=%.0f zNear=%.0f zFar=%.0f\n",
					mp.trans().x, mp.trans().y, mp.trans().z, terrainZ, d2, (int)vis, m->GetScale(),
					ceye.x, ceye.y, ceye.z,
					pe.x, pe.y, pv.z, zplane.x, zplane.y);
				++shown;
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

		// [SurMap5Qt debug] draw the loaded world once in wireframe (after the
		// first world load — the pre-world empty frame is useless): if the unit
		// models show up as line cages over the terrain, the vertex transforms
		// are fine and the problem is in the solid fill (materials/textures);
		// if they stay invisible the geometry never reaches the screen.
		static bool dbgWireArmed = true;
		if(dbgWireArmed && worldLoaded_){
			dbgWireArmed = false;
			gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_WIREFRAME);
			scene_->Draw(camera_);
			gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);
			fprintf(stderr, "[dbgwire] first loaded-world frame drawn in wireframe\n"); fflush(stderr);
		}
		else
			scene_->Draw(camera_);

		// [SurMap5Qt debug] periodic scene stats (every 300th frame after the
		// first three): how many 3dx objects the scene grid holds and how many
		// polygons came out of Draw — a steady-state world whose objPolys
		// collapsed to near zero after the load frames means something (the
		// editor's graphQuant / Quant path) is hiding the objects post-load.
		static int dbgScene = 0;
		++dbgScene;
		if(dbgScene <= 3 || (dbgScene % 300) == 0){
			std::vector<cObject3dx*> objs;
			scene_->GetAllObject3dx(objs);
			int alive = 0;
			int ignored = 0;
			int attached = 0;
			int deleted = 0;
			for(size_t i = 0; i < objs.size(); ++i){
				if(objs[i]){
					++alive;
					if(objs[i]->getAttribute(ATTRUNKOBJ_IGNORE)) ++ignored;
					if(objs[i]->getAttribute(ATTRUNKOBJ_ATTACHED)) ++attached;
					if(objs[i]->getAttribute(ATTRUNKOBJ_DELETED)) ++deleted;
				}
			}
			const int objPolys = gb_RenderDevice->NumberPolygon;
			const int tilePolys = gb_RenderDevice->GetDrawNumberTilemapPolygon();
			fprintf(stderr,
				"[dbgscene] frame=%d scene 3dx objects=%zu alive=%d ignored=%d deleted=%d attached=%d | objPolys=%d tilePolys=%d dips=%d\n",
				dbgScene, objs.size(), alive, ignored, deleted, attached,
				objPolys, tilePolys,
				gb_RenderDevice->GetDrawNumberObjects());
			fflush(stderr);
		}

		// [SurMap5Qt debug] marker crosses at live unit positions, drawn right
		// after the scene — confirms where the engine believes units are.
		if(ownedUniverse_){
			const float s = 40.f;
			Color4c col(255, 255, 0, 255);
			int drawn = 0;
			PlayerVect::const_iterator pi3;
			FOR_EACH(universe()->Players, pi3){
				const UnitList& units3 = (*pi3)->units();
				UnitList::const_iterator it3;
				FOR_EACH(units3, it3){
					UnitBase* u = *it3;
					if(!u || u->auxiliary() || !u->alive())
						continue;
					const Vect3f& p = u->position();
					const Vect3f pz(p.x, p.y, p.z + 60.f);   // a little above ground
					gb_RenderDevice->DrawLine(pz + Vect3f(-s, 0, 0), pz + Vect3f(s, 0, 0), col);
					gb_RenderDevice->DrawLine(pz + Vect3f(0, -s, 0), pz + Vect3f(0, s, 0), col);
					gb_RenderDevice->DrawLine(pz + Vect3f(0, 0, -s), pz + Vect3f(0, 0, s), col);
					if(++drawn >= 200)
						break;
				}
				if(drawn >= 200)
					break;
			}
		}
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
	// filters will drive later.
	if(ownedUniverse_)
		universe()->graphQuant(0.0f);

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