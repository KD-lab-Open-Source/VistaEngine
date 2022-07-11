#ifndef __UNIT_NUMBER_MANAGER_H__
#define __UNIT_NUMBER_MANAGER_H__

#include "UnitAttribute.h"

class UnitNumberManager
{
public:
	UnitNumberManager(const UnitNumbers& unitNumbers);

	void apply(const ArithmeticsData& arithmetics);
	void add(const ParameterSet& parameters);
	void sub(const ParameterSet& parameters);

	void reserve(const UnitFormationTypeReference& type, int counter);

	int number(const UnitFormationTypeReference& type) const;

	void serialize(Archive& ar);

private:
	struct Number 
	{
		UnitFormationTypeReference type;
		ParameterSet numberParameters;
		int reserved;

		Number();
		Number(const UnitNumber& data);

		void serialize(Archive& ar);
		int number() const;
	};
	typedef vector<Number> Numbers;
	Numbers numbers_;
};

#endif //__UNIT_NUMBER_MANAGER_H__
