#include "StdAfx.h"
#include "SpringDamping.h"

bool SpringDamping3DX::isWindNode(const char* name)
{
	return !memcmp(name, "wind", 4);
}

bool SpringDamping3DX::hasWindNodes(cSimply3dx* object )
{
	xassert(object);
	vector<string>::iterator it;
	FOR_EACH(object->GetStatic()->node_name, it)
		if(isWindNode(it->c_str()))
			return true;
	return false;
}

SpringDamping3DX::SpringDamping3DX(cSimply3dx* object)
{
	static float inv_default_size = 0.04f;
	sBox6f bound;
	object->GetBoundBox(bound);
	boundK_ = (bound.max.z-bound.min.z)*inv_default_size;

	MatXf parent_pos = object->GetPosition();
	parent_pos.rot() *= object->GetScale();

	int i = 0;
	vector<string>::iterator it;
	FOR_EACH(object->GetStatic()->node_name, it){
		if(isWindNode(it->c_str()))
			nodes_.push_back(Node(i));
		i++;
	}
}

void SpringDamping3DX::evolve(cSimply3dx* object, const Vect2f& velocity, float dt)
{	
	static float wind_force_K = 20.0;
	static float k = 10.0f;
	static float kr = 2.0f;
	
	Vect2f wind_force_dt = velocity*wind_force_K*dt;
	float k_dt = k*dt;
	float min_kr_dt = 1 - min(kr*dt,0.9f);
	
	MatXf parent_pos = object->GetPosition();
	parent_pos.rot() *= object->GetScale();

	Nodes::iterator iNode;
	FOR_EACH(nodes_, iNode){
		Node& node = *iNode;
		node.velocity += wind_force_dt; //wind force
		node.velocity -= node.position*k_dt; //elasticity force
		node.velocity *= min_kr_dt; //friction force
		node.position += node.velocity*dt;	

		MatXf out_pos = parent_pos*object->GetNodeInitialOffset(node.index);
		out_pos.trans().x += node.position.x*boundK_;
		out_pos.trans().y += node.position.y*boundK_;
		object->SetNodePosition(node.index,out_pos);
	}
}

void SpringDamping3DX::applyImpulse(const Vect2f& velocity )
{
	vector<Node>::iterator it;
	FOR_EACH(nodes_, it)
		it->velocity += velocity;
}
