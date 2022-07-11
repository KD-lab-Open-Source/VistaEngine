#ifndef __VMAP_H__
#define __VMAP_H__

#include "XMath\xmath.h"
#include "Render\src\NParticle.h"
#include "procedurMap.h"
#include "XMath/Colors.h"
#include "terra.h"
#include "undoDispatcher.h"
#include "UpdateMapClient.h"
#include "XTL\UniqueVector.h"
#include "FileUtils\XGUID.h"

class Archive;
class MultiRegion;
enum TerrainType;

// Основная структура карты
struct vrtMapCreationParam {
	enum eCreateWorldMetod {
		FullPlain, Mountains
	};
    enum SIZE_POWER {
        SIZE_128  = 7,	SIZE_256  = 8,	SIZE_512  = 9,	SIZE_1024 = 10,
        SIZE_2048 = 11,	SIZE_4096 = 12,	SIZE_8192 = 13
    };

	SIZE_POWER H_SIZE_POWER;
	SIZE_POWER V_SIZE_POWER;
	eCreateWorldMetod createWorldMetod;
    unsigned short initialHeight;

	vrtMapCreationParam();
	void serialize(Archive& ar);
};

struct vrtMapChangeParam : public vrtMapCreationParam {
	vrtMapChangeParam() : flag_resizeWorld2NewBorder(false), oldWorldBegCoordX(0), oldWorldBegCoordY(0), kScaleModels(1.f){}
	bool flag_resizeWorld2NewBorder;
	int oldWorldBegCoordX;
	int oldWorldBegCoordY;
	float kScaleModels;
	void serialize(Archive& ar);
};

//typedef unsigned char TerrainColor;
typedef unsigned short TerrainColor;

struct vrtMap : public vrtMapCreationParam, FunctorGetZ
{
//////////////////////////////////////////////////////////////////////////
//	PUBLIC fuction для использования 
//////////////////////////////////////////////////////////////////////////
public:
	unsigned int H_SIZE;
	unsigned int clip_mask_x;
	unsigned int V_SIZE;
	unsigned int clip_mask_y;
	vrtMap();
	~vrtMap();

	const char* getTargetName(const char* name);
	const char* getWorldsDir() { return worldsDir.c_str(); }
	void setWorldsDir(const char* dir) { worldsDir = dir; }

	void WorldRender();
	//void renderQuant();

	void create(const char* _worldName = "", unsigned short* vxbuf=0, unsigned long* rgb=0);
	bool load(const char* _worldName, bool flag_useTryColorBuffer=false);
	void save(const char* _worldName);
	bool deleteWorld(const char* _worldName);
	void rebuild();
	void saveMiniMap(int sizex, int sizey);

	bool loadGameMap(Archive& ar);
	bool saveGameMap(Archive& ar);

	void releaseWorld();
	bool isWorldLoaded(){ return flag_worldLoaded; }
	bool isChanged() { return worldChanged;}
	bool isLoad() { return (vxaBuf!=0);}
	bool isCircleSurfaceLeveled(const Vect2i& center, const int radius);

	int XCYCL(int x) const { return x & clip_mask_x; }
	int YCYCL(int y) const { return y & clip_mask_y; }
	int XCYCLG(int x) const { return x & clip_mask_x_g; }
	int YCYCLG(int y) const { return y & clip_mask_y_g; }

	int w2m(int x) { return x >> kmGrid; } // World to map
	int m2w(int x) { return x << kmGrid; } // Map to world

	int w2mClampX(int x) { return w2m(clamp(x, 0, (int)H_SIZE - 1)); } // Clamped
	int w2mClampY(int y) { return w2m(clamp(y, 0, (int)V_SIZE - 1)); }
	
	Vect2i w2m(const Vect2i& v) { return Vect2i(w2m(v.x), w2m(v.y)); }
	Vect2i m2w(const Vect2i& v) { return Vect2i(m2w(v.x), m2w(v.y)); }

	//function для работы с буфером
	//Базовая функция вычисления смещения в буфферах
	int offsetBuf(int x, int y) const { return y*H_SIZE+x; }
	int offsetBufC(int x, int y) const { return YCYCL(y)*H_SIZE+XCYCL(x); }
	int offsetGBuf(int x, int y) const { return y*GH_SIZE+x; }
	int offsetGBufC(int x, int y) const { return YCYCLG(y)*GH_SIZE+XCYCLG(x); }
	int offsetGBufWorldC(int x, int y) const { return YCYCLG(y >> kmGrid)*GH_SIZE+XCYCLG(x >> kmGrid); }
	int offsetBuf2offsetGBuf(int offBuf) const {
		return ( (offBuf>>kmGrid)&clip_mask_x_g ) | ( (offBuf>>(2*kmGrid))&(~clip_mask_x_g) );
	}

	unsigned short getAlt(int offset) const { return Vm_extractHeigh(vxaBuf[offset]); }
	unsigned short getAlt(int x,int y) const { return getAlt(offsetBuf(x,y)); }
	unsigned short getAltC(int x,int y) const { return getAlt(XCYCL(x), YCYCL(y)); }
	unsigned short getAltWhole(int offset) const { return Vm_extractHeighWhole(vxaBuf[offset]); }
	unsigned short getAltWhole(int x,int y) const { return getAltWhole(offsetBuf(x,y)); }

	unsigned short getZ(int x, int y) const { return getAltWhole(offsetBuf(x,y)); }
	float getZf(int x, int y) const { return getAlt(XCYCL(x), YCYCL(y))*inv_vx_fraction; }
	void getNormal(int x,int y,Vect3f& normal);
	void getTileZ(char* Texture,int pitch,int xstart,int ystart,int xend,int yend,int step);

	// Работа с сеткой высот 4x4
	unsigned short getZGrid(int x, int y) { return gVBuf[offsetGBuf(x,y)]; }
	unsigned short getZGridC(int x, int y) { return gVBuf[offsetGBufC(x,y)]; }
	//unsigned short getVoxelW(int x, int y) { return gVBuf[offsetGBufC(w2m(x), w2m(y))]; }
	int getApproxAlt(int x, int y){ return gVBuf[offsetGBufC(x>>kmGrid, y>>kmGrid)]; }

	float analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized);
	void findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax);
	void getZMinMaxAvr(int tile_x,int tile_y,int tile_dx,int tile_dy,int& zmin,int& zmax, int& zavr);
	bool isOnlyTerrainTypePresents(const Vect2i& center, int radius, int terrainTypes);

	bool isImpassability(const Vect2i& pos, int r);

	enum eSettingImpassabilityMetod { SIMM_NotChangeImpassability, SIMM_SetImpassability, SIMM_UnSetImpassability };
	enum eSettingSurfaceKindMetod {	SurfaceKind_NoChange=-1, SurfaceKind_Set1=0, SurfaceKind_Set2=1, SurfaceKind_Set3=2, SurfaceKind_Set4=3 };
	//void setImpassabilityAndSurKind(int _x, int _y, int _r, eSettingImpassabilityMetod impassability, eSettingSurfaceKindMetod surKind);
	void setImpassabilityAndSurKind(int _x, int _y, int _r, enum TerrainType terrainType, bool _surKind4Work[TERRAIN_TYPES_NUMBER]);

	void setInds(int xc, int yc, int r, bool erase=false);
	int getTerrainType(const Vect2i& pos, int r); // не используется
	int getTerrainType(int xw, int yw) { return 1 << getSurKind(xw, yw); } 

	void pointDamageBuildings(int _x, int _y) {
		int offG=offsetGBuf(XCYCLG(_x>>kmGrid), YCYCLG(_y>>kmGrid));
		if(gABuf[offG]&GRIDAT_BUILDING) gABuf[offG]|=GRIDAT_BASE_OF_BUILDING_CORRUPT;
	}
	Vect2i vrtMap::findNearestFoso(Vect3f begPos);

	unsigned char getSurKindG(int xg, int yg){ return (unsigned char)(gABuf[offsetGBufC(xg, yg)]&GRIDAT_MASK_SURFACE_KIND); }
	unsigned char getSurKind(int xw, int yw){ return (unsigned char)(gABuf[offsetGBufWorldC(xw, yw)]&GRIDAT_MASK_SURFACE_KIND); }
	void setSurKindG(int xg, int yg, unsigned char sk){ gABuf[offsetGBufC(xg, yg)]=(sk&GRIDAT_MASK_SURFACE_KIND) | (gABuf[offsetGBufC(xg, yg)]&(~GRIDAT_MASK_SURFACE_KIND)) ; }
	bool isIndestructabilityG(short xg, short yg){ return (gABuf[offsetGBufC(xg, yg)]&GRIDAT_INDESTRUCTABILITY) != 0; }
	bool isIndestructability(short x, short y){ return (gABuf[offsetGBufWorldC(x, y)]&GRIDAT_INDESTRUCTABILITY) != 0; }

	void serialize(Archive& ar);
	void serializeParameters(Archive& ar);
	void serializeRegion(Archive& ar);

	static char* worldRasterDataFileSource;
	static char* worldRGBFileSource;
	static char* worldDataFile;
	//static char* worldGeoPalFile;
	static char* worldDamPalFile;
	static char* worldLeveledTextureFile;
	static char* worldSurfaceKindFile;

	static char* worldRGBCacheFile;
	static char* worldRasterDataCacheFile;
	static char* worldMiniMapFile;

	const vector<sRect>& getChangedAreas() { return changedAreas; }
	void clearChangedAreas() { changedAreas.clear(); }
	//Procedural map
	bool isRecordingPMO() { return flag_recordingPMO; }
	void disableLoadContainerPMO(bool disable=true){disableLoadContainerPMO_=disable;}
	void playPMOperation();

	unsigned long getWorldCRC();
	unsigned long getCurrentGameMapCRC(){ return currentGameMapCRC; }
	unsigned int getGridCRC(bool fullGrid, int cnt=0, unsigned int beginCRC=startCRC32);
	unsigned int getChAreasInformationCRC();
	unsigned long getVxABufCRC();
	unsigned long getDamSurBufCRC();
	const GUID& getGuid() { return guid; }

	void setFilterHeight(int min, int max) {FilterMinHeight=min; FilterMaxHeight=max; }

	const string getWorldName() { return worldName.c_str(); }
	void setWorldName(const char* _worldName) { worldName=_worldName; }

	//Специальные функции для редактора и сети
	void toShowTryColorDamTexture(bool flag_show){ xassert(!flag_show || supBuf); flag_tryColorDamTextures=flag_show; }
	bool isShowTryColorDamTexture(){ return flag_tryColorDamTextures;}
	//enum eShowSpecialInfo{ SSI_NoShow, SSI_ShowHardnessImpassability, SSI_ShowKind  /*SSI_ShowImpassability, SSI_ShowAllGeo*/ };
	//eShowSpecialInfo showSpecialInfo;
	//void toShowSpecialInfo(eShowSpecialInfo _showSpecialInfo){ showSpecialInfo=_showSpecialInfo; }
	//vrtMap::eShowSpecialInfo IsShowSpecialInfo(){ return showSpecialInfo; }
	bool showSurKind;
	void toShowSurKind(bool _showSurKind){ showSurKind=_showSurKind; }
	bool isShowSurKind(){ return showSurKind; }

	void toShowDbgInfo(bool flag){ flag_ShowDbgInfo=flag; }
	bool IsShowDbgInfo(){ return flag_ShowDbgInfo; }

	template <class T> unsigned int getColor32(T x, T y); //for inlininig
	template <>	unsigned int getColor32<int>(int x, int y);
	template <class T> unsigned short getColor16(T x, T y); //for inlininig
	template <>	unsigned short getColor16<int>(int x, int y);
	void getTileColor32Layer(unsigned char* Texture, DWORD pitch, int xstart, int ystart, int xend, int yend, int step);
	void getTileColor16Layer(unsigned char* Texture, DWORD pitch, int xstart, int ystart, int xend, int yend, int step);

	unsigned char GetAtr(int offset) { return vxaBuf[offset]&VmAt_MASK; }
	unsigned char GetAtr(int x,int y) { return vxaBuf[y*H_SIZE+x]&VmAt_MASK; }
	void SetAtr(int offset, unsigned char newat) { vxaBuf[offset] = (vxaBuf[offset]&(~VmAt_MASK)) | (newat&VmAt_MASK); }
	void SetAtr(int x,int y, unsigned char newat) { SetAtr(offsetBuf(x,y), newat); }

	inline void setInds(int offset) { vxaBuf[offset]|=VmAt_Inds; }
	inline void setUnInds(int offset) { vxaBuf[offset]&=~VmAt_Inds; }

	inline void putAlt(int offset, unsigned short V) { vxaBuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(vxaBuf[offset]); }
	inline void putAltInds(int offset, unsigned short V) { vxaBuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(vxaBuf[offset]) | VmAt_Inds; }
	inline void putAltUnInds(int offset, unsigned short V) { vxaBuf[offset]=  Vm_prepHeigh4Buf(V) | (vxaBuf[offset]&(VmAt_MASK&(~VmAt_Inds))); }
	inline void putAlt(int x,int y, unsigned short V) { putAlt(offsetBuf(x,y),V); }
	inline void putAltC(int x,int y, unsigned short V) { putAlt(offsetBufC(x,y), V); }

	void voxSet(int x,int y,int delta) { //фильтруемое изменение высоты
		int offset=offsetBufC(x,y);
		unsigned short data=vxaBuf[offset];
		int h=Vm_extractHeigh(data);
		if(h < FilterMinHeight || h > FilterMaxHeight) return;
		h=clamp(h+delta, FilterMinHeight, FilterMaxHeight);
		vxaBuf[offset]= Vm_prepHeigh4Buf(h) | Vm_extractAtr(vxaBuf[offset]); 
	}

	template<class GenerationMetod>	void drawBitMetod(int x, int y, int rad, GenerationMetod& genMetod, short _minFH, short _maxFH);
	void drawBitGen(int x, int y, struct sTerrainBitmapBase* pTBM, bool erase=false);
	void drawBitmapCircle(int x, int y, int rad, unsigned char cntrAlpha, int bitmapIDX, short minFH, short maxFH, const ColorModificator& colorMod);
	void drawBitmapCircle(const Vect2i& center, int rad, int bitmapIDX, int alpha, float angle, float scale);
	void putBitmap2AllWorld(int bitmapIDX, short minFH, short maxFH, const ColorModificator& colorMod);
	int drawHardnessCircle(int x, int y, int rad, bool erase, short minFH, short maxFH);
	void drawInGrid(int x, int y, int rad, unsigned short flag, unsigned short mask, short minFH, short maxFH);
	void deltaZone(sToolzerPMO& var);//(int x,int y,int rad,int smth,int dh,int smode,int eql);
	void squareDeltaZone(sSquareToolzerPMO& var);//(int x,int y,int rad,int smth,int dh,int smode,int eql);
	void gaussFilter(int x,int y,int rad, float filter_scaling);
	void squareGaussFilter(int _x,int _y,int _rad, double _filter_scaling);
	void specialFilter(int _x,int _y,int _rad, int maxDH, float _kDetail, float _balansH, float kUnBrightness);

	void allWorldGaussFilter(double _filter_scaling);

	bool flag_saveChangedAreas;
	enum eTypeVMapChanges{	TypeCh_Texture=1<<0,	TypeCh_Height=1<<1,		TypeCh_Region=1<<2	};
	void regRender(int LowX,int LowY,int HiX,int HiY, char typeChanges);
	void recalcArea2Grid(int xl, int yt, int xr, int yb, bool flag_init=false);
	int convertVMapTryColor2TerClr(){ return convertVMapTryColor2TerClrT<TerrainColor>(); }
	float changeTotalWorldParam(int deltaVx, float kScale, const vrtMapChangeParam& changeParam); //общий масштаб высоты карты и общую высоту

	MultiRegion& region() { return *region_; }
	void drawMiniDetailTexture(int layer, const Vect2i& point, int radius);
	void drawMiniDetailTextureAux(int layer, const Vect2i& point, int radius);
	void drawPlacementZone(int material, const Vect2i& point, int radius, bool internal = false);
	void erasePlacementZone(const Vect2i& point, int radius);

	void registerUpdateMapClient(UpdateMapClient* client) { updateMapClients_.add(client); }
	void unregisterUpdateMapClient(UpdateMapClient* client) { updateMapClients_.remove(client); }

//////////////////////////////////////////////////////////////////////////
//	END Public function
//////////////////////////////////////////////////////////////////////////
	//UndoRedo dispatcher
	list<sPreChangedArea> preCAs;
	list<sPreChangedArea>::iterator curPreCA;
	void UndoDispatcher_PutPreChangedArea(list<sRect>& _chAreaList, bool saveVx, bool saveRGB, sBasePMOperation* pPMO=0/*ElementPMO* pPMO=0*/);
	void UndoDispatcher_PutPreChangedArea(sRect& _chArea, bool saveVx, bool saveRGB, sBasePMOperation* pPMO=0/*ElementPMO* pPMO=0*/);
	void UndoDispatcher_Undo();
	//void UndoDispatcher_KillLastChange(sBasePMOperation* pPMO/*ElementPMO* pPMO*/);
	void UndoDispatcher_Redo();
	void UndoDispatcher_KillAllUndo();
	bool UndoDispatcher_IsUndoExist();
	bool UndoDispatcher_IsRedoExist();

	void generateChAreasInformation(XBuffer& out);
	void compareChAreasInformation(unsigned char* pFirstCAI, unsigned char* pSecondCAI, XBuffer& textOut, XBuffer& binOut);
	void displayChAreas(unsigned char* pd, unsigned int dsize);

	//Toolzer-ные ф-и
	inline void digAndNoChange(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data));
	}
	inline void digAndSetInds(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)|VmAt_Inds);
	}
	inline void digAndUnInds(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)&(~VmAt_Inds));
	}
	inline void putAndNoChange(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data));
	}
	inline void putAndSetInds(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)|VmAt_Inds);
	}
	inline void putAndUnInds(int offset, int dv){
		unsigned short data=vxaBuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		vxaBuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)&(~VmAt_Inds));
	}
	void autoLace(int laceH, float angle);
///////////////////////////////////////////////////////////////////
protected:
	enum { SVMPHEADER_ID_SIZE = 4 };
	struct sVmpHeader{
		char id[SVMPHEADER_ID_SIZE];
		unsigned short XSize;
		unsigned short YSize;
		unsigned int AUX1;
		unsigned int AUX2;
		unsigned int AUX3;
		sVmpHeader() { /*id[0]='S'; id[1]='M'; id[2]='W'; id[3]='0';*/ }
		void setID(const char* pVmpID);
		bool cmpID(const char* pVmpID);
	};

	vector<sRect> changedAreas;
	unsigned char* gridChAreas;
	unsigned char* gridChAreas2;
	//vector<sRect> renderAreas;
	//PMO
	bool flag_recordingPMO;
	bool disableLoadContainerPMO_;
	ContainerPMO containerPMO;
	unsigned int curNewPMO;
	void clearContainerPMO();
	ContainerPMO::iterator getCurNewPMO(){ return containerPMO.begin()+curNewPMO; }
	void setCurNewPMO(ContainerPMO::iterator p){ curNewPMO=p-containerPMO.begin(); }
	__int64 procMapOp;
	void addProcMapOpStatistic(int sx, int sy){ procMapOp+=sx*sy; }

	unsigned short* vxaBuf;
	TerrainColor * clrBuf;
	//unsigned char* RnrBuf;
	unsigned long* supBuf;

    unsigned long VxABufCRC;
    unsigned long DamSurBufCRC;
	unsigned long currentGameMapCRC;

	float inv_vx_fraction;

	XGUID guid;

	MultiRegion* region_;

	struct PlacementZoneCircle
	{
		Vect2i position;
		int radius;
		int material;
		void serialize(Archive& ar);
		PlacementZoneCircle(const Vect2i& _position = Vect2i::ZERO, int _radius = 0, int _material = 0) : position(_position), radius(_radius), material(_material) {}
		bool operator==(const PlacementZoneCircle& circle) const { return position == circle.position && radius == circle.radius; }
	};
	typedef UniqueVector<PlacementZoneCircle> PlacementZoneCircles;
	PlacementZoneCircles placementZoneCircles_;

	typedef UniqueVector<UpdateMapClient*> UpdateMapClients;
	UpdateMapClients updateMapClients_;

	//Перенести!
public:
	unsigned short* gVBuf; //Сетка вокселей
	unsigned short* gABuf; //сетка атрибутов
	unsigned int GH_SIZE;
	unsigned int clip_mask_x_g;
	unsigned int GV_SIZE;
	unsigned int clip_mask_y_g;
protected:

	void clearGridChangedAreas();
	void updateGridChangedAreas2();
	bool checkGridChangedAreas2();

	Color4c DamPal[MAX_DAM_SURFACE_TYPE];

	int FilterMinHeight;
	int FilterMaxHeight;

	bool flag_worldLoaded;
	unsigned long worldSerial;

	string worldName;
	string worldsDir;

	TerrainColor veryLightDam; //Самый яркий цвет

	bool flag_ShowDbgInfo;
	bool* pTempArray;//Массив для показа несовпадающих областе (при игре по сети)

	template <class T>	unsigned short getColor16T(int off) { xassert(0&& "Not support TerrainColor! "); return 0; }
	template <class T> unsigned int getColor32T(int off) { xassert(0&& "Not support TerrainColor! "); return 0;}
	template <>	unsigned short getColor16T<unsigned char>(int off) { return unsigned short(ConvertTry2HighColor(DamPal[clrBuf[off]].argb)); }
	template <>	unsigned int getColor32T<unsigned char>(int off) { return DamPal[clrBuf[off]].argb;}
	template <>	unsigned short getColor16T<unsigned short>(int off) { return clrBuf[off]; }
	template <>	unsigned int getColor32T<unsigned short>(int off) {
		int c=clrBuf[off];
		return ((c&0xF800)<<8) | ((c&0x07E0)<< 5) | ((c&0x01F)<<3);
	}

	int renderBox(int LowX,int LowY,int HiX,int HiY, char typeChanges);

	void RenderPrepare1();
	int RenderStr(int XL, int Y, int dx);

	void calc_Flood(int x0,int y0,int level,int ttype,int ti = 0);
	void WORLD_Drift(int x0,int y0,int level,int ttype);

	//Special function 4 Surmap
	//int putTgaComplete2AllMap();

	template<class T> int convertVMapTryColor2TerClrT();
	template <> int convertVMapTryColor2TerClrT<unsigned char>();
	template <> int convertVMapTryColor2TerClrT<unsigned short>();

	void wrldShotMapHeight();
	void FlipWorldH();
	void FlipWorldV();
	void RotateWorldP90();
	void RotateWorldM90();

	void scalingHeighMap(int percent); //Уменьшать увеличивать общий масштаб высоты карты

	bool worldChanged;
	bool flag_tryColorDamTextures;

	//Вспомогательные функции загрузки
	void initGeneralVariable();
	void allocMem4Buf(bool flag_useTryColorBuffer);
	void releaseMem4Buf();

	void prepareCircleArr();
	void releaseCircleArr();

	void createDamPal();
	void loadDamPal();
	void saveDamPal();

	bool loadVMP(const char* file, bool flag_NoCheckCRC=0);
	void saveVMPCach(const char* _fname, unsigned int _crcVxA, unsigned int _crcDam);
	bool loadRGBCache(const char* fname);
	void setRGBCacheInSurBuf();
	void createBeginSurface(unsigned short* vxbuf=0, unsigned long* rgb=0);
	void convertPath2WorldName(const char* _dirName);

	int buildMountainWorld();
	//Функции для работы с сеткой
	void initGrid();
	void loadSurKind();
	void saveSurKind();
	//Aux
    void convertMapSize(const char* _fname);
	void convertS5L3ToS5L4();
	void laceLine(int hbeg, float a, int xBeg, int yBeg, int cntMax, int dx, int dy);

	friend class TerToolsDispatcher;
	friend class TerToolTextureBase;
	friend class TerToolTexture;
	template<enum eSetingAtrMetod, enum eTerToolTerrainEffect , enum eTerToolSurfaceEffect , bool> friend  class elementarTool;
	friend class RoadTool;
	friend struct CLandslip;
	template<enum toolzer2TerrainEffect> friend class individualToolzer;
	friend void geoGeneration(sGeoPMO& var);
};
extern vrtMap vMap;

void geoGeneration(sGeoPMO& var);
//Вспомогательные фукции
extern bool testExistingFile(const char* fName);
extern unsigned char* convert_vox2vid(int vox, char* buf);
extern int convert_vid2vox(char* buf);

#endif //__VMAP_H__
