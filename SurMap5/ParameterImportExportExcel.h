#ifndef __PARAMETER_IMPORT_EXPORT_EXCEL_H__
#define __PARAMETER_IMPORT_EXPORT_EXCEL_H__

class ExcelExporter;
class ExcelImporter;

class ParameterExportExcel {
public:

	ParameterExportExcel(const char* filename);
	~ParameterExportExcel();

	void exportParameters();
private:
	
	ExcelExporter* application_;

};

class ParameterImportExcel {
public:

	ParameterImportExcel(const char* filename);
	~ParameterImportExcel();

	void importParameters();
private:

	ExcelImporter* application_;

};

#endif //__PARAMETER_IMPORT_EXPORT_EXCEL_H__
