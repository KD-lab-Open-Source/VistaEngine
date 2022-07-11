#include "StdAfx.h"
#include "UnitAttribute.h"
#include "RealUnit.h"
#include "WhellController.h"
#include "..\Environment\Environment.h"
#include "..\water\water.h"

bool WhellController::init( const WheelDescriptorList& wheelList )
{
	unit_.modelLogic()->Update();
	for(int i=0; i<wheelList.size(); i++) {
		int nodeIndex_ = wheelList[i].nodeLogic;
		if(nodeIndex_ >= 0){
			wheelPositions.push_back(unit_.modelLogic()->GetNodePosition(nodeIndex_).trans());
			int nodeIndexGraph = wheelList[i].nodeGraphics;
			if(nodeIndexGraph >= 0) {
				if(wheelList[i].frontWheel) {
					nodeIndexesFront.push_back(nodeIndexGraph);
					nodeOrientationPrevFront.push_back(Se3f::ID);
				}
				else {
					nodeIndexesBack.push_back(nodeIndexGraph);
					nodeOrientationPrevBack.push_back(Se3f::ID);
				}
			}
		}
	}

	if(!nodeIndexesFront.empty() || !nodeIndexesBack.empty()) {
		Se3f whellPose = unit_.modelLogic()->GetNodePosition(wheelList[0].nodeLogic);
		Se3f unitPose = unit_.modelLogic()->GetNodePosition(unit_.modelLogic()->FindNode("group center"));
		whellRadius = whellPose.trans().z - unitPose.trans().z;
	}

	return wheelPositions.size() >= 3;
}

void WhellController::addVelocity( float linearVelocity )
{
	if(whellRadius) {

		whellAngle -= linearVelocity/whellRadius * logicPeriodSeconds;

		if(whellAngle < -(2*M_PI))
			whellAngle += 2* M_PI;

		if(whellAngle > 2*M_PI)
			whellAngle -= 2* M_PI;
	
		update_ = true;
	}
}

void WhellController::interpolationQuant()
{
	if(update_) {
		Se3f RW(QuatF(whellAngle, Vect3f::I), Vect3f::ZERO);
		Se3f R(QuatF(angle_, Vect3f::K), Vect3f::ZERO);
		R.postmult(RW);
		for(int i=0; i<nodeIndexesFront.size(); i++)
			if(nodeIndexesFront[i]){
				streamLogicInterpolator.set(fNodeTransformInterpolation, unit_.model()) << nodeIndexesFront[i] << nodeOrientationPrevFront[i] << R;
				nodeOrientationPrevFront[i] = R;
			}

		for(int i=0; i<nodeIndexesBack.size(); i++)
			if(nodeIndexesBack[i]){
				streamLogicInterpolator.set(fNodeTransformInterpolation, unit_.model()) << nodeIndexesBack[i] << nodeOrientationPrevBack[i] << RW;
				nodeOrientationPrevBack[i] = RW;
			}

		update_ = false;
	}
}

Vect3f WhellController::placeToGround(const Se3f& pose, float& groundZ, bool waterAnalysis)
{
	float N = wheelPositions.size();

	if(N <= 0){
		xassert(!"Wheels count <= 0");
		groundZ = 0;
		return Vect3f::K;
	}

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
	FOR_EACH(wheelPositions, it){
		
		Vect3f wheelPose = *it;
		pose.xformVect(wheelPose);
		
		float x = wheelPose.x;
		float y = wheelPose.y;

		wheelPose.add(pose.trans());
		float z = 0;
		if(waterAnalysis){
			float zw = environment->water()->GetZ(wheelPose.xi(), wheelPose.yi());
			float zg = vMap.GetApproxAlt(wheelPose.xi(), wheelPose.yi());
			z = max(zg, zw);
		}
		else
			z = vMap.GetAltWhole(wheelPose.xi(), wheelPose.yi());

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

	float t1 = Sxy * N;
	float t5 = Sx * Sy;
	float t7 = Sy * Sy;
	float t9 = Sy * Sxy;
	float t14 = Sx2 * N;
	float t16 = Sxy * Sxy;
	float t18 = Sx * Sx;
	float t20 = Sx * Sxy;
	float t25 = -Sy2 * t14 + t16 * N + Sy2 * t18 - 2 * Sy * t20 + t7 * Sx2;
	
	if(fabs(t25) < FLT_EPS) {
		xassert(!"Деление на ноль в WhellController");
		groundZ = Sz / N;
		return Vect3f::K;
	}

	t25 = 1.0f / t25;
	
	float t32 = Sy * Sx2;
	float A = (Syz * t1 - N * Sxz * Sy2 - Syz * t5 + t7 * Sxz - Sz * t9 + Sx * Sz * Sy2) * t25;
	float B = (-Sz * t20 + Syz * t18 + Sxz * t1 - Syz * t14 - Sxz * t5 + Sz * t32) * t25;
	groundZ = (-Syz * t20 + Sx * Sxz * Sy2 - Sxz * t9 + Syz * t32 - Sz * Sx2 * Sy2 + t16 * Sz) * t25;
	return Vect3f (-A, -B, 1);
}
