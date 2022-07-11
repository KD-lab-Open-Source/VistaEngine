#ifndef __SPRING_DAMPING_H__
#define __SPRING_DAMPING_H__

#include "Render\3dx\Simply3dx.h"

class SpringDamping3DX 
{
public:
	SpringDamping3DX(cSimply3dx* object);
	
	void evolve(cSimply3dx* object, const Vect2f& velocity, float dt);
	void applyImpulse(const Vect2f& velocity);

	static bool hasWindNodes(cSimply3dx* object);

protected:
	struct Node
	{
		int index;
		Vect2f position;
		Vect2f velocity;

		Node(int i){ index = i; velocity = position = Vect2f::ZERO; }
	};

	typedef vector<Node> Nodes;
	Nodes nodes_;
	float boundK_;

	static bool isWindNode(const char* name);
};
#endif
