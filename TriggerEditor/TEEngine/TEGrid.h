#ifndef __T_E_GRID_H_INCLUDED__
#define __T_E_GRID_H_INCLUDED__

class TEGrid  
{
public:
	TEGrid();
	TEGrid(int cellWidth, int cellHeight, int amongCellWidth, int amongCellHeight);
	~TEGrid();

	int getCellWidth() const;
	int getCellHeight() const;

	void setCellWidth(int value);
	void setCellHeight(int value);

	int getAmongCellsW() const;
	int getAmongCellsH() const;


	void setAmongCellsW(int value);
	void setAmongCellsH(int value);

	//! Возвращает горизонтальный индекс ячейки, содержащей точку с координатой х
	int getHorzCellIndex(int x) const;
	//! Возвращает вертикальный индекс ячейки, содержащей точку с координатой y
	int getVertCellIndex(int y) const;
	//! возвращает индексы ячейки, в которую попадает точка
	CPoint const getCellByPoint(POINT const& p) const;
	//! Возвращает координату верхнего ребра ячейки, содержащей точку с координатой y
	int toCellTop(int y) const;
	//! Возвращает координату левого ребра ячейки, содержащей точку с координатой x
	int toCellLeft(int x) const;
	//! Возвращает левый верхний угол ячейки по точке
	void toCellLeftTop(POINT * point) const;
	
	//! Возвращает координату верхнего ребра ячейки по индексу
	int getCellTopByIndex(int idx) const;
	//! Возвращает координату левого ребра ячейки по индексу
	int getCellLeftByIndex(int idx) const;

	//! Возвращает полную ширину ячейки(ширина элемента + расстояние между элеметами)
	int getFullCellWidth() const;
	//! Возвращает полную высоту ячейки(ширина элемента + расстояние между элеметами)
	int getFullCellHeight() const;
protected:
	static int toCellEdge(int x, int cell_sz, int among_cells_sz);
	static int getCellIndex(int x, int cell_sz);
	static int indexToCellEdge(int idx, int cell_sz);

private:
	//! Высота прямоугольника
	int iCellHeight_;
	//! Ширина прямоугольника
	int iCellWidth_;
	//! Расстояние между элементами по высоте
	int iAmongCellsH_;
	//! Расстояние между элементами по ширине
	int iAmongCellsW_;
};

#endif
