#include "stdafx.h"
#include "SurToolRotate.h"

#include "SelectionUtil.h"

#include "..\Render\inc\IRenderDevice.h"
#include "..\Game\RenderObjects.h"

namespace UniverseObjectActions{

struct RotateLocal : UniverseObjectAction{
    RotateLocal (const Vect3f& axis, float angle, bool init)
    : angle_(angle)
    , axis_(axis)
	, init_(init)
	{
	}

    void operator()(BaseUniverseObject& object){
        Se3f pose(object.pose());
        pose.postmult(Se3f(QuatF(angle_, axis_, 0), Vect3f::ZERO));
        object.setPose(pose, init_);
		awakePhysics(object);
    }

	bool init_;
    float angle_;
    Vect3f axis_;
};

struct RotateAroundPoint : UniverseObjectAction{
    RotateAroundPoint (const Vect3f& origin, const Vect3f& axis, float angle, bool init)
    : origin_(origin)
    , axis_(axis)
    , angle_(angle)
	, init_(init) {}

    void operator() (BaseUniverseObject& object) {
        Se3f pose(object.pose());
        pose.trans() -= origin_;
        pose.premult(Se3f(QuatF(angle_, axis_, 0), Vect3f::ZERO));
        pose.trans() += origin_;
        object.setPose(pose, init_);
		awakePhysics(object);
    }

	bool init_;
    Vect3f origin_;
    Vect3f axis_;
    float angle_;
};

};

CSurToolRotate::CSurToolRotate(CWnd* parent)
: CSurToolTransform(parent)
{
    transformAxis_[0] = false;
    transformAxis_[1] = false;
    transformAxis_[2] = true;

	angleStep_ = 15.0f / 180.0f * M_PI;
}

bool CSurToolRotate::CallBack_LMBUp(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	using namespace UniverseObjectActions;
	__super::CallBack_LMBUp(worldCoord, screenCoord);
    if(localTransform_)
        forEachSelected(RotateLocal(transformAxis(), 0, true));
    else
        forEachSelected(RotateAroundPoint(selectionCenter_, transformAxis(), 0, true));
	return true;
}

bool CSurToolRotate::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	using namespace UniverseObjectActions;
	CSurToolTransform::CallBack_TrackingMouse(worldCoord, scrCoord);
    if(buttonPressed_) {
		Vect3f point = projectScreenPointOnPlane(transformAxis (), selectionCenter_, cursorCoord_);
		endPoint_ = point;
		Vect3f unitPoint = selectionCenter_;

		Vect3f a = startPoint_ - unitPoint; 
		Vect3f b = point - unitPoint;
		a.normalize ();
		b.normalize ();
		float cos_of_angle = dot(a, b);
		float angle = acosf (clamp(cos_of_angle, -1.0f, 1.0f));
		
		int index = transformAxisIndex();
        switch(index){
        case 0:
			if((a % b).x < 0)
				angle = -angle;
            break;
        case 1:
			if ((a % b).y < 0)
				angle = -angle;
            break;
        case 2:
			if ((a % b).z < 0)
				angle = -angle;
            break;
        }
		forEachSelected(RestorePose(poses_, true));

		float result_angle = angle;

		if(stepRotationMode())
			result_angle = float(round(angle / angleStep_)) * angleStep_;

		if (localTransform_)
			forEachSelected(RotateLocal(transformAxis(), result_angle, true));
		else
			forEachSelected(RotateAroundPoint(selectionCenter_, transformAxis(), result_angle, true));
    }
	return true;
}

void CSurToolRotate::beginTransformation()
{
	using namespace UniverseObjectActions;
    forEachSelected(StorePose(poses_));

    startPoint_ = projectScreenPointOnPlane(transformAxis(), selectionCenter_, cursorCoord_);
    startPoint_[transformAxisIndex()] = selectionCenter_[transformAxisIndex()];

    endPoint_ = startPoint_;
}

void CSurToolRotate::onTransformAxisChanged(int index)
{
	for(int i = 0; i < 3; ++i)
		transformAxis_[i] = (i == index);
}

bool CSurToolRotate::CallBack_DrawAuxData()
{
    Vect3f position = selectionCenter_;

    drawAxis (position, selectionRadius_, transformAxis_);


	Se3f circle_pose (Se3f (QuatF::ID, selectionCenter_));
	if(buttonPressed_){
		Vect3f unitPoint = selectionCenter_;
		Vect3f a = startPoint_ - unitPoint;
		Vect3f b = endPoint_ - unitPoint;
		if(a.norm() > FLT_EPS && b.norm() > FLT_EPS){
			a.normalize(1.0f);
			b.normalize(1.0f);

			float cos_of_angle = dot(a, b);
			float angle = acosf (clamp(cos_of_angle, -1.0f, 1.0f));

			int index = transformAxisIndex();
			if(index == 0){
				if((a % b).x < 0)
					angle = -angle;
			}
			else if(index == 1){
				if((a % b).y < 0)
					angle = -angle;
			}
			else if(index == 2){
				if((a % b).z < 0)
					angle = -angle;
			}

			float result_angle = angle;

			if(stepRotationMode())
				result_angle = float(round(angle / angleStep_)) * angleStep_;

			QuatF quat(result_angle, transformAxis(), 1);
			Vect3f b = a;
			quat.xform(b);

			if(angle > (M_PI * 0.21f)){
				int a = 3;
			}

			a *= selectionRadius_;
			b *= selectionRadius_;

			gb_RenderDevice->DrawLine (unitPoint, unitPoint + a, sColor4c(255, 255, 255));
			gb_RenderDevice->DrawLine (unitPoint, unitPoint + b, sColor4c(255, 255, 255));
			if (transformAxisIndex () == 1) {
				circle_pose.rot().set (M_PI * 0.5f, Vect3f::I, 0);
			} else if (transformAxisIndex () == 0) {
				circle_pose.rot().set (M_PI * 0.5f, Vect3f::J, 0);
			} else {
				circle_pose.rot().set (0, Vect3f::I, 0);
			}
		}

	}
    drawCircle (circle_pose, selectionRadius_, sColor4c(255 * transformAxis_[0],
                                                        255 * transformAxis_[1],
                                                        255 * transformAxis_[2], 255));
	return true;
}

bool CSurToolRotate::stepRotationMode()
{
	return (GetKeyState(VK_SHIFT) >> 15);
}
