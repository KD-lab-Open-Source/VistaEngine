#include "StdAfx.h"
#include "RigidBodyNode.h"
#include "RigidBodyNodePrm.h"
#include "NormalMap.h"
#include "Render\3dx\Node3dx.h"

///////////////////////////////////////////////////////////////
//
//    class RigidBodyNode
//
///////////////////////////////////////////////////////////////

RigidBodyNode::RigidBodyNode() :
	logicNode_(0),
	graphicNode_(0),
	parent_(-1),
	graphicNodeOffset_(Se3f::ID)
{
}

void RigidBodyNode::init(const RigidBodyNodePrm& nodePrm)
{
	logicNode_ = nodePrm.logicNode;
	bodyPart_ = nodePrm.bodyPartID;
	graphicNode_ = nodePrm.graphicNode;
	parent_ = nodePrm.parent;
}

void RigidBodyNode::setGraphicNodeOffset(const vector<RigidBodyNode>& rigidBodies) 
{ 
	xassert(parent_ >= 0);
	graphicNodeOffset_ = pose();
	graphicNodeOffset_.invert();
	vector<RigidBodyNode>::const_iterator it(rigidBodies.begin() + parent_);
	graphicNodeOffset_.postmult(it->pose());
}

void RigidBodyNode::setGraphicNodeOffset(const Se3f& nodePose) 
{ 
	xassert(parent_== -1);
	graphicNodeOffset_ = pose();
	graphicNodeOffset_.invert();
	graphicNodeOffset_.postmult(nodePose);
}

void RigidBodyNode::updateGraphicNodeOfset(const vector<RigidBodyNode>& rigidBodies, cObject3dx* model)
{
	xassert(parent_ >= 0);
	vector<RigidBodyNode>::const_iterator it(rigidBodies.begin() + parent_);
	Se3f nodePose(it->pose());
	nodePose.invert();
	nodePose.postmult(pose());
	nodePose.premult(graphicNodeOffset_);
	nodeInterpolator_ = nodePose;
	nodeInterpolator_(model, graphicNode_);
}

void RigidBodyNode::resetGraphicNodeOfset(cObject3dx* model)
{
	streamLogicCommand.set(fNodeTransformCopy, model) << graphicNode_;
}

Se3f RigidBodyNode::computeGraphicNodePose() const
{
	xassert(parent_== -1);
	return Se3f(pose()).postmult(graphicNodeOffset_);
}