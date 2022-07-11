#include "stdafxTr.h"
#include "TerrainType.h"
#include "Serialization\Serialization.h"

WRAP_LIBRARY(TerrainTypeDescriptor, "TerrainTypeDescriptor", "Типы поверхности", "Scripts\\Content\\TerrainTypeDescriptor", 0, LIBRARY_EDITABLE);

const EnumDescriptor& getEnumDescriptor(const TerrainType& key)
{
	return TerrainTypeDescriptor::instance();
}

TerrainTypeDescriptor::TerrainTypeDescriptor()
: EnumDescriptor("TerrainType", "TerrainType")
{
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; i++){
		XBuffer buf;
		buf <= i;
		terrainTypeNames_[i] = buf;
		buf.init();
		buf < "Поверхность" <= i;
		terrainTypeNamesAlt_[i] = buf;
	}
	refresh();
}

void TerrainTypeDescriptor::serialize(Archive& ar)
{
	pair<string, Color4c> terrainTypeNameAndColor[TERRAIN_TYPES_NUMBER];
	if(ar.isInput()){
		ar.serializeArray(terrainTypeNameAndColor, "terrainTypeNameAndColor", "Имена и служебный цвет");
		for(int i = 0; i < TERRAIN_TYPES_NUMBER; i++){
			terrainTypeNamesAlt_[i] = terrainTypeNameAndColor[i].first;
			terrainTypeColors_[i] = terrainTypeNameAndColor[i].second;
		}
	}
	else {
		for(int i = 0; i < TERRAIN_TYPES_NUMBER; i++){
			terrainTypeNameAndColor[i].first=terrainTypeNamesAlt_[i];
			terrainTypeNameAndColor[i].second=terrainTypeColors_[i];
		}
		ar.serializeArray(terrainTypeNameAndColor, "terrainTypeNameAndColor", "Имена и служебный цвет");
	}
	if(ar.isInput())
		refresh();
}

void TerrainTypeDescriptor::refresh()
{
	clear();
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; i++){
		add(1 << i, terrainTypeNames_[i].c_str(), terrainTypeNamesAlt_[i].c_str());
	}
}

