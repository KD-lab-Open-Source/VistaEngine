#include "stdafx.h"
//#include <stdlib.h>
//#include "terra.h"
#include "shape3D.h"
#include "xmath.h"
#include "..\terra\terra.h"
#include "..\terra\tools.h"

#include "..\Render\inc\umath.h"
#include "..\Render\inc\IRenderDevice.h"

#pragma warning( disable : 4244 ) //warning при конверте float 2 int

meshS3D wrkMesh;

void meshS3D::releaseBufs(void)
{
	if(vrtx!=0){
		delete [] vrtx;
		vrtx=0;
	}
	numVrtx=0;
	if(face!=0){
		delete [] face;
		face=0;
	}
	numFace=0;

}

bool meshS3D::load(const char* fname)//, int numMesh)
{
	releaseBufs();

	vector<sPolygon> poligonArr;
	vector<Vect3f> pointArr;
	GetAllTriangle3dx(fname, pointArr, poligonArr);

	fModelName=fname;

	numVrtx=pointArr.size();
	vrtx=new vrtxS3D[numVrtx];
	numFace=poligonArr.size();
	face=new faceS3D[numFace];

	int indx;
	for(indx=0; indx< numFace; indx++){
		face[indx].v1=poligonArr[indx].p1;
		face[indx].v2=poligonArr[indx].p2;
		face[indx].v3=poligonArr[indx].p3;
	}
	for(indx=0; indx< numVrtx; indx++){
		vrtx[indx].x=pointArr[indx].x;
		vrtx[indx].y=pointArr[indx].y;//-
		vrtx[indx].z=pointArr[indx].z;
	}
	///calcNormals();

	return 1;
}

void meshS3D::rotateAndScaling(int XA, int YA, int ZA, float kX, float kY, float kZ)
{
	Mat3f A_shape;
	//A_shape.set(Vect3f(0.5,-0.5,-0.5), Vect3f::ZERO);
	A_shape.set(Vect3f(kX, kY, -kZ), Vect3f::ZERO);

	A_shape = Mat3f((float)XA*(M_PI/180.0f),X_AXIS)*A_shape;
	A_shape = Mat3f((float)-YA*(M_PI/180.0f),Y_AXIS)*A_shape;
	A_shape = Mat3f((float)ZA*(M_PI/180.0f),Z_AXIS)*A_shape;

	matrix.xx=A_shape[0][0];
	matrix.xy=A_shape[0][1];
	matrix.xz=A_shape[0][2];

	matrix.yx=A_shape[1][0];
	matrix.yy=A_shape[1][1];
	matrix.yz=A_shape[1][2];

	matrix.zx=A_shape[2][0];
	matrix.zy=A_shape[2][1];
	matrix.zz=A_shape[2][2];
}
void meshS3D::moveModel2(int x, int y)
{
	position.x=(float)vMap.XCYCL(x); 
	position.y=(float)vMap.YCYCL(y); 
}
void meshS3D::shiftModelOn(int dx, int dy)
{
	position.x=(float)vMap.XCYCL(round(position.x)+dx); 
	position.y=(float)vMap.YCYCL(round(position.y)+dy);
}

void meshS3D::draw2Scr(int ViewX, int ViewY, float k_scale, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY)
{
	sizeX05=GB_MAXX>>1;
	sizeY05=GB_MAXY>>1;
	kScale=k_scale;
	GB=GRAF_BUF;
	calcPositionAndLightOrto(ViewX, ViewY);
	drawGouraund();
}

void meshS3D::calcPositionAndLightOrto(int xVision, int yVision)
{
	int i;
	for (i = 0; i < numVrtx; i++) {
		float mx, my, mz;
		float mnx, mny, mnz;
		mx=matrix.xx*vrtx[i].x + matrix.xy*vrtx[i].y + matrix.xz*vrtx[i].z;
		mnx=matrix.xx*vrtx[i].nx + matrix.xy*vrtx[i].ny + matrix.xz*vrtx[i].nz;

		my=matrix.yx*vrtx[i].x + matrix.yy*vrtx[i].y + matrix.yz*vrtx[i].z;
		mny=matrix.yx*vrtx[i].nx + matrix.yy*vrtx[i].ny + matrix.yz*vrtx[i].nz;

		mz=matrix.zx*vrtx[i].x + matrix.zy*vrtx[i].y + matrix.zz*vrtx[i].z;
		mnz=matrix.zx*vrtx[i].nx + matrix.zy*vrtx[i].ny + matrix.zz*vrtx[i].nz;

		mx+=position.x; //mnx+=position.x;
		my+=position.y; //mny+=position.y;
		mz+=position.z; //mnz+=position.z;
		//в дальнейшем возможен более правильный расчет освещенности
		//vrtx[i].light=255* ( (mnz<0) ? (-mnz): 0);
		vrtx[i].light=500* (mnz*SLight.z + mnx*SLight.x + mny*SLight.y);
		if(vrtx[i].light>255) vrtx[i].light=255;
		if(vrtx[i].light<0) vrtx[i].light=0;

		//орто проекция
		mx-=xVision; my-=yVision;
		vrtx[i].sx=sizeX05 + mx*kScale;
		vrtx[i].sy=sizeY05 + my*kScale;
		vrtx[i].z1=1.0f/(mz+2048);

	}
}

void meshS3D::drawGouraund(void)
{
	int sizeX=sizeX05*2;
	float * zBuffer;
	zBuffer=new float[sizeX05*2*sizeY05*2];
    memset(zBuffer, 0, sizeX05*2*sizeY05*2 * sizeof(float));

	int i;
	for (i = 0; i < numFace; i++) {
		vrtxS3D *a, *b, *c, *tmpv;
		a=&vrtx[face[i].v1];
		b=&vrtx[face[i].v2];
		c=&vrtx[face[i].v3];

		// отсортируем вершины грани по sy
		if (a->sy > b->sy) { tmpv = a; a = b; b = tmpv; }
		if (a->sy > c->sy) { tmpv = a; a = c; c = tmpv; }
		if (b->sy > c->sy) { tmpv = b; b = c; c = tmpv; }

		// грань нулевой высоты рисовать не будем
		if (round(c->sy) <= round(a->sy)) continue;

		int current_sx, current_sy;
		float tmp, k, x_start, x_end, c_start, c_end, dc_start, dc_end;
		float dx_start, dx_end, dz1_start, dz1_end;
		float z1_start, z1_end;
		float x, cc, z1, dc, dz1;
		int length;
		unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		k = (b->sy - a->sy) / (c->sy - a->sy);
		x_start = a->sx + (c->sx - a->sx) * k;
		c_start = a->light + (c->light - a->light) * k;
		z1_start = a->z1 + (c->z1 - a->z1) * k;
		x_end = b->sx;
		c_end = b->light;
		z1_end = b->z1;
		dc = (c_start - c_end) / (x_start - x_end);
		dz1 = (z1_start - z1_end) / (x_start - x_end);
		if(abs(round(x_start - x_end))<1) continue;//отсечение по нулевой ширине

		x_start = a->sx;
		c_start = a->light;
		z1_start = a->z1;
		dx_start = (c->sx - a->sx) / (c->sy - a->sy);
		dc_start = (c->light - a->light) / (c->sy - a->sy);
		dz1_start = (c->z1 - a->z1) / (c->sy - a->sy);
//#ifdef SUBPIXEL
		tmp = ceilf(a->sy) - a->sy;
		x_start += dx_start * tmp;
		c_start += dc_start * tmp;
		z1_start += dz1_start * tmp;
//#endif

		if (ceilf(b->sy) > ceilf(a->sy)) {
			tmp = ceilf(a->sy) - a->sy;
			x_end = a->sx;
			c_end = a->light;
			z1_end = a->z1;
			dx_end = (b->sx - a->sx) / (b->sy - a->sy);
			dc_end = (b->light - a->light) / (b->sy - a->sy);
			dz1_end = (b->z1 - a->z1) / (b->sy - a->sy);
		} else {
			tmp = ceilf(b->sy) - b->sy;
			x_end = b->sx;
			c_end = b->light;
			z1_end = b->z1;
			dx_end = (c->sx - b->sx) / (c->sy - b->sy);
			dc_end = (c->light - b->light) / (c->sy - b->sy);
			dz1_end = (c->z1 - b->z1) / (c->sy - b->sy);
		}
//#ifdef SUBPIXEL
		x_end += dx_end * tmp;
		c_end += dc_end * tmp;
		z1_end += dz1_end * tmp;
//#endif

////////////////////////////////

/*		// построчная отрисовка грани
		for (current_sy = ceilf(a->sy); current_sy < ceilf(c->sy); current_sy++) {
			if (current_sy == ceilf(b->sy)) {
				x_end = b->sx;
				c_end = b->light;
				z1_end = b->z1;
				dx_end = (c->sx - b->sx) / (c->sy - b->sy);
				dc_end = (c->light - b->light) / (c->sy - b->sy);
				dz1_end = (c->z1 - b->z1) / (c->sy - b->sy);
	//#ifdef SUBPIXEL
			tmp = ceilf(b->sy) - b->sy;
			x_end += dx_end * tmp;
			c_end += dc_end * tmp;
			z1_end += dz1_end * tmp;
	//#endif
			}

			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  cc = c_end;
			  z1 = z1_end;
			  length = ceilf(x_start) - ceilf(x_end);
			} else {
			  x = x_start;
			  cc = c_start;
			  z1 = z1_start;
			  length = ceilf(x_end) - ceilf(x_start);
			}

			// считаем адрес начала строки в видеопамяти
			dest = GB;
			dest += current_sy * sizeX05*2 + (int)ceilf(x);

			// текстурируем строку
			current_sx = (int)ceilf(x);

			if (length) {
		//#ifdef SUBTEXEL
		//      tmp = ceilf(x) - x;
		//      cc += dc * tmp;
		//#endif
				while (length--) {
				// используем z-буфер для определения видимости текущей точки
					if (zBuffer[current_sy*sizeX + current_sx] <= z1) {
						*dest = palLight[round(cc)];
						zBuffer[current_sy*sizeX + current_sx] = z1;
					}
					cc += dc;
					z1 += dz1;
					dest++;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			c_start += dc_start;
			z1_start += dz1_start;
			x_end += dx_end;
			c_end += dc_end;
			z1_end += dz1_end;
		}
*/
		// построчная отрисовка грани
		for (current_sy = ceilf(a->sy); current_sy < ceilf(b->sy); current_sy++) {

			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  cc = c_end;
			  z1 = z1_end;
			  length = ceilf(x_start) - ceilf(x_end);
			} else {
			  x = x_start;
			  cc = c_start;
			  z1 = z1_start;
			  length = ceilf(x_end) - ceilf(x_start);
			}

			// считаем адрес начала строки в видеопамяти
			if(current_sy >= sizeY05*2 ) break;
			if(current_sy < 0 ) break;
			dest = GB;
			dest += current_sy * sizeX05*2 + (int)ceilf(x);

			// текстурируем строку
			current_sx = (int)ceilf(x);
			if(current_sx < 0 ) break;

			if (length) {
		//#ifdef SUBTEXEL
		//      tmp = ceilf(x) - x;
		//      cc += dc * tmp;
		//#endif
				while ((length--)>0) {
					if(current_sx >= sizeX05*2 ) break;
				// используем z-буфер для определения видимости текущей точки
					if (zBuffer[current_sy*sizeX + current_sx] <= z1) {
						*dest = palLight[round(cc)];
						zBuffer[current_sy*sizeX + current_sx] = z1;
					}
					cc += dc;
					z1 += dz1;
					dest++;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			c_start += dc_start;
			z1_start += dz1_start;
			x_end += dx_end;
			c_end += dc_end;
			z1_end += dz1_end;
		}
////////////

		x_end = b->sx;
		c_end = b->light;
		z1_end = b->z1;
		dx_end = (c->sx - b->sx) / (c->sy - b->sy);
		dc_end = (c->light - b->light) / (c->sy - b->sy);
		dz1_end = (c->z1 - b->z1) / (c->sy - b->sy);
		// x_start должен находиться левее x_end
		if (x_start > x_end) {
		  x = x_end;
		  cc = c_end;
		  z1 = z1_end;
		  length = ceilf(x_start) - ceilf(x_end);
		} else {
		  x = x_start;
		  cc = c_start;
		  z1 = z1_start;
		  length = ceilf(x_end) - ceilf(x_start);
		}
		// построчная отрисовка грани
		for (current_sy; current_sy < ceilf(c->sy); current_sy++) {
			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  cc = c_end;
			  z1 = z1_end;
			  length = ceilf(x_start) - ceilf(x_end);
			} else {
			  x = x_start;
			  cc = c_start;
			  z1 = z1_start;
			  length = ceilf(x_end) - ceilf(x_start);
			}

			// считаем адрес начала строки в видеопамяти
			if(current_sy >= sizeY05*2 ) break;
			if(current_sy < 0 ) break;
			dest = GB;
			dest += current_sy * sizeX05*2 + (int)ceilf(x);

			// текстурируем строку
			current_sx = (int)ceilf(x);
			if(current_sx < 0 ) break;

			if (length) {
		//#ifdef SUBTEXEL
		//      tmp = ceilf(x) - x;
		//      cc += dc * tmp;
		//#endif
				while ((length--)>0) {
					if(current_sx >= sizeX05*2 ) break;
					// используем z-буфер для определения видимости текущей точки
					if (zBuffer[current_sy*sizeX + current_sx] <= z1) {
						*dest = palLight[round(cc)];
						zBuffer[current_sy*sizeX + current_sx] = z1;
					}
					cc += dc;
					z1 += dz1;
					dest++;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			c_start += dc_start;
			z1_start += dz1_start;
			x_end += dx_end;
			c_end += dc_end;
			z1_end += dz1_end;
		}

////////////

	}
	delete [] zBuffer;
}

void meshS3D::put2Earch(int quality)
{
	const int SCALING4PUT2EARCH=quality;
	int i;
	int minX=vMap.H_SIZE;
	int minY=vMap.V_SIZE;
	int maxX=0;
	int maxY=0;
	int minZ=0x7fFFffFF;// максимальное положительное число
	int maxZ=0x80000001;// самое маленькое отрицательное число
	float fMinX,fMaxX,fMinY,fMaxY,fMinZ,fMaxZ;
	for (i = 0; i < numVrtx; i++) {
		float mx, my, mz;
		float mnx, mny, mnz;
		mx=matrix.xx*vrtx[i].x + matrix.xy*vrtx[i].y + matrix.xz*vrtx[i].z;
		mnx=matrix.xx*vrtx[i].nx + matrix.xy*vrtx[i].ny + matrix.xz*vrtx[i].nz;

		my=matrix.yx*vrtx[i].x + matrix.yy*vrtx[i].y + matrix.yz*vrtx[i].z;
		mny=matrix.yx*vrtx[i].nx + matrix.yy*vrtx[i].ny + matrix.yz*vrtx[i].nz;

		mz=matrix.zx*vrtx[i].x + matrix.zy*vrtx[i].y + matrix.zz*vrtx[i].z;
		mnz=matrix.zx*vrtx[i].nx + matrix.zy*vrtx[i].ny + matrix.zz*vrtx[i].nz;

		//mx+=position.x;
		//my+=position.y;
		//mz+=position.z;
		mx*=SCALING4PUT2EARCH; mnx*=SCALING4PUT2EARCH;
		my*=SCALING4PUT2EARCH; mny*=SCALING4PUT2EARCH;
		//mz*=SCALING4PUT2EARCH; mnz*=SCALING4PUT2EARCH;

		//орто проекция
		vrtx[i].sx=mx;
		vrtx[i].sy=my;
		vrtx[i].z1=mz;
		vrtx[i].light=mz;
		if(i==0){
			fMinX=mx; fMaxX=mx; fMinY=my; fMaxY=my; fMinZ=mz; fMaxZ=mz;
		}
		else {
			if(mx < fMinX) fMinX=mx; if(mx > fMaxX) fMaxX=mx;
			if(my < fMinY) fMinY=my; if(my > fMaxY) fMaxY=my;
			if(mz < fMinZ) fMinZ=mz; if(mz > fMaxZ) fMaxZ=mz;
		}
	}
	for (i = 0; i < numVrtx; i++){
		vrtx[i].z1=(fMaxZ-fMinZ)-(vrtx[i].z1-fMinZ);
	}
	minX=round(fMinX)-1; maxX=round(fMaxX)+1;
	minY=round(fMinY)-1; maxY=round(fMaxY)+1;
	minZ=round(fMinZ)-1; maxZ=round(fMaxZ)+1;
	if((position.x*SCALING4PUT2EARCH + minX) < 0) minX=0-position.x*SCALING4PUT2EARCH;
	if((position.x*SCALING4PUT2EARCH + maxX) > vMap.H_SIZE*SCALING4PUT2EARCH) maxX=vMap.H_SIZE*SCALING4PUT2EARCH-position.x*SCALING4PUT2EARCH;
	if((position.y*SCALING4PUT2EARCH + minY) < 0) minY=0-position.y*SCALING4PUT2EARCH;
	if((position.y*SCALING4PUT2EARCH + maxY) > vMap.V_SIZE*SCALING4PUT2EARCH) maxY=vMap.V_SIZE*SCALING4PUT2EARCH-position.y*SCALING4PUT2EARCH;
	if( ((maxX-minX)<=0) || ((maxY-minY)<=0) ) return;
	//if(minX<0) minX=0; if(maxX>=vMap.H_SIZE*SCALING4PUT2EARCH)maxX=vMap.H_SIZE*SCALING4PUT2EARCH;
	//if(minY<0) minY=0; if(maxY>=vMap.V_SIZE*SCALING4PUT2EARCH)maxY=vMap.V_SIZE*SCALING4PUT2EARCH;
////////////////////////////////////////////////
	int sizeX=maxX-minX;
	int sizeY=maxY-minY;
	float * zBuffer;
	int ss=sizeX*sizeY*sizeof(float);
	const int MAX_MEMORY_SIZE=128*1024*1024;//128 M
	if(ss>MAX_MEMORY_SIZE){
		char str[128];
		sprintf(str, "Эта операция потребует %12d Mбайт памяти. Выполнять ?", ss/(1024*1024));
		int result=AfxMessageBox(str, MB_OKCANCEL);
		if(result==IDCANCEL) return;
	}
	zBuffer=new float[sizeX*sizeY];
    memset(zBuffer, 0, sizeX*sizeY * sizeof(float));


	for (i = 0; i < numFace; i++) {
		vrtxS3D *a, *b, *c, *tmpv;
		a=&vrtx[face[i].v1];
		b=&vrtx[face[i].v2];
		c=&vrtx[face[i].v3];

		// отсортируем вершины грани по sy
		if (a->sy > b->sy) { tmpv = a; a = b; b = tmpv; }
		if (a->sy > c->sy) { tmpv = a; a = c; c = tmpv; }
		if (b->sy > c->sy) { tmpv = b; b = c; c = tmpv; }

		// грань нулевой высоты рисовать не будем( а надо, для того-чтоб не пропадали точки когда очень много полигонов на точку)
		///if (round(c->sy) <= round(a->sy)) continue;

		int current_sx, current_sy;
		float tmp, k, x_start, x_end, c_start, c_end, dc_start, dc_end;
		float dx_start, dx_end, dz1_start, dz1_end;
		float z1_start, z1_end;
		float x, cc, z1, dc, dz1;
		int length;
		//unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		k = (b->sy - a->sy) / (c->sy - a->sy);
		x_start = a->sx + (c->sx - a->sx) * k;
		c_start = a->light + (c->light - a->light) * k;
		z1_start = a->z1 + (c->z1 - a->z1) * k;
		x_end = b->sx;
		c_end = b->light;
		z1_end = b->z1;
		dc = (c_start - c_end) / (x_start - x_end);
		dz1 = (z1_start - z1_end) / (x_start - x_end);

		x_start = a->sx;
		c_start = a->light;
		z1_start = a->z1;
		dx_start = (c->sx - a->sx) / (c->sy - a->sy);
		dc_start = (c->light - a->light) / (c->sy - a->sy);
		dz1_start = (c->z1 - a->z1) / (c->sy - a->sy);
//#ifdef SUBPIXEL
		tmp = ceilf(a->sy) - a->sy;
		x_start += dx_start * tmp;
		c_start += dc_start * tmp;
		z1_start += dz1_start * tmp;
//#endif

		if (ceilf(b->sy) > ceilf(a->sy)) {
			tmp = ceilf(a->sy) - a->sy;
			x_end = a->sx;
			c_end = a->light;
			z1_end = a->z1;
			dx_end = (b->sx - a->sx) / (b->sy - a->sy);
			dc_end = (b->light - a->light) / (b->sy - a->sy);
			dz1_end = (b->z1 - a->z1) / (b->sy - a->sy);
		} else {
			tmp = ceilf(b->sy) - b->sy;
			x_end = b->sx;
			c_end = b->light;
			z1_end = b->z1;
			dx_end = (c->sx - b->sx) / (c->sy - b->sy);
			dc_end = (c->light - b->light) / (c->sy - b->sy);
			dz1_end = (c->z1 - b->z1) / (c->sy - b->sy);
		}
//#ifdef SUBPIXEL
		x_end += dx_end * tmp;
		c_end += dc_end * tmp;
		z1_end += dz1_end * tmp;
//#endif

////////////////////////////////

		// построчная отрисовка грани
		for (current_sy = ceilf(a->sy); current_sy < ceilf(c->sy); current_sy++) {
			if((current_sy-minY) >= sizeY ) break;
			//if((current_sy-minY) < 0 ) break;//continue;
			if (current_sy == ceilf(b->sy)) {
				x_end = b->sx;
				c_end = b->light;
				z1_end = b->z1;
				dx_end = (c->sx - b->sx) / (c->sy - b->sy);
				dc_end = (c->light - b->light) / (c->sy - b->sy);
				dz1_end = (c->z1 - b->z1) / (c->sy - b->sy);
//#ifdef SUBPIXEL
				tmp = ceilf(b->sy) - b->sy;
				x_end += dx_end * tmp;
				c_end += dc_end * tmp;
				z1_end += dz1_end * tmp;
//#endif
			}

			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  cc = c_end;
			  z1 = z1_end;
			  length = ceilf(x_start) - ceilf(x_end);
			} else {
			  x = x_start;
			  cc = c_start;
			  z1 = z1_start;
			  length = ceilf(x_end) - ceilf(x_start);
			}

			// считаем адрес начала строки в видеопамяти
			//dest = GB;
			//dest += current_sy * sizeX05*2 + (int)ceilf(x);

			// текстурируем строку
			current_sx = (int)ceilf(x)-minX;
	
			if((current_sy-minY) >= 0 ) if (length) {
		//#ifdef SUBTEXEL
		      tmp = ceilf(x) - x;
		      cc += dc * tmp;
			  z1 += dz1* tmp;
		//#endif
				while (length--) {
				// используем z-буфер для определения видимости текущей точки
					if( (current_sx<sizeX) && (current_sx >= 0)) {
						if (zBuffer[(current_sy-minY)*sizeX + current_sx] <= z1) {
							//*dest = palLight[round(cc)];
							zBuffer[(current_sy-minY)*sizeX + current_sx] = z1;
						}
					}
					cc += dc;
					z1 += dz1;
					//dest++;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			c_start += dc_start;
			z1_start += dz1_start;
			x_end += dx_end;
			c_end += dc_end;
			z1_end += dz1_end;
		}

////////////
	}
	int sSizeY=sizeY/SCALING4PUT2EARCH;
	int sSizeX=sizeX/SCALING4PUT2EARCH;
	if(vMap.flag_record_operation) 
		vMap.UndoDispatcher_PutPreChangedArea( sRect(position.x+minX/SCALING4PUT2EARCH, position.y+minY/SCALING4PUT2EARCH, sSizeX, sSizeY), 1,0 ); //position.x+minX/SCALING4PUT2EARCH+ //position.y+minY/SCALING4PUT2EARCH+
	int j,k,m;//,i;
	for(i=0; i<sSizeY; i++){
		for(j=0; j<sSizeX; j++){
			float h=0;
			for(k=0; k<SCALING4PUT2EARCH; k++){
				for(m=0; m<SCALING4PUT2EARCH; m++){
					int off=(i*SCALING4PUT2EARCH+k)*sizeX+(j*SCALING4PUT2EARCH+m);
					if(zBuffer[off]!=0){
						h+=zBuffer[off];
					}
				}
			}
			if(h!=0)TerrainMetod.put( position.x + minX/SCALING4PUT2EARCH+j, position.y + minY/SCALING4PUT2EARCH+i, round(h/((float)SCALING4PUT2EARCH*SCALING4PUT2EARCH)*(1<<VX_FRACTION)));
		}
	}
	vMap.regRender( position.x+minX/SCALING4PUT2EARCH, position.y+minY/SCALING4PUT2EARCH, position.x+(minX+sizeX)/SCALING4PUT2EARCH, position.y+(minY+sizeY)/SCALING4PUT2EARCH, vrtMap::TypeCh_Height );

	delete [] zBuffer;

}

//////////////////////////////////////////////////////////////////////////////////////
void meshS3D::calcNormals(void)
{
	float ax, ay, az, bx, by, bz, nx, ny, nz, l;
	int i;

	for (i = 0; i < numVrtx; i++) {
		vrtx[i].nx = 0;
		vrtx[i].ny = 0;
		vrtx[i].nz = 0;
	}
	for (i = 0; i < numFace; i++) {
		ax = vrtx[face[i].v3].x - vrtx[face[i].v1].x;
		ay = vrtx[face[i].v3].y - vrtx[face[i].v1].y;
		az = vrtx[face[i].v3].z - vrtx[face[i].v1].z;
		bx = vrtx[face[i].v2].x - vrtx[face[i].v1].x;
		by = vrtx[face[i].v2].y - vrtx[face[i].v1].y;
		bz = vrtx[face[i].v2].z - vrtx[face[i].v1].z;
		nx = ay * bz - az * by;
		ny = az * bx - ax * bz;
		nz = ax * by - ay * bx;
		l = sqrtf(nx * nx + ny * ny + nz * nz);
		nx /= l;
		ny /= l;
		nz /= l;

		vrtx[face[i].v1].nx += nx;
		vrtx[face[i].v1].ny += ny;
		vrtx[face[i].v1].nz += nz;
		vrtx[face[i].v2].nx += nx;
		vrtx[face[i].v2].ny += ny;
		vrtx[face[i].v2].nz += nz;
		vrtx[face[i].v3].nx += nx;
		vrtx[face[i].v3].ny += ny;
		vrtx[face[i].v3].nz += nz;

	}
	for (i = 0; i < numVrtx; i++) {
		l = 1.0f/sqrtf(vrtx[i].nx * vrtx[i].nx + vrtx[i].ny * vrtx[i].ny +
				 vrtx[i].nz * vrtx[i].nz);
		vrtx[i].nx *= l;
		vrtx[i].ny *= l;
		vrtx[i].nz *= l;
	}

}
