#ifndef __PARAMETER_STATISTICS_EXPORT_H_INCLUDED__
#define __PARAMETER_STATISTICS_EXPORT_H_INCLUDED__

#include "Handle.h"
class ExcelExporter;

class ParameterStatisticsExport{
public:
	ParameterStatisticsExport();
	void exportExcel(const char* fileName);

	PtrHandle<ExcelExporter> excel_;
};

#endif
