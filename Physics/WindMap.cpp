#include "StdAfx.h"
#include "Serialization\Serialization.h"
#include "WindMap.h"
#include "NormalMap.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\EnumDescriptor.h"
#include "DebugUtil.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WindMap, WindType, "WindType")
REGISTER_ENUM_ENCLOSED(WindMap, DIRECTION_TO_CENTER_MAP, "Ветер направлен к центру мира")
REGISTER_ENUM_ENCLOSED(WindMap, DIRECTION_FROM_CENTER_MAP, "Ветер направлен от центра мира")
REGISTER_ENUM_ENCLOSED(WindMap, DIRECTION_THROUGH_MAP, "Ветер направлен через весь мир")
END_ENUM_DESCRIPTOR_ENCLOSED(WindMap, WindType)

WindMap * windMap = 0;

WindMap::WindMap( int hSize, int vSize )
:Map2D<Vect2f, tileSizeShl>(hSize, vSize), 
 windType_(DIRECTION_TO_CENTER_MAP),
 direction_(Vect2f(1.0f,0.0f)),perlin(64,64)
{
	perlin_time=0;
	perlin.setNumOctave(2);
	staticMap_ = new Vect2f[sizeX() * sizeY()];
	memset(staticMap(), 0, sizeX() * sizeY() * sizeof(Vect2f));
	clearMap();
}

Vect2f WindMap::getBilinear( const Vect2f& position )
{
	Vect2f retVal(Vect2f::ZERO);

	int x = position.xi() >> tileSizeShl;	
	int y = position.yi() >> tileSizeShl;

	int x2 = x + 1;
	int y2 = y + 1;

	float xDelta = (position.x - (x << tileSizeShl)) / tileSize;
	float yDelta = (position.y - (y << tileSizeShl)) / tileSize;

	if(x<0)
	{
		x2 = x = 0;
		xDelta=0;
	}

	if(x>=sizeX()-1)
	{
		x2 = x = sizeX()-1;
		xDelta=1;
	}

	if(y<0)
	{
		y2 = y = 0;
		yDelta=0;
	}

	if(y>=sizeY()-1)
	{
		y2 = y = sizeY()-1;
		yDelta=1;
	}

	retVal += (*this)(x,y) * (1.0f - xDelta) * (1.0f - yDelta);
	retVal += (*this)(x2,y) * xDelta * (1.0f - yDelta);
	retVal += (*this)(x,y2) * (1.0f - xDelta) * yDelta;
	retVal += (*this)(x2,y2) * xDelta * yDelta;

	return retVal;
}

void WindMap::updateRect(int x, int y, int dx, int dy) 
{
	int xl = x >> tileSizeShl;
	int yl = y >> tileSizeShl;
	int xr = (x + dx) >> tileSizeShl;
	int yr = (y + dy) >> tileSizeShl;

	float windForce = direction_.norm();

	for(int i = xl; i <= xr; i++)
		for(int j = yl; j <= yr; j++){
			switch(windType_){
				case DIRECTION_TO_CENTER_MAP:{ 
					int x_ = (sizeX() >> 1) - i;
					int y_ = (sizeY() >> 1) - j;
					Vect2f dir(x_,y_);
					if(!(x_ == 0 && y_ == 0))
						dir.normalize(windForce);
					updateStaticMapTile(dir, i, j, windForce * 2.0f);
				    break;
				}
				case DIRECTION_FROM_CENTER_MAP:{ 
					int x_ = i - (sizeX() >> 1);
					int y_ = j - (sizeY() >> 1);
					Vect2f dir(x_,y_);
					if(!(x_ == 0 && y_ == 0))
						dir.normalize(windForce);
					updateStaticMapTile(dir, i, j, windForce * 2.0f);
					break;
				}
				case DIRECTION_THROUGH_MAP:
					updateStaticMapTile(direction_,i, j, windForce * 2.0f);
			}
		}
}

void WindMap::updateStaticMapTile(const Vect2f& prevWind, int x, int y, float terrainFactor)
{
	const int normalMapShift = tileSizeShl - NormalMapTile::tileShift;
	const int normalMapTile = 1 << normalMapShift;

	int nx = x << normalMapShift;
	int ny = y << normalMapShift;

	Vect3f normal(Vect3f::ZERO);
	for(int i = nx; i <= (nx + normalMapTile); i++)
		for(int j = ny; j <= (ny + normalMapTile); j++)
			normal += normalMap->normal(i,j);
			
	xassert(y * sizeX() + x < sizeX() * sizeY());
	staticMap_[y*sizeX()+x] = prevWind + Vect2f(normal.x / sqr(normalMapTile) * terrainFactor, normal.y  / sqr(normalMapTile) * terrainFactor);
}

void WindMap::showDebugInfo()
{
	for(int i = 0; i < sizeX(); i++)
		for(int j = 0; j < sizeY(); j++) {
			Vect3f point((i << tileSizeShl) + tileSize/2, (j << tileSizeShl) + tileSize/2, 100);
			show_vector(point, Vect3f((*this)(i,j).x * 50.f, (*this)(i,j).y * 50.f, 0), Color4c::GREEN);
			//Vect2f p=getPerlin(Vect2f(i,j));
			//show_vector(point, Vect3f(p.x * 50.f, p.y * 50.f, 0), Color4c::GREEN);
			
		}
}

WindMapAttributes::WindMapAttributes()
{
	windType_ = WindMap::DIRECTION_TO_CENTER_MAP;
	windPower_ = 1.f;
}

void WindMapAttributes::serialize(Archive& ar)
{
	ar.serialize(windType_, "windType", "Тип ветра");
	ar.serialize(RangedWrapperf(windPower_, 0.01f, 20.0f), "windPower", "Сила ветра");
}

void WindMap::serialize(Archive& ar)
{
	float windPower = direction_.norm();

	ar.serialize(windType_, "windType_", "Тип ветра");
	ar.serialize(RangedWrapperf(windPower, 0.01f, 20.0f), "windPower", "Сила ветра");

	if(ar.isInput()){
		direction_.normalize(windPower);	
		updateRect(0, 0, vMap.H_SIZE - 1, vMap.V_SIZE - 1);
	}
}


void WindMap::quant()
{
	clearMap();
	perlin_time=fmodFast(perlin_time+0.1f,1000);
}

Vect2f WindMap::getPerlin(const Vect2f& position)
{
	Vect2f speed=getBilinear(position);
	float size=0.02f;
	float perturbation=perlin.get(position.x*size,position.y*size+perlin_time);
	speed*=perturbation;
	return speed;
}
