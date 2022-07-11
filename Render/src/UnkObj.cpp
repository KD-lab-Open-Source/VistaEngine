#include "StdAfxRD.h"
#include "Scene.h"

RENDER_API RandomGenerator graphRnd;

#ifdef C_CHECK_DELETE
cCheckExit cCheckDelete::root;
static MTSection gb_checkexit_lock;//Кривой синглетон.

void SaveKindObjNotFree();
cCheckExit::~cCheckExit()
{
	SaveKindObjNotFree();
}

cCheckDelete::cCheckDelete()
{
	MTAuto lock(gb_checkexit_lock);
	if(root.root)
		root.root->prev=this;
	prev=0;
	next=root.root;
	root.root=this;
}

cCheckDelete::~cCheckDelete()
{
	MTAuto lock(gb_checkexit_lock);
	if(next)
		next->prev=prev;

	if(prev)
		prev->next=next;
	else
		root.root=next;
}
#endif C_CHECK_DELETE


int UnknownClass::Release()
{ 
	xassert(m_cRef>0);
	if(DecRef()>0) 
		return m_cRef;
	delete this;
	return 0;
}

extern "C" {
LONG _InterlockedIncrement(LONG volatile* lpAddend);
LONG _InterlockedDecrement(LONG volatile* lpAddend);
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
};

int UnknownClass::AddRef()
{
	return _InterlockedIncrement(&m_cRef);
}

int UnknownClass::DecRef()
{
	return _InterlockedDecrement(&m_cRef);
}

/////////////////////cBaseGraphObject//////////////
BaseGraphObject::BaseGraphObject(int kind) : scene_(0),Kind(eKindUnknownClass(kind))
{
	scene_ = 0;
}

void BaseGraphObject::SetPosition(const MatXf& Matrix)
{
	xassert(0);
}

const MatXf& BaseGraphObject::GetPosition() const
{
	return MatXf::ID;
}

float BaseGraphObject::GetBoundRadius() const
{
	xassert(0);
	return 0;
}

void BaseGraphObject::GetBoundBox(sBox6f& Box) const
{
	xassert(0);
}

/////////////////////cIUnkObj////////////////
cIUnkObj::cIUnkObj(int kind):BaseGraphObject(kind)
{
	if(MT_IS_LOGIC())
		setAttribute(ATTRUNKOBJ_CREATED_IN_LOGIC);
	GlobalMatrix = MatXf::ID;
}

cIUnkObj::~cIUnkObj()
{
}

//!!!!!!!cannot AddRef after ATTRUNKOBJ_DELETED
int BaseGraphObject::Release()
{
	int ref;
	bool is_deleted;

	{
		xassert(scene_==0 || (GetRef()>=1 && GetRef()<100000));
		if(DecRef()>0) 
		{
			xassert(!getAttribute(ATTRUNKOBJ_DELETED));
			return GetRef(); 
		}

		is_deleted=getAttribute(ATTRUNKOBJ_DELETED);

		if(scene_ && !getAttribute(ATTRUNKOBJ_DELETED))
		{

			setAttribute(ATTRUNKOBJ_DELETED|ATTRUNKOBJ_IGNORE);
			//scene()=0;
			scene_->DetachObj(this);
		}
		ref=GetRef();
	}

	if(ref<=0)
	{

		if(scene_ && is_deleted)
		{
			xassert(scene_->DebugInUpdateList());
		}

//		if(scene() && is_deleted)
//		{
//			MTG();
//		}

//		xassert(!getAttribute(ATTRUNKOBJ_TEMP_USED));
		xassert(GetRef()==0);
		delete this;
		return 0;
	}

	return ref;
}

void BaseGraphObject::Attach()
{
	scene_->AttachObj(this);
}

///////////////////cUnkObj///////////////////////////
cUnkObj::cUnkObj(int kind)
: cIUnkObj(kind)
{ 
	scale_=1;
	diffuse.set(1,1,1,1);
	for( int i=0; i<NUMBER_OBJTEXTURE; i++ )
		Texture[i]=0;
}
cUnkObj::~cUnkObj()
{
	for( int i=0; i<NUMBER_OBJTEXTURE; i++ )
		if( Texture[i] )
		{
			Texture[i]->Release();
			Texture[i]=0;
		}
}

void cUnkObj::SetTexture(int n,cTexture *pTexture)
{
	xassert(n>=0 && n<NUMBER_OBJTEXTURE);

	RELEASE(Texture[n]);
	Texture[n]=pTexture;
}

void cUnkObj::SetScale(const float scale)
{
	scale_=scale;
}

void cIUnkObj::SetPosition(const MatXf& Matrix)
{
	MTAccess();
	GlobalMatrix = Matrix;
}

void BaseGraphObject::SetPosition(const Se3f& pos)
{
#ifdef _DEBUG
	xassert(fabsf(pos.rot().norm2()-1)<1e-2f);
#endif
	MatXf m(pos);
	SetPosition(m);
}


