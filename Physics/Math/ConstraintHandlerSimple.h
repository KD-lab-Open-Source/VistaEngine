#ifndef __CONSTRAINT_HANDLER_SIMPLE_H__
#define __CONSTRAINT_HANDLER_SIMPLE_H__

#include "PhysicsMath.h"

class ConstraintHandler;

///////////////////////////////////////////////////////////////
//
//    class ConstraintBase
//
///////////////////////////////////////////////////////////////

class ConstraintBase
{
public:
	virtual void compute(ConstraintHandler& ch) const = 0;
};

///////////////////////////////////////////////////////////////
//
//    class ContactSimple
//
///////////////////////////////////////////////////////////////

class ContactSimple : public ConstraintBase
{
public:
	ContactSimple(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, float penetration);
	Vect6f computeJ(const Vect3f& axis, const Vect3f& local_point) const;
	void compute(ConstraintHandler& ch) const;
	void* operator new(size_t)
	{
		return PhysicsPool<ContactSimple>::get();
	}
	void operator delete(void*)
	{
		xxassert(false, "попытка удаления объекта из PhysicsPool");
	}

protected:
	Vect3f point_;
	Mat3f axes_;
	RigidBodyPhysics* body0_;
	float penetration_;
};

///////////////////////////////////////////////////////////////
//
//    class JointBase
//
///////////////////////////////////////////////////////////////

class JointBase : public ConstraintBase
{
public:
	JointBase() : index_(-1) {}
	void setIndex(int index) { index_ = index; }
	int index() { return index_; }
	bool attached() const { return index_ >= 0; }

private:
	int index_;
};

///////////////////////////////////////////////////////////////
//
//    class JointSimple
//
///////////////////////////////////////////////////////////////

class JointSimple : public JointBase
{
public:
	JointSimple() {}
	JointSimple(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body,
		float relaxationTime,	const Vect3f& upperLimits, const Vect3f& lowerLimits);
	void setPose(const Vect3f& point, const Mat3f& axes);
	static void computeAxes(Mat3f& axesTemp, const QuatF& orientation, const Mat3f& axes);
	static float computePhi(const Vect3f& axis0, const Vect3f& axis1)
	{
		return asinf(clamp(0.5f * dot(axis0, axis1), -1.0f, 1.0f));
	}
	static Vect6f computeJ(const Vect3f& axis, const Vect3f& local_point)
	{
		return Vect6f(axis, Vect3f().cross(local_point, axis));
	}
	void compute(ConstraintHandler& ch) const;
	
protected:
	Vect3f point_;
	Mat3f axes0_;
	RigidBodyPhysics* body0_;
	float relaxationTime_;
	Vect3f localPoint0_;
	Vect3f upperLimits_;
	Vect3f lowerLimits_;
};

///////////////////////////////////////////////////////////////
//
//    class ConstraintHandler
//
///////////////////////////////////////////////////////////////

class ConstraintHandler : public vector<IterateQuantitiesBase*>
{
public:
	void attach(RigidBodyPhysicsBase* body);
	void detach(RigidBodyPhysicsBase* body);
	void attach(JointBase* joint);
	void detach(JointBase* joint);
	void addContact(ConstraintBase* contact) { contacts_.push_back(contact); }
	void quant(int iterationLimit, float dt, int times);
	
private:
    vector<RigidBodyPhysicsBase*> bodies_;
	vector<ConstraintBase*> contacts_;
	vector<JointBase*> joints_;
};

#endif // __CONSTRAINT_HANDLER_SIMPLE_H__