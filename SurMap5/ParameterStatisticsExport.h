#ifndef __PARAMETER_STATISTICS_EXPORT_H_INCLUDED__
#define __PARAMETER_STATISTICS_EXPORT_H_INCLUDED__

#include "Handle.h"
class ExcelExporter;

class ParameterStatisticsExportImpl;
class ParameterStatisticsExport{
public:
	typedef ParameterStatisticsExportImpl Impl;
	ParameterStatisticsExport();
	~ParameterStatisticsExport();
	void exportExcel(const char* fileName);

	void serialize(Archive& ar);
	
	ParameterStatisticsExportImpl& impl() { return *impl_; }
protected:
	ParameterStatisticsExportImpl* impl_;
	PtrHandle<ExcelExporter> excel_;
};

#endif
