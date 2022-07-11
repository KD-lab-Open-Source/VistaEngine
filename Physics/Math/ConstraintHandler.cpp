#include "StdAfx.h"
#include "ConstraintHandler.h"

Contact::Contact(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, RigidBodyPhysics* body1, float penetration) :
	ContactSimple(point, normal, body0, penetration)
{
	body1_ = body1;
}

void Contact::compute(ConstraintHandler& ch) const
{
	float restitution = min(body0_->restitution(), body1_->restitution());
	float friction = body0_->friction() * body1_->friction();
	float relaxationTime = (body0_->relaxationTime() + body1_->relaxationTime()) / 2;
	Vect3f local_point0 = point_, local_point1 = point_;
	local_point0.sub(body0_->centreOfGravity());
	local_point1.sub(body1_->centreOfGravity());
	if(friction > FLT_EPS){
		ch.push_back(new IterateQuantities6fFriction(body0_->core(), body1_->core(), 
			computeJ(axes_.xrow(), local_point0), computeJ(-axes_.xrow(), local_point1), 
			penetration_ / relaxationTime, Limits::GREATER_ZERO, restitution, friction));
		ch.push_back(new IterateQuantities6f(body0_->core(), body1_->core(), 
			computeJ(axes_.yrow(), local_point0), computeJ(-axes_.yrow(), local_point1)));
		ch.push_back(new IterateQuantities6f(body0_->core(), body1_->core(), 
			computeJ(axes_.zrow(), local_point0), computeJ(-axes_.zrow(), local_point1)));
	}else
		ch.push_back(new IterateQuantities6f(body0_->core(), body1_->core(), 
			computeJ(axes_.xrow(), local_point0), computeJ(-axes_.xrow(), local_point1), 
			penetration_ / relaxationTime, Limits::GREATER_ZERO, restitution));
}

void Joint::set(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
	const float relaxationTime,	const Vect3f& upperLimits, const Vect3f& lowerLimits)
{
	xassert(relaxationTime > FLT_EPS);
	body0_ = body0;
	body1_ = body1;
	relaxationTime_ = relaxationTime;
	upperLimits_ = upperLimits;
	lowerLimits_ = lowerLimits;
	setPose(point, axes);
}

void Joint::setPose(const Vect3f& point, const Mat3f& axes)
{
	__super::setPose(point, axes);
	body1_->orientation().invXform(axes.xrow(), axes1_.xrow());
	body1_->orientation().invXform(axes.yrow(), axes1_.yrow());
	body1_->orientation().invXform(axes.zrow(), axes1_.zrow());
	localPoint1_.sub(point_, body1_->centreOfGravity());
	body1_->orientation().invXform(localPoint1_);
}

void Joint::compute(ConstraintHandler& ch) const
{
	Mat3f axes0Temp, axes1Temp;
	computeAxes(axes0Temp, body0_->orientation(), axes0_);
	computeAxes(axes1Temp, body1_->orientation(), axes1_);
	Vect3f point0, point1;
	body0_->orientation().xform(localPoint0_, point0);
	body1_->orientation().xform(localPoint1_, point1);
	Vect3f penetration = body1_->centreOfGravity();
	penetration.add(point1);
	penetration.sub(body0_->centreOfGravity());
	penetration.sub(point0);
	penetration.scale(1.0f / relaxationTime_);
	ch.resize(ch.size() + 6);
	vector<IterateQuantitiesBase*>::iterator i_ch(ch.end() - 6);
	*i_ch = new IterateQuantities6f(body0_->core(), body1_->core(), computeJ(axes0Temp.xrow(), 
		point0), computeJ(-axes0Temp.xrow(), point1), dot(penetration, axes0Temp.xrow()), Limits::FREE);
	*(++i_ch) = new IterateQuantities6f(body0_->core(), body1_->core(), computeJ(axes0Temp.yrow(), 
		point0), computeJ(-axes0Temp.yrow(), point1), dot(penetration, axes0Temp.yrow()), Limits::FREE);
	*(++i_ch) = new IterateQuantities6f(body0_->core(), body1_->core(), computeJ(axes0Temp.zrow(), 
		point0), computeJ(-axes0Temp.zrow(), point1), dot(penetration, axes0Temp.zrow()), Limits::FREE);
	Vect3f temp0, temp1, w;
	temp0.cross(axes0Temp.yrow(), axes1Temp.yrow());
	temp1.cross(axes0Temp.zrow(), axes1Temp.zrow());
	*(++i_ch) = new IterateQuantities6f(body0_->core(), body1_->core(),
		Vect6f(Vect3f::ZERO, axes0Temp.xrow()), Vect6f(Vect3f::ZERO, -axes0Temp.xrow()), 
		computePhi(w.add(temp0, temp1), axes0Temp.xrow()), lowerLimits_.x, upperLimits_.x, relaxationTime_);
	w = temp1;
	temp1.cross(axes0Temp.xrow(), axes1Temp.xrow());
	*(++i_ch) = new IterateQuantities6f(body0_->core(), body1_->core(), 
		Vect6f(Vect3f::ZERO, axes0Temp.yrow()), Vect6f(Vect3f::ZERO, -axes0Temp.yrow()), 
		computePhi(w.add(temp1), axes0Temp.yrow()),	lowerLimits_.y, upperLimits_.y, relaxationTime_);
	*(++i_ch) = new IterateQuantities6f(body0_->core(), body1_->core(), 
		Vect6f(Vect3f::ZERO, axes0Temp.zrow()), Vect6f(Vect3f::ZERO, -axes0Temp.zrow()), 
		computePhi(w.add(temp0, temp1), axes0Temp.zrow()), lowerLimits_.z, upperLimits_.z, relaxationTime_);
}