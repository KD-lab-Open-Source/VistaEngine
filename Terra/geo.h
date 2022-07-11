#ifndef __GEO_H__
#define __GEO_H__

#include "xmath.h"
//#include "break.h"
//#include "geo2.h"
#include "vbitmap.h"
#include "tools.h"
#include "..\util\Timers.h"


const char Path2TTerraResource[]="Scripts\\Resource\\Tools\\";

class CGeoInfluence {
	//unsigned char* inVxG;
	//unsigned char* inAtr;
	unsigned short* inVx;
	unsigned char* inSur;
	int* tmpltGeo;
	unsigned char* tmpltSur;
	int x,y,sx,sy;
	int h_begin;
	int time;
	int kmNewVx;
	void prepTmplt(void);
public:
	CGeoInfluence(int _x, int _y, int _sx, int _sy);
	~CGeoInfluence();
	int quant(int deltaTime=1);
};

/*
	static CGeoInfluence * gI;
	static char flagg=0;
	if(flagg==0){
		flagg=1;
		gI=new CGeoInfluence(mapCrd.x, mapCrd.y, 128, 128);
	}
	else{
		if(gI->quant()==0){
			delete gI;
			flagg=0;
		}
	}
*/

///////////////////////////////////////////////////////////////////////////////////////////////
//                                         ЧЕРВЯК
///////////////////////////////////////////////////////////////////////////////////////////////
class CWormOut {
	short x_,y_;
	short sx_,sy_;
	unsigned short* inVx;
	int *tmpltVx;
	int h_begin;
	int time;
	int kmNewVx;
public:
	CWormOut(int x, int y);
	bool quant();
};


extern void worms(int x, int y);
static const int WSXSY=64;
static const int WSXSY05=WSXSY>>1;
typedef int tFormWorm[WSXSY][WSXSY];
struct CWormFormLib {
	sVBitMap VBitMap1;
	~CWormFormLib(){
			VBitMap1.release();
	};
	CWormFormLib();
	void put2WF(float alpha, tFormWorm* FormWormsNew);
	void put2WF2(tFormWorm* FormWormsNew);
};

class CGeoWorm {
	static CWormFormLib wormFormLib;
	tFormWorm FormWorms1;
	tFormWorm FormWorms2;
	tFormWorm* FormWormsNew;
	tFormWorm* FormWormsOld;
	Vect3f cPosition;
	Vect3f tPosition;
	float cDirection;
	float cSpeed;
	int xOld, yOld;//данные для step-а
	int counter;
public:
	CGeoWorm(int xBeg, int yBeg, int xTrgt=0, int yTrgt=0);
	int quant();
	void step(int x, int y, float angle=0);
	static Vect2s getSize(void){
		return Vect2s(WSXSY,WSXSY);
	}


};


///////////////////////////////////////////////////////////////////////////////////////////////
//                                     ОПОЛЗЕНЬ
///////////////////////////////////////////////////////////////////////////////////////////////
struct srBmp{
	int stpX, stpY;
	int bsx, bsy;
	int begShiftX, begShiftY;
	float alpha;
	srBmp(float _alpha, int _sx, int _sy);;
	inline void setStr(int y){
		if(stpX==(1<<16)|| stpX==(-1<<16)){
			bsx=-stpX*round(y*cos(alpha)*sin(alpha));
			bsy=(stpX*y)-stpY*round(y*cos(alpha)*sin(alpha));
		}
		else {
			bsx=(-stpY*y)+stpX*round(y*sin(alpha)*cos(alpha));
			bsy=stpY*round(y*sin(alpha)*cos(alpha));
		}
	};
	inline int getX(int x){
			return begShiftX + (bsx+x*stpX);
	}
	inline int getY(int x){
			return begShiftY + (bsy+x*stpY);
	}

};

struct CLandslip {
	unsigned short* deltaVx;
	short * surVx;
	short * surVxB;
	int* tmpltGeo;
	int x,y,sx,sy;
	int xPrec, yPrec;//float 
	int hmin, hmax;
	int deltaHLS, hminLS;
	float direction;
	float speed;
	int cntQuant;
	srBmp rbmp;
	CLandslip(int _x, int _y, int _sx, int _sy, int _hmin, int _hmax, float _direction);
	~CLandslip(void);
	void prepTmplt(void);
	int quant(void);
	int geoQuant(void);
};


///////////////////////////////////////////////////////////////////////////////////////////
//                                Разломы
///////////////////////////////////////////////////////////////////////////////////////////
struct point4UP{
	short x;
	short y;
	short z;
};

const short SUPOLIGON_NON_POLIGON=-MAX_VX_HEIGHT;

struct sUPoligon {
	point4UP pArr[3];
	int leftBorder, rightBorder;
	int upBorder, downBorder;
	int sx, sy;
	int xc, yc;

	int hBegin;
	int vMin;
	int vMax;

	short * arrayVx;
	bool * arrayA; 
	sUPoligon(point4UP* _pArr);
	~sUPoligon(void);
	float curAngleG;
	bool quant(){
		curAngleG+=3;
		quant(curAngleG);
		if(curAngleG>35) return 0;
		else return 1;
	}
	void quant(float angle_grad);
	void smoothPoligon(Vect2f a, Vect2f b, Vect2f c);

};

struct AETRecord {
	int x;
	int dx;
	char idxFrom;
	char idxTo;
	char dir;
};

const int SUPOLIGONN_MAX_BASE_POINT=3;
const int SUPOLIGONN_MAX_ADDING_POINT=20;
const int SUPOLIGONN_ALL_POINT_IN_POLIGON=SUPOLIGONN_MAX_ADDING_POINT+2;//(2-это 3 минус 1)
struct sUPoligonN {
	point4UP basePntArr[SUPOLIGONN_MAX_BASE_POINT];
	point4UP addPntArr[SUPOLIGONN_MAX_ADDING_POINT];
	int nElAddPntArr;
	Vect3f allPntArr[SUPOLIGONN_ALL_POINT_IN_POLIGON];
	int nElAllPntArr;
	int critPntArr[SUPOLIGONN_ALL_POINT_IN_POLIGON];
	int nElCritPntArr;
	AETRecord AET[SUPOLIGONN_ALL_POINT_IN_POLIGON]; //после отладки поменять на list
	int nElAET;

	int leftBorder, rightBorder;
	int upBorder, downBorder;
	int sx, sy;
	int xc, yc;

	int hBegin;
	int vMin;
	int vMax;

	Vect2f oldPoint1, oldPoint2;

	short * arrayVx;
	bool * arrayA; 
	sUPoligonN(point4UP* _basePntArr, int maxAddPnt, point4UP* _addPntArr);
	~sUPoligonN(void);
	float curAngleG;
	bool quant(void);
	bool quant(float angle_grad);
	void smoothPoligon(Vect2f a, Vect2f b, Vect2f c);
	void buldCPArr(void);
	void clearAET() {
		nElAET=0;
	};
	void addAETRecord(char idxFrom, char idxTo, char dir);
	void delAETRecord(char idxAET);

};

struct sGeoFault {
	vector<Vect2f> pointArr;
	list<sUPoligonN*> poligonArr;
	int step;
	float fstep;
	sGeoFault(const Vect2f& begPoint, float begAngle, float aLenght);
	bool quant(void);
};


struct sUPoligonNSpec : public sUPoligonN {
	bool flagBegin;
	sUPoligonNSpec(point4UP* _basePntArr, int maxAddPnt, point4UP* _addPntArr)
		:sUPoligonN(_basePntArr, maxAddPnt, _addPntArr){
		flagBegin=0;
	};
};
struct sGeoSwelling {
	list<sUPoligonNSpec*> poligonArr;
	int step;
	sGeoSwelling(int x, int y);
	bool quant(void);
};

///////////////////////////////////////////////////////////////////////////////////////////////
//                   Появление моделей из земли
///////////////////////////////////////////////////////////////////////////////////////////////

struct faceM2VM{
	unsigned short v1, v2, v3;
};

struct vrtxM2VM{
	Vect3f xyz;
	//float x, y, z;
	float z1;
	//float nx, ny, nz;
	//float light;
	float sx, sy;
	int isx, isy, iz1;
};

#define MAX_NAME_MESH_M2VM_LENGHT 20
struct meshM2VM{
	unsigned short numVrtx;	/* Vertice count */
	vrtxM2VM * vrtx;			/* List of vertices */
	unsigned short numFace;	/* Face count */
	faceM2VM *face;			/* List of faces */
	Mat3f matrix;
	Vect3f position;
	Vect3f scaling;

	short sizeX05;
	short sizeY05;
	float kScale;

	char* fname3DS;
	const char* getFName3DS(void);

	///char name[MAX_NAME_MESH_M2VM_LENGHT];
	bool load(const char* fname);//, int numMesh);
	void rotateAndScaling(int XA, int YA, int ZA, float kX, float kY, float kZ);
	void rotateAndScaling(Vect3f& orientation, Vect3f& scaling);
	bool put2KF(int quality, short * KFArr, int sxKF, int syKF, bool flag_inverse=0, short addH=0);

	Vect2s calcSizeXY(void);

	void releaseBuf(void);

	meshM2VM(void){
		numFace=numVrtx=0;
		vrtx=0; face=0;
		position=Vect3f::ZERO;
		fname3DS=0;
	};
	~meshM2VM(void){
		releaseBuf();
	};

};


struct s_commandInformation{
	int numKF;
	int time;
	string names3DS;
	bool flag_inverse;
	short addH;
	int KF2Loop;
	int loopCount;
	short * RasterData;
	meshM2VM* pMesh;
	Vect3f orientation;
	Vect3f scale;
};


struct s_Mesh2VMapDate {
	short * KFAnimationArr;
	short * timeKFArr;
	unsigned short sizeX;
	unsigned short sizeY;
	int amountKF;

	list<s_commandInformation *> commandList_immediately;
	list<s_commandInformation *>::iterator CLIt;
	void resetCommandList();
	void command_setKeyFrame(const int numKF, const char* names3DS, const int time, const Vect3f& orientation, const Vect3f& scale, bool flag_inverse, short addH, int KF2Loop=0, int loopCount=0);
	void compilingCommandList();



	s_Mesh2VMapDate(){
		KFAnimationArr=0;
		amountKF=0;
		sizeX=sizeY=0;
		timeKFArr=0;
	};
	~s_Mesh2VMapDate(){
		if(KFAnimationArr) delete [] KFAnimationArr;
		if(timeKFArr) delete [] timeKFArr;
		resetCommandList();
	};

	void init(int _amountKF, short* _timeKFArr, int sx, int sy);
	void setKFDate(int numKF, const char* names3DS);

};

struct s_EarthUnit {
	s_Mesh2VMapDate* meshDate;

	short xL, yL;
	short nxL, nyL;
	bool flag_move;

	//short previsionKF;
	//short currentKF;

	list<s_commandInformation *>::iterator curKFIt;
	list<s_commandInformation *>::iterator nextKFIt;

	short currentBetweenFrame;

	int frame;

	int vMin;

	short* pCurKFRD;
	short* pNextKFRD;

	//short* array;
	short* inVx;
	short* tmpVx;
	unsigned char* mask;
	short* substare;
	int h_begin;

	bool flag_loop;

	s_EarthUnit(s_Mesh2VMapDate* _meshDate){
		meshDate=_meshDate;
		setLoop();
		mask=0;
		inVx=0;
		tmpVx=0;
		substare=0;
	};

	~s_EarthUnit(){
		if(mask) delete [] mask;
		if(inVx) delete [] inVx;
		if(tmpVx) delete [] tmpVx;
		if(substare) delete [] substare;
	}


	void setLoop(){
		flag_loop=1;
	}
	void resetLoop(){
		flag_loop=0;
	}

	void init(int xMap, int yMap);

	void relativeMove(int dx, int dy);
	void absolutMove(int x, int y);

	bool quant();
};

struct s_Mesh2VMapDispather {
	s_Mesh2VMapDispather(void);
	~s_Mesh2VMapDispather(void);
	vector<s_Mesh2VMapDate*> M2VMDateArr;
	vector<s_EarthUnit*> EUArr;

	//s_EarthUnit* getEarthUnit(const int ID);
	s_EarthUnit* getEarthUnit(s_Mesh2VMapDate * mmDate);

	void deleteEarthUnit(s_EarthUnit* eu, bool autoDeleteMVMDate=1);

	list<meshM2VM*> meshList;
	meshM2VM* getMeshFrom3DS(const char * name3DS);

};




extern s_Mesh2VMapDispather mesh2VMapDispather;

///////////////////////////////////////////////////////////////////////////////////////////////
//                   Муравей
///////////////////////////////////////////////////////////////////////////////////////////////

const int ANT_MAX_MESH2VMAPDATE=36;
class c3DSGeoAction
{
	s_EarthUnit* pEarthUnit;
protected:
	friend class c3DSGeoActionCreator;
	c3DSGeoAction(s_EarthUnit* pEarthUnit_):pEarthUnit(pEarthUnit_){ }
public:
	bool quant(void){
		if(pEarthUnit!=0){
			if(pEarthUnit->quant()==0){
				delEarthUnit();
				return 0;
			}
			else return 1;
		}
		return 0;
	}

	void delEarthUnit(void){
		if(pEarthUnit!=0){
			mesh2VMapDispather.deleteEarthUnit(pEarthUnit, false);
			pEarthUnit=0;
		}
	}

	~c3DSGeoAction(){
		delEarthUnit();
	};
};

struct s3DSGeoParameter
{
	struct CommandSet
	{
		int numKF;
		string names3DS;
		int time;
		bool flag_inverse;
		short addH;
		int KF2Loop;
		int loopCount;
		Vect3f scale;

		CommandSet()
		{
			numKF=0;
			time=0;
			flag_inverse=false;
			addH=0;
			KF2Loop=0;
			loopCount=0;
		}

		CommandSet(const int numKF_, const char* names3DS_, const int time_,
			 const Vect3f& scale_, bool flag_inverse_, short addH_, int KF2Loop_=0, int loopCount_=0)
		{
			numKF=numKF_;
			names3DS=names3DS_;
			time=time_;
			scale=scale_;
			flag_inverse=flag_inverse_;
			addH=addH_;
			KF2Loop=KF2Loop_;
			loopCount=loopCount_;
		}
	};

	vector<CommandSet> command;
};

class c3DSGeoActionCreator
{
public:
	c3DSGeoActionCreator();
	~c3DSGeoActionCreator();

	static c3DSGeoAction* Build(short xc, short yc, float orientation,
		s3DSGeoParameter* command
		//Для разных адресов command образуется разный кеш 
		//Предполагается что это будет лежать в prm
		);

	static c3DSGeoActionCreator* instance();
protected:
	c3DSGeoAction* BuildD(short xc, short yc, float orientation,
		s3DSGeoParameter* command
		);

	friend c3DSGeoAction;
	struct Cache
	{
		s3DSGeoParameter* command;
		vector<s_Mesh2VMapDate*> MMDateArr;
	};
	vector<Cache> array_cache;
};


const float ANT_RAD_IN_CELL_ARR=(2*M_PI)/ANT_MAX_MESH2VMAPDATE;
/*
struct s_AntBirthGeoAction {
	static s_Mesh2VMapDate* MMDateArr[ANT_MAX_MESH2VMAPDATE];
	s_EarthUnit* pEarthUnit;
	s_AntBirthGeoAction(short xc, short yc, float orientation);
	bool quant(void){
		if(pEarthUnit!=0){
			if(pEarthUnit->quant()==0){
				delEarthUnit();
				return 0;
			}
			else return 1;
		}
		return 0;
	};
	void delEarthUnit(void){
		if(pEarthUnit!=0){
			mesh2VMapDispather.deleteEarthUnit(pEarthUnit, false);
			pEarthUnit=0;
		}
	};
	~s_AntBirthGeoAction(){
		delEarthUnit();
	};
};

struct s_AntDeathGeoAction {
	static s_Mesh2VMapDate* MMDateArr[ANT_MAX_MESH2VMAPDATE];
	s_EarthUnit* pEarthUnit;
	s_AntDeathGeoAction(short xc, short yc, float orientation);
	void delEarthUnit(void){
		if(pEarthUnit!=0){
			mesh2VMapDispather.deleteEarthUnit(pEarthUnit, false);
			pEarthUnit=0;
		}
	};
	bool quant(void){
		if(pEarthUnit!=0){
			if(pEarthUnit->quant()==0){
				delEarthUnit();
				return 0;
			}
			else return 1;
		}
		return 0;
	};
	~s_AntDeathGeoAction(void){
		delEarthUnit();
	};
};
*/
///////////////////////////////////////////////////////////////////////////////////////////////
//                   Head
///////////////////////////////////////////////////////////////////////////////////////////////
const unsigned int HEAD_GEOACTION_DAMAGE_RADIUS=90;
struct s_HeadGeoAction {
	s_EarthUnit* pEarthUnit;
	s_HeadGeoAction();
	void init(int x, int y,float radius);
	void finit(void);
	void delEarthUnit(void){
		if(pEarthUnit!=0){
			mesh2VMapDispather.deleteEarthUnit(pEarthUnit, true);
			pEarthUnit=0;
		}
	};
	bool quant(int x, int y){
		if(pEarthUnit!=0){
			pEarthUnit->absolutMove(x,y);
			bool result=pEarthUnit->quant();
			damagingBuildingsTolzer(x, y, HEAD_GEOACTION_DAMAGE_RADIUS);
			//if(!result)delEarthUnit();
			return result;
		}
		return 0;
	};
	~s_HeadGeoAction(void){
		delEarthUnit();
	};
};

extern s_HeadGeoAction headGeoAction;
///////////////////////////////////////////////////////////////////////////////////////////////
//                   Оса
///////////////////////////////////////////////////////////////////////////////////////////////
/*
const int WASP_QUANT_FOR_GEOBREAK=25;
const int WASP_SHIFT_QUANT_FOR_GEOBREAK=5;
struct s_WaspBirthGeoAction {
	short x, y;
	short r;
	GeoBreak* pGeoBrk1;
	int quantCount;
	int radiusCount;
	s_WaspBirthGeoAction(short _xc, short _yc, short _r){
		if(_r > MAX_RADIUS_CIRCLEARR){
			xassert(0&&"exceeding max radius in WaspBirthGeoAction ");
			_r=MAX_RADIUS_CIRCLEARR;
		}
		x=_xc;
		y=_yc;
		r=_r;
		pGeoBrk1=new GeoBreak(Vect2i(x, y), 50, 6);
		quantCount=0;
		radiusCount=0;
	};
	~s_WaspBirthGeoAction(){
		if(pGeoBrk1) delete pGeoBrk1;
	}
	bool quant(void){
		quantCount++;

		if((radiusCount<r) && (pGeoBrk1)) {
			if(pGeoBrk1->quant()==0){
				delete pGeoBrk1;
				pGeoBrk1=0;
			}
		}
		if(quantCount>=WASP_SHIFT_QUANT_FOR_GEOBREAK){
			int i, j;
			const int BORDER=1;
			if(radiusCount < r){
				i = radiusCount;
				int max = maxRad[i];
				int* xx = xRad[i];
				int* yy = yRad[i];
				for(j = 0;j < max;j++) {
					int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
					vMap.PutAlt(offB, 1<<VX_FRACTION);
				}
				for(i = 0;i <= radiusCount; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					for(j = 0;j < max;j++) {
						int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
						vMap.SetSoot(offB);
					}
				}

			}
			else {
				if(radiusCount<r+BORDER){
					for(i = 0; i <= radiusCount; i++){
						i = radiusCount;
						int max = maxRad[i];
						int* xx = xRad[i];
						int* yy = yRad[i];
						for(j = 0;j < max;j++) {
							int offB=vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]));
							vMap.SetSoot(offB);
						}
					}
				}
				else {
					damagingBuildingsTolzer(x, y, r);
					return 0;
				}
			}
			radiusCount++;
			vMap.recalcArea2Grid( vMap.XCYCL(x-radiusCount), vMap.YCYCL(y-radiusCount), vMap.XCYCL(x+radiusCount), vMap.YCYCL(y+radiusCount) );
			vMap.regRender( vMap.XCYCL(x-radiusCount), vMap.YCYCL(y-radiusCount), vMap.XCYCL(x+radiusCount), vMap.YCYCL(y+radiusCount) );

		}
		return 1;
	
	};
};
*/
///////////////////////////////////////////////////////////////////////////////////////
//					Volcano
///////////////////////////////////////////////////////////////////////////////////////
struct sTVolcano {
	unsigned short* array;
	unsigned short* inVx;
	int h_begin;
	int x, y, sx, sy;
	int kmNewVx;
	int vMin;
	int iKScaling;
	int frame;
	int currentKP;
	int currentKPFrame;
	unsigned short* pCurKP;
	unsigned short* pPrevKP;
	unsigned int max_template_volcano_height;
	~sTVolcano();
	sTVolcano(int _x, int _y, int _sx, int _sy);
	int quant(void);
};


#endif //__GEO_H__
