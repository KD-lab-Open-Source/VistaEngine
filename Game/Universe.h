#ifndef __UNIVERSE_H__
#define __UNIVERSE_H__

#include "..\Units\BaseUnit.h"
#include "Player.h"
#include "..\Util\UniqueContainer.h"

class Player;
class CrashSystem;
struct TriggerDispatcher;
class netCommandGame;
class MultiBodyDispatcher;
class SecondMap;
class cTransparentTracking;
class FowUnitDispatcher;
struct MergeOptions;
class XPrmIArchive;
class XPrmOArchive;
struct Action;

typedef void(*PROGRESSCALLBACK)();

//-------------------------------------

typedef Grid2D<UnitBase, 5, GridVector<UnitBase, 8> > UnitGrid;
typedef SwapVector<UnitBase*> VisibleUnits;
typedef vector<PlayerDataEdit> PlayerDataVect;
typedef UniqueContainer<UI_MessageTypeReference> DisabledMessages;
typedef UniqueContainer<Action*> ActiveMessages;

///////////////////////////////////////
//		»грова€ вселенна€
///////////////////////////////////////
class Universe 
{
public:
	Universe(MissionDescription& mission, XPrmIArchive* ia);
	~Universe();

	void setUseHT(bool useHT) { useHT_ = useHT; }

	virtual void Quant();
	void interpolationQuant();
	void triggerQuant(bool pause);

	FogOfWarMap* createFogOfWarMap(int clan);
	void fowQuant();
	void addFowModel(cObject3dx* model);

	STARFORCE_API bool universalSave(const MissionDescription& mission, bool userSave, Archive& oa);
	void relaxLoading();

	Player* activePlayer() { return active_player_; }
	Race activeRace() const { xassert(active_player_); return active_player_->race(); }

	virtual void setActivePlayer(int playerID, int cooperativeIndex = 0);

	int playersNumber() const; // “олько живые и неслужебные
	Player* findPlayer(int playerID) { xassert(playerID >= 0 && playerID < Players.size()); return Players[playerID]; }
	Player* worldPlayer() const { xassert(!Players.empty() && Players.back()->isWorld()); return Players.back(); }

	typedef BitVector<UnitsConstruction> ConstructionState;
	UnitReal* findUnit(const AttributeBase* attr);
	UnitReal* findUnit(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin = 0, const ConstructionState& state = CONSTRUCTED | CONSTRUCTING, bool onlyVisible_ = false);
	UnitReal* findUnitByLabel(const char* label);

	virtual void checkEvent(const Event& event);
	virtual void select(UnitInterface* unit, bool shiftPressed = true, bool no_deselect = false){}
	virtual void deselect(UnitInterface* unit) {};
	virtual void changeUnitNotify(UnitInterface* oldUnit, UnitInterface* newUnit){}

	void graphQuant(float dt);

	void setCountDownTime(int countDownTime) { countDownTime_ = countDownTime; }
	int countDownTime() { return countDownTime_; }

	bool interfaceEnabled() const { return interfaceEnabled_; }
	void setInterfaceEnabled(bool interfaceEnabled);

	void setControlEnabled(bool controlEnabled);

	virtual void receiveCommand(const netCommandGame& command) {}
	virtual void sendCommand(const netCommandGame& command) {}
	virtual bool isMultiPlayer() const { return false; }

	bool forcedDefeat(int playerID);

	void deleteUnit(UnitBase* unit);
	void clearDeletedUnits(bool delete_all);

	void deselectAll();
	void deleteSelected();

	void showDebugInfo();
	virtual void drawDebug2D() const;

	Player* addPlayer(const PlayerData& playerData);

	void updateSkinColor();

	int quantCounter() const { return quant_counter_; }
	bool initialQuants() const { return quantCounter() <= 4; }

	void collectWorldSheets();
	void refreshAttribute();

	PlayerVect Players;
	
	UnitGrid unitGrid;
	MTSection unitGridLock;
	
	CrashSystem* crashSystem;
	SecondMap* secondMap;

	STARFORCE_API void serialize(Archive& ar);

	void TurnOnTransparentTracking(bool on);
	cTransparentTracking* GetTransparentTracking(){ return transparent_tracking;}
	void ClearOverpatchingFOW();//for editor

	void addVisibleUnit(UnitBase* unit) { visibleUnits_.push_back(unit); }

	void exportPlayers(PlayerDataVect& playerDataVect) const;
	void importPlayers(const PlayerDataVect& playerDataVect);
	bool mergeWorld(const MergeOptions& options);

	unsigned short directShootInterpotatePeriod() const { return 4; }

	StreamInterpolator streamInterpolator;
	StreamInterpolator streamCommand;
	VisibleUnits visibleUnits;

	static void setProgressCallback(PROGRESSCALLBACK callback){ progressCallback_ = callback; }
	static void loadProgressUpdate(){ if(progressCallback_) (*progressCallback_)(); }
	static PROGRESSCALLBACK progressCallback(){ return progressCallback_; }
	bool isRandomScenario() const { return randomScenario_; }
	GameType gameType() const { return gameType_; }
	bool userSave() const { return userSave_; } // выставл€етс€ при записи и чтении

	class CircleManager* circleManager() {return circleManager_;}
	class CircleManager* circleManagerTeam() {return circleManagerTeam_;}
	class cCircleShow* circleShow(){return circleShow_;}

	ActiveMessages& activeMessages() { return activeMessages_; }
	DisabledMessages& disabledMessages() { return disabledMessages_; }
	ActiveMessages& activeAnimation() { return activeAnimation_; }

	IntVariables& intVariables() { return intVariables_; }
	virtual IntVariables& currentProfileIntVariables() { return intVariables_; }
	virtual ParameterSet& currentProfileParameters() { return currentProfileParameters_; }
	virtual float voiceFileDuration(const char* fileName, float duration) { return duration; }

protected:
	bool enableEventChecking_;
	unsigned long missionSignature;
	ParameterSet currentProfileParameters_;

private:
	Player* active_player_;

	ActiveMessages activeMessages_;
	DisabledMessages disabledMessages_;
	ActiveMessages activeAnimation_;
	
	int quant_counter_;

	bool userSave_;
	bool useHT_;

	bool interfaceEnabled_;

	bool randomScenario_;
	GameType gameType_;

	struct DeleteData
	{
		list<UnitBase*> unit;
		int quant;
	};
	list<DeleteData> deletedUnits_;

	vector<cObject3dx*> fowModels;
	MTSection fowModelsLock_;

	int countDownTime_;

	VisibleUnits visibleUnits_;

	cTransparentTracking* transparent_tracking;

	typedef StaticMap<int, FogOfWarMap*> FogOfWarMaps;
	FogOfWarMaps fogOfWarMaps_;
    
	static PROGRESSCALLBACK progressCallback_;
	static Universe* universe_;
	CircleManager* circleManager_;
	CircleManager* circleManagerTeam_;
	cCircleShow* circleShow_;

	IntVariables intVariables_;

	//-------------------------------
	friend Universe* universe();
};

inline Universe* universe() { return Universe::universe_; }

#endif //__UNIVERSE_H__
