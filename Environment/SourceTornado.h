#ifndef __TORNADO_SOURCE_H__
#define __TORNADO_SOURCE_H__

#include "SourceEffect.h"
#include "UnitAttribute.h"
#include "..\terra\terTools.h"

class SourceTornado : public SourceDamage
{
public:
	SourceTornado();
	SourceType type() const { return SOURCE_TORNADO; }
	SourceTornado(const SourceTornado& other);
	SourceBase* clone() const { return new SourceTornado(*this); }

	void apply(UnitBase* unit);
	void serialize(Archive& ar);
	void quant();

protected:
	void start();
	void stop();

private:

	Vect3f getForce(const Vect3f& unitPos);
	Vect3f getTorque(const Vect3f& unitPos);
	Vect3f getRandomTorque(const Vect3f& unitPos);

	float height_;
	float tornadoFactor_;
	TerToolCtrl toolser_;

};

#endif //__TORNADO_SOURCE_H__
