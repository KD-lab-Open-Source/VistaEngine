#include "stdafx.h"
#include "Universe.h"
#include "EffectController.h"
#include "UnitActing.h"
#include "Serialization\Serialization.h"
#include "Sound.h"
#include "SoundApp.h"
#include "Water\Water.h"
#include "Water\Ice.h"
#include "Environment\Environment.h"
#include "UnitAttribute.h"
#include "Physics\RigidBodyUnit.h"
#include "Water\SkyObject.h"
#include "Console.h"
#include "RenderObjects.h"
#include "DebugPrm.h"
#include "Terra\vMap.h"
#include "Render\src\Scene.h"

EffectController::EffectController(const BaseUniverseObject* owner, const Vect2f delta) : effectAttribute_(0),
	effect_(0),
	positionDelta_(delta),
	owner_(owner),
	effectPose_(Se3f::ID)
{
	placementMode_ = PLACEMENT_NONE;
	lastSwitchOffState_ = true;
}

EffectController::~EffectController()
{
	// перед уничтожением надо вызывать release()
}

void EffectController::release()
{
	effectStop();
	lastSwitchOffState_ = true;
}

Vect3f EffectController::position() const
{
	Vect3f delta = Vect3f(positionDelta_);

	Se3f owner_pos = owner()->pose();

	if(effectAttribute_->bindOrientation())
		owner_pos.rot().xform(delta);

	Vect3f pos = owner_pos.trans() + delta;
	
	if(placementMode_ != PLACEMENT_NONE){
		pos.z = vMap.getApproxAlt(pos.xi(), pos.yi());
		if(placementMode_ == PLACEMENT_WATER || effectAttribute_->onWaterSurface())
			pos.z = max(environment->water()->GetZ(pos.xi(), pos.yi()), pos.z);
	}

	return pos;
}

void EffectController::initEffect(cEffect* eff)
{
	if(effectAttribute_->ignoreFogOfWar())
		eff->SetUseFogOfWar(false);

	// HINT: до следующего кванта с этим графическим объектом можно работать из логики, если не так - переспросите у Балмера
	eff->setCycled(effectAttribute_->isCycled());
	eff->SetAutoDeleteAfterLife(false);
	// HINT: эффект может создаться выключенным, включается при необходимости на общих основаниях
	eff->SetParticleRate(.0f);

	if(effectAttribute_->ignoreDistanceCheck())
		eff->toggleDistanceCheck(false);

	Vect3f v = position();
	Se3f pos = (effectAttribute_->bindOrientation()) ? Se3f(owner()->orientation(), v) : Se3f(QuatF::ID, v);

	eff->SetPosition(MatXf(pos));

	// так как functor влияет только на тип эмиттера "Поверхность", а тот всегда
	// должен выводиться на поверхности воды, то всегда эффекту передатся functor воды
	eff->SetFunctorGetZ(environment->water()->GetFunctorZ());

	effect_ = eff;
}

bool EffectController::createEffect(EffectKey* key)
{
	xassert(key && owner_);

	cEffect* eff = terScene->CreateEffectDetached(*key, 0);
	if(!eff)
		return false;

	initEffect(eff);

	// HINT: для включения эффекта
	if(logicQuant())
		eff->SetParticleRate(1.0f);

	attachSmart(eff);
	return true;
}

bool EffectController::checkEnvironment(float radius)
{
	// можно вызывать из графического кванта
	
	// для юнитов проверяется по RigidBody - изменения в логику вносить параллельно!

	if(!isEnabled())
		return true;
	
	bool switchedOff = false;

	if(effectAttribute_->switchOffByDay() && environment->isDay())
		switchedOff = true;
	
	Vect3f pos = position();
	// карта льда грубее, чем карта воды, но лед может быть только над водой, поэтому при
	// попадании на лед еще проверяем и попадание в воду
	bool onIce = false;
	if(!switchedOff && environment->temperature()){
		onIce = (environment->temperature()->isOnIce(pos, radius) && environment->water()->isWater(pos, radius)) ? true : false;
		if((onIce && effectAttribute_->switchOffOnIce()) || (!onIce && effectAttribute_->switchOnOnIce()))
				switchedOff = true;
	}

	// если эффект поставлен на поверхность воды, то под водой он оказаться не может
	// если эффект на льду, то это не вода
	if(!switchedOff && !onIce && !effectAttribute_->onWaterSurface() &&
		(!water->isLava() && effectAttribute_->switchOffUnderWater() ||
		water->isLava() && effectAttribute_->switchOffUnderLava()))
				if(environment->water()->isUnderWater(pos, radius))
					switchedOff = true;
	
	return switchedOff;
}

bool EffectController::logicQuant(float radius)
{
	bool switchedOff = checkEnvironment(radius);

	if(isEnabled()){
		xassert(owner_);
		Se3f pose = Se3f(effectAttribute_->bindOrientation() ? owner()->orientation() : QuatF::ID, position());
		streamLogicInterpolator.set(fSe3fInterpolation, effect_) << effectPose_ << pose;
		effectPose_ = pose;
	}

	if(switchedOff != lastSwitchOffState_){
		MTL();
		xassert(effect_);
		streamLogicCommand.set(fCommandSetParticleRate, effect_) << (switchedOff ? 0.0f : 1.0f);
		lastSwitchOffState_ = switchedOff;
	}

	return !switchedOff;
}

bool EffectController::effectStart(const EffectAttribute* attribute, float scale, Color4c skin_color)
{
	release();
	
	if(!attribute || attribute->isEmpty())
		return false;

	effectAttribute_ = attribute;

	effectPose_ = owner()->pose();
	
	if(EffectKey* key = effectAttribute_->effect(effectAttribute_->scale() * scale, skin_color))
		return createEffect(key);
	
	return false;
}

void EffectController::moveToTime(int time) 
{ 
	if(isEnabled() && (time > 0))
		effect_->MoveToTime(time / 1000.0f); 
}

int EffectController::getTime() const
{
	if(isEnabled())
		return effect_->GetTime() * 1000; 
	return 0;
}

void EffectController::effectStop()
{
	if(effect_){
		if(isUnderEditor()){
			if(effectAttribute_->stopImmediately())
				effect_->Release();
			else{
				effect_->setCycled(false);
				effect_->SetAutoDeleteAfterLife(true);
			}
		}
		else{
			MTL();			
			if(effectAttribute_->stopImmediately())
				streamLogicPostCommand.set(fCommandRelease, effect_);
			else{
				streamLogicPostCommand.set(fCommandSetCycle, effect_) << false;
				streamLogicPostCommand.set(fCommandSetAutoDeleteAfterLife, effect_) << true;
			}
		}
		effect_ = 0;
	}
	effectAttribute_ = 0;
}

void EffectController::showDebugInfo() const
{
	if(!isEnabled())
		return;
	
	if(showDebugEffects.showName)
		show_text(owner()->position(), effectAttribute_->effectReference().c_str(), Color4c::BLUE);
	
	if(showDebugEffects.axis){
		MatXf X = effect_->GetPosition();

		Vect3f delta = X.rot().xcol();
		delta.normalize(15);
		show_vector(X.trans(), delta, Color4c::RED);

		delta = X.rot().ycol();
		delta.normalize(15);
		show_vector(X.trans(), delta, Color4c::GREEN);

		delta = X.rot().zcol();
		delta.normalize(15);
		show_vector(X.trans(), delta, Color4c::BLUE);
	}
}

// ------------------- UnitEffectController

const UnitBase* UnitEffectController::owner() const{
	return safe_cast<const UnitBase*>(EffectController::owner());
}

bool UnitEffectController::createEffect(EffectKey* key)
{
	MTL();
	xassert(key && owner());

	cEffect* eff = terScene->CreateEffectDetached(*key, owner()->get3dx());
	if(!eff)
		return false;

	initEffect(eff);

	int inode = 0;

	if(effectNode_ == -1)
		inode = max(attr()->node(), 0);
	else
		inode = effectNode_;

	effect_->LinkToNode(owner()->get3dx(), inode);

	// HINT: для включения эффекта
	if(logicQuant())
		eff->SetParticleRate(1.0f);

	attachSmart(eff);
	return true;
}

bool UnitEffectController::logicQuant(float radius)
{
	MTL();
	// для юнитов всё проверяем по rigidBody, если rigidBody отсутствует - проверяем по сеткам
	RigidBodyBase* rb = owner()->rigidBody();
	if(!rb)
		return __super::logicQuant(radius);

	if(!isEnabled())
		return true;
	
	bool switchedOff = paused_;
	
	if(effectAttribute_->switchOffByDay() && environment->isDay())
		switchedOff = true;
	
	bool onIce = rb->isUnit() && safe_cast<RigidBodyUnit*>(rb)->onIce();
	if(!switchedOff && (onIce && effectAttribute_->switchOffOnIce()) || (!onIce && effectAttribute_->switchOnOnIce()))
		switchedOff = true;

	// если эффект поставлен на поверхность воды, то под водой он оказаться не может
	// если юнит на льду, то это не вода
	if(!switchedOff && !onIce && !effectAttribute_->onWaterSurface() &&
		(!water->isLava() && effectAttribute_->switchOffUnderWater() ||
		water->isLava() && effectAttribute_->switchOffUnderLava()))
			switchedOff = rb->onWater() || rb->onLowWater();


	// если юнит скрыт (транспорт, телепорт и т.д.), то эффекты тоже скрывать
	if(!switchedOff &&
		(owner()->attr().isObjective() && (safe_cast<const UnitReal*>(owner())->hiddenLogic())  
		|| owner()->attr().isActing() && (!effectAttribute_->ignoreInvisibility() && safe_cast<const UnitActing*>(owner())->isInvisible()))) 
			switchedOff = true;

	if(switchedOff != lastSwitchOffState_)
	{
		streamLogicCommand.set(fCommandSetParticleRate, effect_) << (switchedOff ? 0.0f : 1.0f);
		lastSwitchOffState_ = switchedOff;
	}

	return !switchedOff;
}

//========================== GraphEffectController ==============================

void GraphEffectController::effectStop()
{
	if(effect_){
		MTG();
		if(effectAttribute_->stopImmediately())
			effect_->Release();
		else{
			effect_->setCycled(false);
			effect_->SetAutoDeleteAfterLife(true);
		}
		effect_ = 0;
	}
	effectAttribute_ = 0;
}

bool GraphEffectController::logicQuant(float radius){
	bool switchedOff = __super::checkEnvironment(radius);
	
	if(switchedOff != lastSwitchOffState_){
		xassert(effect_);
		effect_->SetParticleRate(switchedOff ? 0.0f : 1.0f);
		lastSwitchOffState_ = switchedOff;
	}

	return !switchedOff;
}

void GraphEffectController::updatePosition()
{
	MTG();
	if(isEnabled()){
		xassert(owner());
		Vect3f v = position();
		Se3f pos = (effectAttribute_->bindOrientation()) ? Se3f(owner()->orientation(), v) : Se3f(QuatF::ID, v);
		effect_->SetPosition(MatXf(pos));
	}
}

// ------------------- SoundController

SoundController::SoundController() :
	sound_(0),
	soundAttribute_(0),
	owner_(0),
	pose_(Vect3f::ZERO)
{
}

SoundController::~SoundController()
{
	// перед уничтожением надо вызывать release()
}

void SoundController::release()
{
	stop();
	delete sound_;
	sound_ = 0;
	soundAttribute_ = 0;
}

bool SoundController::isPlaying() const
{
	return isInited() ? sound_->IsPlayed() : false;
}

bool SoundController::init(const SoundAttribute* soundAttr, const BaseUniverseObject* owner)
{
	extern bool terSoundEnable;
	if(!terSoundEnable)
		return false;
	
	bool jast_playing = false;
	if(isInited())
		jast_playing = sound_->IsPlayed();

	if(!soundAttr /*|| !soundAttr->is3D()*/){
		stop();
		if(sound_)
			sound_->Destroy();
		soundAttribute_ = 0;
		return false;
	}

	if(static_cast<const Sound3DAttribute*>(soundAttr) == soundAttribute_ && owner == owner_){
		if(owner_)
			setPosition(owner_->position());
		return true;
	}

	if(jast_playing)
		stop();

	if(!sound_)
		sound_ = new SNDSound;

	if(!sound_->Init(soundAttr)){
		XBuffer buf;
		buf < "sound_->Init(Sound.GetSoundName())\nSoundName = " < soundAttr->GetPlaySoundName();
		kdWarning("&Sound",buf);
		//xxassert(false, buf);
		delete sound_;
		sound_ = 0;
		soundAttribute_ = 0;
		return false;
	}
	
	soundAttribute_ = static_cast<const Sound3DAttribute*>(soundAttr);
	owner_ = owner;
	if(owner_)
		setPosition(owner_->position());

	if(jast_playing)
		start();
	
	return true;
}

bool SoundController::setVolume(float volume)
{
	if(!isInited())
		return false;

	if(volume > FLT_EPS){
		sound_->SetVolume(volume);
	}else{
		sound_->SetVolume(0.f);
	}
	return true;
}

void SoundController::start()
{
	if(!isInited())
		return;
	
	sound_->SetPos(position());

	if(soundAttribute_->cycled())
		sound_->Play(true);
	else if(owner_)
		sound_->Play(false);
}

void SoundController::stop(bool immediately)
{
	if(isInited())
		if(soundAttribute_->cycled())
			sound_->Stop();
		else if(immediately) // если не зациклен то можно дать доиграть
			sound_->Stop(immediately);
}

bool SoundController::quant()
{
	if(!isInited())
		return false;

	if(owner_)
		setPosition(owner_->position());
	else
		return false;

	return sound_->IsPlayed();
}

void SoundController::setPosition(const Vect3f& pos)
{
	if(isInited()){
		if(!pose_.eq(pos, minDistance)){
			pose_ = pos;	
			sound_->SetPos(pos);
		}
	}
}
