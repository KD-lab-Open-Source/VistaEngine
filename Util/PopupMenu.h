#ifndef __POPUP_MENU_H_INCLUDED__
#define __POPUP_MENU_H_INCLUDED__

#include <list>
#include "Handle.h"

template<class ResultType, class T>
struct BindMethod{
	BindMethod(T& t, ResultType (T::*method)())
	: object_(t)
	, method_(method)
	{        
	}
	ResultType operator()(){
		return (object_.*method_)();
	}
private:
	T& object_;
	ResultType (T::*method_)();
};

template<class ResultType, class T, class Arg1>
struct BindMethod1{
	BindMethod1(T& t, ResultType (T::*method)(Arg1))
	: object_(t)
	, method_(method)
	{        
	}
	ResultType operator()(Arg1 arg1){
		return (object_.*method_)(arg1);
	}
private:
	T& object_;
	ResultType (T::*method_)(Arg1);
};

template<class ResultType, class T>
BindMethod<ResultType, T> bindMethod(T& object, ResultType (T::*method)()){
    return BindMethod<ResultType, T>(object, method);
}

template<class ResultType, class T, class Arg1>
BindMethod1<ResultType, T, Arg1> bindMethod(T& object, ResultType (T::*method)(Arg1)){
    return BindMethod1<ResultType, T, Arg1>(object, method);
}

template<class ResultType, class Func, class Arg>
struct BindArgument{
    BindArgument(Func func, Arg arg)
    : func_(func)
    , arg_(arg)
    {
    }
    ResultType operator()(){
        return func_(arg_);
    }
private:
    Func func_;
    Arg arg_;
};

template<class ResultType, class Func, class Arg>
BindArgument<ResultType, Func, Arg> bindArgument(Func func, Arg arg){
    return BindArgument<ResultType, Func, Arg>(func, arg);
}


//template<class ReturnType = void>
class Functor{
public:
    Functor()
    : impl_(0)
    {
    }

	typedef void ReturnType;

    template<class Func>
    Functor(Func func)
    : impl_(new FunctorImpl<Func>(func))
    {

    }

    template<class T>
    Functor(T& object, void(T::*method)(void)){
		impl_ = Functor::makeFunctorImpl(bindMethod(object, method));
    }

    ReturnType operator()(){
        xassert(impl_);
        return (*impl_)();
    }
    operator bool() const{
        return impl_ != 0;
    }
private:

	class FunctorImplBase : public ShareHandleBase
	{
	public:
        virtual ReturnType operator()() = 0;
    };

	template<class T>
	static FunctorImplBase* makeFunctorImpl(T t){
		return new FunctorImpl<T>(t);
	}


    template<class Func>
    class FunctorImpl : public FunctorImplBase{
    public:
        FunctorImpl(Func func)
        : func_(func)
        {}
        ReturnType operator()(){
            func_();
        }
    private:
        Func func_;
    };
    ShareHandle<FunctorImplBase> impl_;
};

class PopupMenu;
class PopupMenuItem : public ShareHandleBase{
public:
	friend PopupMenu;
    typedef std::list<ShareHandle<PopupMenuItem> > Children;

    PopupMenuItem(const char* text = "")
    : id_(0)
	, parent_(0)
	, text_(text)
	, checked_(false)
	, enabled_(true)
    {}

    PopupMenuItem(const char* text, Functor callback)
    : callback_(callback)
	, id_(0)
	, parent_(0)
	, text_(text)
	, checked_(false)
	, enabled_(true)
    {}
	~PopupMenuItem(){
		if(id_ && !children_.empty()){
			::DestroyMenu(menuHandle());
		}
	}
	void check(bool checked){ checked_ = checked; }
	bool isChecked() const{ return checked_; }

	void enable(bool enabled){ enabled_ = enabled; }
	bool isEnabled() const{ return enabled_; }

    const char* text() { return text_.c_str(); }
	PopupMenuItem& addSeparator()
	{
		return add("-");
	}

    PopupMenuItem& add(const char* text){
		return add(PopupMenuItem(text, Functor()));
	}
	/*
    PopupMenuItem& add(const char* text, Functor func){
		return add(PopupMenuItem(text, func));
	}
	*/
	PopupMenuItem& connect(Functor func){
		callback_ = func;
		return *this;
	}
	PopupMenuItem* find(const char* text);

	Functor callback() { return callback_; }
	PopupMenuItem* parent() { return parent_; }
	Children& children() { return children_; };
    const Children& children() const { return children_; }
	bool empty() const { return children_.empty(); }
private:
    PopupMenuItem& add(PopupMenuItem& item){
        children_.push_back(new PopupMenuItem(item));
        children_.back()->parent_ = this;
		return *children_.back();
    }

    HMENU menuHandle(){
        xassert(!children_.empty());
        return HMENU(id_);
    }
	void setMenuHandle(HMENU menu){
        xassert(!children_.empty());
		id_ = DWORD(menu);
	}
    UINT menuID(){
        xassert(children_.empty());
        return UINT(id_);
    }
	void setMenuID(UINT id){
        xassert(children_.empty());
		id_ = DWORD(id);
	}

    DWORD id_;
    Functor callback_;

	bool checked_;
	bool enabled_;
    std::string text_;
    PopupMenuItem* parent_;
    Children children_;
};


class PopupMenu{
public:
    enum {
        ID_RANGE_MIN = 32768,
        ID_RANGE_MAX = 32767 + 16384
    };
    PopupMenu(int maxItems);
    PopupMenuItem& root() { return root_; };
    const PopupMenuItem& root() const { return root_; };
    void spawn(POINT pt, HWND window);
	void clear();

	BOOL onCommand(WPARAM wParam, LPARAM lParam);
private:
    PopupMenuItem* nextItem(PopupMenuItem* item) const;

    PopupMenuItem root_;

    void assignIDs();

    UINT idRangeStart_;
    UINT idRangeEnd_;
};

#endif
