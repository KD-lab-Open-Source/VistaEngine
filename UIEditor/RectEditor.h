#ifndef __RECT_EDITOR_H_INCLUDED__
#define __RECT_EDITOR_H_INCLUDED__

#include "Rect.h"

class RectEditor {
public:
    RectEditor();
public:
	bool empty() { return false; }
    bool startDrag(const Vect2f& point);
    void drag(const Vect2f& point, bool preserveAspect);
    bool endDrag();

	void setCornerSize(const Vect2f& cornerSize);

	Vect2f snapOffset(const Vect2f& pt);
    //////////////////////////////////////////////////////////////////////////////
protected:
	virtual Rectf getRect (int index) const { return Rectf(); }
	virtual void setRect (int index, const Rectf& rect) {}
	virtual int getRectCount () const { return 0; }
    virtual bool onStartMoving (const Vect2f& point) { return true; }
    virtual void onMove (const Vect2f& delta) {}

	virtual void onClick (const Vect2f& point) {}
	virtual bool onStartScaling (const Vect2f& pos, const Vect2i& corner) { return true; }
	virtual void onScale (const Vect2f& delta) {}
private:
    bool moving_;
	bool scaling_;
	Vect2f cornerSize_;
    Vect2i grabbedCorner_;
    Vect2f lastPoint_;
    Vect2f startPoint_;
	int rectIndex_;
    Rectf initialRect_;
	Rectf rect_;
};

#endif
