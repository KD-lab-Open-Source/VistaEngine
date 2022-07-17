#ifndef __IRONLEGION_H__
#define __IRONLEGION_H__

#include "UnitActing.h"
//#include "AttributeSquad.h"

class Squad;
class UnitTrail;
class AttributeSquad;
class cWaterPlume;

class PlumeNode
{
public:
	PlumeNode();

	int node() const { return node_; }
	float radius() const {return radius_; }

	void serialize(Archive& ar);

private:
	Logic3dxNode node_;
	float radius_;
};

typedef vector<PlumeNode> PlumeNodes;

////////////////////////////////////////

class AttributeLegionary : public AttributeBase
{
public:
	//UnitFormationTypeReference formationType;
	StringTableReference<AttributeSquad, true> squad;
	AttributeItemReferences pickedItems;

	ParameterCustom runningConsumption;
	
	// ������������� 
	AttributeBuildingReferences constructedBuildings;
	ParameterCustom constructionPower;
	ParameterCustom constructionCost;
	float constructionProgressPercent;

	struct Level
	{
		ParameterCustom parameters;
		ParameterArithmetics arithmetics;
		float deathGainFactor;
		ParameterArithmeticsMultiplicator deathGainTypesFactors;
		ParameterArithmeticsMultiplicator deathGainTypesFactorDirectControl;
		EffectAttributeAttachable levelEffect;
		EffectAttributeAttachable levelUpEffect;
		UI_UnitSprite sprite;
		Vect2f spriteOffset;
		float spriteScale;
		SoundReference levelUpSound;
		Level();
		void serialize(Archive& ar);
	};
	typedef vector<Level> Levels;
	Levels levels;

	// ������ ��������
	bool resourcer;
	vector<ParameterCustom> resourcerCapacities;
	AttributeReferences resourceCollectors;

	bool grassTrack;

	PlumeNodes plumeNodes;
	EffectReference sprayEffect;
	float sprayEffectScale;
	bool sprayEffectOnWater;

	/// �������������� ������� � ���������
	bool autoFindTransport;

	float directControlSightRadiusFactor;

	AttributeLegionary();

	void serialize(Archive& ar);

	bool canPick(const UnitBase* item) const;
	bool canBuild(const UnitBase* unit) const;
	
	int resourcerCapacity(const ParameterSet& itemParameters) const; // -1, ���� �� �������
	bool canPickResource(const ParameterTypeReference& type) const;

	float radius() const;
};

/// ����� �� �����
class TraceController
{
public:
	TraceController() : owner_(0) { nodeIndex_ = -1; surfaceKind_ = SURFACE_KIND_1 | SURFACE_KIND_2 | SURFACE_KIND_3 | SURFACE_KIND_4; isUpdated_ = false; wheelNode_ = false; }
	TraceController(UnitReal* owner, const TerToolCtrl& ctrl, unsigned surface, int nodeIndex = 0, bool wheelNode = false);

	void start();
	void stop();
	void update();

	void showDebugInfo();

private:

	bool isUpdated_;
	bool wheelNode_;

	unsigned surfaceKind_;

	/// ���� ��� ��������� ���������
	int nodeIndex_;
	TerToolCtrl  traceCtrl_;

#ifndef _FINAL_VERSION_
	Vect3f lastPosition_;
#endif

	UnitReal* owner_;

	const Se3f& position();
};

////////////////////////////////////////////////////////////////////////////////
//	��������� � �������
////////////////////////////////////////////////////////////////////////////////
enum LegionFireStatus
{
	LEGION_FIRE_STATUS_GROUND_OBSTACLE = 1,
	LEGION_FIRE_STATUS_FRIENDLY_FIRE = 2,
	LEGION_FIRE_STATUS_RELOAD_AMMO = 4,
	LEGION_FIRE_STATUS_DISTANCE = 8,
	LEGION_FIRE_STATUS_FIELD_OBSTACLE = 32,
	LEGION_FIRE_STATUS_HORIZONTAL_ANGLE = 64,
	LEGION_FIRE_STATUS_VERTICAL_ANGLE = 128,
	LEGION_FIRE_STATUS_BORDER_ANGLE = 256,
	LEGION_FIRE_STATUS_BAD_TARGET = 512,
	LEGION_FIRE_STATUS_NOT_ENEMY_TARGET = 1024
};

enum LegionActionStatus
{
	LEGION_ACTION_STATUS_MOVE = 1,
	LEGION_ACTION_STATUS_ATTACK = 2,
	LEGION_ACTION_STATUS_FREEZE = 4
};

class UnitLegionary : public UnitActing
{
public:
	UnitLegionary(const UnitTemplate& data);
	~UnitLegionary();

	const AttributeLegionary& attr() const { return safe_cast_ref<const AttributeLegionary&>(__super::attr()); }
	const AttributeBase* selectionAttribute() const { return &attr(); }

    void Quant();			
	void Kill();
	void executeCommand(const UnitCommand& command);
	void collision(UnitBase* unit, const ContactInfo& contactInfo);
	void changeUnitOwner(Player* player);
	
	void addParameters(const ParameterSet& parameters);
	int level() const { return level_; }
	float levelProgress() const;

	float sightRadius() const;

	int requestStatus() const { return requestStatus_; }
	int fireStatus() const { return fireStatus_; }

	void setSquad(class UnitSquad* p);
	UnitSquad* squad() const { return squad_; }
	UnitSquad* getSquadPoint() { return squad_; }

	bool selectAble() const;
	void setSelectAble(bool selectAble) { selectAble_ = selectAble; }

	void clearOrders();
	float formationRadius() const;
	
	void setInSquad();
	bool inSquad() const { return inSquad_;	}

	void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics);

	bool setResourceItem(UnitInterface* resourceItem);

	void setTransport(UnitActing* transport, int transportSlotIndex);
	bool setTransportAuto();
	void clearTransport();
	void putInTransport();
	void putUnitOutTransport();
	UnitActing* transport() const;
	bool inTransport() const { return currentState() == CHAIN_IN_TRANSPORT; }
	bool isMovingToTransport() const { return isMovingToTransport_; }
	bool canFireInCurentState() const { return (__super::canFireInCurentState() && !isMovingToTransport()) || (inTransport() && canFireInTransport()); }
	bool canFireInTransport() const { return canFireInTransport_; }
	bool directControlPrev() const { return directControlPrev_; }
	void setDirectControlPrev(bool directControlPrev) { directControlPrev_ = directControlPrev; }

	void setTeleport(UnitReal* teleport, UnitReal* telportTo);
	const UnitReal* teleport() const {return teleport_; }
	const UnitReal* teleportTo() const {return teleportTo_; }
	void clearTeleport() { teleport_ = 0; }
	void clearTeleportTo() { teleportTo_ = 0; }

	bool setConstructedBuilding(UnitReal* constructedBuilding, bool queued);
	bool constructingBuilding() const { return constructedBuilding_; }

	bool gatheringResource() const {return resourceItem_ != 0; }

	/// ����� �� �������� �� ����� �������
	bool canExtractResource(const UnitItemResource* item) const;

	/// ����� �� ��������� ��� ��������� ��� ������
	bool canBuild(const UnitReal* building) const;

	/// ����� �� ���������� ������ ��� ������� �� ���
	bool canRun() const;

	bool uniform(const UnitReal* unit) const;
	bool prior(const UnitInterface* unit) const;

	// ������ � ���������

	/// ��������, ������ �� ������� � ���������.
	bool canPutToInventory(const UnitItemInventory* item) const;
	/// ����� ������� � ���������.
	/// ���� �� ����, ���������� false.
	bool putToInventory(const UnitItemInventory* item);

	const InventorySet* getInventory() const { return &inventory_; }

	/// ������������� �������, ������� ���� ���������.
	/// ���� ��������� ������ (�� ����� � ���������), ���������� false.
	bool setItemToPick(UnitBase* item);

	void setTrail(UnitTrail* trail, const Vect3f& point);

	void serialize(Archive& ar);

	void showDebugInfo();
	void graphQuant(float dt);

	enum ResourcerMode
	{
		RESOURCER_IDLE,
		RESOURCER_FINDING,
		RESOURCER_PICKING,
		RESOURCER_RETURNING
	};

	bool isTeleporting() const { return teleport_ != 0; }
	bool isWorking() const { return __super::isWorking() || resourcerMode_ != RESOURCER_IDLE || teleport_ || constructedBuilding_; }
	bool canAutoAttack() const { return (__super::canAutoAttack() && (resourcerMode_ == RESOURCER_IDLE || autoAttackMode() == ATTACK_MODE_OFFENCE) && !teleport_ && !constructedBuilding_);}
	float forwardVelocity();
	void velocityProcessing();

	void setLevel(int newLevel, bool applyArithmetics);
	void deathGainMultiplicator(ParameterArithmetics& arithmetics) const;
	bool setState(UnitLegionary* unit);
	void addWayPoint(const Vect2f& point, bool enableMoveMode = true);

	bool nearConstructedBuilding() const { return nearConstructedBuilding_; }
	const ResourcerMode& resourcerMode() const { return resourcerMode_; }
	UnitReal* resourceItem() const {return resourceItem_; }
	const UnitInterface* resourceCollector() const {return resourceCollector_; }
	int resourcerCapacityIndex() { return resourcerCapacityIndex_; }
	void giveResource();
	
protected:
	void checkLevel(bool applyArithmetics);
	void resourcerQuant();

	bool canAutoMove() const;

private:
	UnitLink<UnitSquad> squad_;
	
	int level_;

	int requestStatus_;
	int fireStatus_;
		
	bool inSquad_;
	bool selectAble_;

	/// true ���� ���� ��� ����� �������
	bool manualAttackTarget_;
	DurationTimer targetEventTimer_;

	/// true ���� ���� ��� ����� - ����� �� ����
	bool hasAttackPosition_;
	Vect3f attackPosition_;

	bool onWater;

	ResourcerMode resourcerMode_;
	UnitLink<UnitReal> resourceItem_;
	UnitLink<UnitInterface> resourceCollector_;
	ParameterConsumer resourcePickingConsumer_;
	ParameterSet resourcerCapacity_;
	int resourcerCapacityIndex_;
	DurationTimer resourceFindingPause_;
	float resourceCollectorDistance_;
	int resourceCollectorI_, resourceCollectorJ_;
	
	UnitLink<UnitActing> transport_;
	int transportSlotIndex_;
	bool isMovingToTransport_;
	ParameterArithmetics additionToTransportInv_;
	bool canFireInTransport_;
	bool directControlPrev_;
	DurationTimer transportFindingPause_;

	UnitLink<UnitReal> teleport_;
	UnitLink<UnitReal> teleportTo_;
	
	UnitLink<UnitReal> constructedBuilding_;
	bool nearConstructedBuilding_;
	typedef vector<UnitLink<UnitReal> > ConstructedBuildings;
	ConstructedBuildings constructedBuildings_;

	/// ���������
	InventorySet inventory_;

	/// �������, ������� ���� ���������
	UnitLink<UnitItemInventory> itemToPick_;

	typedef std::vector<TraceController> TraceControllers;
	TraceControllers traceControllers_;
	bool traceStarted_;

	cWaterPlume* waterPlume;

	//-------------------
	friend UnitSquad;
};

#endif //__IRONLEGION_H__
