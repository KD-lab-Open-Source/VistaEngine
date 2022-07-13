#include "stdafx.h"
#include "UnitListExportExcel.h"
#include "..\ExcelExport\ExcelExporter.h"
#include "..\Units\UnitAttribute.h"
#include "Dictionary.h"


UnitListExportExcel::UnitListExportExcel(const char* filename)
{
	application_ = ExcelExporter::create(filename);
	application_->beginSheet();
}

UnitListExportExcel::~UnitListExportExcel()
{
	application_->free();
}


void UnitListExportExcel::exportUnitList()
{
	application_->setCellText(Vect2i(0,0), TRANSLATE("Тип юнита/Тип юнита"));

	int column = 0;
	int row = 0;
	AttributeLibrary::Map::const_iterator mi;
	FOR_EACH(AttributeLibrary::instance().map(), mi){
		column++;
		row++;
		const AttributeBase* attribute = mi->get();

		application_->setCellText(Vect2i(column, 0), attribute->libraryKey());
		// formatting
		application_->setCellTextOrientation(Vect2i(column, 0), -90);
		application_->setColumnWidthAuto(column);

		application_->setCellText(Vect2i(0, row), attribute->libraryKey());
		
	}	

	application_->setColumnWidthAuto(0);
}
