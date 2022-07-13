#include "StdAfx.h"

#include "..\ExcelExport\ExcelExporter.h"
#include "..\Units\UnitAttribute.h"
#include "..\Units\WeaponAttribute.h"
#include "..\Units\Triggers.h"
#include "..\Units\Weapon.h"

#include "BalanceStats.h"

BalanceParameters::BalanceParameters()
{
	attackEfficiency_ = -1.f;
	defenceEfficiency_ = -1.f;
}

const WeaponPrmDamages BalanceParameters::calculateWeaponDamage(const WeaponPrmCache& cache)
{
	// damage_
	WeaponPrmDamages damages;

	const ParameterSet& prm = cache.parameters();
	float reloadTime = prm.findByType(ParameterType::RELOAD_TIME, 0.1f);
	float fireTime = prm.findByType(ParameterType::FIRE_TIME, 0.5f);

	damages.push_back(WeaponPrmDamage(cache.damage(), fireTime, reloadTime));

	WeaponDamage damage = cache.abnormalState().damage();
	damage *= logicPeriodSeconds;
	damage *= cache.abnormalState().duration();
	// abnormalState_ 
	damages.push_back(WeaponPrmDamage(damage, cache.abnormalState().duration(), 0));

	WeaponSourcePrms::const_iterator i;
	FOR_EACH(cache.sources(), i){
		damages.push_back(WeaponPrmDamage((*i).damage(), (*i).lifeTime(), (*i).activationDelay(), true));
		WeaponDamage damageSource = (*i).abnormalState().damage();
		damageSource *= logicPeriodSeconds;
		damageSource *= (*i).abnormalState().duration();
		damages.push_back(WeaponPrmDamage(damageSource, (*i).abnormalState().duration(), 0));
	} 

	return damages;
}

int BalanceParameters::calculateDeath(const WeaponPrmDamages& damages, const ParameterSet& target_parameters, float time)
{
	int deads = 0;
	float timeLeft = time;
	ParameterSet cur_parameters = target_parameters;
	WeaponPrmDamages::const_iterator i;
	while (timeLeft > 0) {
		float maxTimeToAct = damages.front().activeTime() + damages.front().relaxTime();
		timeLeft -= damages.front().activeTime();
		FOR_EACH(damages, i){
			if(!(*i).isDelay() || ((*i).isDelay() && (*i).relaxTime() < maxTimeToAct))
			{
				ParameterSet damage = (*i).damage();
				if(!(*i).isDelay() && (*i).activeTime() > maxTimeToAct){
					damage *= maxTimeToAct / (*i).activeTime();
				}
				if((*i).isDelay() && (*i).activeTime() > maxTimeToAct - (*i).relaxTime())
				{
					damage *= (maxTimeToAct - (*i).relaxTime()) / (*i).activeTime();
				}
				float prevHealth = cur_parameters.health();
				float prevArmor = cur_parameters.armor();
			
				ParameterSet armorDamage = damage;
				ParameterSet healthDamage = armorDamage;
				healthDamage.set(0, ParameterType::ARMOR);
				armorDamage.subClamped(healthDamage);
				cur_parameters.subClamped(armorDamage);
				
				ParameterSet armor;
				armor.setArmor(cur_parameters, target_parameters);
				healthDamage.subPositiveOnly(armor);
				cur_parameters.subClamped(healthDamage);
				
				cur_parameters.clamp(target_parameters); 

				if(prevHealth > FLT_EPS && cur_parameters.health() < FLT_EPS){
					cur_parameters = target_parameters;
					deads++;
				}
			}
		}
		timeLeft -= damages.front().relaxTime();
	} 
	return deads;
}

bool BalanceParameters::calculate(const AttributeBase& unit1, const AttributeBase& unit2)
{
	if(!unit1.isLegionary() && !unit1.isBuilding())
		return false;

	attackEfficiency_ = -1.f;
	defenceEfficiency_ = -1.f;

	ParameterSet unit1_prm = unit1.parametersInitial;
	ParameterSet unit2_prm = unit1.parametersInitial;

	float attack_eff = 0.f;

	int weapon_count = unit1.weaponAttributes.size();
	
	std::vector<WeaponPrmCache> weapons;
	weapons.resize(weapon_count, WeaponPrmCache());

	float time = 10.f;
	int total = 0;// total - общее колиство убитых unit2 всеми оружиями unit1
	for(int i = 0; i < weapon_count; i++){
		const WeaponPrm* prm = unit1.weaponAttributes[i].weaponPrm();
		if(prm){
			weapons[i].set(prm);
			prm->initCache(weapons[i]);
			if(weapons[i].checkDamage(unit2.parametersInitial)){
				WeaponPrmDamages damages = calculateWeaponDamage(weapons[i]);
				total += calculateDeath(damages, unit2.parametersInitial, time);
			} 
		}
	}

	return true;
}

// -------------------------

BalanceData::BalanceData()
{
	int sz = AttributeLibrary::instance().map().size();
	parameters_.reserve(sz * sz);
	unitNames_.reserve(sz);

	parametersTableWidth_ = 0;
}

void BalanceData::calculateParameters()
{
	parametersTableWidth_ = 0;
	for(int i = 0; i < RaceTable::instance().size(); i++){
		const RaceProperty& race = RaceTable::instance()[i];
		if(!race.instrumentary()){
			AttributeLibrary::Map::const_iterator it;
			FOR_EACH(AttributeLibrary::instance().map(), it){
				if(it->key().race() == &race && it->get()){
					parametersTableWidth_++;
					unitNames_.push_back(it->key().unitName().c_str());
					for(int j = 0; j < RaceTable::instance().size(); j++){
						const RaceProperty& race1 = RaceTable::instance()[j];
						if(!race1.instrumentary()){
							AttributeLibrary::Map::const_iterator it1;
							FOR_EACH(AttributeLibrary::instance().map(), it1){
								if(it1->key().race() == &race1 && it1->get()){
									parameters_.push_back(BalanceParameters());
									parameters_.back().calculate(*it->get(), *it1->get());
								}
							}
						}
					}
				}
			}
		}
	}
}

void BalanceData::saveParameters()
{
	ExcelExporter* exporter;

	exporter = ExcelExporter::create("balance_attack_efficiency_min.xls");
	exporter->beginSheet();

	for(int i = 0; i < parametersTableWidth_; i ++){
		exporter->setCellText(Vect2i(i + 1, 0), unitNames_[i].c_str());
		exporter->setCellTextOrientation(Vect2i(i + 1, 0), 90);
		exporter->setColumnWidthAuto(i + 1);

		exporter->setCellText(Vect2i(0, i + 1), unitNames_[i].c_str());
	}

	exporter->setColumnWidthAuto(0);
	exporter->free();

	exporter = ExcelExporter::create("balance_attack_efficiency_max.xls");
	exporter->beginSheet();

	for(int i = 0; i < parametersTableWidth_; i ++){
		exporter->setCellText(Vect2i(i + 1, 0), unitNames_[i].c_str());
		exporter->setCellTextOrientation(Vect2i(i + 1, 0), 90);
		exporter->setColumnWidthAuto(i + 1);

		exporter->setCellText(Vect2i(0, i + 1), unitNames_[i].c_str());
	}

	exporter->free();
}
