#include "stdafxTr.h"

#include "worldFileDispatcher.h"
#include "stdio.h"
#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"
#include "quantizer.h"
#include "terTools.h"
#include "Serialization\ResourceSelector.h"
#include "tgai.h"
#include "Console.h"
#include "vmap.inl"
#include "Render\Src\MultiRegion.h"
#include "Render\Src\TileMap.h"
#include "Serialization\EnumDescriptor.h"

/* ----------------------------- EXTERN SECTION ---------------------------- */

extern int DEFAULT_TERRAIN;

/* --------------------------- DEFINITION SECTION -------------------------- */
vrtMap vMap;

const unsigned int MAX_CHANGED_RENDER_AREAS=512;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, eCreateWorldMetod, "eCreateWorldMetod")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, FullPlain, "Плоский");
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, Mountains, "Горы");
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, eCreateWorldMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, SIZE_POWER, "Размер")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_512,  "512")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_1024, "1024")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_2048, "2048")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_4096, "4096")
REGISTER_ENUM_ENCLOSED(vrtMapCreationParam, SIZE_8192, "8192")
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMapCreationParam, SIZE_POWER)

//BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eShowSpecialInfo, "Показ спец поверхности")
//REGISTER_ENUM_ENCLOSED(vrtMap, SSI_NoShow,  "не показывать")
//REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowHardnessImpassability, "неразрушаемость-непроходимость")
//REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowKind, "1,2,3,4 рода")
////REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowImpassability, "непроходимость")
////REGISTER_ENUM_ENCLOSED(vrtMap, SSI_ShowAllGeo, "гео")
//END_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eShowSpecialInfo)


char* vrtMap::worldRasterDataFileSource = "output.vmp";
char* vrtMap::worldRGBFileSource = "output.tga";
//char* vrtMap::worldGeoPalFile     = "inGeo.act";
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

vrtMapCreationParam::vrtMapCreationParam() 
{
    H_SIZE_POWER = SIZE_2048;
    V_SIZE_POWER = SIZE_2048;
    initialHeight = 30;
	createWorldMetod = FullPlain;
}

void vrtMapCreationParam::serialize(Archive& ar) 
{
    ar.serialize(H_SIZE_POWER, "H_SIZE_POWER", "Ширина");
    ar.serialize(V_SIZE_POWER, "V_SIZE_POWER", "Высота");

	ar.serialize(createWorldMetod, "createWorldMetod", 0);
	ar.serialize(initialHeight, "initialHeight", "Начальная высота");
}

void vrtMapChangeParam::serialize(Archive& ar) 
{
	vrtMapCreationParam::serialize(ar);
	ar.serialize(flag_resizeWorld2NewBorder, "flag_resizeWorld2NewBorder", "Масштабировать мир к новым границам");
	ar.serialize(oldWorldBegCoordX, "oldWorldBegCoordX", "Начальная координата X старого мира в новом");
	ar.serialize(oldWorldBegCoordY, "oldWorldBegCoordY", "Начальная координата Y старого мира в новом");
	ar.serialize(kScaleModels, "kScaleModels", "Масштаб моделей");
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
		//ar.serialize(geoTxFileName, "geoTxFileName", 0);
		//ar.serialize(matGeoTxFileName, "matGeoTxFileName", 0);
		//ar.serialize(loopedGeoTxFileName, "loopedGeoTxFileName", 0);

		ar.serialize(bitmapDispatcher, "bitmapDispatcher", 0);

		curNewPMO=containerPMO.size();
		unsigned int lowworld_procMapOp=procMapOp&0xffFFffFF;
		int highworld_procMapOp=procMapOp>>32;
		ar.serialize(lowworld_procMapOp, "lowworld_procMapOp", 0);
		ar.serialize(highworld_procMapOp, "highworld_procMapOp", 0);
		procMapOp =(__int64)lowworld_procMapOp;
		procMapOp|=(__int64)highworld_procMapOp <<32;

		ar.serialize(VxABufCRC, "VxABufCRC", 0);
		ar.serialize(DamSurBufCRC, "DamSurBufCRC", 0);

		if(!ar.serialize(guid, "guid", 0)){
			ar.serialize(guid.Data1, "GUID_Data1", 0);
			ar.serialize(guid.Data2, "GUID_Data2", 0);
			ar.serialize(guid.Data3, "GUID_Data3", 0);
			ar.serializeArray(guid.Data4, "GUID_Data4", 0);
		}
		
		if(ar.isOutput() && worldSerial==0){
			worldSerial=generateSerialNumber();
		}
		ar.serialize(worldSerial, "worldSerial", 0);
	}

}

void vrtMap::serializeRegion(Archive& ar)
{
	region_->lock();

	ar.serialize(placementZoneCircles_, "placementZoneCircles", 0);

	ar.serialize(*region_, "MultiDetailRegion", "MultiDetailRegion");

	if(ar.isInput()){
		if(region_->width() != H_SIZE || region_->height() != V_SIZE){
			kdError("3d", "Регион мелкодетальной текстуры имеет некорректные размеры и будет очищен.");
			region_->init(H_SIZE, V_SIZE, 1);
		}

		region_->validate();
		region_->fixRegion(1);
	}
	
	region_->unlock();
}

void vrtMap::serialize(Archive& ar) 
{
    serializeParameters(ar);
	//ar.serialize(geoTxOctree, "geoTxOctree", 0);
}

bool testExistingFile(const char* fName)
{
	DWORD fa = GENERIC_READ;
	DWORD fs = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD fc = OPEN_EXISTING;
	DWORD ff = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	HANDLE hFile=CreateFile(fName, fa, fs, 0, fc, ff, 0);
	if(hFile == INVALID_HANDLE_VALUE) return 0;
	else { CloseHandle(hFile); return 1; }
}

const char* vrtMap::getTargetName(const char* name)
{
	static string buffer;
	if(worldsDir.empty()) buffer.clear();
	else buffer = worldsDir + "\\";

	buffer += worldName + "\\" + name;
	return buffer.c_str();
}

vrtMap::vrtMap()
: region_(new MultiRegion())
{
	flag_worldLoaded=false;
    // params

	worldName = "Default";
	worldsDir = "Resource\\Worlds";

	//changedT = 0;

	gridChAreas = 0;
	gridChAreas2 = 0;

	pTempArray=0;

	worldChanged=0;

	//ClTrBuf=0; AtBuf=0; VxBuf=0;

	//VxGBuf=0; VxDBuf=0; AtrBuf=0; 
	vxaBuf=0; clrBuf=0; //RnrBuf=0;
	gVBuf=0; gABuf=0;
	supBuf=0;

	veryLightDam=0;

	//showSpecialInfo=SSI_NoShow;//SSI_NoShow; //SSI_ShowImpassability;// //SSI_ShowHardness
	showSurKind = false;
	flag_ShowDbgInfo=0;

	FilterMinHeight=0;
	FilterMaxHeight=MAX_VX_HEIGHT;

	inv_vx_fraction = 1.0f/float(1<<VX_FRACTION);

//Очистка списка для Undo  и установка итератора на начальный элемент
	//preCAs.erase(preCAs.begin(), preCAs.end());
	//curPreCA=preCAs.begin();
	containerPMO.reserve(1000);
	UndoDispatcher_KillAllUndo();
	//PreocedureMap
	flag_recordingPMO=true;
	clearContainerPMO();
	procMapOp=0;

	flag_saveChangedAreas=true;

    VxABufCRC=DamSurBufCRC=0;
	guid = XGUID::ZERO;

	flag_tryColorDamTextures=false;

	prepareCircleArr();
	worldSerial=0;
	disableLoadContainerPMO_=false;

	cEmitterInterface::setTerraFunctor(this);
}

vrtMap::~vrtMap()
{
	cEmitterInterface::setTerraFunctor(0);

	delete region_;

	flag_worldLoaded=false;

	//if(changedT) { delete changedT; changedT = 0; }

	releaseMem4Buf();
	releaseCircleArr();
}

void vrtMap::allocMem4Buf(bool flag_useTryColorBuffer)
{
	releaseMem4Buf();
	vxaBuf = new unsigned short [H_SIZE*V_SIZE];

	clrBuf = new TerrainColor [H_SIZE*V_SIZE];
	//RnrBuf = new unsigned char [H_SIZE*V_SIZE];
	if(flag_useTryColorBuffer)
        supBuf = new unsigned long [H_SIZE*V_SIZE];

	gVBuf  = new unsigned short [(H_SIZE>>kmGrid)*(V_SIZE>>kmGrid)];
	gABuf  = new unsigned short [(H_SIZE>>kmGrid)*(V_SIZE>>kmGrid)];

	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	gridChAreas= new unsigned char[sizeGCA];
	gridChAreas2= new unsigned char[sizeGCA];
	clearGridChangedAreas();

}
void vrtMap::releaseMem4Buf()
{
	if(gVBuf) { delete [] gVBuf; gVBuf=0;}
	if(gABuf) { delete [] gABuf; gABuf=0;}

	if(supBuf){ delete [] supBuf; supBuf=0;}
	//delete [] RnrBuf;
	if(clrBuf) { delete [] clrBuf; clrBuf=0;}
	if(vxaBuf) { delete [] vxaBuf; vxaBuf=0;}

	if(gridChAreas) { delete [] gridChAreas; gridChAreas=0; }
	if(gridChAreas2) { delete [] gridChAreas2; gridChAreas2=0; }
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

void vrtMap::initGeneralVariable()
{
	H_SIZE = 1 << H_SIZE_POWER;
	clip_mask_x = H_SIZE - 1;
	GH_SIZE = H_SIZE >> kmGrid;
	clip_mask_x_g = GH_SIZE - 1;

	V_SIZE = 1 << V_SIZE_POWER;
	clip_mask_y = V_SIZE - 1;
	GV_SIZE = V_SIZE >> kmGrid;
	clip_mask_y_g = GV_SIZE - 1;
}


void vrtMap::create(const char* _worldName, unsigned short* vxbuf, unsigned long* rgb)
{
	worldName = _worldName;

	worldSerial=generateSerialNumber();

	bitmapDispatcher.release();
	bitGenDispatcher.release();

	initGeneralVariable();

	
	//RenderPrepare1();

	changedAreas.erase(changedAreas.begin(), changedAreas.end());
	//renderAreas.clear();

	allocMem4Buf(true);

	createDamPal();
	createBeginSurface(vxbuf, rgb);

	//f3d.loadVariable();

	//putNewGeoTexture("","");//matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
	WorldRender();

	initGrid();

	clearGridChangedAreas();
	///loadHardness();
	worldChanged=1;
	//geoTxFileName.clear();
	//f3d.load(geoTxFileName.c_str());
	flag_worldLoaded=true;

	UndoDispatcher_KillAllUndo();
	//PreocedureMap
	flag_recordingPMO=true;
	clearContainerPMO();
	procMapOp=0;

	toShowTryColorDamTexture(true);

	region_->init(H_SIZE, V_SIZE, 1);
}

void vrtMap::createBeginSurface(unsigned short* vxbuf, unsigned long* rgbbuf)
{
	xassert(supBuf);
	if(createWorldMetod == FullPlain){
		int x,y;
		for(y=0; y<V_SIZE; y++){
			for(x=0; x<H_SIZE; x++){
				int offb=offsetBuf(x,y);
				vxaBuf[offb]=vxbuf ? vxbuf[offb] : Vm_prepHeighAtr(initialHeight<<VX_FRACTION, VmAt_Nrml);
				clrBuf[offb]=0;
				supBuf[offb]=rgbbuf ? rgbbuf[offb] : 0x007f7f7f;//Sur2Col32[SurBuf[offb]][255];
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
	//memcpy(backupVxABuf, vxaBuf, sizeof(backupVxABuf[0])*H_SIZE*V_SIZE);
	//memcpy(backupSupBuf, supBuf, sizeof(backupSupBuf[0])*H_SIZE*V_SIZE);
	
	bitmapDispatcher.clearCache();//release вызывать нельзя т.к. colorPic инструменты привязаны к индексам а не к имени файла!
	bitGenDispatcher.release();
	if(testExistingFile(getTargetName(worldRasterDataFileSource))) { //подложка - старый мир
		loadDamPal();
		loadVMP(getTargetName(worldRasterDataFileSource), true);
		///Загрузка из палитры старого мира
		if(!loadRGBCache(getTargetName(worldRGBFileSource)) )
            setRGBCacheInSurBuf();
	}
	else { // подложка - процедурная
		createDamPal();
		createBeginSurface();

		//f3d.loadVariable();

	} // else // подложка - процедурная

	playPMOperation();
	//putNewGeoTexture(matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
	WorldRender();

	initGrid();
	clearGridChangedAreas();
	worldChanged=1;
}

bool vrtMap::load(const char* _worldName, bool flag_useTryColorBuffer) //supBuf
{
	bitmapDispatcher.release();
	bitGenDispatcher.release();

	UndoDispatcher_KillAllUndo(); //Очистка всего буфера Undo-Redo

	worldName=_worldName;

	worldSerial=0;

	placementZoneCircles_.clear();

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

	//RenderPrepare1();

	changedAreas.clear();
	//renderAreas.clear();
	changedAreas.reserve(MAX_CHANGED_RENDER_AREAS);
	//renderAreas.reserve(MAX_CHANGED_RENDER_AREAS);

	allocMem4Buf(flag_useTryColorBuffer);

	//if( loadVMP(getTargetName(worldRasterDataCacheFile)) && loadRGBCache(getTargetName(worldRGBCacheFile)) && DamSurBufCRC==getDamSurBufCRC() )
	bool flagNotBuildWorld=true;
	if(flag_useTryColorBuffer)
		flagNotBuildWorld&=loadRGBCache(getTargetName(worldRGBCacheFile));
	flagNotBuildWorld&=loadVMP(getTargetName(worldRasterDataCacheFile), true);
	if( flagNotBuildWorld ){ //Загрузка из кэша
		loadDamPal();

		//f3d.load(geoTxFileName.c_str());
		//putNewGeoTexture(matGeoTxFileName.c_str(), loopedGeoTxFileName.c_str());
		//initGeoTexture();

		initGrid();
		clearGridChangedAreas();
		//loadHardness();
		loadSurKind();
	}
	else { //Загрузка и компиляция мира
		rebuild();
		///f3d.load(geoTxFileName.c_str());
		unsigned int curVxABufCRC=getVxABufCRC();
		unsigned int curDamBufCRC=getDamSurBufCRC();
		if(VxABufCRC!=curVxABufCRC){
			xassert(0&& "Сгенеренные высоты не совпадают с записанными");
		}
		if(DamSurBufCRC!=curDamBufCRC){
			xassert(0&& "Поверхность текстуры(Dam) сгенеренного мира не совпадает с записанной");
		}
	}


	///elementarTool<SIM_UnSetInds, TTTE_PUT, TTSE_DRAW_DAM> tool;
	///tool.influenceDZ(0, 0, 0, 2*0, 3 );

	worldChanged=0;
	flag_worldLoaded=true;

	if(flag_useTryColorBuffer)
		toShowTryColorDamTexture(true);
	else 
		toShowTryColorDamTexture(false);
	currentGameMapCRC=getWorldCRC();

	region_->init(H_SIZE, V_SIZE, 1);

	return true;
}

void vrtMap::save(const char* _worldName)
{
	worldName=_worldName;

	guid.generate();

	//Проверка каталога на существование и создание если нет
	string path2world;
	if(worldsDir.empty()) path2world.clear();
	else path2world = worldsDir + "\\";
	path2world += worldName;

    VxABufCRC=getVxABufCRC();
    if(supBuf)
		DamSurBufCRC=getDamSurBufCRC();

	WIN32_FIND_DATA FindFileData;
	HANDLE hf = FindFirstFile( path2world.c_str(), &FindFileData );
	if (hf == INVALID_HANDLE_VALUE)
		::CreateDirectory(path2world.c_str(), 0);
	else
		xassert( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 );

	XPrmOArchive oa(getTargetName(worldDataFile));
	oa.serialize(static_cast<vrtMapCreationParam&>(*this), "WorldHeader", 0);
	oa.serialize(*this, "WorldData", 0);

	saveVMPCach(getTargetName(worldRasterDataCacheFile), VxABufCRC, DamSurBufCRC);
	//convertMapSize(getTargetName(worldRasterDataCacheFile));
	if(supBuf){
		TGAHEAD tgahead;
		tgahead.saveRGBL(getTargetName(worldRGBCacheFile), H_SIZE, V_SIZE, supBuf);
	}
	saveDamPal();
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

void vrtMap::saveVMPCach(const char* _fname, unsigned int _crcVxA, unsigned int _crcDam)
{
	sVmpHeader VmpHeader;
	VmpHeader.setID("S5L4");
	VmpHeader.XSize=H_SIZE;
	VmpHeader.YSize=V_SIZE;
	VmpHeader.AUX1=_crcVxA;
	VmpHeader.AUX2=sizeof(clrBuf[0]);
	VmpHeader.AUX3=_crcDam;

	XBuffer buffer(sizeof(VmpHeader) + H_SIZE*V_SIZE*sizeof(vxaBuf[0]) + H_SIZE*V_SIZE*sizeof(clrBuf[0]));
	buffer.write(&VmpHeader,sizeof(VmpHeader));
	buffer.write(&vxaBuf[0], H_SIZE*V_SIZE*sizeof(vxaBuf[0]));
	buffer.write(&clrBuf[0], H_SIZE*V_SIZE*sizeof(clrBuf[0]));
	saveFileSmart(_fname, buffer, buffer.tell());
}

void vrtMap::releaseWorld()
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

void vrtMap::loadSurKind()
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
				gABuf[offG]  =( HDNBuf[yOffsetHBuf+j]   & GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND) | (gABuf[offG]  &(~GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND));
				gABuf[offG+1]=((HDNBuf[yOffsetHBuf+j]>>4)&GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND) | (gABuf[offG+1]&(~GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND));
			}
			yOffsetHBuf+=width;
		}
		delete [] HDNBuf;
		ff.close();
	}
}

void vrtMap::saveSurKind()
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
			itg = gABuf[offG]  &GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND;
			itg|=(gABuf[offG+1]&GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND)<<4;
			HDNBuf[yOffsetHBuf+j]=itg;
		}
		yOffsetHBuf+=width;
	}
	buffer.write(HDNBuf, width*height*sizeof(unsigned char));

	delete [] HDNBuf;

	saveFileSmart(getTargetName(worldSurfaceKindFile), buffer, buffer.tell());
}

void vrtMap::clearGridChangedAreas()
{
	int sizeGCA=(V_SIZE>>kmGridChA)*(H_SIZE>>kmGridChA);
	//обнуление
	memset(gridChAreas, 0, sizeGCA*sizeof(*gridChAreas));
	memset(gridChAreas2, 0, sizeGCA*sizeof(*gridChAreas));
}

void vrtMap::updateGridChangedAreas2()
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
bool vrtMap::checkGridChangedAreas2()
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

void vrtMap::convertS5L3ToS5L4()
{
	//S5L3 
	//VX_FRACTION=5;
	//VX_SHIFT_VX=3;
	// const int MAX_VX_HEIGHT=((1<<(VX_FRACTION + 8))-1);//это 0x1fff//
	//#define VmAt_Inds	(0x2)
	//#define VmAt_MASK (0x7)

	//S5L4
	//VX_FRACTION=5;
	//VX_SHIFT_VX=2;
	// const int MAX_VX_HEIGHT=((1<<(VX_FRACTION + 9))-1);//это 0x3fff//
	//#define VmAt_Inds	(0x1)
	//#define VmAt_MASK (0x3)

	int i,j, cnt=0;
	for(i=0; i<V_SIZE; i++){
		for(j=0; j<H_SIZE; j++){
			vxaBuf[cnt]= (vxaBuf[cnt]>>1) & (~0x2);
			cnt++;
		}
	}
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
					data=Vm_prepHeighAtr( ((pBufG[j]<<VX_FRACTION)|(pBufA[j]&VX_FRACTION_MASK)) , VmAt_Nrml);
				else //dam
					data=Vm_prepHeighAtr( ((pBufD[j]<<VX_FRACTION)|(pBufA[j]&VX_FRACTION_MASK)) , VmAt_Nrml);
				vxaBuf[offsetBuf(j,i)]=data;
			}
		}
		fmap.seek(sizeof(VmpHeader)+ H_SIZE*V_SIZE*3, XS_BEG);
		fmap.read(&clrBuf[0], H_SIZE*V_SIZE);
	}
	else if(VmpHeader.cmpID("S5L2")) { // solid buffer-a
		if(!flag_NoCheckCRC){
			xassert(VmpHeader.XSize==H_SIZE && VmpHeader.YSize==V_SIZE ); //надо перезаписать промежуточную версию формата vmp в итоговую
			if(VmpHeader.AUX1!=VxABufCRC  /*|| VmpHeader.AUX3!=DamSurBufCRC*/)
				return 0;
		}
		VxABufCRC=VmpHeader.AUX1; DamSurBufCRC=VmpHeader.AUX3;
		fmap.read(&vxaBuf[0], H_SIZE*V_SIZE*sizeof(vxaBuf[0]));
		if(sizeof(clrBuf[0])==1)
			fmap.read(&clrBuf[0], H_SIZE*V_SIZE*sizeof(clrBuf[0]));
		else if(supBuf)
			convertVMapTryColor2TerClr();
		else xassert(0&&"Not compatible color in world");;
	}
	else if(VmpHeader.cmpID("S5L3") || VmpHeader.cmpID("S5L4") ) { //S5L3 - 256|HiColor  //S5L4 - 512 высота
		if(VmpHeader.XSize!=H_SIZE || VmpHeader.YSize!=V_SIZE  || VmpHeader.AUX1!=VxABufCRC)
			return 0;
		VxABufCRC=VmpHeader.AUX1; DamSurBufCRC=VmpHeader.AUX3;
		fmap.read(&vxaBuf[0], H_SIZE*V_SIZE*sizeof(vxaBuf[0]));
		if(VmpHeader.AUX2==sizeof(clrBuf[0]))
			fmap.read(&clrBuf[0], H_SIZE*V_SIZE*sizeof(clrBuf[0]));
		else if(supBuf)
			convertVMapTryColor2TerClr();
		else xassert(0&&"Not compatible color in world");;
	}
	else ErrH.Abort("VMP file is not TRUE version");
	if(!VmpHeader.cmpID("S5L4"))
		convertS5L3ToS5L4();

	return 1;
}

bool vrtMap::loadRGBCache(const char* fname)
{
	xassert(supBuf);
	TGAHEAD tgahead;
	if( tgahead.loadHeader(fname) ){
		if((tgahead.PixelDepth!=24) || (tgahead.ImageType!=2)) {
			///AfxMessageBox("Не поддерживаемый тип TGA (необходим 24bit не компрессованный)");
			xassert(0&&"Не поддерживаемый тип TGA (необходим 24bit не компрессованный)");
			return false;
		}
		tgahead.load2RGBL(H_SIZE, V_SIZE, supBuf);
		return true;
	}
	else 
		return false;
}

void vrtMap::setRGBCacheInSurBuf()
{
	xassert(supBuf);
	///Загрузка из палитры
	int y,x;
	for(y=0; y<V_SIZE; y++){
		for(x=0; x<H_SIZE; x++){
			int off=offsetBufC(x,y);
			supBuf[off]=getColor32T<TerrainColor>(off);//Sur2Col32[clrBuf[off]][255];
		}
	}
}

void vrtMap::saveMiniMap(int sizex, int sizey)
{
	unsigned long* pMap=new unsigned long[sizex*sizey];

	int stepXVM=H_SIZE/sizex;
	int stepYVM=V_SIZE/sizey;
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
	xassert(sizeof(TerrainColor)<= sizeof(buf[0]));
	TerrainColor* pBufClr=(TerrainColor*)buf;
	unsigned short* pBufShort=buf;
	int i, j, cnt=0;
	for(i=0; i<vSizeGCA; i++){
		for(j=0; j<hSizeGCA; j++){
			if(gridChAreas2[cnt]==1){
				int k, m, cnt2=0;
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						pBufShort[cnt2]=vxaBuf[offB+m];
						cnt2++;
					}
				}
				ff.write(pBufShort, sizeCellGridCA*sizeCellGridCA*sizeof(short));
				cnt2=0;
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						pBufClr[cnt2]=clrBuf[offB+m];
						cnt2++;
					}
				}
				ff.write(pBufClr, sizeCellGridCA*sizeCellGridCA*sizeof(TerrainColor));

			}
			cnt++;
		}
	}

	//saveGrid(ff);
	char flag_taller_h_zp=0;
	for(i=0; i<(GV_SIZE); i++){
		ff.write(&gVBuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(gVBuf[0]));
		ff.write(&gABuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(gABuf[0]));
	}

	ar.serialize(MemoryBlock(ff), "gameMap", 0);

	worldChanged=0;
	return true;
}

bool vrtMap::loadGameMap(Archive& ar)
{
	xassert(ar.isInput());

	MemoryBlock memoryBlock;
	if(!ar.serialize(memoryBlock, "gameMap", 0))
		return false;
	XBuffer ff(memoryBlock.buffer(), memoryBlock.size());
	
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
	xassert(sizeof(TerrainColor)<= sizeof(buf[0]));
	TerrainColor* pBufClr=(TerrainColor*)buf;
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
						vxaBuf[offB+m]=pBufShort[cnt2];
						cnt2++;
					}
				}
				cnt2=0;
				ff.read(pBufClr, sizeCellGridCA*sizeCellGridCA*sizeof(TerrainColor));
				for(k=0; k<sizeCellGridCA; k++){
					int offB=offsetBuf( (j<<kmGridChA), k+(i<<kmGridChA) );
					for(m=0; m<sizeCellGridCA; m++){
						clrBuf[offB+m]=pBufClr[cnt2];
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
		ff.read(&gVBuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(gVBuf[0]));
		ff.read(&gABuf[offsetGBuf(0,i)], (GH_SIZE)*sizeof(gABuf[0]));
		//for(j=0; j<GH_SIZE; j++){
		//	gABuf[offsetGBuf(j,i)] &= ~(GRIDAT_BUILDING|GRIDAT_BASE_OF_BUILDING_CORRUPT);
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
					crc=crc32((unsigned char*)&vxaBuf[offB], sizeCellGridCA*sizeof(vxaBuf[0]), crc);
					crc=crc32((unsigned char*)&clrBuf[offB], sizeCellGridCA*sizeof(clrBuf[0]), crc);
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

unsigned long vrtMap::getWorldCRC()
{
	unsigned int y;
	unsigned int offB=0;
	unsigned int crc=startCRC32;
	unsigned char * pBufTMP=new unsigned char [H_SIZE];
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&vxaBuf[offB], H_SIZE*sizeof(vxaBuf[0]), crc);
		crc=crc32((unsigned char*)&clrBuf[offB], H_SIZE*sizeof(clrBuf[0]), crc);
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
	crc=crc32((unsigned char*)(&gVBuf[begAdrScan]), sizeScan*sizeof(gVBuf[0]), crc);
	crc=crc32((unsigned char*)(&gABuf[begAdrScan]), sizeScan*sizeof(gABuf[0]), crc);
	crc=~crc;
	return crc;
}

unsigned long vrtMap::getVxABufCRC()
{
	unsigned int y;
	unsigned int offB=0;
	unsigned long crc=startCRC32;
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&vxaBuf[offB], H_SIZE*sizeof(vxaBuf[0]), crc);
		offB+=H_SIZE;
	}
	crc=~crc;
	return crc;
}

unsigned long vrtMap::getDamSurBufCRC()
{
	xassert(supBuf);
	unsigned int y;
	unsigned int offB=0;
	unsigned int crc=startCRC32;
	for(y=0; y<V_SIZE; y++){
		crc=crc32((unsigned char*)&supBuf[offB], H_SIZE*sizeof(supBuf[0]), crc);
		offB+=H_SIZE;
	}
	crc=~crc;
	return crc;
}

void vrtMap::createDamPal()
{
	int i;
	//for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
	//	GeoPal[i]=Color4c(128,128,0);
	//}
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		DamPal[i]=Color4c(128,128,128);
	}
	//convertPal2TableSurCol(GeoPal,Geologic);
	//convertPal2TableSurCol(DamPal, Damming);
}
void vrtMap::loadDamPal()
{
	int i;
	//XZipStream ff(getTargetName(worldGeoPalFile), XS_IN);
	//for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
	//	unsigned long color=0;
	//	ff.read(&color, 3); //3-rgb
	//	GeoPal[i] = Color4c(color);
	//}
	//ff.close();
	//convertPal2TableSurCol(GeoPal,Geologic);
	//

	XZipStream ff1(getTargetName(worldDamPalFile), XS_IN);
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		unsigned long color=0;
		ff1.read(&color, 3); //3-rgb
		DamPal[i] = Color4c(color);
	}
	ff1.close();

	//convertPal2TableSurCol(DamPal, Damming);
}

void vrtMap::saveDamPal()//unsigned char palBuf[SIZE_GEO_PALETTE] //Bufer в формате BGR
{
	int i;
	//XBuffer surBuffer(MAX_GEO_SURFACE_TYPE*3);
	//for(i=0; i<MAX_GEO_SURFACE_TYPE; i++){
	//	unsigned long color=GeoPal[i].argb;
	//	surBuffer.write(&color, 3); //3-rgb
	//}
	//saveFileSmart(getTargetName(worldGeoPalFile), surBuffer, surBuffer.tell());

	XBuffer damBuffer(MAX_DAM_SURFACE_TYPE*3);
	for(i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		unsigned long color=DamPal[i].argb;
		damBuffer.write(&color, 3); //3-rgb
	}
	saveFileSmart(getTargetName(worldDamPalFile), damBuffer, damBuffer.tell());
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

void vrtMap::getTileColor32Layer(unsigned char* Texture, DWORD pitch, int xstart, int ystart, int xend, int yend, int step)
{
	//bool dflag=false;
	for(int y = ystart; y < yend; y += step) {
		DWORD* tx=(DWORD*)Texture;
		int yy=clamp(y , 0, clip_mask_y);
		for (int x = xstart; x < xend; x += step) {
			int xx=clamp(x , 0, clip_mask_x);
			*tx = getColor32(xx,yy);
			//if(dflag==false && x > xstart + (xend-xstart)/2  && y>ystart+(yend-ystart)/2){
			//	*tx=0xffFFffFF; dflag=true;
			//}
			tx++;
		}
		Texture += pitch;
	}
}
void vrtMap::getTileColor16Layer(unsigned char* Texture, DWORD pitch, int xstart, int ystart, int xend, int yend, int step)
{
	//bool dflag=false;
	for(int y = ystart; y < yend; y += step) {
		WORD* tx = (WORD*)Texture;
		int yy=clamp(y , 0, clip_mask_y);
		for (int x = xstart; x < xend; x += step) {
			int xx=clamp(x , 0, clip_mask_x);
			*tx = getColor16(xx,yy) | 0xff000000;
			//if(dflag==false && x > xstart + (xend-xstart)/2  && y>ystart+(yend-ystart)/2){
			//	*tx=0x000000; dflag=true;
			//}
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
			a&=gABuf[offsetGBufC(xC+xx[j], yC+yy[j])];
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
			int z = gVBuf[offsetGBuf(x + xc, y + yc)];
			Sz += z;
			Sxz += x*z;
			Syz += y*z;
		}
	}
	float N = sqr(2.0f * delta + 1.0f);
	float t = 3.0f / (N * delta * (delta + 1.0f));
	normalNonNormalized.set(-Sxz * t, -Syz * t, 4.0f);
	return (float)Sz / N;
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
			int z = gVBuf[offsetGBuf(x + xc, y + yc)];
			zMin = min(zMin, z);
			zMax = max(zMax, z);
		}
	}
}

void vrtMap::getZMinMaxAvr(int tile_x,int tile_y,int tile_dx,int tile_dy,int& zmin,int& zmax, int& zavr)
{
	zmin = 255;
	zmax = 0;
	zavr = 0;
	
	int shift=kmGrid;

	int dx=tile_dx>>shift,dy=tile_dy>>shift;

	int x_start=tile_x>>shift,y_start=tile_y>>shift;
	int x_end=x_start+dx,y_end=y_start+dy;
	for(int y=y_start;y<y_end;y++)
	{
		for(int x=x_start;x<x_end;x++)
		{
			int z = getZGrid(x,y);
			zavr += z;
			if(z<zmin)
				zmin=z;
			if(z>zmax)
				zmax=z;
		}
	}
	zavr /= dx*dy;
}

bool vrtMap::isOnlyTerrainTypePresents(const Vect2i& center, int radius, int terrainTypes)
{
	bool ttPermission[TERRAIN_TYPES_NUMBER];
	for(int i=0; i<TERRAIN_TYPES_NUMBER; i++, terrainTypes>>=1)
		ttPermission[i] = (terrainTypes&0x1)!= 0;
	int delta = max(w2m(radius), 1);
	int xc = clamp(w2m(center.x), delta, GH_SIZE - delta - 1);
	int yc = clamp(w2m(center.y), delta, GV_SIZE - delta - 1);
	for(int y = -delta;y <= delta;y++){
		for(int x = -delta;x <= delta;x++){
			if( !ttPermission[gABuf[offsetGBuf(x + xc, y + yc)]&GRIDAT_MASK_SURFACE_KIND] )
				return false;
		}
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
			a|=gABuf[offGY+x];
		}
	}
	if(a & GRIDAT_IMPASSABILITY) 
		return true;
	else 
		return false;
}
int terrainType2SurKind(TerrainType terrainType)
{
	int t=terrainType;
	for(int i=15; i>=0; i--){
		if( (t&(1<<i)) !=0 )
			return i;
	}
	return 0;
}

//void vrtMap::setImpassabilityAndSurKind(int _x, int _y, int _r, eSettingImpassabilityMetod impassability, eSettingSurfaceKindMetod surKindMetod)
void vrtMap::setImpassabilityAndSurKind(int _x, int _y, int _r, TerrainType terrainType, bool _surKind4Work[TERRAIN_TYPES_NUMBER])
{
	unsigned short atr=0, mask=0;
	//switch(impassability){
	//	case SIMM_SetImpassability:		atr|=GRIDAT_IMPASSABILITY; mask|=GRIDAT_IMPASSABILITY;	break;
	//	case SIMM_UnSetImpassability:	mask|=GRIDAT_IMPASSABILITY;	break;
	//	case SIMM_NotChangeImpassability: default:	break;
	//}

	//if(surKindMetod != SurfaceKind_NoChange){
		int surKind=terrainType2SurKind(terrainType);
		xassert(surKind >=0 && surKind < TERRAIN_TYPES_NUMBER);
		atr|=surKind;
		mask|=GRIDAT_MASK_SURFACE_KIND;
	//}
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
			int offG = offsetGBuf(XCYCLG(x + xx[j]), YCYCLG(y + yy[j]));
			if(_surKind4Work[gABuf[offG]&GRIDAT_MASK_SURFACE_KIND]){
				gABuf[offG] &=mask;
				gABuf[offG] |= atr;
			}
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
				vxaBuf[off]|=VmAt_Inds;
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
				vxaBuf[off]&=~VmAt_Inds;
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
			counters[gABuf[offGY+x] & GRIDAT_MASK_SURFACE_KIND]++;
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

void vrtMap::sVmpHeader::setID(const char* pVmpID)
{
	for(int i=0; i< SVMPHEADER_ID_SIZE; i++){
		if(*pVmpID!=0)
			id[i]=*pVmpID++;
		else {
			do{	id[i++]=0; 
			} while(i< SVMPHEADER_ID_SIZE);
			break;
		}
	}
}
bool vrtMap::sVmpHeader::cmpID(const char* pVmpID)
{
	for(int i=0; i< SVMPHEADER_ID_SIZE; i++){
		if(*pVmpID!=id[i]) return false;
		if(*pVmpID!=0) pVmpID++;
		else return false;
	}
	return true;
}
inline int clampX(const int x, const int xmax)
{
	__asm {
		mov eax, x
		xor ecx, ecx
		cmova eax, ecx
		mov ecx, xmax
		cmova eax, ecx
	}
}

void vrtMap::getTileZ(char* Texture,int pitch,int xstart,int ystart,int xend,int yend,int step)
{
	int size=pitch>>2;
	xassert((yend+step-ystart)<=size*step);
	xassert((xend+step-xstart)<=size*step);
	for(int y = ystart,ybuffer=0; y < yend; y += step,ybuffer++){
		int* tx = (int*)(Texture+ybuffer*pitch);
		int yy = clamp(y, 0, clip_mask_y);
		for (int x = xstart; x < xend; x += step,tx++)
		{
			int xx = clamp(x, 0, clip_mask_x);
			//int xx = clampX(x, clip_mask_x);
			int z = int(getAlt(xx,yy))<<(8-VX_FRACTION);
			*tx=z;
		}
	}
}

void vrtMap::getNormal(int x,int y,Vect3f& normal)
{
	float z0 = vMap.getZf(x, y);
	float zx = vMap.getZf(x + 1, y);
	float zy = vMap.getZf(x, y + 1);
	float dzx=zx-z0;
	float dzy=zy-z0;

	normal.set(-dzx,-dzy,1);
	normal.normalize();
}

void vrtMap::drawMiniDetailTexture(int layer, const Vect2i& point, int radius)
{
	UndoDispatcher_PutPreChangedArea(sRect(point.x - radius, point.y - radius, 2*radius, 2*radius),0,0);
	drawMiniDetailTextureAux(layer, point, radius);
	regRender(point.x - radius, point.y - radius, point.x + radius, point.y + radius, UPDATEMAP_REGION);
}

void vrtMap::drawMiniDetailTextureAux(int layer, const Vect2i& point, int radius)
{
	region_->lock();
	xassert(layer < cTileMap::miniDetailTexturesNumber+cTileMap::placementZoneMaterialNumber);

	ShapeRegion circle;
	circle.circle(radius, layer + 1);
	circle.move(point.x,point.y);
	region_->operate(circle);
	
	region_->unlock();
}

void vrtMap::drawPlacementZone(int material, const Vect2i& point, int radius, bool internal)
{
	region_->lock();
	xassert(material < cTileMap::placementZoneMaterialNumber);

	ShapeRegion circle;
	circle.circle(radius, material + 1 + cTileMap::miniDetailTexturesNumber);
	circle.move(point.x,point.y);
	region_->operate(circle);
	
	region_->unlock();

	regRender(point.x - radius, point.y - radius, point.x + radius, point.y + radius, TypeCh_Region);

	if(!internal)
		placementZoneCircles_.add(PlacementZoneCircle(point, radius, material));
}

void vrtMap::erasePlacementZone(const Vect2i& point, int radius)
{
	region_->lock();

	ShapeRegion circle;
	circle.circle(radius, 1);
	circle.move(point.x,point.y);
	region_->operate(circle);
	
	region_->unlock();

	regRender(point.x - radius, point.y - radius, point.x + radius, point.y + radius, TypeCh_Region);

	placementZoneCircles_.remove(PlacementZoneCircle(point, radius));

	PlacementZoneCircles::iterator i;
	FOR_EACH(placementZoneCircles_, i)
		if(i->position.distance2(point) < sqr(radius) + sqr(i->radius))
			drawPlacementZone(i->material, i->position, i->radius, true);
}

void vrtMap::PlacementZoneCircle::serialize(Archive& ar)
{
	ar.serialize(position, "position", "position");
	ar.serialize(radius, "radius", "radius");
	ar.serialize(material, "material", "material");
}

Vect2i vrtMap::findNearestFoso(Vect3f begPos)
{
	return Vect2i::ZERO;
}

