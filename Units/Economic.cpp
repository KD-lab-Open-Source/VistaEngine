#include "StdAfx.h"
#include "Universe.h"
#include "CameraManager.h"
#include "Squad.h"
#include "RenderObjects.h"
#include "Installer.h"
#include "Config.h"
#include "ForceField.h"

void Player::EconomicQuant()
{
//	watch_i(clusters_.size(), this);
//	watch_i(active_generators_.size(), this);
	UpdateEconomicStructure();
}

void Player::destroyLinkEconomic()
{
	for(int i = 0;i < UNIT_ATTRIBUTE_STRUCTURE_MAX;i++)
		removeNotAlive(BuildingList[i]);
}

void Player::UpdateStructureAccessible()
{	
/*	for(int i = 0;i < UNIT_ATTRIBUTE_STRUCTURE_MAX;i++)
		GetEvolutionBuildingData((terUnitAttributeID)i).clear();

	if(frame()){
		for(int i = 0;i < UNIT_ATTRIBUTE_STRUCTURE_MAX;i++){
			terUnitAttributeID evolution_id = (terUnitAttributeID)i;
			const AttributeBuilding& attr = *safe_cast<const AttributeBuilding*>(unitAttribute(evolution_id));
			EnableData& evolution = GetEvolutionBuildingData(evolution_id);
			bool enabled = true;
			// Если уже есть построенная структура, то не требуется иметь downgrade
			terUnitAttributeID downgrade_id = countUnits(evolution_id) ? attr.downgrade() : UNIT_ATTRIBUTE_NONE;
			for(int j = 0; j < attr.EnableStructure.size(); j++){
				terUnitAttributeID id = attr.EnableStructure[j];
				if(id != downgrade_id && !GetEvolutionBuildingData(id).Worked){
					enabled = false;
					break;
				}
			}
			if(enabled){
				evolution.Enabled = 1;
				Buildings::iterator bi;
				int worked = 0;
				FOR_EACH(BuildingList[i], bi)
					if((*bi)->isBuildingEnable()){
						worked = 1;
						if(!(*bi)->isUpgrading())
							evolution.Worked = 1;
						break;
					}
				for(int j = 0; j < attr.Downgrades.size(); j++){
					EnableData& downgrade = GetEvolutionBuildingData(attr.Downgrades[j]);
					downgrade.Enabled = 1;
					downgrade.Worked |= worked;
				}
			}
			
			// Устанавливаем Requested и Construction независимо от Enable
			Buildings::iterator bi;
			int downgrades_constructed = 0;
			FOR_EACH(BuildingList[i], bi){
				evolution.Requested++;
				if((*bi)->isConstructed())
					evolution.Constructed++;
				if((*bi)->isConstructed() || (*bi)->isUpgrading())
					downgrades_constructed++;
			}
			for(j = 0; j < attr.Downgrades.size(); j++){
				EnableData& downgrade = GetEvolutionBuildingData(attr.Downgrades[j]);
				downgrade.Requested += evolution.Requested;
				downgrade.Constructed += downgrades_constructed;
			}
		}
	}

	for(i = UNIT_ATTRIBUTE_SOLDIER + MUTATION_ATOM_MAX;i < UNIT_ATTRIBUTE_LEGIONARY_MAX; i++){
		EnableData& mutation = GetMutationElement((terUnitAttributeID)i);
		const AttributeLegionary& attr = *safe_cast<const AttributeLegionary*>(unitAttribute((terUnitAttributeID)i));
		mutation.Enabled = 1;
		for(int j = 0;j < attr.EnableStructure.size();j++)
			if(!GetEvolutionBuildingData(attr.EnableStructure[j]).Worked){
				mutation.Enabled = 0;
				break;
				}
		}*/
}

UnitBuilding* Player::buildStructure(const AttributeBuilding* buildingAttr, const Vect3f& pos)
{
	//Buildings::iterator bi;
	//FOR_EACH(buildingList(build_index), bi){
	//	if(!(*bi)->isConstructed())
	//		return 0;
	//}
	if(resource_.empty()){
		//xassert(0 && "Нет ресурса у игрока");
		return 0;
	}

	BuildingInstaller installer;
	installer.InitObject(buildingAttr);
	installer.SetBuildPosition(To3D(pos), pos.z, 0);
	if(!installer.valid())
		return 0;
	installer.Clear();

	UnitBuilding* n = safe_cast<UnitBuilding*>(buildUnit(buildingAttr));
	n->setPose(Se3f(QuatF(pos.z, Vect3f::K), pos), true);
	n->startConstruction();
	return n;
}

void Player::UpdateEconomicStructure()
{
	start_autostop_timer(UpdateEconomicStructure, 2);

	CalcStructureRegion();
	CalcEnergyRegion();

	UpdateStructureAccessible();

	structure_column_.setUnchanged();
	energyColumn().setUnchanged();
}

void Player::CalcStructureRegion()
{
	float area = 0;
	Buildings::iterator bi;
	FOR_EACH(BuildingList[UNIT_ATTRIBUTE_CORE], bi){
		UnitBuilding& b = **bi;
		if(b.isBuildingEnable()){
			const AttributeBase& attr = b.attr();
			area += sqr(attr.ZeroLayerRadius)*M_PI;
			b.placeZeroLayer(false);
		}
		else
			b.freeZeroLayer();
	}
	EnergyData.setAreaIdeal(area);
}

void Player::CalcEnergyRegion()
{
	start_autostop_timer(CalcEnergyRegion, STATISTICS_GROUP_AI);

	if(structureColumn().changed() || universe()->clusterColumn().changed()){
		MTAuto lock(universe()->EnergyRegionLocker());
		energyColumn().intersect(structureColumn(), universe()->clusterColumn());
		if(energyColumn().changed()){
			energy_region_.vectorize(0, false);
			FrameStatData.EnergyArea = energyColumn().area();
			EnergyData.setArea(FrameStatData.EnergyArea);
		}
	}
}

