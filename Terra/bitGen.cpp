#include "stdafxTr.h"

#include "bitGen.h"

BitGenDispatcher bitGenDispatcher;

void sBitGenMetodExp::generate(sTerrainBitmapBase& tb)
{
	unsigned int storeRnd=XRndGet();
	XRndSet(rndVal); //!Важно
	float fMaxHeight=abs(maxHeight);
	if(fMaxHeight < 1e-5f) fMaxHeight=1e-5f;
	float kExpF=-1.f/logf(1.f/(34.f*(fMaxHeight*VOXEL_DIVIDER)));
	int i,j,cnt=0;
	const float XSIZE1_05=1.f/((float)tb.sx/2.f);
	const float YSIZE1_05=1.f/((float)tb.sy/2.f);
	const int SX05=tb.sx/2;
	const int SY05=tb.sy/2;
	for(i=0; i<tb.sy; i++){
		float y=(float)(i-SY05)*YSIZE1_05;
		for(j=0; j<tb.sx; j++){
			float x=(float)(j-SX05)*XSIZE1_05; 
			float f;
			if(expPower==2)
				f=exp(-(x*x+y*y)/kExpF); //(0.4f*0.4f)
			else 
				f=exp(-(fabsf(x*x*x)+fabsf(y*y*y))/kExpF); //(0.4f*0.4f)
			tb.pRaster[cnt]= round((float)maxHeight*f);
			cnt++;
		}
	}
	XRndSet(storeRnd);
}


extern float turbulence(float point[3], float lofreq, float hifreq); 
void sBitGenMetodPN::generate(sTerrainBitmapBase& tb)
{
	unsigned int storeRnd=XRndGet();
	XRndSet(rndVal); //!Важно
	float fMaxHeight=abs(maxHeight);
	if(fMaxHeight < 1e-5f) fMaxHeight=1e-5f;
	float kExpF=-1.f/logf(1.f/(34.f*(fMaxHeight*VOXEL_DIVIDER)));
	int i,j,cnt=0;
	const float XSIZE1_05=1.f/((float)tb.sx/2.f);
	const float YSIZE1_05=1.f/((float)tb.sy/2.f);
	const int SX05=tb.sx/2;
	const int SY05=tb.sy/2;
	for(i=0; i<tb.sy; i++){
		float y=(float)(i-SY05)*YSIZE1_05;
		for(j=0; j<tb.sx; j++){
			float x=(float)(j-SX05)*XSIZE1_05; 
			float f;
			if(expPower==2)
				f=exp(-(x*x+y*y)/kExpF); //(0.4f*0.4f)
			else 
				f=exp(-(fabsf(x*x*x)+fabsf(y*y*y))/kExpF); //(0.4f*0.4f)
			//tb.pRaster[cnt]= round((float)maxHeight*f);

			const float k=.3f;
			float VV[3];
			VV[0]=x*k;
			VV[1]=y*k;
			VV[2]=f*k;
			float tmp=(float) (1.f+sin(VV[0]*VV[1]*VV[2]+turbulence(VV,0.1f,1.f))); 
			tmp=tmp*0.3f+0.7f;
			tb.pRaster[cnt]= round((float)maxHeight*tmp*f);

			cnt++;
		}
	}
	XRndSet(storeRnd);
}

void sBitGenMetodMPD::generate(sTerrainBitmapBase& tb)
{
	unsigned int storeRnd=XRndGet();
	XRndSet(rndVal); //!Важно
	float fMaxHeight=abs(maxHeight);
	if(fMaxHeight < 1e-5f) fMaxHeight=1e-5f;
	float kExpF=-1.f/logf(1.f/(34.f*(fMaxHeight*VOXEL_DIVIDER)));

	int typeGeo=round(floor(kRoughness*5));
	int BEG_INIT_CELLL=1;
	switch(typeGeo){
	case 0:
		BEG_INIT_CELLL=1;
		break;
	case 1:
		BEG_INIT_CELLL=2;
		break;
	case 2:
		BEG_INIT_CELLL=4;
		break;
	case 3:
		BEG_INIT_CELLL=8;
		break;
	case 4:
	default:
		BEG_INIT_CELLL=16;
		break;
	}
	const float MAX_ROUGHNESS=0.5f;
	const float MIN_ROUGHNESS=1.1f;
	float fractionOfKRoughness=(kRoughness*5)-typeGeo;
	const float ROUGHNESS= MIN_ROUGHNESS- fractionOfKRoughness*abs(MAX_ROUGHNESS-MIN_ROUGHNESS);



	xassert(tb.sx==tb.sy);
	int powerSX=returnPower2(tb.sx);
	int powerSY=returnPower2(tb.sy);
	int sx=1<<powerSX;
	int sy=1<<powerSY;
	int clipmaskx=sx-1;
	int clipmasky=sy-1;

	short* geomap=new short[sx*sy];
	for(int s=0; s<sx*sy; s++) geomap[s]=0;
	int initStepPoint=sx/BEG_INIT_CELLL;
	int xi,yi;
	xi=0,yi=0;
	int i,j;
	for(i=0; i<BEG_INIT_CELLL; i++){
		xi=0;
		for(j=0; j<BEG_INIT_CELLL; j++){
			geomap[xi+yi*sx]=abs(maxHeight);//XRnd(30<<VX_FRACTION);
			xi+=initStepPoint;
		}
		yi+=initStepPoint;
	}
/*
	xi=0,yi=0;
	for(i=0; i<BEG_INIT_CELLL; i++){
		xi=0;
		for(j=0; j<BEG_INIT_CELLL; j++){
			int x,y;
			x=0; y=0;
			int stepPoint=initStepPoint;
			int D=abs(maxHeight);//100<<VX_FRACTION;
			float R=ROUGHNESS;//0.9f;
			while(stepPoint > 1){
				int stepPoint05=stepPoint>>1;// /2
				//1-pass
				y=stepPoint05;
				for(; y<initStepPoint; y+=stepPoint){
					x=stepPoint05;
					for(; x<initStepPoint; x+=stepPoint){
						int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
						int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
						int p3=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
						int p4=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
						int rnd=-D/2+XRnd(D);
						int v=((p1+p2+p3+p4)>>2)+ rnd;
						xassert(geomap[xi+x+(yi+y)*sx]==0);
						geomap[xi+x+(yi+y)*sx]=v;
					}
				}
				//2-pass
				y=0;
				for(; y<initStepPoint; y+=stepPoint){
					x=stepPoint05;
					for(; x<initStepPoint; x+=stepPoint){
						int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
						int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
						int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
						int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
						int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
						xassert(geomap[xi+x+(yi+y)*sx]==0);
						geomap[xi+x+(yi+y)*sx]=v;
					}
				}
				//3-pass
				y=stepPoint05;
				for(; y<initStepPoint; y+=stepPoint){
					x=0;
					for(; x<initStepPoint; x+=stepPoint){
						int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
						int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
						int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
						int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
						int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
						xassert(geomap[xi+x+(yi+y)*sx]==0);
						geomap[xi+x+(yi+y)*sx]=v;
					}
				}

				D=round((float)D*pow(2.f,-R));
				stepPoint>>=1;
			}
			xi+=initStepPoint;
		}
		yi+=initStepPoint;
	}
*/

	int D=abs(maxHeight);//100<<VX_FRACTION;
	float R=ROUGHNESS;//0.9f;

	int stepPoint=initStepPoint;
	while(stepPoint > 1){
		int stepPoint05=stepPoint>>1;// /2
		xi=0,yi=0;
		for(i=0; i<BEG_INIT_CELLL; i++){
			xi=0;
			for(j=0; j<BEG_INIT_CELLL; j++){
				int x,y;
				x=0; y=0;
				{
					//1-pass
					y=stepPoint05;
					for(; y<initStepPoint; y+=stepPoint){
						x=stepPoint05;
						for(; x<initStepPoint; x+=stepPoint){
							int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p3=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int p4=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int rnd=-D/2+XRnd(D);
							int v=((p1+p2+p3+p4)>>2)+ rnd;
							xassert(geomap[xi+x+(yi+y)*sx]==0);
							geomap[xi+x+(yi+y)*sx]=v;
						}
					}
/*					//2-pass
					y=0;
					for(; y<initStepPoint; y+=stepPoint){
						x=stepPoint05;
						for(; x<initStepPoint; x+=stepPoint){
							int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
							xassert(geomap[xi+x+(yi+y)*sx]==0);
							geomap[xi+x+(yi+y)*sx]=v;
						}
					}
					//3-pass
					y=stepPoint05;
					for(; y<initStepPoint; y+=stepPoint){
						x=0;
						for(; x<initStepPoint; x+=stepPoint){
							int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
							xassert(geomap[xi+x+(yi+y)*sx]==0);
							geomap[xi+x+(yi+y)*sx]=v;
						}
					}*/
				}
				xi+=initStepPoint;
			}
			yi+=initStepPoint;
		}

		xi=0,yi=0;
		for(i=0; i<BEG_INIT_CELLL; i++){
			xi=0;
			for(j=0; j<BEG_INIT_CELLL; j++){
				int x,y;
				x=0; y=0;
				{
					//2-pass
					y=0;
					for(; y<initStepPoint; y+=stepPoint){
						x=stepPoint05;
						for(; x<initStepPoint; x+=stepPoint){
							int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
							xassert(geomap[xi+x+(yi+y)*sx]==0);
							geomap[xi+x+(yi+y)*sx]=v;
						}
					}
				}
				xi+=initStepPoint;
			}
			yi+=initStepPoint;
		}

		xi=0,yi=0;
		for(i=0; i<BEG_INIT_CELLL; i++){
			xi=0;
			for(j=0; j<BEG_INIT_CELLL; j++){
				int x,y;
				x=0; y=0;
				{
					//3-pass
					y=stepPoint05;
					for(; y<initStepPoint; y+=stepPoint){
						x=0;
						for(; x<initStepPoint; x+=stepPoint){
							int p1=geomap[((xi+x-stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p2=geomap[((xi+x+stepPoint05)&clipmaskx) + ((yi+y)&clipmasky)*sx];
							int p3=geomap[((xi+x)&clipmaskx) + ((yi+y-stepPoint05)&clipmasky)*sx];
							int p4=geomap[((xi+x)&clipmaskx) + ((yi+y+stepPoint05)&clipmasky)*sx];
							int v=((p1+p2+p3+p4)>>2)-D/2+XRnd(D);
							xassert(geomap[xi+x+(yi+y)*sx]==0);
							geomap[xi+x+(yi+y)*sx]=v;
						}
					}
				}
				xi+=initStepPoint;
			}
			yi+=initStepPoint;
		}

		D=round((float)D*pow(2.f,-R));
		stepPoint>>=1;
	}

	//
	int cnt=0;
	const float XSIZE1_05=1.f/((float)tb.sx/2.f);
	const float YSIZE1_05=1.f/((float)tb.sy/2.f);
	const int SX05=tb.sx/2;
	const int SY05=tb.sy/2;
	short kZnak=1;
	if(maxHeight<0)kZnak=-1;
	for(i=0; i<tb.sy; i++){
		float y=(float)(i-SY05)*YSIZE1_05;
		for(j=0; j<tb.sx; j++){
			float x=(float)(j-SX05)*XSIZE1_05; 
			float f;
			if(expPower==2)
				f=exp(-(x*x+y*y)/kExpF); //(0.4f*0.4f)
			else 
				f=exp(-(fabsf(x*x*x)+fabsf(y*y*y))/kExpF); //(0.4f*0.4f)
			//tb.pRaster[cnt]= round((float)maxHeight*f);

			tb.pRaster[cnt]= round((float)geomap[j+i*sx]*f);//round((float)maxHeight*tmp*f);
			if(tb.pRaster[cnt]) tb.pRaster[cnt]=tb.pRaster[cnt]-1 + XRnd(2);
			tb.pRaster[cnt]*=kZnak;

			cnt++;
		}
	}
	delete[] geomap;
	XRndSet(storeRnd);
}

/*
sTerrainBitmap::sTerrainBitmap(int sx, int sy, sBitGenMetod* _pBitGenMetod)
{
	pRaster=new unsigned short[sx*sy];
	pBitGenMetod=_pBitGenMetod;
	pBitGenMetod->generate(*this);
}*/

///////////////////////////

/*
sTerrainBitmap* BitGenDispatcher::getTerrainBitmap(int sx, int sy, sBitGenMetod* _pBitGenMetod)
{
	list<sTerrainBitmap*>::iterator p;
	for(p=bitmaps_.begin(); p!=bitmaps_.end(); p++){
		if( *((*p)->pBitGenMetod)==*_pBitGenMetod && (*p)->sx==sx && (*p)->sy==sy ){
			int s=7;
			break;
		}
	}
	if(p==bitmaps_.end()){
		//добавление нового битмапа
		if(bitmaps_.size() >= CASH_MAX){
			delete bitmaps_.front();
			bitmaps_.pop_front();
		}
		bitmaps_.push_back(new sTerrainBitmap(sx,sy,_pBitGenMetod));
	}
	else {
		delete _pBitGenMetod;
	}
	return 0;
}
*/
