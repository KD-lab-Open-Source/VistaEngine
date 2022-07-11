#include "StdAfx.h"
#include "SpringDamping.h"

//class cPerlinNoize

int SpringDamping3DX::getWindNodesCount( cSimply3dx* object )
{
	xassert(object);
	vector<string>::iterator it;
	int i=0;
	FOR_EACH(object->GetStatic()->node_name, it){
		if (it->size() >= 5 && memcmp(it->c_str(),"wind",4) == 0)
			i++;
	}

	return i;
}

SpringDamping3DX::SpringDamping3DX(cSimply3dx* object)
{
	xassert(object);
	vector<string>::iterator it;
	int i=0;
	FOR_EACH(object->GetStatic()->node_name, it)
	{
		string &str = *it;
		if (str.size()>=5&& memcmp(str.c_str(),"wind",4)==0)
		{
			nodes_.push_back(NodeObj(i));
		}
		i++;
	}
}

SpringDamping3DX::SpringDamping3DX(cObject3dx* object)
{
	xassert(object);
	vector<cStaticNode>::iterator it;
	int i=0;
	FOR_EACH(object->GetStatic()->nodes, it)
	{
		if (it->name.size()>=5&&memcmp(it->name.c_str(),"wind",4)==0)
		{
			nodes_.push_back(NodeObj(i));
		}
		i++;
	}
}	
	
SpringDamping3DX::~SpringDamping3DX()
{	
}	

void SpringDamping3DX::evolve(c3dx* object, const Vect2f& velocity, float dt)
{	
	xassert(object);
	static float wind_force_K = 20.0;
	static float k = 10.0f;
	static float kr = 2.0f;
	static float inv_default_size = 0.04f;
	static float k_phase = 0.4f;
	
	
	Vect2f wind_force_dt = velocity * wind_force_K * dt;
	float k_dt = k*dt;
	float min_kr_dt = 1 - min(kr*dt,0.9f);
	
	MatXf parent_pos = object->GetPosition();
	parent_pos.rot()*=object->GetScale();
	sBox6f Bound;
	object->GetBoundBox(Bound);

	float BoundK = (Bound.max.z-Bound.min.z)*inv_default_size;

	vector<NodeObj>::iterator i;
	FOR_EACH(nodes_, i)
	{
		i->velocity += wind_force_dt; //wind force
		i->velocity -= i->position * k_dt; //elasticity force
		i->velocity *= min_kr_dt; //friction force
		i->position += i->velocity * dt;	

//		Se3f u;			
//		//float rot = i->dis.norm()/object->GetNodePositionMat(i->ix).trans().norm();//Норма с делением, хреново.
//		float rot=velocity.norm()*0.3f;
//		u.rot().set((rot-(maxr/2))*2,velocity%Vect3f::K);
								
//		if(object->GetKind() == KIND_OBJ_3DX)
//		{						
//			cObject3dx* obj = (cObject3dx*)object;
//			obj->SetUserTransform(i->ix,u);
//		}else					
//		{			
			cSimply3dx* obj = (cSimply3dx*)object;
		
			MatXf out_pos;		
			out_pos=parent_pos*obj->GetNodeInitialOffset(i->ix);
/*
			out_pos.trans()-=parent_pos.trans();
			out_pos.rot().mult(u.rot(),out_pos.rot());
			out_pos.trans()+=parent_pos.trans();
/*/
			out_pos.trans().x+=i->position.x*BoundK;
			out_pos.trans().y+=i->position.y*BoundK;
/**/
			obj->SetNodePosition(i->ix,out_pos);
//		}
	}
}

void SpringDamping3DX::applyImpulse( const Vect3f& velocity )
{
	vector<NodeObj>::iterator it;
	FOR_EACH(nodes_, it)
		it->velocity += velocity;
}
