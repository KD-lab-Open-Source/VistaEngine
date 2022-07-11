#ifndef __UNIT_INTERFACE_H__
#define __UNIT_INTERFACE_H__

#include "BaseUnit.h"
#include "UnitCommand.h"
#include "..\Units\Triggers.h"
#include "..\Units\AttributeCache.h"

////////////////////////////////////////////////
// Интерфейс для игрового интерфейса
// Объединяет юниты, здания, предметы и сквады
////////////////////////////////////////////////

typedef vector<UnitCommand> CommandList;

class UnitItemInventory;
class UnitItemResource;
class UnitReal;
class InventorySet;
enum RequestResourceType;

enum UsedByTriggerReason
{
	REASON_DEFAULT, // неопределенный триггер
	REASON_ATTACK,	// триггер связанный с атакой
	REASON_MOVE		// триггер связанный с движением
};


class UnitInterface : public UnitBase, public AttributeCache
{
public:
	UnitInterface(const UnitTemplate& data);
	~UnitInterface();

	void Quant();

	//------------------------------------------------
	// Команды
	void sendCommand(const UnitCommand& command); // MTG
	void receiveCommand(const UnitCommand& command); // MTL
	
	virtual bool canSuspendCommand(const UnitCommand& command) const { return true; }
	virtual void executeCommand(const UnitCommand& command);

	virtual UnitReal* getUnitReal() = 0;
	virtual const UnitReal* getUnitReal() const = 0;
	virtual const AttributeBase* selectionAttribute() const { return &attr(); }
	virtual int selectionListPriority() const { return attr().selectionListPriority; }

	virtual bool requestResource(const ParameterSet& resource, RequestResourceType requestResourceType) const;
	virtual void subResource(const ParameterSet& resource);

	virtual void applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics) {}

	float health() const;
	float sightRadius() const;

	virtual bool selectAble() const { return false; }

	virtual const Se3f& interpolatedPose() const = 0;// использовать только внутри graphQuant()
	
	//------------------------------------------------
	// юнит невидим
	virtual bool isUnseen() const { return false; }

	/// Юнит занят выполнением назначенных действий?
	virtual bool isWorking() const { return false; }

	virtual bool isConstructed() const { return true; }

	//------------------------------------------------

	/// уровень зарядки оружия, [0, 1]
	virtual float weaponChargeLevel(int weapon_id = 0) const { return 0; }
	/// прогресс производства, [0, 1]
	virtual float productionProgress() const { return 0; }
	/// прогресс производства параметров, [0, 1]
	virtual float productionParameterProgress() const { return 0; }

	//------------------------------------------------

	virtual bool canFire(int weaponID, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const { return false; }
	virtual bool fireDistanceCheck(const WeaponTarget& target, bool check_fow = false) const { return false; }
	virtual bool canPutToInventory(const UnitItemInventory* item) const { return false; }
	virtual bool canExtractResource(const UnitItemResource* item) const { return false; }
	virtual bool canBuild(const UnitReal* building) const { return false; }
	virtual bool canRun() const { return false; }
	virtual Accessibility canUpgrade(int upgradeNumber, RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const { return DISABLED; }
	virtual Accessibility canProduction(int number) const { return DISABLED; }
	virtual Accessibility canProduceParameter(int number) const { return DISABLED; }

	virtual bool canDetonateMines() const { return false; }

	//------------------------------------------------

	const InventorySet* inventory() const;

	virtual class UnitSquad* getSquadPoint() { return 0; }
	
	virtual bool isSuspendCommand(CommandID commandID) = 0;
	virtual bool isSuspendCommandWork() = 0;

	const CommandList& suspendCommandList() const { return suspendCommandList_; }

	bool isUsedByTrigger() const { return usedByTrigger_; } 
	bool isShowAISign() const { return showAISign_; }
	void setShowAISign(bool showAISign);
	void setUsedByTrigger(bool usedByTrigger);  
	void startUsedByTrigger(int time, UsedByTriggerReason reason);

	void setAimed() { aimedTimer_.start(1000); }
	bool aimed() const { return aimedTimer_(); }

	virtual bool uniform(const UnitReal* unit = 0) const;
	virtual bool prior(const UnitInterface* unit) const;

	void serialize(Archive& ar);

	void setDebugString(const char* debugString) { debugString_ = debugString; }

protected:
	string debugString_;

private:
	CommandList suspendCommandList_; // Очередь отложенных комманд.

	bool usedByTrigger_;
	bool showAISign_;
	UsedByTriggerReason reason_;
	DelayTimer usedByTriggerTimer_;
	DurationTimer aimedTimer_;
};

#endif //__UNIT_INTERFACE_H__
