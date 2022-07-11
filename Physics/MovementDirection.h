#ifndef __MOVEMENT_DIRECTION_H__
#define __MOVEMENT_DIRECTION_H__

#include "XMath\SafeMath.h"

///////////////////////////////////////////////////////////////
//
//    class MovementDirection
//
///////////////////////////////////////////////////////////////

class MovementDirection
{
public:
	MovementDirection() {}
	MovementDirection(float angle) : angle_(angle) { dassert(isLess(angle_, M_PI) && isGreater(angle_, -M_PI)); }
	MovementDirection(Vect2f direction)	: angle_(-atan2f(direction.x, direction.y)) {}
	operator const Vect2f () const { return Vect2f(-sinf(angle_), cosf(angle_)); }
	operator const QuatF () const { return QuatF(angle_, Vect3f::K, false); }
	operator const float& () const { return angle_; }

	const MovementDirection& negate()
	{
		angle_ = cycleAngle(angle_) - M_PI;
		return *this;
	}

	const MovementDirection& operator += (float angle)
	{
		angle_ = cycleAngle(angle_ + angle + M_PI) - M_PI;
		return *this;
	}

	const MovementDirection& operator -= (float angle)
	{
		angle_ = cycleAngle(angle_ - angle + M_PI) - M_PI;
		return *this;
	}
	
	MovementDirection operator + (float angle) const
	{ 
		return MovementDirection(cycleAngle(angle_ + angle + M_PI) - M_PI);
	}

	MovementDirection operator - (float angle) const
	{
		return MovementDirection(cycleAngle(angle_ - angle + M_PI) - M_PI);
	}

private:
	float angle_;
};

#endif // __MOVEMENT_DIRECTION_H__