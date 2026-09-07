// SurMap5Qt's engine-side stubs.
//
// The original SurMap5/ tree (CMainFrame + the kdw widgets) provided editor
// overrides for the Action/Condition queue the engine uses but does not need
// for a map editor. We do not link the original SurMap5 MFC code, but the
// engine still references the same symbols — `isUnderEditor()` is queried
// from Game/Universe.cpp, Environment/SourceManager.cpp, Game/Player.cpp
// and many more; without an override, every TU that defines it ends up in
// the link and we get a "multiple definition" or "unresolved external" error.
//
// The single definition here is the only `isUnderEditor` the editor needs.
// The engine's behaviour under the editor (the original CMainFrame set it to
// `true` too — see SurMap5/VistaEngineContext.cpp:164) is:
//   - skip game-side logic quant
//   - skip trigger scripts
//   - skip mission .spg
//   - skip fog of war init
//   - skip terrain auto-impermeability
//   - skip water/temperature setup that requires a full mission

// SurMap5 calls these from the engine; they have nothing to do in the editor.
bool isUnderEditor()
{
	return true;
}

bool applicationHasFocus()
{
	return true;
}
