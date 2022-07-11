#ifndef __WEAPON_H__
#define __WEAPON_H__

#include "Timers.h"
#include "Parameters.h"
#include "UnitLink.h"
#include "AbnormalStateAttribute.h"
#include "WeaponTarget.h"

class UnitInterface;
class UnitReal;
class UnitActing;

class SourceWeaponAttribute;
class WeaponPrm;
class WeaponSlotAttribute;
class WeaponRotationController;
class WeaponBase;

class WeaponSourcePrm
{
public:
	WeaponSourcePrm() : /*activationDelay_(0.f), lifeTime_(0.f), */attr_(0) {}
	void serialize(Archive& ar);

	void applyArithmetics(const ArithmeticsData& arithmetics);

	const ParameterCustom& damage() const { return damage_; }
	void setDamage(const ParameterCustom& damage){ damage_ = damage; }

	const AbnormalStateAttribute& abnormalState() const { return abnormalState_; }
	void setAbnormalState(const AbnormalStateAttribute& attr){ abnormalState_ = attr; }

	void setSource(const SourceWeaponAttribute& attr);

	float activationDelay() const { return attr_->activationDelay(); }  
	float lifeTime() const { return attr_->lifeTime(); }

	bool checkKey(const char* key) const { return attr_->key() == key; }
	const SourceAttribute* getSourceByKey(const char* key) const;

private:

	ParameterCustom damage_;
	AbnormalStateAttribute abnormalState_;
	const SourceWeaponAttribute* attr_;
};

typedef std::vector<WeaponSourcePrm> WeaponSourcePrms;

class WeaponPrmCache
{
public:
	WeaponPrmCache();

	void serialize(Archive& ar);

	void set(const WeaponPrm* prm);

	const AbnormalStateAttribute& abnormalState() const { return abnormalState_; }
	void setAbnormalState(const AbnormalStateAttribute& state){ abnormalState_ = state; }

	void applyArithmetics(const ArithmeticsData& arithmetics);

	const WeaponDamage& damage() const { return damage_; }
	void setDamage(const WeaponDamage& damage){ damage_ = damage; }

	/// возвращает true, если оружие может нанести повреждения параметрам target_parameters
	bool checkDamage(const ParameterSet& target_parameters) const;

	const ParameterSet& fireCost() const { return fireCost_; }
	void setFireCost(const ParameterCustom& fire_cost){ fireCost_ = fire_cost; }

	const ParameterSet& parameters() const { return parameters_; }
	void setParameters(const ParameterSet& parameters){ parameters_ = parameters; }
	void setParameter(float value, ParameterType::Type type){ parameters_.set(value, type); }

	const WeaponSourcePrms& sources() const { return sources_; }
	void setSources(const SourceWeaponAttributes& source_attributes);

	bool getParametersForUI(const wchar_t* &name, wchar_t type, ParameterSet& out) const;

private:
	/// наносимые повреждения
	WeaponDamage damage_;

	/// стоимость стрельбы
	ParameterSet fireCost_;

	/// параметры оружия - дальность стрельбы, разброс и т.п.
	ParameterSet parameters_;

	/// воздействие на цель
	AbnormalStateAttribute abnormalState_;

	WeaponSourcePrms sources_;

	const WeaponPrm* weaponPrm_;
};

// -------------------------------------------------------

class WeaponAimAngleController
{
public:

	/// приоритеты поворотов
	enum RotationPriority {
		/// возврат в состояние по-умолчанию
		ROTATION_DEFAULT = 0,
		/// прицеливание
		ROTATION_AIM,
		/// стрельба
		ROTATION_FIRE,
		/// сброс
		ROTATION_RESET
	};

	WeaponAimAngleController(eAxis axis) : rotationAxis_(axis)
	{
		value_ = valuePrev_ = valueDefault_ = 0.0f;
		speed_ = M_PI * 0.2f;
		speed2_ = M_PI * 0.2f;
		valuePrecision_ = M_PI * 0.001f;

		parentValue_ = 0.f;

		nodeInterpolator_ = Se3f::ID;
		offset_ = offsetLogic_ = Se3f::ID;

		nodeIndex_ = 0;
		nodeLogicIndex_ = 0;

		valueMin_ = 0.0f;
		valueMax_ = M_PI * 2.0f;

		rotateByLogic_ = false;
	}

	bool operator == (float val) const { return (fabs(getDeltaAngle(val,valuePrev_)) <= valuePrecision_); }
	bool operator != (float val) const { return !(*this == val); }

	bool init(UnitActing* owner, const WeaponAimAnglePrm& prm);
	void reset(UnitActing* owner, WeaponRotationController& controller);

	void serialize(Archive& ar);

	bool change(float target_angle, bool use_normal_speed, WeaponRotationController& controller, int change_priority = ROTATION_AIM);
	bool changeToDefault(bool use_normal_speed, WeaponRotationController& controller);
	void randomizeDefaultValue();
	bool updateValue(const WeaponRotationController& controller);
	void updateLogic(UnitActing* owner) const;

	void setParentValue(float val){ parentValue_ = val; }

	void interpolationQuant(UnitActing* owner);

	void showDebugInfo(const UnitActing* owner) const;

	float operator()() const { return value_ + parentValue_; }

	int nodeIndex() const { return nodeIndex_; }
	int nodeLogicIndex() const { return nodeLogicIndex_; }

	Rangef valueRange() const { return Rangef(valueMin_, valueMax_); }
	float valueDefault() const { return valueDefault_; }

	Se3f nodeLogicPose(bool with_offset = true) const;
	Se3f nodePose(UnitActing* owner) const;

private:

	float value_;
	float valuePrev_;
	float valueMin_;
	float valueMax_;

	float parentValue_;

	float valuePrecision_;

	float valueDefault_;

	float speed_;
	float speed2_;

	/// индекс графического узла
	int nodeIndex_;
	/// индекс логического узла
	int nodeLogicIndex_;

	/// поворачивать граф. объект по осям логического
	bool rotateByLogic_;
	eAxis rotationAxis_;

	InterpolatorNodeTransform nodeInterpolator_;

	/// Корректирующие оси смещения: invert(offset)*rotation*offset
	Se3f offset_; 
	Se3f offsetLogic_;

	/// возвращает значения в интервале \a [-speed_, speed_] или \a [-speed2_, speed2_]
	float getDelta(float target_angle, bool use_normal_speed = true) const
	{
		float delta = getDeltaAngle(target_angle, value_ + parentValue_);

		float turn_speed = use_normal_speed ? speed_ : speed2_;

		if(delta < 0){
			if(delta < -turn_speed)
				delta = -turn_speed;
		}
		else {
			if(delta > turn_speed)
				delta = turn_speed;
		}

		float val = calcAngle(value_ + delta);
		delta = getDeltaAngle(val, value_);

		return delta;
	}

	float calcAngle(float angle) const
	{
		angle = cycleAngle(angle);

		float range_size2 = (valueMax_ - valueMin_)/2.f;
		float range_center = cycleAngle((valueMin_ + valueMax_)/2.f);

		float angle_delta = getDeltaAngle(angle, range_center);
		if(angle_delta < -range_size2)
			angle = valueMin_;
		if(angle_delta > range_size2)
			angle = valueMax_;

		return angle;
	}
};

/// Управление наведением оружия.
class WeaponSlot
{
public:
	WeaponSlot();
	~WeaponSlot();

	int ID() const { return ID_; }
	void setID(int id){ ID_ = id; }

	bool isEmpty() const;
	UnitActing* owner() const { xassert(owner_); return owner_; }
	const WeaponSlotAttribute* attribute() const { return attribute_; }
	WeaponBase* weapon() const { return weapon_; }

	bool isAimEnabled() const { return isAimEnabled_; }
	bool isAimCorrectionEnabled() const { return isAimCorrectionEnabled_; }

	bool init(UnitActing* owner, const WeaponSlotAttribute* attribute);

	bool createWeapon(const WeaponPrm* prm = 0);
	bool upgradeWeapon();

	int nodeEffectIndex(int barrel_index = 0) const;
	int nodeEffectIndexGraphic(int barrel_index = 0) const;

	void updateLogic() const { psi_.updateLogic(owner()); theta_.updateLogic(owner()); }
	void updateAngles();

	void interpolationQuant(){ psi_.interpolationQuant(owner()); theta_.interpolationQuant(owner()); }

	bool aim(float psi, float theta, bool aim_only = false);

	const WeaponAimAngleController& controllerPsi() const { return psi_; }
	const WeaponAimAngleController& controllerTheta() const { return theta_; }

	bool isAimed(float psi, float theta) const { return (psi_ == psi && theta_ == theta); }
	bool isAimedDefault(){ return !isAimEnabled_ || isAimed(psi_.valueDefault(), theta_.valueDefault()); }

	bool aimUpdate();

	bool aimDefault();
	void aimReset();
	void randomizeDefaultValue(){ psi_.randomizeDefaultValue(); }

	/// координаты "дульного среза" оружия
	/// откуда вылетают снаряды, бъют лучи и т.п.
	Se3f muzzlePosition(int barrel_index = 0) const;
	Se3f aimOrientation() const { return psi_.nodeLogicPose(false) * theta_.nodeLogicPose(false); }

	float psi() const { return psi_(); }
	float theta() const { return theta_(); }

	const Vect3f& rotationCenterPsi() const;
	const Vect3f& rotationCenterTheta() const;

	bool setParentSlot(const WeaponSlot* parent);

	void showDebugInfo() const;

	void kill();

	void serialize(Archive& ar);

private:

	int ID_;

	bool isAimEnabled_;
	bool isAimCorrectionEnabled_;

	WeaponAimAngleController psi_;
	WeaponAimAngleController theta_;

	const WeaponSlot* parentPsi_;
	const WeaponSlot* parentTheta_;

	WeaponBase* weapon_;
	mutable UnitActing* owner_;

	const WeaponSlotAttribute* attribute_;
};

/// Оружие - базовый класс.
class WeaponBase
{
public:
	WeaponBase(WeaponSlot* slot, const WeaponPrm* prm = 0);

	virtual ~WeaponBase();

	bool operator > (const WeaponBase& rhs) const
	{
		if(*attribute()->groupType() > *rhs.attribute()->groupType())
			return true;
		else if(*attribute()->groupType() < *rhs.attribute()->groupType())
			return false;

		return attribute()->priority() > rhs.attribute()->priority();
	}

	virtual bool init(const WeaponBase* old_weapon = 0);

	void preQuant();
	virtual void quant();
	void endQuant();

	virtual void moveQuant();

	void interpolationQuant();

	virtual void showInfo(const Vect2f& pos) const;
	virtual void showDebugInfo();
	virtual bool checkFinalCost() const { return true; }

	bool isShortRange() const;
	bool isLongRange() const;

	/// является ли наступательным
	virtual bool isOffensive() const;

	const WeaponGroupType* groupType() const { return attribute()->groupType(); }
	int priority() const { return attribute()->priority(); }

	bool canAutoFire() const;
	void toggleAutoFire(bool shoot_once = false);
	bool isAutoFireOn() const { return autoFire_ && !autoFireOnce_; }

	AffectMode affectMode() const;
	WeaponAnimationMode animationMode() const;

	bool canAttack(const WeaponTarget& target, bool quick_check = false) const;
	bool checkAttackClass(int attack_class) const;
	bool fireDistanceCheck(const WeaponTarget& target, bool allow_dispersion = false) const;
	/// проверяет, что повреждения могут быть применены к юниту
	bool checkDamage(const UnitInterface* unit) const;
	bool checkTargetMode(UnitInterface* target) const;
	bool checkTargetMode(const WeaponTarget& target) const;
	bool checkFogOfWar(const WeaponTarget& target) const;
	bool checkMovement() const;
	virtual bool checkTerrain() const { return true; }

	virtual bool fire(const WeaponTarget& target) = 0;
	bool fireRequest(const WeaponTarget& target, bool aim_only = false);
	void fireEvent();

	void reloadStart(int time = -1, bool from_inventory = false);
	bool isLoaded() const;
	bool isLoading() const { return (state_ == WEAPON_STATE_LOAD && !isLoaded()); }
	// хватает ресурсов на выстрел
	bool canFire(RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	// хватает ресурсов на минимальный выстрел
	virtual bool canFireMinTime(RequestResourceType triggerAction = NEED_RESOURCE_SILENT_CHECK) const;
	bool hasAmmo() const;
	bool reloadFromInventory() const { return reloadFromInventory_; }

	bool isEnabled() const { return isEnabled_; }
	void enable(bool state = true){ isEnabled_ = state; }

	bool isAccessible() const { return isAccessible_; }
	void setAccessible(bool state){ isAccessible_ = state; }

	void switchOff();

	void updateLogic(){ if(isEnabled() && weaponSlot()->isAimEnabled()) weaponSlot()->updateLogic(); }

	float chargeLevel() const;

	UnitInterface* fireTarget() const { return fireTarget_; }
	const Vect3f& firePosition() const { return firePosition_; }
	const Vect3f& fireDispersion() const { return fireDispersion_; }

	Vect3f firePositionCurrent() const;

	bool isFiring() const { return state_ == WEAPON_STATE_FIRE; }
	/// возвращает true если в данный момент оружие повёрнуто
	bool isTargeting() const;

	virtual void kill();

	virtual void serialize(Archive& ar);

	/// состояния оружия
	enum WeaponState {
		/// зарядка
		WEAPON_STATE_LOAD,
		/// стрельба
		WEAPON_STATE_FIRE
	};

	WeaponSlot* weaponSlot() const { return weaponSlot_; }
	const WeaponSlotAttribute* attribute() const { return attribute_; }
	const WeaponAimControllerPrm& aimControllerPrm() const { return attribute()->aimControllerPrm(aimControllerPrmIndex_); }
	const WeaponPrm* weaponPrm() const { return weaponPrm_; }

	bool isActive() const { return state() == WEAPON_STATE_FIRE; }

	float fireRadius() const { return fireRadius_; }
	float fireRadiusFull() const { return fireRadius_ + fireDispersionRadius_; }
	float fireRadiusMin() const { return fireRadiusMin_; }
	float fireRadiusOptimal() const { return fireRadiusMin() + (fireRadius() - fireRadiusMin())/5.f; }
	float fireDisp() const { return float(fireDisp_); }
	float fireDispersionRadius() const { return fireDispersionRadius_; }

	int fireTime() const { return fireTime_; }

	const WeaponPrmCache& prmCache() const { return prmCache_; }
	const ParameterSet& parameters() const { return prmCache_.parameters(); }
	void setParameters(const ParameterSet& parameters) { return prmCache_.setParameters(parameters); }
	void setParameter(float value, ParameterType::Type type){ prmCache_.setParameter(value, type); }
	const WeaponDamage& damage() const { return prmCache_.damage(); }
	virtual void applyParameterArithmetics(const ArithmeticsData& arithmetics);

	const WeaponPrm* accessibleUpgrade() const;

	bool aimDefault();
	void aimReset();
	bool aimUpdate();

	UnitInterface* autoTarget() const { return autoTarget_; }
	void setAutoTarget(UnitInterface* target);
	bool autoTargetAttacked() const { return autoTargetAttacked_; }

	UnitActing* owner() const { return owner_; }

	static bool checkDamage(const ParameterSet& damage, const ParameterSet& set, const ParameterSet& set_max);
	static bool checkDamage(const ParameterSet& damage, const ParameterSet& set);

	static WeaponBase* create(WeaponSlot* slot, const WeaponPrm* prm = 0);
	static void release(WeaponBase* p){ delete p; }

	static WeaponPrm::UnitMode unitMode(UnitBase* unit);

	virtual UI_MarkObjectModeID analyseMarkObject(UnitInterface* unit) const { return UI_MARK_NONE; }
protected:

	bool aim(const Vect3f& to, bool aim_only = false);
	bool isAimed(const Vect3f& to) const;
	Vect2f aimAngles(const Vect3f& to) const;

	float aimPsi() const { return weaponSlot()->psi(); }
	float aimTheta() const { return weaponSlot()->theta(); }

	const AttributeBase* turnSuggestPrm() const { return turnSuggestPrm_; }
	void setTurnSuggestPrm(const AttributeBase* prm){ turnSuggestPrm_ = prm; }

	virtual void fireEnd();

	void updateOwner();

	void clearFireTarget(){ fireTarget_ = 0; }
	void setFireTarget(UnitInterface* target){ fireTarget_ = target; }
	void setFirePosition(const Vect3f& pos){ firePosition_ = pos; }
	void setFireDispersion(const Vect3f& disp){ fireDispersion_ = disp; }

	bool isFireTargetSet() const { return fireTargetSet_; }

	WeaponState state() const { return state_; }

	int currentBarrel() const { return currentBarrel_; }

	void startFireEffect();
	void stopFireEffect();

	bool fireAnimationCheck() const;

	ParameterSet fireCost() const;

	void applyDamage(UnitInterface* target);

	void setVisibilityOnShoot();

private:

	/// текущее состояние оружия
	WeaponState state_;
	/// время выполнения состояния
	LogicTimer stateTimer_;

	/// координаты цели
	Vect3f firePosition_;
	/// ошибка прицеливания
	Vect3f fireDispersion_;
	/// цель
	UnitLink<UnitInterface> fireTarget_;
	bool hasFireTarget_;

	/// задержка между выстрелами в очереди
	LogicTimer queueDelayTimer_;

	/// текущий ствол
	int currentBarrel_;

	/// true, если цель установлена
	/// используется для постоянно стреляющего оружия
	bool fireTargetSet_;
	/// true, если оружие стреляет
	bool fireStarted_;
	bool fireRequested_;

	bool autoFire_;
	bool autoFireOnce_;

	/// время стрельбы, мс
	int fireTime_;
	/// время перезарядки, мс
	int reloadTime_;
	/// время перезарядки из инвентаря, мс
	int reloadTimeInventory_;

	/// true если оружие заряжено
	bool reloaded_;
	bool reloadFromInventory_;

	/// дальность стрельбы
	float fireRadius_;
	/// минимальная дальность стрельбы
	float fireRadiusMin_;

	/// точность (максимальный разброс) стрельбы
	int fireDisp_;
	float fireDispersionRadius_;

	bool aimResetDisabled_;
	/// true если оружие надо развернуть по-умолчанию
	LogicTimer aimResetTimer_;
	LogicTimer autoScanTimer_;

	bool isEnabled_;
	bool isAccessible_;

	WeaponSlot* weaponSlot_;

	int aimControllerPrmIndex_;

	UnitActing* owner_;
	const WeaponSlotAttribute* attribute_;
	const WeaponPrm* weaponPrm_;

	UnitLink<UnitInterface> autoTarget_;
	bool autoTargetAttacked_;

	/// используется для расчёта углов при наведении
	const AttributeBase* turnSuggestPrm_;

	WeaponPrmCache prmCache_;

	/// перекладывает значения параметров в переменные класса
	bool updateParameters();

	enum FireDistanceStatus
	{
		FIRE_DISTANCE_OK = 0,
		FIRE_DISTANCE_DISP,
		FIRE_DISTANCE_MIN,
		FIRE_DISTANCE_MAX,
	};

	FireDistanceStatus targetDistanceStatus(const WeaponTarget& target) const;

	bool needAutoFire() const;
	/// устанавливает цель для атаки - точку на поверхности или юнита, стоящего в этой точке
	bool setGroundTarget(const Vect3f& ground_pos);
};

/// Управление поворотом логического узла модели из оружия.
class WeaponRotationNode
{
public:
	WeaponRotationNode(int node_index, eAxis axis, float angle = 0.f) : nodeIndex_(node_index),
		axis_(axis),
		angle_(angle)
	{
		angleDelta_ = 0.f;
		changePriority_ = -1;
	}

	bool operator == (const WeaponRotationNode& node) const { return nodeIndex_ == node.nodeIndex() && axis_ == node.axis(); }

	int nodeIndex() const { return nodeIndex_; }
	eAxis axis() const { return axis_; }

	float angle() const { return angle_; }

	bool setChange(float angle_delta, int priority)
	{
		if(priority > changePriority_){
			angleDelta_ = angle_delta;
			changePriority_ = priority;
			return true;
		}

		return false;
	}

	bool quant()
	{
		angle_ += angleDelta_;

		angleDelta_ = 0.f;
		changePriority_ = -1;

		return true;
	};

private:

	/// индекс логического узла
	int nodeIndex_;
	/// ось вращения
	eAxis axis_;

	/// текущий угол
	float angle_;

	/// изменение угла за текущий логический кадр
	float angleDelta_;
	/// приоритет текущего изменения
	/// изменения с меньшим приоритетом игнорируются
	int changePriority_;
};

/// Управление поворотом логической модели из оружия.
/**
Нужен для корректного управления наведением для случая нескольких
оружий на один дамми. Отслеживает, какой дамми на сколько повёрнут,
разрешает конфликты при попытке повернуть один и тот же дамми в
разные стороны.
*/
class WeaponRotationController
{
public:
	WeaponRotationController(int nodes_count = 0)
	{
		nodes_.reserve(nodes_count);
	}

	bool addNode(const WeaponRotationNode& node)
	{
		RotationNodes::const_iterator it = std::find(nodes_.begin(), 
			nodes_.end(), node);

		if(it != nodes_.end()) return false;

		nodes_.push_back(node);
		return true;
	}

	bool changeAngle(int node_index, eAxis axis, float angle_delta, int change_priority)
	{
		WeaponRotationNode node(node_index, axis);
		RotationNodes::iterator it = std::find(nodes_.begin(), 
			nodes_.end(), node);

		if(it != nodes_.end())
			return it->setChange(angle_delta, change_priority);

		return false;
	}

	float angle(int node_index, eAxis axis) const
	{
		WeaponRotationNode node(node_index, axis);
		RotationNodes::const_iterator it = std::find(nodes_.begin(), 
			nodes_.end(), node);

		if(it != nodes_.end())
			return it->angle();

		return 0.f;
	}

	void quant()
	{
		for(RotationNodes::iterator it = nodes_.begin(); it != nodes_.end(); ++it)
			it->quant();
	}

private:
	typedef std::vector<WeaponRotationNode> RotationNodes;
	RotationNodes nodes_;
};

namespace weapon_helpers {

/// Трассировка луча, возвращает false если он упёрся в поверхность.
/// out - координаты точки на поверхности, в которую попал луч
bool traceGround(const Vect3f& from, const Vect3f& to, Vect3f& out);

/// Трассировка луча, возвращает false если он упёрся в поверхность.
/// out - координаты высшей точки на поверхности по ходу луча
bool traceHeight(const Vect3f& from, const Vect3f& to, Vect3f& out);

/// Трассировка луча, возвращает false если он упёрся в юнит.
/// out - координаты пересечения луча с баундом юнита
bool traceUnit(const Vect3f& from, const Vect3f& to, Vect3f& out, const UnitReal* owner = 0, const UnitBase* target = 0, int environment_destruction = 0);

bool traceAimPosition(const Vect3f& from, const Vect3f& to, const UnitReal* owner, Vect3f& aim_pos, const UnitBase*& aim_unit);

/// трассировка видимости одного юнита другим с учетом ландшафта
/// возвращает true, если \a target_unit видим
bool traceVisibility(const UnitBase* unit, const UnitBase* target_unit);

}


#endif /* __WEAPON_H__ */
