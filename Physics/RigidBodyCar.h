#ifndef __RIGID_BODY_CAR_H__
#define __RIGID_BODY_CAR_H__

#include "Math\ConstraintHandlerSimple.h"
#include "NormalMap.h"

class RigidBodyCar;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyWheel
//
///////////////////////////////////////////////////////////////

class RigidBodyWheel : public RigidBodyCollision
{
public:
	///////////////////////////////////////////////////////////
	//
	//    class JointWheel
	//
	///////////////////////////////////////////////////////////

	class JointWheel : public JointBase
	{
	public:
		JointWheel(RigidBodyCar* body, int wheelIndex,
			float upperLimit, float lowerLimit, float drivingForceLimit);
		void setDesiredVelocity(float desiredVelocity) 
		{ 
			desiredVelocity_ = desiredVelocity; 
			enableDriving_ = true;
		}
		void addDesiredVelocity(float desiredVelocity) { desiredVelocity_ += desiredVelocity; }
		void disableDriving() { enableDriving_ = false; }
		void compute(ConstraintHandler& ch) const;

	private:
		RigidBodyCar* body_;
		int wheelIndex_;
		float upperLimit_;
		float lowerLimit_;
		bool enableDriving_;
		float desiredVelocity_;
		float drivingForceLimit_;
	};

	///////////////////////////////////////////////////////////

	RigidBodyWheel(int index, RigidBodyCar* parent, const Vect3f& wheelDisplacement, 
				   float radius, int graphicNodeSuspension, int graphicNodeWheel, 
				   float k, float mu, int wheelType, float upperLimit, float lowerLimit, 
				   float drivingForceLimit, bool waterAnalysis, float waterLevel);
	void addContact(const Vect3f& point, const Vect3f& normal, float penetration) 
	{
		parent_->addWheelContact(point, normal, penetration, index_);
	}
	const Vect2f& wheelPose() const { return wheelPose_; }
	Vect3f wheelAxisLinear() const 
	{ 
		Vect3f wheelAxisLinear = wheelAxes_.linear();
		return parent_->pose().xformVect(wheelAxisLinear);
	}
	Vect3f wheelAxisAngular() const 
	{ 
		Vect3f wheelAxisAngular = wheelAxes_.angular();
		QuatF(steeringAngle_, wheelAxes_.linear()).xform(wheelAxisAngular);
		return parent_->pose().xformVect(wheelAxisAngular);
	}
	Vect3f wheelDisplacement() const
	{
		Vect3f wheelDisplacementGlobal;
		return parent_->orientation().xform(wheelDisplacement_, wheelDisplacementGlobal);
	}
	void boxEvolve(const QuatF& orientation, const Vect2f& velocity, float dt)
	{
		wheelPose_ += velocity * dt;
		updatePose();
	}
	void updatePose()
	{
		Se3f poseNew;
		poseNew.trans() = wheelDisplacement();
		poseNew.trans().scaleAdd(wheelAxisLinear(), wheelPose_.x);
		poseNew.trans() += parent_->centreOfGravity();
		poseNew.rot().set(wheelPose_.y, wheelAxes_.angular());
		poseNew.rot().premult(QuatF(steeringAngle_, wheelAxes_.linear()));
		poseNew.rot().premult(parent_->orientation());
		setPose(poseNew);
	}
	void updateGraphicNodeOfset(cObject3dx* model)
	{
		Se3f nodePose;
		nodePose.trans().scale(wheelAxes_.linear(), wheelPose_.x);
		nodePose.rot() = QuatF::ID;
		nodeInterpolatorSuspension_ = nodePose;
		nodeInterpolatorSuspension_(model, graphicNodeSuspension_);
		nodePose.trans() = Vect3f::ZERO;
		nodePose.rot().set(wheelPose_.y, wheelAxes_.angular());
		nodePose.rot().premult(QuatF(steeringAngle_, wheelAxes_.linear()));
		nodeInterpolatorWheel_ = nodePose;
		nodeInterpolatorWheel_(model, graphicNodeWheel_);
	}
	void computePermanentForce(Vect6f& force6f, Vect2f& force, const Vect3f& gravity, const Vect2f& velocity, float mass) const
	{
		float forceSpringDamping = -(k_ * wheelPose_.x) - (mu_ * velocity.x);
		force.x = wheelAxisLinear().dot(gravity) + forceSpringDamping;
		force.y = 0.0f;
		Vect3f forceLinear;
		forceLinear.scale(wheelAxisLinear(), -forceSpringDamping);
		force6f.linear() += forceLinear;
		Vect3f forceAngular = wheelDisplacement();
		forceAngular.postcross(forceLinear);
		force6f.angular() += forceAngular;
	}
	float waterLevel() const { return waterLevel_; }
	float lavaLevel() const { return lavaLevel_; }
	void setLavaLevel(float level) { lavaLevel_ = level; }
	bool groundCollision() { return waterAnalysis_ && checkLowWater(position(), boundRadius()) ? collisionObject()->waterCollision(this) : collisionObject()->groundCollision(this); }
	void setSteeringAngle(float steeringAngle)
	{
		if((wheelType_ & RigidBodyWheelPrm::STEERING_WHEEL) && (wheelType_ & RigidBodyWheelPrm::FRONT_WHEEL))
			steeringAngle_ = steeringAngle;
		if((wheelType_ & RigidBodyWheelPrm::STEERING_WHEEL) && (wheelType_ & RigidBodyWheelPrm::REAR_WHEEL))
			steeringAngle_ = -steeringAngle;
	}
	void setSteeringVelocity(float steeringVelocity)
	{
		joint_.addDesiredVelocity(steeringVelocity * wheelDisplacement_.x / boundRadius());
	}
	void setDrivingVelocity(float velocityNew)
	{
		if((wheelType_ & RigidBodyWheelPrm::DRIVING_WHEEL) || fabsf(velocityNew) < FLT_EPS)
			joint_.setDesiredVelocity(velocityNew / boundRadius());
		else
			joint_.disableDriving();
	}
	void attachJoint(ConstraintHandler* collide) { return collide->attach(&joint_); }
	void detachJoint(ConstraintHandler* collide) { return collide->detach(&joint_); }
	void show();

private:
	int index_;
	RigidBodyPhysicsBase* parent_;
	Vect2f wheelPose_;
	Vect6f wheelAxes_;
	Vect3f wheelDisplacement_;
	int graphicNodeSuspension_;
	int graphicNodeWheel_;
	InterpolatorNodeTransform nodeInterpolatorSuspension_;
	InterpolatorNodeTransform nodeInterpolatorWheel_;
	float k_;
	float mu_;
	float steeringAngle_;
	int wheelType_;
	bool waterAnalysis_;
	float waterLevel_;
	float lavaLevel_;
	JointWheel joint_;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrix2f
//
///////////////////////////////////////////////////////////////

class MassMatrix2f
{
public:
	MassMatrix2f() :
		massMatrix_(Vect2f::ID),
		wheel_(0)
	{
	}
	~MassMatrix2f()
	{
		delete wheel_;
	}
	void set(RigidBodyWheel* wheel, const Vect2f& massMatrix)
	{
		wheel_ = wheel;
		massMatrix_ = massMatrix;
	}
	RigidBodyWheel& wheel() const { return *wheel_; }
	void multiply(Vect2f& a, const Vect2f& b) const
	{
		a = massMatrix_;
		a *= b;
	}
	void square(const MassMatrix2f& m)
	{
		massMatrix_ = m.massMatrix_;
		massMatrix_ *= m.massMatrix_;
	}
	void invert(const MassMatrix2f& m)
	{
		massMatrix_ = Vect2f::ID;
		massMatrix_ /= m.massMatrix_;
	}
	void computePermanentForce(Vect6f& force6f, Vect2f& force, const Vect3f& gravity, const Vect2f& velocity) const
	{
		wheel_->computePermanentForce(force6f, force, gravity, velocity, massMatrix_.x);
	}
	void setLavaLevel(float level) { wheel_->setLavaLevel(level); }

protected:
	Vect2f massMatrix_;
	RigidBodyWheel* wheel_;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrix6fN2f
//
///////////////////////////////////////////////////////////////

class MassMatrix6fN2f
{
public:
	class MassMatrix6f2f;

	MassMatrix6fN2f(int n) :
		massMatrixN2f_(n)
	{
	}
	MassMatrix6fN2f(const MassMatrix6f2f& b, int n) : 
		massMatrix6f_(b.massMatrix6f_), 
		massMatrixN2f_(n, b.massMatrix2f_) 
	{
	}
	void initWheel(int wheelIndex, RigidBodyWheel* wheel, const Vect2f& massMatrix)
	{
		massMatrixN2f_[wheelIndex].set(wheel, massMatrix);
	}
	void setMass(float mass) { massMatrix6f_.setMass(mass); }
	void setTOI(const Vect3f& toi) { massMatrix6f_.setTOI(toi); }
	const Vect6fN2f& multiply(Vect6fN2f& a, const Vect6fN2f& b) const
	{
		massMatrix6f_.multiply(a.vect6f(), b.vect6f());
		int n = massMatrixN2f_.size();
		for(int i = 0; i < n; ++i)
			massMatrixN2f_[i].multiply(a.vectN2f()[i], b.vectN2f()[i]);
		return a;
	}
	void square(const MassMatrix6fN2f& m)
	{
		massMatrix6f_.square(m.massMatrix6f_);
		int n = massMatrixN2f_.size();
		for(int i = 0; i < n; ++i)
			massMatrixN2f_[i].square(m.massMatrixN2f_[i]);
	}
	MassMatrix6fN2f& invert(const MassMatrix6fN2f& m)
	{
		massMatrix6f_.invert(m.massMatrix6f_);
		int n = massMatrixN2f_.size();
		for(int i = 0; i < n; ++i)
			massMatrixN2f_[i].invert(m.massMatrixN2f_[i]);
		return *this;
	}
	void computePermanentForce(Vect6fN2f& force, const Mat3f& r, const Vect3f& gravity, const Vect6fN2f& velocity, bool coriolisForce)
	{
		massMatrix6f_.computePermanentForce(force.vect6f(), r, gravity, velocity.vect6f(), coriolisForce);
		int n = massMatrixN2f_.size();
		for(int i = 0; i < n; ++i)
			massMatrixN2f_[i].computePermanentForce(force.vect6f(), force.vectN2f()[i], gravity, velocity.vectN2f()[i]);
	}
	void setLavaLevel(float level)
	{
		vector<MassMatrix2f>::const_iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().setLavaLevel(level);
	}
	void computeTOIWorld(const Mat3f& r)
	{
		massMatrix6f_.computeTOIWorld(r);
	}
	Vect6fN2f computeJ(const Vect3f& axis, const Vect3f& localPoint, int wheelIndex)
	{
		if(wheelIndex < 0)
			return Vect6fN2f(massMatrix6f_.computeJ(axis, localPoint), massMatrixN2f_.size());
		Vect6fN2f j(axis, axis, massMatrixN2f_.size());
		RigidBodyWheel& wheel = massMatrixN2f_[wheelIndex].wheel();
		Vect3f wheelAxes = wheel.wheelAxisLinear();
		j.vectN2f()[wheelIndex].x = wheelAxes.dot(axis);
		Vect3f wheelLocalPoint = localPoint;
		wheelLocalPoint.sub(wheel.wheelDisplacement());
		wheelLocalPoint.postcross(axis);
		j.vectN2f()[wheelIndex].y = wheelLocalPoint.dot(wheel.wheelAxisAngular());
		j.vect6f().angular().precross(wheel.wheelDisplacement());
		return j;
	}
	Vect6fN2f computeWheelJ(int wheelIndex)
	{
		RigidBodyWheel& wheel = massMatrixN2f_[wheelIndex].wheel();
		return Vect6fN2f(massMatrix6f_.computeJ(wheel.wheelAxisLinear(), 
			wheel.wheelDisplacement()), massMatrixN2f_.size());
	}
	Vect6fN2f computeDrivingWheelJ(int wheelIndex)
	{
		return Vect6fN2f(Vect6f(Vect3f::ZERO, 
			-massMatrixN2f_[wheelIndex].wheel().wheelAxisAngular()), 
			massMatrixN2f_.size());
	}
	const Vect2f& wheelPose(int wheelIndex) const
	{
		return massMatrixN2f_[wheelIndex].wheel().wheelPose();
	}
	bool groundCollision() const
	{
		bool colliding = false;
		vector<MassMatrix2f>::const_iterator i;
		FOR_EACH(massMatrixN2f_, i)
			colliding |= i->wheel().groundCollision();
		return colliding;
	}
	void updateWheelsPose()
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().updatePose();
	}
	void evolve(Se3f& pose, Vect6fN2f& velocity, float dt, bool angularEvolve) const
	{
		massMatrix6f_.evolve(pose, velocity.vect6f(), dt, angularEvolve);
		int n = massMatrixN2f_.size();
		for(int i = 0; i < n; ++i)
			massMatrixN2f_[i].wheel().boxEvolve(pose.rot(), velocity.vectN2f()[i], dt);
	}
	void setWheelVelocity(float velocityNew)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().setDrivingVelocity(velocityNew);
	}
	void attachWheels(ConstraintHandler* collide)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().attachJoint(collide);
	}
	void detachWheels(ConstraintHandler* collide)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().detachJoint(collide);
	}
	void show()
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().show();
	}
	void updateGraphicNodeOfset(cObject3dx* model)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().updateGraphicNodeOfset(model);
	}
	void setSteeringAngle(float steeringAngle)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().setSteeringAngle(steeringAngle);
	}
	void setSteeringVelocity(float steeringVelocity)
	{
		vector<MassMatrix2f>::iterator i;
		FOR_EACH(massMatrixN2f_, i)
			i->wheel().setSteeringVelocity(steeringVelocity);
	}

	class MassMatrix6f2f
	{
	public:
		friend MassMatrix6fN2f::MassMatrix6fN2f(const MassMatrix6f2f& b, int n);
		
	private:
		MassMatrix6f massMatrix6f_;
		MassMatrix2f massMatrix2f_;
	};

private:
	MassMatrix6f massMatrix6f_;
	vector<MassMatrix2f> massMatrixN2f_;

public:
	static const MassMatrix6f2f ZERO;
};

///////////////////////////////////////////////////////////////
//
//    class MassMatrixFull6fN2f
//
///////////////////////////////////////////////////////////////

class MassMatrixFull6fN2f : public MassMatrixNXf<MassMatrix6fN2f, Vect6fN2f>
{
public:
	MassMatrixFull6fN2f(const MassMatrix6f2f& b, int n) : 
		MassMatrixNXf<MassMatrix6fN2f, Vect6fN2f>(MassMatrix6fN2f(b, n))
	{
	}
	void setTOI(const Vect3f& toi)
	{
		__super::setTOI(toi);
		inverseTOI_.invert(*this);
	}
};

///////////////////////////////////////////////////////////////
//
//    class ContactSimple6fN2f
//
///////////////////////////////////////////////////////////////

class ContactSimple6fN2f : public ConstraintBase
{
public:
	ContactSimple6fN2f(const Vect3f& point, const Vect3f& normal, RigidBodyCar* body0, float penetration, int wheelIndex);
	void compute(ConstraintHandler& ch) const;
	void* operator new(size_t)
	{
		return PhysicsPool<ContactSimple6fN2f>::get();
	}
	void operator delete(void*)
	{
		xxassert(false, "попытка удаления объекта из PhysicsPool");
	}
	
protected:
	Vect3f point_;
	int wheelIndex_;
	Mat3f axes_;
	RigidBodyCar* body0_;
	float penetration_;
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyCar
//
///////////////////////////////////////////////////////////////

class RigidBodyCar : public RigidBodyPhysicsBase
{
public:
	RigidBodyCar(int n, bool turnInPosition) :
		core_(Vect6fN2f(Vect6fN2f::ZERO, n), MassMatrixFull6fN2f(MassMatrixFull6fN2f::ZERO, n)),
		model_(0),
		turnInPosition_(turnInPosition),
		waterAnalysis_(false)
	{
		setColor(Color4c::RED);
	}
	void initWheel(int wheelIndex, RigidBodyWheel* wheel, const Vect2f& massMatrix)
	{
		core_.massMatrix().initWheel(wheelIndex, wheel, massMatrix);
	}
	void build(const RigidBodyPrm& prmIn, const Vect3f& center, const Vect3f& extent, float mass);
	void setModel(cObject3dx* model) { model_ = model; }
	const Vect3f& velocity() { return core_.velocity().vect6f().linear(); }
	void setVelocity(const Vect3f& velocity) { core_.velocity().vect6f().linear() = velocity; }
	void setPose(const Se3f& pose)
	{
		__super::setPose(pose);
		core_.massMatrix().updateWheelsPose();
	}
	void addWheelContact(const Vect3f& point, const Vect3f& normal, float penetration, int wheelIndex) 
	{
		if(attached())
			collide_->addContact(new ContactSimple6fN2f(point, normal, this, penetration, wheelIndex));
	}
	void setLavaLevel(float level)
	{
		core_.massMatrix().setLavaLevel(level);
	}
	void addContact(const Vect3f& point, const Vect3f& normal, float penetration)
	{	
		addWheelContact(point, normal, penetration, -1);
	}
	bool waterAnalysis() { return waterAnalysis_; }
	void setWaterAnalysis(bool waterAnalysis) { waterAnalysis_ = waterAnalysis; }
	void computeForceAndTOI(float dt);
	void computeAccelerationExternal() { core()->computeAccelerationExternal(); }
	void boxEvolve(float dt)
	{
		core_.evolve(dt);
		Se3f poseNew(pose());
		core_.massMatrix().evolve(poseNew, core()->velocity(), dt, true);
		setPose(poseNew);
	}
	RigidBodyPhysicsCore<Vect6fN2f, MassMatrixFull6fN2f>* core() { return &core_; }
	const Vect2f& wheelPose(int wheelIndex) const
	{
		return core_.massMatrix().wheelPose(wheelIndex);
	}
	void attach()
	{ 
		collide_->attach(this);
		core_.massMatrix().attachWheels(collide_);
	}
	void detach()
	{ 
        core_.massMatrix().detachWheels(collide_);
		collide_->detach(this);
	}
	void show()
	{
		__super::show();
		if(attached())
			core()->massMatrix().show();
	}
	void updateGraphicNodeOfset()
	{
		if(attached())
			core()->massMatrix().updateGraphicNodeOfset(model_);
	}
	void update(float velocityNew, float rotAngle, bool moveback, float dt)
	{
		core()->massMatrix().setWheelVelocity(moveback ? -velocityNew : velocityNew);
		if(turnInPosition_)
			core()->massMatrix().setSteeringVelocity(1.5f * rotAngle / dt);
		else
			core()->massMatrix().setSteeringAngle(atan2f(15.0f * (moveback ? -rotAngle : rotAngle), velocityNew * dt));
	}

private:
	RigidBodyPhysicsCore<Vect6fN2f, MassMatrixFull6fN2f> core_;
	cObject3dx* model_;
	bool turnInPosition_;
	bool waterAnalysis_;
};

#endif // __RIGID_BODY_CAR_H__
