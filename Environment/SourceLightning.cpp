#include "StdAfx.h"

#include "Serialization\Serialization.h"
#include "SourceLightning.h"
#include "Environment.h"
#include "Render\src\Scene.h"

#include "vmap.h"
#include "RenderObjects.h"
#include "Squad.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceLightning, AllocationType, "LightingZoneAllocationType");
REGISTER_ENUM_ENCLOSED(SourceLightning, TERRA_CENTER, "В центр зоны");
REGISTER_ENUM_ENCLOSED(SourceLightning, TERRA_ROUND, "Равномерно по окружности");
REGISTER_ENUM_ENCLOSED(SourceLightning, TERRA_RANDOM, "Случайно по зоне");
REGISTER_ENUM_ENCLOSED(SourceLightning, SPHERE_SPHERE, "Случайно по сфере");
REGISTER_ENUM_ENCLOSED(SourceLightning, SPHERE_RANDOM, "Случайно по шару");
END_ENUM_DESCRIPTOR_ENCLOSED(SourceLightning, AllocationType);

#pragma warning(disable: 4355)

bool ChainLightningSourceFunctor::canApply(UnitActing* unit) const
{
	return zone_->canApply(unit);
}

ParameterSet ChainLightningSourceFunctor::damage() const
{
	int time = (zone_->lifeTime() > 0) ? zone_->lifeTime() : SourceBase::MAX_DEFAULT_LIFETIME;
	float dt = (time >= 200) ? (float(logicTimePeriod) / float(time)) : 1.f;

	WeaponSourcePrm prm;
	zone_->getParameters(prm);
	ParameterSet dmg = prm.damage();
	dmg *= dt;

	return dmg;
}

SourceLightning::SourceLightning()
: SourceDamage()
, lightning_(0)
, strike_(0)
, chainLightningFunctor_(this)
, chainLightning_(&chainLightningFunctor_)
{
	turnOfByTarget_ = true;
	num_lights_ = 20;
	height_ = 100.f;
	alloc_type_ = TERRA_ROUND;
	useChainLightning_ = false;
	killInProcess_ = false;
}

SourceLightning::SourceLightning(const SourceLightning& original)
: SourceDamage(original)
, lightning_(0)
, strike_(0)
, turnOfByTarget_(original.turnOfByTarget_)
, num_lights_(original.num_lights_)
, height_(original.height_)
, alloc_type_(original.alloc_type_)
, permanent_effect_(original.permanent_effect_)
, strike_effect_(original.strike_effect_)
, useChainLightning_(original.useChainLightning_)
, chainLightningAttribute_(original.chainLightningAttribute_)
, chainLightningFunctor_(this)
, chainLightning_(&chainLightningFunctor_)
, killInProcess_(false)
{
}

SourceLightning::~SourceLightning()
{
	xassert(!lightning_ && !strike_);
}

void SourceLightning::serialize(Archive& ar){
	__super::serialize(ar);

	ar.serialize(alloc_type_, "alloc_type", "размещение по зоне");

	ar.serialize(height_, "height", "высота образования молний");
	if(alloc_type_ != TERRA_CENTER)
		ar.serialize(num_lights_, "num_lights", "количество молний");

	ar.serialize(permanent_effect_, "permanent_effect", "постоянная молния в зоне");
	ar.serialize(strike_effect_, "strike_effect", "атака юнита в зоне");
	ar.serialize(turnOfByTarget_, "turnOfByTarget", "выключать постоянный эффект молнии при атаке");
	
	ar.serialize(useChainLightning_, "useChainLightning_", "использовать цепной эффект");
	if(useChainLightning_){
		ar.serialize(chainLightningAttribute_, "chainLightningAttribute", "настройки цепной молнии");
		if(!chainLightningAttribute_.strike_effect_.get())
			chainLightningAttribute_.strike_effect_ = strike_effect_;
	}

	serializationApply(ar);
}


Vect3f SourceLightning::setHeight(int x, int y, int h){
	return Vect3f(x, y, vMap.getApproxAlt(x, y)+h);
}

void SourceLightning::start()
{
	xassert(!killInProcess_);
	__super::start();
	if(useChainLightning_)
		chainLightning_.start(&chainLightningAttribute_);
}

void SourceLightning::stop()
{
	__super::stop();
	chainLightning_.stop();
}

bool SourceLightning::killRequest()
{
	if(!chainLightning_.active())
		return true;

	setActivity(false);
	killInProcess_ = true;

	return false;
}

void SourceLightning::quant()
{
	strike_ends_.clear();

	if(killInProcess_ && !chainLightning_.active())
		kill();

	__super::quant();

	if(active() && chainLightning_.needChainUpdate())
		chainLightning_.update(targets_);

	targets_.clear();

	chainLightning_.quant();

	if(!active())
		return;
	
	Update();
}

void SourceLightning::apply(UnitBase* target)
{
	__super::apply(target);

	if(!target->alive() || !target->attr().isActing())
		return;
	
	strike_ends_.push_back(target->position());
	targets_.push_back(safe_cast<UnitActing*>(target));
}

void SourceLightning::effectStop()
{
	__super::effectStop();

	turnOff(lightning_);
	light_ends_.clear();
	
	turnOff(strike_);
	strike_ends_.clear();
}

void SourceLightning::effectStart()
{
	__super::effectStart();
	if(!active_) return;

	center_ = setHeight(position().xi(), position().yi(), height_);

	if(num_lights_ <= 0)
		return;

	float rad = radius(), da;
	int cur;

	switch(alloc_type_){
	case TERRA_CENTER:
		light_ends_.push_back(setHeight(center_.xi(), center_.yi()+1));
		light_ends_.push_back(setHeight(center_.xi()-1, center_.yi()));
		light_ends_.push_back(setHeight(center_.xi()+1, center_.yi()));
		break;
	case TERRA_ROUND:
		da = 2.*M_PI/num_lights_;
		for(int i = 0; i < num_lights_; i++){
			float x = rad*cosf(da*i), y = rad*sinf(da*i);
			light_ends_.push_back(setHeight(center_.xi()+x, center_.yi()+y));
		}
		break;
	case TERRA_RANDOM:
		cur = num_lights_ > 9 ? num_lights_ / 3 : 0;
		da = cur ? 2.*M_PI/cur : 0;
		for(int i = 0; i < cur; i++){
			float x = rad*cosf(da*i), y = rad*sinf(da*i);
			light_ends_.push_back(setHeight(center_.xi()+x, center_.yi()+y));
		}
		cur = num_lights_ - cur;
		while(cur){
			float x = logicRNDfrnd(rad);
			float y = logicRNDfrnd(rad);
			if(x*x + y*y > rad*rad) continue;
			cur--;
			light_ends_.push_back(setHeight(center_.xi()+x, center_.yi()+y));
		}
		break;
	case SPHERE_SPHERE:
		for(int i = 0; i < num_lights_; i++){
			float a = logicRNDfabsRnd(2.*M_PI);
			float g = logicRNDfabsRnd(M_PI);
			float x = rad*cosf(a)*sinf(g);
			float y = rad*sinf(a)*sinf(g);
			float z = rad*cosf(g);
			light_ends_.push_back(center_ + Vect3f(x, y, z));
		}
		break;
	case SPHERE_RANDOM:
		cur = num_lights_;
		while(cur){
			float x = logicRNDfrnd(rad);
			float y = logicRNDfrnd(rad);
			float z = logicRNDfrnd(rad);
			if(x*x + y*y + z*z > rad*rad) continue;
			cur--;
			light_ends_.push_back(center_ + Vect3f(x, y, z));
		}
	}

	Update();

}

void SourceLightning::Update()
{
	if(targetInZone && turnOfByTarget_)
		turnOff(lightning_);
	else{
		xassert(light_ends_.size());
		if(!lightning_ && permanent_effect_.get()){
			lightning_ = environment->scene()->CreateEffectDetached(*permanent_effect_->getEffect(1.f), 0);
			lightning_->SetPosition(MatXf(pose_));
			lightning_->setCycled(true);
			lightning_->SetTarget(center_, light_ends_);
			attachSmart(lightning_);
		}
		else
			setTarget(lightning_, center_, light_ends_);
	}

	if(strike_ends_.size()){
		if(!strike_ && strike_effect_.get()){
			strike_ = environment->scene()->CreateEffectDetached(*strike_effect_->getEffect(1.f), 0);
			strike_->SetPosition(MatXf(pose_));
			attachSmart(strike_);
		}
		setTarget(strike_, center_, strike_ends_);
	}
	else
		turnOff(strike_);
}

void SourceLightning::setPose(const Se3f &_pos, bool init)
{
	Vect3f delta = _pos.trans();
	delta.sub(position());

	if(delta.norm2() < FLT_EPS)
		return;

	__super::setPose(_pos, init);

	center_ = setHeight(position().xi(), position().yi(), height_);

	std::vector<Vect3f>::iterator it;
	FOR_EACH(light_ends_, it)
		it->add(delta);
}

void SourceLightning::setRadius(float radius)
{
	__super::setRadius(radius);
	if(active())
		effectStart();
}

void SourceLightning::setTarget(cEffect* eff, const Vect3f& center, const Vect3fVect& ends)
{
	if(eff){
		if(MT_IS_GRAPH())
			eff->SetTarget(center, ends);
		else{
			streamLogicCommand.set(fCommandSetTarget, eff) << (int)ends.size() << center;
			Vect3fVect::const_iterator it;
			FOR_EACH(ends, it)
				streamLogicCommand << *it;
		}
	}
}

void SourceLightning::turnOff(cEffect*& eff)
{
	if(eff){
		//streamLogicCommand.set(fCommandSetCycle, eff) << false;
		//streamLogicCommand.set(fCommandSetAutoDeleteAfterLife, eff) << true;
		eff->Release();
		eff = 0;
	}
}

void SourceLightning::showEditor() const
{
	__super::showEditor();

	Vect3fVect::const_iterator it;
	
	if(!targetInZone || !turnOfByTarget_)
		FOR_EACH(light_ends_, it){
			Vect3f v = *it;
			gb_RenderDevice->DrawLine(v + Vect3f(-2.0f, 0.0f, 0.0f), v + Vect3f(2.0f, 0.0f, 0.0f), Color4c::YELLOW);
			gb_RenderDevice->DrawLine(v + Vect3f(0.0f, -2.0f, 0.0f), v + Vect3f(0.0f, 2.0f, 0.0f), Color4c::YELLOW);
			gb_RenderDevice->DrawLine(v + Vect3f(0.0f, 0.0f, -2.0f), v + Vect3f(0.0f, 0.0f, 2.0f), Color4c::YELLOW);
		}
	
	FOR_EACH(strike_ends_, it){
		Vect3f v = *it;
		gb_RenderDevice->DrawLine(v + Vect3f(-5.0f, 0.0f, 0.0f), v + Vect3f(5.0f, 0.0f, 0.0f), Color4c::RED);
		gb_RenderDevice->DrawLine(v + Vect3f(0.0f, -5.0f, 0.0f), v + Vect3f(0.0f, 5.0f, 0.0f), Color4c::RED);
		gb_RenderDevice->DrawLine(v + Vect3f(0.0f, 0.0f, -5.0f), v + Vect3f(0.0f, 0.0f, 5.0f), Color4c::RED);
	}
}

void SourceLightning::showDebug() const
{
	__super::showDebug();

	Vect3fVect::const_iterator it;
	if(!targetInZone || !turnOfByTarget_)
		FOR_EACH(light_ends_, it){
			Vect3f v = *it;
			show_line(v + Vect3f(-3.0f, 0.0f, 0.0f), v + Vect3f(3.0f, 0.0f, 0.0f), Color4c::YELLOW);
			show_line(v + Vect3f(0.0f, -3.0f, 0.0f), v + Vect3f(0.0f, 3.0f, 0.0f), Color4c::YELLOW);
			show_line(v + Vect3f(0.0f, 0.0f, -3.0f), v + Vect3f(0.0f, 0.0f, 3.0f), Color4c::YELLOW);
		}

	FOR_EACH(strike_ends_, it){
		Vect3f v = *it;
		show_line(v + Vect3f(-5.0f, 0.0f, 0.0f), v + Vect3f(5.0f, 0.0f, 0.0f), Color4c::RED);
		show_line(v + Vect3f(0.0f, -5.0f, 0.0f), v + Vect3f(0.0f, 5.0f, 0.0f), Color4c::RED);
		show_line(v + Vect3f(0.0f, 0.0f, -5.0f), v + Vect3f(0.0f, 0.0f, 5.0f), Color4c::RED);
	}

	chainLightning_.showDebug();
}
