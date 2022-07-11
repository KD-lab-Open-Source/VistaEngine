#include "stdafxTr.h"
#include "terTools.h"
#include "tgai.h"
#include "scalingEngine.h"

void vrtMap::wrldShotMapHeight()
{
	static int cnt = 0;
	const int SX = H_SIZE;
	const int SY = V_SIZE;
//TGA write
	TGAHEAD thead;
	XBuffer buf;
	//buf < "Ph_A_" <= cnt++ < ".tga";
	buf < worldName.c_str() < "_MH_" <= cnt++ < ".tga";
	XStream ff(buf, XS_OUT);
	thead.PixelDepth=8;
	thead.ImageType=3;
	thead.Width=(short)SX;
	thead.Height=(short)SY;
	ff.write(&thead,sizeof(thead));

	unsigned char* line = new unsigned char[SX],*p;

	register unsigned int i,j;
	for(j = 0; j<V_SIZE; j++){
		p = line;
		for(i = 0; i<H_SIZE; i++){

			int offb=offsetBuf(i,j);
			*p++=getAlt(offb)>>VX_FRACTION;//1;//
		}
		ff.write(line,SX);
	}
	ff.close();
	delete line;
}

void vrtMap::FlipWorldH()
{
	register unsigned int i,j;
	int DY=V_SIZE;
	int DX=H_SIZE>>1;
	int MX=H_SIZE-1;
	for(j = 0; j<DY; j++){
		for(i = 0; i<DX; i++){
			int off1=offsetBuf(i, j);
			int off2=offsetBuf(MX-i, j);
			TerrainColor t8;
			unsigned short t16;
			unsigned long t32;
			t16=vxaBuf[off1]; vxaBuf[off1]=vxaBuf[off2]; vxaBuf[off2]=t16;
			t8=clrBuf[off1]; clrBuf[off1]=clrBuf[off2]; clrBuf[off2]=t8;
			t32=supBuf[off1]; supBuf[off1]=supBuf[off2]; supBuf[off2]=t32;
		}
	}
}

void vrtMap::FlipWorldV()
{
	register unsigned int i,j;
	int DY=V_SIZE>>1;
	int DX=H_SIZE;
	int MY=V_SIZE-1;
	for(j = 0; j<DY; j++){
		for(i = 0; i<DX; i++){
			int off1=offsetBuf(i, j);
			int off2=offsetBuf(i, MY-j);
			TerrainColor t8;
			unsigned short t16;
			unsigned long t32;
			t16=vxaBuf[off1]; vxaBuf[off1]=vxaBuf[off2]; vxaBuf[off2]=t16;
			t8=clrBuf[off1]; clrBuf[off1]=clrBuf[off2]; clrBuf[off2]=t8;
			t32=supBuf[off1]; supBuf[off1]=supBuf[off2]; supBuf[off2]=t32;
		}
	}
}

void vrtMap::RotateWorldP90()
{
	register int i,j;
	//int DY=V_SIZE>>1;
	//int DX=H_SIZE-1;
	//int BEGI=0;
	//const int BORDX=H_SIZE-1;
	//const int BORDY=V_SIZE-1;
	//for(j = 0; j<DY; j++){
	//	for(i = BEGI; i<DX; i++){
	//		int off1=offsetBuf(i, j);
	//		int off2=offsetBuf(BORDX-j, i);
	//		int off3=offsetBuf(BORDX-i, BORDY-j);
	//		int off4=offsetBuf(j, BORDY-i);
	//		unsigned char t8;
	//		unsigned short t16;
	//		unsigned long t32;
	//		t16=vxaBuf[off1]; vxaBuf[off1]=vxaBuf[off4]; 
	//		vxaBuf[off4]=vxaBuf[off3]; vxaBuf[off3]=vxaBuf[off2]; vxaBuf[off2]=t16;

	//		t8=SurBuf[off1]; SurBuf[off1]=SurBuf[off4]; 
	//		SurBuf[off4]=SurBuf[off3]; SurBuf[off3]=SurBuf[off2]; SurBuf[off2]=t8;

	//		t32=supBuf[off1]; supBuf[off1]=supBuf[off4];
	//		supBuf[off4]=supBuf[off3]; supBuf[off3]=supBuf[off2]; supBuf[off2]=t32;
	//	}
	//	DX-=1; BEGI+=1;
	//}

	unsigned short* nVxABuf = new unsigned short [H_SIZE*V_SIZE];
	TerrainColor* nSurBuf = new TerrainColor [H_SIZE*V_SIZE];
    unsigned long* nSupBuf = new unsigned long [H_SIZE*V_SIZE];
	unsigned long cnt=0;
	for(j=0; j <H_SIZE; j++ ){
		for(i=V_SIZE-1; i>=0; i--){
			nVxABuf[cnt]=vxaBuf[offsetBuf(j,i)];
			nSurBuf[cnt]=clrBuf[offsetBuf(j,i)];
			nSupBuf[cnt]=supBuf[offsetBuf(j,i)];
			cnt++;
		}
	}
	swap(nVxABuf,vxaBuf);
	swap(nSurBuf,clrBuf);
	swap(nSupBuf,supBuf);
	swap(H_SIZE,V_SIZE);
	delete [] nVxABuf;
	delete [] nSurBuf;
	delete [] nSupBuf;

}

void vrtMap::RotateWorldM90()
{
	register unsigned int i,j;
	int DY=V_SIZE>>1;
	int DX=H_SIZE-1;
	int BEGI=0;
	const int BORDX=H_SIZE-1;
	const int BORDY=V_SIZE-1;
	for(j = 0; j<DY; j++){
		for(i = BEGI; i<DX; i++){
			int off1=offsetBuf(i, j);
			int off2=offsetBuf(BORDX-j, i);
			int off3=offsetBuf(BORDX-i, BORDY-j);
			int off4=offsetBuf(j, BORDY-i);
			TerrainColor t8;
			unsigned short t16;
			unsigned long t32;
			t16=vxaBuf[off1]; vxaBuf[off1]=vxaBuf[off2]; 
			vxaBuf[off2]=vxaBuf[off3]; vxaBuf[off3]=vxaBuf[off4]; vxaBuf[off4]=t16;

			t8=clrBuf[off1]; clrBuf[off1]=clrBuf[off2]; 
			clrBuf[off2]=clrBuf[off3]; clrBuf[off3]=clrBuf[off4]; clrBuf[off4]=t8;

			t32=supBuf[off1]; supBuf[off1]=supBuf[off2]; 
			supBuf[off2]=supBuf[off3]; supBuf[off3]=supBuf[off4]; supBuf[off4]=t32;
		}
		DX-=1; BEGI+=1;
	}
}


void vrtMap::scalingHeighMap(int percent)
{
	float k=(float)percent/100.f;
	static int cnt = 0;
	register unsigned int i,j;
	for(j = 0; j<V_SIZE; j++){
		for(i = 0; i<H_SIZE; i++){
			int offb=offsetBuf(i,j);
			short v=getAlt(offb);
			v=round(v*k);
			if(v>MAX_VX_HEIGHT)v=MAX_VX_HEIGHT; if(v<0)v=0;
			putAlt(offb, v);
		}
	}
}

float vrtMap::changeTotalWorldParam(int deltaVx, float kScale, const vrtMapChangeParam& changeParam)
{
	//find min height
	int minVx=INT_MAX;
	int maxVx=INT_MIN;
	int i, j;
	for(i=0; i<V_SIZE; i++){
		for(j=0; j<H_SIZE; j++){
			int h=getAlt(j,i);
			if(minVx>h) minVx=h;
			if(maxVx<h) maxVx=h;
		}
	}

	for(i=0; i<V_SIZE; i++){
		int offY=offsetBuf(0,i);
		for(j=0; j<H_SIZE; j++){
			int off=offY+j;
			int h=getAlt(off);
			h = deltaVx + minVx+ round((h-minVx)*kScale);
			h=clamp(h, 0, MAX_VX_HEIGHT);
			putAlt(off, h);
		}
	}
	//recalcArea2Grid(0, 0, H_SIZE-1, V_SIZE-1);
	//regRender(0, 0, H_SIZE-1, V_SIZE-1, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture|vrtMap::TypeCh_Region);

	int sizex = 1<<changeParam.H_SIZE_POWER;
	int sizey = 1<<changeParam.V_SIZE_POWER;
	unsigned short* tmpvx = new unsigned short[sizex*sizey];
	unsigned long* tmprgb = new unsigned long[sizex*sizey];
	unsigned short* tmpg = new unsigned short[(sizex>>kmGrid) * (sizey>>kmGrid)];

	if(changeParam.flag_resizeWorld2NewBorder){
		scaleVxImage(vxaBuf, Vect2s(H_SIZE, V_SIZE), tmpvx, Vect2s(sizex,sizey));
		scaleRGBAImage(supBuf, Vect2s(H_SIZE, V_SIZE), tmprgb, Vect2s(sizex,sizey));
		scaleGImage(gABuf, Vect2s(GH_SIZE, GV_SIZE), tmpg, Vect2s(sizex>>kmGrid, sizey>>kmGrid));
	}
	else {
		int cnt=0;
		for(i=0; i<sizey; i++){
			int curY=i-changeParam.oldWorldBegCoordY;
			for(j=0; j<sizex; j++){
				int curX=j-changeParam.oldWorldBegCoordX;
				if(curY<0  || curY>=V_SIZE || curX<0 || curX>=H_SIZE ){
					tmpvx[cnt]=Vm_prepHeighAtr(changeParam.initialHeight<<VX_FRACTION, VmAt_Nrml);
					tmprgb[cnt]=0x007f7f7f;
				}
				else{
					tmpvx[cnt]=vxaBuf[offsetBuf(curX, curY)];
					tmprgb[cnt]=supBuf[offsetBuf(curX, curY)];
				}
				cnt++;
			}
		}
	}

	static_cast<vrtMapCreationParam&>(*this) = changeParam;
	create("!NewSize", tmpvx, tmprgb);

	memcpy(gABuf, tmpg, GH_SIZE*GV_SIZE*sizeof(gABuf[0]));

	delete tmpg;
	delete tmprgb;
	delete tmpvx;

	return (float)minVx*inv_vx_fraction;
}



void vrtMap::convertMapSize(const char* _fname)
{
	XStream fmap(0);
	sVmpHeader VmpHeader;
	fmap.open(_fname, XS_OUT);
	fmap.seek(0,XS_BEG);
	//VmpHeader.setID("S2T0");
	VmpHeader.setID("S5L2");
	VmpHeader.XSize=H_SIZE;
	VmpHeader.YSize=V_SIZE/2;
	VmpHeader.AUX1=0;
	VmpHeader.AUX2=0;
	VmpHeader.AUX3=0;

	fmap.write(&VmpHeader,sizeof(VmpHeader));

	unsigned short* VxABufT=new unsigned short[VmpHeader.XSize*VmpHeader.YSize];
	unsigned char*  SurBufT=new unsigned char[VmpHeader.XSize*VmpHeader.YSize];;

	int i,j;
	for(i=0; i < VmpHeader.YSize; i++){
		for(j=0; j<VmpHeader.XSize; j++){
			VxABufT[VmpHeader.XSize*i + j]=(((vxaBuf[offsetBuf(j, i*2)]&0xFFF8) +  (vxaBuf[offsetBuf(j, i*2+1)]&0xFFF8))/2);// |VmAt_GeoDam;
			SurBufT[VmpHeader.XSize*i + j]=SurBufT[offsetBuf(j, i/2)];
		}
	}

	fmap.write(&VxABufT[0], VmpHeader.XSize*VmpHeader.YSize*sizeof(VxABufT[0]));
	fmap.write(&SurBufT[0], VmpHeader.XSize*VmpHeader.YSize*sizeof(SurBufT[0]));
	fmap.close();
}

void vrtMap::laceLine(int hbeg, float a, int xBeg, int yBeg, int cntMax, int dx, int dy)
{
	double tangens = tan(a);
	int i,j;
	int cnt=0;
	int hLineBeg=vMap.getAlt(xBeg, yBeg);
	bool flagUP = hLineBeg > hbeg;
	for(i=yBeg, j=xBeg, cnt=0; cnt < cntMax; i+=dy, j+=dx, cnt++){
        int h1 = hbeg + round(tangens*(double)cnt*VOXEL_MULTIPLIER);
        int h2 = hbeg - round(tangens*(double)cnt*VOXEL_MULTIPLIER);
		if(flagUP){ //срезаем до
			if(vMap.getAlt(j,i) < h1) break;
			vMap.putAlt(j, i, h1);
		}
		else{  //hLineBeg < hbeg
			if(vMap.getAlt(j,i) > h2) break;
			vMap.putAlt(j, i, h2);
		}
	}
}

void vrtMap::autoLace(int laceH, float angle)
{
	for(int i=0; i<V_SIZE; i++){
		laceLine(laceH, angle, 0, i,		100,	+1, 0);
		laceLine(laceH, angle, H_SIZE-1, i,	100,	-1, 0);
	}
	for(int j=0; j<H_SIZE; j++){
		laceLine(laceH, angle, j, 0,		100,	0, +1);
		laceLine(laceH, angle, j, V_SIZE-1,	100,	0, -1);
	}

	recalcArea2Grid(0, 0, H_SIZE-1, V_SIZE-1);
	regRender(0, 0, H_SIZE-1, V_SIZE-1, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture|vrtMap::TypeCh_Region);
}



//struct sWorldInfo {
//	string worldName;
//	unsigned short sizeX;
//	unsigned short sizeY;
//	sWorldInfo(const char* wname, unsigned short sizex, unsigned short sizey){
//		worldName=wname;
//		sizeX=sizex;
//		sizeY=sizey;
//	}
//};
//bool vrtMap::getListWorldInfo(list<sWorldInfo>& worldinfoList)
//{
//	WIN32_FIND_DATA FindFileData;
//	XBuffer str;
//	if(!worldsDir.empty()) str < worldsDir.c_str() < "\\";
//	else str <".\\";
//	str < "*";
//
//	HANDLE hf = FindFirstFile( str, &FindFileData );
//	if(hf != INVALID_HANDLE_VALUE){
//		do{
//			if ( FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY ) {
//				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='\0' ) continue;//ѕроверка на служебные записи
//				if(FindFileData.cFileName[0]=='.' && FindFileData.cFileName[1]=='.' && FindFileData.cFileName[2]=='\0' ) continue;
//				vrtMapCreationParam vMapCParam;
//				XBuffer str;
//				if(!worldsDir.empty()) str < worldsDir.c_str() < "\\";
//				else str <".\\";
//				str < FindFileData.cFileName < "\\" < worldDataFile;
//				XPrmIArchive ia;
//				if(ia.open(str, 1024)){
//					ia.serialize(vMapCParam, "WorldHeader", 0);
//				}
//				worldinfoList.push_back(sWorldInfo(FindFileData.cFileName, 1<<vMapCParam.H_SIZE_POWER, 1<<vMapCParam.V_SIZE_POWER));
//			}
//		} while(FindNextFile( hf, &FindFileData ));
//		FindClose( hf );
//	}
//	return true;
//}
