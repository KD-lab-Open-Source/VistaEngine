#ifndef __PROCEDURMAP_H__
#define __PROCEDURMAP_H__

#include "Handle.h"
#include "bitGen.h"

const float MAX_MEGA_PROCEDUR_MAP_OPERATION=1000.f;
class Archive;

enum PMOperationID { // не забывайте регистрировать!
	PMO_ID_NONE,
	PMO_ID_TOOLZER,
	PMO_ID_SQUARE_TOOLZER,
	PMO_ID_GEO,
	PMO_ID_BLUR,
	PMO_ID_COLORPIC,
	PMO_ID_BITGEN,
	PMO_ID_ROAD
};

//template<class base4Wrap>
//class WrapPMO : public base4Wrap {
//public:
//	WrapPMO() { set(); }
//	WrapPMO(int r) { set(r); }
//	WrapPMO(int x,int y,int rad,int smth,int dh,int smode,int eql, int _filterHeigh) { set(x,y,rad,smth,dh,smode,eql,_filterHeigh); }
//	WrapPMO(int _x, int _y, int _rad, float _intensity){ set(_x,_y,_rad,_intensity); }
//	WrapPMO(int _x, int _y, int _sx, int _sy, int _MAX_GG_ALT, int _kPowCellSize, int _kPowShiftCS4RG, int _GeonetMESH, int _NOISELEVEL_GG, int _BorderForm, bool _inverse){
//		set(_x, _y, _sx, _sy, _MAX_GG_ALT, _kPowCellSize, _kPowShiftCS4RG, _GeonetMESH, _NOISELEVEL_GG, _BorderForm, _inverse);
//	}
//	WrapPMO(int _x, int _y, int _rad, unsigned char _cntrAlpha, int _bitmapIDX) { set(_x, _y, _rad, _cntrAlpha, _bitmapIDX); }
//	//WrapPMO(int _x, int _y, int _rad, int _maxDHeigh, int _filterHeigh) { set(_x, _y, _rad, _maxDHeigh, _filterHeigh); }
//	template<class GenerationMetod>
//		WrapPMO(int _x, int _y, int _rad, GenerationMetod& genMetod, int _filterHeigh){set(_x,_y,_rad,genMetod,_filterHeigh);}
//
//};

struct sBasePMOperation : public ShareHandleBase{
	PMOperationID pmoID;
	//sBasePMOperation(){pmoID=PMO_ID_NONE;}
	void set(){
		pmoID=PMO_ID_NONE;
	}
	virtual void actOn()=0;
	virtual void serialize(Archive& ar)=0;
};

struct sToolzerPMO : public sBasePMOperation {
	short x, y, rad;
	char smth;
	short dh;
	char smode;
	short eql;
	//short filterHeigh;
	short minFH, maxFH;
	int rndVal;

	void setRndVal(int _rndVal){
		rndVal=_rndVal;
	}
	int getRndVal(){
		return rndVal;
	}
	sToolzerPMO(){ pmoID=PMO_ID_TOOLZER; }
	sToolzerPMO(int _x,int _y,int _rad,int _smth,int _dh,int _smode,int _eql, short _minFH, short _maxFH, int _rndVal=0){
		pmoID=PMO_ID_TOOLZER;
		x=_x; y=_y; rad=_rad;
		smth=_smth;
		dh=_dh;
		smode=_smode;
		eql=_eql;
		//filterHeigh=_filterHeigh;
		minFH=_minFH;
		maxFH=_maxFH;
		rndVal=_rndVal;
	}

	void serialize(Archive& ar);
	virtual void actOn();
};

struct sSquareToolzerPMO : public sToolzerPMO {
	sSquareToolzerPMO(){ pmoID=PMO_ID_SQUARE_TOOLZER; }
	sSquareToolzerPMO(int _x,int _y,int _rad,int _smth,int _dh,int _smode,int _eql, int _filterHeigh, int _rndVal=0)
		:sToolzerPMO(_x,_y,_rad,_smth,_dh,_smode,_eql,_filterHeigh,_rndVal){
		pmoID=PMO_ID_SQUARE_TOOLZER;
	}
	virtual void actOn();
};

struct sGeoPMO : public sBasePMOperation {
	short x, y, sx, sy;
    short maxGGAlt;
	char powerCellSize, powerShift;
	short geoNetMesh;
	short noiseLevel;
	char borderForm;
	char inverse;
	sGeoPMO() { pmoID=PMO_ID_GEO; }
	sGeoPMO(int _x, int _y, int _sx, int _sy, int _MAX_GG_ALT, int _kPowCellSize, int _kPowShiftCS4RG, int _GeonetMESH, int _NOISELEVEL_GG, int _BorderForm, bool _inverse){
		pmoID=PMO_ID_GEO;
		x=_x; y=_y; sx=_sx; sy=_sy;
		maxGGAlt=_MAX_GG_ALT;
		powerCellSize=_kPowCellSize;
		powerShift=_kPowShiftCS4RG;
		geoNetMesh=_GeonetMESH;
		noiseLevel=_NOISELEVEL_GG;
		borderForm=_BorderForm;
		inverse=_inverse;
	}

	void serialize(Archive& ar);
	virtual void actOn();
};

struct sBlurPMO : public sBasePMOperation {
	short x, y, rad;
	double intensity;

	sBlurPMO() { pmoID=PMO_ID_BLUR; }
	sBlurPMO(int _x, int _y, int _rad, float _intensity){
		pmoID=PMO_ID_BLUR;
		x=_x;
		y=_y;
		rad=_rad;
		intensity=_intensity;
	}

	void serialize(Archive& ar);
	virtual void actOn();
};

struct sColorPicPMO : public sBasePMOperation {
	short x, y, rad;
	float cntrAlpha;
	int bitmapIDX;
	short minFH, maxFH;

	sColorPicPMO(); // in cpp
	sColorPicPMO(int _x, int _y, int _rad, unsigned char _cntrAlpha, int _bitmapIDX, short _minFH, short _maxFH){
		pmoID=PMO_ID_COLORPIC;
		x=_x;
		y=_y;
		rad=_rad;
		cntrAlpha=_cntrAlpha;
		bitmapIDX=_bitmapIDX;
		minFH=_minFH;
		maxFH=_maxFH;
	}
	void serialize(Archive& ar);
	virtual void actOn();
};

struct sBitGenPMO : public sBasePMOperation {
	short x, y, rad;
	//short filterHeigh;
	short minFH, maxFH;

	eBitGenMetodID bitGenMetodID;
	int rndVal;
	short maxHeight;
	char expPower;
	float kRoughness;

	sBitGenPMO() { pmoID=PMO_ID_BITGEN; minFH=0; maxFH=0; }
	template<class GenerationMetod>
	sBitGenPMO(int _x, int _y, int _rad, GenerationMetod& genMetod, short _minFH, short _maxFH){
		pmoID=PMO_ID_BITGEN;
		x=_x; y=_y;
		rad=_rad;
		bitGenMetodID=genMetod.bitGenMetodID;
		rndVal=genMetod.rndVal;
		maxHeight=genMetod.maxHeight;
		minFH=_minFH;
		maxFH=_maxFH;
		expPower=genMetod.expPower;
		//filterHeigh=_filterHeigh;
		minFH=_minFH;
		maxFH=_maxFH;
		kRoughness=0;
	}
	sBitGenPMO(int _x, int _y, int _rad, sBitGenMetodMPD& genMetod, short _minFH, short _maxFH){
		sBitGenMetod& gm=(sBitGenMetod&) genMetod;
		//set(_x,_y,_rad, gm, _filterHeigh);
		pmoID=PMO_ID_BITGEN;
		x=_x; y=_y;
		rad=_rad;
		bitGenMetodID=genMetod.bitGenMetodID;
		rndVal=genMetod.rndVal;
		maxHeight=genMetod.maxHeight;
		expPower=genMetod.expPower;
		//filterHeigh=_filterHeigh;
		minFH=_minFH;
		maxFH=_maxFH;
		kRoughness=genMetod.kRoughness;
	}
	void serialize(Archive& ar);
	virtual void actOn();
};

struct sNodePrimaryData {
	Vect3f pos;
	float width;
	float angleOrWidtEdge;
	void serialize(Archive& ar);
};
struct sRoadPMO : public sBasePMOperation{
	vector<sNodePrimaryData> nodePrimaryDataArr;
	string bitmapFileName;
	string bitmapVolFileName;
	string edgeBitmapFName;
	string edgeVolBitmapFName;
	bool flag_onlyTextured;
	enum eTexturingMetod{
		TM_AlignWidthAndHeight=0,
		TM_AlignOnlyWidth=1,
		TM_1to1=2
	};
	enum ePutMetod{
		PM_ByMaxHeight,
		PM_ByRoadHeight
	};
	sRoadPMO(){ pmoID=PMO_ID_ROAD; flag_onlyTextured=false; texturingMetod=TM_AlignWidthAndHeight; putMetod=PM_ByMaxHeight;}
	sRoadPMO(const list<struct sNode>& nodeArr, const string& _bitmapFileName, const string& _bitmapVolFileName, const string& _edgeBitmapFName, const string& _edgeVolBitmapFName, bool _flag_onlyTextured,
		const eTexturingMetod _texturingMetod, const ePutMetod _putMetod);
	virtual void actOn();
	void serialize(Archive& ar);
	eTexturingMetod texturingMetod;
	ePutMetod putMetod;
};

/*
class ElementPMO {
public:
	union{
		sBasePMOperation	basePMO;
		sGeoPMO				geoPMO;
		sToolzerPMO			toolzerPMO;
		sSquareToolzerPMO	squareToolzerPMO;
		sBlurPMO			blurPMO;
		sColorPicPMO		colorPicPMO;
		sBitGenPMO			bitGenPMO;
		//sRoadPMO			roadPMO;
	};

	ElementPMO(){ basePMO.set(); }
	ElementPMO(sBasePMOperation& d){ basePMO=d; }
	ElementPMO(sToolzerPMO& d){ toolzerPMO=d; }
	ElementPMO(sSquareToolzerPMO& d){ squareToolzerPMO=d; }
	ElementPMO(sGeoPMO& d){ geoPMO=d; }
	ElementPMO(sBlurPMO& d){ blurPMO=d; }
	ElementPMO(sColorPicPMO& d){ colorPicPMO=d; }
	ElementPMO(sBitGenPMO& d){ bitGenPMO=d; }
	//ElementPMO(sRoadPMO& d){ roadPMO=d; }


	void serialize(Archive& ar);
};*/

//typedef vector<ElementPMO> ContainerPMO_Old;
typedef vector< ShareHandle<sBasePMOperation> > ContainerPMO;

#endif //__PROCEDURMAP_H__
