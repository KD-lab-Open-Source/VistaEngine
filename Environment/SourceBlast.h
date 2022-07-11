#ifndef __SOURCE_BLAST_H__
#define __SOURCE_BLAST_H__

#include "SourceBase.h"
#include "UnitAttribute.h"

class SourceBlast : public SourceBase
{
public:
	SourceBlast();
	SourceType type() const { return SOURCE_BLAST; }
	SourceBase* clone() const { return new SourceBlast(*this); }

	void quant();
	void serialize(Archive& ar);

private:

	void windMapQuant();

	float radiusCur;
	float radiusSpeed;
	float blastPower;

};

#endif //__SOURCE_BLAST_H__

