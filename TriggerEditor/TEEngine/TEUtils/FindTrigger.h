#ifndef __FIND_TRIGGER_H_INCLUDED__
#define __FIND_TRIGGER_H_INCLUDED__

#include "TriggerExport.h"

class FindTrigger  
{
	FindTrigger();
	~FindTrigger();

	class TestTrigger;
public:
	static int run(TriggerChain const& chain, CPoint const& point);
};

#endif
