#include "StdAfx.h"

#include "SourceFlash.h"
#include "Serialization.h"
#include "Environment.h"
#include "..\Util\RangedWrapper.h"
#include "EditorVisual.h"
#include "CameraManager.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceFlash, EvolutionType, "тип кривой")
REGISTER_ENUM_ENCLOSED(SourceFlash, LINEAR, "линейная")
REGISTER_ENUM_ENCLOSED(SourceFlash, EXPONENTIAL, "экспонента")
END_ENUM_DESCRIPTOR_ENCLOSED(SourceFlash, EvolutionType)

SourceFlash::SourceFlash()
: SourceBase()
{
	autoKill_ = false;
	intensive_ = 0.5f;
	color_.set(255, 255, 255);
	decByDistance_ = true;
	maxDistance_ = 500;
}

SourceFlash::SourceFlash(const SourceFlash& src)
: SourceBase(src)
{
	autoKill_ = src.autoKill_;
	intensive_ = src.intensive_;
	color_ = src.color_;
	decByDistance_ = src.decByDistance_;
	maxDistance_ = src.maxDistance_;
	increase_ = src.increase_;
	decrease_ = src.decrease_;
}

SourceFlash::~SourceFlash()
{
	if(environment)
		environment->flash()->setActive(false);
}

void SourceFlash::EvolutionPrm::serialize(Archive &ar){
	ar.serialize(increase_, "increase_", 0);
	ar.serialize(time_, "time", "длительность");
	ar.serialize(type_, "type", "тип кривой интенсивности");
	if(type_ == EXPONENTIAL){
		ar.serialize(RangedWrapperf(naklon_, 0.2f, 8.f), "naklon", "крутизна кривой интенсивности");
		ar.serialize(vipuclaya_, "vipuclaya", "кривая выпуклая интенсивности");
	}
}

void SourceFlash::serialize(Archive &ar){
	__super::serialize(ar);

	ar.serialize(color_, "color", "цвет вспышки");
	ar.serialize(decByDistance_, "decByDistancer", "уменьшать засветку при отдалении");
	if(decByDistance_){
		ar.serialize(maxDistance_, "maxDistance", "максимальная дистанция до наблюдателя");
	}
	ar.serialize(autoKill_, "autoKill", "автоматически удалять источник после затухания");
	ar.serialize(increase_, "increase", "параметры нарастания вспышки");
	increase_.increase_ = true;
	ar.serialize(decrease_, "decrease", "параметры убывания вспышки");
	decrease_.increase_ = false;

	// для редактора
	if(ar.isInput() && enabled()){
		setActivity(active_);
	}
}

void SourceFlash::start()
{
	__super::start();
	if(increase_.time_ > FLT_EPS){
		inreaseTime_.start(increase_.time_ * 1000);
		phase_.start(increase_.time_ * 1000);
		intensive_ = 0.f;
	}else{
		inreaseTime_.start(-1);
		intensive_ =  1.f;
	}

	environment->flash()->init(intensive_);
	environment->flash()->setColor(color_);
	environment->flash()->addFlash();

	increase_.init();
	decrease_.init();

	environment->flash()->setActive(true);
}

void SourceFlash::stop()
{
	__super::stop();
	environment->flash()->setActive(false);
}

void SourceFlash::EvolutionPrm::init() const{
	switch(type_){
	case LINEAR:
		scale_ = 1.f;
		expb2_ = 1.f;
		break;
	case EXPONENTIAL:
		expb2_ = expf(naklon_ * naklon_);
		scale_ = 1.f / (expf(sqr(naklon_ + 1.f)) - expb2_);
		break;
	}
}

float SourceFlash::EvolutionPrm::operator()(float phase){
	switch(type_){
	case LINEAR:
		if(increase_)
			return phase;
		else
			return 1.f - phase;
	case EXPONENTIAL:
		if(increase_)
			if(vipuclaya_)
				return 1.f - _exp(1.f - phase);
			else
				return _exp(phase);
		else
			if(vipuclaya_)
				return 1.f - _exp(phase);
			else
				return _exp(1.f - phase);
		
		break;	
	}

	return 0;
}

void SourceFlash::quant(){
	__super::quant();

	if(!active_)
		return/* true*/;

	if(inreaseTime_())
		intensive_ = increase_(phase_());
	else if(!inreaseTime_.was_started()){
		float ph = phase_();
		if(ph >= 1.f - FLT_EPS){
			environment->flash()->setActive(false);
			environment->flash()->decFlash();
			if(autoKill_)
				kill();
		}
		intensive_ = decrease_(ph);
	}else{
		inreaseTime_.stop();
		phase_.start(decrease_.time_ * 1000);
		intensive_ = 1.f;
	}

	if(decByDistance_){
		float dist2 = cameraManager->coordinate().position().distance2(position());
		float max2 = maxDistance_ * maxDistance_;
		if(dist2 > max2)
			intensive_ = 0.f;
		else
			intensive_ *= (1.f - dist2 / max2);
	}
	
	// так плохо делать, надо через команды
	environment->flash()->addIntensity(intensive_);
}

void SourceFlash::showEditor() const
{
	__super::showEditor();

	if(editorVisual().isVisible(objectClass())){
		char buf[255];
		itoa(intensive_*100, buf, 10);
		editorVisual().drawText(position(), buf, EditorVisual::TEXT_PROPERTIES);
	}

}
