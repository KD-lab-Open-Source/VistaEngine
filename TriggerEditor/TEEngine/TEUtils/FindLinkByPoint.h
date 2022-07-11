#ifndef __FIND_LINK_BY_POINT_H_INCLUDED__
#define __FIND_LINK_BY_POINT_H_INCLUDED__

#include <utility>
#include "TriggerExport.h"

class FindLinkByPoint  
{
	FindLinkByPoint();
	~FindLinkByPoint();
public:
	static std::pair<int, int> run(TriggerChain const& chain, 
									CPoint const& point);
};

#endif
