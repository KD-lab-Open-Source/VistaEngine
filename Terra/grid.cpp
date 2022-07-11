#include "stdafxTr.h"

#include "vmap.h"

/**/

/*
	PointInPolygon
	Находится ли точка внутри полигона.
	Построчный алгоритм:
	  Если линия перпендикулярная оси X пересекает до точки point
	  полигон нечётное количество раз, то она находится внутри полигона.
	  Паралельные отрезки не учитываем.
	  
*/
/*template<class T>
bool PointInPolygon (POINT p,T* pg,int N)
{
    int c = 0;
    for (int i = 0, j = N-1; i < N; j = i++)
    {
        if ((pg[i].y<=p.y && p.y<pg[j].y && 
				(pg[j].y-pg[i].y)*(p.x-pg[i].x)<(pg[j].x-pg[i].x)*(p.y-pg[i].y))
        ||  (pg[j].y<=p.y && p.y<pg[i].y && 
				(pg[j].y-pg[i].y)*(p.x-pg[i].x)>(pg[j].x-pg[i].x)*(p.y-pg[i].y)))
        {
            c ^= 1;
        }
    }
    return c?true:false;
}*/






/////////////////////////////////////////////////////////////////////////////////////////////

void vrtMap::initGrid()
{
	recalcArea2Grid(0,0,H_SIZE,V_SIZE, true);
}

void vrtMap::recalcArea2Grid(int xl, int yt, int xr, int yb , bool flag_init)
{
	if(xl > xr || yt > yb) return;
	unsigned short maskClr;
	if(flag_init) maskClr=0;
	else maskClr=0xffFF;

	//int fullhZeroPlast=hZeroPlast<<VX_FRACTION;
	int xlG=xl>>kmGrid;
	int ytG=yt>>kmGrid;

	int dxG=(xr>>kmGrid)-xlG+1;
	if(dxG > H_SIZE>>kmGrid)dxG=H_SIZE>>kmGrid;
	int dyG=(yb>>kmGrid)-ytG+1;
	if(dyG > V_SIZE>>kmGrid)dyG=V_SIZE>>kmGrid;

	int i,j,kx,ky;
	for(i=0; i<dyG; i++){
		for(j=0; j<dxG; j++){
			int of=offsetBuf( XCYCL((xlG+j)<<kmGrid), YCYCL((ytG+i)<<kmGrid) ); 
			int Sum=0;
			unsigned char LVD=0;
			unsigned char INDS=0;
			char flag_taller_h_ZL=0;
			for(ky=0; ky<sizeCellGrid; ky++){
				for(kx=0; kx<sizeCellGrid; kx++){
					//if(Vm_IsLeveled(vxaBuf[of+kx]))
					//	LVD++;
					if(Vm_IsIndestructability(vxaBuf[of+kx]))
						INDS++;
					unsigned short v=getAlt(of+kx);
					Sum+=v;
					//if(v > fullhZeroPlast)
					//	flag_taller_h_ZL=1;
				}
				of+=H_SIZE; //Переход на следующую строку //Зацикливание не нужно т.к. ячейка не переходит границы карты никогда
			}
			int ofG=offsetGBuf(XCYCLG(xlG+j),YCYCLG(ytG+i));
			gVBuf[ofG]=(Sum+1) >>(kmGrid+kmGrid+VX_FRACTION); //Округление и деление на 16 (4x4) и отбрасывание дроби
			//ОБнуляем атрибуты и выставляем признак подготовленности для зеропласта
			unsigned short atg=gABuf[ofG]&maskClr;
			atg&= (~GRIDAT_LEVELED) & (~GRIDAT_INDESTRUCTABILITY); //& (~GRIDAT_TALLER_HZEROPLAST)
			if(LVD==(sizeCellGrid*sizeCellGrid)) atg|=GRIDAT_LEVELED;
			//if(flag_taller_h_ZL) atg|=GRIDAT_TALLER_HZEROPLAST;
			if(INDS > 2*sizeCellGrid) atg|=GRIDAT_INDESTRUCTABILITY;
			gABuf[ofG]=atg;
		}
	}
}

