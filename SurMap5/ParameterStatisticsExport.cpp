#include "StdAfx.h"
#include "ParameterStatisticsExport.h"
#include "..\ExcelExport\ExcelExporter.h"
#include "..\UserInterface\UI_Render.h"
#include "UnitAttribute.h"
#include "WeaponPrms.h"
#include "..\Environment\SourceZone.h"

static std::string unitNameFromInterfaceName(const char* interfaceName)
{
	return UI_Render::instance().extractFirstLineText(interfaceName);
}

ParameterStatisticsExport::ParameterStatisticsExport()
{

}

static int calculateSlotsLimit(const RaceProperty& race)
{
	/*
	if(race.unitNumbers.empty())
		return 0;

	const UnitNumber& unitNumber = race.unitNumbers.front();
	UnitFormationTypeReference formation = unitNumber.type;
	if(!formation.numberParameters.customVector().empty()){
		ParameterValueReference valueRef = unitNumber.numberParameters.customVector().front()[0];
		ParameterTypeRefeernce typeRef = valueRef->type();
		float result = 0;
		
		FOR_EACH(unitAttributes, it){
			UnitAttributeID id = it->key();
			const AttributeBase& attribute = *it->get();
			if(attribute.formationType == formation){
				float capacity = attribute.resourceCapacity.findByName(typeRef.c_str(), 0.0f);
				if(capacity > FLT_EPS){
					...
				}
			}

		}
		return round(result);
	
	}
	else 
		return 0;
	*/
	return 120;
}

static bool exportableUnit(const AttributeBase& attr)
{
	return attr.isBuilding() || attr.isLegionary();
}

static float sourceDamage(const SourceBase* source, const char* name)
{
	if(source->type() == SOURCE_ZONE){
		const SourceZone* zone = safe_cast<const SourceZone*>(source);
		float damage = zone->damage().findByName(name, -1e7f);
		if(damage > -1e6f)
			return damage;
	}
	return -1e7f;
}

static float weaponDamage(const WeaponPrm* weapon, const char* name)
{
	if(weapon->weaponClass() == WeaponPrm::WEAPON_PROJECTILE){
		const WeaponProjectilePrm* weaponProjectile = safe_cast<const WeaponProjectilePrm*>(weapon);
		return weaponProjectile->missileID()->damage().findByName(name, -1e7f);
	}
	else if(weapon->weaponClass() == WeaponPrm::WEAPON_AREA_EFFECT){
		const WeaponAreaEffectPrm* weaponArea = safe_cast<const WeaponAreaEffectPrm*>(weapon);
		
		float sourcesDamage = 0.0f;

		const SourceWeaponAttributes& sources =weaponArea->sources();
		SourceWeaponAttributes::const_iterator it;
		FOR_EACH(sources, it){
			sourcesDamage += sourceDamage(it->source(), name);
		}
		if(sourcesDamage > 0.0f)
			return sourcesDamage;
		else
			return -1e7f;
	}
	else{
		
		return weapon->damage().findByName(name, -1e7f);
	}
}

static void setFloat(ExcelExporter* excel, int x, int y, float value)
{
	excel->setCellFloat(Vect2i(x, y), value);
}

void ParameterStatisticsExport::exportExcel(const char* fileName)
{
	const float INVALID_VALUE = -1e6f;
	AttributeLibrary::Strings::const_iterator it;
	const AttributeLibrary::Strings& unitAttributes = AttributeLibrary::instance().strings();

	ExcelExporter* excel = ExcelExporter::create(fileName);
	excel->beginSheet();

	// Лист 2 - Оружие
	excel->setSheetName("Оружие", 1251);
	excel->setCellText(Vect2i(0, 0), "Название юнита/оружия", 1251);
	excel->setCellText(Vect2i(1, 0), "Раса", 1251);
	excel->setCellText(Vect2i(2, 0), "Повр. по Armour", 1251);
	excel->setCellText(Vect2i(3, 0), "Повр. по Armour4", 1251);
	excel->setCellText(Vect2i(4, 0), "Время выстрела", 1251);
	excel->setCellText(Vect2i(5, 0), "Повр. по Armour/с", 1251);
	excel->setCellText(Vect2i(6, 0), "Повр. по Armour4/с", 1251);
	excel->setCellText(Vect2i(7, 0), "max повр. по Armour/с", 1251);
	excel->setCellText(Vect2i(8, 0), "max повр. по Armour4/с", 1251);
	excel->setCellText(Vect2i(9, 0), "время убийства max-ой 4-й брони", 1251);

	float maxArmour4 = 0.0f;
	FOR_EACH(unitAttributes, it){
		UnitAttributeID id = it->key();
		const AttributeBase& attribute = *it->get();
		maxArmour4 = max(maxArmour4, attribute.parametersInitial.findByName("Armour4", 0.0f));
	}

	int row = 1;
	FOR_EACH(unitAttributes, it){
		UnitAttributeID id = it->key();
		const AttributeBase& attribute = *it->get();
		std::string unitName = unitNameFromInterfaceName(attribute.interfaceName(0));
		if(exportableUnit(attribute)){
			AttributeBase::WeaponSlotAttributes::const_iterator wit;
			FOR_EACH(attribute.weaponAttributes, wit){
				if(const WeaponPrm* weapon = wit->weaponPrm()){
					std::string weaponName = wit->weaponPrmReference().c_str();
					if(weaponName.size() >= 2 &&
						(weaponName[0] == 'W' || weaponName[0] == 'w') &&
						((weaponName[1] == 'S' || weaponName[1] == 's') ||
						 (weaponName[1] == 'C' || weaponName[1] == 'c') ||
						 (weaponName[1] == 'F' || weaponName[1] == 'f')))
					{
						float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 0.1f);
						float fireTime = weapon->parameters().findByType(ParameterType::FIRE_TIME, 0.5f);

						float shotDuration = (reloadTime + fireTime);

						float shotsPerSecond = shotDuration > FLT_EPS ? 1.0f / shotDuration : 0.0f;

						int accountingNumber = attribute.accountingNumber;
						int maxNumber = 0;
						if(accountingNumber > 0){
							maxNumber = int(float(calculateSlotsLimit(*id.race())) / float(accountingNumber));
						}

						XBuffer name;
						name < unitName.c_str();
						name < " / ";
						name < weaponName.c_str();
						excel->setCellText(Vect2i(0, row), name, 1251);
						excel->setCellText(Vect2i(1, row), id.race().c_str(), 1251);
						setFloat(excel, 4, row, shotDuration);

						float weaponDamage1 = weaponDamage(weapon, "Armour");
						if(weaponDamage1 > INVALID_VALUE){
							setFloat(excel, 2, row, weaponDamage1);
							setFloat(excel, 5, row, weaponDamage1 * shotsPerSecond);
							if(maxNumber){
								setFloat(excel, 7, row, weaponDamage1 * maxNumber * shotsPerSecond);
							}
						}
						float weaponDamage4 = weaponDamage(weapon, "Armour4");
						if(weaponDamage4 > INVALID_VALUE){
							setFloat(excel, 3, row, weaponDamage4);
							setFloat(excel, 6, row, weaponDamage4 * shotsPerSecond);
							if(maxNumber){
								setFloat(excel, 8, row, weaponDamage4 * maxNumber * shotsPerSecond);
							}
							if(weaponDamage4 > FLT_EPS && shotsPerSecond > FLT_EPS){
								setFloat(excel, 9, row, maxArmour4 / (weaponDamage4 * shotsPerSecond));
							}
						}
						++row;
					}
				}
			}
		}
	}
	for(int i = 0; i < 10; ++i)
	excel->setColumnWidthAuto(i);

	// Лист 1 - Юниты
	excel->addSheet("Юниты");
	excel->setCellText(Vect2i(0, 0), "Название юнита", 1251);
	excel->mergeCellRange(Recti(Vect2i(0, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(1, 0), "Раса", 1251);
	excel->mergeCellRange(Recti(Vect2i(1, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(2, 0), "Слотов", 1251);
	excel->mergeCellRange(Recti(Vect2i(2, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(3, 0), "max кол-во", 1251);
	excel->mergeCellRange(Recti(Vect2i(3, 0), Vect2i(1, 2)));

	excel->setCellText(Vect2i(4, 0), "max стоимость", 1251);
	excel->mergeCellRange(Recti(Vect2i(4, 0), Vect2i(3, 1)));
	excel->setCellText(Vect2i(4, 1),  "Вода/Биоэнергия", 1251);
	excel->setCellText(Vect2i(5, 1),  "Соларка", 1251);
	excel->setCellText(Vect2i(6, 1),  "Материал/Биомасса", 1251);

	excel->setCellText(Vect2i(7, 0), "Время стр-ва", 1251);
	excel->mergeCellRange(Recti(Vect2i(7, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(8, 0), "Время стр-ва max кол-ва", 1251);
	excel->mergeCellRange(Recti(Vect2i(9, 0), Vect2i(1, 2)));
	
	row = 2;
	FOR_EACH(unitAttributes, it){
		UnitAttributeID id = it->key();
		const AttributeBase& attribute = *it->get();
		if(exportableUnit(attribute)){
			std::string unitName = unitNameFromInterfaceName(attribute.interfaceName(0));
			excel->setCellText(Vect2i(0, row), unitName.c_str(), 1251);
			excel->setCellText(Vect2i(1, row), id.race().c_str(), 1251);
			int accountingNumber = attribute.accountingNumber;
			excel->setCellFloat(Vect2i(2, row), accountingNumber);
			if(accountingNumber > 0){
				int maxNumber = int(float(calculateSlotsLimit(*id.race())) / float(accountingNumber));
				excel->setCellFloat(Vect2i(3, row), maxNumber);

				const char* types[]    =  { "R-fresh water",	"Е-hydro energy",		"A-bioenergy",
											"R-solar energy",	"Е-solar energy",		0,
											"R-salvage",		"Е-science resource",	"А-biomass" };

				for(int j = 0; j < 3; ++j){
					for(int i = 0; i < 3; ++i){
						if(const char* typeName = types[i * 3 + j]){
							float cost = attribute.installValue.findByName(typeName, -1e7f);
							if(cost > -1e6f){
								setFloat(excel, 4 + i, row, cost * maxNumber);
							}
						}
					}
				}
				float maxNumberProductionTime = attribute.creationTime * maxNumber;
				setFloat(excel, 8, row, maxNumberProductionTime);
			}
			setFloat(excel, 7, row, attribute.creationTime);
			++row;
		}
	}

	for(int i = 0; i < 9; ++i)
		excel->setColumnWidthAuto(i);

	excel->free();
}
