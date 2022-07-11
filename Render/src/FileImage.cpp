#include <my_stl.h>
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include <vfw.h>		// AVI include
#include <setjmp.h>		// JPG include
#include <math.h>
#include <XUtil.h>
#include "XZip.h"
#include "FileImage.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "Render\3dx\Umath.h"
#include "Render\Src\filter.h"
#include "TextureAtlas.h"
#include "Serialization\Serialization.h"
#include "Render\inc\FileRead.h"
#include "FileUtils\FileUtils.h"

#pragma comment (lib,"vfw32") // AVI library


#ifdef USE_JPEG
#include "jpeglib.h"	// JPG include
#pragma comment (lib,"jpeg") // JPG library
#endif USE_JPEG

extern bool ResourceIsZIP();


void cFileImage_GetFrameAlpha(void *pDst, int xDst, int yDst, 
							  void *pSrc, int xSrc, int ySrc, 
							  int bytesPerPixel, bool vInvert)
{
	Pixel* tempIn;
	Pixel* tempOut;
	int curByte = bytesPerPixel==4?3:0;
	int srcStart,srcEnd,dir;

	bool needResample = true;
	if (xDst==xSrc && yDst==ySrc)
		needResample = false;

	Pixel* src = (Pixel*)pSrc;
	Pixel* dst = (Pixel*)pDst;

	tempIn = new Pixel[xSrc*ySrc];
	tempOut = new Pixel[xDst*yDst];
	if(!vInvert)
	{
		srcStart = 0;
		srcEnd = ySrc;
		dir = 1;
	}else
	{
		srcStart = ySrc-1;
		srcEnd = -1;
		dir = -1;
	}

	int i=0;
	if(needResample)
	{
		for(int y=srcStart; y != srcEnd; y+=dir)
			for(int x=0; x<xSrc; x++)
			{
				tempIn[i] = src[(x+y*xSrc)*bytesPerPixel+curByte];
				i++;
			}
			resample(tempIn,xSrc,ySrc,tempOut,xDst,yDst,'l');
			for(i=0; i<xDst*yDst; i++)
			{
				dst[i*4+3] = tempOut[i];
			}
	}else
	{
		for(int y=srcStart; y != srcEnd; y+=dir)
			for(int x=0; x<xSrc; x++)
			{
				dst[i*4+3] = src[(x+y*xSrc)*bytesPerPixel+curByte];
				i++;
			}
	}

	delete[] tempIn;
	delete[] tempOut;
}
void cFileImage_GetFrame(void *pDst, int xDst, int yDst, 
						 void *pSrc, int xSrc, int ySrc, 
						 int bytesPerPixel, bool vInvert)
{
	Pixel* tempIn;
	Pixel* tempOut;
	int srcStart,srcEnd,dir;
	
	bool needResample = true;
	if (xDst==xSrc && yDst==ySrc)
		needResample = false;

	Pixel* src = (Pixel*)pSrc;
	Pixel* dst = (Pixel*)pDst;

	tempIn = new Pixel[xSrc*ySrc];
	tempOut = new Pixel[xDst*yDst];
	if(!vInvert)
	{
		srcStart = 0;
		srcEnd = ySrc;
		dir = 1;
	}else
	{
		srcStart = ySrc-1;
		srcEnd = -1;
		dir = -1;
	}

	if(bytesPerPixel==4 || bytesPerPixel==3)
	{
		if(needResample)
		{
			for(int curByte=0; curByte<bytesPerPixel; curByte++)
			{
				int i=0;
				for(int y=srcStart; y != srcEnd; y+=dir)
				for(int x=0; x<xSrc; x++)
				{
					tempIn[i] = src[(x+y*xSrc)*bytesPerPixel+curByte];
					i++;
				}
				resample(tempIn,xSrc,ySrc,tempOut,xDst,yDst,'l');
				for(i=0; i<xDst*yDst; i++)
				{
					dst[i*4+curByte] = tempOut[i];
				}
			}
		}else
		{
			for(int curByte=0; curByte<bytesPerPixel; curByte++)
			{
				int i=0;
				for(int y=srcStart; y != srcEnd; y+=dir)
				for(int x=0; x<xSrc; x++)
				{
					dst[i*4+curByte] = src[(x+y*xSrc)*bytesPerPixel+curByte];
					i++;
				}
			}
		}

		if(bytesPerPixel==3)
		{
			for(int i=0; i<xDst*yDst; i++)
			{
				dst[i*4+3] = 255;
			}
		}
	}else
	if(bytesPerPixel==1)
	{
		if(needResample)
		{
			int curByte=0;
			int i=0;
			for(int y=srcStart; y != srcEnd; y+=dir)
			for(int x=0; x<xSrc; x++)
			{
				tempIn[i] = src[(x+y*xSrc)*bytesPerPixel+curByte];
				i++;
			}
			resample(tempIn,xSrc,ySrc,tempOut,xDst,yDst,'l');
			for(i=0; i<xDst*yDst; i++)
			{
				Pixel c=tempOut[i];
				((Color4c*)&dst[i*4+curByte])->set(c,c,c,255);
			}
		}else
		{
			int curByte=0;
			int i=0;
			for(int y=srcStart; y != srcEnd; y+=dir)
			for(int x=0; x<xSrc; x++)
			{
				Pixel c=src[(x+y*xSrc)*bytesPerPixel+curByte];
				((Color4c*)&dst[i*4+curByte])->set(c,c,c,255);
				i++;
			}
		}
	}else
	{
		xassert(0);
	}

	delete[] tempIn;
	delete[] tempOut;
}


//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cTGAImage
//////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(push,1)

struct TGAHeader
{
unsigned char idFieldLength;
unsigned char colorMapType;
/* The image type. */
#define TGA_TYPE_MAPPED 1
#define TGA_TYPE_COLOR 2
#define TGA_TYPE_GRAY 3
#define TGA_TYPE_MAPPED_RLE 9
#define TGA_TYPE_COLOR_RLE 10
#define TGA_TYPE_GRAY_RLE 11
unsigned char imageType;
unsigned short indexFirstColorMapEntry;
unsigned short countColorMapEntries;
unsigned char numberOfBitsPerColorMapEntry;
unsigned short startX;
unsigned short startY;
unsigned short width;
unsigned short height;
unsigned char bitsPerPixel;

#define TGA_DESC_ABITS 0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL 0x20
unsigned char flags;
};
#pragma pack(pop)

static bool saveFileSmart(const char* fname, const char* buffer, int size)
{
	XStream testf(0);
	if(testf.open(fname, XS_IN)){
		if(testf.size() == size){
			PtrHandle<char> buf = new char[size];
			testf.read(buf, size);
			if(!memcmp(buffer, buf, size))
				return true;
		}
	}
	testf.close();
	
	XStream ff(0);
	if(ff.open(fname, XS_OUT)) {
		ff.write(buffer, size);
	} 
#ifndef _FINAL_VERSION_
	else{
		XBuffer buf;
		buf < "Unable to write file: \n" < fname;
		xxassert(0, buf);
	}
#endif

	return !ff.ioError();
}

bool SaveTga(const char* filename,int width,int height,unsigned char* buf,int byte_per_pixel)
{
	bool bHasAlpha=false;

	TGAHeader Hdr;
	Hdr.idFieldLength=0;
	Hdr.colorMapType=0;
	Hdr.imageType=byte_per_pixel==1?3:2;
	Hdr.indexFirstColorMapEntry=0;
	Hdr.countColorMapEntries=0;
	Hdr.numberOfBitsPerColorMapEntry=0;
	Hdr.startX=0;
	Hdr.startY=0;
	Hdr.width=(unsigned short)width;
	Hdr.height=(unsigned short)height;
	Hdr.bitsPerPixel=(unsigned char)(byte_per_pixel*8);
	Hdr.flags=(bHasAlpha?8:0)|0x20;

	unsigned long Numbytes=Hdr.width*Hdr.height*(Hdr.bitsPerPixel>>3);

	XBuffer buffer(18 + Numbytes);
	buffer.write(&Hdr,18);
	buffer.write(buf,Numbytes);

	return saveFileSmart(filename, buffer, buffer.tell());
}

RENDER_API bool LoadTGA(const char* filename,int& dx,int& dy,unsigned char*& buf,
			 int& byte_per_pixel)
{
	XZipStream f(0);
	if(!f.open(filename, XS_IN))
		return false; 

	TGAHeader Hdr;

	if(f.read(&Hdr,18)!=18)
	{
		f.close(); 
		return false;
	}

	byte_per_pixel=Hdr.bitsPerPixel/8;

	dx=Hdr.width;
	dy=Hdr.height;

	unsigned long Numbytes=Hdr.width*Hdr.height*(Hdr.bitsPerPixel>>3);

	buf=new unsigned char[Hdr.width*Hdr.height*byte_per_pixel];
	int readbytes=f.read(buf,Numbytes);
	f.close(); 
	if(readbytes<readbytes)
		return false;

	bool updown=(Hdr.flags&0x20)?false:true;

	if(updown)
	{
		int maxy=dy/2;
		int size=dx*byte_per_pixel;
		unsigned char* tmp=new unsigned char[size];

		for(int y=0;y<maxy;y++)
		{
			unsigned char* p1=buf+y*size;
			unsigned char* p2=buf+(dy-1-y)*size;
			memcpy(tmp,p1,size);
			memcpy(p1,p2,size);
			memcpy(p2,tmp,size);
		}

		delete tmp;
	}

	return true;
}

class cTGAImage : public cFileImage
{
	void	  *ImageData;
	TGAHeader tga;
public:
	cTGAImage()	
	{
		ImageData=0;
		length=1;
	}
	virtual ~cTGAImage()										{ close(); }
	virtual int close()
	{
		if(ImageData) { delete ImageData; } ImageData=0;
		return 0;
	}
	virtual int load(const char *fname)
	{
		int size=0;
		char *buf=0;
		if(!RenderFileRead(fname,buf,size)) return 1;
		return load(buf,size);
	}
#define RLE_PACKETSIZE 0x80
	int rle_load(void* pointer,int size)
	{
		int readSize = 0;
		int bytePerPixel = bpp/8;
		unsigned char count;
		int offcet = 0;
		int bytes = 0;
		unsigned char *p = new unsigned char[bytePerPixel];
		unsigned char* pDst = (unsigned char*)ImageData;
		unsigned char* pSrc = (unsigned char*)pointer;
		while (readSize < x*y*bytePerPixel)
		{
			if (offcet+1 > size)
			{
				delete [] p;
				return 1;
			}
			count = *(pSrc+offcet);
			offcet++;

			bytes = ((count & ~RLE_PACKETSIZE)+1)*bytePerPixel;
			
			if (count & RLE_PACKETSIZE)
			{
				memcpy(p,pSrc+offcet,bytePerPixel);
				offcet += bytePerPixel;
				for(int i = 0; i<bytes; i+= bytePerPixel)
				{
					memcpy(pDst+readSize,p,bytePerPixel);
					readSize += bytePerPixel;
				}
			}else
			{
				memcpy(pDst+readSize,pSrc+offcet,bytes);
				readSize += bytes;
				offcet += bytes;
			}

		}
		delete [] p;
		return 0;
	}
	virtual int load(void *pointer,int size)
	{
		close();
		bool isRle = false;
		bool isHoriz = false;
		bool isVert = false;

		memcpy(&tga,pointer,sizeof(TGAHeader));

		isHoriz = (tga.flags & TGA_DESC_HORIZONTAL)?true:false;
		isVert = (tga.flags & TGA_DESC_VERTICAL)?true:false;

		switch(tga.imageType)
		{
		case TGA_TYPE_MAPPED_RLE:
		case TGA_TYPE_GRAY_RLE:
		case TGA_TYPE_COLOR_RLE:
			isRle = true;
			break;
		default:
			isRle = false;
		};

		x=tga.width;
		y=tga.height;
		bpp=tga.bitsPerPixel;
		int colormapsize=(tga.countColorMapEntries*tga.numberOfBitsPerColorMapEntry)/8;

		int pre_size=sizeof(TGAHeader)+colormapsize;
		int BytePerPixel=bpp/8;
		ImageData = new char[x*y*BytePerPixel];

		int data_size=size-pre_size;
		unsigned char* src = (unsigned char*)pointer+pre_size;
		int res = 0;
		if(isRle)
		{
			res = rle_load(src,data_size);
		}else
		{
			memcpy(ImageData,(void*)(src),x*y*BytePerPixel);
		}
		//if(data_size<x*y*BytePerPixel)
		//	return 1;
		delete pointer;
		return res;
	}
	virtual int save(char *fname,void *pointer,int bpp,int x,int y,int length=1,int time=0)
	{
		if(!SaveTga(fname,x,y,(unsigned char*)pointer,bpp/8))
			return -1;
		return 0;
	}
	virtual int GetTexture(void *pointer,int time, int xDst,int yDst)
	{ 
		xassert(ImageData);
		int bytesPerPixel = GetBitPerPixel()>>3;
		cFileImage_GetFrame(pointer,xDst,yDst,ImageData,GetX(),GetY(),bytesPerPixel,!(tga.flags&0x20));
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cAVIImage
//////////////////////////////////////////////////////////////////////////////////////////
class cAVIImage : public cFileImage
{
	IGetFrame	*Frame;
	IAVIStream	*pavi;
public:
	cAVIImage()															{ Frame=0; pavi=0; }
	virtual ~cAVIImage()												{ close(); }
	virtual int close()
	{
		if(Frame) AVIStreamGetFrameClose(Frame); Frame=0;
		if(pavi) AVIStreamRelease(pavi); pavi=0;
		return 0;
	}
	virtual int load(const char *fname)
	{
		AVISTREAMINFO FAR psi;

		HRESULT hr=AVIStreamOpenFromFile(&pavi,fname,streamtypeVIDEO,0,OF_READ,0);
		if(hr) return 1;
		time=AVIStreamLengthTime(pavi);
		AVIStreamInfo(pavi,&psi,sizeof(AVISTREAMINFO FAR));
		x=psi.rcFrame.right-psi.rcFrame.left;
		y=psi.rcFrame.bottom-psi.rcFrame.top;
		length=psi.dwLength;
		Frame=AVIStreamGetFrameOpen(pavi,0);
		BITMAPINFO *pbmi=(BITMAPINFO*)AVIStreamGetFrame(Frame,AVIStreamTimeToSample(pavi,0));
		bpp=pbmi->bmiHeader.biBitCount;
		return 0;
	}
	virtual int save(char *fname,void *pointer,int bpp,int x,int y,int length=1,int time=0)
	{ 
		int err;
		BITMAPINFOHEADER bmh;
		memset(&bmh,0,sizeof(BITMAPINFOHEADER));
		bmh.biSize        = sizeof(BITMAPINFOHEADER);
		bmh.biWidth       = x;
		bmh.biHeight      = y;
		bmh.biPlanes      = 1;
		bmh.biBitCount    = bpp;
		bmh.biCompression = BI_RGB;
		bmh.biSizeImage   = x*y*bpp/8;
		if(bpp!=32) return -2;
		PAVISTREAM pcomp=0;
		PAVIFILE fAVI=0;
		remove(fname);
		if(err=AVIFileOpen(&fAVI,fname,OF_CREATE|OF_WRITE,0))
			return 1;
		AVISTREAMINFO avi;
		memset(&avi,0,sizeof(AVISTREAMINFO));
		avi.fccType    = streamtypeVIDEO;
		avi.fccHandler = mmioFOURCC('D', 'I', 'B', ' ');
		avi.dwScale    = 1;
		avi.dwRate     = length*1000/time;
		avi.dwQuality  = 0;
		avi.dwLength   = length;
		if(err=AVIFileCreateStream(fAVI,&pavi,&avi))
			return 2;
		AVICOMPRESSOPTIONS compOptions;
		memset(&compOptions, 0, sizeof(AVICOMPRESSOPTIONS));
		compOptions.dwFlags         = AVICOMPRESSF_VALID | AVICOMPRESSF_KEYFRAMES;
		compOptions.fccType         = streamtypeVIDEO;
		compOptions.fccHandler      = avi.fccHandler;
		compOptions.dwQuality       = avi.dwQuality;
		compOptions.dwKeyFrameEvery = 15;
		if(err=AVIMakeCompressedStream(&pcomp, pavi, &compOptions, 0))
			return 3;
		if(err=AVIStreamSetFormat(pcomp,0,&bmh,bmh.biSize))
			return 4;
		unsigned char *buf=new unsigned char[bmh.biSizeImage];
		for(int i=0;i<length;i++)
		{
			for(int j=0;j<y;j++)
				memcpy(&buf[(y-j-1)*bmh.biSizeImage/y],&((LPBYTE)pointer)[i*bmh.biSizeImage+j*bmh.biSizeImage/y],bmh.biSizeImage/y);
			if(err=AVIStreamWrite(pcomp,i,1,buf,bmh.biSizeImage,AVIIF_KEYFRAME,0,0))
				return 5;
		}
		delete buf;
		if(pcomp) AVIStreamRelease(pcomp); pcomp=0;
		if(pavi) AVIStreamRelease(pavi); pavi=0;
		if(fAVI) AVIFileRelease(fAVI); fAVI=0;
		return 0; 
	}
	virtual int GetTexture(void *pointer,int time, int xDst,int yDst)
	{
		xassert(time>=0 && time<GetLength());
		//int sample=AVIStreamTimeToSample(pavi,t%time);
		BITMAPINFO *pbmi=(BITMAPINFO*)AVIStreamGetFrame(Frame,time/*sample*/);
		if(pbmi->bmiHeader.biCompression) return 1;
		int bytesPerPixel = GetBitPerPixel()>>3;
		cFileImage_GetFrame(pointer,xDst,yDst,((unsigned char*)pbmi->bmiColors),GetX(),GetY(),bytesPerPixel,true);
		return 0;
	}
	static void Init()
	{ 
		AVIFileInit(); /*opens AVIFile library*/ 
	}
	static void Done()
	{ 
		AVIFileExit(); /*closes AVIFile library*/ 
	}
};
//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cJPGImage
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_JPEG
struct my_error_mgr
{
	struct jpeg_error_mgr pub;	// "public" fields
	jmp_buf setjmp_buffer;		// for return to caller
};
typedef struct my_error_mgr * my_error_ptr;

METHODDEF() my_error_exit (j_common_ptr cinfo)
{
	// cinfo->err really points to a my_error_mgr struct, so coerce pointer
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	// Always display the message.
	// We could postpone this until after returning, if we chose.
	(*cinfo->err->output_message) (cinfo);
	// Return control to the setjmp point
	longjmp(myerr->setjmp_buffer, 1);
}
METHODDEF() JPG_init_source (j_decompress_ptr cinfo)
{
	jpeg_source_mgr *src = cinfo->src;
	src->start_of_file = TRUE;
}
METHODDEF(boolean) JPG_fill_input_buffer (j_decompress_ptr cinfo)
{
	jpeg_source_mgr *src = (jpeg_source_mgr*) cinfo->src;
	src->start_of_file = FALSE;
	return TRUE;
}
METHODDEF() JPG_term_source (j_decompress_ptr cinfo)
{
}
METHODDEF() JPG_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr *src = cinfo->src;
	if (num_bytes > 0) 
	{
		while (num_bytes > (long) src->bytes_in_buffer) 
		{
			num_bytes -= (long) src->bytes_in_buffer;
			JPG_fill_input_buffer(cinfo);
		}
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}
void JPG_stdio_source (j_decompress_ptr cinfo,void *FileBuf,int FileSize)
{
	jpeg_source_mgr *src;

	if (cinfo->src == 0) // first time for this JPEG object?
		cinfo->src = (struct jpeg_source_mgr*) (*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(jpeg_source_mgr) );

	src = cinfo->src;
	src->init_source = JPG_init_source;
	src->fill_input_buffer = JPG_fill_input_buffer;
	src->skip_input_data = JPG_skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->term_source = JPG_term_source;
	src->buffer = (unsigned char*) FileBuf;
	src->next_input_byte = src->buffer;
	src->bytes_in_buffer = FileSize;
	src->infile = (FILE*) 0xFFFFFFFF; // hint !!!
}

class cJPGImage : public cFileImage
{
	char *ImageData; 
public:
	cJPGImage()													{ length=1; ImageData=0; }
	virtual ~cJPGImage()										{ close(); }
	virtual int close()											
	{ 
		if(ImageData) delete ImageData; ImageData=0;
		return 0; 
	}
	virtual int load(void *FileBuf,int FileSize)
	{
		close();
		struct jpeg_decompress_struct cinfo;
		struct my_error_mgr jerr;

		// Step 1: allocate and initialize JPEG decompression object
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = my_error_exit;
		// Establish the setjmp return context for my_error_exit to use.
		if (setjmp(jerr.setjmp_buffer)) 
		{
			jpeg_destroy_decompress(&cinfo);
			delete FileBuf;
			return 1;
		}
		// Now we can initialize the JPEG decompression object.
		jpeg_create_decompress(&cinfo);
		// Step 2: specify data source (eg, a file)
		JPG_stdio_source( &cinfo, FileBuf, FileSize ); // jpeg_stdio_src(&cinfo, fp);
		// Step 3: read file parameters with jpeg_read_header()
		jpeg_read_header(&cinfo, TRUE);
		// Step 4: set parameters for decompression
		// Step 5: Start decompressor 
		jpeg_start_decompress(&cinfo);

		x = cinfo.output_width;
		y = cinfo.output_height;
		bpp = cinfo.num_components*8;
		int bpl = cinfo.output_width * cinfo.output_components;
		ImageData = new char[bpl*y];

		// Make a one-row-high sample array that will go away when done with image
		JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, bpl, 1);
		// Step 6: while (scan lines remain to be read) jpeg_read_scanlines(...);
		for(int j=0;cinfo.output_scanline < cinfo.output_height; j++) 
		{
			jpeg_read_scanlines( &cinfo, buffer, 1 );
			memcpy( &ImageData[j*bpl], buffer[0], bpl);
		}
		/* Step 7: Finish decompression */
		jpeg_finish_decompress(&cinfo);
		/* Step 8: Release JPEG decompression object */
		jpeg_destroy_decompress(&cinfo);
		delete FileBuf;
		return 0;
	}
	virtual int load(const char *fname)
	{
		int FileSize=0;
		char *FileBuf=0;
		if(!RenderFileRead( fname, FileBuf, FileSize ) )
			return 1;
		load( FileBuf, FileSize );
		return 0;
	}
	virtual int GetTexture(void *pointer,int time, int xDst,int yDst)
	{ 
		int bytesPerPixel = GetBitPerPixel()>>3;
		cFileImage_GetFrame(pointer,xDst,yDst,ImageData,GetX(),GetY(),bytesPerPixel,false);
		return 0;
	}
	static void Init()													{}
	static void Done()													{}
};

#endif USE_JPEG

///////////////////////////////////////////////
///AVIX
class cAVIXImage : public cFileImage
{
	struct AVIX
	{
		DWORD avix;
		DWORD X;
		DWORD Y;
		DWORD bpp;
		DWORD numframe;
		DWORD time;
		char p[1];
	}* data;
public:
	cAVIXImage()														{ data=0; }
	virtual ~cAVIXImage()												{ close(); }
	virtual int close()
	{
		if(data) delete data; data=0;
		return 0;
	}
	virtual int load(const char *fname)
	{
		char* buf;
		int size;
		char name[512];
		strcpy(name,fname);
		strcat(name,"x");
		if(!RenderFileRead(name,buf,size))
			return 1;
		data=(AVIX*)buf;

		time=data->time;
		x=data->X;
		y=data->Y;
		length=data->numframe;
		bpp=data->bpp;
		return 0;
	}
	virtual int save(char *fname,void *pointer,int bpp,int x,int y,int length=1,int time=0)
	{ 
		xassert(0);
		return 0; 
	}
	virtual int GetTexture(void *pointer,int time, int xDst,int yDst)
	{
		void *pbmi=GetFrameByte((time/GetTimePerFrame())%length);
		int bytesPerPixel = GetBitPerPixel()>>3;
		cFileImage_GetFrame(pointer,xDst,yDst,pbmi,GetX(),GetY(),bytesPerPixel,false);
		return 0;
	}
	static void Init()													{ }
	static void Done()													{ }

	void* GetFrameByte(int n)
	{
		int fsize=x*y*(bpp>>3);
		return data->p+fsize*n;
	}

	int GetTimePerFrame() const 
	{
		if(GetLength()<=1) 
			return 1;
		else
			return (GetTime()-1)/(GetLength()-1);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// реализация интерфейса cFileImage
cFileImage* cFileImage::Create(const char* fname)
{
	string ext = extractFileExt(fname);
	if(ext == ".tga")
		return new cTGAImage;
	else if(ext == ".avi")
		return (cFileImage*)new cAVIImage;
#ifdef USE_JPEG
	else if(ext == ".jpg")
		return new cJPGImage;
#endif USE_JPEG
	return 0;
}
cFileImage* cFileImage::Create(char const * diffuse_name, char const * self_illumination_name, char const *emblem_fname,
							   const char* skin_color_name,
							Color4c skin_color, sRectangle4f* emblem_position, float emblem_angle)
{
	cFileImage* pMainImage=0;
	cFileImage* pSelfIlluminationImage=0;
	cFileImage* pEmblemImage=0;
	cFileImage* pSkinImage=0;


    pMainImage = Create(diffuse_name);
	if (!pMainImage)
		return 0;

	if(self_illumination_name==0 && emblem_fname==0 && skin_color.a==0)
		return pMainImage;

	if(pMainImage->load(diffuse_name))
		{
			delete pMainImage;
			return 0;
		}

	if(self_illumination_name && self_illumination_name[0])
		pSelfIlluminationImage = Create(self_illumination_name);
	if (pSelfIlluminationImage != 0)
        if (pSelfIlluminationImage->load(self_illumination_name))
		{
			delete pMainImage;
			delete pSelfIlluminationImage;
			return 0;
		}
	if(emblem_fname && emblem_fname[0])
		pEmblemImage = Create(emblem_fname);
	if (pEmblemImage != 0)
		if(pEmblemImage->load(emblem_fname))
		{
			delete pMainImage;
			delete pSelfIlluminationImage;
			delete pEmblemImage;
			return 0;
		}

	if(skin_color_name && skin_color_name[0] && strcmp(skin_color_name,diffuse_name)!=0)
		pSkinImage=Create(skin_color_name);
	if(pSkinImage)
		if(pSkinImage->load(skin_color_name))
		{
			delete pMainImage;
			delete pSelfIlluminationImage;
			delete pEmblemImage;
			delete pSkinImage;
			return 0;
		}


	return new cCompositeImage(pMainImage,pSelfIlluminationImage,pEmblemImage,pSkinImage,skin_color,emblem_position,emblem_angle);
}
void cFileImage::InitFileImage()
{
	cAVIImage::Init();
}
void cFileImage::DoneFileImage()
{
	cAVIImage::Done();
}

////////////////////// Реализация cAviScaleFileImage ///////////////////////////

cAviScaleFileImage::cAviScaleFileImage():cFileImage()
{
	dat = 0;
	maxMipLevels = 1;
}
/*
bool cAviScaleFileImage::Init(const char* fName)
{
	char name[1024];
	strcpy(name,fName);
	bool ret = true;
	cFileImage* FileImage=cFileImage::Create(name);
	if (FileImage&&FileImage->load(fName)!=-1&&FileImage->GetX())
	{
		dx = FileImage->GetX();
		dy = FileImage->GetY();
		int rdx = dx+2;
		int rdy = dy+2;
		int t1 = dx+2;
		int t2 = dy+2;
		int t3 = FileImage->GetLength();
		time = FileImage->GetTime();
		GetDimTexture(t1,t2,t3);//change parameters
		if (t3 < FileImage->GetLength())
		{
//			VisError<<"Слишком много кадров!!!\r\n"<<"Попытка создать - "<<FileImage->GetLength()
//				<<",а доступно - "<<t3<<" кадров\r\n"<<fName<<VERR_END;
			n_count = t3;
		}else
		{
			n_count = FileImage->GetLength();
		}
		x = t1;
		y = t2;
		const int x_count = x/rdx;

		delete[] dat;
		dat = new UINT[x*y*n_count];
		if (dat)	
		{
			UINT* lpBuf = new  UINT[dx*dy];
			if (lpBuf) 
			{
				for(int i=0;i<n_count;++i)
				{
					FileImage->GetTexture(lpBuf,i,dx,dy);
					const int offset_y = rdy*(i/x_count)*this->x + rdx*(i%x_count);
					for(int y=0; y<dy; ++y)
					{
						UINT* p = dat + (int)((y+1)*x) + offset_y;
						memcpy(p+1, lpBuf + (int)(y*dx), dx*sizeof(*lpBuf));
						*(p) = lpBuf[(int)(y*dx)];
						*(p+(int)dx+1)= lpBuf[(int)(y*dx+dx-1)];
					}
					memcpy(dat + offset_y, dat + (int)(x) + offset_y, (dx+2)*sizeof(*lpBuf));
					memcpy(dat + (int)((dy+1)*x) + offset_y, dat + (int)(dy*x) + offset_y, (dx+2)*sizeof(*lpBuf));
				}
				delete[] lpBuf;
			}else ret = false;
		}else ret = false;
	}else ret = false;

	delete FileImage;
	return ret;
}
/*/
bool cAviScaleFileImage::Init(const char* fName)
{
	char name[1024];
	strcpy(name,fName);
	bool ret = true;
	cFileImage* FileImage=cFileImage::Create(name);
	if (FileImage&&FileImage->load(fName)!=-1&&FileImage->GetX())
	{

		dx = FileImage->GetX();
		dy = FileImage->GetY();
		time = FileImage->GetTime();

		cTextureAtlas atlas;
		vector<Vect2i> texture_size(FileImage->GetLength(),Vect2i(dx,dy));
		if(!atlas.Init(texture_size))
		{
//			VisError<<"Слишком много кадров!!!\r\n"<<"Попытка создать - "<<FileImage->GetLength()
//				<<",а доступно - "<<t3<<" кадров\r\n"<<fName<<VERR_END;
//			n_count = t3;
			xassert(0);
		}
		maxMipLevels = atlas.GetMaxMip();
		n_count=atlas.GetNumTextures();
		x = atlas.GetDX();
		y = atlas.GetDY();

		delete[] dat;
		DWORD* lpBuf = new  DWORD[dx*dy];
		positions.resize(n_count);
		sizes.resize(n_count);
		for(int i=0;i<n_count;++i)
		{
			FileImage->GetTexture(lpBuf,i,dx,dy);
			atlas.FillTexture(i,lpBuf,dx,dy);
			positions[i]=atlas.GetUV(i);
			sizes[i]=atlas.GetSize(i);
//			if(i==40)
//				SaveTga("data.tga",dx,dy,(BYTE*)lpBuf,4);
		}
		delete[] lpBuf;

		int num_point=atlas.GetDX()*atlas.GetDY();
		dat = new DWORD[num_point];
		memcpy(dat,atlas.GetTexture(),num_point*4);
	}else 
		ret = false;

	delete FileImage;
	return ret;
}
/**/
cAviScaleFileImage::~cAviScaleFileImage()
{
	delete[] dat;
}

int cAviScaleFileImage::GetTexture(void *pointer,int time, int xSize,int ySize)
{
	xassert(dat!=0);
//	cFileImage_GetFrame(pointer,bppDst,bplDst,rc,rs,gc,gs,bc,bs,xDst,yDst,
//				dat,4,GetX()*4,8,16,8,8,8,0,GetX(),-y);
	xassert(sizeof(*dat)==4);
	memcpy(pointer,dat,x*y*sizeof(*dat));
	return 0; 
}

//////////////////////////////////////////////////////////////////////

#define BILIEAR_AND_ALPHABLEND

void BikeRotateBilinearAndApply(BYTE* buffer,int dx,int dy,int pitch,
								BYTE* in_buffer,int in_dx, int in_dy,int in_pitch,
								int in_offsetx,int in_offsety,float angle,bool center)
{
	int shift=8;
	int mul=1<<shift;
	int ix=round(cos(angle)*mul);
	int iy=round(sin(angle)*mul);
	in_offsetx*=mul;
	in_offsety*=mul;
	if(center)
	{
		in_offsetx-=(ix*in_dx/2+iy*in_dy/2);
		in_offsety-=(-iy*in_dx/2+ix*in_dy/2);
	}

	//xx=( x*ix+y*iy>>shift)+in_offsetx;Прямая
	//yy=(-x*iy+y*ix>>shift)+in_offsety;
	int offx=-(in_offsetx*ix-in_offsety*iy)>>shift;
	int offy=-(in_offsetx*iy+in_offsety*ix)>>shift;
	//x=(xx*ix-yy*iy+offx)>>shift
	//y=(xx*iy+yy*ix+offx)>>shift

	int minx,miny,maxx,maxy;
	{
		int x[4]={0,in_dx,   0,in_dx};
		int y[4]={0,    0,in_dy,in_dy};

		for(int i=0;i<4;i++)
		{
			int xx=(( x[i]*ix+y[i]*iy+in_offsetx)>>shift);
			int yy=((-x[i]*iy+y[i]*ix+in_offsety)>>shift);
			if(i==0)
			{
				minx=maxx=xx;
				miny=maxy=yy;
			}else
			{
				minx=min(minx,xx);
				maxx=max(maxx,xx);
				miny=min(miny,yy);
				maxy=max(maxy,yy);
			}
		}

		minx=max(minx,0);
		maxx=min(maxx+1,dx);
		miny=max(miny,0);
		maxy=min(maxy+1,dy);
	}


	for(int yy=miny;yy<maxy;yy++)
		for(int xx=minx;xx<maxx;xx++)
		{
			int x=xx*ix-yy*iy+offx;
			int y=xx*iy+yy*ix+offy;
			Color4c& out=*(Color4c*)(buffer+xx*4+yy*pitch);

			if(0)
			{//point filter
				x=x>>shift;y=y>>shift;
				if(x>=0 && x<in_dx && y>=0 && y<in_dy)
				{
					Color4c& in=*(Color4c*)(in_buffer+x*4+y*in_pitch);
					out=in;
				}
			}else
			{
				int cx=x&255,cy=y&255;
				x=x>>shift;y=y>>shift;
				if(x>=-1 && x<in_dx && y>=-1 && y<in_dy)
				{
					Color4c p00,p01,p10,p11;

#define GET(x,y) *(Color4c*)(in_buffer+(x)*4+(y)*in_pitch)
#define IS_CLAMP(x,y) !((x)>=0 && (x)<in_dx && (y)>=0 && y<in_dy)

					if(x==-1 || x==in_dx-1 || y==-1 || y==in_dy-1)
					{
						p00=GET(max(x,0)        ,max(y,0));        if(IS_CLAMP(x  ,y))  p00.a=0;
						p01=GET(min(x+1,in_dx-1),max(y,0));        if(IS_CLAMP(x+1,y))  p01.a=0;
						p10=GET(max(x,0)        ,min(y+1,in_dy-1));if(IS_CLAMP(x  ,y+1))p10.a=0;
						p11=GET(min(x+1,in_dx-1),min(y+1,in_dy-1));if(IS_CLAMP(x+1,y+1))p11.a=0;

					}else
					{
						p00=GET(x,y);
						p01=GET(x+1,y);
						p10=GET(x,y+1);
						p11=GET(x+1,y+1);
					}

#ifdef BILIEAR_AND_ALPHABLEND
					Color4c c;
#else
					Color4c& c=out;
#endif
					c.r=bilinear(p00.r,p01.r,p10.r,p11.r,cx,cy);
					c.g=bilinear(p00.g,p01.g,p10.g,p11.g,cx,cy);
					c.b=bilinear(p00.b,p01.b,p10.b,p11.b,cx,cy);
					c.a=bilinear(p00.a,p01.a,p10.a,p11.a,cx,cy);
#ifdef BILIEAR_AND_ALPHABLEND
					out.r=ByteInterpolate(out.r,c.r,c.a);
					out.g=ByteInterpolate(out.g,c.g,c.a);
					out.b=ByteInterpolate(out.b,c.b,c.a);
					out.a=min(int(c.a)+int(out.a),255);
#endif
				}
#undef IS_CLAMP
#undef GET
			}
		}
}

void AddEmblem(DWORD* buffer,int dx,int dy,DWORD* emblem_buffer,int emb_xshift, int emb_yshift, int edx, int edy)
{
	DWORD* cur=buffer;
	DWORD* emb_pos=emblem_buffer;
	for(int y=0;y<dy;y++)
	{
		for(int x=0;x<dx;x++,cur++)
		{
			if (emb_xshift > -1 && y<emb_yshift+edy && x<emb_xshift+edx && y>emb_yshift-1 && x>emb_xshift-1)
			{
				Color4c& c=*(Color4c*)cur;
				Color4c& e_c=*(Color4c*)emb_pos;
				c.r=ByteInterpolate(c.r,e_c.r,e_c.a);
				c.g=ByteInterpolate(c.g,e_c.g,e_c.a);
				c.b=ByteInterpolate(c.b,e_c.b,e_c.a);
				//c.a=255;
				emb_pos++;
			}
			
		}
	}
}

void AddSelfIllumination(DWORD* buffer,int dx,int dy,DWORD* lpSelf)
{
	DWORD* cur=buffer;
	DWORD* self=lpSelf;
	for(int y=0;y<dy;y++)
	{
		for(int x=0;x<dx;x++,cur++,self++)
		{
			Color4c& c=*(Color4c*)cur;
			Color4c& s=*(Color4c*)self;
			c.r=ByteInterpolate(c.r,s.r,s.a);
			c.g=ByteInterpolate(c.g,s.g,s.a);
			c.b=ByteInterpolate(c.b,s.b,s.a);
			//c.a=s.a;
		}
	}
}

void AddSkinColorAlpha(DWORD* buffer,DWORD* alpha_buffer,int dx,int dy,Color4c skin_color)
{
	DWORD* cur=buffer;
	DWORD* a_cur=alpha_buffer;
	for(int y=0;y<dy;y++)
	{
		for(int x=0;x<dx;x++,cur++,a_cur++)
		{
			Color4c& c=*(Color4c*)cur;
			Color4c& a_c=*(Color4c*)a_cur;
			c.r=ByteInterpolate(c.r,skin_color.r,a_c.a);
			c.g=ByteInterpolate(c.g,skin_color.g,a_c.a);
			c.b=ByteInterpolate(c.b,skin_color.b,a_c.a);
			c.a=255;
		}
	}
}

void AddSkinColorColor(DWORD* buffer,DWORD* alpha_buffer,int dx,int dy,Color4c skin_color)
{
	DWORD* cur=buffer;
	DWORD* a_cur=alpha_buffer;
	for(int y=0;y<dy;y++)
	{
		for(int x=0;x<dx;x++,cur++,a_cur++)
		{
			Color4c& c=*(Color4c*)cur;
			Color4c& a_c=*(Color4c*)a_cur;
			c.r=ByteInterpolate(c.r,skin_color.r,a_c.r);
			c.g=ByteInterpolate(c.g,skin_color.g,a_c.r);
			c.b=ByteInterpolate(c.b,skin_color.b,a_c.r);
		}
	}
}

cCompositeImage::cCompositeImage(cFileImage* pMainImage_,
			cFileImage* pSelfIlluminationImage_,
			cFileImage* pEmblemImage_,
			cFileImage* pSkinImage_,
		Color4c skin_color_, sRectangle4f* logo_position_,float angle)
{
	pMainImage = pMainImage_;
	pSelfIlluminationImage = pSelfIlluminationImage_;
	pEmblemImage = pEmblemImage_;
	pSkinImage = pSkinImage_;
	xassert(!pMainImage->IsCompositeFileImage());
	if(pSelfIlluminationImage)
		xassert(!pSelfIlluminationImage->IsCompositeFileImage());
	if(pEmblemImage)
		xassert(!pEmblemImage->IsCompositeFileImage());
	

	x = pMainImage->GetX();
	y = pMainImage->GetY();
	fmt = pMainImage->GetFmt();
	time = pMainImage->GetTime();
	length = pMainImage->GetLength();
	bpp = pMainImage->GetBitPerPixel();
	skin_color = skin_color_;
	logo_pos = 0;
	if (logo_position_)
	{
		if (logo_position_->min.x<logo_position_->max.x && logo_position_->min.y<logo_position_->max.y)
			logo_pos = logo_position_;
		logo_angle = angle;
	}
}

int cCompositeImage::GetTexture(void *pointer,int time, int xSize,int ySize)
{
	if (pMainImage == 0)//Если основная текстура не загрузилась, возвращаем 0
		return 0;
	//Суем в буффер изображение основной текстуры
	pMainImage->GetTexture(pointer,time,xSize,ySize);

	if(skin_color.a==255)
	{
		if(pSkinImage)
		{
			unsigned int *Alpha = new unsigned int [xSize*ySize];
			memset(Alpha,0xFF,xSize*ySize*sizeof(Alpha[0]));
			pSkinImage->GetTexture(Alpha,time, xSize, ySize);
			AddSkinColorColor((DWORD*)pointer,(DWORD*)Alpha,xSize,ySize,skin_color);
			delete Alpha;
		}else
		if(pMainImage->GetBitPerPixel()== 32)
		{
			unsigned int *Alpha = new unsigned int [xSize*ySize];
			memset(Alpha,0xFF,xSize*ySize*sizeof(Alpha[0]));
			pMainImage->GetTexture(Alpha,time, xSize, ySize);
			AddSkinColorAlpha((DWORD*)pointer,(DWORD*)Alpha,xSize,ySize,skin_color);
			delete Alpha;
		}
	}


	if (pEmblemImage != 0 && logo_pos != 0)
	{		//Если эмблема загрузилась, то создаем буффер, суем в него эмблему и накладываем ее на основную текстуру
			int edx = (int)((logo_pos->max.x -logo_pos->min.x)*xSize);
			int edy = (int)((logo_pos->max.y -logo_pos->min.y)*ySize);
			int minx= (int)((logo_pos->min.x+(logo_pos->max.x -logo_pos->min.x)*.5f)*xSize);
			int miny= (int)((logo_pos->min.y+(logo_pos->max.y -logo_pos->min.y)*.5f)*ySize);

	
			int edxy = edx*edy;
			unsigned int *lpBuf = new unsigned int [edxy];
			memset(lpBuf,0xFF,edxy*sizeof(lpBuf[0]));
			pEmblemImage->GetTexture(lpBuf,time,edx,edy);
			//AddEmblem((DWORD*)pointer,xSize,ySize,(DWORD*)lpBuf,minx,miny,edx,edy);
			//float angle=0;
			BikeRotateBilinearAndApply((BYTE*)pointer,xSize,ySize,xSize*4,
				(BYTE*)lpBuf,edx,edy,edx*4,minx,miny,logo_angle, true);

			delete lpBuf;
	}

	if (pSelfIlluminationImage != 0)
	{		//Если самосвечение загрузилось, то создаем буффер, суем в него самосвечение и накладываем ее на основную текстуру
			unsigned int *lpBuf = new unsigned int [xSize*ySize];
			memset(lpBuf,0xFF,xSize*ySize*sizeof(lpBuf[0]));
			pSelfIlluminationImage->GetTexture(lpBuf,time,xSize,ySize);
			AddSelfIllumination((DWORD*)pointer,xSize,ySize,(DWORD*)lpBuf);
			delete lpBuf;
	}

	return 1;
}

cCompositeImage::~cCompositeImage()
{
	delete pMainImage;
	delete pSelfIlluminationImage;
	delete pEmblemImage;
}
