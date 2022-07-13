#include "stdafx.h"
#include "ParameterImportExportExcel.h"
#include "..\ExcelExport\ExcelExporter.h"
#include "..\Units\Parameters.h"
#include "..\Units\UnitAttribute.h"
#include "Dictionary.h"


ParameterImportExcel::ParameterImportExcel(const char* filename)
{
	application_ = ExcelImporter::create(filename);
}

ParameterImportExcel::~ParameterImportExcel()
{
	application_->free();
}

void ParameterImportExcel::importParameters()
{
	
	application_->openSheet();
        
    ParameterValueTable::Strings& strings = const_cast<ParameterValueTable::Strings&>(ParameterValueTable::instance().strings());
    ParameterValueTable::Strings::iterator it;

	Recti usedRange(application_->getUsedRange());
	int columns_count = usedRange.width();
	int rows_count = usedRange.height();

	for(int i = 2; i < rows_count; i++) {
		std::string group = application_->getCellText(Vect2i(0, i));
		std::string type = "";
		for(int j = 1; j < columns_count; j++) {
			char dest[512] = "";
			const char* fulltype_ = application_->getCellText(Vect2i(j, 0));
			const char* pdest = strrchr(fulltype_, '[');
			if(pdest) 
				strncpy(dest, fulltype_,(int)(pdest - fulltype_) - 1);
			std::string type_ = dest;
			if(type_ != "") 
				type = type_;
			std::string colName = application_->getCellText(Vect2i(j, 1));
	        FOR_EACH(strings, it)
			    if (group == TRANSLATE((*it).group()->c_str()) && type == TRANSLATE((*it).type()->c_str()) && strcmp((*it).c_str(), "") != 0) {
					char columnName[16] = "";
					int i_ = 0;
					if(strncmp((*it).c_str(), "W", 1) == 0)// если имя параметра начинается с W
						i_ = 2;
					// добавляем в имя столбца цифры
					while(iswdigit(*((*it).c_str() + i_)))
						++i_;
					if(i_) 
						strncpy(columnName, (*it).c_str(), i_);

					if(strcmp(columnName, "") != 0){ 
						if(strcmp(colName.c_str(), columnName) == 0){ 
							std::string value = application_->getCellText(Vect2i(j, i));
							if (value == "")
									xassert(0 && "Проверьте правильность данных!!!");

							if ((*it).canModify()){
								char rawValue[16] = "";	
								char* pdest = strchr(value.c_str(), '/');
								int result = (int)(pdest - value.c_str());
								strncpy(rawValue, value.c_str(), result);
								(*it).setRawValue(atof(rawValue));
								//break;
							}
						}											
					}
				}            
		}
	}
}

ParameterExportExcel::ParameterExportExcel(const char* filename)
{
	application_ = ExcelExporter::create(filename);
	application_->beginSheet();
}

ParameterExportExcel::~ParameterExportExcel()
{
	application_->free();
}


void ParameterExportExcel::exportParameters()
{
	application_->setCellText(Vect2i(0,0), TRANSLATE("Группа/Имя параметра"));

	const ParameterValueTable::Strings& valueStrings = ParameterValueTable::instance().strings();
    ParameterValueTable::Strings::const_iterator vi;

	const ParameterTypeTable::Strings& typeStrings = ParameterTypeTable::instance().strings();
	ParameterTypeTable::Strings::const_iterator ti;

	const ParameterGroupTable::Strings& groupStrings = ParameterGroupTable::instance().strings();
	ParameterGroupTable::Strings::const_iterator gi;

	vector<string> columnNames;
	int subTypeCount = 0;

    FOR_EACH(typeStrings, ti){
		columnNames.clear();
		FOR_EACH(valueStrings,vi)
			if ((*vi).type() == &(*ti) && strcmp((*vi).c_str(), "") != 0){// перебираем все величины с определенным типом
				char columnName[16] = "";
				int i = 0;
				if(strncmp((*vi).c_str(), "W", 1) == 0)// если имя параметра начинается с W
					i = 2;
				// добавляем в имя столбца цифры
				while(iswdigit(*((*vi).c_str() + i)))
					++i;
				if(i) 
					strncpy(columnName, TRANSLATE((*vi).c_str()), i);
				// проверка на существование такого же имени
				if(strcmp(columnName, "") != 0) 
					if(find(columnNames.begin(), columnNames.end(), columnName) == columnNames.end())
						columnNames.push_back(columnName);
			}
		int column = subTypeCount;
		vector<string>::iterator ci;
		if(!columnNames.empty()){ 
			XBuffer buf_(XB_DEFSIZE,1);
			buf_ < TRANSLATE((*ti).c_str());
			int posBracket = buf_.tell() + 2;
			buf_ < " [" < TRANSLATE((*ti).comment()) < "]";
			application_->setCellText(Vect2i(column + 1, 0), buf_);
			application_->setTextColor(Vect2i(column + 1, 0), 1, posBracket - 1 , 0, "Tahoma", 12, true, false);
			application_->setTextColor(Vect2i(column + 1, 0), posBracket , buf_.tell() - posBracket + 1, 54, "Tahoma", 9, false, false);
		}
		FOR_EACH(columnNames, ci){
			column++;
			application_->setCellText(Vect2i(column, 1), (*ci).c_str());
		}
		if (column - subTypeCount > 1)
			application_->mergeCellRange(Recti(subTypeCount + 1, 0, column - subTypeCount, 1));
		// помещаем элементы
		int row = 2;
		FOR_EACH(groupStrings, gi) {
			FOR_EACH(valueStrings,vi)
				if ((*vi).type() == &(*ti) && strcmp((*vi).c_str(), "") != 0 && (*vi).group() == &(*gi)){
					char columnName[16] = "";
					int i = 0;
					if(strncmp((*vi).c_str(), "W", 1) == 0)// если имя параметра начинается с W
						i = 2;
					// добавляем в имя столбца цифры
					while(iswdigit(*((*vi).c_str() + i)))
						++i;
					if(i) 
						strncpy(columnName, (*vi).c_str(), i);

					if(strcmp(columnName, "") != 0){ 
					
						int col = subTypeCount + distance(columnNames.begin(),find(columnNames.begin(), columnNames.end(), columnName)) + 1;
						
						application_->setCellComment(Vect2i(col, row), TRANSLATE((*vi).c_str()));
						XBuffer buf;
						char charBuf[16] = "";
						sprintf(charBuf, "%.7g", (*vi).rawValue()); 
						buf	< charBuf;
						int devidePosition = buf.tell() + 1;
						char charBuf_[16] = "";
						sprintf(charBuf_, "%.7g", (*vi).value()); 
						buf < "/" < charBuf_;
						application_->setCellFormat(Vect2i(col, row), "@"); //"@" - текстовый формат
						application_->setCellText(Vect2i(col, row), buf);
						if ((*vi).canModify()) { 
  							application_->setTextColor(Vect2i(col, row), 1, devidePosition - 1, 5, "Tahoma", 12, true, false);
							int color_ = _stricmp((*vi).formula().get()->formula().c_str(), FormulaString().c_str()) != 0 ? 3 : 5;
							application_->setTextColor(Vect2i(col, row), devidePosition + 1, buf.tell() - devidePosition, color_, "Tahoma", 12, true, false);
						}
						else {
							application_->setTextColor(Vect2i(col, row), 1, devidePosition - 1, 4, "Tahoma", 12, true, false);
							application_->setTextColor(Vect2i(col, row), devidePosition + 1, buf.tell() - devidePosition, 4, "Tahoma", 12, true, false);
						}
					}	
				}
			++row;
		}
		if (column - subTypeCount > 0)
			application_->setTableEdges(Recti(subTypeCount + 1, 0, column - subTypeCount, row));
		application_->setLeftEdgeFat(Recti(subTypeCount + 1, 0, 1, row));
		subTypeCount = column;
	}

	int row = 2;
    FOR_EACH(groupStrings, gi) {
        application_->setCellText(Vect2i(0, row), TRANSLATE((*gi).c_str()));
		++row;
	}
	application_->setFont(Recti(0, 0, 1, 1), "Tahoma", 12.0f, true, false);
	application_->setFont(Recti(0, 1, subTypeCount + 1, row), "Tahoma", 12.0f, false, false);
	application_->setFont(Recti(1, 2, subTypeCount, row - 2), "Tahoma", 12.0f, true, false);
	application_->setFont(Recti(0, 1, 1, row - 1), "Tahoma", 7.0f, false, false);
	application_->setTableEdges(Recti(0, 0, 1, row));
	application_->setRowHeight(0, 60);
	application_->setRowWrap(0, true);
	for(int i = 0; i <= subTypeCount + 1; i++)
		application_->setColumnWidthAuto(i);
	/*application_->setCellText(Vect2i(0,0), "Группа/Имя параметра");

	const ParameterGroupTable::Strings& groupStrings = ParameterGroupTable::instance().strings();
	ParameterGroupTable::Strings::const_iterator gi;

    const ParameterValueTable::Strings& valueStrings = ParameterValueTable::instance().strings();
    ParameterValueTable::Strings::const_iterator vi;

	const ParameterTypeTable::Strings& typeStrings = ParameterTypeTable::instance().strings();
	ParameterTypeTable::Strings::const_iterator ti;

	int row = 1;
    FOR_EACH(groupStrings, gi) {
			application_->setCellText(Vect2i(0, row), (*gi).c_str());
			int column = 1;
			FOR_EACH(typeStrings, ti){
				FOR_EACH(valueStrings,vi)
					if ((*vi).group() == &(*gi) &&  (*vi).type() == &(*ti)) {
						if (strcmp((*vi).formula().get()->formula().c_str(), FormulaString().c_str()) != 0){
							XBuffer buf;
							buf	<= (*vi).value();
							int devidePosition = buf.tell() + 1;
  						    application_->setCellText(Vect2i(column, row), buf);
						    application_->setTextColor(Vect2i(column, row), 1, devidePosition - 1, 3);
						}
						else {
							XBuffer buf;
							buf	<= (*vi).rawValue();
							int devidePosition = buf.tell() + 1;
  						    application_->setCellText(Vect2i(column, row), buf);
						    application_->setTextColor(Vect2i(column, row), 1, devidePosition - 1, 5);
						}
						break;
					}
				column++;
			}
            row++;
    }

	int column = 0;
	FOR_EACH(typeStrings, ti){
		column++;
		application_->setCellText(Vect2i(column, 0), (*ti).c_str());
	}
	application_->setFont(Recti(0, 0, column + 1, 1), "Tahoma", 7.0f, true, false);
	application_->setFont(Recti(0, 1, column + 1, row), "Tahoma", 7.0f, false, false);*/

}
