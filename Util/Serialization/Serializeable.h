#ifndef __SERIALIZEABLE_H_INCLUDED__
#define __SERIALIZEABLE_H_INCLUDED__

#include "Handle.h"

class Archive;

class SerializeableImplBase : public ShareHandleBase{
public:
	SerializeableImplBase(const char* name, const char* nameAlt, const char* typeName)
	: name_(name), nameAlt_(nameAlt), typeName_(typeName)
	{
	}
	virtual ~SerializeableImplBase() {}
	virtual bool serialize(Archive&, const char* name, const char* nameAlt) = 0;
	virtual void* getPointer() const = 0;

	const char* name() const{ return name_; }
	const char* nameAlt() const{ return nameAlt_; }
	const char* typeName() const{ return typeName_; }
protected:
	const char* name_;
	const char* nameAlt_;
	const char* typeName_;
};

template<class T>
struct SerializeableImpl : SerializeableImplBase {
	SerializeableImpl(const T& _data, const char* name, const char* nameAlt)
	: SerializeableImplBase(name, nameAlt, typeid(T).name())
	, data_(const_cast<T&>(_data))
	{
	}
	virtual ~SerializeableImpl () {}
	bool serialize(Archive& ar, const char* name, const char* nameAlt){
		return ar.serialize(data_, name, nameAlt);
	}
	void* getPointer() const{
		return reinterpret_cast<void*>(&data_);
	}
	T& data_;
};

class Serializeable{
public:
    Serializeable()
	: impl_(0)
	{}

	template<class T>
    explicit Serializeable(const T& _data, const char* name = "", const char* nameAlt = ""){
        set(_data, name, nameAlt);
    }

	void setImpl(SerializeableImplBase* impl){
		impl_ = impl;
	}

    ~Serializeable(){}

    template<class T>
    Serializeable& set(const T& _data, const char* name, const char* nameAlt){
        impl_ = new SerializeableImpl<T>(const_cast<T&>(_data), name, nameAlt);
        return *this;
    }

    template<class T>
    Serializeable& set(const T& _data){
        impl_ = new SerializeableImpl<T>(const_cast<T&>(_data), "", "");
        return *this;
    }

    operator bool() const{ return impl_ != 0; }

	bool serialize(Archive& ar){
        if(impl_)
            return impl_->serialize(ar, impl_->name(), impl_->nameAlt());
		return false;
    }

	bool serialize(Archive& ar, const char* name, const char* nameAlt){
        if(impl_)
           return impl_->serialize(ar, name, nameAlt);
		else
			return false;
    }

    const char* name() const{ return impl_ ? impl_->name() : 0; }
    const char* nameAlt() const{ return impl_ ? impl_->nameAlt() : 0; }
	const char* typeName() const { return impl_ ? impl_->typeName() : ""; }

    void* getPointer() const{
        return impl_ ? impl_->getPointer() : 0;
    }

    void release(){
        impl_ = 0;
    }
protected:

	/*
    template<class T>
	struct SerializeableImpl<T*> : SerializeableImplBase {
		SerializeableImpl(const T*& _data)
		: data_(const_cast<T*&>(_data))
		{
			_data->errorFunction();
		}
		~SerializeableImpl(){
		}
		bool serialize(Archive& _archive, const char* name, const char* nameAlt){
			return _archive.serialize(*data_, name, nameAlt);
		}
		void* getPointer() const{
			return reinterpret_cast<void*>(data_);
		}
		T* data_;
	};
	*/

    ShareHandle<SerializeableImplBase> impl_;
};

#endif
