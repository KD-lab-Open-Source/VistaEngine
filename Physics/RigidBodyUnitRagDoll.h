#ifndef __RIGID_BODY_UNIT_RAG_DOLL_H__
#define __RIGID_BODY_UNIT_RAG_DOLL_H__

#include "RigidBodyUnit.h"
#include "RigidBodyNodePrm.h"
#include "RigidBodyNode.h"
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
	~RigidBodyUnitRagDoll();
	void setModel(cObject3dx* model, cObject3dx* modelLogic, const RigidBodyModelPrm& modelPrm);
	void attach();
	void detach();
	void sleepCount(float dt);
	bool evolve(float dt);
	void updateRegion(float x1, float y1, float x2, float y2);
	bool bodyCollision(RigidBodyBase* body, ContactInfo& contactInfo);
	int computeBoxSectionPenetration(Vect3f& contactPoint, const Vect3f& sectionBegin, const Vect3f& sectionEnd) const;
	void show();
	void enableBoxMode();
	void disableBoxMode();
	void addImpulse(const Vect3f& linearPulse, const Vect3f& angularPulse);
	const Vect3f& velocity();
	void setVelocity(const Vect3f& velocity);
	void setAngularVelocity(const Vect3f& velocity);

private:
	vector<RigidBodyNode> rigidBodies_;
	vector<Joint> joints_;
	cObject3dx* model_;
	cObject3dx* modelLogic_;
};

#endif // __RIGID_BODY_UNIT_RAG_DOLL_H__
