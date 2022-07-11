//////////////////////////////////////////////////////////////////
//  Таймеры для отсчета длительностей
//
//  1. Время измеряется в милисекундах.
//
//  2. Таймеры: 
//	отмерение времени
//	задержка события
//	задержка true-условия с "усреднением"
//	выполнение в течении указанного времени
//
//  3. Сброс таймеров - stop().
//
//  4. Типы синхронизации (через SyncroTimer)
//	по clock
//	по frames - с указанием ориентировочного FPS
//
//////////////////////////////////////////////////////////////////
#ifndef	__DURATION_TIMER_H__
#define __DURATION_TIMER_H__

#include "SynchroTimer.h"

class Archive;

extern SyncroTimer global_time;	// детерминированный останавливаемый таймер
extern SyncroTimer frame_time; // недетерминированный неостанавливаемый таймер
extern SyncroTimer scale_time; // недетерминированный ускоряемый таймер

//////////////////////////////////////////////////////////////////
//			Таймера для детерминированной логики
//////////////////////////////////////////////////////////////////
class BaseTimer {
public:
	BaseTimer() { time_ = 0; }
	void stop() { time_ = 0; }

	time_type get_start_time() const { return time_; }
	bool was_started() const { return (time_ > 0); }

	void serialize(Archive& ar);

protected:
	time_type time_;

	static const SyncroTimer& timer() { return global_time; }
};

// Измерение времени
class MeasurementTimer : public BaseTimer {
public:
	void start() { time_ = timer(); }	
	// Время с момента старта
	time_type operator () () const { return time_ ? timer() - time_ : 0; }
};

// Таймер - выполнение в течении указанного времени

class DurationTimer : public BaseTimer {
public:
	void start(time_type duration) { time_ = timer() + duration; }	
	// true:  был start и не прошло время duration, возвращает остаток времени
	time_type operator () () const { return time_ > timer() ? time_ - timer() : 0; }
	int operator ! () const { return (*this)() ? 0 : 1; } 
	void pause() { time_ += timer().delta(); } // вызывать каждый квант для оттягивания конца
};

// Таймер - задержка события
class DelayTimer : public BaseTimer {
public:
	void start(time_type delay)	{ time_ = timer() + delay; }	
	// true:  был start и прошло время delay, возвращает время от конца задержки
	time_type operator () () const { return time_ && timer() >= time_ ? timer() - time_ + 1 : 0; }
	int operator ! () const { return (*this)() ? 0 : 1; } 
	void pause() { if(time_) time_ += timer().delta(); } // вызывать каждый квант для оттягивания конца
};

// Таймер - задержка true-условия 
class DelayConditionTimer : public BaseTimer {
public:
	int operator () (int condition, time_type delay);  // true: condition == true выполнилось время delay назад. 
};

// Таймер - усреднение true-условия 
class AverageConditionTimer : public BaseTimer {
public:
	int operator () (int condition, time_type delay);  // true: condition == true выполнялось время delay. 
};

// Таймер - гистерезис true-условия 
class HysteresisConditionTimer : public BaseTimer {
	int turned_on;
public:
	HysteresisConditionTimer() { turned_on = 0; }
	// true: condition == true выполнялось время on_delay, скидывается, если condition == false время off_delay
	int operator () (int condition, time_type on_delay, time_type off_delay);  
	int operator () (int condition, time_type delay) { return (*this)(condition, delay, delay); }
	int operator () () { return turned_on; }
	void serialize(Archive& ar);
};

class InterpolationTimer : public MeasurementTimer {
public:
	InterpolationTimer() { durationInv_ = 0; }
	void start(time_type duration);
	float operator () () const; // [0..1]
	void serialize(Archive& ar);
private: 
	float durationInv_;
};

//////////////////////////////////////////////////////////////////
//			Таймера для графики
//////////////////////////////////////////////////////////////////
class BaseNonStopTimer {
public:
	BaseNonStopTimer() { time_ = 0; }
	void stop() { time_ = 0; }

	time_type get_start_time() const { return time_; }
	bool was_started() const { return time_ > 0; }

	void serialize(Archive& ar);

protected:
	time_type time_;

	static const SyncroTimer& timer() { return frame_time; }
};

// Измерение времени
class MeasurementNonStopTimer : public BaseNonStopTimer {
public:
	void start() { time_ = timer(); }	
	// Время с момента старта
	time_type operator () () const { return time_ ? timer() - time_ : 0; }
};

// Таймер - выполнение в течении указанного времени

class DurationNonStopTimer : public BaseNonStopTimer {
public:
	void start(time_type duration) { time_ = timer() + duration; }	
	// true:  был start и не прошло время duration, возвращает остаток времени
	time_type operator () () const { return time_ > timer() ? time_ - timer() : 0; }
	int operator ! () const { return (*this)() ? 0 : 1; } 
	void pause() { time_ += timer().delta(); } // вызывать каждый квант для оттягивания конца
};

// Таймер - задержка события
class DelayNonStopTimer : public BaseNonStopTimer {
public:
	void start(time_type delay)	{ time_ = timer() + delay; }	
	// true:  был start и прошло время delay, возвращает время от конца задержки
	time_type operator () () const { return time_ && timer() >= time_ ? timer() - time_ + 1 : 0; }
	int operator ! () const { return (*this)() ? 0 : 1; } 
	void pause() { if(time_) time_ += timer().delta(); } // вызывать каждый квант для оттягивания конца
};

class InterpolationNonStopTimer : public MeasurementNonStopTimer {
public:
	InterpolationNonStopTimer() { durationInv_ = 0; }
	void start(time_type duration);
	float operator () () const; // [0..1]
	void serialize(Archive& ar);
private: 
	float durationInv_;
};

#endif // __DURATION_TIMER_H__
