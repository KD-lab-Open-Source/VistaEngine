#ifndef __I_DRAW_DATA_H_INCLUDED__
#define __I_DRAW_DATA_H_INCLUDED__
#include <utility>

#include "TriggerExport.h"

typedef std::pair<TriggerChain const* const*, int> DrawingData;

interface IDrawData
{
	virtual DrawingData getDrawedChains() const = 0;
};

#endif
