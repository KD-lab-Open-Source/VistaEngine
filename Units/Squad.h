#ifndef __SQUAD_H__
#define __SQUAD_H__

#include "UnitInterface.h"
#include "IronLegion.h"
#include "AttributeSquad.h"

typedef	UnitLink<UnitLegionary> LegionariesLink;
typedef	vector<LegionariesLink> LegionariesLinks;

class UnitSquad : public UnitInterface
{
public:
	UnitSquad(const UnitTemplate& data);
	~UnitSquad();

	const AttributeSquad& attr() const { return safe_cast_ref<const AttributeSquad&>(__super::attr()); }
	const AttributeBase* selectionAttribute() const { return &getUnitReal()->attr(); }
	int selectionListPriority() const { return getUnitReal()->selectionListPriority(); }

	void relaxLoading();

	void Quant();
	void destroyLinkSquad();
	void executeCommand(const UnitCommand& command);

	void updatePose(bool initPose);
	void setPose(const Se3f& pose, bool initPose);

	const LegionariesLinks& units() const { return MT_IS_LOGIC() ? units_ : graphUnits(); }

	bool canAttackTarget(const WeaponTarget& target, bool check_fow = false) const;
	/// все тоже, что что и canAttackTarget, но "прямо сейчас", т.е. с учетом активации оружия и дистанции атаки
	bool fireDistanceCheck(const WeaponTarget& target, bool check_fow = false) const;
	
	bool canDetonateMines() const;
	bool canFire(int weaponID, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	bool canExtractResource(const UnitItemResource* item) const;
	bool canBuild(const UnitReal* building) const;
	bool canRun() const;

	Accessibility canUpgrade(int upgradeNumber, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	Accessibility canProduction(int number) const;
	Accessibility canProduceParameter(int number) const;

	bool uniform(const UnitReal* unit = 0) const;
	bool prior(const UnitInterface* unit) const;

	// Работа с инвентарём

	/// Проверка, влезет ли предмет в инвентарь.
	bool canPutToInventory(const UnitItemInventory* item) const;

	bool putToInventory(const UnitItemInventory* item);
	bool putToInventory(const InventoryItem& item);

	//-------------------------------------
	UnitSquad* getSquadPoint() { return this; }
	const UnitSquad* getSquadPoint() const { return this; }
	
	UnitLegionary* getUnitReal() { xassert(mainUnit_);  return mainUnit_; }
	const UnitLegionary* getUnitReal() const { xassert(mainUnit_); return mainUnit_; }

	float angleZ() const { return formationController_.orientation(); }
	const Vect2f& wayPoint() const { return formationController_.lastWayPoint(); }
	bool getManualMovingPosition(Vect2f& point) const;
	void wayPointsClear() { formationController_.wayPointsClear(); }
	bool wayPointsEmpty() const { return formationController_.wayPointEmpty(); }
	int impassability() const { return formationController_.impassableTerrainTypes(); }
	int passability() const { return formationController_.passableObstacleTypes(); }
	Se3f getNewUnitPosition(UnitLegionary* unit) { return formationController_.getNewUnitPosition(&unit->formationUnit_); }
	void addWayPointS(const Vect2f& way_point) { formationController_.addWayPointS(way_point); }
	void setDisablePathTracking(bool disablePathTracking) { formationController_.setDisablePathTracking(disablePathTracking); }

	
	void setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo);

	void changeUnitOwner(Player* player);

	void clearOrders();
	bool fireTargetExist() const;

	bool selectAble() const;
	const UI_MinimapSymbol* minimapSymbol() const;

	void graphQuant(float dt);
	void setInterpolatedPose(const Se3f& pos) { interpolatedPose_ = pos; }
	const Se3f& interpolatedPose() const { return interpolatedPose_; } // использовать только внутри graphQuant()

	const UI_ShowModeSprite* getSelectSprite() const;

	void setSelected(bool selected);
	bool isInTransport(const AttributeBase* attribute, bool singleOnly) const;
	
	//-------------------------------------------------
	bool checkFormationConditions(UnitFormationTypeReference unitType, int numberUnits = 1) const;
	void cancelRequestedUnitProduction(const UnitActing* factory);
	void addRequestedUnit(const UnitActing* factory, UnitLegionary* unit);
	void addUnit(UnitLegionary* unit);
	void removeUnit(UnitLegionary* unit);
	void addSquad(UnitSquad* squad, float extraRadius2 = 0.f);
	bool canAddWholeSquad(const UnitSquad* squad, float extraRadius2 = 0.f) const;
	bool canAddSquad(const UnitSquad* squad) const;
	void split(bool need_select);
	void Kill();
	
	bool Empty() const { return units_.empty(); }
	int unitsNumber() const { return units().size(); }
	int unitsNumber(const AttributeBase* attribute) const;
	int unitsNumberMax() const;
	bool contain(const AttributeBase* attribute) const;
	
	bool isUnseen() const {  xassert(!units_.empty()); return getUnitReal()->isUnseen(); }

	bool patrolMode() const { return patrolPoints_.size() > 1; }
	
	// Стрельба
	void clearTargets();
	void addTarget(UnitInterface* target, bool moveToTarget = true);
	void addTarget(const Vect3f& v, bool moveToTarget = true);
	void selectWeapon(int weapon_id);
	void fireStop(int wid = -1);
	
	float productionProgress() const;
	float productionParameterProgress() const;

	class RequestedUnit 
	{
	public:
		RequestedUnit(const AttributeBase* unit = 0) : unit_(unit) {}
		void request(UnitActing* factory) { factory_ = factory; }
		bool requested() const { return factory_ != 0; }
		const UnitActing* factory() const { return factory_; }
		void stopProduction();
		float productionProgress() const;
		const AttributeBase* unit() const { return unit_; }
		void serialize(Archive& ar);

	private:
		AttributeReference unit_;
		UnitLink<UnitActing> factory_;
	};
	typedef SwapVector<RequestedUnit> RequestedUnits;

	const RequestedUnits& requestedUnits() const { return requestedUnits_; }
	bool canQueryUnits(const AttributeBase* attribute) const;

	void reserveUnitNumber(const AttributeBase* attribute, int counter);

	float weaponChargeLevel(int weapon_id = 0) const;

	void setSquadToFollow(UnitSquad* squad) { if(squad != this) formationController_.setFollowSquad(squad); }
	bool followSquad() const { return formationController_.followSquad(); }
	bool setConstructedBuilding(UnitReal* building, bool queued);

	void showDebugInfo();

	void setAttackMode(const AttackMode& attack_mode);
	const AttackMode& attackMode() const { return attackMode_; }

	void serialize(Archive& ar);

	bool locked() const;
	void setLocked(bool locked);

	bool canSuspendCommand(const UnitCommand& command) const;
	bool isSuspendCommand(CommandID commandID);
	bool isSuspendCommandWork();

	bool isWorking() const;

	bool addUnitsFromArea(AttributeUnitReferences attrUnits, float radius, bool testOnly = false); 
	// при testOnly == true добавление в скад не происходит, возращает результат возможности добавления в сквад

	void setUsedByTrigger(int priority, const void* action = 0, int time = 0);

	void findAndSetForceMainUnit(UnitLegionary* unit);
	void updateMainUnit();

	enum WaitingMode{
		WAITING_DISABLE = 0,
		WAITING_FOR_MAIN_UNIT = 1,
		WAITING_ALL = 2
	};

	WaitingMode waitingMode() { return waitingMode_; }
	void setWaitingMode(WaitingMode mode);

	UnitSquad* squadForUpgrade() const { return squadForUpgrade_; }
	void setSquadForUpgrade(UnitSquad* squadForUpgrade) { squadForUpgrade_ = squadForUpgrade; }
	FormationController formationController_;

private:
	void stop();
	void setMainUnit(LegionariesLinks::reference mainUnit);
	void setForceMainUnit(LegionariesLinks::reference mainUnit);

	const LegionariesLinks& graphUnits() const;

	Se3f interpolatedPose_;
	Se3f interpolatedPosePrev_;
	
	int unitsReserved_;
	
	AttackMode attackMode_;

	LegionariesLinks units_;
	mutable LegionariesLinks unitsGraphics_;
	LogicTimer clearUnitsGraphicsTimer_;
	UnitLegionary* mainUnit_;
	mutable bool needUpdateUnits_;
	static MTSection unitsLock_;

	WaitingMode waitingMode_;

	//--------------------------------
	RequestedUnits requestedUnits_;
	
	Vect2fVect patrolPoints_;
	int patrolIndex_;

	bool locked_;

	LogicTimer automaticJoinTimer_;

	UnitLink<UnitSquad> squadForUpgrade_;

	/////////////////////////////////////////////////////////////////////
	//			Private Members
	bool offenceMode();

	//------------------------------
	friend class Player;
	friend void fCommandSquadClearUnitsGraphics(XBuffer& stream);
};


#endif //__SQUAD_H__
