#include "stdafxTr.h"

#include "Serialization\Serialization.h"

#include "geo2.h"
#include "tools.h"
#include "bitGen.h"

const char Path2GeoResource[]="Scripts\\Resource\\Tools\\";
//const long	SGEOWAVE_FRONT_WIDTH=10;
//const long	SGEOWAVE_DELTA_FRONT_WIDTH=2;
extern float noise1(float arg);

const short SGEOWAVE_LONG_WAVE=18;
const float	SGEOWAVE_SPEED_WAVE=2.;
const float	SGEOWAVE_MAX_WAVE_AMPLITUDE=6;//<<VX_FRACTION;

const long SGEOWAVE_MAX_TOTAL_RADIUS=MAX_RADIUS_CIRCLEARR;//100;

sGeoWave::sGeoWave()
{
	maxWaveAmplitude=SGEOWAVE_MAX_WAVE_AMPLITUDE;
	waveLenght=SGEOWAVE_LONG_WAVE;
	waveSpeed=SGEOWAVE_SPEED_WAVE;
	begRadius=0;

	pos=Vect2i(0,0);
	maxRadius=0;
	fadeRadius=0;
	begPhase=0;

	flag_Track=false;

	flag_beginInitialization=false;
}

//sGeoWave::sGeoWave(short _x, short _y, short _maxRadius)
//{
//	flag_Track=false;
//
//	maxWaveAmplitude=SGEOWAVE_MAX_WAVE_AMPLITUDE;
//	waveLenght=SGEOWAVE_LONG_WAVE;
//	waveSpeed=SGEOWAVE_SPEED_WAVE;
//	begRadius=0;
//
//	pos.x=_x; pos.y=_y;
//	setRadius(_maxRadius);
//
//	flag_beginInitialization=false;
//}

void sGeoWave::setRadius(short _maxRadius)
{
	if(_maxRadius > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in sGeoWave ");
		_maxRadius=MAX_RADIUS_CIRCLEARR;
	}
	maxRadius=_maxRadius-waveLenght;
}


float sGeoWave::calcMaxAmpWave(short curBegRad)
{
	float amp=1.f;
	//if(curBegRad > (maxRadius*1/2) )
	//	amp=(float)((maxRadius*1/2)-(curBegRad-maxRadius*1/2)) / (maxRadius*1/2);
	if(curBegRad > fadeRadius ){
		amp=(float)((maxRadius-fadeRadius)-(curBegRad-fadeRadius)) / (maxRadius-fadeRadius);
	}
	return (1.f+noise1(3.4f+(float)curBegRad/10))*amp*maxWaveAmplitude*VOXEL_MULTIPLIER;

}
bool sGeoWave::quant()
{
	const long SGEOWAVE_MAX_RADIUS=SGEOWAVE_MAX_TOTAL_RADIUS-waveLenght;
	if(!flag_beginInitialization){
		//if(maxRadius < 0) maxRadius=0;
		//if(maxRadius >= SGEOWAVE_MAX_RADIUS) maxRadius=SGEOWAVE_MAX_RADIUS-1; //Не обязательно (есть проверка ниже)
		maxRadius=clamp(maxRadius, 0, SGEOWAVE_MAX_RADIUS-1);
		begRadius=clamp(begRadius, 0, maxRadius);
		step=0;
		flag_beginInitialization=true;
		//return true;
	}
	short x=pos.x,y=pos.y;
	int i, j;
	float fBegPhase=(float)begPhase*M_PI/180;
	if(step>0){
		unsigned int storeRnd=XRndGet();
		XRndSet(prevRnd);
		short curBegRadWave=1 + begRadius + round(waveSpeed*(float)(step-1));
		float curMaxAmpWave=calcMaxAmpWave(curBegRadWave);
		int kTrack;//=!flag_Track;
		if(flag_Track)
			kTrack=3;
		else 
			kTrack=4;

		long begAmp=round(curMaxAmpWave * sin(fBegPhase + (float)0*((2*M_PI)/waveLenght) ) );
		for(i = 0; i < waveLenght; i++){
			long curAmp=-begAmp + round(curMaxAmpWave * sin(fBegPhase + (float)i*((2*M_PI)/waveLenght) ) );
			int curRad=curBegRadWave+i;
			int max = maxRad[curRad];
			int* xx = xRad[curRad];
			int* yy = yRad[curRad];
			for(j = 0;j < max;j++) {
				int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
				long V=vMap.getAlt(offB);
				V-=curAmp+(((8-(int)XRnd(1<<4))*kTrack)>>2);
				if(V<0)V=0;
				else if(V>MAX_VX_HEIGHT)V=MAX_VX_HEIGHT;
				vMap.putAlt(offB, V);
			}
		}
		XRndSet(storeRnd);
	}

	short curBegRadWave=1 + begRadius + round(waveSpeed*(float)(step));
	if( (curBegRadWave >= maxRadius) || (curBegRadWave >= SGEOWAVE_MAX_RADIUS) ) {
		short renderRad=curBegRadWave+waveLenght;
		damagingBuildingsTolzer(x, y, maxRadius+waveLenght);
		vMap.recalcArea2Grid(vMap.XCYCL(x-renderRad), vMap.YCYCL(y-curBegRadWave), vMap.XCYCL(x + renderRad+1), vMap.YCYCL(y + renderRad+1) );
		vMap.regRender(x-renderRad, y-renderRad, x+renderRad, y+renderRad, vrtMap::TypeCh_Height);
		return 0;
	}

	float curMaxAmpWave=calcMaxAmpWave(curBegRadWave);
	prevRnd=XRndGet();
	long begAmp=round(curMaxAmpWave * sin(fBegPhase + (float)0*((2*M_PI)/waveLenght) ) );
	for(i = 0; i < waveLenght; i++){
		long curAmp=-begAmp + round(curMaxAmpWave * sin(fBegPhase + (float)i*((2*M_PI)/waveLenght) ) );
		int curRad=curBegRadWave+i;
		int max = maxRad[curRad];
		int* xx = xRad[curRad];
		int* yy = yRad[curRad];
		for(j = 0;j < max;j++) {
			int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
			long V=vMap.getAlt(offB);
			V+=curAmp+8-XRnd(1<<4);
			//vMap.putAlt(offB, V);
			if(V<0)V=0;
			else if(V>MAX_VX_HEIGHT)V=MAX_VX_HEIGHT;
			vMap.putAlt(offB, V);
		}
	}
	short renderRad=curBegRadWave+waveLenght;
	vMap.recalcArea2Grid(vMap.XCYCL(x-renderRad), vMap.YCYCL(y-curBegRadWave), vMap.XCYCL(x + renderRad+1), vMap.YCYCL(y + renderRad+1) );
	vMap.regRender(x-renderRad, y-renderRad, x+renderRad, y+renderRad, vrtMap::TypeCh_Height);

	step++;	
	return 1;
}

void sGeoWave::serialize(Archive& ar)
{
	ar.serialize(pos, "pos", "Позиция");
	ar.serialize(maxRadius, "maxRadius", "Радиус");
	ar.serialize(begRadius, "begRadius", "Начальный радиус");
	ar.serialize(fadeRadius, "fadeRadius", "Начальный радиус затухания");
	ar.serialize(maxWaveAmplitude, "maxWaveAmplitude", "Максимальная амплитуда");
	ar.serialize(waveLenght, "waveLenght", "Длинна волны");
	ar.serialize(waveSpeed, "waveSpeed", "Скорость волны");
	ar.serialize(begPhase, "begPhase", "Фаза(-90/0/+90)");
	ar.serialize(flag_Track, "flag_Track", "Оставлять след");
}


//struct sGeoWaveSrc : public SourceEffect {
//	mutable Vect3f full_position_;
//	sGeoWave geo;
//	int period; //0-не периодичный
//	DurationTimer sleepTimer;
//	sGeoWaveSrc() : SourceEffect() {
//		period=0;
//	}
//	sGeoWaveSrc(Vect2i pos, short _maxRadius) : SourceEffect(), geo(pos.x,pos.y,_maxRadius){
//		Vect3f pos3d = To3D(Vect2f(pos.x,pos.y));
//		pose_.trans() = pos3d;
//		period=0;
//	}
//	SourceBase* clone() const {
//		return new sGeoWaveSrc(*this);
//	}
//	void setActivity(bool _active){
//		__super::setActivity(_active);
//		geo.flag_beginInitialization=false;
//	}
//	void quant(){
//		SourceBase::quant();
//		xassert(enabled_);
//		if(!active_ || sleepTimer()) {
//			return/* false*/;
//		}
//		bool result=geo.quant();
//		if(!result){
//			if(period){
//				setActivity(true);
//				sleepTimer.start(period);
//			}
//			else 
//				setActivity(false);
//		}
//        return/*result*/;
//
//	}
//
//	void setPose(const Se3f& pos, bool init) {
//		SourceEffect::setPose(pos, init);
//		geo.setPosition (Vect2i(pos.trans().xi(), pos.trans().yi()));
////		effectStart ();
//	}
//
//	const Vect3f& position () const {
//		full_position_.set(geo.pos.x, geo.pos.y, 0.0f);
//		return full_position_;
//	}
//
//	float radius() const {
//		return float(geo.maxRadius);
//	}
//
//	void serialize(Archive& ar);
//	SourceType type()const{return SOURCE_GEOWAVE;}
//};
//
//struct GeoBreakSrc : public SourceEffect
//{
//	GeoBreak gbreak;
//	float period; //0-не периодичный
//	DurationTimer sleepTimer;
//	GeoBreakSrc() : SourceEffect() {
//		period=0;
//	}
//	SourceBase* clone() const {
//		return new GeoBreakSrc(*this);
//	}
//	GeoBreakSrc(Vect2i _pos, int _rad=100, int _begNumBreak=0) : SourceEffect(), gbreak(_pos,_rad,_begNumBreak)
//	{ //0-случайное кол-во
//		setPose(Se3f(QuatF::ID, Vect3f(_pos.x, _pos.y, 0.0f)), true);
//		period=0;
//	}
//
//	void setActivity(bool _active){
//		__super::setActivity(_active);
//		gbreak.flag_beginInitialization=false;
//	}
//	void quant(){
//		SourceBase::quant();
//		xassert(enabled_);
//
//		if(!active_ || sleepTimer()) {
//			return/* false*/;
//		}
//		bool result=gbreak.quant();
//		if(!result){
//			if(period){
//				setActivity(true);
//				sleepTimer.start(period);
//			}
//			else 
//				setActivity(false);
//		}
//        return/*result*/;	
//
//	}
//
//	void setPose(const Se3f& pos, bool init) { 
//		SourceEffect::setPose(pos, init);
//		gbreak.setPosition (Vect2i (pos.trans().xi(), pos.trans().yi())); 
////		effectStart();
//	}
//
//	void serialize(Archive& ar);
//	SourceType type()const{return SOURCE_BREAK;}
//};
//
//void sGeoWaveSrc::serialize(Archive& ar)
//{
//	geo.serialize(ar);
//	SourceEffect::serialize(ar);
//	ar.serialize(period, "period", "Период");
//}
//
//void GeoBreakSrc::serialize(Archive& ar)
//{
//	gbreak.serialize(ar);
//	SourceEffect::serialize(ar);
//	ar.serialize(period, "period", "Период");
//}
//

const int BEGIN_BUBBLE_DEEP=0<<VX_FRACTION;
const int DELTA_BUBBLE_NOT_DEEP=0<<VX_FRACTION;
static const int tb_arraySX=128;//256;
static const int tb_arraySY=128;//256;
static const int tb_keyPoints=2;
static const short tb_keyPointsTime[tb_keyPoints]={4,2};//5,2//20,15

unsigned short* sTBubble::preImage=0;
int sTBubble::numPreImage=0;

sTBubble::sTBubble(int _x, int _y, int _sx, int _sy, bool _flag_occurrenceGeo)
{
	if(preImage==0){
		const int NUMBER_PRE_IMAGE=2;
		numPreImage=NUMBER_PRE_IMAGE;
		preImage=new unsigned short[tb_arraySX*tb_arraySY*tb_keyPoints*numPreImage];

		char cb[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, cb);
		strcat(cb, "\\"); strcat(cb, Path2GeoResource); strcat(cb, "bub.dat");//"\\RESOURCE\\Tools\\bub.dat"
		XZipStream fi;
		fi.open(cb, XS_IN);
		fi.seek(0,XS_BEG);
		//unsigned short buf[][256*2];
		fi.read(preImage, tb_keyPoints * tb_arraySY*tb_arraySX*sizeof(unsigned short)*numPreImage);
		fi.close();
	};

	x=_x-_sx/2; y=_y-_sy/2; sx=_sx; sy=_sy;
	flag_sleep=0;//_flag_sleep;
	flag_occurrenceGeo=_flag_occurrenceGeo;

	array=new short[sx*sy*tb_keyPoints];
	mask=new unsigned char[sx*sy];
	maxVx=new unsigned char[sx*sy];

	previsionKP=-1;
	currentKP=0;
	frame=0;
	currentKPFrame=0;
	pPrevKP=0; 
	pCurKP=array;

	inVx=new short[sx*sy];
	tmpVx=new short[sx*sy];
	substare=new short[sx*sy];
	int i,j,cnt=0;
	int vmin=MAX_VX_HEIGHT;
	int vmax=0;
	for(i=0; i<sy; i++){
		int offy=vMap.offsetBuf(0, vMap.YCYCL(y+i));
		for(j=0; j<sx; j++){
			int off=offy+vMap.XCYCL(x+j);
			int v=vMap.getAlt(off);
			*(inVx+cnt)=v;
			*(tmpVx+cnt)=0;
			substare[cnt]=v;
			if( v < vmin) vmin=v;
			if( v > vmax) vmax=v;
			mask[cnt]=0;
			maxVx[cnt]=0;
			cnt++;
		}
	}
	//if((vmax-vmin)>DELTA_BUBBLE_NOT_DEEP) h_begin=vmin;
	//else h_begin=vmin-BEGIN_BUBBLE_DEEP;
	h_begin=vmin+(vmax-vmin)/3;//+(vmax-vmin)/4;
	vMin=vmin;
	h_begin=(vmin+vmax)/2;

/*		//int kScl=((MAX_VX_HEIGHT-h_begin)<<16)/(MAX_VX_HEIGHT);
	int kScl=round(1.5f*(float)(1<<16));
	XStream fo;
	fo.open("bub.dat", XS_IN);
	fo.seek(0,XS_BEG);
	unsigned short buf[256*2];
	cnt=0;
	for(i = 0; i < tb_arraySY*tb_keyPoints; i++){
		//fo.read(array+i*tb_arraySX, tb_arraySX*sizeof(unsigned short));
		//fo.read(buf, 256*sizeof(unsigned short)*2);
		fo.read(buf, tb_arraySX*sizeof(unsigned short));
		for(j = 0; j < tb_arraySX; j++) {
			//array[cnt]=(buf[j*2]*kScl)>>16;
			array[cnt]=(buf[j]*kScl)>>16;
			//array[cnt]=array[cnt]>>1 - (5<<VX_FRACTION);
			//if(array[cnt]<0)array[cnt]=0;
			cnt++;
		}
	}*/
	
	Vect3f position(x, y, (float)h_begin/(float)(1<<VX_FRACTION));
	Vect3f outOrientation;
	extern void analyzeTerrain(Vect3f& position, float radius, Vect3f& outOrientation);
	analyzeTerrain(position, sx/2, outOrientation);
	//h_begin=round(position.z*(1<<VX_FRACTION));
	float A=outOrientation.x;
	float B=outOrientation.y;
	float C=-outOrientation.z;
	float D=(float)h_begin/(float)(1<<VX_FRACTION);//position.z;

	unsigned short* curPreImage=preImage+tb_arraySX*tb_arraySY*tb_keyPoints*XRnd(numPreImage);
	cnt=0;
	int kScl=round(1.8f*(float)(1<<16));
	///!int kScl=round(2.4f*(float)(1<<16));
	float fidx=(float)tb_arraySX/(float)sx;
	float fidy=(float)tb_arraySY/(float)sy;
	if(fidx > fidy){ kScl=round(kScl/fidy);}
	else { kScl=round(kScl/fidx); }
	int i_dx = (tb_arraySX << 16)/sx;
	int i_dy = (tb_arraySY << 16)/sy;
	int fx,fy;
	int m;
	for(m=0; m < tb_keyPoints; m++){
		for(i = 0; i < sy; i++){
			fy=i_dy*i;
			fx=0;
			unsigned short* pi=curPreImage+(m*tb_arraySY*tb_arraySX)+(fy>>16)*tb_arraySX;
			for(j = 0; j < sx; j++) {
				array[cnt]=( (*(pi+(fx>>16)))*kScl )>>16;
				//if(array[cnt]) array[cnt]+=round( (-(A*(j-sx/2) + B*(i-sy/2) + D)/C) * (1<<VX_FRACTION) - h_begin );//
				if(m==0){
					float as=(-(A*(j-sx/2) + B*(i-sy/2) + D)/C);
					//substare[cnt]=round( (-(A*(j-sx/2) + B*(i-sy/2) + D)/C ) * (1<<VX_FRACTION) );
				}
				fx+=i_dx;
				cnt++;
			}
		}
	}
}
 
int sTBubble::quant()
{
	int curKScale=((currentKPFrame+1)<<16)/tb_keyPointsTime[currentKP];
	int prevKScale=(1<<16)-curKScale;
	int xxx=0;//x%d;
	int yyy=0;//y%d;
	int i,j,cnt=0;
	for(i = 0; i < sy; i++){
		for(j = 0; j < sx; j++) {
			int offset=vMap.offsetBuf((x+j) & vMap.clip_mask_x, (y+i) & vMap.clip_mask_y);
			int vm=vMap.getAlt(offset);
			if(vm < inVx[cnt]){
				substare[cnt]-=inVx[cnt]-vm;
			}
			inVx[cnt]=vm;

			int prevH;
			if(pPrevKP!=0) 
				prevH = prevKScale*pPrevKP[i*sx+j];
			else 
				prevH = 0;
			int V=(prevH + pCurKP[i*sx+j]*curKScale) >>(16);
			if(V==0) { cnt++; continue;}
			V+=substare[cnt];
			if(V > MAX_VX_HEIGHT) V=MAX_VX_HEIGHT;
			if(vm < V){ //нарастание
				tmpVx[cnt]+=V-vm;
			}
			else {
				int dh= vm - V;
				if(tmpVx[cnt] > dh){
					tmpVx[cnt]-=dh;
				}
				else {
					dh=tmpVx[cnt];
					tmpVx[cnt]=0;
				}
				V=vm-dh;
				if(V<0)V=0;
			}
			vMap.putAlt(offset, V);
			cnt++;

/*
			//if(pCurKP[i*sx+j]==0 && mask[i*sx+j]==0) continue;
			//mask[i*sx+j]=1;
			int offset=vMap.offsetBuf((x+j) & vMap.clip_mask_x, (y+i) & vMap.clip_mask_y);

			int vv=vMap.getAlt(offset);
			if(tmpVx[cnt]!=vv){
				inVx[cnt]+=vv-tmpVx[cnt];
//				if(inVx[cnt]<0)inVx[cnt]=0;
//				if(inVx[cnt]>MAX_VX_HEIGHT)inVx[cnt]=MAX_VX_HEIGHT;
			}

			int Vold=inVx[cnt];
			int prevH;
			if(pPrevKP!=0) 
				prevH = prevKScale*pPrevKP[i*sx+j];
			else 
				prevH = 0;
			int curH=(prevH + pCurKP[i*sx+j]*curKScale) >>(16) ;
			V=curH;
			if(curH==0 && mask[i*sx+j]==0) {tmpVx[cnt]=Vold; cnt++; continue;} //!!!!!!!
			mask[i*sx+j]=1;									  //!!!!!!!

			V+=substare[cnt];
			if(Vold > V)
				V=Vold;

			if(V>>VX_FRACTION > maxVx[cnt])maxVx[cnt]=V>>VX_FRACTION;
			const int DELTA_DEEP_GEO=3;//10;
			if(flag_occurrenceGeo && (V>>VX_FRACTION < maxVx[cnt]-DELTA_DEEP_GEO) ){
				if(Vm_IsDam(vMap.vxaBuf[offset])){
					vMap.vxaBuf[offset]=VmAt_Nrml_Geo; //Hint высота присваивается ниже
					vMap.SurBuf[offset]=vMap.GetGeoType(offset,V);
				}
			}
			//if(V<0)V=0;
			//if(V>MAX_VX_HEIGHT)V=MAX_VX_HEIGHT;

			//для нормальной границы при  разрушении зеропласта
			vMap.putAltSpecial(offset, V);

			tmpVx[cnt]=V;

			cnt++;*/
		}
	}
	vMap.recalcArea2Grid(vMap.XCYCL(x-1), vMap.YCYCL(y-1), vMap.XCYCL(x + sx+1), vMap.YCYCL(y + sy+1) );
	vMap.regRender(x, y, x+sx, y+sy, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture);

	frame++; currentKPFrame++;
	if( currentKPFrame >= tb_keyPointsTime[currentKP] ) {
		previsionKP=currentKP;
		currentKP++;
		currentKPFrame=0;
		pPrevKP = pCurKP;
		pCurKP+=sx*sy;
		if(currentKP>=tb_keyPoints) {
			previsionKP=-1; 
			currentKP=0;
			pPrevKP=0; 
			pCurKP=array;
			return 0;
		}
	}
	return 1;
}


const int bubleWSXSY=100;
const int bubleWSXSY05=bubleWSXSY>>1;
typedef int tBubleWormForm[bubleWSXSY][bubleWSXSY];
tBubleWormForm bubleWormForm;

sTerrainBitmapBase formBitmap;

sTorpedo::sTorpedo()
{
	for(int m=0; m<CTORPEDO_MAX_EL_ARR; m++) bubArr[m]=0;
	num_el_arr=0;
	step=0;
}
sTorpedo::sTorpedo(int x, int y)
{

	float angl=logicRNDfabsRnd(M_PI*2);
	Vect2f tmp(cos(angl), sin(angl));
	init(x, y, tmp);
}

sTorpedo::sTorpedo(int _x, int _y, Vect2f& _direction)
{
	init(_x, _y, _direction);
}

void sTorpedo::init(int _x, int _y, Vect2f& _direction)
{
	beginX=_x; beginY=_y;
	curX=beginX; curY=beginY;
	for(int m=0; m<CTORPEDO_MAX_EL_ARR; m++) bubArr[m]=0;
	num_el_arr=0;
	//begAngle=_begAngle;//-M_PI/10.;
	direction=_direction;
	direction.normalize(1.);
	step=0;
}
bool sTorpedo::insert2Arr(int _x, int _y, int _sx, int _sy, bool _flag_occurrenceGeo)
{

	int bxg, byg, exg, eyg;
	bxg=_x>>kmGrid;
	byg=_y>>kmGrid;
	exg=(_x+_sx)>>kmGrid;
	eyg=(_y+_sy)>>kmGrid;
	///int i,j;
	///for(i=byg; i<=eyg; i++){
	///	for(j=bxg; j<=exg; j++){
	///		if((vMap.gABuf[vMap.offsetGBufC(j, i)]&GRIDAT_BUILDING) != 0) return 0;
	///	}
	///}

	int idx=findFreeEl();
	if(idx<0) return 0;
	else{
		bubArr[idx]=new sTBubble(_x, _y, _sx, _sy, _flag_occurrenceGeo);
		num_el_arr++;
		return 1;
	}
}


sRect sTorpedo::quant(Vect2f& prevPos, Vect2f& curPos)
{
	curX=curPos.x;
	curY=curPos.y;
	//direction=dir;

	individualToolzer<T2TE_CHANGING_TERRAIN_HEIGHT> toolzerChangeTerHeight;

	const float kRoughness=0.2f;
	const int hT=1<<VX_FRACTION;

	static bool flag_firs=1;
	static int k_dh[256];
	if(flag_firs){
		flag_firs=0;
		//int i,j;
		//for(i=0; i<bubleWSXSY; i++){
		//	float y=(float)(i-bubleWSXSY05)/(float)bubleWSXSY05;
		//	for(j=0; j<bubleWSXSY; j++){
		//		float x=(float)(j-bubleWSXSY05)/(float)bubleWSXSY05; 
		//		float f=exp(-(fabsf(x*x)+fabsf(y*y))/(0.4*0.4));
		//		//bubleWormForm[i][j]=round((f*0.7f)*(float)(1<<14)); //15 //(1<<16)+XRnd(1<<14);
		//		bubleWormForm[i][j]=round(f*(float)(1<<14)); //15 //(1<<16)+XRnd(1<<14);
		//	}
		//}
		formBitmap.create(2*radius, 2*radius);
		//sBitGenMetodMPD(2, hT, 0, kRoughness).generate(formBitmap);
		sBitGenMetodExp(2, (1<<14), 0).generate(formBitmap);


		for(int m=0; m<256; m++){
			float x=(float)m/256;//Диапазон от 0 до 1
			k_dh[m]=(0.99+0.1*sin(x*50))* exp(-fabsf((x)*(x)*(x))/(0.4f*0.4f))*(1<<16);//(1.2+0.05*sin(2*tan(x*50)))
		}
	}



	//const float averageSpeed=3.0;//4
	//const float deltaSpeed=2.0;
	//float SPEED=averageSpeed+logicRNDfrnd(deltaSpeed);
	//curX+=/*cos(begAngle)*/direction.x*SPEED;
	//curY+=/*sin(begAngle)*/direction.y*SPEED;

	const int BORDER=radius*2;
	if(curX < BORDER || curX+BORDER>=vMap.H_SIZE) return sRect();
	if(curY < BORDER || curY+BORDER>=vMap.V_SIZE) return sRect();
	int i,j;

//Выпячивание
	float kOffset=4.;//8
	//int curIX=round(curX - /*cos(begAngle)*/direction.x*SPEED*(float)kOffset);
	//int curIY=round(curY - /*sin(begAngle)*/direction.y*SPEED*(float)kOffset);
	//int curIX=round(curX - (curPos.x-prevPos.x)*(float)kOffset);
	//int curIY=round(curY - (curPos.y-prevPos.y)*(float)kOffset);
	int curIX=round(curX);
	int curIY=round(curY);

////	worms(curX-32, curY-32);
	int minV;
	minV=MAX_VX_HEIGHT;
	int countZERO=0;
	for(i = curIY-radius; i < curIY+radius; i++) {
		for(j = curIX-radius; j < curIX+radius; j++) {
			int offset=vMap.offsetBuf(vMap.XCYCL(j), vMap.YCYCL(i));
			//if(minV > vMap.VxGBuf[offset]<<VX_FRACTION)minV=vMap.VxGBuf[offset]<<VX_FRACTION;
			int tvx=vMap.getAltWhole(offset)<<VX_FRACTION;
			if(minV > tvx) minV=tvx;
			if(tvx==0) countZERO++;
		}
	}
	const int Maximum_quantity_of_points_of_zero_height_for_permeability=128;
	if(countZERO>Maximum_quantity_of_points_of_zero_height_for_permeability) return sRect();

	minV-=1<<VX_FRACTION;
	float K_FLUCTUATION=.3f;
	//int curKFluct=round((3.0+logicRNDfrnd(K_FLUCTUATION))*(1<<16));
	int curKFluct=round((1.+logicRNDfrnd(K_FLUCTUATION))*(1<<16));

	int xx,yy;
	for(i = curIY-radius, yy=0; i < curIY+radius; i++, yy++){
		int offY=vMap.offsetBuf(0, vMap.YCYCL(i));
		int offGY=vMap.offsetGBuf(0, vMap.YCYCL(i)>>kmGrid);
		for(j = curIX-radius, xx=0; j < curIX+radius; j++, xx++) {
			int offset=vMap.XCYCL(j) + offY;
			int V;
			V=vMap.getAlt(offset);
			//V+=FormWorms[i-y][j-x]>>8;

			//int dh=((bubleWormForm[yy][xx]>>7)*curKFluct)>>16;
			int dh=((formBitmap.pRaster[yy*formBitmap.sx+xx]>>8)*curKFluct)>>16; // 7
			int idx=(V-minV)>>1; if(idx>255)idx=255;
			//dh=((dh)>>7)*k_dh[idx]>>16; //8
			dh=((dh))*k_dh[idx]>>16; //8
			if(dh==0) continue;
			int offsetG=(vMap.XCYCL(j)>>kmGrid) + offGY;
			if( (vMap.gABuf[offsetG]&GRIDAT_BUILDING) != 0){
				vMap.gABuf[offsetG]|=GRIDAT_BASE_OF_BUILDING_CORRUPT;
				///continue;
			}

			V+=dh;
			if(V>MAX_VX_HEIGHT)V=MAX_VX_HEIGHT;

			vMap.putAlt(offset, V);
		}
	}
	//vMap.recalcArea2Grid(vMap.XCYCL(curX-radius-1), vMap.YCYCL(curY-radius-1), vMap.XCYCL(curX+radius+1), vMap.YCYCL(curY+radius+1) );
	//vMap.regRender(vMap.XCYCL(curX-radius-1), vMap.YCYCL(curY-radius-1), vMap.XCYCL(curX+radius+1), vMap.YCYCL(curY+radius+1), vrtMap::TypeCh_Height );

//

	const int MAX_QUANT_BUBBLE=10;//12//10//8
	const int MIN_QUANT_BUBBLE=5;//5;//8//5//3
	int num_bubble=XRnd(MAX_QUANT_BUBBLE-MIN_QUANT_BUBBLE)+MIN_QUANT_BUBBLE;
	for(i=0; i<num_bubble; i++){
		if(num_el_arr>=CTORPEDO_MAX_EL_ARR) break;
		float dirAngl=logicRNDfabsRnd(2*M_PI);
		float MAX_D=18.f;//50.f;
		float r=abs((sqrt(-log(0.020f+logicRNDfabsRnd(1.f-0.021f)))-1)*MAX_D);
		float addx=cos(dirAngl)*r;
		float addy=sin(dirAngl)*r;
		float MAX_ADDON_R=3.f;//12.f;
		int addon=XRnd( round(MAX_ADDON_R*(1.0-r/MAX_D)) );//abs((sqrt(-log(0.020+logicRNDfabsRnd(1.-0.021)))-1)*MAX_ADDON_R);//XRnd(round(MAX_ADDON_R));//XRnd( round(MAX_ADDON_R*(r/MAX_D)) );
		bool flag_occurrenceGeo=0;
		//if(r <= 0.5*MAX_D && XRnd(10)>0) flag_occurrenceGeo=1;
		if(insert2Arr(round(curX+addx),round(curY+addy),8+addon,8+addon, flag_occurrenceGeo)==0) break;
	}

	int k=0;
	float maxQuantTailBubble=3.f;
	const int SIZE_TAIL=15;//20;   //15;//20;//30
	float dQuantTailBubble=maxQuantTailBubble/(float)SIZE_TAIL;
	const float BEGIN_TAIL_R=18.f;//50.f;
	float tailR=BEGIN_TAIL_R;
	float dTailR=BEGIN_TAIL_R/(float)SIZE_TAIL;
	//const int TAIL_OFFSET=1*SPEED;
	const int TAIL_OFFSET=1*4;
	const float MAX_ADDON_R=8.f;
	float curAddonR=MAX_ADDON_R;
	float dAddonR=MAX_ADDON_R/(float)SIZE_TAIL;
	for(k=TAIL_OFFSET; k< SIZE_TAIL+TAIL_OFFSET; k+=3 ){
		if(step-k<0) break;
		//int curTX=round(curX- direction.x*SPEED*(float)k);
		//int curTY=round(curY- direction.y*SPEED*(float)k);
		int curTX=round(curX- (curPos.x-prevPos.x)*(float)k);
		int curTY=round(curY- (curPos.y-prevPos.y)*(float)k);
		num_bubble=round(maxQuantTailBubble);
		for(i=0; i<num_bubble; i++){
			if(num_el_arr>=CTORPEDO_MAX_EL_ARR) break;
			float dirAngl=logicRNDfabsRnd(2*M_PI);
			float MAX_D=tailR;
			float r=abs((sqrt(-log(0.020f+logicRNDfabsRnd(1.f-0.021f)))-1)*MAX_D);
			float addx=cos(dirAngl)*r;
			float addy=sin(dirAngl)*r;
			int addon=XRnd( round(MAX_ADDON_R*(1.0-r/MAX_D)) );//abs((sqrt(-log(0.020+logicRNDfabsRnd(1.-0.021)))-1)*curAddonR); //XRnd(round(MAX_ADDON_R));//XRnd( round(MAX_ADDON_R*(r/MAX_D)) );
			bool flag_occurrenceGeo=0;
			//if(r <= 0.5*BEGIN_TAIL_R && XRnd(10)>0) flag_occurrenceGeo=1;
			if(insert2Arr(round(curTX+addx), round(curTY+addy),8+addon, 8+addon)==0) break;
		}
		maxQuantTailBubble-=dQuantTailBubble;
		tailR-=dTailR;
		curAddonR-=dAddonR;
	}

	list<sToolzDate>::iterator p;
	for(p = toolzDateLst.begin(); p != toolzDateLst.end(); p++){
		if(XRnd(2)) toolzerChangeTerHeight.influenceDZ(p->x, p->y, p->r, 0, 0);
	};
	toolzDateLst.erase(toolzDateLst.begin(), toolzDateLst.end());

	step++;

	//char buf[20];
	//itoa(num_el_arr, buf, 10);
	//strcat(buf, "\n");
	//::OutputDebugString(buf);

	//return 1;
	return sRect(vMap.XCYCL(curX-radius-1), vMap.YCYCL(curY-radius-1), radius*2, radius*2);
}
void sTorpedo::bubbleQuant()
{
	int m;
	for(m=0; m<num_el_arr; m++){
		if(bubArr[m]){
			int bx=bubArr[m]->x;
			int by=bubArr[m]->y;
			int ex=bx+bubArr[m]->sx;
			int ey=by+bubArr[m]->sy;
			if(bubArr[m]->flag_sleep){
				int k;
				for(k=0; k<m; k++){
					if(bubArr[k]){
						if( ((bubArr[k]->x < bx && (bubArr[k]->x+bubArr[k]->sx) > bx ) ||
							(bubArr[k]->x > bx && (bubArr[k]->x) < ex )) && 
							((bubArr[k]->y < by && (bubArr[k]->y+bubArr[k]->sy) > by ) ||
							(bubArr[k]->y > by && (bubArr[k]->y) < ey )) ) break;
					}
				}
				if(k!=m) continue;
			}
			if(bubArr[m]->quant()==0 || XRnd(30)==1){
				toolzDateLst.push_back(sToolzDate(bubArr[m]->x+bubArr[m]->sx/2, bubArr[m]->y+bubArr[m]->sy/2, bubArr[m]->sx/2+bubArr[m]->sx/4));
				deleteEl(m);
			}
			vMap.recalcArea2Grid(bx, by, ex, ey);
			vMap.regRender(bx, by, ex, ey, vrtMap::TypeCh_Height);
		}
	}
}
