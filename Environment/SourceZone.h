#ifndef __ZONE_SOURCE_H_INCLUDED__
#define __ZONE_SOURCE_H_INCLUDED__

#include "SourceBase.h"
#include "EffectReference.h"
#include "SourceEffect.h"
#include "..\Util\Range.h"
#include "..\Units\UnitAttribute.h"
#include "..\Units\IronBullet.h"
#include "..\render\src\lighting.h"

class UnitBase;

class SourceZone : public SourceBase
{
public:
	/// тип зоны (исключительно дл€ редактировани€)
	enum ZoneEditType {
		ZONE_GENERATOR,
		ZONE_AFFECT,
		ZONE_WALK_EFFECT
	};

	SourceZone();
	SourceZone(const SourceZone& original);
	SourceBase* clone () const {
		return new SourceZone(*this);
	}
	~SourceZone();

	void setRadius (float radius) {
		__super::setRadius(radius);
		if (enabled_ && active_) {
			effectInit();
			effectStart();
		}
	}

	SourceType type() const { return SOURCE_ZONE; }
	Se3f currentPose();

	void serialize(Archive& ar);

	void setEditType(ZoneEditType editType){ editType_ = editType; }
	ZoneEditType editType() const { return editType_; }
	const AttributeBase* generatedUnit() const { return unitReference_; }

	void setEffect(const EffectAttribute& effectAttribute);

	void setEffectRadius (Rangef radius) { effectRadius_ = radius; }
	Rangef effectRadius () const { return effectRadius_; }

	void setEffectScale (Rangef scale) { effectScale_ = scale; }
	Rangef effectScale () const { return effectScale_; }

	const ParameterCustom& damage() const { return damage_; }
	const AbnormalStateAttribute& abnormalState() const { return abnormalState_; }

	void quant();

	void apply(UnitBase* target);

	/// режимы генерации объектов
	enum UnitGenerationMode
	{
		/// объекты по€вл€ютс€ на поверхности мира
		GENERATION_MODE_TERRAIN,
		/// пачка юнитов распределенна€ равномерно по зоне
		GENERATION_MODE_ZONE,
		/// объекты падают с неба
		GENERATION_MODE_SKY
	};

	UnitGenerationMode unitGenerationMode() const { return unitGenerationMode_; } 

	void showDebug() const;

	bool getParameters(WeaponSourcePrm& prm) const;
	bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0);

protected:
	void start();
	void stop();

private:

	/// состо€ние - вкл./выкл.
	ZoneEditType editType_;

	/// поворачивать зону или нет
	bool enableRotation_;

	Rangef effectRadius_;
	Rangef effectScale_;
	EffectAttribute effectAttribute_;
	int effectTime_;

	/// повреждени€, наносимые зоной
	ParameterCustom damage_;
	/// воздействие на юниты
	AbnormalStateAttribute abnormalState_;

	/// генерируемый зоной снар€д
	AttributeProjectileReference projectileReference_;
	/// генерируемый зоной юнит
	AttributeReference unitReference_;
	bool createBuiltBuilding_;
	/// диапазон горизонтальных углов
	Rangef unitAnglePsi_;
	/// диапазон отклонени€ от вертикали
	Rangef unitAngleTheta_;
	/// диапазон начальных скоростей
	Rangef unitVelocity_;
	/// диапазон угловых скоростей
	Rangef unitAngularVelocity_;
	/// режим генерации
	UnitGenerationMode unitGenerationMode_;
	/// периодичность по€влени€ объектов, в секундах
	Rangef unitGenerationPeriod_;
	/// количество одновременно по€вл€ющихс€ юнитов
	int unitCount_;
	/// количество оставшихс€ в очереди юнитов
	int unitCur_;
	/// интервал генерации юнитов (если 0 по€вл€ютс€ все сразу)
	int unitGenerationInterval_;
	/// однократное создание
	bool unitGenerationOnce_;
	/// высота, на которой по€вл€ютс€ объекты (дл€ GENERATION_MODE_SKY)
	float unitGenerationHeight_;
	/// генераци€ по радиусу
	bool generateByRadius_;
	
	DurationTimer unitGenerationTimer_;
	DurationTimer unitIntervalTimer_;

	struct EffectControllerNode{
		EffectController effect;
		int a, b, c, d;
		float angle;
		float effect_radius;
		EffectControllerNode(const BaseUniverseObject* owner, const Vect2f delta = Vect2f::ZERO):
			effect(owner, delta){
			effect.setPlacementMode(EffectController::PLACEMENT_BY_ATTRIBUTE);
			};
		~EffectControllerNode(){};
	};
	typedef std::vector<EffectControllerNode> EffectControllers;
	EffectControllers effectControllers_;
	/// диапазон эффективного радиуса блуждающего эффекта
	Rangef walk_effect_range;

	/// врем€ за которое блуждающий эффект пройдет полный цикл
	int wander_time;

	void effectInit();
	void effectStart();
	void effectStop();

	float scaleRnd() const;

	void generate_effect_modificators(EffectControllerNode&);

	/// создаЄт юнит, устанавливает ему позицию и скорость
	void generateUnits();

	void initZone();

	void applyDamage(UnitBase* target) const;
};

class SourceDetector : public SourceDamage
{
public:
	enum ZoneEditType {
		ZONE_DETECTOR,
		ZONE_HIDER
	};

	SourceDetector();
	SourceType type() const { return SOURCE_DETECTOR; }
	SourceBase* clone() const { return new SourceDetector(*this); }

	void quant();
	void serialize(Archive& ar);

	void sourceApply(SourceBase* source);
	void apply(UnitBase* target);
	bool canApply(const UnitBase* target) const;
private:
	ZoneEditType editType_;
};

class SourceFreeze : public SourceDamage
{
public:
	SourceFreeze();
	SourceType type() const { return SOURCE_FREEZE; }
	SourceBase* clone() const { return new SourceFreeze(*this); }
	
	void apply(UnitBase* target);
};

#endif // #ifndef __ZONE_SOURCE_H_INCLUDED__
