// EditorTool.h — the editor's tool base class, Qt port of CSurToolBase
// (SurMap5/SurToolAux.h).
//
// The original was an MFC dialog (CExtResizableDialog) that doubled as both the
// tool's behaviour and its panel; here the tool is plain behaviour. The panel
// (propertiesDock_) is a Qt widget the MainWindow swaps per tool in Phase 5.
//
// Deliberately engine-free: tools get map coordinates and screen points as
// plain values, and render their aux data through a callback (AuxPainter)
// instead of touching the renderer. This keeps every tool compilable in the
// Qt executable with no engine flags, exactly like the original's separation
// (tools lived in the MFC exe, the engine never saw them).
//
// The callback set mirrors CSurToolBase:
//   onTrackingMouse / onLMBDown / onLMBUp / onRMBDown / onRMBUp /
//   onKeyDown / onDelete / onDrawAuxData / quant
// minus the MFC dialog plumbing (DoDataExchange, OnInitDialog, ...).

#pragma once

#include <string>
#include <vector>

// Engine vectors live in the engine libs; the tools must not include engine
// headers, so coordinates are plain 3-component float triples (Vect3f is
// exactly that). Keep the field order (x,y,z) matching Vect3f so a cast is
// safe on the engine side if ever needed.
struct ToolVec3
{
	float x = 0.f, y = 0.f, z = 0.f;
};

// Screen coordinates (Vect2i).
struct ToolVec2
{
	int x = 0, y = 0;
};

// What an aux-draw callback may draw. Kept minimal: the tools in this phase
// only need a 2D screen-space overlay (cursor circle, selection box) and the
// transform axis. EngineViewport::drawFrame invokes the current tool's
// onDrawAuxData with a painter implementing this interface.
class ToolAuxPainter
{
public:
	virtual ~ToolAuxPainter() = default;

	// Screen-space overlay primitives (already in widget pixels).
	virtual void drawLine2D(const ToolVec2& a, const ToolVec2& b, unsigned colorARGB) = 0;
	virtual void drawRect2D(const ToolVec2& a, const ToolVec2& b, unsigned colorARGB) = 0;
	virtual void drawCircle2D(const ToolVec2& center, int radius, unsigned colorARGB) = 0;
};

// IWorldBridge is the engine-free surface the tools use; see its definition
// after EditorTool below. Forward-declared so EditorTool can hold a pointer.
class IWorldBridge;

class EditorTool
{
public:
	virtual ~EditorTool() = default;

	// The world bridge installed by ToolManager (render view wires it in on
	// construction). Tools that don't touch the world ignore it.
	void setWorldBridge(IWorldBridge* bridge) { bridge_ = bridge; }
	IWorldBridge* bridge() const { return bridge_; }

	// The tool's display name (the tools tree label).
	virtual const char* name() const = 0;

	// --- Input (world/screen from the view) ---

	// Mouse tracking: worldCoord is the ground point under the cursor.
	virtual bool onTrackingMouse(const ToolVec3& worldCoord, const ToolVec2& screenCoord) { (void)worldCoord; (void)screenCoord; return false; }
	virtual bool onLMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)    { (void)worldCoord; (void)screenCoord; return false; }
	virtual bool onLMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord)      { (void)worldCoord; (void)screenCoord; return false; }
	virtual bool onRMBDown(const ToolVec3& worldCoord, const ToolVec2& screenCoord)    { (void)worldCoord; (void)screenCoord; return false; }
	virtual bool onRMBUp(const ToolVec3& worldCoord, const ToolVec2& screenCoord)      { (void)worldCoord; (void)screenCoord; return false; }

	// Key: keyCode is a platform-independent code (Qt::Key when dispatched
	// from Qt; the tool only interprets the ones it cares about).
	virtual bool onKeyDown(unsigned keyCode, bool shift, bool control, bool alt) { (void)keyCode; (void)shift; (void)control; (void)alt; return false; }

	// Delete key while this tool is active (onDelete in the original).
	virtual bool onDelete() { return false; }

	// Called when the selection changes elsewhere (onSelectionChanged).
	virtual void onSelectionChanged() {}

	// --- Per frame ---

	// Called from the view's tick; tools with held state advance here.
	virtual void quant(float /*dt*/) {}

	// Draw the tool's overlay. Return true if anything was drawn.
	virtual bool onDrawAuxData(ToolAuxPainter& painter) { (void)painter; return false; }

	// --- Lifecycle ---

	// Tool becomes/ceases to be the current tool.
	virtual void onActivate() {}
	virtual void onDeactivate() {}

private:
	IWorldBridge* bridge_ = nullptr;
};

// WorldBridge — the engine-free bridge a tool uses to reach the engine side.
//
// The Qt tools must not include engine headers, so the engine-facing surface
// EngineViewport exposes is exactly this interface (a port of the global
// SelectionUtil helpers + universe/sourceManager/cameraManager access). The
// concrete implementation lives engine-side (EngineViewport::WorldBridge);
// RenderViewWidget hands a pointer to ToolManager, which installs it on every
// tool. Tools that never touch the world keep bridge() == nullptr.
//
// This mirrors how CSurToolBase reached the engine globals directly in MFC:
// manipulating a generic object id (a port of BaseUniverseObject) instead of
// the engine's UnitBase/SourceBase/CameraSpline directly.

// An opaque handle to one world object (a unit, source, anchor or camera
// spline). Engine-side it maps to BaseUniverseObject*. Plain int-sized so
// tools can hold it across a selection without engine types.
using EditorObjectId = intptr_t;

// Pose of an editor object: orientation (x,y,z,w quaternion) + position.
// Matches Se3f field-for-field so the engine side can convert freely.
struct EditorPose
{
	// Quaternion (w,x,y,z — Se3f uses QuatF(x,y,z,w), so keep field order
	// and convert by name on the engine side).
	float ox = 0.f, oy = 0.f, oz = 0.f, ow = 1.f;
	ToolVec3 pos;
};

// A visitor over the currently selected objects. Implemented by the tool that
// wants to act on the selection (port of UniverseObjectAction). The object id
// is valid only for the duration of the visit() call.
class IEditorObjectVisitor
{
public:
	virtual ~IEditorObjectVisitor() = default;
	virtual void visit(EditorObjectId id) = 0;
};

class IWorldBridge
{
public:
	virtual ~IWorldBridge() = default;

	// Ports of SelectionUtil globals.
	virtual void forEachSelected(IEditorObjectVisitor& visitor) = 0;
	virtual void deselectAll() = 0;
	virtual void deleteSelected() = 0;

	// Hover/selection helpers used by the tools.
	// Returns the topmost object under the normalized screen point (-0.5..0.5),
	// or kNoObject when nothing is hit.
	virtual EditorObjectId hoverAt(float screenX, float screenY) = 0;
	// Select the whole world by a screen box (pixels, widget space).
	virtual void selectInRect(int x0, int y0, int x1, int y1, bool add) = 0;

	// Pose read/write on a single object (base does not have setPosition;
	// everything goes through setPose).
	virtual EditorPose objectPose(EditorObjectId id) = 0;
	virtual void setObjectPose(EditorObjectId id, const EditorPose& pose, bool init) = 0;
	virtual float objectRadius(EditorObjectId id) = 0;
	virtual void setObjectRadius(EditorObjectId id, float radius) = 0;

	// Terrain height + a ray-cast of a widget pixel to the ground (the
	// tools' screenPointToGround / projectScreenPointOnPlane ports).
	virtual float terrainHeight(float x, float y) = 0;
	virtual bool screenPointToGround(int sx, int sy, ToolVec3& out) = 0;

	// Terrain generation (SurToolGeoNet::onOperationOnMap -> geoGeneration).
	// Applies a geo-net hill generation centred at (x, y) over a brush-sized
	// area. `height` is the target voxel height, `noise` the noise level,
	// `mesh` the grid density. Returns false when no world is loaded.
	virtual bool applyGeoNet(float x, float y, float brushRadius,
	                         int height, int noise, int mesh) = 0;

	// Re-render the whole world (SurToolGeoTx::onOperationOnMap ->
	// vMap.WorldRender). Returns false when no world is loaded.
	virtual bool worldRender() = 0;

	// Camera splines (CameraDialog). Names of the saved camera paths.
	virtual void cameraNames(std::vector<std::string>& out) = 0;
	// Create a new (empty) camera spline with the given name.
	virtual bool createCamera(const std::string& name) = 0;
	// Delete the camera spline with the given name.
	virtual bool deleteCamera(const std::string& name) = 0;
	// Replay the camera spline with the given name (loadPath + startReplayPath).
	virtual bool playCamera(const std::string& name) = 0;

	// Fixed waves (WaveDialog -> environment->fixedWaves()). Names of the
	// wave lines.
	virtual void waveNames(std::vector<std::string>& out) = 0;
	// Create a new wave line with the given name.
	virtual bool createWave(const std::string& name) = 0;
	// Remove the wave line with the given name.
	virtual bool removeWave(const std::string& name) = 0;
	// Apply the wave-line parameters (CWaveDlg::OnBnClickedApply).
	virtual bool applyWave(const std::string& name, float distance, float speed,
	                       float sizeMin, float sizeMax, float generationTime,
	                       bool invert) = 0;

	// Time of day (TimeSliderDialog -> Environment::environmentTime()).
	// Returns the current time in hours (0..24), or -1 when no environment.
	virtual float timeOfDay() = 0;
	// Set the time of day in hours (0..24). Returns false when no environment.
	virtual bool setTimeOfDay(float hours) = 0;

	// --- Libraries (Libraries menu) ---

	// Heads (GlobalAttributes::showHeadNames): the list of head file names.
	virtual void headNames(std::vector<std::string>& out) = 0;
	// Replace the whole head list (GlobalAttributes::showHeadNames + save).
	virtual bool setHeadNames(const std::vector<std::string>& names) = 0;

	// Terrain type names (TerrainTypeDescriptor): 16 entries of (name, color).
	// Colors are 0xRRGGBB.
	virtual void terrainTypeNames(std::vector<std::string>& names,
	                              std::vector<unsigned>& colors) = 0;
	// Replace the terrain type names/colors (TerrainTypeDescriptor + save).
	virtual bool setTerrainTypeNames(const std::vector<std::string>& names,
	                                 const std::vector<unsigned>& colors) = 0;

	// Command colors (CommandColorManager): colors indexed by command id.
	// Returns the command ids + their colors (0xRRGGBB).
	virtual void commandColors(std::vector<int>& ids,
	                           std::vector<unsigned>& colors) = 0;
	// Set one command's color (CommandColorManager + save).
	virtual bool setCommandColor(int id, unsigned color) = 0;

	// Static sentinel representing "no object".
	static constexpr EditorObjectId kNoObject = 0;
};
