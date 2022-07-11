#ifndef __PLACE_OPERATORS_H__
#define __PLACE_OPERATORS_H__

#include "..\Game\Universe.h"
#include "ScanPoly.h"
#include "IronBuilding.h"
#include "UnitItemResource.h"

class Archive;

float angleBeetwenVectors(const Vect2f& vect1, const Vect2f& vect2);
////////////////////////////////////////////
//	Placement Scan Ops
////////////////////////////////////////////
class WeaponScanOp
{
public:
	WeaponScanOp(const UnitActing* unit, Player& aiPlayer, float scanRadius, WeaponPrmReference weaponPrm);

	void checkPosition(const Vect2f& pos);
	bool valid() {return !invalidPosition_;}

	void operator()(UnitBase* unit2);
	
protected:
	Player& aiPlayer_;
	const UnitActing* unit_;
	const AttributeBase* attrVirtualUnit;
	WeaponPrmReference weaponPrm_;
	int scan_radius;
	
	bool invalidPosition_;
};

class WeaponUnitScanOp
{
public:
	// aimUnit - true = мой юнит, false = вражеский юнит
	// health - true = учитывать здоровье, false = не учитывать здоровье
	WeaponUnitScanOp(const UnitActing* unit, Player& aiPlayer, WeaponPrmReference weaponPrm, bool aimUnit, bool health);

	void checkPosition(const Vect2f& pos);
	bool valid() const {return foundPosition_;}
	const Vect3f& foundPos() const {return position_;}
	UnitBase* foundUnit() const { return targetUnit_;}

	void operator()(UnitBase* unit2);

protected:
	Player& aiPlayer_;
	const UnitActing* unit_;
	UnitBase* targetUnit_;

	float maxCriteria;
	float bestDist_;
	WeaponPrmReference weaponPrm_;
	Vect3f position_;

	bool aimUnit_;
	bool foundPosition_;
	bool health_;
	
};


class RadiusScanOp
{
public:
	RadiusScanOp(const UnitReal* unit,const AttributeBase* attr, Player& aiPlayer, float extraRadius);

	void checkPosition(const Vect2f& pos);
	bool valid() {return !invalidPosition_;}

	void operator()(UnitBase* unit2);
	
protected:
	Player& aiPlayer_;
	const UnitReal* unit_;
	const AttributeBase* attr_;
	int scan_radius;

	bool invalidPosition_;
	float extraRadius_;

};


class PlaceScanOp 
{
public:
	PlaceScanOp(UnitReal* unitToOrient, const AttributeBase* attribute, Player& aiPlayer, 
				bool closeToEnemy, float radius, int maxBuildingOfThisType, float extraDistance);

	bool checkEnemyDirection(Vect2f posCenter, Vect2f posInstall, float bestAngle = FLT_INF);
	void checkPosition(const Vect2f& pos);
	void operator()(UnitBase* unit);

	bool found(){ return foundPosition_; }
	Vect2f bestPosition(){ return bestPosition_; }

	void show(Vect2f pos, const sColor4c& color);

protected:

	bool checkBuildingPosition(const AttributeBuilding* attr, const Vect2f& position, Player* player, bool checkUnits, Vect2f& snapPosition_) const;

	UnitReal* unitToOrient_;
	Player& aiPlayer_;
    const AttributeBase* attribute_;
	bool closeToEnemy_;
	float radius_;
	int maxLimit_;
	float extraDistance_;
	int count_;

	bool foundPosition_;
	Vect2f bestPosition_;

	Vect2f installBound;
	Vect2f orientBound;
	Vect2f fullBound;
};

class ScanGroundLineOp
{
	int cnt, max;
	int x0_, y0_, sx_, sy_;
	bool building_;

public:
	ScanGroundLineOp(int x0,int y0,int sx,int sy)
	{
		x0_ = x0;
		y0_ = y0;
		sx_ = sx;
		sy_ = sy;
		cnt = max = 0;
		building_ = false;
	}
	void operator()(int x1,int x2,int y)
	{
		xassert((x1 - x0_) >= 0 && (x2 - x0_) < sx_ && (y - y0_) >= 0 && (y - y0_) < sy_);

		max += x2 - x1 + 1;
		unsigned short* buf = vMap.GABuf + vMap.offsetGBufWorldC(0, y);
		while(x1 <= x2){
			unsigned short p = *(buf + vMap.XCYCLG(x1 >> kmGrid));
			if(!(p & ( GRIDAT_BUILDING | GRIDAT_BASE_OF_BUILDING_CORRUPT))){
				cnt++;
			}
			else if(p & GRIDAT_BUILDING)
				building_ = true;

			x1++;
		}
	}
	bool valid() 
	{ 
		xassert(cnt <= max && "Bad Max");
		return cnt == max; 
	}
	bool building() const { return building_; }
};

class UnitTargetScanOp
{
public:
	UnitTargetScanOp(UnitInterface* unit, Player* player);

	bool operator()(UnitBase* unit);

	UnitInterface* unit(){ return aimUnit_; }

private:

	Vect2f position_;
	UnitInterface* aimUnit_;
	UnitInterface* unit_;

	Player* player_;
};


#endif //__PLACE_OPERATORS_H__
