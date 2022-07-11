#include "StdAfx.h"
#include "EnvironmentColors.h"
#include "Serialization\Serialization.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\ResourceSelector.h"

WaterPlumeAttribute::WaterPlumeAttribute()
{
	waterPlumeTextureName = ".\\Resource\\TerrainData\\Textures\\G_Tex_WaterRings_001.tga";
	waterPlumeFrequency = 0.3f;
}

void WaterPlumeAttribute::serialize(Archive& ar)
{
	float circle_time = 1.0f / waterPlumeFrequency;
	ar.serialize(circle_time, "circle_time", "Время кругов");
	waterPlumeFrequency = 1.0f / circle_time;

	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
	ar.serialize(ResourceSelector(waterPlumeTextureName, textureOptions), "cyrcle_texture_name", "ШЛЕЙФ : Текстура кругов");
}


