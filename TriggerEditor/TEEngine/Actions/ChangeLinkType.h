#ifndef __CHANGE_LINK_TYPE_H_INCLUDED__
#define __CHANGE_LINK_TYPE_H_INCLUDED__

#include "TriggerExport.h"
class ChangeLinkType  
{
public:
	ChangeLinkType(TriggerChain& chain, 
					int triggerIndex, 
					int linkIndex,
					int type);
	~ChangeLinkType();

	bool operator()();
	static bool run(TriggerChain& chain, 
					int triggerIndex, 
					int linkIndex,
					int type);
private:
	TriggerChain& chain_;
	int triggerIndex_;
	int linkIndex_;
	int type_;
};

#endif
