#include "StdAfx.h"
#include "ConstraintHandlerSimple.h"
#include "XMath\SafeMath.h"

ContactSimple::ContactSimple(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, float penetration)
{
	point_ = point;
	body0_ = body0;
	xassert(isEq(normal.norm(), 1.0f, 0.01f));
	axes_.xrow() = normal;
	if(body0_->friction() > FLT_EPS){
		if(fabsf(normal.x) < 0.5f)
			axes_.yrow().cross(normal, Vect3f::I);
		else
			axes_.yrow().cross(normal, Vect3f::J);
		axes_.yrow().normalize();
		axes_.zrow().cross(axes_.yrow(), axes_.xrow());
		axes_.zrow().normalize();
	}
	penetration_ = penetration;
}

Vect6f ContactSimple::computeJ(const Vect3f& axis, const Vect3f& local_point) const
{
	if(body0_->angularEvolve())
		return Vect6f(axis, Vect3f().cross(local_point, axis));
	return Vect6f(axis, Vect3f::ZERO);
}

void ContactSimple::compute(ConstraintHandler& ch) const
{
	Vect3f local_point0 = point_;
	local_point0.sub(body0_->centreOfGravity());
	if(body0_->friction() > FLT_EPS){
		ch.push_back(new IterateQuantitiesSimpleFriction6f(body0_->core(), computeJ(axes_.xrow(), local_point0), 
			penetration_ / body0_->relaxationTime(), Limits::GREATER_ZERO, body0_->restitution(), body0_->friction()));
		ch.push_back(new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes_.yrow(), local_point0)));
		ch.push_back(new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes_.zrow(), local_point0)));
	}else{
		ch.push_back(new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes_.xrow(), local_point0), 
			penetration_ / body0_->relaxationTime(), Limits::GREATER_ZERO, body0_->restitution()));
	}
}

JointSimple::JointSimple(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body, const float relaxationTime,
	const Vect3f& upperLimits, const Vect3f& lowerLimits) :
	body0_(body),
	relaxationTime_(relaxationTime),
	upperLimits_(upperLimits),
	lowerLimits_(lowerLimits)
{
	xassert(relaxationTime > FLT_EPS);
	setPose(point, axes);
}

void JointSimple::setPose(const Vect3f& point, const Mat3f& axes)
{
	point_ = point;
	body0_->orientation().invXform(axes.xrow(), axes0_.xrow());
	body0_->orientation().invXform(axes.yrow(), axes0_.yrow());
	body0_->orientation().invXform(axes.zrow(), axes0_.zrow());
	localPoint0_.sub(point_, body0_->centreOfGravity());
	body0_->orientation().invXform(localPoint0_);
}

void JointSimple::computeAxes(Mat3f& axesTemp, const QuatF& orientation, const Mat3f& axes)
{
	orientation.xform(axes.xrow(), axesTemp.xrow());
	axesTemp.xrow().normalize();
	orientation.xform(axes.yrow(), axesTemp.yrow());
	axesTemp.yrow().normalize();
	orientation.xform(axes.zrow(), axesTemp.zrow());
	axesTemp.zrow().normalize();
}

void JointSimple::compute(ConstraintHandler& ch) const
{
	Mat3f axes0Temp;
	computeAxes(axes0Temp, body0_->orientation(), axes0_);
	Vect3f point0;
	body0_->orientation().xform(localPoint0_, point0);
	Vect3f penetration = point_;
	penetration.sub(body0_->centreOfGravity());
	penetration.sub(point0);
	penetration.scale(1.0f / relaxationTime_);
	ch.resize(ch.size() + 6);
	vector<IterateQuantitiesBase*>::iterator i_ch(ch.end() - 6);
	*i_ch = new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes0Temp.xrow(), point0),
		dot(penetration, axes0Temp.xrow()), Limits::FREE);
	*(++i_ch) = new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes0Temp.yrow(), point0),
		dot(penetration, axes0Temp.yrow()), Limits::FREE);
	*(++i_ch) = new IterateQuantitiesSimple6f(body0_->core(), computeJ(axes0Temp.zrow(), point0),
		dot(penetration, axes0Temp.zrow()), Limits::FREE);
	Vect3f temp0, temp1, w;
	temp0.cross(axes0Temp.yrow(), axes0_.yrow());
	temp1.cross(axes0Temp.zrow(), axes0_.zrow());
	*(++i_ch) = new IterateQuantitiesSimple6f(body0_->core(), Vect6f(Vect3f::ZERO, axes0Temp.xrow()), 
		computePhi(w.add(temp0, temp1), axes0Temp.xrow()), lowerLimits_.x, upperLimits_.x, relaxationTime_);
	w = temp1;
	temp1.cross(axes0Temp.xrow(), axes0_.xrow());
	*(++i_ch) = new IterateQuantitiesSimple6f(body0_->core(), Vect6f(Vect3f::ZERO, axes0Temp.yrow()), 
		computePhi(w.add(temp1), axes0Temp.yrow()), lowerLimits_.y, upperLimits_.y, relaxationTime_);
	*(++i_ch) = new IterateQuantitiesSimple6f(body0_->core(), Vect6f(Vect3f::ZERO, axes0Temp.zrow()), 
		computePhi(w.add(temp0, temp1), axes0Temp.zrow()), lowerLimits_.z, upperLimits_.z, relaxationTime_);
}

void ConstraintHandler::attach(RigidBodyPhysicsBase* body)
{
	if(body->attached())
		return;
	body->setIndex(bodies_.size());
	bodies_.push_back(body);
}

void ConstraintHandler::detach(RigidBodyPhysicsBase* body)
{
	if(!body->attached())
		return;
	if(body->index() != bodies_.size() - 1){
		bodies_.back()->setIndex(body->index());
		bodies_[body->index()] = bodies_.back();
	}
	body->setIndex(-1);
	bodies_.pop_back();
}

void ConstraintHandler::attach(JointBase* joint)
{
	if(joint->attached())
		return;
	joint->setIndex(joints_.size());
	joints_.push_back(joint);
}

void ConstraintHandler::detach(JointBase* joint)
{
	if(!joint->attached())
		return;
	if(joint->index() != joints_.size() - 1){
		joints_.back()->setIndex(joint->index());
		joints_[joint->index()] = joints_.back();
	}
	joint->setIndex(-1);
	joints_.pop_back();
}

void ConstraintHandler::quant(int iterationLimit, float dt, int times)
{
	start_timer_auto();
	vector<RigidBodyPhysicsBase*>::iterator irb;
	FOR_EACH(bodies_, irb)
		(*irb)->preEvolve();
	for(int i = 0; i < times; ++i){
		clear();
		FOR_EACH(bodies_, irb)
			(*irb)->computeForceAndTOI(dt);
		vector<ConstraintBase*>::iterator ic;
		FOR_EACH(contacts_, ic)
			(*ic)->compute(*this);
		contacts_.clear();
		vector<JointBase*>::iterator ij;
		FOR_EACH(joints_, ij)
			(*ij)->compute(*this);
		FOR_EACH(bodies_, irb)
			(*irb)->computeAccelerationExternal();
		if(size() > 0){
			iterator it;
			FOR_EACH(*this, it)
				(*it)->update(dt);
			for(int iter = 0; iter < iterationLimit; ++iter)
				FOR_EACH(*this, it)
					(*it)->quant(it);
			FOR_EACH(*this,it)
				(*it)->preciseDV();
		}
		FOR_EACH(bodies_, irb)
			(*irb)->boxEvolve(dt);
		PhysicsPoolBase::freeAll();
	}
}