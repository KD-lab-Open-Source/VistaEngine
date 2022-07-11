#ifndef __WIND_MAP_H__
#define __WIND_MAP_H__

#include "Map2D.h"
#include "..\Render\src\perlin.h"
struct WindMapAttributes;
class WindMap : public Map2D<Vect2f, 6> {
public:
	enum WindType {
		DIRECTION_TO_CENTER_MAP,
		DIRECTION_FROM_CENTER_MAP,
		DIRECTION_THROUGH_MAP
	};	
	WindMap(int hSize,int vSize);
	~WindMap() { delete[] staticMap_; }

	Vect2f getBilinear(const Vect2f& position);
	Vect2f* staticMap() { return staticMap_; }
	Vect2f getPerlin(const Vect2f& position);

	void setWindType(WindType windType) { windType_ = windType; }
	WindType getWindType() { return windType_; }

	void setDirection(const Vect2f& direction) { direction_ = direction; }
	Vect2f getDirection() { return direction_; }

	void quant();
	void updateRect(int x, int y, int dx, int dy); // Обновлять после обновления NormalMap-а.

	void showDebugInfo();
	//void serialize(Archive& ar);
	void init(WindMapAttributes& attributes);

private:

	// Заполняет карту ветра статическим ветром.
	void clearMap() { memcpy(map(), staticMap(), sizeX() * sizeY() * sizeof(Vect2f)); }
	void updateStaticMapTile(const Vect2f& prevWind, int x, int y, float terrainFactor);

	Vect2f* staticMap_;

	WindType windType_;
	Vect2f direction_; // direction - направление ветра + длина вектора=сила ветра

	Perlin2d perlin;
	float perlin_time;
};
struct WindMapAttributes
{
	WindMapAttributes();
	void serialize(Archive& ar);
	WindMap::WindType windType_;
	float windPower_;
};
extern WindMap * windMap;

#endif //__WIND_MAP_H__
