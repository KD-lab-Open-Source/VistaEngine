#ifndef __ENSURE_TRIGGER_NAME_ORIGINALITY_H_INCLUDED__
#define __ENSURE_TRIGGER_NAME_ORIGINALITY_H_INCLUDED__

#include "TriggerExport.h"
class EnsureTriggerNameOriginality  
{
public:
	EnsureTriggerNameOriginality();
	~EnsureTriggerNameOriginality();
	static bool run(TriggerChain const& chain, CString const& name);
	static CString getUniqueName(TriggerChain const& chain, 
		CString const& name);
};

#endif
