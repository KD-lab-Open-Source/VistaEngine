// EditorVisualOptions.cpp — the one definition of the visibility flags.
// See the header. Compiled into Util so the game and the Qt editor share them.

#include "EditorVisualOptions.h"

// Defaults match the original SurMapOptions (SurMap5/SurMapOptions.cpp:17-18):
// sources and cameras on, models shown, path-finding/camera-borders off.
bool EditorVisualOptions::showSources_ = true;
bool EditorVisualOptions::showCameras_ = true;
bool EditorVisualOptions::hideWorldModels_ = false;
bool EditorVisualOptions::showPathFinding_ = false;
bool EditorVisualOptions::showCameraBorders_ = false;
