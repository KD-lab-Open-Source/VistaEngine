#ifndef __TERRA_H__
#define __TERRA_H__

//Global Define

const int MAX_H_SIZE_POWER=14;
const int MAX_H_SIZE=1<<MAX_H_SIZE_POWER;

const int MAX_V_SIZE_POWER=14;
const int MAX_V_SIZE=1<<MAX_V_SIZE_POWER;

const int MAX_SURFACE_TYPE = 256;
const int MAX_GEO_SURFACE_TYPE = MAX_SURFACE_TYPE;
const int MAX_DAM_SURFACE_TYPE = MAX_SURFACE_TYPE;
const int SIZE_GEO_PALETTE=MAX_GEO_SURFACE_TYPE*3; // 3-это RGB
const int SIZE_DAM_PALETTE=MAX_DAM_SURFACE_TYPE*4; // 3-это RGB

const int MAX_SURFACE_LIGHTING =128; //максимальная освещенность поверхности от 0 до 127
									 //старший бит идентифицирует Geo-0 или Dam-1

const int VX_FRACTION=5; // 6 младших бит - дробь вокселя
const int VX_FRACTION_MASK= ((1<<VX_FRACTION)-1);
const float VOXEL_DIVIDER=1.f/(float)(1<<VX_FRACTION);
const int VOXEL_MULTIPLIER=(1<<VX_FRACTION);

const int VX_SHIFT_VX=3;
const int VX_FRACTION_FULL=VX_FRACTION+VX_SHIFT_VX;

const int MAX_VX_HEIGHT=((1<<(VX_FRACTION + 8))-1);//это 0x3fff//0xFFFF;
const int MIN_VX_HEIGHT=0;
const int MAX_VX_HEIGHT_WHOLE=(MAX_VX_HEIGHT>>VX_FRACTION);

const int MAX_SIMPLE_DAM_SURFACE=10;

const int MAX_RADIUS_CIRCLEARR = 210;//175;

//------Атрибуты------//
#define VmAt_MASK (0x7)

// geo-0 dam-1 //не нужно конверсии
#define VmAt_GeoDam	(0x4) 
#define VmAt_Inds	(0x2)
#define VmAt_Lvld	(0x1)

#define VmAt_Nrml_Geo (0x0)
#define VmAt_Nrml_Dam (VmAt_GeoDam)
#define VmAt_Nrml_Dam_Inds (VmAt_GeoDam|VmAt_Inds)

#define Vm_IsGeo(D) (((D)&VmAt_GeoDam)==0)
#define Vm_IsDam(D) (((D)&VmAt_GeoDam)!=0)

#define Vm_IsLeveled(D) (((D)&VmAt_Lvld)!=0)

#define Vm_IsIndestructability(D) (((D)&VmAt_Inds)!=0)

#define Vm_extractHeigh(D) ((D)>>VX_SHIFT_VX)
#define Vm_extractHeighWhole(D) ((D)>>(VX_SHIFT_VX+VX_FRACTION))
#define Vm_extractAtr(D) ((D)&VmAt_MASK)
#define Vm_prepHeigh4Buf(V) ((V)<<VX_SHIFT_VX )
#define Vm_prepHeighAtr(V, A) (((V)<<VX_SHIFT_VX) | (A))

//сбрасывается неразрушаемость и выравненность
#define Vm_setNormalAtr(D) ((D)&VmAt_GeoDam)

//WorldGen param
const int WORLD_TYPE	       =  3;
const int BIZARRE_ROUGHNESS_MAP = 0;
const int BIZARRE_ALT_MAP	= 0;
const int NOISELEVEL	       = 0xFFff;//256;

extern int* xRad[MAX_RADIUS_CIRCLEARR + 1];
extern int* yRad[MAX_RADIUS_CIRCLEARR + 1];
extern int maxRad[MAX_RADIUS_CIRCLEARR + 1];

struct sRect {
	short x, y;
	short sx, sy;
	sRect(){ x = y = sx = sy = 0; }
	sRect(short _x, short _y, short _sx, short _sy){ x = _x; y = _y; sx = _sx; sy = _sy; }
	void addBound(const Vect2i& v) { 
		if(sx){
			if(v.x < x){
				sx+=x-v.x;
				x=v.x;
			}
			else if(v.x-x > sx)
				sx=v.x-x;
			if(v.y < y){
				sy+=y-v.y;
				y=v.y;
			}
			else if(v.y-y > sy)
				sy=v.y-y;
		}
		else{
			x = v.x; y = v.y;
			sx = 1; sy = 1;
		}
	}
	bool isEmpty(){ return sx==0; }
	short xr() const { return x + sx; }
	short yb() const { return y + sy; }
};

#include "vmap.h"

#endif // __TERRA_H__
