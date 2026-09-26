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

// The PropertyRow model is engine-free (std types only), so the engine-free
// bridge can reference it. The Qt LibraryEditor renders these rows; the
// engine side builds/consumes them via PropertyOArchive/PropertyIArchive.
#include "PropertyRow.h"

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

// Environment placement parameters (SurTool3DM / CSurToolEnvironment dialog).
// Engine-free snapshot of the sliders/checkboxes; the bridge turns this into a
// UnitEnvironment on placement and into a scene model for the cursor preview.
// The scale/angle deltas are the original's random spread (± value).
struct EnvironmentParams
{
	std::string model;             // engine resource path, e.g. Resource\TerrainData\Models\tree.3dx
	int typeIndex = 0;             // convertIdx2EnvironmentType index (0..ENVIRONMENT_TYPE_MAX-1)
	float angle = 0.f;             // angleSlider_.value, degrees
	float angleDelta = 0.f;        // angleDeltaSlider_.value, degrees
	float scale = 100.f;           // scaleSlider_.value, percent (100 == 1.0)
	float scaleDelta = 0.f;        // scaleDeltaSlider_.value, percent
	float spreadRadius = 25.f;     // spreadRadiusSlider_.value, world units
	float spreadRadiusDelta = 10.f;// spreadRadiusDeltaSlider_.value, percent
	float brushRadius = 25.f;      // getBrushRadius(), the fill area for spread
	bool vertical = false;         // m_bVertical (keep upright, do not align to the normal)
	bool spread = false;           // m_bSpread (fill the brush area with a cluster)
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

// --- Trigger editor (TriggerEditor port) ---
//
// Engine-free snapshot of one trigger node (TriggerEditor/TriggerExport.h).
// The Qt graph talks indices, never names (engine names are cp1251, Qt is
// UTF-8 — round-tripping names breaks lookup, as with library elements).
struct TriggerInfo
{
	std::string name;
	int cellX = 0;
	int cellY = 0;
	unsigned colorRGBA = 0xFF80FF80u; // 0xAARRGGBB
	int state = 0;                    // Trigger::State
	std::string conditionType;        // factory name, may be empty
	std::string actionType;           // factory name, may be empty
};

// Engine-free snapshot of one trigger link (TriggerLink).
struct TriggerLinkInfo
{
	int parent = -1;          // trigger index
	int child = -1;           // trigger index
	int colorType = 0;        // ColorType 0..STRATEGY_COLOR_MAX-1
	bool autoRestarted = false;
	bool active = false;
	int parentOffsetX = 0;
	int parentOffsetY = 0;
	int childOffsetX = 0;
	int childOffsetY = 0;
};

// One debug-log record (TriggerEvent).
struct TriggerLogRecord
{
	std::string event;
	std::string triggerName;
	int state = 0;
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
	// `init == true` maps to BaseUniverseObject::setPose(pose, true): for a
	// UnitReal it calls rigidBody()->initPose() and sends a fCommandSetPose
	// instead of interpolating — the original's Move(ZERO, true) commit on
	// mouse-up (SurToolMove.cpp:138), without which the physics step snaps the
	// unit back to its old pose.
	virtual void setObjectPose(EditorObjectId id, const EditorPose& pose, bool init) = 0;
	// SelectionUtil::awakePhysics — wake the unit's rigid body so a moved
	// object actually settles at its new pose (called by Move/Rotate/Scale).
	virtual void awakePhysics(EditorObjectId id) = 0;
	virtual float objectRadius(EditorObjectId id) = 0;
	virtual void setObjectRadius(EditorObjectId id, float radius) = 0;

	// --- Selected object properties (CSurToolSelect's attrib editor) ---
	// The single selected universe object serialized into a PropertyRow tree
	// (the original's attribEditor().attachSerializer(SerializerUniverseObject))
	// for the Properties dock. Null when the selection is not exactly one
	// object. Edits are written back with selectedObjectSetTree.
	virtual editor::PropertyRow* selectedObjectTree(bool editOnly) = 0;
	virtual bool selectedObjectSetTree(editor::PropertyRow* root) = 0;
	// Per-class counts of the current selection (CSurToolSelect::
	// CollectSerializersAndCount) for the multi-selection summary.
	virtual void selectedObjectCounts(int& units, int& environment, int& sources,
	                                  int& cameras, int& anchors) = 0;

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

	// Toolzer (SurToolToolzer::onOperationOnMap -> vMap.deltaZone): raise or
	// lower the terrain under a circular brush. `deltaH` is the signed height
	// change in voxel units (negative digs, positive raises), `smooth` the
	// smoothing/roughness 0..100, `minH`/`maxH` the optional height filter
	// (set both to 0/MAX to disable). Returns false when no world is loaded.
	virtual bool applyToolzer(float x, float y, float brushRadius,
	                          int deltaH, int smooth, int minH, int maxH) = 0;

	// Surface-kind brush (SurToolKind::onOperationOnMap -> vMap.drawInGrid):
	// paint the terrain type (0..TERRAIN_TYPES_NUMBER-1) under a circular
	// brush. `minH`/`maxH` are the optional height filter. Returns false when
	// no world is loaded.
	virtual bool applySurKind(float x, float y, float brushRadius,
	                          int kind, int minH, int maxH) = 0;

	// Toggle the "show surface kind" terrain tint (SurToolKind::OnInitDialog /
	// OnDestroy -> vMap.toShowSurKind + WorldRender). Returns false when no
	// world is loaded.
	virtual bool setShowSurKind(bool on) = 0;

	// Texture brush (SurToolColorPic::onOperationOnMap -> vMap.drawBitmapCircle):
	// paint a bitmap texture (Resource\TerrainData\Pictures\*.tga) under a
	// circular brush, tinted by a ColorModificator. `texturePath` is the engine
	// resource path; `centerAlpha` 0..255; `kColor`/`saturation`/`brightness`
	// the modifiers (percent / 100, the original's slider values); `r/g/b` the
	// tint colour 0..255. `minH`/`maxH` the optional height filter. Returns
	// false when no world is loaded or the texture cannot be loaded.
	virtual bool applyTexturePaint(float x, float y, float brushRadius,
	                               const std::string& texturePath,
	                               int centerAlpha, int kColor,
	                               int saturation, int brightness,
	                               int r, int g, int b,
	                               int minH, int maxH) = 0;

	// Paint the same texture over the whole world (SurToolColorPic::
	// OnBnClicked_Put2World -> vMap.putBitmap2AllWorld). Returns false when no
	// world is loaded or the texture cannot be loaded.
	virtual bool putTextureToAllWorld(const std::string& texturePath,
	                                  int kColor, int saturation, int brightness,
	                                  int r, int g, int b,
	                                  int minH, int maxH) = 0;

	// --- Placement (SurToolUnit / CSurToolEnvironment) ---
	//
	// The unit-attribute names of AttributeLibrary (SurToolPlayerFolder built
	// the unit tree from AttributeLibrary::instance().map()). Index-addressed:
	// the Qt panel shows the names, the engine resolves the index.
	virtual void unitAttributeNames(std::vector<std::string>& out) = 0;
	// Whether AttributeLibrary element `index` is a placeable unit
	// (SurToolPlayerFolder kept isBuilding()/isLegionary(), !internal). The
	// Units catalog shows only these; the index stays the library index.
	virtual bool unitAttributePlaceable(int index) = 0;

	// Place a unit of the given AttributeLibrary index at (x, y) on the ground
	// (SurToolUnit::onOperationOnMap -> Player::buildUnit + setPose). The unit
	// lands at the terrain height; `select` selects it afterwards. Returns the
	// new object id, or kNoObject when the index/world is invalid.
	virtual EditorObjectId placeUnit(int libraryIndex, float x, float y, bool select) = 0;

	// SurToolUnit's live preview: CSurToolUnit built a real auxiliary unit of
	// the picked attribute (unitOnMouse_) and reposed it under the cursor
	// (updateUnitOnMouse). angle/angleDelta are the tool's slider values in
	// degrees. previewUnit creates/replaces it, movePreviewUnit reposes it,
	// killPreviewUnit removes it (Kill()). Return false when invalid.
	virtual bool previewUnit(int libraryIndex, float x, float y,
	                         float angle, float angleDelta) = 0;
	virtual bool movePreviewUnit(float x, float y,
	                             float angle, float angleDelta) = 0;
	virtual void killPreviewUnit() = 0;

	// --- Source / Anchor placement (CSurToolSource / CSurToolAnchor) ---
	//
	// The originals were CSurToolEditable: a list of the placed type plus a
	// live preview object that follows the cursor and an attrib editor for the
	// type's parameters. The Qt side shows the list + a PropertyTree; the
	// engine resolves the index and owns the preview object.

	// The names of SourcesLibrary's elements (CSurToolSource's type list). The
	// index-addressed element is what placeSource/previewSource resolve.
	virtual void sourceNames(std::vector<std::string>& out) = 0;
	// Serialize SourcesLibrary element `index` into a PropertyRow tree for the
	// attrib editor (the original's attribEditor().attachSerializer; editOnly
	// hides the non-editable fields). Null when the index is invalid.
	virtual editor::PropertyRow* sourceElementTree(int index, bool editOnly) = 0;
	// Write the (possibly edited) tree back into the library element.
	virtual bool sourceElementSetTree(int index, editor::PropertyRow* root) = 0;
	// Create/replace the live preview source from element `index` (index < 0
	// kills it); it sits under the cursor and is not saved. Returns false when
	// no world / sourceManager.
	virtual bool previewSource(int index) = 0;
	// Move the live preview to (x, y) on the ground (onTrackingMouse).
	virtual bool movePreviewSource(float x, float y) = 0;
	// Place a saved source of element `index` at (x, y)
	// (CSurToolSource::onOperationOnMap -> sourceManager->addSource + setPose).
	// Returns the new object id or kNoObject.
	virtual EditorObjectId placeSource(int index, float x, float y) = 0;

	// Anchors have no library — CSurToolAnchor created a single editable Anchor
	// instance. The tree is that editable anchor; placeAnchor stamps it.
	virtual editor::PropertyRow* anchorTree(bool editOnly) = 0;
	virtual bool anchorSetTree(editor::PropertyRow* root) = 0;
	// Create/replace the live preview anchor (create==false kills it).
	virtual bool previewAnchor(bool create) = 0;
	virtual bool movePreviewAnchor(float x, float y) = 0;
	// Place a saved anchor at (x, y); generates a unique label like the
	// original (kdw::makeName over the existing anchor labels). Returns the
	// new object id or kNoObject.
	virtual EditorObjectId placeAnchor(float x, float y) = 0;

	// --- Environment placement (SurTool3DM / CSurToolEnvironment) ---
	//
	// CSurToolEnvironment placed UnitEnvironment objects from a picked .3dx
	// model with angle/scale/spread parameters and a live model preview that
	// followed the cursor. The Qt side edits EnvironmentParams; the engine
	// resolves the type index, builds the preview model and the units.

	// The EnvironmentType display names (0..ENVIRONMENT_TYPE_MAX-1), the
	// dialog's attributes combo.
	virtual void environmentTypeNames(std::vector<std::string>& out) = 0;
	// The model list (the mesh cache's entries), the dialog's model combo.
	virtual void environmentModelNames(std::vector<std::string>& out) = 0;
	// Create/rebuild the live preview from params and move it to (x, y). The
	// preview is a scene model (the original's visualObjects), not a unit.
	// `rebuild` forces a new model set (model/spread/radius change, activate,
	// after placing); false only repositions the existing one (mouse tracking)
	// so it survives long enough to render. Returns false when no world/model.
	virtual bool updateEnvironmentPreview(const EnvironmentParams& params,
	                                      float x, float y, bool rebuild) = 0;
	// Kill the live preview model(s).
	virtual void killEnvironmentPreview() = 0;
	// Place the environment object(s) at (x, y)
	// (CSurToolEnvironment::onOperationOnMap -> buildUnit + setModel +
	// setRadius + setPose). With params.spread a cluster fills the brush area.
	// Returns the number of units placed.
	virtual int placeEnvironment(const EnvironmentParams& params, float x, float y) = 0;

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

	// --- Generic library editor (LibraryEditorDialog) ---
	//
	// The universal library editor (port of kdw::LibraryEditor) edits any
	// registered library through the engine's LibrariesManager + Serializer.
	// The Qt side is engine-free, so the bridge exposes the library as a
	// plain list of element names plus a serialized PropertyRow tree per
	// element (the editor::PropertyRow model is engine-free — std types only).
	//
	// The engine side (WorldBridge) implements these with LibrariesManager /
	// LibraryWrapper / PropertyOArchive / PropertyIArchive.

	// The names of the elements in the library (editorElementName over
	// editorSize). Empty when the library is unknown. The names are display
	// only — the engine's names are cp1251, Qt is UTF-8, so they must NOT be
	// round-tripped back to the engine. Use the element index instead.
	virtual void libraryElementNames(const std::string& libraryName,
	                                 std::vector<std::string>& out) = 0;
	// The group of each element (editorElementGroup over editorSize),
	// parallel to libraryElementNames. Groups are the faction folders
	// (WATER/GROUND/...) kdw's buildLibraryTree showed the elements under.
	virtual void libraryElementGroups(const std::string& libraryName,
	                                  std::vector<std::string>& out) = 0;
	// The library's predefined group list (editorGroupsComboList,
	// '|' separated, '\\' nests). Empty when the library defines none —
	// groups then come from the elements alone.
	virtual std::string libraryGroupsComboList(const std::string& libraryName) = 0;
	// Serialize one library element (by index) into a PropertyRow tree.
	// Returns the root row (owned by the caller), or null when the index is
	// out of range. `editOnly` mirrors editorElementSerializer's
	// protectedName flag.
	virtual editor::PropertyRow* libraryElementTree(
		const std::string& libraryName, int elementIndex, bool editOnly) = 0;
	// Write a PropertyRow tree back into one library element (by index) and
	// save the library. Returns false when the index is out of range.
	virtual bool libraryElementSetTree(const std::string& libraryName,
	                                   int elementIndex,
	                                   editor::PropertyRow* root) = 0;
	// Save the library (LibraryWrapper::saveLibrary). Returns false when the
	// library is unknown.
	virtual bool librarySave(const std::string& libraryName) = 0;

	// --- Trigger editor (TriggerEditor port) ---
	//
	// The Qt trigger editor (TriggerEditorDialog) edits a TriggerChain
	// engine-side: the chain lives in the EditorEngine session (a port of
	// TriggerView's TriggerChain& + history_), and the Qt side sees plain
	// TriggerInfo/TriggerLinkInfo snapshots by index. Names are display
	// only (cp1251 vs UTF-8) — every mutation addresses triggers by index.
	//
	// Session: triggerSessionOpen loads a .scr file (TriggerChain::load),
	// triggerSessionSave persists it (chain.save + TextDB::saveLanguage, as
	// CMainFrame::OnEditTriggers did), triggerSessionClose drops it.
	// Only one session is open at a time; opening a new one closes the old.
	virtual bool triggerSessionOpen(const std::string& filePath) = 0;
	virtual bool triggerSessionSave() = 0;
	virtual void triggerSessionClose() = 0;
	virtual bool triggerSessionOpenNow() = 0;
	// The chain's display name (TriggerChain::name).
	virtual std::string triggerChainName() = 0;
	// All triggers in chain order (index 0 is the START trigger).
	virtual void triggerList(std::vector<TriggerInfo>& out) = 0;
	// All outcoming links, flattened (parent/child are trigger indices).
	virtual void triggerLinkList(std::vector<TriggerLinkInfo>& out) = 0;
	// Create a trigger with the given action factory index (as
	// TriggerView::createTrigger did via FactorySelector<Action>), at the
	// given cell. Returns the new trigger index, or -1. The name is made
	// unique via TriggerChain::uniqueName.
	virtual int triggerCreate(int actionTypeIndex, const std::string& nameHint,
	                          int cellX, int cellY) = 0;
	// Delete the trigger at the index (TriggerChain::removeTrigger).
	virtual bool triggerDelete(int triggerIndex) = 0;
	// Rename (TriggerChain::renameTrigger — rewires link names too).
	virtual bool triggerRename(int triggerIndex, const std::string& newName) = 0;
	// Move a trigger on the grid (Trigger::setCellIndex).
	virtual bool triggerSetCell(int triggerIndex, int cellX, int cellY) = 0;
	// Create/delete a link (outcomingLinks push / removeLinkByChild).
	virtual bool triggerCreateLink(int parentIndex, int childIndex,
	                               int colorType, bool autoRestarted) = 0;
	virtual bool triggerDeleteLink(int parentIndex, int childIndex) = 0;
	// Serialize one trigger's condition/action into a PropertyRow tree
	// (PropertyOArchive over Serializer(condition/action), as
	// TriggerView::updatePropertyTree attached Serializer(trigger)).
	virtual editor::PropertyRow* triggerConditionTree(int triggerIndex) = 0;
	virtual editor::PropertyRow* triggerActionTree(int triggerIndex) = 0;
	// Write a PropertyRow tree back (PropertyIArchive). Saves an undo step.
	virtual bool triggerConditionSetTree(int triggerIndex,
	                                     editor::PropertyRow* root) = 0;
	virtual bool triggerActionSetTree(int triggerIndex,
	                                  editor::PropertyRow* root) = 0;
	// Serialize the whole trigger (name/color/condition/action/links) into
	// a PropertyRow tree, and write it back. Used by the property panel.
	virtual editor::PropertyRow* triggerTree(int triggerIndex) = 0;
	virtual bool triggerSetTree(int triggerIndex, editor::PropertyRow* root) = 0;
	// Serialize chain properties (checkType) into a PropertyRow tree.
	virtual editor::PropertyRow* triggerChainTree() = 0;
	virtual bool triggerChainSetTree(editor::PropertyRow* root) = 0;
	// Factory palettes (FactorySelector<Action/Condition> comboStrings +
	// comboStringsAlt, as ClassTree built from them).
	virtual void triggerActionTypes(std::vector<std::string>& names,
	                                std::vector<std::string>& namesAlt) = 0;
	virtual void triggerConditionTypes(std::vector<std::string>& names,
	                                   std::vector<std::string>& namesAlt) = 0;
	// Replace the trigger's action/condition with a fresh instance of the
	// given factory type index (as createTrigger/editConditions did).
	virtual bool triggerSetActionType(int triggerIndex, int typeIndex) = 0;
	virtual bool triggerSetConditionType(int triggerIndex, int typeIndex) = 0;
	// Flip the trigger's root condition inverted flag (ConditionSlot::invert).
	// A direct engine-side toggle: the PropertyRow write-back cannot touch
	// it safely (PropertyIArchive::openPointer returns NULL_POINTER, so a
	// polymorphic input would delete the condition instead of updating it).
	virtual bool triggerSetConditionInverted(int triggerIndex, bool inverted) = 0;
	// Undo/redo over the BinaryOArchive history (TriggerView::saveStep/
	// undo/redo, HISTORY_STEPS = 20).
	virtual bool triggerCanUndo() = 0;
	virtual bool triggerCanRedo() = 0;
	virtual bool triggerUndo() = 0;
	virtual bool triggerRedo() = 0;
	// Debug log (TriggerChain::logData): records for the debugger panel.
	virtual void triggerLogRecords(std::vector<TriggerLogRecord>& out) = 0;

	// Static sentinel representing "no object".
	static constexpr EditorObjectId kNoObject = 0;
};
