#include "quantizer.h"
#include "terTools.h"

template<class GenerationMetod>	
void vrtMap::drawBitMetod(int x, int y, int rad, GenerationMetod& genMetod, short _minFH, short _maxFH)
{
	//включение фильтра по высоте
	int curDH=genMetod.maxHeight;
	//if(curDH>0) { FilterMinHeight=_minFH; FilterMaxHeight=_maxFH; }
	//else if(curDH<0) { FilterMinHeight=_minFH; FilterMaxHeight=MAX_VX_HEIGHT; }
	//else {FilterMinHeight=0; FilterMaxHeight=MAX_VX_HEIGHT; }
	FilterMinHeight=_minFH; FilterMaxHeight=_maxFH;

	genMetod.setMaxHeight(curDH);
	if(isRecordingPMO()) 
		UndoDispatcher_PutPreChangedArea(sRect(x-rad, y-rad, 2*rad+1, 2*rad+1),1,0, new sBitGenPMO(x,y,rad,genMetod, _minFH, _maxFH) );
	addProcMapOpStatistic(2*rad, 2*rad);

	sTerrainBitmapBase* pTBM=bitGenDispatcher.getTerrainBitmap(rad*2, rad*2, genMetod);

	drawBitGen(x-rad, y-rad, pTBM, false);
	regRender( XCYCL(x-rad), YCYCL(y-rad), XCYCL(x+rad), YCYCL(y+rad), vrtMap::TypeCh_Height );

	//выключение фильтра по высоте
	FilterMinHeight=0; FilterMaxHeight=MAX_VX_HEIGHT;
}

template<class T>
int vrtMap::convertVMapTryColor2TerClrT()
{
	xassert(0&& "size TerrainColor 3 or > -  not Support!");
	return 0;
}
template<>
int vrtMap::convertVMapTryColor2TerClrT<unsigned char>()
{
	xassert(supBuf);
	ColorQuantizer cq;
	//int result=cq.quantizeOctree(supBuf, SurBuf, Vect2s(H_SIZE, V_SIZE), 256);
	bool resultColorQuantizerPrepare=cq.prepare4PutColor(MAX_DAM_SURFACE_TYPE);
	xassert(resultColorQuantizerPrepare);
	int i,j,cnt=0;
	for(i=0; i< V_SIZE; i++){
		for(j=0; j< H_SIZE; j++){
			cq.putColor(supBuf[cnt]);
			cnt++;
		}
	}
	//if(flag_useTerToolColor)
		terToolsDispatcher.putAllColorTerTools2ColorQuantizer(cq);
	bool resultColorQuantizerPostProcess=cq.postProcess();
	xassert(resultColorQuantizerPostProcess);
	int result=cq.ditherFloydSteinberg(vMap.supBuf, reinterpret_cast<unsigned char*>(vMap.clrBuf), Vect2s(vMap.H_SIZE, vMap.V_SIZE));

	for(int i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		DamPal[i]=cq.pPalette[i];
	}
	//convertPal2TableSurCol(vMap.DamPal, vMap.Damming);
	return result;
}
template<>
int vrtMap::convertVMapTryColor2TerClrT<unsigned short>()
{
	int i,j;
	for(i=0; i < V_SIZE; i++){
		for(j=0; j < H_SIZE; j++){
			int off=j+i*H_SIZE;
			//clrBuf[off] = (((supBuf[off]&0xff0000)+0x040000>>16+3)<<11) | 
			//	(((supBuf[off]&0x00ff00)+0x0200>>8+2)<<5) |
			//	(((supBuf[off]&0xff)+0x04>>0+3)<<0);
			clrBuf[off] = ConvertTry2HighColor(supBuf[off]);
		}
	}
	return 0;
}

template <>
unsigned int vrtMap::getColor32<int>(int x, int y)
{
	int off=offsetBuf(x,y);
	unsigned int col32;
	if(flag_tryColorDamTextures)
        col32=supBuf[off];//Sur2Col32[SurBuf[off]][brightness];
	else 
        col32=getColor32T<TerrainColor>(off);//DamPal[clrBuf[off]].argb;//Sur2Col32[SurBuf[off]][brightness];
#ifndef _FINAL_VERSION_
	if(flag_ShowDbgInfo){
		if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BUILDING) != 0) col32|=0xFF;
		if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BASE_OF_BUILDING_CORRUPT) != 0) col32|=0xFF0000;
		//if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_LEVELED ) != 0) col32|=0x00ff00;
		//if( gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_HARDNESS == GRIDAT_MASK_HARDNESS ) col32|=0xFF00;
	}
	const unsigned int mask=0x00FEFEFF;
	//switch(showSpecialInfo){
	//case SSI_NoShow:
	//	break;
	//case SSI_ShowHardnessImpassability:
	//	if(gABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_INDESTRUCTABILITY)
	//		col32=((0xFFFFFF&mask) + (col32&mask))>>1;
	//	if((gABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_IMPASSABILITY)) 
	//		col32=((0xFF0000&mask) + (col32&mask))>>1;
	//	break;
	//case SSI_ShowKind:
	//	{
	//		unsigned char tk=gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_SURFACE_KIND;
	//		tk*=8;
	//		unsigned int m=0xffff0000;
	//		m>>=tk;
	//		col32=((m&mask) + (col32&mask))>>1;
	//	}
	//	break;
	//}
	if(showSurKind){
		const Color4c* colorTable = TerrainTypeDescriptor::instance().getColors();
		unsigned char tk=gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_SURFACE_KIND;
		unsigned int m=colorTable[tk].RGBA();
		col32=((m&mask) + (col32&mask))>>1;
	}
#endif
	return col32|0xff000000;
}

template <>	
unsigned short vrtMap::getColor16<int>(int x, int y)
{
	int off=offsetBuf(x,y);
	unsigned short col16;
	if(flag_tryColorDamTextures)
        col16=ConvertTry2HighColor(supBuf[off]);
	else 
        col16=getColor16T<TerrainColor>(off);
#ifndef _FINAL_VERSION_
	if(flag_ShowDbgInfo){
		if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BUILDING) != 0) col16|=0x1F;
		if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_BASE_OF_BUILDING_CORRUPT) != 0) col16|=0xF800;
		//if( (gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_LEVELED ) != 0) col16|=0x07E0;
		//if( gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_HARDNESS == GRIDAT_MASK_HARDNESS ) col16|=0x07E0;
	}
	const unsigned int mask=0xF7DF;
	//switch(showSpecialInfo){
	//case SSI_NoShow:
	//	break;
	//case SSI_ShowHardnessImpassability:
	//	if(gABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_INDESTRUCTABILITY)
	//		col16=((0xFFFF&mask) + (col16&mask))>>1;
	//	if((gABuf[offsetBuf2offsetGBuf(off)] & GRIDAT_IMPASSABILITY)) 
	//		col16=((0xFC00&mask) + (col16&mask))>>1;
	//	break;
	//case SSI_ShowKind:
	//	{
	//		unsigned char tk=gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_SURFACE_KIND;
	//		const unsigned short ca[TERRAIN_TYPES_NUMBER]={ 0xF800, 0xFFE0, 0x07FF, 0x001F };
	//		col16=((ca[tk]&mask) + (col16&mask))>>1;
	//	}
	//	break;
	//}
	if(showSurKind){
		const Color4c* colorTable = TerrainTypeDescriptor::instance().getColors();
		unsigned char tk=gABuf[offsetBuf2offsetGBuf(off)]&GRIDAT_MASK_SURFACE_KIND;
		unsigned int m=colorTable[tk].RGBA();
		col16=((ConvertTry2HighColor(m)&mask) + (col16&mask))>>1;
	}
#endif
	return col16;
}
