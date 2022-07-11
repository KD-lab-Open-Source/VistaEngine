#include "stdafxTr.h"

#include "Serialization\Serialization.h"
#include "quantizer.h"
#include "terTools.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\XPrmArchive.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\StringTable.h"

#include "scalingEngine.h"
#include "tgai.h"

#include "float.h" //временно


//#define TERTOOL_LOG
///////////////////////////////////////

TerToolsDispatcher terToolsDispatcher;
static const char* terToolsDispatcherCfgFName="!test!.scr";
static const char* terToolsDispatcherCfgSection="Terraintools";
TerToolsID TerToolsDispatcher::unengagedTerToolID=1;
#ifdef TERTOOL_LOG
XStream terToolLogStream("tertoollog.txt", XS_OUT);
#endif
///////////////////////////////////////

TerToolsID TerToolsDispatcher::startTerTool(TerToolBase* pTTB)
{
	xassert(pTTB);
	TerToolsID id=getUnengagedID();
	//startedTerTools[id]=pTTB;
	startedTerTools.push_back(StartedTerToolElement(id, pTTB));
	return id;
}
void TerToolsDispatcher::stopTerTool(TerToolsID ttid)
{
	StartedTerTools::iterator p=find(ttid);
	if(p!=startedTerTools.end()){
		delete p->pTerTool;
		startedTerTools.erase(p);
	}
}

void TerToolsDispatcher::pauseTerTool(TerToolsID ttid, bool _pause)
{
	StartedTerTools::iterator p=find(ttid);
	if(p!=startedTerTools.end())
		p->flag_pause=_pause;
}

bool TerToolsDispatcher::isTerToolFinished(TerToolsID ttid)
{
	StartedTerTools::iterator p=find(ttid);
	return (p==startedTerTools.end());
}
void TerToolsDispatcher::setPosition(TerToolsID ttid, const Se3f& pos)
{
	StartedTerTools::iterator p=find(ttid);
	if(p!=startedTerTools.end())
		p->pTerTool->setPosition(pos);
}

void TerToolsDispatcher::quant()
{
	start_timer_auto();
	statistics_add(ToolsTotal, startedTerTools.size());

	//list<TerToolsID> eraseList;
	//list<TerToolsID>::iterator ei;

	//StartedTerTools::iterator p;
	//for(p=startedTerTools.begin(); p!=startedTerTools.end(); p++) {
	//	//string ttname=typeid(*p->second).name();
	//	if(!p->second->quant()){
	//		eraseList.push_back(p->first);
	//	}
	//}
	//for(ei=eraseList.begin(); ei!=eraseList.end(); ei++){
	//	stopTerTool(*ei);
	//}
	int stCnt=0;
	StartedTerTools::iterator p;
	for(p=startedTerTools.begin(); p!=startedTerTools.end(); ) {
		TerToolsID id=p->terToolID;
		//if(!p->flag_pause && !p->pTerTool->pQuant()){
		//	p->pTerTool->absoluteQuant();
		//	delete p->pTerTool;
		//	p=startedTerTools.erase(p);
		//}
		//else {
		//	p->pTerTool->absoluteQuant();
		//	p++;
		//}
		bool flag_delete=false;
		if(!p->flag_pause){
			stCnt++;
			flag_delete=!(p->pTerTool->pQuant());
		}
		p->pTerTool->absoluteQuant();

		if(flag_delete){
			delete p->pTerTool;
			p=startedTerTools.erase(p);
		}
		else p++;
	}
	statistics_add(ToolsWorked, stCnt);
}


TerToolsDispatcher::TerToolsDispatcher()
{
//	if(XStream(0).open(terToolsDispatcherCfgFName, XS_IN)){
//		XPrmIArchive(terToolsDispatcherCfgFName) >> WRAP_NAME(*this, terToolsDispatcherCfgSection);
//	}
}

TerToolsDispatcher::~TerToolsDispatcher()
{
//	XPrmOArchive oa(terToolsDispatcherCfgFName);
//	oa << WRAP_NAME(*this, terToolsDispatcherCfgSection);
}

void TerToolsDispatcher::CashDateBitmap8C::serialize(Archive& ar)
{
	ar.serialize(kScale, "kScale", 0);
	ar.serialize(flag_alfaPresent, "flag_alfaPresent", 0);
	ar.serialize(cashFileName, "cashFileName", 0);
	ar.serialize(sectionSize, "sectionSize", 0);
	ar.serialize(worldFileTime.dwLowDateTime, "worldFileTimeLow", 0);
	ar.serialize(worldFileTime.dwHighDateTime, "worldFileTimeHigh", 0);
	ar.serialize(originalBitmapFileTime.dwLowDateTime, "originalBitmapFileTimeLow", 0);
	ar.serialize(originalBitmapFileTime.dwHighDateTime, "originalBitmapFileTimeHigh", 0);
	ar.serialize(crcBitmap4Dam, "crcBitmap4Dam", 0);
	//ar.serialize(crcpBitmap4Geo, "crcpBitmap4Geo", 0);
	ar.serialize(crcAlfaLayer, "crcAlfaLayer", 0);
}

struct totalCashData : public TerToolsDispatcher::CashDateBitmap8C {
	string bitmapName;
	void serialize(Archive& ar){
		ar.serialize(bitmapName, "bitmapName", 0);
		ar.serialize(static_cast<CashDateBitmap8C&>(*this), "CashDataB8C", 0);
	}
	void set(const string& _bitmapName, TerToolsDispatcher::CashDateBitmap8C& cdb){
		static_cast<CashDateBitmap8C&>(*this)=cdb;
		bitmapName=_bitmapName;
	}
};

void TerToolsDispatcher::loadCashData(const char* fname)
{
	cashDataTable.clear();
	vector <totalCashData> cd;
	XPrmIArchive ia;
	if(ia.open(fname)){
		ia.serialize(cd, "FullCashData", 0);
		vector <totalCashData>::iterator p;
		for(p=cd.begin(); p!=cd.end(); p++){
			//cashDataTable[p->bitmapName.c_str()]=static_cast<CashDateBitmap8C&>(*p);
			CashDataTable::iterator c=cashDataTable.insert(make_pair(p->bitmapName, CashDateBitmap8C()));
			c->second=static_cast<CashDateBitmap8C&>(*p);
		}
	}
}

void TerToolsDispatcher::saveCashData(const char* fname)
{
	vector <totalCashData> cd;
	cd.reserve(cashDataTable.size());
	CashDataTable::iterator p;
	for(p=cashDataTable.begin(); p!=cashDataTable.end(); p++){
		if(!p->second.flag_nonUsedCashData){
			cd.push_back(totalCashData());
			//cd.back().bitmapName=p->first;
			//static_cast<CashDateBitmap8C&>(cd.back())=p->second; //
			cd.back().set(p->first, p->second);
		}
	}
	XPrmOArchive oa(fname);
	oa.serialize(cd, "FullCashData", 0);
}

TerToolsDispatcher::CashDateBitmap8C* TerToolsDispatcher::getCashData(const char* fname, float kScale)
{
	typedef CashDataTable::iterator P;
	pair<P,P> r=cashDataTable.equal_range(fname);
	for(P p=r.first; p!=r.second; ++p){
		if(isEq(p->second.kScale,kScale)){
			return &(p->second);
		}
	}
	return 0;
}

string getCachDirAndCreateIfNotExist()
{
	static const char generalCacheFilePath[]="CacheData";
	static const char cacheFilePath[]="CacheData\\Textures4Worlds";
	createDirectory(generalCacheFilePath);
	createDirectory(cacheFilePath);

	string str=cacheFilePath; str+='\\'; str+=vMap.getWorldName(); 
	createDirectory(str.c_str());

	return str;
}

template <>
void TerToolsDispatcher::prepare4WorldT<unsigned short>()
{
#ifdef TERTOOL_LOG
	terToolLogStream < "=======" < vMap.worldName.c_str() < "\n";
#endif
	bitmap8CTable.clear();
	bitmap8VTable.clear();
	int i;
	TerToolsLibrary::Strings::const_iterator p;
	for(p=TerToolsLibrary::instance().strings().begin(),i=0; p!=TerToolsLibrary::instance().strings().end(); ++p, i++){
#ifdef TERTOOL_LOG
		terToolLogStream < "=TerTool: " < p->c_str() < "\n";
#endif
		bool result=true;
		//if(p->terToolNrml)
		//	result=prepareTerTools4World(p->terToolNrml, i);
		//if(p->terToolInds)
		//	result=result && prepareTerTools4World(p->terToolInds, i);
		if(p->terTool)
			result=prepareTerTools4World(p->terTool, i);
		if(result==false){
			string str="Error in terTool-";
			str+=p->c_str();
			xxassert(0,str.c_str());
		}
	}
}

template <>
void TerToolsDispatcher::prepare4WorldT<unsigned char>()
{
	//_controlfp(_PC_64 , _MCW_PC);
#ifdef TERTOOL_LOG
	terToolLogStream < "=======" < vMap.worldName.c_str() < "\n";
#endif
	ColorQuantizer cqDam,cqGeo;
	cqDam.setPalette(vMap.DamPal, MAX_DAM_SURFACE_TYPE);
	//cqGeo.setPalette(vMap.GeoPal, MAX_GEO_SURFACE_TYPE);

	int i,j;
	for(i=0; i<256; i++){
		Color4c rowColor=vMap.DamPal[i];
		for(j=0; j<256; j++){
			Color4c colColor=vMap.DamPal[j];
			Color4c colA20,colA40,colA60,colA80;
			colA20.interpolate(rowColor, colColor, 0.20f);
			colA40.interpolate(rowColor, colColor, 0.40f);
			colA60.interpolate(rowColor, colColor, 0.60f);
			colA80.interpolate(rowColor, colColor, 0.80f);
			unsigned long v=cqDam.findNearestColor(colA20);
			v|=cqDam.findNearestColor(colA40)<<8;
			v|=cqDam.findNearestColor(colA60)<<16;
			v|=cqDam.findNearestColor(colA80)<<24;
			damAlfaConversion[j][i]=v;
		}
	}

/*	for(i=0; i<256; i++){
		Color4c rowColor=vMap.GeoPal[i];
		for(j=0; j<256; j++){
			Color4c colColor=vMap.GeoPal[j];
			Color4c colA20,colA40,colA60,colA80;
			colA20.interpolate(rowColor, colColor, 0.20f);
			colA40.interpolate(rowColor, colColor, 0.40f);
			colA60.interpolate(rowColor, colColor, 0.60f);
			colA80.interpolate(rowColor, colColor, 0.80f);
			unsigned long v=cqGeo.findNearestColor(colA20);
			v|=cqGeo.findNearestColor(colA40)<<8;
			v|=cqGeo.findNearestColor(colA60)<<16;
			v|=cqGeo.findNearestColor(colA80)<<24;
			geoAlfaConversion[j][i]=v;
		}
	}*/

	//static const char cacheDataFileName[]="cacheFileInfo";
	string cachDir=getCachDirAndCreateIfNotExist(); 
	string cacheDataFileName=cachDir; cacheDataFileName+='\\'; cacheDataFileName+="cacheFileInfo";

	loadCashData(cacheDataFileName.c_str());

	bitmap8CTable.clear();
	bitmap8VTable.clear();
	TerToolsLibrary::Strings::const_iterator p;
	for(p=TerToolsLibrary::instance().strings().begin(),i=0; p!=TerToolsLibrary::instance().strings().end(); ++p, i++){
#ifdef TERTOOL_LOG
		terToolLogStream < "=TerTool: " < p->c_str() < "\n";
#endif
		bool result=true;
		//if(p->terToolNrml)
		//	result=prepareTerTools4World(p->terToolNrml, i, cqGeo, cqDam, cachDir.c_str());
		//if(p->terToolInds)
		//	result=result && prepareTerTools4World(p->terToolInds, i, cqGeo, cqDam, cachDir.c_str());
		if(p->terTool)
			result=prepareTerTools4World(p->terTool, i);
		if(result==false){
			string str="Error in terTool-";
			str+=p->c_str();
			xxassert(0,str.c_str());
		}
	}
	saveCashData(cacheDataFileName.c_str());
}

void TerToolsDispatcher::prepare4World()
{ 
	prepare4WorldT<TerrainColor>(); 
}

bool TerToolsDispatcher::prepareTerTools4World(const TerToolBase* pTerTool, int indexIn, ColorQuantizer& cqGeo, ColorQuantizer& cqDam, const char* cachDir)
{
	list<TerToolBase::ResourceDescription> rdList;
	list<TerToolBase::ResourceDescription>::iterator p;
	pTerTool->getResourceDescription(rdList);
	for(p=rdList.begin(); p!=rdList.end(); p++){
		const string& fname=p->fileName;// pTerTool->getTexture();
		const float terTextureKScale=p->kScale; //pTerTool->getTextureKScale();
		const float terTextureVScale=p->vScale;
		if(!fname.empty()){
			if(p->resourceDescriptionType==TerToolBase::ResourceDescription::RDT_Bitmap8Color4Surface){
				typedef Bitmap8CTable::iterator P;
				pair<P,P> r=bitmap8CTable.equal_range(fname);
				P p;
				for( p=r.first; p!=r.second; ++p){
					if(isEq(p->second.kScale,terTextureKScale))
						break;
				}
				if(p==r.second){
					Bitmap8CTable::iterator m=bitmap8CTable.insert(make_pair(fname, Bitmap8C()));
					CashDateBitmap8C* pCDB=getCashData(fname.c_str(),terTextureKScale);
					if(!pCDB){
						//CashDataTable::iterator mm=cashDataTable.insert(make_pair(fname, CashDateBitmap8C()));
						pCDB=&(cashDataTable.insert(make_pair(fname, CashDateBitmap8C()))->second);
					}
					m->second.load8Andprepare4World(fname.c_str(), terTextureKScale, indexIn, cqGeo, cqDam, pCDB, cachDir);
					#ifdef TERTOOL_LOG
					terToolLogStream < "Bitmap8C Prepared:" < fname.c_str() < " fScale=" <= terTextureKScale < "\n";
					#endif
				}
			}
			else if(p->resourceDescriptionType==TerToolBase::ResourceDescription::RDT_Bitmap8Voxel){
				typedef Bitmap8VTable::iterator P;
				pair<P,P> r=bitmap8VTable.equal_range(fname);
				P p;
				for(p=r.first; p!=r.second; ++p){
					if(isEq(p->second.kScale,terTextureKScale) && isEq(p->second.vScale,terTextureVScale))
						break;
				}
				if(p==r.second){
					Bitmap8VTable::iterator m=bitmap8VTable.insert(make_pair(fname, Bitmap8V()));
					m->second.load(fname.c_str(), terTextureKScale, terTextureVScale);
					#ifdef TERTOOL_LOG
					terToolLogStream < "Bitmap8V Prepared:" < fname.c_str() < " fScale=" <= terTextureKScale < "\n";
					#endif
				}
			}
		}
	}
	return true;
}

bool TerToolsDispatcher::prepareTerTools4World(const TerToolBase* pTerTool, int indexIn)
{
	list<TerToolBase::ResourceDescription> rdList;
	list<TerToolBase::ResourceDescription>::iterator p;
	pTerTool->getResourceDescription(rdList);
	for(p=rdList.begin(); p!=rdList.end(); p++){
		const string& fname=p->fileName;// pTerTool->getTexture();
		const float terTextureKScale=p->kScale; //pTerTool->getTextureKScale();
		const float terTextureVScale=p->vScale;
		if(!fname.empty()){
			if(p->resourceDescriptionType==TerToolBase::ResourceDescription::RDT_Bitmap8Color4Surface){
				typedef Bitmap8CTable::iterator P;
				pair<P,P> r=bitmap8CTable.equal_range(fname);
				P p;
				for( p=r.first; p!=r.second; ++p){
					if(isEq(p->second.kScale,terTextureKScale))
						break;
				}
				if(p==r.second){
					Bitmap8CTable::iterator m=bitmap8CTable.insert(make_pair(fname, Bitmap8C()));
					m->second.load16(fname.c_str(), terTextureKScale, indexIn);
					#ifdef TERTOOL_LOG
					terToolLogStream < "Bitmap8C Prepared:" < fname.c_str() < " fScale=" <= terTextureKScale < "\n";
					#endif
				}
			}
			else if(p->resourceDescriptionType==TerToolBase::ResourceDescription::RDT_Bitmap8Voxel){
				typedef Bitmap8VTable::iterator P;
				pair<P,P> r=bitmap8VTable.equal_range(fname);
				P p;
				for(p=r.first; p!=r.second; ++p){
					if(isEq(p->second.kScale,terTextureKScale) && isEq(p->second.vScale,terTextureVScale))
						break;
				}
				if(p==r.second){
					Bitmap8VTable::iterator m=bitmap8VTable.insert(make_pair(fname, Bitmap8V()));
					m->second.load(fname.c_str(), terTextureKScale, terTextureVScale);
					#ifdef TERTOOL_LOG
					terToolLogStream < "Bitmap8V Prepared:" < fname.c_str() < " fScale=" <= terTextureKScale < "\n";
					#endif
				}
			}
		}
	}
	return true;
}


bool TerToolsDispatcher::putAllColorTerTool2ColorQuantizer(const TerToolBase* pTerTool, ColorQuantizer& cq)
{
	list<TerToolBase::ResourceDescription> rdList;
	list<TerToolBase::ResourceDescription>::iterator p;
	pTerTool->getResourceDescription(rdList);
	for(p=rdList.begin(); p!=rdList.end(); p++){
		const string& fname=p->fileName;
		const float terTextureKScale=p->kScale;
		if(!fname.empty()){
			if(p->resourceDescriptionType==TerToolBase::ResourceDescription::RDT_Bitmap8Color4Surface){
				typedef Bitmap8CTable::iterator P;
				pair<P,P> r=bitmap8CTable.equal_range(fname);
				P p;
				for( p=r.first; p!=r.second; ++p){
					if(isEq(p->second.kScale,terTextureKScale))
						break;
				}
				if(p==r.second){
					Bitmap8CTable::iterator m=bitmap8CTable.insert(make_pair(fname, Bitmap8C()));
					m->second.loadAndPutBitmap2Quantizer(fname.c_str(), terTextureKScale, cq);
				}
			}
		}
	}
	return true;
}

bool TerToolsDispatcher::putAllColorTerTools2ColorQuantizer(ColorQuantizer& cq)
{
	bitmap8CTable.clear();
	TerToolsLibrary::Strings::const_iterator p;
	for(p=TerToolsLibrary::instance().strings().begin(); p!=TerToolsLibrary::instance().strings().end(); ++p){
		bool result=true;
		//if(p->terToolNrml)
		//	result=putAllColorTerTool2ColorQuantizer(p->terToolNrml, cq);
		//if(p->terToolInds)
		//	result=result && putAllColorTerTool2ColorQuantizer(p->terToolInds, cq);
		if(p->terTool)
			result=putAllColorTerTool2ColorQuantizer(p->terTool, cq);
		if(result==false){
			string str="Error in terTool-";
			str+=p->c_str();
			xxassert(0,str.c_str());
		}
	}
	bitmap8CTable.clear();
	return true;
}

void TerToolsDispatcher::serialize(Archive& ar)
{
	//ar & WRAP_OBJECT(bitmap8CTable);
	//ar & WRAP_OBJECT(bitmap8VTable);
	//ar.serialize(bitmaps_, "bitmaps_", 0);
}

bool TerToolsDispatcher::Bitmap8C::loadAndPutBitmap2Quantizer(const char* name, const float terTextureKScale, ColorQuantizer& cq)
{
	TGAHEAD tgahead;
	if(!tgahead.loadHeader(name)){
		string str="Error loading terTool texture-";
		str+=name;
		xxassert(0, str.c_str());
		return false;
	}
	if( (tgahead.PixelDepth!=24 && tgahead.PixelDepth!=32) || tgahead.ImageType!=2 ) {
		string str="Не поддерживаемый тип TGA (необходим 32 или 24bit не компрессированный)-";
		str+=name;
		xxassert(0, str.c_str());
		return false;
	}

	Vect2s originalSize(tgahead.Width,tgahead.Height);
	kScale=terTextureKScale;
	size.x = round((float)originalSize.x*kScale);
	if(size.x < 1) size.x=1;
	size.y = round((float)originalSize.y*kScale);
	if(size.y <1 ) size.y=1;
	maxCoord.x=size.x-1;
	maxCoord.y=size.y-1;
	unsigned long* ptb=new unsigned long[originalSize.x*originalSize.y];
	if(tgahead.PixelDepth==24)
		tgahead.load2RGBL(originalSize.x, originalSize.y, ptb);
    else if(tgahead.PixelDepth==32)
		tgahead.load2buf((unsigned char*)ptb, originalSize.x*originalSize.y*sizeof(unsigned long));
	else xassert(0);
	if(!(originalSize==size)){
		unsigned long* ptbo=ptb;
		ptb=new unsigned long[size.x*size.y];
		scaleRGBAImage(ptbo, originalSize, ptb, size);
		delete ptbo;
	}
	int i,j, cnt=0;
	for(i=0; i<size.y; i++){
		for(j=0; j<size.x; j++){
			if(tgahead.PixelDepth!=32 || (ptb[cnt]>>24)!=0 ){
				cq.putColor(ptb[cnt]);
			}
			cnt++;
		}
	}
	delete ptb;
	return true;
}

bool TerToolsDispatcher::Bitmap8C::validateAndLoadDataCash(const char* fname, CashDateBitmap8C* pCDB, const char* cachDir)
{
	xassert(pCDB);// должно быть всегда
	if(pCDB->cashFileName.empty())
		return false;
	XZipStream fb(0);
	if(!fb.open(fname, XS_IN))
		return false;
	FILETIME ft;
	unsigned int ftime, fdate;
	fb.gettime(fdate, ftime);
	if(!::DosDateTimeToFileTime(fdate, ftime, &ft))
		return false;

//	if(!::GetFileTime((HANDLE)fb.gethandler(), 0, 0, &ft))
//		return false;

	if(::CompareFileTime(&ft, &pCDB->originalBitmapFileTime)!=0)
		return false;
	fb.close();
	if(!fb.open(vMap.getTargetName(vrtMap::worldDataFile), XS_IN)){
		xassert(0&&"Error world testing!");
		return false;
	}
	fb.gettime(fdate, ftime);
	if(!::DosDateTimeToFileTime(fdate, ftime, &ft))
		return false;

//	if(!::GetFileTime((HANDLE)fb.gethandler(), 0, 0, &ft))
//		return false;

	if(::CompareFileTime(&ft, &pCDB->worldFileTime)!=0)
		return false;
	fb.close();
	//Load cash data
	string str=cachDir; str+='\\';
	str+=pCDB->cashFileName;
	if(!fb.open(str.c_str(), XS_IN))
		return false;
	int allsize=fb.size();
	int sectionsize;
	if(pCDB->flag_alfaPresent)
		sectionsize=allsize/2; //(sizeof(TerrainColor)+1); //3
	else 
		sectionsize=allsize/1; //(sizeof(TerrainColor)); // 2
	if(sectionsize!=pCDB->sectionSize)
		return false;

	bool flag_error=false;
	unsigned int ccrc;
	pBitmap4Dam=new TerrainColor[sectionsize];
	//pBitmap4Geo=new unsigned char[sectionsize];
	fb.read(pBitmap4Dam, sectionsize);
	ccrc=crc32((unsigned char*)pBitmap4Dam, sectionsize*sizeof(pBitmap4Dam[0]), startCRC32);
	flag_error|= (ccrc!=pCDB->crcBitmap4Dam);

	//fb.read(pBitmap4Geo, sectionsize);
	//ccrc=crc32(pBitmap4Geo, sectionsize, startCRC32);
	//flag_error|= (ccrc!=pCDB->crcpBitmap4Geo);

	if(pCDB->flag_alfaPresent){
		pAlfaLayer=new unsigned char[sectionsize];
		fb.read(pAlfaLayer, sectionsize);
		ccrc=crc32(pAlfaLayer, sectionsize, startCRC32);
		flag_error|= (ccrc!=pCDB->crcAlfaLayer);
		if(flag_error){
			delete [] pAlfaLayer;
			pAlfaLayer=0;
		}
	}
	if(flag_error){
		delete [] pBitmap4Dam;
		pBitmap4Dam=0;
		//delete [] pBitmap4Geo;
		//pBitmap4Geo=0;
	}
	if(flag_error)
		return false;
	else{
		pCDB->flag_nonUsedCashData=false;
		return true;
	}
}

void TerToolsDispatcher::Bitmap8C::saveDataCash(const char* fname, CashDateBitmap8C* pCDB, const char* cachDir)
{
	int sectionsize=size.x*size.y;
	char nameBuf[_MAX_FNAME];
	_splitpath (fname, 0, 0, &nameBuf[0], 0);
	XBuffer newFName;
	newFName < cachDir < '\\' < nameBuf < 'K' <= kScale;

	//string str=cachDir; str+='\\';
	//str+=newFName;
	XStream fb(0);
	XZipStream fb1(0);
	int flag_noterror=1;
	flag_noterror&=(int)fb.open(newFName.c_str(), XS_OUT);
	if(!flag_noterror) goto error_handler;
	fb.write(pBitmap4Dam, sectionsize);
	//fb.write(pBitmap4Geo, sectionsize);
	if(pAlfaLayer!=0){
		fb.write(pAlfaLayer, sectionsize);
	}

	unsigned int crcdam=crc32((unsigned char*)pBitmap4Dam, sectionsize*sizeof(pBitmap4Dam[0]), startCRC32);
	//unsigned int crcgeo=crc32(pBitmap4Geo, sectionsize, startCRC32);
	unsigned int crcalfa=0;
	if(pAlfaLayer!=0){
		crcalfa=crc32(pAlfaLayer, sectionsize, startCRC32);
	}
	fb.close();

	FILETIME worldFT;
	if(!fb1.open(vMap.getTargetName(vrtMap::worldDataFile), XS_IN)){
		xassert(0&&"Error world testing!");
		goto error_handler;
	}
	unsigned int ftime, fdate;
	fb1.gettime(fdate, ftime);
	flag_noterror&=::DosDateTimeToFileTime(fdate, ftime, &worldFT);
//	flag_noterror&=::GetFileTime((HANDLE)fb.gethandler(), 0, 0, &worldFT);
	fb1.close();

	flag_noterror&=(int)fb1.open(fname, XS_IN);
	FILETIME bitmapFT;
	fb1.gettime(fdate, ftime);
	flag_noterror&=::DosDateTimeToFileTime(fdate, ftime, &bitmapFT);
//	flag_noterror&=::GetFileTime((HANDLE)fb.gethandler(), 0, 0, &bitmapFT);
	if(!flag_noterror) goto error_handler;

	pCDB->set(kScale, pAlfaLayer!=0, newFName, size.x*size.y, worldFT, bitmapFT, crcdam, /*crcgeo,*/ crcalfa);
	return;

error_handler:
	pCDB->flag_nonUsedCashData=true;
	xassert(0&&"Error writing cash file.");
	return;
}

bool TerToolsDispatcher::Bitmap8C::load8Andprepare4World(const char* fname, const float terTextureKScale, int indexIn, ColorQuantizer& cqGeo, ColorQuantizer& cqDam, CashDateBitmap8C* pCDB, const char* cachDir)
{
	index = indexIn;
	TGAHEAD tgahead;
	if(!tgahead.loadHeader(fname)){
		string str="Error loading terTool texture-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}
	if( (tgahead.PixelDepth!=24 && tgahead.PixelDepth!=32) || tgahead.ImageType!=2 ) {
		//AfxMessageBox("Не поддерживаемый тип TGA (необходим 24b it не компрессированный, с размерами, кратными степени 2)");
		//xassert(0&&"Не поддерживаемый тип TGA (необходим 32 или 24bit не компрессированный)");
		string str="Не поддерживаемый тип TGA (необходим 32 или 24bit не компрессированный)-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}

	release();
	Vect2s originalSize(tgahead.Width,tgahead.Height);
	kScale=terTextureKScale;
	size.x = round((float)originalSize.x*kScale);
	if(size.x < 1) size.x=1;
	size.y = round((float)originalSize.y*kScale);
	if(size.y <1 ) size.y=1;
	maxCoord.x=size.x-1;
	maxCoord.y=size.y-1;

	if(validateAndLoadDataCash(fname, pCDB, cachDir)){
//		log_var_crc(pBitmap4Dam, size.x*size.y*sizeof(pBitmap4Dam[0]));
//		log_var_crc(pBitmap4Geo, size.x*size.y*sizeof(pBitmap4Geo[0]));
		if(pAlfaLayer){
//			log_var_crc(pAlfaLayer, size.x*size.y*sizeof(pAlfaLayer[0]));
		}
		return true;
	}

	pBitmap4Dam=new TerrainColor[size.x*size.y];
	//pBitmap4Geo=new unsigned char[size.x*size.y];
	unsigned long* ptb=new unsigned long[originalSize.x*originalSize.y];
	if(tgahead.PixelDepth==24)
		tgahead.load2RGBL(originalSize.x, originalSize.y, ptb);
    else if(tgahead.PixelDepth==32)
		tgahead.load2buf((unsigned char*)ptb, originalSize.x*originalSize.y*sizeof(unsigned long));
	else xassert(0);
	if(!(originalSize==size)){
		unsigned long* ptbi=ptb;
		ptb=new unsigned long[size.x*size.y];
		scaleRGBAImage(ptbi, originalSize, ptb, size);
		delete ptbi;
	}
	if(tgahead.PixelDepth==32){
		pAlfaLayer=new unsigned char[size.x*size.y];
		int i,j, cnt=0;
		for(i=0; i<size.y; i++){
			for(j=0; j<size.x; j++){
				pAlfaLayer[cnt]=ptb[cnt]>>24;
				cnt++;
			}
		}
	}

	cqDam.ditherFloydSteinberg(ptb, reinterpret_cast<unsigned char*>(pBitmap4Dam), size);
	//cqGeo.ditherFloydSteinberg(ptb, pBitmap4Geo, size);

	delete ptb;

//	log_var_crc(pBitmap4Dam, size.x*size.y*sizeof(pBitmap4Dam[0]));
//	log_var_crc(pBitmap4Geo, size.x*size.y*sizeof(pBitmap4Geo[0]));
	if(pAlfaLayer){
//		log_var_crc(pAlfaLayer, size.x*size.y*sizeof(pAlfaLayer[0]));
	}
	saveDataCash(fname, pCDB, cachDir);
	return true;
}

bool TerToolsDispatcher::Bitmap8C::load16(const char* fname, const float terTextureKScale, int indexIn)
{
	index = indexIn;
	TGAHEAD tgahead;
	if(!tgahead.loadHeader(fname)){
		string str="Error loading terTool texture-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}
	if( (tgahead.PixelDepth!=24 && tgahead.PixelDepth!=32) || tgahead.ImageType!=2 ) {
		//AfxMessageBox("Не поддерживаемый тип TGA (необходим 24b it не компрессированный, с размерами, кратными степени 2)");
		//xassert(0&&"Не поддерживаемый тип TGA (необходим 32 или 24bit не компрессированный)");
		string str="Не поддерживаемый тип TGA (необходим 32 или 24bit не компрессированный)-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}

	release();
	Vect2s originalSize(tgahead.Width,tgahead.Height);
	kScale=terTextureKScale;
	size.x = round((float)originalSize.x*kScale);
	if(size.x < 1) size.x=1;
	size.y = round((float)originalSize.y*kScale);
	if(size.y <1 ) size.y=1;
	maxCoord.x=size.x-1;
	maxCoord.y=size.y-1;

	pBitmap4Dam=new TerrainColor[size.x*size.y];
	//pBitmap4Geo=new unsigned char[size.x*size.y];
	unsigned long* ptb=new unsigned long[originalSize.x*originalSize.y];
	if(tgahead.PixelDepth==24)
		tgahead.load2RGBL(originalSize.x, originalSize.y, ptb);
    else if(tgahead.PixelDepth==32)
		tgahead.load2buf((unsigned char*)ptb, originalSize.x*originalSize.y*sizeof(unsigned long));
	else xassert(0);
	if(!(originalSize==size)){
		unsigned long* ptbi=ptb;
		ptb=new unsigned long[size.x*size.y];
		scaleRGBAImage(ptbi, originalSize, ptb, size);
		delete ptbi;
	}
	int i,j;
	int cnt=0;
	for(i=0; i<size.y; i++){
		for(j=0; j<size.x; j++){
			//pBitmap4Dam[cnt]=(((ptb[cnt]&0xff0000)+0x040000>>16+3)<<11) | 
			//	(((ptb[cnt]&0x00ff00)+0x0200>>8+2)<<5) |
			//	(((ptb[cnt]&0xff)+0x04>>0+3)<<0);
			pBitmap4Dam[cnt]=ConvertTry2HighColor(ptb[cnt]);
			cnt++;
		}
	}

	if(tgahead.PixelDepth==32){
		pAlfaLayer=new unsigned char[size.x*size.y];
		//int i,j, cnt=0;
		cnt=0;
		for(i=0; i<size.y; i++){
			for(j=0; j<size.x; j++){
				pAlfaLayer[cnt]=ptb[cnt]>>24;
				cnt++;
			}
		}
	}


	delete ptb;
//	log_var_crc(pBitmap4Dam, size.x*size.y*sizeof(pBitmap4Dam[0]));
	if(pAlfaLayer){
//		log_var_crc(pAlfaLayer, size.x*size.y*sizeof(pAlfaLayer[0]));
	}
	return true;
}


bool TerToolsDispatcher::Bitmap8V::load(const char* fname, const float terTextureKScale, const float terTextureVScale)
{
	TGAHEAD tgahead;
	if(!tgahead.loadHeader(fname)){
		string str="Error loading terTool texture-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}
	if( tgahead.PixelDepth!=8 || tgahead.ImageType!=3 ) {
		//xassert(0&&"Не поддерживаемый тип TGA (необходим монохромный не компрессированный)");
		string str="Не поддерживаемый тип TGA (необходим монохромный не компрессированный)-";
		str+=fname;
		xxassert(0, str.c_str());
		return false;
	}
	release();
	Vect2s originalSize(tgahead.Width,tgahead.Height);
	kScale=terTextureKScale;
	vScale=terTextureVScale;
	size.x = round((float)originalSize.x*kScale);
	if(size.x < 1) size.x=1;
	size.y = round((float)originalSize.y*kScale);
	if(size.y <1 ) size.y=1;
	pBitmapV=new unsigned char[size.x*size.y];
	maxCoord.x=size.x-1;
	maxCoord.y=size.y-1;

	unsigned char* ptmp=new unsigned char[originalSize.x*originalSize.y];
	tgahead.load2buf(ptmp, originalSize.x*originalSize.y*sizeof(unsigned char));
	scaleCImage(ptmp, originalSize, pBitmapV, size);
	//tgahead.savebuf2file("!tmpTGA.tga", pBitmapV, size.x*size.y*sizeof(char), size, sizeof(char));
	//tgahead.savebuf2file("!tmpTGAO.tga", ptmp, originalSize.x*originalSize.y*sizeof(char), originalSize, sizeof(char));
	delete ptmp;

	int i,j;
	int cnt=0;
	for(i=0; i<size.y; i++){
		for(j=0; j<size.x; j++){
			pBitmapV[cnt]=clamp(round((float)pBitmapV[cnt]*vScale), 0, UCHAR_MAX);
			cnt++;
		}
	}

//	log_var_crc(pBitmapV, size.x*size.y*sizeof(pBitmapV[0]));
	return true;
}
