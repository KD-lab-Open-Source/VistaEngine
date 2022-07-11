#ifndef __CALL_WRAPPER_H__
#define __CALL_WRAPPER_H__

class CallWrapperBase {
public:
	virtual bool call() = 0;
};

template<class ClassType>
class CallWrapper : public CallWrapperBase
{
public:
	typedef bool (ClassType::*pFunc)();

	CallWrapper(ClassType* pObj, pFunc pF) :
		object_(pObj),
		func_(pF) {}
	
	virtual bool call() {
		return (object_->*func_)();
	}

private:
	ClassType *object_;
	pFunc func_;
};

/* пример использования:
class cTest{
public:
	cTest() : a(0) {}

	bool myFunc() {
		a = 1;
		return true;
	}
	int a;
} testObj;

CallWrapper<cTest> tmp(&testObj, cTest::myFunc);
tmp.call();
*/

template<class ClassType, class PType>
class CallWrapper1 : public CallWrapperBase
{
public:
	typedef bool (ClassType::*pFunc)(PType&);

	CallWrapper1(ClassType* pObj, pFunc pF, PType& par) :
		object_(pObj),
		func_(pF) {
		par_ = par;
		}
	
	bool call() {
		return (object_->*func_)(par_);
	}

private:
	ClassType *object_;
	pFunc func_;

	// параметр call
	PType par_;
};

#endif //__CALL_WRAPPER_H__
