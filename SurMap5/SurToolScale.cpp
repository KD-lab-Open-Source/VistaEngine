#include "stdafx.h"
#include "SurToolScale.h"
#include "..\Units\BaseUniverseObject.h"

#include "..\Environment\Environment.h"

#include "ObjectsManagerWindow.h"
#include "Serialization.h"
// ^^

#include "..\Game\RenderObjects.h"
#include "..\Game\CameraManager.h"

#include "SelectionUtil.h"
namespace UniverseObjectActions{

struct ScaleOnCenter : UniverseObjectAction{
	ScaleOnCenter(const Vect3f& origin, float scale)
    : origin_(origin)
    , scale_(scale)
	{}

	void operator()(BaseUniverseObject& object){
		Se3f pose (object.pose());

        pose.trans() -= origin_;

		pose.trans() *= scale_;

        pose.trans() += origin_;

        object.setPose(pose, true);
		object.setRadius(object.radius() * scale_);
		awakePhysics(object);
	}

    Vect3f origin_;
    float scale_;
};

struct ScaleLocal : UniverseObjectAction{
	ScaleLocal (float scale)
    : scale_(scale) {}

	void operator()(BaseUniverseObject& object){
		object.setRadius(object.radius() * scale_);
		awakePhysics(object);
	}

    float scale_;
};

};

CSurToolScale::CSurToolScale(CWnd* parent)
: CSurToolTransform(parent)
{
    transformAxis_[0] = true;
    transformAxis_[1] = true;
    transformAxis_[2] = true;
}

bool CSurToolScale::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	using namespace UniverseObjectActions;
	CSurToolTransform::CallBack_TrackingMouse(worldCoord, scrCoord);

    if(buttonPressed_) {
		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay (Vect2f::ZERO, pos, dir);
		Vect3f point = projectScreenPointOnPlane (dir, selectionCenter_, cursorCoord_) - selectionCenter_;
		endPoint_ = point;

		float dist = startPoint_.distance(Vect3f::ZERO);

		float scale = fabsf(dist) < FLT_COMPARE_TOLERANCE ? 1.0f : point.distance(Vect3f::ZERO) / dist;
		forEachSelected (RestorePose(poses_, false));
		if (localTransform_) {
			forEachSelected (ScaleLocal(scale));
		} else {
			forEachSelected (ScaleOnCenter(selectionCenter_, scale));
		}
    }
	return true;
}

void CSurToolScale::beginTransformation()
{
	using namespace UniverseObjectActions;
	forEachSelected(StorePose(poses_));

	Vect3f pos, dir;
	cameraManager->GetCamera()->GetWorldRay (Vect2f::ZERO, pos, dir);
	startPoint_ = projectScreenPointOnPlane(dir, selectionCenter_, cursorCoord_) - selectionCenter_;
	endPoint_ = startPoint_;
}

void CSurToolScale::onTransformAxisChanged(int index)
{
	transformAxis_[0] = true;
	transformAxis_[1] = true;
	transformAxis_[2] = true;
}

bool CSurToolScale::CallBack_DrawAuxData()
{
    drawAxis (selectionCenter_, selectionRadius_, transformAxis_);

	if(buttonPressed_) {
		gb_RenderDevice->DrawLine (selectionCenter_, selectionCenter_ + startPoint_, sColor4c(0, 0, 255));
		gb_RenderDevice->DrawLine (selectionCenter_, selectionCenter_ + endPoint_, sColor4c(255, 255, 255));
	}
	return true;
}
