#ifndef __RIGID_BODY_UNIT_H__
#define __RIGID_BODY_UNIT_H__

#include "RigidBodyBox.h"
#include "Timers.h"
#include "RigidBodyCarPrm.h"
#include "UnitAttribute.h"

class WhellController;
class RigidBodyCar;

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnitBase
//
///////////////////////////////////////////////////////////////

class RigidBodyUnitBase : public RigidBodyBox
{
public:
	RigidBodyUnitBase();
	~RigidBodyUnitBase();
	void build(const RigidBodyPrm& prm, const Vect3f& center, const Vect3f& extent, float mass);
	void initPose(const Se3f& pose);
	void setPose(const Se3f& pose);
	bool evolve(float dt);
	void attach();
	void detach();
	bool unmovable() const { return unmovable_; }
	void makeStatic() { if(!unmovable()) debugMessage("makeStatic"); unmovable_ = 1; }
	void makeDynamic() { if(unmovable()) debugMessage("makeDynamic"); unmovable_ = 0; }
	void setImpassability(int impassability) { impassability_ = impassability; }
	int impassability() const { return impassability_; }
	void setFieldFlag(int fieldFlag) { fieldFlag_ = fieldFlag; } 
	int fieldFlag() const { return fieldFlag_; }
	void setEnvironmentDestruction(int environmentDestruction) { environmentDestruction_ = environmentDestruction; }
	int environmentDestruction() const { return environmentDestruction_; }
	int calcPassabilityFlag() const;
	bool checkImpassability(const Vect2f& pos) const;
	virtual void enableBoxMode();
	virtual void disableBoxMode();
	bool isBoxMode() const { return isBoxMode_; }
	void setAnimatedRise(bool animatedRise) { animatedRise_ = animatedRise; }
	void setSinkParameters(bool canSink, float moveSinkVelocity, float standSinkVelocity);
	bool isSinking() { return isSinking_; }
	void startSinking(float level);
	void setFlyingModeEnabled(bool mode, int time = 0);
	void setFlyingMode(bool mode);
	bool flyingModeEnabled() const { return flyingModeEnabled_; }
	bool flyingMode() const { return flyingModeEnabled_ && flyingMode_; }
	void setFlyingHeight(float flyingHeight) { flyingHeight_ = prm().undergroundMode ? -flyingHeight : flyingHeight; }
	void setFlyingHeightAndDelta(float flyingHeight, float delta, float period);
	virtual float flyingHeight() const { return flyingHeight_; }
	void setFlyingHeightCurrent(float flyingHeightCurrent) { flyingHeightCurrent_ = flyingHeightCurrent; }
	float flyingHeightCurrent() const { return flyingHeightCurrent_; }
	float flyingHeightCurrentWithDelta() const { return flyingHeightCurrent_ * flyingHeightDelta_; }
	void computeFlyingHeightDelta(float dt);
	void enableFlyDownModeByTime(int time);
	void updateFlyDownSpeed();
	void enableFlyDownMode() { enableFlyDownModeByTime(flyDownTime_); }
	void disableFlyDownMode() { flyDownMode_ = false; setFlyingHeightCurrent(0.0f); }
	void setFlyDownTime(int time) { flyDownTime_ = time; }
	bool flyDownMode() { return flyDownMode_; }
	float forwardVelocity() const { return forwardVelocity_; }
	void setForwardVelocity(float v){ forwardVelocity_ = v; }
	bool onDeepWater() const { return onDeepWater_; }
	void resetDeepWaterChanged() { deepWaterChanged_ = false; }
	bool deepWaterChanged() const { return deepWaterChanged_; }
	bool isUnderWaterSilouette() const { return isUnderWaterSilouette_; }
	void enableWaterAnalysis();
	void disableWaterAnalysis();
	void setWhellController(const WheelDescriptorList& wheelList, cObject3dx* model, cObject3dx* modelLogic);
	void setRealSuspension(cObject3dx* model, const RigidBodyCarPrm& carPrm);
	void serialize(Archive& ar);
	void show();
	void setFallFromHill(bool fallFromHill) { fallFromHill_ = fallFromHill; }
	void setAngleAndVelocity(bool moveback, float angleNew, float dirAngle, float velocityNew, float dt);
	float ptVelocity() { return ptVelocity_; }
	void computeUnitVelocity();
	const Vect3f& velocity();
	
	QuatF ptAdditionalRot_;

private:
	void setDeepWater(bool deepWater);
	float checkDeepWater(float soilZ, float waterZ);
	float analyzeAreaFast(const Vect2i& center, int r, Vect3f& normalNonNormalized);
	QuatF placeToGround(float& groundZNew);
	bool groundCheck(int xc, int yc, int r) const;

	float groundZ_;
	float flyingHeight_;
	float flyingHeightDelta_;
	float flyingHeightDeltaPhase_;
	float flyingHeightDeltaOmega_;
	float flyingHeightDeltaFactorMax_;
	float flyingHeightCurrent_;
	float waterLevelPoint_;
	float ptVelocity_;
	float forwardVelocity_;
	float moveSinkVelocity_;
	float standSinkVelocity_;
    int flyDownTime_;
	int impassability_;
	int fieldFlag_;
	int environmentDestruction_;
	QuatF groundRot_;
	float flyDownSpeed_;
	bool unmovable_;
	bool isBoxMode_;
	bool flyingModeEnabled_;
	bool flyingMode_;
	bool flyDownMode_;
	bool onDeepWater_;
	bool deepWaterChanged_;
	bool isUnderWaterSilouette_;
	bool fallFromHill_;
	bool animatedRise_;
	bool canSink_;
	bool isSinking_;
	bool isSurfacing_;
	Vect3f prevVelocity_;
	WhellController* whellController_;
	RigidBodyCar* car;
};

///////////////////////////////////////////////////////////////
//
//    class RigidBodyUnit
//
///////////////////////////////////////////////////////////////

class RigidBodyUnit : public RigidBodyUnitBase
{
public:
	RigidBodyUnit()
		: downUnit_(0)
	{
	}
	float getDownUnitHeight() const { return downUnit() ? downUnit()->flyingHeightCurrentWithDelta() + 2.1f * downUnit()->extent().z : 0.0f; }
	bool isInList(const RigidBodyUnit* unit) const { return downUnit() && ((downUnit() == unit) || downUnit()->isInList(unit)); }

	void setDownUnit(UnitActing* unit);
	void clearDownUnit();
	const RigidBodyUnit* downUnit() const;
	float flyingHeight() const { return __super::flyingHeight() + getDownUnitHeight(); }

private:
	UnitLink<UnitActing> downUnit_;
};

#endif // __RIGID_BODY_UNIT_H__
