#ifndef __VMAP_H__
#define __VMAP_H__

#include "crc.h"
#include "xmath.h"
#include "..\Render\inc\umath.h"
#include "procedurMap.h"
#include "worldFileDispatcher.h"
#include "quantizer.h"
#include "terra.h"
#include "undoDispatcher.h"

class Archive;

struct sWorldInfo {
	string worldName;
	unsigned short sizeX;
	unsigned short sizeY;
	sWorldInfo(const char* wname, unsigned short sizex, unsigned short sizey){
		worldName=wname;
		sizeX=sizex;
		sizeY=sizey;
	}
};

#define kmGrid (2) // Это 2^2 - сетка 4x4
#define sizeCellGrid (1<<kmGrid)

#define TERRAIN_TYPES_NUMBER 4
#define GRIDAT_MASK_SURFACE_KIND (TERRAIN_TYPES_NUMBER - 1)
#define GRIDAT_IMPASSABILITY (0x4)
#define GRIDAT_MASK_IMPASSABILITY_AND_SURFACE_KIND (0x7)

#define GRIDAT_LEVELED (0x10)
#define GRIDAT_INDESTRUCTABILITY (0x20)

#define GRIDAT_BUILDING (0x40)
#define GRIDAT_BASE_OF_BUILDING_CORRUPT (0x80)

#define GRIDTST_BUILDING(V) (V&GRIDAT_BUILDING)
#define GRIDTST_LEVELED(V) (V&GRIDAT_LEVELED)

// Сетка отражения изменений на воксельной поверхности
#define kmGridChA (6) // 2^6 -сетка 64x64
#define sizeCellGridCA (1<<kmGridChA)

// Основная структура карты
struct vrtMapCreationParam {
	enum eCreateWorldMetod {
		FullPlain, Mountains
	};
	enum Quality {
		QUALITY_BAD, QUALITY_GOOD
	};
    enum SIZE_POWER {
        SIZE_128  = 7,	SIZE_256  = 8,	SIZE_512  = 9,	SIZE_1024 = 10,
        SIZE_2048 = 11,	SIZE_4096 = 12,	SIZE_8192 = 13
    };

	SIZE_POWER H_SIZE_POWER;
	SIZE_POWER V_SIZE_POWER;
	eCreateWorldMetod createWorldMetod;
    unsigned char initialHeight;

	vrtMapCreationParam() {
        H_SIZE_POWER = SIZE_2048;
        V_SIZE_POWER = SIZE_2048;
        initialHeight = 30;
		createWorldMetod = FullPlain;
	}
	void serialize(Archive& ar);
};

struct vrtMap : public vrtMapCreationParam {

//////////////////////////////////////////////////////////////////////////
//	PUBLIC fuction для использования 
//////////////////////////////////////////////////////////////////////////
	void prepare(const char* _worldsDir);
	class cTileMap*& currentTileMap();
	void linking2TileMap(class cTileMap* tm);
	const char* getTargetName(const char* name);
	const char* getWorldsDir() { return worldsDir.c_str(); }

	void ShadowControl(bool shadow);
	void WorldRender(void);
	void renderQuant(void);

	void create(const char* _worldName = "");
	bool load(const char* _worldName, bool flag_useTryColorBuffer=false);
	void save(const char* _worldName, int _indexColorsUsed=-1);
	bool deleteWorld(const char* _worldName);
	void rebuild();
	void putNewGeoTexture(const char* fnameMatGeoTx, const char* fnameLoopedGeoTx);
	bool initGeoTexture();
	void saveMiniMap(int sizex, int sizey);
	bool getListWorldInfo(list<sWorldInfo>& worldinfoList);

	bool loadGameMap(Archive& ar);
	bool saveGameMap(Archive& ar);

	void releaseWorld(void);
	bool isWorldLoaded(void){
		return flag_worldLoaded;
	}

	bool IsChanged(void) { return worldChanged;}
	bool isLoad(void) { return (VxABuf!=0);}
	bool isCircleSurfaceLeveled(const Vect2i& center, const int radius);

	void generateChAreasInformation(XBuffer& out);
	unsigned long getWorldCRC(void);
	void compareChAreasInformation(unsigned char* pFirstCAI, unsigned char* pSecondCAI, XBuffer& textOut, XBuffer& binOut);
	void displayChAreas(unsigned char* pd, unsigned int dsize);
	unsigned int getGridCRC(bool fullGrid, int cnt=0, unsigned int beginCRC=startCRC32);
	unsigned int getChAreasInformationCRC();

	int XCYCL(int x)	{ return (x) & clip_mask_x; }
	int YCYCL(int y)	{ return (y) & clip_mask_y; }

	int XCYCLG(int x)	{ return (x) & clip_mask_x_g; }
	int YCYCLG(int y)	{ return (y) & clip_mask_y_g; }

	int w2m(int x) { return x >> kmGrid; } // World to map
	int m2w(int x) { return x << kmGrid; } // Map to world

	int w2mClampX(int x) { return w2m(clamp(x, 0, (int)H_SIZE - 1)); } // Clamped
	int w2mClampY(int y) { return w2m(clamp(y, 0, (int)V_SIZE - 1)); }
	
	int m2wHalf(int x) { return m2w(x) + (1 << (kmGrid - 1)); } // Map to world plus half
		
	Vect2i w2m(const Vect2i& v) { return Vect2i(w2m(v.x), w2m(v.y)); }
	Vect2i m2w(const Vect2i& v) { return Vect2i(m2w(v.x), m2w(v.y)); }

	//function для работы с буфером
	//Базовая функция вычисления смещения в буфферах
	int offsetBuf(int x, int y) { return y*H_SIZE+x; }
	int offsetBufC(int x, int y) { return YCYCL(y)*H_SIZE+XCYCL(x); }
	int offsetGBuf(int x, int y) { return y*GH_SIZE+x; }
	int offsetGBufC(int x, int y) { return YCYCLG(y)*GH_SIZE+XCYCLG(x); }

	int offsetGBufWorldC(int x, int y) { return YCYCLG(y >> kmGrid)*GH_SIZE+XCYCLG(x >> kmGrid); }

	int offsetBuf2offsetGBuf(int offBuf){
		return ( (offBuf>>kmGrid)&clip_mask_x_g ) | ( (offBuf>>(2*kmGrid))&(~clip_mask_x_g) );
	}

	// Шурина! возвращает усредненную высоту по координатам сетки!
	unsigned char getGridVoxel(int x, int y) { return GVBuf[offsetGBuf(x,y)]; }
	//Шурина! возвращает усредненную высоту по координатам сетки!
	unsigned char getGridVoxelC(int x, int y) { return GVBuf[offsetGBufC(x,y)]; }
	// возвращает усредненную высоту по координатам мира
	unsigned char getVoxelW(int x, int y) { return GVBuf[offsetGBufC(w2m(x), w2m(y))]; }
	int GetApproxAlt(int x, int y){ return GVBuf[offsetGBuf(x>>kmGrid, y>>kmGrid)]; }

	float analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized);
	void findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax);
	bool checkUndestructability(const Vect2i& center, int radius);


	bool isImpassability(const Vect2i& pos, int r);
	enum eSettingImpassabilityMetod{
		SIMM_NotChangeImpassability,
		SIMM_SetImpassability,
		SIMM_UnSetImpassability
	};
	enum eSettingSurfaceKindMetod {
		SurfaceKind_NoChange=-1,
		SurfaceKind_Set1=0,
		SurfaceKind_Set2=1,
		SurfaceKind_Set3=2,
		SurfaceKind_Set4=3,
	};
	void setImpassabilityAndSurKind(int _x, int _y, int _r, eSettingImpassabilityMetod impassability, eSettingSurfaceKindMetod surKind);
	void setInds(int xc, int yc, int r, bool erase=false);
	int getTerrainType(const Vect2i& pos, int r);

	void pointDamageBuildings(int _x, int _y) {
		int offG=offsetGBuf(XCYCLG(_x>>kmGrid), YCYCLG(_y>>kmGrid));
		if(GABuf[offG]&GRIDAT_BUILDING) GABuf[offG]|=GRIDAT_BASE_OF_BUILDING_CORRUPT;
	}

	unsigned char getSurKindG(int xg, int yg){
		return GABuf[offsetGBufC(xg, yg)]&GRIDAT_MASK_SURFACE_KIND;
	}
	unsigned char getSurKind(int xw, int yw){
		return GABuf[offsetGBufWorldC(xw, yw)]&GRIDAT_MASK_SURFACE_KIND;
	}
	void setSurKindG(int xg, int yg, unsigned char sk){
		GABuf[offsetGBufC(xg, yg)]=(sk&GRIDAT_MASK_SURFACE_KIND) | (GABuf[offsetGBufC(xg, yg)]&(~GRIDAT_MASK_SURFACE_KIND)) ;
	}
	bool isIndestructabilityG(short xg, short yg){
		return GABuf[offsetGBufC(xg, yg)]&GRIDAT_INDESTRUCTABILITY;
	}
	bool isIndestructability(short x, short y){
		return GABuf[offsetGBufWorldC(x, y)]&GRIDAT_INDESTRUCTABILITY;
	}

	bool isLeveled(int off){ return Vm_IsLeveled(VxABuf[off]); }

//////////////////////////////////////////////////////////////////////////
//	END Public function
//////////////////////////////////////////////////////////////////////////

	enum { SVMPHEADER_ID_SIZE = 4 };

	struct sVmpHeader{
		char id[SVMPHEADER_ID_SIZE];
		unsigned short XSize;
		unsigned short YSize;
		unsigned int AUX1;
		unsigned int AUX2;
		unsigned int AUX3;
		sVmpHeader() {
			//id[0]='S'; id[1]='M'; id[2]='W'; id[3]='0';
		}
		void setID(const char* pVmpID){
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
		bool cmpID(const char* pVmpID){
			for(int i=0; i< SVMPHEADER_ID_SIZE; i++){
				if(*pVmpID!=id[i]) return false;
				if(*pVmpID!=0) pVmpID++;
				else return false;
			}
			return true;
		}
		
	};

	static char* worldRasterDataFileSource;
	static char* worldRGBFileSource;
	static char* worldDataFile;
	static char* worldGeoPalFile;
	static char* worldDamPalFile;
	static char* worldLeveledTextureFile;
	static char* worldSurfaceKindFile;

	static char* worldRGBCacheFile;
	static char* worldRasterDataCacheFile;
	static char* worldMiniMapFile;
    
	vector<sRect> changedAreas;
	unsigned char* gridChAreas;
	unsigned char* gridChAreas2;

	vector<sRect> renderAreas;

///////////////////////////////////////////////////////////////////
	//UndoRedo dispatcher
	list<sPreChangedArea> preCAs;
	list<sPreChangedArea>::iterator curPreCA;
	void UndoDispatcher_PutPreChangedArea(list<sRect>& _chAreaList, bool saveVx, bool saveRGB, sBasePMOperation* pPMO=0/*ElementPMO* pPMO=0*/);
	void UndoDispatcher_PutPreChangedArea(sRect& _chArea, bool saveVx, bool saveRGB, sBasePMOperation* pPMO=0/*ElementPMO* pPMO=0*/);
	void UndoDispatcher_Undo(void);
	void UndoDispatcher_KillLastChange(sBasePMOperation* pPMO/*ElementPMO* pPMO*/);
	void UndoDispatcher_Redo(void);
	void UndoDispatcher_KillAllUndo(void);
	bool UndoDispatcher_IsUndoExist(void);
	bool UndoDispatcher_IsRedoExist(void);
///////////////////////////////////////////////////////////////////
	//Procedural map
	bool flag_record_operation;
	//ContainerPMO_Old containerPMO_Old;
	void disableLoadContainerPMO(bool disable=true){disableLoadContainerPMO_=disable;}
protected:
	bool disableLoadContainerPMO_;
public:
	ContainerPMO containerPMO;
	unsigned int curNewPMO;
	void clearContainerPMO();
	ContainerPMO::iterator getCurNewPMO(){
		return containerPMO.begin()+curNewPMO; 
	}
	void setCurNewPMO(ContainerPMO::iterator p){
		curNewPMO=p-containerPMO.begin();
	}
	__int64 procMapOp;
	void addProcMapOpStatistic(int sx, int sy){
		procMapOp+=sx*sy;
	}

	void playPMOperation(void);

	void serialize(Archive& ar);
	void serializeParameters(Archive& ar);

///////////////////////////////////////////////////////////////////
    // From vrtMapParam
	string geoTxFileName;
	string matGeoTxFileName;
	string loopedGeoTxFileName;
	//ColorQuantizer geoTxColorQuantizer;
	RGBOctree geoTxOctree;
	Bitmap32 loopedGeoTx;
	Bitmap32 matGeoTx;
	int kXScaleMatGeoTx;
	int kYScaleMatGeoTx;
//////////////////////////////////////////////////////////////////////////////

	unsigned int H_SIZE;
	unsigned int clip_mask_x;

	unsigned int V_SIZE;
	unsigned int clip_mask_y;


	unsigned short* VxABuf;
	unsigned char* SurBuf;
	unsigned char* RnrBuf;

	unsigned long* SupBuf;

    unsigned long VxABufCRC;
    unsigned long GeoSurBufCRC;
    unsigned long DamSurBufCRC;
	unsigned long currentGameMapCRC;
	GUID guid;
	unsigned long getVxABufCRC(void);
	unsigned long getGeoSurBufCRC(void);
	unsigned long getDamSurBufCRC(void);

	unsigned char* LvdTex;
	unsigned int LvdTex_clip_mask_x;
	unsigned int LvdTex_clip_mask_y;
	unsigned char LvdTex_X_SIZE_POWER;
	unsigned char LvdTex_Y_SIZE_POWER;
	unsigned int LvdTex_spec_clip_mask_y;
	unsigned char LvdTex_SPEC_SHIFT_X;
	unsigned char LvdTex_SPEC_SHIFT_Y;

	unsigned char* GVBuf; //Сетка вокселей
	unsigned short* GABuf; //сетка атрибутов
	unsigned int GH_SIZE;
	unsigned int clip_mask_x_g;
	unsigned int GV_SIZE;
	unsigned int clip_mask_y_g;


	void clearGridChangedAreas(void);
	void updateGridChangedAreas2(void);
	bool checkGridChangedAreas2(void);


	unsigned short Sur2Col[MAX_SURFACE_TYPE][MAX_SURFACE_LIGHTING*2]; //2 это два слоя Dam и ZP
	unsigned short Tex2Col[MAX_SURFACE_LIGHTING*2][MAX_SURFACE_TYPE];

	unsigned long Sur2Col32[MAX_SURFACE_TYPE][MAX_SURFACE_LIGHTING*2]; //2 это два слоя Dam и ZP
	unsigned long Tex2Col32[MAX_SURFACE_LIGHTING*2][MAX_SURFACE_TYPE];

	sColor4c GeoPal[SIZE_GEO_PALETTE];
	sColor4c DamPal[MAX_DAM_SURFACE_TYPE];
	unsigned char TexPal[SIZE_DAM_PALETTE];
	int indexColorsUsed;

	int FilterMinHeight;
	int FilterMaxHeight;

	bool flag_worldLoaded;
	unsigned long worldSerial;

	string worldName;
	string worldsDir;

	unsigned char veryLightDam; //Самый яркий цвет

	vrtMap(void);
	~vrtMap(void);

	//Специальные функции для редактора и сети
	void toShowTryColorDamTexture(bool flag_show){
		xassert(!flag_show || SupBuf);
		flag_tryColorDamTextures=flag_show;
	}
	bool isShowTryColorDamTexture(){ return flag_tryColorDamTextures;}
	bool flag_ShowDbgInfo;
	bool* pTempArray;//Массив для показа несовпадающих областе (при игре по сети)
	enum eShowSpecialInfo{
		SSI_NoShow,
		SSI_ShowHardness,
		SSI_ShowKind,
		SSI_ShowImpassability,
		SSI_ShowAllGeo
	};
	eShowSpecialInfo showSpecialInfo;
	void toShowSpecialInfo(eShowSpecialInfo _showSpecialInfo){ showSpecialInfo=_showSpecialInfo; }
	vrtMap::eShowSpecialInfo IsShowSpecialInfo(){ return showSpecialInfo; }
	void toShowDbgInfo(bool flag){ flag_ShowDbgInfo=flag; }
	bool IsShowDbgInfo(void){ return flag_ShowDbgInfo; }
	unsigned int getColor32(int x, int y){
		int off=offsetBuf(x,y);
		return getColor32(off);
	}

	unsigned int getColor32(int off){
		unsigned int col32;
		//unsigned short col=Sur2Col[SurBuf[off]][RnrBuf[off]];
		const int brightness=255;
		if(Vm_IsLeveled(VxABuf[off])){
			unsigned int texOff=off&LvdTex_clip_mask_x;
			texOff|=(off>>LvdTex_SPEC_SHIFT_X)&LvdTex_spec_clip_mask_y;
			const unsigned int mask=0x00FEFEFF;
			col32=Tex2Col32[brightness][LvdTex[texOff]]; //col32 = SurZPE2Col32[RnrBuf[off]][SurBuf[off]];
			col32= ( (col32&mask) + (Sur2Col32[SurBuf[off]][brightness]&mask) ) >> 1;
		}
		else{
			if(Vm_IsDam(VxABuf[off])){
				if(flag_tryColorDamTextures)
                    col32=SupBuf[off];//Sur2Col32[SurBuf[off]][brightness];
				else 
                    col32=DamPal[SurBuf[off]].argb;//Sur2Col32[SurBuf[off]][brightness];
			}
			else
				col32=GeoPal[SurBuf[off]].argb;//Sur2Col32[SurBuf[off]][127];
		}

#ifndef _FINAL_VERSION_
		if(flag_ShowDbgInfo){
			if( (GABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BUILDING) != 0) col32|=0xFF;
			if( (GABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BASE_OF_BUILDING_CORRUPT) != 0) col32|=0xFF0000;
			//if( (GABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_LEVELED ) != 0) col32|=0x00ff00;
			//if( GABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_HARDNESS == GRIDAT_MASK_HARDNESS ) col32|=0xFF00;
		}
		const unsigned int mask=0x00FEFEFF;
		switch(showSpecialInfo){
		case SSI_NoShow:
			break;
		case SSI_ShowHardness:
			if(GABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_INDESTRUCTABILITY)
				col32=((0xFFFFFF&mask) + (col32&mask))>>1;
			//if(Vm_IsIndestructability(VxABuf[off])) 
			//	col32=((0xFFFF00&mask) + (col32&mask))>>1;
			break;
		case SSI_ShowKind:
			{
				unsigned char tk=GABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_SURFACE_KIND;
				tk*=8;
				unsigned int m=0xffff0000;
				m>>=tk;
				col32=((m&mask) + (col32&mask))>>1;
			}
			break;
		case SSI_ShowImpassability:
			if((GABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_IMPASSABILITY)) col32=((0xFFFFFF&mask) + (col32&mask))>>1;
			break;
		case SSI_ShowAllGeo:
			col32=GeoPal[GetGeoType(off,128<<VX_FRACTION)].argb;
			break;
		}
#endif
		return col32;
	}

	void drawTile(char* Texture, unsigned long pitch,int xstart,int ystart,int xend,int yend,int step);



	////////////////////////////////////////////////////
	unsigned char GetGeoType(int offset, int h);
	unsigned char GetGeoType(int x, int y, int h) {
		return GetGeoType(offsetBuf(x,y), h); // (x+y)%
	}
	unsigned long GetGeoColor(int x, int y);

	//..................................................

	///////////////////////////////////////
	unsigned char GetTer(int offset) { return (SurBuf[offset]); }
	unsigned char GetTer(int x,int y) { return GetTer(offsetBuf(x,y)); }
	//......................................

	////////////////////////////////////////
	void SetTer(int offset, unsigned char newtype) { SurBuf[offset] = newtype; }
	void SetTer(int x,int y, unsigned char newtype) { SurBuf[offsetBuf(x,y)] = newtype; }
	//.......................................


	unsigned char GetAtr(int offset) { return VxABuf[offset]&VmAt_MASK; }
	unsigned char GetAtr(int x,int y) { return VxABuf[y*H_SIZE+x]&VmAt_MASK; }

	void SetAtr(int offset, unsigned char newat) {
		VxABuf[offset] = (VxABuf[offset]&(~VmAt_MASK)) | (newat&VmAt_MASK);
	}
	void SetAtr(int x,int y, unsigned char newat) {
		SetAtr(offsetBuf(x,y), newat);
	}
	void SetGeo(int offset){ VxABuf[offset]&=~VmAt_GeoDam; }
	void SetGeo(int offset, unsigned short h){
		VxABuf[offset]=Vm_prepHeighAtr(h, VxABuf[offset]&(VmAt_MASK&(~VmAt_GeoDam)) );
	}
	void SetGeoAndGeoRecalc(int offset){
		unsigned short v=Vm_extractHeigh(VxABuf[offset]);
		SurBuf[offset]=GetGeoType(offset, v);
		VxABuf[offset]=Vm_prepHeighAtr(v, VmAt_Nrml_Geo);
	}
	void SetGeoAndGeoRecalc(int offset, unsigned short h){
		SurBuf[offset]=GetGeoType(offset, h);
		VxABuf[offset]=Vm_prepHeighAtr(h, VmAt_Nrml_Geo);
	}


	//.........................................
	inline unsigned short GetAlt(int offset) { return Vm_extractHeigh(VxABuf[offset]); }
	inline unsigned short GetAlt(int x,int y) { return GetAlt(offsetBuf(x,y)); }
	//.......................................
	inline unsigned short GetAltC(int x,int y) { return GetAlt(XCYCL(x), YCYCL(y)); }


	inline unsigned char GetAltWhole(int offset) { return *( ((unsigned char*)(&VxABuf[offset]))+1 ); }
	inline unsigned char GetAltWhole(int x,int y) { return GetAltWhole(offsetBuf(x,y)); }


	inline void PutSurAndNoChange(int offset, unsigned char sur) { SurBuf[offset]=sur; }
	inline void PutSurAndSetInds(int offset, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]|=VmAt_Inds;
	}
	inline void PutSurAndUnInds(int offset, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]&=~VmAt_Inds;
	}
	inline void PutSurAndSetDam(int offset, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]|=VmAt_GeoDam;
	}

	/////////// FUNCTION PutAlt ///////////
	inline void PutAlt(int offset, unsigned short V) { 
		VxABuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(VxABuf[offset]); 
	}
	inline void PutAltInds(int offset, unsigned short V) {
		VxABuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(VxABuf[offset]) | VmAt_Inds;
	}
	inline void PutAltUnInds(int offset, unsigned short V) {
		VxABuf[offset]=  Vm_prepHeigh4Buf(V) | (VxABuf[offset]&(VmAt_MASK&(~VmAt_Inds)));
	}
	inline void PutAlt(int x,int y, unsigned short V) { PutAlt(offsetBuf(x,y),V); }
	inline void PutAltC(int x,int y, unsigned short V) { PutAlt(offsetBufC(x,y), V); }
	inline void PutAltAndNoChange(int offset, unsigned short V, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(VxABuf[offset]);
	}
	//.........................................
	inline void PutAltAndClearZL(int offset, int V) {
		VxABuf[offset]= Vm_prepHeigh4Buf(V) | Vm_setNormalAtr(VxABuf[offset]);
	}
	inline void PutAltAndGeoRecalc(int offset, unsigned short V) {
		if(Vm_IsGeo(VxABuf[offset]))
			SurBuf[offset]=GetGeoType(offset,V);
		VxABuf[offset]=  Vm_prepHeigh4Buf(V) | Vm_extractAtr(VxABuf[offset]);
	}
	inline void PutAltAndSetGeo(int offset, unsigned short V) {
		if(Vm_IsDam(VxABuf[offset])){
			SurBuf[offset]=GetGeoType(offset,V);
		}
		VxABuf[offset]= Vm_prepHeigh4Buf(V) | VmAt_Nrml_Geo;
	}
	inline void PutAltAndSetGeo(int offset, unsigned short V, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]= Vm_prepHeigh4Buf(V) | VmAt_Nrml_Geo;
	}
	inline void PutAltAndSetDamNrml(int offset, unsigned short V, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]= Vm_prepHeigh4Buf(V) | VmAt_Nrml_Dam;
	}
	inline void PutAltAndSetDamInds(int offset, unsigned short V, unsigned char sur) {
		SurBuf[offset]=sur;
		VxABuf[offset]= Vm_prepHeigh4Buf(V) | VmAt_Nrml_Dam_Inds;
	}

	inline void PutAltAndSetLeveling(int offset, unsigned short V) {
		VxABuf[offset] = Vm_prepHeigh4Buf(V) | (VxABuf[offset]&VmAt_MASK) | VmAt_Lvld;
	}

	inline void dig(int x, int y, int dv){
		unsigned int offset=offsetBuf(x, y);
		//SSDig(off, dv);
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data);
		v-=dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_setNormalAtr(data));
	}

	///////////////////
	inline void DigAndNoChange(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data));
	}
	inline void DigAndSetInds(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)|VmAt_Inds);
	}
	inline void DigAndUnInds(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)-dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)&(~VmAt_Inds));
	}
	inline void PutAndNoChange(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data));
	}
	inline void PutAndSetInds(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)|VmAt_Inds);
	}
	inline void PutAndUnInds(int offset, int dv){
		unsigned short data=VxABuf[offset];
		int v=Vm_extractHeigh(data)+dv;
		if(v<0) v=0;
		VxABuf[offset]=Vm_prepHeighAtr(v, Vm_extractAtr(data)&(~VmAt_Inds));
	}
	////////////////////
	
	void voxSet(int x,int y,int delta) {
		x=XCYCL(x);
		y=YCYCL(y);
		int offset=offsetBuf(x,y);

		unsigned short data=VxABuf[offset];
		int h=Vm_extractHeigh(data);
		if(h < FilterMinHeight || h > FilterMaxHeight) return;
		h=clamp(h+delta, FilterMinHeight, FilterMaxHeight);
		if(Vm_IsGeo(data)){
			SurBuf[offset]=GetGeoType(offset, h);
			//VxABuf[offset]=Vm_prepHeighAtr(h, VmAt_Nrml_Geo);
			VxABuf[offset]=  Vm_prepHeigh4Buf(h) | Vm_extractAtr(VxABuf[offset]); 
		}
		else{ 
			//VxABuf[offset]=Vm_prepHeighAtr(h, VmAt_Nrml_Dam);
			VxABuf[offset]= Vm_prepHeigh4Buf(h) | Vm_extractAtr(VxABuf[offset]); 
		}
	}

	void putAltSpecial(int offset, unsigned short V){
		unsigned short data=VxABuf[offset];
		if( Vm_extractHeighWhole(data) ==(V>>VX_FRACTION) )
			VxABuf[offset]=Vm_prepHeighAtr(V, Vm_extractAtr(data));
		else 
			VxABuf[offset]=Vm_prepHeighAtr(V, Vm_setNormalAtr(data));
	}


	template<class GenerationMetod>	void drawBitMetod(int x, int y, int rad, GenerationMetod& genMetod, int filterHeigh);

	void drawBitGen(int x, int y, struct sTerrainBitmapBase* pTBM, bool erase=false);
	void drawBitmapCircle(int x, int y, int rad, unsigned char cntrAlpha, int bitmapIDX, short minFH=0, short maxFH=MAX_VX_HEIGHT);
	void drawBitmapCircle(const Vect2i& center, int rad, int bitmapIDX, int alpha, float angle, float scale);
	int drawHardnessCircle(int x, int y, int rad, bool erase);
	void drawInGrid(int x, int y, int rad, unsigned short flag, unsigned short mask);
	void deltaZone(sToolzerPMO& var);//(int x,int y,int rad,int smth,int dh,int smode,int eql);
	void squareDeltaZone(sSquareToolzerPMO& var);//(int x,int y,int rad,int smth,int dh,int smode,int eql);
	void gaussFilter(int x,int y,int rad, float filter_scaling);
	void squareGaussFilter(int _x,int _y,int _rad, double _filter_scaling);

	void allWorldGaussFilter(double _filter_scaling);

	bool flag_saveChangedAreas;
	enum eTypeVMapChanges{
		TypeCh_Texture=1<<0,
		TypeCh_Height=1<<1,
		TypeCh_Region=1<<2
	};
	void RenderRegStr(int Yh,int Yd);
	void regRender(int LowX,int LowY,int HiX,int HiY, char typeChanges);
	int renderBox(int LowX,int LowY,int HiX,int HiY, char typeChanges);

	void RenderPrepare1(void);
	int RenderStr(int XL, int Y, int dx);
	void RenderStr(int Y);

	void recalcArea2Grid(int xl, int yt, int xr, int yb, bool flag_init=false);

	void calc_Flood(int x0,int y0,int level,int ttype,int ti = 0);
	void WORLD_Drift(int x0,int y0,int level,int ttype);

	//Special function 4 Surmap
	//int putTgaComplete2AllMap(void);
	int convertVMapTryColor2IdxColor(bool flag_useTerToolColor=true);
	void wrldShotMapHeight(void);
	void FlipWorldH(void);
	void FlipWorldV(void);
	void RotateWorldP90(void);
	void RotateWorldM90(void);

	void scalingHeighMap(int percent); //Уменьшать увеличивать общий масштаб высоты карты
	void changeTotalWorldHeight(int deltaVx, float kScale); //общий масштаб высоты карты и общую высоту

protected:
	bool shadow_control;
	bool worldChanged;
	bool flag_tryColorDamTextures;

	//Вспомогательные функции загрузки
	void initGeneralVariable(void);
	void allocMem4Buf(bool flag_useTryColorBuffer);
	void releaseMem4Buf(void);

	void allocChAreaBuf();
	void releaseChAreaBuf();

	void prepareCircleArr(void);
	void releaseCircleArr(void);

	void createGeoDamPal(void);
	void loadGeoDamPal(void);
	void saveGeoDamPal(void);

	void restore3Buf(void);

	bool loadVMP(const char* file, bool flag_NoCheckCRC=0);
	void saveVMPCach(const char* _fname, unsigned int _crcVxA, unsigned int _crcGeo, unsigned int _crcDam);
	bool loadRGBCache(const char* fname);
	void setRGBCacheInSurBuf();
	void createBeginSurface();
	void convertPath2WorldName(const char* _dirName);

	enum eSurfaceMode{
		Geologic, Damming
	};
	void convertPal2TableSurCol(const sColor4c palBuf[SIZE_GEO_PALETTE],eSurfaceMode SurfaceMode);
	void convertPal2TableTexCol(void);
	void delLeveledTexture(void);
	int loadLeveledTexture(void);
	int convertSupBuf2SurBuf();

	int buildMountainWorld(void);
	//Функции для работы с сеткой
	void initGrid(void);
	void loadSurKind(void);
	void saveSurKind(void);
	//Aux
    void convertMapSize(const char* _fname);
};

void geoGeneration(sGeoPMO& var);


extern vrtMap vMap;

//Вспомогательные фукции
extern bool testExistingFile(const char* fName);

//Работа с INI Файлом
extern char* GetINIstringV(const char* iniFile,const char* section,const char* key);
extern void SaveINIstringV(const char* iniFile,const char* section,const char* key,const char* var);

extern unsigned char* convert_vox2vid(int vox, char* buf);
extern int convert_vid2vox(char* buf);


inline int returnPower2(int val)
{
	const int MAX_POWER2=14;//16384
	int v=1;
	for(int i=1; i<=MAX_POWER2; i++){
		v*=2;
		if(val<=v) return i;
	}
	return MAX_POWER2;
}

template<class GenerationMetod>	void vrtMap::drawBitMetod(int x, int y, int rad, GenerationMetod& genMetod, int filterHeigh)
{
	//включение фильтра по высоте
	int curDH=genMetod.maxHeight;
	if(curDH>0) { FilterMinHeight=0; FilterMaxHeight=filterHeigh; }
	else if(curDH<0) { FilterMinHeight=filterHeigh; FilterMaxHeight=MAX_VX_HEIGHT; }
	else {FilterMinHeight=0; FilterMaxHeight=MAX_VX_HEIGHT; }

	genMetod.setMaxHeight(curDH);
	if(flag_record_operation) UndoDispatcher_PutPreChangedArea(sRect(x-rad, y-rad, 2*rad+1, 2*rad+1),1,0, new sBitGenPMO(x,y,rad,genMetod,filterHeigh) );
	addProcMapOpStatistic(2*rad, 2*rad);

	sTerrainBitmapBase* pTBM=bitGenDispatcher.getTerrainBitmap(rad*2, rad*2, genMetod);

	drawBitGen(x-rad, y-rad, pTBM, false);
	vMap.regRender( vMap.XCYCL(x-rad), vMap.YCYCL(y-rad), vMap.XCYCL(x+rad), vMap.YCYCL(y+rad), vrtMap::TypeCh_Height );

	//выключение фильтра по высоте
	FilterMinHeight=0; FilterMaxHeight=MAX_VX_HEIGHT;
}

//////////////////////////////////////////////////////

struct VMapTile {
	static const int SIDE_SIZE=64; //128
	static const int SIZE=SIDE_SIZE*SIDE_SIZE;
	unsigned short VxABuf[SIZE];
	unsigned char  SurBuf[SIZE];
};
struct VMapTileFull : public VMapTile {
	unsigned char GVBuf[SIZE>>kmGrid]; //Сетка вокселей
	unsigned short GABuf[SIZE>>kmGrid]; //сетка атрибутов
};

struct VMapTileSpec : public VMapTileFull {
	unsigned long  SupBuf[SIZE];
};

struct VMapBuf{
	int hTileAmount;
	int vTileAmount;
	int H_SIZE;
	int GH_SIZE;
	int begX;
	int endX;
	int begY;
	int endY;
	unsigned short* VxABuf;
	unsigned char*  SurBuf;
	unsigned char* GVBuf;
	unsigned short* GABuf;

	enum ChangedTilesAtr {
		ChTA_NoChange=0,
		ChTA_Change=1,
		ChTA_ChangeEvenOnce=2,
		ChTA_SetChange=3
	};
	unsigned char* changedTiles;
	VMapBuf(){
		VxABuf=0; SurBuf=0;
		changedTiles=0;
		hTileAmount=vTileAmount=0;
		begX=begY=endX=endY=H_SIZE=GH_SIZE=0;
		GVBuf=0; GABuf=0;
	};
	void release(){
		if(VxABuf) { delete [] VxABuf; VxABuf=0; }
		if(SurBuf) { delete [] SurBuf; SurBuf=0; }
		if(changedTiles) { delete [] changedTiles; changedTiles=0; }
		if(GVBuf) { delete [] GVBuf; GVBuf=0; }
		if(GABuf) { delete [] GABuf; GABuf=0; }
	}
	void alloc(int _hTileAmount, int _vTileAmount){
		xassert(_hTileAmount); xassert(_vTileAmount);
		release();
		hTileAmount=_hTileAmount; vTileAmount=_vTileAmount;
		H_SIZE=hTileAmount*VMapTile::SIDE_SIZE; 
		GH_SIZE=H_SIZE>>kmGrid; 
		const int vsize=vTileAmount*VMapTile::SIDE_SIZE;
		const int gvsize=vsize>>kmGrid;
		VxABuf=new unsigned short [H_SIZE*vsize];
		SurBuf=new unsigned char [H_SIZE*vsize];
		changedTiles=new unsigned char[H_SIZE*vsize];
		GVBuf=new unsigned char[GH_SIZE*gvsize];
		GABuf=new unsigned short[GH_SIZE*gvsize];
	}
};

class VMap : public VMapBuf, vrtMapCreationParam {
	VMapBuf auxBuf;
    int curPMOCnt;
	static char* worldRasterDataFileSource;
	static char* worldRGBFileSource;
	static char* worldDataFile;
	static char* worldGeoPalFile;
	static char* worldDamPalFile;
	static char* worldLeveledTextureFile;
	static char* worldSurfaceKindFile;

	static char* worldRGBCacheFile;
	static char* worldRasterDataCacheFile;
	static char* worldMiniMapFile;

	vector<sRect> changedAreas;

	///////////////////////////////////////////////////////////////////
	list<sPreChangedArea> preCAs;
	list<sPreChangedArea>::iterator curPreCA;
	///////////////////////////////////////////////////////////////////

	void serialize(Archive& ar);
	void serializeParameters(Archive& ar);

};


#endif //__VMAP_H__
