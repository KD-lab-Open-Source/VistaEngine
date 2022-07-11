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
		STATIC_DUE_TO_DEATH = 1 << 16,
		STATIC_DUE_TO_WAITING = 1 << 17,
		STATIC_DUE_TO_NOISE = 1 << 18,
		STATIC_DUE_TO_PICKING_ITEM = 1 << 19,
		STATIC_DUE_TO_TOUCH_DOWN = 1 << 20
	};

	// Причины выключения полета
	enum FlyingReason {
		FLYING_DUE_TO_TRANSPORT_LOAD = 1,
		FLYING_DUE_TO_TRANSPORT_UNLOAD = 2,
		FLYING_DUE_TO_UPGRADE = 4
	};

	UnitActing(const UnitTemplate& data);
	~UnitActing();

	int selectionListPriority() const { return selectionListPriority_; }
	void updateSelectionListPriority();

	void Kill();
	void frozenQuant();
	void Quant();
	
	int impassability() const { return rigidBody()->impassability(); }
	
	bool corpseQuant();

	void graphQuant(float dt);
	bool canSuspendCommand(const UnitCommand& command) const;
	void executeCommand(const UnitCommand& command);
	Vect2f getFreePosition(const Vect2f& point)  const;
	void setPose(const Se3f& poseIn, bool initPose);
	void showDebugInfo();
	void serialize(Archive& ar);
	void changeUnitOwner(Player* playerIn);

	MovementState getMovementState();
	bool isMoving() const { return isMoving_; }
	virtual int movementMode() { return 0; }

    float angleZ() const;

	bool checkInPathTracking(const UnitBase* tracker) const;

	RigidBodyUnit* rigidBody() const { return safe_cast<RigidBodyUnit*>(rigidBody_); }

	void enableFlying(int flyingReason);
	void disableFlying(int flyingReason, int time = 0);
	virtual void makeStatic(int staticReason);
	virtual void makeDynamic(int staticReason);
	virtual void makeStaticXY(int staticReason) {}
	virtual void makeDynamicXY(int staticReason) {}

	int getStaticReason() const { return staticReason_; }
	virtual int getStaticReasonXY() const { return 0; }

	const UI_MinimapSymbol* minimapSymbol(bool permanent) const;
	const struct UI_ShowModeSprite* getSelectSprite() const;

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
	bool transportEmpty() const;
	bool isInTransport(const AttributeBase* attribute) const;

	bool autoFindTransport() const { return autoFindTransport_; }
	void toggleAutoFindTransport(bool state){ autoFindTransport_ = state; } 

	// Производство.
	bool producedAllParameters() const;
	Accessibility canProduction(int number) const;
	Accessibility canProduceParameter(int number) const;
	void startProductionParamater(const ProducedParameters& prm);
	bool startProduction(const AttributeBase* producedUnit, UnitSquad* shippedSquad, int counter, bool restartFromQueue = false); // true - производство запущено
	bool isProducing() const { return producedUnit_ || producedParameter_; }
	float productionProgress() const { return productionConsumer_.progress(); }
	const ProducedQueue& producedQueue() const { return producedQueue_; }
	float productionParameterProgress() const;
	const AttributeBase* producedUnit() const { return producedUnit_; }
	const ProducedParameters* producedParameter() const { return producedParameter_; }
	float currentProductionProgress() const { xassert(!producedParameter_ || !producedUnit_); return producedUnit_ ? productionProgress() : productionParameterProgress(); }
	UnitLegionary* shippedUnit() const { return shippedUnit_; }
	int productionCounter() { return productionCounter_; }
	void finishProduction();
	void shipProduction();

	DirectControlMode directControl() const { return directControl_; }
	bool isDirectControl() const { return (directControl_ & DIRECT_CONTROL_ENABLED) != 0; }
	bool isSyndicateControl() const { return (directControl_ & SYNDICATE_CONTROL_ENABLED) != 0; }

	/// не повторяемые условия, только для активного игрока в прямом упровлении
	bool activeDirectControl() { return activeDirectControl_ != 0; }
	bool isActiveDirectControl() { return (activeDirectControl_ & DIRECT_CONTROL_ENABLED) != 0; }
	bool isActiveSyndicateControl() { return (activeDirectControl_ & SYNDICATE_CONTROL_ENABLED) != 0; }

	bool isUnseen() const { return isInvisible() || (hiddenLogic() & (HIDE_BY_TELEPORT | HIDE_BY_TRANSPORT)) != 0; }
	//если хоть кто-то подсвечивает, то юнит виден
	// если никто не подсвечивает, то если юнит невидим или кем-то скрыт, то он невидим
	bool isInvisible() const;
	void setVisibility(bool visible, float time = -1.f);

	void setSpecialMinimapMark(bool flag) { specialMinimapSymbolActivated_ = flag; }
	bool isSpecialMinimapMark() const { return specialMinimapSymbolActivated_; }

	/// Запрос на очистку региона от юнитов.
	void clearRegion(Vect2i position, float radius);

	void setImpulseAbnormalState(const AbnormalStateAttribute& state, UnitBase* owner);
	bool isWorking() const;

	/// Режимы атаки. Автоматическое поведение.
	void attackQuant();
	void targetController();	// Анализ и выставление целей.
	void noiseTargetController();
	bool needAiUpdate() const { return !aiScanTimer.busy(); }
	///////////////////////////////////////////////////////////////////////////////////

	void setDirectControl(DirectControlMode mode, bool setFoceMainUnit = true);
	virtual void executeDirectKeys(const UnitCommand& command);
	void setDirectKeysCommand(const UnitCommand& command) { directKeysCommand = command; }
	/// переключение оружия в прямом управлении
	/// direction < 0 - назад, direction >= 0 - вперёд
	bool changeDirectControlWeapon(int direction);
	int directControlWeaponID() const;
	const Vect3f& directControlOffset() const;
	void setActiveDirectControl(DirectControlMode activeDirectControl, int transitionTime = 1000);

	bool canForceFire();

	void wayPointsClear();
	void stop();
		
	// Оружие
	typedef SwapVector<WeaponBase*> Weapons;
	typedef std::vector<WeaponSlot> WeaponSlots;

	void initWeapon();

	float weaponChargeLevel(int weapon_id = 0) const;
	bool hasWeapon(int weapon_id = 0) const;
	WeaponBase* findWeapon(int weapon_id) const;
	const WeaponBase* findWeapon(const char* label) const;

	UnitInterface* fireUnit() const;
	Vect3f firePosition() const;
	Vect3f specialFirePosition() const;
	float targetRadius();
	bool fireDistanceCheck() const; // true - если цель(и) в радиусах атаки.
	bool fireWeaponModeCheck(const WeaponBase* weapon) const;
	bool fireTargetExist() const;
	bool fireTargetDocked() const;
	bool isFireTarget(const UnitInterface* unit) const;
	bool isAttackedFireTarget(const UnitInterface* unit) const;
	bool specialFireTargetExist() const;
	bool isDefaultWeaponExist();
	bool isInSightSector(const UnitBase* target) const;

	bool canFire(int weaponID, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	bool fireRequest();
	bool fireCheck(WeaponTarget& target) const;
	bool fireDistanceCheck(const WeaponTarget& target, bool check_fow = false) const;
	WeaponTarget fireTarget() const;
	/// допустимая дистанция стрельбы по цели
	Rangef fireDistance(const WeaponTarget& target) const;
	void fireStop(int weaponID = -1);
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
	int preSelectedWeaponID() const { return preSelectedWeaponID_; }
	void setPreSelectedWeaponID(int weapon_id) { preSelectedWeaponID_ = weapon_id; }
	bool replaceWeaponInSlot(int slot_index, InventoryItem& item);
	bool removeWeaponFromSlot(int slot_index);
	bool weaponAccessible(const WeaponPrm* weapon) const;
	virtual bool reloadInventoryWeapon(int slot_index){ return false; }
	virtual bool removeInventoryWeapon(int slot_index){ return false; }
	virtual bool updateInventoryWeapon(int slot_index){ return false; }

	UnitInterface* targetUnit() const { return targetUnit_; }
	void setTargetUnit(UnitInterface* targetUnit) { targetUnit_ = targetUnit; }
	void setManualTarget(UnitInterface* unit, bool moveToTarget = true);
	void setTargetPoint(const Vect3f& point, bool aim_only = false, bool moveToTarget = true);
	virtual void clearAttackTarget(int weaponID = 0);
	void clearAutoAttackTargets();
	void updateTargetFromMainUnit(const UnitActing* mainUnit);

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
	bool attackTargetUnreachable() { return attackTargetUnreachable_; }
	void setAttackTargetUnreachable(bool unreachable) { attackTargetUnreachable_ = unreachable; }
	virtual bool canAutoAttack() const { return (unitState() != ATTACK_MODE || !selectedWeaponID_) && (!attr().isTransport() || !attackModeAttr().disableEmptyTransportAttack() || !transportEmpty()); }
	bool isAutoAttackForced();
	bool isAutoAttackDisabled();

	float fireDispersionRadius() const;
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
	WeaponAnimationType getWeaponAnimationType(const WeaponSlot& weaponSlot) const;

	void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics);
	void applyPickedItem(UnitBase* unit);

	void showCircles(const Vect2f& pos) const;
	void showEditor();

	///////////////////////////////////////////////////////////////////////////////////

	// Дамадж
	void setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo = 0);
	virtual void deathGainMultiplicator(ParameterArithmetics& arithmetics) const {}
	bool underAttack() const { return eventAttackTimer_.busy(); }

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
	void reserveSquadNumber(const AttributeBase* attr, int count);

	void setIgnoreFreezedByTrigger(bool ignoreFreezedByTrigger) { ignoreFreezedByTrigger_ = ignoreFreezedByTrigger; }
	static void setFreezedByTrigger(bool freezedByTrigger) { freezedByTrigger_ = freezedByTrigger; }

	const AttributeBase* executedUpgrade() const { return executedUpgradeIndex_ >= 0 ? &*attr().upgrades[executedUpgradeIndex_].upgrade : 0; }

	bool canFireInCurentState() const { return currentState() < CHAIN_FROZEN; }

	void attachUnit(UnitReal* unit);
	void detachUnit(UnitReal* unit);

	// Части тела
	bool hasParts() const { return !bodyParts_.empty(); }
	bool putOnItem(const InventoryItem& inventoryItem);
	void putOffItem(InventoryItem& inventoryItem);
	bool requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const;
	void subResource(const ParameterSet& resource);
	bool accessibleParameter(const ProducedParameters& parameter) const;

	Player* playerPrev() const { return playerPrev_; }

	void relaxLoading();

	LogicTimer* 	fireRequestTimer() { return &fireRequestTimer_;};

	bool productionParameterStarted() { return productionParameterTimer_.busy(); }

	bool traceFireTarget() const;

	const UnitBase* noiseTarget() { return noiseTarget_; }

	static bool freezedByTrigger() { return freezedByTrigger_; }

protected:
	// Оружие
	void weaponQuant();
	void weaponPostQuant();

	virtual UnitInterface* attackTarget(UnitInterface* unit, int weapon_id = 0, bool moveToTarget = true);
	
	bool isMoving_;
	
	WeaponSlots weaponSlots_;
	/// отсортированный по группам и приоритетам список оружия,	через него раздаются цели
	Weapons weapons_;
	bool weaponAnimationMode_;
	int directControlWeaponSlot_;

	/// выбранное для атаки оружие, 0 если не установлено
	int selectedWeaponID_;
	// выбрано оружие в интерфейсе, но атака еще не произошла
	int preSelectedWeaponID_;
	WeaponRotationController weaponRotation_;

	UnitLink<UnitInterface> targetUnit_;  // Юнит для атаки
	Vect3f targetPoint_;  // Точка для атаки
	bool hadTargetUnit_;

	UnitLink<UnitInterface> specialTargetUnit_;  // Юнит для атаки
	Vect3f specialTargetPoint_;  // Точка для атаки

	bool targetPointEnable; // Указана точка атаки?
	bool specialTargetPointEnable; // Указана точка атаки?
	bool attackTargetUnreachable_;

	UnitLink<UnitBase> noiseTarget_; // замеченный источник шума

	AttackMode attackMode_;

	enum DirectControlFireMode
	{
		DIRECT_CONTROL_PRIMARY_WEAPON	= 1,
		DIRECT_CONTROL_SECONDARY_WEAPON = 2,
		SYNDICAT_CONTROL_AIM_WEAPON	= 4,
	};

	int directControlFireMode_;

	DirectControlMode directControl_;
	DirectControlMode activeDirectControl_;

	// Режимы атаки.
	LogicTimer aiScanTimer;
	LogicTimer noiseScanTimer_;

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
	LogicTimer callFinishProductionTimer_;
	int totalProductionCounter_;
	bool teleportProduction_;
	bool wasProduced_;
	bool squadReserved_;
	
	const ProducedParameters* producedParameter_;
	LogicTimer productionParameterTimer_;

	AbnormalStateAttribute impulseAbnormalState_;
	UnitLink<UnitBase> impulseOwner_;

	int executedUpgradeIndex_;
	int upgradeTime_;
	int previousUpgradeIndex_;
	int finishUpgradeTime_;

	int rotationDeath;

	struct BodyPart	
	{
		const BodyPartAttribute& partAttr;
		ParameterSet parameters; // Текущие значения параметров надетого предмета
		AttributeItemInventoryReference item; // Атрибуты надетого предмета

		int visibilitySet;
		int visibilityGroup;

		BodyPart(const BodyPartAttribute& partAttr);
		void putOn(const InventoryItem& inventoryItem, const BodyPartAttribute::Garment& garment);
		void putOff();
		void putOff(InventoryItem& inventoryItem);
		void serialize(Archive& ar);
	};
	typedef vector<BodyPart> BodyParts;
	BodyParts bodyParts_;

	const Vect2f& shipmentPosition() const { return shipmentPosition_; }
	void setShipmentPosition(const Vect2f& shipmentPosition) { shipmentPosition_ = shipmentPosition; }
	void cancelUnitProduction(ProduceItem& item);
	void cancelProduction();

	void commandsQueueQuant();
	
private:
	int flyingReason_;
	int staticReason_;
	
	LogicTimer eventAttackTimer_;
	LogicTimer fireRequestTimer_;

	bool specialMinimapSymbolActivated_;
	bool isInvisiblePrev_;
	bool isUnderWaterSilouettePrev_;

	bool ignoreFreezedByTrigger_;
	static bool freezedByTrigger_;

	AttributeBase* beforeUpgrade_;

	UnitCommand directKeysCommand;
	
	typedef vector<UnitLink<UnitReal> > RealsLinks;
	RealsLinks dockedUnits;

	int selectionListPriority_;

	ShowChangeParametersController showParamController_;

	LogicTimer visibleTimer_;
	LogicTimer unVisibleTimer_;

	bool fireRequest(WeaponTarget& target, bool no_movement_weapons_only = false);
	bool fireRequestAuto();

	void updatePart(const BodyPart& part);
};

#endif //__UNIT_ACTING_H__
