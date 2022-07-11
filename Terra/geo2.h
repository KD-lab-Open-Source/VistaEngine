#ifndef __GEO2_H__
#define __GEO2_H__

//#include "..\util\Timers.h"

/////////////////////////////////////////////////////////////////////////////////////
//                  Wave
/////////////////////////////////////////////////////////////////////////////////////


struct sGeoWave {
	Vect2i pos;
	//short x,y;
	short begRadius;
	short maxRadius;
	short fadeRadius;
	short maxWaveAmplitude;
	short waveLenght;
	short waveSpeed;
	short begPhase;
	bool flag_beginInitialization;
	bool flag_Track;

	sGeoWave();
	//sGeoWave(short _x, short _y, short _maxRadius);
	bool quant(void);
	void setRadius(short maxRadius);
	int getRadius(){ return maxRadius; }
	void setPosition(const Vect2i& _pos){ pos=_pos; }

	void serialize(Archive& ar);
private:
	int step;
	float calcMaxAmpWave(short curBegRad);
	unsigned int prevRnd;
};

///////////////////////////////////////////////////////////////////////////////////////
//                              Пузырь и торпеда
///////////////////////////////////////////////////////////////////////////////////////
struct sTBubble {
	static int numPreImage;
	static unsigned short* preImage;
	short* array;
	short* inVx;
	short* tmpVx;
	unsigned char* mask;
	unsigned char* maxVx;
	short* substare;
	int h_begin;
	int x, y, sx, sy;
	int vMin;
	int frame;
	int previsionKP;
	int currentKP;
	int currentKPFrame;
	short* pCurKP;
	short* pPrevKP;
	bool flag_sleep;
	bool flag_occurrenceGeo;
	~sTBubble(){
		delete [] array;
		delete [] tmpVx;
		delete [] inVx;
		delete [] mask;
		delete [] maxVx;
		delete [] substare;
	};
	static void free(){
		if(preImage!=0) { delete [] preImage; preImage=0; }
	};
	sTBubble(int _x, int _y, int _sx, int _sy, bool _flag_occurrenceGeo=0);
	int quant(void);
};

const int CTORPEDO_MAX_EL_ARR=200;
struct sTorpedo {
	struct sToolzDate{ 
		sToolzDate(short _x, short _y, short _r) {
			x=_x; y=_y; r=_r;
		};
		short x, y, r;
	};
	int step;
	int radius;
	//float begAngle;
	Vect2f direction;
	int num_el_arr;
	int beginX, beginY;
	float curX, curY;
	sTBubble * bubArr[CTORPEDO_MAX_EL_ARR];

	list<sToolzDate> toolzDateLst;

	sTorpedo();
	sTorpedo(int x, int y, Vect2f& _direction);
	sTorpedo(int x, int y);
	void init(int x, int y, Vect2f& _direction);
	int findFreeEl(void){
		if(num_el_arr >= CTORPEDO_MAX_EL_ARR) return -1;
		for(int i=0; i<CTORPEDO_MAX_EL_ARR; i++) if(bubArr[i]==0) return i;
		return -1;
	};
	bool insert2Arr(int _x, int _y, int _sx, int _sy, bool _flag_occurrenceGeo=0);
	void deleteEl(int idx){
		delete bubArr[idx];
		bubArr[idx]=0;
		num_el_arr--;
	};
	struct sRect quant(Vect2f& prevPos, Vect2f& curPos);
	void bubbleQuant();
};

#endif //__GEO2_H__
