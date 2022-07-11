#ifndef __UIEDITOR_SELECTION_H_INCLUDED__
#define __UIEDITOR_SELECTION_H_INCLUDED__

#include <vector>

#include "IncludeContainer.h"
#include "Rect.h"

class UI_ControlBase;

typedef std::vector<UI_ControlBase*> SelectedControlsArray;

class Selection : public IncludeContainer<UI_ControlBase*, std::vector>
{
public:
    Rectf calculateBounds() const;
	void setRect(const Rectf& new_rect);

	void move(const Vect2f& delta);
    void scale(const Vect2f& _scale, const Vect2f& _origin);
};

#endif
