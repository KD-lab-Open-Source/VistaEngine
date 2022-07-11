#ifndef __PERIMETER_GENERIC_CONTROLS_
#define __PERIMETER_GENERIC_CONTROLS_

#include "UnitAttribute.h"
#include "Serialization\Factory.h"
#include "Animation.h"
#include "EffectController.h"
#include "Timers.h"
#include "BaseUniverseObject.h"
#include "XTL\SwapVector.h"
#include "Grid2D.h"

class RigidBodyBase;
struct ContactInfo;
class Player;
class Archive;
class WeaponTarget;
class InventorySet;


/// ненормальное состояние юнита - горение, дымление и т.п.
class AbnormalState
{
public:
	AbnormalState(const AbnormalStateType* type = 0, UnitBase* ownerUnit = 0) : type_(type), ownerUnit_(ownerUnit) { isActive_ = false; frozen_ = false; frozenAttack_ = false; useArithmetics_ = 0; }

	bool operator == (const AbnormalStateType* type) const { return type_ == type; }

	const AbnormalStateType* type() const { return type_; }

	void serialize(Archive& ar);

	bool end() const { return !timer_.busy(); }
	void stop()  { timer_.stop(); }

	bool activate(const AbnormalStateAttribute& attribute, UnitBase* ownerUnit);
	bool deactivate(UnitBase* ownerUnit);
	bool isActive() const { return isActive_; }

	const WeaponDamage& damage() const { return damage_; }
	bool frozen() const { return frozen_; }
	bool frozenAttack() const { return frozenAttack_; }
	
	UnitBase* ownerUnit() const { return ownerUnit_; }

private:
	const AbnormalStateType* type_;

	WeaponDamage damage_;
	bool frozen_;
	bool frozenAttack_;

	bool isActive_;
	LogicTimer timer_;

	bool useArithmetics_;
	ParameterArithmetics arithmetics_;

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
	enum HideReason {
		HIDE_BY_FOW = 1,
		HIDE_BY_TELEPORT = 2,
		HIDE_BY_TRANSPORT = 4,
		HIDE_BY_UPGRADE = 8,
		HIDE_BY_TRIGGER = 16,
		HIDE_BY_INITIAL_APPEARANCE = 32,
		HIDE_BY_PLACED_BUILDING = 64,
		HIDE_BY_EDITOR = 128
	};

	UnitBase(const UnitTemplate& data);
	virtual ~UnitBase();

	virtual void Quant();
	void Kill();
	bool deadQuant() { return --deadCounter_ > 0; }
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

	bool canCollide(UnitBase* activeCollider) const;
	virtual void collision(UnitBase* p, const ContactInfo& contactInfo);
	virtual void testCollision();

	bool intersect(const Vect3f& p0,const Vect3f& p1, Vect3f& collision, bool exactCollision = true, float radius = 0.0f) const;
    
	virtual bool isUnseen() const { return false; }
	virtual void dayQuant(bool invisible) {}

	bool isVisibleUnderForOfWar() const;

	virtual bool checkInPathTracking(const UnitBase* tracker) const { return attr().enablePathTracking; } 

	void computeTargetPosition(Vect3f& targetPosition) const;
	
	//-----------------------------------------------------

	virtual bool canBeTransparent() const { return attr().canBeTransparent; }
	virtual void updateSkinColor() {}

	//-----------------------------------------------------
	// Редактор
	virtual void showEditor();

	virtual void showDebugInfo();
	void showPathTrackingMap();

	//-----------------------------------------------------
	//	Координаты
	virtual void setPose(const Se3f& pose, bool initPose); // true - инициализация - выставление в первый раз с изменением z
														   // основная функция координатных фич, все остальные сеттеры - просто short-cuts	
	float minimapRadius() const { return radius_ * attr().minimapScale_; }

	//-----------------------------------------------------
	// Спецэффекты

	virtual cObject3dx* model() const { return 0; }
	virtual c3dx* get3dx() const { return 0; }
	
	float height() const { return height_; } // Спецэффекты масштабируются по высоте

	virtual int modelNodeIndex(const char* node_name) const { return -1; }
	virtual void setModelNodeTransform(int node_index, const Se3f& pos) {}

	/// старт эффекта
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

	virtual bool isConstructed() const { return true; }
	 
	//-----------------------------------------------------

	int unitAttackClass() const { return unitAttackClass_; }
	void setUnitAttackClass(AttackClass unitAttackClass){ unitAttackClass_ = unitAttackClass; }

	virtual bool canAttackTarget(const WeaponTarget& target, bool check_fow = false) const { return false; }

	virtual float fireRadius() const { return 0; }
	virtual float fireRadiusMin() const { return 0; }
	virtual float sightRadius() const { return 300; }
	virtual float noiseRadius() const { return 0; }
	virtual float hearingRadius() const { return 100; }

	//------------------------------------------------

	virtual void mapUpdate(float x0,float y0,float x1,float y1); // Реакция на изменение поверхности земли или воды 

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
	const UnitColorEffective& color() { return color_; }
	virtual void setOpacity(float op);
	float opacity() const { return color_.color.a / 255.f; }
	virtual const UnitColor& defaultColor() const { return UnitColor::ZERO; }

	void setAuxiliary(bool auxiliary) { auxiliary_ = auxiliary; }
	bool auxiliary() const { return auxiliary_; }
	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_UNIT; }

	void hide(int reason, bool hide);
	int hiddenLogic() const { return hideReason_ &~ HIDE_BY_FOW; }
	int hiddenGraphic() const { return hideReason_; } // !!! Не повторяется вызывать только из графики

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
	int hideReason_;

private:
	const AttributeBase* attr_;
	Player* player_;

	bool placedIntoDeleteList_;
	/// вспомогательный юнит: записывать не надо(для редактора)
	bool auxiliary_;

	int collisionGroup_;

	AttackClass unitAttackClass_;

	enum {
		COLOR_CHANGE_PHASE_SET = 1,
		COLOR_CHANGE_PHASE_RELAX = 2
	};
	/// \a true если установлено изменение цвета
	unsigned char colorChange_;
	/// назначенный цвет
	UnitColorEffective color_;
	/// фаза изменения цвета, 0 - цвет по-умолчанию, 1 - цвет из \a colors_
	float colorPhase_;

	typedef SwapVector<UnitEffectController> EffectControllers;
	EffectControllers effectControllers_;

	typedef SwapVector<AbnormalState> AbnormalStates;
	AbnormalStates abnormalStates_;

	float possibleDamage_;
	LogicTimer pfUnitMapTimer_;

	float cameraDistance2_;

	friend class TransparentTracking;
};

/////////////////////////////////////////////
class UnitFactoryArg2 
{
public:
	static void setPlayer(Player* player) { player_ = player; }
	static void setAttribute(const AttributeBase* attribute) { attribute_ = attribute; }

protected:
	static Player* player_;
	static const AttributeBase* attribute_;

	template<class Derived>
	UnitBase* createArg() { 
		if(!attribute_)
			return 0;
		UnitBase* unit = 0;
		const char* attributeTypeName = UnitFactory::instance().typeName(attribute_->unitClass());
		if(!strcmp(typeid(Derived).name(), attributeTypeName))
			unit = new Derived(UnitTemplate(attribute_, player_));
		else{
			xassertStr(0 && "Смена класса юнита", (string(typeid(Derived).name()) + " -> " + attributeTypeName).c_str());
			return UnitBase::create(UnitTemplate(attribute_, player_));
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

typedef SerializationFactory<UnitBase, UnitFactoryArg2> UnitSerializationFactory;

template<>
struct FactorySelector<UnitBase>
{
	typedef UnitSerializationFactory Factory;
};


typedef Factory<UnitClass, UnitBase, UnitFactoryArg2> UnitFactory;

class UnitSerializer
{
public:
	UnitSerializer(UnitBase* unit = 0) : unit_(unit) {}
	operator UnitBase* () const { return unit_; }
	UnitBase* operator->() const { return unit_; }
	UnitBase& operator*() const { return *unit_; }

	void serialize(Archive& ar);

    UnitBase*& unit() { return unit_; }

private:
	UnitBase* unit_;
};

typedef SwapVector<UnitSerializer> UnitList; 

#endif
