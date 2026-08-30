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
#include "Terra/VMAP.H"              // vMap (load/create, H_SIZE/V_SIZE)

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
}

EngineViewport::EngineViewport() = default;

EngineViewport::~EngineViewport()
{
	done();
}

bool EngineViewport::init(int width, int height)
{
	if(inited_)
		return true;
	if(!nativeWindow_)
		return false;

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

	renderWindow_ = gb_RenderDevice->createRenderWindow((HWND)nativeWindow_);
	if(!renderWindow_)
		return false;
	gb_RenderDevice->selectRenderWindow(renderWindow_);

	// The scene graph + a camera off it, as initScene() (Game/RenderObjects.cpp)
	// does. No world is loaded yet — the scene holds the engine's globals and
	// the camera renders whatever the world load (Phase 3b) will add.
	scene_ = gb_VisGeneric->CreateScene();
	if(!scene_)
		return false;
	camera_ = scene_->CreateCamera();
	if(!camera_)
		return false;

	applyCamera();
	inited_ = true;
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
	// Camera is owned by the scene (CreateCamera); releasing the scene drops it.
	if(scene_)
		RELEASE(scene_);
	scene_ = nullptr;
	camera_ = nullptr;
	renderWindow_ = nullptr;
	inited_ = false;
}

bool EngineViewport::loadWorld(const char* worldsDir, const char* worldName)
{
	if(!inited_ || !scene_)
		return false;

	doneWorld();

	// CMainFrame::OnFileOpen: vMap.load(world, true) + reInitWorld (minus the
	// Universe, which is the Game exe's). vMap::load reads world.cls from
	// <worldsDir>\<worldName>\ and builds the terrain buffers.
	vMap.setWorldsDir(worldsDir);
	if(!vMap.load(worldName, /*flag_useTryColorBuffer=*/true))
		return false;

	// Universe's ctor created the terrain tile map: terScene->CreateMap().
	// cScene::CreateMap uses vMap.H_SIZE/V_SIZE and registers with vMap; the
	// map becomes the scene's tile map and draws with cScene::Draw.
	scene_->CreateMap(true);

	// reInitWorld's camera reset: centre the orbit on the map, then let the
	// camera keep its height (createScene used the map centre too).
	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 128.0f;
	orbit_.distance = 512.f;
	orbit_.psi = 0.f;
	orbit_.theta = 0.65f;
	applyCamera();

	worldLoaded_ = true;
	return true;
}

void EngineViewport::doneWorld()
{
	if(!worldLoaded_)
		return;

	// doneScene's tile map release: cScene::~cScene releases tileMap_, and
	// cTileMap unregisters from vMap. Drop the whole scene and rebuild it —
	// the camera is owned by the scene, so it comes back too.
	if(scene_)
		RELEASE(scene_);
	scene_ = gb_VisGeneric->CreateScene();
	camera_ = scene_ ? scene_->CreateCamera() : nullptr;

	vMap.releaseWorld();
	worldLoaded_ = false;
	applyCamera();
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

	// Same terrain setup as loadWorld.
	scene_->CreateMap(true);

	orbit_.px = vMap.H_SIZE * 0.5f;
	orbit_.py = vMap.V_SIZE * 0.5f;
	orbit_.pz = 128.0f;
	orbit_.distance = 512.f;
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

void EngineViewport::tick(float /*dt*/)
{
	// The orbit state is applied immediately on input (like CGeneralView's
	// WindowProc, which sets cameraManager->setCoordinate per event), so there
	// is nothing to integrate per-frame yet. Phase 3b (world + animation) will
	// advance the scene here.
	applyCamera();
}

void EngineViewport::drawFrame()
{
	if(!inited_ || !gb_RenderDevice || !camera_)
		return;

	// The editor viewport's clear (CGeneralView used the environment's fone
	// colour; with no world loaded, a neutral slate).
	gb_RenderDevice->Fill(32, 48, 64, 255);
	gb_RenderDevice->BeginScene();

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
		const Vect2f zPlane(30.0f, 12000.0f);
		camera_->SetFrustum(&center, &clip, &focus, &zPlane);
		scene_->Draw(camera_);
	}

	// CGeneralView::graphQuant drew the grid after terScene->Draw().
	drawGrid();

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
	const Vect2f zPlane(30.0f, 12000.0f);
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
		orbit_.px + orbit_.distance * cosf(orbit_.theta) * cosf(orbit_.psi),
		orbit_.py + orbit_.distance * cosf(orbit_.theta) * sinf(orbit_.psi),
		orbit_.pz + orbit_.distance * sinf(orbit_.theta));

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
