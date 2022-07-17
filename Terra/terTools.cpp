#include "stdafxTr.h"

#include "Serialization.h"
#include "TypeLibraryImpl.h"
#include "quantizer.h"
#include "terTools.h"

#include "EditArchive.h"
#include "..\Util\ResourceSelector.h"

#include "..\\Render\\inc\\IRenderDevice.h"

#include "scalingEngine.h"


//REGISTER_CLASS(TerToolLibElement, TerToolLibElement, "Сдвоенный тулзер");

REGISTER_CLASS(TerToolBase, TerToolTextureTrack, "След тектурный");
REGISTER_CLASS(TerToolBase, TerToolTexture, "Тектура");
REGISTER_CLASS(TerToolBase, TerToolTrack1, "След 1");
REGISTER_CLASS(TerToolBase, TerToolCrater1, "Кратер 1");
REGISTER_CLASS(TerToolBase, TerToolSimpleCrater, "Простой кратер");
REGISTER_CLASS(TerToolBase, TerToolLeveler, "Выравниватель");
REGISTER_CLASS(TerToolBase, TerToolGeoWave, "гео-волна");
REGISTER_CLASS(TerToolBase, TerToolGeoBreak, "гео-трещина");
REGISTER_CLASS(TerToolBase, TerToolPutConstantModel, "гео-модель");
REGISTER_CLASS(TerToolBase, TerToolTorpedo, "торпеда");

WRAP_LIBRARY(TerToolsLibrary, "TerToolsLibrary", "ТерТулзеры", "Scripts\\Content\\TerToolsLibrary", 0, true);

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TerToolPutConstantModel, eLayMetod, "LayMetod")
REGISTER_ENUM_ENCLOSED(TerToolPutConstantModel, LM_AbsoluteHeight, "Абсолютно")
REGISTER_ENUM_ENCLOSED(TerToolPutConstantModel, LM_RelativeHeightPut, "Относительно")
REGISTER_ENUM_ENCLOSED(TerToolPutConstantModel, LM_RelativeHeightDig, "Относительно инвертируя")
END_ENUM_DESCRIPTOR_ENCLOSED(TerToolPutConstantModel, eLayMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TerToolTexture, eDrawMetod, "DrawMetod")
REGISTER_ENUM_ENCLOSED(TerToolTexture, DM_Cyrcle, "рисовать по кругу")
REGISTER_ENUM_ENCLOSED(TerToolTexture, DM_Bitmap, "рисовать по битмапу")
REGISTER_ENUM_ENCLOSED(TerToolTexture, DM_ReliefBitmapPressIn, "вдавливать по битмапу")
REGISTER_ENUM_ENCLOSED(TerToolTexture, DM_ReliefBitmapSwellOut, "вспучивать по битмапу")
END_ENUM_DESCRIPTOR_ENCLOSED(TerToolTexture, eDrawMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TerToolTextureTrack, eMoveMetod, "MoveMetod")
REGISTER_ENUM_ENCLOSED(TerToolTextureTrack, MM_Uninterrupted, "непрерывно")
REGISTER_ENUM_ENCLOSED(TerToolTextureTrack, MM_ChangePosition, "по изменению позиции")
REGISTER_ENUM_ENCLOSED(TerToolTextureTrack, MM_Stepwise, "по шагу")
END_ENUM_DESCRIPTOR_ENCLOSED(TerToolTextureTrack, eMoveMetod)

BEGIN_ENUM_DESCRIPTOR(eSetingAtrMetod, "SetingIndsMetod")
REGISTER_ENUM(SIM_NotChange, "не менять")
REGISTER_ENUM(SIM_SetInds, "уст. неразрушаемость")
REGISTER_ENUM(SIM_UnSetInds, "сбрасывать неразрушаемость")
REGISTER_ENUM(SIM_SetGeo, "уст. Geo")
REGISTER_ENUM(SIM_SetDam, "уст. Dam")
END_ENUM_DESCRIPTOR(eSetingAtrMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eSettingImpassabilityMetod, "SettingImpassabilityMetod")
REGISTER_ENUM_ENCLOSED(vrtMap, SIMM_NotChangeImpassability, "не менять")
REGISTER_ENUM_ENCLOSED(vrtMap, SIMM_SetImpassability, "устанавливать")
REGISTER_ENUM_ENCLOSED(vrtMap, SIMM_UnSetImpassability, "сбрасывать")
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eSettingImpassabilityMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eSettingSurfaceKindMetod, "SettingSurfaceKindMetod")
REGISTER_ENUM_ENCLOSED(vrtMap, SurfaceKind_NoChange, "не менять")
REGISTER_ENUM_ENCLOSED(vrtMap, SurfaceKind_Set1, "1 рода")
REGISTER_ENUM_ENCLOSED(vrtMap, SurfaceKind_Set2, "2 рода")
REGISTER_ENUM_ENCLOSED(vrtMap, SurfaceKind_Set3, "3 рода")
REGISTER_ENUM_ENCLOSED(vrtMap, SurfaceKind_Set4, "4 рода")
END_ENUM_DESCRIPTOR_ENCLOSED(vrtMap, eSettingSurfaceKindMetod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TerToolBase, eDetaledTextueType, "DetaledTextueType")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_NotDetailedTexture, "нет детализированной текстуры")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture0, "дет.текстура по умолчанию")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture1, "дет.текстура - Песок")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture2, "дет.текстура - Земля")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture3, "дет.текстура - Трава")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture4, "дет.текстура - Трещины")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture5, "дет.текстура - Дорога")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture6, "дет.текстура - Камни")
REGISTER_ENUM_ENCLOSED(TerToolBase, DTT_DetailedTexture7, "дет.текстура - Кратер")
END_ENUM_DESCRIPTOR_ENCLOSED(TerToolBase, eDetaledTextueType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(TerToolLeveler, eLevelerMetod, "eLevelerMetod")
REGISTER_ENUM_ENCLOSED(TerToolLeveler, LevMetod_averageH, "по средней высоте")
REGISTER_ENUM_ENCLOSED(TerToolLeveler, LevMetod_minH, "по минимальной высоте")
END_ENUM_DESCRIPTOR_ENCLOSED(TerToolLeveler, eLevelerMetod)

///////////////////////////////////////


template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
inline void elementarTool<_TT_SETING_ATR_METOD_,_TT_TER_EFF_,_TT_SUR_EFF_,INTERNAL_ALPHA>::PutAltAndSur(int offB, short v, int x, int y, short alfa)
{
	int offTexture=0;

	if(_TT_SUR_EFF_!=TTSE_NOT_CHANGE){ 
		if(_TT_SUR_EFF_==TTSE_ROTATE_DRAW_CURRENT){
			int x1,y1;
			x1 = round((A11*x + A12*y + X)+(textureSizeX>>1));
			y1 = round((A21*x + A22*y + Y)+(textureSizeY>>1));
			// Не зацикленное наложение
			if(x1<0 || y1<0 || x1>=textureSizeX || y1>=textureSizeY) return;
			// зацикленное наложение
			//x1%=textureSizeX;
			//y1%=textureSizeY;
			//if(x1<0)x1+=textureSizeX;
			//if(y1<0)y1+=textureSizeY;
			offTexture=y1*textureSizeX + x1; 
		}
		else {
			x=(x+textureShiftX)%textureSizeX;
			y=(y+textureShiftY)%textureSizeY;
			if(x<0)x+=textureSizeX;
			if(y<0)y+=textureSizeY;
			offTexture=x + y*textureSizeX;
		}
		if(INTERNAL_ALPHA){
			alfa=255-pRastrAlpha[offTexture];
			alfa/=43;
			alfa-=1;
		}
		else {
			alfa+=+(18-XRnd(37));
			alfa/=37;
			alfa-=2;
		}
	}

	switch(_TT_SUR_EFF_){ //Заменит на if !
	case TTSE_NOT_CHANGE:
		//vMap.PutAlt(offB, v); //, vMap.GetGeoType(offB,v)
		switch(_TT_SETING_ATR_METOD_){
		case SIM_SetInds:			vMap.PutAltInds(offB, v);	break;
		case SIM_UnSetInds:			vMap.PutAltUnInds(offB, v);	break;
		case SIM_SetGeo:			vMap.PutAltAndSetGeo(offB, v);	break;
		case SIM_SetDam:			vMap.PutAlt(offB, v);			break;//!!! не может быть установка Dam без текстуры!
		case SIM_NotChange:default:	vMap.PutAlt(offB, v); 		break;
		}
		break;
	case TTSE_DRAW_CURRENT: case TTSE_ROTATE_DRAW_CURRENT:
		{
			if(Vm_IsGeo(vMap.VxABuf[offB])){ //current Geo
				if(_TT_SETING_ATR_METOD_==SIM_SetDam){
					vMap.PutAltAndSetDamNrml(offB, v, pRastrDam[offTexture]);
				}
				else {
					unsigned char sur=pRastrGeo[offTexture];
					if(alfa>3)
						sur=vMap.SurBuf[offB];
					else if(alfa>=0)
						sur=*((unsigned char*)(&terToolsDispatcher.geoAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
					vMap.SurBuf[offB]=sur;
					switch(_TT_SETING_ATR_METOD_){
					case SIM_SetInds:			vMap.PutAltInds(offB, v);	break;
					case SIM_UnSetInds:			vMap.PutAltUnInds(offB, v);	break;
					case SIM_SetGeo:			vMap.PutAlt(offB, v);		break;
					case SIM_NotChange:default:	vMap.PutAlt(offB, v); 		break;
					}
				}
			}
			else { // current Dam
				if(_TT_SETING_ATR_METOD_==SIM_SetGeo){
					vMap.PutAltAndSetGeo(offB, v, vMap.GetGeoType(offB,v));
				}
				else {
					unsigned char sur=pRastrDam[offTexture];
					if(alfa>3)
						sur=vMap.SurBuf[offB];
					else if(alfa>=0)
						sur=*((unsigned char*)(&terToolsDispatcher.damAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
					vMap.SurBuf[offB]=sur;
					switch(_TT_SETING_ATR_METOD_){
					case SIM_SetInds:			vMap.PutAltInds(offB, v);	break;
					case SIM_UnSetInds:			vMap.PutAltUnInds(offB, v);	break;
					case SIM_SetDam:			vMap.PutAlt(offB, v);		break;
					case SIM_NotChange:default:	vMap.PutAlt(offB, v); 		break;
					}
				}
			}
		}
		break;
	}

/*
	//Для простоты _TT_SETING_ATR_METOD_ используется ТОЛЬКО на DAM и TTSE_DRAW_CURRENT и TTSE_ROTATE_DRAW_CURRENT
	if(Vm_IsGeo(vMap.VxABuf[offB])){
		switch(_TT_SUR_EFF_){
		case TTSE_NOT_CHANGE:
			vMap.PutAlt(offB, v); //, vMap.GetGeoType(offB,v)
			break;
		case TTSE_DRAW_GEO:
			{
				unsigned char sur=pRastrGeo[offTexture];
				if(alfa>3)
					sur=vMap.SurBuf[offB];
				else if(alfa>=0)
					sur=*((unsigned char*)(&terToolsDispatcher.geoAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
				vMap.PutAltAndSetGeo(offB, v, sur);
			}
			break;
		case TTSE_DRAW_CURRENT: case TTSE_ROTATE_DRAW_CURRENT:
			{
				unsigned char sur=pRastrGeo[offTexture];
				if(alfa>3)
					sur=vMap.SurBuf[offB];
				else if(alfa>=0)
					sur=*((unsigned char*)(&terToolsDispatcher.geoAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
				vMap.PutAltAndNoChange(offB, v, sur);
			}
			break;
		case TTSE_DRAW_DAM:
			{
				unsigned char sur=pRastrDam[offTexture];
				vMap.PutAltAndSetDamNrml(offB, v, sur);
			}
			break;
		}
	}
	else {
		switch(_TT_SUR_EFF_){
		case TTSE_NOT_CHANGE:
			
			switch(_TT_SETING_ATR_METOD_){
			case SIM_SetInds:				vMap.PutAltInds(offB,v);	break;
			case SIM_UnSetInds:				vMap.PutAltUnInds(offB,v);	break;
			case SIM_NotChange:default:	vMap.PutAlt(offB,v);		break;
			}
			break;
		case TTSE_DRAW_GEO:
			vMap.PutAltAndSetGeo(offB, v, vMap.GetGeoType(offB,v));
			break;
		case TTSE_DRAW_DAM:
			{
				unsigned char sur=pRastrDam[offTexture];
				if(alfa>3)
					sur=vMap.SurBuf[offB];
				else if(alfa>=0)
					sur=*((unsigned char*)(&terToolsDispatcher.damAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
				vMap.PutAltAndSetDamNrml(offB, v, sur);
			}
			break;
		case TTSE_DRAW_CURRENT: case TTSE_ROTATE_DRAW_CURRENT:
			{
				unsigned char sur=pRastrDam[offTexture];
				if(alfa>3)
					sur=vMap.SurBuf[offB];
				else if(alfa>=0)
					sur=*((unsigned char*)(&terToolsDispatcher.damAlfaConversion[vMap.SurBuf[offB]][sur]) + alfa);
				
				switch(_TT_SETING_ATR_METOD_){
				case SIM_SetInds:				vMap.PutAltAndSetDamInds(offB, v, sur);	break;
				case SIM_UnSetInds:				vMap.PutAltAndSetDamNrml(offB, v, sur);	break;
				case SIM_NotChange:default:	vMap.PutAltAndNoChange(offB, v, sur);	break;
				}
			}
			break;
		}					
	}*/

}

template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
inline int elementarTool<_TT_SETING_ATR_METOD_,_TT_TER_EFF_,_TT_SUR_EFF_,INTERNAL_ALPHA>::voxSet(short x, short y, short v, short alfa)
{
	int offB=vMap.offsetBuf(x,y);
	switch (_TT_TER_EFF_){
	case TTTE_SMOOTH:
		{
			if(v>MAX_VX_HEIGHT) v=MAX_VX_HEIGHT;
			if(v<0) v=0;
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;
	case TTTE_DIG:
		{
			if(v<0) v=0;
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;
	case TTTE_PUT:
		{
			if(v>MAX_VX_HEIGHT) v=MAX_VX_HEIGHT;
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;
	}
	return 0;
}


template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
inline int elementarTool<_TT_SETING_ATR_METOD_,_TT_TER_EFF_,_TT_SUR_EFF_,INTERNAL_ALPHA>::tVoxSet(short x, short y, short dV, short alfa)
{
	int offB=vMap.offsetBuf(x,y);
	if(_TT_TER_EFF_==TTTE_ALIGNMENT_DIG || _TT_TER_EFF_==TTTE_DIG){
		if(Vm_IsIndestructability(vMap.VxABuf[offB])) return 0;
	}
	int v=vMap.GetAlt(offB);

	switch (_TT_TER_EFF_){
	case TTTE_PLOTTING_TEXTURE:
		{
			//if(dV)
			//	vMap.SetSoot(offB);
		}
		break;
	case TTTE_DIG:
		{
			v+=dV;
			if(v<0) v=0;
			//vMap.PutAltAndGeoRecalc(offB, v);
			//TERTOOLS_PUTALT(offB, v, sur);
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;
	case TTTE_ALIGNMENT_DIG: 
		{
			if(v > hAppr){ //Воздействие если инструмент убирает и высота больше hAppr
				v+=dV;
				if(v<=hAppr){
					v=hAppr;
					vMap.PutAltAndSetLeveling(offB, v);
				}
				else
					//vMap.PutAltAndGeoRecalc(offB, v);
					//TERTOOLS_PUTALT(offB, v, sur);
					PutAltAndSur(offB, v, x, y, alfa);
			}
		}
		break;
	case TTTE_PUT:
		{
			v+=dV;
			if(v>MAX_VX_HEIGHT) v=MAX_VX_HEIGHT;
			//vMap.PutAltAndGeoRecalc(offB, v);
			//TERTOOLS_PUTALT(offB, v, sur);
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;
	case TTTE_ALIGNMENT_PUT:
		{
			if(v < hAppr){//Воздействие если инструмент добавляет и высота меньше hAppr
				v+=dV;
				if(v>=hAppr) {
					v=hAppr;
					vMap.PutAltAndSetLeveling(offB, v);
				}
				else 
					//vMap.PutAltAndGeoRecalc(offB, v);
					//TERTOOLS_PUTALT(offB, v, sur);
					PutAltAndSur(offB, v, x, y, alfa);
			}
		}
		break;
	case TTTE_ALIGNMENT_SMOOTH:
		{
			if( dV >0 ){
				if(v < hAppr){//Воздействие если инструмент добавляет и высота меньше hAppr
					v+=dV;
					if(v>=hAppr) {
						v=hAppr;
						vMap.PutAltAndSetLeveling(offB, v);
					}
					else 
						//vMap.PutAltAndGeoRecalc(offB, v);
						//TERTOOLS_PUTALT(offB, v, sur);
						PutAltAndSur(offB, v, x, y, alfa);
				}
			}
			else {
				if(v > hAppr){//Воздействие если инструмент убирает и высота больше hAppr
					v+=dV;
					if(v<=hAppr){
						v=hAppr;
						vMap.PutAltAndSetLeveling(offB, v);
					}
					else
						//vMap.PutAltAndGeoRecalc(offB, v);
						//TERTOOLS_PUTALT(offB, v, sur);
						PutAltAndSur(offB, v, x, y, alfa);
				}
			}
		}
		break;
	case TTTE_SMOOTH:
		{
			v+=dV;
			if(v>MAX_VX_HEIGHT) v=MAX_VX_HEIGHT;
			if(v<0) v=0;
			//vMap.PutAltAndGeoRecalc(offB, v);
			//TERTOOLS_PUTALT(offB, v, sur);
			PutAltAndSur(offB, v, x, y, alfa);
		}
		break;

	}
	return 0;
}



template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
int elementarTool<_TT_SETING_ATR_METOD_,_TT_TER_EFF_,_TT_SUR_EFF_,INTERNAL_ALPHA>::influenceBM(int x, int y, int sx, int sy, unsigned char * imageArea)
{

	int begx=vMap.XCYCL(x- (sx>>1));
	int begy=vMap.YCYCL(y- (sy>>1));
	unsigned char * ia=imageArea;
	int i,j;
	int offY=vMap.offsetBuf(0,begy);
	for(i=0; i<sy; i++){
		int offY=vMap.offsetBuf(0,vMap.YCYCL(begy+i));
		for(j=0; j<sx; j++){
			int dV=*(ia);
			int offB=offY+vMap.XCYCL(j+begx);
			switch(_TT_TER_EFF_){
			case TTTE_PLOTTING_TEXTURE: case TTTE_ALIGNMENT_DIG: case TTTE_ALIGNMENT_PUT:
			case TTTE_ALIGNMENT_SMOOTH: case TTTE_DIG: case TTTE_PUT: case TTTE_SMOOTH:
				{
					//tVoxSet(offB, dV);
				}
				break;
			}
			ia++;
		}
	}
	//Эта операция должна быть совмещена с рендером
	vMap.recalcArea2Grid(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + sx+1), vMap.YCYCL(begy + sy+1) );
	vMap.regRender(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + sx+1), vMap.YCYCL(begy + sy+1) );

	return 0;
}

template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA>
int elementarTool<_TT_SETING_ATR_METOD_,_TT_TER_EFF_,_TT_SUR_EFF_,INTERNAL_ALPHA>::influenceDZ(int x, int y, int rad, short dh, int smMode, unsigned char picMode, bool flag_Render)
{
	if(rad > MAX_RADIUS_CIRCLEARR){
		xassert(0&&"exceeding max radius in influenceDZ");
		rad=MAX_RADIUS_CIRCLEARR;
	}

	int begx=vMap.XCYCL(x - rad);
	int begy=vMap.YCYCL(y - rad);
	//void vrtMap::deltaZone(int x,int y,int rad,int smth,int dh,int smode,int eql)
	int eql=0;

	register int i,j;
	int max;
	int* xx,*yy;

	int locp=logicRnd();
	//log_var(locp);

	int r = rad - rad*smMode/10;
	float d = 1.0f/(rad /*- r*/ + 1),dd,ds,s;
	int v, h, k, mean;

	if(dh){
		for(i = 0, dd = 1.0f; i <= r; i++, dd -= d){
			max = maxRad[i];
			xx = xRad[i];
			yy = yRad[i];
			h = round(dd*dh);
			for(j = 0;j < max;j++) {
				///tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), dh);
				tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), /*dh*/h, 0);
			}
		}

		for(i = r + 1/*,dd = 1.0f - d*/; i <= rad; i++,dd -= d){
			max = maxRad[i];
			xx = xRad[i];
			yy = yRad[i];
			h = round(dd*dh);//(int)
			if(!h) 
				h = dh > 0 ? 1 : -1;
			switch(picMode){
				case 0:
					v = round(dd*max);//(int)
					ds = (float)v/(float)max;
					for(s=ds, k=0, j=locp%max; k<max; j=(j+1==max) ?0:j+1, k++, s+=ds)
						if(s >= 1.0){
							//tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), dh, 128);
							tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), h, 128);
							s -= 1.0;
						}
					break;
				case 1:
					v = round(dd*1000000.0);//(int)
					for(j = 0; j < max; j++)
						if((int)XRnd(1000000) < v) {
							//tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), dh, 128);
							tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), h, 128);
						}
					break;
				case 2:
					v = round(dd*max);//(int)
					for(k=0, j=locp%max; k<v; j=(j+1==max)? 0:j+1, k++){
						//tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), dh, 128);
						tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), h, 128);
					}
					locp += max;
					break;
				}
		}
		locp++;
	}
	else {
		const int DH_MEAN = 1; //дельта по которой усредняются высоты
		int cx,h,cy,cx_;
		if(eql){
			mean = k = 0;
			for(i = 0;i <= r;i++){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				for(j = 0;j < max;j++){
					cx = vMap.XCYCL(x + xx[j]);
					mean += vMap.GetAlt(cx,vMap.YCYCL(y+yy[j]));
				}
				k += max;
			}
			mean /= k;
			for(i = 0;i <= r;i++){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				for(j = 0;j < max;j++){
					cy = vMap.YCYCL(y+yy[j]);
					cx = vMap.XCYCL(x + xx[j]);
					h = vMap.GetAlt(cx,cy);
					if(abs(h - mean) < eql)
						if(h > mean){
							///tVoxSetAll( vMap.offsetBuf(cx, cy), -DH_MEAN);
							tVoxSet(cx, cy, -DH_MEAN, 0);
						}
						else if(h < mean){ //voxSet(cx,cy,1);
							///tVoxSetAll( vMap.offsetBuf(cx, cy), DH_MEAN);
							tVoxSet(cx, cy, DH_MEAN, 0);
						}
				}
			}
			for(i = r + 1,dd = 1.0f - d;i <= rad;i++,dd -= d){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				h = round(dd*dh);//(int)
				if(!h) h = dh > 0 ? 1 : -1;
				v = round(dd*max);//(int)
				ds = (float)v/(float)max;
				for(s = ds,k = 0,j = locp%max;k < max;j = (j + 1 == max) ? 0 : j + 1,k++,s += ds)
					if(s >= 1.0){
						cy = vMap.YCYCL(y+yy[j]);
						cx = vMap.XCYCL(x + xx[j]);
						h = vMap.GetAlt(cx,cy);
						if(abs(h - mean) < eql)
							if(h > mean){ //voxSet(cx,cy,-1);
								///tVoxSetAll( vMap.offsetBuf(cx, cy), -DH_MEAN);
								tVoxSet(cx, cy, -DH_MEAN, 128);
							}
							else if(h < mean){ //voxSet(cx,cy,1);
								///tVoxSetAll( vMap.offsetBuf(cx, cy), DH_MEAN);
								tVoxSet(cx, cy, DH_MEAN, 128);
							}
						s -= 1.0;
						}
			}
		}
		else {
			int dx,dy;
			for(i = 0;i <= r;i++){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				for(j = 0;j < max;j++){
					cy = vMap.YCYCL(y+yy[j]);
					cx = vMap.XCYCL(x + xx[j]);
					h = vMap.GetAlt(cx,cy);
					v = 0;
					switch(picMode){
						case 0:
							for(dy = -1;dy <= 1;dy++)
								for(dx = -1;dx <= 1;dx++){
									cx_ = vMap.XCYCL(cx+dx);
									v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
								}
							v -= h;
							v >>= 3;
							break;
						case 1:
						case 2:
							for(dy = -1;dy <= 1;dy++){
								for(dx = -1;dx <= 1;dx++){
									cx_ = vMap.XCYCL(cx+dx);
									if(abs(dx) + abs(dy) == 2)
										v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
								}
							}
							v >>= 2;
							break;
					}
					///tVoxSetAll( vMap.offsetBuf(cx, cy), v-h);
					tVoxSet(cx, cy, v-h, i*255/rad);
				}
			}
			for(i = r + 1,dd = 1.0f - d;i <= rad;i++,dd -= d){
				max = maxRad[i];
				xx = xRad[i];
				yy = yRad[i];
				h = round(dd*dh);//(int)
				if(!h) h = dh > 0 ? 1 : -1;

				v = round(dd*max);//(int)
				ds = (float)v/(float)max;
				for(s = ds,k = 0,j = locp%max;k < max;j = (j + 1 == max) ? 0 : j + 1,k++,s += ds){
					if(s >= 1.0){
						cy = vMap.YCYCL(y+yy[j]);
						cx = vMap.XCYCL(x + xx[j]);
						h = vMap.GetAlt(cx,cy);
						v = 0;
						switch(picMode){
							case 0:
								for(dy = -1;dy <= 1;dy++){
									for(dx = -1;dx <= 1;dx++){
										cx_ = vMap.XCYCL(cx+dx);
										v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
									}
								}
								v -= h;
								v >>= 3;
								break;
							case 1:
							case 2:
								for(dy = -1;dy <= 1;dy++){
									for(dx = -1;dx <= 1;dx++){
										cx_ = vMap.XCYCL(cx+dx);
										if(abs(dx) + abs(dy) == 2)
											v += vMap.GetAlt(cx_,vMap.YCYCL(cy +dy));
									}
								}
								v >>= 2;
								break;
						}
						///tVoxSetAll( vMap.offsetBuf(cx, cy), v-h);
						tVoxSet(cx, cy, v-h, i*255/rad);
						s -= 1.0;
					}
				}
			}
		}
	}

	if(flag_Render){
		vMap.recalcArea2Grid(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1) );
		char typeChanges=vrtMap::TypeCh_Height;
		if(_TT_SUR_EFF_ != TTSE_NOT_CHANGE)
			typeChanges|=vrtMap::TypeCh_Texture;
		vMap.regRender(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1), typeChanges);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////

void TerToolCrater1::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.tga", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of texture file");
	ar.serialize(ResourceSelector(terTexture, options), "terTexture", "тертекстура");
	ar.serialize(terTextureKScale, "terTextureKScale", "масштаб тертекстуры");
	ar.serialize(r, "r", "радиус");
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
	if(r<=0) r=1;
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolCrater1::quant()
{
	start_timer_auto();
	float kScale=r/32.f;
	Vect3f& center=position.trans();

	elementarTool<setingIndsMetod, TTTE_PUT, TTSE_NOT_CHANGE> tool;
	elementarTool<setingIndsMetod, TTTE_DIG, TTSE_NOT_CHANGE> tooldig;
	//tool.influenceDZ(x,y,r, 0, 0);
	switch(quantCnt++){
	case 0:
		tool.influenceDZ(center.x, center.y, round(kScale*32), 2*(1<<VX_FRACTION), 3 );
		break;
	case 1:
		tooldig.influenceDZ(center.x, center.y, round(kScale*24), -4*(1<<VX_FRACTION), 3 );
		break;
	case 2:
		tooldig.influenceDZ(center.x, center.y, round(kScale*16), -4*(1<<VX_FRACTION), 4 );
		break;
	case 3:
		{
			TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
			if(pbmp){
				if(pbmp->pAlfaLayer==0){
					elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_DRAW_CURRENT> toolSmooth;
					toolSmooth.setTexture(pbmp,center.x,center.y);
					toolSmooth.influenceDZ(center.x, center.y, round(kScale*32), 0, 2 );
				}
				else {
					elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_DRAW_CURRENT, true> toolSmooth;
					toolSmooth.setTexture(pbmp,center.x,center.y);
					toolSmooth.influenceDZ(center.x, center.y, round(kScale*32), 0, 2 );
				}
			}
			drawRegionAndImpassability(center.x, center.y, round(kScale*32));

		}
		break;
	default:
		return false;
	}
	return true;
}
//////////////////////////////////////////////////////

void TerToolSimpleCrater::serialize(Archive& ar)
{
	ar.serialize(r, "r", "радиус");
	if(r<=0) r=1;
	ar.serialize(dh, "dh", "глубина");
	ar.serialize(flag_waving, "flag_waving", "волнистось");
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolSimpleCrater::quant()
{
	start_timer_auto();

	Vect3f& center=position.trans();
	elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_NOT_CHANGE> tooldig;

	if(flag_waving){
		tooldig.influenceDZ(center.x, center.y, r, dh*2, 10, 2, false); 
		tooldig.influenceDZ(center.x, center.y, r, 0,     4, 0, true );
	}
	else {
		tooldig.influenceDZ(center.x, center.y, r, dh*2, 8, 0, false); 
		tooldig.influenceDZ(center.x, center.y, r, 0,    4, 0, true);
	}

	//int rOrg=r*2;
	//int rad=r*2;

	//int x=center.x; int y=center.y;
	//int deltaH=dh*2;
	//int smMode=10;
	//int picMode=0;

	//int locp=logicRnd();

	//if(rad > MAX_RADIUS_CIRCLEARR){
	//	xassert(0&&"exceeding max radius in influenceDZ");
	//	rad=MAX_RADIUS_CIRCLEARR;
	//}

	//int begx=vMap.XCYCL(x - rad);
	//int begy=vMap.YCYCL(y - rad);
	////void vrtMap::deltaZone(int x,int y,int rad,int smth,int deltaH,int smode,int eql)
	//int eql=0;

	//register int i,j;
	//int max;
	//int* xx,*yy;

	//int r = rad - rad*smMode/10;
	//float d = 1.0f/(rad - r + 1),dd,ds,s;
	//int v, h, k;

	//if(deltaH){
	//	for(i = 0; i <= r; i++){
	//		max = maxRad[i];
	//		xx = xRad[i];
	//		yy = yRad[i];
	//		for(j = 0;j < max;j++) {
	//			///tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), deltaH);
	//			tooldig.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), deltaH, 0);
	//		}
	//	}

	//	for(i = r + 1,dd = 1.0f - d; i <= rad; i++,dd -= d){
	//		max = maxRad[i];
	//		xx = xRad[i];
	//		yy = yRad[i];
	//		h = round(dd*deltaH);
	//		if(!h) 
	//			h = deltaH > 0 ? 1 : -1;
	//		switch(picMode){
	//			case 0:
	//				v = round(dd*max);//(int)
	//				ds = (float)v/(float)max;
	//				for(s = ds,k = 0,j = locp % max; k < max; j = (j + 1 == max) ? 0 : j + 1,k++,s += ds)
	//					if(s >= 1.0){
	//						///tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), deltaH);
	//						tooldig.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), h, 128);
	//						s -= 1.0;
	//						}
	//				break;
	//			case 1:
	//				v = round(dd*1000000.0);//(int)
	//				for(j = 0;j < max;j++)
	//					if((int)XRnd(1000000) < v) {
	//						///tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), deltaH);
	//						tooldig.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), deltaH, 128);
	//					}
	//				break;
	//			case 2:
	//				v = round(dd*max);//(int)
	//				for(k = 0,j = locp%max;k < v;j = (j + 1 == max) ? 0 : j + 1,k++){
	//					///tVoxSet( vMap.offsetBuf(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j])), deltaH);
	//					tooldig.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), h, 128);
	//				}
	//				locp += max;
	//				break;
	//			}
	//	}
	//	locp++;
	//}

	//vMap.recalcArea2Grid(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1) );
	//char typeChanges=vrtMap::TypeCh_Height;
	//vMap.regRender(vMap.XCYCL(begx-1), vMap.YCYCL(begy-1), vMap.XCYCL(begx + 2*rad+1), vMap.YCYCL(begy + 2*rad+1), typeChanges);

	//tooldig.influenceDZ(center.x, center.y, rOrg, 0, 5 );

	return false;
}

//////////////////////////////////////////////////////
TerToolLeveler::TerToolLeveler()
{
	r=10;
	kRoughness=0.4f;
	amountFrame=20;
	levelerMetod=LevMetod_averageH;
}

void TerToolLeveler::serialize(Archive& ar)
{
	ar.serialize(r, "r", "радиус");
	if(r<=0) r=1;
	ar.serialize(kRoughness, "kRoughness", "неровность");
	kRoughness=clamp(kRoughness, 0, 1.f);
	ar.serialize(amountFrame, "amountFrame", "количество кадров");
	ar.serialize(levelerMetod, "levelerMetod", "выравнивание");
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolLeveler::quant()
{
	start_timer_auto();
	//r=100;
	xassert(r < vMap.H_SIZE && r < vMap.V_SIZE);
	Vect3f& center=position.trans();
	int begx=round(center.x)-r;
	int begy=round(center.y)-r;
	begx=clamp(begx, 0, vMap.H_SIZE-2*r);
	begy=clamp(begy, 0, vMap.V_SIZE-2*r);
	if(quantCnt<=0){ //подразумевается ==
		//Инициализация
		int minh=MAX_VX_HEIGHT;
		int maxh=0;
		int sum=0;
		int i,j;
		for(i=r/2; i<(r+r/2); i++){
			int offY=vMap.offsetBuf(0,begy+i);
			xassert(begy+i<vMap.V_SIZE);
			for(j=r/2; j<(r+r/2); j++){
				xassert(begx+j < vMap.H_SIZE);
				short h=vMap.GetAlt( offY+ begx+j );
				minh=min(minh, h);
				maxh=max(maxh, h);
				sum+=h;
			}
		}
		if(levelerMetod==LevMetod_averageH){
			hApproximation=sum/(r*r);
			upBitmap.create(2*r, 2*r);
			downBitmap.create(2*r, 2*r);
			if(kRoughness<0.19f){
				sBitGenMetodExp(3, (hApproximation-minh)*2, 0).generate(upBitmap);
				sBitGenMetodExp(3, (maxh-hApproximation)*2, 0).generate(downBitmap);
			}
			else {
				sBitGenMetodMPD(3, (hApproximation-minh)*2, 0, kRoughness).generate(upBitmap);
				sBitGenMetodMPD(3, (maxh-hApproximation)*2, 0, kRoughness).generate(downBitmap);
			}
		}
		else if(levelerMetod==LevMetod_minH){
			hApproximation=minh;
			downBitmap.create(2*r, 2*r);
			if(kRoughness<0.19f)
				sBitGenMetodExp(3, (maxh-hApproximation)*2, 0).generate(downBitmap);
			else 
				sBitGenMetodMPD(3, (maxh-hApproximation)*2, 0, kRoughness).generate(downBitmap);
		}
		else 
			return false; //not support type
		quantCnt=1;
		return true;
	}
	// основной цикл

	int i,j, cnt=0;
	int curAmountFrame= 30;
	xassert(quantCnt);
	int k1=(1<<16)*quantCnt/curAmountFrame;
	int k2=(1<<16)*(quantCnt-1)/curAmountFrame;

	short* rastrUp=upBitmap.pRaster;
	short* rastrDown=downBitmap.pRaster;
	if(rastrUp && rastrDown){
		for(i=0; i<2*r; i++){
			int offY=vMap.offsetBufC(0,begy+i);
			for(j=0; j<2*r; j++){
				int off=offY+vMap.XCYCL(begx+j);
				int h=vMap.GetAlt(off);
				int dV;
				if(h<hApproximation){
					dV=rastrUp[cnt];
					dV = ((dV*k1)>>16) - ((dV*k2)>>16);
					h+=dV;
					if(h>hApproximation) h=hApproximation;
					vMap.PutAlt(off, h);
				}
				else if(h>hApproximation){
					dV=rastrDown[cnt];
					dV = ((dV*k1)>>16) - ((dV*k2)>>16);
					h-=dV;
					if(h < hApproximation) h=hApproximation;
					vMap.PutAlt(off, h);
				}
				cnt++;
			}
		}
	}
	else if(rastrDown){
		for(i=0; i<2*r; i++){
			int offY=vMap.offsetBufC(0,begy+i);
			for(j=0; j<2*r; j++){
				int off=offY+vMap.XCYCL(begx+j);
				int h=vMap.GetAlt(off);
				int dV;
				if(h>hApproximation){
					dV=rastrDown[cnt];
					dV = ((dV*k1)>>16) - ((dV*k2)>>16);
					h-=dV;
					if(h < hApproximation) h=hApproximation;
					vMap.PutAlt(off, h);
				}
				cnt++;
			}
		}
	}

	quantCnt++;

	bool result=true;
	char typeChanges=vrtMap::TypeCh_Height;
	if(quantCnt>curAmountFrame){
		if(drawRegionAndImpassability(begx + 2*r, begy +2*r, r))
			typeChanges|=vrtMap::TypeCh_Region;
		result=false;
	}
	vMap.recalcArea2Grid(begx, begy, begx+2*r, begy+2*r );
	vMap.regRender(begx, begy, begx+2*r, begy+2*r, typeChanges);
	return result;
}

////////////////////////////////////////////////////////////////////////
void TerToolGeoWave::serialize(Archive& ar)
{
	sGeoWave::serialize(ar);
	//ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
}
void TerToolGeoBreak::serialize(Archive& ar)
{
	GeoBreak::serialize(ar);
	//ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
}

///////////////////////////////////////////////////////////////////////

TerToolPutConstantModel::TerToolPutConstantModel()
{
	//Редактируемые переменные
	amountFrame=1;
	amountFrame2=0;
	layMetod=LM_AbsoluteHeight;
	scaleFactor=1.f;
	independentTerTexture=false;
	noiseAmp=0;

	//Внутренние переменные
	quantCnt=0;
	chainNum=0;
	//position=Se3f(MatXf(Mat3f::ZERO, Vect3f::ZERO));
	minimalZ=0;
	relativeCenterZ=0;
	minCoord=Vect2f::ZERO;
	flag_waitStartAnimationChain=true;
}

void TerToolPutConstantModel::setPosition(const Se3f& _pos, bool flag_begPos)
{
	position=_pos;
	//position=Vect2i(_pos.trans().x, _pos.trans().y);
}
void TerToolPutConstantModel::setScale(float _scaleFactor)
{
	scaleFactor=scaleFactor*_scaleFactor;
}

void TerToolPutConstantModel::serialize(Archive& ar)
{
	static ModelSelector::Options options("*.3dx", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of 3DX model");
	ar.serialize(ResourceSelector(modelName, options), "terModel", "термодель");
	ar.serialize(scaleFactor, "scaleFactor", "Масштаб модели");
	static ResourceSelector::Options options2("*.tga", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of texture file");
	ar.serialize(ResourceSelector(terTexture, options2), "terTexture", "тертекстура");
	ar.serialize(terTextureKScale, "terTextureKScale", "масштаб тертекстуры");
	ar.serialize(independentTerTexture, "independentTerTexture", "Независимая тертекстура");
	ar.serialize(layMetod, "layMetod", "метод");
	//if( !ar.isEdit() || layMetod!=LM_AbsoluteHeight ){
		ar.serialize(amountFrame, "amountFrame", "количество кадров");
		ar.serialize(amountFrame2, "amountFrame2", "количество кадров2");
	//}
	if(amountFrame < 1) amountFrame=1;
	ar.serialize(noiseAmp, "noiseAmp", "Шум");
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
	if(!ar.isEdit()) {
		ar.serialize(quantCnt, "quantCnt", 0);
		ar.serialize(chainNum, "chainNum", 0);
		ar.serialize(position, "position", 0);
		ar.serialize(minimalZ, "minimalZ", 0);
		ar.serialize(relativeCenterZ, "relativeCenterZ", 0);
		ar.serialize(minCoord, "minCoord", 0);
		ar.serialize(flag_waitStartAnimationChain, "flag_waitStartAnimationChain", 0);
	}
}

template<eSetingAtrMetod _TT_SETING_ATR_METOD_, eTerToolTerrainEffect _TT_TER_EFF_, eTerToolSurfaceEffect _TT_SUR_EFF_, bool INTERNAL_ALPHA> 
bool TerToolPutConstantModel::elemetarQuant(elementarTool<_TT_SETING_ATR_METOD_, _TT_TER_EFF_, _TT_SUR_EFF_, INTERNAL_ALPHA>& curTool)
{
	char typeChanges=vrtMap::TypeCh_Height; //for regRender
	if(_TT_SUR_EFF_ != TTSE_NOT_CHANGE)
		typeChanges|=vrtMap::TypeCh_Texture;

	if(layMetod==LM_AbsoluteHeight){
		short* rstr=voxelBitmap.pRaster;
		int begX=ceilf(minCoord.x);//position.trans().x-voxelBitmap.sx/2;
		int begY=ceilf(minCoord.y);//position.trans().y-voxelBitmap.sy/2;
		int i,j, cnt=0;
		for(i=0; i<voxelBitmap.sy; i++){
			int begOffsetY=vMap.offsetBufC(0, begY+i);
			for(j=0; j<voxelBitmap.sx; j++){
				int dV=rstr[cnt];
				if(dV>=0){
					dV+=XRnd(noiseAmp)-(noiseAmp>>1);
					curTool.voxSet(vMap.XCYCL(begX+j), vMap.XCYCL(begY+i),dV+minimalZ);// PutAltAndSur(begOffsetY+vMap.XCYCL(begX+j), dV+minimalZ, j, i);
				}
				cnt++;
			}
		}
		vMap.recalcArea2Grid(vMap.XCYCL(begX), vMap.YCYCL(begY), vMap.XCYCL(begX + voxelBitmap.sx), vMap.YCYCL(begY +voxelBitmap.sy) );
		vMap.regRender(vMap.XCYCL(begX), vMap.YCYCL(begY), vMap.XCYCL(begX + voxelBitmap.sx), vMap.YCYCL(begY + voxelBitmap.sy), typeChanges );
		return false;
	}
	else {
		if(chainNum>0 && flag_waitStartAnimationChain)
			return true;
		short* rstr=voxelBitmap.pRaster;
		int begX=ceilf(minCoord.x);//position.trans().x-voxelBitmap.sx/2;
		int begY=ceilf(minCoord.y);//position.trans().y-voxelBitmap.sy/2;
		int i,j, cnt=0;
		int curAmountFrame= (chainNum==0) ? amountFrame : amountFrame2;
		xassert(curAmountFrame);
		int k1=(1<<16)*quantCnt/curAmountFrame;
		int k2=(1<<16)*(quantCnt-1)/curAmountFrame;

		unsigned int storernd=XRndGet();
		XRndSet(81818181);
		for(i=0; i<voxelBitmap.sy; i++){
			//int begOffsetY=vMap.offsetBufC(0, begY+i);
			for(j=0; j<voxelBitmap.sx; j++){
				int dV=rstr[cnt];
				if(dV>=0) {
					dV-=relativeCenterZ;
					dV+=XRnd(noiseAmp)-(noiseAmp>>1);
					if(layMetod==LM_RelativeHeightDig)
						dV=-dV;
					dV = ((dV*k1)>>16) - ((dV*k2)>>16);
					curTool.tVoxSet(vMap.XCYCL(begX+j), vMap.YCYCL(begY+i), dV);
				}
				//else{ //Отрисовка зоны где нет изменений
				//	vMap.DamPal[255].argb=0x00ff0000;
				//	vMap.SetTer(vMap.XCYCL(begX+j), vMap.YCYCL(begY+i), 255);
				//}
				cnt++;
			}
		}
		XRndSet(storernd);
		bool result;
		quantCnt++;
		if(quantCnt>curAmountFrame){
			if(amountFrame2==0 || chainNum>=1){
				result=false;
				drawRegionAndImpassability(vMap.XCYCL(begX + voxelBitmap.sx/2), vMap.YCYCL(begY +voxelBitmap.sy/2), min(voxelBitmap.sx/2,voxelBitmap.sy/2));
			}
			else {
				result=true;
				chainNum=1;
				quantCnt=1;
				layMetod=(layMetod==LM_RelativeHeightDig)?LM_RelativeHeightPut:LM_RelativeHeightDig;
			}
		}
		else result=true;
		vMap.recalcArea2Grid(vMap.XCYCL(begX), vMap.YCYCL(begY), vMap.XCYCL(begX + voxelBitmap.sx), vMap.YCYCL(begY +voxelBitmap.sy) );
		vMap.regRender(vMap.XCYCL(begX), vMap.YCYCL(begY), vMap.XCYCL(begX + voxelBitmap.sx), vMap.YCYCL(begY + voxelBitmap.sy), typeChanges );
		return result;
	}
}

void TerToolPutConstantModel::startNextAnimationChain()
{
	flag_waitStartAnimationChain=false;
}

float convertPosition2AngleAboutZ(Se3f& position)
{
	float angle;
	if(position.rot().z() > 0)
		angle=position.rot().angle(); 
	else {
		QuatF q = position.rot();
		q.negate();
		angle=q.angle(); 
	}
	angle+=M_PI/2.f;
	angle=fmod(angle, 2*M_PI);
	if(angle<0)angle+=2*M_PI;
	return angle;
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolPutConstantModel::quant()
{
	start_timer_auto();

	if(quantCnt<=1 && flag_stopOnInds){ //проверка на 0 и 1-м кванте
		if(vMap.isIndestructability(round(position.trans().x), round(position.trans().y))) 
			return false;
	}
	if(quantCnt<=0){ //Инициализация
		if(modelName.empty()){
			xassert(0&&"Toolzer isn't contented 3dx!");
			return false;
		}
		putModel2VBitmap(position);
		quantCnt=1;
		return true;
	}

	bool result=false;
	if(!terTexture.empty() && !independentTerTexture){
        float angle=convertPosition2AngleAboutZ(position);
		TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
		if(pbmp->pAlfaLayer==0){
			elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_DRAW_CURRENT, false> tool;
			const float dX=1.f;
			tool.setTexture(pbmp, round(floorf(position.trans().x)+dX), round(floorf(position.trans().y)) );
			result=elemetarQuant(tool);
		}
		else {
			elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_DRAW_CURRENT, true> tool;
			const float dX=1.f;
			tool.setTexture(pbmp,round(ceilf(position.trans().x)+dX),round(ceilf(position.trans().y)));
			result=elemetarQuant(tool);
		}
	}
	else {
		elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_NOT_CHANGE> tool;
		result=elemetarQuant(tool);
	}

	if(result==false){ //Наложение независимой текстуры на последнем кванте
		if(independentTerTexture && !terTexture.empty()){
	        float angle=convertPosition2AngleAboutZ(position);
			TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
			if(pbmp->pAlfaLayer==0){
				//elementarTool<SIM_NotChange, TTTE_SMOOTH, TTSE_DRAW_CURRENT> curTool;
				elementarTool<SIM_NotChange, TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT> curTool;
				curTool.setTexture(pbmp, position.trans().x, position.trans().y, angle);

				int x=round(position.trans().x);
				int y=round(position.trans().y);
				int i,j;
				int radius=min(pbmp->size.x/2, pbmp->size.y/2);
				if(radius > MAX_RADIUS_CIRCLEARR) radius=MAX_RADIUS_CIRCLEARR;
				const int alfaBorder=radius/3;
				for(i = 0;i < radius-alfaBorder; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					for(j = 0;j < max;j++) {
						curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0, 0);
					}
				}
				if(alfaBorder) for(i; i <= radius; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					short curAlpha=(i-(radius-alfaBorder))*255/alfaBorder;
					for(j = 0;j < max;j++) {
						curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0  , curAlpha);
					}
				}
				vMap.recalcArea2Grid(vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x + radius), vMap.YCYCL(y+radius) );
				vMap.regRender(vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius), vrtMap::TypeCh_Texture );
			}
			else { // if(pbmp->pAlfaLayer==0)
				//elementarTool<SIM_NotChange, TTTE_SMOOTH, TTSE_DRAW_CURRENT, true> curTool;
				elementarTool<SIM_NotChange, TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT, true> curTool;
				curTool.setTexture(pbmp, round(position.trans().x), round(position.trans().y), angle);
				int xb=round(position.trans().x) - pbmp->size.x/2;
				int yb=round(position.trans().y) - pbmp->size.y/2;
				int i,j;
				for(i=0; i<pbmp->size.y; i++){
					for(j=0; j<pbmp->size.x; j++){
						curTool.tVoxSet(vMap.XCYCL(xb + j), vMap.YCYCL(yb + i), 0, 0);
					}
				}
				vMap.recalcArea2Grid(vMap.XCYCL(xb), vMap.YCYCL(yb), vMap.XCYCL(xb + pbmp->size.x), vMap.YCYCL(yb+pbmp->size.y) );
				vMap.regRender(vMap.XCYCL(xb), vMap.YCYCL(yb), vMap.XCYCL(xb + pbmp->size.x), vMap.YCYCL(yb+pbmp->size.y), vrtMap::TypeCh_Texture );
			}
		}
	}

	return result;
}

struct Vect3i {
	int x, y, z;
	Vect3i() {}
	Vect3i(int x_, int y_, int z_) {x = x_; y = y_; z = z_;}
	typedef int int3[3];
	Vect3i(const int3& v) {x = v[0]; y = v[1]; z = v[2];}
};


//#define PN_FRACTION (16)
//#define PN_ROUND_MAKEWEIGHT (1<<15)
//#define PN_HALF_FRACTION (8)
#define PN_FRACTION (18)
#define PN_ROUND_MAKEWEIGHT (1<<17)
#define PN_HALF_FRACTION (9)
//#define PN_FRACTION (24)
//#define PN_ROUND_MAKEWEIGHT (1<<23)
//#define PN_HALF_FRACTION (12)

//#define roundFIntF0(a) ((a)>>16)
//#define mroundFIntF0(a) (((a)+(1<<15))>>16)
//#define cvrtFIntF8(a) ((a+(1<<7))>>8)
//#define ceilFIntF16(a) (((a)+0xffFF)&0xFFff0000)
//#define ceilFIntF0(a) (((a)+0xffFF)>>16)
//#define floorFIntF0(a) ((a)>>16)
//#define floorFIntF16(a) ((a)&0xFFff0000)

#define roundFIntF0(a) ((a)>>18)
#define mroundFIntF0(a) (((a)+(1<<17))>>18)
#define cvrtFIntF8(a) ((a+(1<<8))>>9)
#define ceilFIntF16(a) (((a)+0x3ffFF)&0xFFfc0000)
#define ceilFIntF0(a) (((a)+0x3ffFF)>>18)
#define floorFIntF0(a) ((a)>>18)
#define floorFIntF16(a) ((a)&0xFFfC0000)

//#define roundFIntF0(a) ((a)>>24)
//#define mroundFIntF0(a) (((a)+(1<<23))>>24)
//#define cvrtFIntF8(a) ((a+(1<<11))>>12)
//#define ceilFIntF16(a) (((a)+0xFFffFF)&0xffFFffFFff000000)
//#define ceilFIntF0(a) (((a)+0xFFffFF)>>24)
//#define floorFIntF0(a) ((a)>>24)
//#define floorFIntF16(a) ((a)&0xffFFffFFff000000)


#define SUBPIXEL
#define SUBTEXEL

bool TerToolPutConstantModel::putModel2VBitmap(const Se3f& pos)
{
	vector<sPolygon> poligonArr;
	vector<Vect3f> pointArr;
	GetAllTriangle3dx(modelName.c_str(), pointArr, poligonArr);

	if(!pointArr.size()) return false;

	//MatXf pose(pos);
	MatXf pose;
	if(layMetod==LM_AbsoluteHeight){
		pose.set(pos);
	}
	else {
		QuatF q(pos.rot());
		q.x()=0; q.y()=0; q.norm();
		//q.z()=0; q.s()=1;
		//pose.set(Mat3f::ID, pos.trans());
		pose.set(q, pos.trans());
	}
	pose.rot().scale(scaleFactor);
	//pose.rot().scale(model->GetScale());
	//pose.rot().scale(4.0f);

	pose.xformPoint(pointArr[0]);
	float minX,maxX; minX=maxX=pointArr[0].x;
	float minY,maxY; minY=maxY=pointArr[0].y;
	float minZ; minZ=pointArr[0].z;
	int i;
	for(i = 1; i<pointArr.size(); i++){
		pose.xformPoint(pointArr[i]);
		if(pointArr[i].x < minX) minX=pointArr[i].x;
		else if(pointArr[i].x > maxX)maxX=pointArr[i].x;
		if(pointArr[i].y < minY) minY=pointArr[i].y;
		else if(pointArr[i].y > maxY) maxY=pointArr[i].y;
		if(pointArr[i].z < minZ) minZ=pointArr[i].z;
	}

	minimalZ=round(minZ*(1<<VX_FRACTION));
	relativeCenterZ=round((pos.trans().z-minZ)*(1<<VX_FRACTION));

	int iminX=round(minX-1);
	int iminY=round(minY-1);
	minCoord.x=minX;
	minCoord.y=minY;

	//vector<Vect3i> iPntArr;
	//iPntArr.resize(pointArr.size());
	//for (i = 0; i < pointArr.size(); i++){
	//	//float x=pointArr[i].x;
	//	//float y=pointArr[i].y;
	//	//float z=pointArr[i].z;
	//	//xassert(abs(x)<32000);
	//	//xassert(abs(y)<32000);
	//	//xassert(abs(z)<32000);
	//	iPntArr[i].x=round((pointArr[i].x-minX)*(1<<PN_FRACTION));
	//	iPntArr[i].y=round((pointArr[i].y-minY)*(1<<PN_FRACTION));
	//	iPntArr[i].z=round((pointArr[i].z-minZ)*(1<<PN_FRACTION));
	//}

	for (i = 0; i < pointArr.size(); i++){
		pointArr[i].z=pointArr[i].z-minZ;
	}

	voxelBitmap.create(round(maxX+1)- round(minX -1), round(maxY+1)- round(minY -1));
	memset(voxelBitmap.pRaster, 0xFF, sizeof(voxelBitmap.pRaster[0])*voxelBitmap.sx*voxelBitmap.sy);

	vector<sPolygon>::iterator p;
	for(p=poligonArr.begin(); p!=poligonArr.end(); p++){

		//float -алгоритм
		const Vect3f* a = &pointArr[p->p1]; // Для сортировки по Y.
		const Vect3f* b = &pointArr[p->p2];
		const Vect3f* c = &pointArr[p->p3];
		if(a->y > b->y) swap(a, b);
		if(a->y > c->y) swap(a, c);
		if(b->y > c->y) swap(b, c);
		// грань нулевой высоты рисовать не будем( а надо, для того-чтоб не пропадали точки когда очень много полигонов на точку)
		///if (round(c->y) <= round(a->y)) continue;

		int current_sx, current_sy;
		float tmp, k, x_start, x_end;
		float dx_start, dx_end, dz1_start, dz1_end;
		float z1_start, z1_end;
		float x, z1, dz1;
		int length;
		//unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		float divisor;
		divisor=(c->y - a->y);
		if(divisor) k = (b->y - a->y) / divisor;
		else k=0;
		x_start = a->x + (c->x - a->x) * k;
		z1_start = a->z + (c->z - a->z) * k;
		x_end = b->x;
		z1_end = b->z;
		divisor=(x_start - x_end);
		if(divisor) dz1 = (z1_start - z1_end) / divisor;
		else dz1 = 0;

		x_start = a->x;
		z1_start = a->z;
		divisor=(c->y - a->y);
		if(divisor){
			dx_start = (c->x - a->x) / divisor;
			dz1_start = (c->z - a->z) / divisor;
		}
		else { dx_start =0; dz1_start=0;}
#ifdef SUBPIXEL
		tmp = ceilf(a->y) - a->y;
		x_start += dx_start * tmp;
		z1_start += dz1_start * tmp;
#endif

		if (ceilf(b->y) > ceilf(a->y)) {
			tmp = ceilf(a->y) - a->y;
			x_end = a->x;
			z1_end = a->z;
			divisor=(b->y - a->y);
			if(divisor){
				dx_end = (b->x - a->x) / divisor;
				dz1_end = (b->z - a->z) / divisor;
			}
			else { dx_end=0; dz1_end=0; }
		} else {
			tmp = ceilf(b->y) - b->y;
			x_end = b->x;
			z1_end = b->z;
			divisor=(c->y - b->y);
			if(divisor){
				dx_end = (c->x - b->x) / divisor;
				dz1_end = (c->z - b->z) / divisor;
			}
			else { dx_end=0; dz1_end=0; }
		}
#ifdef SUBPIXEL
		x_end += dx_end * tmp;
		z1_end += dz1_end * tmp;
#endif

////////////////////////////////

		// построчная отрисовка грани
		for (current_sy = ceilf(a->y); current_sy <= floorf(c->y); current_sy++) { //current_sy < ceilf(c->y)
			if((current_sy-iminY) >= voxelBitmap.sy) break;
			//if((current_sy-minY) < 0 ) break;//continue;
			if (current_sy == ceilf(b->y)) {
				x_end = b->x;
				z1_end = b->z;
				divisor=(c->y - b->y);
				if(divisor){
                    dx_end = (c->x - b->x) / divisor;
					dz1_end = (c->z - b->z) / divisor;
				}
				else { dx_end=0; dz1_end=0; }
#ifdef SUBPIXEL
				tmp = ceilf(b->y) - b->y;
				x_end += dx_end * tmp;
				z1_end += dz1_end * tmp;
#endif
			}

			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  z1 = z1_end;
			  length = ceilf(x_start) - ceilf(x_end);
			} else {
			  x = x_start;
			  z1 = z1_start;
			  length = ceilf(x_end) - ceilf(x_start);
			}

			// считаем адрес начала строки в видеопамяти
			//dest = GB;
			//dest += current_sy * sizeX05*2 + (int)ceilf(x);

			// текстурируем строку
			current_sx = round(ceilf(x))-iminX;
	
			if((current_sy-iminY) >= 0 ) if (length) {
		#ifdef SUBTEXEL
		      tmp = ceilf(x) - x;
			  z1 += dz1* tmp;
		#endif
				while (length--) {
				// используем z-буфер для определения видимости текущей точки
					if( (current_sx<voxelBitmap.sx) && (current_sx >= 0)) {
						//if (zBuffer[(current_sy-iminY)*voxelBitmap.sx + current_sx] <= z1) {
						//	//*dest = palLight[round(cc)];
						//	zBuffer[(current_sy-iminY)*voxelBitmap.sx + current_sx] = z1;
						//}
						register int bufoff=(current_sy-iminY)*voxelBitmap.sx + current_sx;
						if(voxelBitmap.pRaster[bufoff]<= round(z1*(1<<VX_FRACTION)) ){
							voxelBitmap.pRaster[bufoff] = round(z1*(1<<VX_FRACTION));
						}
					}
					z1 += dz1;
					//dest++;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			z1_start += dz1_start;
			x_end += dx_end;
			z1_end += dz1_end;
		}


/*
		//1-й целочисленный алгоритм - на балистик- 1 точка

		const Vect3i* a = &iPntArr[p->p1]; // Для сортировки по Y.
		const Vect3i* b = &iPntArr[p->p2];
		const Vect3i* c = &iPntArr[p->p3];
		if(a->y > b->y) swap(a, b);
		if(a->y > c->y) swap(a, c);
		if(b->y > c->y) swap(b, c);

		int curMinX,curMaxX;
		curMinX=curMaxX=a->x;
		if(curMinX > b->x)curMinX=b->x;
		if(curMaxX < b->x)curMaxX=b->x;
		if(curMinX > c->x)curMinX=c->x;
		if(curMaxX < c->x)curMaxX=c->x;
		curMinX=clamp(curMinX, 0, round(maxX)<<PN_FRACTION);
		curMaxX=clamp(curMaxX, 0, round(maxX)<<PN_FRACTION);
		//curMinX=0;
		//curMaxX=round(maxX)<<PN_FRACTION;

		//if(roundFIntF0(c->y - a->y)<=0) continue;

		int current_sx, current_sy;

		//float
		int k, tmp, x_start, x_end; 
		int dx_start, dx_end, dz1_start, dz1_end;
		int z1_start, z1_end;
		int x, z1, dz1;

		int length;
		//unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		int divisor;
		divisor=(c->y - a->y);
		//if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_HALF_FRACTION) / divisor;// F8
		//else k=0;
		//x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
		//z1_start = a->z + (cvrtFIntF8(c->z - a->z))*(k);
		if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_FRACTION) / divisor;// F16
		else k=0;
		x_start = a->x + (int)mroundFIntF0((__int64)(c->x - a->x)*(k));//На самом деле F16(F18)!
		z1_start = a->z + (int)mroundFIntF0((__int64)(c->z - a->z)*(k));
		x_end = b->x;
		z1_end = b->z;

		divisor= x_start - x_end;
		if(roundFIntF0(divisor)) {
			dz1 = (((__int64)(z1_start - z1_end)<<PN_FRACTION)/divisor);
		}
		else dz1=0;

		x_start = a->x;
		z1_start = a->z;
		divisor=(c->y - a->y);
		if(roundFIntF0(divisor)){
			dx_start = ((__int64)(c->x - a->x)<<PN_FRACTION) / divisor; 
			dz1_start = ((__int64)(c->z - a->z)<<PN_FRACTION) / divisor; 
		}
		else { dx_start=0; dz1_start=0;}
#ifdef SUBPIXEL
		tmp = ceilFIntF16(a->y) - a->y;
		x_start += (__int64)dx_start*tmp>>PN_FRACTION;//(dx_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
		z1_start += (__int64)dz1_start*tmp>>PN_FRACTION;//(dz1_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif
		if (ceilFIntF16(b->y) > ceilFIntF16(a->y)) {
			tmp = ceilFIntF16(a->y) - (a->y);
			x_end = a->x;
			z1_end = a->z;
			divisor=b->y - a->y;
			if(roundFIntF0(divisor)){ 
				dx_end = ((__int64)(b->x - a->x)<<PN_FRACTION) / divisor;
                dz1_end = ((__int64)(b->z - a->z)<<PN_FRACTION) / divisor;
			}
			else {dx_end =0; dz1_end =0; }
		} else {
			tmp = ceilFIntF16(b->y) - b->y;			//???????????
			x_end = b->x;
			z1_end = b->z;
			divisor=c->y - b->y;
			if(roundFIntF0(divisor)){
				dx_end = ((__int64)(c->x - b->x)<<PN_FRACTION) / divisor;
                dz1_end = ((__int64)(c->z - b->z)<<PN_FRACTION) / divisor;
			}
			else{ dx_end=0; dz1_end=0; }
		}
#ifdef SUBPIXEL
		x_end += (__int64)dx_end*tmp>>PN_FRACTION;//(dx_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
		z1_end += (__int64)dz1_end*tmp>>PN_FRACTION;//(dz1_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif

////////////////////////////////
//loc_scip01:;
		// построчная отрисовка грани
		for (current_sy = ceilFIntF0(a->y); current_sy <= floorFIntF0(c->y); current_sy++) { //ceilFIntF0(c->y)
			if((current_sy) >= voxelBitmap.sy ) break;
			if (current_sy == ceilFIntF0(b->y)) {
				x_end = b->x;
				z1_end = b->z;
				divisor=(c->y - b->y);
				if(roundFIntF0(divisor)){
					dx_end=((__int64)(c->x - b->x)<<PN_FRACTION)/divisor;
					dz1_end=((__int64)(c->z - b->z)<<PN_FRACTION)/divisor;
				}
				else { dx_end=0; dz1_end=0; }
#ifdef SUBPIXEL
				tmp = ceilFIntF16(b->y) - b->y;
				x_end += (__int64)dx_end*tmp>>PN_FRACTION;//(dx_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
				z1_end += (__int64)dz1_end*tmp>>PN_FRACTION;//(dz1_end>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif
			}

			//xassert(x_start >= curMinX -(1<<16));
			//xassert(x_start <= curMaxX +(1<<16));
			//xassert(x_end >= curMinX -(1<<16));
			//xassert(x_end <= curMaxX +(1<<16));
			//x_start=clamp(x_start, curMinX, curMaxX);
			//x_end=clamp(x_end, curMinX, curMaxX);
			// x_start должен находиться левее x_end
			if (x_start > x_end) {
			  x = x_end;
			  z1 = z1_end;
			  length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
			} else {
			  x = x_start;
			  z1 = z1_start;
			  length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
			}
			length++;

			// текстурируем строку
			current_sx = ceilFIntF0(x);
	
			if((current_sy) >= 0 ) if (length) {
		#ifdef SUBTEXEL
		      //tmp = ceil(x) - x;
		      tmp = ceilFIntF16(x) - x;
			  z1 += (__int64)dz1*tmp>>PN_FRACTION;//(dz1>>PN_HALF_FRACTION)* (tmp>>PN_HALF_FRACTION);
		#endif
				while (length--) {
				// используем z-буфер для определения видимости текущей точки
					//xassert(current_sx <= ceilFIntF0(a->x)-iminX+10 || current_sx <= ceilFIntF0(b->x)-iminX+10 || current_sx <= ceilFIntF0(c->x)-iminX+10);
					//xassert(current_sx >= ceilFIntF0(a->x)-iminX-10 || current_sx >= ceilFIntF0(b->x)-iminX-10 || current_sx >= ceilFIntF0(c->x)-iminX-10);
					//xassert(z1 <= a->z+(20<<16) || z1 <= b->z+(20<<16) || z1 <= c->z+(20<<16));
					//xassert(z1 >= a->z-(20<<16) || z1 >= b->z-(20<<16) || z1 >= c->z-(20<<16));
					if( (current_sx<voxelBitmap.sx) && (current_sx >= 0)) {
						register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
						if(voxelBitmap.pRaster[bufoff]<= z1>>(PN_FRACTION-VX_FRACTION) ){
							voxelBitmap.pRaster[bufoff] = z1>>(PN_FRACTION-VX_FRACTION);
						}
					}
					z1 += dz1;
					current_sx++;
				}
			}

			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			z1_start += dz1_start;
			x_end += dx_end;
			z1_end += dz1_end;
		}
*/

/*
		//2-й целочисленный алгоритм - баги на wall на балистик- 1 точка
#undef SUBPIXEL
#undef SUBTEXEL
		const Vect3i* a = &iPntArr[p->p1]; // Для сортировки по Y.
		const Vect3i* b = &iPntArr[p->p2];
		const Vect3i* c = &iPntArr[p->p3];
		if(a->y > b->y) swap(a, b);
		if(a->y > c->y) swap(a, c);
		if(b->y > c->y) swap(b, c);

		int curMinX,curMaxX;
		curMinX=curMaxX=a->x;
		if(curMinX > b->x)curMinX=b->x;
		if(curMaxX < b->x)curMaxX=b->x;
		if(curMinX > c->x)curMinX=c->x;
		if(curMaxX < c->x)curMaxX=c->x;
		curMinX=clamp(curMinX, 0, round(maxX)<<PN_FRACTION);
		curMaxX=clamp(curMaxX, 0, round(maxX)<<PN_FRACTION);
		//curMinX=0;
		//curMaxX=round(maxX)<<PN_FRACTION;

		//if(roundFIntF0(c->y - a->y)<=0) continue;


		//float
		int k, tmp, x_start, x_end; 
		int dx_start, dx_end, dz1_start, dz1_end;
		int z1_start, z1_end;
		int dz1;

		//unsigned short *dest;

		// посчитаем du/dsx, dv/dsx, d(1/z)/dsx
		// считаем по самой длинной линии (т.е. проходящей через вершину B)
		int divisor;
		divisor=(c->y - a->y);
		//if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_HALF_FRACTION) / divisor;// F8
		//else k=0;
		//x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
		//z1_start = a->z + (cvrtFIntF8(c->z - a->z))*(k);
		if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<PN_FRACTION) / divisor;// F16
		else k=0;
		x_start = a->x + (int)mroundFIntF0((__int64)(c->x - a->x)*(k));//На самом деле F16(F18)!
		z1_start = a->z + (int)mroundFIntF0((__int64)(c->z - a->z)*(k));
		x_end = b->x;
		z1_end = b->z;

		divisor= x_start - x_end;
		if(roundFIntF0(divisor)) {
			dz1 = (((__int64)(z1_start - z1_end)<<PN_FRACTION)/divisor);
		}
		else dz1=0;

		x_start = a->x;
		z1_start = a->z;
		divisor=(c->y - a->y);
		if(roundFIntF0(divisor)){
			dx_start = ((__int64)(c->x - a->x)<<PN_FRACTION) / divisor; 
			dz1_start = ((__int64)(c->z - a->z)<<PN_FRACTION) / divisor; 
		}
		else { dx_start=0; dz1_start=0;}
#ifdef SUBPIXEL
		tmp = ceilFIntF16(a->y) - a->y;
		x_start += (__int64)dx_start*tmp>>PN_FRACTION;//(dx_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
		z1_start += (__int64)dz1_start*tmp>>PN_FRACTION;//(dz1_start>>PN_HALF_FRACTION) * (tmp>>PN_HALF_FRACTION); //Норма
#endif

		int current_sy=mroundFIntF0(a->y);
		if(mroundFIntF0(b->y) > current_sy) {
			x_end = a->x;
			z1_end = a->z;
			divisor=b->y - a->y;
			if(roundFIntF0(divisor)){ 
				dx_end = ((__int64)(b->x - a->x)<<18) / divisor;
				dz1_end = ((__int64)(b->z - a->z)<<18) / divisor;
			}
			else {dx_end =0; dz1_end=0; }
	#ifdef SUBPIXEL
			//tmp = ceilFIntF16(a->y) - (a->y);
			tmp = mroundFIntF0(a->y) - (a->y);
			x_end += (dx_end>>8) * (tmp>>8); //Норма
			z1_end += (dz1_end>>8) * (tmp>>8); //Норма
	#endif
			// построчная отрисовка грани
			do {
				// x_start должен находиться левее x_end
				x_start=clamp(x_start, curMinX, curMaxX);
				x_end=clamp(x_end, curMinX, curMaxX);
				int x,z1;
				int xe;
				if (x_start > x_end) {
					x = x_end;
					z1 = z1_end;
					//length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
					//xe = ceilFIntF0(x_start);
					xe = mroundFIntF0(x_start);
				}else {
					x = x_start;
					z1 = z1_start;
					//length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
					//xe = ceilFIntF0(x_end);
					xe = mroundFIntF0(x_end);
				}
				// текстурируем строку
				//int current_sx = ceilFIntF0(x);
				int current_sx = mroundFIntF0(x);
				xassert(current_sy>=0 && current_sy < voxelBitmap.sy);
				//if((current_sy) >= 0 ) if (length) {
		#ifdef SUBTEXEL
					tmp = ceilFIntF16(x) - x;
					z1 += (dz1>>8)* (tmp>>8);
		#endif
					for(current_sx; current_sx<=xe; current_sx++){
						register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
						if(voxelBitmap.pRaster[bufoff] <= z1>>(18-VX_FRACTION) ){
							voxelBitmap.pRaster[bufoff] = z1>>(18-VX_FRACTION);
						}
						z1 += dz1;
					}
				//}
				// сдвигаем начальные и конечные значения x/u/v/(1/z)
				x_start += dx_start;
				x_end += dx_end;
				z1_start += dz1_start;
				z1_end += dz1_end;
				current_sy++;
			}while(current_sy < mroundFIntF0(b->y));
		}
		///////////////////////////////////////
		if(mroundFIntF0(c->y) >= current_sy) {
			x_end = b->x;
			z1_end = b->z;
			divisor = c->y - b->y;
			if(roundFIntF0(divisor)){
				dx_end = ((__int64)(c->x - b->x)<<18) / divisor;
				dz1_end = ((__int64)(c->z - b->z)<<18) / divisor;
			}
			else{ dx_end=0; dz1_end=0; }
		#ifdef SUBPIXEL
			//tmp = ceilFIntF16(b->y) - b->y;
			tmp = mroundFIntF0(b->y) - b->y;
			x_end += (dx_end>>8) * (tmp>>8); //Норма
			z1_end += (dz1_end>>8) * (tmp>>8); //Норма
		#endif
			// построчная отрисовка грани
			// построчная отрисовка грани
			do {
				// x_start должен находиться левее x_end
				x_start=clamp(x_start, curMinX, curMaxX);
				x_end=clamp(x_end, curMinX, curMaxX);
				int x,z1;
				int xe;
				if (x_start > x_end) {
					x = x_end;
					z1 = z1_end;
					//length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
					//xe = ceilFIntF0(x_start);
					xe = mroundFIntF0(x_start);
				}else {
					x = x_start;
					z1 = z1_start;
					//length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
					//xe = ceilFIntF0(x_end);
					xe = mroundFIntF0(x_end);
				}
				// текстурируем строку
				//int current_sx = ceilFIntF0(x);
				int current_sx = mroundFIntF0(x);
				xassert(current_sy>=0 && current_sy < voxelBitmap.sy);
				//if((current_sy) >= 0 ) if (length) {
		#ifdef SUBTEXEL
					tmp = ceilFIntF16(x) - x;
					z1 += (dz1>>8)* (tmp>>8);
		#endif
					for(current_sx; current_sx<=xe; current_sx++){
						register int bufoff=(current_sy)*voxelBitmap.sx + current_sx;
						if(voxelBitmap.pRaster[bufoff] <= z1>>(18-VX_FRACTION) ){
							voxelBitmap.pRaster[bufoff] = z1>>(18-VX_FRACTION);
						}
						z1 += dz1;
					}
				//}
				// сдвигаем начальные и конечные значения x/u/v/(1/z)
				x_start += dx_start;
				x_end += dx_end;
				z1_start += dz1_start;
				z1_end += dz1_end;
				current_sy++;
			}while(current_sy <= mroundFIntF0(c->y));
		}*/
	
	}
	return true;
}

////////////////////////////////////
template <eSetingAtrMetod setingIndsMetod>
bool TerToolTrack1::quant()
{
	start_timer_auto();

	if(position.trans().distance2(prevPos.trans()) < curstep*curstep)
		return true;
	if(!r)
		return false;
	prevPos=position;
	const float STEP=(float)r/3.f;
	const float DELTA_STEP=(float)r/4.f;
	curstep = STEP + logicRNDfabsRnd(DELTA_STEP);

	const int DELTA_RADIUS=6+ r/10;
	int radius=r + XRnd(DELTA_RADIUS);
	if(radius>MAX_RADIUS_CIRCLEARR){
		radius=MAX_RADIUS_CIRCLEARR;
	}
	
	int x=position.trans().x;
	int y=position.trans().y;
	int i, j;
	const unsigned short levH=position.trans().z*(1<<VX_FRACTION);
	char typeChanges=0;
	if(terTexture.empty()){
		elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_NOT_CHANGE> curTool;
		for(i = 0;i <= radius; i++){
			int max = maxRad[i];
			int* xx = xRad[i];
			int* yy = yRad[i];
			for(j = 0;j < max;j++) {
				if( vMap.GetAltC( x + xx[j], y + yy[j] ) + (5<<VX_FRACTION) > levH){
					curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), -( ((radius-i)<<VX_FRACTION)/12 ) );
					//curTool.voxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), levH -( ((radius-i)<<VX_FRACTION)/10 ) );
				}
			}
		}
		typeChanges=vrtMap::TypeCh_Height;
	}
	else {
		elementarTool<setingIndsMetod, TTTE_SMOOTH, TTSE_DRAW_CURRENT> curTool;
		curTool.setTexture(terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale),position.trans().x,position.trans().y);
		for(i = 0;i <= radius; i++){
			int max = maxRad[i];
			int* xx = xRad[i];
			int* yy = yRad[i];
			for(j = 0;j < max;j++) {
				//curTool.voxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), levH);
				if( vMap.GetAltC( x + xx[j], y + yy[j] ) + (5<<VX_FRACTION) > levH){
					curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), -( ((radius-i)<<VX_FRACTION)/12 ) , i*255/radius);
				}
			}
		}
		typeChanges=vrtMap::TypeCh_Texture|vrtMap::TypeCh_Height;
	}
	if(drawRegionAndImpassability(x, y, radius))
		typeChanges|=vrtMap::TypeCh_Region;
	vMap.recalcArea2Grid( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius) );
	vMap.regRender( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius), typeChanges);
	return true;
}

void TerToolTrack1::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.tga", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of texture file");
	ar.serialize(ResourceSelector(terTexture, options), "terTexture", "тертекстура");
	ar.serialize(terTextureKScale, "terTextureKScale", "масштаб тертекстуры");
	ar.serialize(r, "r", "радиус");
	if(r<=0) r=1;
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
}
///////////////////////////////////////////////////////////////////////////////////
TerToolTorpedo::TerToolTorpedo()
{
	r=1; 
	curstep=0;
	step=1.f;
	deltaStep=0.f;
}

void TerToolTorpedo::serialize(Archive& ar)
{
	ar.serialize(r, "r", "радиус");
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
	if(r<=0) r=1;
}

template <eSetingAtrMetod setingIndsMetod> 
bool TerToolTorpedo::quant()
{
	start_timer_auto();

	bool result=true;
	radius=24; ///!!!
	if(position.trans().distance2(prevPos.trans()) >= curstep*curstep) {
		r=4;

		const float STEP=(float)r*1.5;// /2.f;
		const float DELTA_STEP=(float)r/2.f; //4
		curstep = STEP + logicRNDfabsRnd(DELTA_STEP);

		const int DELTA_RADIUS= r/8;
		int rad=r + XRnd(DELTA_RADIUS);
		if(rad>MAX_RADIUS_CIRCLEARR){
			rad=MAX_RADIUS_CIRCLEARR;
		}
		float angle;
		if(position.rot().z() > 0)
			angle=position.rot().angle(); 
		else {
			QuatF q = position.rot();
			q.negate();
			angle=q.angle(); 
		}
		angle+=M_PI/2.f;
		angle=fmod(angle, 2*M_PI);
		if(angle<0)angle+=2*M_PI;

		//Vect3f d=position.rot().xform(Vect3f::ID);
		sRect cha=sTorpedo::quant(Vect2f(prevPos.trans()), Vect2f(position.trans()));
		prevPos=position;
		char typeChanges=vrtMap::TypeCh_Height;
		if(drawRegionAndImpassability(cha.x+cha.sx/2, cha.y+cha.sy/2, min(cha.sx/2, cha.sy/2)))
			typeChanges|=vrtMap::TypeCh_Region;
		vMap.recalcArea2Grid(cha.x, cha.y, cha.xr(), cha.yb());
		vMap.regRender(cha.x, cha.y, cha.xr(), cha.yb(), typeChanges);

		//
		const int TILE_ADDON_SIZE=2;
		if(round(Vect2f(position.trans()).distance2(prevPosInds)) >  sqr(radius+TILE_ADDON_SIZE)){
			int xi=round(prevPosInds.x);
			int yi=round(prevPosInds.y);
			if(setingIndsMetod==SIM_SetInds)
                vMap.setInds(xi, yi, radius);
			else if(setingIndsMetod==SIM_UnSetInds)
                vMap.setInds(xi, yi, radius, true);
			vMap.recalcArea2Grid(xi-radius, yi-radius, xi+radius, yi+radius);
			prevPosInds=Vect2f(position.trans());
		}
		
	}
	//if(!result)
	//	drawRegionAndImpassability(round(position.trans().x), round(position.trans().y), r);

	//char typeChanges=vrtMap::TypeCh_Height;
	//if(drawRegionAndImpassability(p[0].x+dx05, p[0].y+dy05, min(dx05,dy05)))
	//	typeChanges|=vrtMap::TypeCh_Region;
	//vMap.recalcArea2Grid(p[0].x, p[0].y, p[1].x, p[1].y);
	//vMap.regRender(p[0].x, p[0].y, p[1].x, p[1].y, typeChanges);

	return result;
}

void TerToolTorpedo::absoluteQuant()
{
	bubbleQuant();
}

///////////////////////////////////////////////////////////////////////////////////
TerToolTexture::TerToolTexture()
{
	r=1;
	amountFrame=1;
	drawMetod=DM_Cyrcle;
	prevSize05=Vect2f::ZERO;
	flag_restoreSurface=false;
}

void TerToolTexture::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.tga", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of texture file");
	ar.serialize(ResourceSelector(terTexture, options), "terTexture", "тертекстура");
	ar.serialize(terTextureKScale, "terTextureKScale", "масштаб тертекстуры");
	ar.serialize(drawMetod, "drawMetod", "Метод отрисовки");
	ar.serialize(r, "r", "радиус");
	ar.serialize(amountFrame, "amountFrame", "количество кадров");
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
	if(r<=0) r=1;
	if(amountFrame<=0) amountFrame=0;

	ar.serialize(flag_restoreSurface, "flag_restoreSurface", "Восстанавливать поверхность");
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolTexture::quant()
{
	start_timer_auto();

	////////////////////////////////////////
	if(terTexture.empty()) return false;

	if(drawMetod==DM_Cyrcle){
		int radius=r;
		if(radius>MAX_RADIUS_CIRCLEARR){
			radius=MAX_RADIUS_CIRCLEARR;
		}
		float angle;
		if(position.rot().z() > 0)
			angle=position.rot().angle(); 
		else {
			QuatF q = position.rot();
			q.negate();
			angle=q.angle(); 
		}
		angle+=M_PI/2.f;
		angle=fmod(angle, 2*M_PI);
		if(angle<0)angle+=2*M_PI;

		int x=position.trans().x;
		int y=position.trans().y;
		int i, j;
		const unsigned short levH=position.trans().z*(1<<VX_FRACTION);

		TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
		if(!pbmp) return false;
		if(pbmp->pAlfaLayer==0){
			elementarTool<setingIndsMetod,TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT> curTool;
			curTool.setTexture(pbmp, position.trans().x, position.trans().y, angle);
			const int alfaBorder=radius/3;
			for(i = 0;i < radius-alfaBorder; i++){
				int max = maxRad[i];
				int* xx = xRad[i];
				int* yy = yRad[i];
				for(j = 0;j < max;j++) {
					curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0, 0);
				}
			}
			if(alfaBorder) for(i; i <= radius; i++){
				int max = maxRad[i];
				int* xx = xRad[i];
				int* yy = yRad[i];
				short curAlpha=(i-(radius-alfaBorder))*255/alfaBorder;
				for(j = 0;j < max;j++) {
					curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0  , curAlpha);
				}
			}
		}
		else { // if(pbmp->pAlfaLayer==0)
			elementarTool<setingIndsMetod,TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT, true> curTool;
			curTool.setTexture(pbmp, position.trans().x, position.trans().y, angle);
			for(i = 0;i < radius; i++){
				int max = maxRad[i];
				int* xx = xRad[i];
				int* yy = yRad[i];
				for(j = 0;j < max;j++) {
					curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0, 0);
				}
			}
		}
		drawRegionAndImpassability(x, y, radius);
		vMap.recalcArea2Grid( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius) );
		vMap.regRender( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius), vrtMap::TypeCh_Texture );
	}
	else if(drawMetod==DM_Bitmap){ ////////////////////////////////////////
		TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
		if(!pbmp) return false;
		if(pbmp->pAlfaLayer==0) {
			xassert(0 && "bitmap track Alfa not present!");
			return false; //Не может быть без альфы следа по битмапу
		}
		float sx05=(pbmp->size.y-1)*quantCnt/(amountFrame*2.f);
		float sy05=(pbmp->size.x-1)*quantCnt/(amountFrame*2.f);
		float diag2=sqr(sx05)+sqr(sy05);
		if(diag2-prevSize05.norm2()> 1.5f*1.5f){
			prevSize05.set(sx05, sy05);
			Vect3f dxy1(-sx05, -sy05,0);
			Vect3f dxy2(sx05, -sy05,0);
			Vect3f dxy3(sx05, sy05,0);
			Vect3f dxy4(-sx05, sy05,0);
			dxy1=position.rot().xform(dxy1);
			dxy2=position.rot().xform(dxy2);
			dxy3=position.rot().xform(dxy3);
			dxy4=position.rot().xform(dxy4);

			Vertex2i p[4]= {
				Vertex2i(round((1<<16)*(position.trans().x+dxy1.x)), round((1<<16)*(position.trans().y+dxy1.y)), 0, 0),
				Vertex2i(round((1<<16)*(position.trans().x+dxy2.x)), round((1<<16)*(position.trans().y+dxy2.y)), 0, (pbmp->size.y-1)<<16),
				Vertex2i(round((1<<16)*(position.trans().x+dxy3.x)), round((1<<16)*(position.trans().y+dxy3.y)), (pbmp->size.x-1)<<16, (pbmp->size.y-1)<<16),
				Vertex2i(round((1<<16)*(position.trans().x+dxy4.x)), round((1<<16)*(position.trans().y+dxy4.y)), (pbmp->size.x-1)<<16, 0)
			};
			for(int k=0; k<4; k++){
				if(p[k].x<0 || p[k].y<0) return true;
				if(p[k].x >= (vMap.H_SIZE<<16) || p[k].y >= (vMap.V_SIZE<<16) ) return true;
			}
			if(flag_restoreSurface){
				int cnt=0;
				for(int i=0; i<backupBmp.sy; i++){
					int off=vMap.offsetBuf(0, backupBmp.mY+i);
					for(int j=0; j<backupBmp.sx; j++)
						vMap.SurBuf[off + backupBmp.mX +j] = backupBmp.data[cnt++];
				}
				draw4SidePolygon<TerToolsDispatcher::Bitmap8C,setingIndsMetod>(p, pbmp, &backupBmp);
			}
			else 
				draw4SidePolygon<TerToolsDispatcher::Bitmap8C,setingIndsMetod>(p, pbmp);
			int dx05=(p[1].x-p[0].x)/2;
			int dy05=(p[1].y-p[0].y)/2;
			char typeChanges=vrtMap::TypeCh_Texture;
			if(drawRegionAndImpassability(p[0].x+dx05, p[0].y+dy05, min(dx05,dy05)))
				typeChanges|=vrtMap::TypeCh_Region;
			vMap.recalcArea2Grid(p[0].x, p[0].y, p[1].x, p[1].y);
			vMap.regRender(p[0].x, p[0].y, p[1].x, p[1].y, typeChanges);
		}
		quantCnt++;
		if(quantCnt>amountFrame) return false;
		else return true;
	}
	else if(drawMetod==DM_ReliefBitmapPressIn || drawMetod==DM_ReliefBitmapSwellOut){
		TerToolsDispatcher::Bitmap8V* pbmp=terToolsDispatcher.getBitmap8V(terTexture.c_str(), 1.f);
		if(!pbmp) return false;
		float sx2=(pbmp->size.y*terTextureKScale-1)/2.f;
		float sy2=(pbmp->size.x*terTextureKScale-1)/2.f;
		Vect3f dxy1(-sx2, -sy2,0);
		Vect3f dxy2(sx2, -sy2,0);
		Vect3f dxy3(sx2, sy2,0);
		Vect3f dxy4(-sx2, sy2,0);
		dxy1=position.rot().xform(dxy1);
		dxy2=position.rot().xform(dxy2);
		dxy3=position.rot().xform(dxy3);
		dxy4=position.rot().xform(dxy4);
		Vertex2i p[4]= {
			Vertex2i(round((1<<16)*(position.trans().x+dxy1.x)), round((1<<16)*(position.trans().y+dxy1.y)), 0, 0),
			Vertex2i(round((1<<16)*(position.trans().x+dxy2.x)), round((1<<16)*(position.trans().y+dxy2.y)), 0, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy3.x)), round((1<<16)*(position.trans().y+dxy3.y)), (pbmp->size.x-1)<<16, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy4.x)), round((1<<16)*(position.trans().y+dxy4.y)), (pbmp->size.x-1)<<16, 0)
		};
		char typeChanges=vrtMap::TypeCh_Height;
		draw4SidePolygon<TerToolsDispatcher::Bitmap8V,setingIndsMetod>(p, pbmp);
		int dx05=(p[1].x-p[0].x)/2;
		int dy05=(p[1].y-p[0].y)/2;
		if(drawRegionAndImpassability(p[0].x+dx05, p[0].y+dy05, min(dx05,dy05)))
			typeChanges|=vrtMap::TypeCh_Region;
		vMap.recalcArea2Grid(p[0].x, p[0].y, p[1].x, p[1].y);
		vMap.regRender(p[0].x, p[0].y, p[1].x, p[1].y, typeChanges);
	}
	return false;
}


TerToolTextureTrack::TerToolTextureTrack() 
{ 
	r=1; curstep=0; textureOff=0; 
	drawMetod=DM_Cyrcle;
	moveMetod=MM_ChangePosition;
	step=1.f;
	deltaStep=0.f;
}

template <eSetingAtrMetod setingIndsMetod>
bool TerToolTextureTrack::quant()
{
	start_timer_auto();

	if(terTexture.empty()) 
		return false;
	if(drawMetod==DM_Cyrcle){
		if(position.trans().distance2(prevPos.trans()) >= curstep*curstep) {
			const float STEP=(float)r*1.5;// /2.f;
			const float DELTA_STEP=(float)r/2.f; //4
			curstep = STEP + logicRNDfabsRnd(DELTA_STEP);

			const int DELTA_RADIUS= r/8;
			int radius=r + XRnd(DELTA_RADIUS);
			if(radius>MAX_RADIUS_CIRCLEARR){
				radius=MAX_RADIUS_CIRCLEARR;
			}
			textureOff+=position.trans().distance(prevPos.trans());
			float angle;
			if(position.rot().z() > 0)
				angle=position.rot().angle(); 
			else {
				QuatF q = position.rot();
				q.negate();
				angle=q.angle(); 
			}
			angle+=M_PI/2.f;
			angle=fmod(angle, 2*M_PI);
			if(angle<0)angle+=2*M_PI;

			int x=position.trans().x;
			int y=position.trans().y;
			int i, j;
			const unsigned short levH=position.trans().z*(1<<VX_FRACTION);

			TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
			if(!pbmp) return false;
			if(pbmp->pAlfaLayer==0){

				elementarTool<setingIndsMetod,TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT> curTool;

				curTool.setTexture(pbmp, position.trans().x-textureOff*cos(angle), position.trans().y-textureOff*sin(angle), angle);

				const int alfaBorder=radius/3;
				for(i = 0;i < radius-alfaBorder; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					for(j = 0;j < max;j++) {
						curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0, 0);
					}
				}
				if(alfaBorder) for(i; i <= radius; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					short curAlpha=(i-(radius-alfaBorder))*255/alfaBorder;
					for(j = 0;j < max;j++) {
						curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0  , curAlpha);
					}
				}
			}
			else { // if(pbmp->pAlfaLayer==0)
	
				elementarTool<setingIndsMetod,TTTE_SMOOTH, TTSE_ROTATE_DRAW_CURRENT, true> curTool;

				curTool.setTexture(pbmp, position.trans().x-textureOff*cos(angle), position.trans().y-textureOff*sin(angle), angle);

				for(i = 0;i < radius; i++){
					int max = maxRad[i];
					int* xx = xRad[i];
					int* yy = yRad[i];
					for(j = 0;j < max;j++) {
						curTool.tVoxSet(vMap.XCYCL(x + xx[j]), vMap.YCYCL(y + yy[j]), 0, 0);
					}
				}
			}
			char typeChanges=vrtMap::TypeCh_Texture;
			if(drawRegionAndImpassability(x, y, radius))
				typeChanges|=vrtMap::TypeCh_Region;
			vMap.recalcArea2Grid( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius) );
			vMap.regRender( vMap.XCYCL(x-radius), vMap.YCYCL(y-radius), vMap.XCYCL(x+radius), vMap.YCYCL(y+radius), typeChanges );
			prevPos=position;
		}
		return true;
	}
	else if(drawMetod==DM_Bitmap){ ////////////////////////////////////////
		TerToolsDispatcher::Bitmap8C* pbmp=terToolsDispatcher.getBitmap8C(terTexture.c_str(), terTextureKScale);
		if(!pbmp) return false;
		if(pbmp->pAlfaLayer==0) {
			xassert(0 && "bitmap track Alfa not present!");
			return false; //Не может быть без альфы следа по битмапу
		}
		if(moveMetod==MM_Uninterrupted){ //Непрерывный след
			//минимальный шаг при повороте
			if(position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x/2) ) 
				return true;
			bool flag_changeOrientation;//=position.rot()!=prevPos.rot();
			QuatF t=position.rot()-prevPos.rot();
			if(t.norm2() < sqr(0.1f) ) flag_changeOrientation=false;
			else flag_changeOrientation=true;
			//flag_changeOrientation=false;
			//flag_uninterruptedTrack=false;
			if(!flag_changeOrientation && position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x)) 
				return true;
			if(!flag_changeOrientation) {
				Vect3f c;
				c=position.trans()-prevPos.trans();
				c.Normalize();
				c*=pbmp->size.x;
				c+=prevPos.trans();
				position.trans()=c;
			}
		}
		else if(moveMetod==MM_ChangePosition){ //для следов работающих по анимации
			if(position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x/2) )
				return true;
		}
		else if(moveMetod==MM_Stepwise){ //для следов у которых задается шаг
			if(position.trans().distance2(prevPos.trans()) < sqr(curstep) ) 
				return true;
			//const float minstep=step-0.5f*deltaStep;
			Vect3f c;
			c=position.trans()-prevPos.trans();
			c.Normalize();
			c*=curstep;
			c+=prevPos.trans();
			position.trans()=c;

			curstep = pbmp->size.x*(step + logicRNDfrnd(deltaStep));
		}
		else //не подерживаемые режимы
			return false;
		float sx2=(pbmp->size.y-1)/2.f;
		float sy2=(pbmp->size.x-1)/2.f;
		Vect3f dxy1(-sx2, -sy2,0);
		Vect3f dxy2(sx2, -sy2,0);
		Vect3f dxy3(sx2, sy2,0);
		Vect3f dxy4(-sx2, sy2,0);
		dxy1=position.rot().xform(dxy1);
		dxy2=position.rot().xform(dxy2);
		dxy3=position.rot().xform(dxy3);
		dxy4=position.rot().xform(dxy4);
		Vertex2i p[4]= {
			Vertex2i(round((1<<16)*(position.trans().x+dxy1.x)), round((1<<16)*(position.trans().y+dxy1.y)), 0, 0),
			Vertex2i(round((1<<16)*(position.trans().x+dxy2.x)), round((1<<16)*(position.trans().y+dxy2.y)), 0, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy3.x)), round((1<<16)*(position.trans().y+dxy3.y)), (pbmp->size.x-1)<<16, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy4.x)), round((1<<16)*(position.trans().y+dxy4.y)), (pbmp->size.x-1)<<16, 0)
		};
		draw4SidePolygon<TerToolsDispatcher::Bitmap8C, setingIndsMetod>(p, pbmp);
		int dx05=(p[1].x-p[0].x)/2;
		int dy05=(p[1].y-p[0].y)/2;
		char typeChanges=vrtMap::TypeCh_Texture;
		if(drawRegionAndImpassability(p[0].x+dx05, p[0].y+dy05, min(dx05,dy05)))
			typeChanges|=vrtMap::TypeCh_Region;
		vMap.recalcArea2Grid(p[0].x, p[0].y, p[1].x, p[1].y);
		vMap.regRender(p[0].x, p[0].y, p[1].x, p[1].y, typeChanges);
		prevPos=position;
		return true;
	}
	else if(drawMetod==DM_ReliefBitmapPressIn || drawMetod==DM_ReliefBitmapSwellOut){
		TerToolsDispatcher::Bitmap8V* pbmp=terToolsDispatcher.getBitmap8V(terTexture.c_str(), 1.f);
		if(!pbmp) 
			return false;
		if(moveMetod==MM_Uninterrupted){ //Непрерывный след
			//минимальный шаг при повороте
			if(position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x*terTextureKScale/2) ) 
				return true;
			bool flag_changeOrientation;
			QuatF t=position.rot()-prevPos.rot();
			if(t.norm2() < sqr(0.1f) ) flag_changeOrientation=false;
			else flag_changeOrientation=true;
			if(!flag_changeOrientation && position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x*terTextureKScale)) 
				return true;
			if(!flag_changeOrientation) {
				Vect3f c;
				c=position.trans()-prevPos.trans();
				c.Normalize();
				c*=pbmp->size.x*terTextureKScale;
				c+=prevPos.trans();
				position.trans()=c;
			}
		}
		else if(moveMetod==MM_ChangePosition){ //для следов работающих по анимации
			if(position.trans().distance2(prevPos.trans()) < sqr(pbmp->size.x*terTextureKScale/2) )
				return true;
		}
		else if(moveMetod==MM_Stepwise){ //для следов у которых задается шаг
			if(position.trans().distance2(prevPos.trans()) < sqr(curstep) )
				return true;
			//const float minstep=step-0.5f*deltaStep;
			Vect3f c;
			c=position.trans()-prevPos.trans();
			c.Normalize();
			c*=curstep;
			c+=prevPos.trans();
			position.trans()=c;

			curstep = pbmp->size.x*terTextureKScale*(step + logicRNDfrnd(deltaStep));
		}
		else //не подерживаемые режимы
			return false;

		float sx2=(pbmp->size.y*terTextureKScale-1)/2.f;
		float sy2=(pbmp->size.x*terTextureKScale-1)/2.f;
		Vect3f dxy1(-sx2, -sy2,0);
		Vect3f dxy2(sx2, -sy2,0);
		Vect3f dxy3(sx2, sy2,0);
		Vect3f dxy4(-sx2, sy2,0);
		dxy1=position.rot().xform(dxy1);
		dxy2=position.rot().xform(dxy2);
		dxy3=position.rot().xform(dxy3);
		dxy4=position.rot().xform(dxy4);
		Vertex2i p[4]= {
			//Vertex2i(round((1<<16)*(position.trans().x+dxy1.x)), round((1<<16)*(position.trans().y+dxy1.y)), 0, 0),
			//Vertex2i(round((1<<16)*(position.trans().x+dxy2.x)), round((1<<16)*(position.trans().y+dxy2.y)), 0, (pbmp->size.y-1)<<16),
			//Vertex2i(round((1<<16)*(position.trans().x+dxy3.x)), round((1<<16)*(position.trans().y+dxy3.y)), (pbmp->size.x-1)<<16, (pbmp->size.y-1)<<16),
			//Vertex2i(round((1<<16)*(position.trans().x+dxy4.x)), round((1<<16)*(position.trans().y+dxy4.y)), (pbmp->size.x-1)<<16, 0)
			Vertex2i(round((1<<16)*(position.trans().x+dxy1.x)), round((1<<16)*(position.trans().y+dxy1.y)), 0, 0),
			Vertex2i(round((1<<16)*(position.trans().x+dxy2.x)), round((1<<16)*(position.trans().y+dxy2.y)), 0, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy3.x)), round((1<<16)*(position.trans().y+dxy3.y)), (pbmp->size.x-1)<<16, (pbmp->size.y-1)<<16),
			Vertex2i(round((1<<16)*(position.trans().x+dxy4.x)), round((1<<16)*(position.trans().y+dxy4.y)), (pbmp->size.x-1)<<16, 0)
		};
		draw4SidePolygon<TerToolsDispatcher::Bitmap8V,setingIndsMetod>(p, pbmp);
		int dx05=(p[1].x-p[0].x)/2;
		int dy05=(p[1].y-p[0].y)/2;
		char typeChanges=vrtMap::TypeCh_Height;
		if(drawRegionAndImpassability(p[0].x+dx05, p[0].y+dy05, min(dx05,dy05)))
			typeChanges|=vrtMap::TypeCh_Region;
		vMap.recalcArea2Grid(p[0].x, p[0].y, p[1].x, p[1].y);
		vMap.regRender(p[0].x, p[0].y, p[1].x, p[1].y, typeChanges);
		prevPos=position;

		return true;

	}
	else //не поддерживаемый тип метода
		return false;

	return true;
}

#undef roundFIntF0
#undef mroundFIntF0
#undef cvrtFIntF8
#undef ceilFIntF16
#undef ceilFIntF0
#undef floorFIntF0
#undef floorFIntF16
#define roundFIntF0(a) ((a)>>16)
#define mroundFIntF0(a) (((a)+(1<<15))>>16)
#define cvrtFIntF8(a) ((a+(1<<7))>>8)
#define ceilFIntF16(a) (((a)+0xffFF)&0xFFff0000)
#define ceilFIntF0(a) (((a)+0xffFF)>>16)
#define floorFIntF0(a) ((a)>>16)
#define floorFIntF16(a) ((a)&0xFFff0000)

//#undef SUBPIXEL
//#undef SUBTEXEL
//#define SUBPIXEL
//#define SUBTEXEL


template<class Bitmap8, eSetingAtrMetod setingIndsMetod>
void TerToolTextureBase::draw4SidePolygon(Vertex2i point[4], Bitmap8* pBmp, sBitMap8* pBackupBmp) //TerToolsDispatcher::Bitmap8C* pBmp)
{
	point[0].y=clamp(point[0].y, 0, (vMap.V_SIZE-1)<<16);
	point[1].y=clamp(point[1].y, 0, (vMap.V_SIZE-1)<<16);
	point[2].y=clamp(point[2].y, 0, (vMap.V_SIZE-1)<<16);
	point[3].y=clamp(point[3].y, 0, (vMap.V_SIZE-1)<<16);
	int curMinX,curMaxX;
	curMinX=curMaxX=point[0].x;
	if(curMinX > point[1].x)curMinX=point[1].x;
	if(curMaxX < point[1].x)curMaxX=point[1].x;
	if(curMinX > point[2].x)curMinX=point[2].x;
	if(curMaxX < point[2].x)curMaxX=point[2].x;
	if(curMinX > point[3].x)curMinX=point[3].x;
	if(curMaxX < point[3].x)curMaxX=point[3].x;
	curMinX=clamp(curMinX, 0, (vMap.H_SIZE-1)<<16);
	curMaxX=clamp(curMaxX, 0, (vMap.H_SIZE-1)<<16);
	
	Vertex2i* a=&point[0];
	Vertex2i* b=&point[1];
	Vertex2i* c=&point[2];
	Vertex2i* d=&point[3];
	if(a->y > b->y) swap(a, b);
	if(a->y > c->y) swap(a, c);
	if(a->y > d->y) swap(a, d);
	if(b->y > c->y) swap(b, c);
	if(b->y > d->y) swap(b, d);
	if(c->y > d->y) swap(c, d);
	if(pBackupBmp){
		int cminx=ceilFIntF0(curMinX);
		int cmaxx=ceilFIntF0(curMaxX);
		int sx = cmaxx - cminx + 1;
		int cminy=ceilFIntF0(a->y)-1; cminy=max(cminy, 0);
		int cmaxy=ceilFIntF0(d->y)+1; cmaxy=min(cmaxy, vMap.V_SIZE-1);
		int sy= cmaxy - cminy +1;
		xassert(sx>=0 && sy>=0);
		pBackupBmp->create(sx, sy);
		pBackupBmp->mX=cminx;
		pBackupBmp->mY=cminy;
		int cnt=0;
		for(int i=0; i<sy; i++){
			int off=vMap.offsetBuf(0, cminy+i);
			for(int j=0; j<sx; j++)
				pBackupBmp->data[cnt++]=vMap.SurBuf[off + cminx +j];
		}
	}
	bool flag_Right;
	if(a->x < b->x)  flag_Right=true;
	else flag_Right=false;
	if(c->y==d->y) {
		if( c->x > d->x){
			if(flag_Right) swap(c, d);
		}
		else if(!flag_Right) swap(c, d);
	}

	int divisor, k;
	divisor=(c->y - a->y);
	if(roundFIntF0(divisor)) k = ((__int64)(b->y - a->y)<<8) / divisor;// F8
	else k=0;
	int x_start = a->x + (cvrtFIntF8(c->x - a->x))*(k);
	int u_start = a->u + (cvrtFIntF8(c->u - a->u))*(k);
	int v_start = a->v + (cvrtFIntF8(c->v - a->v))*(k);
	int x_end = b->x;
	int u_end = b->u;
	int v_end = b->v;

	int du,dv;
	divisor= x_start - x_end;
	if(roundFIntF0(divisor)) {
		du = (((__int64)(u_start - u_end)<<16)/divisor);
		dv = (((__int64)(v_start - v_end)<<16)/divisor);
	}
	else { du=0; dv=0; }

//	int tmp;
	int dx_start ,du_start, dv_start;
	int dx_end, du_end, dv_end;
	x_start = a->x;
	u_start = a->u;
	v_start = a->v;
	divisor=(c->y - a->y);
	if(roundFIntF0(divisor)){
		dx_start = ((__int64)(c->x - a->x)<<16) / divisor; 
		du_start = ((__int64)(c->u - a->u)<<16) / divisor; 
		dv_start = ((__int64)(c->v - a->v)<<16) / divisor; 
	}
	else { dx_start=0; du_start=0; dv_start=0; }
//#ifdef SUBPIXEL
	//tmp = ceilFIntF16(a->y) - a->y;
	//x_start += (dx_start>>8) * (tmp>>8); //Норма
	//u_start += (du_start>>8) * (tmp>>8); //Норма
	//v_start += (dv_start>>8) * (tmp>>8); //Норма
//#endif
	//int current_sy=ceilFIntF0(a->y);
	int current_sy=mroundFIntF0(a->y);
	if(mroundFIntF0(b->y) > current_sy) {
		x_end = a->x;
		u_end = a->u;
		v_end = a->v;
		divisor=b->y - a->y;
		if(roundFIntF0(divisor)){ 
			dx_end = ((__int64)(b->x - a->x)<<16) / divisor;
			du_end = ((__int64)(b->u - a->u)<<16) / divisor;
			dv_end = ((__int64)(b->v - a->v)<<16) / divisor;
		}
		else {dx_end =0; du_end=0; dv_end=0; }
//#ifdef SUBPIXEL
		//tmp = ceilFIntF16(a->y) - (a->y);
		//x_end += (dx_end>>8) * (tmp>>8); //Норма
		//u_end += (du_end>>8) * (tmp>>8); //Норма
		//v_end += (dv_end>>8) * (tmp>>8); //Норма
//#endif
		// построчная отрисовка грани
		do {
			// x_start должен находиться левее x_end
			x_start=clamp(x_start, curMinX, curMaxX);
			x_end=clamp(x_end, curMinX, curMaxX);
			int x,u,v,length;
			int xe;
			if (x_start > x_end) {
				x = x_end;
				u = u_end;
				v = v_end;
				length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
				xe = ceilFIntF0(x_start);
			}else {
				x = x_start;
				u = u_start;
				v = v_start;
				length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
				xe = ceilFIntF0(x_end);
			}
			// текстурируем строку
			int current_sx = ceilFIntF0(x);
			if((current_sy) >= 0 ) if (length) {
	//#ifdef SUBTEXEL
				//tmp = ceilFIntF16(x) - x;
				//u += (du>>8)* (tmp>>8);
				//v += (dv>>8)* (tmp>>8);
	//#endif
				//while (length--) {
				//	drawPixel(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
				//	u += du;
				//	v += dv;
				//	current_sx++;
				//}
				for(current_sx=ceilFIntF0(x); current_sx<=xe; current_sx++){
					drawPixel<setingIndsMetod>(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
					u += du;
					v += dv;
				}
			}
			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			x_end += dx_end;
			u_start += du_start;
			v_start += dv_start;
			u_end += du_end;
			v_end += dv_end;
			current_sy++;
		}while(current_sy < mroundFIntF0(b->y));
	}
	///////////////////////////////////////
	x_end = b->x;
	u_end = b->u;
	v_end = b->v;
	divisor = d->y - b->y;
	if(roundFIntF0(divisor)){
		dx_end = ((__int64)(d->x - b->x)<<16) / divisor;
		du_end = ((__int64)(d->u - b->u)<<16) / divisor;
		dv_end = ((__int64)(d->v - b->v)<<16) / divisor;
	}
	else{ dx_end=0; du_end=0; dv_end=0; }
//#ifdef SUBPIXEL
	//tmp = ceilFIntF16(b->y) - b->y;
	//x_end += (dx_end>>8) * (tmp>>8); //Норма
	//u_end += (du_end>>8) * (tmp>>8); //Норма
	//v_end += (dv_end>>8) * (tmp>>8); //Норма
//#endif
	if(mroundFIntF0(c->y) > current_sy) {
		// построчная отрисовка грани
		do {
			//if((current_sy) >= imaxY ) break;
			// x_start должен находиться левее x_end
			x_start=clamp(x_start, curMinX, curMaxX);
			x_end=clamp(x_end, curMinX, curMaxX);
			int x,u,v,length;
			int xe;
			if (x_start > x_end) {
				x = x_end;
				u = u_end;
				v = v_end;
				length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
				xe = ceilFIntF0(x_start);
			}else {
				x = x_start;
				u = u_start;
				v = v_start;
				length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
				xe = ceilFIntF0(x_end);
			}
			// текстурируем строку
			int current_sx = ceilFIntF0(x);
			if((current_sy) >= 0 ) if (length) {
	//#ifdef SUBTEXEL
				//tmp = ceilFIntF16(x) - x;
				//u += (du>>8)* (tmp>>8);
				//v += (dv>>8)* (tmp>>8);
	//#endif
				//while (length--) {
				//	drawPixel(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
				//	u += du;
				//	v += dv;
				//	current_sx++;
				//}
				for(current_sx=ceilFIntF0(x); current_sx<=xe; current_sx++){
					drawPixel<setingIndsMetod>(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
					u += du;
					v += dv;
				}
			}
			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			x_end += dx_end;
			u_start += du_start;
			v_start += dv_start;
			u_end += du_end;
			v_end += dv_end;
			current_sy++;
		}while(current_sy < mroundFIntF0(c->y));
	}
	if(mroundFIntF0(d->y) >= current_sy) {
		x_start = c->x; /// обязательно???
		u_start = c->u;
		v_start = c->v;
		divisor=(d->y - c->y);
		if(roundFIntF0(divisor)){
			dx_start = ((__int64)(d->x - c->x)<<16) / divisor; 
			du_start = ((__int64)(d->u - c->u)<<16) / divisor; 
			dv_start = ((__int64)(d->v - c->v)<<16) / divisor; 
		}
		else { dx_start=0; du_start=0; dv_start=0; }
	//#ifdef SUBPIXEL
		//tmp = ceilFIntF16(c->y) - c->y;
		//x_start += (dx_start>>8) * (tmp>>8); //Норма
		//u_start += (du_start>>8) * (tmp>>8); //Норма
		//v_start += (dv_start>>8) * (tmp>>8); //Норма
	//#endif
		// построчная отрисовка грани
		do {
			// x_start должен находиться левее x_end
			x_start=clamp(x_start, curMinX, curMaxX);
			x_end=clamp(x_end, curMinX, curMaxX);
			int x,u,v,length;
			int xe;
			if (x_start > x_end) {
				x = x_end;
				u = u_end;
				v = v_end;
				length = ceilFIntF0(x_start) - ceilFIntF0(x_end);
				xe = ceilFIntF0(x_start);
			}else {
				x = x_start;
				u = u_start;
				v = v_start;
				length = ceilFIntF0(x_end) - ceilFIntF0(x_start);
				xe = ceilFIntF0(x_end);
			}
			// текстурируем строку
			int current_sx = ceilFIntF0(x);
			if((current_sy) >= 0 ) if (length) {
	//#ifdef SUBTEXEL
				//tmp = ceilFIntF16(x) - x;
				//u += (du>>8)* (tmp>>8);
				//v += (dv>>8)* (tmp>>8);
	//#endif
				//while (length--) {
				//	drawPixel(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
				//	u += du;
				//	v += dv;
				//	current_sx++;
				//}
				for(current_sx=ceilFIntF0(x); current_sx<=xe; current_sx++){
					drawPixel<setingIndsMetod>(vMap.offsetBuf(current_sx,current_sy), u, v, pBmp);
					u += du;
					v += dv;
				}
			}
			// сдвигаем начальные и конечные значения x/u/v/(1/z)
			x_start += dx_start;
			x_end += dx_end;
			u_start += du_start;
			v_start += dv_start;
			u_end += du_end;
			v_end += dv_end;
			current_sy++;
		}while(current_sy <= mroundFIntF0(d->y));
	}
	//int minx=ceilFIntF0(curMinX);
	//int maxx=ceilFIntF0(curMaxX);
	//int miny=ceilFIntF0(a->y);
	//int maxy=ceilFIntF0(d->y);
	//int dx05=(maxx-minx)/2;
	//int dy05=(maxy-miny)/2;
	//drawRegionAndImpassability(minx+dx05, miny+dy05, min(dx05,dy05));
	//vMap.recalcArea2Grid(minx, miny, maxx, maxy);
	//vMap.regRender(minx, miny, maxx, maxy);
	point[0].x=ceilFIntF0(curMinX);
	point[1].x=ceilFIntF0(curMaxX);
	//a и d указатели ! на point[]
	int ominY=ceilFIntF0(a->y);
	int omaxY=ceilFIntF0(d->y);
	point[0].y=ominY;
	point[1].y=omaxY;
}

template <eSetingAtrMetod _TT_SETING_ATR_METOD_> 
void TerToolTextureBase::drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8C* pBmp)
{
	if(u <0 ) u=0; if(v <0 ) v=0;
	unsigned int textureOff=(ceilFIntF0(u))%pBmp->size.x+(ceilFIntF0(v))%pBmp->size.y*pBmp->size.x;
	//short cu=ceilFIntF0(u);
	//short cuc=ceilFIntF0(u)%pBmp->size.x;
	//vMap.SurBuf[bufoff]= pBmp->pBitmap4Dam[offBitmap];

	short alfa=255 - pBmp->pAlfaLayer[textureOff];
	alfa/=43;
	alfa-=1;
/*	unsigned char sur;
	if(Vm_IsGeo(vMap.VxABuf[vBufOff])){
		sur=pBmp->pBitmap4Geo[textureOff];
		if(alfa>3)
			sur=vMap.SurBuf[vBufOff];
		else if(alfa>=0)
			sur=*((unsigned char*)(&terToolsDispatcher.geoAlfaConversion[vMap.SurBuf[vBufOff]][sur]) + alfa);
		//vMap.SurBuf[vBufOff]=sur;
	}
	else {
		sur=pBmp->pBitmap4Dam[textureOff];
		if(alfa>3)
			sur=vMap.SurBuf[vBufOff];
		else if(alfa>=0)
			sur=*((unsigned char*)(&terToolsDispatcher.damAlfaConversion[vMap.SurBuf[vBufOff]][sur]) + alfa);
		//vMap.SurBuf[vBufOff]=sur;
	}
	switch(_TT_SETING_ATR_METOD_){
	case SIM_SetInds:				vMap.PutSurAndSetInds(vBufOff, sur);	break;
	case SIM_UnSetInds:				vMap.PutSurAndUnInds(vBufOff, sur);	break;
	case SIM_NotChange:default:	vMap.PutSurAndNoChange(vBufOff, sur);	break;
	}*/

	if(Vm_IsGeo(vMap.VxABuf[vBufOff])){ //current Geo
		if(_TT_SETING_ATR_METOD_==SIM_SetDam){
			vMap.PutSurAndSetDam(vBufOff, pBmp->pBitmap4Dam[textureOff]);
		}
		else {
			unsigned char sur=pBmp->pBitmap4Geo[textureOff];
			if(alfa>3)
				sur=vMap.SurBuf[vBufOff];
			else if(alfa>=0)
				sur=*((unsigned char*)(&terToolsDispatcher.geoAlfaConversion[vMap.SurBuf[vBufOff]][sur]) + alfa);
			switch(_TT_SETING_ATR_METOD_){
			case SIM_SetInds:			vMap.PutSurAndSetInds(vBufOff, sur);break;
			case SIM_UnSetInds:			vMap.PutSurAndUnInds(vBufOff, sur);	break;
			case SIM_SetGeo:			vMap.SurBuf[vBufOff]=sur;		break;
			case SIM_NotChange:default:	vMap.SurBuf[vBufOff]=sur; 		break;
			}
		}
	}
	else { // current Dam
		if(_TT_SETING_ATR_METOD_==SIM_SetGeo){
			vMap.PutAltAndSetGeo(vBufOff, v, vMap.GetGeoType(vBufOff,v));
		}
		else {
			unsigned char sur=pBmp->pBitmap4Dam[textureOff];
			if(alfa>3)
				sur=vMap.SurBuf[vBufOff];
			else if(alfa>=0)
				sur=*((unsigned char*)(&terToolsDispatcher.damAlfaConversion[vMap.SurBuf[vBufOff]][sur]) + alfa);
			vMap.SurBuf[vBufOff]=sur;
			switch(_TT_SETING_ATR_METOD_){
			case SIM_SetInds:			vMap.PutSurAndSetInds(vBufOff, sur);break;
			case SIM_UnSetInds:			vMap.PutSurAndUnInds(vBufOff, sur);	break;
			case SIM_SetDam:			vMap.SurBuf[vBufOff]=sur;		break;
			case SIM_NotChange:default:	vMap.SurBuf[vBufOff]=sur; 		break;
			}
		}
	}

}

template <eSetingAtrMetod _TT_SETING_ATR_METOD_>
void TerToolTextureBase::drawPixel(unsigned int vBufOff, int u, int v, TerToolsDispatcher::Bitmap8V* pBmp)
{
	//if(u <0 ) u=0; if(v <0 ) v=0;
	//unsigned int textureOff=(ceilFIntF0(u))%pBmp->size.x+(ceilFIntF0(v))%pBmp->size.y*pBmp->size.x;
	//unsigned int textureOff=clamp(ceilFIntF0(u),0,pBmp->maxCoord.x) + clamp(ceilFIntF0(v),0,pBmp->maxCoord.y)*pBmp->size.x;
	//u=ceilFIntF0(u)%pBmp->size.x;
	//v=ceilFIntF0(v)%pBmp->size.y;
	//if(u <0 ) u+=pBmp->size.x; if(v <0 ) v+=pBmp->size.y;
	//unsigned int textureOff=u + v*pBmp->size.x;
	//int dv=pBmp->pBitmapV[textureOff];
	int du=u-floorFIntF16(u);
	int dv=v-floorFIntF16(v);
	u=floorFIntF0(u)%pBmp->size.x;
	v=floorFIntF0(v)%pBmp->size.y;
	if(u <0 ) u+=pBmp->size.x; if(v <0 ) v+=pBmp->size.y;
	int u1=(u+1)%pBmp->size.x;
	int v1=(v+1)%pBmp->size.y;
	v*=pBmp->size.x;
	v1*=pBmp->size.x;
	int c0=pBmp->pBitmapV[u +v ] + ((pBmp->pBitmapV[u1+v ] - pBmp->pBitmapV[u +v ])*du>>16);
	int c1=pBmp->pBitmapV[u +v1] + ((pBmp->pBitmapV[u1+v1] - pBmp->pBitmapV[u +v1 ])*du>>16);
	short ov=c0+((c1-c0)*dv>>16);
	if(drawMetod==DM_ReliefBitmapPressIn){
		switch(_TT_SETING_ATR_METOD_){
		case SIM_SetInds:				vMap.DigAndSetInds(vBufOff, ov);	break;
		case SIM_UnSetInds:				vMap.DigAndUnInds(vBufOff, ov);	break;
		case SIM_SetGeo:			vMap.PutAltAndSetGeo(vBufOff, ov);	break;
		case SIM_SetDam:			vMap.PutAlt(vBufOff, ov);			break;//!!! не может быть установка Dam без текстуры!
		case SIM_NotChange:default:	vMap.DigAndNoChange(vBufOff, ov);	break;
		}
	}
	else { // DM_ReliefBitmapSwellOut
		switch(_TT_SETING_ATR_METOD_){
		case SIM_SetInds:				vMap.PutAndSetInds(vBufOff, ov);	break;
		case SIM_UnSetInds:				vMap.PutAndUnInds(vBufOff, ov);	break;
		case SIM_SetGeo:			vMap.PutAltAndSetGeo(vBufOff, ov);	break;
		case SIM_SetDam:			vMap.PutAlt(vBufOff, ov);			break;//!!! не может быть установка Dam без текстуры!
		case SIM_NotChange:default:	vMap.PutAndNoChange(vBufOff, ov);	break;
		}
	}
}



void TerToolTextureTrack::serialize(Archive& ar)
{
	static ResourceSelector::Options options("*.tga", ".\\RESOURCE\\TerrainData\\TerTools", "Will select location of texture file");
	ar.serialize(ResourceSelector(terTexture, options), "terTexture", "тертекстура");
	ar.serialize(terTextureKScale, "terTextureKScale", "масштаб тертекстуры");
	ar.serialize(drawMetod, "drawMetod", "Метод отрисовки");
	ar.serialize(r, "r", "радиус");
	ar.serialize(moveMetod, "moveMetod", "метод перемещения");
	ar.serialize(step, "step", "шаг");
	ar.serialize(deltaStep, "deltaStep", "относительная дельта шага");
	ar.serialize(setingAtrMetod_, "setingAtrMetod_", "Изменение типа поверхности");
	ar.serialize(settingImpassabilityMetod_, "settingImpassabilityMetod_", "Непроходимость");
	ar.serialize(settingSurfaceKindMetod_, "settingSurfaceKindMetod_", "Поверхность");
	ar.serialize(detaledTextueType_, "detaledTextueType_", "Мелкодетальная текстура");
	ar.serialize(detaledTextueScale_, "detaledTextueScale_", "Масштаб мелкодетальной текстуры");
	if(r<=0) r=1;
}

///////////////////////////////////////////////////////////////////////////////////
TerToolCtrl::TerToolCtrl()
{
	terToolNrmlID=TerToolsDispatcher::NOT_TERTOOL_ID;
	terToolIndsID=TerToolsDispatcher::NOT_TERTOOL_ID;
	scaleFactor=1.f;
	currentScaleFactor=1.f;
}
TerToolCtrl::~TerToolCtrl()
{
	stop();
}

void TerToolCtrl::serialize(Archive& ar)
{
	ar.serialize(terToolReference, "terToolReference", "&Тертулзер");
	//if( !ar.isEdit() || (terToolReference && terToolReference->isPermitChangeModel()) )
	//	ar.serialize(ModelSelector(modelName), "modelName", "Имя модели");
	ar.serialize(scaleFactor, "scaleFactor", "&Масштаб");
	if(!ar.isEdit()) {
		ar.serialize(currentScaleFactor, "currentScaleFactor", 0);
	}
}

void TerToolCtrl::start(const Se3f& pos, float _scaleFactor)
{
	stop();
	currentScaleFactor=scaleFactor*_scaleFactor;
	if(isEmpty()) return;
	setPosition(pos);
}

void TerToolCtrl::stop() 
{
	if(terToolNrmlID!=TerToolsDispatcher::NOT_TERTOOL_ID){
		terToolsDispatcher.stopTerTool(terToolNrmlID);
		terToolNrmlID=TerToolsDispatcher::NOT_TERTOOL_ID;
	}
	if(terToolIndsID!=TerToolsDispatcher::NOT_TERTOOL_ID){
		terToolsDispatcher.stopTerTool(terToolIndsID);
		terToolIndsID=TerToolsDispatcher::NOT_TERTOOL_ID;
	}
}


void TerToolCtrl::setPosition(const Se3f& pos)
{
	if(isEmpty())
		return;
	int xg=round(pos.trans().x)>>kmGrid;
	int yg=round(pos.trans().y)>>kmGrid;
	char surkind=vMap.getSurKindG(xg, yg);
	xassert(surkind>=0 && surkind<TERRAIN_TYPES_NUMBER);
	if(!vMap.isIndestructabilityG( xg, yg )){
		if(terToolNrmlID==TerToolsDispatcher::NOT_TERTOOL_ID && terToolReference->terToolNrml){
			TerToolBase* pTTB=terToolReference->terToolNrml->clone();
			pTTB->setPosition(pos,true);
			if(pTTB->isExternalScalable())
				pTTB->setScale(currentScaleFactor);
			else
				pTTB->setScale(scaleFactor);
			pTTB->stopOnInds(true);
			terToolNrmlID=terToolsDispatcher.startTerTool(pTTB);
		}
		if(terToolReference->interplayArr[surkind]==true)
            terToolsDispatcher.pauseTerTool(terToolNrmlID, false);
		else 
            terToolsDispatcher.pauseTerTool(terToolNrmlID, true);
		if(terToolIndsID!=TerToolsDispatcher::NOT_TERTOOL_ID)
			terToolsDispatcher.pauseTerTool(terToolIndsID, true);
	}
	else {
		if(terToolIndsID==TerToolsDispatcher::NOT_TERTOOL_ID && terToolReference->terToolInds){
			TerToolBase* pTTB=terToolReference->terToolInds->clone();
			pTTB->setPosition(pos,true);
			if(pTTB->isExternalScalable())
				pTTB->setScale(currentScaleFactor);
			else
				pTTB->setScale(scaleFactor);
			terToolIndsID=terToolsDispatcher.startTerTool(pTTB);
		}
		if(terToolReference->interplayArr[surkind]==true)
            terToolsDispatcher.pauseTerTool(terToolIndsID, false);
		else 
            terToolsDispatcher.pauseTerTool(terToolIndsID, true);
		if(terToolNrmlID!=TerToolsDispatcher::NOT_TERTOOL_ID)
			terToolsDispatcher.pauseTerTool(terToolNrmlID, true);
	}

	if(terToolNrmlID!=TerToolsDispatcher::NOT_TERTOOL_ID)
		terToolsDispatcher.setPosition(terToolNrmlID, pos);
	if(terToolIndsID!=TerToolsDispatcher::NOT_TERTOOL_ID)
		terToolsDispatcher.setPosition(terToolIndsID, pos);
}

///////////////////////////////////////
void TerToolLibElement::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);
	if(ar.isInput() && !ar.isEdit()){ // !!! CONVERSION 26.04.06
		string name_;
		if(ar.serialize(name_, "name_", 0))
			setName(name_.c_str());
	}


	ar.serialize(terToolNrml, "terToolNrml", "&Тертулзер для обычной поверхности");
	ar.serialize(terToolInds, "terToolInds", "&Тертулзер для неразрушаемой поверхности");
	ar.serializeArray(interplayArr, "interplayArr", "Типы поверхности для взаимодействия");
}

