// EngineViewport.h — the engine-touching half of the editor's 3D viewport.
//
// This file is compiled in the EditorEngine static library, which carries the
// engine's compile flags (EngineIncludes: _HAS_STD_BYTE=0, /FIwindows.h on
// Windows). Qt translation units must never include engine headers, so the
// engine surface the Qt side needs is exactly what this class exposes:
//   init/done     the SDL GPU device + this viewport's render window
//   resize        tell the render window its size changed
//   tick          the per-frame update (camera input state, animation)
//   drawFrame     BeginScene/EndScene/Flush — one rendered frame
//   input         mouse wheel / buttons / move / keys, mapped from Qt events
//   nativeWindow() the OS handle to wrap in the SDL foreign window
//
// Phase 3: an orbital camera (psi/theta/distance/fi + position) drives a
// Render::Camera directly — the CameraManager from Game pulls in the whole
// game module cascade, so the editor keeps its own lightweight orbit control
// (the same math CameraManager::quant uses) until the world load lands.
#pragma once

#include "MapChangeParams.h"

#include <memory>

// Forward declarations only — the full engine headers stay in the .cpp.
class cInterfaceRenderDevice;
class cRenderWindow;
class Camera;
class cScene;
class MissionDescription;
class Universe;

class EngineViewport
{
public:
	struct CameraState
	{
		float centerX = 0.f, centerY = 0.f, centerZ = 0.f;
		float distance = 512.f;
		float yaw = 0.f, pitch = 0.f, roll = 0.f;
	};

	EngineViewport();
	~EngineViewport();

	// The OS window handle (HWND on Windows) this viewport wraps.
	void* nativeWindow() const { return nativeWindow_; }
	void setNativeWindow(void* hwnd) { nativeWindow_ = hwnd; }

	// Create the render device once, then the render window on nativeWindow_.
	// Returns false if the device or window could not be created.
	bool init(int width, int height);
	void done();

	// The render window's size changed (Qt resize event).
	void resize();

	// --- World (Phase 3b) ---

	// Load a world by name from the given worlds directory (CMainFrame's
	// OnFileOpen: vMap.load + reInitWorld, minus Universe). Returns false if
	// the world data could not be loaded.
	bool loadWorld(const char* worldsDir, const char* worldName);
	// Create a default world by name (CMainFrame's OnFileNew: vMap.create).
	bool createWorld(const char* worldsDir, const char* worldName);
	// Release the loaded world (doneScene's tile map release).
	void doneWorld();
	bool worldLoaded() const { return worldLoaded_; }

	// Rebuild the terrain scene from the in-memory world after the terrain
	// data changed in place (CMainFrame's view_->reInitWorld: drop the tile
	// map, re-create it from the current vMap buffers). The world stays
	// loaded; only the render-side map is rebuilt.
	bool reinitWorld();

	// The loaded world's name (vMap.getWorldName), empty when none.
	const char* worldName() const;

	// Per-frame update: advance the camera from the held input state.
	// dt is seconds. Called from the editor's ~60 Hz loop.
	void tick(float dt);

	// Present one frame: clear, camera, whatever the scene holds.
	void drawFrame();

	// --- Input (mapped from Qt events by RenderViewWidget) ---
	// wheelDelta: signed notch count (+up / -down). modifiers: 1=Shift, 2=Ctrl, 4=Alt.
	void mouseWheel(int wheelDelta, int modifiers);
	// button: 0=none, 1=left, 2=middle, 4=right. pressed=true on press, false on release.
	// x,y are widget-local pixels.
	void mouseButton(int button, bool pressed, int x, int y);
	void mouseMove(int x, int y);

	// Ray-cast the given widget-local pixel into the world and write the terrain
	// intersection point. Port of CGeneralView::CoordScr2vMap (SurMap5/GeneralView.cpp:
	//405): normalize to (x/sizeX-0.5, y/sizeY-0.5), camera_->GetWorldRay, scene_->
	//TraceDir. Returns false when no world is loaded or the ray misses the terrain
	// (out stays untouched). x,y are widget-local pixels.
	bool screenPointToGround(int x, int y, float& outX, float& outY, float& outZ);

	// --- World data (U4 dialogs) ---

	// The loaded map's grid size in vertices (vMap.H_SIZE/V_SIZE). Returns
	// false when no world is loaded.
	bool mapSize(int& hSize, int& vSize) const;

	// The map's creation parameters the properties dialog shows (vMap.H_SIZE_
	// POWER/V_SIZE_POWER, createWorldMetod, initialHeight). Returns false when
	// no world is loaded.
	bool mapCreationParams(int& hSizePower, int& vSizePower,
	                       int& createWorldMetod, int& initialHeight) const;

	// A 256-bin histogram of the loaded world's vertex heights (voxel units,
	// binned over MAX_VX_HEIGHT+1) plus the min/max heights. Port of
	// world2Histogram (SurMap5/DlgChangeTotalWorldHeight.cpp). `out` must be a
	// 256-int array; each bin holds the sqrt-scaled column height the dialog
	// draws. Returns false when no world is loaded.
	bool worldHeightHistogram(int out[256], int& minVx, int& maxVx);

	// Apply changeTotalWorldParam(deltaVx, kScale, params) to the loaded world
	// (vMap's terrain height transform + optional resize; vmap4vi.cpp:178).
	// Returns the base height (min height in world units), like the original.
	// Only valid when a world is loaded.
	float changeTotalWorldParam(int deltaVx, float kScale, const Editor::MapChangeParams& params);

	// --- Texture statistics (U5) ---

	// One row of the texture-statistics dialog (DlgTexturesStatistics): name +
	// size in bytes (cTexture::CalcTextureSize).
	struct TextureStat {
		const char* name = nullptr;
		int size = 0;
	};

	// The loaded texture library's rows (GetTexLibrary: Render/src/TexLibrary.h).
	// `out` receives <count> entries; the dialog reads them while this call is
	// alive (the names point into the texture library). Returns the count, and
	// writes the summed size to `totalSize`.
	int textureStatistics(const TextureStat*& out, int& totalSize);

	// --- Minimap (U6) ---

	// The minimap's pixel size (vMap.H_SIZE/16 x V_SIZE/16 — the same
	// reduction OnFileSaveminimaptoworld used; SurMap5/MainFrame.cpp:1182).
	// Returns false when no world is loaded.
	bool minimapSize(int& sizex, int& sizey) const;

	// Fill `out` (sizex*sizey entries) with the minimap's averaged terrain
	// colors — one 0xFF000000|RGB pixel per entry. Port of vMap::saveMiniMap
	// (Terra/VMAP.CPP:917) minus the TGA write, so the Qt side can show the
	// map without touching the engine's file formats. Returns false on failure.
	bool minimapPixels(unsigned long* out, int sizex, int sizey);

	// OnFileSaveminimaptoworld: vMap.saveMiniMap(H_SIZE/16, V_SIZE/16) — writes
	// map.tga into the world's directory. Returns false when no world is loaded.
	bool saveMiniMapToFile();

	// The orbit centre in world coordinates (the camera marker the minimap
	// panel draws over the map). Returns false when no world is loaded.
	bool cameraCenter(float& x, float& y) const;

	// Center the orbit on a world point (a minimap click; CMiniMapWindow routed
	// it through minimap().pressEvent -> cameraToEvent).
	void setCameraCenter(float x, float y);

	// --- World save (U7) ---

	// CMainFrame::save (SurMap5/MainFrame.cpp:1076): vMap.save(worldName) —
	// persists the world's terrain buffers to <worldsDir>\<worldName>\ (vMap
	// creates the directory if missing). Returns false when no world is loaded.
	// The world name passed to vMap.load/create is stored in vMap, so the
	// caller only supplies it for Save As.
	bool saveWorld(const char* worldName);
	// Save under the world's current name (CMainFrame::OnFileSave).
	bool saveWorld();
	// Undo/redo state and operations from vrtMap's existing dispatcher.
	bool canUndo() const;
	bool canRedo() const;
	bool undo();
	bool redo();
	// --- Rolling border (D1) ---

	// CMainFrame::OnEditRollingborder: vMap.autoLace(laceH, angle) — laces the
	// world's edge relief (DlgBorderRolling's borderHeight*VOXEL_MULTIPLIER and
	// borderAngle*M_PI/180). Returns false when no world is loaded.
	bool autoLace(int laceHeightVoxels, float angleRadians);

	// --- Terrain maintenance (D3) ---

	// CMainFrame::OnEditRebuildworld: vMap.rebuild() — re-derives the world's
	// caches from the source raster (bitmap/bitGen dispatchers are cleared and
	// the surface is recreated), then re-creates the render-side tile map.
	// Returns false when no world is loaded.
	bool rebuildWorld();

	// CGeneralView::updateSurface (SurMap5/GeneralView.cpp:942):
	// vMap.recalcArea2Grid + regRender over the whole map — marks every
	// tile's grid/region dirty so the renderer regenerates them.
	bool updateSurface();

	// OnDebugShowpalettetexture: vMap.toShowTryColorDamTexture(!current) —
	// toggles the "try-color dam texture" debug view and re-renders the world.
	// Returns the new state, or -1 when no world is loaded.
	int toggleTryColorDamTexture();
	// --- Camera default (U7) ---

	// The current orbit centre/distance/theta, for persisting the camera
	// default (GlobalAttributes::setCameraCoordinate reads only distance +
	// theta; the orbit centre is the map centre anyway).
	void orbitCamera(float& distance, float& theta) const;
	// Apply a saved camera default (distance + theta) to the orbit; the
	// centre stays the map centre, as the original's camera init did.
	void setOrbitCamera(float distance, float theta);
	void cameraState(CameraState& state) const;
	void setCameraState(const CameraState& state);
	void fitCameraToWorld();

	// --- Grid visibility (U7) ---

	// Show/hide the editor grid (surMapOptions.enableGrid_). True by default
	// (the original loaded the persisted option; the Qt editor has no
	// SurMapOptions yet, and the View menu starts checked).
	void setGridVisible(bool visible);
	bool gridVisible() const { return gridVisible_; }

	// --- Status bar (U8) ---

	// Terrain info under a world point for the status bar's panes
	// (CGeneralView::UpdateStatusBar): the surface-kind name (TerrainType
	// descriptor's nameAlt of the surkind at the point), the exact height in
	// voxels, the approximate grid height and the water height at the point.
	// Returns false when no world is loaded or the point is off-map.
	bool terrainInfoAt(float x, float y, char* surfName, int surfNameSize,
	                   int& altVox, int& approxAlt, int& waterZ) const;

	// --- Object list (Objects Manager) ---

	// Tab id for objectList (matches ObjectsManagerTab: 0=sources, 1=environment,
	// 2=units, 3=cameras, 4=anchors).
	enum class ObjectTab { Sources = 0, Environment = 1, Units = 2, Cameras = 3, Anchors = 4 };

	// Fill `out` with at most `maxCount` display labels for the given tab of
	// the objects manager tree. Each label is null-terminated UTF-8 (or
	// Latin-1, like the original). The strings are heap-allocated; the
	// caller owns them and should `free()` each. Returns the actual count.
	int objectList(ObjectTab tab, char** out, int maxCount);

	// --- Object selection (SurMap5/SelectionUtil.cpp) ---

	// How many world objects (units, sources, anchors, camera splines) are
	// currently selected. 0 when no world is loaded.
	int selectedObjectsCount();

	// Deselect every object (sourceManager->deselectAll + universe()->deselectAll
	// + cameraManager splines) — SelectionUtil::deselectAll.
	void deselectAllObjects();

	// Select the topmost world object under the widget pixel (unitHoverAll:
	// cast a camera ray through the point, pick the nearest unit whose
	// intersect() the ray hits). Mode: 0 = replace (deselect all, then select),
	// 1 = toggle (ctrl), 2 = add (shift). Returns true when an object was hit.
	bool selectObjectAt(int screenX, int screenY, int mode);

	// Select every object whose screen box intersects [x0,y0]-[x1,y1]
	// (SelectionUtil::selectByScreenRectangle). Returns true when the
	// selection changed.
	bool selectObjectsInRect(int x0, int y0, int x1, int y1);

	// Delete the selected objects (SelectionUtil::deleteSelectedUniverseObjects).
	void deleteSelectedObjects();

	// The number of world objects the given tab lists (0 when no world):
	// convenience for empty-tree checks.
	int objectCount(ObjectTab tab);

	bool inited() const { return inited_; }

private:
	// The camera's orbit state (CameraManager::CameraCoordinate equivalent).
	struct Orbit
	{
		float psi = 0.f;       // yaw, radians
		float theta = 0.f;     // pitch, radians
		float fi = 0.f;        // roll, radians
		float distance = 512.f;
		float focus = 0.8f;    // HardwareCameraFocus
		float px = 0.f, py = 0.f, pz = 0.f;   // orbit centre
	};
	void applyCamera();

	// CGeneralView::drawGrid — the editor's terrain grid.
	void drawGrid();

	void*                nativeWindow_ = nullptr;
	cInterfaceRenderDevice* renderDevice_ = nullptr;
	cRenderWindow*       renderWindow_ = nullptr;
	cScene*              scene_ = nullptr;     // == terScene once initScene() ran
	Camera*              camera_ = nullptr;    // == cameraManager->GetCamera() once initScene() ran
	Orbit                orbit_;
	// The Universe + MissionDescription the editor built for the current
	// world (CMainFrame::reInitWorld's `new Universe(mission, ia)`). The
	// unique_ptr's dtor calls `delete universe()`, which is the original
	// teardown order (CGeneralView::doneUniverse). Held as members so
	// reloading a world drops the old one before building the next.
	std::unique_ptr<Universe> ownedUniverse_;
	std::unique_ptr<MissionDescription> ownedMission_;
	bool                 inited_ = false;
	bool                 worldLoaded_ = false;
	bool                 gridVisible_ = true;   // surMapOptions.enableGrid_ (U7)
	// loadAllLibraries() (the SurMap5 initRenderDevice prelude) ran — the
	// UI_* + attribute libraries are loaded, so the first Universe ctor's
	// UI_Dispatcher::instance() has its dependencies ready.
	bool                 librariesLoaded_ = false;

	// Mouse capture state (port of CGeneralView::WindowProc's statics).
	bool   mouseMiddle_ = false;
	bool   mouseLeft_ = false;
	bool   mouseRight_ = false;
	int    dragStartX_ = 0, dragStartY_ = 0;
	float  dragStartPsi_ = 0.f, dragStartTheta_ = 0.f;
	float  dragStartPx_ = 0.f, dragStartPy_ = 0.f, dragStartPz_ = 0.f;
};
