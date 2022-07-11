#include "StdAfxRD.h"
#include "Font.h"
#include "FileImage.h"
//#include "..\..\PluginMAX\src\StreamBuffer.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <direct.h>
#include "FileImage.h"

#ifndef ASSERT
#define ASSERT(x) VISASSERT(x)
#endif 

#include "..\gemsiii\filter.h"
//#include "..\..\Terra\tgai.h"
#include "FontInternal.h"

cTexture* cFont::GetTexture() const
{
	return pFont->pTexture;
}

float cFont::GetHeight() const
{
	return pFont->GetHeight();
}

float cFont::size() const
{
	return GetInternal()->FontHeight * GetInternal()->GetTexture()->GetHeight() * GetScale().y;
}

static char* cache_dir="cacheData\\Fonts";

class cFontImage
{
public:
	BYTE* ImageData;
	Vect2i size;
	int bits_per_pixel;

	cFontImage()
	{
		bits_per_pixel=8;
		ImageData=NULL;
	}
	virtual ~cFontImage()
	{
		delete[] ImageData;
	}

	//Если убрать &, то начинает падать в release в VC7.1 (Microsoft Visual C++ .NET   69586-335-0000007-18787) 
	virtual void Create(BYTE* gray_in,const Vect2i& size_)
	{
		size=size_;
		bits_per_pixel=8;

		if(ImageData)delete ImageData;
		int sz2=size.x*size.y;
		ImageData=new BYTE[sz2];
		for(int i=0;i<sz2;i++)
			ImageData[i]=gray_in[i];
	}

	bool Save(const char* fname);
	bool Load(const char* fname);
};

cFontInternal::cFontInternal()
{
	pTexture=NULL;
	pFontData=NULL;
	sizefontdata=0;

	Font.resize(256);
	FontHeight=0;
	statement_height=0;
	for(int i=0;i<256;i++)
		Font[i].set(0,0,0);

}
cFontInternal::~cFontInternal()
{
	RELEASE(pTexture);
}

#pragma pack(push,1)
	struct OneChar
	{
		int width;
		int real_width;
		BYTE* bits;
	};
#pragma pack(pop,1)

bool LoadFontImage(XBuffer& rd,int&  real_height,
	WORD& char_min,WORD& char_max,OneChar chars[256]
)
{
	char signature[4];

	rd.read(signature,4);
	if(signature[0]!='f' || signature[1]!='o' || 
		signature[2]!='n' || signature[3]!='t')
	{
		return false;
	}

	rd>real_height;
	rd>char_min;
	rd>char_max;

	VISASSERT(char_min<char_max);
	VISASSERT(char_min<=256);//VISASSERT(char_min>=0 && char_min<=256);
	VISASSERT(char_max<=256);//VISASSERT(char_max>=0 && char_max<=256);

	int i;
	for(i=0;i<256;i++)
	{
		chars[i].width=0;
		chars[i].bits=NULL;
	}

	for(i=char_min;i<char_max;i++)
	{
		OneChar& c=chars[i];
		rd.read(&c.width,4);
		c.real_width=(c.width+7)/8;
		int size_size=c.real_width*real_height;
		c.bits=new BYTE[size_size];
		rd.read(c.bits,size_size);
	}

	return true;
}

bool cFontInternal::CreateImage(LPCSTR filename,LPCSTR fontname,int height,class cFontImage* FontImage)
{
	int  real_height;
	WORD char_min,char_max;
	OneChar chars[256];
	if(filename)
	{
		char* buf;
		int size;
		if(!RenderFileRead(filename,buf,size))
			return false;
		XBuffer rd(buf,size);
		if(!LoadFontImage(rd,real_height,char_min,char_max,chars))
			return false;
		delete buf;
	}else
	{
		XBuffer rd(pFontData,sizefontdata);
		if(!LoadFontImage(rd,real_height,char_min,char_max,chars))
			return false;
	}

	//Рассчитываем величину текстуры
	Vect2i sizes[]={
		Vect2i(128,128),
		Vect2i(256,128),
		Vect2i(256,256),
		Vect2i(512,256),
		Vect2i(512,512),
		Vect2i(1024,512),
		Vect2i(1024,1024),
	};
	const int sizes_size= sizeof(sizes)/sizeof(sizes[0]);
	Vect2i size(0,0);

	float mul=height/float(real_height);
	int yborder=max(round(2*mul),2);
	int i;
	int sz;
	for(sz=0;sz<sizes_size;sz++)
	{
		size=sizes[sz];

		int x=0,y=0;
		for(i=char_min;i<char_max;i++)
		{

			int dx=round(chars[i].width*mul+2);
			if(x+dx>size.x)
			{
				y+=(height+yborder);
				x=0;
			}

			x+=dx;
		}

		if(y+height+yborder<=size.y && x<=size.x)
			break;
	}

	if(sz>=sizes_size)
	{
		for(i=0;i<256;i++)
			delete chars[i].bits;
		return false;
	}

	FontHeight=height/float(size.y);

	//Создаём текстуру
	Vect2i real_size(round(size.x/mul),round(size.y/mul));
	BYTE* gray_in=new BYTE[real_size.x*real_size.y];
	memset(gray_in,0,real_size.x*real_size.y);

	int x=0,y=0;
	for(i=char_min;i<char_max;i++)
	{

		int w=chars[i].width;
		int dx=round(w*mul+2);
		if(x+dx>size.x)
		{
			y+=(height+yborder);
			x=0;
		}

		//Рисуем
		int realx=int(x/mul),realy=int(y/mul);
		for(int yy=0;yy<real_height;yy++)
		for(int xx=0;xx<w;xx++)
		{
			int cury=yy+realy;
			int curx=xx+realx;

			int bit=xx+yy*chars[i].real_width*8;
			int b=chars[i].bits[bit>>3]&(1<<(7-bit&7));

			if(curx<real_size.x && cury<real_size.y)
				gray_in[curx+cury*real_size.x]=b?255:0;
		}

		Font[i].x=x/float(size.x);
		Font[i].y=y/float(size.y);
		Font[i].z=(dx)/float(size.x);

		x+=dx;
	}

	for(i=0;i<256;i++)
		delete chars[i].bits;

	BYTE* gray_out=new BYTE[size.x*size.y];

//	TGAHEAD h;
//	h.save3layers("file_in.tga",real_size.x,real_size.y,gray_in,gray_in,gray_in);

	resample(gray_in,real_size.x,real_size.y,gray_out,size.x,size.y,'l');
	delete gray_in;
	
	FontImage->Create(gray_out,size);

//	h.save3layers("file.tga",size.x,size.y,gray_out,gray_out,gray_out);
	delete gray_out;
	
	if(fontname)
		Save(fontname,*FontImage);
	return true;
}

bool cFontInternal::CreateTexture(LPCSTR fontname,LPCSTR filename,int height)
{
	if(height>100)
	{
		return false;
	}

	if(pTexture)
		delete pTexture;
	pTexture=NULL;

/*
	height=14;//22;//18;
	filename="Arial";
*/
	cFontImage FontImage;
	if(!Load(fontname,FontImage))
	if(!CreateImage(filename,fontname,height,&FontImage))
	{
		return false;
	}

	int dx=FontImage.size.x,dy=FontImage.size.y;
	pTexture=GetTexLibrary()->CreateAlphaTexture(dx,dy);
	if(!pTexture)
		return false;

	int Pitch;
	BYTE* out=pTexture->LockTexture(Pitch);
	for(int y=0;y<dy;y++)
	{
		memcpy(out+Pitch*y,FontImage.ImageData+y*dx,dx);
	}
	pTexture->UnlockTexture();

	return pTexture?true:false;
}

bool cFontImage::Save(const char* fname)
{
	return SaveTga(fname,size.x,size.y,ImageData,bits_per_pixel/8);
}

bool cFontImage::Load(const char* fname)
{
	if(!fname)
		return false;
	int byte_per_pixel;
	if(!LoadTGA(fname,size.x,size.y,ImageData,byte_per_pixel))
		return false;
	if(byte_per_pixel!=1)
		return false;
	return true;
}

bool cFontInternal::Save(const char* fname,cFontImage& fnt)
{
	string ftga,ffont;
	ftga=cache_dir;ftga+='\\';
	ffont=cache_dir;ffont+='\\';
	ftga+=fname; ftga+=".tga";
	ffont+=fname;ffont+=".xfont";
	mkdir("cacheData");
	mkdir(cache_dir);

	if(!fnt.Save(ftga.c_str()))
		return false;
	int file=_open(ffont.c_str(),_O_WRONLY|_O_TRUNC|_O_CREAT|_O_BINARY,_S_IREAD|_S_IWRITE);
	if(file==-1)
		return false;

	DWORD size=Font.size();
	_write(file,&FontHeight,sizeof(FontHeight));
	_write(file,&size,sizeof(size));
	_write(file,&Font[0],size*sizeof(Vect3f));
	_close(file);
	return true;
}

bool cFontInternal::Load(const char* fname,cFontImage& fnt)
{
	if(!fname)
		return false;
	string ftga,ffont;
	ftga=cache_dir;ftga+='\\';
	ffont=cache_dir;ffont+='\\';
	ftga+=fname; ftga+=".tga";
	ffont+=fname;ffont+=".xfont";

	if(!fnt.Load(ftga.c_str()))
		return false;

	char* buf;
	int buf_size;
	if(!RenderFileRead(ffont.c_str(),buf,buf_size))
		return false;
	XBuffer xb(buf,buf_size);

	DWORD size=0;
	xb.read(&FontHeight,sizeof(FontHeight));
	xb.read(&size,sizeof(size));
	Font.resize(size);
	xb.read(&Font[0],size*sizeof(Vect3f));
	delete buf;
	return true;
}

void str_replace_slash(char* str)
{
	for(char* p=str;*p;p++)
		if(*p=='/' || *p=='\\')
			*p='_';
}

void str_add_slash(char* str)
{
	int len=strlen(str);
	if(len>0)
	{
		if(str[len-1]!='/' && str[len-1]!='\\')
		{
			str[len]='\\';
			str[len+1]=0;
		}
	}
}

bool cFontInternal::Create(LPCSTR fname,int h,bool silentErr)
{
	int ScreenY=gb_RenderDevice->GetSizeY();

	int height;
	if(h<0)
		height=ABS(h);
	else
		height=round((h*ScreenY)/768.0f);
	statement_height=h;

	char texture_name[_MAX_PATH];
	sprintf(texture_name,"%s-%i",fname,height);
	_strlwr(texture_name);
	str_replace_slash(texture_name);

	if(!CreateTexture(texture_name,fname,height))
	{
		if(!silentErr) VisError<<"Cannot load font: "<< fname <<VERR_END;
		return false;
	}

	font_name=fname;
	_strlwr((char*)font_name.c_str());
	return true;
}

bool cFontInternal::Create(void* pFontData_,int sizefontdata_,int h,bool silentErr)
{
	pFontData=pFontData_;
	sizefontdata=sizefontdata_;
	int ScreenY=gb_RenderDevice->GetSizeY();

	int height;
	if(h<0)
		height=ABS(h);
	else
		height=round((h*ScreenY)/768.0f);
	statement_height=h;

	if(!CreateTexture(NULL,NULL,height))
	{
		if(!silentErr) VisError<<"Cannot load font in memory"<<VERR_END;
		return false;
	}

	return true;
}

bool cFontInternal::Reload()
{
	string f=font_name;
	return Create(f.c_str(),GetStatementHeight());
}

float cFont::GetLength(const char *string)
{
	const cFontInternal* cf=GetInternal();
	float xOfs=0;
	float xOfsMax=0;
	float xSize = GetScale().x*GetTexture()->GetWidth();
	sColor4c diffuse(0,0,0,0);

	for(const char* str=string;*str;str++)
	{
		ChangeTextColor(str,diffuse);
		BYTE c=(unsigned char)*str;
		if(!c)break;

		if(c == '\n'){
			xOfsMax = max(xOfs, xOfsMax);
			xOfs = 0;
		}

		if(c<32)continue;
		xOfs+=xSize*cf->Font[c].z-1;
	}

	return max(xOfs, xOfsMax); 
}

float cFont::GetHeight(const char *string)
{
	const cFontInternal* cf=GetInternal();
	float str_height = GetScale().y*cf->FontHeight*(1<<cf->GetTexture()->GetY());
	float height = str_height;

	for(const char* str=string;*str;str++){
		BYTE c=(unsigned char)*str;
		if(!c)break;

		if(c == '\n')
			height += str_height;
	}

	return height;
}

float cFont::GetLineLength(const char *string)
{
	const cFontInternal* cf = GetInternal();
	float xOfs = 0;
	float xSize = GetScale().x*GetTexture()->GetWidth();

	sColor4c diffuse(0,0,0,0);

	for(const char* str=string;*str;str++)
	{
		ChangeTextColor(str,diffuse);
		BYTE c = (unsigned char)*str;
		if(!c || c == '\n')
			break;

		if(c < 32)
			continue;
		
		xOfs += xSize * cf->Font[c].z - 1;
	}

	return xOfs;
}

float cFont::GetCharLength(const char c)
{
	BYTE ch = (unsigned char)c;
	if(ch < 32 || ch == '\n')
		return 0.f;
	return GetScale().x * GetTexture()->GetWidth() * GetInternal()->Font[ch].z - 1;
}

#undef FromHex

inline int FromHex(char a)
{
	if(a>='0' && a<='9')
		return a-'0';
	if(a>='A' && a<='F')
		return a-'A'+10;
	if(a>='a' && a<='f')
		return a-'a'+10;
	return -1;
}
/*
	Синтаксис строки 
	string &FEAB89 string && fdsfsdgs
	&FEAB89 - меняет цвет символа
	&& - преобразуется к &
*/
void ChangeTextColor(const char* &str,sColor4c& diffuse)
{
	while(*str=='&')
	{
		if(str[1]=='&')
		{
			str++;
			return;
		}

		DWORD s=0;
		int i;
		for(i=1;i<=6;i++)
		{
			int a=FromHex(str[i]);
			if(a<0)return;
			s=(s<<4)+a;
		}

		diffuse.RGBA()&=0xFF000000;
		diffuse.RGBA()|=s;
		str+=i;
	}
}
