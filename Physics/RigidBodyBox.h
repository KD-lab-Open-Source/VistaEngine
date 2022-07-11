#ifndef __RIGID_BODY_BOX_H__
#define __RIGID_BODY_BOX_H__

#include "RigidBodyPhysics.h"
#include "Math//ConstraintHandlerSimple.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyBox
//
///////////////////////////////////////////////////////////////

class RigidBodyBox : public RigidBodyPhysics
{
public:
	RigidBodyBox();
	~RigidBodyBox();
	void build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass);
	void buildGeomMesh(vector<Vect3f>& vertex, bool computeBox);
	void initPose(const Se3f& pose);
	void setPose(const Se3f& pose);
	void addVelocity(const Vect6f& velocity);
	void addContact(const Vect3f& point, const Vect3f& normal, float penetration);
	virtual void boxEvolve(float dt);
	bool evolve(float dt);
	void evolveHoldingOrientation(float dt);
	void enableAngularEvolve(bool enable = true) { angularEvolve = enable; }
	virtual void setImpulse(const Vect3f& linearPulse);
	void setTemporalForce(const Vect3f& linearForce);
	void setForwardVelocity(float v){ velocity_.linear().y = v; }
	void avoidCollisionAtStart();
	void startFall(const Vect3f& point);
	float angle() const { return angle_; }
	void setAngle(float angle);
	void setGravityZ(float gravityZ) { gravity_.z = gravityZ; }

protected:
	Joint* joint_;
	vector<ContactSimple> contacts_;
	ConstraintHandlerSimple collide_;
	float dtRest_;
	bool angularEvolve;
	float angle_;
	bool avoidCollisionAtStart_;
};

#endif // __RIGID_BODY_BOX_H__
