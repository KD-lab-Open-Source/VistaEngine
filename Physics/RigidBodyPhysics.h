#ifndef __RIGID_BODY_PHYSICS_H__
#define __RIGID_BODY_PHYSICS_H__

#include "Math\Vect6f.h"
#include "RigidBodyBase.h"

class ConstraintHandler;
class ContactSimple;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysicsCore
//
///////////////////////////////////////////////////////////////

template<class VectXf, class MassMatrix>
class RigidBodyPhysicsCore
{
public:
	RigidBodyPhysicsCore(const VectXf& vectXfZero = VectXf::ZERO, 
		const MassMatrix& massMatrixZero = MassMatrix::ZERO) :
		massMatrix_(massMatrixZero),
		velocity_(vectXfZero),
		force_(vectXfZero),
		accelerationExternal_(vectXfZero),
		dv_(vectXfZero),
		pulseExternal_(vectXfZero)
	{
	}
	float mass() const { return massMatrix_.mass(); }
	MassMatrix& massMatrix() { return massMatrix_; }
	const MassMatrix& massMatrix() const { return massMatrix_; }
	VectXf& velocity() { return velocity_; }
	void addImpulse(const VectXf& pulse) { pulseExternal_ += pulse; }
	VectXf& pulseExternal() { return pulseExternal_; }
	void computeForceAndTOI(const Mat3f& r, float dt, bool coriolisForce)
	{
		accelerationExternal_ = VectXf::ZERO;
		massMatrix_.computePermanentForce(force_, r, velocity_, coriolisForce);
		pulseExternal_ /= dt;
		force_ += pulseExternal_;
		pulseExternal_ = VectXf::ZERO;
	}
	float computeDet(VectXf& b, const VectXf& j) { return massMatrix_.multiply(b, j).dot(j); }
	void computeAccelerationExternal() { massMatrix_.multiply(dv_, force_); }
	void preciseAccelerationExternal(const VectXf& b, float lambda) { accelerationExternal_.scaleAdd(b, lambda); }
	float computeEta(const VectXf& j, float dt) const 
	{ 
		VectXf temp = dv_;
		temp.scaleAdd(velocity_, 1.0f / dt);
		return j.dot(temp);
	}
	float preciseEta(const VectXf& j) const { return accelerationExternal_.dot(j); }
	void preciseDV(const VectXf& b, float lambda) { dv_.scaleAdd(b, lambda); }
	void evolve(float dt) 
	{ 
		velocity_.scaleAdd(dv_, dt);
	}
	
private:
	MassMatrix massMatrix_;
	VectXf velocity_;
	VectXf force_;
	VectXf accelerationExternal_;
	VectXf dv_;
	VectXf pulseExternal_;
};

typedef RigidBodyPhysicsCore<Vect6f, MassMatrixFull> RigidBodyPhysicsCore6f;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysicsBase
//
///////////////////////////////////////////////////////////////

class RigidBodyPhysicsBase : public RigidBodyBase
{
public:
	RigidBodyPhysicsBase();
	~RigidBodyPhysicsBase() { xassert(!attached()); }
	static void initConstraintHandler(ConstraintHandler* collide) { collide_ = collide; }
	void setIndex(int index) { index_ = index; }
	int index() { return index_; }
	bool attached() const { return index_ >= 0; }
	float friction() const { return friction_; }
	void setFriction(float friction) { friction_ = friction; }
	float restitution() const { return restitution_; }
	float relaxationTime() const { return relaxationTime_; }
	
	virtual void preEvolve() {}
	virtual void computeForceAndTOI(float dt);
	virtual void computeAccelerationExternal() = 0;
	virtual void boxEvolve(float dt) = 0;
	
	virtual void addWheelContact(const Vect3f& point, const Vect3f& normal, float penetration, int wheelIndex) {}

	void setWaterLevel(float waterLevel) { waterLevel_ = waterLevel; }
	float waterLevel() const { return waterLevel_; }

protected:
	static ConstraintHandler* collide_;
	int index_;
	float friction_;
	float restitution_;
	float relaxationTime_;
	float waterLevel_;
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyPhysics
//
///////////////////////////////////////////////////////////////

class RigidBodyPhysics : public RigidBodyPhysicsBase
{
public:
	RigidBodyPhysics();
	void build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass);
	
	void setVelocity(const Vect3f& velocity) { core_.velocity().linear() = velocity; }
	virtual void setAngularVelocity(const Vect3f& velocity) { core_.velocity().angular() = velocity; }
	const Vect3f& velocity() { return core_.velocity().linear(); }
	
	void computeForceAndTOI(float dt);
	void computeAccelerationExternal() { core()->computeAccelerationExternal(); }
	void boxEvolve(float dt);

	void addContact(const Vect3f& point, const Vect3f& normal, float penetration);
	
	virtual void sleepCount(float dt);
	virtual void addImpulse(const Vect3f& linearPulse, const Vect3f& angularPulse)
	{
		core()->addImpulse(Vect6f(linearPulse, angularPulse));
	}
	void addPointImpulse(const Vect3f& point, const Vect3f& linearPulse)
	{
		Vect3f angularPulse;
		angularPulse.sub(point, centreOfGravity());
		angularPulse.postcross(linearPulse);
		addImpulse(linearPulse, angularPulse);
	}
	bool waterAnalysis() const { return waterAnalysis_; }
	void setWaterAnalysis(bool waterAnalysis) { waterAnalysis_ = waterAnalysis; }
	void setWaterWeight(float waterWeight) { waterWeight_ = waterWeight; }
	virtual void updateRegion(float x1, float y1, float x2, float y2);
	bool angularEvolve() { return angularEvolve_; }
	void setAngularEvolve(bool enable) { angularEvolve_ = enable; }
	RigidBodyPhysicsCore6f* core() { return &core_; }
	void serialize(Archive& ar);

protected:
	float waterWeight() const { return waterWeight_; }
		
private:
	RigidBodyPhysicsCore6f core_;
	bool angularEvolve_;
	bool waterAnalysis_;
	float waterWeight_;
};

#endif // __RIGID_BODY_PHYSICS_H__