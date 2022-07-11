#include "stdafx.h"
#include "Universe.h"
#include "UnitEnvironment.h"
#include "TransparentTracking.h"
#include "NormalMap.h"
#include "GlobalAttributes.h"
#include "EditorVisual.h"

UnitEnvironment::UnitEnvironment(const UnitTemplate& data)
: UnitBase(data)
{
	radius_ = attr().boundRadius;
	modelName_ = attr().modelName;
	environmentType_ = ENVIRONMENT_PHANTOM;
	ptBoundCheck_ = false;
	checkGround_ = true;
	checkGroundPoint_ = false;
	holdOrientation_ = false;
	verticalOrientation_ = false;

	lighted_ = true;
	burnt_ = false;
	destroyInWater_ = false;
	destroyInAbnormalState_ = false;

	hideByDistance = true;
	canBeTransparent_ = false;
	fieldOfViewMapAdd_ = false;
	fieldOfViewMapAdded_ = false;
	lodDistance_ = OBJECT_LOD_DEFAULT;
}

void UnitEnvironment::serialize(Archive& ar)
{
	__super::serialize(ar);

	float _radius = radius();
	ar.serialize(_radius, "radius", "Радиус");
	ar.serialize(ptBoundCheck_, "ptBoundCheck", "Учитывать баунд в поиске пути");
	string modelName = modelName_;
	static ModelSelector::Options options("*.3dx", "RESOURCE\\TerrainData\\Models", "Модель");
	ar.serialize(ModelSelector(modelName, options), "modelName", "Имя модели");
	ar.serialize(environmentType_, "environmentType", "Тип");
	ar.serialize(lodDistance_,"distanceLod","ЛОД: Дистанция переключения");
	if(environmentType_ != ENVIRONMENT_TREE && environmentType_ != ENVIRONMENT_BUSH &&
	   environmentType_ != ENVIRONMENT_FENCE && environmentType_ != ENVIRONMENT_FENCE2 && 
	   environmentType_ != ENVIRONMENT_STONE){
		if(environmentType_ == ENVIRONMENT_PHANTOM2){
			environmentType_ = ENVIRONMENT_INDESTRUCTIBLE;
			checkGround_ = false;
		}
		ar.serialize(checkGround_, "checkGround", "Реагировать на изменение поверхности");
		if(environmentType_ == ENVIRONMENT_PHANTOM2){
			environmentType_ = ENVIRONMENT_INDESTRUCTIBLE;
			checkGround_ = false;
		}
		if(checkGround_)
			ar.serialize(checkGroundPoint_, "checkGroundPoint", "Анализировать поверхность только по центру");
		else
			checkGroundPoint_ = false;
	} else {
		checkGround_ = true;
		checkGroundPoint_ = false;
	}

	if(environmentType_ == ENVIRONMENT_TREE){
		ar.serialize(holdOrientation_, "holdOrientation", "Сохранять ориентацию");
		if(holdOrientation_)
			ar.serialize(verticalOrientation_, "verticalOrientation", "Ориентировать вертикально");
		else
			verticalOrientation_ = false;
	}else{
		holdOrientation_ = false;
		verticalOrientation_ = false;
	}

	ar.serialize(lighted_, "lighted", "Освещен");
	ar.serialize(burnt_, "burnt", "Горелый");

	ar.serialize(destroyInWater_, "destroyInWater", "Разрушается в воде");
	ar.serialize(destroyInAbnormalState_, "destroyInAbnormalState", "Разрушается при любом воздействии");

	ar.serialize(hideByDistance, "hideByDistance", "Исчезает при удалении");
	ar.serialize(canBeTransparent_, "canBeTransparent", "Становится прозрачным если позади юнит");
	ar.serialize(fieldOfViewMapAdd_, "fieldOfViewMapAdd", "Добавлять в карту препятствий");
	
	if(ar.isInput()){
		setModel(modelName.c_str());
		if(!alive())
			return;
		setRadius(_radius);
		//setPose(pose(), true);

		setUnitAttackClass((AttackClass)environmentType_);
	}

	ObjectShadowType shadowType = get3dx()->getShadowType();
	float shadowRadius, shadowHeight;
	get3dx()->getCircleShadowParam(shadowRadius, shadowHeight);
	
	ar.serialize(shadowType, "shadowType", "Тип тени");
	if(shadowType == OST_SHADOW_CIRCLE)
		ar.serialize(shadowRadius, "shadowRadius", "Радиус круглой тени");
	
	if(ar.isInput()){
		get3dx()->SetShadowType(shadowType);
		get3dx()->SetCircleShadowParam(shadowRadius, -1);
	}

	if(rigidBody() && rigidBody()->isEnvironment()){
		float angleZ = safe_cast<RigidBodyEnvironment*>(rigidBody())->angleZ();
		ar.serialize(angleZ, "angleZ", 0);
		safe_cast<RigidBodyEnvironment*>(rigidBody())->setAngleZ(angleZ);
	}
}

void UnitEnvironment::Kill()
{
	if(fieldOfViewMapAdded_){
		fieldOfViewMapAdded_ = false;
		streamLogicCommand.set(fCommandFieldOfViewMapRemove, get3dx());
	}

	__super::Kill();
}

void UnitEnvironment::setPose(const Se3f& poseIn, bool initPose)
{
	if(rigidBody() && initPose){
		rigidBody()->initPose(poseIn);
		UnitBase::setPose(rigidBody()->pose(), initPose);
	}
	else
		UnitBase::setPose(poseIn, initPose);

	if(initPose && fieldOfViewMapAdd_ && get3dx()){
		if(fieldOfViewMapAdded_)
			streamLogicCommand.set(fCommandFieldOfViewMapRemove, get3dx());
		streamLogicPostCommand.set(fCommandFieldOfViewMapAdd, get3dx());
		fieldOfViewMapAdded_ = true;
	}
}

void UnitEnvironment::setRadius(float new_radius)
{
	radius_ = clamp(new_radius, 0.1f, 400.f);
	setModel(modelName_.c_str());
}

void UnitEnvironment::Quant()
{
	start_timer_auto();

	UnitBase::Quant(); 

	if(abnormalStateType())
		burnt_ = true;

	if(rigidBody()){
		start_timer_auto();
		if(rigidBody()->evolve(logicPeriodSeconds))
			setPose(rigidBody()->pose(), false);
	}
}

bool UnitEnvironment::checkInPathTracking(const UnitBase* tracker) const
{
	if(rigidBody() && tracker->attr().isActing() && safe_cast<const UnitActing*>(tracker)->rigidBody()->flyingMode()) {
		Vect3f trackerPoint(tracker->rigidBody()->extent());
		trackerPoint.negate();
		tracker->rigidBody()->orientation().xform(trackerPoint);
		trackerPoint.add(tracker->rigidBody()->centreOfGravity());
		Vect3f buildingPoint(rigidBody()->extent());
		rigidBody()->orientation().xform(buildingPoint);
		buildingPoint.add(rigidBody()->centreOfGravity());
		if(buildingPoint.z < trackerPoint.z)
			return false;
	}

	if(environmentType_ == ENVIRONMENT_PHANTOM || environmentType_ == ENVIRONMENT_BUSH || environmentType_ == ENVIRONMENT_TREE || environmentType_ == ENVIRONMENT_BRIDGE)
		return false;

	if(environmentType_ & tracker->attr().environmentDestruction)
		return false;

	return true;
}

bool UnitEnvironment::checkInBuildingPlacement() const
{
	switch(environmentType()){
	case ENVIRONMENT_PHANTOM:
	case ENVIRONMENT_PHANTOM2:
	case ENVIRONMENT_BUSH:
		return false;
	default:
		return true;
	}
}

void UnitEnvironment::mapUpdate(float x0,float y0,float x1,float y1)
{
	__super::mapUpdate(x0, y0, x1, y1);
}

void UnitEnvironment::showDebugInfo()
{
	__super::showDebugInfo();

	if(showDebugUnitBase.lodDistance_)
	{
		XBuffer buf;
		float radius;
		if(rigidBody())
		{
			const Vect3f& bound = rigidBody()->extent();
			radius = (bound.x+bound.y+bound.z)/3;
		}else
		{
			sBox6f bound;
			get3dx()->GetBoundBox(bound);
			Vect3f extent(bound.max);
			extent -= bound.min;
			extent *= 0.5f;
			radius = (extent.x+extent.y+extent.z)/3;
		}
		buf <= numLodDistance < "(" <= radius <")";
		show_text(position(), buf, Color4c::RED);
	}

	if(showDebugUnitEnvironment.rigidBody && rigidBody())
		rigidBody()->show();

	if(showDebugUnitEnvironment.environmentType)
		show_text(position(), getEnumName(environmentType_), Color4c::CYAN);
	if(showDebugUnitEnvironment.modelName)
		show_text(position(), modelName_.c_str(), Color4c::RED);
}

bool UnitEnvironment::setAbnormalState( const AbnormalStateAttribute& state, UnitBase* ownerUnit)
{
	if(destroyInAbnormalState_ && deathParameters_.abnormalStateEffect(state.type())){
		explode();
		Kill();
		return true; 
	}
	else
		return __super::setAbnormalState(state, ownerUnit);
}

bool UnitEnvironment::hasAbnormalState(const AbnormalStateAttribute& state) const
{
	if(destroyInAbnormalState_ && deathParameters_.abnormalStateEffect(state.type()))
		return true;
	else
		return __super::hasAbnormalState(state);
}

void UnitEnvironment::showEditor()
{
	__super::showEditor();

	if(editorVisual().isVisible(objectClass())){
		if(selected())
			editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
		else
			editorVisual().drawImpassabilityRadius(*this);
	}
}
