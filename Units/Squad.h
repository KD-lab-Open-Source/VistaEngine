#ifndef __SQUAD_H__
#define __SQUAD_H__

#include "UnitInterface.h"
#include "IronLegion.h"
#include "PositionGeneratorCheck.h"
#include "PositionGeneratorFormation.h"
#include "UnitNumberManager.h"
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

	//-------------------------------------
	UnitSquad* getSquadPoint() { return this; }
	
	UnitReal* getUnitReal() { xassert(mainUnit_);  return mainUnit_; }
	const UnitReal* getUnitReal() const { xassert(mainUnit_); return mainUnit_; }

	void setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo);

	void setHomePosition(const Vect2f& homePosition) { homePosition_ = homePosition; }
	const Vect2f& homePosition() const { return homePosition_; }

	void changeUnitOwner(Player* player);

	void clearOrders();
	bool fireTargetExist() const;

	bool selectAble() const;
	void graphQuant(float dt);
	const Se3f& interpolatedPose() const { return pose(); } // использовать только внутри graphQuant()

	const vector<Vect2f>& getWayPoints() const { return wayPoints_; }
	const vector<Vect2f>& getPatrolPoints() const {	return patrolPoints_; }

	void setSelected(bool selected);
	bool isInTransport(const AttributeBase* attribute, bool singleOnly) const;
	
	//-------------------------------------------------
	bool addUnit(UnitLegionary* unit, bool set_position, bool set_wayPoint = false);
	void removeUnit(UnitLegionary* unit);
	bool checkFormationConditions(UnitFormationTypeReference unitType, int numberUnits = 1) const;
	bool addSquad(UnitSquad* squad, float extraRadius2 = 0.f);
	bool canAddWholeSquad(const UnitSquad* squad, float extraRadius2 = 0.f) const;
	bool canAddSquad(const UnitSquad* squad) const;
	void split(bool need_select);
	void Kill();
	
	bool Empty() const { return units_.empty(); }
	int unitsNumber() const { return units().size(); }
	int unitsNumber(const AttributeBase* attribute) const;
	bool contain(const AttributeBase* attribute) const;
	float formationRadius() const;

	bool isUnseen() const {  xassert(!units_.empty()); return getUnitReal()->isUnseen(); }

	bool patrolMode() const { return patrolPoints_.size() > 1; }
	
	// Передвижение
	void clearWayPoints();
	void addWayPoint(const Vect2f& way_point, bool setLeaderPoint = true, bool enableMoveMode = true);
	bool noWayPoints() const { return wayPoints_.empty(); }
	int wayPointsNumber() const { return wayPoints_.size(); }
	
	// Стрельба
	void clearTargets();
	void addTarget(UnitInterface* target);
	void addTarget(const Vect3f& v);
	void selectWeapon(int weapon_id);
	void fireStop();
	
	// Coordinate funcs
	const MatX2f& stablePose() const { return stablePose_; }
	void setStablePosition(const Vect2f& position) { stablePose_.trans = position; }
	void setStableOrientation(const Mat2f& orientation) { stablePose_.rot = orientation; }

	Vect2f forwardDirection() const { return stablePose().rot.ycol(); }

	void holdProduction();
	void unholdProduction();
	float productionProgress() const;
	float productionParameterProgress() const;

	struct RequestedUnit 
	{
		AttributeReference unit;
		bool paused;
		bool requested;
		RequestedUnit(const AttributeBase* unitIn = 0) : unit(unitIn), paused(false), requested(false) {}
		void serialize(Archive& ar);
	};
	typedef list<RequestedUnit> RequestedUnits;

	const RequestedUnits& requestedUnits() const { return requestedUnits_; }

	const UnitNumberManager& unitNumberManager() const { return unitNumberManager_; }

	float weaponChargeLevel(int weapon_id = 0) const;

	void setSquadToFollow(UnitBase* squadToFollow);
	UnitBase* getSquadToFollow() const { return &*squadToFollow_; }
	bool setConstructedBuilding(UnitReal* building, bool queued);

	void showDebugInfo();

	void setCurrentFormation(int num);

	void setLastWayPoint(const Vect2f& point) { lastWayPoint = point; }

	void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics);

	void setAttackMode(const AttackMode& attack_mode);
	const AttackMode& attackMode() const { return attackMode_; }

	void serialize(Archive& ar);

	bool locked() const;
	void setLocked(bool locked);

	bool canSuspendCommand(const UnitCommand& command) const;
	bool isSuspendCommand(CommandID commandID);
	bool isSuspendCommandWork();

	bool isWorking() const;

	float getMinVelocity() const;
	float getAvrVelocity() const;
	bool correctSpeed() const { return attr().formations[currentFormation].correctSpeed; }

	bool addUnitsFromArea(AttributeUnitReferences attrUnits, float radius, bool testOnly = false); 
	// при testOnly == true добавление в скад не происходит, возращает результат возможности добавления в сквад

	void startUsedByTrigger(int time, UsedByTriggerReason reason);
	void setShowAISign(bool showAISign);

	void updateMainUnit();

private:
	const LegionariesLinks& graphUnits() const;
	
	AttackMode attackMode_;

	LegionariesLinks units_;
	mutable LegionariesLinks unitsGraphics_;
	DelayTimer clearUnitsGraphicsTimer_;
	UnitLegionary* mainUnit_;
	mutable bool needUpdateUnits_;
	static MTSection unitsLock_;

	Vect2f average_position;
	int currentFormation;
	
	MatX2f stablePose_; // Центр сквада, куда он стремится.
	Vect2f homePosition_; 
	bool stablePoseRestarted_;

	Vect2f lastWayPoint;
	bool updateSquadWayPoints;
	
	//--------------------------------
	RequestedUnits requestedUnits_;
	
	Vect2fVect wayPoints_;

	Vect2fVect patrolPoints_;
	int patrolIndex_;

	PositionGeneratorFormation positionFormation;
	bool underDirectControl_;

	bool locked_;

	UnitLink<UnitBase> squadToFollow_;
	DurationTimer followTimer;

	UnitNumberManager unitNumberManager_;

	DurationTimer automaticJoinTimer_;

	
	/////////////////////////////////////////////////////////////////////
	//			Private Members
	void calcCenter();
	
	void followQuant();
	void followLeaderQuant();

	//------------------------------
	friend class Player;
	friend void fCommandSquadClearUnitsGraphics(XBuffer& stream);
};


#endif //__SQUAD_H__
