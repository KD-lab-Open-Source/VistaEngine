#ifndef __BREAK_H__
#define __BREAK_H__

#include "XMath\xmath.h"
#include "vmap.h"
//#include "Timers.h"

///////////////////////////////////////////////////////////////////////////////////////////////
//                                     ТРЕЩИНЫ(РАЗЛОМЫ)
///////////////////////////////////////////////////////////////////////////////////////////////

enum eReturnQuantResult{
	END_QUANT=0,
	HEAD_IN_FINAL_POINT=1,
	CONTINUE_QUANT=2
};

const float DENSITY_NOISE=6.f; //один излом на 4-е точки
const int LENGHT_TAIL=50; //Длинна хвоста(в сегментах)
const float MAX_WIDTH=20.f;
const float MAX_DEEP=20.f;

struct GeoBreakParam {
	GeoBreakParam(){
		density_noise=DENSITY_NOISE;
		lenght_tail=LENGHT_TAIL;
		max_width=MAX_WIDTH;
		max_deep=MAX_DEEP;
	}
	float max_width;
	float max_deep;
	float density_noise;
	int lenght_tail;
	
	void serialize(Archive& ar);
};

struct elementGeoBreak;
struct GeoBreak { //точечный разлом
	//Vect2i pos;
	Vect2f pos;
	float angle;
	int rad, minBegBreak, maxBegBreak;
	int maxGeneration;
	float maxLenghtElement;
	GeoBreakParam geoBreakParam;
	bool flag_beginInitialization;
	list<elementGeoBreak*> elGB;
	GeoBreak();
	//GeoBreak(Vect2f _pos, int _rad=100, int _begNumBreak=0){ //0-случайное кол-во
	//	pos=_pos;
	//	rad=_rad; maxBegBreak=_begNumBreak;
	//	flag_beginInitialization=false;
	//}
	void init();

	list<elementGeoBreak*>::iterator delEementGeoBreak(list<elementGeoBreak*>::iterator pp);
	bool quant();
	void setPosition(const Se3f& _pos);
	void setRadius(short _rad){
		rad=_rad;
	}
	int getRadius(){ return rad; }

	void serialize(Archive& ar);
};

#endif //__BREAK_H__
