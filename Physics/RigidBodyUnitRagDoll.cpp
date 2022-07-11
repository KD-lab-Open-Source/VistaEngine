#include "StdAfx.h"
#include "RigidBodyUnitRagDoll.h"
#include "NormalMap.h"
#include "UnitAttribute.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_UNIT_RAG_DOLL, RigidBodyUnitRagDoll)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnitRagDoll
//
///////////////////////////////////////////////////////////////

RigidBodyUnitRagDoll::RigidBodyUnitRagDoll() :
	modelLogic_(0)
{
	setRigidBodyType(RIGID_BODY_UNIT_RAG_DOLL);
}

RigidBodyUnitRagDoll::~RigidBodyUnitRagDoll()
{
	detach();
}

void RigidBodyUnitRagDoll::setModel(cObject3dx* model, cObject3dx* modelLogic, const RigidBodyModelPrm& modelPrm)
{
	model_ = model;
	modelLogic_ = modelLogic;
	int size(modelPrm.size());
	rigidBodies_.resize(size);
	float mass = 0;
	RigidBodyPrmReference rigidBodyPrm("RigidBodyNode");
	for(int i = 0; i < size; ++i){
		rigidBodies_[i].init(modelPrm[i]);
		sBox6f boundBox(modelLogic_->GetLogicBoundScaledUntransformed(modelPrm[i].logicNode));
		rigidBodies_[i].build(*rigidBodyPrm, boundBox.center(), boundBox.extent(), modelPrm[i].mass);
		mass += modelPrm[i].mass;
	}
	Vect3f toi;
	collisionObject()->getTOI(mass * prm().TOI_factor, toi);
	core()->massMatrix().setMass(mass);
	core()->massMatrix().setTOI(toi);
	vector<RigidBodyNode>::iterator irb;
	FOR_EACH(rigidBodies_, irb)
		irb->setPose(modelLogic_->GetNodePositionMats(irb->logicNode()).se());
	joints_.resize(size - 1);
	for(int i = 1; i < size; ++i)
		joints_[i-1].set(rigidBodies_[i].position(), Mat3f(rigidBodies_[i].orientation()), &rigidBodies_[i], 
			&rigidBodies_[modelPrm[i].parent], rigidBodies_[i].relaxationTime(),
			modelPrm[i].upperLimits, modelPrm[i].lowerLimits);
}

void RigidBodyUnitRagDoll::attach()
{ 
	if(isBoxMode() && !isFrozen()){
		vector<RigidBodyNode>::iterator node;
		FOR_EACH(rigidBodies_, node)
			collide_->attach(&*node);
		vector<Joint>::iterator ij;
		FOR_EACH(joints_, ij)
			collide_->attach(&*ij);
	}
}

void RigidBodyUnitRagDoll::detach()
{ 
	if(isBoxMode() && !isFrozen()){
		vector<Joint>::iterator ij;
		FOR_EACH(joints_, ij)
			collide_->detach(&*ij);
		vector<RigidBodyNode>::iterator node;
		FOR_EACH(rigidBodies_, node)
			collide_->detach(&*node);
	}
}
void RigidBodyUnitRagDoll::sleepCount(float dt)
{
	bool allSleep = true;
	vector<RigidBodyNode>::iterator irb;
	FOR_EACH(rigidBodies_, irb){
		irb->sleepCount(dt);
			if(!irb->asleep()){
				allSleep = false;
				break;
			}
	}
	if(allSleep)
		sleep();
	else
		awake();
}

bool RigidBodyUnitRagDoll::evolve(float dt)
{
	start_timer_auto();
	bool move(__super::evolve(dt));
	if(!isBoxMode()){
		modelLogic_->SetPosition(pose());
		modelLogic_->Update();
		vector<RigidBodyNode>::iterator node;
		FOR_EACH(rigidBodies_, node)
			node->setPose(modelLogic_->GetNodePositionMats(node->logicNode()).se());
	}else{
		vector<RigidBodyNode>::iterator node = rigidBodies_.begin();
		RigidBodyBox::setPose(node->computeGraphicNodePose());
		while(++node < rigidBodies_.end())
			node->updateGraphicNodeOfset(rigidBodies_, model_);
	}
	return move;
}

bool RigidBodyUnitRagDoll::bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo) 
{ 
	if(collisionObject()->bodyCollision(this, body, contactInfo)){
		vector<RigidBodyNode>::iterator irb;
		FOR_EACH(rigidBodies_, irb)
			if(irb->bodyCollision(body, contactInfo)){
				return true;
			}
	}
	return false; 
}

int RigidBodyUnitRagDoll::computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const
{
	if(__super::computeBoxSectionPenetration(contactPoint, sectionBegin, sectionEnd) != -1){
		vector<RigidBodyNode>::const_iterator irb;
		FOR_EACH(rigidBodies_, irb)
			if(irb->computeBoxSectionPenetration(contactPoint, sectionBegin, sectionEnd) != -1){
				return irb->bodyPart();
			}
	}
	return -1;
}

void RigidBodyUnitRagDoll::updateRegion(float x1, float y1, float x2, float y2)
{
	vector<RigidBodyNode>::iterator irb;
	FOR_EACH(rigidBodies_, irb)
		irb->updateRegion(x1, y1, x2, y2);
}

void RigidBodyUnitRagDoll::show()
{
	__super::show();

	vector<RigidBodyNode>::iterator irb;
	FOR_EACH(rigidBodies_, irb)
		irb->show();
}

void RigidBodyUnitRagDoll::enableBoxMode()
{
	if(isSinking())
		return;

	__super::enableBoxMode();
	
	rigidBodies_[0].setGraphicNodeOffset(modelLogic_->GetNodePositionMats(0).se());
	rigidBodies_[0].awake();
	int size = rigidBodies_.size();
	for(int i = 1; i < size; ++i){
		joints_[i-1].setPose(rigidBodies_[i].position(), Mat3f(rigidBodies_[i].orientation()));
		rigidBodies_[i].setGraphicNodeOffset(rigidBodies_);
		rigidBodies_[i].awake();
	}
}

void RigidBodyUnitRagDoll::disableBoxMode()
{
	vector<RigidBodyNode>::iterator node = rigidBodies_.begin();
	while(++node < rigidBodies_.end())
		node->resetGraphicNodeOfset(model_);

	__super::disableBoxMode();

	setAngle(atan2f(orientation().z_ * orientation().x_ + orientation().y_ * orientation().s_, 
		orientation().x_ * orientation().s_ - orientation().z_ * orientation().y_));
}

const Vect3f& RigidBodyUnitRagDoll::velocity()
{ 
	if(!isBoxMode())
		computeUnitVelocity();
	return rigidBodies_.front().core()->velocity().linear();
}

void RigidBodyUnitRagDoll::addImpulse(const Vect3f& linearPulse, const Vect3f& angularPulse)
{
	rigidBodies_.front().addImpulse(linearPulse, angularPulse);
}

void RigidBodyUnitRagDoll::setVelocity(const Vect3f& v)
{
	rigidBodies_.front().addImpulse(v * core()->mass(), Vect3f::ZERO);
}

void RigidBodyUnitRagDoll::setAngularVelocity(const Vect3f& v)
{
	rigidBodies_.front().addImpulse(Vect3f::ZERO, v * core()->mass());
}