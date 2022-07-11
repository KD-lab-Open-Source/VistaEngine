#include "stdafx.h"
#include "MaskEditor.h"
#include "UserInterface\UI_Types.h"
#include "ActionManager.h"

#include "EditorView.h"

MaskEditor::MaskEditor()
: moving_(false)
, startPoint_(Vect2f::ZERO)
, lastPoint_(Vect2f::ZERO)
, mask_(0)
, currentPoint_(0)
, pixelSize_(1.0f, 1.0f)
, transform_(0.0f, 0.0f, 1.0f, 1.0f)
{

}

bool MaskEditor::startRDrag (const Vect2f& point) {
	if(!mask_)
		return false;
	UI_Mask::Polygon& polygon = mask_->polygon();
	
	if(polygon.empty())
		return false;

	Vect2f offset(transform_.left_top());

	for(int i = 0; i < polygon.size(); ++i){
		Vect2f& pos = polygon[i];
		Vect2f relativePos = pos + offset;

		if(Rectf(relativePos - CUIEditorView::CORNER_SIZE * pixelWidth(), CUIEditorView::CORNER_SIZE * 2.0f * pixelWidth()).point_inside(point)){
			polygon.erase(polygon.begin() + i);
			moving_ = false;
			currentPoint_ = -1;
			return true;
		}
	}
	return false;
}

bool MaskEditor::startDrag (const Vect2f& point) {
	if(!mask_)
		return false;

	Vect2f mousePoint(point - transform_.left_top());

	UI_Mask::Polygon& polygon = mask_->polygon();
	currentPoint_ = -1;
	moving_ = false;

	Vect2f offset(transform_.left_top());

	for(int i = 0; i < polygon.size(); ++i){
		Vect2f& pos = polygon[i];
		Vect2f relativePos = pos + offset;

		if(Rectf(relativePos - CUIEditorView::CORNER_SIZE * pixelWidth(), CUIEditorView::CORNER_SIZE * 2.0f * pixelWidth()).point_inside(point)){
			currentPoint_ = i;
			moving_ = true;
			return true;
		}
	}
	if(polygon.size() >= 2){
		if(!moving_){
			Vect2f point1, point2;
			float precision = 0.01f;
			for(int i = 0; i < polygon.size(); )
			{
				Vect2f point1 = polygon[i] + offset;
				++i;
				if(i == polygon.size())
					point2 = polygon[0] + offset;
				else
					point2 = polygon[i] + offset;

				Vect2f dir(point2-point1);

				float length = dir.norm();
				float distance = fabs(dir % (point - point1) / length);
				if(distance < precision && point.distance(point1) < length && point.distance(point2) < length){
					polygon.insert(polygon.begin() + i, mousePoint);
					currentPoint_ = i;
					moving_ = true;
					return true;
				}
			}
		}
	}
	else{
		polygon.push_back(mousePoint);
		currentPoint_ = polygon.size() - 1;
		moving_ = true;
	}
    return false;
}

void MaskEditor::drag(const Vect2f& point)
{
    Vect2f delta = point - lastPoint_;
	if(mask_ && moving_ && currentPoint_ >= 0){
		mask_->polygon().at(currentPoint_) += delta;
		ActionManager::the().setChanged();
    }
    lastPoint_ = point;
}

bool MaskEditor::endDrag()
{
	if(!mask_)
		return false;

    if(moving_){
        moving_ = false;/*
        if (!startPoint_.eq(lastPoint_)) {
            Vect3f delta(lastPoint_ - startPoint_);

            return true;
        }*/
		return true;
    }
    return false;
}

void MaskEditor::setMask(UI_Mask* mask, const Rectf& transform)
{
	mask_ = mask;
	transform_ = transform;
	//if(mask_->polygon().size()
}

UI_Mask* MaskEditor::mask()
{
	return mask_;
}
