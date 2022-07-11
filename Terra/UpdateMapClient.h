#pragma once

#include "XMath/xmath.h"

enum UpdateMapType
{
	UPDATEMAP_TEXTURE=1<<0,
	UPDATEMAP_HEIGHT=1<<1,
	UPDATEMAP_REGION=1<<2,
	UPDATEMAP_ALL=UPDATEMAP_TEXTURE|UPDATEMAP_HEIGHT|UPDATEMAP_REGION,
};

class UpdateMapClient
{
public:
	virtual void updateMap(const Vect2i& pos1, const Vect2i& pos2, UpdateMapType type) = 0;
};
