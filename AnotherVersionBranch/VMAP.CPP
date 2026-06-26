#include "stdafxTr.h"

#include "stdio.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "quantizer.h"
#include "terTools.h"
#include "ResourceSelector.h"
#include "tgai.h"
#include "..\Util\Console.h"

// коммент не удалять!  нужно для перевода: _VISTA_ENGINE_EXTERNAL_

/* ----------------------------- EXTERN SECTION ---------------------------- */

extern int DEFAULT_TERRAIN;

/* --------------------------- DEFINITION SECTION -------------------------- */
vrtMap vMap;

const unsigned int MAX_CHANGED_RENDER_AREAS=512;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, eCreateWorldMetod, "eCreateWorldMetod")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, FullPlain, "Плоский");
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, Mountains, "Горы");
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, eCreateWorldMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, Quality, "Quality")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, QUALITY_BAD, "Плохое");
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, QUALITY_GOOD, "Хорошее");
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, Quality)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, SIZE_POWER, "Размер")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_512,  "512")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_1024, "1024")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_2048, "2048")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_4096, "4096")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_8192, "8192")
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, SIZE_POWER)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eShowSpecialInfo, "Показ спец поверхности")
REGISTER_ENUM_ENCLOSED(vrtMap, SSI_NoShow,  "не показывать")
REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowHardness, "неразрушаемость")
REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowKind, "1,2,3,4 рода")
REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowImpassability, "непроходимость")
REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowAllGeo, "гео")
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eShowSpecialInfo)


char* vrtMap::worldRasterDataFileSource = "output.vmp";
char* vrtMap::worldRGBFileSource = "output.tga";
char* vrtMap::worldGeoPalFile     = "inGeo.act";
char* vrtMap::worldDamPalFile     = "inDam.act";
char* vrtMap::worldLeveledTextureFile= "leveledSurfaceTexture.tga";
char* vrtMap::worldSurfaceKindFile= "surkind.bin";
char* vrtMap::worldDataFile			= "world.cls";
char* vrtMap::worldRGBCacheFile			= "cache.tga";
char* vrtMap::worldRasterDataCacheFile	= "cache.vmp";
char* vrtMap::worldMiniMapFile			= "map.tga";

unsigned long generateSerialNumber()
{
	RandomGenerator myrnd;
	myrnd.set(GetTickCount());
    return myrnd();
}

void vrtMapCreationParam::serialize(Archive& ar) 
{
    ar.serialize(H_SIZE_POWER, "H_SIZE_POWER", "Ширина");
    ar.serialize(V_SIZE_POWER, "V_SIZE_POWER", "Высота");

#ifdef _DEBUG
	ar.serialize(createWorldMetod, "createWorldMetod", "Метод создания");
#else
	ar.serialize(createWorldMetod, "createWorldMetod", 0);
#endif
	ar.serialize(initialHeight, "initialHeight", "Начальная высота");
}

//////////////////// Для конвертации старого контейнера PMO
class ElementPMO {
public:
	sGeoPMO				geoPMO;
	sToolzerPMO			toolzerPMO;
	sSquareToolzerPMO	squareToolzerPMO;
	sBlurPMO			blurPMO;
	sColorPicPMO		colorPicPMO;
	sBitGenPMO			bitGenPMO;
	PMOperationID		ID;
	void serialize(Archive& ar){
		ar.serialize(ID, "ID", 0);
		switch(ID){
		case PMO_ID_TOOLZER:		ar.serialize(toolzerPMO, "toolzerPMO", 0);		break;
		case PMO_ID_SQUARE_TOOLZER:	ar.serialize(squareToolzerPMO, "squareToolzerPMO", 0);break;
		case PMO_ID_GEO:			ar.serialize(geoPMO, "geoPMO", 0);			break;
		case PMO_ID_BLUR:			ar.serialize(blurPMO, "blurPMO", 0);		break;
		case PMO_ID_COLORPIC:		ar.serialize(colorPicPMO, "colorPicPMO", 0);	break;
		case PMO_ID_BITGEN:			ar.serialize(bitGenPMO, "bitGenPMO", 0);		break;
		}
	}
};
//.........................................................
void vrtMap::serializeParameters(Archive& ar)
{
    if(!ar.isEdit()){
		ar.serialize(geoTxFileName, "geoTxFileName", 0);
		ar.serialize(matGeoTxFileName, "matGeoTxFileName", 0);
		ar.serialize(loopedGeoTxFileName, "loopedGeoTxFileName", 0);

		ar.serialize(bitmapDispatcher, "bitmapDispatcher", 0);

		curNewPMO=containerPMO.size();
		unsigned int lowworld_procMapOp=procMapOp&0xffFFffFF;
		int highworld_procMapOp=procMapOp>>32;
		ar.serialize(lowworld_procMapOp, "lowworld_procMapOp", 0);
		ar.serialize(highworld_procMapOp, "highworld_procMapOp", 0);
		procMapOp =(__int64)lowworld_procMapOp;
		procMapOp|=(__int64)highworld_procMapOp <<32;

		ar.serialize(VxABufCRC, "VxABufCRC", 0);
		ar.serialize(GeoSurBufCRC, "GeoSurBufCRC", 0);
		ar.serialize(DamSurBufCRC, "DamSurBufCRC", 0);
		ar.serialize(guid.Data1, "GUID_Data1", 0);
		ar.serialize(guid.Data2, "GUID_Data2", 0);
		ar.serialize(guid.Data3, "GUID_Data3", 0);
		ar.serializeArray(guid.Data4, "GUID_Data4", 0);
		
		ar.serialize(indexColorsUsed, "indexColorsUsed", 0);

		if(ar.isOutput() && worldSerial==0){
			worldSerial=generateSerialNumber();
		}
		ar.serialize(worldSerial, "worldSerial", 0);
	}

}

void vrtMap::serialize(Archive& ar) 
{
    serializeParameters(ar);
	ar.serialize(geoTxOctree, "geoTxOctree", 0);
}


unsigned char vrtMap::GetGeoType(int offset, int h) 
{
	int j=offset & clip_mask_x;
	int i=offset>>H_SIZE_POWER;
	unsigned long c=GetGeoColor(j,i);
	unsigned char idxc=geoTxOctree.getIdxColor(c);
	return idxc;
}
unsigned long vrtMap::GetGeoColor(int x, int y) 
{
	unsigned long cl=0;
	unsigned long cm=0;
	if(matGeoTx.pData){
		int xp=x*kXScaleMatGeoTx>>16;
		int kx=(x*kXScaleMatGeoTx>>8)&0xFF;
		int kx1=(1<<8)-kx;
		int yp=y*kYScaleMatGeoTx>>16;
		int ky=(y*kYScaleMatGeoTx>>8)&0xFF;
		int ky1=(1<<8)-ky;
		unsigned long cm0=matGeoTx.pData[(xp)%matGeoTx.sx + (yp)%matGeoTx.sy*matGeoTx.sx];
		unsigned long cm1=matGeoTx.pData[(xp+1)%matGeoTx.sx + (yp)%matGeoTx.sy*matGeoTx.sx];
		unsigned long cm2=matGeoTx.pData[(xp)%matGeoTx.sx + (yp+1)%matGeoTx.sy*matGeoTx.sx];
		unsigned long cm3=matGeoTx.pData[(xp+1)%matGeoTx.sx + (yp+1)%matGeoTx.sy*matGeoTx.sx];
		cm0= (((cm0&0xFF)*kx1 + (cm1&0xFF)*kx)>>8) + ((((cm0&0xFF00)*kx1 + (cm1&0xFF00)*kx)>>8)&0xFF00) + ((((cm0&0xFF0000)*kx1 + (cm1&0xFF0000)*kx)>>8)&0xFF0000);
		cm1= (((cm2&0xFF)*kx1 + (cm3&0xFF)*kx)>>8) + ((((cm2&0xFF00)*kx1 + (cm3&0xFF00)*kx)>>8)&0xFF00) + ((((cm2&0xFF0000)*kx1 + (cm3&0xFF0000)*kx)>>8)&0xFF0000);
		cm = (((cm0&0xFF)*ky1 + (cm1&0xFF)*ky)>>8) + ((((cm0&0xFF00)*ky1 + (cm1&0xFF00)*ky)>>8)&0xFF00) + ((((cm0&0xFF0000)*ky1 + (cm1&0xFF0000)*ky)>>8)&0xFF0000);
	}
	if(loopedGeoTx.pData) {
		cl=loopedGeoTx.pData[x%loopedGeoTx.sx + y%loopedGeoTx.sy*loopedGeoTx.sx];
		if(matGeoTx.pData){
			const unsigned long mask=0x00FEFEFF;
			unsigned long c = ( (cl&mask) + (cm&mask) ) >> 1;
			return c;
		}
	}
	return cl+cm;
}

bool testExistingFile(const char* fName)
{
	DWORD fa = GENERIC_READ;
	DWORD fs = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD fc = OPEN_EXISTING;
	DWORD ff = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	HANDLE hFile=CreateFile(fName, fa, fs, NULL, fc, ff, 0);
	if(hFile == INVALID_HANDLE_VALUE) return 0;
	else { CloseHandle(hFile); return 1; }
}

char* GetINIstringV(const char* iniFile,const char* section,const char* key)
{
	static char buffer[255+1];
	static char fullPath[_MAX_PATH];
	if(_fullpath(fullPath, iniFile, _MAX_PATH) == NULL) ErrH.Abort("Invalid path..." );
	if(!GetPrivateProfileString(section,key,"",buffer,255,fullPath)){//fullPath
		buffer[0]=0;
	}
	return buffer;
}
void SaveINIstringV(const char* iniFile,const char* section,const char* key,const char* var)
{
	//WritePrivateProfileString(section,key,var,iniFile);
	static char fullPath[_MAX_PATH];
	if(_fullpath(fullPath, iniFile, _MAX_PATH) == NULL) ErrH.Abort("Invalid path..." );
	WritePrivateProfileString(section,key,var,fullPath);//fullPath
}

const char* vrtMap::getTargetName(const char* name)
{
	static string buffer;
	if(worldsDir.empty()) buffer.clear();
	else buffer = worldsDir + "\\";

	buffer += worldName + "\\" + name;
	return buffer.c_str();
}

vrtMap::vrtMap(void)
{
	flag_worldLoaded=false;
    // params

	worldName = "Default";
	worldsDir = "Resource\\Worlds";

	//changedT = NULL;

	gridChAreas = NULL;
	gridChAreas2 = NULL;

	pTempArray=NULL;

	worldChanged=0;

	//ClTrBuf=0; AtBuf=0; VxBuf=0;

	//VxGBuf=0; VxDBuf=0; AtrBuf=0; 
	VxABuf=0; SurBuf=0; RnrBuf=0;
	GVBuf=0; GABuf=0;
	SupBuf=0;

	LvdTex=0;

	veryLightDam=0;

	showSpecialInfo=SSI_NoShow;//SSI_NoShow; //SSI_ShowImpassability;// //SSI_ShowHardness
	flag_ShowDbgInfo=0;

	FilterMinHeight=0;
	FilterMaxHeight=MAX_VX_HEIGHT;

//Очистка списка для Undo  и установка итератора на начальный элемент
	//preCAs.erase(preCAs.begin(), preCAs.end());
	//curPreCA=preCAs.begin();
	containerPMO.reserve(1000);
	UndoDispatcher_KillAllUndo();
	shadow_control=true;
	//PreocedureMap
	flag_record_operation=true;
	clearContainerPMO();
	procMapOp=0;

	flag_saveChangedAreas=true;

    VxABufCRC=GeoSurBufCRC=DamSurBufCRC=0;
	const GUID guidzero = {0, 0, 0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
	guid = guidzero;

	flag_tryColorDamTextures=false;

	prepareCircleArr();
	worldSerial=0;
	disableLoadContainerPMO_=false;
}

class cTileMap*& vrtMap::currentTileMap()
{
	static cTileMap* tileMap=0;
	return tileMap;
}
void vrtMap::linking2TileMap(class cTileMap* tm)
{
	currentTileMap()=tm;
}

vrtMap::~vrtMap(void)
{

	flag_worldLoaded=false;

	//if(changedT) { delete changedT; changedT = NULL; }
	releaseChAreaBuf();

	delLeveledTexture();//Необходимо вызывать до удаления VxDBuf !
	if(VxABuf!=0) releaseMem4Buf();

	releaseCircleArr();
}

void vrtMap::allocMem4Buf(bool flag_useTryColorBuffer)
{
	VxABuf = new unsigned short [H_SIZE*V_SIZE];

	SurBuf = new unsigned char [H_SIZE*V_SIZE];
	RnrBuf = new unsigned char [H_SIZE*V_SIZE];
	if(flag_useTryColorBuffer)
        SupBuf = new unsigned long [H_SIZE*V_SIZE];


	GVBuf  = new unsigned char [(H_SIZE>>kmGrid)*(V_SIZE>>kmGrid)];
	GABuf  = new unsigned short [(H_SIZE>>kmGrid)*(V_SIZE>>kmGrid)];

}
void vrtMap::releaseMem4Buf(void)
{
	delete [] GVBuf;
	delete [] GABuf;

	if(SupBuf){
		delete [] SupBuf; SupBuf=0;
	}
	delete [] RnrBuf;
	delete [] SurBuf;
	delete [] VxABuf;
	VxABuf=0;
}


void ConvertProcedure(char* name)
{
	XStream fin(name,XS_IN);
	char* outname = strdup(name);
	memcpy(outname + strlen(name) - 3,"CNV",3);

	XStream fout(outname,XS_OUT);

	fin.seek(512,XS_BEG);

	const int xsize = vMap.H_SIZE;
	const int ysize = vMap.V_SIZE;

	unsigned char* buf = new unsigned char[xsize];

	register int i;
	for(i = 0;i < ysize;i++){
		fin.read(buf,xsize);
		fout.write(buf,xsize);
		fin.seek(xsize,XS_CUR);
		fin.read(buf,xsize);
		fout.write(buf,xsize);
		}

	delete buf;
	fin.close();
	fout.close();
}

// world ini special function
static const char* version_ = "1.4";
static const char* secGlobal = "Global Parameters";
static const char* secStorage = "Storage";
static const char* secCreation = "Creation Parameters";

static const char* STR_MAP_POWER_X="Map Power X";
static const char* STR_MAP_POWER_Y="Map Power Y";


void vrtMap::prepare(const char* _worldsDir)
{
	worldsDir=_worldsDir;
}


void vrtMap::initGeneralVariable()
{
	if(H_SIZE_POWER  > MAX_H_SIZE_POWER) ErrH.Abort("X-Size > MAX");
	if(V_SIZE_POWER  > MAX_V_SIZE_POWER) ErrH.Abort("Y-Size > MAX");

	H_SIZE = 1 << H_SIZE_POWER;
	clip_mask_x = H_SIZE - 1;
	GH_SIZE = H_SIZE >> kmGrid;
	clip_mask_x_g = GH_SIZE - 1;

	V_SIZE = 1 << V_SIZE_POWER;
	clip_mask_y = V_SIZE - 1;
	GV_SIZE = V_SIZE >> kmGrid;
	clip_mask_y_g = GV_SIZE - 1;
}

void vrtMap::releaseChAreaBuf()
{
	//if(changedT){ delete changedT; changedT = 0; }

	if(gridChAreas) { delete [] gridChAreas; gridChAreas=0; }
	if(gridChAreas2) { delete [] gridChAreas2; gridChAreas2=0; }
}

void vrtMap::allocChAreaBuf()
{
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	gridChAreas= new unsigned char[sizeGCA];
	gridChAreas2= new unsigned char[sizeGCA];
	clearGridChangedAreas();
}

void vrtMap::putNewGeoTexture(const char* fnameMatGeoTx, const char* fnameLoopedGeoTx)
{
	matGeoTxFileName=fnameMatGeoTx;
	loopedGeoTxFileName=fnameLoopedGeoTx;
	initGeoTexture();

	ColorQuantizer geoTxColorQuantizer;
	geoTxColorQuantizer.prepare4PutColor(MAX_GEO_SURFACE_TYPE);
	int i,j;
	for(i=0; i< V_SIZE; i++){
		for(j=0; j< H_SIZE; j++){
			geoTxColorQuantizer.putColor(GetGeoColor(j,i));
		}
	}
	geoTxColorQuantizer.postProcess();

	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		GeoPal[i]=geoTxColorQuantizer.pPalette[i];
	}
	convertPal2TableSurCol(GeoPal, Geologic);

	//RGBOctree rgbo(geoTxColorQuantizer);
	geoTxOctree.import(geoTxColorQuantizer);

	worldChanged=1; //т.к. регрендер можно не вызывать дублируется тут!
}

bool vrtMap::initGeoTexture()
{
	loopedGeoTx.release();
	matGeoTx.release();

	if(!matGeoTxFileName.empty()){
		TGAHEAD tgahead;
		if(!tgahead.loadHeader(matGeoTxFileName.c_str())){
			string str="Error loading geo texture-";
			str+=matGeoTxFileName;
			xxassert(0, str.c_str());
			return false;
		}
		if( tgahead.PixelDepth!=24 || tgahead.ImageType!=2 ) {
			xassert(0&&"Не поддерживаемый тип TGA (24bit не компрессованный)");
			return false;
		}
		matGeoTx.create(tgahead.Width,tgahead.Height);
		tgahead.load2RGBL(matGeoTx.sx, matGeoTx.sy, matGeoTx.pData);
	}
	if(!loopedGeoTxFileName.empty()){
		TGAHEAD tgahead;
		if(!tgahead.loadHeader(loopedGeoTxFileName.c_str())){
			string str="Error loading geo texture-";
			str+=loopedGeoTxFileName;
			xxassert(0, str.c_str());
			return false;
		}
		if( tgahead.PixelDepth!=24 || tgahead.ImageType!=2 ) {
			xassert(0&&"Не поддерживаемый тип TGA (24bit не компрессованный)");
			return false;
		}
		loopedGeoTx.create(tgahead.Width,tgahead.Height);
		tgahead.load2RGBL(loopedGeoTx.sx, loopedGeoTx.sy, loopedGeoTx.pData);
	}
	kXScaleMatGeoTx=(1<<16)*matGeoTx.sx / H_SIZE;
	kYScaleMatGeoTx=(1<<16)*matGeoTx.sy / V_SIZE;
	return true;
}

void vrtMap::create(const char* _worldName)
{
	worldName = _worldName;

	worldSerial=generateSerialNumber();

	bitmapDispatcher.release();
	bitGenDispatcher.release();

	initGeneralVariable();

	
	releaseChAreaBuf();
	allocChAreaBuf();

	RenderPrepare1();

	changedAreas.erase(changedAreas.begin(), changedAreas.end());
	renderAreas.clear();



	if(VxABuf!=0) releaseMem4Buf();
	allocMem4Buf(true);

	createGeoDamPal();
	createBeginSurface();

	//f3d.loadVariable();

	//putNewGeoTexture(geoTxFileName.c_str());
	putNewGeoTexture("","");//matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
	WorldRender();

	initGrid();

	loadLeveledTexture(); //необходимо вызывать после загрузки VxDBuf и палитры

	clearGridChangedAreas();
	///loadHardness();
	worldChanged=1;
	geoTxFileName.clear();
	//f3d.load(geoTxFileName.c_str());
	flag_worldLoaded=true;

	UndoDispatcher_KillAllUndo();
	//PreocedureMap
	flag_record_operation=true;
	clearContainerPMO();
	procMapOp=0;

	vMap.toShowTryColorDamTexture(true);
}

void vrtMap::createBeginSurface()
{
	xassert(SupBuf);
	if(createWorldMetod == FullPlain){
		int x,y;
		for(y=0; y<V_SIZE; y++){
			for(x=0; x<H_SIZE; x++){
				int offb=offsetBuf(x,y);
				VxABuf[offb]=Vm_prepHeighAtr(initialHeight<<VX_FRACTION, VmAt_Nrml_Geo);
				SurBuf[offb]=0;
				SupBuf[offb]=0;//Sur2Col32[SurBuf[offb]][255];
			}
		}
	}
	else if(Mountains){
		buildMountainWorld();
	}
}

void vrtMap::rebuild()
{

	//unsigned short* backupVxABuf=new unsigned short[H_SIZE*V_SIZE];
	//unsigned long* backupSupBuf = new unsigned long [H_SIZE*V_SIZE];
	//memcpy(backupVxABuf, VxABuf, sizeof(backupVxABuf[0])*H_SIZE*V_SIZE);
	//memcpy(backupSupBuf, SupBuf, sizeof(backupSupBuf[0])*H_SIZE*V_SIZE);
	
	bitmapDispatcher.clearCache();//release вызывать нельзя т.к. colorPic инструменты привязаны к индексам а не к имени файла!
	bitGenDispatcher.release();
	if(testExistingFile(getTargetName(worldRasterDataFileSource))) { //подложка - старый мир
		loadGeoDamPal();
		loadVMP(getTargetName(worldRasterDataFileSource), true);
		///Загрузка из палитры старого мира
		if(!loadRGBCache(getTargetName(worldRGBFileSource)) )
            setRGBCacheInSurBuf();
	}
	else { // подложка - процедурная
		createGeoDamPal();
		createBeginSurface();

		//f3d.loadVariable();

	} // else // подложка - процедурная

	playPMOperation();
	putNewGeoTexture(matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
	WorldRender();

	initGrid();
	clearGridChangedAreas();
	worldChanged=1;
}

bool vrtMap::load(const char* _worldName, bool flag_useTryColorBuffer) //SupBuf
{
	bitmapDispatcher.release();
	bitGenDispatcher.release();

	UndoDispatcher_KillAllUndo(); //Очистка всего буфера Undo-Redo

	worldName=_worldName;

	worldSerial=0;

	clearContainerPMO();
    
	XPrmIArchive ia;
	if(ia.open(getTargetName(worldDataFile))){
		ia.serialize(static_cast<vrtMapCreationParam&>(*this), "WorldHeader", 0);
		ia.serialize(*this, "WorldData", 0);
	}
	else {
		//string str=TRANSLATE("Error at loading the world-");
		string str="Error at loading the world-";
		str+=_worldName;
		xxassert(0,str.c_str());
		return false;
	}

	initGeneralVariable();

	releaseChAreaBuf();
	allocChAreaBuf();

	RenderPrepare1();

	changedAreas.clear();
	renderAreas.clear();
	changedAreas.reserve(MAX_CHANGED_RENDER_AREAS);
	renderAreas.reserve(MAX_CHANGED_RENDER_AREAS);

	delLeveledTexture();//Необходимо вызывать до удаления VxDBuf !
	if(VxABuf!=0) releaseMem4Buf();
	allocMem4Buf(flag_useTryColorBuffer);

	//if( loadVMP(getTargetName(worldRasterDataCacheFile)) && loadRGBCache(getTargetName(worldRGBCacheFile)) && DamSurBufCRC==getDamSurBufCRC() )
	bool flagNotBuildWorld=loadVMP(getTargetName(worldRasterDataCacheFile), true);
	if(flagNotBuildWorld && flag_useTryColorBuffer)
		flagNotBuildWorld&=loadRGBCache(getTargetName(worldRGBCacheFile));
	if( flagNotBuildWorld ){ //Загрузка из кэша
		loadGeoDamPal();

		//f3d.load(geoTxFileName.c_str());
		//putNewGeoTexture(matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
		initGeoTexture();

		initGrid();
		clearGridChangedAreas();
		//loadHardness();
		loadSurKind();
	}
	else { //Загрузка и компиляция мира
		rebuild();
		///f3d.load(geoTxFileName.c_str());
		unsigned int curVxABufCRC=getVxABufCRC();
		unsigned int curGeoBufCRC=getGeoSurBufCRC();
		unsigned int curDamBufCRC=getDamSurBufCRC();
		if(VxABufCRC!=curVxABufCRC){
			xassert(0&& "Сгенеренные высоты не совпадают с записанными");
		}
		if(GeoSurBufCRC!=curGeoBufCRC){
			xassert(0&& "Поверхность 3D текстуры(Geo) сгенеренного мира не совпадает с записанной");
		}
		if(DamSurBufCRC!=curDamBufCRC){
			xassert(0&& "Поверхность текстуры(Dam) сгенеренного мира не совпадает с записанной");
		}
	}

	loadLeveledTexture(); //необходимо вызывать после загрузки VxDBuf и палитры

	///elementarTool<SIM_UnSetInds, TTTE_PUT, TTSE_DRAW_DAM> tool;
	///tool.influenceDZ(0, 0, 0, 2*0, 3 );

	worldChanged=0;
	flag_worldLoaded=true;

	if(flag_useTryColorBuffer)
		vMap.toShowTryColorDamTexture(true);
	else 
		vMap.toShowTryColorDamTexture(false);
	currentGameMapCRC=getWorldCRC();
	return true;
}

void vrtMap::save(const char* _worldName, int _indexColorsUsed)
{
	worldName=_worldName;

	HRESULT resultCreateGUID=CoCreateGuid(&guid);
	xassert(resultCreateGUID==S_OK);

	//Проверка каталога на существование и создание если нет
	string path2world;
	if(worldsDir.empty()) path2world.clear();
	else path2world = worldsDir + "\\";
	path2world += worldName;

	//indexColorsUsed=convertSupBuf2SurBuf();
	if(_indexColorsUsed!=-1)
		indexColorsUsed=_indexColorsUsed;

    VxABufCRC=getVxABufCRC();
    GeoSurBufCRC=getGeoSurBufCRC();
    if(SupBuf)
		DamSurBufCRC=getDamSurBufCRC();

	WIN32_FIND_DATA FindFileData;
	HANDLE hf = FindFirstFile( path2world.c_str(), &FindFileData );
	if (hf == INVALID_HANDLE_VALUE)
		xassert( ::CreateDirectory(path2world.c_str(), NULL) );
	else
		xassert( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 );

	XPrmOArchive oa(getTargetName(worldDataFile));
	oa.serialize(static_cast<vrtMapCreationParam&>(*this), "WorldHeader", 0);
	oa.serialize(*this, "WorldData", 0);

	saveVMPCach(getTargetName(worldRasterDataCacheFile), VxABufCRC, GeoSurBufCRC, DamSurBufCRC);
	//convertMapSize(getTargetName(worldRasterDataCacheFile));
	if(SupBuf){
		TGAHEAD tgahead;
		tgahead.saveRGBL(getTargetName(worldRGBCacheFile), H_SIZE, V_SIZE, SupBuf);
	}
	saveGeoDamPal();
	saveSurKind();
	worldChanged=0;
	clearGridChangedAreas();
}

bool vrtMap::deleteWorld(const char* _worldName)
{
	//Проверка каталога на существование и создание если нет
	string path2world;
	if(worldsDir.empty()) path2world.clear();
	else path2world = worldsDir + "\\";
	path2world += _worldName;

	string filemask=path2world + "\\" + "*.*";
	BOOL result=TRUE;

	WIN32_FIND_DATA FindFileData;
	HANDLE hf = ::FindFirstFile( filemask.c_str(), &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if ( FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY ) {
				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='\0' ) continue;//Проверка на служебные записи
				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='.' && FindFileData.cFileName[2]=='\0' ) continue;
				xassert(0 && "В каталоге мира лишние каталоги!");
			}
			else {
				filemask=path2world + "\\" + FindFileData.cFileName;
				result&=::DeleteFile(filemask.c_str());
			}
		} while(::FindNextFile( hf, &FindFileData ));
		::FindClose( hf );
	}
	result&=::RemoveDirectory(path2world.c_str());
	return result;
}

void vrtMap::saveVMPCach(const char* _fname, unsigned int _crcVxA, unsigned int _crcGeo, unsigned int _crcDam)
{
	sVmpHeader VmpHeader;
	VmpHeader.setID("S5L2");
	VmpHeader.XSize=H_SIZE;
	VmpHeader.YSize=V_SIZE;
	VmpHeader.AUX1=_crcVxA;
	VmpHeader.AUX2=_crcGeo;
	VmpHeader.AUX3=_crcDam;

	XBuffer buffer(sizeof(VmpHeader) + H_SIZE*V_SIZE*sizeof(VxABuf[0]) + H_SIZE*V_SIZE*sizeof(SurBuf[0]));
	buffer.write(&VmpHeader,sizeof(VmpHeader));
	buffer.write(&VxABuf[0], H_SIZE*V_SIZE*sizeof(VxABuf[0]));
	buffer.write(&SurBuf[0], H_SIZE*V_SIZE*sizeof(SurBuf[0]));
	saveFileSmart(_fname, buffer, buffer.tell());
}

void vrtMap::releaseWorld(void)
{
	flag_worldLoaded=false;
}

void vrtMap::convertPath2WorldName(const char* _dirName)
{
	string tmp=_dirName;
	string::size_type i=tmp.find_last_of("\\");
	if(i!=string::npos){
		worldsDir=tmp.substr(0, i);
		worldName=&(tmp.c_str()[i+1]);
	}
	else{
		worldName=tmp;
		worldsDir="";
	}
}

void vrtMap::loadSurKind(void)
{
	XZipStream ff(0);
	if(ff.open(getTargetName(worldSurfaceKindFile), XS_IN) ){
		unsigned short width, height;
		ff.read(&width, sizeof(width));
		ff.read(&height, sizeof(height));
		const unsigned int KPack=2;
		if( (width*KPack!=GH_SIZE) || (height!=GV_SIZE) ){
			xassert(0&&"Wrong size surkind.bin for the world");
			return;
		}

		unsigned char * HDNBuf;
		HDNBuf=new unsigned char[width*height];
		ff.read(HDNBuf, width*height*sizeof(unsigned char));

		int i,j;
		int yOffsetHBuf=0;
		for(i=0; i<height; i++){
			for(j=0; j<width; j++){
				//KPack=2!
				int offG=offsetGBuf(j*KPack, i);
				GABuf[offG]  =( HDNBuf[yOffsetHBuf+j]   & GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND) | (GABuf[offG]  &(~GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND));
				GABuf[offG+1]=((HDNBuf[yOffsetHBuf+j]>>3)&GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND) | (GABuf[offG+1]&(~GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND));
			}
			yOffsetHBuf+=width;
		}
		delete [] HDNBuf;
		ff.close();
	}
}

void vrtMap::saveSurKind(void)
{
	const unsigned int KPack=2;
	unsigned short width, height;
	width=GH_SIZE/KPack;
	height=GV_SIZE;

	XBuffer buffer(sizeof(width) + sizeof(height) + width*height*sizeof(unsigned char));
	buffer.write(&width, sizeof(width));
	buffer.write(&height, sizeof(height));

	unsigned char * HDNBuf;
	HDNBuf=new unsigned char[width*height];

	int i,j;
	int yOffsetHBuf=0;
	for(i=0; i<height; i++){
		for(j=0; j<width; j++){
			//KPack=2!
			int offG=offsetGBuf(j*KPack, i);
			unsigned char itg=0;
			itg = GABuf[offG]  &GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND;
			itg|=(GABuf[offG+1]&GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND)<<3;
			HDNBuf[yOffsetHBuf+j]=itg;
		}
		yOffsetHBuf+=width;
	}
	buffer.write(HDNBuf, width*height*sizeof(unsigned char));

	delete [] HDNBuf;

	saveFileSmart(getTargetName(worldSurfaceKindFile), buffer, buffer.tell());
}

void vrtMap::clearGridChangedAreas(void)
{
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	//обнуление
	memset(gridChAreas, 0, sizeGCA*sizeof(*gridChAreas));
	memset(gridChAreas2, 0, sizeGCA*sizeof(*gridChAreas));
}

void vrtMap::updateGridChangedAreas2(void)
{
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);

	int vSizeGCA=(V_SIZE>>kmGridChA);
	int hSizeGCA=(H_SIZE>>kmGridChA);
	int i, j, cnt=0;
	for(i=0; i<vSizeGCA; i++){
		for(j=0; j<hSizeGCA; j++){
			if(gridChAreas[cnt]==1) gridChAreas2[cnt]|=1;
			cnt++;
		}
	}
//	memset(gridChAreas, 0, sizeGCA*sizeof(*gridChAreas));
}
bool vrtMap::checkGridChangedAreas2(void)
{
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);

	int vSizeGCA=(V_SIZE>>kmGridChA);
	int hSizeGCA=(H_SIZE>>kmGridChA);
	int i, j, cnt=0;
	for(i=0; i<vSizeGCA; i++){
		for(j=0; j<hSizeGCA; j++){
			if(gridChAreas2[cnt]==1) 
				return 1;
			cnt++;
		}
	}
	return 0;
}
bool vrtMap::loadVMP(const char* file, bool flag_NoCheckCRC)
{
	XZipStream fmap(0);
	sVmpHeader VmpHeader;

	if(!fmap.open(file, XS_IN))
		return 0;

	fmap.seek(0,XS_BEG);
	fmap.read(&VmpHeader,sizeof(VmpHeader));
	if(VmpHeader.cmpID("S2T0")){
		unsigned char* pBufG=new unsigned char[H_SIZE];
		unsigned char* pBufD=new unsigned char[H_SIZE];
		unsigned char* pBufA=new unsigned char[H_SIZE];
		int i, j;
		for(i=0; i<V_SIZE; i++){
			fmap.seek(sizeof(VmpHeader)+i*H_SIZE, XS_BEG);
			fmap.read(pBufG,H_SIZE);

			fmap.seek(sizeof(VmpHeader)+i*H_SIZE+ H_SIZE*V_SIZE, XS_BEG);
			fmap.read(pBufD, H_SIZE);

			fmap.seek(sizeof(VmpHeader)+i*H_SIZE+ H_SIZE*V_SIZE*2,XS_BEG);
			fmap.read(pBufA, H_SIZE);

			for(j=0; j<H_SIZE; j++){
				unsigned short data;
				if(pBufD[j]==0) //geo
					data=Vm_prepHeighAtr( ((pBufG[j]<<VX_FRACTION)|(pBufA[j]&VX_FRACTION_MASK)) , VmAt_Nrml_Geo);
				else //dam
					data=Vm_prepHeighAtr( ((pBufD[j]<<VX_FRACTION)|(pBufA[j]&VX_FRACTION_MASK)) , VmAt_Nrml_Dam);
				VxABuf[offsetBuf(j,i)]=data;
			}
		}
		fmap.seek(sizeof(VmpHeader)+ H_SIZE*V_SIZE*3, XS_BEG);
		fmap.read(&SurBuf[0], H_SIZE*V_SIZE);
	}
	else if(VmpHeader.cmpID("S5L2")) {
		if(!flag_NoCheckCRC){
			xassert(VmpHeader.XSize==H_SIZE && VmpHeader.YSize==V_SIZE ); //надо перезаписать промежуточную версию формата vmp в итоговую
			if(VmpHeader.AUX1!=VxABufCRC || VmpHeader.AUX2!=GeoSurBufCRC /*|| VmpHeader.AUX3!=DamSurBufCRC*/)
				return 0;
		}
		VxABufCRC=VmpHeader.AUX1; GeoSurBufCRC=VmpHeader.AUX2; DamSurBufCRC=VmpHeader.AUX3;
		fmap.read(&VxABuf[0], H_SIZE*V_SIZE*sizeof(VxABuf[0]));
		fmap.read(&SurBuf[0], H_SIZE*V_SIZE*sizeof(SurBuf[0]));
	}
	else ErrH.Abort("VMP file is not TRUE version");

	return 1;
}

bool vrtMap::loadRGBCache(const char* fname)
{
	xassert(SupBuf);
	TGAHEAD tgahead;
	if( tgahead.loadHeader(fname) ){
		if((tgahead.PixelDepth!=24) || (tgahead.ImageType!=2)) {
			///AfxMessageBox("Не поддерживаемый тип TGA (необходим 24bit не компрессованный)");
			xassert(0&&"Не поддерживаемый тип TGA (необходим 24bit не компрессованный)");
			return false;
		}
		tgahead.load2RGBL(H_SIZE, V_SIZE, SupBuf);
		return true;
	}
	else 
		return false;
}

void vrtMap::setRGBCacheInSurBuf()
{
	xassert(SupBuf);
	///Загрузка из палитры
	int y,x;
	for(y=0; y<V_SIZE; y++){
		for(x=0; x<H_SIZE; x++){
			int off=offsetBufC(x,y);
			SupBuf[off]=Sur2Col32[SurBuf[off]][255];
		}
	}
}

void vrtMap::saveMiniMap(int sizex, int sizey)
{
	unsigned long* pMap=new unsigned long[sizex*sizey];

	int stepXVM=H_SIZE/sizex;
	int stepYVM=V_SIZE/sizex;
	int stepPoints=stepXVM*stepYVM;
	int i,j,cnt=0;
	for(i=0; i<V_SIZE; i+=stepYVM){
		for(j=0; j<H_SIZE; j+=stepXVM){
            int k,m;
			int r=0,g=0,b=0;
			for(k=0; k<stepYVM; k++){
				for(m=0; m<stepXVM; m++){
					int color=getColor32(m+j, k+i);
					r+=(color>>16)&0xFF;
					g+=(color>>8)&0xFF;
					b+=color&0xFF;
				}
			}
			pMap[cnt]= ((r/stepPoints)<<16)  |  ((g/stepPoints)<<8) | (b/stepPoints);
			cnt++;
		}
	}

	TGAHEAD tgahead;
	tgahead.saveRGBL(getTargetName(worldMiniMapFile), sizex, sizey, pMap);
}

// {125A9A29-7CB3-4083-A14A-F67C5F9FB527}
static const GUID worldSurfaceChangeFileGUID = 
{ 0x125a9a29, 0x7cb3, 0x4083, { 0xa1, 0x4a, 0xf6, 0x7c, 0x5f, 0x9f, 0xb5, 0x27 } };

bool vrtMap::saveGameMap(Archive& ar)
{
	xassert(ar.isOutput());

	updateGridChangedAreas2();
	if(!checkGridChangedAreas2()){ //случай, когда нет изменений на игровом мире
		worldChanged=0;
		return true;
	}
	if(!worldChanged) // случай, когда не менялся игровой мир после загрузки
		return true;
	XBuffer ff;
	//запись идентификатора 
	ff.write(&worldSurfaceChangeFileGUID, sizeof(GUID));
	//Запись серийного номера мира
	ff.write(&worldSerial, sizeof(worldSerial));
	//Запись crc мира
	ff.write(&VxABufCRC, sizeof(VxABufCRC));
	//Запись размера мира
	ff.write(&H_SIZE, sizeof(H_SIZE));
	ff.write(&V_SIZE, sizeof(V_SIZE));
	//Запись таблицы измененных областей
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	ff.write(gridChAreas2, sizeGCA*sizeof(unsigned char));
	//запись измененных тайлов
	int vSizeGCA=(V_SIZE>>kmGridChA);
	int hSizeGCA=(H_SIZE>>kmGridChA);
	unsigned short buf[sizeCellGridCA*sizeCellGridCA];
	unsigned char* pBufChar=(unsigned char*)buf;
	unsigned short* pBufShort=buf;
	int i, j, cnt=0;
	for(i=0; i<vSizeGCA; i++){
		for(j=0; j<hSizeGCA; j++){
			if(gridChAreas2[cnt]==1){
				int k, m, cnt2=0;
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						pBufShort[cnt2]=VxABuf[offB+m];
						cnt2++;
					}
				}
				ff.write(pBufShort, sizeCellGridCA*sizeCellGridCA*sizeof(short));
				cnt2=0;
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						pBufChar[cnt2]=SurBuf[offB+m];
						cnt2++;
					}
				}
				ff.write(pBufChar, sizeCellGridCA*sizeCellGridCA*sizeof(char));

			}
			cnt++;
		}
	}

	//saveGrid(ff);
	char flag_taller_h_zp=0;
	for(i=0; i<(GV_SIZE); i++){
		ff.write(&GVBuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(GVBuf[0]));
		ff.write(&GABuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(GABuf[0]));
	}

	ar.serialize(ff, "gameMap", 0);

	worldChanged=0;
	return true;
}

bool vrtMap::loadGameMap(Archive& ar)
{
	xassert(ar.isInput());

	XBuffer ff;
	if(!ar.serialize(ff, "gameMap", 0))
		return false;
    ff.set(0);
	
	//запись идентификатора 
	GUID fguid;
	ff.read(&fguid, sizeof(GUID));
	//if(worldSurfaceChangeFileGUID!=fguid) ErrH.Abort("Incorrect game map file(id)!:", XERR_USER, 0, fname);
	if(worldSurfaceChangeFileGUID!=fguid){
		xassert(0&&"Incorrect game map file(id)!");
		return false;
	}
	//Запись серийного номера мира
	unsigned long loadGameMapSerial;
	xassert(sizeof(loadGameMapSerial)==sizeof(worldSerial));
	ff.read(&loadGameMapSerial, sizeof(worldSerial));
	xassert(loadGameMapSerial==worldSerial);
	//Запись crc мира
	unsigned long loadGameMapCRC;
	xassert(sizeof(loadGameMapCRC)==sizeof(VxABufCRC));
	ff.read(&loadGameMapCRC, sizeof(VxABufCRC));
	//xassert(loadGameMapCRC==VxABufCRC);
	if(loadGameMapCRC!=VxABufCRC) { kdWarning("IvnAR", "loadGameMapCRC!=VxABufCRC");; }//вместо xassert
	//Запись размера мира
	int loadGameMapHSize, loadGameMapVSizel;
	xassert(sizeof(loadGameMapHSize)==sizeof(H_SIZE) && sizeof(loadGameMapVSizel)==sizeof(V_SIZE));
	ff.read(&loadGameMapHSize, sizeof(H_SIZE));
	ff.read(&loadGameMapVSizel, sizeof(V_SIZE));
	//if(loadGameMapHSize!=H_SIZE || loadGameMapVSizel!=V_SIZE) ErrH.Abort("Incorrect game map file(size)!:", XERR_USER, 0, fname);
	if(loadGameMapHSize!=H_SIZE || loadGameMapVSizel!=V_SIZE) {
		xassert(0&&"Incorrect game map file(size)!");
		return false;
	}
	//Чтение таблицы измененных областей
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	ff.read(gridChAreas, sizeGCA*sizeof(unsigned char));
	//Чтение записанных измененных тайлов
	int vSizeGCA=(V_SIZE>>kmGridChA);
	int hSizeGCA=(H_SIZE>>kmGridChA);
	unsigned short buf[sizeCellGridCA*sizeCellGridCA];
	unsigned char* pBufChar=(unsigned char*)buf;
	unsigned short* pBufShort=buf;
	int i, j, cnt=0;
	for(i=0; i<vSizeGCA; i++){
		for(j=0; j<hSizeGCA; j++){
			if(gridChAreas[cnt]==1){
				int k, m, cnt2=0;
				ff.read(pBufShort, sizeCellGridCA*sizeCellGridCA*sizeof(short));
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						VxABuf[offB+m]=pBufShort[cnt2];
						cnt2++;
					}
				}
				cnt2=0;
				ff.read(pBufChar, sizeCellGridCA*sizeCellGridCA*sizeof(char));
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						SurBuf[offB+m]=pBufChar[cnt2];
						cnt2++;
					}
				}

			}
			cnt++;
		}
	}

	//loadGrid(ff);
	char flag_taller_h_zp=0;
	for(i=0; i<(GV_SIZE); i++){
		ff.read(&GVBuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(GVBuf[0]));
		ff.read(&GABuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(GABuf[0]));
		//for(j=0; j<GH_SIZE; j++){
		//	GABuf[offsetGBuf(j,i)] &= ~(GRIDAT_BUILDING|GRIDAT_BASE_OF_BUILDING_CORRUPT);
		//}
	}

	currentGameMapCRC=getWorldCRC();
	worldChanged=0;
	
	WorldRender();

	return true;
}


typedef unsigned int typeAmountCellChAreas;
typedef unsigned short typeCoordinatChAreas;
const int MAX_COORDINAT_CHAREAS=USHRT_MAX;
void vrtMap::generateChAreasInformation(XBuffer& out)
{
	unsigned int save_begin_position=out.tell();
	typeAmountCellChAreas amountCellChAreas=0;
	out.write(&amountCellChAreas, sizeof(amountCellChAreas));

	int vSizeGChA=(V_SIZE>>kmGridChA);
	int hSizeGChA=(H_SIZE>>kmGridChA);


	///unsigned char buffer[sizeCellGridCA*sizeCellGridCA];
	typeCoordinatChAreas i, j;
	unsigned int cnt=0;
	for(i=0; i<vSizeGChA; i++){
		for(j=0; j<hSizeGChA; j++){
			if(gridChAreas[cnt]==1){
				gridChAreas2[cnt]|=1;
				amountCellChAreas++;
				out.write(&j, sizeof(j));//x
				out.write(&i, sizeof(i));//y
				int k;
				unsigned int crc=startCRC32;
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					crc=crc32((unsigned char*)&VxABuf[offB], sizeCellGridCA*sizeof(VxABuf[0]), crc);
					crc=crc32(&SurBuf[offB], sizeCellGridCA*sizeof(SurBuf[0]), crc);
				}

				crc=~crc;
				out.write(&crc, sizeof(crc));//CRC
			}
			cnt++;
		}
	}
	*((typeAmountCellChAreas*)(out.buffer()+save_begin_position))=amountCellChAreas;
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	memset(gridChAreas, 0, sizeGCA*sizeof(*gridChAreas));
}

unsigned int vrtMap::getChAreasInformationCRC() 
{ 
	XBuffer buf(256, 1);
	generateChAreasInformation(buf);
	return crc32((unsigned char*)buf.buffer(), buf.tell(), startCRC32); 
}


void vrtMap::compareChAreasInformation(unsigned char* pFirstCAI, unsigned char* pSecondCAI, XBuffer& textOut, XBuffer& binOut)
{
	typeAmountCellChAreas sizeFirst= *((typeAmountCellChAreas*)pFirstCAI);
	pFirstCAI+=sizeof(sizeFirst);
	typeAmountCellChAreas sizeSecond= *((typeAmountCellChAreas*)pSecondCAI);
	pSecondCAI+=sizeof(sizeSecond);

	typeCoordinatChAreas curFtX, curFtY, curSdX, curSdY;
	unsigned int fcrc, scrc;

	bool flag_get_first=1;
	bool flag_get_second=1;
	while(sizeFirst || sizeSecond){
		if(flag_get_first){
			if(sizeFirst){
				curFtX=*((typeCoordinatChAreas*)pFirstCAI);
				pFirstCAI+=sizeof(typeCoordinatChAreas);
				curFtY=*((typeCoordinatChAreas*)pFirstCAI);
				pFirstCAI+=sizeof(typeCoordinatChAreas);
				fcrc=*((unsigned int*)pFirstCAI);
				pFirstCAI+=sizeof(unsigned int);
				sizeFirst--;
			}
			else {
				curFtX=MAX_COORDINAT_CHAREAS;
				curFtY=MAX_COORDINAT_CHAREAS;
			}
			flag_get_first=0;
		}
		if(flag_get_second){
			if(sizeSecond){
				curSdX=*((typeCoordinatChAreas*)pSecondCAI);
				pSecondCAI+=sizeof(typeCoordinatChAreas);
				curSdY=*((typeCoordinatChAreas*)pSecondCAI);
				pSecondCAI+=sizeof(typeCoordinatChAreas);
				scrc=*((unsigned int*)pSecondCAI);
				pSecondCAI+=sizeof(unsigned int);
				sizeSecond--;
			}
			else {
				curSdX=MAX_COORDINAT_CHAREAS;
				curSdY=MAX_COORDINAT_CHAREAS;
			}
			flag_get_second=0;
		}
		int x=-1, y=-1;
		bool flag_crc_notequal=0;
		if(curFtY==curSdY){
			if(curFtX==curSdX){
				if(fcrc!=scrc){
					x=curFtX; y=curFtY;
					flag_crc_notequal=1;
				}
				flag_get_first=1;
				flag_get_second=1;
			}
			else if(curFtX<curSdX){
				x=curFtX; y=curFtY;
				// load next first
				flag_get_first=1;
			}
			else {//curFtX>curSdX
				x=curSdX; y=curSdY;
				// load next second
				flag_get_second=1;
			}
		}
		else if(curFtY<curSdY){
			x=curFtX; y=curFtY;
			// load next first
			flag_get_first=1;
		}
		else {//curFtY>curSdY
			x=curSdX; y=curSdY;
			// load next second
			flag_get_second=1;
		}

		if(x!=-1) {
			if(flag_crc_notequal){
				textOut < "Не совпадает CRC в квадратах ";
			}
			else {
				textOut < "Не совпадают квадраты ";
			}
			textOut < "X=" <= (x<<kmGridChA) <"+" <=(sizeCellGridCA)  < " Y=" <= (y<<kmGridChA) <"+" <=(sizeCellGridCA) < "\r\n";
			binOut < x < y;
		}
	}
}

void vrtMap::displayChAreas(unsigned char* pd, unsigned int dsize)
{
	int size=dsize/sizeof(int);
	if( (size&0x01) || (dsize%sizeof(int)) ){
		xassert(0 && "Не корректный размер несовпадающей зоны");
		return;
	}
	size/=2;

	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	pTempArray=new bool[sizeGCA];
	memset(pTempArray, 0, sizeGCA);

	int vSizeGChA=(V_SIZE>>kmGridChA);
	int hSizeGChA=(H_SIZE>>kmGridChA);
	int x,y;
	for(; size>0; size--){
		x = *((int*)pd);
		pd+=sizeof(int);
		y = *((int*)pd);
		pd+=sizeof(int);
		pTempArray[x+y*hSizeGChA]=1;
	}
	//toShowHardness(true);
	WorldRender();

}

unsigned long vrtMap::getWorldCRC(void)
{
	unsigned int y;
	unsigned int offB=0;
	unsigned int crc=startCRC32;
	unsigned char * pBufTMP=new unsigned char [H_SIZE];
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&VxABuf[offB], H_SIZE*sizeof(VxABuf[0]), crc);
		crc=crc32(&SurBuf[offB], H_SIZE, crc);
		offB+=H_SIZE;
	}
	delete [] pBufTMP;
	crc=getGridCRC(true, 0, crc);
	crc=~crc;
	return crc;
}

unsigned int vrtMap::getGridCRC(bool fullGrid, int cnt, unsigned int beginCRC)
{
	unsigned int begAdrScan, sizeScan;
	const int sizeGrid=GV_SIZE*GH_SIZE;
	if(!fullGrid){
		const int PARTS=8;
		const int sizePart=sizeGrid/PARTS;
		int curPart=cnt%PARTS;
		begAdrScan=curPart*sizePart;
		sizeScan=sizePart;
	}
	else {
		begAdrScan=0;
		sizeScan=sizeGrid;
	}
	unsigned int crc=beginCRC;
	crc=crc32(&GVBuf[begAdrScan], sizeScan*sizeof(GVBuf[0]), crc);
	crc=crc32((unsigned char*)(&GABuf[begAdrScan]), sizeScan*sizeof(GABuf[0]), crc);
	crc=~crc;
	return crc;
}

unsigned long vrtMap::getVxABufCRC(void)
{
	unsigned int y;
	unsigned int offB=0;
	unsigned long crc=startCRC32;
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&VxABuf[offB], H_SIZE*sizeof(VxABuf[0]), crc);
		offB+=H_SIZE;
	}
	crc=~crc;
	return crc;
}
unsigned long vrtMap::getGeoSurBufCRC(void)
{
	unsigned int y,x;
	unsigned int offB=0;
	unsigned int crc=startCRC32;
	unsigned char * pBufTMP=new unsigned char [H_SIZE];
	for(y=0; y<V_SIZE; y++){
		unsigned char *pBuf=pBufTMP;
		for(x=0; x<H_SIZE; x++){
			if(Vm_IsGeo(GetAtr(offB+x))){
				*pBuf++=GetTer(offB+x);
			}
		}
		crc=crc32(pBufTMP, pBuf-pBufTMP, crc);
		offB+=H_SIZE;
	}
	delete [] pBufTMP;
	crc=~crc;
	return crc;
}

unsigned long vrtMap::getDamSurBufCRC(void)
{
	xassert(SupBuf);
	unsigned int y;
	unsigned int offB=0;
	unsigned int crc=startCRC32;
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&SupBuf[offB], H_SIZE*sizeof(SupBuf[0]), crc);
		offB+=H_SIZE;
	}
	crc=~crc;
	return crc;
}


void vrtMap::convertPal2TableSurCol(const sColor4c palBuf[SIZE_GEO_PALETTE], eSurfaceMode SurfaceMode)
{
	int SurBufOffset=0;
	switch (SurfaceMode){
	case Geologic:
		SurBufOffset=0;
		break;
	case Damming:
		SurBufOffset=128;
		break;
	}
	double Lum=1.5;//1.5;
	//double Lum0=0.2;
	double Lum0=0.05;
	double dLum=(Lum-Lum0)/(double)MAX_SURFACE_LIGHTING;
	unsigned char R,G,B;
	float max_lighting=0;
	for(int i=0; i<MAX_SURFACE_TYPE; i++){ //MAX_SURFACE_TYPE =256
		R =palBuf[i].r;
		G =palBuf[i].g;
		B =palBuf[i].b;
		if(SurfaceMode==Damming){
			float lighting=0.3*(float)R + 0.59*(float)G + 0.11*(float)B;
			if(max_lighting <= lighting){
				max_lighting=lighting;
				veryLightDam=i;
			}
		}
		for(int j=0; j<MAX_SURFACE_LIGHTING; j++){ //MAX_SURFACE_LIGHTING =127
			unsigned short col;
			unsigned int col32;
			double cb,cg,cr;
			cb=B*(Lum0+dLum*(double)j); if(cb<0)cb=0; if(cb>255)cb=255;
			cg=G*(Lum0+dLum*(double)j); if(cg<0)cg=0; if(cg>255)cg=255;
			cr=R*(Lum0+dLum*(double)j); if(cr<0)cr=0; if(cr>255)cr=255;
			col =(round(cb)  >>3)&0x1F;
			col+=(round(cg)  <<3)&0x7E0;
			col+=(round(cr)  <<8)&0x0F800;
			Sur2Col[i][j+SurBufOffset]=col;
			col32=round(cb); col32+=round(cg)<<8; col32+=round(cr)<<16;
			Sur2Col32[i][j+SurBufOffset]=col32;
			//ВРЕМЕННО !!!! (для совместимости с отсутствием текстуры выровненной поверхности)
			Tex2Col[j+SurBufOffset][i]=col;
			Tex2Col32[j+SurBufOffset][i]=col32;
		}
	}
}

void vrtMap::createGeoDamPal(void)
{
	int i;
	for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
		GeoPal[i]=sColor4c(128,128,0);
	}
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		DamPal[i]=sColor4c(128,128,128);
	}
	convertPal2TableSurCol(GeoPal,Geologic);
	convertPal2TableSurCol(DamPal, Damming);
}
void vrtMap::loadGeoDamPal(void)
{
	int i;
	XZipStream ff(getTargetName(worldGeoPalFile), XS_IN);
	for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
		unsigned long color=0;
		ff.read(&color, 3); //3-rgb
		GeoPal[i] = sColor4c(color);
	}
	ff.close();

	convertPal2TableSurCol(GeoPal,Geologic);
	//
	XZipStream ff1(getTargetName(worldDamPalFile), XS_IN);
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		unsigned long color=0;
		ff1.read(&color, 3); //3-rgb
		DamPal[i] = sColor4c(color);
	}
	ff1.close();

	convertPal2TableSurCol(DamPal, Damming);
}

void vrtMap::saveGeoDamPal(void)//unsigned char palBuf[SIZE_GEO_PALETTE] //Bufer в формате BGR
{
	int i;
	XBuffer surBuffer(MAX_GEO_SURFACE_TYPE*3);
	for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
		unsigned long color=GeoPal[i].argb;
		surBuffer.write(&color, 3); //3-rgb
	}
	saveFileSmart(getTargetName(worldGeoPalFile), surBuffer, surBuffer.tell());

	XBuffer damBuffer(MAX_DAM_SURFACE_TYPE*3);
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		unsigned long color=DamPal[i].argb;
		damBuffer.write(&color, 3); //3-rgb
	}
	saveFileSmart(getTargetName(worldDamPalFile), damBuffer, damBuffer.tell());
}

void vrtMap::convertPal2TableTexCol()
{
	double Lum=1.5;//1.5;
	//double Lum0=0.2;
	double Lum0=0.05;
	double dLum=(Lum-Lum0)/(double)MAX_SURFACE_LIGHTING;
	unsigned char R,G,B;
	for(int i=0; i<MAX_SURFACE_TYPE; i++){ //MAX_SURFACE_TYPE =256
		R =TexPal[i*3 + 0];
		G =TexPal[i*3 + 1];
		B =TexPal[i*3 + 2];
		for(int j=0; j<MAX_SURFACE_LIGHTING; j++){ //MAX_SURFACE_LIGHTING =127
			unsigned short col;
			unsigned int col32;
			double cb,cg,cr;
			cb=B*(Lum0+dLum*(double)j); if(cb<0)cb=0; if(cb>255)cb=255;
			cg=G*(Lum0+dLum*(double)j); if(cg<0)cg=0; if(cg>255)cg=255;
			cr=R*(Lum0+dLum*(double)j); if(cr<0)cr=0; if(cr>255)cr=255;
			col =(round(cb)  >>3)&0x1F;
			col+=(round(cg)  <<3)&0x7E0;
			col+=(round(cr)  <<8)&0x0F800;
			Tex2Col[j][i]=Tex2Col[j+128][i]=col;
			col32=round(cb); col32+=round(cg)<<8; col32+=round(cr)<<16;
			Tex2Col32[j][i]=Tex2Col32[j+128][i]=col32;
		}
	}
}

void vrtMap::restore3Buf(void)
{
	XStream fmap(0);
	sVmpHeader VmpHeader;

	UndoDispatcher_KillAllUndo(); //Очистка всего буфера Undo-Redo
	loadGeoDamPal();

	fmap.open(getTargetName(worldRasterDataFileSource),XS_IN | XS_OUT);
	fmap.seek(0,XS_BEG);
	fmap.read(&VmpHeader,sizeof(VmpHeader));
	if (VmpHeader.cmpID("S5L2")){
		int i;
		for(i=0;i<V_SIZE;i++){
			//if(changedT[i]){
				fmap.seek(sizeof(VmpHeader)+i*H_SIZE*sizeof(VxABuf[0]), XS_BEG);
				fmap.read(&VxABuf[i*H_SIZE], H_SIZE);
			//}
		}
		for(i=0;i<V_SIZE;i++){
			//if(changedT[i]){
				//if(i>0)changedT[i-1]=1; //Для правильного рендера т.к. при рендере используются 2 строки
				fmap.seek(sizeof(VmpHeader)+i*H_SIZE + H_SIZE*V_SIZE*2, XS_BEG);
				fmap.read(&SurBuf[i*H_SIZE], H_SIZE);
			//}
		}
	}
	else ErrH.Abort("Invalid VMP file ");

	loadGeoDamPal();
}



//Необходимо вызывать до удаления VxDBuf!!!
void vrtMap::delLeveledTexture(void)
{
	if(SurBuf!=LvdTex){
		if(LvdTex!=0) delete [] LvdTex;
	}
	LvdTex=0;
}

//необходимо вызывать после загрузки VxDBuf и палитры
int vrtMap::loadLeveledTexture(void)
{
	// По умолчанию для отображения DAM слоя
	LvdTex=SurBuf;
	LvdTex_X_SIZE_POWER=H_SIZE_POWER;
	LvdTex_Y_SIZE_POWER=V_SIZE_POWER;

	LvdTex_clip_mask_x=(1<<LvdTex_X_SIZE_POWER)-1;
	LvdTex_clip_mask_y=(1<<LvdTex_Y_SIZE_POWER)-1;
	LvdTex_spec_clip_mask_y=LvdTex_clip_mask_y<<LvdTex_Y_SIZE_POWER;
	LvdTex_SPEC_SHIFT_X=H_SIZE_POWER-LvdTex_X_SIZE_POWER;
	LvdTex_SPEC_SHIFT_Y=(V_SIZE_POWER-LvdTex_Y_SIZE_POWER) + LvdTex_SPEC_SHIFT_X;


	TGAHEAD thead;

	thead.init();
	XZipStream ff(0);
	if(!ff.open(getTargetName(worldLeveledTextureFile), XS_IN)) {
		string fp="Scripts\\Resource\\Textures\\";
		fp+=worldLeveledTextureFile;
		if(!ff.open(fp.c_str(), XS_IN)) {
			//xassert(0 && "loadLeveledTexture: file not found");
			return 0;
		}
	}

	ff.read(&thead,sizeof(TGAHEAD));
	if(!thead.ColorMapType || thead.CMapDepth!=24) {
		xassert(0 && "loadLeveledTexture: texture not 256 color");
		return 0;
	}

	if( (thead.Width > H_SIZE) || (thead.Height > V_SIZE) ) {
		xassert(0 && "loadLeveledTexture: size texture > size map");
		return 0; //Ограничение на размер связано с лишней проверкой при выборке цвета по offset
	}

	int powerWidth=BitSR(thead.Width);
	int powerHeight=BitSR(thead.Height);
	if( (thead.Width!=(1<<powerWidth)) || (thead.Height!=(1<<powerHeight)) ) {
		xassert(0 && "loadLeveledTexture: not multiple to two a degree a size");
		return 0;
	}

	LvdTex_X_SIZE_POWER=powerWidth;
	LvdTex_Y_SIZE_POWER=powerHeight;

	delLeveledTexture();
	LvdTex=new unsigned char[(1<<LvdTex_X_SIZE_POWER)*(1<<LvdTex_Y_SIZE_POWER)];

	LvdTex_clip_mask_x=(1<<LvdTex_X_SIZE_POWER)-1;
	LvdTex_clip_mask_y=(1<<LvdTex_Y_SIZE_POWER)-1;
	LvdTex_spec_clip_mask_y=LvdTex_clip_mask_y<<LvdTex_Y_SIZE_POWER;
	LvdTex_SPEC_SHIFT_X=H_SIZE_POWER-LvdTex_X_SIZE_POWER;
	LvdTex_SPEC_SHIFT_Y=(V_SIZE_POWER-LvdTex_Y_SIZE_POWER) + LvdTex_SPEC_SHIFT_X;

	//CMapStart=0;CMapLenght=0;CMapDepth=0;
	//unsigned char palRGB[256*3];
	unsigned char SurPal[MAX_DAM_SURFACE_TYPE*3];
	ff.read(&SurPal[thead.CMapStart*3],thead.CMapDepth*thead.CMapLenght>>3);

	unsigned char* line = new unsigned char[thead.Width], *p;
	int ibeg,jbeg,iend,jend,ik,jk,i,j;
	if(thead.ImageDescriptor&0x20) { jbeg=0; jend=thead.Height; jk=1;}
	else { jbeg=thead.Height-1; jend=-1; jk=-1;}
	if((thead.ImageDescriptor&0x10)==0) { ibeg=0; iend=thead.Width; ik=1;}
	else { ibeg=thead.Width-1; iend=-1; ik=-1;}
	bool flag_err_ovewrflow=0;
	for(j=jbeg; j!=jend; j+=jk){
		p = line;
		int off=j*thead.Width;
		ff.read(line, thead.Width);
		for(i=ibeg; i!=iend; i+=ik, p++){
			unsigned char sur=(*p);
			LvdTex[off+i]=sur;
		}
	}
	ff.close();
	delete line;


	//Конвертирование палитры из BGR в RGB
	for(i=0; i< MAX_DAM_SURFACE_TYPE; i++){
		TexPal[i*3+0]=SurPal[(i)*3+2];
		TexPal[i*3+1]=SurPal[(i)*3+1];
		TexPal[i*3+2]=SurPal[(i)*3+0];
	}
	convertPal2TableTexCol();

	return 1;
}

////////// AUX ///////////////////////////
static int* LineTable = 0;
static int LineTableLenght;
void calcLineTable(int curr_lenght,int k_vu,int base_step,int up_step)
{
	if(!LineTable){
		LineTableLenght = 1024;
		if(!(LineTableLenght & 1))
			LineTableLenght++;
		memset(LineTable = new int[LineTableLenght],0,sizeof(int)*LineTableLenght);
		}
	int fv = (-k_vu)*curr_lenght/2 + (1 << 15);
	int old_v,v;
	int v_diff;

	int* c = LineTable;
	old_v = (fv >> 16)*up_step;
	for(int u = 0;u <= curr_lenght;u++){
		fv += k_vu;
		v = (fv >> 16)*up_step;
		v_diff = v - old_v + base_step;
		old_v = v;
		*c++ = v_diff;
		}
}


unsigned char* convert_vox2vid(int vox, char* buf)
{
	int fraction,cel;
	if(vox>=0){
		fraction=vox & VX_FRACTION_MASK;
		cel=vox>>VX_FRACTION;
		sprintf(buf,"%4hi.%02hu\0",cel,fraction);
	}
	else {
		fraction= (-vox) & VX_FRACTION_MASK; //Дробную часть надо показывать без знака
		cel=(-vox)>>VX_FRACTION;				
		sprintf(buf,"-%04hi.%02hu\0",cel,fraction);
	}
	return (unsigned char*)buf;
}
int convert_vid2vox(char* buf)
{
	char cc[10]={'0','0','\0'};
	int fraction=0;
	short cels=0;
	sscanf(buf,"%hd%*c%s",&cels,cc);
	float Znak=0;
	sscanf(buf,"%f",&Znak);
	if(cc[1]==0) cc[1]='0';
	cc[2]=0;
	fraction=atoi(cc);
	if(fraction>VX_FRACTION_MASK)fraction=VX_FRACTION_MASK;
	int cel=(int)cels; //Необходимо т.к. 
	int vox;
	if (Znak>=0) vox= (cel<<VX_FRACTION) | (fraction);
	else {
		vox= ((-cel)<<VX_FRACTION) | (fraction); //Дробная часть знака не имеет
		vox=-vox;
	}
	return vox;
}

void vrtMap::drawTile(char* Texture,unsigned long pitch,int xstart,int ystart,int xend,int yend,int step)
{
	if(step!=1) return;
	for(int y = ystart; y < yend; y += step)
	{
		DWORD* tx=(DWORD*)Texture;
		int yy=max(0,y);
		for (int x = xstart; x < xend; x += step)
		{
			int xx=max(0,x);
			DWORD color=vMap.getColor32(xx,yy)|0xFF000000;
			*tx = color;
			tx++;
		}
		Texture += pitch;
	}
}

bool vrtMap::isCircleSurfaceLeveled(const Vect2i& center, const int radius){
	short xC=center.x>>kmGrid;
	short yC=center.y>>kmGrid;
	short rG=radius>>kmGrid;

	if(rG > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in isCircleSurfaceLeveled");
		rG=MAX_RADIUS_CIRCLEARR;
	}

	unsigned short a=0xffFF;

	for(int i = 0;i <= rG;i++){
		int maxr = maxRad[i];
		int* xx = xRad[i];
		int* yy = yRad[i];
		for(int j = 0;j < maxr; j++) {
			a&=GABuf[offsetGBufC(xC+xx[j], yC+yy[j])];
		}
	}
	if(a&GRIDAT_LEVELED) 
		return true;
	else 
		return false;
}



///////////////////////////////////////////////////////////

float vrtMap::analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized)
{
	int Sz = 0;
	int Sxz = 0;
	int Syz = 0;
	int delta = max(w2m(radius), 1);
	int xc = clamp(w2m(center.x), delta, GH_SIZE - delta - 1);
	int yc = clamp(w2m(center.y), delta, GV_SIZE - delta - 1);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			int z = GVBuf[offsetGBuf(x + xc, y + yc)];
			Sz += z;
			Sxz += x*z;
			Syz += y*z;
		}
	}

	float t14 = delta*delta;
	float t13 = 2.0f*delta+1.0f;
	float t9 = 2.0f*delta;
	float t8 = 6.0f*t14;
	float N = 4.0f*t14+t9+t13;
	float A = 3.0f*Sxz/delta/(t9+1.0f+t8+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	float B = 3.0f*Syz/delta/(t8+t13+(3.0f+(4.0f*delta+2.0f)*delta)*delta);
	normalNonNormalized.set(-A, -B, 4);
	return (float)Sz/N;
}

void vrtMap::findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax)
{
	zMin = 1000;
	zMax = 0;
	int delta = max(w2m(radius), 1);
	int xc = clamp(w2m(center.x), delta, GH_SIZE - delta - 1);
	int yc = clamp(w2m(center.y), delta, GV_SIZE - delta - 1);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			int z = GVBuf[offsetGBuf(x + xc, y + yc)];
			zMin = min(zMin, z);
			zMax = max(zMax, z);
		}
	}
}

bool vrtMap::checkUndestructability(const Vect2i& center, int radius)
{
	int delta = max(w2m(radius), 1);
	int xc = clamp(w2m(center.x), delta, GH_SIZE - delta - 1);
	int yc = clamp(w2m(center.y), delta, GV_SIZE - delta - 1);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			if(GRIDAT_INDESTRUCTABILITY & GABuf[offsetGBuf(x + xc, y + yc)])
				return true;
		}
	}
	return false;
}


bool vrtMap::getListWorldInfo(list<sWorldInfo>& worldinfoList)
{
	WIN32_FIND_DATA FindFileData;
	XBuffer str;
	if(!worldsDir.empty()) str < worldsDir.c_str() < "\\";
	else str <".\\";
	str < "*";

	HANDLE hf = FindFirstFile( str, &FindFileData );
	if(hf != INVALID_HANDLE_VALUE){
		do{
			if ( FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY ) {
				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='\0' ) continue;//Проверка на служебные записи
				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='.' && FindFileData.cFileName[2]=='\0' ) continue;
				vrtMapCreationParam vMapCParam;
				XBuffer str;
				if(!worldsDir.empty()) str < worldsDir.c_str() < "\\";
				else str <".\\";
				str < FindFileData.cFileName < "\\" < worldDataFile;
				XPrmIArchive ia;
				if(ia.open(str, 1024)){
					ia.serialize(vMapCParam, "WorldHeader", 0);
				}
				worldinfoList.push_back(sWorldInfo(FindFileData.cFileName, 1<<vMapCParam.H_SIZE_POWER, 1<<vMapCParam.V_SIZE_POWER));
			}
		} while(FindNextFile( hf, &FindFileData ));
		FindClose( hf );
	}
	return true;
}

bool vrtMap::isImpassability(const Vect2i& pos, int r)
{
	int xL = w2mClampX(pos.x - r);
	int xR = w2mClampX(pos.x + r);
	int yT = w2mClampY(pos.y - r);
	int yD = w2mClampX(pos.y + r);
	unsigned int a = 0;
	for(int y=yT; y<=yD; y++){
		int offGY=offsetGBuf(0, y);
		for(int x=xL; x<=xR; x++){
			a|=GABuf[offGY+x];
		}
	}
	if(a & GRIDAT_IMPASSABILITY) 
		return true;
	else 
		return false;
}

void vrtMap::setImpassabilityAndSurKind(int _x, int _y, int _r, eSettingImpassabilityMetod impassability, eSettingSurfaceKindMetod surKindMetod)
{
	unsigned short atr=0, mask=0;
	//atr = impassability ? GRIDAT_IMPASSABILITY : 0;
	switch(impassability){
		case SIMM_SetImpassability:		atr|=GRIDAT_IMPASSABILITY; mask|=GRIDAT_IMPASSABILITY;	break;
		case SIMM_UnSetImpassability:	mask|=GRIDAT_IMPASSABILITY;	break;
		case SIMM_NotChangeImpassability: default:	break;
	}

	if(surKindMetod != SurfaceKind_NoChange){
		xassert((int)surKindMetod >=0 && (int)surKindMetod < TERRAIN_TYPES_NUMBER);
		atr|=surKindMetod;
		mask|=GRIDAT_MASK_SURFACE_KIND;
	}
	mask=~mask;
	int x = _x >> kmGrid;
	int y = _y >> kmGrid;
	int r = _r >> kmGrid;
	if((r << kmGrid) != _r ) 
		r++;
	if(r > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in setImpassability");
		r = MAX_RADIUS_CIRCLEARR;
	}
	for(int i = 0; i <= r; i++){
		int max = maxRad[i];
		int* xx = xRad[i];
		int* yy = yRad[i];
		for(int j=0; j<max; j++) {
			int offG = vMap.offsetGBuf(vMap.XCYCLG(x + xx[j]), vMap.YCYCLG(y + yy[j]));
			vMap.GABuf[offG] &=mask;
			vMap.GABuf[offG] |= atr;
		}
	}
}

void vrtMap::setInds(int xc, int yc, int rad, bool erase)
{
	if(rad > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in drawHardness ");
		rad=MAX_RADIUS_CIRCLEARR;
	}

	int result=0;
	if(!erase){
		for(int i=0; i<=rad; i++){ //draw
			int maxr = maxRad[i];
			int* xx = xRad[i];
			int* yy = yRad[i];
			for(int j=0; j<maxr; j++) {
				int off=offsetBufC(xc+xx[j], yc+yy[j]);
				VxABuf[off]|=VmAt_Inds;
			}
		}
	}
	else {
		for(int i=0; i<=rad; i++){ //erase
			int maxr = maxRad[i];
			int* xx = xRad[i];
			int* yy = yRad[i];
			for(int j=0; j<maxr; j++) {
				int off=offsetBufC(xc+xx[j], yc+yy[j]);
				VxABuf[off]&=~VmAt_Inds;
			}
		}
	}
}

int vrtMap::getTerrainType(const Vect2i& pos, int r)
{
	int xL = w2mClampX(pos.x - r);
	int xR = w2mClampX(pos.x + r);
	int yT = w2mClampY(pos.y - r);
	int yD = w2mClampX(pos.y + r);
	int counters[TERRAIN_TYPES_NUMBER];
	memset(&counters, 0, sizeof(counters));
	for(int y=yT; y<=yD; y++){
		int offGY=offsetGBuf(0, y);
		for(int x=xL; x<=xR; x++){
			counters[GABuf[offGY+x] & GRIDAT_MASK_SURFACE_KIND]++;
		}
	}

	int iMax = 0, nMax = 0;
	for(int i = 0; i < TERRAIN_TYPES_NUMBER; i++)
		if(nMax < counters[i]){
			nMax = counters[i];
			iMax = i;
		}
	return iMax;
}
