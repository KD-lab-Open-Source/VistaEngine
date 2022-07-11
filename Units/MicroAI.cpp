#include "stdafx.h"

#include "Player.h"
#include "UnitActing.h"
#include "IronBuilding.h"
#include "Squad.h"
#include "MicroAI.h"
#include "Environment\SourceShield.h"
#include "GlobalAttributes.h"

// веса параметров для расчёта приоритета цели
const float WEIGHT_MILITARY = 1.f;
const float WEIGHT_OFFENSIVE = 3.f;
const float WEIGHT_UNIT_CLASS = 1.f;
const float WEIGHT_DISTANCE = 1.f;
const float WEIGHT_FIRE_TARGET = 5.f;
const float WEIGHT_AGRESSOR = 1.f;
const float WEIGHT_PRIORITY = 10.f;

const float WEIGHT_OLD_TARGET = 0.7f;

const int SCAN_ENEMIES_MAX = 16;
const int SCAN_ALLIES_MAX = 16;

float microAI_TargetPriority(const UnitActing* unit, const UnitInterface* target)
{
	float priority = 0.f;

	float class_priority = 0.0f;
	switch(target->attr().unitAttackClass){
	case ATTACK_CLASS_MEDIUM:
		class_priority = 0.5f;
		break;
	case ATTACK_CLASS_HEAVY:
		class_priority = 1.f;
		break;
	case ATTACK_CLASS_AIR_MEDIUM:
		class_priority = 0.5f;
		break;
	case ATTACK_CLASS_AIR_HEAVY:
		class_priority = 1.f;
		break;
	}

	priority += WEIGHT_UNIT_CLASS * class_priority;

	if(const UnitActing* unitActing = safe_cast<const UnitActing*>(target)){
		if(unitActing->isAttackedFireTarget(unit))
			priority += WEIGHT_AGRESSOR;

		if(unitActing->isFireTarget(unit))
			priority += WEIGHT_FIRE_TARGET;

		if(!unitActing->weaponSlots().empty()){
			for(UnitActing::WeaponSlots::const_iterator it = unitActing->weaponSlots().begin(); it != unitActing->weaponSlots().end(); ++it){
				if(it->weapon()->isOffensive()){
					priority += WEIGHT_OFFENSIVE;
					break;
				}
			}
			priority += WEIGHT_MILITARY;
		}
	}

	return priority;
}

void MicroAI_Scaner::operator()(UnitBase* unit)
{
	start_timer_auto();

	if(!unit->alive() || !unit->attr().isActing() || owner_ == unit)
		return;

	UnitInterface* ui = safe_cast<UnitInterface*>(unit);

	Targets::const_iterator it = std::find(targets_.begin(), targets_.end(), ui);
	if(it != targets_.end())
		return;

	if(canAttack(ui))
		addTarget(ui);

	if(weaponID_) return;

	if(owner_->isEnemy(unit)){
		if(enemies_.size() < SCAN_ENEMIES_MAX){
			Enemies::const_iterator it = std::find(enemies_.begin(), enemies_.end(), unit);
			if(it == enemies_.end())
				enemies_.push_back(ui);
		}
	}
	else {
		if(owner_->attr().attackTargetNotificationMode & AttributeBase::TARGET_NOTIFY_ALL){
			if(allies_.size() < SCAN_ALLIES_MAX){
				UnitActing* ua = safe_cast<UnitActing*>(unit);
				if(ua->unitState() == UnitReal::AUTO_MODE && !ua->targetUnit()){
					float r = owner_->position2D().distance2(ua->position2D());
					if(r <= notificationRadius_ && (!(owner_->attr().attackTargetNotificationMode & AttributeBase::TARGET_NOTIFY_SQUAD)
						|| !owner_->getSquadPoint() || ua->getSquadPoint() != owner_->getSquadPoint())){
							Allies::const_iterator it = std::find(allies_.begin(), allies_.end(), unit);
							if(it == allies_.end())
								allies_.push_back(ua);
					}
				}
			}
		}
	}
}

UnitInterface* MicroAI_Scaner::processTargets()
{
	start_timer_auto();

	if(!weaponID_){
		UnitInterface* target = assignTargets(true);

		if(owner_->attr().attackTargetNotificationMode & AttributeBase::TARGET_NOTIFY_SQUAD){
			if(UnitSquad* sp = owner_->getSquadPoint()){
				UnitActing* first_owner = owner_;
				int unit_count = 0;
				bool leader_notify = false;
				const LegionariesLinks& units = sp->units();
				for(LegionariesLinks::const_iterator it = units.begin(); it != units.end(); ++it){
					UnitActing* ua = *it;

					if(ua == sp->getUnitReal())
						leader_notify = true;

					if(ua != first_owner && ua->unitState() == UnitReal::AUTO_MODE && !ua->targetUnit()){
						targets_.clear();
						owner_ = ua;
						for(Enemies::iterator it1 = enemies_.begin(); it1 != enemies_.end(); ++it1){
							if(canAttack(*it1))
								addTarget(*it1);
						}
						UnitInterface* p = assignTargets(false);
						owner_->setTargetUnit(p);
						if(++unit_count >= SCAN_ALLIES_MAX)
							break;
					}
				}

				if(!leader_notify){
					UnitActing* ua = sp->getUnitReal();

					if(ua != first_owner && ua->unitState() == UnitReal::AUTO_MODE && !ua->targetUnit()){
						targets_.clear();
						owner_ = ua;
						for(Enemies::iterator it1 = enemies_.begin(); it1 != enemies_.end(); ++it1){
							if(canAttack(*it1))
								addTarget(*it1);
						}
						UnitInterface* p = assignTargets(false);
						owner_->setTargetUnit(p);
					}
				}
			}
		}

		if(owner_->attr().attackTargetNotificationMode & AttributeBase::TARGET_NOTIFY_ALL){
			for(Allies::iterator it = allies_.begin(); it != allies_.end(); ++it){
				targets_.clear();
				owner_ = *it;
				for(Enemies::iterator it1 = enemies_.begin(); it1 != enemies_.end(); ++it1){
					if(canAttack(*it1))
						addTarget(*it1);
				}
				UnitInterface* p = assignTargets(false);
				owner_->setTargetUnit(p);
			}
		}
		return target;
	}
	else
		return selectedWeaponTarget();

}

bool MicroAI_Scaner::isTargetFree(const UnitInterface* target) const
{
	for(UnitActing::Weapons::const_iterator iw = owner_->weapons().begin(); iw != owner_->weapons().end(); ++iw){
		if((*iw)->groupType() == currentGroup_ && (*iw)->weaponPrm()->exclusiveTarget() && (*iw)->autoTarget() == target)
			return false;
	}
	return true;
}

void MicroAI_Scaner::addTarget(UnitInterface* target)
{
	TargetData data(target);
	targets_.push_back(data);
}

void MicroAI_Scaner::updateTarget(TargetData& target)
{
	target.priority_ = microAI_TargetPriority(owner_, target.target_);
	target.distance_ = owner_->position2D().distance(target.target_->position2D()) - owner_->radius() - target.target_->radius();
}

float MicroAI_Scaner::targetPriority(const TargetData& target,  const WeaponBase* weapon) const 
{
	float optimal_distance = weapon->fireRadiusOptimal();
	return (target.priority_ + WEIGHT_DISTANCE / (1 + sqr(target.distance_ - optimal_distance))) * WEIGHT_PRIORITY * (1.f + float(weapon->priority()));
}

bool MicroAI_Scaner::isVisible(UnitBase* unit, bool trace_visibility) const
{
	start_timer_auto();

	if(owner_->isEnemy(unit) && unit->isUnseen())
		return false;

	if(owner_->player()->fogOfWarMap()){
		if(owner_->player()->fogOfWarMap()->checkFogStateInCircle(Vect2i(unit->position().xi(), unit->position().yi()), unit->radius())){
			if(!SourceShield::traceShieldsCoherence(owner_->position(), unit->position(), owner_->player()))
				return false;
		}
		else
			return false;
	}

	if(trace_visibility && GlobalAttributes::instance().enableUnitVisibilityTrace){
		if(!owner_->isInSightSector(unit) || !weapon_helpers::traceVisibility(owner_, unit))
			return false;
	}

	return true;
}

bool MicroAI_Scaner::checkTransport(UnitBase* unit) const
{
	if(unit->attr().isActing() && unit->attr().isTransport()){
		UnitActing* ua = safe_cast<UnitActing*>(unit);
		if(ua->attackModeAttr().disableEmptyTransportAttack() && ua->transportEmpty())
			return false;
	}

	return true;
}

bool MicroAI_Scaner::canAttack(UnitInterface* unit) const
{
	start_timer_auto();

	if(unit->attr().isBuilding() && !safe_cast<UnitBuilding*>(unit)->isConnected())
		return false;

	if(!checkTransport(unit))
		return false;

	if(!weaponID_){
		if(!unit->attr().excludeFromAutoAttack){
			switch(owner_->attackMode().autoTargetFilter()){
			case AUTO_ATTACK_BUILDINGS:
				if(!unit->attr().isBuilding())
					return false;
				break;
			case AUTO_ATTACK_UNITS:
				if(!unit->attr().isLegionary())
					return false;
				break;
			}

			if(owner_->canAutoAttackTarget(WeaponTarget(unit)) && isVisible(unit))
				return true;
		}
	}
	else {
		if(isVisible(unit) && owner_->canAttackTarget(WeaponTarget(unit, weaponID_)))
			return true;
	}

	return false;
}

UnitInterface* MicroAI_Scaner::assignTargets(bool add_targets_to_list)
{
	start_timer_auto();

	for(Targets::iterator it = targets_.begin(); it != targets_.end(); ++it)
		updateTarget(*it);

	TargetData best_target(0);

	excludeTargets_.clear();
	currentGroup_ = 0;

	bool attack_disabled = owner_->isAutoAttackDisabled();

	for(UnitActing::Weapons::const_iterator iw = owner_->weapons().begin(); iw != owner_->weapons().end(); ++iw){
		WeaponBase* weapon = *iw;
		if(weapon->groupType() != currentGroup_){
			currentGroup_ = weapon->groupType();
			excludeTargets_.clear();
		}
		if(weapon->isAccessible() && weapon->canAutoFire() && owner_->fireWeaponModeCheck(weapon)){
			UnitInterface* target = weapon->autoTarget();
			weapon->setAutoTarget(0);

			float priority = 0.f;
			if(target){
				if((!weapon->weaponPrm()->exclusiveTarget() || (!isTargetExcluded(target) && isTargetFree(target))) && isVisible(target) && checkTransport(target) && weapon->checkTargetMode(target) && weapon->canAttack(WeaponTarget(target))){
					TargetData data(target);
					updateTarget(data);

					priority = targetPriority(data, weapon);
					if(weapon->fireDistanceCheck(WeaponTarget(target), true))
						priority += WEIGHT_OLD_TARGET;

					if(!weapon->weaponPrm()->disableOwnerMove() && priority > best_target.priority_)
						best_target = TargetData(target, priority);

					if(add_targets_to_list && owner_->isEnemy(target)){
						Enemies::const_iterator ie = std::find(enemies_.begin(), enemies_.end(), target);
						if(ie == enemies_.end())
							enemies_.push_back(target);
					}
				}
				else
					target = 0;
			}

			for(Targets::iterator it = targets_.begin(); it != targets_.end(); ++it){
				if((!weapon->weaponPrm()->exclusiveTarget() || (!isTargetExcluded(it->target_) && isTargetFree(it->target_))) && weapon->checkTargetMode(it->target_) && weapon->canAttack(WeaponTarget(it->target_))){
					if(mode_ != ATTACK_MODE_DEFENCE || weapon->fireDistanceCheck(WeaponTarget(it->target_), true)){
						float p = targetPriority(*it, weapon);
						if(p > priority){
							priority = p;
							target = it->target_;
						}

						if(!weapon->weaponPrm()->disableOwnerMove() && p > best_target.priority_)
							best_target = TargetData(it->target_, p);
					}
				}
			}

			if(!attack_disabled){
				weapon->setAutoTarget(target);
				if(weapon->weaponPrm()->exclusiveTarget())
					excludeTarget(target);
			}
		}
	}

	if(!attack_disabled)
		return best_target.target_;

	return 0;
}

UnitInterface* MicroAI_Scaner::selectedWeaponTarget()
{
	for(Targets::iterator it = targets_.begin(); it != targets_.end(); ++it)
		updateTarget(*it);

	TargetData best_target(0);

	excludeTargets_.clear();
	currentGroup_ = 0;

	for(UnitActing::Weapons::const_iterator iw = owner_->weapons().begin(); iw != owner_->weapons().end(); ++iw){
		WeaponBase* weapon = *iw;
		if(weapon->groupType() != currentGroup_){
			currentGroup_ = weapon->groupType();
			excludeTargets_.clear();
		}
		if(weapon->isAccessible() && weapon->weaponPrm()->ID() == weaponID_){
			UnitInterface* target = 0;
			weapon->setAutoTarget(0);

			float priority = 0.f;

			for(Targets::iterator it = targets_.begin(); it != targets_.end(); ++it){
				if((!weapon->weaponPrm()->exclusiveTarget() || !isTargetExcluded(it->target_)) && weapon->checkTargetMode(it->target_) && weapon->canAttack(WeaponTarget(it->target_, weaponID_))){
					float p = targetPriority(*it, weapon);
					if(p > priority){
						priority = p;
						target = it->target_;
					}

					if(p > best_target.priority_)
						best_target = TargetData(it->target_, p);
				}
			}

			if(weapon->weaponPrm()->exclusiveTarget())
				excludeTarget(target);
		}
	}

	return best_target.target_;
}

MicroAI_NoiseScaner::MicroAI_NoiseScaner(UnitActing* owner) : owner_(owner), target_(0)
{
	targetDist2_ = 0;
	sightRadius2_ = sqr(owner_->sightRadius());
}

void MicroAI_NoiseScaner::operator()(UnitBase* unit)
{
	start_timer_auto();

	if(!unit->alive() || !owner_->isEnemy(unit))
		return;

	float dist2 = owner_->position2D().distance2(unit->position2D());
	float noise_r2 = sqr(unit->noiseRadius());
	if(dist2 < noise_r2){
		if(!target_ || dist2 < targetDist2_){
			target_ = unit;
			targetDist2_ = dist2;
		}
	}
}
