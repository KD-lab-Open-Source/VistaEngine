#include "stdafx.h"
#include "Range.h"

#include "..\Util\Serialization\Serialization.h"

void Rangef::set(float _min, float _max)
{
	min_ = _min;
	max_ = _max;
}

Rangef Rangef::intersection (const Rangef& _range)
{
	float begin;
	float end;
	if(maximum() < _range.minimum() || minimum() > _range.maximum())
		return Rangef(0.f, 0.f);

	if(include(_range.minimum()))
		begin = _range.minimum();
	else
		begin = minimum();

	if(include(_range.maximum()))
		end = _range.maximum();
	else
		end = maximum();
	return Rangef(begin, end);
}


float Rangef::clip(float& _value) const
{
	if(include(_value))
		return _value;
	else{
		if(_value < minimum())
			return minimum();
		else
			return maximum();
	}
}

void Rangef::serialize(Archive& ar)
{
	ar.serialize(min_, "min_", "Минимум");
	ar.serialize(max_, "max_", "Максимум");
}


// --------------------- Rangei

void Rangei::set(int _min, int _max)
{
	min_ = _min;
	max_ = _max;
}

Rangei Rangei::intersection (const Rangei& _range)
{
	int begin;
	int end;
	if(maximum() < _range.minimum() || minimum() > _range.maximum())
		return Rangei(0, 0);

	if(include(_range.minimum()))
		begin = _range.minimum();
	else
		begin = minimum();

	if(include(_range.maximum()))
		end = _range.maximum();
	else
		end = maximum();
	return Rangei(begin, end);
}


int Rangei::clip(int& _value)
{
	if(include(_value))
		return _value;
	else{
		if(_value < minimum())
			return minimum();
		else
			return maximum();
	}
}

void Rangei::serialize(Archive& ar)
{
	ar.serialize(min_, "min_", "Минимум");
	ar.serialize(max_, "max_", "Максимум");
}


/*
/// Абстракция закрытого интервала (отрезка).
template<typename ScalarType = float>
class Range
{
public:
	typedef Range<ScalarType> RangeType;

	Range (ScalarType _min = ScalarType(0), ScalarType _max = ScalarType(0)) :
	min_ (_min),
		max_ (_max)
	{}

	inline ScalarType minimum () const 
	{
		return min_;
	}
	inline void minimum (ScalarType _min) 
	{
		min_ = _min;
	}
	inline ScalarType maximum () const 
	{
		return max_;
	}
	inline void maximum (ScalarType _max) 
	{
		max_ = _max;
	}
	inline void set (ScalarType _min, ScalarType _max)
	{
		min_ = _min;
		max_ = _max;
	}

	inline ScalarType length () const
	{
		return (maximum () - minimum ());
	}

	inline ScalarType center() const
	{
		return (maximum() + minimum()) / 2;
	}

	/// Корректен ли интервал (нет - в случае когда minimum > maximum);
	inline bool is_valid () const
	{
		return (minimum () <= maximum ());
	}

	/// Включает ли отрезок (закрытый интервал) точку \c _value.
	inline bool include (ScalarType _value) const
	{
		return (minimum () <= _value) && (maximum () >= _value);
	}
	/// Включает ли интервал в себя \c _range.
	inline bool include (const RangeType& _range) const
	{
		return (minimum () <= _range.minimum ()) && (maximum () >= _range.maximum ());
	}

	/// Возвращает пересечение интервала *this и \c _range.
	inline RangeType intersection (const RangeType& _range)
	{
		ScalarType begin;
		ScalarType end;
		if (maximum () < _range.minimum () || minimum () > _range.maximum ())
			return RangeType (0, 0);

		if (include (_range.minimum ()))
			begin = _range.minimum ();
		else
			begin = minimum ();

		if (include (_range.maximum ()))
			end = _range.maximum ();
		else
			end = maximum ();
		return RangeType (begin, end);
	}

	/// Возвращает \c _value в пределах интервала [minimum, maximum].
	inline ScalarType clip (ScalarType& _value)
	{
		if (include (_value))
			return _value;
		else
		{
			if (_value < minimum ())
				return minimum ();
			else
				return maximum ();
		}
	}

		void serialize(Archive& ar){
			ar.serialize(min_, "min_", "Минимум");
			ar.serialize(max_, "max_", "Максимум");
		}

private:
	ScalarType min_;
	ScalarType max_;
};
*/
