#ifndef __SOURCE_EFFECT_H__
#define __SOURCE_EFFECT_H__

#include "SourceBase.h"
#include "EffectReference.h"
#include "Units\EffectController.h"
#include "Units\AbnormalStateAttribute.h"

class cEffect;
class SourceEffect : public SourceBase  {
public:
	SourceEffect ();
	SourceEffect (const SourceEffect& original);
	~SourceEffect ();

	void effectPause(bool pause = true);

	void quant();

	void showDebug() const;

	void serialize(Archive& ar);

protected:
	virtual void effectStart();
	virtual void effectStop();
	
	void start()
	{
		__super::start();
		effectStart();
	}

	void stop()
	{
		__super::stop();
		effectStop();
	}


private:
	EffectAttribute effectAttribute_;
	EffectController effectController_;
	bool effectPause_;
	int effectTime_;
};

class SourceDamage : public SourceEffect
{
	/// повреждения, наносимые зоной
	ParameterCustom damage_;
	/// воздействие на юниты
	AbnormalStateAttribute abnormalState_;

public:
	SourceDamage() : SourceEffect() {}
	~SourceDamage() {}
	
	void serialize(Archive& ar);

	void showDebug() const;

	void apply(UnitBase* target);

	const ParameterCustom& damage() const { return damage_; }
	const AbnormalStateAttribute& abnormalState() const { return abnormalState_; }

	bool getParameters(WeaponSourcePrm& prm) const;
	bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0);

protected:
	void start()
	{
		__super::start();
		setScanEnvironment(true);
	}

	void stop()
	{
		__super::stop();
		setScanEnvironment(false);
	}

	void applyDamage(UnitBase* target) const;
};

#endif // __SOURCE_EFFECT_H__

