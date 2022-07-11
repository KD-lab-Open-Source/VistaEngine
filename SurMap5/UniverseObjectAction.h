#ifndef __UNIVERSE_OBJECT_H_INCLUDED__
#define __UNIVERSE_OBJECT_H_INCLUDED__

#include "Handle.h"
#include "BaseUniverseObject.h"
#include "Serialization\Factory.h"

class UniverseObjectActionList : public std::vector<const char*>{
public:
	template<class Action>
	void add(){
		push_back(typeid(Action).name());
	}
};

class UniverseObjectAction : public ShareHandleBase{
public:
	virtual void operator()(BaseUniverseObject& object) = 0;
	virtual const char* description() const{ return ""; }
	virtual bool canBeUsedOn(BaseUniverseObject& object) { return true; }
};

namespace UniverseObjectActions{
	typedef ::Factory<std::string, UniverseObjectAction> Factory;
};


#endif
