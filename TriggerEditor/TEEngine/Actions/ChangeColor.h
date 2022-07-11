#ifndef __CHANGE_COLOR_H_INCLUDED__
#define __CHANGE_COLOR_H_INCLUDED__

#include "TriggerExport.h"
class ChangeColor  
{
public:
	ChangeColor(TriggerChain& chain, int triggerIndex, COLORREF color);
	~ChangeColor();
	bool operator()();
	static bool run(TriggerChain& chain, 
					int triggerIndex, 
					COLORREF color);
private:
	COLORREF color_;
	int		triggerIndex_;
	TriggerChain& chain_;
};

#endif
