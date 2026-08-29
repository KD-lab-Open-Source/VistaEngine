// MiniMapPanel.h — Qt port of CMiniMapWindow (SurMap5/MiniMapWindow.cpp) for
// the Qt editor: the map image, the camera marker, and click-to-center.
//
// The original was a second SDL render window drawing the world from above
// through a dedicated camera (initRenderDevice/updateCameraFrustum). The Qt
// editor has one render device owned by the 3D view; the panel instead draws
// the map.tga image the engine already knows how to produce (vMap.saveMiniMap)
// with a QPainter overlay — the same information, without a second swapchain.
//
// Data flow: the panel asks RenderViewWidget (engine-free facade) for the
// minimap pixels (EngineViewport::minimapPixels, the saveMiniMap port) and the
// camera orbit centre, then repaints. A left click is a minimap click: it
// converts the widget point to world coordinates and calls
// setCameraCenter — CMiniMapWindow::OnLButtonDown -> minimap().pressEvent ->
// cameraToEvent did the same.

#pragma once

#include <QImage>
#include <QWidget>

class RenderViewWidget;

class MiniMapPanel : public QWidget
{
	Q_OBJECT
public:
	explicit MiniMapPanel(RenderViewWidget* view, QWidget* parent = nullptr);

	// Refresh the cached minimap image from the loaded world (called when a
	// world is opened/created and after "Save MiniMap to World").
	void reload();

	// The map's world dimensions (vMap.H_SIZE x V_SIZE); the panel maps widget
	// points to world points with these. False when no world is loaded.
	bool worldSize(int& hSize, int& vSize) const;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

private:
	RenderViewWidget* view_ = nullptr;
	QImage image_;      // the minimap (worldSize().first x second px, ARGB32)
	int worldW_ = 0;    // world units across (vMap.H_SIZE)
	int worldH_ = 0;    // world units down (vMap.V_SIZE)
};
