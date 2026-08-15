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

class EditorTool
{
public:
	virtual ~EditorTool() = default;

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
};
