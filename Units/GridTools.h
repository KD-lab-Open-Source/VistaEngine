#ifndef __GRIDTOOLS_H__
#define __GRIDTOOLS_H__

#include "LaunchData.h"
#include "IronLegion.h"
#include "RigidBody.h"
#include "Weapon.h"

struct Segment
{
	Vect3f begin; // начало орезка
	Vect3f direct; // единичное направление
	float length; // длина

	void set(const Vect3f& begin_, const Vect3f& end)
	{
		begin = begin_;
		direct.sub(end, begin);
		length = direct.norm();
		if(length > FLT_EPS)
			direct /= length;
	}

	void set(const Vect3f& begin_, const Vect3f& end,float d)
	{
		begin = begin_;
		direct.sub(end, begin);
		length = d;
		if(length > FLT_EPS)
			direct /= length;
	}
	
	int intersection(const Vect3f& point, float radius)
	{
		Vect3f dr, n;
		dr.sub(point, begin);
		float l = dot(dr, direct);
		if(l < -radius || l > length + radius)
			return 0;
		n.scaleAdd(dr, direct, -l);
		if(n.norm2() > (radius * radius))
			return 0;
		return 1;
	}
};

struct terUnitGridFireCheckOperator
{
	int CollisionGroup;
	Segment CheckProcess;
	UnitBase* Contact;
	const UnitBase* Owner;

	terUnitGridFireCheckOperator(const Vect3f& from,const Vect3f& to,float d,int collision_group,const UnitBase* owner)
	{
		CollisionGroup = collision_group;
		Owner = owner;
		Contact = NULL;
		CheckProcess.set(from,to,d);
	}

	int operator()(UnitBase* p)
	{
		if(p->alive() && (CollisionGroup & p->collisionGroup()) && p != Owner && CheckProcess.intersection(p->position(),p->radius())){
			Contact = (UnitBase*)(p);
			return 0;
		}
		return 1;
	}
};

struct terUnitGridFireCheckIgnoreOperator : terUnitGridFireCheckOperator
{
	UnitBase* IgnorePoint;

	terUnitGridFireCheckIgnoreOperator(const Vect3f& from,const Vect3f& to,float d,int collision_group,const UnitBase* owner)
		: terUnitGridFireCheckOperator(from,to,d,collision_group,owner)
	{
		IgnorePoint = NULL;
	}

	int operator()(UnitBase* p)
	{
		if(p->alive() && (CollisionGroup & p->collisionGroup()) && p != Owner && CheckProcess.intersection(p->position(),p->radius())){
			if(!IgnorePoint && p->attr().IgnoreTargetTrace)
				IgnorePoint = (UnitBase*)p;
			else{
				Contact = (UnitBase*)(p);
				return 0;
			}
		}
		return 1;
	}
};

//---------------------------------------

struct terUnitGridPenetrationDamageOperator
{
	int CollisionGroup;
	Segment CheckProcess;
	UnitBase* Owner;
	ParameterSet Damage;

	terUnitGridPenetrationDamageOperator(const Vect3f& from,Vect3f& to,float d,int collision_group,UnitBase* owner,const ParameterSet& damage)
	{
		CollisionGroup = collision_group;
		Owner = owner;
		CheckProcess.set(from,to,d);
		Damage = damage;
	}

	void operator()(UnitBase* p)
	{
		if(p->alive() && (CollisionGroup & p->collisionGroup()) && Owner->isEnemy(p) && CheckProcess.intersection(p->position(),p->radius()))
			p->setDamage(Damage,Owner);
	}
};

//---------------------------------------

class terUnitGridTeamOffensiveOperator
{
public:
	terUnitGridTeamOffensiveOperator(const UnitReal* owner) : owner_(owner),
		offensivePoint_(NULL)
	{
		position_ = owner_->position2D();

		fireDistanceMin_ = 0;
		fireDistanceMax_ = sqr(owner->fireRadius());
		sightDistance_ = sqr(owner->attr().sightRadius());

		bestFactor_ = 0;

		fireTest_ = false;
	}

	void operator()(UnitBase* p)
	{
		if(owner_->isEnemy(p) && p->alive() && owner_->checkFireClass(p) 
		  && !p->isUnseen()){
			float dist = p->position2D().distance2(position_);
			float f = 1.f/(1.f + dist);

			if(p->possibleDamage() >= p->health())
				f /= 1000 + dist;
			else
				f -= float(p->possibleDamage()) / float(p->health() + 0.1f);

			if(dist > fireDistanceMin_ && dist < sightDistance_){
				bool fire_test = (dist < fireDistanceMax_) ? owner_->fireCheck(WeaponTarget(p)) : false;
				if(fire_test){
					if(!fireTest_ || bestFactor_ < f){
						fireTest_ = true;
						offensivePoint_ = p;
						bestFactor_ = f;
					}
				}
				else {
					if(!fireTest_ && bestFactor_ < f ){
						offensivePoint_ = p;
						bestFactor_ = f;
					}
				}
			}
		}
	}

	UnitBase* offensivePoint(){ return offensivePoint_; }

private:

	const UnitReal* owner_;

	Vect2f position_;

	float fireDistanceMin_;
	float fireDistanceMax_;
	float sightDistance_;

	bool fireTest_;
	UnitBase* offensivePoint_;
	float bestFactor_;
};

//---------------------------------------

class terUnitGridSplashDamageOperator
{
public:
	terUnitGridSplashDamageOperator(UnitBase* source_unit, UnitBase* owner_unit) 
	: sourceUnit_(source_unit), ownerUnit_(owner_unit)
	{
		radius_ = 0; //sourceUnit_->attr().weaponSetup.damage.splashDamageRadius;
		//damage_ = sourceUnit_->attr().weaponSetup.damage.splashDamage;
	}

	void operator()(UnitBase* p)
	{
		if(!radius_)
			return;

		if(p->rigidBody() && p->rigidBody()->diggingModeLagged())
			return;

		if(p->isEnemy(sourceUnit_) && !p->isDocked() && sourceUnit_->position().distance2(p->position()) < sqr(radius_ + p->radius()))
			p->setDamage(damage_,ownerUnit_);
	}

private:
	float radius_;
	ParameterSet damage_;

	UnitBase* sourceUnit_;
	UnitBase* ownerUnit_;
};

class terUnitGridInvisibilityGeneratorOperator
{
public:
	terUnitGridInvisibilityGeneratorOperator(UnitBase* source_unit,float radius_min,float radius_max) : sourceUnit_(source_unit)
	{
		radius_min_ = radius_min * radius_min;
		radius_max_ = radius_max * radius_max;

		position_ = sourceUnit_->position2D();
	}

	void operator()(UnitBase* p)
	{
		const int terInvisibilityTime = 4000;
		if(p->attr().isLegionary() && &p->attr() != &sourceUnit_->attr() && !sourceUnit_->isEnemy(p) && p->alive()){
			UnitLegionary* lp = safe_cast<UnitLegionary*>(p);
			lp->setInvisibility(terInvisibilityTime);
		}
	}

private:

	float radius_min_;
	float radius_max_;

	Vect2f position_;

	UnitBase* sourceUnit_;
};

class terUnitGridAreaDamageOperator
{
public:
	terUnitGridAreaDamageOperator(UnitBase* owner,const Vect2f& position,float radius_min,float radius_max) : owner_(owner)
	{
		radius_min_ = sqr(radius_min);
		radius_max_ = sqr(radius_max);

		position_ = position;
	}

	void operator()(UnitBase* p)
	{
		if(owner_->isEnemy(p) && p->alive() && !p->isDocked() && owner_->checkFireClass(p) 
		  && !p->isUnseen()){
			float dist = p->position2D().distance2(position_);

//			if(dist >= radius_min_ && dist < radius_max_)
//				p->setDamage(owner_->damageData(),owner_);
		}
	}

private:
	float radius_min_;
	float radius_max_;

	Vect2f position_;

	UnitBase* owner_;
};
#endif //__GRIDTOOLS_H__
