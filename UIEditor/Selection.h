#ifndef __UIEDITOR_SELECTION_H_INCLUDED__
#define __UIEDITOR_SELECTION_H_INCLUDED__

#include <vector>

#include "XTL\Rect.h"

class UI_ControlBase;

typedef std::vector<UI_ControlBase*> SelectedControlsArray;

class Selection : public vector<UI_ControlBase*>
{
public:
    Rectf calculateBounds() const;
	void setRect(const Rectf& new_rect);

	void move(const Vect2f& delta);
    void scale(const Vect2f& _scale, const Vect2f& _origin);
};

#endif
