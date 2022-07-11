#ifndef __SELECTION_CORNER_H_INCLUDED__
#define __SELECTION_CORNER_H_INCLUDED__

#include "EditorView.h"

class SelectionCorner
{
public:
    typedef std::vector<SelectionCorner> Corners;

    static const Corners& allCorners () {
        static bool corners_created = false;
        static Corners corners;
        if (! corners_created)
        {
            for (int i = -1; i <= 1; ++i)
                for (int j = -1; j <= 1; ++j)
                    if (i != 0 || j != 0)
                        corners.push_back (SelectionCorner (Vect2i (i, j)));
            corners_created = true;
        }
        return corners;
    }

    SelectionCorner (const Vect2i& _scale)
      : scale_ (_scale)
      {
          xassert (scale_.x != 0 || scale_.y != 0);
      }
    inline Vect2f getCornerPosition (const Rectf& _rect, float _corner_distance) const
    {
        float half_width = _rect.width () * 0.5f;
        float half_height = _rect.height () * 0.5f;

        return Vect2f (_rect.center () + Vect2f (half_width * static_cast<float>(xScale ()),
                                                 half_height * static_cast<float>(yScale ())));

    }
    inline void setCornerPosition (Rectf& _rect, float _corner_distance, const Vect2f& _position) const
    {
		if (xScale () == -1) {
			_rect.width (_rect.right() - _position.x);
			_rect.left (_position.x);
		} else if (xScale () == 1) {
			_rect.width (_position.x - _rect.left());
			_rect.right (_position.x);
		}
		if (yScale () == -1) {
			_rect.height (_rect.bottom() - _position.y);
			_rect.top (_position.y);
		} else if (yScale () == 1) {
			_rect.height (_position.y - _rect.top());
			_rect.bottom (_position.y);
		}
    }
    inline SelectionCorner inverted () const
    {
        SelectionCorner new_corner = *this;
        new_corner.scale_ = -scale_;
        return new_corner;
    }
    Vect2i scale () const {
        return scale_;
    }
    inline float xScale () const {
        return scale_.x;
    }
    inline float yScale () const {
        return scale_.y;
    }
private:
    Vect2i scale_;
};

class CornerUnderPoint{
public:
    CornerUnderPoint(const Vect2f& _point, const Rectf& _rect,
                     const Vect2f& _corner_size, float _corner_distance)
	: point_ (_point)
	, rect_ (_rect)
	, corner_size_ (_corner_size)
	, corner_distance_ (_corner_distance)
    {        
    }

    bool operator()(const SelectionCorner& _corner)
	{
        Vect2f delta = _corner.getCornerPosition (rect_, corner_distance_) - point_;
        if (fabs (delta.x) <= corner_size_.x &&
            fabs (delta.y) <= corner_size_.y)
            return true;
        else
            return false;
    }
private:
    Vect2f point_;
    Rectf  rect_;
    Vect2f corner_size_;
    float  corner_distance_;
};

#endif

