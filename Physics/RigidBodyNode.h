#ifndef __RIGID_BODY_NODE_H__
#define __RIGID_BODY_NODE_H__

#include "RigidBodyPhysics.h"
#include "Units\\Interpolation.h"

struct RigidBodyNodePrm;
class cObject3dx;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyNode
//
///////////////////////////////////////////////////////////////

class RigidBodyNode : public RigidBodyPhysics
{
public:
	RigidBodyNode();
	void init(const RigidBodyNodePrm& nodePrm);
	int logicNode() const { return logicNode_; }
	int bodyPart() const { return bodyPart_; }
	void setGraphicNodeOffset(const vector<RigidBodyNode>& rigidBodies);
	void setGraphicNodeOffset(const Se3f& nodePose);
	void updateGraphicNodeOfset(const vector<RigidBodyNode>& rigidBodies, cObject3dx* model);
	void resetGraphicNodeOfset(cObject3dx* model);
	Se3f computeGraphicNodePose() const;

private:
	int logicNode_;
	int bodyPart_;
	int graphicNode_;
	int parent_;
	Se3f graphicNodeOffset_;
	InterpolatorNodeTransform nodeInterpolator_;
};

#endif // __RIGID_BODY_NODE_H__