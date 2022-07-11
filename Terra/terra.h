#ifndef __TERRA_H__
#define __TERRA_H__

#include "XMath/Colors.h"

//Global Define
const int MAX_SURFACE_TYPE = 256;
//const int MAX_GEO_SURFACE_TYPE = MAX_SURFACE_TYPE;
const int MAX_DAM_SURFACE_TYPE = MAX_SURFACE_TYPE;
//const int SIZE_GEO_PALETTE=MAX_GEO_SURFACE_TYPE*3; // 3-это RGB
const int SIZE_DAM_PALETTE=MAX_DAM_SURFACE_TYPE*4; // 3-это RGB

//const int MAX_SURFACE_LIGHTING =128; //максимальна€ освещенность поверхности от 0 до 127
									 //старший бит идентифицирует Geo-0 или Dam-1

const int VX_FRACTION=5; // 5 младших бит - дробь воксел€ (было6)
const int VX_FRACTION_MASK= ((1<<VX_FRACTION)-1);
const float VOXEL_DIVIDER=1.f/(float)(1<<VX_FRACTION);
const int VOXEL_MULTIPLIER=(1<<VX_FRACTION);

const int VX_SHIFT_VX=2;
const int VX_FRACTION_FULL=VX_FRACTION+VX_SHIFT_VX;

const int MAX_VX_HEIGHT=((1<<(VX_FRACTION + 9))-1);//это 0x3fff (512)
const int MIN_VX_HEIGHT=0;
const int MAX_VX_HEIGHT_WHOLE=(MAX_VX_HEIGHT>>VX_FRACTION);


const int MAX_RADIUS_CIRCLEARR = 210;//175;

//------јтрибуты------//
#define VmAt_MASK (0x3)

#define VmAt_Inds	(0x1)

#define VmAt_Nrml (0x0)

#define Vm_IsIndestructability(D) (((D)&VmAt_Inds)!=0)

#define Vm_extractHeigh(D) ((D)>>VX_SHIFT_VX)
#define Vm_extractHeighWhole(D) ((D)>>(VX_SHIFT_VX+VX_FRACTION))
#define Vm_extractAtr(D) ((D)&VmAt_MASK)
#define Vm_prepHeigh4Buf(V) ((V)<<VX_SHIFT_VX )
#define Vm_prepHeighAtr(V, A) (((V)<<VX_SHIFT_VX) | (A))

//сбрасываетс€ неразрушаемость и выравненность
#define Vm_setNormalAtr(D) ((D)& 0)


//—етки
#define kmGrid (2) // Ёто 2^2 - сетка 4x4
#define sizeCellGrid (1<<kmGrid)

#define TERRAIN_TYPES_NUMBER 16
#define GRIDAT_MASK_SURFACE_KIND (TERRAIN_TYPES_NUMBER - 1)
#define GRIDAT_IMPASSABILITY (0x0) //(0x4)
#define GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND (0xF)

#define GRIDAT_LEVELED (0x10)
#define GRIDAT_INDESTRUCTABILITY (0x20)

#define GRIDAT_BUILDING (0x40)
#define GRIDAT_BASE_OF_BUILDING_CORRUPT (0x80)

#define GRIDTST_BUILDING(V) (V&GRIDAT_BUILDING)
#define GRIDTST_LEVELED(V) (V&GRIDAT_LEVELED)

// —етка отражени€ изменений на воксельной поверхности
#define kmGridChA (6) // 2^6 -сетка 64x64
#define sizeCellGridCA (1<<kmGridChA)

//#define ConvertTry2HighColor(c) ( ((((c)&0xff0000)+0x040000>>16+3)<<11) | ((((c)&0x00ff00)+ 0x0200>>8+2)<<5) | ((((c)&0xff)+0x04>>0+3)<<0) )
#define ConvertTry2HighColor(c) ( ((((c)&0xff0000)>>16+3)<<11) | ((((c)&0x00ff00)>>8+2)<<5) | ((((c)&0xff)>>0+3)<<0) )


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

inline int returnPower2(int val)
{
	const int MAX_POWER2=14;//16384
	int v=1;
	for(int i=1; i<=MAX_POWER2; i++){
		v*=2;
		if(val<=v) return i;
	}
	return MAX_POWER2;
}

bool putModel2VBitmap(const char* fName, const Se3f& pos, struct sVoxelBitmap& voxelBitmap, bool flag_placePlane, float scaleFactor, float zScale=1.f);

class ColorModificator {
public:
	ColorModificator(Color4c _modcolor, float _kColor, float _kSaturation, float _kBrightness) :
		modcolor(_modcolor) {
		kColor=_kColor;
		kSaturation=_kSaturation;
		kBrightness=_kBrightness;
	}
	Color4c get(Color4c inColor) const {
		float h,s,v;
		inColor.HSV(h, s, v);
		s = min(s*kSaturation, 1.f);
		v = min(v*kBrightness, 1.f);
		Color4f out;
		out.setHSV(h, s, v, 0); 
		out += modcolor*kColor*v;
		return Color4c().setSafe(out);
	}
private:
	Color4f modcolor;
	float kColor;
	float kSaturation;
	float kBrightness;
};

#endif // __TERRA_H__
