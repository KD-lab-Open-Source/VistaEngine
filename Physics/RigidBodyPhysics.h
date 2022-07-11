#ifndef __RIGID_BODY_PHYSICS_H__
#define __RIGID_BODY_PHYSICS_H__

#include "RigidBodyBase.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysics
//
///////////////////////////////////////////////////////////////

class RigidBodyPhysics : public RigidBodyBase
{
public:
	RigidBodyPhysics();
	void build(const RigidBodyPrm& prmIn, const Vect3f& boxMin, const Vect3f& boxMax, float mass);
	void checkTOI();
	void setVelocity(const Vect3f& velocity) { velocity_.linear() = velocity; }
	void setVelocity6f(const Vect6f& velocity) { velocity_ = velocity; }
	void setAngularVelocity(const Vect3f& velocity) { velocity_.angular() = velocity; }
	void addVelocity(const Vect6f& velocity) { velocity_ += velocity; }
	const Vect3f& velocity() { return velocity_.getLinear();	}
	const Vect6f& velocity6f() const { return velocity_;	}
	void translateX(float trans) { pose_.trans().x += trans; }
	void translateY(float trans) { pose_.trans().y += trans; }
	float restitution() const { return restitution_; }
	float relaxationTime() const { return relaxationTime_; }
	float friction() const { return friction_; }
	float mass() const { return mass_; }
	virtual void addContact(const Vect3f& point, const Vect3f& normal, float penetration) = 0;
	void computeForceAndTOI(Vect6f& force, MassMatrix& massMatrixWorldInverse_, float dt, bool coriolisForce);
	virtual void sleepCount(float dt);
	bool evolve(float dt);
	void applyExternalForce(const Vect3f& force, const Vect3f& point);
	void applyExternalImpulse(const Vect3f& pulse) { pulseExternal_.linear() += pulse; }
	void applyExternalImpulse(const Vect3f& pulse, const Vect3f& point);
	void setImpulse(const Vect3f& linearPulse, const Vect3f& angularPulse);
	void setTemporalForce(const Vect3f& linearForce, const Vect3f& angularForce);
	bool waterAnalysis() const { return waterAnalysis_; }
	void setWaterAnalysis(bool waterAnalysis) { waterAnalysis_ = waterAnalysis; }
	void setWaterWeight(float waterWeight) { waterWeight_ = waterWeight; }
	float waterWeight() const { return waterWeight_; }
	void setWaterLevel(float waterLevel) { waterLevel_ = waterLevel; }
	void setIceLevel(float iceLevel) { iceLevel_ = iceLevel; }
	float waterLevel() const { return waterLevel_; };
	float iceLevel() const { return iceLevel_; };
	virtual void updateRegion(float x1, float y1, float x2, float y2);
	void checkGroundCollision();
	const Se3f& posePrev() const { return posePrev_; }
	bool isFrozen() const { return isFrozen_; }
	void makeFrozen(bool onIce_);
	void unFreeze(){ isFrozen_ = false; }

protected:
	bool iceMapCheck( int xc, int yc, int r ); // true если лед присетствует во всем радиусе.
	bool iceMapCheckPrev( int xc, int yc, int r ); // true если лед присетствует во всем радиусе.
	void checkGround();

	float mass_;
	Vect3f toi_;
	Vect6f velocity_;
	float linearDamping;
	float angularDamping;
	Vect6f forceExternal_;
	Vect6f pulseExternal_;
	Vect6f forceExternalTemporal_;
	Vect3f gravity_;
	float friction_;
	float restitution_;
	float relaxationTime_;
	bool waterAnalysis_;
	float waterWeight_;
	float waterLevel_;
	float iceLevel_;
	bool isFrozen_;
	Se3f posePrev_;
};

#endif // __RIGID_BODY_PHYSICS_H__