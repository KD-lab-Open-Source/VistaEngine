#include "stdafx.h"
#include "Universe.h"
#include "Animation.h"
#include "Serialization.h"
#include "..\HT\StreamInterpolation.h"
#include "BaseUnit.h"
#include "Sound.h"
#include "GlobalAttributes.h"

PhaseController::PhaseController():
phase_(0),
deltaPhase_(0),
finished_(true)
{
}

void PhaseController::initPeriod(bool reversed,int period, bool randomPhase)
{
	computeDeltaPhase(reversed, period);
	phase_ = reversed ? 1 : 0;
	if(randomPhase)
		phase_ = logicRNDfrand();
	finished_ = false;
}

void PhaseController::computeDeltaPhase(bool reversed,int period)
{
	deltaPhase_ = period ? logicTimePeriod/(float)period : 0;
	if(reversed)
		deltaPhase_ = -deltaPhase_;
}

void PhaseController::reversePhase() 
{ 
	finished_ = false; 
	deltaPhase_ = -deltaPhase_; 
}

void PhaseController::start(bool reversed) 
{ 
	if(finished_){
		phase_ = reversed ? 1 : 0;
		finished_ = false;
	}
}

void PhaseController::stop() 
{ 
	finished_ = true; 
}

bool PhaseController::quant(bool reversed,bool cycled,bool sound_finished,float timeFactor)
{
	phase_ += deltaPhase_ * timeFactor;
	if(!reversed){
		if(phase_ >= 1 || sound_finished){
			if(!cycled){
				phase_ = 1;
				finished_ = true;
			}
			else
			{
				if(phase_ >= 1)
					phase_ = cycle(phase_, 1.0f);
				else
					phase_ = 0;
				return true; // рестарт анимации true
			}
		}
	}
	else{
		if(phase_ <= 0 || sound_finished){
			if(!cycled){
				phase_ = 0;
				finished_ = true;
			}
			else
			{
				if(phase_ <= 0)
					phase_ = cycle(phase_, 1.0f);
				else
					phase_ = 1;
				return true; // рестарт анимации true
			}
		}
	}
	return false; // рестарт анимации false
}

void PhaseController::serialize(Archive& ar) 
{
	ar.serialize(phase_, "phase", 0);
	ar.serialize(finished_, "finished", 0);
}

ChainController::ChainController() :
chain_(0),
chainPrev_(0),
animationGroup_(0),
currentSurface_(0),
interpolated_(false),
prevIsInited_(false),
currentSound_(NULL),
soundStarted_(false)
{
}

ChainController::ChainController(int animationGroup) :
chain_(0),
chainPrev_(0),
animationGroup_(animationGroup),
currentSurface_(0),
interpolated_(false),
prevIsInited_(false),
currentSound_(NULL),
soundStarted_(false)
{
}

ChainController::~ChainController()
{
	sound_.release();
}

void ChainController::setChain(const AnimationChain& chain,UnitBase* owner_, cObject3dx* model_, cObject3dx* modelLogic_, bool additionalChain)
{
	if(chain_ && chain_->compare(chain) && (additionalChain || chain_->visibilityGroup() == chain.visibilityGroup())
		&& ((chain_->counter == chain.counter && chain_->cycled == chain.cycled) ||	(!finished() 
		&&	chain_->reversed ? phase() > phase_.deltaPhase() : phase() < 1 - phase_.deltaPhase())))
		return;

	stop(owner_);

	chain_ = &chain;
	
	if(chainPrev_ && chainPrev_->chainIndex() == chain_->chainIndex()){
		phase_ = phasePrev_;
		if(chain_->period != chainPrev_->period){
			phase_.computeDeltaPhase(chain_->reversed, chain_->period);
			phase_.start(chain_->reversed);
		}else{
			if(chain_->reversed != chainPrev_->reversed)
				phase_.reversePhase();
			else
				phase_.start(chain_->reversed);
		}

	}else{
		phase_.initPeriod(chain_->reversed, chain_->period, chain_->randomPhase);
		if(GlobalAttributes::instance().enableAnimationInterpolation){
			if(chainPrev_ && (additionalChain || chainPrev_->visibilityGroup() == chain_->visibilityGroup())){
				interpolator_phase_prev.initialize();
				interpolated_ = true;
				interpolator_phase_prev = Vect2f(phasePrev_.phase(), 1.0f);
				interpolationPhase_.initPeriod(true, GlobalAttributes::instance().animationInterpolationTime);
				streamLogicCommand.set(fChainInterpolationFade, model_) << chainPrev_->chainIndex() << animationGroup_ << phasePrev_.phase() << 1.0f;
				if(modelLogic_){
					modelLogic_->GetInterpolation()->SetAnimationGroupChain(animationGroup_, chainPrev_->chainIndex());
					modelLogic_->GetInterpolation()->SetAnimationGroupPhase(animationGroup_, phasePrev_.phase());
					modelLogic_->GetInterpolation()->SetAnimationGroupInterpolation(animationGroup_, 100.0f / GlobalAttributes::instance().animationInterpolationTime);
				}
			}else{
				interpolationPhase_.stop();
				if(modelLogic_){
					modelLogic_->GetInterpolation()->SetAnimationGroupPhase(animationGroup_, phasePrev_.phase());
					modelLogic_->GetInterpolation()->SetAnimationGroupInterpolation(animationGroup_, 0.0f);
				}
				interpolator_phase_prev = Vect2f(phasePrev_.phase(), 0.0f);
				interpolator_phase_prev(model_, animationGroup_);
			}
		}
	}
	
	interpolator_phase.initialize();
	interpolator_phase = phase();
	streamLogicCommand.set(fChainInterpolation) << model_ << chain_->chainIndex() << animationGroup_ << phase();
	if(!additionalChain)
		streamLogicCommand.set(fVisibilityGroupInterpolation, model_) << chain_->visibilityGroup();
	
	if(modelLogic_){
		if(!additionalChain)
			modelLogic_->SetVisibilityGroup(chain_->visibilityGroup());
		modelLogic_->SetAnimationGroupChain(animationGroup_, chain_->chainIndex());
		modelLogic_->SetAnimationGroupPhase(animationGroup_, phase());
	}

	startEffects(owner_);
	currentSurface_ = vMap.getSurKind(owner_->position().xi(),owner_->position().yi())+1;
	const SoundAttribute* snd = chain.soundReferences.getSound(currentSurface_);
	if(snd){
		currentSound_ = snd;
		sound_.init(snd, owner_);
		//sound_.start();
		soundStarted_ = false;
	}
	soundMarkers_ = chain.soundMarkers;
}

void ChainController::stop(UnitBase* owner)
{
	//xxassert(!chain_ || !prevIsInited_, "Повторное переключение состояния юнита");
	if(chain_ && !prevIsInited_){
		prevIsInited_ = true;
		chainPrev_ = chain_;
		phasePrev_ = phase_;
		stopEffects(owner);
	}
	sound_.stop();
	phase_.stop();
	chain_ = 0;
}

void ChainController::quant(UnitBase* owner_, cObject3dx* model_, cObject3dx* modelLogic_, float timeFactor)
{
	prevIsInited_ = false;

	bool sound_finished = false;
	if(sound_.isInited()){
		if(!soundStarted_){
			sound_.quant();
			sound_.start();
			soundStarted_ = true;
		}
		sound_finished = !sound_.quant();
	}
	
	if(finished())
		return;

	if(!chain_->syncBySound)
		sound_finished = false;

	if(phase_.quant(chain_->reversed, chain_->cycled, sound_finished, timeFactor)){
		restartEffects(owner_);
	}

	interpolator_phase = phase();
	interpolator_phase(model_, animationGroup_);

	if(modelLogic_)
		modelLogic_->SetAnimationGroupPhase(animationGroup_, phase());
	
	if(GlobalAttributes::instance().enableAnimationInterpolation && interpolated_){
		if(!interpolationPhase_.finished()){
			phasePrev_.quant(chainPrev_->reversed, chainPrev_->cycled, sound_finished, timeFactor);
			interpolationPhase_.quant(true, false, sound_finished, timeFactor);
		}else
			interpolated_ = false;
		if(modelLogic_){
			modelLogic_->GetInterpolation()->SetAnimationGroupPhase(animationGroup_, phasePrev_.phase());
			modelLogic_->GetInterpolation()->SetAnimationGroupInterpolation(animationGroup_, interpolationPhase_.phase());
		}
		interpolator_phase_prev = Vect2f(phasePrev_.phase(), interpolationPhase_.phase());
		interpolator_phase_prev(model_, animationGroup_);
	}

	int sur = vMap.getSurKind(owner_->position().xi(),owner_->position().yi())+1;
	if (chain_ && (currentSurface_ != sur))
	{
		currentSurface_ = sur;
		const SoundAttribute* snd = chain_->soundReferences.getSound(currentSurface_);
		if(snd&&currentSound_ != snd)
		{
			sound_.stop();
			currentSound_ = snd;
			sound_.init(snd, owner_);
			sound_.start();
		}
	}

	updateSoundMarkers(phase_.phase(),phase_.deltaPhase(), currentSurface_, owner_);
}

void ChainController::updateSoundMarkers(float phase, float deltaPhase, int surfKind, UnitBase* owner)
{
	if(deltaPhase < 0){
		phase = 1 - phase;
		deltaPhase = -deltaPhase;
	}

	SoundMarkers::iterator i;
	FOR_EACH(soundMarkers_, i){
		if(i->phase_ > phase - deltaPhase && i->phase_ < phase + deltaPhase){
			if(!i->active_){
				const SoundAttribute* attr = i->soundReferences_.getSound(surfKind);
				if(attr)
				{
					attr->play(owner ? owner->position() : Vect3f::ID);
				}
				i->active_ = true;
			}
		}
		else
			i->active_ = false;
	}
}

void ChainController::serialize(Archive& ar) 
{
	ar.serialize(phase_, "phase_", 0);
}

void ChainController::startEffects(UnitBase* owner)
{
	owner->updateEffects();
	EffectAttributes::const_iterator it;
	FOR_EACH(chain_->effects, it)
		if(!it->isEmpty())
			owner->startEffect(&(*it));
}

void ChainController::stopEffects(UnitBase* owner)
{
	EffectAttributes::const_iterator it;
	FOR_EACH(chain_->effects, it)
		if(!it->isEmpty())
			owner->stopEffect(&(*it));
}
void ChainController::restartEffects(UnitBase* owner)
{
	EffectAttributes::const_iterator it;
	FOR_EACH(chain_->effects, it)
		if(!it->isEmpty()&&it->isSynchronize())
		{
			owner->stopEffect(&(*it));
			owner->startEffect(&(*it));
		}
}

//////////////////////
ChainControllers::ChainControllers(UnitBase* owner)
:owner_(owner),model_(0),modelLogic_(0)
{
}

ChainControllers::~ChainControllers()
{
}

void ChainControllers::setModel(cObject3dx* model)
{
	model_ = model;

	int groups = model_->GetAnimationGroupNumber();
	chainControllers_.resize(groups);
	for(int i=0;i<groups;i++)
		chainControllers_[i]=ChainController(i);
}

void ChainControllers::setModelLogic(cObject3dx* modelLogic)
{
	modelLogic_ = modelLogic;
}

void ChainControllers::copyChainControllers(ChainControllers& chainControllers) 
{ 
	model_ = chainControllers.model_;
	chainControllers_.swap(chainControllers.chainControllers_); 
}

void ChainControllers::quant(float timeFactor)
{
	VChainControllers::iterator ci;
	FOR_EACH(chainControllers_, ci)
		ci->quant(owner_, model_, modelLogic_, timeFactor);
}

void ChainControllers::setChain(const AnimationChain* chain, bool additionalChain)
{
	if(chain && !chainControllers_.empty()){
		if(chain->animationGroup() >= 0)
			chainControllers_[chain->animationGroup()].setChain(*chain, owner_, model_, modelLogic_, additionalChain);
		else{
			VChainControllers::iterator ichain;
			FOR_EACH(chainControllers_, ichain)
				ichain->setChain(*chain, owner_, model_, modelLogic_);
		}
	}
}

bool ChainControllers::setChainExcludeGroups(const AnimationChain* chain, vector<int>& animationGroups)
{
	if(chain && !chainControllers_.empty() && chain->animationGroup() < 0){
		VChainControllers::iterator ichain;
		int i = 0;
		FOR_EACH(chainControllers_, ichain){
			vector<int>::iterator iag;
			bool find(false);
			FOR_EACH(animationGroups, iag)
				if(*iag == i)
					find = true;
			if(!find)
				ichain->setChain(*chain, owner_, model_, modelLogic_);
			i++;
		}
		return true;
	}
	return false;
}

void ChainControllers::stop()
{
	VChainControllers::iterator i;
	FOR_EACH(chainControllers_, i)
		i->stop(owner_);
}

void ChainControllers::stopChain(const AnimationChain* chain)
{
	if(!chainControllers_.empty())
		if(chain->animationGroup() >= 0)
			chainControllers_[chain->animationGroup()].stop(owner_);
		else
			stop();
}

void ChainControllers::showDebugInfo()
{
	XBuffer msg(256, 1);
	msg.SetDigits(2);
	VChainControllers::iterator ci;
	FOR_EACH(chainControllers_, ci){
		if(ci->chain()){
			
			msg < ci->chain()->name();
			msg < "(" < model_->GetChain(ci->chain()->chainIndex())->name.c_str();
			if(ci->finished())
				msg < "): finished\n";
			else 
				msg < "): " <= ci->phase() < "\n";
			if(ci->chainPrev()){
				msg < "(" < ci->chainPrev()->name();
				msg < "(" < model_->GetChain(ci->chainPrev()->chainIndex())->name.c_str();
				if(ci->finishedPrev())
					msg < "): finished)\n";
				else 
					msg < "): " <= ci->phasePrev() < ")\n";
			}
		}
		else 
			msg < "none\n";
	}
	show_text(owner_->position() + Vect3f(0, 0, owner_->height()/2), msg, BLUE);
}

void ChainControllers::serialize(Archive& ar) 
{
	ar.serialize(chainControllers_, "chainControllers", 0);
}