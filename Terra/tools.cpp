#include "stdafxTr.h"

#include "tools.h"

/* ----------------------------- EXTERN SECTION ---------------------------- */
/* --------------------------- PROTOTYPE SECTION --------------------------- */
/* --------------------------- DEFINITION SECTION -------------------------- */
//BitMap placeBMP;
//BitMap mosaicBMP(1);

sVBitMapMosaic VBitMapMosaic;


extern int* xRad[MAX_RADIUS_CIRCLEARR + 1];
extern int* yRad[MAX_RADIUS_CIRCLEARR + 1];
extern int maxRad[MAX_RADIUS_CIRCLEARR + 1];



//////////////////////////////////////////////////////////////////////////////

#define MIN(a,b)	(((a) < (b))?(a):(b))
#define MAX(a,b)	(((a) > (b))?(a):(b))

sTerrainMetod TerrainMetod;
void sTerrainMetod::put(int xx, int yy, int v)
{
	int vv;
	//v=inverse ? -v : v;
	v+=level;
	if(noiseLevel && noiseAmp){
		if((int)XRnd(100) < noiseLevel) v += noiseAmp - XRnd((noiseAmp << 1) + 1);
	}
	switch(mode){
	case PM_Absolutely:
		vv = v;
		break;
	case PM_AbsolutelyMAX:
		vv = MAX(v,vMap.GetAlt(xx,yy));//*pv);
		if(vv != v) return;
		break;
	case PM_AbsolutelyMIN:
		vv = MIN(v,vMap.GetAlt(xx,yy));//*pv);
		if(vv != v) return;
		break;
	//case 3:
	//	vv = (v + vMap.GetAlt(xx,yy))>>1;//*pv >> 1;
	//	break;
	case PM_Relatively:
		vv = v + vMap.GetAlt(xx,yy);//*pv;
		break;
	}
	vMap.voxSet(xx,yy,vv - vMap.GetAlt(xx,yy));//*pv);
}

/////////////////////////////////////////////////////////////////////////////////////////
