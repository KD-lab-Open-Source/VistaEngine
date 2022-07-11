#ifndef __VIS_GRID2D_H_INCLUDED__
#define __VIS_GRID2D_H_INCLUDED__

class cBaseGraphObject;

struct sGrid2d
{
protected:
	vector<cBaseGraphObject*> slot;
public:
	typedef vector<cBaseGraphObject*>::iterator iterator;
	
	sGrid2d() { }
	~sGrid2d() { }

	inline int size()	{ return slot.size(); }
	cBaseGraphObject* operator[](int i){return slot[i];}
	iterator begin(){return slot.begin();}
	iterator end(){return slot.end();}

	void Attach(cBaseGraphObject *UnkObj);
	void Detach(cBaseGraphObject *UnkObj);
};

///Нет лишним темплейтам!!!!
class QuatTreeVoid
{
public:
	typedef vector<void*> vect_void;
	typedef void (*find_proc)(void* obj,void* param);
	struct OneObj
	{
		int xmin,ymin,xmax,ymax;
		void* obj;
	};
protected:
	struct QuatNode
	{
		int xmin,ymin,xmax,ymax;
		vect_void obj;
		QuatNode *left,*right;

		QuatNode()
		{
			left=right=NULL;
		}

		~QuatNode()
		{
			delete left;
			delete right;
		}
	};

	QuatNode* root;

	int add_xmin,add_ymin,add_xmax,add_ymax;
	find_proc add_proc;
	void* add_param;
	void add(QuatNode& cur);

	typedef vector<OneObj*> pvect;
	void build(int xmin,int ymin,int xmax,int ymax,pvect& blist,QuatNode* cur);

public:
	QuatTreeVoid();
	~QuatTreeVoid();

	void find(Vect2i pos,int radius,find_proc proc,void* param);
	void find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param);

	void build(vector<OneObj>& tlist);
	void clear();
};

class cObject3dx;
class QuatTree
{
	QuatTreeVoid tree;
public:
	typedef vector<cObject3dx*> vect;
	typedef void (*find_proc)(cObject3dx* obj,void* param);

	QuatTree();
	~QuatTree();

	void find(Vect2i pos,int radius,find_proc proc,void* param);
	void find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param);

	void build(vect& blist);
	void clear();
protected:
	void GetBorder(cObject3dx* p,int& xmin,int& ymin,int& xmax,int& ymax);
};

#endif
