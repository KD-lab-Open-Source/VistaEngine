#ifndef __EXCEL_IM_EX_H_INCLUDED__
#define __EXCEL_IM_EX_H_INCLUDED__

#include  <list>
#include  <string>
#include  <direct.h>
#include "ProgDlg.h"
#include "..\Util\TextDB.h"
#include "GlobalAttributes.h"
#include "ExcelFileStruct.h"
#include "..\ExcelExport\ExcelExporter.h"

typedef TextDB LanguageText;

class ImportImpl {

public:

	ImportImpl(const char* filename) {
		application_ = ExcelImporter::create(filename);
	}
	
	~ImportImpl(void) {
		application_->free();      	
	} 

    bool importLangList(LanguagesList& languagesList_)	{	
		languagesList_.clear();

		application_->openSheet();

		ExcelFileStruct structure;
		if (!structure.check(application_)) {
			assert(0);
			return false;
		}
		else
			return importLanguagesList(application_, structure, languagesList_);
	}

	// импортирует все языковые данные из файла XLS (имя исходного файла
	// определяется в конструкторе класса), в папку с именем path\<язык>\Text\Texts.tdb 
	bool importAllLangText(LanguagesList languagesList) {
		LanguageText languageText_;
		bool failed;
		failed = false;

		// создание дерева каталогов	
		char buffer[_MAX_PATH];
		_getcwd( buffer, _MAX_PATH );
		XBuffer buf;
		buf < buffer < "\\resource\\LocData";
		_mkdir(buf);
		int pos = buf.tell();
		for(LanguagesList::iterator i = GlobalAttributes::instance().languagesList.begin();
			i != GlobalAttributes::instance().languagesList.end(); ++i) {
				buf.set(pos, 0);
			    buf < "\\" < (*i).language.c_str();
				int pos_ = buf.tell();
				_mkdir(buf);
				buf.set(pos_, 0);
				_mkdir(buf < _T("\\Text"));
				buf.set(pos_, 0);
				_mkdir(buf < _T("\\Fonts"));
				buf.set(pos_, 0);
				_mkdir(buf < _T("\\UI_Textures"));
				buf.set(pos_, 0);
				_mkdir(buf < _T("\\Video"));
				buf.set(pos_, 0);
				_mkdir(buf < _T("\\Voice"));
		}

		buf.set(0, 0);
		buf < buffer < "\\resource\\LocData";
		for(LanguagesList::iterator i = languagesList.begin(); i != languagesList.end(); ++i) {
			languageText_.clear();
			if ((!failed) && (!importLangText(i, languageText_)))
				failed = true;
			XBuffer buf_;
			buf_ < buf < "\\" < (*i).language.c_str() < "\\Text\\Texts.tdb";
			languageText_.save(buf_);
		}
		return !failed;
	}

	bool importLangText(LanguagesList::iterator language,  LanguageText& languageText_) {
		languageText_.clear();

		application_->openSheet();

		ExcelFileStruct structure;
		if (!structure.check(application_)) {
			assert(0);
			return false;
		}
		else {
			CProgressDlg progress(IDS_IMPORT_XLS_FILE);
		    progress.Create(NULL);
			return importLanguagesData(application_, structure, language, &progress, languageText_);
		}
    }

	ExcelImporter* application() const
	{
		return application_;
	}

private:

	ExcelImporter* application_;
	
protected:

	bool importLanguagesList(ExcelImporter* excelImporter, 
							 ExcelFileStruct const& structure,
							 LanguagesList& languagesList_);

	bool importLanguagesData(ExcelImporter* excelImporter, 
							 ExcelFileStruct const& structure,
							 LanguagesList::iterator language, 
							 CProgressDlg* progress,
							 LanguageText& languageText_);


};

bool ImportImpl::importLanguagesData(ExcelImporter* excelImporter, 
									 ExcelFileStruct const& structure,
									 LanguagesList::iterator language, 
									 CProgressDlg* progress,
									 LanguageText& languageText_) {
	Recti recti_ = excelImporter->getUsedRange();
	int columns = recti_.width();
	int rows = recti_.height();

	// initializing progress window

	if (rows-structure.getFirstTextRow() > 1) 
		progress->SetRange(1,rows-structure.getFirstTextRow());
	else 
		progress->SetRange(1,2);
	progress->SetPos(1);
	CString status;
	status.Format(IDS_STATUS_LOAD_LANGUAGE, (*language).language.c_str());
	progress->SetStatus(status);

	bool findLanguage;
	findLanguage = false;
	for (int i = structure.getFirstLanguageColumn(); i <= columns; ++i)	{
		CString str(excelImporter->getCellText(Vect2i(i - 1, structure.getLanguagesRow() - 1)));
		str.Trim();
		if (str.GetLength()) 
			if (str.CompareNoCase((*language).language.c_str()) == 0) {
				findLanguage = true;
				for (int j = structure.getFirstTextRow(); j <= rows; ++j) {
					std::string strID;
					strID = excelImporter->getCellText(Vect2i(structure.getIdColumn() - 1, j - 1));
					std::string strText;
					strText = excelImporter->getCellText(Vect2i(i - 1, j - 1), language->codePage);
					languageText_.insert(std::make_pair(strID, strText));
					
					if (progress->BreakProcess()) {
						//languageText_.clear();
						return false;
					}
					progress->StepIt();
				}
			}
	}
	if (!findLanguage)		 
		for (int j = structure.getFirstTextRow(); j <= rows; ++j) {
			std::string strID;
			strID = excelImporter->getCellText(Vect2i(structure.getIdColumn() - 1, j - 1));
			languageText_.insert(std::make_pair(strID, ""));
					
			if (progress->BreakProcess()) {
				//languageText_.clear();
				return false;
			}
					progress->StepIt();
		}
	return true;
}

bool ImportImpl::importLanguagesList(ExcelImporter* excelImporter, 
								 ExcelFileStruct const& structure,
								 LanguagesList& languagesList_) {
	Recti recti_ = excelImporter->getUsedRange();
	int columns = recti_.width();
	for (int i = structure.getFirstLanguageColumn(); i <= columns; ++i)	{
		CString str(excelImporter->getCellText(Vect2i(i - 1, structure.getLanguagesRow() - 1)));
		str.Trim();
		if (str.GetLength()) {
			long version = structure.getVersion();
			unsigned codePage = CP_ACP;
			switch(version) {
				case ExcelFileStruct::FIRST_VERSION::NUM: {
					codePage = unsigned(excelImporter->getCellFloat(Vect2i(i - 1, structure.getCodePagesRow() - 1)));
				}
				break;
				default: 
					return false;
			}
			Language language;
			language.language = str;
			language.codePage = codePage;
			languagesList_.push_back(language);
		}
	}
	return true;
}

class ExportImpl {

public:

	ExportImpl(const char* filename) {
		application_ = ExcelExporter::create(filename);
		application_->beginSheet();
	}
	
	~ExportImpl(void) {
        application_->free();      	
	} 

	bool exportLangText(LanguagesList::iterator language, LanguageText& languageText_) {
		if (languageText_.empty()) 
			return false;
		ExcelFileStruct structure;
		structure.writeHeader(application_);

		CProgressDlg progress(IDS_EXPORT_XLS_FILE);
	    progress.Create(NULL);

		return exportLanguagesData(application_, structure, language, languageText_, &progress);
    }

	// экспортирует все языковые данные в файл XLS (имя исходного файла
	// определяется в конструкторе класса), из папки с именем <язык>\Text\Texts.tdb 
	bool exportAllLangText(TextIdMap textIdMap, LanguagesList languagesList) {
		LanguageText languageText_;
		bool failed;
		char buffer[_MAX_PATH];
		_getcwd( buffer, _MAX_PATH );
		failed = false;
		ExcelFileStruct structure;
		structure.writeHeader(application_);
		int i_ = structure.getFirstLanguageColumn();
		for(LanguagesList::iterator i = languagesList.begin(); i != languagesList.end(); ++i) {
			// перебор всех языков
			XBuffer buf;
			buf < buffer < "\\resource\\LocData\\" < (*i).language.c_str() < "\\Text\\Texts.tdb";

			//загрузка языка в languageText
			languageText_.clear();
			if (!failed)
				languageText_.load(buf);

			// запись названия языка и соотв. кодовой страницы
			application_->setCellText(Vect2i(i_ - 1, structure.getLanguagesRow() - 1), (*i).language.c_str());
			application_->setCellFloat(Vect2i(i_ - 1, structure.getCodePagesRow() - 1),(*i).codePage);

			// чистка languageText
			LanguageText::iterator it = languageText_.begin();
			while(it != languageText_.end()) {
				if(textIdMap.map().find(it->first) == textIdMap.map().end())
					languageText_.erase(it);
				else 
					++it;
			}

			// добавление новых ключей
			TextIdMap::Map::const_iterator it_ = textIdMap.map().begin();
			while(it_ != textIdMap.map().end()) {
				if(languageText_.find(it_->first) == languageText_.end())
					languageText_.insert(std::make_pair(it_->first,""));
				++it_;
			}

			if (!languageText_.empty()) {
				CProgressDlg progress(IDS_EXPORT_XLS_FILE);
				progress.Create(NULL);

				if (languageText_.size() > 1) 
					progress.SetRange(1,int(languageText_.size()));
				else
					progress.SetRange(1,2);
                progress.SetPos(1);
				CString status;
				status.Format(IDS_STATUS_LOAD_LANGUAGE, (*i).language.c_str());
				int codePage = (*i).codePage;
				progress.SetStatus(status);

				int j = structure.getFirstTextRow();
				for (LanguageText::iterator it = languageText_.begin(); it != languageText_.end(); ++it) {
					if (i_ == structure.getFirstLanguageColumn())
						application_->setCellText(Vect2i(structure.getIdColumn() - 1, j - 1), it->first.c_str());
					application_->setCellText(Vect2i(i_ - 1, j - 1), it->second.c_str(), codePage);
					j++;
					if (progress.BreakProcess()) {
						//languageText_.clear();
						failed = true;
					}
					progress.StepIt();
				}
			}
			i_++;
		}
		return !failed;
	}

private:

	ExcelExporter* application_;

protected:

	bool exportLanguagesData(ExcelExporter* excelExporter, ExcelFileStruct const& structure,
		LanguagesList::iterator language, LanguageText& languageText_, CProgressDlg* progress);

};

bool ExportImpl::exportLanguagesData(ExcelExporter* excelExporter, 
									 ExcelFileStruct const& structure,
									 LanguagesList::iterator language, 
									 LanguageText& languageText_, 
									 CProgressDlg* progress) {
	int i = structure.getFirstLanguageColumn();
	excelExporter->setCellText(Vect2i(i - 1, structure.getLanguagesRow() - 1), (*language).language.c_str());
	excelExporter->setCellFloat(Vect2i(i - 1, structure.getCodePagesRow() - 1),(*language).codePage);

	if (!languageText_.empty()) {
		// initializing progress window
		if (languageText_.size() > 1)
			progress->SetRange(1,int(languageText_.size()));
		else
			progress->SetRange(1,2);
		progress->SetPos(1);
		CString status;
		status.Format(IDS_STATUS_LOAD_LANGUAGE, (*language).language.c_str());
		int codePage = (*language).codePage;
		progress->SetStatus(status);

		int j = structure.getFirstTextRow();
		for (LanguageText::iterator it = languageText_.begin(); it != languageText_.end(); ++it) {
			excelExporter->setCellText(Vect2i(structure.getIdColumn() - 1, j - 1), it->first.c_str());
			excelExporter->setCellText(Vect2i(i - 1, j - 1), it->second.c_str(), codePage);
			j++;
			if (progress->BreakProcess()) {
				//languageText_.clear();
				return false;
			}
			progress->StepIt();
		}
	}
	return true;
}

#endif
