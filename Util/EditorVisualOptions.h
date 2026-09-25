#ifndef __EDITOR_VISUAL_OPTIONS_H_INCLUDED__
#define __EDITOR_VISUAL_OPTIONS_H_INCLUDED__

// EditorVisualOptions — the flags EditorVisual::isVisible reads.
//
// In the MFC editor these lived in SurMap5/SurMapOptions (surMapOptions); the Qt
// port keeps the same flags here, in the engine's Util library, so both the
// engine (isVisible, UnitBase::showEditor) and the Qt editor write the one copy.
// The Qt editor's MainWindow sets them from its View menu; the engine-side
// editorVisual() reads them. Defaults match the original SurMapOptions: sources,
// cameras and camera borders off, models shown.
//
// Only the flags the visibility hook needs live here; the rest of SurMapOptions
// (last dirs, dock state, grid colour, ...) is Qt-side and QSettings-backed.

class EditorVisualOptions
{
public:
	static bool showSources() { return showSources_; }
	static void setShowSources(bool on) { showSources_ = on; }

	static bool showCameras() { return showCameras_; }
	static void setShowCameras(bool on) { showCameras_ = on; }

	static bool hideWorldModels() { return hideWorldModels_; }
	static void setHideWorldModels(bool on) { hideWorldModels_ = on; }

	static bool showPathFinding() { return showPathFinding_; }
	static void setShowPathFinding(bool on) { showPathFinding_ = on; }

	static bool showCameraBorders() { return showCameraBorders_; }
	static void setShowCameraBorders(bool on) { showCameraBorders_ = on; }

private:
	static bool showSources_;
	static bool showCameras_;
	static bool hideWorldModels_;
	static bool showPathFinding_;
	static bool showCameraBorders_;
};

#endif
