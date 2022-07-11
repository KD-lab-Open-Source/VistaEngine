#ifndef __PLACE_OPERATORS_H__
#define __PLACE_OPERATORS_H__

#include "Game\Universe.h"
#include "IronBuilding.h"
#include "UnitItemResource.h"
#include "Terra\vMap.h"
#include "Environment\SourceShield.h"

class Archive;

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
	RadiusScanOp(const UnitActing* unit,const AttributeBase* attr, Player& aiPlayer, float extraRadius);

	void checkPosition(const Vect2f& pos);
	bool valid() {return !invalidPosition_;}

	void operator()(UnitBase* unit2);
	
protected:
	Player& aiPlayer_;
	const UnitActing* unit_;
	const AttributeBase* attr_;
	int scan_radius;

	bool invalidPosition_;
	float extraRadius_;

};

class PlaceScanOp 
{
public:
	PlaceScanOp(UnitReal* unitToOrient, const AttributeBase* attribute, Player& aiPlayer, 
				bool closeToEnemy, float radius);

	bool checkEnemyDirection(Vect2f posCenter, Vect2f posInstall, float bestAngle = FLT_INF);
	void checkPosition(const Vect2f& pos);
	void operator()(UnitBase* unit);

	bool found(){ return foundPosition_; }
	Vect2f bestPosition(){ return bestPosition_; }

	void show(Vect2f pos, const Color4c& color);

protected:

	bool checkBuildingPosition(const AttributeBuilding* attr, const Vect2f& position, Player* player, bool checkUnits, Vect2f& snapPosition_) const;

	UnitReal* unitToOrient_;
	Player& aiPlayer_;
    const AttributeBase* attribute_;
	bool closeToEnemy_;
	float radius_;
	float priority_;

	bool foundPosition_;
	Vect2f bestPosition_;

	Vect2f installBound;
	Vect2f installPos;
	Vect2f orientBound;
	Vect2f fullBound;
};

class ScanGroundHeightLineOp
{
	int height_;
	CompareOperator compareOperator_;
	bool valid_;
public:
	ScanGroundHeightLineOp(CompareOperator compareOp, int height)
	{
		compareOperator_ = compareOp;
		height_ = height;
		valid_ = true;
	}

	void operator()(int x1,int x2,int y)
	{
		if(valid_)
			if(y >= 0 && x1 >= 0 && y < vMap.GV_SIZE && x2 < vMap.GH_SIZE)
				while(x1 <= x2){
					int z = vMap.gVBuf[vMap.offsetGBuf(x1, y)];
					if(!Condition::compare(z, height_, compareOperator_)){
						valid_ = false;
						break;
					}
					x1++;
				}
	}
	bool valid() 
	{ 
		return  valid_;
	}
};

class ScanGroundDamagedLineOp
{
	int n;
	float Sz2;
	float Sz;

public:
	ScanGroundDamagedLineOp()
	{
		n = 0;
		Sz2 = 0.f;
		Sz = 0.f;
	}

	void operator()(int x1,int x2,int y)
	{
		if(y >= 0 && x1 >= 0 && y < vMap.GV_SIZE && x2 < vMap.GH_SIZE)
			while(x1 <= x2){
				int z = vMap.gVBuf[vMap.offsetGBuf(x1, y)];
				x1++;
				++n;
				Sz += z;
				Sz2 += sqr(z);
			}
	}
	float sigma2() const 
	{ 
		return (Sz2 - sqr(Sz)/n)/(n*(n - 1));
	}
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
		unsigned short* buf = vMap.gABuf + vMap.offsetGBufWorldC(0, y);
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

class ShieldScanOp
{
public:

	ShieldScanOp(const Vect3f& point,  Player* player)
	{
		underShield_ = false;
		position_ = point;
		player_ = player;
	}

	void operator()(SourceBase* src)
	{
		if(underShield_)
			return;

		if(src->type() == SOURCE_SHIELD && src->active()){
			const SourceShield* shield = safe_cast<const SourceShield*>(src);
			if(shield->inZone(position_) && shield->player() != player_){
				underShield_ = true;
				return;
			}
		}
	}
	
	bool underShield() const { return underShield_; }

private:

	Vect3f position_;
	bool underShield_;
	Player* player_;
};

#endif //__PLACE_OPERATORS_H__
