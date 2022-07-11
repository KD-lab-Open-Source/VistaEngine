#ifndef __UNKNOWN_H_INCLUDED__
#define __UNKNOWN_H_INCLUDED__

#ifndef _FINAL_VERSION_
#define C_CHECK_DELETE
#endif 

#ifdef C_CHECK_DELETE
class cCheckExit
{
public:
	class cCheckDelete* root;
	cCheckExit():root(0) {}
	~cCheckExit();
};

#include "..\stack.h"
class cCheckDelete
{
	static cCheckExit root;
	cCheckDelete *next,*prev;
public:
	CreateStack stack;

	cCheckDelete();
	virtual ~cCheckDelete();

	static cCheckDelete* GetDebugRoot(){return root.root;}
	inline cCheckDelete* GetDebugNext(){return next;}
};

#endif C_CHECK_DELETE

// базовый класс для всех
// любой класс наследованный как TYPE_CLASS_POINTER, должен уметь удаляться по обращению к Release()
class cUnknownClass
#ifdef C_CHECK_DELETE
: public cCheckDelete
#endif C_CHECK_DELETE
{	
	LONG	m_cRef;
public:
	cUnknownClass()
	{ 
		m_cRef=1; 
	}
	virtual ~cUnknownClass()								
	{ 
	}
	virtual int Release();
	inline int GetRef()	const	{ return m_cRef; }
	int AddRef();
	int DecRef();
};

#define RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

template <class xvector>
void remove_null_element(xvector& ar)
{
	int cur=0;
	for(int i=0;i<ar.size();i++)
	if(ar[i])
	{
		ar[cur]=ar[i];
		cur++;
	}

	ar.resize(cur);
}

#endif
