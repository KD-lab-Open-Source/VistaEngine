#include "stdafxTr.h"
#include "scalingEngine.h"

void scaleRGBAImage(unsigned long* srcBuf, Vect2s srcBufSize, unsigned long* dstBuf, Vect2s dstBufSize)
{
	Image isrc(srcBuf, srcBufSize);
	Image idst(dstBuf, dstBufSize);
	FilterScaling<BSplineFilter> flt;
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

/*
#include "tgai.h"
#include "typeinfo"
struct RT{
	RT(){
		TGAHEAD tgahead;
		if(tgahead.loadHeader("\\!#\\oni_theme2_1024.tga")){
			Image is(tgahead.Width,tgahead.Height);
			tgahead.load2RGBL(tgahead.Width, tgahead.Height, is.data);

			{
				FilterScaling<BSplineFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<BoxFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<TriangleFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			////////////////////
			{
				FilterScaling<FilterFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<BellFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<Lanczos3Filter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<MitchellFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			//////////////////////
			{
				FilterScaling<GaussianFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<HammingFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
				}
				out.saveRGBL(buf, id.xsize, id.ysize, id.data);
			}
			{
				FilterScaling<CBlackmanFilter> flt;
				Image id(200,200);
				flt.zoom(&is,&id);
				TGAHEAD out;
				XBuffer buf;
				buf < "\\!#\\" <= id.xsize < 'x' <= id.ysize < typeid(flt).name() < ".tga";
				for(int i=0; i<buf.tell(); i++)  {
					if(buf.address()[i]== '<' || buf.address()[i]=='>') buf.address()[i]='#';
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
*/

