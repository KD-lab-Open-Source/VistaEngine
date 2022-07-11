#include "StdAfx.h"
#include "BaseUniverseObject.h"
#include "RenderObjects.h"
#include "Serialization\Serialization.h"

BaseUniverseObject::BaseUniverseObject() 
: radius_(10.f), 
pose_(Se3f::ID), 
selected_(false)
{
}

BaseUniverseObject::~BaseUniverseObject()
{
	if(!dead())
		unitID_.unregisterUnit();
}

void BaseUniverseObject::enable()
{ 
	unitID_.registerUnit(this);
}

void BaseUniverseObject::Kill()
{
	if(!dead())
		unitID_.unregisterUnit();
}

void BaseUniverseObject::serialize(Archive& ar)
{
	if(!ar.isEdit()){
		if(ar.isInput()){
			unitID_.unregisterUnit();
			xassert(!unitID_.numRefs());
		}
		unitID_.serialize(ar);
		if(ar.isInput() && !dead())
			unitID_.registerUnit(this);
	}
}

float BaseUniverseObject::angleZ() const 
{ 
	return SIGN(orientation().angle(), orientation().z()); 
} 
