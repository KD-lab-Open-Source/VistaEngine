#ifndef __CONSTRAINT_HANDLER_H__
#define __CONSTRAINT_HANDLER_H__

#include "ConstraintHandlerSimple.h"

///////////////////////////////////////////////////////////////
//
//    class Contact
//
///////////////////////////////////////////////////////////////

class Contact : public ContactSimple
{
public:
	Contact(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, RigidBodyPhysics* body1, float penetration);
	void compute(ConstraintHandler& ch) const;
	void* operator new (size_t)
	{
		return PhysicsPool<Contact>::get();
	}

private:
	RigidBodyPhysics* body1_;
};

///////////////////////////////////////////////////////////////
//
//    class Joint
//
///////////////////////////////////////////////////////////////

class Joint : public JointSimple
{
public:
	void set(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
		float relaxationTime,	const Vect3f& upperLimits, const Vect3f& lowerLimits);
	void setPose(const Vect3f& point, const Mat3f& axes);
	void compute(ConstraintHandler& ch) const;

private:
	Mat3f axes1_;
	RigidBodyPhysics* body1_;
	Vect3f localPoint1_;
};

#endif // __CONSTRAINT_HANDLER_H__
