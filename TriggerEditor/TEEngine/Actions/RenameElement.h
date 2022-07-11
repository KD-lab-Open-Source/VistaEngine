#ifndef __RENAME_ELEMENT_H_INCLUDED__
#define __RENAME_ELEMENT_H_INCLUDED__

#include "TriggerExport.h"

class RenameElement  
{
public:
	RenameElement(TriggerChain& chain, 
					int triggerIndex, 
					CString const& newName);
	~RenameElement();
	bool operator()() const;
private:
	TriggerChain& chain_;
	int triggerIndex_;
	CString newName_;
};

#endif
