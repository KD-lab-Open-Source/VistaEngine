#include "StdAfx.h"
#include "..\Units\UnitAttribute.h"
#include "RigidBodyPrm.h"
#include "RigidBodyNode.h"

REGISTER_CLASS_IN_FACTORY(RigidBodyFactory, RIGID_BODY_NODE, RigidBodyNode)

///////////////////////////////////////////////////////////////
//
//    class RigidBodyNode
//
///////////////////////////////////////////////////////////////

RigidBodyNode::RigidBodyNode() :
	bodyIndex_(0),
	contacts_(0),
	logicNode_(0),
	graphicNode_(0),
	parent_(-1),
	graphicNodeOffset_(Se3f::ID)
{
	rigidBodyType = RIGID_BODY_NODE;
}

void RigidBodyNode::init(int bodyIndex, vector<Contact>* contacts, const RigidBodyNodePrm& nodePrm)
{
	bodyIndex_ = bodyIndex;
	contacts_ = contacts;
	logicNode_ = nodePrm.logicNode;
	bodyPart_ = nodePrm.bodyPartID;
	graphicNode_ = nodePrm.graphicNode;
	parent_ = nodePrm.parent;
}

void RigidBodyNode::buildBoxFromGeom(vector<Vect3f>& vertex)
{
	//xassert(geom_ == &box() || geom_ == &sphere());
	Vect3f extent_;
	VectN3f::const_iterator iv;
	FOR_EACH(vertex, iv){
		if(extent_.x < iv->x)
			extent_.x = iv->x;
		else
			if(centreOfGravityLocal_.x > iv->x)
				centreOfGravityLocal_.x = iv->x;
		if(extent_.y < iv->y)
			extent_.y = iv->y;
		else
			if(centreOfGravityLocal_.y > iv->y)
				centreOfGravityLocal_.y = iv->y;
		if(extent_.z < iv->z)
			extent_.z = iv->z;
		else
			if(centreOfGravityLocal_.z > iv->z)
				centreOfGravityLocal_.z = iv->z;
	}
	centreOfGravityLocal_ += extent_;
	centreOfGravityLocal_ /= 2.0f;
	extent_ -= centreOfGravityLocal_;
	box().setExtent(extent_);
	sphere().setRadius(extent_.norm());
	radius_ = sqrtf(sqr(extent_.x) + sqr(extent_.y));
	mass_ *= geom()->getVolume(); 
	geom()->getTOI(mass_ * prm().TOI_factor, toi_);
	checkTOI();
}

void RigidBodyNode::addContact(const Vect3f& point, const Vect3f& normal, float penetration)
{
	contacts_->push_back(Contact(point, normal, this, 0, bodyIndex_, -1, penetration));
}

void RigidBodyNode::addDependentNode(int index, Mats nodePose) 
{ 
//	dependentNodes_.push_back(Node(index, nodePose)); 
}

const Se3f& RigidBodyNode::baseNodePose() 
{
//	xassert(dependentNodes_.size() > 0);
//	return dependentNodes_[0].pose_.se();
	return Se3f::ID;
}

void RigidBodyNode::computeCentreOfGravity()
{
	baseNodePose().xformPoint(centreOfGravityLocal_, pose_.trans());
	centreOfGravityLocal_ = Vect3f::ZERO;
	pose_.rot() = baseNodePose().rot();
	//vector<Node>::iterator inode;
	//FOR_EACH(dependentNodes_, inode){
	//	Se3f invertPose;
	//	invertPose.invert(pose());
	//	inode->pose_.se().premult(invertPose);
	//}
}

void RigidBodyNode::updateDependentNodesPose(cObject3dx* model)
{
	//vector<Node>::iterator inode;
	//FOR_EACH(dependentNodes_, inode){
	//	Mats nodePose(inode->pose_);
	//	nodePose.se().premult(pose());
	//	model->SetNodePositionMats(inode->index_, nodePose);
	//}
}

void RigidBodyNode::setGraphicNodeOffset(const vector<RigidBodyNode>& rigidBodies) 
{ 
	xassert(parent_ >= 0);
	graphicNodeOffset_ = pose_;
	graphicNodeOffset_.invert();
	vector<RigidBodyNode>::const_iterator it(rigidBodies.begin() + parent_);
	graphicNodeOffset_.postmult(it->pose());
}

void RigidBodyNode::setGraphicNodeOffset(const Se3f& nodePose) 
{ 
	xassert(parent_== -1);
	graphicNodeOffset_ = pose_;
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
	nodeInterpolator_ = Se3f::ID;
	nodeInterpolator_(model, graphicNode_);
}

void RigidBodyNode::computeGraphicNodePose(Se3f& nodePose) const
{
	xassert(parent_== -1);
	nodePose.mult(pose_, graphicNodeOffset_);
}