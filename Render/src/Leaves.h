#ifndef __LEAVES_H_INCLUDED__
#define __LEAVES_H_INCLUDED__

#include "Render\Src\UnkObj.h"
#include "Render\3dx\Static3dx.h"

struct Leaf
{
	Leaf()
	{
		normal.set(0,0,1);
	}
	MatXf pos;
	Color4f color;
	int size;
	int inode;
	cTexture* texture;
	Vect3f normal;
};

struct LeafTexture
{
	cTexture* texture;
	vector<Leaf*> leaves;
};


class Leaves : public cUnkObj
{
public:
	Leaves();
	~Leaves();
	
	virtual void Animate(float dt);
	virtual void PreDraw(Camera* camera);
	virtual void Draw(Camera* camera);
	virtual const MatXf& GetPosition()const { return GlobalMatrix; }

	Leaf* AddLeaf();
	int GetLeavesCount();
	Leaf* GetLeaf(int i);
	void SetTexture(cTexture* texture);
	void CalcBound();
	void CalcLeafColor();
	void SetLod(int ilod);
	void CalcLods(StaticVisibilityGroup::VisibleNodes& visible_nodes, int lod);
	void SetDistanceLod(float distanceLod12,float distanceLod23);

protected:
	int lod;
	float distanceLod12_;
	float distanceLod23_;
	float lodChangeDistance_;
	vector<Leaf*> lod_leaves_[StaticVisibilitySet::num_lod];
	vector<Leaf*> leaves_;
	cTexture* texture_;
	struct SortedLeaf
	{
		Leaf *leaf;
		float distance;
	};
	vector<SortedLeaf> sortedLeaves_;
	struct LeafSortByRadius
	{
		inline bool operator()(const SortedLeaf& o1,const SortedLeaf& o2)
		{
			return o1.distance>o2.distance;
		}
	};
	sBox6f bound_;
	float Intersect(Vect3f&pos1,float radius,Vect3f& pos, Vect3f& dir);
};

#endif
