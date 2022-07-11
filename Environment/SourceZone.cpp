#include "StdAfx.h"

#include "Serialization.h"
#include "SourceZone.h"

#include "..\Util\RangedWrapper.h"
#include "RenderObjects.h"
#include "ExternalShow.h"
#include "Squad.h"
#include "..\Environment\Environment.h"
#include "..\Water\Water.h"
#include "IronBuilding.h"
#include "ObjectSpreader.h"
#include "..\Game\Universe.h"
#include "TypelibraryImpl.h"
#include "..\Util\MillisecondsWrapper.h"

// ------------------- SourceZone

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceZone, ZoneEditType, "ZoneEffectType");
REGISTER_ENUM_ENCLOSED(SourceZone, ZONE_AFFECT, "Зона гоздействия");
REGISTER_ENUM_ENCLOSED(SourceZone, ZONE_WALK_EFFECT, "Луч света");
REGISTER_ENUM_ENCLOSED(SourceZone, ZONE_GENERATOR, "Зона генерации");
END_ENUM_DESCRIPTOR_ENCLOSED(SourceZone, ZoneEditType);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceDetector, ZoneEditType, "ZoneDetectorType");
REGISTER_ENUM_ENCLOSED(SourceDetector, ZONE_DETECTOR, "Детектор");
REGISTER_ENUM_ENCLOSED(SourceDetector, ZONE_HIDER, "Генератор невидимости");
END_ENUM_DESCRIPTOR_ENCLOSED(SourceDetector, ZoneEditType);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceZone, UnitGenerationMode, "Появление объектов");
REGISTER_ENUM_ENCLOSED(SourceZone, GENERATION_MODE_TERRAIN, "Выброс из центра");
REGISTER_ENUM_ENCLOSED(SourceZone, GENERATION_MODE_ZONE, "Равномерно по зоне");
REGISTER_ENUM_ENCLOSED(SourceZone, GENERATION_MODE_SKY, "На заданной высоте");
END_ENUM_DESCRIPTOR_ENCLOSED(SourceZone, UnitGenerationMode);

SourceZone::SourceZone()
: SourceBase()
//, editable_(true)
, editType_(ZONE_AFFECT)
{
	pose_ = Se3f::ID;

    effectRadius_.set (0.2f, 0.3f);
    effectScale_.set (0.8f, 1.2f);

	effectTime_ = 0;

    radius_ = 10;

	enableRotation_ = true;

	createBuiltBuilding_ = false;

    unitAnglePsi_.set(0, 360);
    unitAngleTheta_.set(0, 0);

    unitGenerationMode_ = GENERATION_MODE_ZONE;
    unitGenerationHeight_ = 0.0f;
	
    unitVelocity_.set(50, 50);
    unitAngularVelocity_.set(1, 1);

	wander_time = 20;
	walk_effect_range = Rangef(10, 10);

	unitCount_ = 1;
	unitCur_ = 1;
	unitGenerationInterval_ = 0;
	unitGenerationOnce_ = false;

	generateByRadius_ = false;
}

SourceZone::SourceZone(const SourceZone& original)
: SourceBase(original)
//, editable_(original.editable_)
, editType_(original.editType_)
, enableRotation_(original.enableRotation_)
, effectRadius_(original.effectRadius_)
, effectScale_(original.effectScale_)
, effectAttribute_(original.effectAttribute_)
, effectTime_(original.effectTime_)
, damage_(original.damage_)
, abnormalState_(original.abnormalState_)
, projectileReference_(original.projectileReference_)
, unitReference_(original.unitReference_)
, unitAnglePsi_(original.unitAnglePsi_)
, createBuiltBuilding_(original.createBuiltBuilding_)
, unitAngleTheta_(original.unitAngleTheta_)
, unitVelocity_(original.unitVelocity_)
, unitAngularVelocity_(original.unitAngularVelocity_)
, unitGenerationMode_(original.unitGenerationMode_)
, unitGenerationOnce_(original.unitGenerationOnce_)
, unitGenerationPeriod_(original.unitGenerationPeriod_)
, unitGenerationHeight_(original.unitGenerationHeight_)
, unitCount_(original.unitCount_)
, unitCur_(original.unitCur_)
, unitGenerationInterval_(original.unitGenerationInterval_)
, wander_time(original.wander_time)
, walk_effect_range(original.walk_effect_range)
, generateByRadius_(original.generateByRadius_)
{
}

SourceZone::~SourceZone()
{
	for(EffectControllers::iterator it = effectControllers_.begin(); it != effectControllers_.end(); ++it)
		it->effect.release();
}

void SourceZone::serialize(Archive& ar)
{
    SourceBase::serialize(ar);

    ar.serialize(editType_, "editType", 0);

	bool enableEffect = true;
	if(universe() && universe()->userSave()){ 
		int effectTime = 0;
		if(ar.isOutput() && !effectControllers_.empty())
			effectTime = effectControllers_.begin()->effect.getTime();
		ar.serialize(effectTime, "effectTime", 0);
		if(ar.isInput())
			effectTime_ = effectTime;
	}
	ar.serialize(effectAttribute_, "effectAttribute", "эффект");

	if(editType_ != ZONE_WALK_EFFECT){
		ar.serialize(effectScale_, "effectScale", "масштаб эффектов");
		ar.serialize(effectRadius_, "effectRadius", "радиус эффектов");
	}
    
    if(editType_ == ZONE_GENERATOR){
        ar.openBlock("unitGeneration", "генерация объектов");
		
		ar.serialize(unitGenerationMode_, "generationMode", "место появления");

		if(unitGenerationMode_ == GENERATION_MODE_ZONE){
			ar.serialize(unitReference_, "unitReference", "&Юнит");
			if(ar.isEdit() && unitReference_ && unitReference_->isBuilding())			
				ar.serialize(createBuiltBuilding_, "createBuiltBuilding", "Создавать достроенные здания");
		}
		else
			ar.serialize(projectileReference_, "projectile", "&Снаряд");

		ar.serialize(unitGenerationOnce_, "unitGenerationOnce", "Однократно");
		if(!unitGenerationOnce_)
			ar.serialize(unitGenerationPeriod_, "unitGenerationPeriod", "периодичность появления");
		ar.serialize(unitCount_, "unitCount", "&количество юнитов");
		if(unitCount_ < 1) 
			unitCount_ = 1;
		unitCur_ = 0;
		switch(unitGenerationMode_){
		case GENERATION_MODE_SKY:
			ar.serialize(unitGenerationHeight_, "unitGenerationHeight", "высота появления");
		case GENERATION_MODE_TERRAIN:
			ar.serialize(unitAnglePsi_, "unitAnglePsi", "направление");
			ar.serialize(unitAngleTheta_, "unitAngleTheta", "отклонение от вертикали");
			ar.serialize(unitVelocity_, "unitVelocity", "начальная скорость");
			ar.serialize(unitAngularVelocity_, "unitAngularVelocity", "скорость вращения");
			break;
		case GENERATION_MODE_ZONE:
			ar.serialize(MillisecondsWrapper(unitGenerationInterval_), "unitGenerationInterval", "интервал появления");
			ar.serialize(unitGenerationHeight_, "unitGenerationHeight", "высота появления");
			ar.serialize(generateByRadius_, "generateByRadius", "генерировать по радиусу");
			unitCur_ = unitCount_;
		default:
			;
		}
        
		ar.closeBlock();
    } else {
		if(editType_ == ZONE_WALK_EFFECT){
			ar.serialize(RangedWrapperi(wander_time, 5, 60) , "wander_time", "Период блуждания");
			ar.serialize(walk_effect_range, "walk_effect_range", "Радиус действия блуждающего эффекта");
		}
        ar.serialize(damage_, "Damage", "повреждения");
        ar.serialize(abnormalState_, "abnormalState", "воздействие на юниты");
    }


	serializationApply(ar);
	//if(ar.isInput() && enabled())
    //    initZone();
}

void SourceZone::start()
{
	__super::start();

	effectInit();
	effectStart();
	setScanEnvironment(true);
}

void SourceZone::stop()
{
	__super::stop();

	effectStop();
	setScanEnvironment(false);
}

Se3f SourceZone::currentPose(){
	if(editType_ == ZONE_WALK_EFFECT && active() && !effectControllers_.empty()){
		return  effectAttribute_.bindOrientation() ? Se3f(orientation(), effectControllers_.front().effect.position()) : Se3f(QuatF::ID, effectControllers_.front().effect.position());
	}
	return __super::currentPose();
}

void SourceZone::quant()
{
	__super::quant();

	if(!active()) return;
	
	int eff_count = 0;
	EffectControllers::iterator it;
	for(it = effectControllers_.begin(); it != effectControllers_.end(); ++it){
		if(it->effect.logicQuant())
			eff_count++;
	}

	if(editType_ == ZONE_GENERATOR){
		if(!isUnderEditor()){
			if(!unitGenerationTimer_() && !unitIntervalTimer_()){
				generateUnits();
				if(unitCur_){
					unitIntervalTimer_.start(unitGenerationInterval_);
				}else{
					if(unitGenerationOnce_){
						kill();
					}else{
						unitGenerationTimer_.start((unitGenerationPeriod_.minimum() + logicRNDfabsRnd(unitGenerationPeriod_.length())) * 1000);
						if(unitGenerationMode_ == GENERATION_MODE_ZONE)
							unitCur_ = unitCount_;
					}
				}
			}
		}
	} 
	else {
		if(editType_ == ZONE_WALK_EFFECT){
			xassert (wander_time > 0);
			float da = 2 * M_PI / (10 * wander_time), rr = radius_ / 2.5;
			FOR_EACH (effectControllers_, it) {
				float angle = it->angle + da;
				it->effect.setPositionDelta(Vect2f(rr * (sinf(it->a*angle) + cosf(it->b*angle)), rr * (sinf(it->c*angle) + cosf(it->d*angle))));
				if(angle >= 2 * M_PI)
					generate_effect_modificators(*it);
				else
					it->angle = angle;
			}
		}
		if(!effectControllers_.empty() && !eff_count) // эффектов не осталось - надо прибить;
			kill();
	}
	
	if(!effectControllers_.empty())
		sound_.setVolume(float(eff_count)/float(effectControllers_.size()));

}

struct CircleInRadius {
    CircleInRadius (float radius) : radius_ (radius) {}
    inline bool operator() (const ObjectSpreader::Circle& circle) const {
        return (circle.position.norm() + circle.radius < radius_);
    }
    float radius_;
};


void SourceZone::generate_effect_modificators(EffectControllerNode &node){
	int a, b, c, d;
	do{
		do{
			a = logicRNDinterval(2, 7);
			a = logicRNDfrand() > .5 ? a: -a;
			c = logicRNDinterval(2, 7);
			c = logicRNDfrand() > .5 ? c: -c;
		}while (SIGN(a) == SIGN(c));
		do{
			b = 1 + logicRNDinterval(1, 3) << 1;
			d = 1 + logicRNDinterval(1, 3) << 1;
		}while(b == d);
	}while(abs(a)==abs(b) || abs(c) == abs(d));
	node.a = a;
	node.b = b;
	node.c = c;
	node.d = d;
	node.angle = (float)0;
	node.effect_radius = logicRNDfabsRndInterval(walk_effect_range.minimum(), walk_effect_range.maximum());
}

void SourceZone::effectInit()
{
    effectStop();
    effectControllers_.clear();

	if(effectAttribute_.isEmpty())
		return;

	if(editType_ == ZONE_WALK_EFFECT){
		
		EffectControllerNode node(this, Vect2f(radius_/2.f, radius_/2.f));
		generate_effect_modificators(node);
		effectControllers_.push_back(node);
	
	}else{

		ObjectSpreader spreader;
		
		float max_radius = radius() * effectRadius_.maximum();
		float min_radius = radius() * effectRadius_.minimum();
		
		spreader.setRadius (Rangef (min_radius, max_radius));
		spreader.fill (CircleInRadius (radius() + min_radius));

		ObjectSpreader::CirclesList::const_iterator it;

		FOR_EACH (spreader.circles(), it) {
			if (it->active) {
				EffectControllerNode node(this, it->position);
				effectControllers_.push_back(node);
			}
		}
	}

}

void SourceZone::effectStart()
{
	for(EffectControllers::iterator it = effectControllers_.begin(); it != effectControllers_.end(); ++it){
		start_timer_auto();
		it->effect.effectStart(&effectAttribute_, scaleRnd());
		it->effect.moveToTime(effectTime_);
	}
}

void SourceZone::effectStop()
{
    for(EffectControllers::iterator it = effectControllers_.begin(); it != effectControllers_.end(); ++it)
        it->effect.release();
}

float SourceZone::scaleRnd() const
{
	const int num_scale_steps = 10;
	float scale_step = effectScale_.length()/float(num_scale_steps);
	if(scale_step < FLT_EPS)
		return effectScale_.minimum();

	return effectScale_.minimum() + float(logicRND(num_scale_steps)) * scale_step;
}

void SourceZone::setEffect (const EffectAttribute& effectAttribute)
{
    effectAttribute_ = effectAttribute;
    effectInit ();
    effectStart ();
}

void SourceZone::apply(UnitBase* target)
{
	SourceBase::apply(target);

	if(!active())
		return;
	
	if(!target->alive())
		return;

	if(!effectAttribute_.isEmpty())
		if((effectAttribute_.switchOffUnderWater() || effectAttribute_.switchOffUnderLava() || effectAttribute_.switchOffByDay())
			&& ((effectAttribute_.switchOffByDay() && environment->isDay())
				|| (((environment->waterIsLava() && effectAttribute_.switchOffUnderLava()) || (!environment->waterIsLava() && effectAttribute_.switchOffUnderWater()))
				&& environment->water()->isUnderWater(target->position(), target->radius())	)
				)
		)
		{
			return;
		}
	
	if(editType_ == ZONE_WALK_EFFECT){
		for(EffectControllers::iterator it = effectControllers_.begin(); it != effectControllers_.end(); ++it)
			if(!isUnderEditor() && target->position2D().distance(it->effect.position()) < it->effect_radius + target->radius()){
				applyDamage(target);
				if(abnormalState().isEnabled())
					target->setAbnormalState(abnormalState(), owner());
				return;
			}
	}
	else if(!isUnderEditor()) {
		xassert(!breakWhenApply_);
		applyDamage(target);
		if(abnormalState().isEnabled())
			target->setAbnormalState(abnormalState(), owner());
		return;
	}	

}

void SourceZone::generateUnits()
{
	if(unitGenerationMode_ == GENERATION_MODE_ZONE){
		
		const AttributeBase* attr = unitReference_.get();
		if(!attr){
			unitCur_ = 0;
			return;
		}

		ObjectSpreader spreader;

		float angle(2 * M_PI / unitCount_);
		do{
			if(!player()->checkUnitNumber(attr)){
				unitCur_ = 0;
				return;
			}

			UnitBase* unit = player()->buildUnit(unitReference_);
			if(unit->attr().isObjective())
				safe_cast<UnitObjective*>(unit)->registerInPlayerStatistics();
			Vect2f point;
			if(generateByRadius_){
				point = Vect2f(position());
				float angleCur = angle * unitCur_;
				point.x += radius() * sinf(angleCur);
				point.y += radius() * cosf(angleCur);
			}
			else
				point = Vect2f(position()) + spreader.addCircle(unit->radius()).position;
			float groundZ = vMap.GetApproxAlt(point.xi(), point.yi());
			Vect3f unitPosition(point.x, point.y, groundZ + unitGenerationHeight_);
			if(unitGenerationHeight_ > FLT_EPS && unit->rigidBody()){
				unit->rigidBody()->awake();
				safe_cast<RigidBodyUnit*>(unit->rigidBody())->setFlyingHeightCurrent(unitGenerationHeight_);
				safe_cast<RigidBodyUnit*>(unit->rigidBody())->enableFlyDownModeByTime(unit->attr().chainFlyDownTime);
				safe_cast<UnitReal*>(unit)->startState(StateBirthInAir::instance());
			}
			if(unit->attr().isBuilding() && !createBuiltBuilding_)
				safe_cast<UnitBuilding*>(unit)->startConstruction();
			unit->setPose(Se3f(QuatF::ID, unitPosition), true);
			if(unit->attr().isLegionary()){
				UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
				UnitSquad* squad = safe_cast<UnitSquad*>(player()->buildUnit(&*legionary->attr().squad));
				squad->setPose(Se3f(QuatF::ID, unitPosition), true);
				squad->addUnit(legionary, false);
			}
			--unitCur_;
		}while(unitCur_ && !unitGenerationInterval_);
	}
	else{
		for(int i = 0; i < unitCount_; i++){
			UnitBase* unit = player()->buildUnit(&*projectileReference_);
			
			float psi = G2R(unitAnglePsi_.minimum() + logicRNDfabsRnd(unitAnglePsi_.length()));
			float theta = G2R(90.0f + unitAngleTheta_.minimum() + logicRNDfabsRnd(unitAngleTheta_.length()));

			if(unitGenerationMode_ == GENERATION_MODE_SKY)
				theta = -theta;

			QuatF rot(psi, Vect3f::K);
			rot.postmult(QuatF(theta, Vect3f::I));

			Vect2f trans (position().x, position().y);

			float angle = logicRNDfabsRnd(M_PI * 2.0f);
			float r = logicRNDfabsRnd(radius());

			trans.x += r * cos(angle);
			trans.y += r * sin(angle);
		    
			Vect3f trans3d = (unitGenerationMode_ == GENERATION_MODE_TERRAIN) ? 
				To3D(trans) : Vect3f(trans.x, trans.y, unitGenerationHeight_);
			
			if(unit->rigidBody() && unit->rigidBody()->isBox()){
				trans3d.z += unit->rigidBody()->boundRadius();
				safe_cast<RigidBodyBox*>(unit->rigidBody())->setForwardVelocity(unitVelocity_.minimum() + logicRNDfabsRnd(unitVelocity_.length()));
				Vect3f v(logicRNDfrnd(1.f), logicRNDfrnd(1.f), logicRNDfrnd(1.f));
				v.scale(unitAngularVelocity_.minimum() + logicRNDfabsRnd(unitAngularVelocity_.length()));
				safe_cast<RigidBodyBox*>(unit->rigidBody())->setAngularVelocity(v);
				safe_cast<RigidBodyBox*>(unit->rigidBody())->avoidCollisionAtStart();
			}

			unit->setPose(Se3f(rot, trans3d), true);

			if(unit->rigidBody() && unit->rigidBody()->isMissile()){
				Vect3f v(0, unitVelocity_.minimum() + logicRNDfabsRnd(unitVelocity_.length()), 0);
				rot.xform(v);
				safe_cast<RigidBodyMissile*>(unit->rigidBody())->setVelocity(v);

				v = Vect3f(logicRNDfrnd(1.f), logicRNDfrnd(1.f), logicRNDfrnd(1.f)) * 
					(unitAngularVelocity_.minimum() + logicRNDfabsRnd(unitAngularVelocity_.length()));
				safe_cast<RigidBodyMissile*>(unit->rigidBody())->setAngularVelocity(v);
				
				safe_cast<RigidBodyMissile*>(unit->rigidBody())->missileStart();
			}
		}
	}
}

void SourceZone::initZone()
{
	xassert(enabled_);

    effectInit();
    if(active_)
        effectStart();
}

void SourceZone::applyDamage(UnitBase* target) const
{
	int time = (lifeTime_ > 0) ? lifeTime_ : MAX_DEFAULT_LIFETIME;

	float dt = (time >= 200) ? (float(logicTimePeriod) / float(time)) : 1.f;
	ParameterSet dmg = damage();
	dmg *= dt;

	target->setDamage(dmg, owner());
}

void SourceZone::showDebug() const
{
	__super::showDebug();

	if(showDebugSource.zoneDamage)
		damage_.showDebug(position(), MAGENTA);
	
	if(showDebugSource.zoneStateDamage)
		abnormalState_.damage().showDebug(position(), MAGENTA);

	EffectControllers::const_iterator it;
	FOR_EACH(effectControllers_, it)
		it->effect.showDebugInfo();
}

bool SourceZone::getParameters(WeaponSourcePrm& prm) const
{
	prm.setDamage(damage_);
	prm.setAbnormalState(abnormalState_);

	return true;
}

bool SourceZone::setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target)
{
	damage_ = prm.damage();
	abnormalState_ = prm.abnormalState();

	return true;
}

/*	------====== Detector Source ========---------						*/

SourceDetector::SourceDetector()
: SourceDamage()
{
	editType_ = ZONE_DETECTOR;
}

void SourceDetector::serialize(Archive& ar)
{
	ar.serialize(editType_, "editType", "Режим работы");
	__super::serialize(ar);
}

class SourceScanOperator
{
	float radius_;
	Vect2f position_;
	SourceDetector* owner_;

public:
	SourceScanOperator(SourceDetector* zone = 0){
		if(zone)
			setOwner(zone);
	}
	void setOwner(SourceDetector* zone){
		owner_ = zone;
		radius_ = owner_->radius();
		position_.set(owner_->position().x, owner_->position().y);
	}

	void operator()(SourceBase* src){
		if(src->position2D().distance2(position_) < sqr(radius_ + src->radius()))
			owner_->sourceApply(src);
	}
};

void SourceDetector::quant()
{
	__super::quant();

	if(!active())
		return;

	static SourceScanOperator scanOp;
	scanOp.setOwner(this);
	environment->sourceGrid.Scan(position().xi(), position().yi(), round(radius()), scanOp);
}

void SourceDetector::sourceApply(SourceBase* source)
{
	source->setDetected();
}

void SourceDetector::apply(UnitBase* target)
{
	__super::apply(target);

	if(UnitLegionary* unit = dynamic_cast<UnitLegionary*>(target))
		unit->setVisibility(editType_ == ZONE_DETECTOR);
}

bool SourceDetector::canApply(const UnitBase* target) const
{
	if(affectMode_ != AFFECT_ENEMY_UNITS || target->attr().isProjectile())
		return __super::canApply(target);

	return target->alive() && (player()->isEnemy(target) || (!player()->isWorld() && target->player()->isWorld()));
}

/*	------====== Freezing Source ========---------						*/

SourceFreeze::SourceFreeze():
SourceDamage()
{
}

void SourceFreeze::apply(UnitBase* target)
{
	__super::apply(target);
	if(active() && target->attr().isObjective())
		safe_cast<UnitReal*>(target)->makeFrozen();
}
