#include "StdAfx.h"

#include "ExcelExport\ExcelExporter.h"
#include "UserInterface\UI_Render.h"
#include "UnitAttribute.h"
#include "WeaponPrms.h"
#include "Environment\SourceZone.h"
#include "Serialization\StringTable.h"
#include "UnicodeConverter.h"
#include "ParameterStatisticsExport.h"
#include "WBuffer.h"

class ParameterStatisticsExportImpl{
public:
	ParameterStatisticsExportImpl(ParameterStatisticsExport* owner);

	// группа ресурсов
	class ResourceGroup{
	public:
		void serialize(Archive& ar);       

		typedef ParameterTypeReference Type;
		typedef std::vector<Type> Types;

		std::string name_;
		Types types_;
	};

	bool validWeaponName(const char* name) const;
	int slotsLimit(const RaceProperty& race) const;
	std::string unitName(const AttributeBase& attribute);
	void serialize(Archive& ar);

	typedef std::vector<ResourceGroup> ResourceGroups;
	typedef ParameterTypeReference ArmorType;
	typedef std::vector<ArmorType> ArmorTypes;
	ResourceGroups resourceGroups_;

	int slotsLimit_;
	bool onlyWithPrefixes_;
	bool useInterfaceUnitNames_;
	ArmorTypes armorTypes_;
protected:
	ParameterStatisticsExport* owner_;
};

ParameterStatisticsExportImpl::ParameterStatisticsExportImpl(ParameterStatisticsExport* owner)
: owner_(owner)
, onlyWithPrefixes_(false)
, useInterfaceUnitNames_(false)
, slotsLimit_(120)
{
	//armorTypes_.
}

void ParameterStatisticsExportImpl::ResourceGroup::serialize(Archive& ar)
{
	ar.serialize(name_, "name", "Заголовок");
	ar.serialize(types_, "types", "Типы параметров");
}

int ParameterStatisticsExportImpl::slotsLimit(const RaceProperty& race) const
{
	return slotsLimit_;
}

void ParameterStatisticsExportImpl::serialize(Archive& ar)
{
	ar.serialize(resourceGroups_, "resourceGroups", "Группы ресурсов");
	ar.serialize(armorTypes_, "armorTypes", "Типы брони");
	ar.serialize(slotsLimit_, "slotsLimit", "Лимит слотов");
	ar.serialize(onlyWithPrefixes_, "onlyWithPrefixes", "Оружие только с префиксами WS, WC, WF");
	ar.serialize(useInterfaceUnitNames_, "useInterfaceUnitNames", "Использовать интерфейсные названия юнитов");
}

std::string ParameterStatisticsExportImpl::unitName(const AttributeBase& attribute)
{
	if(useInterfaceUnitNames_)
		return UI_Render::instance().extractFirstLineText(attribute.interfaceName(0));
	else
		return attribute.libraryKey();
}

bool ParameterStatisticsExportImpl::validWeaponName(const char* name) const
{
	if(!onlyWithPrefixes_)
		return true;
	else
		if(strlen(name) > 2 &&
		(name[0] == 'W' || name[0] == 'w') &&
		((name[1] == 'S' || name[1] == 's') || (name[1] == 'C' || name[1] == 'c') || (name[1] == 'F' || name[1] == 'f')))
		return true;
    return false;									
}

// -----------------------------------------------------------------

void ParameterStatisticsExport::serialize(Archive& ar)
{
	impl().serialize(ar);
}


ParameterStatisticsExport::ParameterStatisticsExport()
: impl_(new ParameterStatisticsExportImpl(this))
{
}

ParameterStatisticsExport::~ParameterStatisticsExport()
{
	delete impl_;
	impl_ = 0;
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
	excel->setSheetName(L"Оружие");
	excel->setCellText(Vect2i(0, 0), L"Название юнита/оружия");
	excel->setCellText(Vect2i(1, 0), L"Раса");
	excel->setCellText(Vect2i(2, 0), L"Время выстрела");
	int column = 3;

	Impl::ArmorTypes& armorTypes = impl().armorTypes_;
	Impl::ArmorTypes::iterator ait;
	int numArmors = armorTypes.size();
	float* maxArmors = new float[numArmors];

	int index = 0;
	FOR_EACH(armorTypes, ait){
		Impl::ArmorType& armorType = *ait;
		wstring armorName = a2w(armorType.c_str());

		excel->setCellText(Vect2i(column + index + 0, 0),
						   (wstring(L"Повр. по ") + armorName).c_str());
		excel->setCellText(Vect2i(column + index + numArmors, 0),
						   (wstring(L"Повр. по ") + armorName + L"/с").c_str());
		excel->setCellText(Vect2i(column + index + numArmors * 2, 0),
						   (wstring(L"Max повр. по ") + armorName + L"/с").c_str());
		excel->setCellText(Vect2i(column + index + numArmors * 3, 0),
						   (wstring(L"Время убийства Max ") + armorName).c_str());

		FOR_EACH(unitAttributes, it){
			UnitAttributeID id = it->key();
			const AttributeBase& attribute = *it->get();
			maxArmors[index] = max(maxArmors[index], attribute.parametersInitial.findByName(armorType.c_str(), 0.0f));
		}

	   ++index;
	}

	int row = 1;
	FOR_EACH(unitAttributes, it){
		UnitAttributeID id = it->key();
		const AttributeBase& attribute = *it->get();
		std::string unitName = impl().unitName(attribute);
		if(exportableUnit(attribute)){
			WeaponSlotAttributes::const_iterator wit;
			FOR_EACH(attribute.weaponAttributes, wit){
				if(const WeaponPrm* weapon = wit->second.weaponPrm()){
					std::string weaponName = wit->second.weaponPrmReference().c_str();
					if(impl().validWeaponName(weaponName.c_str())){
						float reloadTime = weapon->parameters().findByType(ParameterType::RELOAD_TIME, 0.1f);
						float fireTime = weapon->parameters().findByType(ParameterType::FIRE_TIME, 0.5f);

						float shotDuration = (reloadTime + fireTime);

						float shotsPerSecond = shotDuration > FLT_EPS ? 1.0f / shotDuration : 0.0f;

						int accountingNumber = attribute.accountingNumber;
						int maxNumber = 0;
						if(accountingNumber > 0){
							maxNumber = int(float(impl().slotsLimit(*id.race())) / float(accountingNumber));
						}

						WBuffer name;
						name < unitName.c_str() < L" / " < weaponName.c_str();
						excel->setCellText(Vect2i(0, row), name.c_str());
						excel->setCellText(Vect2i(1, row), a2w(id.race().c_str()).c_str());
						setFloat(excel, 2, row, shotDuration);

						Impl::ArmorTypes::iterator ait;

						int index = 0;
						FOR_EACH(armorTypes, ait){
							Impl::ArmorType& armorType = *ait;
							float damage = weaponDamage(weapon, armorType.c_str());
							if(damage > INVALID_VALUE){
								setFloat(excel, index + column, row, damage);
								setFloat(excel, index + column + numArmors, row, damage * shotsPerSecond);
								if(maxNumber){
									setFloat(excel, index + column + numArmors * 2, row, damage * maxNumber * shotsPerSecond);
								}
								if(damage > FLT_EPS && shotsPerSecond > FLT_EPS){
									setFloat(excel, index + column + numArmors * 3, row, maxArmors[index] / (damage * shotsPerSecond));
								}
							}
							++index;
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
	excel->addSheet(L"Юниты");
	excel->setCellText(Vect2i(0, 0), L"Название юнита");
	excel->mergeCellRange(Recti(Vect2i(0, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(1, 0), L"Раса");
	excel->mergeCellRange(Recti(Vect2i(1, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(2, 0), L"Слотов");
	excel->mergeCellRange(Recti(Vect2i(2, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(3, 0), L"max кол-во");
	excel->mergeCellRange(Recti(Vect2i(3, 0), Vect2i(1, 2)));

	int numResourceGroups = impl().resourceGroups_.size();
	if(numResourceGroups > 0){
		excel->setCellText(Vect2i(4, 0), L"max стоимость");
		excel->mergeCellRange(Recti(Vect2i(4, 0), Vect2i(numResourceGroups, 1)));
		
		int index = 0;
		Impl::ResourceGroups& groups = impl().resourceGroups_;
		Impl::ResourceGroups::iterator git;
		FOR_EACH(groups, git){
			excel->setCellText(Vect2i(4 + index, 1), a2w(git->name_).c_str());
			++index;
		}
	}
	column = 4 + impl().resourceGroups_.size();

	excel->setCellText(Vect2i(column, 0), L"Время стр-ва");
	excel->mergeCellRange(Recti(Vect2i(column, 0), Vect2i(1, 2)));
	excel->setCellText(Vect2i(column + 1, 0), L"Время стр-ва max кол-ва");
	excel->mergeCellRange(Recti(Vect2i(column + 1, 0), Vect2i(1, 2)));
	
	row = 2;
	FOR_EACH(unitAttributes, it){
		UnitAttributeID id = it->key();
		const AttributeBase& attribute = *it->get();
		if(exportableUnit(attribute)){
			std::string unitName = impl().unitName(attribute);
			excel->setCellText(Vect2i(0, row), a2w(unitName.c_str()).c_str());
			excel->setCellText(Vect2i(1, row), a2w(id.race().c_str()).c_str());
			int accountingNumber = attribute.accountingNumber;
			excel->setCellFloat(Vect2i(2, row), accountingNumber);
			if(accountingNumber > 0){
				int maxNumber = int(float(impl().slotsLimit(*id.race())) / float(accountingNumber));
				excel->setCellFloat(Vect2i(3, row), maxNumber);

				Impl::ResourceGroups& resourceGroups = impl().resourceGroups_;
				Impl::ResourceGroups::iterator git;
				
				int index = 0;
				FOR_EACH(resourceGroups, git){
					Impl::ResourceGroup& group = *git;

					Impl::ResourceGroup::Types::iterator tit;
					FOR_EACH(group.types_, tit){
						Impl::ResourceGroup::Type& type = *tit;
						float cost = attribute.installValue.findByName(type.c_str(), -1e7f);
						if(cost > -1e6f){
							setFloat(excel, 4 + index, row, cost * maxNumber);
							break;
						}
					}
					++index;
				}

				float maxNumberProductionTime = attribute.creationTime * maxNumber;
				setFloat(excel, column + 1, row, maxNumberProductionTime);
			}
			setFloat(excel, column, row, attribute.creationTime);
			++row;
		}
	}

	for(int i = 0; i < 9; ++i)
		excel->setColumnWidthAuto(i);

	excel->free();
}
