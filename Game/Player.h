#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Network\NetPlayer.h"
#include "Units\UnitPad.h"
#include "Units\Triggers.h"
#include "TriggerEditor\TriggerExport.h"
#include "PlayerStatistics.h"
#include "Starforce.h"

class Event;
class UnitBuilding;
class UnitSquad;
class UnitObjective;
class UnitActing;
class FogOfWarMap;
class FieldDispatcher;

class WBuffer;

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
	const wchar_t* name() const { return name_.c_str(); }

	int teamIndex() const { return teamIndex_; }
	bool teamMode() const { return teamMode_; }
	const wchar_t* teamName(int index) const { return teamNames_[index].c_str(); }

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

	SquadList& squads() { return squads_; }
	const SquadList& squadList(const class AttributeSquad* attribute) const;

	int unitNumber(ParameterTypeReferenceZero type) const;
	int unitNumberMax(ParameterTypeReferenceZero type) const;
	int unitNumberMaxReal(ParameterTypeReferenceZero type) const;
	int unitNumberReserved() const { return unitNumberReserved_; }
	void reserveUnitNumber(const AttributeBase* attribute, int counter);
	int checkUnitNumber(const AttributeBase* attribute, const AttributeBase* upgradingAttribute = 0) const;
	bool accessible(const AttributeBase* attribute) const;
	bool accessibleByBuildings(const AttributeBase* attribute) const;
	void printAccessible(WBuffer& out, const AccessBuildingsList& buildingsList, const wchar_t* enabledColor, const wchar_t* disabledColor) const;
	const RealUnits& realUnits(const AttributeBase* attribute) const; // Юниты и здания
	UnitActing* findFreeFactory(const AttributeBase* unitAttribute, int priority = 0);
	
	bool accessibleByBuildings(const ProducedParameters& parameter) const;

	bool weaponAccessible(const WeaponPrm* weapon) const;
	const wchar_t* printWeaponAccessible(WBuffer& buffer, const WeaponPrm* weapon, const wchar_t* enabledColor, const wchar_t* disabledColor) const;

	void addUniqueParameter(const char* parameterName, int value = 0);
	void removeUniqueParameter(const char* parameterName);
	bool checkUniqueParameter(const char* parameterName, int value = 0) const;

	//-----------------------------
	int countUnits(const AttributeBase* attribute) const;
	int countUnitsConstructed(const AttributeBase* attribute) const;
	int countSquads(const AttributeSquad* attribute) const;
	//-----------------------------
	virtual void Quant();
	void triggerQuant(bool pause);
	void checkEvent(const Event& event);

	void interfaceToggled();

	void sendCommand(const UnitCommand& command);
	void executeCommand(const UnitCommand& command);

	UnitBase* buildUnit(const AttributeBase* attr);
	Accessibility canBuildStructure(const AttributeBase* buildingAttr, const AttributeBase* attrSquad = 0) const;
	UnitBuilding* buildStructure(const AttributeBase* buildingAttr, const Vect3f& pos, bool checkPosition = true);
	void addUnit(UnitBase* p);
	void removeFromRealUnits(UnitObjective* unit);
	void removeUnit(UnitBase* unit);
	void removeUnitAccount(UnitInterface* unit);
	void addResourceCapacity(const ParameterSet& capacity) { resourceCapacity_ += capacity; }
	void subClampedResourceCapacity(const ParameterSet& capacity);

	const AttributeCache* attributeCache(const AttributeBase* attribute) const; // Кешируются только юниты, здания, предметы и сквады
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

	UnitPlayer* playerUnit() const { return playerUnit_; }
	float currentCapacity() const { return playerUnit_ ? playerUnit_->currentCapacity() : 0.f; }

	void startUsedByTriggerAttack(int time);
	bool isUsedByTriggerAttack() { return  usedByTriggerTimer_.busy(); }

	//-------------------------------------------------------
	int colorIndex() const { return colorIndex_; }
	int signIndex() const { return signIndex_; }
	const Color4f& unitColor() const { return unitColor_; }
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

	FieldDispatcher* fieldDispatcher() const { return fieldDispatcher_; }

	//-------------------------------------------------------
	void setActivePlayer(int cooperativeIndex);
	void SetDeactivePlayer();

	void updateSkinColor();

	IntVariables& intVariables() { return intVariables_; }

	void startTigger(const TriggerChainName& triggerChainName);

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
	bool shootPointValid() const;
	Vect2f shootPoint2D() const;

	bool updateShootPoint() const;
	void setUpdateShootPoint();

	bool shootFailed() const { return shootFailed_; }
	void setShootFailed(bool state){ shootFailed_ = state; }

	Vect2f centerPosition() const { return centerPosition_; }
	float maxDistance() const { return maxDistance_; }

	const AttackModeAttribute& attackMode() const { return attackMode_; }

	const Anchor* requestedAssemblyPosition() const;
	void showAssemblyCommand(const Vect3f& pos, unsigned int data);
	void directShootCommand(const Vect3f& pos, unsigned int data);
	void directShootCommandMouse(const Vect3f& pos);

	void drawDebug2D(XBuffer& buffer);

private:
	MTSection units_lock;
	UnitList units_;
	SquadList squads_;
	RealUnits realUnits_;

	LogicTimer usedByTriggerTimer_;
	UnitLink<UnitPlayer> playerUnit_;

	typedef list<TriggerChain> TriggerChains;
	TriggerChains triggerChains_;
	TriggerChainNames triggerChainNames_; 
	CameraSplineName startCamera_;

	PlayerStatistics playerStatistics_;
	Scores* scores_;

	bool active_;
	bool controlEnabled_;
	bool triggersDisabled_;
	int playerID_;
	int shuffleIndex_;
	int clan_;
	Difficulty difficulty_;
	wstring name_;
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
	wstring teamNames_[NETWORK_TEAM_MAX];

	bool hasAttackMode_;
	AttackModeAttribute attackMode_;

	Color4f unitColor_;
	UnitColor underwaterColor_;
	std::string unitSign_;

	typedef StaticMap<const AttributeBase*, RealUnits> RealUnitsMap;
	RealUnitsMap realUnitsMap_;

	typedef StaticMap<const AttributeSquad*, SquadList> SquadsTypeMap;
	SquadsTypeMap squadsTypeMap_;

	FogOfWarMap* fogOfWarMap_;

	FieldDispatcher* fieldDispatcher_;

	IntVariables intVariables_;

	ParameterSet resource_;
	ParameterSet resourcePrev_;
	ParameterSet resourceDelta_;
	ParameterSet resourceCapacity_;

	int unitNumber_;
	int unitNumberReserved_;
	ParameterSet unitNumberByType_;
	ParameterSet unitNumberReservedByType_;

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
	// надо слать координаты мышикаждый квант
	unsigned short shootPositionNeedUpdate_;
	// время показа точки сбора
	LogicTimer showAssemblyCommandTimer_;
	// точка сбора
	UnitLink<Anchor> assemblyPosition_;
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

#define CUNITS_LOCK(player) MTAuto lock_player_units((player)->UnitsLock())

#endif //__PLAYER_H__
