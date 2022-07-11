#include "Stdafx.h"
#include "Timers.h"
#include "Serialization.h"

SyncroTimer global_time;
SyncroTimer frame_time;
SyncroTimer scale_time;

void BaseTimer::serialize(Archive& ar) 
{
	ar.serialize(time_, "time", 0);
	if(ar.isInput() && timer() == 1)
		time_ = 0;
}

void BaseNonStopTimer::serialize(Archive& ar) 
{
	ar.serialize(time_, "time", 0);
	if(ar.isInput() && timer() == 1)
		time_ = 0;
}

// Таймер - задержка true-условия 
int DelayConditionTimer::operator () (int condition, time_type delay)
{
	if(condition){
		if(time_){
			if(timer() - time_ >= delay)
				return 1;
			}
		else
			time_ = timer();
		}
	return 0;
}

// Таймер - усреднение true-условия 
int AverageConditionTimer::operator () (int condition, time_type delay)
{
	if(condition){
		if(time_){
			if(timer() - time_ >= delay)
				return 1;
			}
		else
			time_ = timer();
		}
	else 
		time_ = 0;
	return 0;
}

// Таймер - гистерезис true-условия 
int HysteresisConditionTimer::operator () (int condition, time_type on_delay, time_type off_delay)
{
	if(turned_on && condition || !turned_on && !condition){
		time_ = 0;
		return turned_on;
		}

	if(!turned_on && condition){
		if(time_){
			if(timer() - time_ >= on_delay){
				turned_on = 1;
				time_ = 0;
				}
			}
		else
			time_ = timer();
		}

	if(turned_on && !condition){
		if(time_){
			if(timer() - time_ >= off_delay){
				turned_on = 0;
				time_ = 0;
				}
			}
		else
			time_ = timer();
		}

	return turned_on;
}

void InterpolationTimer::start(time_type duration)
{
	durationInv_ = duration ? 1/(float)duration : 0;
	MeasurementTimer::start();
}

float InterpolationTimer::operator()() const 
{
	float t = MeasurementTimer::operator()()*durationInv_;
	if(t < 1.f)
		return t;
	else
		return 1.f;
}

void InterpolationNonStopTimer::start(time_type duration)
{
	durationInv_ = duration ? 1/(float)duration : 0;
	MeasurementNonStopTimer::start();
}

float InterpolationNonStopTimer::operator()() const 
{
	float t = MeasurementNonStopTimer::operator()()*durationInv_;
	if(t < 1.f)
		return t;
	else
		return 1.f;
}

void HysteresisConditionTimer::serialize(Archive& ar) 
{
	BaseTimer::serialize(ar);
	ar.serialize(turned_on, "turned_on", 0);
}

void InterpolationTimer::serialize(Archive& ar) 
{
	BaseTimer::serialize(ar);
	ar.serialize(durationInv_, "durationInv", 0);
}

void InterpolationNonStopTimer::serialize(Archive& ar) 
{
	BaseNonStopTimer::serialize(ar);
	ar.serialize(durationInv_, "durationInv", 0);
}
