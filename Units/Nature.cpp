#include "StdAfx.h"
#include "Universe.h"
#include "Nature.h"
#include "RenderObjects.h"
#include "GlobalAttributes.h"
#include "Physics\crash\CrashSystem.h"
#include "Environment\Environment.h"
#include "EnvironmentSimple.h"
#include "Serialization\BinaryArchive.h"
#include "Serialization\SerializationFactory.h"
#include "EditorVisual.h"
#include "VistaRender\FieldOfView.h"
#include "Render\src\Scene.h"

DECLARE_SEGMENT(UnitEnvironmentBuilding)
REGISTER_CLASS(UnitBase, UnitEnvironmentBuilding, "UnitEnvironmentBuilding")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ENVIRONMENT, UnitEnvironmentBuilding);

BEGIN_ENUM_DESCRIPTOR(EnvironmentType, "Type")
REGISTER_ENUM(ENVIRONMENT_PHANTOM, "Фантом")
REGISTER_ENUM(ENVIRONMENT_PHANTOM2, "Никогда неразрушаемое здание")
REGISTER_ENUM(ENVIRONMENT_BUSH, "Куст")
REGISTER_ENUM(ENVIRONMENT_TREE, "Дерево")
REGISTER_ENUM(ENVIRONMENT_FENCE, "Забор")
REGISTER_ENUM(ENVIRONMENT_FENCE2, "Неразрушаемый забор")
REGISTER_ENUM(ENVIRONMENT_STONE, "Камень")
REGISTER_ENUM(ENVIRONMENT_ROCK, "Скала")
REGISTER_ENUM(ENVIRONMENT_BASEMENT, "Фундамент здания")
REGISTER_ENUM(ENVIRONMENT_BARN, "Сарай")
REGISTER_ENUM(ENVIRONMENT_BUILDING, "Здание")
REGISTER_ENUM(ENVIRONMENT_BRIDGE, "Мост")
REGISTER_ENUM(ENVIRONMENT_INDESTRUCTIBLE, "Неразрушаемое строение")
REGISTER_ENUM(ENVIRONMENT_BIG_BUILDING, "Большое здание")
END_ENUM_DESCRIPTOR(EnvironmentType)

#pragma warning(disable: 4355)

UnitEnvironmentBuilding::UnitEnvironmentBuilding(const UnitTemplate& data)
: UnitEnvironment(data), chainControllers_(this)
{
	model_ = 0;
	deviationCosMin_ = 0;
}

UnitEnvironmentBuilding::~UnitEnvironmentBuilding()
{
	RELEASE(model_);
}

void UnitEnvironmentBuilding::showEditor()
{
	__super::showEditor();

	if(model()){
		if(!editorVisual().isVisible(objectClass()))
			streamLogicPostCommand.set(fCommandSetIgnored, model()) << true;
		else
			if(isVisibleUnderForOfWar())
				streamLogicPostCommand.set(fCommandSetIgnored, model()) << false;
	}

	// Конверсия в симпл
	if(!dead() && isEnvironmentSimple(environmentType_)){
		UnitBase* unit = player()->buildUnit(AuxAttributeReference(AUX_ATTRIBUTE_ENVIRONMENT_SIMPLE));
		BinaryOArchive oa;
		oa.serialize(*this, "name", 0);
		Kill();
		BinaryIArchive ia(oa);
		ia.serialize(*unit, "name", 0);
	}
}

void UnitEnvironmentBuilding::Quant()
{
	start_timer_auto();

	__super::Quant(); 

	if(dead())
		return;

	if(environment->dayChanged())
		dayQuant(hiddenLogic());

	chainControllers_.quant();
    
	if(model()->GetAnimationGroupNumber())
		if(!burnt_){
			if(!getChain())
				setChain(&animationChain_);
		}
		else if(!getChain() || getChain() != &animationChainBurnt_)
			setChain(&animationChainBurnt_);

	if(rigidBody()){
		if(destroyInWater_ && rigidBody()->onWater() && !isUnderEditor()){
			burnt_ = true;
			explode();
			Kill();
			return;
		}
	}

	fowQuant();
}

void UnitEnvironmentBuilding::dayQuant(bool invisible)
{
	if(environment->isDay() || invisible)
		model()->setAttribute(ATTR3DX_HIDE_LIGHTS);
	else
		model()->clearAttribute(ATTR3DX_HIDE_LIGHTS);
}


void UnitEnvironmentBuilding::serialize(Archive& ar) 
{
	__super::serialize(ar);

	if(!alive())
		return;

	ar.serialize(permanentColor_, "color", "Цвет");

	if(ar.isOutput()){
		AttributeBase::setModel(model(), 0);
		AttributeBase::setCurrentAttribute(0);
	}

	ar.serialize(animationChain_, "animationChain", "Анимационная цепочка");
	ar.serialize(animationChainBurnt_, "animationChainBurnt", "Анимационная цепочка горения");
	animationChain_.cycled = true;
	animationChainBurnt_.cycled = false;

	int angle = round(R2G(acosf(deviationCosMin_)));
	ar.serialize(angle, "angle", "Угол при котором разрушать здание");
	deviationCosMin_ = cosf(G2R(angle));

	if(environmentType_ == ENVIRONMENT_BARN || environmentType_ == ENVIRONMENT_BUILDING || environmentType_ == ENVIRONMENT_BIG_BUILDING)
		deathParameters_.serializeEnvironment(ar);
	else
		deathParameters_.serializeAbnormalStateEffects(ar);

	if(!model())
		Kill(); 

	if(ar.isOutput())
		AttributeBase::setModel(0, 0);
}

void UnitEnvironmentBuilding::setPose(const Se3f& poseIn, bool initPose)
{
	Se3f posePrev = pose();
	__super::setPose(poseIn, initPose);

	if(model()){
		if(initPose)
			posePrev = pose();
		streamLogicInterpolator.set(fSe3fInterpolation, model()) << posePrev << pose();
	}
}

void UnitEnvironmentBuilding::setModel(const char* name)
{
	bool attached = true;
	if(modelName_ != name || !model()) {
		if(!strlen(name))
			return;
		cObject3dx* modelIn = terScene->CreateObject3dxDetached(name, NULL, GlobalAttributes::instance().enableAnimationInterpolation);
		attached = false;
		if(!modelIn)
			return;

		if(model_)
			model_->Release();

		model_ = modelIn;
//		SetLodDistance(model_,lodDistance_);

		if(attr().hideByDistance)
			model()->setAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);
		chainControllers_.setModel(model());

		model()->SetPosition(pose());
	}
	else
		model()->SetScale(1);

	if(!model())
		return;

	if(name && strlen(name))
		modelName_ = name;

	sBox6f boundBox;
	cObject3dx* modelLogic = terScene->CreateLogic3dx(modelName_.c_str());
	if(modelLogic){
		modelLogic->GetBoundBox(boundBox);
		modelLogic->Release();
	}
	else
		model()->GetBoundBox(boundBox);

	if(radius() < FLT_EPS){
		radius_ = boundBox.radius2D();
		if(!attached)
			model()->Attach();
		return;
	}

	scale_ = radius()/max(boundBox.radius2D(), 0.001f);
	height_ = (boundBox.max.z - boundBox.min.z)*scale_;
	
	model()->SetScale(scale_);
		
	if(environmentType_ != ENVIRONMENT_PHANTOM && environmentType_ != ENVIRONMENT_BRIDGE && radius_ > 0.001f){
		setCollisionGroup(COLLISION_GROUP_COLLIDER);

		string referenceName;
		if(checkGround_)
			referenceName = "Environment";
		else
			referenceName = "Environment Phantom";
		RigidBodyPrmReference rigidBodyPrm(referenceName.c_str());

		if(!rigidBody()  || rigidBody()->prm().rigidBodyType != rigidBodyPrm->rigidBodyType){
			delete rigidBody_;
			rigidBody_ = RigidBodyBase::buildRigidBody(rigidBodyPrm->rigidBodyType, rigidBodyPrm, scale_ * boundBox.center(), scale_ * boundBox.extent(), 1.0f);
		}else
			rigidBody()->build(*rigidBodyPrm, scale_ * boundBox.center(), scale_ * boundBox.extent(), 1.0f);
		rigidBody()->setPose(pose());
		if(checkGroundPoint_ && rigidBody()->isEnvironment())
			safe_cast<RigidBodyEnvironment*>(rigidBody())->setPointAreaAnalize();
		rigidBody()->setBoundCheck(ptBoundCheck_);
	} else {
		if(rigidBody()) {
			delete rigidBody_;
			rigidBody_ = 0;
		}
	}

	if(lighted_)
		model()->clearAttribute(ATTRUNKOBJ_NOLIGHT);
	else
		model()->setAttribute(ATTRUNKOBJ_NOLIGHT);

	dayQuant(false);

	if(hideByDistance)
		model()->setAttribute(ATTRUNKOBJ_HIDE_BY_DISTANCE);

	if(permanentColor_ != UnitColor::ZERO)
		UnitColorEffective(permanentColor_).apply(model(), 1.f);
	SetLodDistance(model(),lodDistance_);
	if(!attached)
		model()->Attach();
}

void UnitEnvironmentBuilding::collision(UnitBase* p, const ContactInfo& contactInfo)
{
	xassert(!dead());
	__super::collision(p, contactInfo);

	xassert(!dead());
	
	if(p->attr().isEnvironment() || !(environmentType_ & p->attr().environmentDestruction))
		return;

	switch(environmentType_){
	case ENVIRONMENT_BARN:
	case ENVIRONMENT_BUILDING:
	case ENVIRONMENT_BIG_BUILDING:
		explode();
		Kill();
		break;
	}
}

void UnitEnvironmentBuilding::explode()
{
	if(deathAttr().explodeReference->enableExplode && model())
		universe()->crashSystem->addCrashModel(deathAttr(), model(), position(), lastContactPoint_, lastContactWeight_, GlobalAttributes::instance().debrisLyingTime);
		
	__super::explode();
}

void UnitEnvironmentBuilding::Kill()
{
	if(model())
		streamLogicPostCommand.set(fCommandSetIgnored, model()) << true;

	__super::Kill();
}

void UnitEnvironmentBuilding::setChain(const AnimationChain* chain)
{
	chainControllers_.setChain(chain, MovementState::DEFAULT);
}

const AnimationChain* UnitEnvironmentBuilding::getChain(int animationGroup) const
{
	return chainControllers_.getChain(animationGroup);
}

void UnitEnvironmentBuilding::fowQuant()
{
	FogOfWarMap* fow = universe()->activePlayer()->fogOfWarMap();
	if(!fow || !model())
		return;

	if(!terScene->IsFogOfWarEnabled() && !(isUnderEditor() && !editorVisual().isVisible(objectClass()))){
		if(model()->getAttribute(ATTRUNKOBJ_IGNORE))
			streamLogicCommand.set(fCommandSetIgnored, model()) << false;
		return;
	}

	switch(attr().fow_mode){
	case FVM_HISTORY_TRACK:
		switch(fow->getFogState(position2D().xi(), position2D().yi())){
		case FOGST_NONE:
			streamLogicCommand.set(fCommandSetIgnored, model()) << false;
			break;
		case FOGST_HALF:
			if (!model()->getAttribute(ATTRUNKOBJ_IGNORE)){
				universe()->addFowModel(model());
				streamLogicCommand.set(fCommandSetIgnored, model()) << true;
			}
			break;
		case FOGST_FULL:
			streamLogicCommand.set(fCommandSetIgnored, model()) << true;
			break;
		}
		break;
	case FVM_NO_FOG:
		if (fow->getFogState(position2D().xi(), position2D().yi())==FOGST_NONE)
			streamLogicCommand.set(fCommandSetIgnored, model()) << false;
		else
			streamLogicCommand.set(fCommandSetIgnored, model()) << true;
		break;
	}
}
void UnitEnvironmentBuilding::mapUpdate( float x0, float y0, float x1, float y1 )
{
	__super::mapUpdate(x0, y0, x1, y1);

	if(rigidBody() && rigidBody()->cosTheta() <= deviationCosMin_){
		explode();
		Kill();
	}
}
