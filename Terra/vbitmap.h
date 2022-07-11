#ifndef __VBITMAP_H__
#define __VBITMAP_H__

#include "vmap.h"

struct sBitMap8 {
	unsigned short mX, mY;
	unsigned short sx,sy;
	//unsigned char* data;
	TerrainColor * data;
	sBitMap8(){ mX=mY=sx=sy=0; data=0; }
	~sBitMap8() { release(); }
	void create(int _sx, int _sy){
		release();
		sx=_sx; sy=_sy;
		//data=new unsigned char[sx*sy];
		data=new TerrainColor[sx*sy];
	};
	void clear(){
		const int size=sx*sy;
		for(int i=0; i<size; i++) data[i]=0;
	}
	void release(){
		if(data != 0) {
			delete [] data;
			data=0;
		}
		mX=mY=sx=sy=0;
	};
};

struct sVBitMap : public sBitMap8 {

	float A11, A12, A21, A22;
	float X, Y;
	float kX, kY, kZ;


	void put(int VBMmode, int VBMlevel, int VBMnoiseLevel, int VBMnoiseAmp, bool VBMinverse);

	inline int getColor(int x, int y){
		int x1,y1;
		x1 = round((A11*x + A12*y + X)*kX+(sx>>1));
		y1 = round((A21*x + A22*y + Y)*kY+(sy>>1));
		if(x1<0 || y1<0 || x1>=sx || y1>=sy) return -1;
		return data[(y1%sy)*sx + (x1%sx)]; 
	};

	inline int getPreciseColor(int x, int y){
		int x1,y1,xt, yt;
		xt = round( ((A11*x + A12*y + X)*kX+(sx>>1))*256 );
		yt = round( ((A21*x + A22*y + Y)*kY+(sy>>1))*256 );
		x1=xt>>8; y1=yt>>8; xt&=0xFF; yt&=0xFF;
		if(x1<0 || y1<0 || (x1+1)>=sx || (y1+1)>=sy) return 0;
		int h0,h1,h2,h3;
		h0=data[y1*sx + x1];
		h1=data[y1*sx + x1+1];
		h2=data[(y1+1)*sx + x1];
		h3=data[(y1+1)*sx + x1+1];
		h0=(h0<<8)+xt*(h1-h0);
		h2=(h2<<8)+xt*(h3-h2);
		return( (h0<<8)+yt*(h2-h0) )>>16; 
	};

	void Draw(int ViewX, int ViewY, float k_scale, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY){
		int x,y;
		float Ik_scale= 1.0f/k_scale;
		unsigned short * b=GRAF_BUF;
		int addx=ViewX-round((float)(GB_MAXX>>1)*Ik_scale);
		int addy=ViewY-round((float)(GB_MAXY>>1)*Ik_scale);
		for(y=0; y<GB_MAXY; y++){
			for(x=0; x<GB_MAXX; x++){
				int c=getColor(addx+round((float)x*Ik_scale), addy+round((float)y*Ik_scale));
				//if(c!=-1) *b=c;
				if(c>0) *b=c;
				b++;
			}
		}
	};

	void set_alpha(float alpha, int X0, int Y0, float _kX, float _kY, float _kZ){
		A11 = A22 = cosf(alpha*3.1415926535f/180.f);
		A21 = -(A12 = sinf(alpha*3.1415926535f/180.f));
		X = - A11*X0 - A12*Y0 ;
		Y = - A21*X0 - A22*Y0 ;
		kX=_kX; kY=_kY; kZ=_kZ;
		mX=X0; mY=Y0;
	};
	void set_alphaR(float alpha, int X0, int Y0){
		A11 = A22 = cosf(alpha);
		A21 = -(A12 = sinf(alpha));
		X = - A11*X0 - A12*Y0 ;
		Y = - A21*X0 - A22*Y0 ;
		kX=1.f; kY=1.f; kZ=1.f;
	};

};

//extern sVBitMap VBitMap;

struct sVBitMap16 {
	unsigned short mX, mY;
	unsigned short sx,sy;
	unsigned short* data;
	float A11, A12, A21, A22;
	float X, Y;
	float kX, kY, kZ;

	sVBitMap16(){
		data = 0;
	};

	void create(int _sx, int _sy){
		sx=_sx; sy=_sy;
		data=new unsigned short[sx*sy];
	};
	void clear(){
		const int size=sx*sy;
		for(int i=0; i<size; i++) data[i]=0;
	}
	void release(){
		if(data != 0) {
			delete [] data;
			data=0;
		}
	};
	~sVBitMap16(){
		release();
	};


	inline int getVoxel(int x, int y){
		int x1,y1;
		x1 = round((A11*x + A12*y + X)*kX+(sx>>1));
		y1 = round((A21*x + A22*y + Y)*kY+(sy>>1));
		if(x1<0 || y1<0 || x1>=sx || y1>=sy) return -1;
		return data[(y1%sy)*sx + (x1%sx)]; 
	};

	inline int getPreciseVoxel(int x, int y){
		int x1,y1,xt, yt;
		xt = round( ((A11*x + A12*y + X)*kX+(sx>>1))*256 );
		yt = round( ((A21*x + A22*y + Y)*kY+(sy>>1))*256 );
		x1=xt>>8; y1=yt>>8; xt&=0xFF; yt&=0xFF;
		if(x1<0 || y1<0 || (x1+1)>=sx || (y1+1)>=sy) return 0;
		int h0,h1,h2,h3;
		h0=data[y1*sx + x1];
		h1=data[y1*sx + x1+1];
		h2=data[(y1+1)*sx + x1];
		h3=data[(y1+1)*sx + x1+1];
		h0=(h0<<8)+xt*(h1-h0);
		h2=(h2<<8)+xt*(h3-h2);
		return( (h0<<8)+yt*(h2-h0) )>>16; 
	};

	void set_alpha(float alpha, int X0, int Y0, float _kX, float _kY, float _kZ){
		A11 = A22 = cosf(alpha*3.1415926535f/180.f);
		A21 = -(A12 = sinf(alpha*3.1415926535f/180.f));
		X = - A11*X0 - A12*Y0 ;
		Y = - A21*X0 - A22*Y0 ;
		kX=_kX; kY=_kY; kZ=_kZ;
		mX=X0; mY=Y0;
	};
	void set_alphaR(float alpha, int X0, int Y0){
		A11 = A22 = cosf(alpha);
		A21 = -(A12 = sinf(alpha));
		X = - A11*X0 - A12*Y0 ;
		Y = - A21*X0 - A22*Y0 ;
		kX=1.f; kY=1.f; kZ=1.f;
	};

};


#endif //__VBITMAP_H__
