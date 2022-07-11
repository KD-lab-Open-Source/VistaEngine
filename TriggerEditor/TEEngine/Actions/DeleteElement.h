#ifndef __DELETE_ELEMENT_H_INCLUDED__
#define __DELETE_ELEMENT_H_INCLUDED__

#include "TriggerExport.h"

class TEBaseWorkMode;

class DeleteElement
{
public:
	DeleteElement(TriggerChain & chain, int triggerIndex);
	bool operator()();
	~DeleteElement(void);
	static bool run(TriggerChain & chain, int triggerIndex);
private:
	TriggerChain & chain_;
	int triggerIndex_;
};

#endif
