#ifndef __CONSTRAINT_HANDLER_H__
#define __CONSTRAINT_HANDLER_H__

#include "..\RigidBodyNode.h"

///////////////////////////////////////////////////////////////
//
//    class JointSpringDumping
//
///////////////////////////////////////////////////////////////

class JointSpringDumping
{
public:
	JointSpringDumping() {}
	void compute(const vector<IterateQuantities>::iterator ch, SparsityMatrix forceAndTOI) {}
	
private:
	RigidBodyPhysics* body0_;
	RigidBodyPhysics* body1_;
	int map0_, map1_;
	float relaxationTime_;
};

///////////////////////////////////////////////////////////////
//
//    class ConstraintHandler
//
///////////////////////////////////////////////////////////////

class ConstraintHandler :public vector<IterateQuantities>
{
public:
	ConstraintHandler(vector<RigidBodyNode>& bodies, vector<Joint>& joints, vector<JointSpringDumping>& springDumpings);
	void reinitialize();
	void update(vector<Contact>& contacts, float dt);
	void computeDV(int iterationLimit);
	const VectN6f& getDV() const { return dv_; }

private:
	VectN6f accelerationExternal_;
	SparsityMatrix forceAndTOI_;
	VectN6f dv_;
	float dt_;
	vector<RigidBodyNode>& bodies_;
	vector<Joint>& joints_;
	vector<JointSpringDumping>& springDumpings_;
};

#endif // __CONSTRAINT_HANDLER_H__
