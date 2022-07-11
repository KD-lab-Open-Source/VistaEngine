#ifndef __UNKNOWN_H_INCLUDED__
#define __UNKNOWN_H_INCLUDED__

#ifndef _FINAL_VERSION_
//#define C_CHECK_DELETE
#endif 

#include "rd.h"

#ifdef C_CHECK_DELETE
class RENDER_API cCheckExit
{
public:
	class cCheckDelete* root;
	cCheckExit():root(0) {}
	~cCheckExit();
};

#include "Render\Util\stack.h"
class RENDER_API cCheckDelete
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
class RENDER_API  UnknownClass
#ifdef C_CHECK_DELETE
: public cCheckDelete
#endif C_CHECK_DELETE
{	
	long	m_cRef;
public:
	UnknownClass() { m_cRef = 1; }
	virtual ~UnknownClass() {}
	virtual int Release();
	int GetRef()	const	{ return m_cRef; }
	int AddRef();
	int DecRef();
};


template<class Type>
class UnknownHandle {
public:
	UnknownHandle(Type *p = 0) { set((UnknownClass*)p); }
	UnknownHandle(const UnknownHandle& orig) { set(orig.get()); }
	
	~UnknownHandle() 
	{ 
		release();
	}

	void set(UnknownClass *p) 
	{ 
		ptr_ = p; 
		if(ptr_) 
			ptr_->AddRef(); 
	}
  
	UnknownHandle& operator=(const UnknownHandle& orig) 
	{
		if(ptr_ != orig.ptr_) 
			release();
		set(orig.get());
		return *this;
	}
	
	UnknownHandle& operator=(Type* p) 
	{
		if(ptr_ != p) 
			release();
		set(p);
		return *this;
	}
  
	Type* get() const { return (Type*)ptr_; }
  
	Type* operator->() const { return get(); }
	Type& operator*() const { return *get(); }
	Type* operator() () const { return get(); }
	
	operator Type* () const { return get(); }

	template<class U>
	operator UnknownHandle<U> () { return UnknownHandle<U> (get()); }

private:
	UnknownClass *ptr_;

	void release()
	{
		if(ptr_ && ptr_->DecRef() <= 1) 
			ptr_->Release();
	}
};

#define RELEASE(p) { if(p) { (p)->Release(); (p)=0; } }

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
