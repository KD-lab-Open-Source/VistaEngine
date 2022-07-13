#include "StdAfx.h"
#include "ExcelImEx.h"
#include  <direct.h>
#include "ProgDlg.h"
#include "Util\TextDB.h"
#include "GlobalAttributes.h"
#include "UnicodeConverter.h"
#include <set>

static long const LANGUAGES_ROW_NUM = 1;
static long const TEXT_FIRST_ROW = LANGUAGES_ROW_NUM + 2;

static long const ID_COLUMN = 1;
static long const FIRST_LANGUAGE_COLUMN = ID_COLUMN + 1;

static long const LANGUAGE_COLUMN_WIDTH = 50;

void prepareLang(string& out, const wchar_t* in)
{
	w2a(out, in);
	size_t pos = out.find_first_not_of(' ');
	if(pos == string::npos)
		out.clear();
	else {
		if(pos != 0)
			out.erase(0, pos - 1);
		if((pos = out.find_last_not_of(' ')) < out.size() - 1)
			out.erase(pos);
		_strlwr(const_cast<char*>(out.c_str()));
	}
}

ImportImpl::ImportImpl(const char* filename)
{
	application_ = ExcelImporter::create(filename);
}

ImportImpl::~ImportImpl(void)
{
	application_->free();
}

bool ImportImpl::importLangList(AvaiableLanguages& languagesList)
{
	application_->openSheet();

	languagesList.clear();

	int columns = application_->getUsedRange().width();
	string langdir;
	for(int i = FIRST_LANGUAGE_COLUMN; i <= columns; ++i){
		prepareLang(langdir, application_->getCellText(Vect2i(i - 1, LANGUAGES_ROW_NUM - 1)));
		LanguageCombo language(langdir.c_str());
		if(language.valid())
			languagesList.push_back(language);
	}
	
	return true;
}

bool ImportImpl::getLanguage(const char* language, TextDB& textDatabase)
{
	application_->openSheet();

	textDatabase.clear();

	Recti recti = application_->getUsedRange();
	int columns = recti.width();
	int rows = recti.height();

	CProgressDlg progress(IDS_IMPORT_XLS_FILE);
	progress.Create(NULL);

	if(rows - TEXT_FIRST_ROW > 1) 
		progress.SetRange(1, rows - TEXT_FIRST_ROW);
	else 
		progress.SetRange(1, 2);
	progress.SetPos(1);

	CString status;
	status.Format(IDS_STATUS_LOAD_LANGUAGE, language);
	progress.SetStatus(status);

	bool languageDiscovered = false;
	string langdir;
	for(int col = FIRST_LANGUAGE_COLUMN; col <= columns; ++col){
		prepareLang(langdir, application_->getCellText(Vect2i(col - 1, LANGUAGES_ROW_NUM - 1)));
		if(_stricmp(langdir.c_str(), language) == 0){
			languageDiscovered = true;
			for(int row = TEXT_FIRST_ROW; row <= rows; ++row){
				string id(w2a(application_->getCellText(Vect2i(ID_COLUMN - 1, row - 1))));
				wstring data(application_->getCellText(Vect2i(col - 1, row - 1)));
				textDatabase.insert(std::make_pair(id, data));
				if(progress.BreakProcess())
					return false;
				progress.StepIt();
			}
			break;
		}
	}

	return languageDiscovered;
}


// импортирует все языковые данные из файла XLS (имя исходного файла
// определяется в конструкторе класса), в папку с именем path\<язык>\Text\Texts.tdb 
bool ImportImpl::importAllLangText(const AvaiableLanguages& languagesList)
{
	// создание дерева каталогов	
	XBuffer buf;
	buf < "Resource\\LocData";
	int rootpos = buf.tell();
	_mkdir(buf);

	AvaiableLanguages::const_iterator lang;
	FOR_EACH(languagesList, lang){
		buf.set(rootpos, 0);

		buf < "\\" < *lang;
		int langdir = buf.tell();
		_mkdir(buf);

		buf.set(langdir, 0);
		_mkdir(buf < "\\Text");

		buf.set(langdir, 0);
		_mkdir(buf < "\\Fonts");

		buf.set(langdir, 0);
		_mkdir(buf < "\\UI_Textures");

		buf.set(langdir, 0);
		_mkdir(buf < "\\Video");

		buf.set(langdir, 0);
		_mkdir(buf < "\\Voice");
	}

	TextDB textDatabase;

	bool allGood = true;
	FOR_EACH(languagesList, lang){
		textDatabase.loadLanguage(*lang);
		if(getLanguage(*lang, textDatabase))
			textDatabase.saveLanguage();
		else
			allGood = false;
	}

	return allGood;
}

// --------------------------------------------------------------------------


ExportImpl::ExportImpl(const char* filename)
{
	application_ = ExcelExporter::create(filename);
	application_->beginSheet();
}

ExportImpl::~ExportImpl(void)
{
	application_->free();
}

bool ExportImpl::exportAllLangText(const AvaiableLanguages& languagesList)
{
	application_->setCellText(Vect2i(ID_COLUMN - 1, LANGUAGES_ROW_NUM - 1), L"Loc key");
	application_->setColumnWidth(ID_COLUMN - 1, 40);

	list<TextDB> textDatabases;
	set<string> newValues;

	CProgressDlg progress(IDS_EXPORT_XLS_FILE);
	progress.Create(NULL);

	progress.SetRange(1, max(languagesList.size(), 2));
	progress.SetPos(1);
	CString status;

	int col = FIRST_LANGUAGE_COLUMN - 1;
	AvaiableLanguages::const_iterator lang;
	FOR_EACH(languagesList, lang){
		application_->setColumnWidth(col, 50);
		// запись названия языка
		application_->setCellText(Vect2i(col++, LANGUAGES_ROW_NUM - 1), a2w(*lang).c_str());

		status.Format(IDS_STATUS_LOAD_LANGUAGE, *lang);
		progress.SetStatus(status);

		textDatabases.push_back(TextDB(*lang));
		TextDB& database = textDatabases.back();

		// чистка старых ключей, все, что не прогружено - будет удалено
		TextDB::iterator it = database.begin();
		while(it != database.end()) {
			if(TextDB::instance().find(it->first) == TextDB::instance().end())
				it = database.erase(it);
			else 
				++it;
		}

		progress.StepIt();
		if(progress.BreakProcess())
			return false;

		// добавление новых ключей: прогружены, но в старом списке отсутствуют
		FOR_EACH(TextDB::instance(), it)
			if(database.insert(std::make_pair(it->first, a2w(it->first).c_str())).second)
				newValues.insert(it->first);

		xassert(database.size() == TextDB::instance().size());
	}

	status.LoadString(IDS_STATUS_SAVE_LANGUAGE);
	progress.SetStatus(status);

	progress.SetRange(1, max(TextDB::instance().size() / 10, 2));
	progress.SetPos(1);

	int row = TEXT_FIRST_ROW - 1;
	set<string> emptyValues;

	for(int part = 0; part < 3; ++part){
		list<TextDB>::const_iterator data;
		TextDB::const_iterator it;
		FOR_EACH(TextDB::instance(), it){
			switch(part){
			case 0: // только новые
				if(newValues.find(it->first) == newValues.end())
					continue;
				break;
			case 1: // все непустые, кроме выведенных на этапе 0
				if(newValues.find(it->first) != newValues.end())
					continue;
				FOR_EACH(textDatabases, data)
					if(!data->operator[](it->first).empty())
						break;
				if(data == textDatabases.end()){
					emptyValues.insert(it->first);
					continue;
				}
				break;
			default: // пропущенные на этапе 1
				if(emptyValues.find(it->first) == emptyValues.end())
					continue;
			    break;
			}
			
			application_->setCellText(Vect2i(ID_COLUMN - 1, row), a2w(it->first).c_str());
			
			col = FIRST_LANGUAGE_COLUMN - 1;
			FOR_EACH(textDatabases, data)
				application_->setCellText(Vect2i(col++, row), data->operator[](it->first).c_str());
			
			++row;
			if((row - TEXT_FIRST_ROW) % 10 == 0){
				progress.StepIt();
				if(progress.BreakProcess())
					return false;
			}
		}
	}

	xxassert(row - TEXT_FIRST_ROW + 1 ==  TextDB::instance().size(), "не все строки выгрузились");

	return true;
}