#ifndef __RANGED_WRAPPER_H_INCLUDED__
#define __RANGED_WRAPPER_H_INCLUDED__

#include "Range.h"

class Archive;

class RangedWrapperf
{
public:

	RangedWrapperf()
	: value_(0.f)
	, valuePointer_(&value_)
	, range_(Rangef(0.f, 1.f))
	, step_(1.f)
	{}

	RangedWrapperf(float& _value, float _range_min, float _range_max, float _step = 0.f)
	: value_(0.f)
	, valuePointer_(&_value)
	, range_(Rangef(_range_min, _range_max))
	, step_(_step)
	{}
	
	float& value() { return *valuePointer_; }
	const float& value() const { return *valuePointer_; }

	const Rangef& range() const { return range_; }
	float step() const { return step_; }
	void clip();

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:
	Rangef range_;
	float step_;
	float* valuePointer_;
	float value_;
};


class RangedWrapperi
{
public:

	RangedWrapperi()
		: value_(0.f)
		, valuePointer_(&value_)
		, range_(Rangei(0, 1))
		, step_(1)
	{}

	RangedWrapperi(int& _value, int _range_min, int _range_max, int _step = 0)
		: value_(0.f)
		, valuePointer_(&_value)
		, range_(Rangei(_range_min, _range_max))
		, step_(_step)
	{}

	int& value() { return *valuePointer_; }
	const int& value() const { return *valuePointer_; }

	const Rangei& range() const { return range_; }
	int step() const { return step_; }
	void clip();

	bool serialize(Archive& ar, const char* name, const char* nameAlt);

private:
	Rangei range_;
	int step_;
	int* valuePointer_;
	int value_;
};

#endif
