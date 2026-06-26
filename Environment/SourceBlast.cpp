#include "stdafx.h"
#include "SourceBlast.h"
#include "Physics/windMap.h"

SourceBlast::SourceBlast()
:SourceBase()
{
	radiusCur = 0;
	radiusSpeed = 200.0f;
	blastPower = 200.0f;
}

void SourceBlast::quant()
{
	SourceBase::quant();
	
	if(!active_)
		return;
	
	radiusCur += radiusSpeed * 0.1f;
	
	if(radiusCur > radius()){
		kill();
		return;
	}
	
	windMapQuant();
}

void SourceBlast::serialize( Archive& ar )
{
	SourceBase::serialize(ar);
	ar.serialize(radiusSpeed, "radiusSpeed", "�������� [100..300]");
	ar.serialize(blastPower, "blastPower", "�������� [100..300]");

	radius_ = 500;
}

void SourceBlast::windMapQuant()
{
	// ������������ ����� ����� ��������� ���� �� ������ ������.
	float heightCur = blastPower - radiusCur/radius() * blastPower;
	
	int xl = max(int(round(position().x - radiusCur)) >> windMap->tileSizeShl, 0);
	int xr = min(int(round(position().x + radiusCur)) >> windMap->tileSizeShl, windMap->sizeX() - 1);
	int yl = max(int(round(position().y - radiusCur)) >> windMap->tileSizeShl, 0);
	int yr = min(int(round(position().y + radiusCur)) >> windMap->tileSizeShl, windMap->sizeY() - 1);

	for(int j=yl; j<yr; j++){
		float y = (j << windMap->tileSizeShl) + (windMap->tileSize >> 1);
		for(int i=xl; i<xr; i++){
			float x = (i << windMap->tileSizeShl) + (windMap->tileSize >> 1);
			Vect2f tileWind;
			tileWind.x = x - position().x + 0.1f;
			tileWind.y = y - position().y + 0.1f;
			if(tileWind.norm2() <= sqr(radiusCur)){
				tileWind.normalize(heightCur);
				(*windMap)(i,j) += tileWind;
			}
		}
	}
}

