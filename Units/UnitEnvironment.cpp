#include "stdafx.h"
#include "Universe.h"
#include "UnitEnvironment.h"
#include "TransparentTracking.h"
#include "NormalMap.h"
#include "ExternalShow.h"
#include "GlobalAttributes.h"
#include "EditorVisual.h"

UnitEnvironment::UnitEnvironment(const UnitTemplate& data)
: UnitBase(data)
{
	radius_ = attr().boundRadius;
	modelName_ = attr().modelName;
	environmentType_ = ENVIRONMENT_PHANTOM;
	checkGround_ = true;
	checkGroundPoint_ = false;
	holdOrientation_ = false;
	verticalOrientation_ = false;

	lighted_ = true;
	burnt_ = false;
	destroyInWater_ = false;
	destroyInAbnormalState_ = false;

	hideByDistance = true;
	lodDistance_=OBJECT_LOD_DEFAULT;
}

void UnitEnvironment::serialize(Archive& ar)
{
	__super::serialize(ar);

	float _radius = radius();
	ar.serialize(_radius, "radius", "Радиус");

	std::string modelName = modelName_;
	static ModelSelector::Options options("*.3dx", ".\\RESOURCE\\TerrainData\\Models", "Will select location of 3DX model");
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

	if(ar.isInput()){
		setModel(modelName.c_str());
		if(!alive())
			return;
		setRadius(_radius);
		//setPose(pose(), true);

		setUnitAttackClass((AttackClass)environmentType_);
	}

	ShadowType shadowType = SHADOW_REAL;

	if(ar.isEdit())
		environment->shadowWrapper().serializeForModel(ar, get3dx(), scale_ / radius_);
	else if(ar.isInput() && ar.serialize(shadowType, "shadowType", "Тип тени")){ // conversion 25.09
		float shadowRadius = 0.5 * scale_;
		if(shadowType == SHADOW_CIRCLE){
			ar.serialize(shadowRadius, "shadowRadius", "Радиус тени для круглой тени");
			shadowRadius /= scale_;
		}
		environment->shadowWrapper().setForModel(get3dx(), shadowType, shadowRadius);
	}
		
}

void UnitEnvironment::setPose(const Se3f& poseIn, bool initPose)
{
	if(rigidBody_ && initPose){
		rigidBody_->initPose(poseIn);
		UnitBase::setPose(rigidBody_->pose(), initPose);
	}
	else
		UnitBase::setPose(poseIn, initPose);
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

	if(rigidBody_){
		start_timer_auto();
		if(rigidBody_->evolve(logicPeriodSeconds))
			setPose(rigidBody_->pose(), false);
	}
}

bool UnitEnvironment::checkInPathTracking(const UnitBase* tracker) const
{
	if(rigidBody_ && tracker->rigidBody() && tracker->rigidBody()->isUnit() && safe_cast<RigidBodyUnit*>(tracker->rigidBody())->flyingMode()) {
		Vect3f trackerPoint(tracker->rigidBody()->extent());
		trackerPoint.negate();
		tracker->rigidBody()->orientation().xform(trackerPoint);
		trackerPoint.add(tracker->rigidBody()->centreOfGravity());
		Vect3f buildingPoint(rigidBody_->extent());
		rigidBody_->orientation().xform(buildingPoint);
		buildingPoint.add(rigidBody_->centreOfGravity());
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
		if(rigidBody_)
		{
			const Vect3f& bound = rigidBody_->box().getExtent();
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
		show_text(position(), buf, RED);
	}

	if(showDebugUnitEnvironment.rigidBody && rigidBody_)
		rigidBody_->show();

	if(showDebugUnitEnvironment.environmentType)
		show_text(position(), getEnumName(environmentType_), CYAN);
	if(showDebugUnitEnvironment.modelName)
		show_text(position(), modelName_.c_str(), RED);
}

void UnitEnvironment::refreshAttribute()
{
	setRadius(radius());
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
	if(editorVisual().isVisible(objectClass())){
		if(selected())
			editorVisual().drawRadius(position(), radius(), EditorVisual::RADIUS_OBJECT, selected());
		else
			editorVisual().drawImpassabilityRadius(*this);
	}
}
