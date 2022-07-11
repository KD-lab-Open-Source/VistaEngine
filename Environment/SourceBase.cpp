#include "stdafx.h"

#include "Serialization.h"
#include "RangedWrapper.h"
#include "SourceBase.h"
#include "RenderObjects.h"
#include "..\Game\Universe.h"
#include "..\Water\Water.h"
#include "Dictionary.h"
#include "..\Units\BaseUnit.h"
#include "..\Environment\Environment.h"
#include "..\physics\WindMap.h"
#include "..\UserInterface\UI_Logic.h"
//#include "ExternalShow.h"
//#include "CameraManager.h"
#include "EditorVisual.h"

#pragma warning(disable: 4355)

const char* SourceBase::MapName[SOURCE_MAX];

UNIT_LINK_GET(SourceBase)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, TargetClass, "ZoneEffectType");
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_ATTACK_ALL, "Реагировать на все виды юнитов");
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_ATTACK_GROUND, "Реагировать только на наземных");
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_ATTACK_AIR, "Реагировать только на летающих");
END_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, TargetClass);

BEGIN_ENUM_DESCRIPTOR(SourceType, "SourceType");
REGISTER_ENUM(SOURCE_WATER,     "Источник воды");
REGISTER_ENUM(SOURCE_BUBBLE,    "Источник пузырьков");
REGISTER_ENUM(SOURCE_LIGHT,     "Источник света");
REGISTER_ENUM(SOURCE_BREAK,     "Источник трещин");
REGISTER_ENUM(SOURCE_GEOWAVE,   "Источник гео волн");
REGISTER_ENUM(SOURCE_TERROOL,   "Источник тулзеров");
REGISTER_ENUM(SOURCE_ICE,       "Источник льда");
REGISTER_ENUM(SOURCE_FREEZE,    "Источник заморозки");
REGISTER_ENUM(SOURCE_ZONE,      "Зона на мире");
REGISTER_ENUM(SOURCE_IMPULSE,   "Источник импульсов");
REGISTER_ENUM(SOURCE_TORNADO,   "Источник торнадо");
REGISTER_ENUM(SOURCE_BLAST,     "Источник взрывной волны");
REGISTER_ENUM(SOURCE_LIGHTING,  "Источник молний");
REGISTER_ENUM(SOURCE_CAMERA_SHAKING, "Источник тряски камеры");
REGISTER_ENUM(SOURCE_FLASH,     "Источник вспышки");
REGISTER_ENUM(SOURCE_DETECTOR,  "Источник обнаружения");
REGISTER_ENUM(SOURCE_WATER_WAVE,"Источник волны на воде");
REGISTER_ENUM(SOURCE_SHIELD,	"Источник защитного поля");
REGISTER_ENUM(SOURCE_TELEPORT,	"Источник телепортатор");
REGISTER_ENUM(SOURCE_DELETE_GRASS, "Источник удаления травы");
REGISTER_ENUM(SOURCE_FLOCK,     "Источник - стая на мире");
END_ENUM_DESCRIPTOR(SourceType);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, SurfaceClass, "SourceBase::SurfaceClass")
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_SURFACE_ANY, "на любой поверхности")
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_SURFACE_GROUND, "только на земле")
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_SURFACE_WATER, "только на воде")
REGISTER_ENUM_ENCLOSED(SourceBase, SOURCE_SURFACE_ICE, "только на льду")
END_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, SurfaceClass)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, PlacementMode, "SourceBase::PlacementMode")
REGISTER_ENUM_ENCLOSED(SourceBase, PLACE_NONE, "Оставить где положили")
REGISTER_ENUM_ENCLOSED(SourceBase, PLACE_TO_GROUND, "Ставить на землю")
REGISTER_ENUM_ENCLOSED(SourceBase, PLACE_TO_WATER,  "Ставить на воду")
END_ENUM_DESCRIPTOR_ENCLOSED(SourceBase, PlacementMode)

BEGIN_ENUM_DESCRIPTOR(SurfaceKind, "SurfaceKind")
REGISTER_ENUM(SURFACE_KIND_1, "Поверхность 1 рода")
REGISTER_ENUM(SURFACE_KIND_2, "Поверхность 2 рода")
REGISTER_ENUM(SURFACE_KIND_3, "Поверхность 3 рода")
REGISTER_ENUM(SURFACE_KIND_4, "Поверхность 4 рода")
END_ENUM_DESCRIPTOR(SurfaceKind)

void SourceBase::serialize(Archive& ar) 
{
	if(!ar.isEdit()){
		if(ar.isInput()){
			unitID_.unregisterUnit();
			xassert(!unitID_.numRefs());
		}
		unitID_.serialize(ar);
	}
	
	if(enabled() || !ar.isEdit())
		ar.serialize(active_, "active", "Активный");
	
	ar.serialize(label_, "label", "Имя метки");
	if(!ar.isEdit())
		ar.serialize(pose_, "pose", "Позиция");
	ar.serialize(RangedWrapperf (radius_, 1.0f, 2000.0f, 1.0f), "radius", "Радиус");

	ar.openBlock("", "Взаимодействие со средой");
		ar.serialize(surfaceClass_, "surfaceClass", "Можно поставить");
		ar.serialize(surfaceKind_, "surfaceKind", "Можно поставить (род поверхности)");
		ar.serialize(placementMode_, "placementMode", "Установка на поверхность");
		ar.serialize(move_by_wind_, "move_by_wind", "Сносится ветром");
		ar.serialize(windSensitivity_, "windSensitivity", "Коэффициент скорости по ветру");
		ar.serialize(move_by_water_, "move_by_water", "Сносится водой");
		ar.serialize(mapUpdateDeactivate_, "mapUpdateDeactivate", "Выключать при изменении поверхности");
		ar.serialize(mapUpdateActivate_, "mapUpdateActivate", "Включать при изменении поверхности");
		if(mapUpdateDeactivate_)
			mapUpdateActivate_ = false;
	ar.closeBlock();

	ar.openBlock("", "Автоактивация");
		ar.serialize(waiting_target_, "waiting_target", "Активироваться при цели в зоне");
		if(waiting_target_){
			ar.serialize(activate_by_detonator_, "activate_by_detonator", "Активироваться только детонатором");
			if(!activate_by_detonator_){
				ar.serialize(activatorMode_, "activatorMode", "Тип цели для активации");
				ar.serialize(activate_by_projectile_, "activate_by_projectile", "Активироваться при попадании ракеты");
			}
			ar.serialize(killOwner_, "killOwner", "Убивать хозяина при активации");
		}
		ar.serialize(waitingEffectAttribute_, "waitingEffectAttribute", "Эффект в режиме ожидания");
		ar.serialize(visibleTime_, "visibleTime", "Время затухания после обнаружения");
	ar.closeBlock();
	
	ar.serialize(targetClass_, "targetClass_", "Класс атакуемых юнитов");

	ar.serialize(soundReference_, "soundReference", "звук");

	ar.serialize(followPath_, "followPath", "Следовать по траектории");
	if(!ar.isEdit()) {
		ar.serialize(path_, "path", "Траектория");
		ar.serialize(origin_, "origin", "Сдвиг траектории");
	} else {
        ar.serialize(path_.cycled_, "cycledPath", "Зациклить траекторию");
        ar.serialize(path_.speed_, "speed", "Скорость на траектории");
	}

	ar.serialize(childSources_, "childSources_", "производимые источники");

	ar.serialize(interfaceLightIndex_, "interfaceLightIndex", "Источник света в интерфейсе");

	ar.serialize(activationTimer_, "activationTimer_", 0);
	ar.serialize(activation_started_, "activation_started_", 0);
	ar.serialize(killTimer_, "killTimer_", 0);
	ar.serialize(lifeTime_, "lifeTime_", 0);
	ar.serialize(showTimer_, "showTimer_", 0);
	ar.serialize(deadQuant_, "deadQuant_", 0);
	ar.serialize(affectMode_, "affectMode_", 0);

	int player_id = player_ ? player_->playerID() : -1;
	ar.serialize(player_id, "player_id", "Игрок");
	if(ar.isInput()){
		if(!universe() || player_id >= universe()->Players.size() || player_id < 0)
			player_id = -1;
		player_ = (player_id != -1 && universe()) ? universe()->findPlayer(player_id) : 0;
	}
	ar.serialize(owner_, "owner", 0);

	if(ar.isInput() && enabled()){
		soundInit();
		if(active())
			waitingEffectStop();
	}

	ar.serialize(velocity_, "velocity_", 0);

	ar.serialize(breakWhenApply_, "breakWhenAffect", "Выводить сообщение для отладки при воздействии");
}

void SourceBase::enable()
{ 
	xassert(!enabled_);
	unitID_.registerUnit(this);

	enabled_ = true; 
	soundInit();

	if(!inserted())
		environment->sourceGrid.Insert(*this, position().xi(), position().yi(), round(radius()));

	delete environmentPoints_;
	environmentPoints_ = new Vect3f[ENVIRONMENT_POINTS_COUNT + 1];
	recalcEnvironmentPoints();
}

void SourceBase::showEditor() const
{
	if(editorVisual().isVisible(objectClass())){
		string txtOut = getEnumName(type());
		if(strlen(label())){
			txtOut += " - ";
			txtOut += label();
		}

		editorVisual().drawText(position(), txtOut.c_str(), EditorVisual::TEXT_LABEL);
		editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
		editorVisual().drawOrientationArrow(pose(), selected());
	}
}

void SourceBase::setPose(const Se3f& _pos, bool init)
{
	Se3f pos(_pos);
	if(placementMode_ != PLACE_NONE){
		pos.trans().z = vMap.getVoxelW(pos.trans().xi(), pos.trans().yi());
		if(placementMode_ == PLACE_TO_WATER)
			pos.trans().z = max(environment->water()->GetZ(pos.trans().xi(), pos.trans().yi()), pos.trans().z);
	}
	if (init) {
		origin_ += pos.trans() - pose_.trans();
	}
	BaseUniverseObject::setPose(pos, init);
	sound_.setPosition(position());
	if(inserted())
		environment->sourceGrid.Move(*this, position().xi(), position().yi(), round(radius()));
}

void SourceBase::setRadius(float _radius)
{
	xassert(enabled_);
	__super::setRadius(_radius);
	xassert(environmentPoints_);
	recalcEnvironmentPoints();
}

class UnitScanOperator
{
public:
	UnitScanOperator(SourceBase* zone = 0){
		if(zone)
			setOwner(zone);
	}

	void setOwner(SourceBase* zone){
		zone_ = zone;
		radius_ = zone_->radius();
		position_ = Vect2f(zone_->position());
	}

	void operator()(UnitBase* p){
		if(zone_->canApply(p) && checkTargetMode(p) && p->position2D().distance2(position_) < sqr(radius_ + p->radius()))
			zone_->apply(p);
	}

private:

	float radius_;
	Vect2f position_;
	SourceBase* zone_;

	bool checkTargetMode(UnitBase* p){
		switch(zone_->targetClass()){
		case SourceBase::SOURCE_ATTACK_AIR:
			return WeaponBase::unitMode(p) == WeaponPrm::ON_AIR;
		case SourceBase::SOURCE_ATTACK_GROUND:
			return WeaponBase::unitMode(p) != WeaponPrm::ON_AIR;
		}
		return true;
	}
};

void SourceBase::quant()
{
	xassert (enabled_);

	if (followPath_ && active_) {
		path_.quant();

        Vect3f pos = path_.currentPosition();
        pose().rot().xform(pos);
		setPose(Se3f(orientation(), origin_ + pos), false);
    }
	
	sound_.quant();

	if(!active())
		killTimer_.pause();

	if(killTimer_() && isUseKillTimer())
		kill();

	if(!isAlive()){
		unitID_.unregisterUnit();
		return;
	}

	if((scanEnvironment_ || (waiting_target_&& !activation_started_))){
		static UnitScanOperator unitScanOperator;
		unitScanOperator.setOwner(this);
		targetInZone = false;
		universe()->unitGrid.Scan(position().xi(), position().yi(), round(radius()), unitScanOperator);
	}

	if(!active() && waiting_target_ && !activation_started_)
		if(targetInZone){
			if(activationTimer_.was_started())
				activation_started_ = true;
			else
				setActivity(true);
			if(killOwner_ && owner()){
				owner()->Kill();
				setOwner(0);
			}
		}
		else {
			activationTimer_.pause();

			if(player()->active())
				waitingEffectStart();
			else if(!showTimer_())
				waitingEffectStop();
		}

	if(active()){
		ChildSources::iterator it;
		FOR_EACH(childSources_, it){
			ChildSource& src = *it;
			if(src.source_.isEmpty()) continue;
			if(!src.activationTimer_.was_started() || (src.activationTimer_() && src.generationDelay_ > FLT_EPS)){
				src.activationTimer_.start(src.generationDelay_ * 1000);
				if(SourceBase* source = environment->createSource(&src.source_, currentPose(), false)){
					source->setPlayer(player_);
					source->setOwner(owner());
				}
			}
		}
		if(move_by_wind_ || move_by_water_)
			environmentAnalysis();
	}
}

void SourceBase::recalcEnvironmentPoints()
{
	xassert(environmentPoints_);
	float R = 0.9f * radius();
	for(int pn = 0; pn < ENVIRONMENT_POINTS_COUNT; pn++){
		float angle = 2.f * M_PI * pn / ENVIRONMENT_POINTS_COUNT;
		environmentPoints_[pn] = Vect3f(R * cosf(angle), R * sinf(angle), 0.f);
	}
	environmentPoints_[ENVIRONMENT_POINTS_COUNT] = Vect3f::ZERO;
}

void SourceBase::environmentAnalysis()
{
	xassert(environmentPoints_);
	xassert(move_by_wind_ || move_by_water_);

	Vect3f V = Vect3f::ZERO; // линейная скорость
	Vect3f Rot = Vect3f::ZERO; //вращение

	Se3f currPose = pose();

	if(move_by_water_){
		for(int pn = 0; pn <= ENVIRONMENT_POINTS_COUNT; ++pn){
			Vect3f point = clampWorldPosition(position() + environmentPoints_[pn], 0) - position();
			Vect3f vel(environment->water()->GetVelocity(position() + point));
			V += vel;
			Rot += vel.precross(point);
		}
		if(abs(Rot.z) > FLT_EPS){
			QuatF rotate(0.001 * Rot.z / ENVIRONMENT_POINTS_COUNT * logicPeriodSeconds, Vect3f::K);
			currPose.rot().postmult(rotate);
		}
		V *= (logicPeriodSeconds / ENVIRONMENT_POINTS_COUNT);
	}

	if(move_by_wind_){
		Vect2f wnd = windMap->getBilinear(clampWorldPosition(position(), 2));
		wnd *= logicRndInterval(windSensitivity_);
		QuatF rndRotate(logicRNDfabsRndInterval(-M_PI/2.f, M_PI/2.f), Vect3f::K, 0);
		Vect3f windVelocity(wnd, 0.f);
		rndRotate.xform(windVelocity);
		V.x += windVelocity.x;
		V.y += windVelocity.y;
	}

	velocity_.interpolate(velocity_, V, 0.7f);
	currPose.trans() += velocity_;
	setPose(currPose, false);
}

void SourceBase::apply(UnitBase* target)
{
	targetInZone = true;
}

bool SourceBase::checkTarget(AffectMode mode, const UnitBase* target) const
{
	switch(mode){
	case AFFECT_NONE_UNITS:
		return false;
	case AFFECT_OWN_UNITS:
		return (player() == target->player());
	case AFFECT_FRIENDLY_UNITS:
		return !player()->isEnemy(target);
	case AFFECT_ALLIED_UNITS:
		return !player()->isEnemy(target) && (player() != target->player());
	case AFFECT_ENEMY_UNITS:
		return player()->isEnemy(target);
	case AFFECT_ALL_UNITS:
		return true;
	}
	return false;
}

bool SourceBase::isDetonator(const UnitBase* target) const
{
	// если активация только по детонатору, то его и ждем
	bool thisDetonator = (target->attr().unitClass() == UNIT_CLASS_ZONE && player() == target->player());
	if(activate_by_detonator_ || thisDetonator)
		return thisDetonator;

	if(target->attr().isProjectile()) // со снарядами разбираемся отдельно
		if(activate_by_projectile_)
			return !target->alive(); // попавшая ракета - мертвая ракета
		else
			return false; // снаряд не ждем или уже дождались

	if(target->attr().isActing()) // на декорации не реагируем
		return checkTarget(activatorMode_, target);
	
	return false;
}

bool SourceBase::canApply(const UnitBase* target) const
{
	if(active_)
		return checkTarget(affectMode_, target);
	else if(waiting_target_)
		return isDetonator(target);
	return false;
}

float SourceBase::Path::length() const
{
    float len = 0.0f;
    for(int i = 0; i < size() - 1; ++i) {
        len += (*this)[i].distance((*this)[i+1]);
    }
	if(cycled_) {
        len += front().distance(back());
	}
    return len;
}

float SourceBase::Path::period() const
{
    float time = 0.0f;
    for(int i = 0; i < size() - 1; ++i) {
		float vel = ((*this)[i].velocity_ + (*this)[i+1].velocity_) * 0.5f;
		time += (*this)[i].distance((*this)[i+1]) / vel;
    }
	if(cycled_) {
		float vel = (back().velocity_ + front().velocity_) * 0.5f;
		time += back().distance(front()) / vel;
	}
	return time;
}

void SourceBase::Path::init()
{
}

Vect3f SourceBase::Path::currentPosition() const
{
	if(empty())
		return Vect3f::ZERO;

	if(size() == 1)	 {
		return front();
	} else {
		const Path& self = *this;
		Vect3f dir = self[(segment_ + 1)%size()] - self[segment_];
		return self[segment_] + dir * position_;
	}
}

void SourceBase::Path::quant()
{
	if(size() <= 1)
		return;

	int segmentsCount = size();

	const float timeElapsed = logicPeriodSeconds;
	const PathPosition& last = (*this)[segment_];
	const PathPosition& next = (*this)[(segment_ + 1)%segmentsCount];

    Vect3f dir = next - last;
    float segmentLength = dir.norm();
    float velocity = speed_ * next.velocity_ * position_ + speed_ * last.velocity_ * (1.0f - position_);

	position_ += logicPeriodSeconds * speed_ * velocity / segmentLength;
	while (position_ > 1.0f) { 
		position_ -= 1.0f;
		if(cycled_) {
			segment_ = (segment_ + 1) % segmentsCount;	
		} else {
			if((++segment_) == segmentsCount - 1)
				segment_ = 0;
		}
	}
}

void SourceBase::PathPosition::serialize(Archive& ar)
{
    Vect3f::serialize(ar);
    ar.serialize(velocity_, "velocity", "Скорость");
}

void SourceBase::Path::serialize(Archive& ar)
{
	ar.serialize(static_cast< std::vector<SourceBase::PathPosition>& >(*this), "nodes", "Узлы");
    ar.serialize(cycled_, "cycled", "Зациклен");
    ar.serialize(speed_, "speed", "Скорость");
    ar.serialize(position_, "segmentPosition", "Позиция на отрезке");
    ar.serialize(segment_, "segment", "Отрезок");
}

string SourceBase::getDisplayName(SourceType type)
{
	xassert(type>=0 && type<SOURCE_MAX);
	return TRANSLATE(getEnumNameAlt(type));
}

void SourceBase::soundInit(){
	xassert(enabled_);
	sound_.init(soundReference_, this);
	if(active())
		sound_.start();
}

void SourceBase::start()
{
	xxassert(pose_.trans().xi() || pose_.trans().yi(), "Источник в нуле");
	if(!sound_.isInited())
		soundInit();
	else
		sound_.start();
	waitingEffectStop();

	if(interfaceLightIndex_ != -1)
		UI_LogicDispatcher::instance().addLight(interfaceLightIndex_, pose_.trans());
}

void SourceBase::stop()
{
	sound_.stop();
}

void SourceBase::setActivity(bool _active){
	xxassert(enabled_ || !_active, "запуск не разрешенного источника");
	xxassert(isAlive() || !_active, "запуск мертвого источника");
	if(active_ != _active){
		active_ = _active;
		if(active_)
			start();
		else
			stop();
	}
}

void SourceBase::serializationApply(const Archive& ar)
{
	if(enabled() && ar.isInput())
		if(active())
			start();
		else
			stop();
}

void SourceBase::setAffectMode(AffectMode mode)
{
	affectMode_ = mode;
	if(activatorMode_ == AFFECT_NONE_UNITS)
		activatorMode_ = mode;
}

void SourceBase::setDetected()
{
	if(active() || player()->active() || visibleTime_ < FLT_EPS)
		return;
	
	if(!showTimer_.was_started())
		waitingEffectStart();

	showTimer_.start(visibleTime_ * 1000);
}

Player* SourceBase::worldPlayer() const
{
	return universe()->worldPlayer();
}

UnitBase* SourceBase::owner() const 
{
	return owner_;
}

void SourceBase::setOwner(UnitBase* unit)
{
	xassert(!player_ || !unit || unit->player() == player_);
	owner_ = unit;
}

void SourceBase::waitingEffectStart()
{
	if(!waitingEffectController_.isEnabled()){
		start_timer_auto();
		waitingEffectController_.effectStart(&waitingEffectAttribute_);
	}
}

void SourceBase::waitingEffectStop()
{
	waitingEffectController_.release();
	showTimer_.stop();
}

SourceBase::SourceBase(const SourceBase& src) : waitingEffectController_(this)
{
    label_ = src.label_;
	pose_ = src.pose_;
    radius_ = src.radius_;

	lifeTime_ = src.lifeTime_;

	interfaceLightIndex_ = src.interfaceLightIndex_;

    origin_ = pose_.trans();
    followPath_ = src.followPath_;
    path_ = src.path_;
	path_.segment_ = 0;
	path_.position_ = 0.0f;

	affectMode_ = src.affectMode_;
	activatorMode_ = src.activatorMode_;
    enabled_ = false;
    deadQuant_ = 0;
    active_ = false;
    targetInZone = false;
	activation_started_ = false;
    waiting_target_ = src.waiting_target_;
	killOwner_  = src.killOwner_;
	activate_by_detonator_ = src.activate_by_detonator_;
	activate_by_projectile_ = src.activate_by_projectile_;
    targetClass_ = src.targetClass_;
	surfaceClass_ = src.surfaceClass_;
	surfaceKind_ = src.surfaceKind_;
	placementMode_ = src.placementMode_;
    
	player_ = src.player_;
	visibleTime_ = src.visibleTime_;
	
	childSources_ = src.childSources_;

    soundReference_ = src.soundReference_;

	waitingEffectAttribute_ = src.waitingEffectAttribute_;
	scanEnvironment_ = true;
	environmentPoints_ = 0;
	velocity_ = src.velocity_;
	move_by_wind_ = src.move_by_wind_;
	windSensitivity_ = src.windSensitivity_;
	move_by_water_ = src.move_by_water_;
	mapUpdateDeactivate_ = src.mapUpdateDeactivate_;
	mapUpdateActivate_ = src.mapUpdateActivate_;
	setScanEnvironment(false);

	breakWhenApply_ = src.breakWhenApply_;
}

SourceBase::SourceBase() : waitingEffectController_(this)
{
    pose_ = Se3f::ID;
    radius_ = 100.0f;

    origin_ = Vect3f::ZERO;
    followPath_ = false;

	affectMode_ = AFFECT_ENEMY_UNITS;
	activatorMode_ = AFFECT_NONE_UNITS;

	interfaceLightIndex_ = -1;

	lifeTime_ = 0;

    enabled_ = false;
    active_ = false;
    deadQuant_ = 0;
    targetInZone = false;
	activate_by_detonator_ = false;
    waiting_target_ = false;
	killOwner_ = false;
	activation_started_ = false;
	activate_by_projectile_ = false;
    targetClass_ = SOURCE_ATTACK_ALL;
	surfaceClass_ = SOURCE_SURFACE_ANY;
	surfaceKind_ = 	SURFACE_KIND_1 | SURFACE_KIND_2 | SURFACE_KIND_3 | SURFACE_KIND_4;
	placementMode_ = PLACE_NONE;

	player_ = 0;
	visibleTime_ = 0.f;
	scanEnvironment_ = true;
	environmentPoints_ = 0;
	velocity_ = Vect3f::ZERO;
	move_by_wind_ = false;
	windSensitivity_.set(0.9f, 1.1f);
	move_by_water_ = false;
	mapUpdateDeactivate_ = false;
	mapUpdateActivate_ = false;
	setScanEnvironment(false);

	breakWhenApply_ = false;
}

SourceBase::~SourceBase()
{
	dassert(!active() && "убийство не остановленного источника");
	sound_.release();
	waitingEffectController_.release();
	if(inserted())
		environment->sourceGrid.Remove(*this);
	delete environmentPoints_;
}

void SourceBase::showDebug() const
{
	if(showDebugSource.name)
		show_text(position(), name_.c_str(), MAGENTA);

	if(showDebugSource.label)
		show_text(position(), label(), MAGENTA);

	if(showDebugSource.radius)
		show_vector(position(), radius(), MAGENTA);

	if(showDebugSource.type)
		show_text(position()+Vect3f(0, -5, 0), getEnumNameAlt(type()), MAGENTA);

	if(showDebugSource.showEnvironmentPoints)
		if(environmentPoints_){
			for(int pn = 0; pn < ENVIRONMENT_POINTS_COUNT; pn++)
				show_vector(position()+environmentPoints_[pn], 2.f, GREEN);
		}

	if(showDebugSource.axis){
		MatXf X(pose());
		Vect3f delta = X.rot().xcol();
		delta.normalize(15);
		show_vector(X.trans(), delta, X_COLOR);

		delta = X.rot().ycol();
		delta.normalize(15);
		show_vector(X.trans(), delta, Y_COLOR);

		delta = X.rot().zcol();
		delta.normalize(15);
		show_vector(X.trans(), delta, Z_COLOR);
	}
}

void SourceBase::kill() 
{
	if(isAlive() && (killRequest() || isUnderEditor())){
		incDeadCnt();
		setActivity(false);
	}
}

void SourceBase::mapUpdate(float x0,float y0,float x1,float y1) 
{
	if((mapUpdateDeactivate_ || mapUpdateActivate_) && position2D().distance2(Vect2f(x0 + x1, y0 + y1)/2) < sqr(radius()) + sqr((x1 - x0)/2) + sqr((y1 - y0)/2))
		setActivity(mapUpdateDeactivate_ ? false : true);
}

void SourceBase::ChildSource::serialize(Archive &ar){
	ar.serialize(generationDelay_, "generationDelay", "интервал появления");
	
	source_.serialize(ar);
	
	if(ar.isOutput() && isUnderEditor())
		activationTimer_.stop();
	ar.serialize(activationTimer_, "activationTimer_", 0);
}

const SourceAttribute* SourceBase::getChildSourceByKey(const char* key) const
{
	ChildSources::const_iterator it;
	FOR_EACH(childSources_, it)
		if(const SourceAttribute* satt = it->source_.getByKey(key))
			return satt;
	return 0;
}

SourceAttribute::SourceAttribute()
{
	activationDelay_ = lifeTime_ = 0;
}

void SourceAttribute::serialize(Archive& ar)
{
	ar.serialize(activationDelay_, "activationDelay", "задержка старта");
	ar.serialize(lifeTime_, "lifeTime", "время действия");
	ar.serialize(key_, "key", "Ключ для типсов");
	ar.serialize(sourceReference_, "sourceReference", "&источник");
}

SourceWeaponAttribute::SourceWeaponAttribute()
{
	isAutonomous_ = false;
	positionDelta_ = Vect2f(0,0);

	showColor_.color.set(0, 0, 200, 0);
}

const SourceAttribute* SourceAttribute::getByKey(const char* key) const
{
	if(key_ != key)
		return source()->getChildSourceByKey(key);
	return this;
}

void SourceWeaponAttribute::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(showColor_, "showColor", "Отображение радиуса источника");
	ar.serialize(isAutonomous_, "isAutonomous", "не выключать до окончания времени действия источника");
	ar.serialize(positionDelta_, "positionDelta", "смещение");
}