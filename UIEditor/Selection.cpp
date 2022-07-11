#include "StdAfx.h"

#include "..\UserInterface\UserInterface.h"
#include "Selection.h"
#include "ControlUtils.h"

Rectf Selection::calculateBounds() const
{
    Rectf control_rect = get_rect (*front ());
    float min_left   = control_rect.left ();
    float min_top    = control_rect.top ();
    float max_right  = control_rect.right ();
    float max_bottom = control_rect.bottom ();

    const_iterator it;
    FOR_EACH (*this, it)
    {
        control_rect = get_rect (**it);
        if (min_left > control_rect.left ())
            min_left = control_rect.left ();
        if (min_top > control_rect.top ())
            min_top = control_rect.top ();
        if (max_right < control_rect.right ())
            max_right = control_rect.right ();
        if (max_bottom < control_rect.bottom ())
            max_bottom = control_rect.bottom ();
    }
    return Rectf (min_left, min_top, max_right - min_left, max_bottom - min_top);
}

void Selection::setRect(const Rectf& new_rect)
{

    if(size() == 1){
        set_rect (*front(), new_rect);
    }
    else{
        Rectf rect = calculateBounds();
        Vect2f scale_center = rect.center();

        scale (Vect2f (new_rect.width() / rect.width(),
                       new_rect.height() / rect.height()), scale_center);

        Vect2f delta = new_rect.center() - scale_center;
        move (delta);
    }
}

void Selection::move(const Vect2f& delta)
{
    iterator it;
    FOR_EACH(*this, it)
    set_rect (**it, get_rect (**it) + delta);
}

void Selection::scale(const Vect2f& _scale, const Vect2f& _origin)
{
    iterator it;
    FOR_EACH(*this, it){
        Rectf rect = get_rect(**it).scaled(_scale, _origin);
        rect.validate();
        set_rect(**it, rect);
    }
}
