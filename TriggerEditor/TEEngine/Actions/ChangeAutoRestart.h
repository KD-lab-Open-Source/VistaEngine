#ifndef __CHANGE_AUTO_RESTART_H_INCLUDED__
#define __CHANGE_AUTO_RESTART_H_INCLUDED__

#include "TriggerExport.h"

class ChangeAutoRestart  
{
public:
	ChangeAutoRestart(TriggerChain& chain, 
					int triggerIndex, 
					int linkIndex,
					bool autoRestart);
	~ChangeAutoRestart();
	bool operator()();
	static bool run(TriggerChain& chain, 
					int triggerIndex, 
					int linkIndex,
					bool autoRestart);
private:
	TriggerChain& chain_;
	int triggerIndex_;
	int linkIndex_;
	bool autoRestart_;
};

#endif
