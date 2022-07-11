#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "..\Render\inc\Umath.h"
#include "..\UTIL\DebugUtil.h"
#include "..\Units\UnitAttribute.h"


extern int* xRad[MAX_RADIUS_CIRCLEARR + 1];
extern int* yRad[MAX_RADIUS_CIRCLEARR + 1];
extern int maxRad[MAX_RADIUS_CIRCLEARR + 1];

extern int MosaicTypes[8];

struct BitMap {
	unsigned short sx,sy;
	int sz;
	unsigned char* data;
	unsigned char* palette;
	int force,mode,border,copt;
	int mosaic;
	int size,level,Kmod,modeC2H,alpha;
	int xoffset,yoffset;
	double A11, A12, A21, A22;
	double X, Y;

		BitMap(int _mosaic = 0);

	void load(const char* name);
	void convert(void);
	void place(char* name,int x, int y,int _force,int _mode,int _border = 0,int _level = 0,int _size = 0,int Kmod=0, int _modeC2H = 0,int alpha=90, int ter_type=-1);
	inline int getDelta(int x,int y,int delta){
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		if(!mode) return delta*(64 - data[(y1%sy)*sx + (x1%sx)])/64; 
		return delta + force*(64 - data[(y1%sy)*sx + (x1%sx)])/64;
		//if(!mode) return delta*(64 - data[(y%sy)*sx + (x%sx)])/64;
		//return delta + force*(64 - data[(y%sy)*sx + (x%sx)])/64;
	}
	inline int getType(int x,int y){
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		return MosaicTypes[data[(y1%sy)*sx + (x1%sx)]%8]; 
		//return MosaicTypes[data[(y%sy)*sx + (x%sx)]%8]; 
	}
	inline int getColor(int x, int y){
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		return data[(y1%sy)*sx + (x1%sx)]; 
	};

	void set_alpha(double alpha, int X0, int Y0){
		X0+=vMap.H_SIZE;
		Y0+=vMap.V_SIZE;
		alpha-=90;
		A11 = A22 = cos(alpha*3.1415926535/180);
		A21 = -(A12 = sin(alpha*3.1415926535/180));
		X = X0 - A11*X0 - A12*Y0;
		Y = Y0 - A21*X0 - A22*Y0;
	};

};

extern BitMap placeBMP;
extern BitMap mosaicBMP;
extern int curBmpIndex;

extern void PutTrackPoints(int num_track);


//возвращает в указателе на слово следующее слово, а указатель на буфер передвигает
inline int get_world_in_buf(char*& buf, char* world)
{
	int counter=0;
	while( ((isspace(*buf) ) || (*buf=='=')) && (*buf) ) buf++;
	while( (!(isspace(*buf)) && (*buf!='=')) && (*buf) ){
		*world=*buf; world++, buf++, counter ++;
	}
	*world=0;
	return counter;
}


inline void damagingBuildingsTolzer(int _x, int _y, int _r)
{
	int x = _x >> kmGrid;
	int y = _y >> kmGrid;
	int r = _r >> kmGrid;
	if((r << kmGrid) != _r ) 
		r++;
	if(r > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in damaginBuildingsTolzer");
		r = MAX_RADIUS_CIRCLEARR;
	}
	for(int i = 0;i <= r; i++){
		int max = maxRad[i];
		int* xx = xRad[i];
		int* yy = yRad[i];
		for(int j = 0;j < max;j++) {
			int offG = vMap.offsetGBuf(vMap.XCYCLG(x + xx[j]), vMap.YCYCLG(y + yy[j]));
			if(vMap.GABuf[offG]&GRIDAT_BUILDING){
				vMap.GABuf[offG] |= GRIDAT_BASE_OF_BUILDING_CORRUPT;
			}
		}
	}
}

inline void damagingBuildingsTolzerS(int _xL, int _yU, int _xR, int _yD)
{
	int xL=vMap.w2mClampX(_xL);
	int xR=vMap.w2mClampX(_xR);
	int yU=vMap.w2mClampY(_yU);
	int yD=vMap.w2mClampY(_yD);
	int i,j;
	for(i=yU; i<=yD; i++){
		int offGY=vMap.offsetGBuf(0,i);
		for(j=xL; j<=xR; j++){
			if(vMap.GABuf[offGY+j]&GRIDAT_BUILDING){
				vMap.GABuf[offGY+j]|=GRIDAT_BASE_OF_BUILDING_CORRUPT;
			}
		}
	}
}

inline void damagingBuildingsTolzerS(int _x, int _y, int _r)
{
	damagingBuildingsTolzerS(_x-_r, _y-_r, _x+_r, _y+_r);
}


enum toolzer2TerrainEffect {
	T2TE_PLOTTING_VERY_LIGHT_DAM,
	T2TE_ALIGNMENT_TERRAIN_4ZP,
	T2TE_ALIGNMENT_TERRAIN_VARIABLE_H,
	T2TE_CHANGING_TERRAIN_HEIGHT,
	T2TE_CHANGING_TERRAIN_HEIGHT_IFNOTZP,
};
enum toolzerMovingTerrainType{
	TMTT_NOT_SUPPORT,
	TMTT_SUPPORT
};

///////////////////////////////////////////////////////////////////////////////////////////////////
template<toolzer2TerrainEffect _T2TE_> class individualToolzer{
public:
	int hAppr;
	individualToolzer(){
		hAppr=0;//vMap.hZeroPlast<<VX_FRACTION;
		locp=0;// 4 influenceDZ !
	}
	~individualToolzer(void){};

	void setHAppoximation(int hAppoximation){
		hAppr=hAppoximation<<VX_FRACTION;
	}

	int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea){

		int begx=vMap.XCYCL(x- (sx>>1));
		int begy=vMap.YCYCL(y- (sy>>1));
		unsigned char * ia=imageArea;
		int i,j;
		int offY=vMap.offsetBuf(0,begy);
		for(i=0; i<sy; i++){
			int offY=vMap.offsetBuf(0,vMap.YCYCL(begy+i));
			for(j=0; j<sx; j++){
				int dV=*(ia);
				int offB=offY+vMap.XCYCL(j+begx);
				switch(_T2TE_){
				case T2TE_PLOTTING_VERY_LIGHT_DAM:
					{
						if(dV) {
							vMap.SetAtr(offB, VmAt_Nrml_Dam);
							vMap.SurBuf[offB]=vMap.veryLightDam;
						}
					}
					break;
				case T2TE_ALIGNMENT_TERRAIN_VARIABLE_H:
				case T2TE_ALIGNMENT_TERRAIN_4ZP:
				case T2TE_CHANGING_TERRAIN_HEIGHT:
				case T2TE_CHANGING_TERRAIN_HEIGHT_IFNOTZP:
					{
						tVoxSet(offB, dV);
					}
					break;
				}
				ia++;
			}
		}
		//Эта операция должна быть совмещена с рендером
		vMap.recalcArea2Grid(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + sx+1), vMap.YCYCL(begy + sy+1) );
		vMap.regRender(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + sx+1), vMap.YCYCL(begy + sy+1) );

		return 0;
	}

//////////////////////////////////////////
	int locp;
	int influenceDZ(int x, int y, int rad, short dh, int smMode, unsigned char picMode=0){

		if(rad > MAX_RADIUS_CIRCLEARR){
			xassert(0&&"exceeding max radius in influenceDZ");
			rad=MAX_RADIUS_CIRCLEARR;
		}

		int begx=vMap.XCYCL(x - rad);
		int begy=vMap.YCYCL(y - rad);

		//void vrtMap::deltaZone(int x,int y,int rad,int smth,int dh,int smode,int eql)
		int eql=0;

		register int i,j;
		int max;
		int* xx,*yy;

		int r = rad - rad*smMode/10;
		float d = 1.0f/(rad - r + 1),dd,ds,s;
		int v, h, k, mean;

		if(dh){
			for(i = 0; i <= r; i++){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				for(j = 0;j < max;j++) {
					tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh);
				}
			}

			for(i = r + 1,dd = 1.0f - d; i <= rad; i++,dd -= d){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				h = round(dd*dh);//(int)
				if(!h) 
					h = dh > 0 ? 1 : -1;
				switch(picMode){
					case 0:
						v = round(dd*max);//(int)
						ds = (float)v/(float)max;
						for(s = ds,k = 0,j = locp % max; k < max; j = (j + 1 == max) ? 0 : j + 1,k++,s += ds)
							if(s >= 1.0){
								//tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh, hZeroPlast, heap);
								tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh);
								s -= 1.0;
								}
						break;
					case 1:
						v = round(dd*1000000.0);//(int)
						for(j = 0;j < max;j++)
							if((int)XRnd(1000000) < v) {
								//tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh, hZeroPlast, heap);
								tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh);
							}
						break;
					case 2:
						v = round(dd*max);//(int)
						for(k = 0,j = locp%max;k < v;j = (j + 1 == max) ? 0 : j + 1,k++){
							//tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh, hZeroPlast, heap);
							tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh);
						}
						locp += max;
						break;
					}
			}
			locp++;
		}
		else {
			const int DH_MEAN = 1; //дельта по которой усредняются высоты
			int cx,h,cy,cx_;
			if(eql){
				mean = k = 0;
				for(i = 0;i <= r;i++){
					max = maxRad[i];
					xx = xRad[i];
					yy = yRad[i];
					for(j = 0;j < max;j++){
						cx = vMap.XCYCL(x + xx[j]);
						mean += vMap.GetAlt(cx,vMap.YCYCL(y+yy[j]));
					}
					k += max;
				}
				mean /= k;
				for(i = 0;i <= r;i++){
					max = maxRad[i];
					xx = xRad[i];
					yy = yRad[i];
					for(j = 0;j < max;j++){
						cy = vMap.YCYCL(y+yy[j]);
						cx = vMap.XCYCL(x + xx[j]);
						h = vMap.GetAlt(cx,cy);
						if(abs(h - mean) < eql)
							if(h > mean){ //voxSet(cx,cy,-1);
								//tVoxSet( vMap.offsetBuf(cx, cy), -DH_MEAN);
								tVoxSetAll( vMap.offsetBuf(cx, cy), -DH_MEAN);
							}
							else if(h < mean){ //voxSet(cx,cy,1);
								//tVoxSet( vMap.offsetBuf(cx, cy), DH_MEAN);
								tVoxSetAll( vMap.offsetBuf(cx, cy), DH_MEAN);
							}
					}
				}
				for(i = r + 1,dd = 1.0f - d;i <= rad;i++,dd -= d){
					max = maxRad[i];
					xx = xRad[i];
					yy = yRad[i];
					h = round(dd*dh);//(int)
					if(!h) h = dh > 0 ? 1 : -1;
					v = round(dd*max);//(int)
					ds = (float)v/(float)max;
					for(s = ds,k = 0,j = locp%max;k < max;j = (j + 1 == max) ? 0 : j + 1,k++,s += ds)
						if(s >= 1.0){
							cy = vMap.YCYCL(y+yy[j]);
							cx = vMap.XCYCL(x + xx[j]);
							h = vMap.GetAlt(cx,cy);
							if(abs(h - mean) < eql)
								if(h > mean){ //voxSet(cx,cy,-1);
									//tVoxSet( vMap.offsetBuf(cx, cy), -DH_MEAN);
									tVoxSetAll( vMap.offsetBuf(cx, cy), -DH_MEAN);
								}
								else if(h < mean){ //voxSet(cx,cy,1);
									//tVoxSet( vMap.offsetBuf(cx, cy), DH_MEAN);
									tVoxSetAll( vMap.offsetBuf(cx, cy), DH_MEAN);
								}
							s -= 1.0;
							}
				}
			}
			else {
				int dx,dy;
				for(i = 0;i <= r;i++){
					max = maxRad[i];
					xx = xRad[i];
					yy = yRad[i];
					for(j = 0;j < max;j++){
						cy = vMap.YCYCL(y+yy[j]);
						cx = vMap.XCYCL(x + xx[j]);
						h = vMap.GetAlt(cx,cy);
						v = 0;
						switch(picMode){
							case 0:
								for(dy = -1;dy <= 1;dy++)
									for(dx = -1;dx <= 1;dx++){
										cx_ = vMap.XCYCL(cx+dx);
										v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
									}
								v -= h;
								v >>= 3;
								break;
							case 1:
							case 2:
								for(dy = -1;dy <= 1;dy++){
									for(dx = -1;dx <= 1;dx++){
										cx_ = vMap.XCYCL(cx+dx);
										if(abs(dx) + abs(dy) == 2)
											v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
									}
								}
								v >>= 2;
								break;
						}
						//voxSet(cx,cy,v - h);
						//tVoxSet( vMap.offsetBuf(cx, cy), v-h);
						tVoxSetAll( vMap.offsetBuf(cx, cy), v-h);
					}
				}
				for(i = r + 1,dd = 1.0f - d;i <= rad;i++,dd -= d){
					max = maxRad[i];
					xx = xRad[i];
					yy = yRad[i];
					h = round(dd*dh);//(int)
					if(!h) h = dh > 0 ? 1 : -1;

					v = round(dd*max);//(int)
					ds = (float)v/(float)max;
					for(s = ds,k = 0,j = locp%max;k < max;j = (j + 1 == max) ? 0 : j + 1,k++,s += ds){
						if(s >= 1.0){
							cy = vMap.YCYCL(y+yy[j]);
							cx = vMap.XCYCL(x + xx[j]);
							h = vMap.GetAlt(cx,cy);
							v = 0;
							switch(picMode){
								case 0:
									for(dy = -1;dy <= 1;dy++){
										for(dx = -1;dx <= 1;dx++){
											cx_ = vMap.XCYCL(cx+dx);
											v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
										}
									}
									v -= h;
									v >>= 3;
									break;
								case 1:
								case 2:
									for(dy = -1;dy <= 1;dy++){
										for(dx = -1;dx <= 1;dx++){
											cx_ = vMap.XCYCL(cx+dx);
											if(abs(dx) + abs(dy) == 2)
												v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
										}
									}
									v >>= 2;
									break;
							}
							//voxSet(cx,cy,v - h);
							//tVoxSet( vMap.offsetBuf(cx, cy), v-h);
							tVoxSetAll( vMap.offsetBuf(cx, cy), v-h);
							s -= 1.0;
						}
					}
				}
			}
		}

		//Эта операция должна быть совмещена с рендером
		vMap.recalcArea2Grid(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1) );
		vMap.regRender(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1), vrtMap::TypeCh_Height );


		return 0;
	}
///////////////////////////////////////////////////
	inline int tVoxSet(int offB, int dV){
		///if(vMap.GABuf[vMap.offsetBuf2offsetGBuf(offB)]&GRIDAT_BUILDING) return 0;
		if(dV<0){
			//dV=dV*((vMap.GABuf[vMap.offsetBuf2offsetGBuf(offB)]&GRIDAT_MASK_HARDNESS)<<(vMap.GABuf[vMap.offsetBuf2offsetGBuf(offB)]&GRIDAT_MASK_HARDNESS));
			//dV=dV/(24);
			if(Vm_IsIndestructability(vMap.VxABuf[offB])) return 0;
		}
		return tVoxSetAll(offB, dV);
	};
	inline int tVoxSetAll(int offB, int dV){
		int v=vMap.GetAlt(offB);

		switch (_T2TE_){
		case T2TE_PLOTTING_VERY_LIGHT_DAM:
			{
				if(dV) {
					vMap.SetAtr(offB, VmAt_Nrml_Dam);
					vMap.SurBuf[offB]=vMap.veryLightDam;
				}
			}
			break;

		case T2TE_ALIGNMENT_TERRAIN_4ZP:
			if( dV >0 ){
				if(v < hAppr){//Воздействие если инструмент добавляет и высота меньше hAppr
					v+=dV;
					if(v>hAppr) v=hAppr;
					vMap.PutAltAndGeoRecalc(offB, v);
				}
			}
			else {
				if(v > hAppr){//Воздействие если инструмент убирает и высота больше hAppr
					v+=dV;
					if(v<hAppr) v=hAppr;
					vMap.PutAltAndGeoRecalc(offB, v);
				}
			}
			if(v == hAppr) vMap.PutAltAndSetLeveling(offB, hAppr);
			return 0;
			break;
		case T2TE_ALIGNMENT_TERRAIN_VARIABLE_H:
			if( dV >0 ){
				if(v < hAppr){//Воздействие если инструмент добавляет и высота меньше hAppr
					v+=dV;
					if(v>hAppr) v=hAppr;
					vMap.PutAltAndGeoRecalc(offB, v);
					return dV;
				}
			}
			else {
				if(v > hAppr){//Воздействие если инструмент убирает и высота больше hAppr
					v+=dV;
					if(v<hAppr) v=hAppr;
					vMap.PutAltAndGeoRecalc(offB, v);
					return dV;
				}
			}
			return 0;
			break;
		case T2TE_CHANGING_TERRAIN_HEIGHT_IFNOTZP:
		case T2TE_CHANGING_TERRAIN_HEIGHT:
			if( dV >0 ){ //инструмент добавляет
				v+=dV;
				if(v>MAX_VX_HEIGHT) v=MAX_VX_HEIGHT;
				vMap.PutAltAndGeoRecalc(offB, v);
				return dV;
			}
			else { //if(dV <=0)  инструмент убирает 
				v+=dV;
				if(v< MIN_VX_HEIGHT) v=MIN_VX_HEIGHT;
				vMap.PutAltAndGeoRecalc(offB, v);
				return dV;
			}
			return 0;
			break;
		}
		return 0;
	};

};

class baseToolzer{
public:
	individualToolzer<T2TE_ALIGNMENT_TERRAIN_4ZP> toolzerAligmentTerrain4ZP;
	individualToolzer<T2TE_ALIGNMENT_TERRAIN_VARIABLE_H> toolzerAligmentTerrainVariableH;
	individualToolzer<T2TE_CHANGING_TERRAIN_HEIGHT> toolzerChangeTerHeight;
	individualToolzer<T2TE_CHANGING_TERRAIN_HEIGHT_IFNOTZP> toolzerChangeTerHeightIfNotZP;
	individualToolzer<T2TE_PLOTTING_VERY_LIGHT_DAM> toolzerPlottingVeryLightDam;
	int phase;
	int x,y;
	baseToolzer(){
		phase=0;
	};
	void start(){ phase=0; }
	void start(int _x, int _y){
		x=vMap.XCYCL(_x); y=vMap.YCYCL(_y);
		start();
	};
	int quant(int _x, int _y){
		x=vMap.XCYCL(_x); y=vMap.YCYCL(_y);
		return quant();
	};
	int quant(){
		int result=simpleQuant();
		if(result) 
			phase=phase++;
		else 
			phase=0;
		return result;
	};
	virtual int simpleQuant()=0;

	void saveStatus(XBuffer& ff){
		ff.write(&phase, sizeof(phase));
	};
	void loadStatus(XBuffer& ff){
		ff.read(&phase, sizeof(phase));
	};

};

inline void clearAtrBaseOfBuildingCorrupt(int x, int y, int rad)
{
	int i,j;
	const int begxg=(x-rad)>>kmGrid;
	const int begyg=(y-rad)>>kmGrid;
	const int endxg=(x+rad)>>kmGrid;
	const int endyg=(y+rad)>>kmGrid;
	for(j=begyg; j<=endyg; j++){
		int offGB=vMap.offsetGBuf(0,vMap.YCYCLG(j));
		for(i=begxg; i<=endxg; i++){
			int curoff=offGB+vMap.XCYCLG(i);
			if(vMap.GABuf[curoff]&GRIDAT_BUILDING && ((vMap.GABuf[curoff]&GRIDAT_LEVELED)==0) ) continue; //в случае здания только с полностью выровненной поверхноти снимается аттрибут поврежденной
			vMap.GABuf[curoff]&=~GRIDAT_BASE_OF_BUILDING_CORRUPT;
		}
	}
}

//individualToolzer<T2TE_ALIGNMENT_TERRAIN_4ZP> toolzerAligmentTerrain4ZP;
//individualToolzer<T2TE_ALIGNMENT_TERRAIN_VARIABLE_H> toolzerAligmentTerrainVariableH;
//toolzerAligmentTerrainVariableH
class baseDigPutToolzer : public baseToolzer{
public:
	bool flag_leveled_status;
	baseDigPutToolzer(){
		flag_leveled_status=1;
	};
	void setHAppoximation(int wh){
		toolzerAligmentTerrain4ZP.setHAppoximation(wh);
		toolzerAligmentTerrainVariableH.setHAppoximation(wh);

		flag_leveled_status=(wh!=0);
	};
};

class craterToolzerDestroyZP : public baseToolzer{
public:
	float kScale;
	craterToolzerDestroyZP(float _kScale=1.f){
		kScale=_kScale;
	};
	void setKScale(float _kScale=1.f){
		kScale=_kScale;
	}
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*32), 2*(1<<VX_FRACTION), 3 );
			break;
		case 1:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*24), -4*(1<<VX_FRACTION), 3 );
			break;
		case 2:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*16), -4*(1<<VX_FRACTION), 4 );
			break;
		case 3:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*32), 0, 2 );
			break;
		case 4:
			damagingBuildingsTolzer(x, y, round(kScale*32));
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};
////////////////
class wormInGroundToolzer: public baseToolzer{
public:
	float angle;
	wormInGroundToolzer(float _angle=0.f){
		setAngle(_angle);
	};
	void setAngle(float _angle){
		angle=_angle + M_PI/2;
	};
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)
		int xu=vMap.XCYCL(round(x+cos(angle)*10.));
		int yu=vMap.YCYCL(round(y+sin(angle)*10.));
		int xd=vMap.XCYCL(round(x-cos(angle)*10.));
		int yd=vMap.YCYCL(round(y-sin(angle)*10.));
		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 14, 4*(1<<VX_FRACTION), 10 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 14, 4*(1<<VX_FRACTION), 10 );
			toolzerChangeTerHeight.influenceDZ(xu, yu, 16, 0, 3 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 16, 0, 3 );
			break;
		case 1:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 14, round(0.5*(1<<VX_FRACTION)), 10 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 14, round(0.5*(1<<VX_FRACTION)), 10 );
			break;
		case 2:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 14, -round(0.5*(1<<VX_FRACTION)), 10 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 14, -round(0.5*(1<<VX_FRACTION)), 10 );
			break;
		case 3:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 16, 0, 3 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 16, 0, 3 );
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};


class wormOutGroundToolzer: public baseToolzer{
public:
	float angle;
	wormOutGroundToolzer(float _angle=0.f){
		setAngle(_angle);
	};
	void setAngle(float _angle){
		angle=_angle + M_PI/2;
	};
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)
		int xu=vMap.XCYCL(round(x+cos(angle)*10.));
		int yu=vMap.YCYCL(round(y+sin(angle)*10.));
		int xd=vMap.XCYCL(round(x-cos(angle)*10.));
		int yd=vMap.YCYCL(round(y-sin(angle)*10.));
		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 20, 2*(1<<VX_FRACTION), 5 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 20, 2*(1<<VX_FRACTION), 5 );
			toolzerChangeTerHeight.influenceDZ(xu, yu, 24, 0, 3 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 24, 0, 3 );

			//toolzerChangeTerHeight.influenceDZ(x, y, 28, 2*(1<<VX_FRACTION), 10 );
			break;
		case 1:
			toolzerChangeTerHeight.influenceDZ(xu, yu, 13, -4*(1<<VX_FRACTION), 10 );
			toolzerChangeTerHeight.influenceDZ(x,  y,  13, -4*(1<<VX_FRACTION), 10 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 13, -4*(1<<VX_FRACTION), 10 );
			toolzerChangeTerHeight.influenceDZ(xu, yu, 16, 0, 5 );
			toolzerChangeTerHeight.influenceDZ(x , y , 16, 0, 5 );
			toolzerChangeTerHeight.influenceDZ(xd, yd, 16, 0, 5 );
			return 0;
			//toolzerChangeTerHeight.influenceDZ(x, y, 32, 0, 3 );
			break;
		case 2:
			//toolzerChangeTerHeight.influenceDZ(x, y, 28, -2*(1<<VX_FRACTION), 10 );
			break;
		case 3:
			//toolzerChangeTerHeight.influenceDZ(x, y, 32, 0, 3 );
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};


class debrisToolzer: public baseToolzer{
public:
	int radius;
	float kScale;
	debrisToolzer(int _radius=32){
		setRadius(_radius);
	};
	void setRadius(int _radius=32){
		radius=_radius;
		kScale=(float)radius/32.f;
	}
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*32), -2*(1<<VX_FRACTION), 3 );
			break;
		case 1:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*24), +4*(1<<VX_FRACTION), 3 );
			break;
		case 2:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*16), +4*(1<<VX_FRACTION), 4 );
			break;
		case 3:
			toolzerChangeTerHeight.influenceDZ(x, y, round(kScale*32), 0, 2 );
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};


class ghostToolzer: public baseToolzer{
public:
	int radius;
	float kScale;
	ghostToolzer(int _radius=32){
		setRadius(_radius);
	};
	void setRadius(int _radius=32){
		radius=_radius;
		kScale=(float)radius/32.f;
	}
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			toolzerPlottingVeryLightDam.influenceDZ(x, y, round(kScale*27), 1*(1<<VX_FRACTION), 5);
			break;
		case 1:
			toolzerPlottingVeryLightDam.influenceDZ(x, y, round(kScale*32), 1*(1<<VX_FRACTION), 10);
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};

class demonToolzer: public baseToolzer{
public:
	int radius;
	float kScale;
	demonToolzer(int _radius=32){
		setRadius(_radius);
	};
	void setRadius(int _radius=32){
		radius=_radius;
		kScale=(float)radius/32.f;
	}
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			return 0;
			break;
		default:
			return 0;
		};
		return 1;
	};
};


class craterToolzerAbyssMake : public baseToolzer{
public:
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(x, y,5,-5*(1<<VX_FRACTION), 3 );
//			toolzerChangeTerHeight.influenceDZ(x, y,16,0,5);
			return 0;
		default:
			return 0;
		};
		return 1;
	};
};

class craterToolzerWallMake : public baseToolzer{
public:
	int simpleQuant(){
		//toolzerChangeTerHeight
		//toolzerChangeTerHeightIfNotZP
		//int influenceDZ(int x, int y, int rad, short dz, int smMode)
		//int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)

		switch(phase){
		case 0:
			toolzerChangeTerHeight.influenceDZ(x,y,16,3*(1<<VX_FRACTION),2);
			toolzerChangeTerHeight.influenceDZ(x,y,16,4,10);
			return 0;
		default:
			return 0;
		};
		return 1;
	};
};



inline void plotCircleZL(int x, int y, int r){
/*	if(r>MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in plotCircleZL ");
		r=MAX_RADIUS_CIRCLEARR;
	}
	int i, j;
	for(i = 0;i <= r; i++){
		int max = maxRad[i];
		int* xx = xRad[i];
		int* yy = yRad[i];
		for(j = 0;j < max;j++) {
			int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
			if( (vMap.AtrBuf[offB]&At_LEVELED)!=0 ) vMap.AtrBuf[offB]|=At_ZEROPLAST;
		}
	}
	vMap.recalcArea2Grid( vMap.XCYCL(x-r), vMap.YCYCL(y-r), vMap.XCYCL(x+r), vMap.YCYCL(y+r) );
	vMap.regRender( vMap.XCYCL(x-r), vMap.YCYCL(y-r), vMap.XCYCL(x+r), vMap.YCYCL(y+r) );*/
}

inline void eraseCircleZL(int x, int y, int r){
/*	if(r>MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in eraseCircleZL ");
		r=MAX_RADIUS_CIRCLEARR;
	}
	int i, j;
	for(i = 0;i <= r; i++){
		int max = maxRad[i];
		int* xx = xRad[i];
		int* yy = yRad[i];
		for(j = 0;j < max;j++) {
			int curX=vMap.XCYCL(x + xx[j]);
			int curY=vMap.YCYCL(y + yy[j]);
			int offB=vMap.offsetBuf(curX, curY);
				if((vMap.AtrBuf[offB]&At_ZPMASK)==At_ZEROPLAST) vMap.AtrBuf[offB]=(vMap.AtrBuf[offB]&(~At_NOTPURESURFACE))|At_LEVELED;
		}
	}
	vMap.recalcArea2Grid( vMap.XCYCL(x-r), vMap.YCYCL(y-r), vMap.XCYCL(x+r), vMap.YCYCL(y+r) );
	vMap.regRender( vMap.XCYCL(x-r), vMap.YCYCL(y-r), vMap.XCYCL(x+r), vMap.YCYCL(y+r) );*/
}



//#################################################################################################



///////////////////////////////////////////////////////////////////////
struct sVBitMapMosaic {
	unsigned short sx,sy;
	int sz;
	unsigned char* data;
	int effect;//force,
	int k_effect;
	int xoffset,yoffset;
	double A11, A12, A21, A22;
	double X, Y;
	bool Enable;

	sVBitMapMosaic(void){
		data = NULL;
		//force = 256; 
		effect = 0;
		k_effect = 256;
		xoffset = yoffset = 0;
		Enable=0;
	};

	void create(int _sx, int _sy){
		sx=_sx; sy=_sy;
		data=new unsigned char[sx*sy];
	};
	void release(void){
		if(data != NULL) {
			delete [] data;
			data=NULL;
		}
		Enable=0;
	};
	~sVBitMapMosaic(void){
		release();
	};

	inline int getDelta(int x,int y,int delta){
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		if(!effect) return delta*(data[(y1%sy)*sx + (x1%sx)])/256; //-128
		return delta + k_effect*(data[(y1%sy)*sx + (x1%sx)]-128)/256;//force*
	}
/*	inline int getType(int x,int y){ 
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		return MosaicTypes[data[(y1%sy)*sx + (x1%sx)]%8]; 
	}*/
	inline int getColor(int x, int y){
		x += xoffset;
		y += yoffset;
		x+=vMap.H_SIZE;
		y+=vMap.V_SIZE;
		int x1,y1;
		x1 = round(A11*x + A12*y + X);
		y1 = round(A21*x + A22*y + Y);
		while (x<0){ x+=sx;}
		while (y<0){ y+=sy;}
		return data[(y1%sy)*sx + (x1%sx)]; 
	};

	void set_alpha(double alpha, int X0, int Y0){
		X0+=vMap.H_SIZE;
		Y0+=vMap.V_SIZE;
		//alpha-=90;
		A11 = A22 = cos(alpha*3.1415926535/180);
		A21 = -(A12 = sin(alpha*3.1415926535/180));
		X = X0 - A11*X0 - A12*Y0;
		Y = Y0 - A21*X0 - A22*Y0;
	};

};
extern sVBitMapMosaic VBitMapMosaic;
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////

struct sTerrainMetod {
	enum ePlaceMetod{
		PM_Absolutely,
		PM_AbsolutelyMAX,
		PM_AbsolutelyMIN,
		PM_Relatively
	};
	ePlaceMetod mode;
	int level;
	int noiseLevel;
	int noiseAmp;
	//bool inverse;
	sTerrainMetod(void){
		mode=PM_Absolutely; level=0; noiseLevel=0; noiseAmp=1<<VX_FRACTION;
	};
	void update( ePlaceMetod _mode, int _level, int _noiseLevel, int _noiseAmp){
		mode=_mode; level=_level; noiseLevel=_noiseLevel; noiseAmp=_noiseAmp;
	};
	void put(int x, int y, int v);
};
extern sTerrainMetod TerrainMetod;

//////////////////////////////////////////////
#endif // __TOOLS_H__
