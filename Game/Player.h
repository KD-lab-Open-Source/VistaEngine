#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "..\Network\NetPlayer.h"
#include "..\Units\RealUnit.h"
#include "..\Units\Triggers.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "..\Units\UnitNumberManager.h"
#include "PlayerStatistics.h"
#include "Starforce.h"

class Event;
class UnitBuilding;
class UnitSquad;
class UnitObjective;
class UnitActing;
class FogOfWarMap;

typedef SwapVector<UnitSquad*> SquadList;
typedef SwapVector<UnitObjective*> RealUnits;

typedef vector<PointerWrapper<Player> > PlayerVect;

typedef StaticMap<string, int> IntVariables;

struct CameraSplineName : string
{
	CameraSplineName(const char* nameIn = "") : string(nameIn) {}
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

enum AuxPlayerType {
	AUX_PLAYER_TYPE_ORDINARY_PLAYER,
	AUX_PLAYER_TYPE_COMMON_FRIEND,
	AUX_PLAYER_TYPE_COMMON_ENEMY
};

struct PlayerDataEdit : PlayerData
{
	AuxPlayerType auxPlayerType;
	TriggerChainNames triggerChainNames;
	CameraSplineName startCamera;

	bool hasAutomaticAttackMode;
	AttackModeAttribute attackMode;

	PlayerDataEdit();
	void serialize(Archive& ar);
};


///////////////////////////////////////
//			Игрок
///////////////////////////////////////
class Player
{
public:
	Player(const PlayerData& playerData);
	virtual ~Player();

	//-----------------------------
	void addCooperativePlayer(const PlayerData& playerData);
	STARFORCE_API void createRandomScenario(const Vect2f& location);

	bool triggersDisabled() const { return triggersDisabled_; }
	void setTriggersDisabled() { triggersDisabled_ = true; }

	void getPlayerData(PlayerDataEdit& playerData) const;
	void setPlayerData(const PlayerDataEdit& playerData);

	const PlayerStatistics& playerStatistics() const { return playerStatistics_; }
	
	//-----------------------------
	bool active() const { return active_; }
	
	bool controlEnabled() const { return controlEnabled_; }
	void setControlEnabled(bool controlEnabled) { controlEnabled_ = controlEnabled; }

	int playerID() const { return playerID_; }
	int shuffleIndex() const { return shuffleIndex_; }
	bool isWorld() const { return isWorld_;	}
	const Race& race() const { return race_; }
	const char* name() const { return name_.c_str(); }

	int teamIndex() const { return teamIndex_; }
	bool teamMode() const { return teamMode_; }
	const char* teamName(int index) const { return teamNames_[index].c_str(); }

	AuxPlayerType auxPlayerType() const { return auxPlayerType_; }
	void setAuxPlayerType(AuxPlayerType auxPlayerType) { auxPlayerType_ = auxPlayerType; }

	RealPlayerType realPlayerType() const { return realPlayerType_; }
	void setRealPlayerType(RealPlayerType realPlayerType) { realPlayerType_ = realPlayerType; }

	int clan() const { return clan_; }
	void setClan(int clan) { clan_ = clan; }

	Difficulty difficulty() const { return difficulty_; }
	
	const ParameterSet& resource() const { return resource_; }
	const ParameterSet& resourceDelta() const { return resourceDelta_; }
	const ParameterSet& resourceCapacity() const { return resourceCapacity_; }
	void addResource(const ParameterSet& resource, bool registerEvent = false);
	bool requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const; // проверка на наличие ресурса 
	void subResource(const ParameterSet& resource); 

	SquadList& squadList() { return squads; }
	UnitSquad* squad(int index) { MTL(); return index < squads.size() ? squads[index] : 0; }

	int unitNumber() const { return legionariesAndBuildings_; }
	int unitNumberMax() const;
	void reserveUnitNumber(const AttributeBase* attribute, int counter);
	int checkUnitNumber(const AttributeBase* attribute, const AttributeBase* upgradingAttribute = 0) const;
	bool accessible(const AttributeBase* attribute) const;
	bool accessibleByBuildings(const AttributeBase* attribute) const;
	void printAccessible(XBuffer& out, const AccessBuildingsList& buildingsList, const char* enabledColor, const char* disabledColor) const;
	const RealUnits& realUnits(const AttributeBase* attribute) const; // Юниты и здания
	UnitActing* findFreeFactory(const AttributeBase* unitAttribute);
	
	bool accessibleByBuildings(const ProducedParameters& parameter) const;

	bool weaponAccessible(const WeaponPrm* weapon) const;
	string printWeaponAccessible(const WeaponPrm* weapon, const char* enabledColor, const char* disabledColor) const;

	void addUniqueParameter(const char* parameterName, int value = 0);
	void removeUniqueParameter(const char* parameterName);
	bool checkUniqueParameter(const char* parameterName, int value = 0) const;

	//-----------------------------
	int countUnits(const AttributeBase* attribute) const;
	int countUnitsConstructed(const AttributeBase* attribute) const;
	int countSquads(const AttributeBase* attribute) const;
	//-----------------------------
	virtual void Quant();
	void triggerQuant(bool pause);
	void checkEvent(const Event& event);

	void interfaceToggled();

	void CollisionQuant();

	void refreshAttribute();

	void sendCommand(const UnitCommand& command);
	void executeCommand(const UnitCommand& command);

	UnitBase* buildUnit(const AttributeBase* attr);
	Accessibility canBuildStructure(const AttributeBase* buildingAttr) const;
	UnitBuilding* buildStructure(const AttributeBase* buildingAttr, const Vect3f& pos, bool checkPosition = true);
	void addUnit(UnitBase* p);
	void removeFromRealUnits(UnitObjective* unit);
	void removeUnit(UnitBase* unit);
	void removeObjectiveUnit(UnitObjective* unit);
	void addResourceCapacity(ParameterSet capacity) { resourceCapacity_ += capacity; }
	void subClampedResourceCapacity(ParameterSet capacity);

	const AttributeCache* attributeCache(const AttributeBase* attribute) const; // Кешируются только юниты, здания, предметы
	void applyParameterArithmetics(const AttributeBase* attribute, const ParameterArithmetics& arithmetics);

	const WeaponPrmCache* weaponPrmCache(const WeaponPrm* prm) const;
	
	void killAllUnits();

	void showEditor();

	UnitReal* findUnit(const AttributeBase* attr);
	typedef BitVector<UnitsConstruction> ConstructionState;
	UnitReal* findUnit(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin = 0, const ConstructionState& state = CONSTRUCTED | CONSTRUCTING, bool onlyVisible = false);
	UnitReal* findUnitByLabel(const char* label);

	bool isEnemy(const Player* player) const;
	bool isEnemy(const UnitBase* unit) const;

	//-------------------------------------------------------
	int colorIndex() const { return colorIndex_; }
	int signIndex() const { return signIndex_; }
	const sColor4f& unitColor() const { return unitColor_; }
	const UnitColor& underwaterColor() const { return underwaterColor_; }
	const char* unitSign() const {return unitSign_.c_str(); }
	void setColorIndex(int colorIndex);
	void setSignIndex(int signIndex);

	void setModelSkinColor(cObject3dx* model) { model->SetSkinColor(unitColor(), unitSign()); }

	int silhouetteColorIndex() const { return silhouetteColorIndex_; }
	int underwaterColorIndex() const { return underwaterColorIndex_; }
    void setSilhouetteColorIndex(int silhouetteColorIndex);
	void setUnderwaterColorIndex(int underwaterColorIndex);
	void initFogOfWarMap();
	FogOfWarMap* fogOfWarMap() const { return fogOfWarMap_; }

	//-------------------------------------------------------
	void setActivePlayer(int cooperativeIndex);
	void SetDeactivePlayer();

	void updateSkinColor();

	IntVariables& intVariables() { return intVariables_; }

	// ----- AI Interface ---------
	bool isAI() const { return isAI_; }
	virtual void setAI(bool isAI);
	
	// ----- Defeat Interface -------
	bool isWin() const { return isWin_;}
	void setWin(); 

	bool isDefeat() const { return isDefeat_;}
	void setDefeat(); 
	//-----------------------------------------
	virtual void showDebugInfo();

	TriggerChain* getStrategyToEdit();

    //-----------------------------------------
	const UnitList& units() const {	xassert(MT_IS_LOGIC() || units_lock.locked()); return units_; }
	const RealUnits& realUnits() const {	xassert(MT_IS_LOGIC() || units_lock.locked()); return realUnits_; }

	MTSection& UnitsLock() { return units_lock; }

	STARFORCE_API void serialize(Archive& ar);
	void relaxLoading();

	bool shootKeyDown() const { return shootKeyDown_; }
	Vect2f shootPoint2D() const;
	bool shootFailed() const { return shootFailed_; }
	void setShootFailed(bool state){ shootFailed_ = state; }

	Vect2f centerPosition() const { return centerPosition_; }
	float maxDistance() const { return maxDistance_; }

	const AttackModeAttribute& attackMode() const { return attackMode_; }

private:
	MTSection units_lock;
	UnitList units_;
	SquadList squads;
	RealUnits realUnits_;

	typedef list<TriggerChain> TriggerChains;
	TriggerChains triggerChains_;
	TriggerChainNames triggerChainNames_; 
	CameraSplineName startCamera_;

	PlayerStatistics playerStatistics_;

	bool active_;
	bool controlEnabled_;
	bool triggersDisabled_;
	int playerID_;
	int shuffleIndex_;
	int clan_;
	Difficulty difficulty_;
	string name_;
	bool isWorld_;
	Race race_;
	int colorIndex_;
	int silhouetteColorIndex_;
	int underwaterColorIndex_;
	int signIndex_;
	AuxPlayerType auxPlayerType_;
	RealPlayerType realPlayerType_;
	bool isAI_;
	bool isDefeat_;
	bool isWin_;

	bool teamMode_;
	int teamIndex_;
	string teamNames_[NETWORK_TEAM_MAX];

	bool hasAttackMode_;
	AttackModeAttribute attackMode_;

	sColor4f unitColor_;
	UnitColor underwaterColor_;
	std::string unitSign_;

	typedef StaticMap<const AttributeBase*, RealUnits> RealUnitsMap;
	RealUnitsMap realUnitsMap_;

	FogOfWarMap* fogOfWarMap_;

	IntVariables intVariables_;

	ParameterSet resource_;
	ParameterSet resourcePrev_;
	ParameterSet resourceDelta_;
	ParameterSet resourceCapacity_;

	UnitNumberManager unitNumberManager_;
	int legionariesAndBuildings_;
	int unitNumberReserved_;

	typedef StaticMap<string, AttributeCache> AttributeCacheMap;
	AttributeCacheMap attributeCacheMap_;

	typedef vector<WeaponPrmCache> WeaponPrmCacheVector;
	WeaponPrmCacheVector weaponPrmCache_;

	// нажата кнопка стрельбы
	bool shootKeyDown_;
	// предыдущее и текущее положение стрельбы
	Vect3f shootPosition_[2];
	// время с момента обновления в логических квантах
	unsigned short shootPositionUpdateTime_;
	/// нельзя стрельнуть с нажатой кнопкой
	bool shootFailed_;

	/// индекс юнита, для которого вызывать квант microAI
	int aiUpdateIndex_;

	void calculateResourceDelta();

	// время с момента обновления в логических квантах
	unsigned short centerPositionUpdateTime_;
	// центр базы игрока
	Vect2f centerPosition_;
	// максимальный радиус базы
	float maxDistance_;

	void centerPositionQuant();
};

template<>
class ObjectCreator<Player, Player> 
{
public:
	static Player* create() { // пока заглушка - игроки создаются addPlayer
		xassert(0);
		return 0;
	}
};

#define CUNITS_LOCK(player) MTAuto lock_player_units((player)->UnitsLock())

#endif //__PLAYER_H__
