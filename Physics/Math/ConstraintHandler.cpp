#include "StdAfx.h"
#include "..\RigidBodyPrm.h"
#include "ConstraintHandler.h"

ConstraintHandler::ConstraintHandler(vector<RigidBodyNode>& bodies, vector<Joint>& joint, vector<JointSpringDumping>& springDumpings) :
	dt_(0),
	bodies_(bodies),
	joints_(joint),
	springDumpings_(springDumpings)
{
	accelerationExternal_.resize(bodies_.size());
	dv_.resize(bodies_.size());
	forceAndTOI_.resize(bodies_.size());
}
void ConstraintHandler::reinitialize()
{
    accelerationExternal_.resize(bodies_.size());
	dv_.resize(bodies_.size());
	forceAndTOI_.resize(bodies_.size());
	clear();
}
void ConstraintHandler::update(vector<Contact>& contacts, float dt)
{
	dt_ = dt;
	vector<RigidBodyNode>::iterator irb;
	SparsityMatrix::iterator itoi = forceAndTOI_.begin();
	FOR_EACH(bodies_, irb){
		irb->computeForceAndTOI(itoi->force(), itoi->inverseTOIWorld(), dt_, true);
		++itoi;
	}
	resize(joints_.size() * 6 + springDumpings_.size() * 6);
	if(contacts.size() + joints_.size() > 0){
		iterator it = begin();
		vector<Joint>::iterator ij;
		FOR_EACH(joints_, ij){
			ij->compute(it);
			it += 6;
		}
		vector<JointSpringDumping>::iterator isd;
		FOR_EACH(springDumpings_, isd){
			isd->compute(it, forceAndTOI_);
			it += 6;
		}
		forceAndTOI_.computeAccelerationExternal(dv_);
		vector<Contact>::iterator ic;
		FOR_EACH(contacts, ic)
			ic->compute(*this);
		forceAndTOI_.computeBMatrix(*this, dt);
		FOR_EACH(*this, it){
			it->eta *= (it->restitution - 1) / dt_;
			SparsityRowSimple& rowSimple(it->j);
			if(rowSimple.map1() >= 0)
				it->eta += rowSimple.matrix1().computeEta(bodies_[rowSimple.map1()].velocity6f(), dv_[rowSimple.map1()], dt_);
			it->eta += rowSimple.matrix0().computeEta(bodies_[rowSimple.map0()].velocity6f(), dv_[rowSimple.map0()], dt_);
		}
	}else{
		forceAndTOI_.computeAccelerationExternal(dv_);
	}
}
void ConstraintHandler::computeDV(int iterationLimit)
{
	if(size() > 0){
		accelerationExternal_ = Vect6f::ZERO;
		iterator it;
		FOR_EACH(*this, it){
			SparsityRowSimple& rowSimple(it->j);
			accelerationExternal_[rowSimple.map0()].addTimes(it->b0, it->lambda / (it->restitution + 1));
			if(rowSimple.map1() >= 0)
				accelerationExternal_[rowSimple.map1()].addTimes(it->b1, it->lambda / (it->restitution + 1));
			it->d = rowSimple.matrix0() * it->b0 + rowSimple.matrix1() * it->b1;
		}
		for(int iter = 0; iter < iterationLimit; ++iter)
			FOR_EACH(*this,it){
				it->lambda0 = it->lambda;
				xassert(it->d != 0);
				SparsityRowSimple& rowSimple(it->j);
				it->lambda -= (it->eta + rowSimple.matrix0()*accelerationExternal_[rowSimple.map0()]) / it->d;
				if(rowSimple.map1() >= 0)
					it->lambda -= rowSimple.matrix1() * accelerationExternal_[rowSimple.map1()] / it->d;
				it->limits();
				float temp = (it->lambda - it->lambda0) / (it->restitution + 1);
				accelerationExternal_[rowSimple.map0()].addTimes(it->b0,temp);
				if(rowSimple.map1() >= 0)
					accelerationExternal_[rowSimple.map1()].addTimes(it->b1,temp);
				if(it->friction > FLT_EPS){
					(it+2)->lambda_max = (it+1)->lambda_max = it->friction*it->lambda;
					(it+2)->lambda_min = (it+1)->lambda_min = -it->friction*it->lambda;
				}
			}
		FOR_EACH(*this,it){
			SparsityRowSimple& rowSimple(it->j);
			if(rowSimple.map1() >= 0)
				dv_[rowSimple.map1()].addTimes(it->b1, it->lambda);
			dv_[rowSimple.map0()].addTimes(it->b0, it->lambda);
		}
	}
	dv_ *= dt_;
}
