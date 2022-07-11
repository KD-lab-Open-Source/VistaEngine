#include "StdAfx.h"
#include "Object3dxInterface.h"
#include "Interpolation.h"
#include "RigidBodyPrm.h"
#include "RigidBodyCarPrm.h"
#include "WhellController.h"
#include "normalMap.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyCarPrm
//
///////////////////////////////////////////////////////////////

RigidBodyCarPrm::RigidBodyCarPrm() : 
	rigidBodyPrm("RigidBodyCar"),
	turnInPosition(false)
{
}

void RigidBodyCarPrm::serialize(Archive& ar)
{
	ar.serialize(rigidBodyPrm, "rigidBodyPrm", "Тип подвески");
	ar.serialize(static_cast<vector<RigidBodyWheelPrm>&>(*this), "carPrm", "Колеса");
	ar.serialize(turnInPosition, "turnInPosition", 0);
}

///////////////////////////////////////////////////////////////
//
//    class WhellController
//
///////////////////////////////////////////////////////////////

WhellController::WhellController(const WheelDescriptorList& wheelList, cObject3dx* model, cObject3dx* modelLogic) : 
	angle_(0), 
	whellAngle_(0), 
	model_(model)
{ 
	WheelDescriptorList::const_iterator i;
	FOR_EACH(wheelList, i){
		positions_.push_back(modelLogic->GetNodePosition(i->nodeLogic).trans());
		if(i->frontWheel)
			steeringWheels_.push_back(Wheel(i->nodeGraphics));
		else
			wheels_.push_back(Wheel(i->nodeGraphics));
	}
	whellRadius_ = modelLogic->GetNodePosition(wheelList[0].nodeLogic).trans().z 
				  - modelLogic->GetNodePosition(0).trans().z;
}

void WhellController::quant(float angle, float linearVelocity)
{
	angle_ = angle;
	whellAngle_ -= linearVelocity/whellRadius_ * logicPeriodSeconds;
	whellAngle_ = cycleAngle(whellAngle_);
	Se3f r(QuatF(whellAngle_, Vect3f::I), Vect3f::ZERO);
	vector<Wheel>::iterator i;
	FOR_EACH(wheels_, i)
		i->interpolate(r, model_);
	r.premult(Se3f(QuatF(angle_, Vect3f::K), Vect3f::ZERO));
	FOR_EACH(steeringWheels_, i)
		i->interpolate(r, model_);
}

Vect3f WhellController::placeToGround(const Se3f& pose, float& groundZ, bool waterAnalysis)
{
	float Sx = 0;
	float Sy = 0;
	float Sz = 0;
	float Sxy = 0;
	float Syz = 0;
	float Sxz = 0;
	float Sx2 = 0;
	float Sy2 = 0;
	float Sz2 = 0;
	vector<Vect3f>::iterator it;
	FOR_EACH(positions_, it){
		Vect3f wheelPose = *it;
		pose.xformVect(wheelPose);
		float x = wheelPose.x;
		float y = wheelPose.y;
		wheelPose.add(pose.trans());
		int xi = wheelPose.xi();
		int yi = wheelPose.yi();
		float z = waterAnalysis ? max((float)vMap.getApproxAlt(xi, yi), environment->water()->GetZ(xi, yi)) :
								  vMap.getAltWhole(xi, yi);
		Sx += x;
		Sy += y;
		Sz += z;
		Sxy += x * y;
		Syz += y * z;
		Sxz += x * z;
		Sx2 += sqr(x);
		Sy2 += sqr(y);
		Sz2 += sqr(z);
	}
	int N = positions_.size();
	float t1 = Sxy * Sxy - Sx2 * Sy2;
	float t2 = Sx * Sy2 - Sy * Sxy;
	float t3 = Sy * Sx2 - Sx * Sxy;
	float t4 = 1.0f / (t1 * N + t2 * Sx + t3 * Sy);
	float t5 = Syz * N  - Sy * Sz;
	float t6 = Sx * Sz - Sxz * N;
	float t7 = Sy * Sxz - Sx * Syz;
	float A = (t5 * Sxy + t6 * Sy2 + t7 * Sy) * t4;
	float B = -(t5 * Sx2 + t6 * Sxy + t7 * Sx) * t4;
	groundZ = (t1 * Sz + t2 * Sxz + t3 * Syz) * t4;
	return Vect3f (-A, -B, 1);
}