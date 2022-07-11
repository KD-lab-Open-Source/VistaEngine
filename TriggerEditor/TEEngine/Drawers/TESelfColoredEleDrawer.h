#ifndef __T_E_SELF_COLORED_ELE_DRAWER_H_INCLUDED__
#define __T_E_SELF_COLORED_ELE_DRAWER_H_INCLUDED__

#include "TEConditionColoredEleDrawer.h"

class TESelfColoredEleDrawer : public TEConditionColoredEleDrawer  
{
public:
	TESelfColoredEleDrawer();
	virtual ~TESelfColoredEleDrawer();
protected:
	virtual HBRUSH	SelectTriggerBackBrush(Trigger const& trigger) const;
};

#endif
