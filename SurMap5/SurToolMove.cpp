#include "stdafx.h"
#include "SurToolMove.h"
#include "SelectionUtil.h"
#include "Game\RenderObjects.h"
#include "Game\CameraManager.h"
#include "Render\Src\cCamera.h"
#include "SystemUtil.h"

namespace UniverseObjectActions{
struct Move : UniverseObjectAction{
	Move (const Vect3f& delta, bool init = true)
    : delta_(delta)
    , init_(init)
    {
    }
	void operator()(BaseUniverseObject& object){
		object.setPose(Se3f(object.orientation(), object.position() + delta_), init_);
		awakePhysics(object);
	}
	Vect3f delta_;
    bool init_;
};
};

CSurToolMove::CSurToolMove(CWnd* parent)
: CSurToolTransform(parent) 
, cloneOnMove_(false)
{
	
    transformAxis_[0] = true;
    transformAxis_[1] = true;
    transformAxis_[2] = false;
}

bool CSurToolMove::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	using namespace UniverseObjectActions;
	CSurToolTransform::onTrackingMouse(worldCoord, scrCoord);
	const float CLONE_THRESHOLD = 10.0f; // world units

	if(buttonPressed_){
		if(startPoint_.distance(endPoint_) > CLONE_THRESHOLD && cloneOnMove_){
			ClonedSources sources;
			ClonedUnits units;

			forEachSelected(Cloner(units, sources));
            forEachSelected(RestorePose(poses_, true), false);

			deselectAll();

			ClonedUnits::iterator uit;
			FOR_EACH(units, uit)
				(*uit)->setSelected(true);

			ClonedSources::iterator sit;
			FOR_EACH(sources, sit)
				(*sit)->setSelected(true);

			cloneOnMove_ = false;

			beginTransformation();
		}
		if(transformAxis().z){
			Vect3f pos, dir;
			cameraManager->GetCamera()->GetWorldRay (Vect2f::ZERO, pos, dir);
			dir.z = 0;
			if (dir.norm() > FLT_EPS) {
				dir.normalize (-1.0f);
				Vect3f point = projectScreenPointOnPlane (dir, selectionCenter_, cursorCoord_);
				Vect3f delta = (point - endPoint_);
				forEachSelected(Move(Vect3f(0.0f, 0.0f, delta.z), false));
				selectionCenter_ += Vect3f(0.0f, 0.0f, delta.z);
				endPoint_ = point;
			}
		}
		else{
			Vect3f point = screenPointToGround(cursorCoord_);
			forEachSelected(Move(point - endPoint_), false);
			selectionCenter_ += point - endPoint_;
			endPoint_ = point;
		}
	}
	return true;
}

bool CSurToolMove::onDrawAuxData()
{
    Vect3f position = selectionCenter_;
    drawAxis (position, selectionRadius_, transformAxis_);
	return true;
}

void CSurToolMove::beginTransformation()
{
	using namespace UniverseObjectActions;
    forEachSelected (StorePose (poses_));

	if (transformAxis ().z) {
		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay (Vect2f::ZERO, pos, dir);
		dir.z = 0;
		if (dir.norm() > FLT_EPS) {

			dir.normalize (-1.0f);
			startPoint_ = projectScreenPointOnPlane(dir, selectionCenter_, cursorCoord_);
		}
	} else {
		startPoint_ = screenPointToGround(cursorCoord_);
	}

    endPoint_ = startPoint_;
}

void CSurToolMove::onTransformAxisChanged(int index)
{
	if(index == 2) {
		transformAxis_[0] = !transformAxis_[index];
		transformAxis_[1] = !transformAxis_[index];
	} 
	
	if(index == 0 || index == 1) {
		transformAxis_[0] = transformAxis_[index];
		transformAxis_[1] = transformAxis_[index];
		transformAxis_[2] = !transformAxis_[index];
	}
}

bool CSurToolMove::onLMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord )
{
	if(::isAltPressed())
		cloneOnMove_ = true;
	return __super::onLMBDown(worldCoord, scrCoord);
}

bool CSurToolMove::onLMBUp(const Vect3f& coord, const Vect2i& screenCoord)
{
	using namespace UniverseObjectActions;
	forEachSelected(Move(Vect3f::ZERO, true));
	return __super::onLMBUp(coord, screenCoord);
}
