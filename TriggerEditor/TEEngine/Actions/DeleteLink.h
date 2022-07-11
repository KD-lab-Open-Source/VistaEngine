#ifndef __DELETE_LINK_H_INCLUDED__
#define __DELETE_LINK_H_INCLUDED__

#include "TriggerExport.h"

class DeleteLink
{
public:
	DeleteLink(TriggerChain & chain, 
				int linkOwenerIndex, 
				int linkChildIndex);
	~DeleteLink(void);
	bool operator()();
	static bool run(TriggerChain &chain, 
				int linkOwnerIndex, 
				int linkChildIndex);
private:
	TriggerChain &chain_;
	int					linkOwnerIndex_;
	int					linkChildIndex_;
};

#endif
