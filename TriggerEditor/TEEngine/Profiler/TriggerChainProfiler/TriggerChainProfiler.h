#ifndef __TRIGGER_CHAIN_PROFILER_H_INCLUDED__
#define __TRIGGER_CHAIN_PROFILER_H_INCLUDED__

#include "ITriggerChainProfiler.h"
#include "TriggerExport.h"

/**
 	Вспомогательный класс, для того, чтобы изолировать остальную часть кода от периметра
*/
class TriggerChainProfiler : public ITriggerChainProfiler  
{
public:
	TriggerChainProfiler();
	~TriggerChainProfiler();

	void setTriggerChain(TriggerChain* ptrChain);

	virtual int getEntriesCount() const;
	virtual char const* getEntryText(int entryIndex) const;
	virtual bool setCurrentEntry(int entryIndex);
	virtual bool isDataEnabled() const;
private:
	TriggerChain* ptrChain_;
};

#endif
