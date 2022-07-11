#include "stdafxTr.h"
#include "tgai.h"
#include "serialization.h"

void TGAHEAD::save3layers(const char* fname,int sizeX,int sizeY,unsigned char* Ra,unsigned char* Ga,unsigned char* Ba)
{
	init();
	XStream ff(fname, XS_OUT);
	Width=(short)sizeX;
	Height=(short)sizeY;
	ff.write(this,sizeof(TGAHEAD));
	unsigned char *line = new unsigned char[sizeX*3],*p;
	register unsigned int i,j;
	for(j = 0; j<sizeY; j++){
		p = line;
		for(i = 0; i<sizeX; i++){
			*p++ = Ba[j*sizeX+i];
			*p++ = Ga[j*sizeX+i];
			*p++ = Ra[j*sizeX+i];
		}
		ff.write(line,sizeX*3);
	}
	ff.close();
	delete line;
}

void TGAHEAD::load3layers(const char* fname,int sizeX,int sizeY,unsigned char* Ra,unsigned char* Ga,unsigned char* Ba)
{
	init();
	XZipStream ff(fname, XS_IN);
	ff.read(this,sizeof(TGAHEAD));
	if( (Width!=sizeX) || (Height!=sizeY) ) return;
	unsigned char *line = new unsigned char[sizeX*3],*p;
	int ibeg,jbeg,iend,jend,ik,jk;
	if(ImageDescriptor&0x20) { jbeg=0; jend=vMap.V_SIZE; jk=1;}
	else { jbeg=vMap.V_SIZE-1; jend=-1; jk=-1;}
	if((ImageDescriptor&0x10)==0) { ibeg=0; iend=vMap.H_SIZE; ik=1;}
	else { ibeg=vMap.H_SIZE-1; iend=-1; ik=-1;}
	for(int j=jbeg; j!=jend; j+=jk){
		p = line;
		ff.read(line,vMap.H_SIZE*3);
		for(int i = ibeg; i!=iend; i+=ik){
			Ba[j*sizeX+i] = *p++;
			Ga[j*sizeX+i] = *p++;
			Ra[j*sizeX+i] = *p++;
		}
	}

	ff.close();
	delete line;
}


static XZipStream tgaFile(0);
bool TGAHEAD::loadHeader(const char* fname)
{
	init();
	if(!tgaFile.open(fname, XS_IN)) return 0;
	tgaFile.read(this,sizeof(TGAHEAD));
	return 1;
}

bool TGAHEAD::load2buf(unsigned char* buf, unsigned int sizeBuf)
{
	int byteInPixel;
	if(PixelDepth==8) byteInPixel=1;
	else if(PixelDepth==16) byteInPixel=2;
	else if(PixelDepth==24) byteInPixel=3;
	else if(PixelDepth==32) byteInPixel=4;
	else return 0;
	if(sizeBuf!=Height*Width*byteInPixel){
		xassert(0&&"Incorrect bufer size for tga bitmap");
		return 0;
	}

	unsigned char *line = new unsigned char[Width*byteInPixel],*p;
	register unsigned int i,j,k;
	int ibeg,jbeg,iend,jend,ik,jk;
	if(ImageDescriptor&0x20) { jbeg=0; jend=Height; jk=1;}
	else { jbeg=Height-1; jend=-1; jk=-1;}
	if((ImageDescriptor&0x10)==0) { ibeg=0; iend=Width; ik=1;}
	else { ibeg=Width-1; iend=-1; ik=-1;}
	for(j=jbeg; j!=jend; j+=jk){
		p = line;
		tgaFile.read(line, Width*byteInPixel);
		for(i = ibeg; i!=iend; i+=ik){
			for(k=0; k<byteInPixel; k++) {
				//*buf++=*p++;
				buf[j*Width*byteInPixel+i*byteInPixel+k]=*p++;
			}
		}
	}

	tgaFile.close();
	delete line;
	return 1;
}

bool TGAHEAD::savebuf2file(const char* fname, unsigned char* buf, unsigned int bufSizeInByte, Vect2s bufSize, int byteInPixel)
{
	if(bufSizeInByte!=bufSize.x*bufSize.y*byteInPixel){
		xassert(0&&"Incorrect bufer size for tga header");
		return 0;
	}
	init();
	XStream ff(fname, XS_OUT);
	Width=(short)bufSize.x;
	Height=(short)bufSize.y;
	PixelDepth=24;
	switch(byteInPixel){
	case 1:
		PixelDepth=8;
		break;
	case 2:
		PixelDepth=16;
		break;
	case 3:
		PixelDepth=24;
		break;
	case 4:
		PixelDepth=32;
		break;
	default:
		xassert(0 && "Incorrect byteInPixel value!");
		return 0;
	}
	ImageType=2;
	ff.write(this,sizeof(TGAHEAD));
	ff.write(buf, bufSizeInByte);
	ff.close();
	return 1;
}

void TGAHEAD::load2RGBL(int sizeX,int sizeY, unsigned long* RGBLBuf)
{
	if( (Width!=sizeX) || (Height!=sizeY) ) return;
	unsigned char *line = new unsigned char[sizeX*3],*p;
	register unsigned int i,j;
	int ibeg,jbeg,iend,jend,ik,jk;
	if(ImageDescriptor&0x20) { jbeg=0; jend=sizeY; jk=1;}
	else { jbeg=sizeY-1; jend=-1; jk=-1;}
	if((ImageDescriptor&0x10)==0) { ibeg=0; iend=sizeX; ik=1;}
	else { ibeg=sizeX-1; iend=-1; ik=-1;}
	for(j=jbeg; j!=jend; j+=jk){
		p = line;
		tgaFile.read(line,sizeX*3);
		for(i = ibeg; i!=iend; i+=ik){
			unsigned long c= (*p++&0xFF);//<<16;
			c|= (*p++&0xFF)<<8;
			c|= (*p++&0xFF)<<16;
			RGBLBuf[j*sizeX+i]=c;
		}
	}

	tgaFile.close();
	delete line;
}

void TGAHEAD::saveRGBL(const char* fname,int sizeX,int sizeY, unsigned long* RGBLBuf)
{
	init();
	Width=(short)sizeX;
	Height=(short)sizeY;
	PixelDepth=24;
	ImageType=2;
	XBuffer buffer(sizeof(TGAHEAD) + sizeX*sizeY*3);
	buffer.write(this,sizeof(TGAHEAD));
	unsigned char *line = new unsigned char[sizeX*3],*p;
	register unsigned int i,j,k;
	k=0;
	for(j = 0; j<sizeY; j++){
		p = line;
		for(i = 0; i<sizeX; i++){
			*p++ = ((unsigned char*)(&RGBLBuf[k]))[0];
			*p++ = ((unsigned char*)(&RGBLBuf[k]))[1];
			*p++ = ((unsigned char*)(&RGBLBuf[k]))[2];
			k++;
		}
		buffer.write(line,sizeX*3);
	}
	delete line;
	saveFileSmart(fname, buffer, buffer.tell());
}

