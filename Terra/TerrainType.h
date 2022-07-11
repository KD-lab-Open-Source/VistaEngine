#pragma once

#include "Serialization\EnumDescriptor.h"
#include "Serialization\LibraryWrapper.h"
#include "Terra.h"

enum TerrainType
{
	TERRAIN_TYPE0 = 1 << 0,
	TERRAIN_TYPE1 = 1 << 1,
	TERRAIN_TYPE2 = 1 << 2,
	TERRAIN_TYPE3 = 1 << 3,
	TERRAIN_TYPE4 = 1 << 4,
	TERRAIN_TYPE5 = 1 << 5,
	TERRAIN_TYPE6 = 1 << 6,
	TERRAIN_TYPE7 = 1 << 7,
	TERRAIN_TYPE8 = 1 << 8,
	TERRAIN_TYPE9 = 1 << 9,
	TERRAIN_TYPE10 = 1 << 10,
	TERRAIN_TYPE11 = 1 << 11,
	TERRAIN_TYPE12 = 1 << 12,
	TERRAIN_TYPE13 = 1 << 13,
	TERRAIN_TYPE14 = 1 << 14,
	TERRAIN_TYPE15 = 1 << 15,
};
enum {
	TERRAIN_TYPEALL = 0xffFF
};


class TerrainTypeDescriptor : public EnumDescriptor, public LibraryWrapper<TerrainTypeDescriptor>
{
public:
	TerrainTypeDescriptor();
	void serialize(Archive& ar);
	const Color4c* getColors(){ return terrainTypeColors_; }

private:
	string terrainTypeNames_[TERRAIN_TYPES_NUMBER];
	string terrainTypeNamesAlt_[TERRAIN_TYPES_NUMBER];
	Color4c terrainTypeColors_[TERRAIN_TYPES_NUMBER];

	void refresh();
};