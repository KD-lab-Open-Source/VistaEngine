#ifndef __PERIMETER_GENERIC_CONTROLS_
#define __PERIMETER_GENERIC_CONTROLS_

#include "UnitAttribute.h"
#include "Factory.h"
#include "Animation.h"
#include "EffectController.h"
#include "Timers.h"
#include "BaseUniverseObject.h"
#include "SwapVector.h"
#include "Grid2D.h"

class RigidBodyBase;
struct ContactInfo;
class Player;
class Archive;
class WeaponTarget;
class InventorySet;

/// ������������ ��������� ����� - �������, �������� � �.�.
class AbnormalState
{
public:
	AbnormalState(const AbnormalStateType* type = 0, UnitBase* ownerUnit = 0) : type_(type), ownerUnit_(ownerUnit) { isActive_ = false; frozen_ = false; useArithmetics_ = 0; }

	bool operator == (const AbnormalStateType* type) const { return type_ == type; }

	const AbnormalStateType* type() const { return type_; }

	void serialize(Archive& ar);

	bool end() const { return !timer_; }
	void stop()  { timer_.stop(); }

	bool activate(const AbnormalStateAttribute& attribute, UnitBase* ownerUnit);
	bool deactivate(UnitBase* ownerUnit);
	bool isActive() const { return isActive_; }

	const WeaponDamage& damage() const { return damage_; }
	bool frozen() const { return frozen_; }
	
	UnitBase* ownerUnit() const { return ownerUnit_; }

private:
	const AbnormalStateType* type_;

	WeaponDamage damage_;
	bool frozen_;

	bool isActive_;
	DurationTimer timer_;

	bool useArithmetics_;
	ParameterArithmetics arithmeticsInv_;

	UnitLink<UnitBase> ownerUnit_;
};

//-----------------------------------------
//		Creation Template
class UnitTemplate
{
public:
	UnitTemplate(const AttributeBase* attribute, Player* player) : attribute_(attribute), player_(player) {}

	const AttributeBase* attribute() const { return attribute_; }
	Player* player() const { return player_; }

private:
	const AttributeBase* attribute_;
	Player* player_;
};

//=============================================================================
// 
//=============================================================================
class UnitBase : public BaseUniverseObject, public GridElementType, public ShareHandleBase
{
public:
	UnitBase(const UnitTemplate& data);
	virtual ~UnitBase();

	virtual void Quant();
	void Kill();
	bool deadQuant() { return (--deadCounter_) > 0; }
	void removeFromUnitGrid();

	virtual bool isRegisteredInRealUnits() { return false; }
	bool alive() const { return alive_; }
	int placedIntoDeleteList() const { return placedIntoDeleteList_; }
	void setPlacedIntoDeleteList() { placedIntoDeleteList_ = true; }

	//----------------------------------------------------

	int collisionGroup() const { return collisionGroup_; }
	void setCollisionGroup(int group) { collisionGroup_ = group; }
	int excludeCollision() const { return attr().excludeCollision; }
	
	virtual UnitBase* ignoredUnit() const { return 0; }

	virtual void collision(UnitBase* p, const ContactInfo& contactInfo);
	void testRealCollision();
    
	virtual bool isUnseen() const { return false; }

	virtual bool checkInPathTracking(const UnitBase* tracker) const { return attr().enablePathTracking; } 

	void computeTargetPosition(Vect3f& targetPosition) const;
	
	//-----------------------------------------------------

	bool transparentMode() { return attr().transparent_mode; }
	virtual void updateSkinColor() {}

	//-----------------------------------------------------
	// ��������
	virtual void showEditor();

	virtual void refreshAttribute() {}
	virtual void showDebugInfo();
	void showPathTrackingMap();

	//-----------------------------------------------------
	//	����������
	virtual void setPose(const Se3f& pose, bool initPose); // true - ������������� - ����������� � ������ ��� � ���������� z
														   // �������� ������� ������������ ���, ��� ��������� ������� - ������ short-cuts	
	float angleZ() const; // slow!!!

	float minimapRadius() const { return radius_ * attr().minimapScale_; }

	//-----------------------------------------------------
	// �����������

	virtual cObject3dx* model() const { return 0; }
	virtual c3dx* get3dx() const { return 0; }
	
	float height() const { return height_; } // ����������� �������������� �� ������

	virtual int modelNodeIndex(const char* node_name) const { return -1; }
	virtual void setModelNodeTransform(int node_index, const Se3f& pos) {}

	/// ����� �������
	bool startEffect(const EffectAttributeAttachable* effect, bool unique = true, int node = -1, int life_time = 0);
	void startPermanentEffects();
	bool stopEffect(const EffectAttributeAttachable* effect);
	void stopPermanentEffects();
	void updateEffects();
	virtual bool animationChainEffectMode() { return false; }
	void stopAllEffects();

	bool startSoundEffect(const SoundAttribute* sound);
	void stopSoundEffect(const SoundAttribute* sound);

	virtual void explode();

	bool createExplosionSources(const Vect2f& center);
	virtual bool isExplosionSourcesEnabled() const { return true; }
	virtual AffectMode explosionAffectMode() const { return AFFECT_ENEMY_UNITS; }
	virtual const WeaponPrmCache* explosionParameters() const { return 0; }

	virtual bool setAbnormalState(const AbnormalStateAttribute& state, UnitBase* ownerUnit);
	virtual bool hasAbnormalState(const AbnormalStateAttribute& state) const;
	virtual const AbnormalStateType* abnormalStateType() const;

	//-----------------------------------------------------
	// Damage System
	virtual float health() const { return 1; }
	
	virtual void setDamage(const ParameterSet& damage, UnitBase* agressor, const ContactInfo* contactInfo = 0) {}

	const DeathAttribute& deathAttr() const;

	//-----------------------------------------------------
	RigidBodyBase* rigidBody() const { return rigidBody_; }

	virtual bool isDocked() const { return false; }
	 
	//-----------------------------------------------------

	int unitAttackClass() const { return unitAttackClass_; }
	void setUnitAttackClass(AttackClass unitAttackClass){ unitAttackClass_ = unitAttackClass; }

	virtual bool canAttackTarget(const WeaponTarget& target, bool check_fow = false) const { return false; }

	virtual float fireRadius() const { return 0; }
	virtual float fireRadiusMin() const { return 0; }
	virtual float sightRadius() const { return 300; }

	//------------------------------------------------

	virtual void mapUpdate(float x0,float y0,float x1,float y1); // ������� �� ��������� ����������� ����� ��� ���� 

	//---------------------------------------------
	virtual void graphQuant(float dt) {}

	bool isEnemy(const UnitBase* unit) const; // this should attack unit

	virtual void setActivity(bool activate) {} // To activate/deactivates some props by triggers
	virtual bool activity() const { return true; } 
	
	Player* player() const { return player_; }
	void setPlayer(Player* player);
	virtual void changeUnitOwner(Player* player);

	//-----------------------------------------
	virtual const HarmAttribute& harmAttr() const { return attr().harmAttr; }
	
	float possibleDamage() const { return possibleDamage_; }
	void clearPossibleDamage(){ possibleDamage_ = 0; }
	void addPossibleDamage(float damage_delta){ possibleDamage_ += damage_delta; if(possibleDamage_ < 0) possibleDamage_ = 0; }

	//-----------------------------------------
	const AttributeBase& attr() const { return *attr_; }

	void serialize(Archive& ar);

	virtual void relaxLoading() {}

	static UnitBase* create(const UnitTemplate& data);

	const Vect3f& lastContactPoint() { return lastContactPoint_; }
	float lastContactWeight() { return lastContactWeight_; }

	void setColor(const UnitColor& clr);
	void setOpacity(float op);
	float opacity() const { return color_.color.a / 255.f; }
	virtual const UnitColor& defaultColor() const { return UnitColor::ZERO; }

	void setAuxiliary(bool auxiliary) { auxiliary_ = auxiliary; }
	bool auxiliary() const { return auxiliary_; }
	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_UNIT; }
protected:
	float height_;
	bool poseInited_;
	bool alive_;
	char deadCounter_;

	Vect3f lastContactPoint_;
	float lastContactWeight_;

	RigidBodyBase* rigidBody_;

	void SetLodDistance(c3dx* p3dx,enum ObjectLodPredefinedType lodDistance);
	int numLodDistance;
private:
	const AttributeBase* attr_;
	Player* player_;

	bool placedIntoDeleteList_;
	/// ��������������� ����: ���������� �� ����(��� ���������)
	bool auxiliary_;

	int collisionGroup_;

	AttackClass unitAttackClass_;

	enum {
		COLOR_CHANGE_PHASE_SET = 1,
		COLOR_CHANGE_PHASE_RELAX = 2
	};
	/// \a true ���� ����������� ��������� �����
	unsigned char colorChange_;
	/// ����������� ����
	UnitColorEffective color_;
	/// ���� ��������� �����, 0 - ���� ��-���������, 1 - ���� �� \a colors_
	float colorPhase_;

	typedef SwapVector<UnitEffectController> EffectControllers;
	EffectControllers effectControllers_;

	typedef SwapVector<AbnormalState> AbnormalStates;
	AbnormalStates abnormalStates_;

	float possibleDamage_;
	DurationTimer pfUnitMapTimer_;

};

/////////////////////////////////////////////
class UnitCreatorBase
{
public:
	static void setPlayer(Player* player) { player_ = player; }
	static void setAttribute(const AttributeBase* attribute) { attribute_ = attribute; }

protected: 
	static Player* player_;
	static const AttributeBase* attribute_;
};

template<class Derived>
class ObjectCreator<UnitBase, Derived> : public UnitCreatorBase
{
public:
	static UnitBase* create() { 
		if(!attribute_)
			return 0;
		UnitBase* unit = 0;
		if(!strcmp(typeid(Derived).name(), UnitFactory::instance().typeName(attribute_->unitClass()))){
			unit = new Derived(UnitTemplate(attribute_, player_));
		}
		else{
			xassertStr(0 && "����� ������ �����", 
				(string(typeid(Derived).name()) + " -> " + UnitFactory::instance().typeName(attribute_->unitClass())).c_str());
			unit = UnitBase::create(UnitTemplate(attribute_, player_)); 
			return unit;
		}
		if(!unit->dead()){
			player_->addUnit(unit);
			return unit;
		}
		else{
			delete unit;
			return 0;
		}
	}
};

/*
struct UnitArguments : Arguments2<Player*, const AttributeBase*>{
	template<class Derived>
	static Derived* passToNew(UnitArguments args){
		Player* player = args.arg1;
		const AttributeBase* attribute = args.arg2;

		if(!attribute)
			return 0;
		UnitBase* unit = 0;
		if(!strcmp(typeid(Derived).name(), UnitFactory::instance().typeName(attribute->unitClass()))){
			unit = new Derived(UnitTemplate(attribute, player));
		}
		else{
			xassertStr(0 && "����� ������ �����", 
				(string(typeid(Derived).name()) + " -> " + UnitFactory::instance().typeName(attribute->unitClass())).c_str());
			unit = UnitBase::create(UnitTemplate(attribute, player)); 
			return unit;
		}
		if(!unit->dead()){
			player->addUnit(unit);
			return unit;
		}
		else{
			delete unit;
			return 0;
		}
	}
};


typedef Factory<UnitClass, UnitBase, UnitArguments> UnitFactory;
*/

typedef Factory<UnitClass, UnitBase> UnitFactory;

template<class Unit>
class UnitSerializer
{
public:
	UnitSerializer(Unit* unit = 0) : unit_(unit) {}
	operator Unit* () const { return unit_; }
	Unit* operator->() const { return unit_; }
	Unit& operator*() const { return *unit_; }

	void serialize(Archive& ar) {
		AttributeType attributeType = unit_ ? unit_->attr().attributeType() : ATTRIBUTE_NONE;
		ar.serialize(attributeType, "attributeType", 0);
		switch(attributeType){
		case ATTRIBUTE_LIBRARY: {
			AttributeReference attribute = unit_ ? &unit_->attr() : 0;
			ar.serialize(attribute, "attribute", 0);
			UnitCreatorBase::setAttribute(attribute);
			xassertStr(ar.isOutput() || attribute, (string("�� ������ ���� � ����������: ") + attribute.c_str()).c_str());
			break; }
		case ATTRIBUTE_AUX_LIBRARY: {
			AuxAttributeReference auxAttribute = unit_ ? &unit_->attr() : 0;
			ar.serialize(auxAttribute, "auxAttribute", 0);
			UnitCreatorBase::setAttribute(auxAttribute);
			xassertStr(ar.isOutput() || auxAttribute, (string("�� ������ ���� � ����������: ") + auxAttribute.c_str()).c_str());
			break; }
		case ATTRIBUTE_SQUAD: {
			AttributeSquadReference attributeSquad(unit_ ? safe_cast_ref<const AttributeSquad&>(unit_->attr()).c_str() : "");
			ar.serialize(attributeSquad, "attributeSquad", 0);
			UnitCreatorBase::setAttribute(&*attributeSquad);
			break; }
  		case ATTRIBUTE_PROJECTILE: {
			AttributeProjectileReference attributeProjectile(unit_ ? safe_cast_ref<const AttributeProjectile&>(unit_->attr()).c_str() : "");
			ar.serialize(attributeProjectile, "attributeProjectile", 0);
			UnitCreatorBase::setAttribute(&*attributeProjectile);
			break; }
  		case ATTRIBUTE_NONE: 
			xassert(0 && "����������� ���� �� ����");
			break; 
		}

		PolymorphicWrapper<UnitBase> unitBase = unit_;
   		ar.serialize(unitBase, "unit", 0);
		unit_ = safe_cast<Unit*>(unitBase.get());
	}

    Unit*& unit() { return unit_; }

private:
	Unit* unit_;
};

typedef SwapVector<UnitSerializer<UnitBase> > UnitList; 

#endif
