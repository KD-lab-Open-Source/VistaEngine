#include "StdAfx.h"
#include "UnitPad.h"
#include "Player.h"
#include "universe.h"
#include "Serialization\SerializationFactory.h"

UNIT_LINK_GET(UnitPad)

DECLARE_SEGMENT(UnitPad)
REGISTER_CLASS(AttributeBase, AttributePad, "Лапа");
REGISTER_CLASS(UnitBase, UnitPad, "Лапа")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PAD, UnitPad)

UNIT_LINK_GET(UnitPlayer);

REGISTER_CLASS(AttributeBase, AttributePlayer, "Юнит-игрок");
REGISTER_CLASS(UnitBase, UnitPlayer, "Юнит-игрок")
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_PLAYER, UnitPlayer)

AttributePad::AttributePad()
{
	unitAttackClass = ATTACK_CLASS_LIGHT;
	collisionGroup = COLLISION_GROUP_REAL;
	rigidBodyPrm = RigidBodyPrmReference("Unit Pad");

	unitClass_ = UNIT_CLASS_PAD;

	accountingNumber = 0;
}

// --------------------------------------------------------------------------

UnitPad::UnitPad(const UnitTemplate& data)
: UnitActing(data)
{
	stateController_.initialize(this, UnitPadPosibleStates::instance());
}

void AttributePad::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(nodes, "nodes", "Узлы линковки для юнитов при переноске");
}

// --------------------------------------------------------------------------

AttributePlayer::AttributePlayer()
{
	padRespawnTime = 5.f;
	accountingNumber = 0;
}

void AttributePlayer::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(padRespawnTime, "padRespawnTime", "Время восстановления лапы");
	
	if(ar.isInput()){
		unitClass_ = UNIT_CLASS_PLAYER;
		minimapSymbolType_ = UI_MINIMAP_SYMBOLTYPE_NONE;
		modelName = "Scripts\\Resource\\Models\\PlayerUnit.3dx";
	}
}

// --------------------------------------------------------------------------

UnitPlayer::UnitPlayer(const UnitTemplate& data) : UnitActing(data)
{
	setCollisionGroup(0);
	hide(HIDE_BY_TRIGGER, true);

	currentCapacity_ = 0.f;
	padWeaponId_ = 0;
	padSelfDestroyed_ = false;
}

void UnitPlayer::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	destroyPad(0);

	ar.serialize(currentCapacity_, "currentCapacity", 0);
	ar.serialize(padPrm_, "padPrm", 0);
	ar.serialize(padSelfDestroyed_, "padSelfDestroyed", 0);
	ar.serialize(padWaitRespawn_, "padWaitRespawn", "padWaitRespawn_");
}

void UnitPlayer::executeCommand(const UnitCommand& command)
{
	switch(command.commandID()){
	case COMMAND_ID_ATTACK:
		selectWeapon(command.commandData());
		setTargetPoint(command.position());
		break;
	case COMMAND_ID_STOP:
		fireStop();
		break;
	}	

	__super::executeCommand(command);
}

void UnitPlayer::validatePad()
{
	if(!pad_ && padWeaponId_){ // отстрелили лапу
		padWeaponId_ = 0;
		padWaitRespawn_.start(attr().padRespawnTime * 1000);

		padSelfDestroyed_ = false;
		currentCapacity_ = 0.f;
	}
}

void UnitPlayer::Quant()
{
	validatePad();

	__super::Quant();
}

UnitPad* UnitPlayer::createPad(int weaponid, const AttributePad* attr)
{
	xassert(weaponid);
	
	validatePad();

	if(padWaitRespawn_.busy()){
		xassert(!pad_);
		return 0;
	}

	padWeaponId_ = weaponid;
	
	if(pad_){
		if(attr != &pad_->attr())
			destroyPad(0);
		else
			return pad_;
	}

	UnitPad* pad = safe_cast<UnitPad*>(player()->buildUnit(attr));
	pad->setPose(Se3f(QuatF::ID, To3D(player()->shootPoint2D())), true);
	pad->makeStaticXY(UnitActing::STATIC_DUE_TO_TRIGGER);
	if(!padPrm_.empty() && padSelfDestroyed_)
		pad->setParameters(padPrm_);

	xassert(pad);
	pad_ = pad;
	return pad;
}

void UnitPlayer::destroyPad(int weaponid)
{
	if(pad_ && (!weaponid || weaponid == padWeaponId_)){
		padSelfDestroyed_ = true;
		padPrm_ = pad_->getParameters();
		pad_->Kill();
		pad_ = 0;
		padWeaponId_ = 0;
	}
}

