#ifndef __FILEIMAGE_H__
#define __FILEIMAGE_H__
#include "Render\Inc\rd.h"
#include "XMath\Colors.h"
#include "XMath\Rectangle4f.h"
#include <vector>
#include <string>
using namespace std;

class RENDER_API cFileImage
{
public:
	cFileImage()																{ x=y=length=bpp=time=fmt=0; }
	virtual ~cFileImage()														{}
	virtual int load(void *pointer,int size)									{ return -1; }
	virtual int load(const char *fname)												{ return -1; }
	virtual int save(const char *fname,void *pointer,int bpp,int x,int y,int length=1,int time=0){ return -1; }
	virtual int close()															{ return -1; }
	
	virtual int GetTexture(void *pointer,int time, int xSize,int ySize)			{return -1;}
	inline int GetX()															{ if(x>=0) return x; return -x; }
	inline int GetY()															{ if(y>=0) return y; return -y; }
	inline int GetFmt()															{ return fmt; }
	inline int GetTime() const 													{ return time; }
	inline int GetLength() const												{ return length; }
	inline int GetBitPerPixel()													{ return bpp; }
	static void InitFileImage();
	static void DoneFileImage();
	static cFileImage* Create(const char *fname);
	static cFileImage* Create(char const * diffuse_name, char const * self_illumination_name, char const *emblem_fname,
							const char* skin_color_name,
							Color4c skin_color, sRectangle4f* emblem_position, float emblem_angle);

	virtual bool IsCompositeFileImage(){return false;}
protected:
	int	x,y,bpp,length,time,fmt;
};

class cAviScaleFileImage : public cFileImage
{
protected:
	DWORD* dat;
	int dx;
	int dy;
	int n_count;
	int maxMipLevels;
public:
	vector<sRectangle4f> positions;
	vector<Vect2i> sizes;

	cAviScaleFileImage();
	~cAviScaleFileImage();
	bool Init(const char* fName);
	int GetXFrame()		{return dx;}
	int GetYFrame()		{return dy;}
	int GetFramesCount()	{return n_count;}
	virtual int GetTexture(void *pointer,int time, int xSize,int ySize);
	int GetMaxMipLevels(){return maxMipLevels;}
};

void cFileImage_GetFrame(void *pDst, int xDst, int yDst, 
								void *pSrc, int xSrc, int ySrc, 
								int byteCount, bool vInvert);

void cFileImage_GetFrameAlpha(void *pDst, int xDst, int yDst, 
							  void *pSrc, int xSrc, int ySrc, 
							  int bytesPerPixel, bool vInvert);

bool SaveTga(const char* filename,int width,int height,unsigned char* buf,int byte_per_pixel);
RENDER_API bool LoadTGA(const char* filename,int& dx,int& dy,unsigned char*& buf,int& byte_per_pixel);

class cCompositeImage : public cFileImage {
public:
	cCompositeImage(cFileImage* pMainImage,
			cFileImage* pSelfIlluminationImage,
			cFileImage* pEmblemImage,
			cFileImage* pSkinImage,
		Color4c skin_color, sRectangle4f* logo_position,float angle);
	~cCompositeImage();
	int GetTexture(void *pointer,int time, int xSize,int ySize);
	virtual bool IsCompositeFileImage(){return true;}
protected:
	cFileImage* pMainImage;
	cFileImage* pSelfIlluminationImage;
	cFileImage* pEmblemImage;
	cFileImage* pSkinImage;
	Color4c skin_color;
	sRectangle4f* logo_pos;
	float logo_angle;
};

class cComplexFileImage : public cAviScaleFileImage
{
public:
	cComplexFileImage();
	~cComplexFileImage();
	bool Init(vector<string>& names,bool line=false);
protected:
};

class cFileImageData : public cFileImage
{
	Color4c* data;
public:
	cFileImageData(int dx,int dy,Color4c* data_)
	{
		x=dx;
		y=dy;
		data=data_;
	}

	virtual int GetTexture(void *pointer,int time, int xSize,int ySize)
	{
		memcpy(pointer,data,x*y*4);
		return 0;
	}
};

#endif //__FILEIMAGE_H__
