#pragma once

#include "UnitLink.h"
#include "Serialization\SerializationTypes.h"

class UnitReal;
class SourceBase;

class LabelObject 
{
public:
	LabelObject(const char* label = "");
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	const char* c_str() const { return label_; }
	operator BaseUniverseObject*() const;
	BaseUniverseObject* operator->() const { return operator BaseUniverseObject*(); } 

private:
	ComboListString label_;
	mutable bool found_;
	mutable UnitLink<BaseUniverseObject> link_;
};

class LabelUnit 
{
public:
	LabelUnit(const char* label = "");
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	const char* c_str() const { return label_; }
	operator UnitReal*() const;
	UnitReal* operator->() const { return operator UnitReal*(); } 

private:
	ComboListString label_;
	mutable bool found_;
	mutable UnitLink<UnitReal> link_;
};

class LabelSource : public UnitLink<SourceBase> 
{
public:
	LabelSource(const char* label = "");
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	const char* c_str() const { return label_; }
	operator SourceBase*() const;
	SourceBase* operator->() const { return operator SourceBase*(); } 

private:
	ComboListString label_;
	mutable bool found_;
	mutable UnitLink<SourceBase> link_;
};