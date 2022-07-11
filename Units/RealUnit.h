#ifndef __REALUNIT_H__
#define __REALUNIT_H__

#include "UnitInterface.h"
#include "Interpolation.h"
#include "Inventory.h"
#include "SafeCast.h"
#include "Weapon.h"
#include "ShowCircleController.h"
#include "StateBase.h"
#include "..\Physics\RigidBodyPrm.h"
#include "..\Physics\RigidBodyUnit.h"

class UnitReal;
class UnitLegionary;
class UnitSquad;
class AttributeCache;
class WeaponTarget;
class WhellController;

typedef vector<UnitLink<UnitLegionary> > LegionariesLinks;

typedef vector<pair<int, ShowCircleController> > ShowCircleControllers;

class DockingController
{
public:
	DockingController();

	void set(UnitActing* dock, int nodeIndex);
	void setDock(UnitActing* dock) { dock_ = dock; }
	void clear();
	bool attached() const { return dock_; }
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
	// Причины статичности
	enum StaticReason {
		STATIC_DUE_TO_ATTACK = 1,
		STATIC_DUE_TO_PRODUCTION = 2,
		STATIC_DUE_TO_UPGRADE = 4,
		STATIC_DUE_TO_SITTING_IN_TRANSPORT = 8,
		STATIC_DUE_TO_TRANSPORT_REQUIREMENT = 16,
		STATIC_DUE_TO_TRANSPORT_LOADING = 32,
		STATIC_DUE_TO_ANIMATION = 64,
		STATIC_DUE_TO_BUILDING = 128,
		STATIC_DUE_TO_WEAPON = 256,
		STATIC_DUE_TO_RISE = 512,
		STATIC_DUE_TO_FROZEN = 1024,
		STATIC_DUE_TO_GIVE_RESOURCE = 1 << 11,
		STATIC_DUE_TO_TELEPORTATION = 1 << 12,
		STATIC_DUE_TO_TRANSITION = 1 << 13,
		STATIC_DUE_TO_OPEN_FOR_LOADING = 1 << 14,
		STATIC_DUE_TO_TRIGGER = 1 << 15,
		STATIC_DUE_TO_DEATH = 1 << 16
	};

	// Причины выключения полета
	enum FlyingReason {
		FLYING_DUE_TO_TRANSPORT_LOAD = 1,
		FLYING_DUE_TO_TRANSPORT_UNLOAD = 2,
		FLYING_DUE_TO_UPGRADE = 4
	};

	// Причины остановки анимации.
	enum AnimationStopReason {
		ANIMATION_FROZEN = 1
	};

	enum UnitState {
		AUTO_MODE, // У юнита нет внешних команд к исполнению. Автоматический режим.
		ATTACK_MODE, // Выполняет внешние команды. Атака.
		MOVE_MODE, // Выполняет внешние команды. Движение.
		TRIGGER_MODE // При включении анимации из триггера
 	};

	enum HideReason {
		HIDE_BY_FOW = 1,
		HIDE_BY_TELEPORT = 2,
		HIDE_BY_TRANSPORT = 4,
		HIDE_BY_UPGRADE = 8,
		HIDE_BY_TRIGGER = 16,
		HIDE_BY_INITIAL_APPEARANCE = 32,
		HIDE_BY_PLACED_BUILDING = 64,
		HIDE_BY_EDITOR = 128
	};

	UnitReal(const UnitTemplate& data);
	~UnitReal();

	void serialize(Archive& ar);

	void Kill();

	void MoveQuant();
	void Quant();
	
	void dayQuant(bool invisible);

	void executeCommand(const UnitCommand& command);

	void setPose(const Se3f& pose, bool initPose);
	void setRadius(float radius);

	void setCorpse();
	bool corpseQuant();
	void killCorpse();

	virtual void WayPointController();
	UnitReal* getUnitReal() { return this; }
	const UnitReal* getUnitReal() const { return this; }

	const char* label() const { return label_.c_str(); }
	void setLabel(const char* label) { label_ = label; }

	// Графическая модель Object3dx;
	cObject3dx* model() const { return model_; }
	c3dx* get3dx() const { return model_; }

	virtual void setModel(const char* name);
	void setModel(cObject3dx* model);
	
	virtual int modelNodeIndex(const char* node_name) const;
	virtual void setModelNodeTransform(int node_index, const Se3f& pos);
	virtual void updateSkinColor();

	const AnimationChain* setChain(ChainID chainID, int counter = -1); 
	bool setChainExcludeGroups(ChainID chainID, int counter, vector<int>& animationGroups);
	const AnimationChain* setChainWithTimer(ChainID chainID, int chainTime, int counter = -1);
	const AnimationChain* setAdditionalChain(ChainID chainID, int counter = -1);
	const AnimationChain* setAdditionalChainWithTimer(ChainID chainID, int chainTime, int counter = -1);
	void setAdditionalChainByHealth(ChainID chainID);
	void setChainByFactor(ChainID chainID, float factor);
	void setChainByHealth(ChainID chainID);
	bool setChainByFactorExcludeGroups(ChainID chainID, float factor, vector<int>& animationGroups);
	void setChainByHealthWithTimer(ChainID chainID, int chainTime);
	void setChainTransition(MovementState stateFrom, MovementState stateTo, int chainTime);
	void setChain(const AnimationChain* chain);
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
	void refreshAttribute();

	void showEditor();
	void showDebugInfo();

	// Docking for Child
	void attachToDock(UnitActing* dock, int nodeIndex, bool initPose, bool useGripNode = false);
	void detachFromDock();
	bool isDocked() const { return dockingController_.attached(); }
	UnitActing* dock() const { return dockingController_.dock(); }
	void updateDock(UnitActing* dock) { dockingController_.setDock(dock); }
	void updateDockPose();

	void moveToPoint(const Vect3f& v);
	float getPathFinderDistance(const UnitBase* unit_) const;

	Vect3fList& wayPoints() { return wayPoints_; }
	const Vect3fList& wayPoints() const { return wayPoints_; }
	const Vect3f& getManualUnitPosition() const {return manualUnitPosition; }

	virtual bool addWayPoint(const Vect3f& p);
	void setWayPoint(const Vect3f& p);

	void addWayPoint(const Vect2f& p) { addWayPoint(Vect3f(p.x, p.y, 0)); }
	void setWayPoint(const Vect2f& p) { setWayPoint(Vect3f(p.x, p.y, 0)); }

	void goToRun();
	void stopToRun();

	bool runMode() const { return manualRunMode; }

	void stop();

	bool isMoving() const;
	
	void graphQuant(float dt);
	
	void drawCircle(int id, const Vect3f pos, float radius, const CircleEffect& circle_attr);
	const Se3f& interpolatedPose() const { MTG(); return model()->GetPositionSe(); }// использовать только внутри graphQuant()
    	
	void explode();

	void changeUnitOwner(Player* player);

	MovementState getMovementState();
	
	virtual bool inTransport() const { return false; }
	virtual bool canFireInCurentState() const { return false; }

	bool nearUnit(const UnitBase* unit, float radiusMin = 16.f);
	bool rot2Point(const Vect3f& point);

	int getStaticReason() const { return staticReason_; }
	int getStaticReasonXY() const { return staticReasonXY_; }
	void disableFlying(int flyingReason, int time = 0);
	void enableFlying(int flyingReason, int time = 0);
	void makeStatic(int staticReason);
	void makeStaticXY(int staticReason);
	void makeDynamic(int staticReason);
	void makeDynamicXY(int staticReason);

	// в кванте счетчик уменьшается на единицу, при frozeCounter_ > 0 юнит заморожен.
	void freezeAnimation() { freezeAnimationCounter_ = 2; }
	void makeFrozen() { frozenCounter_ = 2; }
	bool isFrozen() { return frozenCounter_ > 0 || freezeAnimationCounter_ > 0; }

	void disableAnimation(AnimationStopReason reason) { animationStopReason_ |= reason; }
	void enableAnimation(AnimationStopReason reason) { animationStopReason_ &= ~reason; }

	// Работа с автоматическими режимами.
	bool checkInPathTracking(const UnitBase* tracker) const;
	UnitState getUnitState() const { return unitState; }
	void setUnitState(UnitState state) { unitState = state; }

	void hide(int reason, bool hide);
	int hiddenLogic() const { return hideReason_ &~ HIDE_BY_FOW; }
	int hiddenGraphic() const { return hideReason_; } // !!! Не повторяется вызывать только из графики

	bool initPoseTimer() const { return initPoseTimer_(); }

	bool isSuspendCommand(CommandID commandID);
	bool isSuspendCommandWork() { return unitState != AUTO_MODE; }
	
	RigidBodyUnit* rigidBody() const { return safe_cast<RigidBodyUnit*>(rigidBody_); }

	ChainControllers& chainControllers() { return chainControllers_; }

	///////////////////////////////////////////////////////

	bool changeState(StateBase* state);
	void switchToState(StateBase* state);
	bool finishState(StateBase* state);
	bool interuptState(StateBase* state);
	void startState(StateBase* state);
	ChainID currentState() const { return currentStateID_; }
	void clearCurrentState() { currentStateID_ = CHAIN_NONE; currentState_ = 0; }
	bool chainDelayTimerFinished() { return chainDelayTimer.was_started() && chainDelayTimer(); }
	void finishChainDelayTimer() { chainDelayTimer.stop(); } 

protected:
	
	void stateQuant();

	// Таймер текущей анимационной цепочки
	DelayTimer chainDelayTimer;

	const States* posibleStates_;
	StateBase* currentState_;
	ChainID currentStateID_;

	cObject3dx* model_;
	cObject3dx* modelLogic_;
	
	mutable bool modelLogicUpdated_;

	ChainControllers chainControllers_;
	const AnimationChain* mainChain_;

	// Автоматическое поведение..
	Vect3f manualUnitPosition;
	
	UnitState unitState;
	int freezeAnimationCounter_;
	int frozenCounter_;

	bool manualRunMode;
	
	bool isMoving_;

	float forwardVelocityFactor_;
	
	DockingController dockingController_;
	Se3f gripNodeRelativePose_;
	bool useGripNode_;

	// Операционные данные. желательно не менять напрямую.
	Vect3fList wayPoints_;
	
	WhellController* whellController;

private:
	DurationTimer recalcPathTimer_;
	bool pathFindSucceeded_;
	Vect2i pathFindTarget_;
	vector<Vect2i> pathFindList_;

	string label_;

	int flyingReason_;
	int staticReason_;
	int staticReasonXY_;
	int animationStopReason_;
	int hideReason_;
	
	DurationTimer initPoseTimer_;

	const AttributeCache* attributeCache_;

	StateBase* prevState_;
	StateBase* desiredState_;

	int rotationDeath;

	ShowCircleControllers showCircleControllers_;

	void CalcSilouetteHeuristic();
};

#endif //__REALUNIT_H__
