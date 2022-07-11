#!/usr/bin/python


import win32com.client as com

xlDown  = -4121
xlToLeft  = -4159
xlToRight = -4161

xlLastCell = 11

columnNames = [ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' ]

class Excel:
    def __init__(s):
        s.app = com.Dispatch(u'Excel.Application')
        s.app.DisplayAlerts = False

    def __del__(s):
        if(not s.app.Visible):
            s.close()

    def close(s):
        s.app.Quit()

    def newWorkbook(s):
        s.app.Workbooks.Add()

    def show(s):
        s.app.Visible = 1

    def open(s, fileName):
        s.app.Workbooks.Open(fileName)

    def saveAs(s, fileName):
        s.app.Workbooks(1).SaveAs(fileName)

    def beginSheet(s):
        pass

    def endSheet(s):
        pass

    def setCurrentSheet(s, sheetName):
        s.app.Sheets(sheetName).Select()

    def removeSheet(s, sheetName):
        #s.setCurrentSheet(sheetName)
        s.app.Sheets(sheetName).Delete()

    def setSheetName(s, newName):
        s.app.ActiveSheet.Name = newName

    def duplicateSheet(s, newName):
        s.app.Sheets(s.app.Sheets.Count).Select()
        s.app.Sheets(s.app.Sheets.Count).Copy(Before = s.app.ActiveWorkbook.Sheets(s.app.ActiveWorkbook.Sheets.Count))
        s.app.ActiveSheet.Name = newName

    def setCellText(s, pos, text):
        assert(not text is None)
        assert(type(text) == unicode or type(text) == str or type(text) == int or type(text) == float)
        s.app.Cells(pos[1] + 1, pos[0] + 1).Value = text

    def lockCell(s, pos):
        s.app.Cells(pos[1] + 1, pos[0] + 1).Locked = True

    def lockRange(s, pos, size):
        s.app.Cells(pos[1] + 1, pos[0] + 1).Resize(size[0], size[1]).Locked = True

    def getCellText(s, pos):
        value = s.app.Cells(pos[1] + 1, pos[0] + 1).Value
        if value is None:
            return u""
        else:
            return value

    def usedRange(s):
        width = s.app.ActiveSheet.UsedRange.Columns.Count
        height = s.app.ActiveSheet.UsedRange.Rows.Count
        return (width, height)

    def cellName(s, pos):
        return u"$%s$%i" % (columnNames[pos[0]], pos[1] + 1)

    def setColumnWidth(s, column, width):
        s.app.ActiveSheet.UsedRange.Columns(column + 1).ColumnWidth = float(width)

    def setColumnWidthAuto(s, column, wrap):
        pass

    def insertRowBefore(s, rowIndex, count = 1):
        for i in xrange(0, count):
            s.app.Range(u"A%i" % (rowIndex + 1 + i)).EntireRow.Insert(xlDown, 0)

    def removeRow(s, rowIndex):
        s.app.Range(u"A%i" % (rowIndex + 1)).EntireRow.Delete()

    def findCellsByText(s, text):
        result = []
        sel = s.app.ActiveCell.SpecialCells(xlLastCell)

        width = int(sel.Column)
        height = int(sel.Row)

        for i in xrange(0, width):
            for j in xrange(0, height):
                cell_text = s.getCellText((i, j))
                if(cell_text == text):
                    result += [(i, j)]

        return result

