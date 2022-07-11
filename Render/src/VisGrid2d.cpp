#include "StdAfxRD.h"
#include "VisGrid2d.h"
#include "..\3dx\Node3dx.h"

/*
	В случае одного потока - объекты должны удаляться в конце прорисовки сцены.
	В случае многих потоков - объекты должны удаляться в конце прорисовки сцены на следующий логический квант.
		добавляться на следующий графический квант, но только если еще не удалены.		
*/

inline bool IsNowDelete(cBaseGraphObject *UnkObj)
{
//	if(MT_IS_GRAPH() && UnkObj->GetAttr(ATTRUNKOBJ_CREATED_IN_LOGIC))
//	{
//		int k=0;
//	}

	if(MT_IS_GRAPH() && MT_IS_LOGIC())
		return true;//В случае одного потока - удалять спокойно

	return MT_IS_GRAPH() && !UnkObj->GetAttr(ATTRUNKOBJ_CREATED_IN_LOGIC);
}

///////////////////////////////////////////////////////////////////////////////
QuatTreeVoid::QuatTreeVoid()
{
	root=NULL;
}

QuatTreeVoid::~QuatTreeVoid()
{
	clear();	
}

void QuatTreeVoid::clear()
{
	delete root;
	root=NULL;
}

void QuatTreeVoid::find(Vect2i pos,int radius,find_proc proc,void* param)
{
	find(pos.x-radius,pos.y-radius,pos.x+radius,pos.y+radius,proc,param);
}

void QuatTreeVoid::find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param)
{
	add_proc=proc;
	add_param=param;

	add_xmin=xmin;
	add_ymin=ymin;
	add_xmax=xmax;
	add_ymax=ymax;
	if(root)
		add(*root);
}

void QuatTreeVoid::add(QuatNode& cur)
{
	if((cur.xmin<add_xmax && cur.xmax>add_xmin)&&
	   (cur.ymin<add_ymax && cur.ymax>add_ymin))
	{
		int sz=cur.obj.size();
		for(int i=0;i<sz;i++)
			add_proc(cur.obj[i],add_param);

		if(cur.left)
			add(*cur.left);
		if(cur.right)
			add(*cur.right);
	}
}

void QuatTreeVoid::build(vector<OneObj>& tlist)
{
	clear();
	int xmin=INT_MAX,ymin=INT_MAX,xmax=INT_MIN,ymax=INT_MIN;
	int sz=tlist.size();
	vector<OneObj*> plist(sz);
	for(int i=0;i<sz;i++)
	{
		OneObj& o=tlist[i];
		plist[i]=&o;
		xmin=min(xmin,o.xmin);
		ymin=min(ymin,o.ymin);
		xmax=max(xmax,o.xmax);
		ymax=max(ymax,o.ymax);
	}

	root=new QuatNode;
	build(xmin,ymin,xmax,ymax,plist,root);
}

void QuatTreeVoid::build(int xmin,int ymin,int xmax,int ymax,pvect& blist,QuatNode* node)
{
	node->xmin=xmin;
	node->xmax=xmax;
	node->ymin=ymin;
	node->ymax=ymax;

	int divigex=(xmin+xmax)>>1;
	int divigey=(ymin+ymax)>>1;
	if(divigex==xmin || divigey==ymin || blist.size()<4)
	{
		int sz=blist.size();
		node->obj.resize(sz);
		for(int i=0;i<sz;i++)
			node->obj[i]=blist[i]->obj;
		return;
	}
	int intersectx=(xmax-xmin)/6;
	int intersecty=(ymax-ymin)/6;
	int divigex_min=divigex+intersectx;
	int divigex_max=divigex-intersectx;
	int divigey_min=divigey+intersecty;
	int divigey_max=divigey-intersecty;

	int divigex_min_real=divigex;
	int divigex_max_real=divigex;
	int divigey_min_real=divigey;
	int divigey_max_real=divigey;

	pvect horz_cur,horz_left,horz_right,
		  vert_cur,vert_top,vert_bottom;

	pvect::iterator it;
	//x
	FOR_EACH(blist,it)
	{
		OneObj* o=*it;
		bool bmax=true;
		if(o->xmin>=divigex_max && o->xmax<=divigex_min)
		{
			bmax=(o->xmin-divigex_max)>(divigex_min-o->xmax);
		}

		if(o->xmin>=divigex_max && bmax)
		{
			divigex_max_real=min(divigex_max_real,o->xmin);
			horz_right.push_back(o);
		}else
		if(o->xmax<=divigex_min)
		{
			divigex_min_real=max(divigex_min_real,o->xmax);
			horz_left.push_back(o);
		}else
			horz_cur.push_back(o);
	}

	//y
	FOR_EACH(blist,it)
	{
		OneObj* o=*it;
		bool bmax=true;
		if(o->ymin>=divigey_max && o->ymax<=divigey_min)
		{
			bmax=(o->ymin-divigey_max)>(divigey_min-o->ymax);
		}

		if(o->ymin>=divigey_max && bmax)
		{
			divigey_max_real=min(divigey_max_real,o->ymin);
			vert_bottom.push_back(o);
		}else
		if(o->ymax<=divigey_min)
		{
			divigey_min_real=max(divigey_min_real,o->ymax);
			vert_top.push_back(o);
		}else
			vert_cur.push_back(o);
	}

	int xt=horz_cur.size()+max(horz_left.size(),horz_right.size());
	int yt=vert_cur.size()+max(vert_top.size(),vert_bottom.size());

	if(xt<yt)
	{
		//
		int sz=horz_cur.size();
		node->obj.resize(sz);
		for(int i=0;i<sz;i++)
			node->obj[i]=horz_cur[i]->obj;

		{
			pvect().swap(vert_cur);
			pvect().swap(vert_top);
			pvect().swap(vert_bottom);
			pvect().swap(horz_cur);
		}

		if(!horz_left.empty())
		{
			node->left=new QuatNode;
			build(xmin,ymin,divigex_min_real,ymax,horz_left,node->left);
		}

		if(!horz_right.empty())
		{
			node->right=new QuatNode;
			build(divigex_max_real,ymin,xmax,ymax,horz_right,node->right);
		}
	}else
	{
		int sz=vert_cur.size();
		node->obj.resize(sz);
		for(int i=0;i<sz;i++)
			node->obj[i]=vert_cur[i]->obj;

		{
			pvect().swap(horz_cur);
			pvect().swap(horz_left);
			pvect().swap(horz_right);
			pvect().swap(vert_cur);
		}

		if(!vert_top.empty())
		{
			node->left=new QuatNode;
			build(xmin,ymin,xmax,divigey_min_real,vert_top,node->left);
		}

		if(!vert_bottom.empty())
		{
			node->right=new QuatNode;
			build(xmin,divigey_max_real,xmax,ymax,vert_bottom,node->right);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
QuatTree::QuatTree()
{
}

QuatTree::~QuatTree()
{
}

void QuatTree::clear()
{
	tree.clear();
}

void QuatTree::find(Vect2i pos,int radius,find_proc proc,void* param)
{
	find(pos.x-radius,pos.y-radius,pos.x+radius,pos.y+radius,proc,param);
}

void QuatTree::find(int xmin,int ymin,int xmax,int ymax,find_proc proc,void* param)
{
	tree.find(xmin,ymin,xmax,ymax,(QuatTreeVoid::find_proc)proc,param);
}

void QuatTree::GetBorder(cObject3dx* obj,int& xmin,int& ymin,int& xmax,int& ymax)
{
	sBox6f b;
	Vect2f mi,ma;
	obj->GetBoundBox(b);
	Vect3f	p[8];
	MatXf matrix=obj->GetPosition();
	matrix.xformPoint(Vect3f(b.min.x,b.min.y,b.min.z),p[0]);
	matrix.xformPoint(Vect3f(b.max.x,b.min.y,b.min.z),p[1]);
	matrix.xformPoint(Vect3f(b.min.x,b.max.y,b.min.z),p[2]);
	matrix.xformPoint(Vect3f(b.max.x,b.max.y,b.min.z),p[3]);
	matrix.xformPoint(Vect3f(b.min.x,b.min.y,b.max.z),p[4]);
	matrix.xformPoint(Vect3f(b.max.x,b.min.y,b.max.z),p[5]);
	matrix.xformPoint(Vect3f(b.min.x,b.max.y,b.max.z),p[6]);
	matrix.xformPoint(Vect3f(b.max.x,b.max.y,b.max.z),p[7]);

	mi.set(p[0].x,p[0].y);
	ma=mi;
	for(int k=0;k<8;k++)
	{
		mi.x=min(mi.x,p[k].x);
		mi.y=min(mi.y,p[k].y);
		ma.x=max(ma.x,p[k].x);
		ma.y=max(ma.y,p[k].y);
	}

	xmin=round(mi.x);
	xmax=round(ma.x);
	ymin=round(mi.y);
	ymax=round(ma.y);
}

void QuatTree::build(vect& blist)
{
	clear();
	int sz=blist.size();
	if(!sz)
		return;
	vector<QuatTreeVoid::OneObj> tlist(sz);
	for(int i=0;i<sz;i++)
	{
		QuatTreeVoid::OneObj& o=tlist[i];
		GetBorder(blist[i],o.xmin,o.ymin,o.xmax,o.ymax);
		o.obj=blist[i];
	}

	tree.build(tlist);
}

/////////////////sGrid2d/////////////////
void sGrid2d::Attach(cBaseGraphObject *UnkObj)
{
#ifdef _DEBUG
	vector<cBaseGraphObject*>::iterator it;
	it=find(slot.begin(),slot.end(),UnkObj);
	if(it!=slot.end())
	{
		xassert(0);
	}
#endif
	slot.push_back(UnkObj);
}

void sGrid2d::Detach(cBaseGraphObject *UnkObj)
{
	MTG();
	vector<cBaseGraphObject*>::iterator it;
	it=find(slot.begin(),slot.end(),UnkObj);
	if(it!=slot.end())
	{
		slot.erase(it);

		if(false)//буде дед лок с MTAuto lock(gb_VisGeneric->GetReleaseLock()); MTAuto mtauto(&critial_attach);
		{
			UnkObj->Release();
		}else
		{
			xassert(UnkObj->GetRef()==1);
			xassert(UnkObj->GetAttr(ATTRUNKOBJ_DELETED));
			UnkObj->DecRef();
			delete UnkObj;
		}
	}else
		xassert(0);
}
