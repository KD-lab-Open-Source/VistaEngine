#ifndef __ATTRIBUTE_CACHE_H__
#define __ATTRIBUTE_CACHE_H__

#include "AbnormalStateAttribute.h"

// Для модификации параметров юнитов данного игрока
// кешируется только AttributeLibrary: юниты, здания и предметы
class AttributeCache 
{ 
public:
	AttributeCache(){}
	AttributeCache(const AttributeBase& attribute);

	const ParameterSet& parameters() const { return parameters_; }
	ParameterSet& getParameters() { return parameters_; } 
	void setParameters(const ParameterSet& parameters) { parameters_ = parameters; } 
	const ParameterSet& parametersMax() const { return parametersMax_; }

	const AbnormalStateAttribute& waterEffect() const { return waterEffect_; }
	const AbnormalStateAttribute& lavaEffect() const { return lavaEffect_; }
	const AbnormalStateAttribute& iceEffect() const { return iceEffect_; }
	const AbnormalStateAttribute& earthEffect() const { return earthEffect_; }

	void applyArithmetics(const ArithmeticsData& arithmetics);
	void serialize(Archive& ar);

protected:
	ParameterSet parameters_;
	ParameterSet parametersMax_;

	AbnormalStateAttribute waterEffect_;
	AbnormalStateAttribute lavaEffect_;
	AbnormalStateAttribute iceEffect_;
	AbnormalStateAttribute earthEffect_;

};

#endif //__ATTRIBUTE_CACHE_H__
