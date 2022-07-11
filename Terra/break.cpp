#include "stdafxTr.h"

#include "vmap.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"
#include "limits.h"
#include "auxInlineFuction.h"
#include "break.h"

////////////////////////////////////////////////////////////////////////////////////
// Служебные функции
//static float FRnd()
//{
//	return (float)XRnd(0xFFF)/(float)(0xFFF);//(float)(RAND_MAX+1);
//}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                     ТРЕЩИНЫ(РАЗЛОМЫ)
///////////////////////////////////////////////////////////////////////////////////////////////
const int DELTA_H_BREAK_GEOBREAK=20;
const int DELTA_H_STEP_BREAK_GEOBREAK=3;

struct elementGeoBreak {
	//int xbeg, ybeg, xend, yend;
	Vect2f begPoint;
	Vect2f endPoint;
	//int sx, sy;
	//int dx_step, dy_step;
	Vect2f dStep;
	float alpha;
	int generation;

	GeoBreakParam* pGeoBreakParam;

	static int unengagedID; //инициализируется в cpp
	int ownerID;
	int ID;
	//int bLong;
	//int time;
	//int lifetime;
	int numSections;
	int headSection;
	//int * xp;
	//int * yp;
	Vect3f* gp;
	Vect2f* gpn;
	float* gpw05;
	unsigned char beginHeight;
	unsigned char oldHeight;
	unsigned int quantCnt;
	elementGeoBreak(Vect2f& _begPoint, float _alpha, float _lenght, GeoBreakParam* _pGBP, int _generation, int _ownerID=0){//
		pGeoBreakParam=_pGBP;
		//_ownerID=0 означает что нет родителей
		generation=_generation;
		ID=unengagedID++;
		ownerID=_ownerID;
		if(_lenght<pGeoBreakParam->density_noise)_lenght=pGeoBreakParam->density_noise; //Минимальная длинна

		alpha=_alpha;
		begPoint=endPoint=_begPoint;

		numSections=round(_lenght/pGeoBreakParam->density_noise);//bLong/DENSITY_NOISE;
		//dx_step=round(_lenght*cosf(alpha))*(1<<16)/numSections;//sx*(1<<16)/numSections;
		//dy_step=round(_lenght*sinf(alpha))*(1<<16)/numSections;//sy*(1<<16)/numSections;
		dStep.x=_lenght*cosf(alpha)/numSections;
		dStep.y=_lenght*sinf(alpha)/numSections;
		//xp=new int [numSections+1];//1 это начальная точка
		//yp=new int [numSections+1];
		//xp[0]=xbeg;
		//yp[0]=ybeg;
		gp = new Vect3f[numSections+1];//1 это начальная точка
		gpn = new Vect2f[numSections+1];
		gpw05 = new float[numSections+1];
		gp[0]=Vect3f(begPoint, vMap.getApproxAlt(vMap.XCYCL(begPoint.xi()), vMap.YCYCL(begPoint.yi())));
		gpn[0]=Vect2f::ZERO;
		gpw05[0]=0;
		headSection=0;
		beginHeight=oldHeight=vMap.getApproxAlt(vMap.XCYCL(begPoint.xi()), vMap.YCYCL(begPoint.yi()));
		quantCnt=0;
	};
	~elementGeoBreak(){
		//delete [] yp;
		//delete [] xp;
		delete [] gp;
		delete [] gpn;
		delete [] gpw05;
	};

	//inline void geoLine(int xbeg, int ybeg, int sx, int sy, int width=1, int fl_new_width=0){

	//	int x_cur=xbeg<<16;
	//	int y_cur=ybeg<<16;

	//	if(abs(sx)>=abs(sy)){
	//		int stepdy=sy*(1<<16)/abs(sx);
	//		int stepdx;
	//		if(sx>0)stepdx=1; else stepdx=-1;
	//		stepdx=stepdx<<16;
	//		for(int i=0; i!=sx; i+=stepdx>>16, x_cur+=stepdx, y_cur+=stepdy){
	//			//vMap.voxSet(vMap.XCYCL(x_cur>>16), vMap.YCYCL(y_cur>>16), -6);
	//			vMap.digAndUnInds(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)), 6);

	//			//if(fl_new_width){
	//			//	for(int k=10+XRnd(30);k >0; k--){
	//			//		vMap.SetTer(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)-(k+1)), vMap.GetTer(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)-k)) );
	//			//		vMap.vxaBuf[vMap.offsetBuf(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)-(k+1)))]= vMap.vxaBuf[vMap.offsetBuf(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)-k))];

	//			//		vMap.SetTer(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)+(k+1)), vMap.GetTer(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)+k)) );
	//			//		vMap.vxaBuf[vMap.offsetBuf(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)+(k+1)))]= vMap.vxaBuf[vMap.offsetBuf(vMap.XCYCL(x_cur>>16), vMap.YCYCL((y_cur>>16)+k))];
	//			//	}
	//			//}

	//			for(int k=1; k<width; k++){
	//				vMap.digAndUnInds(vMap.XCYCL((x_cur + stepdy*k)>>16), vMap.YCYCL((y_cur-stepdx*k)>>16), 6);
	//				vMap.digAndUnInds(vMap.XCYCL((x_cur - stepdy*k)>>16), vMap.YCYCL((y_cur+stepdx*k)>>16), 6);
	//			}
	//		}
	//	}
	//	else { // sy_real> sx_real
	//		if(abs(sy)>abs(sx)){
	//			int stepdx=sx*(1<<16)/abs(sy);
	//			int stepdy;
	//			if(sy>0)stepdy=1; else stepdy=-1;
	//			stepdy=stepdy<<16;
	//			for(int i=0; i!=sy; i+=stepdy>>16, y_cur+=stepdy, x_cur+=stepdx){
	//				vMap.digAndUnInds(vMap.XCYCL(x_cur>>16), vMap.YCYCL(y_cur>>16), 6);
	//				for(int k=1; k<width; k++){
	//					//vMap.digAndUnInds(vMap.XCYCL((x_cur>>16)-k), vMap.YCYCL(y_cur>>16), 6);
	//					//vMap.digAndUnInds(vMap.XCYCL((x_cur>>16)+k), vMap.YCYCL(y_cur>>16), 6);
	//					vMap.digAndUnInds(vMap.XCYCL((x_cur + stepdy*k)>>16), vMap.YCYCL((y_cur-stepdx*k)>>16), 6);
	//					vMap.digAndUnIndsdig(vMap.XCYCL((x_cur - stepdy*k)>>16), vMap.YCYCL((y_cur+stepdx*k)>>16), 6);
	//				}
	//			}
	//		}
	//	}
	//	int xL, yT, xR, yD;
	//	if(sx>0) { xL=vMap.XCYCL(xbeg-width); xR=vMap.XCYCL(xbeg+sx+width); }
	//	else{ xL=vMap.XCYCL(xbeg+sx-width); xR=vMap.XCYCL(xbeg+width); }
	//	if(sy>0) { yT=vMap.YCYCL(ybeg-width); yD=vMap.YCYCL(ybeg+sy+width); }
	//	else { yT=vMap.YCYCL(ybeg+sy-width); yD=vMap.YCYCL(ybeg+width); }
	//	vMap.recalcArea2Grid(xL, yT, xR, yD );
	//	//extern int STATUS_DEBUG_RND;
	//	//STATUS_DEBUG_RND=1;
	//	vMap.regRender(xL, yT, xR, yD, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture);
	//	//STATUS_DEBUG_RND=0;
	//}

	Vect3f vbL, vbR, vbC;
	void calc3Point(int idx, Vect3f& vL, Vect3f& vC, Vect3f&vR, float subV){
		Vect2f n=gpn[idx]*gpw05[idx];
		vL.set(gp[idx].x+n.x, gp[idx].y+n.y, gp[idx].z);
		vC=gp[idx];
		vC.z=max(0.f, (vC.z-subV));
		vR.set(gp[idx].x-n.x, gp[idx].y-n.y, gp[idx].z);
	}
	void initGeoLine(int begIdx, float subV){
		calc3Point(begIdx, vbL, vbC, vbR, subV);
	}

	inline void geoLine(int endIdx, float subV){
		Vect3f veL, veC, veR;
		calc3Point(endIdx, veL, veC, veR, subV);

		putPolygon(&vbL,&vbC,&veL);
		putPolygon(&vbC,&veL,&veC);
		putPolygon(&vbR,&vbC,&veR);
		putPolygon(&vbC,&veC,&veR);

		int xL=min(round(vbC.x-gpw05[endIdx]), round(veC.x-gpw05[endIdx]));
		int xR=max(round(vbC.x+gpw05[endIdx]), round(veC.x+gpw05[endIdx]));
		int yT=min(round(vbC.y-gpw05[endIdx]), round(veC.y-gpw05[endIdx]));
		int yD=max(round(vbC.y+gpw05[endIdx]), round(veC.y+gpw05[endIdx]));

		vMap.recalcArea2Grid(xL, yT, xR, yD );
		vMap.regRender(xL, yT, xR, yD, vrtMap::TypeCh_Height);

		vbL=veL; vbR=veR; vbC=veC;
	}

	eReturnQuantResult quant(){
		headSection+=1;
		if(headSection <= numSections){//Идет рост трещины
			//int x_recomend=xbeg+(dx_step*headSection>>16);
			//int y_recomend=ybeg+(dy_step*headSection>>16);
			Vect2f pnt_recomended=begPoint + dStep*headSection;

			int curHeight=vMap.getApproxAlt(vMap.XCYCL(pnt_recomended.xi()), vMap.YCYCL(pnt_recomended.yi()));
			if( abs(curHeight-beginHeight)>DELTA_H_BREAK_GEOBREAK || abs(curHeight-oldHeight)>DELTA_H_STEP_BREAK_GEOBREAK
				/*|| (vMap.gABuf[offGrid] & GRIDAT_INDESTRUCTABILITY)*/ ) numSections=headSection;//return END_QUANT;
			oldHeight=curHeight;

			//int sx_real=x_recomend - xp[headSection-1];
			//int sy_real=y_recomend - yp[headSection-1];
			Vect2f s_real=pnt_recomended - gp[headSection-1];
			//int x_cur=xp[headSection-1]<<16;
			//int y_cur=yp[headSection-1]<<16;
			Vect2f cur=gp[headSection-1];
			//Noising
			//float ss=sqrtf(float(sy_real*sy_real+sx_real*sx_real));
			float ss=s_real.norm();
			//float cosAlpha=(float)sx_real/ss;
			//float sinAlpha=(float)sy_real/ss;
			float cosAlpha=s_real.x/ss;
			float sinAlpha=s_real.y/ss;
			float nx = logicRNDfabsRndInterval(pGeoBreakParam->density_noise/2.f, pGeoBreakParam->density_noise);//  (DENSITY_NOISE>>1) + XRnd(DENSITY_NOISE>>1); //DENSITY_NOISE;//
			float ny = logicRNDfabsRndInterval(-4.,4.);//XRnd(8)-4;
			//sx_real=round((float)nx*cosAlpha+ (float)ny*sinAlpha);
			//sy_real=round(-(-(float)nx*sinAlpha+ (float)ny*cosAlpha));
			s_real.x=nx*cosAlpha+ny*sinAlpha;
			s_real.y=-(-nx*sinAlpha+ ny*cosAlpha);
			//sx_real=sx_real+ XRnd(DENSITY_NOISE<<1)- (DENSITY_NOISE<<1);// if(sx_real<0) sx_real=0;
			//sy_real=sy_real+ XRnd(DENSITY_NOISE<<1)- (DENSITY_NOISE<<1);// if(sy_real<0) sy_real=0;
			//DrawLine
			//geoLine(xp[headSection-1], yp[headSection-1], sx_real, sy_real);

			//vMap.pointDamageBuildings(xp[headSection-1], yp[headSection-1]);
			//vMap.pointDamageBuildings(xp[headSection-1]+sx_real, yp[headSection-1]+sy_real);
			vMap.pointDamageBuildings(cur.xi(), cur.yi());
			cur+=s_real;
			vMap.pointDamageBuildings(cur.xi(), cur.yi());
			//x_cur+=sx_real<<16;
			//y_cur+=sy_real<<16;

			//xend = xp[headSection] = x_cur>>16;
			//yend = yp[headSection] = y_cur>>16;
			endPoint = cur;

			//gp[headSection]=Vect3f(x_cur/(1<<16),y_cur/(1<<16), vMap.getApproxAlt(vMap.XCYCL(xend), vMap.YCYCL(yend)));
			gp[headSection]=Vect3f(cur, (float)vMap.getApproxAlt(vMap.XCYCL(cur.xi()), vMap.YCYCL(cur.yi())));
			gpn[headSection]=Vect2f::ZERO;
			gpw05[headSection]=0;

			gpn[headSection-1]=Vect2f(gp[headSection])-Vect2f(gp[headSection-1]);
			gpn[headSection-1].normalize(1.f);
			swap(gpn[headSection-1].x, gpn[headSection-1].y);
			gpn[headSection-1].x=-gpn[headSection-1].x;
			//gpw05[headSection-1]=1.f;

			initGeoLine(headSection,0);
			//geoLine(headSection-1, 1*0.5f);
		}

		// расширение трещины
		float dwidth=pGeoBreakParam->max_width/(float)pGeoBreakParam->lenght_tail;
		float ddeep=pGeoBreakParam->max_deep/(float)pGeoBreakParam->lenght_tail;
		//int width=(1<<16);/// + ((quantCnt<<16)/12);
		float width=dwidth; //1.f;/// + ((quantCnt<<16)/12);
		float deep=ddeep;
		//int fl=0;
		int endSection=headSection-pGeoBreakParam->lenght_tail;//LENGHT_TAIL;
		if(endSection<0)endSection=0; 
		int begSection=headSection;
		int d;
		for(d=begSection; d>=endSection; d--){
			if(d==numSections){
				gpw05[d]=width/2.;
				initGeoLine(d, deep);
			}
			if(d < numSections){
				gpw05[d]=width/2.f;
				geoLine(d, deep);
			}
			//fl=0;
			//int oldwidth=width;
			//width+=(1<<11)+(1<<10);//(1<<12);
			//if( (width>>16) > (oldwidth>>16)) fl=1;
			width+=dwidth;//0.4f;
			deep+=ddeep;
		}
		quantCnt++;
		if(headSection == numSections) 
			return HEAD_IN_FINAL_POINT;
		if( d >= numSections) 
			return END_QUANT;
		return CONTINUE_QUANT;
	}
	void putPolygon(Vect3f* a, Vect3f* b, Vect3f* c);

};

int elementGeoBreak::unengagedID=1; //Инициализация уникального ида элемента трещины для поиска родителей при завершении

//////////////////////////////////////////////////////////////////////////////////////////

const double pi = 3.1415926535;
const int MAX_BEGIN_BREAKS=9;
//const int MAX_LENGHT_ELEMENTGEOBREAK=100;
//const int MAX_LENGHT_ELEMENTGEOBREAK=70;
const int MAX_BRANCHINGS_BREAKS=5;
const int MAX_ACTIVE_BREAKS=20;
const float CORNER_DISPERSION_BREAKS=(float)(pi+pi/2);
//const float CORNER_DISPERSION_BREAKS=(float)(pi);

const int MAX_BREAK_GENERATION=4;
const int DISPERSION_MAX_BREAK_GENERATION=1;//1+1;

GeoBreak::GeoBreak()
{
	minBegBreak=3;
	maxBegBreak=MAX_BEGIN_BREAKS;
	//pos=Vect2i(0,0);
	pos=Vect2f::ZERO;
	rad=30;
	maxGeneration=1;
	
	flag_beginInitialization=false;

}

void GeoBreak::setPosition(const Se3f& _pos)
{ 
	pos=Vect2f(_pos.trans()); 
	if(_pos.rot().z() > 0)
		angle=_pos.rot().angle(); 
	else {
		QuatF q = _pos.rot();
		q.negate();
		angle=q.angle(); 
	}
	angle+=M_PI_2;
	angle=fmodFast(angle, 2*M_PI);
	if(angle<0)angle+=2*M_PI;
}


void GeoBreak::init()
{
	maxLenghtElement=(float)rad/(float)maxGeneration;
	elGB.erase(elGB.begin(), elGB.end());//очистка списка элементов трещин
	int begBreak;
	if(maxBegBreak==0) 
		begBreak= logicRNDinterval(3, MAX_BEGIN_BREAKS);//Диапазон от 3 до MAX_BEGIN_BREAKS
	if(maxBegBreak==1 || maxBegBreak==2){
		float lenght= logicRNDfabsRndInterval(0.75f*maxLenghtElement, 1.25f*maxLenghtElement);
		const int generation=0;
		elementGeoBreak* el;
		el=new elementGeoBreak(pos, angle, lenght, &geoBreakParam, generation);
		elGB.push_back(el);
		if(maxBegBreak==2){
			el=new elementGeoBreak(pos, angle+M_PI, lenght, &geoBreakParam, generation);
			elGB.push_back(el);
		}
	}
	else {
		begBreak = minBegBreak != maxBegBreak ? logicRNDinterval(minBegBreak, maxBegBreak) : minBegBreak;
		float range_corner=2*pi/((float)begBreak);
		float begin_corner=0;
		for(int i=0; i<begBreak; i++){
			float alpha1=begin_corner+(logicRNDfabsRnd(1.f)*range_corner);
			for(;alpha1>2*pi; alpha1-=2*pi);
			//float lenght=MAX_LENGHT_ELEMENTGEOBREAK+logicRND(MAX_LENGHT_ELEMENTGEOBREAK);
			float lenght= logicRNDfabsRndInterval(0.75f*maxLenghtElement, 1.25f*maxLenghtElement);
			const int generation=0;
			elementGeoBreak* el=new elementGeoBreak(pos, alpha1, lenght, &geoBreakParam, generation);
			elGB.push_back(el);
			begin_corner+=range_corner;
		}
	}
	flag_beginInitialization=true;
}


list<elementGeoBreak*>::iterator GeoBreak::delEementGeoBreak(list<elementGeoBreak*>::iterator pp)
{
	int _ownID=(*pp)->ownerID;
	if(_ownID!=0){
		list<elementGeoBreak*>::iterator pp2;
		int counterChildren=0;
		for(pp2 = elGB.begin(); pp2 != elGB.end(); pp2++) if((*pp2)->ownerID==_ownID)counterChildren++;
		if(counterChildren==1){
			for(pp2 = elGB.begin(); pp2 != elGB.end(); ){
				if((*pp2)->ID==_ownID) pp2=delEementGeoBreak(pp2);
				else pp2++;
			}
		}
	}
	delete (*pp);
	return(elGB.erase(pp));
};

bool GeoBreak::quant()
{

	if(!flag_beginInitialization){
		init();
		return true;
	}
	list<elementGeoBreak*>::iterator pp;
	for(pp = elGB.begin(); pp != elGB.end(); ){
		eReturnQuantResult result=(*pp)->quant();

		if(result==HEAD_IN_FINAL_POINT) {
			//int branhings=1+XRnd(MAX_BRANCHINGS_BREAKS);
			//for(int i=0; i<branhings; i++)
			//выбор количества разломо(такой способ используется для того что-бы MAX_BRANCHINGS_BREAKS выбор был наименее вероятен)
			const int generation=(*pp)->generation + 1;
			//float dx2=(*pp)->xend-(*pp)->xbeg;
			//dx2=dx2*dx2;
			//float dy2=(*pp)->yend-(*pp)->ybeg;
			//dy2=dy2*dy2;
			Vect2f delta=(*pp)->endPoint-(*pp)->begPoint;
			int branchings;
			for(branchings=1; branchings<MAX_BRANCHINGS_BREAKS; branchings++) if(logicRND(2)==0) break;
			if(branchings==0 || (rad*rad < delta.norm2()/*dx2+dy2*/ || elGB.size() > MAX_ACTIVE_BREAKS 
				|| generation >= maxGeneration -logicRND(DISPERSION_MAX_BREAK_GENERATION) ) ) {
				pp=delEementGeoBreak(pp);
				continue;
			}
			float range_corner=CORNER_DISPERSION_BREAKS/((float)branchings);
			float begin_corner=(*pp)->alpha-(CORNER_DISPERSION_BREAKS/2);
			for(int i=0; i<branchings; i++){
				//float alpha1=begin_corner+(FRnd()*range_corner);
				float rnd=logicRNDfabsRnd(1.f);
				rnd=rnd*rnd;//^2
				float znak=(float)(-1*(int)logicRND(2));
				float alpha1=begin_corner+range_corner*0.5f+(znak*rnd*range_corner*0.5);
				//for(;alpha1>2*pi; alpha1-=2*pi);
				if(alpha1>2*pi){ 
					alpha1-=2*pi;
					if(alpha1>2*pi) 
						alpha1-=2*pi;
				}
				//float lenght=MAX_LENGHT_ELEMENTGEOBREAK+logicRND(MAX_LENGHT_ELEMENTGEOBREAK);
				float lenght= logicRNDfabsRndInterval(0.75f*maxLenghtElement, 1.25f*maxLenghtElement);
				elementGeoBreak* el=new elementGeoBreak((*pp)->endPoint, alpha1, lenght, &geoBreakParam, generation, (*pp)->ID );
				elGB.push_back(el);
				begin_corner+=range_corner;
			}
			//if(i==0){
				//delete (*pp);
				//pp=elGB.erase(pp);
			//	pp=delEementGeoBreak(pp);
			//}
		}
		else if(result==END_QUANT) { //Удаление элемента трещины
				//delete (*pp);
				//pp=elGB.erase(pp);
				pp=delEementGeoBreak(pp);
				continue;
		}
		pp++; 
	}
	if(elGB.size()==0){
		return false;
	}
	return true;//active;
}

void GeoBreakParam::serialize(Archive& ar)
{
	ar.serialize(max_width, "max_width", "Максимальная ширина");
	ar.serialize(max_deep, "max_deep", "Максимальная глубина");
	ar.serialize(lenght_tail, "lenght_tail", "Время расширения(кванты)");
	ar.serialize(density_noise, "density_noise", "Крупнота");
	if(density_noise < 1.f) density_noise=1.f;
}

void GeoBreak::serialize(Archive& ar)
{
	ar.serialize(pos, "pos", "Позиция");
	ar.serialize(rad, "rad", "Радиус");
	ar.serialize(minBegBreak, "minBegBreak", "Min начальное количество");
	ar.serialize(maxBegBreak, "maxBegBreak", "Max начальное количество");
	ar.serialize(maxGeneration, "maxGeneration", "Ветвистось");
	geoBreakParam.serialize(ar);
	
	if(ar.isInput()){
		minBegBreak = max(minBegBreak, 1);
		maxBegBreak = max(maxBegBreak, minBegBreak);
		maxGeneration = clamp(maxGeneration, 1, 20);
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// New break

void elementGeoBreak::putPolygon(Vect3f* a, Vect3f* b, Vect3f* c)
{
	if(a->y > b->y) swap(a, b);
	if(a->y > c->y) swap(a, c);
	if(b->y > c->y) swap(b, c);

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
//#ifdef SUBPIXEL
	tmp = ceilf(a->y) - a->y;
	x_start += dx_start * tmp;
	z1_start += dz1_start * tmp;
//#endif

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
//#ifdef SUBPIXEL
	x_end += dx_end * tmp;
	z1_end += dz1_end * tmp;
//#endif

////////////////////////////////

	// построчная отрисовка грани
	for (current_sy = ceilf(a->y); current_sy <= floorf(c->y); current_sy++) { //current_sy < ceilf(c->y)
		if((current_sy) >= vMap.V_SIZE) break;
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
//#ifdef SUBPIXEL
			tmp = ceilf(b->y) - b->y;
			x_end += dx_end * tmp;
			z1_end += dz1_end * tmp;
//#endif
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

		// текстурируем строку
		current_sx = round(ceilf(x));

		if((current_sy) >= 0 ) if (length) {
//	#ifdef SUBTEXEL
		    tmp = ceilf(x) - x;
			z1 += dz1* tmp;
//	#endif
			while (length--) {
			// используем z-буфер для определения видимости текущей точки
				if( (current_sx< vMap.H_SIZE) && (current_sx >= 0)) {
					register int bufoff=vMap.offsetBuf(current_sx, current_sy);
					if(vMap.getAlt(bufoff) > round(z1*(1<<VX_FRACTION)))
						vMap.putAlt(bufoff, round(z1*(1<<VX_FRACTION)));
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
}
