#ifndef __SERIALIZEABLE_UNIVERSE_OBJECT_H_INCLUDED__
#define __SERIALIZEABLE_UNIVERSE_OBJECT_H_INCLUDED__

#include "Serializeable.h"
#include "BaseUniverseObject.h"
#include "UnitLink.h"

typedef UnitLink<BaseUniverseObject> Link;

struct SerializeableUniverseObjectImpl : SerializeableImplBase{
	SerializeableUniverseObjectImpl(Link link, const char* name, const char* nameAlt)
	: link_(link)
	, SerializeableImplBase(name, nameAlt, typeid(*link).name())
	{
	}
	void* getPointer() const{
		return reinterpret_cast<void*>(&*link_);
	}
	bool serialize(Archive& ar, const char* name, const char* nameAlt){
		if(link_){
			BaseUniverseObject& object = *link_;
			return ar.serialize(object, name, nameAlt);
		}
		else
			xassert(0);
		return false;
	}
	Link link_;
};

class SerializeableUniverseObject : public Serializeable
{
public:
    SerializeableUniverseObject(Link link, const char* name = "", const char* nameAlt = 0){
		set(link, name, nameAlt);
    }

    SerializeableUniverseObject& set(Link link, const char* name = "", const char* nameAlt = ""){
        impl_ = new SerializeableUniverseObjectImpl(link, name, nameAlt ? nameAlt : "");
		return *this;
    }
};

#endif
