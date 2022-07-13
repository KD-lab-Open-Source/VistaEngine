#ifndef __EXCEL_IM_EX_H_INCLUDED__
#define __EXCEL_IM_EX_H_INCLUDED__

#include "ExcelExport\ExcelExporter.h"

struct LanguageCombo;
typedef vector<LanguageCombo> AvaiableLanguages;

class TextDB;

class ImportImpl
{
public:
	ImportImpl(const char* filename);
	~ImportImpl(void); 

    bool importLangList(AvaiableLanguages& languagesList);
	bool importAllLangText(const AvaiableLanguages& languagesList);

	ExcelImporter* application() const { return application_; }

private:
	bool getLanguage(const char* language,  TextDB& textDatabase);

	ExcelImporter* application_;
};

class ExportImpl
{
public:
	ExportImpl(const char* filename);
	~ExportImpl(void); 

	bool exportAllLangText(const AvaiableLanguages& languagesList);

private:
	ExcelExporter* application_;
};

#endif
