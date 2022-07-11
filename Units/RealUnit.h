#ifndef __REALUNIT_H__
#define __REALUNIT_H__

#include "UnitInterface.h"
#include "Interpolation.h"
#include "Inventory.h"
#include "XTL\SafeCast.h"
#include "Weapon.h"
#include "StateBase.h"
#include "Physics\RigidBodyPrm.h"
#include "Physics\RigidBodyUnit.h"

class UnitReal;
class UnitLegionary;
class UnitSquad;
class WeaponTarget;

typedef vector<UnitLink<UnitLegionary> > LegionariesLinks;

class DockingController
{
public:
	DockingController();

	void set(UnitActing* dock, int nodeIndex);
	void setDock(UnitActing* dock) { dock_ = dock; }
	void clear();
	bool attached() const { return dock_ != 0; }
	const Se3f& pose() const;
	UnitActing* dock() const { return dock_; }
	void serialize(Archive& ar);
	void showDebug();

private:
	UnitLink<UnitActing> dock_;
	int tileIndex_;
};

//--------------------------------
// Очередь производства
//--------------------------------
enum ProduceType {
	PRODUCE_INVALID,
	PRODUCE_RESOURCE,
	PRODUCE_UNIT,
};

class ProduceItem {
public:
	ProduceType type_;
	int data_;
	
	ProduceItem(ProduceType type = PRODUCE_INVALID, int data = 0): type_(type), data_(data) {}
	
	void serialize(Archive& ar) {
		ar.serialize(type_, "type", 0);
		ar.serialize(data_, "data", 0);
	}
};

typedef vector<ProduceItem> ProducedQueue;

//--------------------------------
class UnitReal : public UnitInterface
{
public:
	// Причины остановки анимации.
	enum AnimationStopReason {
		ANIMATION_FROZEN = 1,
		ANIMATION_DISCONNECTED = 2
	};

	enum UnitState {
		AUTO_MODE, // У юнита нет внешних команд к исполнению. Автоматический режим.
		ATTACK_MODE, // Выполняет внешние команды. Атака.
		MOVE_MODE, // Выполняет внешние команды. Движение.
		TRIGGER_MODE // При включении анимации из триггера
 	};

	UnitReal(const UnitTemplate& data);
	~UnitReal();

	void serialize(Archive& ar);

	void Kill();

	void MoveQuant();
	void Quant();
	
	void dayQuant(bool invisible);

	void setPose(const Se3f& pose, bool initPose);
	void setRadius(float radius){}

	virtual void setCorpse();
	virtual bool corpseQuant();
	void killCorpse();

	bool canPlaiceInOnePoint();

	UnitReal* getUnitReal() { return this; }
	const UnitReal* getUnitReal() const { return this; }

	const char* label() const { return label_.c_str(); }
	void setLabel(const char* label) { label_ = label; }

	// Графическая модель Object3dx;
	cObject3dx* model() const { return model_; }
	c3dx* get3dx() const { return model_; }

	virtual void setModel(const char* name);
	void setModel(cObject3dx* model);

	const UI_ShowModeSprite* getSelectSprite() const;
	
	virtual int modelNodeIndex(const char* node_name) const;
	virtual void setModelNodeTransform(int node_index, const Se3f& pos);
	virtual void updateSkinColor();

	void setChain(ChainID chainID, int counter = -1, WeaponAnimationType weapon = WeaponAnimationType()); 
	void setChainWithTimer(ChainID chainID, int chainTime, int counter = -1);
	const AnimationChain* findChain(ChainID chainID, MovementState mstate, int counter, WeaponAnimationType weapon);
	void setAdditionalChain(ChainID chainID, int counter = -1, WeaponAnimationType weapon = WeaponAnimationType());
	void setAdditionalChainWithTimer(ChainID chainID, int chainTime, int counter = -1);
	void setAdditionalChainByFactor(ChainID chainID, float factor);
	void setAdditionalChainByHealth(ChainID chainID);
	void setChainByFactor(ChainID chainID, float factor);
	void setChainByHealth(ChainID chainID);
	bool setChainByHealthExcludeGroups(ChainID chainID, vector<bool>& animationGroups);
	void setChainByHealthWithTimer(ChainID chainID, int chainTime);
	void setChainByHealthTransition(MovementState stateFrom, MovementState stateTo, int chainTime);
	void setChain(const AnimationChain* chain, MovementState mstate, bool additionalChain = false);
	void startChainTimer(int chainTime) {	chainDelayTimer.start(chainTime); }
	void stopChains();
	void stopCurrentChain();
	void stopAdditionalChain(ChainID chainID, int counter = -1);
	const AnimationChain* getChain(int animationGroup = 0) const; 
	bool isChainFinished(int animationGroup = 0) const;
	bool animationChainEffectMode();

	// Логическая модель
	cObject3dx* modelLogic() const { return modelLogic_; }
	void setModelLogic(const char* name);
	
	int modelLogicNodeIndex(const char* node_name) const;
	const Se3f& modelLogicNodePosition(int node_index) const;
	bool modelLogicNodeOffset(int node_index, Mats& offset, int& parent_node) const;
	void setModelLogicNodeTransform(int node_index, const Se3f& pos);
	bool isModelLogicNodeVisible(int node_index) const;
	
	void interpolationQuant();

	virtual bool isConnected() const { return true; }
	virtual bool isUpgrading() const { return false; }
	virtual int executedUpgradeIndex() const { return -1; }
//--------------------
	void showEditor();
	void showDebugInfo();

	// Docking for Child
	void attachToDock(UnitActing* dock, int nodeIndex, bool initPose, bool useGripNode = false);
	void detachFromDock();
	bool isDocked() const { return dockingController_.attached(); }
	UnitActing* dock() const { return dockingController_.dock(); }
	void updateDock(UnitActing* dock) { dockingController_.setDock(dock); }
	void updateDockPose();

	const Se3f& interpolatedPose() const { MTG(); return model()->GetPositionSe(); }// использовать только внутри graphQuant()
    	
	void explode();

	void changeUnitOwner(Player* player);

	virtual MovementState getMovementState() { return MovementState(); }
	
	virtual bool inTransport() const { return false; }
	virtual bool canFireInCurentState() const { return false; }

	// в кванте счетчик уменьшается на единицу, при frozeCounter_ > 0 юнит заморожен.
	void freezeAnimation(bool frozenAttack = false) { freezeAnimationCounter_ = 2; frozenAttack_ = frozenAttack; }
	void makeFrozen(bool frozenAttack = false) { frozenCounter_ = 2; frozenAttack_ = frozenAttack; }
	bool isFrozen() { return !frozenAttack_ && (frozenCounter_ > 0 || freezeAnimationCounter_ > 0); }
	bool isFrozenAttack() { return frozenAttack_ && (frozenCounter_ > 0 || freezeAnimationCounter_ > 0); }

	void disableAnimation(AnimationStopReason reason) { animationStopReason_ |= reason; }
	void enableAnimation(AnimationStopReason reason) { animationStopReason_ &= ~reason; }

	// Работа с автоматическими режимами.
	bool checkInPathTracking(const UnitBase* tracker) const;
	UnitState unitState() const { return unitState_; }
	void setUnitState(UnitState state) { unitState_ = state; }


	bool initPoseTimer() const { return initPoseTimer_.busy(); }

	bool isSuspendCommand(CommandID commandID);
	bool isSuspendCommandWork() { return unitState() != AUTO_MODE; }
	
	ChainControllers& chainControllers() { return chainControllers_; }

	///////////////////////////////////////////////////////

	bool changeState(StateBase* state);
	void switchToState(StateBase* state);
	bool finishState(StateBase* state);
	bool interuptState(StateBase* state);
	void startState(StateBase* state);
	ChainID currentState() const { return stateController_.currentState(); }
	void clearCurrentState() { stateController_.clearCurrentState(); }
	bool chainDelayTimerFinished() { return chainDelayTimer.time(); }
	bool chainDelayTimerWasStarted() { return chainDelayTimer.started(); }
	void finishChainDelayTimer() { chainDelayTimer.stop(); } 

	void setRiseFur(int time);

	float noiseRadius() const;

protected:
	
	void setFurPhase(float phase);


	// Таймер текущей анимационной цепочки
	LogicTimer chainDelayTimer;

	cObject3dx* model_;
	cObject3dx* modelLogic_;
	
	mutable bool modelLogicUpdated_;

	ChainControllers chainControllers_;
	const AnimationChain* mainChain_;

	InterpolationLogicTimer furTimer_;
	bool riseFur_;

	// Автоматическое поведение..
		
	UnitState unitState_;
	int freezeAnimationCounter_;
	int frozenCounter_;
	bool frozenAttack_;

	int corpseColisionTimer_;
	
	DockingController dockingController_;
	Se3f gripNodeRelativePose_;
	bool useGripNode_;

	StateController stateController_;

	int animationStopReason_;

private:
	void setRadiusInternal();

	string label_;

	LogicTimer initPoseTimer_;

	void CalcSilouetteHeuristic();
};

#endif //__REALUNIT_H__
