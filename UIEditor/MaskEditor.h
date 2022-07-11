#ifndef __MASK_EDITOR_H_INCLUDED__
#define __MASK_EDITOR_H_INCLUDED__

#include "XTL\Rect.h"

class UI_Mask;

class MaskEditor{
public:
    MaskEditor();

	void setPixelSize(const Vect2f& pixelSize) { pixelSize_ = pixelSize; }
	float pixelWidth() const{ return pixelSize_.x; }
    bool startDrag(const Vect2f& point);
	bool startRDrag(const Vect2f& point);

    void drag(const Vect2f& point);
    bool endDrag();

	void setMask(UI_Mask* mask, const Rectf& transform);
	UI_Mask* mask();
private:
	UI_Mask* mask_;
	Rectf transform_;

	Vect2f pixelSize_;

	int currentPoint_;
    bool moving_;
    Vect2f startPoint_;
    Vect2f lastPoint_;
};

#endif
