#ifndef __BITGEN_H__
#define __BITGEN_H__

struct sTerrainBitmapBase;
enum eBitGenMetodID{
	BGMID_EXP,
	BGMID_PN,
	BGMID_MPD
};
struct sBitGenMetod {
	eBitGenMetodID bitGenMetodID;
	int UID;
	int rndVal;
	short maxHeight;
	char expPower;
	sBitGenMetod(){};
	sBitGenMetod(eBitGenMetodID _bitGenMetodID, char _expPower, short _maxHeight, int _rndVal){
		bitGenMetodID=_bitGenMetodID;
		expPower=_expPower;
		maxHeight=_maxHeight;
		rndVal=_rndVal;
	}
	void calcUID(){
		UID=crc32((const unsigned char*)&bitGenMetodID, sizeof(bitGenMetodID), startCRC32);
		UID=crc32((const unsigned char*)&expPower, sizeof(expPower), UID);
		UID=crc32((const unsigned char*)&maxHeight, sizeof(maxHeight), UID);
		UID=crc32((const unsigned char*)&rndVal, sizeof(rndVal), UID);
	}
	virtual void generate(sTerrainBitmapBase& tb)=0;
	virtual void setMaxHeight(short _maxHeight)=0;
	//virtual bool operator ==(const sBitGenMetod& s)=0;
};

struct sBitGenMetodMPD : public sBitGenMetod
{
	float kRoughness;
	sBitGenMetodMPD(char _expPower, short _maxHeight, int _rndVal, float _kRoughness=0.9f):sBitGenMetod(BGMID_MPD,_expPower,_maxHeight,_rndVal){
		kRoughness=_kRoughness;
		calcUID();
	}
	void calcUID(){
		sBitGenMetod::calcUID();
		sBitGenMetod::UID=crc32((const unsigned char*)&kRoughness, sizeof(kRoughness), sBitGenMetod::UID);
	}
	sBitGenMetodMPD(const sBitGenMetodMPD& donor){ *this=donor; }
	virtual void generate(sTerrainBitmapBase& tb);
	virtual void setMaxHeight(short _maxHeight){
		maxHeight=_maxHeight;
		calcUID();
	}
};

struct sBitGenMetodPN : public sBitGenMetod
{
	sBitGenMetodPN(char _expPower, short _maxHeight, int _rndVal):sBitGenMetod(BGMID_PN, _expPower,_maxHeight,_rndVal){
		calcUID();
	}
	sBitGenMetodPN(const sBitGenMetodMPD& donor){ *this=donor; }
	virtual void generate(sTerrainBitmapBase& tb);
	virtual void setMaxHeight(short _maxHeight){
		maxHeight=_maxHeight;
		calcUID();
	}
};

struct sBitGenMetodExp : public sBitGenMetod
{
	sBitGenMetodExp(char _expPower, short _maxHeight, int _rndVal):sBitGenMetod(BGMID_EXP, _expPower,_maxHeight,_rndVal){
		calcUID();
	}
	sBitGenMetodExp(const sBitGenMetodExp& donor){ *this=donor; }
	void generate(sTerrainBitmapBase& tb);
/*	bool operator ==(const sBitGenMetod& s){
		const sBitGenMetodExp3* p;
		if(p=dynamic_cast<const sBitGenMetodExp3*> (&s)){
			if(maxHeight==p->maxHeight)
				return true;
		}
		return false;
	}*/
	virtual void setMaxHeight(short _maxHeight){
		maxHeight=_maxHeight;
		calcUID();
	}
};


struct sTerrainBitmapBase
{
	int sx,sy;
	short* pRaster;
	sTerrainBitmapBase(int _sx, int _sy){
		sx=_sx;
		sy=_sy;
		pRaster=new short[sx*sy];
	}
	sTerrainBitmapBase(){ sx=sy=0; pRaster=0; } 
	void create(int _sx, int _sy){
		xassert( sx==0 && sy==0 && pRaster==0 );
		sx=_sx;
		sy=_sy;
		pRaster=new short[sx*sy];
	}
	~sTerrainBitmapBase(){
		delete pRaster;
	}
	virtual int getGenMetodUID(){return 0;}//=0;
};

template<class GenerationMetod> struct sTerrainBitmap : public sTerrainBitmapBase
{
	GenerationMetod genMetod;
	sTerrainBitmap(int sx, int sy, GenerationMetod& _genMetod) 
		: sTerrainBitmapBase(sx,sy), genMetod(_genMetod) {
		genMetod.generate(*this);
	}
	int getGenMetodUID(){
		return genMetod.UID;
	}
};
/*
struct sTerrainBitmap{
	int sx,sy;
	unsigned short* pRaster;
	sBitGenMetod* pBitGenMetod;

	template<class GenerationMetod>
	sTerrainBitmap(int sx, int sy, GenerationMetod& genMetod){
		pRaster=new unsigned short[sx*sy];
		pBitGenMetod=new GenerationMetod(genMetod);
		*pBitGenMetod=genMetod;
		pBitGenMetod->generate(*this);
	}
	//sTerrainBitmap(int sx, int sy, sBitGenMetod* _pBitGenMetod);
	~sTerrainBitmap(){
		delete pRaster;
		delete pBitGenMetod;
	}
};
*/

/*
class BitGenDispatcher
{
public:
	template<class GenerationMetod>
	sTerrainBitmap* getTerrainBitmap(int sx, int sy, GenerationMetod& genMetod){
		list<sTerrainBitmap*>::iterator p;
		for(p=bitmaps_.begin(); p!=bitmaps_.end(); p++){
			if( *((*p)->pBitGenMetod)==genMetod && (*p)->sx==sx && (*p)->sy==sy ){
				return *p;
			}
		}
		//добавление нового битмапа
		if(bitmaps_.size() >= CASH_MAX){
			delete bitmaps_.front();
			bitmaps_.pop_front();
		}
		sTerrainBitmap* retpnt=new sTerrainBitmap(sx,sy,genMetod);
		bitmaps_.push_back(retpnt);
		return retpnt;
	}
	//sTerrainBitmap* getTerrainBitmap(int sx, int sy, sBitGenMetod* _pBitGenMetod);
	void release(){
		list<sTerrainBitmap*>::iterator p;
		for(p=bitmaps_.begin(); p!=bitmaps_.end(); p++){
			delete *p;
		}
		bitmaps_.clear();
	}
	~BitGenDispatcher(){
		release();
	}
private:
	enum { CASH_MAX = 5 };
	list<sTerrainBitmap*> bitmaps_;
};*/

class BitGenDispatcher
{
public:
	template<class GenerationMetod>
	sTerrainBitmapBase* getTerrainBitmap(int sx, int sy, GenerationMetod& genMetod){
		list<sTerrainBitmapBase*>::iterator p;
		for(p=bitmaps_.begin(); p!=bitmaps_.end(); p++){
			if( ( (*p)->getGenMetodUID())==genMetod.UID && (*p)->sx==sx && (*p)->sy==sy ){
				return *p;
			}
		}
		//добавление нового битмапа
		if(bitmaps_.size() >= CASH_MAX){
			delete bitmaps_.front();
			bitmaps_.pop_front();
		}
		sTerrainBitmapBase* retpnt=new sTerrainBitmap<GenerationMetod>(sx,sy,genMetod);
		bitmaps_.push_back(retpnt);
		return retpnt;
	}
	//sTerrainBitmap* getTerrainBitmap(int sx, int sy, sBitGenMetod* _pBitGenMetod);
	void release(){
		list<sTerrainBitmapBase*>::iterator p;
		for(p=bitmaps_.begin(); p!=bitmaps_.end(); p++){
			delete *p;
		}
		bitmaps_.clear();
	}
	~BitGenDispatcher(){
		release();
	}
private:
	enum { CASH_MAX = 5 };
	list<sTerrainBitmapBase*> bitmaps_;
};

extern BitGenDispatcher bitGenDispatcher;

#endif //__BITGEN_H__
