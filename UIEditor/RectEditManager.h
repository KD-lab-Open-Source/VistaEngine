#ifndef __RECT_EDIT_MANAGER_H_INCLUDED__
#define __RECT_EDIT_MANAGER_H_INCLUDED__

class RectEditManager {
public:
    RectEditManager()
    : moving_(false)
	, startPoint_ (Vect2f::ZERO)
	, lastPoint_ (Vect2f::ZERO)
    {}
public:
	inline bool empty () {
		return false;
	}
    inline bool startDrag (const Vect2f& point) {
        //Rectf& rect = getRect();
        if (!empty()) {
            if (onStartMoving (point)) {
				startPoint_ = point;
				lastPoint_ = point;
                moving_ = true;
                return true;
            }
        }
        return false;
    }
    inline void drag (const Vect2f& point) {
        if (!moving_)
            return;
        onMoving (point - lastPoint_);
        lastPoint_ = point;
    }
    inline bool endDrag () {
        if (!moving_)
            return false;
        moving_ = false;
        if (!(startPoint_ == lastPoint_)) {
            onMove (lastPoint_ - startPoint_);
			return true;
        }
		return false;
    }
    //////////////////////////////////////////////////////////////////////////////
protected:
	virtual Rectf getRect () const { return Rectf(); }
	virtual void setRect (const Rectf& rect) {}
    virtual bool onStartMoving (const Vect2f& point) { return true; }
    virtual void onMoving (const Vect2f& delta) {}
    virtual void onMove (const Vect2f& delta) {}
private:
    bool moving_;
    Vect2f lastPoint_;
    Vect2f startPoint_;
};

#endif
