#ifndef __INSERT_ROWS_H_INCLUDED__
#define __INSERT_ROWS_H_INCLUDED__

#include "TriggerExport.h"

class TriggerEditorLogic;
class TEGrid;

class InsertRows
{
public:
	InsertRows(size_t rowsCount, 
					TriggerEditorLogic const& logic, 
					TriggerChain & chain,
					CPoint const& basePoint,
					TEGrid const& grid);
	~InsertRows(void);
	bool operator()();
	static bool run(size_t rowsCount, 
					TriggerEditorLogic const& logic, 
					TriggerChain & chain,
					CPoint const& basePoint,
					TEGrid const& grid);
private:
	size_t						rowsCount_;
	CPoint						basePoint_;
	TriggerEditorLogic const&	logic_;
	TriggerChain& chain_;
	TEGrid const&				grid_;
};

#endif
