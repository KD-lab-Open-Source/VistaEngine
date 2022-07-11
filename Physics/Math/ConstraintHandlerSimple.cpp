#include "StdAfx.h"
#include "ConstraintHandlerSimple.h"
#include "RigidBodyPrm.h"
#include "RigidBodyBox.h"
#include "float.h"

ContactSimple::ContactSimple() :
	point_(Vect3f::ZERO),
	axes_(Mat3f::ID),
	body0_(0),
	penetration_(0)
{
}
ContactSimple::ContactSimple(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, float penetration)
{
	point_ = point;
	xassert(body0 != 0);
	body0_ = body0;
	axes_.xrow() = normal;
	if(body0_->friction() > FLT_EPS){
		if(fabsf(normal.x - 1) > FLT_EPS)
			axes_.yrow().cross(normal, Vect3f::I);
		else
			axes_.yrow().cross(normal, Vect3f::J);
		axes_.yrow().Normalize();
		axes_.zrow().cross(axes_.yrow(), axes_.xrow());
		axes_.zrow().Normalize();
	}
	penetration_ = penetration;
}
void ContactSimple::compute(vector<IterateQuantities>& ch, bool angularEvolve) const
{
	vector<IterateQuantities>::iterator i_ch;

	if(body0_->friction() < FLT_EPS){
		ch.resize(ch.size() + 1);
		i_ch = ch.end() - 1;
	}
	else{
		ch.resize(ch.size() + 3);
		i_ch = ch.end() - 3;
	}
	Vect3f local_point0;
	local_point0.sub(point_, body0_->centreOfGravity());
	i_ch->j.setMap0(0);
	i_ch->j.setMap1(-1);
	i_ch->j.matrix0().setLinear(axes_.xrow());
	if(angularEvolve)
		i_ch->j.matrix0().angular().cross(local_point0, axes_.xrow());
	else
		i_ch->j.matrix0().setAngular(Vect3f::ZERO);
	i_ch->eta = penetration_ / body0_->relaxationTime();
	i_ch->lambda_max = FLT_MAX;
	i_ch->lambda_min = 0.0f;
	i_ch->restitution = body0_->restitution();
	i_ch->friction = body0_->friction();
	i_ch->lambda = 0;
	i_ch->j.setMatrix1(Vect6f::ZERO);
	int k = 1;
	++i_ch;
	if(i_ch < ch.end()){
		i_ch->j.setMap0(0);
		i_ch->j.setMap1(-1);
		i_ch->j.matrix0().linear()=axes_.yrow();
		if(angularEvolve)
			i_ch->j.matrix0().angular().cross(local_point0, axes_.yrow());
		else
			i_ch->j.matrix0().setAngular(Vect3f::ZERO);
		i_ch->eta = 0;
		i_ch->lambda_max = 0;
		i_ch->lambda_min = 0;
		i_ch->restitution = 0;
		i_ch->friction = 0;
		i_ch->lambda = 0;
		i_ch->j.setMatrix1(Vect6f::ZERO);
		++i_ch;
		i_ch->j.setMap0(0);
		i_ch->j.setMap1(-1);
		i_ch->j.matrix0().linear()=axes_.zrow();
		if(angularEvolve)
			i_ch->j.matrix0().angular().cross(local_point0, axes_.zrow());
		else
			i_ch->j.matrix0().setAngular(Vect3f::ZERO);
		i_ch->eta = 0;
		i_ch->lambda_max = 0;
		i_ch->lambda_min = 0;
		i_ch->restitution = 0;
		i_ch->friction = 0;
		i_ch->lambda = 0;
		i_ch->j.setMatrix1(Vect6f::ZERO);
	}
}

Contact::Contact() :
	body1_(0),
	map0_(-1),
	map1_(-1),
	friction_(0.0f),
	relaxationTime_(0.01f)
{
}

Contact::Contact(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, RigidBodyPhysics* body1, 
	int map0, int map1, float penetration)
{
	point_ = point;
	xassert(body0 != 0);
	body0_ = body0;
	body1_ = body1;
	friction_ = body0->friction();
	relaxationTime_ = body0->relaxationTime();
	if(map1 >= 0){
		xassert(body1_ != 0);
		friction_ *= body0->friction();
		relaxationTime_ += body0->relaxationTime();
		relaxationTime_ /= 2;
	}
	axes_.xrow() = normal;
	if(friction_ > FLT_EPS){
		if(fabsf(normal.x - 1) > FLT_EPS)
			axes_.yrow().cross(normal, Vect3f::I);
		else
			axes_.yrow().cross(normal, Vect3f::J);
		axes_.yrow().Normalize();
		axes_.zrow().cross(axes_.yrow(), axes_.xrow());
		axes_.zrow().Normalize();
	}
	xassert(map0 >= 0);
	map0_ = map0;
	xassert(map1 >= -1);
	map1_ = map1;
	penetration_ = penetration;
}
void Contact::compute(vector<IterateQuantities>& ch) const
{
	vector<IterateQuantities>::iterator i_ch;

	if(friction_ < FLT_EPS){
		ch.resize(ch.size() + 1);
		i_ch = ch.end() - 1;
	}
	else{
		ch.resize(ch.size() + 3);
		i_ch = ch.end() - 3;
	}
	Vect3f local_point0, local_point1;
	local_point0.sub(point_, body0_->centreOfGravity());
	i_ch->j.setMap0(map0_);
	i_ch->j.setMap1(map1_);
	i_ch->j.matrix0().setLinear(axes_.xrow());
	i_ch->j.matrix0().angular().cross(local_point0,axes_.xrow());
	i_ch->eta = penetration_ / relaxationTime_;
	i_ch->lambda_max = FLT_MAX;
	i_ch->lambda_min = 0.0f;
	i_ch->restitution = body0_->restitution();
	i_ch->friction = friction_;
	i_ch->lambda = 0;
	if(map1_ >= 0){
		xassert(body1_ != 0);
		local_point1.sub(point_, body1_->centreOfGravity());
		i_ch->j.matrix1().linear().negate(axes_.xrow());
		i_ch->j.matrix1().angular().cross(axes_.xrow(), local_point1);
		if(body1_->restitution() < i_ch->restitution)
			i_ch->restitution = body1_->restitution();
	}
	else
		i_ch->j.setMatrix1(Vect6f::ZERO);
	int k = 1;
	++i_ch;
	if(i_ch < ch.end()){
		i_ch->j.setMap0(map0_);
		i_ch->j.setMap1(map1_);
		i_ch->j.matrix0().linear()=axes_.yrow();
		i_ch->j.matrix0().angular().cross(local_point0, axes_.yrow());
		i_ch->eta = 0;
		i_ch->lambda_max = 0;
		i_ch->lambda_min = 0;
		i_ch->restitution = 0;
		i_ch->friction = 0;
		i_ch->lambda = 0;
		if(map1_ >= 0){
			i_ch->j.matrix1().linear().negate(axes_.yrow());
			i_ch->j.matrix1().angular().cross(axes_.yrow(), local_point1);
		}
		else
			i_ch->j.setMatrix1(Vect6f::ZERO);
		++i_ch;
		i_ch->j.setMap0(map0_);
		i_ch->j.setMap1(map1_);
		i_ch->j.matrix0().linear()=axes_.zrow();
		i_ch->j.matrix0().angular().cross(local_point0, axes_.zrow());
		i_ch->eta = 0;
		i_ch->lambda_max = 0;
		i_ch->lambda_min = 0;
		i_ch->restitution = 0;
		i_ch->friction = 0;
		i_ch->lambda = 0;
		if(map1_ >= 0){
			i_ch->j.matrix1().linear().negate(axes_.zrow());
			i_ch->j.matrix1().angular().cross(axes_.zrow(), local_point1);
		}
		else
			i_ch->j.setMatrix1(Vect6f::ZERO);
	}
}

Joint::Joint() :
	point_(Vect3f::ZERO),
	axes0_(Mat3f::ID),
	axes1_(Mat3f::ID),
	body0_(0),
	body1_(0),
	map0_(-1),
	map1_(-1),
	relaxationTime_(0),
	localPoint0_(Vect3f::ZERO),
	localPoint1_(Vect3f::ZERO),
	penetration_(Vect3f::ZERO),
	upperLimits_(Vect3f::ZERO),
	lowerLimits_(Vect3f::ZERO)
{
}
Joint::Joint(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
	const int map0, const int map1, const float relaxationTime,
	const Vect3f& upperLimits, const Vect3f& lowerLimits)
{
	set(point, axes, body0, body1, map0, map1, relaxationTime, upperLimits, lowerLimits);
}
void Joint::set(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
	const int map0, const int map1, const float relaxationTime,
	const Vect3f& upperLimits, const Vect3f& lowerLimits)
{
	xassert(map0 >= 0);
	map0_ = map0;
	xassert(map1 >= -1);
	map1_ = map1;
	xassert(body0 != 0);
	body0_ = body0;
	body1_ = body1;
	xassert(relaxationTime > FLT_EPS);
	relaxationTime_ = relaxationTime;
	upperLimits_ = upperLimits;
	lowerLimits_ = lowerLimits;
	setPose(point, axes);
}
void Joint::setPose(const Vect3f& point, const Mat3f& axes)
{
	point_ = point;
	body0_->orientation().invXform(axes.xrow(), axes0_.xrow());
	body0_->orientation().invXform(axes.yrow(), axes0_.yrow());
	body0_->orientation().invXform(axes.zrow(), axes0_.zrow());
	localPoint0_.sub(point_, body0_->centreOfGravity());
	body0_->orientation().invXform(localPoint0_);
	if(map1_ >= 0){
		xassert(body1_ != 0);
		body1_->orientation().invXform(axes.xrow(), axes1_.xrow());
		body1_->orientation().invXform(axes.yrow(), axes1_.yrow());
		body1_->orientation().invXform(axes.zrow(), axes1_.zrow());
		localPoint1_.sub(point_, body1_->centreOfGravity());
		body1_->orientation().invXform(localPoint1_);
	}
	else{
		axes1_ = Mat3f::ID;
		localPoint1_ = Vect3f::ZERO;
	}
}
void Joint::compute(const vector<IterateQuantities>::iterator ch)
{
	Mat3f axes0Temp, axes1Temp(axes0_);
	Vect3f point0, point1;
	body0_->orientation().xform(axes0_.xrow(), axes0Temp.xrow());
	body0_->orientation().xform(axes0_.yrow(), axes0Temp.yrow());
	body0_->orientation().xform(axes0_.zrow(), axes0Temp.zrow());
	body0_->orientation().xform(localPoint0_, point0);
	if(map1_ >= 0){
		xassert(body1_ != 0);
		body1_->orientation().xform(axes1_.xrow(), axes1Temp.xrow());
		body1_->orientation().xform(axes1_.yrow(), axes1Temp.yrow());
		body1_->orientation().xform(axes1_.zrow(), axes1Temp.zrow());
		body1_->orientation().xform(localPoint1_, point1);
		penetration_.add(body1_->centreOfGravity(), point1);
	}
	else
		penetration_ = point_;
	penetration_.sub(body0_->centreOfGravity());
	penetration_.sub(point0);
	float restitution = 0.0f;
	//float restitution=body0_->restitution();
	//if(map1_>=0)
	//	if(body1_->restitution() < i_ch->restitution);
	//		i_ch->restitution=body1_->restitution();
	vector<IterateQuantities>::iterator i_ch(ch), i_ch3(ch + 3);
	for(int i = 0; i < 3; ++i){
		i_ch->eta = dot(penetration_, axes0Temp[i]) / relaxationTime_;
		i_ch3->eta = 0;
		i_ch->j.setMap0(map0_);
		i_ch->j.setMap1(map1_);
		i_ch3->j.setMap0(map0_);
		i_ch3->j.setMap1(map1_);
		i_ch->j.matrix0().setLinear(axes0Temp[i]);
		i_ch->j.matrix0().angular().cross(point0, axes0Temp[i]);
		i_ch3->j.matrix0().set(Vect3f::ZERO, axes0Temp[i]);
		if(map1_>=0){
			i_ch->j.matrix1().linear().negate(axes0Temp[i]);
			i_ch->j.matrix1().angular().cross(axes0Temp[i], point1);
			i_ch3->j.matrix1().set(Vect3f::ZERO, -axes0Temp[i]);
		}
		else{
			i_ch->j.setMatrix1(Vect6f::ZERO);
			i_ch3->j.setMatrix1(Vect6f::ZERO);
		}
		i_ch->lambda_max = FLT_MAX;
		i_ch->lambda_min = -FLT_MAX;
		i_ch->restitution = restitution;
		i_ch3->restitution = restitution;
		i_ch->friction = 0;
		i_ch3->friction = 0;
		i_ch->lambda = 0;
		i_ch3->lambda = 0;
		++i_ch;
		++i_ch3;
	}
	Vect3f temp0, temp1;
	temp0.cross(axes0Temp.yrow(), axes1Temp.yrow());
	temp1.cross(axes0Temp.zrow(), axes1Temp.zrow());
	Vect3f w;
	w.add(temp0, temp1);
	w.scale(0.5f);
	Vect3f phi;
	phi.x = dot(w, axes0Temp.xrow());
	if(phi.x > 1.0f)
		phi.x = 1.0f;
	if(phi.x < -1.0f)
		phi.x = -1.0f;
	phi.x = asinf(phi.x);
	w = temp1;
	temp1.cross(axes0Temp.xrow(), axes1Temp.xrow());
	w.add(temp1);
	w.scale(0.5f);
	phi.y = dot(w, axes0Temp.yrow());
	if(phi.y > 1.0f)
		phi.y = 1.0f;
	if(phi.y < -1.0f)
		phi.y = -1.0f;
	phi.y = asinf(phi.y);
	w.add(temp0, temp1);
	w.scale(0.5f);
	phi.z = dot(w, axes0Temp.zrow());
	if(phi.z > 1.0f)
		phi.z = 1.0f;
	if(phi.z < -1.0f)
		phi.z = -1.0f;
	phi.z = asinf(phi.z);
	if(phi.x <= lowerLimits_.x){
		i_ch->lambda_min = -FLT_MAX;
		i_ch->eta = -(lowerLimits_.x - phi.x) / relaxationTime_;
	}
	else
		i_ch->lambda_min = 0;
	if(phi.x >= upperLimits_.x){
		i_ch->lambda_max = FLT_MAX;
		i_ch->eta = -(upperLimits_.x - phi.x) / relaxationTime_;
	}
	else
		i_ch->lambda_max = 0;
	++i_ch;
	if(phi.y <= lowerLimits_.y){
		i_ch->lambda_min = -FLT_MAX;
		i_ch->eta = -(lowerLimits_.y - phi.y) / relaxationTime_;
	}
	else
		i_ch->lambda_min = 0;

	if(phi.y >= upperLimits_.y){
		i_ch->lambda_max = FLT_MAX;
		i_ch->eta = -(upperLimits_.y - phi.y) / relaxationTime_;
	}
	else
		i_ch->lambda_max = 0;

	++i_ch;
	if(phi.z <= lowerLimits_.z){
		i_ch->lambda_min = -FLT_MAX;
		i_ch->eta = -(lowerLimits_.z - phi.z) / relaxationTime_;
	}
	else
		i_ch->lambda_min = 0;
	if(phi.z >= upperLimits_.z){
		i_ch->lambda_max = FLT_MAX;
		i_ch->eta = -(upperLimits_.z - phi.z) / relaxationTime_;
	}
	else
		i_ch->lambda_max = 0;
}
Vect3f Joint::point() const
{
	return localPoint0_ + body0_->centreOfGravity();
}


ConstraintHandlerSimple::ConstraintHandlerSimple(RigidBodyBox* body) :
	dt_(0),
	body_(body),
	joint_(0),
	accelerationExternal_(Vect6f::ZERO),
	dv_(Vect6f::ZERO),
	inverseTOIWorld_(0, Mat3f::ZERO)
{
}
void ConstraintHandlerSimple::set(Joint* joint)
{
	joint_ = joint;
	clear();
}
void ConstraintHandlerSimple::update(vector<ContactSimple>& contacts, float dt, bool angularEvolve)
{
	dt_ = dt;
	Vect6f force;
	body_->computeForceAndTOI(force, inverseTOIWorld_, dt_, false);
	if(joint_){
		resize(6);
		joint_->compute(begin());
		dv_.multiply(inverseTOIWorld_, force);
	}
	else{
		clear();
		dv_.multiply(inverseTOIWorld_, force); 
	}
	if(contacts.size() > 0){
		vector<ContactSimple>::iterator ic;
		FOR_EACH(contacts, ic)
			ic->compute(*this, angularEvolve);
		iterator it;
		FOR_EACH(*this,it){
			it->b0.multiply(inverseTOIWorld_, it->j.matrix0());
			it->b1 = Vect6f::ZERO;
		}
		FOR_EACH(*this, it){
			it->eta *= (it->restitution - 1) / dt_;
			it->eta += it->j.matrix0().computeEta(body_->velocity6f(), dv_, dt_);
		}
	}
}
void ConstraintHandlerSimple::computeDV(int iterationLimit)
{
	if(size() > 0){
		accelerationExternal_ = Vect6f::ZERO;
		iterator it;
		FOR_EACH(*this, it){	
			accelerationExternal_.addTimes(it->b0, it->lambda / (it->restitution + 1));
			it->d = it->j.matrix0() * it->b0 + it->j.matrix1() * it->b1;
		}
		for(int iter = 0; iter < iterationLimit; ++iter)
			FOR_EACH(*this,it){
				it->lambda0 = it->lambda;
				if(it->d != 0)
					it->lambda -= (it->eta + it->j.matrix0()*accelerationExternal_) / it->d;
				it->limits();
				float temp = (it->lambda - it->lambda0) / (it->restitution + 1);
				accelerationExternal_.addTimes(it->b0,temp);
				if(it->friction > FLT_EPS){
					(it+2)->lambda_max = (it+1)->lambda_max = it->friction*it->lambda;
					(it+2)->lambda_min = (it+1)->lambda_min = -it->friction*it->lambda;
				}
			}
			FOR_EACH(*this,it){
				dv_.addTimes(it->b0, it->lambda);
			}
	}
	dv_ *= dt_;
}
