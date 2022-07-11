#ifndef __IMPULSE_SOURCE_H__
#define __IMPULSE_SOURCE_H__

#include "SourceEffect.h"
#include "UnitAttribute.h"

class SourceImpulse : public SourceEffect
{
public:
	SourceImpulse();
	SourceImpulse(const SourceImpulse& src);
	SourceType type() const { return SOURCE_IMPULSE; }
	SourceBase* clone() const { return new SourceImpulse(*this); }

	void quant();
	void apply(UnitBase* unit);
	void serialize(Archive& ar);
	const AbnormalStateAttribute& abnormalState() const{ return abnormalState_; }

	bool getParameters(WeaponSourcePrm& prm) const;
	bool setParameters(const WeaponSourcePrm& prm, const WeaponTarget* target = 0);

private:
	bool instantaneous;
	bool applyOnFlying;
	float horizontalImpulse;
	float verticalImpulse;
	float torqueImpulse;
	float environmentTorqueImpulse;
	AbnormalStateAttribute abnormalState_;
};

#endif //__IMPULSE_SOURCE_H__

