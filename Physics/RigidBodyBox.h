#ifndef __RIGID_BODY_BOX_H__
#define __RIGID_BODY_BOX_H__

#include "RigidBodyPhysics.h"
#include "MovementDirection.h"

class JointSimple;

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
	void build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass);
	void buildGeomMesh(vector<Vect3f>& vertex, bool computeBox);
	void initPose(const Se3f& pose);
	void setPose(const Se3f& pose);
	const MovementDirection& angle() const { return angle_; }
	const Se3f& posePrev() const { return posePrev_; }
	void boxEvolve(float dt);
	void preEvolve();
	bool evolve(float dt);
	virtual void addImpulseLinear(const Vect3f& linearPulse);
	void setForwardVelocity(float v){ core()->velocity().linear().y = v; }
	void avoidCollisionAtStart();
	void startFall(const Vect3f& point);
	void setGravityZ(float gravityZ) { core()->massMatrix().setGravityZ(gravityZ); }
	void addPathTrackingAngle(float angle) { angle_ += angle; }
	void setParameters(float gravity, float friction, float restitution);
	bool onIce() { return onIce_; }
	bool isFrozen() const { return isFrozen_; }
	void makeFrozen(bool onIce_);
	void unFreeze();
	void attach();
	void detach();
	void serialize(Archive& ar);

protected:
	void setAngle(float angle) { angle_ = angle; }
	void checkGround();
	bool onIcePrev() { return onIcePrev_; }

	Se3f posePrev_;
	bool onIce_;

private:
    bool iceMapCheck( int xc, int yc, int r ); // true если лед присетствует во всем радиусе.
	bool iceMapCheckPrev( int xc, int yc, int r ); // true если лед присетствует во всем радиусе.

	MovementDirection angle_;
	JointSimple* joint_;
	bool avoidCollisionAtStart_;
	bool onIcePrev_;
	bool isFrozen_;
	float iceLevel_;
};

#endif // __RIGID_BODY_BOX_H__
