#ifndef __RIGID_BODY_UNIT_RAG_DOLL_H__
#define __RIGID_BODY_UNIT_RAG_DOLL_H__

#include "RigidBodyUnit.h"
#include "RigidBodyNodePrm.h"
#include "Math\ConstraintHandler.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnitRagDoll
//
///////////////////////////////////////////////////////////////

class RigidBodyUnitRagDoll : public RigidBodyUnit
{
public:
	RigidBodyUnitRagDoll();
	void setModel(cObject3dx* model, cObject3dx* modelLogic, const RigidBodyModelPrm& modelPrm);
	void sleepCount(float dt);
	void boxEvolve(float dt);
	bool evolve(float dt);
	void updateRegion(float x1, float y1, float x2, float y2);
	bool bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo);
	int computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const;
	void show();
	void enableBoxMode();
	void disableBoxMode();
	void setImpulse(const Vect3f& linearPulse);

private:
	vector<RigidBodyNode> rigidBodies_;
	vector<Joint> joints_;
	vector<JointSpringDumping> springDumpings_;
	vector<Contact> contacts_;
	ConstraintHandler collide_;
	cObject3dx* model_;
	cObject3dx* modelLogic_;
};

#endif // __RIGID_BODY_UNIT_RAG_DOLL_H__
