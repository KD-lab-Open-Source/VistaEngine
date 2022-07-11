#include "stdafxTr.h"
#include "scalingEngine.h"
#include "terra.h"

struct ImageVx : public Image {
	unsigned short*	data;
	ImageVx(int _sx, int _sy, bool _autoRelease=true):Image(_sx,_sy,true){
		data=new unsigned short[xsize*ysize];
	}
	ImageVx(unsigned short* _data, Vect2s _size) : Image(_size.x,_size.y, false) {
		data=_data;
	}

	void resizeH(const ContribList& contribListH, ImageVx* tmpDst){
		int i, j, k;
		unsigned short* raster; //a row or column of pixels
		for(k = 0; k < tmpDst->ysize; ++k) {
			//get_row(raster, src, k);
			raster=&(data[k*xsize]);
			for(i = 0; i < tmpDst->xsize; ++i) {
				float z=0;
				unsigned short a=0;
				int maxContrib=contribListH.amountContrib(i);
				for(j = 0; j < maxContrib; ++j) {
					unsigned long c=raster[contribListH.contrib(i,j).pixel];
					const float& kf=contribListH.contrib(i,j).weight;
					z+=(float)Vm_extractHeigh(c)*kf;
					a = Vm_extractAtr(c);
				}
				long zi;
				zi = round(z);
				zi = clamp(zi, 0, MAX_VX_HEIGHT);
				tmpDst->data[i+ k*tmpDst->xsize]=Vm_prepHeighAtr(zi,a);
			}
		}
	}
	void resizeV(const ContribList& contribListV, ImageVx* dst){
		int i, j, k;
        unsigned short* tmpColumn = new unsigned short[ysize];
		for(k = 0; k < dst->xsize; ++k) {
			//get_column(raster, tmp, k);
			for(int m=0; m < ysize; m++) 
				tmpColumn[m]=data[m*xsize + k];
			for(i = 0; i < dst->ysize; ++i) {
				float z=0;
				unsigned short a=0;
				int maxContrib=contribListV.amountContrib(i);
				for(j = 0; j < maxContrib; ++j) {
					unsigned long c = tmpColumn[contribListV.contrib(i,j).pixel];
					const float& kf=contribListV.contrib(i,j).weight;
					z+=(float)Vm_extractHeigh(c)*kf;
					a = Vm_extractAtr(c);
				}
				long zi;
				zi = round(z);
				zi = clamp(zi, 0, MAX_VX_HEIGHT);
				dst->data[k + i*dst->xsize]=Vm_prepHeighAtr(zi,a);
			}
		}
		delete tmpColumn;
	}
	~ImageVx(){
		if(autoRelease)
			delete data;
	}
};

struct ImageGB : public Image {
	unsigned short*	data;
	ImageGB(int _sx, int _sy, bool _autoRelease=true):Image(_sx,_sy,true){
		data=new unsigned short[xsize*ysize];
	}
	ImageGB(unsigned short* _data, Vect2s _size) : Image(_size.x,_size.y, false) {
		data=_data;
	}

	void resizeH(const ContribList& contribListH, ImageGB* tmpDst){
		int i, j, k;
		unsigned short* raster; //a row or column of pixels
		for(k = 0; k < tmpDst->ysize; ++k) {
			//get_row(raster, src, k);
			raster=&(data[k*xsize]);
			for(i = 0; i < tmpDst->xsize; ++i) {
				unsigned short a=0;
				int maxContrib=contribListH.amountContrib(i);
				{
					j = maxContrib>>1;
					unsigned long c=raster[contribListH.contrib(i,j).pixel];
					a = (c);
				}
				tmpDst->data[i+ k*tmpDst->xsize]=a;
			}
		}
	}
	void resizeV(const ContribList& contribListV, ImageGB* dst){
		int i, j, k;
        unsigned short* tmpColumn = new unsigned short[ysize];
		for(k = 0; k < dst->xsize; ++k) {
			//get_column(raster, tmp, k);
			for(int m=0; m < ysize; m++) 
				tmpColumn[m]=data[m*xsize + k];
			for(i = 0; i < dst->ysize; ++i) {
				unsigned short a=0;
				int maxContrib=contribListV.amountContrib(i);
				{
					j = maxContrib>>1;
					unsigned long c = tmpColumn[contribListV.contrib(i,j).pixel];
					a = (c);
				}
				dst->data[k + i*dst->xsize] = a;
			}
		}
		delete tmpColumn;
	}
	~ImageGB(){
		if(autoRelease)
			delete data;
	}
};


void scaleRGBAImage(unsigned long* srcBuf, Vect2s srcBufSize, unsigned long* dstBuf, Vect2s dstBufSize)
{
	ImageL isrc(srcBuf, srcBufSize);
	ImageL idst(dstBuf, dstBufSize);
	FilterScaling<Lanczos3Filter> flt; //BSplineFilter
	flt.zoom(&isrc,&idst);
	//TGAHEAD out;
	//XBuffer buf;
	//buf < "\\!#\\" <= idst.xsize < 'x' <= idst.ysize < ".tga";
	//for(int i=0; i<buf.tell(); i++)  {
	//	if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
	//}
	////out.saveRGBL(buf, idst.xsize, idst.ysize, idst.data);
	//out.savebuf2file(buf, (unsigned char*)(idst.data), idst.xsize*idst.ysize*4, Vect2s(idst.xsize, idst.ysize), 4);
}
void scaleVxImage(unsigned short* srcBuf, Vect2s srcBufSize, unsigned short* dstBuf, Vect2s dstBufSize)
{
	ImageVx isrc(srcBuf, srcBufSize);
	ImageVx idst(dstBuf, dstBufSize);
	FilterScaling<Lanczos3Filter> flt; //BSplineFilter
	flt.zoom(&isrc,&idst);
}

void scaleGImage(unsigned short* srcBuf, Vect2s srcBufSize, unsigned short* dstBuf, Vect2s dstBufSize)
{
	ImageGB isrc(srcBuf, srcBufSize);
	ImageGB idst(dstBuf, dstBufSize);
	FilterScaling<Lanczos3Filter> flt; //BSplineFilter
	flt.zoom(&isrc,&idst);
}

void scaleCImage(unsigned char* srcBuf, Vect2s srcBufSize, unsigned char* dstBuf, Vect2s dstBufSize)
{
	ImageC isrc(srcBuf, srcBufSize);
	ImageC idst(dstBuf, dstBufSize);
	FilterScaling<Lanczos3Filter> flt; //BSplineFilter
	flt.zoom(&isrc,&idst);
}

/**
#include "tgai.h"
#include "typeinfo"
struct RT{
	RT(){
		TGAHEAD tgahead;
		if(tgahead.loadHeader("\\!#\\oni_theme2_1024.tga")){
			ImageL is(tgahead.Width,tgahead.Height);
			tgahead.load2RGBL(tgahead.Width, tgahead.Height, is.data);
			const int newXSize=200;
			const int newYSize=200;

			{
				FilterScaling<BSplineFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<BoxFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<TriangleFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			////////////////////
			{
				FilterScaling<FilterFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<BellFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<Lanczos3Filter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<MitchellFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			//////////////////////
			{
				FilterScaling<GaussianFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<HammingFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<CBlackmanFilter> flt;
				ImageL id(newXSize, newYSize);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.buffer()[i]== '<' || buf.buffer()[i]=='>') buf.buffer()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}


		}

		//Image is(10,10);
		//Image id(100,100);
		//flt.zoom(&is,&id);
		//scaleRGBAImage(0,Vect2s(10,10), 0, Vect2s(10,10));
	}
};
RT rt;
/**/

