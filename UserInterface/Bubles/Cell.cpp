#include "StdAfx.h"
#include "Serialization\Serialization.h"
#include "cell.h"
#include "blobs.h"


kdCell::kdCell( const Vect2f& position, const Vect2f& endPosition, int colorIndex )
{
	position_ = position;
	endPosition_ = endPosition;
	velocity_ = Vect2f::ZERO;
	velocity_ = (endPosition - position);
	velocity_.set(velocity_.y, -velocity_.x) * 0.1f;
	colorIndex_ = colorIndex;
	work_ = false;
	colorPhase = 0.0f;
	offPhase_ = false;
}

void kdCell::quant(float dt)
{
	if(!offPhase_) {

		if(colorPhase < 1.0f)
			colorPhase += 0.01f;

		if(colorPhase > 1.0f)
			colorPhase = 1.0f;
	}
	else {
		colorPhase -= 0.01f;
		if(colorPhase <= 0)
			work_ = false;
	}
		
	Vect2f a = (endPosition_ - position_) * 0.5f * dt;

	Vect2f addVelocity_(xm_random_generator.fabsRnd(-1,1),xm_random_generator.fabsRnd(-1,1));

	velocity_ += a * dt + addVelocity_ * 0.5f; 
	velocity_ *= 0.97f;
	position_ += velocity_ * dt;
}

void kdCell::setAnchor( const Vect2f& position, float dt)
{
	Vect2f force = (position_ - position);
	force *= 1000/force.norm2();
	velocity_ += force * dt;
}

