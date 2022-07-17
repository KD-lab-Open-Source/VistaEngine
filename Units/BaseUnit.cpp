#include "StdAfx.h"
#include "..\Game\Universe.h"
#include "..\Environment\Environment.h"
#include "Interpolation.h"
#include "Sound.h"
#include "SoundApp.h"
#include "terra.h"
#include "ExternalShow.h"
#include "RenderObjects.h"
#include "Serialization.h"
#include "TransparentTracking.h"
#include "UnitObjective.h"
#include "PFTrap.h"
#include "GlobalAttributes.h"

UNIT_LINK_GET(BaseUniverseObject)
UNIT_LINK_GET(const BaseUniverseObject)
UNIT_LINK_GET(UnitBase)

REGISTER_CLASS(UnitBase, UnitBase, "Базовый юнит");

FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ITEM_INVENTORY, UnitItemInventory)
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ITEM_RESOURCE, UnitItemResource)
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_REAL, UnitReal)
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_BUILDING, UnitBuilding)
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE, ProjectileBase);
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE_BULLET, ProjectileBullet);
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PROJECTILE_MISSILE, ProjectileMissile);
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_LEGIONARY, UnitLegionary)
//FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_TRAIL, UnitTrail)
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ENVIRONMENT, UnitEnvironmentBuilding);
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ENVIRONMENT_SIMPLE, UnitEnvironmentSimple);
FORCE_REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_SQUAD, UnitSquad);

REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ZONE, UnitBase)

Player* UnitCreatorBase::player_;
const AttributeBase* UnitCreatorBase::attribute_; 

/////////////////////////////////////////////////
UnitBase::UnitBase(const UnitTemplate& data) 
{
	unitID_.registerUnit(this);

	attr_ = data.attribute();
	player_ = data.player();
	placedIntoDeleteList_ = false;
	auxiliary_ = false;
	alive_ = true;

	height_ = 0;

	collisionGroup_ = attr().collisionGroup;

	colorChange_ = 0;
	colorPhase_ = 0.0f;

	radius_ = 1;
	rigidBody_ = 0;

	poseInited_ = false;
	pose_ = Se3f::ID;

 	possibleDamage_ = 0;

	unitAttackClass_ = attr().unitAttackClass;

	if(universe()->GetTransparentTracking())
		universe()->GetTransparentTracking()->RegisterUnit(this);

	lastContactPoint_ = Vect3f::ZERO;
	lastContactWeight_ = 0;

	pfUnitMapTimer_.start(logicRND(2000));

	numLodDistance = 0;

	deadCounter_ = 3;
}

UnitBase::~UnitBase()
{
	if(universe()->GetTransparentTracking())
		universe()->GetTransparentTracking()->UnRegisterUnit(this);

	if(rigidBody_){
		delete rigidBody_;
		rigidBody_ = 0;
	}

}

void UnitBase::removeFromUnitGrid()
{
	if(inserted()){
		MTAuto lock(universe()->unitGridLock);
		universe()->unitGrid.Remove(*this);
	}
}

void UnitBase::setPlayer(Player* player) 
{ 
	player_ = player; 
}

void UnitBase::serialize(Archive& ar) 
{
	BaseUniverseObject::serialize(ar);
	
	if(dead())
		alive_ = false;

	ar.serialize(pose_, "pose", 0);
	ar.serialize(abnormalStates_, "anbormalStates", 0);
	if(universe()->userSave())
		ar.serialize(collisionGroup_, "collisionGroup", 0);
}

void fCommandSetColor(XBuffer& stream)
{
	cObject3dx* obj;
	stream.read(obj);
	UnitColorEffective clr;
	stream.read(clr);
	float phase;
	stream.read(phase);
	clr.apply(obj, phase);
}

void UnitBase::setColor(const UnitColor& clr)
{
	if(colorChange_ == COLOR_CHANGE_PHASE_SET)
		color_.setColor(clr, false);
	else {
		color_.setColor(clr, true);
		colorChange_ = COLOR_CHANGE_PHASE_SET;
	}
}

void UnitBase::setOpacity(float op)
{
	if(colorChange_ == COLOR_CHANGE_PHASE_SET)
		color_.setOpacity(op, false);
	else {
		color_.setOpacity(op, true);
		colorChange_ = COLOR_CHANGE_PHASE_SET;
	}
}

void UnitBase::Quant()
{
	if(!pfUnitMapTimer_) {
		pathFinder->projectUnit(this);
		pfUnitMapTimer_.start(2900);
	}

	if(alive() && health() < FLT_EPS){
		explode();
		Kill();
		return;
	}

	if(colorChange_){
		if(cObject3dx* obj = model()){
			average(colorPhase_, (colorChange_ == COLOR_CHANGE_PHASE_SET) ? 1.f : 0.f, 0.15f);
			if(colorChange_ == COLOR_CHANGE_PHASE_RELAX && colorPhase_ < 0.002f){
				colorChange_ = 0;
				colorPhase_ = 0.f;
				color_.setColor(defaultColor(), true);
			}
			else
				colorChange_ = COLOR_CHANGE_PHASE_RELAX;
			streamLogicCommand.set(fCommandSetColor) << obj << color_ << colorPhase_;
		}
		else
			colorChange_ = 0;
	}

	for(EffectControllers::iterator ie = effectControllers_.begin(); ie != effectControllers_.end();){
		ie->logicQuant(0.f);
		if(ie->killTimer()){
			ie->release();
			ie = effectControllers_.erase(ie);
		}
		else
			++ie;
	}

	if(!alive())
		return;

	for(AbnormalStates::iterator it = abnormalStates_.begin(); it != abnormalStates_.end();){
		if(const AbnormalStateEffect* eff = harmAttr().abnormalStateEffect(it->type())){
			EffectControllers::iterator eff_it = std::find(effectControllers_.begin(),
				effectControllers_.end(), &eff->effectAttribute());

			//if(!environment->soundIsPlaying(eff->soundAttribute(), this))
			//	stopSoundEffect(eff->soundAttribute());

			if(eff_it != effectControllers_.end()){
				if(eff_it->isSwitchedOff()){
					stopEffect(&eff->effectAttribute());
					it->deactivate(this);
				}
			}

			if(it->isActive()){
				// цвет держится лишний квант чтобы избежать мигания
				if(eff->needColorChange())
					setColor(eff->color());
				if(it->end()){
					stopEffect(&eff->effectAttribute());
					stopSoundEffect(eff->soundAttribute());
					it->deactivate(this);
				}
				else {
					setDamage(it->damage(), it->ownerUnit());
					if(it->frozen() && attr().isReal())
						safe_cast<UnitReal*>(this)->freezeAnimation();

					if(!alive())
						return;
				}
			}
			else
				stopSoundEffect(eff->soundAttribute());
		}

		if(!it->isActive())
			it = abnormalStates_.erase(it);
		else
			++it;
	}
}

void UnitBase::Kill()
{
	MTL();

	BaseUniverseObject::Kill();

	alive_ = false;

	stopAllEffects();
}

const DeathAttribute& UnitBase::deathAttr() const
{
	return harmAttr().deathAttribute(abnormalStateType());
}

bool UnitBase::isEnemy(const UnitBase* unit) const 
{ 
	return player()->isEnemy(unit);
}

void UnitBase::setPose(const Se3f& pose, bool initPose) 
{
	pose_.trans() = clampWorldPosition(pose.trans(), radius() + 2);
	pose_.rot() = pose.rot();
	//log_var(pose);

	if(initPose)
		poseInited_ = true;
	else
		xassert(poseInited_);

	if(collisionGroup()){
		MTAuto lock(universe()->unitGridLock);
		if(!universe()->unitGrid.scanned()){
			if(inserted())
				universe()->unitGrid.Move(*this, round(position().x), round(position().y), round(radius()));
			else
				universe()->unitGrid.Insert(*this, round(position().x), round(position().y), round(radius()));
		}
	}
}

void UnitBase::changeUnitOwner(Player* playerIn)
{
	xassert(playerIn != player());
	player()->removeUnit(this);
	unitID_.unregisterUnit();
	unitID_.registerUnit(this);
	playerIn->addUnit(this);

	AbnormalStates::iterator it;
	FOR_EACH(abnormalStates_, it)
		it->stop();

	stopPermanentEffects();
	startPermanentEffects();
}

float UnitBase::angleZ() const 
{ 
	if(orientation().z() > 0)
		return orientation().angle(); 
	else{
		QuatF q = orientation();
		q.negate();
		return q.angle(); 
	}
} 

void UnitBase::mapUpdate(float x0,float y0,float x1,float y1) 
{
	if(rigidBody())
		rigidBody()->awake();
}

void UnitBase::collision(UnitBase* p, const ContactInfo& contactInfo) 
{
	if(rigidBody())
		rigidBody()->awake();

	lastContactPoint_ = contactInfo.collisionPoint(this);
	lastContactWeight_ = p->attr().contactWeight;
}

void UnitBase::computeTargetPosition(Vect3f& targetPosition) const 
{ 
	if(rigidBody_)
		targetPosition = rigidBody_->centreOfGravity();
	else{
		targetPosition = position();
		targetPosition.z += height()/2.f;
	}
}

//--------------------------------------------------
void UnitBase::showDebugInfo()
{
	if(showDebugUnitBase.radius){
		show_vector(position(), 1, GREEN);
		show_vector(position(), radius(), GREEN);
	}
	
	if(showDebugUnitBase.modelName)
		show_text(position(), attr().modelName.c_str(), RED);

	if(showDebugUnitBase.libraryKey)
		show_text(position(), attr().libraryKey(), GREEN);

	if(showDebugUnitBase.abnormalState && abnormalStateType()){
		XBuffer msg;
		msg.SetDigits(2);

		AbnormalStates::iterator it;
		FOR_EACH(abnormalStates_, it)
			if(it->isActive())
				msg < it->type()->c_str() < "\n";

		show_text(position(), msg, RED);
	}

	if(showDebugUnitBase.effects){
		XBuffer msg;
		//msg.SetDigits(2);
		msg < "N Effects: " <= effectControllers_.size();
		show_text(position()+Vect3f(0, 0, 15), msg, BLUE);
		
		EffectControllers::iterator ie;
		FOR_EACH(effectControllers_, ie)
			ie->showDebugInfo();
	}
	
	if(showDebugUnitBase.clan){
		XBuffer msg(256, 1);
		msg < "Player: " <= player()->playerID() < ", clan: " <= player()->clan();
		show_text(position(), msg, RED);
	}

	if(showDebugUnitBase.producedPlacementZone)
		show_vector(position(), attr().producedPlacementZoneRadius, YELLOW);

	if(show_pathtracking_map_for_selected)
		showPathTrackingMap();
}

UnitBase* UnitBase::create(const UnitTemplate& data)
{
	UnitCreatorBase::setPlayer(data.player());
	UnitCreatorBase::setAttribute(data.attribute());
	UnitBase* unit = UnitFactory::instance().create(data.attribute()->unitClass());
	return unit;
}

bool UnitBase::startSoundEffect(const SoundAttribute* sound)
{
	return environment->soundAttach(sound, this);
}

bool UnitBase::startEffect(const EffectAttributeAttachable* effect, bool unique, int node, int life_time)
{
	MTL();
	xassert(effect);

	if(dead() || effect->isEmpty())
		return false;

	if(unique){
		EffectControllers::const_iterator it = std::find(effectControllers_.begin(),
			effectControllers_.end(), effect);

		if(it != effectControllers_.end())
			return false;
	}

	effectControllers_.push_back(UnitEffectController(this));
	UnitEffectController& ctrl = effectControllers_.back();

	const float one_size_model=40.0f;
	ctrl.setEffectNode(node);
	ctrl.setPlacementMode(EffectController::PLACEMENT_BY_ATTRIBUTE);
	bool onPause = effect->switchOffByInterface() && !universe()->interfaceEnabled()
		|| effect->switchOffByAnimationChain() && animationChainEffectMode();
	ctrl.setPause(onPause);
	ctrl.effectStart(effect, effect->scaleByModel() ? height()/one_size_model : 1.f, effect->legionColor() ? player()->unitColor() : sColor4c(255,255,255,255));
	if(life_time)
		ctrl.setKillTimer(life_time);

	return true;
}

void UnitBase::startPermanentEffects()
{
	EffectAttributes::const_iterator it;
	FOR_EACH(attr().permanentEffects, it)
		startEffect(&*it);
}

void UnitBase::stopSoundEffect(const SoundAttribute* sound)
{
	environment->soundRelease(sound, this);
}

bool UnitBase::stopEffect(const EffectAttributeAttachable* effect)
{
	xassert(effect);
	if(effect->isEmpty())
		return false;

	EffectControllers::iterator it = std::find(effectControllers_.begin(),
		effectControllers_.end(), effect);

	if(it != effectControllers_.end()){
		it->release();
		effectControllers_.erase(it);
		return true;
	}

	return false;
}


void UnitBase::stopPermanentEffects()
{
	EffectAttributes::const_iterator it;
	FOR_EACH(attr().permanentEffects, it)
		stopEffect(&*it);
}

void UnitBase::updateEffects()
{
	bool interfacePause = !universe()->interfaceEnabled();
	bool animationChainPause = animationChainEffectMode();
	EffectControllers::iterator it = effectControllers_.begin();
	FOR_EACH(effectControllers_, it)
		if(it->isEnabled()){
			bool onPause = it->attr()->switchOffByInterface() && interfacePause
				|| it->attr()->switchOffByAnimationChain() && animationChainPause;
			it->setPause(onPause);
		}
}

void UnitBase::stopAllEffects()
{
	EffectControllers::iterator ie;
	FOR_EACH(effectControllers_, ie)
		ie->release();

	effectControllers_.clear();
}

bool UnitBase::setAbnormalState(const AbnormalStateAttribute& state, UnitBase* ownerUnit)
{
	if(!alive())
		return false;

	if(const AbnormalStateEffect* eff = harmAttr().abnormalStateEffect(state.type())){
		AbnormalStates::iterator it = std::find(abnormalStates_.begin(), abnormalStates_.end(), state.type());
		if(it == abnormalStates_.end()){
			abnormalStates_.push_back(AbnormalState(state.type(), ownerUnit));

			if(abnormalStates_.back().activate(state, this)){
				startEffect(&eff->effectAttribute());
				startSoundEffect(eff->soundAttribute());
			}
		}
	}

	return false;
}

bool UnitBase::hasAbnormalState(const AbnormalStateAttribute& state) const
{
	return harmAttr().abnormalStateEffect(state.type()) != 0;
}

const AbnormalStateType* UnitBase::abnormalStateType() const
{
	const AbnormalStateType* type = 0;
	AbnormalStates::const_iterator it;
	FOR_EACH(abnormalStates_, it)
		if(it->isActive() && (!type || type->priority < it->type()->priority))
			type = it->type();

	return type;
}

//--------------------------------------------------

void AbnormalState::serialize(Archive& ar)
{
	AbnormalStateTypeReference ref(type_ ? type_->c_str() : "");
	ar.serialize(ref, "type", 0);

	if(ar.isInput())
		type_ = ref;

	ar.serialize(isActive_, "isActive", 0);
	ar.serialize(timer_, "timer", 0);
	ar.serialize(damage_, "damage", 0);
	ar.serialize(frozen_, "frozen", 0);
	ar.serialize(useArithmetics_, "useArithmetics", 0);
	ar.serialize(arithmeticsInv_, "arithmeticsInv", 0);
}

bool AbnormalState::activate(const AbnormalStateAttribute& attribute, UnitBase* ownerUnit)
{
	int time = round((attribute.duration() + logicRNDfrnd(1.f)*attribute.durationRnd()) * 1000.0f);
	if(time <= 0) 
		return false;

	timer_.start(time);
	damage_ = attribute.damage();
	frozen_ = attribute.freeze();
	isActive_ = true;

	if((useArithmetics_ = attribute.useArithmetics()) != 0){
		arithmeticsInv_ = attribute.arithmetics();
		arithmeticsInv_.setInvertOnApply();
		safe_cast<UnitObjective*>(ownerUnit)->applyParameterArithmetics(arithmeticsInv_);
	}

	return true;
}

bool AbnormalState::deactivate(UnitBase* ownerUnit)
{
	isActive_ = false;

	if(useArithmetics_){
		arithmeticsInv_.setInverted();
		safe_cast<UnitObjective*>(ownerUnit)->applyParameterArithmetics(arithmeticsInv_);
	}

	return true;
}

void UnitBase::explode()
{
	xassert(!dead());
	if(!deathAttr().explodeReference->animatedDeath){
		if(isExplosionSourcesEnabled())
			createExplosionSources(position2D());
	}
}										

bool UnitBase::createExplosionSources(const Vect2f& center)
{
	const WeaponPrmCache* explosion_prm = explosionParameters();

	for(int i = 0; i < deathAttr().sources.size(); i++){
		if(!deathAttr().sources[i].isEmpty()){
			Se3f pos = Se3f(QuatF::ID, To3D(center + deathAttr().sources[i].positionDelta()));
			if(SourceBase* source = environment->createSource(&deathAttr().sources[i], pos, false)){
				source->setPlayer(player());
				source->setAffectMode(explosionAffectMode());

				if(explosion_prm && i < explosion_prm->sources().size())
					source->setParameters(explosion_prm->sources()[i]);
			}
		}
	}

	return true;
}

void UnitBase::showEditor()
{
}

//--------------------------------------------------------

struct RealCollisionOperator
{
	Vect3f CenterOfGravity;
	RigidBodyBase* rigidBody;
 	UnitBase* unit_;
 	UnitBase* ignoredUnit_;

	RealCollisionOperator(UnitBase* p)
	{
		unit_ = p;
		rigidBody = p->rigidBody();
		ignoredUnit_ = p->ignoredUnit();
	}

	void operator()(UnitBase* p)
	{
		if(p->alive() && p != unit_ && p != ignoredUnit_ && unit_ != p->ignoredUnit() && 
 		  !(unit_->excludeCollision() & p->excludeCollision())){
			if(p->collisionGroup() & COLLISION_GROUP_COLLIDER){
				RigidBodyBase* b = p->rigidBody();
				if (b) {
					if(!(unit_->attr().isLegionary() && p->attr().isLegionary()) 
						&& !(unit_->attr().isBuilding() && p->attr().isEnvironment())){
						ContactInfo contactInfo;
						if(rigidBody->bodyCollision(b, contactInfo)){
							contactInfo.unit1 = unit_;
							contactInfo.unit2 = p;
							unit_->collision(p, contactInfo);
							p->collision(unit_, contactInfo);
						}
					}
				}
			}
		  }
	}
};

void UnitBase::testRealCollision()
{
	universe()->unitGrid.setAsPassed(*this);
	universe()->unitGrid.Scan(rigidBody()->position().xi(), rigidBody()->position().yi(), round(radius()), RealCollisionOperator(this));
}

void RigidBodyBase::show()
{
	sColor4c color = asleep() ? WHITE : debugColor_;

	if(showDebugRigidBody.mesh && geomType_ == Geom::GEOM_MESH)
		geom_->showDebugInfo(pose());

	if(showDebugRigidBody.boundingBox){
		Vect3f vertex[8];
		box().computeVertex(vertex, centreOfGravity(), orientation());
		
		show_vector(vertex[0], vertex[1], vertex[2], vertex[3], color);
		show_vector(vertex[4], vertex[5], vertex[6], vertex[7], color);
		show_vector(vertex[0], vertex[1], vertex[5], vertex[4], color);
		show_vector(vertex[3], vertex[2], vertex[6], vertex[7], color);
	}

	if(showDebugRigidBody.radius)									
		show_vector(position(), radius(), color);

	if(showDebugRigidBody.boundRadius)
		show_vector(centreOfGravity(), boundRadius(), color);

#ifndef _FINAL_VERSION_
	
	if(showDebugRigidBody.showDebugMessages) {
		XBuffer buf;
		buf <= universe()->quantCounter() < ":";
		string temp = debugMessageList_;
		temp += buf;
		show_text(position(), temp.c_str(), WHITE);
	}

#endif
}

#ifndef _FINAL_VERSION_
void RigidBodyBase::debugMessage(const char* text)
{
	XBuffer buf;
	buf <= universe()->quantCounter() < ": ";
	debugMessageList_ += buf;
	debugMessageList_ += text;
	debugMessageList_ += '\n';

	int numLines = count(debugMessageList_.begin(), debugMessageList_.end(), '\n');
	
	if(numLines > showDebugRigidBody.debugMessagesCount)
		debugMessageList_.erase(0, debugMessageList_.find('\n') + 1);

}
#endif

void UnitBase::SetLodDistance(c3dx* p3dx,enum ObjectLodPredefinedType lodDistance)
{
	numLodDistance=lodDistance;
	if(lodDistance == OBJECT_LOD_DEFAULT)
	{
		numLodDistance=OBJECT_LOD_VERY_BIG;
		float radius;
		if(rigidBody())
		{
			const Vect3f& bound = rigidBody()->box().getExtent();
			radius = (bound.x+bound.y+bound.z)/3;
		}else
		{
			sBox6f bound;
			p3dx->GetBoundBox(bound);
			Vect3f extent(bound.max);
			extent -= bound.min;
			extent *= 0.5f;
			radius = (extent.x+extent.y+extent.z)/3;
		}
		for(int i=OBJECT_LOD_SIZE-1; i>=0; i--)
		{
			if(radius<=GlobalAttributes::instance().lod_border[i].radius)
				numLodDistance = i;
		}
	}
	GlobalAttributes::LodBorder& l=GlobalAttributes::instance().lod_border[numLodDistance];
	p3dx->SetLodDistance(l.lod12,l.lod23);
	p3dx->SetHideDistance(l.hideDistance);
}
