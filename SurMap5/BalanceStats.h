#ifndef __BALANCE_STATS_H__
#define __BALANCE_STATS_H__

#include "AttributeReference.h"

class WeaponPrmDamage
{
public:
	WeaponPrmDamage(const ParameterCustom& damage, float activeTime, float relaxTime, bool delay = false) 
	{
		activeTime_ = activeTime;
		relaxTime_ = relaxTime;
		damage_ = damage;
		isDelay_ = delay;
	}

	const ParameterCustom& damage() const { return damage_; }
	float activeTime() const { return activeTime_; }
	float relaxTime() const { return relaxTime_; }
	bool isDelay() const { return isDelay_; }

	void setDamage(const WeaponDamage& damage) { damage_ = damage; }
	void setActiveTime(float time) { activeTime_ = time; }
	void setRelaxTime(float time) { relaxTime_ = time; }
	void setIsDelay(bool delay) { isDelay_ = delay; }

private:
	// повреждения
	ParameterCustom damage_;
	// время нанесения повреждений
	float activeTime_;
	// задержка активации/перезарядка
	float relaxTime_;
	// relaxTime_ - задержка?
	bool isDelay_;
};

typedef vector<WeaponPrmDamage> WeaponPrmDamages; 

class BalanceParameters
{
public:
	BalanceParameters();

	/// расчёт балансовых параметров для unit1 относительно unit2
	bool calculate(const AttributeBase& unit1, const AttributeBase& unit2);

private:

	const WeaponPrmDamages calculateWeaponDamage(const WeaponPrmCache& cache);
	int calculateDeath(const WeaponPrmDamages& damages, const ParameterSet& target_parameters, float time = 10.f); 
	/// эффективность атаки против unit2
	Rangef attackEfficiency_;
	/// эффективность защиты от unit2
	Rangef defenceEfficiency_;
};

class BalanceData
{
public:
	BalanceData();

	void calculateParameters();
	void saveParameters();

private:

	/// таблица балансовых параметров для всех юнитов из AttributeLibrary
	/// размер - AttributeLibrary::instance().size() * AttributeLibrary::instance().size()
	std::vector<BalanceParameters> parameters_;

	std::vector<std::string> unitNames_;

	int parametersTableWidth_;
};

#endif /* __BALANCE_STATS_H__ */

