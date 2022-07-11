//2000 Balmer (Poskryakov Dmitry) 
#pragma once

//Bound - класс предназначенный для хранения в себе некой 
//		  области или границы области
//Bound - Новая редакция - более гибкая работа с памятью
//Bound - Принципиально пассивный класс

//Переписальи с vector, стало универсальнее, но скорее всего возникли дополнительные баги.

struct BoundRange
{
	int xmin,xmax;
};

class Bound
{
public:
	typedef vector<BoundRange> vBoundRange;
	virtual void New();
	virtual void Delete();

	//Копирует строчку с потерей данных в приёмнике
	void CopyLine(int i,//Куда копировать
		vBoundRange& from//Откуда копировать
		);
private:
	void OnLineOr(int ii,vBoundRange& b);

	void OnLineAnd(int ii,vBoundRange& b);

	void CopyLine(int i,//Куда копировать
		Bound& b,int j//Откуда копировать
		);

	void OnLineOrAdd(int ii,vBoundRange& b,int move_x);

	void OnLineClip(int ii,
				  int r_left,int r_right
				  );

	void CopyLinePlus(int i,
		Bound& b,int j,
		int move_x
		);
public:
	vBoundRange* yrange;
	int basey,dy;

	Bound();
	Bound(int _basey,int _dy);
	~Bound();
private:
	Bound(const Bound& ){};
protected:
	bool Realloc(int _basey,int _dy,bool proverka=true);//true - Ok
public:
	vBoundRange temp;
	Bound& operator |=(Bound& bnd);
	Bound& operator &=(Bound& bnd);
	virtual Bound& operator =(Bound& bnd);

	void NewSize(int _basey,int _dy);//Уничтожить старый и создать новый неинициализированный
	
	void InitEmpty(int _basey,int _dy);//Уничтожить старый и создать пустой

	Bound* Not(RECT r);//r - Всё пространство (см. Clip() )
//	Bound& NotAnd(Bound& bnd);

	void Clear();
	//Эта функция очищает Bound
	//	но вот в чём дело, первое что приходит на ум 
	//	это усечь Bound до нулевой высоты
	//	в принципе работать будет, но не следует забывать 
	//	о людях работающих напрямую с данными этого класса
	//	поэтому данная функция очищает Bound построчно

	bool IsEmpty();

	enum{COMP_REALLOC=1,COMP_SUM=2};
	void Compress(int flag=COMP_REALLOC|COMP_SUM);
					//Если есть неиспользуемые строки в начале 
					//или в конце переаллокирует при флаге COMP_REALLOC
					//Если есть блоки между которыми нет пустого места
					//то делает из них один более длинный интервал COMP_SUM

	RECT CalcRect();

	//Удивительно, но факт - вышеперечисленные действия не
	//все простейшие действия которые возможно совершить над 
	//обьектом, далее идут действия переноса Bound с места на место
	void Clip(RECT r);
	//r - здесь можно представить двояко
	//	1 - всё пространство за которым ничего нет
	//	2 - делаем & с Bound сгенерённый из данного r (см. CreateRect)

	void Move(int move_x,int move_y);
	//Переносит обьект с места на место 
	//на расстояние move_x,move_y
	
	Bound& OrMove(Bound& b,int move_x,int move_y,RECT r);
	//функция введена как часто используемая
	//сначала осуществляется сдвиг b на (move_x,move_y)
	//потом обрезка полученного результата прямоугольником r
	//при этом предполагается, что исходная область полностью
	//попадала в r, иначе результат слабоопределённ

	bool IsIn(POINT p);//Находится ли точка внутри?
//	bool In(Bound& b);//Находится ли полигон внутри этого полигона

//	void Copy(ScrBitmap& b,int x,int y);

	void SetRect(int x1,int y1,int x2,int y2);
	void SetCircle(int x0,int y0,int r);
	void SetEllipse(int x1,int y1,int x2,int y2);

	void AddRect(const RECT& rc);//Размер не увеличивается

	void Print(FILE* f);
};


void ShowBound(BYTE* pg,int mx,int my,Bound& b,BYTE color);

void AbsoluteShowBound(BYTE* pg,int x,int y,int mx,int my,Bound& b,BYTE color);
void AbsoluteShowBoundAdd(BYTE* pg,int x,int y,int mx,int my,Bound& b,BYTE color);

Bound* CreateRect(int x1,int y1,int x2,int y2);
Bound* CreateCirc(int x0,int y0,int r);
