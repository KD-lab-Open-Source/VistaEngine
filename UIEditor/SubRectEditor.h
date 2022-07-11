#ifndef __SUB_RECT_EDITOR_H_INCLUDED__
#define __SUB_RECT_EDITOR_H_INCLUDED__

#include "RectEditor.h"

class SubRectEditor : public RectEditor
{
public:
    virtual Rectf getRect(int index) const;
    virtual void setRect (int index, const Rectf& rect);
	virtual int getRectCount () const;

    virtual void onClick (const Vect2f& point);

	virtual bool onStartScaling (const Vect2f& pos, const Vect2i& corner);

    virtual bool onStartMoving (const Vect2f& pos);
    virtual void onMove (const Vect2f& delta);

private:
	Vect2i grabbed_corner_;
	Rectf start_rect_;
};

#endif
