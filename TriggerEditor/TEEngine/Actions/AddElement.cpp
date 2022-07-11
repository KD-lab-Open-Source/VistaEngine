#include "stdafx.h"
#include "addelement.h"
#include "TEEngine/TEUtils/BoundingRectCalculator.h"
//#include "Game/GameInclude.h"

#include "TriggerExport.h"

AddElement::AddElement(TEGrid const& grid, 
					   CPoint const& cell,
					   TriggerChain & chain, 
					   Trigger const& trigger,
					   int position):
  grid_(grid)
, cell_(cell)
, chain_(chain)
//, trigger_(trigger) // нельзя так делать, т.к. условие и действе должны иметь единственного владельца
, position_(position)
{
	triggerInterface().copyTrigger(trigger_, trigger);
}

AddElement::~AddElement(void)
{
}

bool AddElement::operator()()
{
	return run(grid_, cell_, chain_, trigger_, position_);
}

bool AddElement::run(TEGrid const& grid, 
				CPoint const& cell,
				TriggerChain& chain, 
				Trigger const& trigger,
				int position)
{
	Trigger triggerCopy;
	triggerInterface().copyTrigger(triggerCopy, trigger);
	Trigger * const res = chain.insertTrigger(position, triggerCopy);
	if (!res)
		return false;

	res->setCellIndex(cell);
	BoundingRectCalculator::run(grid, *res);

	CRect rcStrategy(chain.boundingRect());
	rcStrategy.UnionRect(&rcStrategy, &res->boundingRect());
	chain.setBoundingRect(rcStrategy);

	return true;
}
