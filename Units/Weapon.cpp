#include "StdAfx.h"

#include "Universe.h"
#include "Weapon.h"
#include "WeaponPrms.h"
#include "Squad.h"
#include "UnitActing.h"
#include "UnitPad.h"
#include "UnitEnvironment.h"
#include "IronBullet.h"
#include "IronBuilding.h"
#include "RenderObjects.h"
#include "UnitAttribute.h"
#include "Serialization\ResourceSelector.h"
#include "Water\Water.h"
#include "Water\Ice.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "Environment\SourceTeleport.h"
#include "Environment\SourceShield.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UserInterface.h"
#include "Game\StreamCommand.h"
#include "Environment\SourceZone.h"
#include "Environment\ChainLightningController.h"
#include "Console.h"
#include "Water\CircleManager.h"
#include "Serialization\MillisecondsWrapper.h"
#include "Serialization\RadianWrapper.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\SerializationFactory.h"
#include "Terra\vMap.h"
#include "Render\src\Scene.h"

#include "Serialization\Factory.h"

#include "Serialization\Dictionary.h"
#include "Serialization\Serialization.h"
#include "Serialization\StringTableImpl.h"
#include "UnicodeConverter.h"

// -------------------------------------------------------
#pragma warning(disable: 4355)

typedef Factory<WeaponPrm::WeaponClass, WeaponBase, FactoryArg2<WeaponBase, WeaponSlot*, const WeaponPrm*> > WeaponFactory;
typedef SerializationFactory<WeaponBase, FactoryArg2<WeaponBase, WeaponSlot*, const WeaponPrm *> > WeaponSerializationFactory;

template<>
struct FactorySelector<WeaponBase>
{
	typedef WeaponSerializationFactory Factory;
};

namespace weapon_helpers
{

class AimPositionScanOp
{
public:
	AimPositionScanOp(const Vect3f& begin_pos, const Vect3f& end_pos, const UnitReal* owner) : ownerUnit_(owner),
		beginPosition_(begin_pos),
		endPosition_(end_pos),
		aimPosition_(end_pos),
		aimUnit_(0)
	{
	}

	bool operator()(UnitBase* unit)
	{
		if(!unit->alive() || !unit->attr().isActing() || !unit->rigidBody())
			return true;

		Vect3f pos;
		if(unit->rigidBody()->computeBoxSectionPenetration(pos, beginPosition_, endPosition_)){
			aimUnit_ = unit;
			aimPosition_ = pos;
			return false;
		}

		return true;
	}

	bool traceGround()
	{
		Vect3f pos;
		if(!weapon_helpers::traceGround(beginPosition_, endPosition_, pos)){
			if(aimUnit_){
				if(beginPosition_.distance2(aimUnit_->position()) > beginPosition_.distance2(pos)){
					aimUnit_ = 0;
					aimPosition_ = pos;
				}
			}
			else
				aimPosition_ = pos;

			return true;
		}

		return false;
	}

	const Vect3f& aimPosition() const { return aimPosition_; }
	UnitBase* aimUnit() const { return aimUnit_; }

private:

	Vect3f beginPosition_;
	Vect3f endPosition_;

	Vect3f aimPosition_;
	UnitBase* aimUnit_;

	const UnitReal* ownerUnit_;
};

class TraceUnitScanOp
{
public:
	TraceUnitScanOp(const Vect3f& from, const Vect3f& to, const WeaponPrm* weapon = 0, const UnitReal* owner = 0, const UnitBase* target_unit = 0, int destruction_types = 0) : beginPosition_(from),
		endPosition_(to),
		weapon_(weapon),
		ownerUnit_(owner),
		targetUnit_(target_unit)
	{
		environmentDestruction_ = destruction_types;
	}

	bool operator()(UnitBase* unit)
	{
		if(!unit->alive())
			return true;

		bool need_stop = needStop(unit);
		bool need_destruct = needDestruct(unit);

		if(need_stop || need_destruct){
			Vect3f pos;
			if(unit->intersect(beginPosition_, endPosition_, pos)){
				if(need_destruct)
					unitsToDestruct_.push_back(ContactInfo(unit, pos, 0));
				if(need_stop){
					traceEnd_ = ContactInfo(unit, pos, 0);
					return false;
				}
			}
			//int bodyPart(unit->rigidBody()->computeBoxSectionPenetration(pos, beginPosition_, endPosition_));
			//if(bodyPart >= 0){
			//	if(need_destruct)
			//		unitsToDestruct_.push_back(ContactInfo(unit, pos, bodyPart));
			//	if(need_stop){
			//		traceEnd_ = ContactInfo(unit, pos, bodyPart);
			//		return false;
			//	}
			//}
		}

		return true;
	}

	struct ContactInfo
	{
		UnitBase* unit;
		Vect3f contactPosition;
		int bodyPart;
		ContactInfo(UnitBase* p = 0, Vect3f pos = Vect3f::ZERO, int bp = -1) : unit(p), contactPosition(pos), bodyPart(bp) { }
	};

	typedef std::list<ContactInfo> ContactList;
	const ContactList& unitsToDestruct() const { return unitsToDestruct_; }

	const Vect3f& endPosition() const { return traceEnd_.contactPosition; }
	UnitBase* endUnit(){ return traceEnd_.unit; }

private:

	Vect3f beginPosition_;
	Vect3f endPosition_;

	const UnitBase* targetUnit_;
	const UnitReal* ownerUnit_;

	const WeaponPrm* weapon_;

	int environmentDestruction_;

	/// список юнитов, которые должны быть уничтожены лучом
	ContactList unitsToDestruct_;

	/// юнит, в который упёрся луч
	ContactInfo traceEnd_;

	bool needDestruct(const UnitBase* unit) const
	{
		if(unit == targetUnit_ || unit == ownerUnit_)
			return false;

		return (environmentDestruction_ && (unit->unitAttackClass() & environmentDestruction_));
	}

	bool needStop(const UnitBase* unit) const
	{
		if(unit == targetUnit_)
			return true;

		if(unit == ownerUnit_)
			return false;

		if(unit->unitAttackClass() & (ENVIRONMENT_FENCE2 | ENVIRONMENT_BARN | ENVIRONMENT_BUILDING |
			ENVIRONMENT_BIG_BUILDING | ENVIRONMENT_STONE | ENVIRONMENT_INDESTRUCTIBLE))
			return true;

		if(unit->attr().isEnvironment())
			return false;

		if(!weapon_){
			if(ownerUnit_)
				return unit->attr().isBuilding() && ownerUnit_->isEnemy(unit);

			return false;
		}

		switch(weapon_->affectMode()){
		case AFFECT_OWN_UNITS:
			return ownerUnit_->player() == unit->player();
		case AFFECT_FRIENDLY_UNITS:
			return !ownerUnit_->isEnemy(unit);
		case AFFECT_ALLIED_UNITS:
			return !ownerUnit_->isEnemy(unit) && (ownerUnit_->player() != unit->player());
		case AFFECT_ENEMY_UNITS:
			return ownerUnit_->isEnemy(unit);
		case AFFECT_ALL_UNITS:
			return true;
		case AFFECT_NONE_UNITS:
			return false;
		}

		return false;
	}
};

class TraceVisibilityScanOp
{
public:
	TraceVisibilityScanOp(const UnitBase* unit, const UnitBase* target_unit) : unit_(unit), targetUnit_(target_unit)
	{
		beginPosition_ = unit->position();
		beginPosition_.z += unit->height()*.7f;

		endPosition_ = target_unit->position();
		endPosition_.z += target_unit->height()*.7f;

		endTracePosition_ = endPosition_;
	}

	bool operator()(UnitBase* unit)
	{
		if(needTrace(unit)){
			Vect3f pos;
			int bodyPart(unit->rigidBody()->computeBoxSectionPenetration(pos, beginPosition_, endPosition_));
			if(bodyPart >= 0){
				endTracePosition_ = pos;
				return false;
			}
		}

		return true;
	}

	const Vect3f& beginPosition() const { return beginPosition_; }
	const Vect3f& endPosition() const { return endPosition_; }

	const Vect3f& endTracePosition() const { return endTracePosition_; }

private:

	Vect3f beginPosition_;
	Vect3f endPosition_;

	Vect3f endTracePosition_;

	const UnitBase* targetUnit_;
	const UnitBase* unit_;

	bool needTrace(const UnitBase* unit) const
	{
		if(!unit->alive() || !unit->rigidBody())
			return false;

		if(unit == targetUnit_ || unit == unit_)
			return false;
/*
		if(unit->unitAttackClass() & (ENVIRONMENT_FENCE2 | ENVIRONMENT_BARN | ENVIRONMENT_BUILDING |
			ENVIRONMENT_BIG_BUILDING | ENVIRONMENT_STONE | ENVIRONMENT_INDESTRUCTIBLE))
			return true;
*/
		if(unit->attr().isEnvironment())
			return static_cast<const UnitEnvironment*>(unit)->isInFieldOfViewMap();

		if(unit->attr().isBuilding() && static_cast<const UnitBuilding*>(unit)->attr().fieldOfViewMapAdd)
			return true;

		return false;
	}
};

}; // namespace weapon_helpers

/// Оружие, стреляющее снарядами по навесной траектории.
class WeaponProjectile : public WeaponBase
{
public:
	WeaponProjectile(WeaponSlot* slot, const WeaponPrm* prm = 0) : WeaponBase(slot, prm)
	{
		if(weaponPrm()->missileID())
			setTurnSuggestPrm(weaponPrm()->missileID());

		projectiles_.resize(aimControllerPrm().barrelCount(), 0);
	}

	void quant()
	{
		if(owner() && !owner()->alive()){
			clearProjectiles();
		}
		else {
			if(state() == WEAPON_STATE_LOAD && weaponPrm()->needMissileVisualisation())
				createProjectile();
		}

		__super::quant();
	}

	bool fire(const WeaponTarget& target);
	void fireEnd();

	void kill();

protected:
	typedef std::vector<ProjectileBase*> Projectiles;
	/// визуализация снарядов
	Projectiles projectiles_;

	void createProjectile()
	{
		int idx = currentBarrel();
		int barrelCount = projectiles_.size();
		for(int i = 0; i < barrelCount; i++){
			if(!projectiles_[idx]){
				xassert(owner() && weaponPrm()->missileID());
				ProjectileBase* p = safe_cast<ProjectileBase*>(owner()->player()->buildUnit(weaponPrm()->missileID()));

				if(p)
					p->attachToDock(owner(), aimControllerPrm().nodeLogic(idx), true);

				projectiles_[idx] = p;
				return;
			}

			if(++idx >= barrelCount)
				idx = 0;
		}
	}

	void clearProjectiles()
	{
		Projectiles::iterator i;
		FOR_EACH(projectiles_, i){
			if(ProjectileBase* projectile = *i){
				projectile->detachFromDock();
				projectile->Kill();
				*i = 0;
			}
		}
	}

	void shootProjectile();

	const WeaponProjectilePrm* weaponPrm() const { return static_cast<const WeaponProjectilePrm*>(__super::weaponPrm()); }
};

struct ChainLightningWeaponFunctor : public ChainLightningOwnerInterface
{
	ChainLightningWeaponFunctor(const WeaponBase* weapon) : weapon_(weapon) {}
	Vect3f	position()						const	{ return weapon_->firePosition(); }
	bool	canApply(UnitActing* unit)		const	{ return weapon_->canAttack(WeaponTarget(unit, weapon_->weaponPrm()->ID()), false); }
	ParameterSet damage()	const
		{	float dt = weapon_->fireTime() > FLT_EPS ? (logicPeriodSeconds / (float(weapon_->fireTime()) / 1000.f)) : 1.f;
			ParameterSet dmg = weapon_->damage();
			dmg *= dt;
			return dmg;
		}
	UnitBase* owner()		const	{ return weapon_->owner(); }

private:
	const WeaponBase* weapon_;
};

class WeaponBeamTrace
{
public:
	WeaponBeamTrace(){ isEmpty_ = true; }

	enum Mode
	{
		TRACE_GROUND = 0,
		TRACE_SHIELD,
		TRACE_UNITS
	};

	bool isEmpty() const { return isEmpty_; }

	void set(Mode mode, const Vect3f& pos, float dist2)
	{
		traceInfo_[mode].enabled_ = true;
		traceInfo_[mode].position_ = pos;
		traceInfo_[mode].dist2_ = dist2;

		isEmpty_ = false;
	}

	bool isEnabled(Mode mode) const { return traceInfo_[mode].enabled_; }
	const Vect3f& position(Mode mode) const { return traceInfo_[mode].position_; }
	float dist2(Mode& mode) const { return traceInfo_[mode].dist2_; }

private:

	struct TraceInfo
	{
		bool enabled_;
		Vect3f position_;
		float dist2_;

		TraceInfo(){
			enabled_ = false;
			position_ = Vect3f::ZERO;
		}
	};

	TraceInfo traceInfo_[3];
	bool isEmpty_;
};

/// Лучевое оружие.
class WeaponBeam : public WeaponBase
{
public:
	WeaponBeam(WeaponSlot* slot, const WeaponPrm* prm = 0);
	~WeaponBeam();

	void quant();
	bool fire(const WeaponTarget& target);
	void kill();

	void showDebugInfo();

	void serialize(Archive& ar);

protected:

	const WeaponBeamPrm* weaponPrm() const { return static_cast<const WeaponBeamPrm*>(__super::weaponPrm()); }
	void fireEnd();

private:

	/// визуализация луча
	cEffect* effect_;

	Vect3f beamPrevPos_[2];

	Vect3f firePositionInitial_;
	/// ошибка прицеливания
	Vect3f fireDispersionInitial_;
	/// цель
	UnitLink<UnitInterface> fireTargetInitial_;

	/// цепной эффект
	ChainLightningWeaponFunctor chainLightningFunctor_;
	ChainLightningController chainLightningController_;

	bool createEffect();
	bool updateEffect();

	bool createTargetEffects();

	void chainUpdate();

	bool releaseEffect()
	{
		if(effect_){
			if(weaponPrm()->effectStopImmediately())
				streamLogicPostCommand.set(fCommandRelease, effect_);
			else {
				streamLogicCommand.set(fCommandSetCycle, effect_) << false;
				streamLogicCommand.set(fCommandSetAutoDeleteAfterLife, effect_) << true;
			}
			effect_ = 0;
			return true;
		}
		return false;
	}

	bool createSources();
	void updateTarget();

	bool groundHit() const;
};

/// захват
class WeaponGrip : public WeaponBase
{
public:
	WeaponGrip(WeaponSlot* slot, const WeaponPrm* prm = 0);

	void quant();

	bool fire(const WeaponTarget& target);

protected:

	const WeaponGripPrm* weaponPrm() const { return static_cast<const WeaponGripPrm*>(__super::weaponPrm()); }

	void fireEnd();

	void finishGrip(UnitActing* target);

	LogicTimer gripTimer;
};

/// управление источником из оружия
class WeaponSourceController
{
public:
	WeaponSourceController(const SourceWeaponAttribute* attribute = 0);

	bool create();
	void start();
	bool release();

	bool quant();

	void setOwner(UnitBase* p);
	void setPose(const Se3f& pose);
	void setLifeTime(int time){ if(source_) source_->setLifeTime(time); }

	int ID() const { return ID_; }
	void setID(int id){ ID_ = id; }

	void setAffectMode(AffectMode mode)
	{
		if(source_) source_->setAffectMode(mode);
	}

	void setParameters(const WeaponSourcePrm& parameters, const WeaponTarget* target = 0)
	{
		if(source_)
			source_->setParameters(parameters, target);
	}
	const SourceWeaponAttribute* attr() const { return attribute_; }

	void serialize(Archive& ar)
	{
		ar.serialize(source_, "source", 0);
		ar.serialize(pose_, "pose", 0);
	}

private:

	int ID_;

	UnitLink<SourceBase> source_;
	const SourceWeaponAttribute* attribute_;

	Se3f pose_;
};

/// оружие, действующее на область
class WeaponPad : public WeaponBase
{
public:
	WeaponPad(WeaponSlot* slot, const WeaponPrm* prm = 0);
	~WeaponPad();

	bool fire(const WeaponTarget& target);
	void quant();
	void fireEnd();

	void serialize(Archive& ar);

	UI_MarkObjectModeID analyseMarkObject(UnitInterface* unit) const;
	bool canFireMinTime(RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const; 

protected:
	const WeaponPadPrm* weaponPrm() const { return static_cast<const WeaponPadPrm*>(__super::weaponPrm()); }

private:
	float changeCapacity(float value);

	UnitPad* getPad(UnitPlayer* &up) const;
	UnitPad* updatePad(UnitPlayer* &up);

	void deselect();

	FOW_HANDLE fowHandle_;

	UnitLink<UnitSquad> squadPicked_;

	Toolser toolser_;
	bool started_;
	Vect3f prevPos_;

	float digLenght_;
};

/// оружие, действующее на область
class WeaponAreaEffect : public WeaponBase
{
public:
	WeaponAreaEffect(WeaponSlot* slot, const WeaponPrm* prm = 0);
	~WeaponAreaEffect();

	void quant();

	void showInfo(const Vect2f& pos) const;
	bool checkFinalCost() const;
	bool checkTerrain() const;

	bool fire(const WeaponTarget& target);
	void fireEnd();

	bool isOffensive() const { return __super::isOffensive() && weaponPrm()->zonePositionMode() == WeaponAreaEffectPrm::ZONE_POS_TARGET; }

	void kill();

	QuatF sourcesOrientation() const;

	void serialize(Archive& ar);

protected:

	const WeaponAreaEffectPrm* weaponPrm() const { return static_cast<const WeaponAreaEffectPrm*>(__super::weaponPrm()); }

private:

	typedef std::vector<WeaponSourceController> SourceControllers;
	SourceControllers sourceControllers_;

	FOW_HANDLE fowHandle_;

	void updatePosition(bool init = false);

	void releaseSources();
};

//-------------------------------------------------------
//class WeaponTeleportPrm : public WeaponPrm
//{
//public:
//	WeaponTeleportPrm() {
//		weaponClass_ = WeaponPrm::WEAPON_TELEPORT;
//	}
//};
//
//class WeaponTeleport : public WeaponBase
//{
//public:
//	WeaponTeleport(UnitReal* owner, const WeaponSlotAttribute* attribute, const WeaponPrm* prm = 0) :
//	  WeaponBase(owner, attribute, prm)
//	  {
//	  }
//
//	bool fire(const WeaponTarget& target);
//
//protected:
//
//	const WeaponTeleportPrm* weaponPrm() const { return static_cast<const WeaponTeleportPrm*>(__super::weaponPrm()); }
//
//};
//
//bool WeaponTeleport::fire(const WeaponTarget& target)
//{
//	const UnitInterfaceList& selection;
//	
//	PositionGeneratorCircle<const UnitInterfaceList> positionGenerator;
//	positionGenerator.init(0, Vect2f(firePosition() + fireDispersion()), &selection);
//
//	UnitInterfaceList::const_iterator ui = selection.begin();
//
//	while(ui != selection.end()){
//		Vect2f point = positionGenerator.get(*ui);
//		(*ui)->setPosition(To3D(point));
//		++ui;
//	}
//
//	return true;
//}
//
//-------------------------------------------------------


WeaponWaitingSourcePrm::WeaponWaitingSourcePrm()
{
	weaponClass_ = WeaponPrm::WEAPON_WAITING_SOURCE;
	type_ = MINING;
	delayTime_ = 0.2f;
	alwaysPutInQueue_ = true;
	miningLimit_ = -1;
}

void WeaponWaitingSourcePrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);
	ar.serialize(type_, "type", "фаза процесса");
	if(type_ == MINING){
		ar.serialize(source_, "sources", "источник"); // с активацией детонатором
		ar.serialize(delayTime_, "delayTime", "задержка между взрывами");
		ar.serialize(miningLimit_, "miningLimit", "Максимум мин");
	}
}

//-------------------------------------------------------

/// оружие расставляющее источники с запоминанием места и отложенной телопортацией детонаторов в эти места
class WeaponWaitingSource : public WeaponBase
{
public:
	WeaponWaitingSource(WeaponSlot* slot, const WeaponPrm* prm = 0);
	~WeaponWaitingSource();

	void serialize(Archive& ar);

	bool init(const WeaponBase* old_weapon = 0);

	bool checkFinalCost() const;
	bool fire(const WeaponTarget& target);
	void detonate();

	bool canDetonate() const;
	int mineCount() const { return mineCount_; }

	const WeaponWaitingSourcePrm* weaponPrm() const { return static_cast<const WeaponWaitingSourcePrm*>(__super::weaponPrm()); }

private:
	void updateCount(){
		mineCount_ = sourceCoordinats_.size();
	}

	typedef std::vector<Vect3f> SourceCoordinats;
	SourceCoordinats sourceCoordinats_;
	int mineCount_;
};


//-------------------------------------------------------

bool WeaponAimAngleController::init(UnitActing* owner, const WeaponAimAnglePrm& prm)
{
	speed_ = G2R(prm.turnSpeed());
	speed2_ = G2R(prm.turnSpeedDirectControl());

	valueMin_ = G2R(prm.valueMin());
	valueMax_ = G2R(prm.valueMax());

	value_ = G2R(prm.valueDefault());
	valueDefault_ = value_;

	parentValue_ = 0.f;

	valuePrecision_ = G2R(prm.precision());

	offset_ = Se3f::ID;
	offsetLogic_ = Se3f::ID;

	if(prm.hasNodeGraphics()){
		nodeIndex_ = prm.nodeGraphics();
		if(nodeIndex_ != -1)
			offset_ = prm.nodeGraphicsOffset();
	}
	else
		nodeIndex_ = -1;

	if(prm.hasNodeLogic()){
		nodeLogicIndex_ = prm.nodeLogic();
		if(nodeLogicIndex_ != -1){
			offsetLogic_ = prm.nodeLogicOffset();
			owner->weaponRotation().addNode(WeaponRotationNode(nodeLogicIndex_, rotationAxis_, valueDefault_));
		}
	}
	else
		nodeLogicIndex_ = -1;

/*
	offset_ = Se3f::ID;
	offsetLogic_ = Se3f::ID;

	if(prm.hasNodeGraphics()){
		nodeIndex_ = prm.nodeGraphics();
		if(nodeIndex_ != -1){
			owner->model()->RestoreUserTransform(nodeIndex_);
			owner->model()->Update();
			offset_ = owner->model()->GetNodePosition(nodeIndex_);
			offset_.trans() = Vect3f::ZERO;
		}
		else
			kdWarning("&stlr", (string(TRANSLATE("Не найдена графическая нода")) + owner->attr().modelName + " / " + prm.nodeGraphics().c_str()).c_str());
	}
	else
		nodeIndex_ = -1;

	if(prm.hasNodeLogic()){
		nodeLogicIndex_ = prm.nodeLogic();

		if(nodeLogicIndex_ != -1){
			owner->modelLogic()->RestoreUserTransform(nodeLogicIndex_);
			owner->modelLogic()->Update();
			offsetLogic_ = owner->modelLogicNodePosition(nodeLogicIndex_);
			offsetLogic_.trans() = Vect3f::ZERO;

			owner->weaponRotation().addNode(WeaponRotationNode(nodeLogicIndex_, rotationAxis_, valueDefault_));

			if(prm.rotateByLogic()){
				string parent_node_name = prm.nodeLogic().c_str();
				parent_node_name += "_logic";
				int parent_node = owner->modelLogicNodeIndex(parent_node_name.c_str());
				if(parent_node != -1){
					Mats mat = Mats::ID;
					int index = -1;
					if(owner->modelLogicNodeOffset(parent_node, mat, index)){
						if(index == nodeLogicIndex_){
							offsetLogic_ = Se3f(mat.rot(), Vect3f::ZERO);
							rotateByLogic_ = true;
						}
						else
							kdWarning("&stlr", (string(TRANSLATE("Неправильно прилинкована логическая нода ")) + owner->attr().modelName + " / " + parent_node_name).c_str());
					}
				}
				else
					kdWarning("&stlr", (string(TRANSLATE("Не найдена логическая нода")) + owner->attr().modelName + " / " + parent_node_name).c_str());
			}
		}
		else
			kdWarning("&stlr", (string(TRANSLATE("Не найдена логическая нода")) + owner->attr().modelName + " / " + prm.nodeLogic().c_str()).c_str());
	}
	else
		nodeLogicIndex_ = -1;
*/
	return true;
}

void WeaponAimAngleController::reset(UnitActing* owner, WeaponRotationController& controller)
{
	value_ = valueDefault_;
	valuePrev_ = value_ + parentValue_;

	if(nodeIndex_ != -1)
		streamLogicPostCommand.set(fNodeTransformClear, owner->model()) << nodeIndex_;

	if(nodeLogicIndex_ != -1)
		owner->modelLogic()->RestoreUserTransform(nodeLogicIndex_);
}

void WeaponAimAngleController::serialize(Archive& ar)
{
	ar.serialize(value_, "value", 0);
	ar.serialize(valuePrev_, "valuePrev", 0);
}

bool WeaponAimAngleController::change(float target_angle, bool use_normal_speed, WeaponRotationController& controller, int change_priority)
{
	valuePrev_ = value_ + parentValue_;

	float delta = getDelta(target_angle, use_normal_speed);
	if(nodeLogicIndex_ != -1){
		if(controller.changeAngle(nodeLogicIndex_, rotationAxis_, delta, change_priority))
			value_ += delta;
	}
	else
		value_ += delta;

	return true;
}

bool WeaponAimAngleController::changeToDefault(bool use_normal_speed, WeaponRotationController& controller)
{
	change(valueDefault_, use_normal_speed, controller, ROTATION_DEFAULT);
	return (fabs(getDeltaAngle(value_ + parentValue_, valueDefault_)) <= FLT_COMPARE_TOLERANCE);
}

void WeaponAimAngleController::randomizeDefaultValue()
{
	float delta = valueMax_ - valueMin_;
	delta *= logicRNDfrand();
	valueDefault_ = valueMin_ + delta;
}

bool WeaponAimAngleController::updateValue(const WeaponRotationController& controller)
{
	if(nodeLogicIndex_ != -1)
		value_ = controller.angle(nodeLogicIndex_, rotationAxis_);

	return true;
}

void WeaponAimAngleController::updateLogic(UnitActing* owner) const
{
	if(nodeLogicIndex_ != -1)
		owner->setModelLogicNodeTransform(nodeLogicIndex_, nodeLogicPose());
}

void WeaponAimAngleController::interpolationQuant(UnitActing* owner)
{
	if(nodeIndex_ != -1){
		nodeInterpolator_ = nodePose(owner);
		nodeInterpolator_(owner->model(), nodeIndex_);
	}
}

void WeaponAimAngleController::showDebugInfo(const UnitActing* owner) const
{
	if(nodeLogicIndex_ != -1){
		MatXf X(owner->modelLogicNodePosition(nodeLogicIndex_));

		Vect3f delta = X.rot().xcol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::RED);

		delta = X.rot().ycol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::BLUE);

		delta = X.rot().zcol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::GREEN);
	}
}

Se3f WeaponAimAngleController::nodeLogicPose(bool with_offset) const 
{
	Se3f R(QuatF(value_, rotationAxis_ == X_AXIS ? Vect3f::I : (rotationAxis_ == Y_AXIS ? Vect3f::J : Vect3f::K)), Vect3f::ZERO);

	if(with_offset){
		Se3f inv;
		inv.invert(offsetLogic_);

		if(rotateByLogic_){
			R.premult(offsetLogic_);
			R.postmult(inv);
		}
		else {
			R.premult(inv);
			R.postmult(offsetLogic_);
		}
	}

	return R;
}

Se3f WeaponAimAngleController::nodePose(UnitActing* owner) const 
{
	if(!rotateByLogic_){
		Se3f R(QuatF(value_, rotationAxis_ == X_AXIS ? Vect3f::I : (rotationAxis_ == Y_AXIS ? Vect3f::J : Vect3f::K)), Vect3f::ZERO);

		Se3f inv;
		inv.invert(offset_);
		R.premult(inv);
		R.postmult(offset_);
		return R;
	}
	else
		return nodeLogicPose();
}

WeaponSlot::WeaponSlot() : owner_(0),
	weapon_(0),
	psi_(Z_AXIS),
	theta_(X_AXIS),
	parentPsi_(0),
	parentTheta_(0)
{
	ID_ = -1;
	isAimEnabled_ = false;
	isAimCorrectionEnabled_ = false;
}

WeaponSlot::~WeaponSlot()
{
	if(weapon_){
		WeaponBase::release(weapon_);
		weapon_ = 0;
	}
}

void WeaponSlot::updateAngles()
{
	if(isAimEnabled_){
		psi_.updateValue(owner()->weaponRotation());
		theta_.updateValue(owner()->weaponRotation());
	}
}

bool WeaponSlot::aim(float psi, float theta, bool aim_only)
{
	if(isAimEnabled_){
		WeaponAimAngleController::RotationPriority pr = (aim_only) ? WeaponAimAngleController::ROTATION_AIM : WeaponAimAngleController::ROTATION_FIRE;
		psi_.change(psi, !owner()->isDirectControl() && !owner()->isSyndicateControl(), owner()->weaponRotation(), pr + attribute()->priority());
		theta_.change(theta, !owner()->isDirectControl() && !owner()->isSyndicateControl(), owner()->weaponRotation(), pr + attribute()->priority());
		return isAimed(psi, theta);
	}

	return true;
}

bool WeaponSlot::aimUpdate()
{
	if(parentPsi_)
		psi_.setParentValue(parentPsi_->psi_());
	if(parentTheta_)
		theta_.setParentValue(parentTheta_->theta_());

	return true;
}

bool WeaponSlot::aimDefault()
{
	if(isAimEnabled_){
		bool psi_default = psi_.changeToDefault(!owner()->isDirectControl(), owner()->weaponRotation());
		bool theta_default = theta_.changeToDefault(!owner()->isDirectControl(), owner()->weaponRotation());

		return psi_default && theta_default;
	}
	else
		return false;
}

void WeaponSlot::aimReset()
{
	if(isAimEnabled_ && owner()){
		psi_.reset(owner(), owner()->weaponRotation());
		theta_.reset(owner(), owner()->weaponRotation());
//		isAimEnabled_ = false;
	}
}

Se3f WeaponSlot::muzzlePosition(int barrel_index) const 
{ 
	if(owner()){
		if(nodeEffectIndex(barrel_index) != -1){
			updateLogic();
			return owner()->modelLogicNodePosition(nodeEffectIndex(barrel_index));
		}

		Se3f pose = owner()->pose();
		pose.trans().z += owner()->height()*0.7f;
		return pose;
	}

	return Se3f::ID;
}

bool WeaponSlot::setParentSlot(const WeaponSlot* parent)
{
	bool ret = false;
	if(parent != this && isAimEnabled() && parent->isAimEnabled()){
		if(const cObject3dx* model = owner_->model()){
			if(psi_.nodeIndex() && parent->psi_.nodeIndex() && psi_.nodeIndex() != -1 && parent->psi_.nodeIndex() != -1 && model->CheckDependenceOfNodes(psi_.nodeIndex(), parent->psi_.nodeIndex()))
				parentPsi_ = parent;
			if(theta_.nodeIndex() && parent->theta_.nodeIndex() && theta_.nodeIndex() != -1 && parent->theta_.nodeIndex() != -1 && model->CheckDependenceOfNodes(theta_.nodeIndex(), parent->theta_.nodeIndex()))
				parentTheta_ = parent;
		}
	}

	return ret;
}

const Vect3f& WeaponSlot::rotationCenterPsi() const 
{
	if(owner()){
		updateLogic();
		if(psi_.nodeLogicIndex() != -1)
			return owner()->modelLogicNodePosition(psi_.nodeLogicIndex()).trans();
		else
			return owner()->modelLogicNodePosition(0).trans();
	}

	return Vect3f::ID;
}

const Vect3f& WeaponSlot::rotationCenterTheta() const 
{
	if(owner()){
		updateLogic();
		if(theta_.nodeLogicIndex() != -1)
			return owner()->modelLogicNodePosition(theta_.nodeLogicIndex()).trans();
		else
			return owner()->modelLogicNodePosition(0).trans();
	}

	return Vect3f::ID;
}

void WeaponSlot::showDebugInfo() const 
{
	Se3f muzzle_pos = muzzlePosition();

	if(showDebugWeapon.direction){
		Vect3f v(0, 5, 0);
		muzzle_pos.xformVect(v);
		show_vector(muzzle_pos.trans(), v, Color4c::GREEN);
	}

	if(showDebugWeapon.horizontalAngle){
		int node = max(0, psi_.nodeLogicIndex());
		MatXf X(owner()->modelLogicNodePosition(node));

		Vect3f delta = X.rot().xcol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::RED);

		delta = X.rot().ycol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::BLUE);

		delta = X.rot().zcol();
		delta.normalize(5);
		show_vector(X.trans(), delta, Color4c::GREEN);
	}

	if(showDebugWeapon.verticalAngle){
		if(theta_.nodeLogicIndex() > 0){
			MatXf X(owner()->modelLogicNodePosition(theta_.nodeLogicIndex()));

			Vect3f delta = X.rot().xcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::RED);

			delta = X.rot().ycol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::BLUE);

			delta = X.rot().zcol();
			delta.normalize(5);
			show_vector(X.trans(), delta, Color4c::GREEN);
		}
	}

	if(showDebugWeapon.angleValues){
		XBuffer buf;
		buf <= (int)R2G(psi_()) < " " <= (int)R2G(theta_());
		show_text(muzzle_pos.trans(), buf, Color4c::GREEN);
	}

	if(showDebugWeapon.angleLimits){
		Vect3f dir(5.f * sinf(-psi_.valueRange().minimum()), 5.f * cosf(-psi_.valueRange().minimum()), 0);
		dir = owner()->pose().xformVect(dir);
		show_vector(muzzle_pos.trans(), dir, Color4c::RED);

		dir = Vect3f(5.f * sinf(-psi_.valueRange().maximum()), 5.f * cosf(-psi_.valueRange().maximum()), 0);
		dir = owner()->pose().xformVect(dir);
		show_vector(muzzle_pos.trans(), dir, Color4c::RED);
	}
}

void WeaponSlot::kill()
{
	if(weapon_)
		weapon_->kill();
}

int WeaponSlot::nodeEffectIndex(int barrel_index) const
{
	if(weapon_)
		return weapon_->aimControllerPrm().nodeLogic(barrel_index);
	return -1;
}

int WeaponSlot::nodeEffectIndexGraphic(int barrel_index) const
{
	if(weapon_)
		return weapon_->aimControllerPrm().nodeGraphics(barrel_index);
	return -1;
}

//-------------------------------------------------------

void fCommandReleaseWeapon(XBuffer& stream)
{
    WeaponBase* weapon;
	stream.read(weapon);
	WeaponBase::release(weapon);
}

bool WeaponSlot::isEmpty() const
{ 
	return weapon()->weaponPrm()->ID() == -1; 
}

bool WeaponSlot::createWeapon(const WeaponPrm* prm)
{
	if(weapon_)
		weapon_->aimReset();

	WeaponBase* p = WeaponBase::create(this, prm);
	p->init(weapon_);

	if(weapon_){
		weapon_->kill();
		streamLogicCommand.set(fCommandReleaseWeapon) << weapon_;
		owner_->removeWeapon(weapon_);
	}

	p->enable(owner_->isConstructed());

	weapon_ = p;
	owner_->addWeapon(weapon_);

	const WeaponAimControllerPrm& aimControllerPrm = weapon_->aimControllerPrm();

	isAimEnabled_ = aimControllerPrm.isEnabled();
	isAimCorrectionEnabled_ = aimControllerPrm.isCorrectionEnabled();

	if(isAimEnabled_){
		psi_.init(owner_, aimControllerPrm.anglePsiPrm());
		theta_.init(owner_, aimControllerPrm.angleThetaPrm());
	}

	return true;
}

bool WeaponSlot::upgradeWeapon()
{
	start_timer_auto();

	if(weapon_){
		if(const WeaponPrm* upgrade = weapon_->accessibleUpgrade()){
			createWeapon(upgrade);

			owner_->startEffect(&owner_->player()->race()->weaponUpgradeEffect(), true, -1, 3000);

			return true;
		}
	}

	return false;
}

bool WeaponSlot::init(UnitActing* owner, const WeaponSlotAttribute* attribute)
{
	owner_ = owner;
	attribute_ = attribute;

	const WeaponPrm* weapon_prm = 0;
	if(!attribute_->isEmpty())
		weapon_prm = attribute_->weaponPrm()->accessibleUpgrade(owner_);

	createWeapon(weapon_prm);

	return true;
}

void WeaponSlot::serialize(Archive& ar)
{
	if(isAimEnabled_){
		ar.serialize(psi_, "psi", 0);
		ar.serialize(theta_, "theta", 0);
	}
	WeaponPrmReference weaponPrm;
	if(ar.isOutput() && weapon_)
		weaponPrm = weapon_->weaponPrm();
	ar.serialize(weaponPrm, "weaponPrm", 0);
	if(ar.isInput() && weaponPrm && (!weapon_ || weapon_->weaponPrm() != weaponPrm))
		createWeapon(weaponPrm);
	if(weapon_)
		ar.serialize(*weapon_, "weapon", 0);
}

//-------------------------------------------------------

WeaponBase::WeaponBase(WeaponSlot* slot, const WeaponPrm* prm) : owner_(slot->owner()),
	attribute_(slot->attribute()),
	weaponSlot_(slot),
	weaponPrm_(prm),
	fireTarget_(0),
	turnSuggestPrm_(0)
{
	xassert(owner_ && attribute_);

	currentBarrel_ = 0;

	if(!weaponPrm_) weaponPrm_ = attribute_->weaponPrm();

	aimControllerPrmIndex_ = attribute_->findAimControllerPrm(weaponPrm_->animationType());

	isEnabled_ = false;
	isAccessible_ = true;

	reloaded_ = true;
	reloadFromInventory_ = false;

	autoFire_ = false;
	autoFireOnce_ = false;

	hasFireTarget_ = false;
	fireTargetSet_ = false;
	fireStarted_ = false;
	fireRequested_ = false;

	firePosition_ = Vect3f::ZERO;
	fireDispersion_ = Vect3f::ZERO;

	fireTime_ = 0;
	reloadTime_ = 0;
	reloadTimeInventory_ = 0;
	fireRadius_ = 100.0f;
	fireRadiusMin_ = 0.f;
	fireDisp_ = 0;

	aimResetDisabled_ = false;

	autoTarget_ = 0;
	autoTargetAttacked_ = false;

	if(weaponPrm()->ID() != -1)
		prmCache_ = *owner_->player()->weaponPrmCache(weaponPrm());

	updateParameters();

	if(weaponPrm()->ammoType())
		setParameter(weaponPrm()->parameters().findByType(ParameterType::AMMO, 0.f), ParameterType::AMMO);

	setParameter(weaponPrm()->parameters().findByType(ParameterType::WEAPON_DURABILITY, 0.f), ParameterType::WEAPON_DURABILITY);

	reloadStart(0);
}

WeaponBase::~WeaponBase()
{
}

bool WeaponBase::isShortRange() const
{
	return weaponPrm()->isShortRange();
}

bool WeaponBase::isLongRange() const
{
	return weaponPrm()->isLongRange();
}

bool WeaponBase::isOffensive() const
{
	return canAutoFire();
}

bool WeaponBase::canAutoFire() const
{
	return weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_DEFAULT;
}

void WeaponBase::toggleAutoFire(bool shoot_once)
{
	if(!autoFire_){
		bool move_cancel = !checkMovement();

		if(!move_cancel){
			autoFire_ = true;
			autoFireOnce_ = shoot_once;
		}
		else {
			switchOff();
			fireTargetSet_ = false;
		}
	}
	else {
		autoFire_ = false;
		fireTargetSet_ = false;
		switchOff();
	}
}

AffectMode WeaponBase::affectMode() const 
{
	return weaponPrm()->affectMode(); 
}

WeaponAnimationMode WeaponBase::animationMode() const
{
	if(aimControllerPrm().hasAnimation()){
		if(isFiring())
			return WEAPON_ANIMATION_FIRE;
		
		if(isLoading())
			return WEAPON_ANIMATION_RELOAD;
		
		if(isTargeting())
			return WEAPON_ANIMATION_AIM;
	}

	return WEAPON_ANIMATION_NONE;
}

bool WeaponBase::canAttack(const WeaponTarget& target, bool quick_check) const
{
	start_timer_auto();

	if(!hasAmmo())
		return false;

	if(!quick_check && !checkFinalCost())
		return false;

	if(weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_ALWAYS){
		if(target.weaponID() != weaponPrm()->ID())
			return false;
	}

	if(weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_SQUAD_LEADER){
		if(target.weaponID() != weaponPrm()->ID() || !owner()->getSquadPoint() || owner() != owner()->getSquadPoint()->getUnitReal())
			return false;
	}

	if(weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_INTERFACE){
		if(target.weaponID() != weaponPrm()->ID())
			return false;
	}

	if(target.weaponID() && target.weaponID() != weaponPrm()->ID())
		return false;

	if(UnitInterface* unit = target.unit()){
		if(unit == owner_)
			return false;

		switch(affectMode()){
		case AFFECT_OWN_UNITS:
			if(owner()->player() != unit->player())
				return false;
			break;
		case AFFECT_FRIENDLY_UNITS:
			if(owner()->isEnemy(unit))
				return false;
			break;
		case AFFECT_ALLIED_UNITS:
			if((owner()->player() == unit->player()) || owner()->isEnemy(unit))
				return false;
			break;
		case AFFECT_ENEMY_UNITS:
			if(!owner()->isEnemy(unit))
				return false;
			break;
		}

		if(!checkAttackClass(unit->unitAttackClass()))
			return false;

		if(!quick_check && !checkDamage(unit))
			return false;

		return true;
	}

	if(!target.valid())
		return false;

	if(weaponPrm()->attackClass() & ATTACK_CLASS_GROUND)
		return checkAttackClass(target.terrainAttackClass());

	return false;
}

WeaponBase* WeaponBase::create(WeaponSlot* slot, const WeaponPrm* prm)
{
	if(slot->attribute()->isEmpty() && !prm)
		prm = WeaponPrm::defaultPrm();

	WeaponFactory::instance().setArguments(slot, prm);
	return WeaponFactory::instance().create(prm ? prm->weaponClass() : slot->attribute()->weaponPrm()->weaponClass());
}

bool WeaponBase::init(const WeaponBase* old_weapon)
{
	xassert(owner_);
	return true;
}
/*
void WeaponBase::reloadStart()
{ 
	state_ = WEAPON_STATE_LOAD;

	if(!reloadTime_)
		reloaded_ = true;

    fireDispersion_ = Vect3f::ZERO;

	stateTimer_.start(reloadTime_);
}
*/
void WeaponBase::reloadStart(int time, bool from_inventory)
{ 
	if(time == -1)
		time = reloadTime_;

	reloadTime_ = time;
	reloadFromInventory_ = from_inventory;

	if(weaponPrm()->mainSquadUnitReload())
		time = 0;

	state_ = WEAPON_STATE_LOAD;
	reloaded_ = false;

	if(!time)
		reloaded_ = true;

	fireDispersion_ = Vect3f::ZERO;

	stateTimer_.start(time);
}

bool WeaponBase::isLoaded() const
{
	return (state_ == WEAPON_STATE_LOAD && reloaded_ && hasAmmo());
}

bool WeaponBase::hasAmmo() const
{
	return !weaponPrm()->ammoType() || parameters().findByType(ParameterType::AMMO, 0.f) > 0.5f;
}

void WeaponBase::switchOff()
{
	if(!isEnabled()) return;

	autoFire_ = false;

	if(isFiring()){
		reloadStart();
	}
}

void WeaponBase::serialize(Archive& ar) 
{
	ar.serialize(fireTarget_, "fireTarget", 0);
	ar.serialize(firePosition_, "firePosition", 0);
	ar.serialize(reloaded_, "reloaded", 0);
	ar.serialize(state_, "state", 0);
	ar.serialize(stateTimer_, "stateTimer", 0);
	ar.serialize(prmCache_, "prmCache", 0);
	if(universe()->userSave()){
		ar.serialize(fireDispersion_, "fireDispersion", 0);
		ar.serialize(hasFireTarget_, "hasFireTarget", 0);
		ar.serialize(queueDelayTimer_, "queueDelayTimer", 0);
		ar.serialize(currentBarrel_, "currentBarrel", 0);
		ar.serialize(fireTargetSet_, "fireTargetSet", 0);
		ar.serialize(fireStarted_, "fireStarted", 0);
		ar.serialize(fireRequested_, "fireRequested", 0);
		ar.serialize(autoFire_, "autoFire", 0);
		ar.serialize(autoFireOnce_, "autoFireOnce", 0);
		ar.serialize(aimResetTimer_, "aimResetTimer", 0);
		ar.serialize(isEnabled_, "isEnabled", 0);
		ar.serialize(isAccessible_, "isAccessible", 0);
		ar.serialize(autoTarget_, "autoTarget", 0);
		ar.serialize(autoTargetAttacked_, "autoTargetAttacked", 0);
		ar.serialize(reloadFromInventory_, "reloadFromInventory", 0);
	}
}

void WeaponBase::applyParameterArithmetics(const ArithmeticsData& arithmetics)
{
	if(arithmetics.checkWeapon(weaponPrm())){
		prmCache_.applyArithmetics(arithmetics);
		updateParameters();
	}
}

const WeaponPrm* WeaponBase::accessibleUpgrade() const
{
	start_timer_auto();

	return weaponPrm()->accessibleUpgrade(owner());
}

void WeaponBase::preQuant()
{
	if(isEnabled_){
		switch(state_){
		case WEAPON_STATE_FIRE:
			aim(firePosition_ + fireDispersion_);
			break;
		}
	}
}

void WeaponBase::quant()
{
	start_timer_auto();

	bool resource_request = true;

	if(isEnabled_){
		if(weaponPrm()->enableAutoScan() && weaponSlot()->isAimEnabled() && !aimResetTimer_.finished() && !autoScanTimer_.busy()){
			autoScanTimer_.start(round(weaponPrm()->autoScanPeriod() * 1000.f));
			weaponSlot()->randomizeDefaultValue();
			aimResetTimer_.start(weaponPrm()->aimLockTime());
		}

		switch(state_){
		case WEAPON_STATE_LOAD: {
			if(autoFire_ || needAutoFire()){
				if(!checkMovement()){
					if(aimResetTimer_.finished() && !weaponPrm()->disableAimReturn() && !owner()->isDirectControl()){
						if(aimDefault())
							aimResetTimer_.stop();
					}
					break;
				}

				if(isLoaded() && owner()->alive()){
					if(!fireTargetSet_){
						Vect2f pos = owner()->position2D();

						float angle = logicRNDfrand()*M_PI*2;
						float dist = owner()->radius() + fireRadiusMin_ + logicRNDfrand()*(fireRadius_ - fireRadiusMin_);

						pos.x += dist * cosf(angle);
						pos.y += dist * sinf(angle);
						
						setGroundTarget(clampWorldPosition(To3D(pos), 20.f));
					}

					if(fireTargetSet_ && fireRequest(WeaponTarget(fireTarget_, firePosition_))){
						if(!owner()->fireRequestTimer()->busy() || owner()->selectedWeaponID() != weaponPrm()->ID()){
							owner()->fireRequestTimer()->start(logicRND(5000));
							owner()->player()->checkEvent(EventUnitPlayerInt(Event::UNIT_ATTACKING, owner(), owner()->player(), weaponPrm()->ID()));;
						}
						if(autoFire_ && (autoFireOnce_ || weaponPrm()->clearTargets()))
							autoFire_ = false;

						fireEvent();
					}
				}
			}
			else {
				start_timer(1);

				if(aimResetTimer_.finished() && !weaponPrm()->disableAimReturn() && !owner()->isDirectControl() && !aimResetDisabled_){
					if(aimDefault())
						aimResetTimer_.stop();
				}

				stop_timer(1);
			}

			if(!stateTimer_.busy()){
				const ParameterSet& prm = prmCache_.parameters();
				reloadTime_ = round(prm.findByType(ParameterType::RELOAD_TIME, 0.1f) * 1000.0f);
				reloaded_ = true;
			}

			if(!weaponPrm()->fireCostAtOnce()){
				ParameterSet prm = fireCost();
				resource_request = owner()->requestResource(prm, NEED_RESOURCE_SILENT_CHECK);
			}

			aimResetDisabled_ = false;
			}
			
			break;
		case WEAPON_STATE_FIRE: {
			start_timer(2);

			aimResetTimer_.start(weaponPrm()->aimLockTime());
			if(fireTarget_)
				firePosition_ = WeaponTarget(fireTarget_).position();

			ParameterSet prm = fireCost();

			if(!weaponPrm()->fireCostAtOnce())
				resource_request = owner()->requestResource(prm, NEED_RESOURCE_TO_FIRE);

			if(resource_request && ((weaponPrm()->fireDuringClick() && owner()->player()->shootKeyDown()) || 
					(needAutoFire() && !reloadTime_) ||
					(fireTime_ && stateTimer_.busy())))
			{
				if(!weaponPrm()->fireCostAtOnce())
					owner()->subResource(prm);

				setVisibilityOnShoot();

				if(weaponPrm()->queueFire() && queueDelayTimer_.finished()){
					fire(WeaponTarget(fireTarget_, firePosition_));
					startFireEffect();

					queueDelayTimer_.start(weaponPrm()->queueFireDelay());

					if(++currentBarrel_ >= aimControllerPrm().barrelCount())
						currentBarrel_ = 0;
				}
			}
			else {
				if(weaponPrm()->fireDuringClick() && !resource_request)
					owner()->player()->setShootFailed(true);

				if(!weaponPrm()->fireCostAtOnce() && fireTime_ && !stateTimer_.busy())
					owner()->subResource(prm);

				reloadStart();
			}

			stop_timer(2);
		}
		break;
		}
	}
	else 
		stateTimer_.pause();

	if(owner()->player()->active()){
		if(!owner()->isDirectControl()){
			if(isEnabled()){
				if(resource_request && isLoaded()){
					if(!weaponPrm()->effect().isEmpty())
						owner()->startEffect(&weaponPrm()->effect());
					if(!weaponPrm()->disabledEffect().isEmpty())
						owner()->stopEffect(&weaponPrm()->disabledEffect());
				}
				else {
					if(!weaponPrm()->effect().isEmpty())
						owner()->stopEffect(&weaponPrm()->effect());
					if(!weaponPrm()->disabledEffect().isEmpty())
						owner()->startEffect(&weaponPrm()->disabledEffect());
				}
			}
		}

		if(!weaponPrm()->directControlEffect().isEmpty()){
			if(resource_request && owner()->directControl() != DIRECT_CONTROL_DISABLED && isLoaded() && weaponPrm()->ID() == owner()->directControlWeaponID())
				owner()->startEffect(&weaponPrm()->directControlEffect());
			else
				owner()->stopEffect(&weaponPrm()->directControlEffect());
		}
		if(!weaponPrm()->directControlDisabledEffect().isEmpty()){
			if((!resource_request || !isLoaded()) && owner()->directControl() != DIRECT_CONTROL_DISABLED && weaponPrm()->ID() == owner()->directControlWeaponID())
				owner()->startEffect(&weaponPrm()->directControlDisabledEffect());
			else
				owner()->stopEffect(&weaponPrm()->directControlDisabledEffect());
		}
	}
	else {
		if(!weaponPrm()->effect().isEmpty())
			owner()->stopEffect(&weaponPrm()->effect());
		if(!weaponPrm()->disabledEffect().isEmpty())
			owner()->stopEffect(&weaponPrm()->disabledEffect());
		if(!weaponPrm()->directControlEffect().isEmpty())
			owner()->stopEffect(&weaponPrm()->directControlEffect());
		if(!weaponPrm()->directControlDisabledEffect().isEmpty())
			owner()->stopEffect(&weaponPrm()->directControlDisabledEffect());
	}
}

void WeaponBase::endQuant()
{
	if(isEnabled_){
		switch(state_){
		case WEAPON_STATE_FIRE:
			if(hasFireTarget_ && !fireTarget())
				switchOff();

			if(!checkMovement()){
				bool flag = autoFire_;
				switchOff();
				aimResetTimer_.stop();
				autoFire_ = flag;
			}
			break;
		}
	}

	if(fireStarted_ && (!isEnabled() || state_ != WEAPON_STATE_FIRE)){
		fireEnd();
		if(state_ == WEAPON_STATE_FIRE)
			reloadStart();
	}
}

void WeaponBase::moveQuant()
{
	updateLogic();
}

void WeaponBase::interpolationQuant()
{
	if(isEnabled() && weaponSlot()->isAimEnabled())
		weaponSlot()->interpolationQuant();
}

void WeaponBase::showInfo(const Vect2f& pos) const
{
	universe()->circleManager()->addCircle(pos, owner()->radius()+fireRadius(), weaponPrm()->fireRadiusCircle());
	universe()->circleManager()->addCircle(pos, owner()->radius()+fireRadiusMin(), weaponPrm()->fireMinRadiusCircle());
	universe()->circleManager()->addCircle(pos, owner()->radius()+fireRadius()+fireDispersionRadius(), weaponPrm()->fireEffectiveRadiusCircle());
}

void WeaponBase::showDebugInfo()
{
	if(showDebugWeapon.showSelectedOnly && !owner()->selected())
		return;

	if(showDebugWeapon.showWeaponID && weaponPrm()->ID() != showDebugWeapon.showWeaponID)
		return;

	if(showDebugWeapon.showWeaponSlotID != -1 && weaponSlot()->ID() != showDebugWeapon.showWeaponSlotID)
		return;

	if(weaponSlot()->isAimEnabled())
		weaponSlot()->showDebugInfo();

	if(isFireTargetSet() && showDebugWeapon.targetingPosition){
		show_vector(firePosition() + fireDispersion(), 5, Color4c::RED);
		show_line(weaponSlot()->muzzlePosition(currentBarrel()).trans(), firePosition() + fireDispersion(), Color4c::RED);
	}

	if(showDebugWeapon.ownerTarget && owner()->targetUnit())
		show_line(weaponSlot()->muzzlePosition(currentBarrel()).trans(), owner()->targetUnit()->position(), Color4c::RED);

	if(showDebugWeapon.ownerSightRadius)
		show_vector(owner()->position(), owner()->sightRadius(), Color4c::GREEN);

	if(showDebugWeapon.autoTarget && autoTarget())
		show_line(weaponSlot()->muzzlePosition(currentBarrel()).trans(), autoTarget()->position(), Color4c::BLUE);

	if(showDebugWeapon.fireRadius){
		show_vector(owner()->position(), fireRadiusMin() + owner()->radius(), Color4c::BLUE);
		show_vector(owner()->position(), fireRadius() + owner()->radius(), Color4c::BLUE);
		show_vector(owner()->position(), fireRadius() + fireDispersionRadius() + owner()->radius(), Color4c::YELLOW);
	}

	if(showDebugWeapon.load){
		if(!isEnabled()){
			show_vector(weaponSlot()->muzzlePosition(currentBarrel()).trans(), 5.f, Color4c::RED);
		}
		else {
			if(isFiring()){
				show_vector(weaponSlot()->muzzlePosition(currentBarrel()).trans(), 5.f, Color4c::BLUE);
			}
			else if(isLoading()){
				show_vector(weaponSlot()->muzzlePosition(currentBarrel()).trans(), 5.f, reloadFromInventory() ? Color4c::WHITE : Color4c::YELLOW);
			}
			else if(isTargeting()){
				show_vector(weaponSlot()->muzzlePosition(currentBarrel()).trans(), 5.f, Color4c::GREEN);
			}
		}
	}
}

bool WeaponBase::fireRequest(const WeaponTarget& target0, bool aim_only)
{
	xassert(owner_);

	aimResetDisabled_ = true;

	if(state_ == WEAPON_STATE_FIRE && (!owner()->isDirectControl() || aim_only) && !weaponPrm()->continuousFire())
		return false;

	if(!checkMovement())
		return false;
   
	WeaponTarget target = target0;

	if(!checkFogOfWar(target) || !checkTargetMode(target))
		return false;

	if(!target.unit()){
		if(owner()->isDirectControl()){
//			Vect3f pos = target.position() + Vect3f(0.f, 0.f, owner()->height()*0.7f);
//			target.setPosition(pos);
		}
		else {
			if(!weaponPrm()->targetOnWaterSurface())
				target.setPosition(To3D(Vect2f(target.position())));
		}
	}

	firePosition_ = target.position();
	fireTargetSet_ = true;

	if(fireDisp_/* && !owner()->isDirectControl()*/){
		float dist = owner()->position2D().distance(Vect2f(firePosition_)) - owner()->radius() - fireRadius();
		if(target.unit())
			dist -= target.unit()->radius();
		if(dist > 0.f){
			if(fabs(fireDispersion_.x) + fabs(fireDispersion_.y) < FLT_EPS){
				float dispersion = fireDispersionRadius_ < FLT_EPS ? 1.f : clamp(dist / fireDispersionRadius_, 0.f, 1.f);
				float fireDisp = logicRNDfabsRnd(dispersion * float(fireDisp_));
				float angle = logicRNDfabsRnd(M_PI * 2.f);
				float angle2 = logicRNDfabsRnd(M_PI);
				float cosAngle2 = cosf(angle2);
				fireDispersion_.x = fireDisp * cosf(angle) * cosAngle2;
				fireDispersion_.y = fireDisp * sinf(angle) * cosAngle2;
				fireDispersion_.z = fireDisp * sinf(angle2);
			}
		}
		else
	    	fireDispersion_ = Vect3f::ZERO;
	}
	else
    	fireDispersion_ = Vect3f::ZERO;

	if(aim_only){
		aimResetTimer_.start(weaponPrm()->aimLockTime());
		return aim(target.position(), true);
	}

	if(!isLoaded() && state_ != WEAPON_STATE_FIRE){
		if(owner()->isDirectControl() || (owner()->isSyndicateControl() && owner()->attr().syndicateControlAimEnabled)){
			aimResetTimer_.start(weaponPrm()->aimLockTime());
			aim(target.position());
		}
		return false;
	}

	if(!canFireMinTime() && !canFire(NEED_RESOURCE_TO_FIRE)){
		if(owner()->player()->active())
			UI_Dispatcher::instance().sendMessage(UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_SHOOTING);

		return false;
	}

	bool distance_check = fireDistanceCheck(target, true);

	if(!distance_check){
		if(owner()->isDirectControl()){
			Vect3f pos = target.position() - owner()->position();
			pos.normalize(owner()->radius() + fireRadius());
			distance_check = true;
		}
		else
			aimResetDisabled_ = false;
	}

	if(distance_check){
		aimResetTimer_.start(weaponPrm()->aimLockTime());

		Vect3f pos = target.position();

		if(aim(pos + fireDispersion())){
			if(fireAnimationCheck()){
				firePosition_ = target.position();
				fireTarget_ = target.unit();
				if(state_ != WEAPON_STATE_FIRE)
					fireRequested_ = true;
				return true;
			}
		}
		else {
			if(weaponPrm()->syndicateControlMode() == WEAPON_SYNDICATE_CONTROL_FORCE && owner()->isSyndicateControl()){
				pos = firePositionCurrent();
				if(fireAnimationCheck()){
					firePosition_ = pos;
					fireTarget_ = 0;
					if(state_ != WEAPON_STATE_FIRE)
						fireRequested_ = true;
					return true;
				}
			}
		}
	}

	return false;
}

bool WeaponBase::fireDistanceCheck(const WeaponTarget& target, bool allow_dispersion) const
{
	switch(targetDistanceStatus(target)){
		case FIRE_DISTANCE_OK:
			return true;
		case FIRE_DISTANCE_DISP:
			return allow_dispersion;
	}

	return false;
}

WeaponBase::FireDistanceStatus WeaponBase::targetDistanceStatus(const WeaponTarget& target) const
{
	xassert(owner());

	float r = owner_->radius();
	if(target.unit()) r += target.unit()->radius();

	Vect2f dr = Vect2f(target.position()) - owner_->position2D();
	float dist2 = dr.norm2();

	if(dist2 < sqr(r))
		dist2 = sqr(r);

	if(dist2 > sqr(r + fireRadius_ + fireDispersionRadius_))
		return FIRE_DISTANCE_MAX;
	else if(dist2 > sqr(r + fireRadius_) && dist2 <= sqr(r + fireRadius_ + fireDispersionRadius_))
		return FIRE_DISTANCE_DISP;
	else if(dist2 < sqr(r + fireRadiusMin_))
		return FIRE_DISTANCE_MIN;

	return FIRE_DISTANCE_OK;
}

bool WeaponBase::canFire(RequestResourceType triggerAction) const
{
	return owner()->requestResource(prmCache().fireCost(), triggerAction);
}

bool WeaponBase::canFireMinTime(RequestResourceType triggerAction) const
{
	if(weaponPrm()->fireCostAtOnce())
		return false;

	ParameterSet prm = prmCache().fireCost();

	if(weaponPrm()->fireTimeMin() > FLT_EPS)
		prm *= weaponPrm()->fireTimeMin() * 1000.f / fireTime_;

	return owner()->requestResource(prm, triggerAction);
}

void WeaponBase::fireEnd()
{
	fireTargetSet_ = false;
	fireStarted_ = false;

    fireDispersion_ = Vect3f::ZERO;

	if(weaponPrm()->clearTargets() && !needAutoFire() && !owner()->isDirectControl())
		owner_->clearAttackTarget(weaponPrm()->ID());

	stopFireEffect();

	if(weaponPrm()->ammoType()){
		float ammo = parameters().findByType(ParameterType::AMMO, 0.f);
		if(ammo < FLT_EPS){
			owner_->reloadInventoryWeapon(weaponSlot()->ID());
			reloadStart(reloadTimeInventory_, true);
			owner_->updateInventoryWeapon(weaponSlot()->ID());
		}
	}

	float durability = parameters().findByType(ParameterType::WEAPON_DURABILITY, -1.f);
	if(fabs(durability) < FLT_EPS)
		owner_->removeInventoryWeapon(weaponSlot()->ID());
}

void WeaponBase::updateOwner()
{
	updateLogic();
}

void WeaponBase::startFireEffect()
{
	if(!weaponPrm()->fireEffect().isEmpty()){
		if(weaponPrm()->fireEffect().isCycled())
			owner()->startEffect(&weaponPrm()->fireEffect(), true, weaponSlot()->nodeEffectIndexGraphic(currentBarrel()));
		else
			owner()->startEffect(&weaponPrm()->fireEffect(), false, weaponSlot()->nodeEffectIndexGraphic(currentBarrel()), 100);
	}

	if(const SoundAttribute* sound = weaponPrm()->fireSound()){
		if(sound->cycled())
			owner()->startSoundEffect(sound);
		else
			sound->play(owner()->position());
	}
}

void WeaponBase::stopFireEffect()
{
	if(!weaponPrm()->fireEffect().isEmpty())
		owner()->stopEffect(&weaponPrm()->fireEffect());

	if(const SoundAttribute* sound = weaponPrm()->fireSound()){
		if(sound->cycled())
			owner()->stopSoundEffect(sound);
	}
}

bool WeaponBase::fireAnimationCheck() const
{
	return (needAutoFire() || !weaponSlot()->isAimEnabled() 
		|| !aimControllerPrm().hasAnimation() || owner()->weaponAnimationMode());
}

ParameterSet WeaponBase::fireCost() const
{
	float dt = (!weaponPrm()->fireCostAtOnce() && fireTime_) ? (logicPeriodSeconds / (float(fireTime_) / 1000.f)) : 1.f;
	ParameterSet prm = prmCache().fireCost();
	prm *= dt;

	return prm;
}

void WeaponBase::setVisibilityOnShoot()
{
	if(!owner()->attr().isActing() || weaponPrm()->visibleTimeOnShoot() <= FLT_EPS)
		return;
	safe_cast<UnitActing*>(owner())->setVisibility(true, weaponPrm()->visibleTimeOnShoot());
}

void WeaponBase::applyDamage(UnitInterface* target)
{
	float dt = fireTime_ ? (logicPeriodSeconds / (float(fireTime_) / 1000.f)) : 1.f;
	ParameterSet dmg = damage();
	dmg *= dt;

	if(target)
		target->setDamage(dmg, owner());
}

bool WeaponBase::aimDefault()
{
	if(weaponSlot()->isAimEnabled())
		return weaponSlot()->aimDefault();
	else
		return true;
}

void WeaponBase::aimReset()
{
	if(weaponSlot()->isAimEnabled())
		weaponSlot()->aimReset();
}

bool WeaponBase::aimUpdate()
{
	if(isEnabled()){
		weaponSlot()->updateAngles();

		if(fireRequested_ && !isAimed(firePosition_ + fireDispersion_))
			fireRequested_ = false;
	}

	return true;
}


void WeaponBase::setAutoTarget(UnitInterface* target)
{
	if(autoTarget_ != target)
		autoTargetAttacked_ = false;

	autoTarget_ = target;
}

bool WeaponBase::aim(const Vect3f& to, bool aim_only)
{
	if(weaponSlot()->isAimEnabled() && !owner()->inTransport()){
		Vect2f angles = aimAngles(to);
		return weaponSlot()->aim(angles.x, angles.y, aim_only);
	}

	return true;
}

bool WeaponBase::isAimed(const Vect3f& to) const
{
	if(weaponSlot()->isAimEnabled() && !owner()->inTransport()){
		Vect2f angles = aimAngles(to);
		return weaponSlot()->isAimed(angles.x, angles.y);
	}
	return true;
}

Vect2f WeaponBase::aimAngles(const Vect3f& to) const
{
	if(!weaponSlot()->isAimCorrectionEnabled()){
		Vect3f r;
		Se3f(owner()->rigidBody()->orientation(), weaponSlot()->rotationCenterPsi()).invXformPoint(to, r);
		float psi = r.psi() - M_PI_2;

		Se3f(owner()->rigidBody()->orientation(), weaponSlot()->rotationCenterTheta()).invXformPoint(to, r);
		float theta = turnSuggestPrm_ ?
			turnSuggestPrm_->rigidBodyPrm->calcTurnTheta(sqrt(sqr(r.x) + sqr(r.y)), r.z, turnSuggestPrm_->forwardVelocity) : M_PI_2 - r.theta();

		return Vect2f(psi, theta);
	}
	else {
		Vect3f r;
		Vect3f delta = weaponSlot()->rotationCenterPsi() - weaponSlot()->muzzlePosition(currentBarrel()).trans();
		Vect3f to1 = to + delta;

		Se3f(owner()->rigidBody()->orientation(), weaponSlot()->rotationCenterPsi()).invXformPoint(to1, r);
		float psi = r.psi() - M_PI_2;

		delta = weaponSlot()->rotationCenterTheta() - weaponSlot()->muzzlePosition(currentBarrel()).trans();
		to1 = to + delta;
		Se3f(owner()->rigidBody()->orientation(), weaponSlot()->rotationCenterTheta()).invXformPoint(to1, r);
		float theta = turnSuggestPrm_ ?
			turnSuggestPrm_->rigidBodyPrm->calcTurnTheta(sqrt(sqr(r.x) + sqr(r.y)), r.z, turnSuggestPrm_->forwardVelocity) : M_PI_2 - r.theta();

		return Vect2f(psi, theta);
	}
}

void WeaponBase::fireEvent()
{
	if(fireRequested_){
		if(isEnabled() && owner()->alive()){
			ParameterSet prm = fireCost();
			if(owner()->requestResource(prm, NEED_RESOURCE_TO_FIRE) && fire(WeaponTarget(fireTarget_, firePosition_))){
				if(weaponPrm()->fireCostAtOnce())
					owner()->subResource(prm);

				if(weaponPrm()->ammoType()){
					float ammo = parameters().findByType(ParameterType::AMMO, 0.f);
					ammo = max(0.f, ammo - 1.f);
					setParameter(ammo, ParameterType::AMMO);
				}

				float durability = parameters().findByType(ParameterType::WEAPON_DURABILITY, 0.f);
				durability = max(0.f, durability - 1.f);
				setParameter(durability, ParameterType::WEAPON_DURABILITY);

				owner_->updateInventoryWeapon(weaponSlot()->ID());

				updateOwner();

				queueDelayTimer_.start(weaponPrm()->queueFireDelay());

				reloaded_ = false;
				fireStarted_ = true;

				state_ = WEAPON_STATE_FIRE;
				stateTimer_.start(fireTime_);

				hasFireTarget_ = fireTarget() != 0;

				startFireEffect();
				setVisibilityOnShoot();

				if(++currentBarrel_ >= aimControllerPrm().barrelCount())
					currentBarrel_ = 0;

				autoTargetAttacked_ = fireTarget_ && fireTarget_ == autoTarget_;

//				if(weaponPrm()->syndicateControlMode() == WEAPON_SYNDICATE_CONTROL_FORCE && owner()->isSyndicateControl())
//					kdWarning("&stlr", XBuffer() < "fireEvent() ID=" <= weaponPrm()->ID() < " Time=" <= universe()->quantCounter());
			}
		}

		fireRequested_ = false;
	}
}

float WeaponBase::chargeLevel() const
{
	if(state_ == WEAPON_STATE_LOAD && !reloaded_){
		if(reloadTime_)
			return 1.f - float(stateTimer_.timeRest()) / float(reloadTime_);
	}

	return 1.0f;
}

Vect3f WeaponBase::firePositionCurrent() const
{
	if(weaponSlot()->isAimEnabled()){
		Se3f muzzle_pos = weaponSlot()->muzzlePosition();
		Vect3f mp = muzzle_pos.trans();

		Vect3f target_pos;

		Vect3f dir(0, 3000.f, 0);
		muzzle_pos.xformVect(dir);

		Vect3f pos;
		if(!weapon_helpers::traceGround(mp, mp + dir, pos)){
			if(!fireDistanceCheck(WeaponTarget(pos), true)){
				dir.normalize(fireRadius());
				target_pos = To3D(Vect2f(mp + dir));
			}
			else
				target_pos = pos;
		}
		else {
			dir.normalize(fireRadius());
			target_pos = To3D(Vect2f(mp + dir));
		}

		return target_pos;
	}

	return firePosition() + fireDispersion();
}

bool WeaponBase::isTargeting() const
{
	return aimResetTimer_.started();
}

void WeaponBase::kill()
{
	if(fireStarted_)
		fireEnd();

	fireTarget_ = 0;
	autoTarget_ = 0;
}

bool WeaponBase::checkAttackClass(int attack_class) const
{
	if(weaponPrm()->attackClass() & attack_class)
		return true;

	return false;
}

bool WeaponBase::checkDamage(const UnitInterface* unit) const
{
	start_timer_auto();

	const ParameterSet& set = unit->parameters();
	const ParameterSet& set_max = unit->parametersMax();

	bool has_damage = false;

	bool posessed = owner_->isEnemy(unit) || unit->parametersMax().possession() - unit->parameters().possession() > FLT_EPS;

	const ParameterSet& damage = prmCache_.damage();
	if(!damage.empty()){
		has_damage = true;
		if(checkDamage(damage, set, set_max)){
			if((damage.health() < 0.f || damage.possession() < 0.f) && !unit->isConstructed())
				return false;
			return true;
		}
	}

	if(prmCache().abnormalState().isEnabled()){
		const ParameterSet& damage = prmCache().abnormalState().damage();
		if(!damage.empty()){
			has_damage = true;
			if(unit->hasAbnormalState(prmCache().abnormalState()) && checkDamage(damage, set, set_max)){
				if((damage.health() < 0.f || damage.possession() < 0.f) && !unit->isConstructed())
					return false;
				return true;
			}
		}
	}

	for(WeaponSourcePrms::const_iterator it = prmCache_.sources().begin(); it != prmCache_.sources().end(); ++it){
		const ParameterSet& damage = it->damage();
		if(!damage.empty()){
			has_damage = true;
			if(checkDamage(damage, set, set_max)){
				if((damage.health() < 0.f || damage.possession() < 0.f) && !unit->isConstructed())
					return false;
				return true;
			}
		}

		if(it->abnormalState().isEnabled()){
			const ParameterSet& damage = it->abnormalState().damage();
			if(!damage.empty()){
				has_damage = true;
				if(unit->hasAbnormalState(it->abnormalState()) && checkDamage(damage, set, set_max)){
					if((damage.health() < 0.f || damage.possession() < 0.f) && !unit->isConstructed())
						return false;
					return true;
				}
			}
		}
	}

	return !has_damage;
}

bool WeaponBase::checkTargetMode(UnitInterface* target) const
{
	if(!target)
		return true;

	return weaponPrm()->checkTargetMode(unitMode(owner()), unitMode(target));
}

bool WeaponBase::checkTargetMode(const WeaponTarget& target) const
{
	start_timer_auto();

	if(target.unit())
		return weaponPrm()->checkTargetMode(unitMode(owner()), unitMode(target.unit()));
	else {
		WeaponPrm::UnitMode mode = WeaponPrm::ON_GROUND;
		if(target.terrainAttackClass() & ATTACK_CLASS_WATER){
			if(!weaponPrm()->targetOnWaterSurface())
				mode = WeaponPrm::ON_WATER_BOTTOM;
			else
				mode = WeaponPrm::ON_WATER;
		}

		return weaponPrm()->checkTargetMode(unitMode(owner()), mode);
	}
}

bool WeaponBase::checkFogOfWar(const WeaponTarget& target) const
{
	if(weaponPrm()->canShootUnderFogOfWar() || owner_->isDirectControl())
		return true;

	int radius = target.unit() ? target.unit()->radius() : 1;
	if(owner()->player()->fogOfWarMap()){
		if(!owner()->player()->fogOfWarMap()->checkFogStateInCircle(Vect2i(target.position().xi(), target.position().yi()), radius))
			return false;
	}

	return true;
}

bool WeaponBase::checkMovement() const
{
	if(!weaponPrm()->canShootWhileInTransport() && owner()->inTransport())
		return false;

	return weaponPrm()->canShootInMovementMode(owner()->isMoving(), owner()->movementMode());
}

bool WeaponBase::updateParameters()
{
	const ParameterSet& prm = prmCache_.parameters();

	fireRadiusMin_ = prm.findByType(ParameterType::FIRE_RANGE_MIN, 0.0f);
	fireRadius_ = prm.findByType(ParameterType::FIRE_RANGE, 100.0f);

	xassertStr(fireRadius_ > fireRadiusMin_ && "Минимальный радиус атаки больше максимального", owner()->attr().libraryKey());

	reloadTime_ = round(prm.findByType(ParameterType::RELOAD_TIME, 0.1f) * 1000.0f);
	reloadTimeInventory_ = round(prm.findByType(ParameterType::RELOAD_TIME_INVENTORY, float(reloadTime_)/1000.f) * 1000.0f);
	fireTime_ = round(prm.findByType(ParameterType::FIRE_TIME, 0.5f) * 1000.0f);
	
	fireDisp_ = round(prm.findByType(ParameterType::FIRE_DISPERSION, 0));
	fireDispersionRadius_ = prm.findByType(ParameterType::FIRE_RANGE_EFFECTIVE, 0);

	return true;
}

bool WeaponBase::checkDamage(const ParameterSet& damage, const ParameterSet& set, const ParameterSet& set_max)
{
	for(int i = 0; i < damage.size(); i++){
		const ParameterSet::Value& val = damage[i];
		if(val.value < 0.f){
			float value_current = set.findByIndex(val.index);
			float value_max = set_max.findByIndex(val.index);
			if(value_max - value_current > FLT_EPS)
				return true;
		}
		else {
			float value_max = set_max.findByIndex(val.index);
			if(value_max > FLT_EPS)
				return true;
		}
	}

	return false;
}

bool WeaponBase::checkDamage(const ParameterSet& damage, const ParameterSet& set)
{
	for(int i = 0; i < damage.size(); i++){
		const ParameterSet::Value& val = damage[i];
		if(val.value > 0.f){
			float value = set.findByIndex(val.index);
			if(value > FLT_EPS)
				return true;
		}
	}

	return false;
}

class GroundTargetScanOp
{
public:
	GroundTargetScanOp(const Vect2f& pos, const WeaponBase* weapon) : unit_(0),
		position_(pos),
		weapon_(weapon)
	{
	}

	bool operator()(UnitBase* unit)
	{
		if(unit->attr().isEnvironment()){
			if(!(weapon_->weaponPrm()->attackClass() & unit->unitAttackClass()) && unit->position2D().distance2(position_) < sqr(unit->radius()))
				return false;
		}

		if(unit->attr().isResourceItem() || unit->attr().isInventoryItem()){
			if(unit->position2D().distance2(position_) < sqr(unit->radius()))
				return false;
		}

		if(unit->attr().isActing()){
			if(unit->position2D().distance2(position_) < sqr(unit->radius())){
				UnitInterface* ui = safe_cast<UnitInterface*>(unit);
				if(!weapon_->canAttack(WeaponTarget(ui, weapon_->weaponPrm()->ID()))){
					unit_ = ui;
					return false;
				}
			}
		}

		return true;
	}

	UnitInterface* unit(){ return unit_; }

private:

	Vect2f position_;
	const WeaponBase* weapon_;
	UnitInterface* unit_;
};

bool WeaponBase::needAutoFire() const
{
	return weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_ALWAYS ||
		(weaponPrm()->shootingMode() == WeaponPrm::SHOOT_MODE_SQUAD_LEADER && owner()->getSquadPoint() &&
		owner() == owner()->getSquadPoint()->getUnitReal());
}

bool WeaponBase::setGroundTarget(const Vect3f& ground_pos)
{
	GroundTargetScanOp op(Vect2f(ground_pos), this);
	if(!universe()->unitGrid.scanPointConditional(ground_pos.xi(), ground_pos.yi(), op))
		return false;

	WeaponTarget target(ground_pos, weaponPrm()->ID());

	if(!checkFogOfWar(target))
		return false;

	if(canAttack(target)){
		fireTarget_ = 0;
		firePosition_ = ground_pos;
		fireTargetSet_ = true;
		return true;
	}

	return false;
}

WeaponPrm::UnitMode WeaponBase::unitMode(UnitBase* unit)
{
	start_timer_auto();

	if(unit->attr().isActing()){
		if(RigidBodyUnit* bp = safe_cast<UnitActing*>(unit)->rigidBody()){
			if(bp->flyingMode() && !bp->prm().hoverMode)
				return WeaponPrm::ON_AIR;

			if(bp->onDeepWater()){
				if(!bp->waterAnalysis())
					return WeaponPrm::ON_WATER_BOTTOM;

				return WeaponPrm::ON_WATER;
			}

			if(unit->attr().isLegionary() && (safe_cast<UnitLegionary*>(unit)->movementMode() & MODE_CRAWL))
				return WeaponPrm::ON_GROUND_LYING;

			return WeaponPrm::ON_GROUND;
		}
	}

	return WeaponPrm::ON_GROUND;
}

//-------------------------------------------------------

bool WeaponPrmCache::getParametersForUI(const wchar_t* &name, wchar_t type, ParameterSet& out) const
{
	if(type == L'w'){ // личные параметры оружия
		out = parameters();
		return true;
	}

	xassert(type == L'd' || type == L's');
	if(const wchar_t* delimeter = wcschr(name, L'/')){ // задано еще одно имя
		if(delimeter != name){
			string sourceLabel;
			w2a(sourceLabel, wstring(name, delimeter));
			name =  delimeter + 1;

			//сначала ищем непосредственно в кеше
			WeaponSourcePrms::const_iterator it;
			FOR_EACH(sources(), it)
				if(it->checkKey(sourceLabel.c_str())){
					if(type == L'd')
						out = it->damage();
					else
						out = it->abnormalState().damage();
					return true;
				}

			//затем по всему дереву производных источников,
			//эти параметры не кешированы и не могут апгрейдиться
			WeaponSourcePrm prm;
			FOR_EACH(sources(), it)
				if(const SourceAttribute* sattr = it->getSourceByKey(sourceLabel.c_str())){
					sattr->source()->getParameters(prm);
					if(type == L'd')
						out = prm.damage();
					else
						out = prm.abnormalState().damage();
					return true;
				}

		}
	}
	else {
		if(type == L'd')
			out = damage();
		else
			out = abnormalState().damage();
		return true;
	}

	return false;
}

//-------------------------------------------------------

bool WeaponProjectile::fire(const WeaponTarget& target)
{
	xassert(owner() && weaponPrm()->missileID());

	if(weaponPrm()->missileLaunchType() == WeaponProjectilePrm::LAUNCH_SHOT_BEGIN)
		shootProjectile();

	return true;
}

void WeaponProjectile::fireEnd()
{
	if(weaponPrm()->missileLaunchType() == WeaponProjectilePrm::LAUNCH_SHOT_END)
		shootProjectile();

	__super::fireEnd();
}

void WeaponProjectile::kill()
{
	__super::kill();

	clearProjectiles();
}

void WeaponProjectile::shootProjectile()
{
	if(owner()->alive()){
		ProjectileBase* p = 0;

		if(!projectiles_.empty() && projectiles_[currentBarrel()]){
			p = projectiles_[currentBarrel()];
			projectiles_[currentBarrel()]->detachFromDock();
			projectiles_[currentBarrel()] = 0;
		}
		else
			p = safe_cast<ProjectileBase*>(owner()->player()->buildUnit(weaponPrm()->missileID()));

		if(p){
			p->setExplosionParameters(affectMode(), prmCache(), max(1, aimControllerPrm().barrelCount()), weaponPrm()->canShootThroughShield());
			p->setSource(owner(), weaponSlot()->muzzlePosition(currentBarrel()));
			p->setTarget(fireTarget(), firePosition(), fireDispersion().norm());
		}

	}
}

//-------------------------------------------------------

WeaponBeam::WeaponBeam(WeaponSlot* slot, const WeaponPrm* prm) : WeaponBase(slot, prm)
	, effect_(0)
	, chainLightningFunctor_(this)
	, chainLightningController_(&chainLightningFunctor_)
	, fireTargetInitial_(0)
{
	beamPrevPos_[0] = beamPrevPos_[1] = Vect3f::ZERO;
	firePositionInitial_ = Vect3f::ZERO;
	fireDispersionInitial_ = Vect3f::ZERO;
}

WeaponBeam::~WeaponBeam()
{
}

void WeaponBeam::quant()
{
	start_timer_auto();

	__super::quant();

	if(state() == WeaponBase::WEAPON_STATE_FIRE){
		if(weaponPrm()->continuousFire() || owner()->isDirectControl()){
			updateTarget();
			createSources();
			if(fireTarget())
				createTargetEffects();
		}
		updateEffect();

		if(fireTarget()){
			if(prmCache().abnormalState().isEnabled())
				fireTarget()->setAbnormalState(prmCache().abnormalState(), owner());

			if(!weaponPrm()->damageAtOnce())
				applyDamage(fireTarget());

			if(!fireTarget() && !owner()->isDirectControl() && !weaponPrm()->continuousFire())
				switchOff();
		}

		if(weaponPrm()->continuousFire()){
			setFireTarget(fireTargetInitial_);
			setFirePosition(firePositionInitial_);
			setFireDispersion(fireDispersionInitial_);
		}
	}

	chainLightningController_.quant();
}

bool WeaponBeam::fire(const WeaponTarget& target)
{
	start_timer_auto();

	if(weaponPrm()->continuousFire()){
		fireTargetInitial_ = fireTarget();
		firePositionInitial_ = firePosition();
		fireDispersionInitial_ = fireDispersion();
	}

	updateTarget();

//	if(fireTarget())
//		setFireDispersion(Vect3f::ZERO);

	createSources();
	createEffect();

	if(fireTarget()){
		createTargetEffects();
		if(weaponPrm()->damageAtOnce())
			fireTarget()->setDamage(damage(), owner());
	}

	if(weaponPrm()->useChainEffect() && fireTarget()){
		chainLightningController_.start(&weaponPrm()->chainLightningAttribute());
		chainUpdate();
	}

	return true;
}

void WeaponBeam::kill()
{
	__super::kill();

	chainLightningController_.release();

	releaseEffect();
}

void WeaponBeam::showDebugInfo()
{
	__super::showDebugInfo();

	if(showDebugWeapon.showSelectedOnly && !owner()->selected())
		return;

	chainLightningController_.showDebug();

	if(showDebugWeapon.load && isFiring()){
		Vect3f v0 =	weaponSlot()->muzzlePosition(currentBarrel()).trans();
		Vect3f v1 = firePosition() + fireDispersion();

		show_vector(v0, 3.f, Color4c::RED);
		show_vector(v1, 3.f, Color4c::RED);
	}
}

void WeaponBeam::serialize(Archive& ar)
{
	__super::serialize(ar);

	if(universe()->userSave()){
		ar.serialize(fireTargetInitial_, "fireTargetInitial", 0);
		ar.serialize(firePositionInitial_, "firePositionInitial", 0);
		ar.serialize(fireDispersionInitial_, "fireDispersionInitial", 0);
	}
}

void WeaponBeam::fireEnd()
{
	__super::fireEnd();

	chainLightningController_.stop();

	releaseEffect();
}

bool WeaponBeam::createEffect()
{
	if(!effect_){
		Vect3f v0 =	weaponSlot()->muzzlePosition(currentBarrel()).trans();
		Vect3f v1 = firePosition() + fireDispersion();
		
		if(v0.distance2(v1) < 1.f)
			return false;

		if(EffectKey* eff_key = weaponPrm()->effect(weaponPrm()->effectScale(), owner()->player()->unitColor()))
			effect_ = terScene->CreateEffectDetached(*eff_key, 0);

		if(effect_){
			effect_->SetTarget(v0, v1);

			beamPrevPos_[0] = v0;
			beamPrevPos_[1] = v1;

			attachSmart(effect_);
		}
	}

	return false;
}

bool WeaponBeam::updateEffect()
{
	chainUpdate();

	if(effect_){
		Vect3f v0 =	weaponSlot()->muzzlePosition(currentBarrel()).trans();
		Vect3f v1 = firePosition() + fireDispersion();

		streamLogicInterpolator.set(fBeamInterpolation, effect_) << v0 << v1 << beamPrevPos_[0] << beamPrevPos_[1];

		beamPrevPos_[0] = v0;
		beamPrevPos_[1] = v1;

		return true;
	}

	return false;
}

bool WeaponBeam::createTargetEffects()
{
	const TargetEffects& effects = weaponPrm()->targetEffects();
	if(effects.empty())
		return false;

	UnitInterface* unit = fireTarget();
	if(!unit) return false;

	Vect3f pos0 = weaponSlot()->muzzlePosition(currentBarrel()).trans();
	Vect3f pos1 = firePosition() + fireDispersion();
	pos1 += pos1 - pos0;

	Vect3f pos;
	if(unit->intersect(pos0, pos1, pos)){
		WeaponPrm::UnitMode target_mode = unitMode(fireTarget());
		for(TargetEffects::const_iterator it = effects.begin(); it != effects.end(); ++it){
			if(it->targetMode() & (1 << target_mode) && (it->targetAttackClass() == ATTACK_CLASS_IGNORE || (unit->unitAttackClass() & it->targetAttackClass()))){
				BaseUniverseObject ancor;
				ancor.setPose(Se3f(QuatF::ID, pos), true);

				EffectController controller(&ancor);
				controller.effectStart(&it->effect());

				controller.release();

				if(const SoundAttribute* sound = it->sound()){
					if(!sound->cycled())
						sound->play(firePosition() + fireDispersion());
				}
			}
		}
		return true;
	}

	return false;
}

void WeaponBeam::chainUpdate()
{
	if(UnitInterface* unit = fireTarget()){
		unit = unit->getUnitReal();
		if(unit->attr().isActing() && chainLightningController_.needChainUpdate()){
			UnitActingList chainEmitter;
			chainEmitter.push_back(safe_cast<UnitActing*>(unit));
			chainLightningController_.update(chainEmitter);
		}
	}
	else
		chainLightningController_.stop();
}

bool WeaponBeam::createSources()
{
	switch(weaponPrm()->sourcesCreationMode()){
	case WEAPON_SOURCES_CREATE_ON_TARGET_HIT:
		if(!fireTarget())
			return false;
		break;
	case WEAPON_SOURCES_CREATE_ON_GROUND_HIT:
		if(fireTarget() || !groundHit())
			return false;
		break;
	case WEAPON_SOURCES_CREATE_ALWAYS:
		if(!fireTarget() && !groundHit())
			return false;
		break;
	}

	int idx = 0;
	SourceWeaponAttributes::const_iterator it;
	FOR_EACH(weaponPrm()->sourceAttr(), it){
		if(!it->isEmpty()){
			Se3f pos = Se3f(weaponSlot()->isAimEnabled() ? QuatF(aimPsi(), Vect3f::J, 0) : owner()->orientation(), firePosition() + fireDispersion());
			if(SourceBase* p = sourceManager->createSource(&*it, pos, false)){
				p->setAffectMode(affectMode());
				p->setPlayer(owner()->player());
				p->setOwner(owner());
				p->setParameters(prmCache().sources()[idx], &WeaponTarget(fireTarget(), firePosition() + fireDispersion()));
			}
		}
		idx++;
	}

	return true;
}

void WeaponBeam::updateTarget()
{
	if(weaponPrm()->needShieldTrace() || weaponPrm()->needSurfaceTrace() || weaponPrm()->needUnitTrace()){
		WeaponBeamTrace trace_info;

		Se3f muzzle_pos = weaponSlot()->muzzlePosition();
		Vect3f mp = muzzle_pos.trans();
		Vect3f target_pos = firePosition() + fireDispersion();

		bool ground_pos_update = false;
		bool ground_trace_missed = false;
		if(weaponPrm()->continuousFire()){
			if(weaponSlot()->isAimEnabled()){
				Vect3f dir(0, 3000.f, 0);
				muzzle_pos.xformVect(dir);

				Vect3f pos;
				if(!weapon_helpers::traceGround(mp, mp + dir, pos)){
					target_pos = pos;
					ground_pos_update = true;

					if(!fireDistanceCheck(WeaponTarget(pos), true)){
						dir.normalize(fireRadius());
						target_pos = To3D(Vect2f(mp + dir));
						ground_pos_update = true;
					}
				}
				else {
					dir.normalize(fireRadius());
					target_pos = To3D(Vect2f(mp + dir));
					ground_pos_update = true;
					ground_trace_missed = true;
				}
			}
		}

		Vect3f pos;

		Vect3f shieldCenter;
		if(weaponPrm()->needShieldTrace()){
			if(!SourceShield::traceShieldsThrough(mp, target_pos, owner()->player(), &pos, &shieldCenter))
				trace_info.set(WeaponBeamTrace::TRACE_SHIELD, pos, mp.distance2(pos));
		}

		if(weaponPrm()->needSurfaceTrace()){
			if(!weapon_helpers::traceGround(mp, target_pos, pos))
				trace_info.set(WeaponBeamTrace::TRACE_GROUND, pos, mp.distance2(pos));
		}

		Vect3f scan_pos = target_pos + target_pos - mp;
		weapon_helpers::TraceUnitScanOp unit_scan_op(mp, scan_pos, weaponPrm(), owner(), fireTarget(), weaponPrm()->environmentDestruction());

		if(weaponPrm()->needUnitTrace()){
			if(!universe()->unitGrid.ConditionLine(mp.xi(), mp.yi(), scan_pos.xi(), scan_pos.yi(), unit_scan_op)){
				pos = unit_scan_op.endPosition();
				trace_info.set(WeaponBeamTrace::TRACE_UNITS, pos, mp.distance2(pos));
			}
		}

		if(!trace_info.isEmpty()){
			bool select = false;
			WeaponBeamTrace::Mode selected_mode = WeaponBeamTrace::TRACE_GROUND;

			for(int i = 0; i < 3; i++){
				WeaponBeamTrace::Mode mode = WeaponBeamTrace::Mode(i);
				if(trace_info.isEnabled(mode) && (!select || trace_info.dist2(mode) < trace_info.dist2(selected_mode))){
					select = true;
					selected_mode = mode;
				}
			}

			pos = trace_info.position(selected_mode);

			bool clear_fire_target = true;
			if(weaponPrm()->needUnitTrace()){
				weapon_helpers::TraceUnitScanOp::ContactList::const_iterator it;
				float dist = trace_info.dist2(selected_mode);
				for(it = unit_scan_op.unitsToDestruct().begin(); it != unit_scan_op.unitsToDestruct().end(); ++it){
					if(it->unit->alive() && owner()->position2D().distance2(it->contactPosition) < dist){
						it->unit->explode();
						it->unit->Kill();
					}
				}

				if(UnitBase* p = unit_scan_op.endUnit()){
					if(p->alive() && p->attr().isActing()){
						if(selected_mode == WeaponBeamTrace::TRACE_UNITS && canAttack(WeaponTarget(safe_cast<UnitInterface*>(p), weaponPrm()->ID()))){
							owner()->setAttackTargetUnreachable(true);
							setFireTarget(safe_cast<UnitInterface*>(p));
							setFirePosition(WeaponTarget(fireTarget()).position());
							setFireDispersion(pos - firePosition());
							clear_fire_target = false;
						}
					}
				}
			}

			if(selected_mode == WeaponBeamTrace::TRACE_SHIELD)
				SourceShield::shieldExplodeEffect(shieldCenter, pos, weaponPrm()->shieldEffect());
			
			if(clear_fire_target){
				owner()->setAttackTargetUnreachable(true);
				clearFireTarget();
			}

			if(!fireTarget()){
				setFirePosition(pos);
				setFireDispersion(Vect3f::ZERO);
			}
		}
		else {
			if(!ground_trace_missed || !fireTarget()){
				clearFireTarget();

				if(ground_pos_update){
					setFirePosition(target_pos);
					setFireDispersion(Vect3f::ZERO);
				}
			}

			if(weaponPrm()->needUnitTrace()){
				weapon_helpers::TraceUnitScanOp::ContactList::const_iterator it;
				for(it = unit_scan_op.unitsToDestruct().begin(); it != unit_scan_op.unitsToDestruct().end(); ++it){
					if(it->unit->alive()){
						it->unit->explode();
						it->unit->Kill();
					}
				}
			}
		}
	}
}

bool WeaponBeam::groundHit() const
{
	const float max_delta = 5.f;

	Vect3f pos = To3D(firePosition());
	if(environment->water() && environment->water()->isFullWater(pos))
		pos.z = environment->water()->GetZFast(pos.xi(), pos.yi());

	return firePosition().z - pos.z <= max_delta;
}

// ------------------------------

WeaponProjectilePrm::WeaponProjectilePrm()
{
	weaponClass_ = WeaponPrm::WEAPON_PROJECTILE;
	needMissileVisualisation_ = false;
	missileLaunchType_ = LAUNCH_SHOT_BEGIN;
}

bool WeaponProjectilePrm::targetOnWaterSurface() const
{
	if(missileID_)
		return missileID_->rigidBodyPrm->waterAnalysis;
	else
		return true;
}

void WeaponProjectilePrm::initCache(WeaponPrmCache& cache) const
{
	if(const AttributeProjectile* attr = missileID()){
		cache.setDamage(attr->damage());
		cache.setAbnormalState(attr->explosionState());
		cache.setSources(attr->harmAttr.deathAttribute(0).sources);
	}
}

void WeaponProjectilePrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);

	ar.serialize(missileID_, "projectile", "Снаряд");
	ar.serialize(needMissileVisualisation_, "needMissileVisualisation", "Показывать снаряд на оружии");
	ar.serialize(missileLaunchType_, "missileLaunchType", "Когда выстреливать снаряд");
}

bool WeaponProjectilePrm::needMissileVisualisation() const
{
	return needMissileVisualisation_;
}

const AttributeProjectile* WeaponProjectilePrm::missileID() const
{
	return &*missileID_;
}
// ------------------------------

WeaponBeamPrm::WeaponBeamPrm()
{
	weaponClass_ = WeaponPrm::WEAPON_BEAM;
	needSurfaceTrace_ = false;
	needShieldTrace_ = true;
	needUnitTrace_ = true;
	damageAtOnce_ = false;
	effectScale_ = 1.f;
	effectStopImmediately_ = false;
	legionColor_ = false;
	sourcesCreationMode_ = WEAPON_SOURCES_CREATE_ON_GROUND_HIT;
	environmentDestruction_ = 0;
	useChainEffect_ = false;
}

void WeaponBeamPrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);

	ar.serialize(needSurfaceTrace_, "needSurfaceTrace", "учитывать поверхность");
	ar.serialize(needUnitTrace_, "needUnitTrace", "учитывать юнитов");
	ar.serialize(needShieldTrace_, "needShieldTrace", "учитывать защитное поле");
	ar.serialize(damageAtOnce_, "damageAtOnce", "Наносить все повреждения сразу");
	ar.serialize(shieldEffect_, "shieldEffect", "Эффект при попадании в защитное поле");
	ar.serialize(targetEffects_, "targetEffects", "Эффекты при попадании в цель");
	ar.serialize(effectReference_, "effectReference", "спецэффект луча");
	ar.serialize(effectScale_, "effectScale", "масштаб спецэффекта луча");
	ar.serialize(effectStopImmediately_, "effectStopImmediately", "обрывать спецэффект луча по окончании выстрела");

	ar.serialize(legionColor_, "legionColor", "красить луч в цвет легиона");

	if(needUnitTrace_)
		ar.serialize(environmentDestruction_, "environmentDestruction", "Разрушение объектов окружения");

	ar.serialize(sourcesCreationMode_, "sourcesCreationMode", "создавать источники");

	ar.serialize(sources_, "sources", "параметры источников");

	ar.serialize(useChainEffect_, "useChainEffect", "создавать цепной эффект");
	if(useChainEffect_){
		ar.serialize(chainLightningAttribute_, "chainLightningAttribute", "параметры цепного эффекта");
		if(!chainLightningAttribute_.strike_effect_.get())
			chainLightningAttribute_.strike_effect_ = effectReference_;
	}
}

EffectKey* WeaponBeamPrm::effect(float scale, Color4c color) const
{ 
	if(!legionColor_) color = Color4c(255,255,255,255);
	return effectReference_ ? effectReference_->getEffect(scale, color) : 0;
}

void WeaponBeamPrm::initCache(WeaponPrmCache& cache) const
{
	__super::initCache(cache);
	cache.setSources(sources_);
}

void TargetEffect::serialize(Archive& ar)
{
	ar.serialize(effect_, "effect", "Эффект");
	ar.serialize(sound_, "sound", "Звук попадания");
	ar.serialize(targetMode_, "targetMode", "Выводить на целях");
	ar.serialize(targetAttackClass_, "targetAttackClass", "Тип цели");
}

//---------------------------------------------

WeaponAreaEffectPrm::WeaponAreaEffectPrm()
{
	weaponClass_ = WeaponPrm::WEAPON_AREA_EFFECT;
	zonePositionMode_ = ZONE_POS_OWNER;
	fowRadius_ = 0;
}

void WeaponAreaEffectPrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);

	ar.serialize(sources_, "sources", "параметры источников");
	ar.serialize(zonePositionMode_, "zonePositionMode_", "позиционирование источников");

	if(!ar.inPlace()){
		fowRadius_ *= FogOfWar::step;
		ar.serialize(fowRadius_, "fowRadiusW", "радиус видимости");
		fowRadius_ /= FogOfWar::step;
	}
}

void WeaponAreaEffectPrm::initCache(WeaponPrmCache& cache) const
{
	__super::initCache(cache);
	cache.setSources(sources_);
}

// --------------------------------------------------------------------------

WeaponPadPrm::WeaponPadPrm()
{
	weaponClass_ = WeaponPrm::WEAPON_PAD;
	type_ = DIG_IN;
	surfaceType_ = (unsigned int)(-1);
	fowRadius_ = 0;

	power_ = 400;
	digLenght_ = 100;
	deselectByPower_ = false;
}

void WeaponPadPrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);

	ar.serialize(attrPad_, "attrPad", "^Юнит-лапа");
	
	ar.serialize(pickedUnits_, "pickedUnits", "Переносимые юниты");
	ar.serialize(pickedItems_, "pickedItems", "Подбираемые предметы");

	ar.serialize(type_, "type", "&Работа с землей");

	if(attrPad_.key() >= 0){
		if(!ar.serialize(toolser_, "padtoolser", "Тулзеры")){ ///  CONVERSION 2008-1-24
			vector<TerToolCtrl> toolsers;
			TerToolCtrl tert;
			if(!ar.serialize(tert, "toolser", 0))
				ar.serialize(toolsers, "toolsers", 0);
			else
				toolsers.push_back(tert);
			toolser_.setControllers(toolsers);
		}
		
		ar.serialize(surfaceType_, "surfaceType", "Поверхность на которой работает");

		if(type_ == DIG_IN){
			ar.serialize(RangedWrapperf(power_, 200.f, 1500.f), "diginPower", "Длина траншеи для полного наполнения");
			ar.serialize(RangedWrapperf(digLenght_, 50.f, power_), "digLenght", "Макс длина единичного копка");
		}

		if(type_ == POUR_OUT){
			ar.serialize(RangedWrapperf(power_, 0.5f, 7.f), "pouroutSpeed", "Время (сек) полного высыпания");
			ar.serialize(RangedWrapperf(digLenght_, 4.f, 30.f), "dispersion", "Разброс высыпания");
		}

		ar.serialize(deselectByPower_, "deselectByPower", "Сбрасывать оружие при полном наборе/высыпании");

		if(const AttributeBase* pad = attrPad_.get())
			linkNode_.setName(pad->modelName.c_str());
		ar.serialize(linkNode_, "linkNode", "Узел привязки связи с курсором");
		ar.serialize(linkToCursor_, "linkToCursor", "Эффект связи с курсором");
		
		ar.serialize(fowRadius_, "fowRadius", "радиус видимости");
	}
}

bool WeaponPadPrm::canPickItem(const UnitObjective* item) const
{
	AttributeItemReferences::const_iterator it;
	FOR_EACH(pickedItems_, it)
		if(&item->attr() == *it)
			return true;
	return false;
}

bool WeaponPadPrm::canPickUnit(const UnitObjective* unit) const
{
	AttributeUnitReferences::const_iterator it;
	FOR_EACH(pickedUnits_, it)
		if(&unit->attr() == *it)
			return true;
	return false;
}

//---------------------------------------------

WeaponGripPrm::WeaponGripPrm()
{
	weaponClass_ = WeaponPrm::WEAPON_GRIP;
	horizontalImpulse_ = 100;
	verticalImpulse_ = 400;
	forwardAngle_ = 0;
	dispersionAngle_ = 45;
	startGripTime_ = 0;
	finishGripTime_ = 2000;
}

void WeaponGripPrm::serialize(Archive& ar)
{
	WeaponPrm::serialize(ar);
	ar.serialize(horizontalImpulse_, "horizontalImpulse", "Горизонтальный импульс");
	ar.serialize(verticalImpulse_, "verticalImpulse", "Вертикальный импульс");
	ar.serialize(RadianWrapper(forwardAngle_), "forwardAngle", "Направление выбрасывания (градусы)");
	ar.serialize(dispersionAngle_, "dispersionAngle", "Угол разлета");
	ar.serialize(MillisecondsWrapper(startGripTime_), "startGripTime", "Время начала захвата");
	finishGripTime_ -= startGripTime_;
	ar.serialize(MillisecondsWrapper(finishGripTime_), "finishGripTime", "Время захвата");
	finishGripTime_ += startGripTime_;
}

//---------------------------------------------

namespace weapon_helpers {

const int SHOOT_TRACE_SHIFT = 10;

bool traceGround(const Vect3f& from, const Vect3f& to, Vect3f& out)
{
	start_timer_auto();

	int x,y,z;
	int dx,dy,dz;
	int step,max_step;	
	int x0,y0,x1,y1,z0,z1;
	
	x0 = round(from.x) >> kmGrid;
	y0 = round(from.y) >> kmGrid;
	z0 = round(from.z);
	
	x1 = round(to.x) >> kmGrid;
	y1 = round(to.y) >> kmGrid;
	z1 = round(to.z);
	
	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;
	
	if(!dx && !dy){
		out = to;
		return true;
	}
	
	if(abs(dx) > abs(dy)){
		if(dx > 0){
			max_step = dx;
			dy = (dy << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dx = 1 << SHOOT_TRACE_SHIFT;
		}else{
			max_step = -dx;
			dy = (dy << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dx = -(1 << SHOOT_TRACE_SHIFT);
		}
	}else{
		if(dy > 0){
			max_step = dy;
			dx = (dx << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dy = 1 << SHOOT_TRACE_SHIFT;
		}else{
			max_step = -dy;
			dx = (dx << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dy = -(1 << SHOOT_TRACE_SHIFT);
		}
	}
	
	step = max_step;
	x = x0 << SHOOT_TRACE_SHIFT;
	y = y0 << SHOOT_TRACE_SHIFT;
	z = z0 << SHOOT_TRACE_SHIFT;

	int x_max = vMap.H_SIZE << SHOOT_TRACE_SHIFT;
	int y_max = vMap.V_SIZE << SHOOT_TRACE_SHIFT;
	
	while(step > 0){
		if(x >= 0 && x < x_max && y >= 0 && y < y_max){
			if(vMap.gVBuf[vMap.offsetGBufC(x >> SHOOT_TRACE_SHIFT, y >> SHOOT_TRACE_SHIFT)] > (z >> SHOOT_TRACE_SHIFT)){
				out = Vect3f((x >> SHOOT_TRACE_SHIFT) << kmGrid, (y >> SHOOT_TRACE_SHIFT) << kmGrid, vMap.gVBuf[vMap.offsetGBufC(x >> SHOOT_TRACE_SHIFT, y >> SHOOT_TRACE_SHIFT)]);
				return false;
			}
		}
		
		x += dx;
		y += dy;
		z += dz;
		step--;
	}

	out = to;
	return true;
}

bool traceHeight(const Vect3f& from, const Vect3f& to, Vect3f& out)
{
	int x,y,z;
	int dx,dy,dz;
	int step,max_step;	
	int x0,y0,x1,y1,z0,z1;
	
	x0 = round(from.x) >> kmGrid;
	y0 = round(from.y) >> kmGrid;
	z0 = round(from.z);
	
	x1 = round(to.x) >> kmGrid;
	y1 = round(to.y) >> kmGrid;
	z1 = round(to.z);
	
	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;
	
	if(!dx && !dy){
		out = to;
		return true;
	}
	
	if(abs(dx) > abs(dy)){
		if(dx > 0){
			max_step = dx;
			dy = (dy << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dx = 1 << SHOOT_TRACE_SHIFT;
		}else{
			max_step = -dx;
			dy = (dy << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dx = -(1 << SHOOT_TRACE_SHIFT);
		}
	}else{
		if(dy > 0){
			max_step = dy;
			dx = (dx << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dy = 1 << SHOOT_TRACE_SHIFT;
		}else{
			max_step = -dy;
			dx = (dx << SHOOT_TRACE_SHIFT) / max_step;
			dz = (dz << SHOOT_TRACE_SHIFT) / max_step;
			dy = -(1 << SHOOT_TRACE_SHIFT);
		}
	}
	
	int maxMapZ = 0;
	bool retVal = true;

	step = max_step;
	x = x0 << SHOOT_TRACE_SHIFT;
	y = y0 << SHOOT_TRACE_SHIFT;
	z = z0 << SHOOT_TRACE_SHIFT;
	
	while(step > 0){
		int mapZ = vMap.gVBuf[vMap.offsetGBufC(x >> SHOOT_TRACE_SHIFT, y >> SHOOT_TRACE_SHIFT)];
		if(mapZ > (z >> SHOOT_TRACE_SHIFT) && mapZ > maxMapZ){
			maxMapZ = mapZ;
			out = Vect3f((x >> SHOOT_TRACE_SHIFT) << kmGrid, (y >> SHOOT_TRACE_SHIFT) << kmGrid, vMap.gVBuf[vMap.offsetGBufC(x >> SHOOT_TRACE_SHIFT, y >> SHOOT_TRACE_SHIFT)]);
			retVal = false;
		}
		
		x += dx;
		y += dy;
		z += dz;
		step--;
	}

	return retVal;
}

bool traceUnit(const Vect3f& from, const Vect3f& to, Vect3f& out, const UnitReal* owner, const UnitBase* target, int environment_destruction)
{
	TraceUnitScanOp op(from, to, 0, owner, target, environment_destruction);
	if(!universe()->unitGrid.ConditionLine(from.xi(), from.yi(), to.xi(), to.yi(), op)){
		out = op.endPosition();
		return op.endUnit() == target;
	}

	return true;
}

bool traceAimPosition(const Vect3f& from, const Vect3f& to, const UnitReal* owner, Vect3f& aim_pos, const UnitBase*& aim_unit)
{
	AimPositionScanOp op(from, to, owner);
	universe()->unitGrid.ConditionLine(from.xi(), from.yi(), to.xi(), to.yi(), op);
	op.traceGround();

	aim_pos = op.aimPosition();
	aim_unit = op.aimUnit();

	return true;
}

bool traceVisibility(const UnitBase* unit, const UnitBase* target_unit)
{
	TraceVisibilityScanOp op(unit, target_unit);

	Vect3f from = op.beginPosition();
	Vect3f to = op.endPosition();

	Vect3f ground_pos;
	if(traceHeight(from, to, ground_pos))
		return universe()->unitGrid.ConditionLine(from.xi(), from.yi(), to.xi(), to.yi(), op);

	return false;
}

}; // namespace weapon_helpers

//-------------------------------------------------------

bool WeaponSourceController::create()
{
	xassert(attribute_);

	if(source_)
		release();

	bool startFlag = false;
	source_ = sourceManager->createSource(attribute_, pose_, !attribute_->isAutonomous(), &startFlag);

	return startFlag;
}

void WeaponSourceController::start()
{
	xassert(source_);
	source_->setActivity(true);
}

bool WeaponSourceController::release()
{
	if(source_){
		if(!attribute_->isAutonomous() || !source_->isKillTimerStarted())
			source_->kill();

		source_ = 0;

		return true;
	}

	return false;
}

bool WeaponSourceController::quant()
{
	if(source_){
		if(!source_->followPath())
			source_->setPose(pose_, true);
		if(!source_->isAlive())
			source_ = 0;
	}

	return false;
}

void WeaponSourceController::setOwner(UnitBase* p)
{
	if(source_){
		source_->setPlayer(p->player());
		source_->setOwner(p);
	}
}

void WeaponSourceController::setPose(const Se3f& pose)
{
	Vect3f delta = Vect3f(attribute_->positionDelta());
	pose.rot().xform(delta);
	pose_ = pose;

	pose_.trans() += delta;
	
	if(source_)
		source_->setPose(pose_, true);
}

WeaponSourceController::WeaponSourceController(const SourceWeaponAttribute* attribute) 
: attribute_(attribute), source_(0), pose_(Se3f::ID)
{
	ID_ = 0;
}


//-------------------------------------------------------

WeaponAreaEffect::WeaponAreaEffect(WeaponSlot* slot, const WeaponPrm* prm) : WeaponBase(slot, prm)
{
	fowHandle_ = -1;

	sourceControllers_.reserve(weaponPrm()->sources().size());

	for(int i = 0; i < weaponPrm()->sources().size(); i++){
		if(!weaponPrm()->sources()[i].isEmpty()){
			sourceControllers_.push_back(WeaponSourceController(&weaponPrm()->sources()[i]));
			sourceControllers_.back().setID(i);
		}
	}
}

WeaponAreaEffect::~WeaponAreaEffect()
{
	xassert(fowHandle_==-1);
}

void WeaponAreaEffect::quant()
{
	start_timer_auto();

	__super::quant();

	if(state() == WeaponBase::WEAPON_STATE_FIRE)
		updatePosition();

	std::for_each(sourceControllers_.begin(), sourceControllers_.end(),
		std::mem_fun_ref(&WeaponSourceController::quant));
}

void WeaponAreaEffect::showInfo(const Vect2f& pos) const
{
	__super::showInfo(pos);

	Vect2f shootPos(pos);

	if(weaponPrm()->zonePositionMode() != WeaponAreaEffectPrm::ZONE_POS_OWNER)
		shootPos = UI_LogicDispatcher::instance().hoverPosition();

	SourceControllers::const_iterator it;
	FOR_EACH(sourceControllers_, it)
		if(const SourceBase* source = it->attr()->source())
			universe()->circleManager()->addCircle(shootPos, source->radius(), it->attr()->showParam());
		else{
			xassert(0 && "не разыменовался источник из библиотеки");
		}
}

bool WeaponAreaEffect::checkFinalCost() const
{
	for(int i = 0; i < sourceControllers_.size(); i++){
		if(sourceControllers_[i].attr()->source()->type() == SOURCE_ZONE){
			const SourceZone* sourceZone = safe_cast<const SourceZone*>(sourceControllers_[i].attr()->source());
			if(sourceZone && sourceZone->editType() == SourceZone::ZONE_GENERATOR && 
				sourceZone->unitGenerationMode() == SourceZone::GENERATION_MODE_ZONE){ 
					const AttributeBase* attrUnit = sourceZone->generatedUnit();
					if(attrUnit){
						// проверка для AI
						if(owner()->player()->isAI() && owner()->player()->checkUnitNumber(attrUnit)==1 && attrUnit->isBuilding() && attrUnit->hasProdused())
							return false;
						//
						if(!owner()->player()->checkUnitNumber(attrUnit))
							return false;
						//if(!owner()->player()->accessible(attrUnit))
						//	return false;
						//if(!owner()->requestResource(attrUnit->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
						//	return false;
						//return true;
					}
			}
		}
	} 
	return true;
}

bool WeaponAreaEffect::checkTerrain() const
{
	if(!__super::checkTerrain())
		return false;

	if(weaponPrm()->zonePositionMode() == WeaponAreaEffectPrm::ZONE_POS_OWNER && (weaponPrm()->attackClass() & ATTACK_CLASS_GROUND_ALL)){
		if(!canAttack(WeaponTarget(owner()->position(), weaponPrm()->ID()), true))
			return false;
	}

	return true;
}

bool WeaponAreaEffect::fire(const WeaponTarget& target)
{
	xassert(!isFiring() && owner()->alive());

	if(weaponPrm()->fowRadius()){
		if(owner()->player()->fogOfWarMap())
			fowHandle_ = owner()->player()->fogOfWarMap()->createVisible();
	}

	updatePosition(true);
	for(int i = 0; i < sourceControllers_.size(); i++){
		bool needStart = sourceControllers_[i].create();
		sourceControllers_[i].setAffectMode(affectMode());
		sourceControllers_[i].setOwner(owner());

		if(!weaponPrm()->sources()[i].isAutonomous())
			sourceControllers_[i].setLifeTime(fireTime());

		int id = sourceControllers_[i].ID();
		if(id >= 0 && id < prmCache().sources().size())
			sourceControllers_[i].setParameters(prmCache().sources()[id], &target);
		
		if(needStart)
			sourceControllers_[i].start();
	}

	return true;
}

void WeaponAreaEffect::fireEnd()
{
	__super::fireEnd();

	releaseSources();
}

void WeaponAreaEffect::kill()
{
	__super::kill();

	releaseSources();
}

QuatF WeaponAreaEffect::sourcesOrientation() const
{
	QuatF rot = owner()->orientation();

	if(weaponSlot()->isAimEnabled())
		return rot.postmult(QuatF(aimPsi(), Vect3f::K));
	else
		return rot;
}


void WeaponAreaEffect::updatePosition(bool init)
{
	if(weaponPrm()->zonePositionMode() == WeaponAreaEffectPrm::ZONE_POS_OWNER){
		for(int i = 0; i < sourceControllers_.size(); i++)
			sourceControllers_[i].setPose(Se3f(sourcesOrientation(), owner()->position()));
	}
	else if(weaponPrm()->zonePositionMode() == WeaponAreaEffectPrm::ZONE_POS_MOUSE && !init){
		Vect2f dir = owner()->player()->shootPoint2D();
		dir -= owner()->position2D();
		dir.normalize(clamp(dir.norm(), fireRadiusMin(), fireRadius()));
		dir += owner()->position2D();
		Vect3f shootPoint = To3D(dir);
		for(int i = 0; i < sourceControllers_.size(); i++)
			sourceControllers_[i].setPose(Se3f(sourcesOrientation(), shootPoint));
	}
	else
	{
		if(init){
			for(int i = 0; i < sourceControllers_.size(); i++)
				sourceControllers_[i].setPose(Se3f(sourcesOrientation(), firePosition() + fireDispersion()));
		}
	}

	if(weaponPrm()->fowRadius()){
		Vect3f pos = (weaponPrm()->zonePositionMode() == WeaponAreaEffectPrm::ZONE_POS_OWNER) ? owner()->position() : (firePosition() + fireDispersion());

		if(owner()->player()->fogOfWarMap())
			owner()->player()->fogOfWarMap()->moveVisibleOuterRadius(fowHandle_, pos.x, pos.y, weaponPrm()->fowRadius());
	}
}

void WeaponAreaEffect::releaseSources()
{
	std::for_each(sourceControllers_.begin(), sourceControllers_.end(),
		std::mem_fun_ref(&WeaponSourceController::release));

	if(fowHandle_ != -1){
		if(owner()->player()->fogOfWarMap()){
			owner()->player()->fogOfWarMap()->deleteVisible(fowHandle_);
			fowHandle_ = -1;
		}
	}
}

void WeaponAreaEffect::serialize(Archive& ar)
{
	__super::serialize(ar);

	if(universe()->userSave()){
		ar.serialize(sourceControllers_, "sourceControllers", 0);
		bool fowHandleInited(fowHandle_ >= 0);
		ar.serialize(fowHandleInited, "fowHandleInited", 0);
		if(ar.isInput() && fowHandleInited)
			fowHandle_ = owner()->player()->fogOfWarMap()->createVisible();
	}
}

// --------------------------------------------------------------------------

WeaponPad::WeaponPad(WeaponSlot* slot, const WeaponPrm* prm) : WeaponBase(slot, prm)
, toolser_(weaponPrm()->toolser())
{
	fowHandle_ = -1;
	squadPicked_ = 0;

	prevPos_ = Vect3f::ZERO;
	digLenght_ = 0;
	started_ = false;
}

WeaponPad::~WeaponPad()
{
	xassert(fowHandle_ == -1);
}

bool WeaponPad::canFireMinTime(RequestResourceType triggerAction) const
{
	UnitPlayer* up = owner()->player()->playerUnit();
	if(isLoading() || (up && up->waitPadRespawn()))
		return false;
	
	return __super::canFireMinTime(triggerAction);
}

UI_MarkObjectModeID WeaponPad::analyseMarkObject(UnitInterface* ui) const 
{ 
	if(ui){
		const UnitObjective* unit = safe_cast<const UnitObjective*>(ui->getUnitReal());
		if(unit->attr().isResourceItem() && weaponPrm()->canPickItem(unit))
			return UI_MARK_CAN_PICK_RESOURCE;
		else if(unit->player() == owner()->player() && weaponPrm()->canPickUnit(unit))
			return UI_MARK_CAN_PICK_UNIT;
		else if(owner()->isEnemy(ui) && canAttack(WeaponTarget(ui, weaponPrm()->ID())))
			return UI_MARK_CAN_ATTACK_ENEMY;
	}
	return UI_MARK_NONE;
}

UnitPad* WeaponPad::getPad(UnitPlayer* &up) const
{
	up = owner()->player()->playerUnit();
	if(!up)
		return 0;
	return up->getPad(weaponPrm()->ID());
}

UnitPad* WeaponPad::updatePad(UnitPlayer* &up)
{
	owner()->player()->setUpdateShootPoint();

	UnitPad* pad = getPad(up);

	if(!pad && isLoaded())
		if(owner()->preSelectedWeaponID() == weaponPrm()->ID() && owner()->player()->shootPointValid())
			if(pad = up->createPad(weaponPrm()->ID(), weaponPrm()->attrPad()))
				if(fowHandle_ == -1 && weaponPrm()->fowRadius() && owner()->player()->fogOfWarMap())
					fowHandle_ = owner()->player()->fogOfWarMap()->createVisible();

	if(pad){
		//xassert(owner()->player()->shootPointValid());
		Vect3f shootPoint = To3D(owner()->player()->shootPoint2D());
		shootPoint.z += 5.f;

		pad->rigidBody()->setPose(Se3f(pad->orientation(), shootPoint));
		pad->rigidBody()->awake();

		if(fowHandle_ >= 0)
			owner()->player()->fogOfWarMap()->moveVisibleOuterRadius(fowHandle_, shootPoint.xi(), shootPoint.yi(), weaponPrm()->fowRadius() / FogOfWar::step);	

		return pad;
	}

	return 0;
}

bool WeaponPad::fire(const WeaponTarget& target)
{
	xassert(!isFiring() && owner()->alive());

	UnitPlayer* up;
	if(UnitPad* pad = updatePad(up)){
		bool targetUnit = false;
		UnitInterface* trg = target.unit();
		if(trg && trg != pad && trg != owner()){
			if(trg->attr().isResourceItem() && weaponPrm()->canPickItem(safe_cast<const UnitObjective*>(trg))){
				targetUnit = true;
				pad->applyPickedItem(trg);
			}
			if(!targetUnit && trg->player() == owner()->player() && weaponPrm()->canPickUnit(safe_cast<const UnitObjective*>(trg->getUnitReal()))){
				squadPicked_ = safe_cast<UnitSquad*>(trg->getSquadPoint());
				int n = squadPicked_->units().size();
				xxassert(n <= pad->attr().nodes.size(), "У лапы слишком мало узлов для линковки");
				for(int i = 0; i < n; ++i)
					squadPicked_->units()[i]->attachToDock(pad, pad->attr().nodes[i], true);
				targetUnit = true;
				pad->startState(StatePadCarry::instance());
			}
			if(!targetUnit && owner()->isEnemy(trg) && canAttack(WeaponTarget(trg, weaponPrm()->ID()))){
				if(prmCache().abnormalState().isEnabled())
					trg->setAbnormalState(prmCache().abnormalState(), owner());
				applyDamage(trg);
				targetUnit = true;
			}
		}

		if(!targetUnit && weaponPrm()->type()){
			if(vMap.getTerrainType(pad->position().xi(), pad->position().yi()) & weaponPrm()->surfaceType() == 0)
				return false;

			prevPos_ = pad->position();
			digLenght_ = 0.f;
			float capacity = owner()->attr().isPlayer() ? safe_cast<UnitPlayer*>(owner())->currentCapacity() : 0.5f;

			if(weaponPrm()->type() == WeaponPadPrm::DIG_IN)
				if(capacity < 1.f)
					pad->startState(StatePadGet::instance());
				else
					return false;
			else
				if(capacity > 0.f)
					pad->startState(StatePadPut::instance());
				else
					return false;
			
			started_ = false;
			toolser_.stop();
			
			return true;
		}
		return targetUnit;
	}
	return false;
}

float WeaponPad::changeCapacity(float value)
{
	if(owner()->attr().isPlayer()){
		float cur = clamp(safe_cast<UnitPlayer*>(owner())->currentCapacity() + value, 0.f, 1.f);
		safe_cast<UnitPlayer*>(owner())->setCurrentCapacity(cur);
		return cur;
	}
	return 0.f;
}

void WeaponPad::deselect()
{
	fireEnd();
	owner()->fireStop();
	if(owner()->player() == UI_LogicDispatcher::instance().player())
		UI_LogicDispatcher::instance().clearClickMode();
}

void WeaponPad::quant()
{
	start_timer_auto();

	__super::quant();

	if(owner()->preSelectedWeaponID() != weaponPrm()->ID()){
		fireEnd();
		return;
	}

	UnitPlayer* up;
	UnitPad* pad = updatePad(up);
	if(!pad)
		return;

	if(!isFiring() || squadPicked_)
		return;

	Vect3f prePos = prevPos_;
	prevPos_ = pad->position();

	if(vMap.getTerrainType(pad->position().xi(), pad->position().yi()) & weaponPrm()->surfaceType() == 0){
		fireEnd();
		return;
	}

	Vect3f curPos = pad->position();
	float digDist = prePos.distance(curPos);
	if(weaponPrm()->type() == WeaponPadPrm::DIG_IN){
		if(digDist < 5.f){
			prevPos_ = prePos;
			return;
		}
	}
	else  if(digDist < weaponPrm()->once() * 0.66f){ //POUR_OUT
		float dispersion = logicRNDfabsRndInterval(weaponPrm()->once(), weaponPrm()->once() * 1.5f);
		float angle = logicRNDfabsRnd(2.f * M_PI);
		curPos.x += dispersion * cosf(angle);
		curPos.y += dispersion * sinf(angle);
	}

	Vect3f dir;
	dir.sub(curPos, prePos); //.normalize();
	float angle = -atan2f(dir.x, dir.y);
	QuatF rot(angle, Vect3f::K, false);

	if(!started_){
		started_ = true;
		toolser_.start(Se3f(rot, prePos));
	}
	toolser_.setPosition(Se3f(rot, curPos));

	if(weaponPrm()->type() == WeaponPadPrm::DIG_IN){
		digLenght_ += digDist;
		float part = digDist / weaponPrm()->power();
		part = changeCapacity(part);
		if(part >= 1.f - FLT_EPS)
			if(weaponPrm()->deselectByPower())
				deselect();
			else
				fireEnd();
		else if(digLenght_ > weaponPrm()->once())
			fireEnd();
	}
	else {
		xassert(weaponPrm()->type() == WeaponPadPrm::POUR_OUT);
		float part = logicPeriodSeconds / weaponPrm()->power();
		part = changeCapacity(-part);
		if(part < FLT_EPS)
			if(weaponPrm()->deselectByPower())
				deselect();
			else
				fireEnd();
	}
}

void WeaponPad::fireEnd()
{
	__super::fireEnd();

	UnitPlayer* up;
	UnitPad* pad = getPad(up);

	if(!pad)
		return;

	if(squadPicked_){ 	// отпускаем юнита
		pad->finishState(StatePadCarry::instance());
		LegionariesLinks::const_iterator li;
		FOR_EACH(squadPicked_->units(), li)
			(*li)->detachFromDock();
		squadPicked_ = 0;
	}
	else if(isFiring()){
		switchOff();
		
		if(weaponPrm()->type() == WeaponPadPrm::DIG_IN)
			pad->finishState(StatePadPut::instance());
		else
			pad->finishState(StatePadGet::instance());
	}

	if(owner()->preSelectedWeaponID() != weaponPrm()->ID()){
		up->destroyPad(weaponPrm()->ID());
		if(fowHandle_ != -1){
			owner()->player()->fogOfWarMap()->deleteVisible(fowHandle_);
			fowHandle_ = -1;
		}
	}
}

void WeaponPad::serialize(Archive& ar)
{
	__super::serialize(ar);

	if(universe()->userSave()){
		bool fowHandleInited = fowHandle_ >= 0;
		ar.serialize(fowHandleInited, "fowHandleInited", 0);
		if(ar.isInput() && fowHandleInited)
			fowHandle_ = owner()->player()->fogOfWarMap()->createVisible();
	}
}

//-------------------------------------------------------

WeaponGrip::WeaponGrip(WeaponSlot* slot, const WeaponPrm* prm) : WeaponBase(slot, prm)
{
}

void WeaponGrip::quant()
{
	__super::quant();

	if(state() == WeaponBase::WEAPON_STATE_FIRE){
		if(fireTarget()){
			if(prmCache().abnormalState().isEnabled())
				fireTarget()->setAbnormalState(prmCache().abnormalState(), owner());

			applyDamage(fireTarget());
		}
	}

	if(UnitActing* p = dynamic_cast<UnitActing*>(fireTarget())){
		if(gripTimer.time() > weaponPrm()->startGripTime() && !p->isDocked()){
			p->attachToDock(owner(), aimControllerPrm().nodeLogic(), true, true);
			p->startState(StateWeaponGrip::instance());
		}
		if(gripTimer.time() > weaponPrm()->finishGripTime()){
			finishGrip(p);
			return;
		}
	} else {
		gripTimer.stop();
	}
}

bool WeaponGrip::fire(const WeaponTarget& target)
{
	xassert(owner());
	
	UnitReal* p = dynamic_cast<UnitActing*>(target.unit());

	if(p && !p->isDocked()){
		gripTimer.start();
		return true;
	}
	else {
		clearFireTarget();
		return false;
	}
}

void WeaponGrip::fireEnd()
{
	__super::fireEnd();
	if(UnitActing* p = dynamic_cast<UnitActing*>(fireTarget()))
		finishGrip(p);
}

void WeaponGrip::finishGrip(UnitActing* target)
{
	if(target->isDocked()){
		QuatF orientation(weaponPrm()->forwardAngle() + logicRNDfrnd(G2R(weaponPrm()->dispersionAngle())), Vect3f::K);
		orientation.premult(owner()->orientation());
		Vect3f dir(Vect3f::J);
		orientation.xform(dir);
		float pulse(sqrtf(target->rigidBody()->core()->mass()));
		dir.x *= weaponPrm()->horizontalImpulse() * pulse;
		dir.y *= weaponPrm()->horizontalImpulse() * pulse;
		dir.z = weaponPrm()->verticalImpulse() * pulse;
		target->finishState(StateWeaponGrip::instance());
		target->rigidBody()->addImpulseLinear(dir);
		target->rigidBody()->avoidCollisionAtStart();
		target->rigidBody()->enableBoxMode();
		target->rigidBody()->setVelocity(Vect3f::ZERO);
	}
	gripTimer.stop();
}

//-------------------------------------------------------

WeaponTarget::WeaponTarget(UnitInterface* unit, int weapon_id) : unit_(unit),
	position_(0,0,0)
{
	if(unit)
		unit->computeTargetPosition(position_);

	terrainAttackClass_ = -1;

	weaponID_ = weapon_id;
}

WeaponTarget::WeaponTarget(const Vect3f& position, int weapon_id) : unit_(0),
	position_(position)
{
	terrainAttackClass_ = -1;
	updateTerrainAttackClass();

	weaponID_ = weapon_id;
}

WeaponTarget::WeaponTarget(UnitInterface* unit, const Vect3f& position, int weapon_id) :
	unit_(unit),
	position_(position)
{
	terrainAttackClass_ = -1;
	
	if(unit)
		unit->computeTargetPosition(position_);
	else
        updateTerrainAttackClass();

	weaponID_ = weapon_id;
}

void WeaponTarget::updateTerrainAttackClass()
{
	if(position_.x >= 0 && position_.x < vMap.H_SIZE && position_.y >= 0 && position_.y < vMap.V_SIZE){
		if(environment->water() && environment->water()->isWater(position_)){
			if(environment->temperature() && environment->temperature()->isOnIce(position_))
				terrainAttackClass_ = ATTACK_CLASS_ICE;
			else {
				if(environment->water()->isFullWater(position_))
					terrainAttackClass_ = ATTACK_CLASS_WATER;
				else
					terrainAttackClass_ = ATTACK_CLASS_WATER_LOW;
			}
		}
		else {
			terrainAttackClass_ = vMap.isIndestructability(position_.xi(), position_.yi()) ?
				ATTACK_CLASS_TERRAIN_HARD : ATTACK_CLASS_TERRAIN_SOFT;
		}
	}
	else
		terrainAttackClass_ = ATTACK_CLASS_TERRAIN_SOFT;
}

int WeaponTarget::attackClass() const
{
	if(unit_)
		return unit_->unitAttackClass();

	return terrainAttackClass();
}

// ------------------------------

void WeaponSourcePrm::serialize(Archive& ar)
{
	ar.serialize(damage_, "damage", 0);
	ar.serialize(abnormalState_, "abnormalState", 0);
}

const SourceAttribute* WeaponSourcePrm::getSourceByKey(const char* key) const
{
	xassert(attr_);
	return attr_->getByKey(key);
}

void WeaponSourcePrm::applyArithmetics(const ArithmeticsData& arithmetics)
{
	if(arithmetics.address & ArithmeticsData::DAMAGE){
		arithmetics.apply(damage_);
		abnormalState_.applyParameterArithmetics(arithmetics);
	}
}

void WeaponSourcePrm::setSource(const SourceWeaponAttribute& attr)
{
	attr_ = &attr;
}

// ------------------------------
WeaponPrmCache::WeaponPrmCache()
{
	weaponPrm_ = 0;
}

void WeaponPrmCache::serialize(Archive& ar)
{
	ar.serialize(damage_, "damage", 0);
	ar.serialize(fireCost_, "fireCost", 0);
	ar.serialize(parameters_, "parameters", 0);
	ar.serialize(abnormalState_, "abnormalState", 0);
	ar.serialize(sources_, "sources", 0);
}

void WeaponPrmCache::set(const WeaponPrm* prm)
{
	damage_ = prm->damage();
	fireCost_ = prm->fireCost();
	parameters_ = prm->parameters();
	abnormalState_ = prm->abnormalState();
	weaponPrm_ = prm;
}

void WeaponPrmCache::applyArithmetics(const ArithmeticsData& arithmetics)
{
	if(arithmetics.checkWeapon(weaponPrm_)){
		if(arithmetics.address & ArithmeticsData::WEAPON)
			arithmetics.apply(parameters_);

		if(arithmetics.address & ArithmeticsData::DAMAGE)
			arithmetics.apply(damage_);

		abnormalState_.applyParameterArithmetics(arithmetics);

		if(arithmetics.address & ArithmeticsData::SOURCE){
			WeaponSourcePrms::iterator it;
			FOR_EACH(sources_, it)
				it->applyArithmetics(arithmetics);
		}
	}
}

bool WeaponPrmCache::checkDamage(const ParameterSet& target_parameters) const
{
	return false;
}

void WeaponPrmCache::setSources(const SourceWeaponAttributes& source_attributes)
{
	sources_.resize(source_attributes.size());
	for(int i = 0; i < source_attributes.size(); i++){
		if(source_attributes[i].source()){
			source_attributes[i].source()->getParameters(sources_[i]);
			sources_[i].setSource(source_attributes[i]);
		}
	}
}

//-------------------------------------------------------

WeaponWaitingSource::WeaponWaitingSource(WeaponSlot* slot, const WeaponPrm* prm) : WeaponBase(slot, prm)
{
	mineCount_ = 0;
}

WeaponWaitingSource::~WeaponWaitingSource()
{
}

void WeaponWaitingSource::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(sourceCoordinats_, "sourceCoordinats", 0);
	updateCount();
}

bool WeaponWaitingSource::init(const WeaponBase* old_weapon)
{
	if(!__super::init(old_weapon))
		return false;

	if(old_weapon){
		// список зарядов при апгрейде нужно сохранить
		if(old_weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE){
			if(weaponPrm()->type() == WeaponWaitingSourcePrm::MINING){
				sourceCoordinats_ = safe_cast<const WeaponWaitingSource*>(old_weapon)->sourceCoordinats_;
				updateCount();
			}
		}
	}

	return true;
}

bool WeaponWaitingSource::checkFinalCost() const
{
	if(weaponPrm()->type() == WeaponWaitingSourcePrm::MINING)
		if(weaponPrm()->miningLimit() > 0 && mineCount() >= weaponPrm()->miningLimit())
			return false;
	return true;
}

bool WeaponWaitingSource::fire(const WeaponTarget& target)
{
	if(weaponPrm()->type() == WeaponWaitingSourcePrm::MINING){
		if(weaponPrm()->miningLimit() > 0 && sourceCoordinats_.size() >= weaponPrm()->miningLimit())
			return false;
		if(SourceBase *source = sourceManager->createSource(&weaponPrm()->source(), Se3f(owner()->orientation(), firePosition()))){
			source->setAffectMode(affectMode());
			source->setPlayer(owner()->player());
			source->setActivationTime(source->getActivationTime() + weaponPrm()->delayTime() * sourceCoordinats_.size() * 1000);
			sourceCoordinats_.push_back(firePosition());
			updateCount();
		}
	}
	else{
		owner()->detonateMines();
		switchOff();
	}

	return true;
}

bool WeaponWaitingSource::canDetonate() const
{
	return mineCount() > 0 && (weaponPrm()->miningLimit() <= 0 || mineCount() == weaponPrm()->miningLimit());
}

void WeaponWaitingSource::detonate()
{
	if(weaponPrm()->type() == WeaponWaitingSourcePrm::MINING){
		SourceCoordinats::iterator it;
		FOR_EACH(sourceCoordinats_, it){
			const AttributeBase* detonatorRef = AuxAttributeReference(AUX_ATTRIBUTE_DETONATOR);
			UnitBase* detonator = owner()->player()->buildUnit(detonatorRef);
			detonator->setCollisionGroup(COLLISION_GROUP_COLLIDER);
			detonator->setUnitAttackClass(ATTACK_CLASS_ALL);
			detonator->setPose(Se3f(QuatF::ID, *it), true);
			detonator->Kill();
		}
		sourceCoordinats_.clear();
		updateCount();
	}
}

void UnitActing::detonateMines()
{
	WeaponSlots::iterator it;
	FOR_EACH(weaponSlots_, it)
		if(it->weapon()->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE)
			safe_cast<WeaponWaitingSource*>(it->weapon())->detonate();
}

bool UnitActing::canDetonateMines()
{
	WeaponSlots::const_iterator it;
	FOR_EACH(weaponSlots_, it)
		if(it->weapon()->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE)
			if(safe_cast<WeaponWaitingSource*>(it->weapon())->canDetonate())
				return true;
	return false;
}


int UnitActing::queuredMines() const
{
	MTL();
	int mininInQueure = 0;
	if(selectedWeaponID_ && specialFireTargetExist() && !fireDistanceCheck())
		if(const WeaponBase* weapon = findWeapon(selectedWeaponID_))
			if(weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE
				&& safe_cast<const WeaponWaitingSource*>(weapon)->weaponPrm()->type() == WeaponWaitingSourcePrm::MINING)
			mininInQueure = 1;

	CommandList::const_iterator cmd;
	const CommandList& lst = attr().isLegionary() ? safe_cast<const UnitLegionary*>(this)->squad()->suspendCommandList() : suspendCommandList();
	FOR_EACH(lst, cmd)
		if(cmd->commandID() == COMMAND_ID_ATTACK)
			if(const WeaponBase* weapon = findWeapon(cmd->commandData()))
				if(weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE
					&& safe_cast<const WeaponWaitingSource*>(weapon)->weaponPrm()->type() == WeaponWaitingSourcePrm::MINING)
					++mininInQueure;
	return mininInQueure;
}

bool UnitActing::canSuspendCommand(const UnitCommand& command) const
{
	switch(command.commandID()){
	case COMMAND_ID_ATTACK:	
		if(const WeaponBase* weapon = findWeapon(command.commandData())){
			if(weapon->weaponPrm()->weaponClass() == WeaponPrm::WEAPON_WAITING_SOURCE){
				const WeaponWaitingSource* wp = safe_cast<const WeaponWaitingSource*>(weapon);
				return wp->weaponPrm()->type() != WeaponWaitingSourcePrm::MINING
					|| wp->weaponPrm()->miningLimit() <= 0
					|| wp->mineCount() + queuredMines() < wp->weaponPrm()->miningLimit();
			}
		}
		else
			return false;

	}
	return true;
}

//-------------------------------------------------------

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponAreaEffectPrm, ZonePositionMode, "WeaponAreaEffectPrm::ZonePositionMode")
REGISTER_ENUM_ENCLOSED(WeaponAreaEffectPrm, ZONE_POS_OWNER, "вокруг себя")
REGISTER_ENUM_ENCLOSED(WeaponAreaEffectPrm, ZONE_POS_TARGET, "вокруг цели")
REGISTER_ENUM_ENCLOSED(WeaponAreaEffectPrm, ZONE_POS_MOUSE, "привязать к мыши")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponAreaEffectPrm, ZonePositionMode)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponWaitingSourcePrm, ExecuteType, "WeaponWaitingSourcePrm::ExecuteType")
REGISTER_ENUM_ENCLOSED(WeaponWaitingSourcePrm, MINING, "минирование")
REGISTER_ENUM_ENCLOSED(WeaponWaitingSourcePrm, DETONATE, "детонация")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponWaitingSourcePrm, ExecuteType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponProjectilePrm, MissileLaunchType, "WeaponPprojectilePrm::MissileLaunchType")
REGISTER_ENUM_ENCLOSED(WeaponProjectilePrm, LAUNCH_SHOT_BEGIN, "В начале выстрела")
REGISTER_ENUM_ENCLOSED(WeaponProjectilePrm, LAUNCH_SHOT_END, "В конце выстрела")
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponProjectilePrm, MissileLaunchType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponPadPrm, WorkType, "WeaponBase::WeaponState")
REGISTER_ENUM_ENCLOSED(WeaponPadPrm, NONE, "ничего");
REGISTER_ENUM_ENCLOSED(WeaponPadPrm, DIG_IN, "копаем");
REGISTER_ENUM_ENCLOSED(WeaponPadPrm, POUR_OUT, "насыпаем");
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponPadPrm, WorkType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(WeaponBase, WeaponState, "WeaponBase::WeaponState")
REGISTER_ENUM_ENCLOSED(WeaponBase, WEAPON_STATE_LOAD, "зарядка");
REGISTER_ENUM_ENCLOSED(WeaponBase, WEAPON_STATE_FIRE, "стрельба");
END_ENUM_DESCRIPTOR_ENCLOSED(WeaponBase, WeaponState)

REGISTER_CLASS(WeaponBase, WeaponProjectile, "оружие, стреляющее снарядами");
REGISTER_CLASS(WeaponBase, WeaponBeam, "лучевое оружие");
REGISTER_CLASS(WeaponBase, WeaponAreaEffect, "действующее на зону оружие");
REGISTER_CLASS(WeaponBase, WeaponPad, "Лапа");
REGISTER_CLASS(WeaponBase, WeaponWaitingSource, "зона с отложенной активацией");

REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_PROJECTILE, WeaponProjectile);
REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_BEAM, WeaponBeam);
REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_AREA_EFFECT, WeaponAreaEffect);
REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_PAD, WeaponPad);
REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_WAITING_SOURCE, WeaponWaitingSource);
REGISTER_CLASS_IN_FACTORY(WeaponFactory, WeaponPrm::WEAPON_GRIP, WeaponGrip);

REGISTER_CLASS(WeaponPrm, WeaponBeamPrm, "лучевое оружие");
REGISTER_CLASS(WeaponPrm, WeaponProjectilePrm, "стреляющее снарядами оружие");
REGISTER_CLASS(WeaponPrm, WeaponAreaEffectPrm, "действующее на зону оружие");
REGISTER_CLASS(WeaponPrm, WeaponPadPrm, "лапа");
REGISTER_CLASS(WeaponPrm, WeaponWaitingSourcePrm, "зоны с отложенной активацией");
REGISTER_CLASS(WeaponPrm, WeaponGripPrm, "оружие - захват");

WRAP_LIBRARY(WeaponPrmLibrary, "WeaponLibrary", "Оружие", "Scripts\\Content\\WeaponLibrary", 0, LIBRARY_EDITABLE);
