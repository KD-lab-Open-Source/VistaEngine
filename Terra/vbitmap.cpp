#include "stdafxTr.h"

#include "vbitmap.h"
//sVBitMap VBitMap;

void sVBitMap::put(int VBMmode, int VBMlevel, int VBMnoiseLevel, int VBMnoiseAmp, bool VBMinverse)
{
	int x,y;
	int sizeMX=round((float)sx/kX);
	int sizeMY=round((float)sy/kY);
	int sizeMXY;
	if(sizeMX>sizeMY)sizeMXY=sizeMX;
	else sizeMXY=sizeMY;
	int addx=mX-(sizeMXY>>1);
	int addy=mY-(sizeMXY>>1);

	vMap.UndoDispatcher_PutPreChangedArea(sRect(addx, addy, sizeMXY, sizeMXY),1,0);

	for(y=0; y<sizeMXY; y++){
		int yy=vMap.YCYCL(addy+y);
		for(x=0; x<sizeMXY; x++){
			int xx=vMap.XCYCL(addx+x);
			int c=getPreciseColor(xx,yy);//getColor(xx,yy);//
			//if(c!=-1) *b=c;
			if(c>0) {
				int vv,v=round((float)c/kZ);
				v=VBMinverse ? -v : v;
				v+=VBMlevel;
				if(VBMnoiseLevel && VBMnoiseAmp){
					if((int)XRnd(100) < VBMnoiseLevel) v += VBMnoiseAmp - XRnd((VBMnoiseAmp << 1) + 1);
				}
				switch(VBMmode){
				case 0:
					vv = v;
					break;
				case 1:
					vv = max(v,vMap.GetAlt(xx,yy));//*pv);
					if(vv != v) continue;
					break;
				case 2:
					vv = min(v,vMap.GetAlt(xx,yy));//*pv);
					if(vv != v) continue;
					break;
				case 3:
					vv = (v + vMap.GetAlt(xx,yy))>>1;//*pv >> 1;
					break;
				case 4:
					vv = v + vMap.GetAlt(xx,yy);//*pv;
					break;
				}
				vMap.voxSet(xx,yy,vv - vMap.GetAlt(xx,yy));//*pv);
			}
		}
	}
	vMap.regRender(addx, addy, addx + sizeMXY, addy + sizeMXY, vrtMap::TypeCh_Height);
}
