#include "StdAfx.h"
#include "UnitItemInventory.h"
#include "Player.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\SerializationFactory.h"
#include "Game\CameraManager.h"
#include "Render\Src\cCamera.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UI_Font.h"
#include "UserInterface\UI_TextParser.h"
#include "UserInterface\UI_Render.h"
#include "UserInterface\UI_GlobalAttributes.h"

extern class CameraManager* cameraManager;

UNIT_LINK_GET(UnitItemInventory)

DECLARE_SEGMENT(UnitItemInventory)
REGISTER_CLASS(AttributeBase, AttributeItemInventory, "Предмет-инвентарь")
REGISTER_CLASS(UnitBase, UnitItemInventory, "Предмет-инвентарь");
REGISTER_CLASS_IN_FACTORY(UnitFactory, UNIT_CLASS_ITEM_INVENTORY, UnitItemInventory)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AttributeItemInventory, EquipmentType, "Типы снаряжения")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_NONE, "Не снаряжение")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_GENERAL, "Снаряжение")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_WEAPON, "Оружие")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_AMMO, "Боеприпасы")
REGISTER_ENUM_ENCLOSED(AttributeItemInventory, EQUIPMENT_HEALTH, "Аптечка")
END_ENUM_DESCRIPTOR_ENCLOSED(AttributeItemInventory, EquipmentType)

AttributeItemInventory::AttributeItemInventory()
: AttributeBase()
{
	unitClass_ = UNIT_CLASS_ITEM_INVENTORY;
	unitAttackClass = ATTACK_CLASS_LIGHT;
	collisionGroup = COLLISION_GROUP_REAL;
	rigidBodyPrm = RigidBodyPrmReference("Item Inventory");

	stayTime = 5;
	arealRadius = 100;
	sightRadius = 300;

	appearanceDelay = 0;
	useLifeTime = false;
	lifeTime = 60;

	inventorySize = Vect2i(1, 1);
	equipmentType = EQUIPMENT_NONE;

	jumpImpulse = 10.f;
	jumpAngularSpeed = M_PI;

	enablePathTracking = false;

	initialHeightUIParam = 10.f;
}

void AttributeItemInventory::serialize(Archive& ar)
{
    __super::serialize(ar);

	if(ar.openBlock("tips", "подсказки для лежащих на земле")){
		ar.serialize(initialHeightUIParam, "initialHeightUIParam", "высота предмета для вывода значений");
		ar.serialize(tipText, "tipText", "Текст подсказки");
		ar.closeBlock();
	}

	collisionGroup = COLLISION_GROUP_REAL;
	unitClass_ = UNIT_CLASS_ITEM_INVENTORY;
	excludeCollision = unitClass() == UNIT_CLASS_ITEM_INVENTORY ? EXCLUDE_COLLISION_BULLET : 0;

	ar.serialize(parametersInitial, "parametersInitial", "Добавляемые параметры (расходуемые)");
	ar.serialize(parametersArithmetics, "parametersArithmetics", "Арифметика при надевании");

	ar.serialize(inventorySize, "inventorySize", "Занимаемое пространство");
	ar.serialize(inventorySprite, "inventorySprite", "Картинка");
	ar.serialize(quickAccessSprite, "quickAccessSprite", "Картинка для инвентаря быстрого доступа");
	ar.serialize(inventoryCellType, "inventoryCellTypeReference", "Тип ячейки инвентаря");
	ar.serialize(equipmentType, "equipmentType", "Тип снаряжения");

	if(isEquipment()){
		ar.serialize(accessValue, "accessValue", "Необходимые параметры");

		ar.serialize(equipmentSlotType, "equipmentSlotTypeReference", "Слот снаряжения");
		ar.serialize(quickAccessSlotType, "quickAccessSlotType", "Слот быстрого доступа");

		switch(equipmentType){
		case EQUIPMENT_WEAPON:
			ar.serialize(weaponReference, "weaponReference", "Оружие");
			break;
		case EQUIPMENT_AMMO:
			ar.serialize(ammoTypeReference, "ammoTypeReference", "Тип боеприпасов");
			break;
		}
	}

	ar.serialize(appearanceDelay, "appearanceDelay", "Задержка появления");
	ar.serialize(useLifeTime, "useLifeTime", "Ограничивать время жизни");
	if(useLifeTime)
		ar.serialize(lifeTime, "lifeTime", "Время жизни");

	ar.serialize(mass, "mass", "Масса");
	ar.serialize(jumpImpulse, "jumpImpulse", "Импульс прыжка если не влезает в инвентарь");
	ar.serialize(jumpAngularSpeed, "jumpAngularSpeed", "Угловая скорость в прыжке если не влезает в инвентарь");
}

int AttributeItemInventory::getInventoryCellType(UI_InventoryType inventory_type) const
{
	switch(inventory_type){
	case UI_INVENTORY:
		return inventoryCellType->stringIndex();
	case UI_INVENTORY_EQUIPMENT:
		return equipmentSlotType->stringIndex();
	case UI_INVENTORY_QUICK_ACCESS:
		return quickAccessSlotType->stringIndex();
	}

	return 0;
}

UnitItemInventory::UnitItemInventory(const UnitTemplate& data) 
: UnitObjective(data)
{ 
	if(attr().appearanceDelay){
		appearanceTimer_.start(attr().appearanceDelay*1000);
		setCollisionGroup(0);
		streamLogicCommand.set(fCommandSetIgnored, model()) << true;
		stopPermanentEffects();
	}
	if(attr().useLifeTime)
		killTimer_.start((attr().appearanceDelay + attr().lifeTime)*1000);
	
	stateController_.initialize(this, UnitItemPosibleStates::instance());
}

void UnitItemInventory::Quant()
{
	start_timer_auto();

	__super::Quant();

	if(abnormalStateType())
		parameters_.set(0.001f);

	if(appearanceTimer_.finished()){
		appearanceTimer_.stop();
		setCollisionGroup(attr().collisionGroup);
		streamLogicCommand.set(fCommandSetIgnored, model()) << false;
		startPermanentEffects();
	}

	if(killTimer_.finished())
		Kill();
	
}

void UnitItemInventory::graphQuant(float dt)
{
	if(!UI_LogicDispatcher::instance().showItemsHintMode())
		return;
	
	Vect3f pos3d = position();
	pos3d.z += attr().initialHeightUIParam;

	Vect3f scr, w;
	cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos3d, &w, &scr);
	if(w.z > 0){
		wstring text(attr().tipText.c_str());
		UI_LogicDispatcher::instance().expandTextTemplate(text, ExpandInfo(ExpandInfo::UNIT, this, &attr()));
		if(text.empty())
			return;

		const HintAttributes& prm = UI_GlobalAttributes::instance().hintAttributes();

		UI_TextParser parser(prm.tipFont && prm.tipFont->font()
			? prm.tipFont->font()
			: UI_Render::instance().defaultFont()->font());

		const UI_TextFormat& format = signHovered() ? prm.tipTextFormatHover : prm.tipTextFormat;

		parser.parseString(text.c_str(), format.textColor(), UI_Render::instance().screenSize(Vect2f(prm.tipTextMaxWidth, 0)).x);

		// посчитаннуая экранная точка должна лежать на середине нижней границы
		Recti txtpos(Vect2i(scr.x - parser.size().x / 2, scr.y - parser.size().y), parser.size());
		
		Recti realout = UI_Render::instance().windowPosition().intersection(txtpos);
		if(realout.width() < 2 * txtpos.width() / 3 || realout.height() < txtpos.height() / 2)
			return;

		const int borderWidth = 1;
		Recti pos(txtpos);
		pos.left(pos.left() - borderWidth - 1);
		pos.top(pos.top() - borderWidth);
		pos.width(pos.width() + borderWidth * 2 + 1);
		pos.height(pos.height() + borderWidth * 2);
		Rectf border(UI_RenderBase::instance().relativeCoords(pos));
		
		if(prm.tipFill.a > 0)
			UI_Render::instance().drawRectangle(border, prm.tipFill);
		
		UI_Render::instance().outText(UI_Render::instance().relativeCoords(txtpos),
			parser, parser.outNodes().begin(), parser.outNodes().end(),
			&format, UI_TEXT_VALIGN_TOP | UI_TEXT_ALIGN_LEFT, 1.f, true);

		if(prm.tipBorder.a > 0)
			UI_Render::instance().drawRectangle(border, prm.tipBorder, true);

		UI_LogicDispatcher::instance().addSquadSelectSign(pos, this);
	}

	__super::graphQuant(dt);
}

void UnitItemInventory::jump()
{
	RigidBodyUnit* rbody = safe_cast<RigidBodyUnit*>(rigidBody());
	if(rbody->isBoxMode())
		return;

	Vect3f linearPulse(Vect3f::ZERO);
	float mass(rbody->core()->mass());
	linearPulse.z = attr().jumpImpulse * mass;
	Vect3f angularPulse(Vect3f::I);
	orientation().xform(angularPulse);
	angularPulse *= attr().jumpAngularSpeed * mass * sqr(rbody->boundRadius());
	rbody->addImpulse(linearPulse, angularPulse);
	rbody->avoidCollisionAtStart();
	rbody->enableBoxMode();
	rbody->setVelocity(Vect3f::ZERO);
}

void UnitItemInventory::setPose(const Se3f& poseIn, bool initPose)
{
	__super::setPose(poseIn, initPose);

	fowQuant();
}
