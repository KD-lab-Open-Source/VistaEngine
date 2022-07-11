#ifndef __UIEDITOR_OPTIONS_H_INCLUDED__
#define __UIEDITOR_OPTIONS_H_INCLUDED__

#include "xmath.h"
#include "XMath\Colors.h"
#include "Serialization\LibraryWrapper.h"

class Archive;

class Options : public LibraryWrapper<Options>
{
public:

	enum ZoomMode{
		ZOOM_SHOW_ALL,
		ZOOM_ONE_TO_ONE,
		ZOOM_CUSTOM
	};

    enum GridMetrics{
        GRID_METRICS_RELATIVE,
        GRID_METRICS_PIXELS,
        GRID_METRICS_COUNT
    };

    Options();

    GridMetrics gridMetrics() const{ return grid_metrics_; }
    Vect2f gridSize() const{ return grid_size_; }
    Vect2f gridSizePixels() const{ return grid_size_pixels_; }
    Vect2f gridSizeCount() const{ return grid_size_count_; }

	Vect2f calculateRelativeGridSize() const;

	Vect2i largeGridSize () const { return large_grid_size_; }
    void setLargeGridSize (const Vect2i& v) { large_grid_size_ = v; }

    bool showGrid() const{ return show_grid_; }
    void setShowGrid (bool show) { show_grid_ = show; }

    bool snapToGrid() const{ return snap_to_grid_; }
    void setSnapToGrid (bool snap) { snap_to_grid_ = snap; }

	bool showBorder() const{ return show_border_; }
	void setShowBorder(bool show) { show_border_ = show; }

	bool snapToBorder() const{ return snap_to_border_; }
	void setSnapToBorder(bool snap) { snap_to_border_ = snap; }

	float rulerWidth () const{ return ruler_width_; }
	void setRulerWidth (float width) { ruler_width_ = width; }
    
    ZoomMode zoomMode() const{ return zoom_mode_; }
    void setZoomMode(ZoomMode zoomMode) { zoom_mode_ = zoomMode; }

	const Color4c& workspaceColor() const{ return workspaceColor_; }
	const Color4c& backgroundColor() const{ return backgroundColor_; }
	const Vect2i& resolution() const{ return resolution_; }

    void serialize (Archive& ar);
	
	struct Guide {
		enum Type {
			HORIZONTAL,
			VERTICAL
		};

		Guide (float _position = 0.0f, Type _type = HORIZONTAL) {
			position = _position;
			type = _type;
		}
		void serialize(Archive& ar);
		float position;
        Type type;
	};


	typedef std::vector<Guide> Guides;

	Guides& guides () { return guides_; }

private:
    Guides guides_;

	Color4c workspaceColor_;
	Color4c backgroundColor_;

    bool show_grid_;
    bool snap_to_grid_;

    GridMetrics grid_metrics_;
    Vect2f grid_size_;        // GRID_METRICS_RELATIVE
    Vect2i grid_size_pixels_; // GRID_METRICS_PIXELS
    Vect2i grid_size_count_;  // GRID_METRICS_COUNT

	Vect2i resolution_;

    Vect2i large_grid_size_;

	bool show_border_;
	bool snap_to_border_;

	float ruler_width_;

	ZoomMode zoom_mode_;
};


#endif // #ifndef __UIEDITOR_OPTIONS_H_INCLUDED__
