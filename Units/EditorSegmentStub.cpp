// Editor data-segment anchors for the cross-platform build.
//
// FORCE_SEGMENT(X) in the engine references an `int dataSegmentX` that the
// matching editor .cpp defines via DECLARE_SEGMENT(X) — a linker trick to force
// the editor object file (and its factory registrations) into the link. Those
// editor sources are Windows/editor-only and not built here, so we provide the
// anchors directly. DECLARE_SEGMENT(X) expands to `int dataSegmentX;`, which we
// inline here to avoid pulling Factory.h (and its XBuffer dependency).
int dataSegmentCommandEditor;
int dataSegmentFormationEditor;
int dataSegmentUISpriteEditor;
int dataSegmentPropertyRowReference;
int dataSegmentPropertyRowFormula;
int dataSegmentLibraryCustomEffectEditor;
int dataSegmentLibraryCustomUnitEditor;
int dataSegmentLibraryCustomParameterEditor;
