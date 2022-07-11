#ifndef __CONSTRAINT_HANDLER_SIMPLE_H__
#define __CONSTRAINT_HANDLER_SIMPLE_H__

#include "PhysicsMath.h"

class RigidBodyBox;
class RigidBodyPhysics;

///////////////////////////////////////////////////////////////
//
//    class IterateQuantities
//
///////////////////////////////////////////////////////////////

struct IterateQuantities
{
public:
	SparsityRowSimple j;
	Vect6f b0;
	Vect6f b1;
	float lambda;
	float lambda0;
	float lambda_min;
	float lambda_max;
	float restitution;
	float friction;
	float eta;
	float d;

	xm_inline void limits()
	{
		lambda = max(lambda_min, min(lambda, lambda_max));
	}
};

///////////////////////////////////////////////////////////////
//
//    class ContactSimple
//
///////////////////////////////////////////////////////////////

class ContactSimple
{
public:
	ContactSimple();
	ContactSimple(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, float _penetration);
	virtual void compute(vector<IterateQuantities>& ch, bool angularEvolve) const;
	const Vect3f& point() const { return point_; }

protected:
	Vect3f point_;
	Mat3f axes_;
	RigidBodyPhysics* body0_;
	float penetration_;
};


///////////////////////////////////////////////////////////////
//
//    class Contact
//
///////////////////////////////////////////////////////////////

class Contact : public ContactSimple
{
public:
	Contact();
	Contact(const Vect3f& point, const Vect3f& normal, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
		int map0, int map1, float _penetration);
	virtual void compute(vector<IterateQuantities>& ch) const;
	
private:
	RigidBodyPhysics* body1_;
	int map0_, map1_;
	float friction_;
	float relaxationTime_;
};

///////////////////////////////////////////////////////////////
//
//    class Joint
//
///////////////////////////////////////////////////////////////

class Joint
{
public:
	Joint();
	Joint(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
		const int map0, const int map1, const float relaxationTime,
		const Vect3f& upperLimits, const Vect3f& lowerLimits);
	void set(const Vect3f& point, const Mat3f& axes, RigidBodyPhysics* body0, RigidBodyPhysics* body1,
		const int map0, const int map1, const float relaxationTime,
		const Vect3f& upperLimits, const Vect3f& lowerLimits);
	void setPose(const Vect3f& point, const Mat3f& axes);
	void compute(const vector<IterateQuantities>::iterator ch);
	Vect3f point() const;

private:
	Vect3f point_;
	Mat3f axes0_;
	Mat3f axes1_;
	RigidBodyPhysics* body0_;
	RigidBodyPhysics* body1_;
	int map0_, map1_;
	float relaxationTime_;
	Vect3f localPoint0_, localPoint1_;
	Vect3f penetration_;
	Vect3f upperLimits_;
	Vect3f lowerLimits_;
};

///////////////////////////////////////////////////////////////
//
//    class ConstraintHandlerSimple
//
///////////////////////////////////////////////////////////////

class ConstraintHandlerSimple :public vector<IterateQuantities>
{
public:
	ConstraintHandlerSimple(RigidBodyBox* body);
	void setBody(RigidBodyBox* body) { body_ = body; }
	void set(Joint* joint);
	void update(vector<ContactSimple>& contacts, float dt, bool angularEvolve);
	void computeDV(int iterationLimit);
	const Vect6f& getDV() const { return dv_; }

private:
	Vect6f accelerationExternal_;
	Vect6f dv_;
	MassMatrix inverseTOIWorld_;
	float dt_;
	RigidBodyBox* body_;
	Joint* joint_;
};

#endif // __CONSTRAINT_HANDLER_SIMPLE_H__
