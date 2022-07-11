#ifndef __BOUNDING_RECT_CALCULATOR_H_INCLUDED__
#define __BOUNDING_RECT_CALCULATOR_H_INCLUDED__

#include "TEEngine/TEGrid.h"
#include "TriggerExport.h"

class BoundingRectCalculator  
{
	TEGrid const& grid_;
public:
	BoundingRectCalculator(TEGrid const& grid):grid_(grid){}

	static void run(TEGrid const& grid, Trigger& trigger);
	void operator()(Trigger& trigger) const;
};

#endif
