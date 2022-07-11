#ifndef __SPRING_DAMPING_H__
#define __SPRING_DAMPING_H__
struct NodeObj
{
	int ix;
	Vect2f position;
	Vect2f velocity;

	NodeObj(int i){ix= i; velocity = position = Vect2f::ZERO;};
};

class SpringDamping3DX 
{
public:
	SpringDamping3DX(cSimply3dx* object);
	SpringDamping3DX(cObject3dx* object);
	~SpringDamping3DX();
	
	void evolve(c3dx* object, const Vect2f& velocity, float dt);
	void applyImpulse(const Vect3f& velocity);

	static int getWindNodesCount(cSimply3dx* object);

protected:
	vector<NodeObj> nodes_;
};
#endif
