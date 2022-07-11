#include "stdafxTr.h"

#include "Render\inc\IRenderDevice.h"
#include "terTools.h"
#include "tools.h"

//#define PN_FRACTION (16)
//#define PN_ROUND_MAKEWEIGHT (1<<15)
//#define PN_HALF_FRACTION (8)
#define PN_FRACTION (18)
#define PN_ROUND_MAKEWEIGHT (1<<17)
#define PN_HALF_FRACTION (9)
//#define PN_FRACTION (24)
//#define PN_ROUND_MAKEWEIGHT (1<<23)
//#define PN_HALF_FRACTION (12)

//#define roundFIntF0(a) ((a)>>16)
//#define mroundFIntF0(a) (((a)+(1<<15))>>16)
//#define cvrtFIntF8(a) ((a+(1<<7))>>8)
//#define ceilFIntF16(a) (((a)+0xffFF)&0xFFff0000)
//#define ceilFIntF0(a) (((a)+0xffFF)>>16)
//#define floorFIntF0(a) ((a)>>16)
//#define floorFIntF16(a) ((a)&0xFFff0000)

#define roundFIntF0(a) ((a)>>18)
#define mroundFIntF0(a) (((a)+(1<<17))>>18)
#define cvrtFIntF8(a) ((a+(1<<8))>>9)
#define ceilFIntF16(a) (((a)+0x3ffFF)&0xFFfc0000)
#define ceilFIntF0(a) (((a)+0x3ffFF)>>18)
#define floorFIntF0(a) ((a)>>18)
#define floorFIntF16(a) ((a)&0xFFfC0000)

//#define roundFIntF0(a) ((a)>>24)
//#define mroundFIntF0(a) (((a)+(1<<23))>>24)
//#define cvrtFIntF8(a) ((a+(1<<11))>>12)
//#define ceilFIntF16(a) (((a)+0xFFffFF)&0xffFFffFFff000000)
//#define ceilFIntF0(a) (((a)+0xFFffFF)>>24)
//#define floorFIntF0(a) ((a)>>24)
//#define floorFIntF16(a) ((a)&0xffFFffFFff000000)


#define SUBPIXEL
#define SUBTEXEL

void drawPoligon(Vect3f* a, Vect3f* b, Vect3f* c, sVoxelBitmap& voxelBitmap);

bool putModel2VBitmap(const char* fName, const Se3f& pos, sVoxelBitmap& voxelBitmap, bool flag_placePlane, float scaleFactor, float zScale)
{
	vector<sPolygon> poligonArr;
	vector<Vect3f> pointArr;
	GetAllTriangle3dx(fName, pointArr, poligonArr);

	if(!pointArr.size()) return false;


	//MatXf pose(pos);
	MatXf pose;
	if(flag_placePlane){
		pose.set(pos);
	}
	else {
		QuatF q(pos.rot());
		q.x()=0; q.y()=0; q.norm();
		//q.z()=0; q.s()=1;
		//pose.set(Mat3f::ID, pos.trans());
		pose.set(q, pos.trans());
	}
	Vect3f scalefactor(scaleFactor, scaleFactor, scaleFactor*zScale);
	pose.rot().scale(scalefactor);
	//pose.rot().scale(model->GetScale());
	//pose.rot().scale(4.0f);

	pose.xformPoint(pointArr[0]);
	float minX,maxX; minX=maxX=pointArr[0].x;
	float minY,maxY; minY=maxY=pointArr[0].y;
	float minZ; minZ=pointArr[0].z;
	int i;
	for(i = 1; i<pointArr.size(); i++){
		pose.xformPoint(pointArr[i]);
		if(pointArr[i].x < minX) minX=pointArr[i].x;
		else if(pointArr[i].x > maxX)maxX=pointArr[i].x;
		if(pointArr[i].y < minY) minY=pointArr[i].y;
		else if(pointArr[i].y > maxY) maxY=pointArr[i].y;
		if(pointArr[i].z < minZ) minZ=pointArr[i].z;
	}

	int iminX=round(minX-1);
	int iminY=round(minY-1);
	voxelBitmap.minCoord.x=minX;
	voxelBitmap.minCoord.y=minY;
	voxelBitmap.minCoord.z=minZ;
	voxelBitmap.relativeCenterZ=round((pos.trans().z-minZ)*(1<<VX_FRACTION));

	//vector<Vect3i> iPntArr;
	//iPntArr.resize(pointArr.size());
	//for (i = 0; i < pointArr.size(); i++){
	//	//float x=pointArr[i].x;
	//	//float y=pointArr[i].y;
	//	//float z=pointArr[i].z;
	//	//xassert(abs(x)<32000);
	//	//xassert(abs(y)<32000);
	//	//xassert(abs(z)<32000);
	//	iPntArr[i].x=round((pointArr[i].x-minX)*(1<<PN_FRACTION));
	//	iPntArr[i].y=round((pointArr[i].y-minY)*(1<<PN_FRACTION));
	//	iPntArr[i].z=round((pointArr[i].z-minZ)*(1<<PN_FRACTION));
	//}

	for (i = 0; i < pointArr.size(); i++){
		pointArr[i].x=pointArr[i].x-minX;
		pointArr[i].y=pointArr[i].y-minY;
		pointArr[i].z=pointArr[i].z-minZ;
	}

	voxelBitmap.create(round(maxX+1)- round(minX -1), round(maxY+1)- round(minY -1));
	memset(voxelBitmap.pRaster, 0xFF, sizeof(voxelBitmap.pRaster[0])*voxelBitmap.sx*voxelBitmap.sy);

	vector<sPolygon>::iterator p;
	for(p=poligonArr.begin(); p!=poligonArr.end(); p++){
		drawPoligon(&pointArr[p->p1], &pointArr[p->p2], &pointArr[p->p3], voxelBitmap);
	}
	return true;
}


void drawPoligon(Vect3f* a, Vect3f* b, Vect3f* c, sVoxelBitmap& voxelBitmap)
{
	//float -алгоритм
	if(a->y > b->y) swap(a, b);
	if(a->y > c->y) swap(a, c);
	if(b->y > c->y) swap(b, c);
	// грань нулевой высоты рисовать не будем( а надо, для того-чтоб не пропадали точки когда очень много полигонов на точку)
	///if (round(c->y) <= round(a->y)) continue;

	int current_sx, current_sy;
	float tmp, k, x_start, x_end;
	float dx_start, dx_end, dz1_start, dz1_end;
	float z1_start, z1_end;
	float x, z1, dz1;
	int length;
	//unsigned short *dest;

	// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
	// считаем по самой длинной линии (т.е. проходящей через вершину B)
	float divisor;
	divisor=(c->y - a->y);
	if(divisor) k = (b->y - a->y) / divisor;
	else k=0;
	x_start = a->x + (c->x - a->x) * k;
	z1_start = a->z + (c->z - a->z) * k;
	x_end = b->x;
	z1_end = b->z;
	divisor=(x_start - x_end);
	if(divisor) dz1 = (z1_start - z1_end) / divisor;
	else dz1 = 0;

	x_start = a->x;
	z1_start = a->z;
	divisor=(c->y - a->y);
	if(divisor){
		dx_start = (c->x - a->x) / divisor;
		dz1_start = (c->z - a->z) / divisor;
	}
	else { dx_start =0; dz1_start=0;}
#ifdef SUBPIXEL
	tmp = ceilf(a->y) - a->y;
	x_start += dx_start * tmp;
	z1_start += dz1_start * tmp;
#endif

	if (ceilf(b->y) > ceilf(a->y)) {
		tmp = ceilf(a->y) - a->y;
		x_end = a->x;
		z1_end = a->z;
		divisor=(b->y - a->y);
		if(divisor){
			dx_end = (b->x - a->x) / divisor;
			dz1_end = (b->z - a->z) / divisor;
		}
		else { dx_end=0; dz1_end=0; }
	} else {
		tmp = ceilf(b->y) - b->y;
		x_end = b->x;
		z1_end = b->z;
		divisor=(c->y - b->y);
		if(divisor){
			dx_end = (c->x - b->x) / divisor;
			dz1_end = (c->z - b->z) / divisor;
		}
		else { dx_end=0; dz1_end=0; }
	}
#ifdef SUBPIXEL
	x_end += dx_end * tmp;
	z1_end += dz1_end * tmp;
#endif

////////////////////////////////

	// построчная отрисовка грани
	for (current_sy = ceilf(a->y); current_sy <= floorf(c->y); current_sy++) { //current_sy < ceilf(c->y)
		if((current_sy) >= voxelBitmap.sy) break; //-iminY
		//if((current_sy-minY) < 0 ) break;//continue;
		if (current_sy == ceilf(b->y)) {
			x_end = b->x;
			z1_end = b->z;
			divisor=(c->y - b->y);
			if(divisor){
                dx_end = (c->x - b->x) / divisor;
				dz1_end = (c->z - b->z) / divisor;
			}
			else { dx_end=0; dz1_end=0; }
#ifdef SUBPIXEL
			tmp = ceilf(b->y) - b->y;
			x_end += dx_end * tmp;
			z1_end += dz1_end * tmp;
#endif
		}

		// x_start должен находиться левее x_end
		if (x_start > x_end) {
			x = x_end;
			z1 = z1_end;
			length = ceilf(x_start) - ceilf(x_end);
		} else {
			x = x_start;
			z1 = z1_start;
			length = ceilf(x_end) - ceilf(x_start);
		}

		// считаем адрес начала строки в видеопамяти
		//dest = GB;
		//dest += current_sy * sizeX05*2 + (int)ceilf(x);

		// текстурируем строку
		current_sx = round(ceilf(x));//-iminX;

		if((current_sy) >= 0 ) if (length) { //-iminY
	#ifdef SUBTEXEL
		    tmp = ceilf(x) - x;
			z1 += dz1* tmp;
	#endif
			while (length--) {
			// используем z-буфер для определения видимости текущей точки
				if( (current_sx<voxelBitmap.sx) && (current_sx >= 0)) {
					//if (zBuffer[(current_sy-iminY)*voxelBitmap.sx + current_sx] <= z1) {
					//	//*dest = palLight[round(cc)];
					//	zBuffer[(current_sy-iminY)*voxelBitmap.sx + current_sx] = z1;
					//}
					register int bufoff=(current_sy)*voxelBitmap.sx + current_sx; //-iminY
					if(voxelBitmap.pRaster[bufoff]<= round(z1*(1<<VX_FRACTION)) ){
						voxelBitmap.pRaster[bufoff] = round(z1*(1<<VX_FRACTION));
					}
				}
				z1 += dz1;
				//dest++;
				current_sx++;
			}
		}

		// сдвигаем начальные и конечные значения x/u/v/(1/z)
		x_start += dx_start;
		z1_start += dz1_start;
		x_end += dx_end;
		z1_end += dz1_end;
	}


/*
	//1-й целочисленный алгоритм - на балистик- 1 точка

	const Vect3i* a = &iPntArr[p->p1]; // Для сортировки по Y.
	const Vect3i* b = &iPntArr[p->p2];
	const Vect3i* c = &iPntArr[p->p3];
	if(a->y > b->y) swap(a, b);
	if(a->y > c->y) swap(a, c);
	if(b->y > c->y) swap(b, c);

	int curMinX,curMaxX;
	curMinX=curMaxX=a->x;
	if(curMinX > b->x)curMinX=b->x;
	if(curMaxX < b->x)curMaxX=b->x;
	if(curMinX > c->x)curMinX=c->x;
	if(curMaxX < c->x)curMaxX=c->x;
	curMinX=clamp(curMinX, 0, round(maxX)<<PN_FRACTION);
	curMaxX=clamp(curMaxX, 0, round(maxX)<<PN_FRACTION);
	//curMinX=0;
	//curMaxX=round(maxX)<<PN_FRACTION;

	//if(roundFIntF0(c->y - a->y)<=0) continue;

	int current_sx, current_sy;

	//float
	int k, tmp, x_start, x_end; 
	int dx_start, dx_end, dz1_start, dz1_end;
	int z1_start, z1_end;
	int x, z1, dz1;

	int length;
	//unsigned short *dest;

	// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
	// считаем по самой длинной линии (т.е. проходящей через вершину B)
	int divisor;
	divisor=(c->y - a->y);
	//if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_HALF_FRACTION) / divisor;// F8
	//else k=0;
	//x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
	//z1_start = a->z + (cvrtFIntF8(c->z - a->z))*(k);
	if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_FRACTION) / divisor;// F16
	else k=0;
	x_start = a->x + (int)mroundFIntF0((__int64)(c->x - a->x)*(k));//На самом деле F16(F18)!
	z1_start = a->z + (int)mroundFIntF0((__int64)(c->z - a->z)*(k));
	x_end = b->x;
	z1_end = b->z;

	divisor= x_start - x_end;
	if(roundFIntF0(divisor)) {
		dz1 = (((__int64)(z1_start - z1_end)<<PN_FRACTION)/divisor);
	}
	else dz1=0;

	x_start = a->x;
	z1_start = a->z;
	divisor=(c->y - a->y);
	if(roundFIntF0(divisor)){
		dx_start = ((__int64)(c->x - a->x)<<PN_FRACTION) / divisor; 
		dz1_start = ((__int64)(c->z - a->z)<<PN_FRACTION) / divisor; 
	}
	else { dx_start=0; dz1_start=0;}
#ifdef SUBPIXEL
	tmp = ceilFIntF16(a->y) - a->y;
	x_start += (__int64)dx_start*tmp>>PN_FRACTION;//(dx_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
	z1_start += (__int64)dz1_start*tmp>>PN_FRACTION;//(dz1_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif
	if (ceilFIntF16(b->y) > ceilFIntF16(a->y)) {
		tmp = ceilFIntF16(a->y) - (a->y);
		x_end = a->x;
		z1_end = a->z;
		divisor=b->y - a->y;
		if(roundFIntF0(divisor)){ 
			dx_end = ((__int64)(b->x - a->x)<<PN_FRACTION) / divisor;
            dz1_end = ((__int64)(b->z - a->z)<<PN_FRACTION) / divisor;
		}
		else {dx_end =0; dz1_end =0; }
	} else {
		tmp = ceilFIntF16(b->y) - b->y;			//???????????
		x_end = b->x;
		z1_end = b->z;
		divisor=c->y - b->y;
		if(roundFIntF0(divisor)){
			dx_end = ((__int64)(c->x - b->x)<<PN_FRACTION) / divisor;
            dz1_end = ((__int64)(c->z - b->z)<<PN_FRACTION) / divisor;
		}
		else{ dx_end=0; dz1_end=0; }
	}
#ifdef SUBPIXEL
	x_end += (__int64)dx_end*tmp>>PN_FRACTION;//(dx_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
	z1_end += (__int64)dz1_end*tmp>>PN_FRACTION;//(dz1_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif

////////////////////////////////
//loc_scip01:;
	// построчная отрисовка грани
	for (current_sy = ceilFIntF0(a->y); current_sy <= floorFIntF0(c->y); current_sy++) { //ceilFIntF0(c->y)
		if((current_sy) >= voxelBitmap.sy ) break;
		if (current_sy == ceilFIntF0(b->y)) {
			x_end = b->x;
			z1_end = b->z;
			divisor=(c->y - b->y);
			if(roundFIntF0(divisor)){
				dx_end=((__int64)(c->x - b->x)<<PN_FRACTION)/divisor;
				dz1_end=((__int64)(c->z - b->z)<<PN_FRACTION)/divisor;
			}
			else { dx_end=0; dz1_end=0; }
#ifdef SUBPIXEL
			tmp = ceilFIntF16(b->y) - b->y;
			x_end += (__int64)dx_end*tmp>>PN_FRACTION;//(dx_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
			z1_end += (__int64)dz1_end*tmp>>PN_FRACTION;//(dz1_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif
		}

		//xassert(x_start >= curMinX -(1<<16));
		//xassert(x_start <= curMaxX +(1<<16));
		//xassert(x_end >= curMinX -(1<<16));
		//xassert(x_end <= curMaxX +(1<<16));
		//x_start=clamp(x_start, curMinX, curMaxX);
		//x_end=clamp(x_end, curMinX, curMaxX);
		// x_start должен находиться левее x_end
		if (x_start > x_end) {
			x = x_end;
			z1 = z1_end;
			length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
		} else {
			x = x_start;
			z1 = z1_start;
			length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
		}
		length++;

		// текстурируем строку
		current_sx = ceilFIntF0(x);

		if((current_sy) >= 0 ) if (length) {
	#ifdef SUBTEXEL
		    //tmp = ceil(x) - x;
		    tmp = ceilFIntF16(x) - x;
			z1 += (__int64)dz1*tmp>>PN_FRACTION;//(dz1>>PN_HALF_FRACTION)* (tmp>>PN_HALF_FRACTION);
	#endif
			while (length--) {
			// используем z-буфер для определения видимости текущей точки
				//xassert(current_sx <= ceilFIntF0(a->x)-iminX+10 || current_sx <= ceilFIntF0(b->x)-iminX+10 || current_sx <= ceilFIntF0(c->x)-iminX+10);
				//xassert(current_sx >= ceilFIntF0(a->x)-iminX-10 || current_sx >= ceilFIntF0(b->x)-iminX-10 || current_sx >= ceilFIntF0(c->x)-iminX-10);
				//xassert(z1 <= a->z+(20<<16) || z1 <= b->z+(20<<16) || z1 <= c->z+(20<<16));
				//xassert(z1 >= a->z-(20<<16) || z1 >= b->z-(20<<16) || z1 >= c->z-(20<<16));
				if( (current_sx<voxelBitmap.sx) && (current_sx >= 0)) {
					register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
					if(voxelBitmap.pRaster[bufoff]<= z1>>(PN_FRACTION-VX_FRACTION) ){
						voxelBitmap.pRaster[bufoff] = z1>>(PN_FRACTION-VX_FRACTION);
					}
				}
				z1 += dz1;
				current_sx++;
			}
		}

		// сдвигаем начальные и конечные значения x/u/v/(1/z)
		x_start += dx_start;
		z1_start += dz1_start;
		x_end += dx_end;
		z1_end += dz1_end;
	}
*/

/*
	//2-й целочисленный алгоритм - баги на wall на балистик- 1 точка
#undef SUBPIXEL
#undef SUBTEXEL
	const Vect3i* a = &iPntArr[p->p1]; // Для сортировки по Y.
	const Vect3i* b = &iPntArr[p->p2];
	const Vect3i* c = &iPntArr[p->p3];
	if(a->y > b->y) swap(a, b);
	if(a->y > c->y) swap(a, c);
	if(b->y > c->y) swap(b, c);

	int curMinX,curMaxX;
	curMinX=curMaxX=a->x;
	if(curMinX > b->x)curMinX=b->x;
	if(curMaxX < b->x)curMaxX=b->x;
	if(curMinX > c->x)curMinX=c->x;
	if(curMaxX < c->x)curMaxX=c->x;
	curMinX=clamp(curMinX, 0, round(maxX)<<PN_FRACTION);
	curMaxX=clamp(curMaxX, 0, round(maxX)<<PN_FRACTION);
	//curMinX=0;
	//curMaxX=round(maxX)<<PN_FRACTION;

	//if(roundFIntF0(c->y - a->y)<=0) continue;


	//float
	int k, tmp, x_start, x_end; 
	int dx_start, dx_end, dz1_start, dz1_end;
	int z1_start, z1_end;
	int dz1;

	//unsigned short *dest;

	// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
	// считаем по самой длинной линии (т.е. проходящей через вершину B)
	int divisor;
	divisor=(c->y - a->y);
	//if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_HALF_FRACTION) / divisor;// F8
	//else k=0;
	//x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
	//z1_start = a->z + (cvrtFIntF8(c->z - a->z))*(k);
	if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_FRACTION) / divisor;// F16
	else k=0;
	x_start = a->x + (int)mroundFIntF0((__int64)(c->x - a->x)*(k));//На самом деле F16(F18)!
	z1_start = a->z + (int)mroundFIntF0((__int64)(c->z - a->z)*(k));
	x_end = b->x;
	z1_end = b->z;

	divisor= x_start - x_end;
	if(roundFIntF0(divisor)) {
		dz1 = (((__int64)(z1_start - z1_end)<<PN_FRACTION)/divisor);
	}
	else dz1=0;

	x_start = a->x;
	z1_start = a->z;
	divisor=(c->y - a->y);
	if(roundFIntF0(divisor)){
		dx_start = ((__int64)(c->x - a->x)<<PN_FRACTION) / divisor; 
		dz1_start = ((__int64)(c->z - a->z)<<PN_FRACTION) / divisor; 
	}
	else { dx_start=0; dz1_start=0;}
#ifdef SUBPIXEL
	tmp = ceilFIntF16(a->y) - a->y;
	x_start += (__int64)dx_start*tmp>>PN_FRACTION;//(dx_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
	z1_start += (__int64)dz1_start*tmp>>PN_FRACTION;//(dz1_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif

	int current_sy=mroundFIntF0(a->y);
	if(mroundFIntF0(b->y) > current_sy) {
		x_end = a->x;
		z1_end = a->z;
		divisor=b->y - a->y;
		if(roundFIntF0(divisor)){ 
			dx_end = ((__int64)(b->x - a->x)<<18) / divisor;
			dz1_end = ((__int64)(b->z - a->z)<<18) / divisor;
		}
		else {dx_end =0; dz1_end=0; }
#ifdef SUBPIXEL
		//tmp = ceilFIntF16(a->y) - (a->y);
		tmp = mroundFIntF0(a->y) - (a->y);
		x_end += (dx_end>>8) * (tmp>>8); //Норма
		z1_end += (dz1_end>>8) * (tmp>>8); //Норма
#endif
		// построчная отрисовка грани
		do {
			// x_start должен находиться левее x_end
			x_start=clamp(x_start, curMinX, curMaxX);
			x_end=clamp(x_end, curMinX, curMaxX);
			int x,z1;
			int xe;
			if (x_start > x_end) {
				x = x_end;
				z1 = z1_end;
				//length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
				//xe = ceilFIntF0(x_start);
				xe = mroundFIntF0(x_start);
			}else {
				x = x_start;
				z1 = z1_start;
				//length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
				//xe = ceilFIntF0(x_end);
				xe = mroundFIntF0(x_end);
			}
			// текстурируем строку
			//int current_sx = ceilFIntF0(x);
			int current_sx = mroundFIntF0(x);
			xassert(current_sy>=0 && current_sy < voxelBitmap.sy);
			//if((current_sy) >= 0 ) if (length) {
	#ifdef SUBTEXEL
				tmp = ceilFIntF16(x) - x;
				z1 += (dz1>>8)* (tmp>>8);
	#endif
				for(current_sx; current_sx<=xe; current_sx++){
					register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
					if(voxelBitmap.pRaster[bufoff] <= z1>>(18-VX_FRACTION) ){
						voxelBitmap.pRaster[bufoff] = z1>>(18-VX_FRACTION);
					}
					z1 += dz1;
				}
			//}
			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			x_end += dx_end;
			z1_start += dz1_start;
			z1_end += dz1_end;
			current_sy++;
		}while(current_sy < mroundFIntF0(b->y));
	}
	///////////////////////////////////////
	if(mroundFIntF0(c->y) >= current_sy) {
		x_end = b->x;
		z1_end = b->z;
		divisor = c->y - b->y;
		if(roundFIntF0(divisor)){
			dx_end = ((__int64)(c->x - b->x)<<18) / divisor;
			dz1_end = ((__int64)(c->z - b->z)<<18) / divisor;
		}
		else{ dx_end=0; dz1_end=0; }
	#ifdef SUBPIXEL
		//tmp = ceilFIntF16(b->y) - b->y;
		tmp = mroundFIntF0(b->y) - b->y;
		x_end += (dx_end>>8) * (tmp>>8); //Норма
		z1_end += (dz1_end>>8) * (tmp>>8); //Норма
	#endif
		// построчная отрисовка грани
		// построчная отрисовка грани
		do {
			// x_start должен находиться левее x_end
			x_start=clamp(x_start, curMinX, curMaxX);
			x_end=clamp(x_end, curMinX, curMaxX);
			int x,z1;
			int xe;
			if (x_start > x_end) {
				x = x_end;
				z1 = z1_end;
				//length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
				//xe = ceilFIntF0(x_start);
				xe = mroundFIntF0(x_start);
			}else {
				x = x_start;
				z1 = z1_start;
				//length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
				//xe = ceilFIntF0(x_end);
				xe = mroundFIntF0(x_end);
			}
			// текстурируем строку
			//int current_sx = ceilFIntF0(x);
			int current_sx = mroundFIntF0(x);
			xassert(current_sy>=0 && current_sy < voxelBitmap.sy);
			//if((current_sy) >= 0 ) if (length) {
	#ifdef SUBTEXEL
				tmp = ceilFIntF16(x) - x;
				z1 += (dz1>>8)* (tmp>>8);
	#endif
				for(current_sx; current_sx<=xe; current_sx++){
					register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
					if(voxelBitmap.pRaster[bufoff] <= z1>>(18-VX_FRACTION) ){
						voxelBitmap.pRaster[bufoff] = z1>>(18-VX_FRACTION);
					}
					z1 += dz1;
				}
			//}
			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			x_end += dx_end;
			z1_start += dz1_start;
			z1_end += dz1_end;
			current_sy++;
		}while(current_sy <= mroundFIntF0(c->y));
	}*/
	
}

void putVoxelBitmap2VMap(const sVoxelBitmap& voxelBitmap, bool flag_relAbs, bool flag_putDig)
{
	short minimalZ=0;
	if(flag_relAbs==false)
		TerrainMetod.mode=sTerrainMetod::PM_Relatively;
	else { //Absolutly
		minimalZ = round(voxelBitmap.minCoord.z*(1<<VX_FRACTION));
        if(flag_putDig==0) //
			TerrainMetod.mode=sTerrainMetod::PM_AbsolutelyMAX;
		else
			TerrainMetod.mode=sTerrainMetod::PM_AbsolutelyMIN;
	}
	const int SCALING4PUT2EARCH=1;
	int sSizeY=voxelBitmap.sy;//sizeY/SCALING4PUT2EARCH;
	int sSizeX=voxelBitmap.sx;//sizeX/SCALING4PUT2EARCH;
	int minX=round(voxelBitmap.minCoord.x);
	int minY=round(voxelBitmap.minCoord.y);
	if(vMap.isRecordingPMO()) 
		vMap.UndoDispatcher_PutPreChangedArea( sRect(minX, minY, sSizeX, sSizeY), 1,0 ); //position.x+minX/SCALING4PUT2EARCH+ //position.y+minY/SCALING4PUT2EARCH+
	int i,j,k,m;//,i;
	for(i=0; i<sSizeY; i++){
		for(j=0; j<sSizeX; j++){
			//float h=0;
			int dv=0;
			for(k=0; k<SCALING4PUT2EARCH; k++){
				for(m=0; m<SCALING4PUT2EARCH; m++){
					//int off=(i*SCALING4PUT2EARCH+k)*sizeX+(j*SCALING4PUT2EARCH+m); 
					int off=(i+k)*voxelBitmap.sx+ (j + m); 
					if(voxelBitmap.pRaster[off]!=0){
						dv+=voxelBitmap.pRaster[off];
					}
				}
			}
			if(dv>=0){
				int x=minX+j;
				int y=minY+i;
				if(x>=0 && x<vMap.H_SIZE && y>=0 && y<vMap.V_SIZE)
					TerrainMetod.put( x, y, minimalZ + (flag_putDig==0 ? dv : -dv) ); //round(h/((float)SCALING4PUT2EARCH*SCALING4PUT2EARCH)*(1<<VX_FRACTION))
			}
		}
	}
	vMap.recalcArea2Grid(minX, minY, minX+sSizeX, minY+sSizeY);
	vMap.regRender( minX, minY, (minX+sSizeX), (minY+sSizeY), vrtMap::TypeCh_Height );
}
