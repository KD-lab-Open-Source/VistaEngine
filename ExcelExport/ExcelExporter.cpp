#include "stdafx.h"
#include "ExcelExporter.h"

#pragma warning (disable:4192)
#import "excel9.olb" auto_search\
	rename ("RGB", "rgb")  rename("DialogBox", "dialogBox") \
	rename("DocumentProperties", "documentProperties")\
	rename("CopyFile", "copyFile") rename("ReplaceText", "replaceText") 

const wchar_t* unicode_convert(const char* str, unsigned codePage)
{
	static std::wstring conv_str(2048,0);

	int length = MultiByteToWideChar(codePage,0,str,-1,NULL,0);
	conv_str.resize(length);

	MultiByteToWideChar(codePage,0,str,-1,&*conv_str.begin(),length);

	return conv_str.c_str();
}

const char* ansi_convert(const std::wstring& wstr, unsigned codePage)
{
	static std::string conv_str(2048,0);

	int length = WideCharToMultiByte(codePage,0,wstr.c_str(),-1,NULL,0,NULL,NULL);
	conv_str.resize(length);

	WideCharToMultiByte(codePage,0,wstr.c_str(),-1,&*conv_str.begin(),length,NULL,NULL);

	return conv_str.c_str();
}

class ExcelExporterImpl : public ExcelExporter {
public:
    ExcelExporterImpl(const char* filename);
	virtual ~ExcelExporterImpl();
	
	int addSheet(const char* name, int codePage = 0);
	void setSheetName(const char* name, int codePage = 0);
    void beginSheet();
	void setCurrentSheet(int index);

	void setCellFormat(const Vect2i& pos, const char* format);
    void setCellText(const Vect2i& pos, const char* text, int codePage = 0);
	void setCellComment(const Vect2i& pos, const char* comment, int codePage = 0);
    void setCellFloat(const Vect2i& pos, float value);

	void addHyperlink(const Vect2i& pos, const char* address, const char* subAddress, const char* cellText, const char* tipText);

	void removeColumn(int column);
    void setColumnWidth(int column, float width);
	void setColumnWidthAuto(int column); 
    void setColumnWrap(int column, bool wrap);
	
    void setRowHeight(int row, float height);
    void setRowWrap(int row, bool wrap);
	void setCellTextOrientation(const Vect2i& pos, int angle);

	void setTextColor(const Vect2i& pos, int startSymbol, int countSymbol, int colorIndex, const char* fontName, int fontSize, bool bold, bool italic);
    void setFont(const Recti& range, const char* fontName, float size, bool bold, bool italic);
    void setBackColor(const Recti& range, int colorIndex);
	void setPageMargins(float left, float top, float right, float bottom);
	void setPageOrientation(bool landscape);
	void mergeCellRange(const Recti& rect);
	void setTableEdges(const Recti& rect);
	void setLeftEdgeFat(const Recti& rect);
    void endSheet();
	void free();

	const char* cellName(const Vect2i& pos);
	//const char* cellAddressFromVect()
protected:
	Excel::RangePtr ExcelExporterImpl::rangeFromVect(const Vect2i& pos);
	void handleError(_com_error& error);
private:
	std::string filename_;

	Excel::_ApplicationPtr application_;
	Excel::_WorkbookPtr book_;
	Excel::_WorksheetPtr sheet_;

	// buffers:
	std::string cellNameBuffer_;
};

EXCELEXPORT_API ExcelExporter* ExcelExporter::create(const char* filename)
{
    return new ExcelExporterImpl(filename);
}

void ExcelExporterImpl::free()
{	
	(*this).endSheet();
    delete this;
}

const char* ExcelExporterImpl::cellName(const Vect2i& pos)
{
	const int numLetters = 26;
	const char* letters[26] = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
							    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
							    "U", "V", "W", "X", "Y", "Z" };

	cellNameBuffer_ = "";

	int column = pos.x;
	do{
		int lastLetter = column % numLetters;
		cellNameBuffer_ = std::string(letters[lastLetter]) + cellNameBuffer_;
		column -= lastLetter;
		column /= numLetters;
	}while(column);

    XBuffer row;
	row <= (pos.y + 1);
	cellNameBuffer_ += static_cast<const char*>(row);
	return cellNameBuffer_.c_str();
}

void ExcelExporterImpl::setCurrentSheet(int index)
{
	using namespace Excel;
	Excel::_WorksheetPtr sheet = application_->Sheets->Item[index + 1];
	sheet->Select();
	sheet_ = application_->ActiveSheet;
}

void ExcelExporterImpl::beginSheet()
{
	using namespace Excel;
	try	{
        book_ = application_->Workbooks->Add((long)xlWorksheet);
        sheet_ = book_->Sheets->Item[1];
	}
	catch (_com_error& e) {
		XBuffer buf;
		buf < "beginSheet(), COM Error: " < e.ErrorMessage();
		MessageBox(0, buf, "COM Error", MB_OK | MB_ICONERROR);
	}

}

void ExcelExporterImpl::endSheet()
{
	using namespace Excel;
    try {
		book_->SaveAs(filename_.c_str(), vtMissing, vtMissing, vtMissing, 
					  vtMissing, vtMissing, Excel::xlNoChange);
		book_->Close();
    }
	catch (_com_error& e) {
		XBuffer buf;
		buf < "endSheet(), COM Error: " < e.ErrorMessage();
		MessageBox(0, buf, "COM Error", MB_OK | MB_ICONERROR);
	}
}

ExcelExporterImpl::ExcelExporterImpl(const char* filename)
: application_("Excel.Application")
, filename_(filename)
{
	application_->PutVisible(false, false);
}

void ExcelExporterImpl::setCellComment(const Vect2i& pos, const char* comment, int codePage)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(pos.y, pos.x)->GetResize(1, 1));
	range->ClearComments();
	range->AddComment();
	if(codePage)
		range->GetComment()->Text(unicode_convert(comment, codePage));
	else
		range->GetComment()->Text(comment);
}
Excel::RangePtr ExcelExporterImpl::rangeFromVect(const Vect2i& pos)
{
	using namespace Excel;
    return RangePtr(sheet_->GetRange("A1", "A1")->GetOffset(pos.y, pos.x)->GetResize(1, 1));
}

void ExcelExporterImpl::setCellTextOrientation(const Vect2i& pos, int angle)
{
	using namespace Excel;
    RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(pos.y, pos.x)->GetResize(1, 1));
    range->Orientation = angle;
}

void ExcelExporterImpl::handleError(_com_error& error)
{
	std::string str = "COM Error: ";
	str += static_cast<const char*>(error.Description());
	::MessageBox(0, str.c_str(), "Error!", MB_ICONERROR | MB_OK);
}

void ExcelExporterImpl::setCellFormat(const Vect2i& pos, const char* format)
{
	try{
		using namespace Excel;
		RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(pos.y, pos.x)->GetResize(1, 1));
		range->NumberFormat = format;
	}
	catch(_com_error& error){
		handleError(error);
		throw;
	}
}

void ExcelExporterImpl::setCellText(const Vect2i& pos, const char* text, int codePage)
{
	using namespace Excel;
    int line = pos.y + 1;
    int column = pos.x + 1;
	if(codePage)
        sheet_->Cells->Item[line][column] = unicode_convert(text, codePage);
	else
		sheet_->Cells->Item[line][column] = text;
}


void ExcelExporterImpl::setCellFloat(const Vect2i& pos, float value)
{
	using namespace Excel;
    int line = pos.y + 1;
    int column = pos.x + 1;
    sheet_->Cells->Item[line][column] = value;
}


void ExcelExporterImpl::removeColumn(int column)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Columns->Item[column + 1]);
	range->Delete(xlToLeft);
}

void ExcelExporterImpl::setColumnWidth(int column, float width)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Columns->Item[column + 1]);

    range->ColumnWidth = width;
}

void ExcelExporterImpl::setColumnWidthAuto(int column)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Columns->Item[column + 1]);

    range->AutoFit();
}

void ExcelExporterImpl::setColumnWrap(int column, bool wrap)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Columns->Item[column + 1]);

    range->WrapText = wrap ? VARIANT_TRUE : VARIANT_FALSE;
}

void ExcelExporterImpl::setRowHeight(int row, float height)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Rows->Item[row + 1]);

    range->RowHeight = height;
}

void ExcelExporterImpl::setRowWrap(int row, bool wrap)
{
	using namespace Excel;
    RangePtr range(sheet_->UsedRange->Rows->Item[row + 1]);

    range->WrapText = wrap ? VARIANT_TRUE : VARIANT_FALSE;
}


void ExcelExporterImpl::setFont(const Recti& rect, const char* fontName, float size, bool bold, bool italic)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(rect.top(), rect.left())->GetResize(rect.height(), rect.width()));
	range->Font->Name = fontName;
	range->Font->Size = size;
	range->Font->Bold = bold ? VARIANT_TRUE : VARIANT_FALSE;
	range->Font->Italic = italic ? VARIANT_TRUE : VARIANT_FALSE;
}

void ExcelExporterImpl::mergeCellRange(const Recti& rect)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(rect.top(), rect.left())->GetResize(rect.height(), rect.width()));
	range->Merge();
	range->HorizontalAlignment = xlCenter;
	range->VerticalAlignment = xlCenter;
}

void ExcelExporterImpl::setTableEdges(const Recti& rect)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(rect.top(), rect.left())->GetResize(rect.height(), rect.width()));
	range->GetBorders()->LineStyle = xlContinuous;
	range->GetBorders()->Weight = xlThin;
}

void ExcelExporterImpl::setLeftEdgeFat(const Recti& rect)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(rect.top(), rect.left())->GetResize(rect.height(), rect.width()));
	range->GetBorders()->GetItem(xlEdgeLeft)->LineStyle = xlContinuous;
	range->GetBorders()->GetItem(xlEdgeLeft)->Weight = xlThick;
}

void ExcelExporterImpl::setTextColor(const Vect2i& pos, int startSymbol, int countSymbol, int colorIndex, const char* fontName, int fontSize, bool bold, bool italic)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(pos.y, pos.x)->GetResize(1, 1));
	CharactersPtr characters(range->GetCharacters(startSymbol, countSymbol));
	characters->Font->ColorIndex = colorIndex;
	characters->Font->Size = fontSize;
	characters->Font->Bold = bold ? VARIANT_TRUE : VARIANT_FALSE;
	characters->Font->Italic = italic ? VARIANT_TRUE : VARIANT_FALSE;
	characters->Font->Name = fontName;
}

void ExcelExporterImpl::setBackColor(const Recti& rect, int colorIndex)
{
	using namespace Excel;
	RangePtr range(sheet_->GetRange("A1", "A1")->GetOffset(rect.top(), rect.left())->GetResize(rect.height(), rect.width()));
	range->Interior->ColorIndex = colorIndex;
}

void ExcelExporterImpl::setPageMargins(float left, float top, float right, float bottom)
{
	sheet_->PageSetup->TopMargin = top;
    sheet_->PageSetup->LeftMargin = left;
    sheet_->PageSetup->RightMargin = right;
    sheet_->PageSetup->BottomMargin = bottom;
}

void ExcelExporterImpl::setPageOrientation(bool landscape)
{
	using namespace Excel;
	sheet_->PageSetup->Orientation = landscape ? xlLandscape : xlPortrait;
}

int ExcelExporterImpl::addSheet(const char* name, int codePage)
{
	int count = application_->Sheets->Count;
	//application_->Sheets->Add(_variant_t(), count, _variant_t(), _variant_t());
	application_->Sheets->Add();
	sheet_ = application_->ActiveSheet;
	setSheetName(name, codePage);
	return count;
}

void ExcelExporterImpl::setSheetName(const char* name, int codePage)
{
	sheet_->Name = name;
}

void ExcelExporterImpl::addHyperlink(const Vect2i& pos, const char* address, const char* subAddress, const char* cellText, const char* tipText)
{
	sheet_->Hyperlinks->Add(rangeFromVect(pos), address, subAddress, tipText, cellText);
}

ExcelExporterImpl::~ExcelExporterImpl()
{
	if(application_) {
		application_->Quit();
	}
}

//////////////////////////////////////////////////////////////////////////////
class ExcelImporterImpl : public ExcelImporter {
public:
    ExcelImporterImpl(const char* filename)
	: application_("Excel.Application")
	, filename_(filename)
	{
        using namespace Excel;

		application_->PutVisible(false, false);

        try	{
            book_ = application_->Workbooks->Open(filename_.c_str());
        }
        catch (_com_error& e) {
            XBuffer buf;
            buf < "COM Error: " < e.ErrorMessage();
            MessageBox(0, buf, "COM Error", MB_OK | MB_ICONERROR);
        }
    }

	~ExcelImporterImpl() {

    }
	
//	void read(TreeNode* node);

	void setCurrentSheet(int index);

	void openSheet() {
        using namespace Excel;

        try	{
            sheet_ = book_->Sheets->Item[1];
            /*
            TreeNode::iterator it;
            FOR_EACH (*node, it) {
                if (it == node->begin()) {
                    ++currentLine_;
                }
                readLine(*it);
            }
            */
        }
        catch (_com_error& e) {
            XBuffer buf;
            buf < "COM Error: " < e.ErrorMessage();
            MessageBox(0, buf, "COM Error", MB_OK | MB_ICONERROR);
        }
    }

	const char* getCellText(const Vect2i& pos, int codePage) {
        _bstr_t value = sheet_->Cells->Item[pos.y + 1][pos.x + 1];
		if(codePage){
			static std::wstring text;
			text = static_cast<const wchar_t*>(value);
			return ansi_convert(text.c_str(), codePage);
		}
		else{
			static std::string text;
			text = static_cast<const char*>(value);
			return text.c_str();
		}
    }

	float getCellFloat(const Vect2i& pos) {
        double value = sheet_->Cells->Item[pos.y + 1][pos.x + 1];
        return float(value);
    }

	void closeSheet() {
        using namespace Excel;

        try	{
            book_->Close();
        }
        catch (_com_error& e) {
            XBuffer buf;
            buf < "COM Error: " < e.ErrorMessage();
            MessageBox(0, buf, "COM Error", MB_OK | MB_ICONERROR);
        }
    }

	Recti getUsedRange() 
	{
		Excel::RangePtr usedRange(sheet_->UsedRange);
		Recti recti_;
		recti_.left(0);
		recti_.top(0);
		recti_.width(usedRange->Columns->Count);
		recti_.height(usedRange->Rows->Count);
		return recti_;
	}


	void free() {
		(*this).closeSheet();
		delete this;
	}
private:
	std::string filename_;

	int currentLine_;
	Excel::_ApplicationPtr application_;
	Excel::_WorkbookPtr book_;
	Excel::_WorksheetPtr sheet_;
};

void ExcelImporterImpl::setCurrentSheet(int index)
{
	using namespace Excel;
	Excel::_WorksheetPtr sheet = application_->Sheets->Item[index + 1];
	sheet->Select();
	sheet_ = application_->ActiveSheet;
}


EXCELEXPORT_API ExcelImporter* ExcelImporter::create(const char* filename)
{
    return new ExcelImporterImpl(filename);
}


