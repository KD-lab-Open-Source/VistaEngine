#ifndef __EXCEL_FILE_STRUCT_H_INCLUDED__
#define __EXCEL_FILE_STRUCT_H_INCLUDED__

#include "..\ExcelExport\ExcelExporter.h"
#include <assert.h>

// Excel File Structure
class ExcelFileStruct {

	template <int ver> struct V { 
	};

	template <> struct V<1> {
		static long const NUM = 1;
		static long const VERSION_ROW_NUM = 1;
		static long const CODE_PAGES_ROW_NUM = 2;
		static long const LANGUAGES_ROW_NUM = 3;
		static long const TEXT_FIRST_ROW = LANGUAGES_ROW_NUM + 2;

		static long const ID_COLUMN = 1;
		static long const FIRST_LANGUAGE_COLUMN = 2;

		static long const LANGUAGE_COLUMN_WIDTH = 50;

		static bool check(ExcelImporter* excelImporter) {
			Recti recti_ = excelImporter->getUsedRange();
			// проверяем размер используемой области в файле
			if ((recti_.width() < 2)||(recti_.height() < 3)) 
				return false;
			// проверяем наличие номера версии
			int version = int(excelImporter->getCellFloat(Vect2i(1, VERSION_ROW_NUM - 1)));
			if (version != NUM) 
				return false;
			// проверяем наличие заголовков в файле для версии, кодовой страницы и ключа
			CString value((excelImporter->getCellText(Vect2i(0, VERSION_ROW_NUM - 1))));
			if (!compareString(value, IDS_LANGUAGE_FILE_VERSION))
				return false;
			value = (excelImporter->getCellText(Vect2i(ID_COLUMN - 1,LANGUAGES_ROW_NUM - 1)));
			if (!compareString(value, IDS_ID_COLUMN_HEADER))
				return false;
			value = (excelImporter->getCellText(Vect2i(0, CODE_PAGES_ROW_NUM - 1)));
			if (!compareString(value, IDS_CODEPAGE_NUMS))
				return false;
	
			return true;
		}
	};

public:

	static long const CURRENT_VERSION = 1;

	typedef V<1> FIRST_VERSION;
	
	//! текущий набор констант версии
	typedef V<CURRENT_VERSION> CV;

	ExcelFileStruct(void) : version_(0) {
	}

	~ExcelFileStruct(void) {
	}

	long getVersion() const	{
		return version_;
	}

	long getVersionRow() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::VERSION_ROW_NUM;
		}
		assert(0);
		return -1;
	}

	long getFirstTextRow() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::TEXT_FIRST_ROW;
		}
		assert(0);
		return -1;
	}

	long getLanguagesRow() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::LANGUAGES_ROW_NUM;
		}
		assert(0);
		return -1;
	}

	long getCodePagesRow() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::CODE_PAGES_ROW_NUM;
		}
		assert(0);
		return -1;
	}

	long getIdColumn() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::ID_COLUMN;
		}
		assert(0);
		return -1;
	}
	
	long getFirstLanguageColumn() const {
		switch(getVersion()) {
			case V<1>::NUM:
				return V<1>::FIRST_LANGUAGE_COLUMN;
		}
		assert(0);
		return -1;
	}

	void writeHeader(ExcelExporter* excelExporter) {
		CString str;
		str.LoadString(IDS_LANGUAGE_FILE_VERSION);
		excelExporter->setCellText(Vect2i(0,CV::VERSION_ROW_NUM - 1), static_cast<LPCTSTR>(str));
		excelExporter->setCellFloat(Vect2i(1,CV::VERSION_ROW_NUM - 1), CURRENT_VERSION);
		version_ = CURRENT_VERSION;

		str.LoadString(IDS_CODEPAGE_NUMS);
		excelExporter->setCellText(Vect2i(0,CV::CODE_PAGES_ROW_NUM - 1), static_cast<LPCTSTR>(str));
		
		str.LoadString(IDS_ID_COLUMN_HEADER);
		excelExporter->setCellText(Vect2i(CV::ID_COLUMN - 1,CV::LANGUAGES_ROW_NUM - 1), static_cast<LPCTSTR>(str));
		excelExporter->setColumnWidth(CV::ID_COLUMN - 1, str.GetLength());		 
	}

	bool check(ExcelImporter* excelImporter) {
		version_ = -1;
		if (V<1>::check(excelImporter))
			version_ = V<1>::NUM;
		return (version_ != -1);
	}

private:

	static bool compareString(CString value, UINT strID) {
		CString str;
		str.LoadString(strID);
		return (str.CompareNoCase(value) == 0);
	}

	int version_;

};

#endif
