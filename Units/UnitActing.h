#ifndef __UNIT_ACTING_H__
#define __UNIT_ACTING_H__

#include "UnitObjective.h"
#include "DirectControlMode.h"

//////////////////////////////////////////
// "Действующий" юнит: легионер и здание
//////////////////////////////////////////
class UnitActing : public UnitObjective
{
public:
	UnitActing(const UnitTemplate& data);
	~UnitActing();

	int selectionListPriority() const { return selectionListPriority_; }
	void updateSelectionListPriority();

	void Kill();
	void Quant();

	void graphQuant(float dt);
	bool canSuspendCommand(const UnitCommand& command) const;
	void executeCommand(const UnitCommand& command);
	Vect2f getFreePosition(const Vect2f& point)  const;
	void setPose(const Se3f& poseIn, bool initPose);
	void showDebugInfo();
	void serialize(Archive& ar);
	void changeUnitOwner(Player* playerIn);
    
	void mapUpdate(float x0,float y0,float x1,float y1);
	void collision(UnitBase* unit, const ContactInfo& contactInfo);

	// Транспорт.
	bool canPutInTransport(const UnitBase* unit) const;
	bool putInTransport(UnitSquad* squad);
	void putUnitInTransport(UnitLegionary* unit);
	bool waitingForPassenger() const;
	void putOutTransport(int index = -1);
	int findMainUnitInTransport();
	LegionariesLinks& transportSlots() { return transportSlots_; } 
	void setCargo(UnitLegionary* unit) { cargo_ = unit; }
	const UnitLegionary* cargo() const { return cargo_; }
	void detachFromTransportSlot(int index, int slotsNumber);
	void setReadyForLanding(bool _readyForLanding) { readyForLanding = (currentState() == CHAIN_OPEN_FOR_LANDING) && _readyForLanding; }
	bool isReadyForLanding() { return readyForLanding && (getStaticReasonXY() & STATIC_DUE_TO_OPEN_FOR_LOADING); }
	void resetCargo();
	bool isInTransport(const AttributeBase* attribute) const;

	bool autoFindTransport() const { return autoFindTransport_; }
	void toggleAutoFindTransport(bool state){ autoFindTransport_ = state; } 

	// Производство.
	bool producedAllParameters() const;
	Accessibility canProduction(int number) const;
	Accessibility canProduceParameter(int number) const;
	bool startProduction(const AttributeBase* producedUnit, UnitSquad* ownerSquad, int counter, bool restartFromQueue = false); // true - производство запущено
	bool isProducing() const { return producedUnit_ || producedParameter_; }
	float productionProgress() const { return productionConsumer_.progress(); }
	const ProducedQueue& producedQueue() const { return producedQueue_; }
	float productionParameterProgress() const;
	const AttributeBase* producedUnit() const { return producedUnit_; }
	UnitLegionary* shippedUnit() const { return shippedUnit_; }
	int productionCounter() { return productionCounter_; }
	void finishProduction();
	void shipProduction();

	DirectControlMode directControl() const { return directControl_; }
	bool isDirectControl() const { return directControl_ & DIRECT_CONTROL_ENABLED; }
	bool isSyndicateControl() const { return directControl_ & SYNDICATE_CONTROL_ENABLED; }

	/// не повторяемые условия, только для активного игрока в прямом упровлении
	bool activeDirectControl() { return activeDirectControl_; }
	bool isActiveDirectControl() { return activeDirectControl_ & DIRECT_CONTROL_ENABLED; }
	bool isActiveSyndicateControl() { return activeDirectControl_ & SYNDICATE_CONTROL_ENABLED; }

	bool isUnseen() const { return isInvisible() || (hiddenLogic() & (HIDE_BY_TELEPORT | HIDE_BY_TRANSPORT)); }
	//если хоть кто-то подсвечивает, то юнит виден
	// если никто не подсвечивает, то если юнит невидим или кем-то скрыт, то он невидим
	bool isInvisible() const;
	void setVisibility(bool visible, float time = -1.f);

	/// Запрос на очистку региона от юнитов.
	void clearRegion(Vect2i position, float radius);

	void setImpulseAbnormalState(const AbnormalStateAttribute& state, UnitBase* owner);
	virtual bool isWorking() const { return unitState != UnitReal::AUTO_MODE || isProducing() || isUpgrading() || isDocked(); }

	/// Режимы атаки. Автоматическое поведение.
	void attackQuant();
	void moveController();		// Контроль WayPoint-ов.
	void targetController();	// Анализ и выставление целей.
	bool needAiUpdate() const { return !aiScanTimer(); }
	void toggleAiUpdate(){ aiUpdateFlag_ = true; }

	///////////////////////////////////////////////////////////////////////////////////

	void setDirectControl(DirectControlMode mode);
	void executeDirectKeys(const UnitCommand& command);
	void setDirectKeysCommand(const UnitCommand& command) { directKeysCommand = command; }
	/// переключение оружия в прямом управлении
	/// direction < 0 - назад, direction >= 0 - вперёд
	bool changeDirectControlWeapon(int direction);
	int directControlWeaponID() const;
	const Vect3f& directControlOffset() const;
	void setActiveDirectControl(DirectControlMode activeDirectControl);
	
	bool addWayPoint(const Vect3f& p);

	// Оружие
	typedef SwapVector<WeaponBase*> Weapons;
	typedef std::vector<WeaponSlot> WeaponSlots;

	void initWeapon();

	float weaponChargeLevel(int weapon_id = 0) const;
	bool hasWeapon(int weapon_id = 0) const;
	WeaponBase* findWeapon(int weapon_id) const;
	const WeaponBase* findWeapon(const char* label) const;

	Vect3f firePosition() const;
	Vect3f specialFirePosition() const;
	float targetRadius();
	bool fireDistanceCheck() const; // true - если цель(и) в радиусах атаки.
	bool fireWeaponModeCheck(const WeaponBase* weapon) const;
	bool fireTargetExist() const;
	bool isFireTarget(const UnitInterface* unit) const;
	bool isAttackedFireTarget(const UnitInterface* unit) const;
	bool specialFireTargetExist() const;
	bool isDefaultWeaponExist();

	bool canFire(int weaponID, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	bool fireRequest();
	bool fireCheck(WeaponTarget& target) const;
	bool fireDistanceCheck(const WeaponTarget& target, bool check_fow = false) const;
	WeaponTarget fireTarget() const;
	/// допустимая дистанция стрельбы по цели
	Rangef fireDistance(const WeaponTarget& target) const;
	void fireStop();
	void detonateMines();
	bool canDetonateMines();
	int queuredMines() const;
	bool weaponAnimationMode() const { return weaponAnimationMode_; }

	const Weapons& weapons() const { return weapons_; }
	bool addWeapon(WeaponBase* weapon);
	bool removeWeapon(WeaponBase* weapon);

	void selectWeapon(int weapon_id);
	const WeaponPrm* selectedWeapon() const;  
	int selectedWeaponID() const { return selectedWeaponID_; }
	bool replaceWeapon(int equipment_type, const WeaponPrm* weapon);
	bool removeWeapon(int equipment_type, const WeaponPrm* weapon);
	bool weaponAccessible(const WeaponPrm* weapon) const;

	UnitInterface* targetUnit() const { return targetUnit_; }
	void setTargetUnit(UnitInterface* targetUnit) { targetUnit_ = targetUnit; }
	void setManualTarget(UnitInterface* unit);
	void setTargetPoint(const Vect3f& point);
	void clearAttackTarget(int weaponID = 0);
	void clearAutoAttackTargets();

	void setAutoAttackMode(AutoAttackMode mode);
	AutoAttackMode autoAttackMode() const { return attackMode_.autoAttackMode(); }
	void setWalkAttackMode(WalkAttackMode mode);
	WalkAttackMode walkAttackMode() const { return attackMode_.walkAttackMode(); }
	void setWeaponMode(WeaponMode mode);
	WeaponMode weaponMode() const { return attackMode_.weaponMode(); }
	AutoTargetFilter autoTargetFilter() const { return attackMode_.autoTargetFilter(); }
	void setAutoTargetFilter(AutoTargetFilter filter);

	const AttackMode& attackMode() const { return attackMode_; }
	void setAttackMode(const AttackMode& prm){ attackMode_ = prm; }

	const WeaponSlots& weaponSlots() const { return weaponSlots_; }

	WeaponRotationController& weaponRotation(){ return weaponRotation_; }
	const WeaponRotationController& weaponRotation() const { return weaponRotation_; }

	bool canAttackTarget(const WeaponTarget& target, bool check_fow = false) const;
	bool canAutoAttackTarget(const WeaponTarget& target) const;
	void attackTargetUnreachable(bool unreachable) { attackTargetUnreachable_ = unreachable; }
	virtual bool canAutoAttack() const { return unitState != ATTACK_MODE || !selectedWeaponID_; }

	float fireRadius() const;
	float fireRadiusOfWeapon(int weapon_id) const;
	float minFireRadiusOfWeapon(int weapon_id) const;
	float fireRadiusMin() const;

	float autoFireRadius() const;
	float autoFireRadiusMin() const;

	const AttackModeAttribute& attackModeAttr() const;
	/// Установка анимации оружия (стрельба/прицеливание).
	/// Возвращает true если оружием установлана какая-то анимация для всей модели.
	bool weaponChainQuant(MovementState state);

	void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics);

	void showCircles(const Vect3f& pos);
	void showEditor();

	///////////////////////////////////////////////////////////////////////////////////

	// Дамадж
	void setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo = 0);
	virtual void deathGainMultiplicator(ParameterArithmetics& arithmetics) const {}
	bool underAttack() const { return eventAttackTimer_(); }

	/// Апгрейды.
	Accessibility canUpgrade(int upgradeNumber, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	bool upgrade(int upgradeNumber);
	float upgradeProgres(int number, bool forFinishPhase) const;
	void setUpgradeProgresParameters(int index = -1, int time = 0, int finishTime = 0);
	void finishUpgrade();
	bool isUpgrading() const { return executedUpgrade() || (currentState() & CHAIN_IS_UPGRADED); }
	int executedUpgradeIndex() const { return executedUpgradeIndex_; }
	void cancelUpgrade();
	const AttributeBase& beforeUpgrade() const { return *beforeUpgrade_; }
	void setBeforeUpgrade(const AttributeBase* attr) { beforeUpgrade_ = const_cast<AttributeBase*>(attr);}

	void setIgnoreFreezedByTrigger(bool ignoreFreezedByTrigger) { ignoreFreezedByTrigger_ = ignoreFreezedByTrigger; }
	static void setFreezedByTrigger(bool freezedByTrigger) { freezedByTrigger_ = freezedByTrigger; }

	const AttributeBase* executedUpgrade() const { return executedUpgradeIndex_ >= 0 ? &*attr().upgrades[executedUpgradeIndex_].upgrade : 0; }

	bool canFireInCurentState() const { return currentState() < CHAIN_GIVE_RESOURCE; }

	void attachUnit(UnitReal* unit);
	void detachUnit(UnitReal* unit);

	// Части тела
	bool hasParts() const { return !bodyParts_.empty(); }
	bool putOnItem(const AttributeItemInventory* item);
	void putOffItem(const AttributeItemInventory* item);
	bool requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const;
	void subResource(const ParameterSet& resource);
	bool accessibleParameter(const ProducedParameters& parameter) const;

	Player* playerPrev() const { return playerPrev_; }

protected:
	// Оружие
	void weaponQuant();
	void weaponPostQuant();

	WeaponSlots weaponSlots_;
	/// отсортированный по группам и приоритетам список оружия,	через него раздаются цели
	Weapons weapons_;
	bool weaponAnimationMode_;
	int directControlWeaponSlot_;

	/// выбранное для атаки оружие, 0 если не установлено
	int selectedWeaponID_;
	WeaponRotationController weaponRotation_;

	UnitLink<UnitInterface> targetUnit_;  // Юнит для атаки
	Vect3f targetPoint_;  // Точка для атаки

	UnitLink<UnitInterface> specialTargetUnit_;  // Юнит для атаки
	Vect3f specialTargetPoint_;  // Точка для атаки

	bool targetPointEnable; // Указана точка атаки?
	bool specialTargetPointEnable; // Указана точка атаки?
	bool attackTargetUnreachable_;

	AttackMode attackMode_;

	enum DirectControlFireMode
	{
		DIRECT_CONTROL_PRIMARY_WEAPON	= 1,
		DIRECT_CONTROL_SECONDARY_WEAPON = 2
	};

	int directControlFireMode_;

	DirectControlMode directControl_;
	DirectControlMode activeDirectControl_;

	float manualModeVelocityFactor;

	// Режимы атаки.
	DurationTimer aiScanTimer;
	bool aiUpdateFlag_;

	DelayTimer patrolTimer_;
	DelayTimer patrolStopTimer_;

	// Транспорт.
	LegionariesLinks transportSlots_;
	Player* playerPrev_;
	UnitLink<UnitLegionary> cargo_;
	int putOutTransportIndex_;
	bool landToUnloadStart;
	bool readyForLanding;

	bool autoFindTransport_;
	
	// Производство.
	int productionCounter_;
	const AttributeBase* producedUnit_;
	ParameterConsumer productionConsumer_;
	UnitLink<UnitLegionary> shippedUnit_;
	UnitLink<UnitSquad> shippedSquad_;
	UnitLink<UnitInterface> resourceItemForProducedUnit_;
	Vect2f shipmentPosition_;
	ProducedQueue producedQueue_;
	ParameterSet productivityRest_;
	DelayTimer callFinishProductionTimer_;
	int totalProductionCounter_;
	bool teleportProduction_;
	bool wasProduced_;
	
	const ProducedParameters* producedParameter_;
	DurationTimer productionParameterTimer_;

	AbnormalStateAttribute impulseAbnormalState_;
	UnitLink<UnitBase> impulseOwner_;

	int executedUpgradeIndex_;
	int upgradeTime_;
	int previousUpgradeIndex_;
	int finishUpgradeTime_;

	struct BodyPart	
	{
		const BodyPartAttribute& partAttr;
		ParameterSet parameters; // Текущие значения параметров надетого предмета
		AttributeItemInventoryReference item; // Атрибуты надетого предмета

		int visibilitySet;
		int visibilityGroup;

		BodyPart(const BodyPartAttribute& partAttr);
		void putOn(const BodyPartAttribute::Garment& garment);
		void putOff();
		void serialize(Archive& ar);
	};
	typedef vector<BodyPart> BodyParts;
	BodyParts bodyParts_;

	const Vect2f& shipmentPosition() const { return shipmentPosition_; }
	void setShipmentPosition(const Vect2f& shipmentPosition) { shipmentPosition_ = shipmentPosition; }
	void cancelUnitProduction(ProduceItem& item);
	void cancelProduction();

	virtual bool canAutoMove() const { return true; }

	bool move2firePosition();
	bool rot2FireTarget();
	bool traceFireTarget() const;
	bool setPatrolPoint();
	bool flyAroundTarget();

	DurationTimer visibleTimer_;
	DurationTimer unVisibleTimer_;

private:
	int selectingTeamIndex_;
	DurationTimer eventAttackTimer_;
	DurationTimer fireRequestTimer_;
	bool ignoreFreezedByTrigger_;
	static bool freezedByTrigger_;

	bool isInvisiblePrev_;

	AttributeBase* beforeUpgrade_;

	void updatePart(const BodyPart& part);
	
	bool targetOnSecondMap;
	UnitCommand directKeysCommand;
	
	typedef vector<UnitLink<UnitReal> > RealsLinks;
	RealsLinks dockedUnits;

	int selectionListPriority_;

	ShowChangeParametersController showParamController_;

	bool fireRequest(WeaponTarget& target, bool no_movement_weapons_only = false);
	bool fireRequestAuto();
};

#endif //__UNIT_ACTING_H__
