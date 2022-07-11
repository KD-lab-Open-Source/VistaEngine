#include "stdafxTr.h"
#include "terTools.h"
#include "tgai.h"


void vrtMap::wrldShotMapHeight(void)
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
			*p++=GetAlt(offb)>>VX_FRACTION;//1;//
		}
		ff.write(line,SX);
	}
	ff.close();
	delete line;
}

void vrtMap::FlipWorldH(void)
{
	register unsigned int i,j;
	int DY=V_SIZE;
	int DX=H_SIZE>>1;
	int MX=H_SIZE-1;
	for(j = 0; j<DY; j++){
		for(i = 0; i<DX; i++){
			int off1=offsetBuf(i, j);
			int off2=offsetBuf(MX-i, j);
			unsigned char t8;
			unsigned short t16;
			unsigned long t32;
			t16=VxABuf[off1]; VxABuf[off1]=VxABuf[off2]; VxABuf[off2]=t16;
			t8=SurBuf[off1]; SurBuf[off1]=SurBuf[off2]; SurBuf[off2]=t8;
			t32=SupBuf[off1]; SupBuf[off1]=SupBuf[off2]; SupBuf[off2]=t32;
		}
	}
}

void vrtMap::FlipWorldV(void)
{
	register unsigned int i,j;
	int DY=V_SIZE>>1;
	int DX=H_SIZE;
	int MY=V_SIZE-1;
	for(j = 0; j<DY; j++){
		for(i = 0; i<DX; i++){
			int off1=offsetBuf(i, j);
			int off2=offsetBuf(i, MY-j);
			unsigned char t8;
			unsigned short t16;
			unsigned long t32;
			t16=VxABuf[off1]; VxABuf[off1]=VxABuf[off2]; VxABuf[off2]=t16;
			t8=SurBuf[off1]; SurBuf[off1]=SurBuf[off2]; SurBuf[off2]=t8;
			t32=SupBuf[off1]; SupBuf[off1]=SupBuf[off2]; SupBuf[off2]=t32;
		}
	}
}

void vrtMap::RotateWorldP90(void)
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
	//		t16=VxABuf[off1]; VxABuf[off1]=VxABuf[off4]; 
	//		VxABuf[off4]=VxABuf[off3]; VxABuf[off3]=VxABuf[off2]; VxABuf[off2]=t16;

	//		t8=SurBuf[off1]; SurBuf[off1]=SurBuf[off4]; 
	//		SurBuf[off4]=SurBuf[off3]; SurBuf[off3]=SurBuf[off2]; SurBuf[off2]=t8;

	//		t32=SupBuf[off1]; SupBuf[off1]=SupBuf[off4];
	//		SupBuf[off4]=SupBuf[off3]; SupBuf[off3]=SupBuf[off2]; SupBuf[off2]=t32;
	//	}
	//	DX-=1; BEGI+=1;
	//}

	unsigned short* nVxABuf = new unsigned short [H_SIZE*V_SIZE];
	unsigned char* nSurBuf = new unsigned char [H_SIZE*V_SIZE];
    unsigned long* nSupBuf = new unsigned long [H_SIZE*V_SIZE];
	unsigned long cnt=0;
	for(j=0; j <H_SIZE; j++ ){
		for(i=V_SIZE-1; i>=0; i--){
			nVxABuf[cnt]=VxABuf[offsetBuf(j,i)];
			nSurBuf[cnt]=SurBuf[offsetBuf(j,i)];
			nSupBuf[cnt]=SupBuf[offsetBuf(j,i)];
			cnt++;
		}
	}
	swap(nVxABuf,VxABuf);
	swap(nSurBuf,SurBuf);
	swap(nSupBuf,SupBuf);
	swap(H_SIZE,V_SIZE);
	delete [] nVxABuf;
	delete [] nSurBuf;
	delete [] nSupBuf;

}

void vrtMap::RotateWorldM90(void)
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
			unsigned char t8;
			unsigned short t16;
			unsigned long t32;
			t16=VxABuf[off1]; VxABuf[off1]=VxABuf[off2]; 
			VxABuf[off2]=VxABuf[off3]; VxABuf[off3]=VxABuf[off4]; VxABuf[off4]=t16;

			t8=SurBuf[off1]; SurBuf[off1]=SurBuf[off2]; 
			SurBuf[off2]=SurBuf[off3]; SurBuf[off3]=SurBuf[off4]; SurBuf[off4]=t8;

			t32=SupBuf[off1]; SupBuf[off1]=SupBuf[off2]; 
			SupBuf[off2]=SupBuf[off3]; SupBuf[off3]=SupBuf[off4]; SupBuf[off4]=t32;
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
			short v=GetAlt(offb);
			v=round(v*k);
			if(v>MAX_VX_HEIGHT)v=MAX_VX_HEIGHT; if(v<0)v=0;
			PutAlt(offb, v);
		}
	}
}
void vrtMap::changeTotalWorldHeight(int deltaVx, float kScale)
{
	//find min height
	int minVx=INT_MAX;
	int maxVx=INT_MIN;
	int i, j;
	for(i=0; i<V_SIZE; i++){
		for(j=0; j<H_SIZE; j++){
			int h=GetAlt(j,i);
			if(minVx>h) minVx=h;
			if(maxVx<h) maxVx=h;
		}
	}

	for(i=0; i<V_SIZE; i++){
		int offY=offsetBuf(0,i);
		for(j=0; j<H_SIZE; j++){
			int off=offY+j;
			int h=GetAlt(off);
			h = deltaVx + minVx+ round((h-minVx)*kScale);
			h=clamp(h, 0, MAX_VX_HEIGHT);
			PutAlt(off, h);
		}
	}
	//WorldRender();
	recalcArea2Grid(0, 0, H_SIZE-1, V_SIZE-1);
	regRender(0, 0, H_SIZE-1, V_SIZE-1, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture|vrtMap::TypeCh_Region);
}


int vrtMap::convertVMapTryColor2IdxColor(bool flag_useTerToolColor) //SupBuf2SurBuf
{
	xassert(vMap.SupBuf);
	ColorQuantizer cq;
	//int result=cq.quantizeOctree(SupBuf, SurBuf, Vect2s(H_SIZE, V_SIZE), 256);
	bool resultColorQuantizerPrepare=cq.prepare4PutColor(MAX_DAM_SURFACE_TYPE);
	xassert(resultColorQuantizerPrepare);
	int i,j,cnt=0;
	for(i=0; i< vMap.V_SIZE; i++){
		for(j=0; j< vMap.H_SIZE; j++){
			cq.putColor(vMap.SupBuf[cnt]);
			cnt++;
		}
	}
	if(flag_useTerToolColor)
		terToolsDispatcher.putAllColorTerTools2ColorQuantizer(cq);
	bool resultColorQuantizerPostProcess=cq.postProcess();
	xassert(resultColorQuantizerPostProcess);
	int result=cq.ditherFloydSteinberg(vMap.SupBuf, vMap.SurBuf, Vect2s(vMap.H_SIZE, vMap.V_SIZE));

	for(int i=0; i<MAX_DAM_SURFACE_TYPE; i++){
		vMap.DamPal[i]=cq.pPalette[i];
	}
	vMap.convertPal2TableSurCol(vMap.DamPal, vMap.Damming);
	return result;
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
			VxABufT[VmpHeader.XSize*i + j]=(((VxABuf[offsetBuf(j, i*2)]&0xFFF8) +  (VxABuf[offsetBuf(j, i*2+1)]&0xFFF8))/2) |VmAt_GeoDam;
			SurBufT[VmpHeader.XSize*i + j]=SurBufT[offsetBuf(j, i/2)];
		}
	}

	fmap.write(&VxABufT[0], VmpHeader.XSize*VmpHeader.YSize*sizeof(VxABufT[0]));
	fmap.write(&SurBufT[0], VmpHeader.XSize*VmpHeader.YSize*sizeof(SurBufT[0]));
	fmap.close();
}
