#include "StdAfxRD.h"
#include "Scene.h"

RandomGenerator graphRnd;

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
	prev=NULL;
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


int cUnknownClass::Release()
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

int cUnknownClass::AddRef()
{
	return _InterlockedIncrement(&m_cRef);
}

int cUnknownClass::DecRef()
{
	return _InterlockedDecrement(&m_cRef);
}

/////////////////////cBaseGraphObject//////////////
cBaseGraphObject::cBaseGraphObject(int kind) : IParent(0),Kind(eKindUnknownClass(kind))
{
}

void cBaseGraphObject::SetAnimKeys(struct sParticleKey *AnimKeys,int size)
{
	VISASSERT(0);
}

void cBaseGraphObject::SetPosition(const MatXf& Matrix)
{
	VISASSERT(0);
}

float cBaseGraphObject::GetBoundRadius() const
{
	VISASSERT(0);
	return 0;
}

void cBaseGraphObject::GetBoundBox(sBox6f& Box) const
{
	VISASSERT(0);
}

/////////////////////cIUnkObj////////////////
cIUnkObj::cIUnkObj(int kind):cBaseGraphObject(kind)
{
	if(MT_IS_LOGIC())
		SetAttr(ATTRUNKOBJ_CREATED_IN_LOGIC);
	Identity(GlobalMatrix);
	IParent=0;
}

cIUnkObj::~cIUnkObj()
{
}

//!!!!!!!cannot AddRef after ATTRUNKOBJ_DELETED
int cBaseGraphObject::Release()
{
	int ref;
	bool is_deleted;

	{
		xassert(IParent==NULL || (GetRef()>=1 && GetRef()<100000));
		if(DecRef()>0) 
		{
			VISASSERT(!GetAttribute(ATTRUNKOBJ_DELETED));
			return GetRef(); 
		}

		is_deleted=GetAttribute(ATTRUNKOBJ_DELETED);

		if(IParent && !GetAttribute(ATTRUNKOBJ_DELETED))
		{

			SetAttribute(ATTRUNKOBJ_DELETED|ATTRUNKOBJ_IGNORE);
			//IParent=NULL;
			IParent->DetachObj(this);
		}
		ref=GetRef();
	}

	if(ref<=0)
	{

		if(IParent && is_deleted)
		{
			VISASSERT(IParent->DebugInUpdateList());
		}

//		if(IParent && is_deleted)
//		{
//			MTG();
//		}

//		xassert(!GetAttribute(ATTRUNKOBJ_TEMP_USED));
		VISASSERT(GetRef()==0);
		delete this;
		return 0;
	}

	return ref;
}

void cBaseGraphObject::Attach()
{
	IParent->AttachObj(this);
}

///////////////////cIUnkObjScale////////////////////
cIUnkObjScale::cIUnkObjScale(int kind)
:cIUnkObj(kind)
{
	scale_=1;
}

cIUnkObjScale::~cIUnkObjScale()
{
}

///////////////////cUnkObj///////////////////////////
cUnkObj::cUnkObj(int kind):cIUnkObjScale(kind)
{ 
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
	VISASSERT(n>=0 && n<NUMBER_OBJTEXTURE);

	RELEASE(Texture[n]);
	Texture[n]=pTexture;
}

void cIUnkObjScale::SetScale(const float scale)
{
	scale_=scale;
}

void cIUnkObj::SetPosition(const MatXf& Matrix)
{
	CheckMatrix(Matrix);
	GlobalMatrix=Matrix;
}

void cBaseGraphObject::SetPosition(const Se3f& pos)
{
#ifdef _DEBUG
	xassert(fabsf(pos.rot().norm2()-1)<1e-2f);
#endif
	MatXf m(pos);
	SetPosition(m);
}
