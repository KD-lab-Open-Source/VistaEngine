// MapChangeParams.h — the engine-free surface of vrtMapChangeParam.
//
// EngineViewport::changeTotalWorldParam takes one of these; the Qt dialogs
// (ChangeTotalWorldHeightDialog) build it without ever seeing engine headers.
// Kept as its own header so RenderViewWidget.h can name it by value without
// including the engine-touching EngineViewport.h (which lives in EditorEngine
// and carries EngineIncludes flags).
#pragma once

// The namespace is Editor, NOT EngineViewport: EngineViewport is a class (the
// forward-declared viewport in RenderViewWidget.h), and a class and a
// namespace cannot share a name.
namespace Editor {
// The vrtMapChangeParam surface (SurMap5/DlgChangeTotalWorldHeight.h): the
// vrtMapCreationParam base plus the move/resize flags, engine-free.
struct MapChangeParams
{
	int hSizePower = 0;              // vrtMapCreationParam::SIZE_POWER
	int vSizePower = 0;
	int createWorldMetod = 0;        // vrtMapCreationParam::eCreateWorldMetod
	unsigned short initialHeight = 0;
	bool flag_resizeWorld2NewBorder = false;
	int oldWorldBegCoordX = 0;
	int oldWorldBegCoordY = 0;
	float kScaleModels = 1.f;
};
}  // namespace Editor
