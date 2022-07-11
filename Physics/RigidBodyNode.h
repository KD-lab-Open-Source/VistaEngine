#ifndef __RIGID_BODY_NODE_H__
#define __RIGID_BODY_NODE_H__

#include "RigidBodyPhysics.h"
#include "..\Units\\Interpolation.h"

struct RigidBodyNodePrm;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyNode
//
///////////////////////////////////////////////////////////////

class RigidBodyNode : public RigidBodyPhysics
{
public:
	RigidBodyNode();
	void init(int bodyIndex, vector<Contact>* contacts, const RigidBodyNodePrm& nodePrm);
	void addContact(const Vect3f& point, const Vect3f& normal, float penetration);
	void buildBoxFromGeom(vector<Vect3f>& vertex);
	void addDependentNode(int index, Mats nodePose);
	const Se3f& baseNodePose();
	void computeCentreOfGravity();
	void updateDependentNodesPose(cObject3dx* model);
	int logicNode() const { return logicNode_; }
	int bodyPart() const { return bodyPart_; }
	void setGraphicNodeOffset(const vector<RigidBodyNode>& rigidBodies);
	void setGraphicNodeOffset(const Se3f& nodePose);
	void updateGraphicNodeOfset(const vector<RigidBodyNode>& rigidBodies, cObject3dx* model);
	void resetGraphicNodeOfset(cObject3dx* model);
	void computeGraphicNodePose(Se3f& nodePose) const;

private:
	int logicNode_;
	int bodyIndex_;
	int bodyPart_;
	vector<Contact>* contacts_;
	int graphicNode_;
	int parent_;
	Se3f graphicNodeOffset_;
	InterpolatorNodeTransform nodeInterpolator_;
};

#endif // __RIGID_BODY_NODE_H__