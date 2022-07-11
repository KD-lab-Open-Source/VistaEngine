#ifndef __TERTOOLS_H__
#define __TERTOOLS_H__

//	elementarTool::locp; //!!!Повторяемость

#include "..\Util\TypeLibrary.h"
#include "..\Util\DebugUtil.h"

#include "vbitmap.h"

#include "geo2.h"
#include "break.h"
#include "bitGen.h"
#include <map>
struct ColorQuantizer;

inline bool floatEqual(float a, float b)
{
	return fabs(a-b)<FLT_COMPARE_TOLERANCE;
}

class TerToolsDispatcher {
public:
	typedef int TerToolsID;
	static const TerToolsID NOT_TERTOOL_ID=0;
	struct CashDateBitmap8C{
		//cash data
		CashDateBitmap8C(){
			flag_nonUsedCashData=true;
			cashFileName.clear();
		}
		CashDateBitmap8C(const CashDateBitmap8C& donor){
			set(donor.kScale, donor.flag_alfaPresent, donor.cashFileName.c_str(), donor.sectionSize,
				donor.worldFileTime, donor.originalBitmapFileTime,
				donor.crcBitmap4Dam, donor.crcpBitmap4Geo, donor.crcAlfaLayer);
			flag_nonUsedCashData=true;
		}
		void set(float _kScale, bool _flag_alfaPresent, const char* _cashFileName, int _sectionSize, 
			FILETIME _worldFileTime, FILETIME _originalBitmapFileTime,
			unsigned int _crcBitmap4Dam, unsigned int _crcpBitmap4Geo, unsigned int _crcAlfaLayer){
			flag_nonUsedCashData=false;
			kScale=_kScale;
			flag_alfaPresent=_flag_alfaPresent; cashFileName=_cashFileName; sectionSize=_sectionSize;
			worldFileTime=_worldFileTime; originalBitmapFileTime=_originalBitmapFileTime;
			crcBitmap4Dam=_crcBitmap4Dam; crcpBitmap4Geo=_crcpBitmap4Geo; crcAlfaLayer=_crcAlfaLayer;
		}
		void serialize(Archive& ar);
		bool flag_nonUsedCashData;
		bool flag_alfaPresent;
		float kScale;
		string cashFileName;
		int sectionSize;
		FILETIME worldFileTime;
		FILETIME originalBitmapFileTime;
		unsigned int crcBitmap4Dam;
		unsigned int crcpBitmap4Geo;
		unsigned int crcAlfaLayer;
	};
	struct Bitmap8C {
		//string srcFileName;
		unsigned char* pBitmap4Dam;
		unsigned char* pBitmap4Geo;
		unsigned char* pAlfaLayer;
		Vect2i size;
		Vect2i maxCoord;
		float kScale;
		unsigned int index;
		Bitmap8C(){
			pBitmap4Dam=0;
			pBitmap4Geo=0;
			pAlfaLayer=0;
			kScale=1.f;
		}
		~Bitmap8C(){
			release();
		}
		void release(){
			if(pBitmap4Dam){ delete pBitmap4Dam; pBitmap4Dam=0; }
			if(pBitmap4Geo){ delete pBitmap4Geo; pBitmap4Geo=0; }
			if(pAlfaLayer) { delete pAlfaLayer; pAlfaLayer=0; }
		}
		bool loadAndprepare4World(const char* name, const float terTextureKScale, int index, ColorQuantizer& cqGeo, ColorQuantizer& cqDam, CashDateBitmap8C* pCDB, const char* cachDir);
		bool loadAndPutBitmap2Quantizer(const char* name, const float terTextureKScale, ColorQuantizer& cq);//Для подмешивания цветов текстур тулзеров в карту
		bool validateAndLoadDataCash(const char* fname, CashDateBitmap8C* pCDB, const char* cachDir);
		void saveDataCash(const char* fname, CashDateBitmap8C* pCDB, const char* cachDir);
	};
	struct Bitmap8V {
		unsigned char* pBitmapV;
		Vect2i size;
		Vect2i maxCoord;
		float kScale;
		Bitmap8V(){
			pBitmapV=0;
			kScale=1.f;
		}
		~Bitmap8V(){
			release();
		}
		void release(){
			if(pBitmapV){ delete pBitmapV; pBitmapV=0; }
		}
		bool load(const char* fname, const float terTextureKScale);
	};
	//typedef map<string, Bitmap> Bitmap8CTable;
	typedef multimap<string, Bitmap8C> Bitmap8CTable;
	typedef multimap<string, Bitmap8V> Bitmap8VTable;

	typedef multimap<string, CashDateBitmap8C> CashDataTable;

	TerToolsDispatcher();
	~TerToolsDispatcher();
	void prepare4World();
	void loadCashData(const char* fname);
	void saveCashData(const char* fname);

	void serialize(Archive& ar);
	unsigned long damAlfaConversion[256][256];
	unsigned long geoAlfaConversion[256][256];

	Bitmap8C* getBitmap8C(const char* fname, float kScale){
		//Bitmap8CTable::iterator p=bitmap8CTable.find(name);
		//if(p!=bitmap8CTable.end())
		//	return &(p->second);
		//return 0;
		typedef Bitmap8CTable::iterator P;
		pair<P,P> r=bitmap8CTable.equal_range(fname);
		for(P p=r.first; p!=r.second; ++p){
			if(floatEqual(p->second.kScale,kScale)){
				return &(p->second);
			}
		}
		xassert("TerTool bitmap not found!");
		return 0;
	}

	Bitmap8V* getBitmap8V(const char* fname, float kScale){
		typedef Bitmap8VTable::iterator P;
		pair<P,P> r=bitmap8VTable.equal_range(fname);
		for(P p=r.first; p!=r.second; ++p){
			if(floatEqual(p->second.kScale,kScale)){
				return &(p->second);
			}
		}
		xassert(0&&"TerTool bitmap not found!");
		return 0;
	}

	CashDateBitmap8C* getCashData(const char* fname, float kScale);

	TerToolsID startTerTool(class TerToolBase* pTTB);
	void stopTerTool(TerToolsID ttid);
	void pauseTerTool(TerToolsID ttid, bool _pause);
	bool isTerToolFinished(TerToolsID ttid);
	void setPosition(TerToolsID ttid, const Se3f& pos);
	void quant();
	bool putAllColorTerTools2ColorQuantizer(ColorQuantizer& cq);
protected:
	CashDataTable cashDataTable;
	Bitmap8CTable bitmap8CTable;//bitmaps_;
	Bitmap8VTable bitmap8VTable;
	bool prepareTerTools4World(const TerToolBase* pTerTool, int indexIn, ColorQuantizer& cqGeo, ColorQuantizer& cqDam, const char* cachDir);
	bool putAllColorTerTool2ColorQuantizer(const TerToolBase* pTerTool, ColorQuantizer& cq);

	struct StartedTerToolElement{
		TerToolsID terToolID;
		class TerToolBase* pTerTool;
		bool flag_pause;
		//StartedTerToolElement(){
		//	terToolID=0;
		//	pTerTool=0;
		//	flag_pause=false;
		//}
		StartedTerToolElement(TerToolsID _ttID, class TerToolBase* _pTT){
			terToolID=_ttID;
			pTerTool=_pTT;
			flag_pause=false;
		}
	};
	//typedef map<TerToolsID, class TerToolBase*> StartedTerTools;
	//StartedTerTools startedTerTools;
	typedef list<StartedTerToolElement> StartedTerTools;
	list<StartedTerToolElement> startedTerTools;
	StartedTerTools::iterator find(TerToolsID ttID){
		list<StartedTerToolElement>::iterator p;
		for(p=startedTerTools.begin(); p!=startedTerTools.end(); ++p){
			if(p->terToolID==ttID)
				break;
		}
		return p;
	}
	static TerToolsID unengagedTerToolID; //инициализируется в cpp
	TerToolsID getUnengagedID() { return unengagedTerToolID++; }
};
extern TerToolsDispatcher terToolsDispatcher;


struct sVoxelBitmap {
	short sx, sy;
	short* pRaster;
	sVoxelBitmap(){
		sx=sy=0;
		pRaster=0;
	}
	sVoxelBitmap(const sVoxelBitmap& donor){
		sx=sy=0;
		pRaster=0;
		*this=donor;
	}
	void release(){
		sx=sy=0;
		if(pRaster) { delete [] pRaster; pRaster=0; }
	}
	short* create(short _sx, short _sy){
		if(sx!=_sx || sy!=_sy){
			release();
            sx=_sx; sy=_sy;
			pRaster=new short[sx*sy];
		}
		return pRaster;
	}
	~sVoxelBitmap(){
		release();
	}
	sVoxelBitmap& operator =(const sVoxelBitmap& donor){
		create(donor.sx, donor.sy);
		memcpy(pRaster, donor.pRaster, sx*sy*sizeof(pRaster[0]));
		return *this;
	}
	void put(short x, short y, short v){
		pRaster[x+y*sx]=v;
	}
};

enum eTerToolTerrainEffect {
	TTTE_PLOTTING_TEXTURE,
	TTTE_ALIGNMENT_DIG,
	TTTE_ALIGNMENT_PUT,
	TTTE_ALIGNMENT_SMOOTH,
	TTTE_DIG,
	TTTE_PUT,
	TTTE_SMOOTH
};
enum eTerToolSurfaceEffect{
	TTSE_NOT_CHANGE,
	TTSE_DRAW_DAM,
	TTSE_DRAW_GEO,
	TTSE_DRAW_CURRENT,
	TTSE_ROTATE_DRAW_CURRENT
};

enum eSetingAtrMetod{
	SIM_NotChange,
	SIM_SetInds,
	SIM_UnSetInds,
	SIM_SetGeo,
	SIM_SetDam
};

template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA=false> 
class elementarTool {
public:
	int hAppr;
	int textureShiftX;
	int textureShiftY;
	int textureMaskX;
	int textureMaskY;
	short textureSizeX;
	short textureSizeY;
	unsigned char* pRastrGeo;
	unsigned char* pRastrDam;
	unsigned char* pRastrAlpha;
	//for rotate texture
	float A11, A12, A21, A22;
	float X, Y;
	//float kX, kY, kZ;
	elementarTool(){
		//locp=0;// 4 influenceDZ !
		if(_TT_SUR_EFF_!=TTSE_NOT_CHANGE){
			textureShiftX=textureShiftY=0;
			textureMaskX=textureMaskY=0;
			pRastrAlpha=pRastrGeo=pRastrDam=0;
		}
	}
	~elementarTool(void){};

	void setHAppoximation(int hAppoximation){
		xassert(_TT_TER_EFF_==TTTE_ALIGNMENT_DIG || _TT_TER_EFF_==TTTE_ALIGNMENT_PUT || _TT_TER_EFF_==TTTE_ALIGNMENT_SMOOTH);
		hAppr=hAppoximation<<VX_FRACTION;
	}
	void setTexture(TerToolsDispatcher::Bitmap8C* pbmp, int centerX, int centerY){
		xassert(_TT_SUR_EFF_!=TTSE_NOT_CHANGE);
		pRastrGeo=pbmp->pBitmap4Geo;
		pRastrDam=pbmp->pBitmap4Dam;
		pRastrAlpha=pbmp->pAlfaLayer;
		//xassert() надо вставить на размер кратный ^2!
		textureSizeX=pbmp->size.x;
		textureSizeY=pbmp->size.y;
		textureMaskX=pbmp->size.x-1;
		textureMaskY=pbmp->size.y-1;
		textureShiftX=-(centerX - pbmp->size.x/2);
		textureShiftY=-(centerY - pbmp->size.y/2);
	}
	void setTexture(TerToolsDispatcher::Bitmap8C* pbmp, float centerX, float centerY, float alpha){
		xassert(_TT_SUR_EFF_==TTSE_ROTATE_DRAW_CURRENT);
		setTexture(pbmp, round(centerX), round(centerY));

		A11 = A22 = cosf(alpha);
		A21 = -(A12 = sinf(alpha));
		X = - A11*centerX - A12*centerY ;
		Y = - A21*centerX - A22*centerY ;
		//kX=1.f; kY=1.f; kZ=1.f;
	}

	void PutAltAndSur(int offB, short v, int x=0, int y=0, short alfa=0);
	int influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea);

	//int locp; //!!!Повторяемость
	int influenceDZ(int x, int y, int rad, short dh, int smMode, unsigned char picMode=0, bool flag_Render=true);
///////////////////////////////////////////////////
	inline int tVoxSet(short x, short y, short dV, short alfa=0);
	inline int voxSet(short x, short y, short v, short alfa=0);

};

/////////////////////////////////////////////////////////////////
/// Базовый класс всех тулзеров

class TerToolBase : public ShareHandleBase {
public:
	TerToolBase(){ 
		quantCnt=0; 
		position=Se3f(MatXf(Mat3f::ZERO, Vect3f::ZERO)); 
		terTextureKScale=1.f;
		setingAtrMetod_=SIM_NotChange;
		settingImpassabilityMetod_=vrtMap::SIMM_NotChangeImpassability;
		settingSurfaceKindMetod_=vrtMap::SurfaceKind_NoChange;
		detaledTextueType_=DTT_NotDetailedTexture;
		detaledTextueScale_=1.f;
		flag_stopOnInds=false;
	}
	void stopOnInds(bool flag){
		flag_stopOnInds=flag;
	}
	virtual ~TerToolBase(){};
	virtual void serialize(Archive& ar){}
	virtual bool isExternalStop() const { return false; }
	virtual bool isPermitChangeModel() const { return false; }
	virtual bool isExternalScalable() const { return true; }
	virtual void startNextAnimationChain() { return; }

	virtual bool pQuant()=0 ;
	virtual void absoluteQuant(){ return; }

	virtual TerToolBase* clone()const=0;
	virtual void setPosition(const Se3f& _pos, bool flag_begPos=false) { 
		position=_pos;
		//position=Vect2i(pos_.trans().x, pos_.trans().y); 
	}
	virtual const Se3f& getPosition(void) { return position; }
	virtual void setScale(float scaleFactor) {};
	//const string& getTexture() const { return terTexture;}
	//const float getTextureKScale() const { return terTextureKScale; }
	struct ResourceDescription {
		enum eResourceDescriptionType{
			RDT_Bitmap8Color4Surface,
			RDT_Bitmap8Voxel
		};
		eResourceDescriptionType resourceDescriptionType;
		string fileName;
		float kScale;
		ResourceDescription(eResourceDescriptionType _type, const string& _fname, float _kScale){
			resourceDescriptionType=_type;
			fileName=_fname;
			kScale=_kScale;
		}
	};
	virtual void getResourceDescription(list<ResourceDescription>& resdscrlist) const{
		resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Color4Surface, terTexture, terTextureKScale));
	}
	enum eDetaledTextueType{
		DTT_NotDetailedTexture=-1,
		DTT_DetailedTexture0=0,
		DTT_DetailedTexture1,
		DTT_DetailedTexture2,
		DTT_DetailedTexture3,
		DTT_DetailedTexture4,
		DTT_DetailedTexture5,
		DTT_DetailedTexture6,
		DTT_DetailedTexture7
	};
protected:
	string terTexture;
	float terTextureKScale;
	//Vect2i position;
	Se3f position;
	unsigned int quantCnt;
	eSetingAtrMetod setingAtrMetod_;
	vrtMap::eSettingImpassabilityMetod settingImpassabilityMetod_;
	vrtMap::eSettingSurfaceKindMetod settingSurfaceKindMetod_;
	eDetaledTextueType detaledTextueType_;
	float detaledTextueScale_;
	bool flag_stopOnInds;
	bool drawRegionAndImpassability(int _x, int _y, int _r){
		vMap.setImpassabilityAndSurKind(_x, _y, _r, settingImpassabilityMetod_, settingSurfaceKindMetod_);
		if(detaledTextueType_!=DTT_NotDetailedTexture){
			extern void DrawOnRegion(int layer, const Vect2i& point, float radius);
			DrawOnRegion(detaledTextueType_+1, Vect2i(_x,_y), round(_r*detaledTextueScale_));
			return true;
		}
		return false;
	}
};

template <class cclass>
class CTCreator : public TerToolBase{
public:
	TerToolBase* clone(/*const cclass& donor*/)const{
		cclass* pc=new cclass;
		*pc=*(static_cast<const cclass*>(this));
		return pc;
	}
	virtual bool pQuant(){
		switch(setingAtrMetod_){
		case SIM_SetInds:
			return (static_cast<cclass*>(this))->quant<SIM_SetInds>();
		case SIM_UnSetInds:
			return (static_cast<cclass*>(this))->quant<SIM_UnSetInds>();
		case SIM_SetGeo:
			return (static_cast<cclass*>(this))->quant<SIM_SetGeo>();
		case SIM_SetDam:
			return (static_cast<cclass*>(this))->quant<SIM_SetDam>();
		case SIM_NotChange:
		default:
			return (static_cast<cclass*>(this))->quant<SIM_NotChange>();
		}
	}
};

//Конкретные тулзеры
class TerToolTrack1  : public CTCreator<TerToolTrack1> /*, public TerToolBase*/ {
public:
	TerToolTrack1() { r=1; curstep=0;}
	//virtual bool quant();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual bool isExternalStop() const { return true; }
	virtual void serialize(Archive& ar);
	//virtual void setPosition(const Se3f& _pos, bool flag_begPos) {
	//	//position=Vect2i(_pos.trans().x, _pos.trans().y); 
	//	position=_pos;
	//}
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
protected:
	int r;
	//Se3f pos;
	Se3f prevPos;
	float curstep;
};

class TerToolTextureBase {
public:
	//TerToolTextureBase();
	struct Vertex2i{
		int x,y;
		int u,v;
		Vertex2i(int _x, int _y, int _u, int _v){
			x=_x; y=_y; u=_u; v=_v;
		}
	};
	//inline void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8C* pBmp);
	//inline void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8V* pBmp);
	template <eSetingAtrMetod setingIndsMetod> void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8C* pBmp);
	template <eSetingAtrMetod setingIndsMetod> void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8V* pBmp);

	template<class Bitmap8, eSetingAtrMetod setingIndsMetod> void draw4SidePolygon(Vertex2i point[4], Bitmap8* pBmp, sBitMap8* pBackupBmp=0);
	enum eDrawMetod {
		DM_Cyrcle,
		DM_Bitmap,
		DM_ReliefBitmapPressIn,
		DM_ReliefBitmapSwellOut,
	};
	//for relief textue
protected:
	float A11, A12, A21, A22;
	float X, Y;
protected:
	int r;
	eDrawMetod drawMetod;
};

class TerToolTexture  : public CTCreator<TerToolTexture>, public TerToolTextureBase /*, public TerToolBase*/ {
public:
	TerToolTexture();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	virtual void serialize(Archive& ar);
	virtual void setPosition(const Se3f& _pos, bool flag_begPos) { 
		//position=Vect2i(pos.trans().x, pos.trans().y); 
		position=_pos;
	}
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
	virtual void getResourceDescription(list<ResourceDescription>& resdscrlist) const{
		switch(drawMetod){
		case DM_Cyrcle:
		case DM_Bitmap:
			resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Color4Surface, terTexture, terTextureKScale));
			break;
		case DM_ReliefBitmapPressIn:
		case DM_ReliefBitmapSwellOut:
			resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Voxel, terTexture, 1.f));
			break;
		}
	}
protected:
	int amountFrame;
	bool flag_restoreSurface;
	Vect2f prevSize05;
	sBitMap8 backupBmp;
};

class TerToolTextureTrack  : public CTCreator<TerToolTextureTrack> , public TerToolTextureBase/*, public TerToolBase*/ {
public:
	TerToolTextureTrack();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual bool isExternalStop() const { return true; }
	virtual void serialize(Archive& ar);
	virtual void setPosition(const Se3f& _pos, bool flag_begPos) { 
		//position=Vect2i(pos.trans().x, pos.trans().y); 
		position=_pos;
		if(flag_begPos)
			prevPos=_pos;
	}
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
	////struct Vertex2i{
	////	int x,y;
	////	int u,v;
	////	Vertex2i(int _x, int _y, int _u, int _v){
	////		x=_x; y=_y; u=_u; v=_v;
	////	}
	////};
	////inline void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8C* pBmp);
	////inline void drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8V* pBmp);

	////template<class Bitmap8> void draw4SidePolygon(Vertex2i point[4], Bitmap8* pBmp);
	////enum eDrawMetod{
	////	DM_Cyrcle,
	////	DM_Bitmap
	////};

	virtual void getResourceDescription(list<ResourceDescription>& resdscrlist) const{
		//resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Color4Surface, terTexture, terTextureKScale));
		//resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Voxel, reliefTexture, 1.f));
		switch(drawMetod){
		case DM_Cyrcle:
		case DM_Bitmap:
			resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Color4Surface, terTexture, terTextureKScale));
			break;
		case DM_ReliefBitmapPressIn:
		case DM_ReliefBitmapSwellOut:
			resdscrlist.push_back(ResourceDescription(ResourceDescription::RDT_Bitmap8Voxel, terTexture, 1.f));
			break;
		}
	}
	enum eMoveMetod {
		MM_Uninterrupted,
		MM_ChangePosition,
		MM_Stepwise
	};
	//for relief textue
protected:
	////float A11, A12, A21, A22;
	////float X, Y;
protected:
	////string reliefTexture;
	////int r;
	Se3f prevPos;
	float curstep;
	float textureOff;
	/////eDrawMetod drawMetod;
	//bool flag_uninterruptedTrack;
	eMoveMetod moveMetod;
	float step;
	float deltaStep;
};

class TerToolCrater1 : public CTCreator<TerToolCrater1> /*, public TerToolBase*/ {
public:
	TerToolCrater1(){ r=1; }
	//virtual bool quant();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual void serialize(Archive& ar);
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
protected:
	int r;
};

class TerToolSimpleCrater : public CTCreator<TerToolSimpleCrater> /*, public TerToolBase*/ {
public:
	TerToolSimpleCrater(){ r=1; dh=1; flag_waving=true;}
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	virtual void serialize(Archive& ar);
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
protected:
	int r;
	int dh;
	bool flag_waving;
};

class TerToolLeveler : public CTCreator<TerToolLeveler> /*, public TerToolBase*/ {
public:
	TerToolLeveler();
	//virtual bool quant();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual void serialize(Archive& ar);
	virtual bool isExternalScalable() const { return false; }
	virtual void setScale(float scaleFactor) {
		r=round((float)r*scaleFactor);
		if(r<=0) r=1;
	}
	enum eLevelerMetod {
		LevMetod_averageH,
		LevMetod_minH
	};
protected:
	int r;
	float kRoughness;
	int amountFrame;
	eLevelerMetod levelerMetod;
	//aux date
	int hApproximation;
	sTerrainBitmapBase upBitmap;
	sTerrainBitmapBase downBitmap;
};


class TerToolGeoWave : public CTCreator<TerToolGeoWave>/*, public TerToolBase*/, public sGeoWave {
public:
	virtual void serialize(Archive& ar);

	//virtual bool quant(){
	//	return sGeoWave::quant();
	//}
	template <eSetingAtrMetod setingIndsMetod> bool quant(){
		bool result=sGeoWave::quant();
		if(!result)
			drawRegionAndImpassability(round(position.trans().x), round(position.trans().y), sGeoWave::getRadius());
		return result;
	}
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual void setPosition(const Se3f& _pos, bool flag_begPos){
		position=_pos;
		sGeoWave::setPosition(Vect2i(_pos.trans().x, _pos.trans().y));
	}
	virtual void setScale(float scaleFactor) {
		sGeoWave::setRadius(round((float)sGeoWave::getRadius()*scaleFactor));
	}
};


class TerToolGeoBreak : public CTCreator<TerToolGeoBreak>/*, public TerToolBase*/, public GeoBreak {
public:
	virtual void serialize(Archive& ar);
	//virtual bool quant(){
	//	return GeoBreak::quant();
	//}
	template <eSetingAtrMetod setingIndsMetod> bool quant(){
		bool result=GeoBreak::quant();
		if(!result)
			drawRegionAndImpassability(round(position.trans().x), round(position.trans().y), GeoBreak::getRadius());
		return result;
	}
	//virtual TerToolBase* clone()const{ return cloneI(*this); }
	virtual void setPosition(const Se3f& _pos, bool flag_begPos){
		position=_pos;
		GeoBreak::setPosition(_pos);
	}
	virtual void setScale(float scaleFactor) {
		GeoBreak::setRadius(round((float)GeoBreak::getRadius()*scaleFactor));
	}
};

class TerToolTorpedo : public CTCreator<TerToolTorpedo>/*, public TerToolBase*/, public sTorpedo {
public:
	TerToolTorpedo();
	virtual bool isExternalStop() const { return true; }
	virtual void serialize(Archive& ar);

	template <eSetingAtrMetod setingIndsMetod> bool quant();

	virtual void setPosition(const Se3f& _pos, bool flag_begPos){
		position=_pos;
		if(flag_begPos){
			prevPos=_pos;
			prevPosInds=Vect2f(_pos.trans());
			//sTorpedo::setPosition(Vect2i(_pos.trans().x, _pos.trans().y));
		}
	}
	virtual void setScale(float scaleFactor) {
		//sTorpedo::setRadius(round((float)sTorpedo::getRadius()*scaleFactor));
	}
	virtual void absoluteQuant();
protected:
	int r;
	Se3f prevPos;
	float curstep;
	float step;
	float deltaStep;
	Vect2f prevPosInds;
};

class TerToolPutConstantModel : public CTCreator<TerToolPutConstantModel>/*, public TerToolBase*/ {
	sVoxelBitmap voxelBitmap;
public:
	TerToolPutConstantModel();
	virtual void serialize(Archive& ar);
	//virtual bool quant();
	virtual void startNextAnimationChain();
	template <eSetingAtrMetod setingIndsMetod> bool quant();
	virtual void setPosition(const Se3f& pos, bool flag_begPos);
	virtual void setScale(float _scaleFactor);
	enum eLayMetod{
		LM_AbsoluteHeight,
		LM_RelativeHeightPut,
		LM_RelativeHeightDig,
	};
protected:
	template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
		bool elemetarQuant(elementarTool<_TT_SETING_ATR_METOD_, _TT_TER_EFF_, _TT_SUR_EFF_, INTERNAL_ALPHA>& curTool);
	bool putModel2VBitmap(const Se3f& pos);
	string modelName;
	int amountFrame;
	int amountFrame2;
	int chainNum;
	//Se3f pos;
	short minimalZ;
	short relativeCenterZ;
	eLayMetod layMetod;
	float scaleFactor;
	bool independentTerTexture;
	int noiseAmp;
	Vect2f minCoord;
	bool flag_waitStartAnimationChain;
};

class TerToolLibElement : public StringTableBase { //: public ShareHandleBase
public:
	ShareHandle<TerToolBase> terToolNrml;
	ShareHandle<TerToolBase> terToolInds;
	bool interplayArr[TERRAIN_TYPES_NUMBER];
	void init(){
		for(int i=0; i<sizeof(interplayArr)/sizeof(interplayArr[0]); i++)
			interplayArr[i]=true;
	}
	void serialize(Archive& ar);
	bool isEmpty() const {
		return terToolNrml==0 && terToolInds==0;
	}

	TerToolLibElement(const char* name = "") : StringTableBase(name) {
		init();
	}
};
typedef StringTable<TerToolLibElement> TerToolsLibrary;
typedef StringTableReference<TerToolLibElement, false> TerToolReference;

/////////////////////////////////////////////////////////////////
class TerToolCtrl {
public:
	TerToolCtrl();
	~TerToolCtrl();
	void serialize(Archive& ar);
	bool isFinished() const {
		return terToolsDispatcher.isTerToolFinished(terToolNrmlID) && terToolsDispatcher.isTerToolFinished(terToolIndsID);
	}

	void start(const Se3f& pos, float _scaleFactor=1.0f);

	void stop();
	void setPosition(const Se3f& pos);
	bool isEmpty() const { 
		//return !terToolReference && !terToolReferenceInds;
		//if( !terToolReference ) return true;
		//return !terToolReference->terToolNrml && !terToolReference->terToolInds;
		if(!terToolReference) return true;
		return terToolReference->isEmpty();
	}

private:

	TerToolsDispatcher::TerToolsID terToolNrmlID;
	TerToolsDispatcher::TerToolsID terToolIndsID;

	string modelName;
	float scaleFactor;
	float currentScaleFactor;
	TerToolReference terToolReference;
};

#endif //__TERTOOLS_H__
