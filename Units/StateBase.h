#ifndef __STATE_BASE_H__
#define __STATE_BASE_H__

#include "UnitAttribute.h"

class UnitReal;

///////////////////////////////////////////////////////////////
//
//    class StateBase
//
///////////////////////////////////////////////////////////////

class StateBase
{
public:
	StateBase() : id_(CHAIN_NONE) {}
	StateBase(ChainID id) : id_(id) {}
	virtual void start(UnitReal* owner) const {}
	virtual void finish(UnitReal* owner, bool finished = true) const;
	virtual bool canStart(UnitReal* owner) const { return false; }
	virtual bool canFinish(UnitReal* owner) const { return false; }
	int priority() const { return id_ >> 4; }
	ChainID id() const { return id_; }
	static StateBase* instance() { return &Singleton<StateBase>::instance(); }

protected:
	bool firstStart(UnitReal* owner) const;

private:
	ChainID id_;
};

///////////////////////////////////////////////////////////////
//
//    class StateFinishedByTimer
//
///////////////////////////////////////////////////////////////

class StateFinishedByTimer : public StateBase
{
public:
	StateFinishedByTimer(ChainID id) : StateBase(id) {}
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
};

///////////////////////////////////////////////////////////////
//
//    class StateFall
//
///////////////////////////////////////////////////////////////

class StateFall : public StateBase
{
public:
	StateFall() : StateBase(CHAIN_FALL) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateFall* instance() { return &Singleton<StateFall>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateRise
//
///////////////////////////////////////////////////////////////

class StateRise : public StateFinishedByTimer
{
public:
	StateRise() : StateFinishedByTimer(CHAIN_RISE) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateRise* instance() { return &Singleton<StateRise>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateWeaponGrip
//
///////////////////////////////////////////////////////////////

class StateWeaponGrip : public StateBase
{
public:
	StateWeaponGrip() : StateBase(CHAIN_WEAPON_GRIP) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateWeaponGrip* instance() { return &Singleton<StateWeaponGrip>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateTransition
//
///////////////////////////////////////////////////////////////

class StateTransition : public StateFinishedByTimer
{
public:
	StateTransition() : StateFinishedByTimer(CHAIN_TRANSITION) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	static StateTransition* instance() { return &Singleton<StateTransition>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateMovements
//
///////////////////////////////////////////////////////////////

class StateMovements : public StateBase
{
public:
	StateMovements() : StateBase(CHAIN_MOVEMENTS) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const { return true; }
	static StateMovements* instance() { return &Singleton<StateMovements>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateFlyUp
//
///////////////////////////////////////////////////////////////

class StateFlyUp : public StateFinishedByTimer
{
public:
	StateFlyUp() : StateFinishedByTimer(CHAIN_FLY_UP) {}
	void start(UnitReal* owner) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateFlyUp* instance() { return &Singleton<StateFlyUp>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateFlyDown
//
///////////////////////////////////////////////////////////////

class StateFlyDown : public StateBase
{
public:
	StateFlyDown() : StateBase(CHAIN_FLY_DOWN) {}
	void start(UnitReal* owner) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateFlyDown* instance() { return &Singleton<StateFlyDown>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateWork
//
///////////////////////////////////////////////////////////////

class StateWork : public StateBase
{
public:
	StateWork() : StateBase(CHAIN_WORK) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateWork* instance() { return &Singleton<StateWork>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateGiveResource
//
///////////////////////////////////////////////////////////////

class StateGiveResource : public StateFinishedByTimer
{
public:
	StateGiveResource() : StateFinishedByTimer(CHAIN_GIVE_RESOURCE) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateGiveResource* instance() { return &Singleton<StateGiveResource>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateAttack
//
///////////////////////////////////////////////////////////////

class StateAttack : public StateBase
{
public:
	StateAttack() : StateBase(CHAIN_ATTACK) {}
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateAttack* instance() { return &Singleton<StateAttack>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateTeleporting
//
///////////////////////////////////////////////////////////////

class StateTeleporting : public StateFinishedByTimer
{
public:
	StateTeleporting() : StateFinishedByTimer(CHAIN_TELEPORTING) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateTeleporting* instance() { return &Singleton<StateTeleporting>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateBeBuild
//
///////////////////////////////////////////////////////////////

class StateBeBuild : public StateBase
{
public:
	StateBeBuild() : StateBase(CHAIN_BE_BUILT) {}
	void start(UnitReal* owner) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateBeBuild* instance() { return &Singleton<StateBeBuild>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateDisconnect
//
///////////////////////////////////////////////////////////////

class StateDisconnect : public StateBase
{
public:
	StateDisconnect() : StateBase(CHAIN_DISCONNECT) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const ;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateDisconnect* instance() { return &Singleton<StateDisconnect>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateBuildingStand
//
///////////////////////////////////////////////////////////////

class StateBuildingStand : public StateBase
{
public:
	StateBuildingStand() : StateBase(CHAIN_BUILDING_STAND) {}
	bool canStart(UnitReal* owner) const;
	static StateBuildingStand* instance() { return &Singleton<StateBuildingStand>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateItemStand
//
///////////////////////////////////////////////////////////////

class StateItemStand : public StateBase
{
public:
	StateItemStand() : StateBase(CHAIN_STAND) {}
	bool canStart(UnitReal* owner) const;
	static StateItemStand* instance() { return &Singleton<StateItemStand>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateProduction
//
///////////////////////////////////////////////////////////////

class StateProduction : public StateBase
{
public:
	StateProduction() : StateBase(CHAIN_PRODUCTION) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateProduction* instance() { return &Singleton<StateProduction>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateMoveToCargo
//
///////////////////////////////////////////////////////////////

class StateMoveToCargo : public StateBase
{
public:
	StateMoveToCargo() : StateBase(CHAIN_MOVE_TO_CARGO) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateMoveToCargo* instance() { return &Singleton<StateMoveToCargo>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateLandToLoad
//
///////////////////////////////////////////////////////////////

class StateLandToLoad : public StateBase
{
public:
	StateLandToLoad() : StateBase(CHAIN_LAND_TO_LOAD) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateLandToLoad* instance() { return &Singleton<StateLandToLoad>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateOpenForLanding
//
///////////////////////////////////////////////////////////////

class StateOpenForLanding : public StateFinishedByTimer
{
public:
	StateOpenForLanding() : StateFinishedByTimer(CHAIN_OPEN_FOR_LANDING) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateOpenForLanding* instance() { return &Singleton<StateOpenForLanding>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateCloseForLanding
//
///////////////////////////////////////////////////////////////

class StateCloseForLanding : public StateFinishedByTimer
{
public:
	StateCloseForLanding() : StateFinishedByTimer(CHAIN_CLOSE_FOR_LANDING) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateCloseForLanding* instance() { return &Singleton<StateCloseForLanding>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateLanding
//
///////////////////////////////////////////////////////////////

class StateLanding : public StateFinishedByTimer
{
public:
	StateLanding() : StateFinishedByTimer(CHAIN_LANDING) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canStart(UnitReal* owner) const;
	bool canFinish(UnitReal* owner) const;
	static StateLanding* instance() { return &Singleton<StateLanding>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateInTransport
//
///////////////////////////////////////////////////////////////

class StateInTransport : public StateBase
{
public:
	StateInTransport() : StateBase(CHAIN_IN_TRANSPORT) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateInTransport* instance() { return &Singleton<StateInTransport>::instance(); }
};


///////////////////////////////////////////////////////////////
//
//    class StateOpen
//
///////////////////////////////////////////////////////////////

class StateOpen : public StateFinishedByTimer
{
public:
	StateOpen() : StateFinishedByTimer(CHAIN_OPEN) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateOpen* instance() { return &Singleton<StateOpen>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateClose
//
///////////////////////////////////////////////////////////////

class StateClose : public StateFinishedByTimer
{
public:
	StateClose() : StateFinishedByTimer(CHAIN_CLOSE) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateClose* instance() { return &Singleton<StateClose>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateUpgrade
//
///////////////////////////////////////////////////////////////

class StateUpgrade : public StateFinishedByTimer
{
public:
	StateUpgrade() : StateFinishedByTimer(CHAIN_UPGRADE) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateUpgrade* instance() { return &Singleton<StateUpgrade>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateIsUpgraded
//
///////////////////////////////////////////////////////////////

class StateIsUpgraded : public StateFinishedByTimer
{
public:
	StateIsUpgraded() : StateFinishedByTimer(CHAIN_IS_UPGRADED) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateIsUpgraded* instance() { return &Singleton<StateIsUpgraded>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateBirth
//
///////////////////////////////////////////////////////////////

class StateBirth : public StateFinishedByTimer
{
public:
	StateBirth() : StateFinishedByTimer(CHAIN_BIRTH) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateBirth* instance() { return &Singleton<StateBirth>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateBirthInAir
//
///////////////////////////////////////////////////////////////

class StateBirthInAir : public StateBase
{
public:
	StateBirthInAir() : StateBase(CHAIN_BIRTH_IN_AIR) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateBirthInAir* instance() { return &Singleton<StateBirthInAir>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateTrigger
//
///////////////////////////////////////////////////////////////

class StateTrigger : public StateBase
{
public:
	StateTrigger() : StateBase(CHAIN_TRIGGER) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateTrigger* instance() { return &Singleton<StateTrigger>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateUninstal
//
///////////////////////////////////////////////////////////////

class StateUninstal : public StateFinishedByTimer
{
public:
	StateUninstal() : StateFinishedByTimer(CHAIN_UNINSTALL) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	static StateUninstal* instance() { return &Singleton<StateUninstal>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class StateDeath
//
///////////////////////////////////////////////////////////////

class StateDeath : public StateFinishedByTimer
{
public:
	StateDeath() : StateFinishedByTimer(CHAIN_DEATH) {}
	void start(UnitReal* owner) const;
	void finish(UnitReal* owner, bool finished = true) const;
	bool canFinish(UnitReal* owner) const;
	static StateDeath* instance() { return &Singleton<StateDeath>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class UnitLegionaryPosibleStates
//
///////////////////////////////////////////////////////////////

typedef vector<StateBase*> States;

class UnitLegionaryPosibleStates : public States
{
public:
	UnitLegionaryPosibleStates();
	static UnitLegionaryPosibleStates* instance() { return &Singleton<UnitLegionaryPosibleStates>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class UnitBuildingPosibleStates
//
///////////////////////////////////////////////////////////////

class UnitBuildingPosibleStates : public States
{
public:
	UnitBuildingPosibleStates();
	static UnitBuildingPosibleStates* instance() { return &Singleton<UnitBuildingPosibleStates>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class UnitItemPosibleStates
//
///////////////////////////////////////////////////////////////

class UnitItemPosibleStates : public States
{
public:
	UnitItemPosibleStates();
	static UnitItemPosibleStates* instance() { return &Singleton<UnitItemPosibleStates>::instance(); }
};

///////////////////////////////////////////////////////////////
//
//    class UnitItemPosibleStates
//
///////////////////////////////////////////////////////////////

class UnitAllPosibleStates : public States
{
public:
	UnitAllPosibleStates();
	static UnitAllPosibleStates* instance() { return &Singleton<UnitAllPosibleStates>::instance(); }
	StateBase* find(ChainID id);
};

#endif // __STATE_BASE_H__
