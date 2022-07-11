#include "StdAfxRD.h"
#include "xutil.h"

#include "Font.h"

#pragma comment(lib,"dxguid.lib")
//#pragma comment(lib,"ddraw.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"d3d9.lib")

FILE* fRD=NULL;

char* GetErrorText(HRESULT hr)
{
switch(hr)
{
case D3D_OK :
return "No error occurred. ";
case D3DERR_CONFLICTINGRENDERSTATE :
return "The currently set render states cannot be used together. ";
case D3DERR_CONFLICTINGTEXTUREFILTER :
return "The current texture filters cannot be used together. ";
case D3DERR_CONFLICTINGTEXTUREPALETTE :
return "The current textures cannot be used simultaneously. This generally occurs when a multitexture device requires that all palletized textures simultaneously enabled also share the same palette. ";
case D3DERR_DEVICELOST :
return "The device is lost and cannot be restored at the current time, so rendering is not possible. ";
case D3DERR_DEVICENOTRESET :
return "The device cannot be reset. ";
case D3DERR_DRIVERINTERNALERROR :
return "Internal driver error. ";
case D3DERR_INVALIDCALL :
return "The method call is invalid. For example, a method's parameter may have an invalid value. ";
case D3DERR_INVALIDDEVICE :
return "The requested device type is not valid. ";
case D3DERR_MOREDATA :
return "There is more data available than the specified buffer size can hold. ";
case D3DERR_NOTAVAILABLE :
return "This device does not support the queried technique. ";
case D3DERR_NOTFOUND :
return "The requested item was not found. ";
case D3DERR_OUTOFVIDEOMEMORY :
return "Direct3D does not have enough display memory to perform the operation. ";
case D3DERR_TOOMANYOPERATIONS :
return "The application is requesting more texture-filtering operations than the device supports. ";
case D3DERR_UNSUPPORTEDALPHAARG :
return "The device does not support a specified texture-blending argument for the alpha channel. ";
case D3DERR_UNSUPPORTEDALPHAOPERATION :
return "The device does not support a specified texture-blending operation for the alpha channel. ";
case D3DERR_UNSUPPORTEDCOLORARG :
return "The device does not support a specified texture-blending argument for color values. ";
case D3DERR_UNSUPPORTEDCOLOROPERATION :
return "The device does not support a specified texture-blending operation for color values. ";
case D3DERR_UNSUPPORTEDFACTORVALUE :
return "The device does not support the specified texture factor value. ";
case D3DERR_UNSUPPORTEDTEXTUREFILTER :
return "The device does not support the specified texture filter. ";
case D3DERR_WRONGTEXTUREFORMAT :
return "The pixel format of the texture surface is not valid. ";
case E_FAIL :
return "An undetermined error occurred inside the Direct3D subsystem. ";
case E_INVALIDARG :
return "An invalid parameter was passed to the returning function ";
//case E_INVALIDCALL :
//return "The method call is invalid. For example, a method's parameter may have an invalid value. ";
case E_OUTOFMEMORY :
return "Direct3D could not allocate sufficient memory to complete the call. ";
}
return "Unknown error";
};

void RDOpenLog(char *fname="RenderDevice.!!!")
{
	fRD=fopen(fname,"wt");
	fprintf(fRD,"----------------- Compilation data: %s time: %s -----------------\n",__DATE__,__TIME__);
}
int RDWriteLog(HRESULT err,char *exp,char *file,int line)
{
#ifndef _FINAL_VERSION_
	if(fRD==0) RDOpenLog();
	fprintf(fRD,"%s line: %i - %s = 0x%X , %s\n",file,line,exp,err,GetErrorText(err));
	fflush(fRD);
#endif
	return err;
}
void RDWriteLog(char *exp,int size)
{
#ifndef _FINAL_VERSION_
	if(fRD==0) RDOpenLog();
	if(size==-1)
		size=strlen(exp);
	fwrite(exp,size,1,fRD);
	fprintf(fRD,"\n");
	fflush(fRD);
#endif
}

void RDWriteLog(DDSURFACEDESC2 &ddsd)
{
#ifndef _FINAL_VERSION_
	if(fRD==0) RDOpenLog();
	fprintf(fRD,"DDSURFACEDESC2\n{\n");
	fprintf(fRD,"   dwSize            = %i\n",ddsd.dwSize);
	fprintf(fRD,"   dwFlags           = 0x%X\n",ddsd.dwFlags);
	fprintf(fRD,"   dwHeight          = %i\n",ddsd.dwHeight);;
	fprintf(fRD,"   dwWidth           = %i\n",ddsd.dwWidth);
	fprintf(fRD,"   lPitch            = %i\n",ddsd.lPitch);
	fprintf(fRD,"   dwBackBufferCount = %i\n",ddsd.dwBackBufferCount);
	fprintf(fRD,"   dwMipMapCount     = %i\n",ddsd.dwMipMapCount);
	fprintf(fRD,"   dwAlphaBitDepth   = %i\n",ddsd.dwAlphaBitDepth);
	fprintf(fRD,"   dwReserved        = %i\n",ddsd.dwReserved);
	fprintf(fRD,"   lpSurface         = 0x%X\n",(int)ddsd.lpSurface);
	fprintf(fRD,"   ddckCKDestOverlay = %i - %i\n",ddsd.ddckCKDestOverlay.dwColorSpaceLowValue,ddsd.ddckCKDestOverlay.dwColorSpaceHighValue);
	fprintf(fRD,"   ddckCKDestBlt     = %i - %i\n",ddsd.ddckCKDestBlt.dwColorSpaceLowValue,ddsd.ddckCKDestBlt.dwColorSpaceHighValue);
	fprintf(fRD,"   ddckCKSrcOverlay  = %i - %i\n",ddsd.ddckCKSrcOverlay.dwColorSpaceLowValue,ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue);
	fprintf(fRD,"   ddckCKSrcBlt      = %i - %i\n",ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,ddsd.ddckCKSrcBlt.dwColorSpaceHighValue);
/*
	DDPIXELFORMAT ddpfPixelFormat;
		DDSCAPS2      ddsCaps;
*/
	fprintf(fRD,"   dwTextureStage    = %i\n",ddsd.dwTextureStage);
	fprintf(fRD,"}\n");
	fflush(fRD);
#endif
}

cInterfaceRenderDevice::cInterfaceRenderDevice()
{ 
	NumberPolygon=0; 
	NumDrawObject=0;
	NumberTilemapPolygon=0;
	PtrNumberPolygon=&NumberPolygon;
	RenderMode=0;
	xScr=yScr=0;
	xScrMin=yScrMin=xScrMax=yScrMax=0;

	DrawNode=0;
	DefaultFont=CurrentFont=0;
	cur_render_window=NULL;
	global_render_window=NULL;
}

cInterfaceRenderDevice::~cInterfaceRenderDevice()
{ 
	RELEASE(global_render_window);
	VISASSERT(RenderMode==0);
	VISASSERT(xScr==0&&yScr==0&&xScrMin==0&&yScrMin==0&&xScrMax==0&&yScrMax==0);
	VISASSERT(xScr==0&&yScr==0);
	VISASSERT(CurrentFont==0 || CurrentFont==DefaultFont);
	VISASSERT(cur_render_window==NULL);
	VISASSERT(all_render_window.empty());
	gb_RenderDevice=NULL;
}

void cInterfaceRenderDevice::SetFont(cFont *pFont)
{
	MTG();
	CurrentFont=pFont;
	if(!CurrentFont)
		CurrentFont=DefaultFont;
}

void cInterfaceRenderDevice::SetDefaultFont(cFont *pFont)
{
	MTG();
	DefaultFont=pFont;
	if(!CurrentFont)
		CurrentFont=DefaultFont;
}

bool DeclarationHasEntry(IDirect3DVertexDeclaration9* declaration, BYTE type, BYTE usage)
{
	D3DVERTEXELEMENT9 elements[256];
	unsigned int elementsCount = 0;
	RDCALL(declaration->GetDeclaration(elements, &elementsCount));
	int result = 0;
	for(unsigned int i = 0; i < elementsCount; ++i) {
		if(elements[i].Type == type && elements[i].Usage == usage)
			return true;
	}
	return false;
}

int cD3DRender::GetSizeFromDeclaration(IDirect3DVertexDeclaration9* declaration)
{
	if(!declaration)
		return 0;

	D3DVERTEXELEMENT9 elements[256];
	memset(elements, 0, sizeof(elements));
	unsigned int elementsCount = 0;
	RDCALL(declaration->GetDeclaration(elements, &elementsCount));
	int result = 0;
	for(unsigned int i = 0; i < elementsCount; ++i) {
		switch(elements[i].Type) {
		case D3DDECLTYPE_FLOAT1:
			result += 4;
			break;
		case D3DDECLTYPE_FLOAT2:
			result += 8;
			break;
		case D3DDECLTYPE_FLOAT3:
			result += 12;
			break;
		case D3DDECLTYPE_FLOAT4:
			result += 16;
			break;
		case D3DDECLTYPE_UBYTE4:
			result += 4;
			break;
		case D3DDECLTYPE_D3DCOLOR:
			result += 4;
			break;
		case D3DDECLTYPE_UNUSED:
			result += 0;
			break;
		case D3DDECLTYPE_SHORT2:
			result += 4;
			break;
		case D3DDECLTYPE_SHORT4:
			result += 8;
			break;
		default:
			VISASSERT(0);
		}
	}
	return result;
}

int cD3DRender::GetSizeFromFmt(int fmt)
{
	int size=0;

	int position=fmt&D3DFVF_POSITION_MASK;
	switch(position)
	{
	case D3DFVF_XYZ:
		size+=3*sizeof(float);
		break;
	case D3DFVF_XYZRHW:
	case D3DFVF_XYZW:
		size+=4*sizeof(float);
		break;
	case D3DFVF_XYZB1:
		size+=4*sizeof(float);
		break;
	case D3DFVF_XYZB2:
		size+=5*sizeof(float);
		break;
	case D3DFVF_XYZB3:
		size+=6*sizeof(float);
		break;
	case D3DFVF_XYZB4:
		size+=7*sizeof(float);
		break;
	case D3DFVF_XYZB5:
		size+=8*sizeof(float);
		break;
	default:
//		xassert(0);
		;
	}

	if(fmt&D3DFVF_NORMAL) size+=3*sizeof(float);
	if(fmt&D3DFVF_DIFFUSE) size+=sizeof(unsigned int);
	if(fmt&D3DFVF_SPECULAR) size+=sizeof(unsigned int);

	size += ((fmt & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) * 2 * sizeof(float);
	for(int i=0;i<4;i++)
	{
		if((fmt&D3DFVF_TEXCOORDSIZE3(i))==D3DFVF_TEXCOORDSIZE3(i))
			size+=sizeof(float);
	}
	return size;
}

void BuildBumpMap(int xs,int ys,void *pSrc,void *pDst,int fmtBumpMap)
{
	int xm=xs-1,ym=ys-1;
	unsigned char *dst=(unsigned char*)pDst,*src=(unsigned char*)pSrc;
    for( int y=0; y<ys; y++ )
        for( int x=0; x<xs; x++ )
        {
			int v00 = src[4*(x+y*xs)+0];
            int v01 = src[4*(((x+1)&xm)+y*xs)+0];
            int vM1 = src[4*(((x-1)&xm)+y*xs)+0];
            int v10 = src[4*(x+((y+1)&ym)*xs)+0];
            int v1M = src[4*(x+((y-1)&ym)*xs)+0];
            int iDu = vM1-v01, iDv = v1M-v10;
            if( v00<vM1 && v00<v01 )
            {
                iDu = vM1-v00;
                if( iDu < v00-v01 ) iDu = v00-v01;
            }
            int uL = ( v00>1 ) ? 63 : 127;
            switch( fmtBumpMap )
            {
                case D3DFMT_V8U8:
                    *dst++ = (BYTE)iDu;
                    *dst++ = (BYTE)iDv;
                    break;

                case D3DFMT_L6V5U5:
                    *(WORD*)dst  = (WORD)( ( (iDu>>3) & 0x1f ) <<  0 ) 
						| (WORD)( ( (iDv>>3) & 0x1f ) <<  5 )
						| (WORD)( ( ( uL>>2) & 0x3f ) << 10 );
                    dst += 2;
                    break;

                case D3DFMT_X8L8V8U8:
                    *dst++ = (BYTE)iDu;
                    *dst++ = (BYTE)iDv;
                    *dst++ = (BYTE)uL;
                    *dst++ = (BYTE)0L;
                    break;
            }
        }
}
void BuildDot3Map(int xs,int ys,void *pSrc,void *pDst)
{ // трансформация карты цветов в карту нормалей
	float scale=255*2*0.1f;
	Vect3f TexNormal(0.5f*255,0.5f*255,0.5f*255);
	unsigned int *dst=(unsigned int *)pDst,*src=(unsigned int *)pSrc;
	int xm=xs-1,ym=ys-1;
	for(int j=0;j<ys;j++)
		for(int i=0;i<xs;i++)
		{
			unsigned int hl=src[((i-1)&xm)+j*xs]&255;
			unsigned int hr=src[((i+1)&xm)+j*xs]&255;
			unsigned int hd=src[i+((j-1)&ym)*xs]&255;
			unsigned int hu=src[i+((j+1)&ym)*xs]&255;
			Vect3f a(scale,0,hr-hl),b(0,scale,hu-hd);
			Vect3f n; n.cross(a,b); n.normalize(); 
			n=(n+Vect3f(1,1,1))*TexNormal;
			dst[i+j*xs]=(round(n.x)<<16)|(round(n.y)<<8)|(round(n.z)<<0);
		}
}

unsigned short ColorByNormalWORD(Vect3f n);
Vect3f NormalByColorWORD(WORD d);

void BuildMipMap(int x,int y,int bpp,int bplSrc,void *pSrc,int bplDst,void *pDst,
				 int Attr)
{
	char* Src=(char*)pSrc,*Dst=(char*)pDst;
	int ofsDst=bplDst-x*bpp, ofsSrc=bplSrc-2*x*bpp;

	if(Attr&TEXTURE_BUMP)
	{
		bpp=2;
		for(int j=0;j<y;j++,Src+=2*bpp*x)
		for(int i=0;i<x;i++,Dst+=bpp,Src+=2*bpp)
		{
			WORD p00=*((WORD*)&Src[0]),p01=*((WORD*)&Src[bpp]),
				p10=*((WORD*)&Src[bplSrc]),p11=*((WORD*)&Src[bplSrc+bpp]);
			Vect3f v=NormalByColorWORD(p00)+NormalByColorWORD(p01)+
					 NormalByColorWORD(p10)+NormalByColorWORD(p11);
			v.Normalize();
			WORD color=ColorByNormalWORD(v);
			*(WORD*)Dst=color;
		}
	}else
	{
		sColor4c color;
		for(int j=0;j<y;j++,Dst+=ofsDst,Src+=ofsSrc+bplSrc)
			for(int i=0;i<x;i++,Dst+=bpp,Src+=2*bpp)
			{

				if( Attr&TEXTURE_MIPMAP_POINT )
				{
					color=*((sColor4c*)&Src[0]);
				}else
				{ // bilinear/trilinear mipmap
					sColor4c p00=*((sColor4c*)&Src[0]),p01=*((sColor4c*)&Src[bpp]),
						p10=*((sColor4c*)&Src[bplSrc]),p11=*((sColor4c*)&Src[bplSrc+bpp]);
					int r=(int(p00.r)+p01.r+p10.r+p11.r)>>2;
					int g=(int(p00.g)+p01.g+p10.g+p11.g)>>2;
					int b=(int(p00.b)+p01.b+p10.b+p11.b)>>2;
					color.r=r;
					color.g=g;
					color.b=b;
					if(Attr&TEXTURE_MIPMAP_POINT_ALPHA)
					{
						color.a=p00.a;
					}else
					{
						int a=(int(p00.a)+p01.a+p10.a+p11.a)>>2;
						color.a=a;
					}
				}
				memcpy(Dst,&color,bpp);
			}
	}
	
}

cInterfaceRenderDevice *gb_RenderDevice=0;

cInterfaceRenderDevice* CreateIRenderDevice(bool multiThread)
{
	xassert(gb_VisGeneric==NULL);
	xassert(gb_RenderDevice==NULL);
	gb_VisGeneric=new cVisGeneric(multiThread);
	return gb_RenderDevice=new cD3DRender;
}

cRenderWindow::cRenderWindow(HWND hwnd_)
:hwnd(hwnd_)
{
	constant_size=false;
	size_x=size_y=-1;
	CalcSize();
}

void cRenderWindow::CalcSize()
{
	RECT rect;
	GetClientRect(hwnd,&rect);
	xassert(rect.left==0 && rect.top==0);
	size_x=rect.right;
	size_y=rect.bottom;
}

void cRenderWindow::ChangeSize()
{
	//if(constant_size)
	//	return;
	CalcSize();
	gb_RenderDevice->RecalculateDeviceSize();
}

cRenderWindow* cInterfaceRenderDevice::CreateRenderWindow(HWND hwnd)
{
	cRenderWindow* wnd=new cRenderWindow(hwnd);
	all_render_window.push_back(wnd);
	return wnd;
}

void cInterfaceRenderDevice::SelectRenderWindow(cRenderWindow* window)
{
	if(window)
	{
		cur_render_window=window;
	}else
	{
		cur_render_window=global_render_window;
	}
}

void cInterfaceRenderDevice::DeleteRenderWindow(cRenderWindow* wnd)
{
	vector<cRenderWindow*>::iterator it=find(all_render_window.begin(),all_render_window.end(),wnd);
	if(it==all_render_window.end())
	{
		VISASSERT(0 && "Bad RenderWindow");
		return;
	}
	all_render_window.erase(it);
}

int cInterfaceRenderDevice::GetSizeX()
{
	return cur_render_window->SizeX();
}

int cInterfaceRenderDevice::GetSizeY()
{
	return cur_render_window->SizeY();
}

cRenderWindow::~cRenderWindow()
{
	if(gb_RenderDevice)
		gb_RenderDevice->DeleteRenderWindow(this);
}

