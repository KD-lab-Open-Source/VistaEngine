#ifndef __RIGID_BODY_NODE_PRM_H__
#define __RIGID_BODY_NODE_PRM_H__

#include "..\Units\Object3dxInterface.h"

struct RigidBodyNodePrm
{
public:
	Logic3dxNodeBound logicNode;
	Object3dxNode graphicNode;
	int parent;
	int bodyPartID;
	Vect3f upperLimits;
	Vect3f lowerLimits;
	float mass;

	RigidBodyNodePrm();
	void serialize(Archive& ar);
};

typedef vector<RigidBodyNodePrm> RigidBodyModelPrm;

#endif // __RIGID_BODY_NODE_PRM_H__