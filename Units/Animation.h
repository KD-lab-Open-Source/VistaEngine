#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "UnitAttribute.h"
#include "Interpolation.h"

class cObject3dx;
class UnitBase;
class Archive;
class ChainControllerFade;

class SND3DSound;

class PhaseController
{
public:
	PhaseController();
	xm_inline float phase() const { return phase_; }
	xm_inline bool finished() const { return finished_; }
	xm_inline float deltaPhase() const { return deltaPhase_; }
	
	void initPeriod(bool reversed,int period, bool randomPhase = false);
	void computeDeltaPhase(bool reversed,int period);
	void reversePhase();
	bool quant(bool reversed,bool cycled,bool sound_finished,float timeFactor=1.0f);
	void start(bool reversed);
	void stop();
	void serialize(Archive& ar);

protected:
	float phase_;
	float deltaPhase_;
	bool finished_;
};

class ChainController
{
public:
	ChainController();
	ChainController(int animationGroup);
	~ChainController();

	void quant(UnitBase* owner, cObject3dx* model, cObject3dx* modelLogic, float timeFactor = 1.0f); 
	void setChain(const AnimationChain& chain, UnitBase* owner, cObject3dx* model_, cObject3dx* modelLogic_, MovementState mstate, bool additionalChain = false);
	void continueChain(UnitBase* owner, cObject3dx* model, cObject3dx* modelLogic);
	bool stop(UnitBase* owner, bool interrupt);
	void clear(UnitBase* owner);
	int animationGroup() const { return animationGroup_; }

	xm_inline const AnimationChain* chain() const { return chain_; }
	xm_inline const AnimationChain* chainPrev() const { return chainPrev_; }
	xm_inline bool finished() const { return phase_.finished(); }
	xm_inline bool finishedPrev() const { return phasePrev_.finished(); }
	xm_inline float phase() const { return phase_.phase(); }
	xm_inline float phasePrev() const { return phasePrev_.phase(); }

	void serialize(Archive& ar);

private:
	PhaseController phase_;
	PhaseController phasePrev_;
	PhaseController interpolationPhase_;
	const AnimationChain* chain_;
	const AnimationChain* chainPrev_;
	const AnimationChain* chainInterrupted_;
	
	int animationGroup_;

	SoundController sound_;
	SoundMarkers soundMarkers_;

	InterpolatorPhase interpolator_phase;
	InterpolatorPhaseFade interpolator_phase_prev;
		
	void startEffects(UnitBase* owner);
	void stopEffects(UnitBase* owner);
	void restartEffects(UnitBase* owner);
	void updateSoundMarkers(float phase, float deltaPhase, int surfKind, UnitBase* owner);

	int currentSurface_;
	const SoundAttribute* currentSound_;
	bool prevIsInited_;
	bool soundStarted_;
};

class ChainControllers
{
public:
	ChainControllers(UnitBase* owner);
	~ChainControllers();

	void setModel(cObject3dx* modelIn);
	void setModelLogic(cObject3dx* modelLogicIn);
	void copyChainControllers(ChainControllers& chainControllers);
	void quant(float timeFactor = 1.0f);
	void setChain(const AnimationChain* chain, MovementState mstate, bool additionalChain = false);
	bool setChainExcludeGroups(const AnimationChain* chain, vector<bool>& animationGroups, MovementState mstate);
	void stop();
	void clear();
	void stopChain(const AnimationChain* chain);
	int size() const { return chainControllers_.size(); }
	const AnimationChain* getChain(int animationGroup) const { return chainControllers_[animationGroup].chain(); }
	bool isChainFinished(int animationGroup) const { return chainControllers_[animationGroup].finished(); }

	void showDebugInfo();
	
	vector<ChainController>& chainControllers() { return chainControllers_; }

    void serialize(Archive& ar);
	
protected:
	typedef vector<ChainController> VChainControllers;
	VChainControllers chainControllers_;
	
	UnitBase* owner_;
	cObject3dx* model_;
	cObject3dx* modelLogic_;
};


#endif //__ANIMATION_H__
