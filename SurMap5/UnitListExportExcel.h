#ifndef __UNITLIST_EXPORT_EXCEL_H__
#define __UNITLIST_EXPORT_EXCEL_H__

class ExcelExporter;

class UnitListExportExcel {
public:

	UnitListExportExcel(const char* filename);
	~UnitListExportExcel();

	void exportUnitList();
private:
	
	ExcelExporter* application_;

};

#endif //__UNITLIST_EXPORT_EXCEL_H__
