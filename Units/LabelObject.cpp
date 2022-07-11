#include "stdafx.h"
#include "LabelObject.h"
#include "Universe.h"
#include "Environment\SourceManager.h"
#include "Units\RealUnit.h"
#include "Environment\SourceBase.h"

LabelObject::LabelObject(const char* label)
: label_(label), found_(false)
{
}

bool LabelObject::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit() && ar.isOutput() && sourceManager)
		label_.setComboList(sourceManager->anchorLabelsComboList().c_str());
	
	bool result = ar.serialize(label_, name, nameAlt);
	
	if(ar.isInput())
		found_ = false;

	return result;
}

LabelObject::operator BaseUniverseObject*() const 
{ 
	if(!found_ && sourceManager && strlen(label_)){
		link_ = sourceManager->findAnchor(label_);
		found_ = true;
	}
	return link_; 
}

LabelUnit::LabelUnit(const char* label)
: label_(label), found_(false)
{
}

bool LabelUnit::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit() && ar.isOutput() && universe())
		label_.setComboList(universe()->unitLabelsComboList().c_str());
	
	bool result = ar.serialize(label_, name, nameAlt);
	
	if(ar.isInput())
		found_ = false;

	return result;
}

LabelUnit::operator UnitReal*() const 
{ 
	if(!found_ && universe() && strlen(label_)){
		link_ = universe()->findUnitByLabel(label_);
		found_ = true;
	}
	return link_; 
}

LabelSource::LabelSource(const char* label)
: label_(label), found_(false)
{
}

bool LabelSource::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit() && ar.isOutput() && sourceManager)
		label_.setComboList(sourceManager->sourceLabelsComboList().c_str());
	
	bool result = ar.serialize(label_, name, nameAlt);
	
	if(ar.isInput() && sourceManager && strlen(label_))
		found_ = false;

	return result;
}

LabelSource::operator SourceBase*() const 
{ 
	if(!found_ && sourceManager && strlen(label_)){
		link_ = sourceManager->findSource(label_);
		found_ = true;
	}
	return link_; 
}
