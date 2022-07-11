#ifndef __IRONBUILDING_H__
#define __IRONBUILDING_H__

#include "UnitActing.h"

class Archive;

enum BuildingStatus
{
	BUILDING_STATUS_CONSTRUCTED = 1, // Построено
	BUILDING_STATUS_PLUGGED_IN = 2, // Включен в интерфейсе
	BUILDING_STATUS_CONNECTED = 4, // Здание подключено: Я-Т посредством воздушных связей, остальные - через зеропласт
	BUILDING_STATUS_ENABLED = 8, // Разрешено деревом развития
	BUILDING_STATUS_POWERED = 16, // Получает энергию, не выводится иконка отсутствия энергии
	BUILDING_STATUS_UPGRADING = 32, // Апгрейдится в данный момент
	BUILDING_STATUS_MOUNTED = 64, // Разложено
	BUILDING_STATUS_HOLD_CONSTRUCTION = 128 // Остановлено строительство
};

class AttributeBuilding : public AttributeBase
{
public:
	enum PlacementMode 
	{
		PLACE_ON_GROUND_OR_WATER,
		PLACE_ON_GROUND,
		PLACE_ON_GROUND_DELTA,
		PLACE_ON_GROUND_DELTA_HEIGTH,
		PLACE_ON_WATER_DEPTH, 
		PLACE_ON_GROUND_OR_UNDERWATER,
		PLACE_ON_GROUND_DELTA_UNDERWATER
	};

	enum InteractionType 
	{
		INTERACTION_PHANTOM = ENVIRONMENT_PHANTOM,
		INTERACTION_TREE = ENVIRONMENT_TREE,
		INTERACTION_FENCE = ENVIRONMENT_FENCE, 
		INTERACTION_FENCE2 = ENVIRONMENT_FENCE2, 
		INTERACTION_BARN = ENVIRONMENT_BARN,
		INTERACTION_BUILDING = ENVIRONMENT_BUILDING,
		INTERACTION_BIG_BUILDING = ENVIRONMENT_BIG_BUILDING,
		INTERACTION_NORMAL = ENVIRONMENT_INDESTRUCTIBLE
	};

	InteractionType interactionType;
	PlacementMode placementMode;
	PlacementZone placementZone;
	int placementDeltaHeight;
	int placementHeight;
	bool checkUndestructability;
	bool analyzeTerrain;
	float deviationCosMin;
	int cancelConstructionTime;

	bool installerLight;

	TerToolCtrl toolzer;
	bool killAfterToolzerFinished;

	bool teleport;
	AttributeUnitReferences teleportedUnits;
	int teleportationTime;

	bool includeBase;

	AttributeBuilding();

	void serialize(Archive& ar);

	bool checkPlacementZone(const Vect2f& position, Player* player, Vect2f* snapPosition_) const;
	bool checkBuildingPosition(const Vect2f& position, const Mat2f& orientation, Player* player, bool checkUnits, Vect2f& snapPosition_) const;
	float buildingPositionZ(const Vect2f& position) const;
	bool canTeleportate(const UnitBase* unit) const;
};

//--------------------------------------
class UnitBuilding : public UnitActing
{
public:
	UnitBuilding(const UnitTemplate& data);
	virtual ~UnitBuilding();

	const AttributeBuilding& attr() const {
		return safe_cast_ref<const AttributeBuilding&>(__super::attr());
	}

	void Kill();
	void Quant();
	
	void setPose(const Se3f& pose, bool initPose);

	void startConstruction(bool afterUpgrade = false);
	bool addResourceToBuild(const ParameterSet& resource);
	float constructionsSpeedFactor() const;
	float constructionProgress() const { return isConstructed() ? 1 : buildConsumer_.progress(); }
	UnitLegionary* constructor() const { return constructor_; }
	void setConstructor(UnitLegionary* unit); 
	int buildersCounter() const { return buildersCounter_; }

	void executeCommand(const UnitCommand& command);

	void uninstall();

	void changeUnitOwner(Player* player);

	int buildingStatus() const { return buildingStatus_; }
	void setBuildingStatus(int st){ buildingStatus_ = st; }
	
	bool isConstructed() const { return buildingStatus() & BUILDING_STATUS_CONSTRUCTED; }
	bool isConnected() const { return buildingStatus() & BUILDING_STATUS_CONNECTED; }

	void collision(UnitBase* unit, const ContactInfo& contactInfo);
	bool checkInPathTracking(const UnitBase* tracker) const;

	void showDebugInfo();

	void graphQuant(float dt);

	bool basementInstalled() const { return basementInstalled_; }
	void installBasement();
	void uninstallBasement();

	void serialize(Archive& ar);

	/// может ли произвести кого-то для добычи из этого ресурса
	bool canExtractResource(const UnitItemResource* item) const;

	/// может ли произвести кого-то для достройки здания
	bool canBuild(const UnitReal* building) const;

	bool constructionInProgress() const { return constructionInProgressTimer_(); } // идет строительство и вкачивается ресурс

	UnitReal* findTeleport() const;

private:
	BitVector<BuildingStatus> buildingStatus_;
	ParameterConsumer buildConsumer_;
	DurationTimer cancelConstructionTimer_;
	DurationTimer constructionInProgressTimer_;
	int buildersCounter_;

	bool basementInstalled_;
	Vect2f basementInstallPosition_;
	float basementInstallAngle_;

	TerToolCtrl toolzer_;
	bool toolzerFinished_;

	bool resourceCapacityAdded_;

	UnitLink<UnitLegionary> constructor_;

	int teleportID_;
};

#endif //__IRONBUILDING_H__
