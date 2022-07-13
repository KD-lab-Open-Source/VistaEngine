#ifndef __I_CONDITION_DROPPABLE_H_INCLUDED__
#define __I_CONDITION_DROPPABLE_H_INCLUDED__

class IConditionDroppable {
public:
    virtual void beginExternalDrag (const CPoint& point, int typeIndex) = 0;
	virtual bool canBeDropped(const CPoint& point) = 0;
};

#endif
