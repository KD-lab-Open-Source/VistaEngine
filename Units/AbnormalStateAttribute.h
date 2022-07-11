
#ifndef __ABNORMAL_STATE_ATTRIBUTE_H__
#define __ABNORMAL_STATE_ATTRIBUTE_H__

#include "Parameters.h"
#include "..\Util\EffectReference.h"
#include "..\Environment\SourceBase.h"

//=======================================================
struct ExplodeProperty : public StringTableBase
{
	bool enableExplode;
	bool enableRagDoll;
	bool animatedDeath;
	bool alphaDisappear;
	Vect3f explodeFactors;
	float selfPower;
	float liveTime;

	float z_gravity;
	float friction;
	float restitution;
	
	ExplodeProperty(const char* name = "") : StringTableBase(name) {
		enableExplode = false;
		animatedDeath = false;
		enableRagDoll = false;

		alphaDisappear = true;
		explodeFactors = Vect3f::ID;
		selfPower = 0;
		liveTime = 10000;

		z_gravity = 9.8f;
		friction = 0.8f;
		restitution = 0.2f;;
	}

	void serialize(Archive& ar);
};

typedef StringTable<ExplodeProperty> ExplodeTable;
typedef StringTableReference<ExplodeProperty, true> ExplodeReference;

/// параметры гибели
struct DeathAttribute
{
	ExplodeReference explodeReference;
	EffectAttributeAttachable effectAttr;
	EffectAttributeAttachable effectAttrFly;
	bool enableExplodeFantom;
	float explodeFactor;

	/// источники, остающиеся после гибели юнита
	SourceWeaponAttributes sources;

	DeathAttribute();

	void serialize(Archive& ar);
};

struct AbnormalStateType : public StringTableBase
{
	int priority;

	AbnormalStateType(const char* name = "");

	int mask() const { return mask_; }

	void serialize(Archive& ar);

private:
	int mask_;
	static int maskCounter_;
};

typedef StringTable<AbnormalStateType> AbnormalStateTypeTable;
typedef StringTableReference<AbnormalStateType, false> AbnormalStateTypeReference;
typedef vector<AbnormalStateTypeReference> AbnormalStateTypeReferences;

struct UnitColor
{
	UnitColor() : color(255, 255, 255, 255), isBrightColor(false) {}
	UnitColor(sColor4c color_, bool isBrightColor_) : color(color_), isBrightColor(isBrightColor_) {}

	void serialize(Archive& ar);

	bool operator != (const UnitColor& rs) const {
		return isBrightColor != rs.isBrightColor || color.RGBA() != rs.color.RGBA();
	}

	sColor4c color;
	bool isBrightColor;

	static UnitColor ZERO;
};

class UnitColorEffective : public UnitColor
{
public:
	UnitColorEffective() : UnitColor(), fill_(255) {}
	UnitColorEffective(const UnitColor& src)  : UnitColor() { setColor(src, true); }

	void setColor(const UnitColor& clr, bool reset);
	void setOpacity(float op, bool reset);

	void apply(cObject3dx* model, float phase);

private:
	int fill_;
};

class AbnormalStateEffect
{
public:
	AbnormalStateEffect();
	~AbnormalStateEffect();

	void serialize(Archive& ar);

	bool operator == (const AbnormalStateType* tp) const { return type() == tp; }

	const AbnormalStateType* type() const { return type_; }
	const AbnormalStateTypeReference& typeRef() const { return type_; }

	bool needColorChange() const { return effectColor_ != UnitColor::ZERO; }
	const UnitColor& color() const { return effectColor_; }

	const EffectAttributeAttachable& effectAttribute() const { return effectAttribute_; }
	const SoundAttribute* soundAttribute() const { return soundReference_; }

	bool hasDeathAttribute() const { return deathAttribute_; }
	const DeathAttribute& deathAttribute() const { return *deathAttribute_; }

private:
	AbnormalStateTypeReference type_;

	/// цвет состояния
	UnitColor effectColor_;

	/// спецэффект состояния
	EffectAttributeAttachable effectAttribute_;

	/// звук состояния
	SoundReference soundReference_;

	/// собственные настройки гибели
	DeathAttribute* deathAttribute_;
};

/// параметры ненормального состояния юнита (горение, дымление и т.п.)
class AbnormalStateAttribute
{
public:
	AbnormalStateAttribute();

	void serialize(Archive& ar);

	const AbnormalStateType* type() const { return type_; }

	bool operator == (const AbnormalStateType* tp) const { return type() == tp; }

	bool isEnabled() const { return type() != 0; }

	float duration() const { return duration_; }
	float durationRnd() const { return durationRnd_; }

	const WeaponDamage& damage() const { return damage_; }
	bool freeze() const { return freeze_; }

	void applyParameterArithmetics(const ArithmeticsData& arithmetics);
	void applyParameterArithmeticsOnDamage(const ArithmeticsData& parameterArithmetics);
    
	bool useArithmetics() const { return useArithmetics_; }
	const ParameterArithmetics& arithmetics() const { return arithmetics_; }

private:

	AbnormalStateTypeReference type_;

	/// время действия в секундах
	float duration_;
	/// случайная добавка к времени действия, плюс-минус
	float durationRnd_;

	/// наносимые за секунду повреждения
	WeaponDamage damage_;
	bool freeze_;

	bool useArithmetics_;
	ParameterArithmetics arithmetics_;
};

#endif // __ABNORMAL_STATE_ATTRIBUTE_H__
