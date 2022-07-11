#ifndef __DELETEGRASS_SOURCE_H__
#define __DELETEGRASS_SOURCE_H__

#include "SourceEffect.h"

class SourceDeleteGrass : public SourceEffect
{
public:
	SourceDeleteGrass() {};
	SourceType type() const { return SOURCE_DELETE_GRASS; }
	SourceBase* clone() const { return new SourceDeleteGrass(*this); }

	void quant();

private:
};

#endif //__DELETEGRASS_SOURCE_H__

