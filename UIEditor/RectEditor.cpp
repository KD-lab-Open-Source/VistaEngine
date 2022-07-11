#include "StdAfx.h"
#include "RectEditor.h"

#include "Options.h"
#include "SelectionCorner.h"

RectEditor::RectEditor()
: moving_(false)
, scaling_(false)
, startPoint_(Vect2f::ZERO)
, lastPoint_(Vect2f::ZERO)
, cornerSize_(Vect2f::ZERO)
{
}

bool RectEditor::startDrag(const Vect2f& point)
{
    onClick (point);
    int rectCount = getRectCount ();

    for (int i = 0; i < rectCount; ++i) {
        Rectf rect = getRect (i);
        SelectionCorner::Corners::const_iterator corn_it;
        corn_it = std::find_if(SelectionCorner::allCorners().begin(), SelectionCorner::allCorners().end(),
                               CornerUnderPoint(point, rect, Vect2f(0.005f, 0.005f * 1.25f), 0.0f));
        if(corn_it != SelectionCorner::allCorners().end()){
            if(onStartScaling (point, corn_it->scale())){
                startPoint_ = point;
                lastPoint_ = point;
                grabbedCorner_ = corn_it->scale ();
                scaling_ = true;

                rectIndex_ = i;
                rect_ = rect;
				initialRect_ = rect;
                return true;
            }
        } else if(rect.point_inside(point)){
            if (onStartMoving (point)) {
                startPoint_ = point;
                lastPoint_ = point;
                moving_ = true;
                rectIndex_ = i;
                rect_ = rect;
				initialRect_ = rect;
                return true;
            }
        }
    }
    return false;
}

void RectEditor::drag(const Vect2f& point, bool preserveAspect)
{
    Vect2f delta = point - lastPoint_;
    if(moving_){
        rect_ = rect_ + delta;
        Vect2f offset1 = snapOffset (Vect2f(rect_.left(),rect_.top()));
        Vect2f offset2 = snapOffset (Vect2f(rect_.right(),rect_.bottom()));
        Vect2f offset (fabs(offset1.x) < fabs(offset2.x) ? offset1.x : offset2.x,
                       fabs(offset1.y) < fabs(offset2.y) ? offset1.y : offset2.y);
        if (offset.x > 1.0f)
            offset.x = 0.0f;
        if (offset.y > 1.0f)
            offset.y = 0.0f;
        setRect (rectIndex_, rect_ + offset);
    }
    if(scaling_){
        SelectionCorner corner(grabbedCorner_);
        Vect2f oldCornerPosition = corner.getCornerPosition(initialRect_, 0.0f);

        if(preserveAspect){
			delta = point - oldCornerPosition;

			Vect2f sz = initialRect_.size() * grabbedCorner_;
            sz.normalize(1.0f);
            delta = sz * sz.dot(delta);

            rect_ = initialRect_;
			corner.setCornerPosition(rect_, 0.0f, oldCornerPosition + delta);
		}
		else{
			corner.setCornerPosition(rect_, 0.0f, point);
        }

		if(rect_.width() < 0.0f){
			initialRect_ = initialRect_ + Vect2f(-grabbedCorner_.x * initialRect_.width(), 0.0f);
            grabbedCorner_.x = -grabbedCorner_.x;
		}
		if(rect_.height() < 0.0f){
			initialRect_ = initialRect_ + Vect2f(0.0f, -grabbedCorner_.y * initialRect_.height());
            grabbedCorner_.y = -grabbedCorner_.y;
		}
        rect_.validate();

        Rectf resultRect;
        if(preserveAspect){
            resultRect = rect_;
        }
        else{
			Vect2f offset;
			resultRect = rect_;
			offset = snapOffset(point);
			if(offset.x > 1.0f)
				offset.x = 0.0f;
			if(offset.y > 1.0f)
				offset.y = 0.0f;
			corner.setCornerPosition(resultRect, 0.0f, point + offset);
        }
        setRect(rectIndex_, resultRect);
    }
    lastPoint_ = point;
}

bool RectEditor::endDrag()
{
    if (moving_) {
        moving_ = false;
        if (!startPoint_.eq(lastPoint_)) {
            onMove (lastPoint_ - startPoint_);
            return true;
        }
    }
    if (scaling_) {
        scaling_ = false;
        if (!startPoint_.eq(lastPoint_)) {
            onScale (lastPoint_ - startPoint_);
            return true;
        }
    }
    return false;
}

void RectEditor::setCornerSize(const Vect2f& cornerSize)
{
	cornerSize_ = cornerSize;
}

Vect2f RectEditor::snapOffset(const Vect2f& pt)
{
    Vect2f result (10.0f, 10.0f);
    if (Options::instance().snapToGrid ()) {
        Vect2f gridSize = Options::instance().calculateRelativeGridSize();
        result.x = round(pt.x / gridSize.x) * gridSize.x - pt.x;
        result.y = round(pt.y / gridSize.y) * gridSize.y - pt.y;
    } 
    if (Options::instance().snapToBorder()) {
        float snap_distance = 0.01f;
        if (fabs(pt.x) <= snap_distance) {
            result.x = -pt.x;
        } 
        if (fabs(1.0f - pt.x) <= snap_distance) {
            result.x = 1.0f - pt.x;
        } 
        if (fabs(pt.y) <= snap_distance / 0.75f) {
            result.y = -pt.y;
        } 
        if (fabs(1.0f - pt.y) <= snap_distance / 0.75f) {
            result.y = 1.0f - pt.y;
        } 
    }
    return result;
}
