#ifndef __SELECT_IF_INTERSECT_H_INCLUDED__
#define __SELECT_IF_INTERSECT_H_INCLUDED__

#include "TriggerExport.h"
class SelectionManager;
class SelectIfIntersect
{
	CRect const& rect_;
	SelectionManager& selectionManager_;
public:
	SelectIfIntersect(CRect const& rect, SelectionManager& selectionManager);
	void operator()(Trigger& trigger) const;
};

#endif
