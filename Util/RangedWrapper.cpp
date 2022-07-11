#include "stdafx.h"
#include "RangedWrapper.h"

#include "..\Util\Serialization\Serialization.h"
#include "TreeEditor.h"

#include "AttribEditorInterface.h"
AttribEditorInterface& attribEditorInterface();

void RangedWrapperf::clip()
{
	*valuePointer_ = range_.clip(*valuePointer_);
}

bool RangedWrapperf::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	const char* typeName = typeid(RangedWrapperf).name();
	bool editorRegistered = (attribEditorInterface().isTreeEditorRegistered(typeName) != 0);

	if(ar.isOutput())
		clip();

	bool nodeExists;
	if(editorRegistered && ar.isEdit()){
		nodeExists = ar.openStruct(name, nameAlt, typeName);
		ar.serialize(*valuePointer_, "value", 0);
		ar.serialize(range_, "range", 0);
		ar.serialize(step_, "step", 0);
		ar.closeStruct(name);
	}
	else
		nodeExists = ar.serialize(*valuePointer_, name, nameAlt);

	if(ar.isInput())
		clip();

	return nodeExists;
}

void RangedWrapperi::clip()
{
	*valuePointer_ = range_.clip(*valuePointer_);
}


bool RangedWrapperi::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	const char* typeName = typeid(RangedWrapperi).name();
	bool editorRegistered = (attribEditorInterface().isTreeEditorRegistered(typeName) != 0);

	if(ar.isOutput())
		clip();

	bool nodeExists;
	if(editorRegistered && ar.isEdit()){
		nodeExists = ar.openStruct(name, nameAlt, typeName);
		ar.serialize(*valuePointer_, "value", 0);
		ar.serialize(range_, "range", 0);
		ar.serialize(step_, "step", 0);
		ar.closeStruct(name);
	}
	else
		nodeExists = ar.serialize(*valuePointer_, name, nameAlt);

	if(ar.isInput())
		clip();

	return nodeExists;
}


/*
template<typename ScalarType = float>
class RangedWrapper
{
public:
	typedef Range<ScalarType> RangeType;
	typedef RangedWrapper<ScalarType> RangedWrapperType;

	RangedWrapper()
		: value_(ScalarType(0))
		, valuePointer_(&value_)
		, range_(RangeType(ScalarType(0), ScalarType(1)))
		, step_(ScalarType(1))
	{}

	RangedWrapper(ScalarType& _value, ScalarType _range_min, ScalarType _range_max, ScalarType _step = ScalarType(0))
		: value_(ScalarType(0))
		, valuePointer_(&_value)
		, range_(RangeType(_range_min, _range_max))
		, step_(_step)
	{}

	ScalarType& value() { return *valuePointer_; }
	const ScalarType& value() const { return *valuePointer_; }

	inline const RangeType& range () const { return range_; }
	inline ScalarType step() const { return step_; }
	inline void clip () {
		*valuePointer_ = range_.clip(*valuePointer_);
	}
	void serialize (Archive& ar, const char* name, const char* nameAlt) {
		static const char* typeName = typeid(RangedWrapper).name();
		static bool editorRegistered = (TreeEditorFactory::instance().find(typeName) != 0);

		if(ar.isOutput())
			clip();

		if(editorRegistered && ar.isEdit()){
			ar.openStruct(name, nameAlt, typeName);
			ar.serialize(*valuePointer_, "value", 0);
			ar.serialize(range_, "range", 0);
			ar.serialize(step_, "step", 0);
			ar.closeStruct(name);
		}
		else
			ar.serialize(*valuePointer_, name, nameAlt);

		if(ar.isInput())
			clip();
	}
private:
	RangeType range_;
	ScalarType step_;
	ScalarType* valuePointer_;
	ScalarType value_;
};
*/
