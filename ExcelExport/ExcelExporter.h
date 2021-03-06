#ifndef __EXCEL_EXPORTER_H_INCLUDED__
#define __EXCEL_EXPORTER_H_INCLUDED__

#include "XMath/xmath.h"
#include "XTL\Rect.h"

#ifdef EXCELEXPORT_EXPORTS
#define EXCELEXPORT_API __declspec(dllexport)
#else
#define EXCELEXPORT_API __declspec(dllimport)
#endif

class TreeNode;

class ExcelExporter {
public:
	virtual ~ExcelExporter() {};

    virtual void beginSheet() = 0;
	virtual int addSheet(const wchar_t* name) = 0;
	virtual void setSheetName(const wchar_t* name) = 0;
	virtual void setCurrentSheet(int index) = 0;

	virtual void setCellFormat(const Vect2i& pos, const char* format) = 0;
    virtual void setCellText(const Vect2i& pos, const wchar_t* text) = 0;
	virtual void setCellComment(const Vect2i& pos, const wchar_t* comment) = 0;
    virtual void setCellFloat(const Vect2i& pos, float value) = 0;

	virtual void addHyperlink(const Vect2i& pos, const char* address, const char* subAddress, const wchar_t* cellText, const wchar_t* tipText) = 0;

    virtual void setColumnWidth(int column, float width) = 0;
	virtual void setColumnWidthAuto(int column) = 0;
    virtual void setColumnWrap(int column, bool wrap) = 0;

	virtual void removeColumn(int column) = 0;
    virtual void setRowHeight(int row, float height) = 0;
    virtual void setRowWrap(int row, bool wrap) = 0;
	virtual void setCellTextOrientation(const Vect2i& pos, int angle) = 0; 
				
	virtual void setTextColor(const Vect2i& pos, int startSymbol, int countSymbol, int colorIndex, const char* fontName, int fontSize, bool bold, bool italic) = 0;	
    virtual void setFont(const Recti& range, const char* fontName, float size, bool bold = false, bool italic = false) = 0;
    virtual void setBackColor(const Recti& range, int colorIndex) = 0;

	virtual void setPageMargins(float left, float top, float right, float bottom) = 0;
	virtual void setPageOrientation(bool landscape) = 0;

    virtual void endSheet() = 0;
	virtual void free() = 0;
	virtual void mergeCellRange(const Recti& rect) = 0;
	virtual void setTableEdges(const Recti& rect) = 0;
	virtual void setLeftEdgeFat(const Recti& rect) = 0;
	virtual const char* cellName(const Vect2i& pos) = 0;

	EXCELEXPORT_API static ExcelExporter* create(const char* filename);
};

class ExcelImporter {
public:
    //ExcelImporter(const char* filename);
	virtual ~ExcelImporter() {};
	
//	void read(TreeNode* node);

	virtual void openSheet() = 0;
	virtual void setCurrentSheet(int index) = 0;

	virtual const wchar_t* getCellText(const Vect2i& pos) = 0;
	virtual float getCellFloat(const Vect2i& pos) = 0;

	virtual void closeSheet() = 0;
	virtual void free() = 0;

	virtual Recti getUsedRange() = 0;

	EXCELEXPORT_API static ExcelImporter* create(const char* filename);
};

#ifndef _NO_EXCEL_EXPORT_AUTOMATIC_LINK_
#pragma message("Automatically linking with ExcelExport.lib") 
#pragma comment(lib, "ExcelExport.lib")
#endif

#endif
