#include "StdAfx.h"
#include "RigidBodyPrm.h"
#include "RigidBodyUnitRagDoll.h"
#include "NormalMap.h"
#include "..\Units\UnitAttribute.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_UNIT_RAG_DOLL, RigidBodyUnitRagDoll)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnitRagDoll
//
///////////////////////////////////////////////////////////////

RigidBodyUnitRagDoll::RigidBodyUnitRagDoll() :
	collide_(rigidBodies_, joints_, springDumpings_),
	modelLogic_(0)
{
	rigidBodyType = RIGID_BODY_UNIT_RAG_DOLL;
}

void RigidBodyUnitRagDoll::setModel(cObject3dx* model, cObject3dx* modelLogic, const RigidBodyModelPrm& modelPrm)
{
	model_ = model;
	modelLogic_ = modelLogic;
	int size(modelPrm.size());
	rigidBodies_.resize(size);
	RigidBodyModelPrm::const_iterator nodePrm(modelPrm.begin());
	vector<RigidBodyNode>::iterator node;
	RigidBodyPrmReference rigidBodyPrm("RigidBodyNode");
	int i = 0;
	FOR_EACH(rigidBodies_, node){
		node->init(i, &contacts_, *nodePrm);
		sBox6f boundBox(modelLogic_->GetLogicBoundScaledUntransformed(nodePrm->logicNode));
		node->build(*rigidBodyPrm, boundBox.min, boundBox.max, nodePrm->mass);
		++nodePrm;
		++i;
	}
	vector<RigidBodyNode>::iterator irb;
	FOR_EACH(rigidBodies_, irb)
		irb->setPose(modelLogic_->GetNodePositionMats(irb->logicNode()).se());
	i = 1;
	joints_.resize(size - 1);
	node = rigidBodies_.begin() + 1;
	nodePrm = modelPrm.begin() + 1;
	vector<Joint>::iterator ij;
	FOR_EACH(joints_, ij){	
		ij->set(node->position(), Mat3f(node->orientation()), &*(node), 
			&*(rigidBodies_.begin() + nodePrm->parent), i, nodePrm->parent, node->relaxationTime(),
			nodePrm->upperLimits, nodePrm->lowerLimits);
		++node;
		++nodePrm;
		++i;
	}
	collide_.reinitialize();
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

void RigidBodyUnitRagDoll::boxEvolve(float dt)
{
	start_timer_auto();
	vector<RigidBodyNode>::iterator irb;
	start_timer(1);
	FOR_EACH(rigidBodies_, irb)
		irb->checkGroundCollision();
	stop_timer(1);
	start_timer(2);
	collide_.update(contacts_, dt);
	stop_timer(2);
	start_timer(3);
	collide_.computeDV(10);
	stop_timer(3);
	contacts_.clear();
	float trans_x(0), trans_y(0);
	VectN6f::const_iterator idv = collide_.getDV().begin();
	FOR_EACH(rigidBodies_, irb){
		irb->addVelocity(*idv);
		irb->evolve(dt);
		float temp;
		temp = vMap.H_SIZE - 1 - irb->maxRadius() - irb->position().x;
		if(temp < 0 && temp < trans_x)
			trans_x = temp;
		else{
			temp = irb->maxRadius() - irb->position().x;
			if(temp > 0 && temp > trans_x)
				trans_x = temp;
		}
		temp = vMap.V_SIZE - 1 - irb->maxRadius() - irb->position().y;
		if(temp < 0 && temp < trans_y)
			trans_y = temp;
		else{
			temp = irb->maxRadius() - irb->position().y;
			if(temp > 0 && temp > trans_y)
				trans_y = temp;
		}
		++idv;
	}
	if(fabsf(trans_x) > FLT_EPS || fabsf(trans_y) > FLT_EPS)
		FOR_EACH(rigidBodies_, irb){
			irb->translateX(trans_x);
			irb->translateY(trans_y);
		}
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
		node->computeGraphicNodePose(pose_);
		while(++node < rigidBodies_.end()){
			node->updateGraphicNodeOfset(rigidBodies_, model_);
		}
	}
	return move;
}

bool RigidBodyUnitRagDoll::bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo) 
{ 
	if(geom()->bodyCollision(this, body, contactInfo)){
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
	if(__super::computeBoxSectionPenetration(contactPoint, sectionBegin, sectionEnd)){
		vector<RigidBodyNode>::const_iterator irb;
		FOR_EACH(rigidBodies_, irb)
			if(irb->computeBoxSectionPenetration(contactPoint, sectionBegin, sectionEnd)){
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
	__super::enableBoxMode();
	
	vector<Joint>::iterator ij;
	vector<RigidBodyNode>::iterator node = rigidBodies_.begin();
	node->setGraphicNodeOffset(modelLogic_->GetNodePositionMats(0).se());
	node->awake();
	FOR_EACH(joints_, ij){	
		++node;
		ij->setPose(node->position(), Mat3f(node->orientation()));
		node->setGraphicNodeOffset(rigidBodies_);
		node->awake();
	}
}

void RigidBodyUnitRagDoll::disableBoxMode()
{
	__super::disableBoxMode();

	vector<RigidBodyNode>::iterator node = rigidBodies_.begin();
	while(++node < rigidBodies_.end())
		node->resetGraphicNodeOfset(model_);
}

void RigidBodyUnitRagDoll::setImpulse(const Vect3f& linearPulse)
{
	vector<RigidBodyNode>::iterator node;
	FOR_EACH(rigidBodies_, node)
		node->setImpulse(linearPulse, Vect3f::ZERO);
}