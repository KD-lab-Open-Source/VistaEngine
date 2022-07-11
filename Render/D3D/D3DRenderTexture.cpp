#include "StdAfxRD.h"
#include "FileImage.h"
#include "D3DRender.h"
#include "VisGeneric.h"

static bool visDebugMipMapColors=false;
static int visDebugMipMapForce=0;


IDirect3DTexture9* cD3DRender::CreateSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute)
{

	if((TextureFormat==SURFMT_COLOR || TextureFormat==SURFMT_COLORALPHA) && 
		Option_FavoriteLoadDDS
		//&& MipMap!=1 
		&& !(attribute&TEXTURE_NODDS)
		)
	{
		IDirect3DTexture9* lpTexture=0;
		HRESULT hr = D3DDevice_->CreateTexture(x,y,MipMap,0,
			(TextureFormat==SURFMT_COLOR)?D3DFMT_DXT1:D3DFMT_DXT5,
			D3DPOOL_MANAGED,&lpTexture,0);

		xassert(hr==D3D_OK);

		xassert(lpTexture);
		return lpTexture;
	}

	return CreateUnpackedSurface(x,y,TextureFormat,MipMap,enable_assert,attribute);
}
IDirect3DTexture9* cD3DRender::CreateUnpackedSurface(int x,int y,eSurfaceFormat TextureFormat,int MipMap,bool enable_assert,DWORD attribute)
{
	IDirect3DTexture9* lpTexture=0;

	xassert(x&&y&&TextureFormat>=0&&TextureFormat<SURFMT_NUMBER);
	int Usage=0;
	D3DPOOL Pool=D3DPOOL_MANAGED;
	if(TextureFormat==SURFMT_RENDERMAP16 || TextureFormat==SURFMT_RENDERMAP32 || TextureFormat==SURFMT_RENDERMAP_FLOAT || TextureFormat==SURFMT_R32F)
	{
		Usage=D3DUSAGE_RENDERTARGET;
		Pool=D3DPOOL_DEFAULT;
		xassert(MipMap==1);
	}

	if(attribute&TEXTURE_DYNAMIC)
	{
		xassert(attribute&TEXTURE_D3DPOOL_DEFAULT);
		Pool=D3DPOOL_DEFAULT;
		Usage|=D3DUSAGE_DYNAMIC;
	}else
	if(attribute&TEXTURE_D3DPOOL_DEFAULT)
	{
		Pool=D3DPOOL_DEFAULT;
	}

	HRESULT hr=D3DDevice_->CreateTexture(x,y,MipMap,Usage,TexFmtData[TextureFormat],Pool,&lpTexture,0);
	if(FAILED(hr))
	{
		if(enable_assert)
		{
			xassert(0 && "Cannot create texture");
		}
		return 0;
	}
	xassert(lpTexture);
	return lpTexture;
}

void ApplySkinColor(DWORD* buffer,int dx,int dy,Color4c skin_color,DWORD* alpha_buffer = 0)
{
	DWORD* cur=buffer;
	for(int y=0;y<dy;y++)
	{
		for(int x=0;x<dx;x++,cur++)
		{
			Color4c& c=*(Color4c*)cur;
			c.r=ByteInterpolate(c.r,skin_color.r,c.a);
			c.g=ByteInterpolate(c.g,skin_color.g,c.a);
			c.b=ByteInterpolate(c.b,skin_color.b,c.a);
			c.a=255;
		}
	}
}

void ApplySelfIllumination(DWORD* buffer,int dx,int dy,cFileImage *FileImageSelfIllumination)
{
	DWORD *lpSelf= new DWORD[dx*dy];
	memset(lpSelf,0xFF,dx*dy*sizeof(lpSelf[0]));
	FileImageSelfIllumination->GetTexture(lpSelf,0,dx,dy);

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
			c.a=s.a;
		}
	}

	delete lpSelf;
}

void FillMip(int mip,unsigned int* in_buf,int dx,int dy)
{
	Color4c* buf=(Color4c*)in_buf;
	Color4c colors[]=
	{
		Color4c(0,255,0),
		Color4c(0,0,255),
		Color4c(255,255,0),
		Color4c(255,0,0),
		Color4c(255,255,255),
	};
	int size=sizeof(colors)/sizeof(colors[0]);
	if(mip>=size)
		mip=size-1;
	Color4c col=colors[mip];
	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		buf->r=col.r;
		buf->g=col.g;
		buf->b=col.b;
		buf++;
	}
}

int cD3DRender::CreateTexture(cTexture *Texture,cFileImage *FileImage,int dxout,int dyout,bool enable_assert)
 { // только создает в памяти поверхности 
	D3DFORMAT &tfd = TexFmtData[Texture->format()];

	if(Texture->GetWidth()>dwSuportMaxSizeTextureX) Texture->SetWidth(dwSuportMaxSizeTextureX);
	if(Texture->GetHeight()>dwSuportMaxSizeTextureY) Texture->SetHeight(dwSuportMaxSizeTextureY);
	
	int dx=Texture->GetWidth(),dy=Texture->GetHeight();
	int dxy=dx*dy;

	int dx_surface,dy_surface;

	bool pow2up = (Texture->format()==SURFMT_COLOR || Texture->format()==SURFMT_COLORALPHA) && !Texture->getAttribute(TEXTURE_NODDS);

	if(dxout<0)
		dx_surface=dxout=pow2up?Power2up(dx):dx;
	else
		dx_surface=Power2up(dxout);

	if(dyout<0)
		dy_surface=dyout=pow2up?Power2up(dy):dy;
	else
		dy_surface=Power2up(dyout);


	xassert((dx==dxout && dy==dyout)||(Texture->mipmapNumber()==1) );
	bool resample=!(dx==dxout && dy==dyout);
	bool is_alpha_test=false;
	bool is_alpha_blend=false;

	if(Texture->mipmapNumber()>1 && (Texture->mipmapNumber()>Texture->GetX() || Texture->mipmapNumber()>Texture->GetY()))
		Texture->setMipmapNumber(min(Texture->GetX(),Texture->GetY())+1);

	int force_mipmap = visDebugMipMapForce;
	if(force_mipmap>Texture->mipmapNumber())
		force_mipmap=Texture->mipmapNumber();

	for(int i=0;i<Texture->frameNumber();i++){
		unsigned int *lpBuf=0;
		if(FileImage){
			lpBuf = new unsigned int [dxy];
			memset(lpBuf,0xFF,dxy*sizeof(lpBuf[0]));
			FileImage->GetTexture(lpBuf,i,dx,dy);
			
			if(Texture->isAlpha() || Texture->isAlphaTest()){// загрузка только прозрачности
				int num_0=0,num_alpha=0;
				for(int i=0; i < dxy; i++){
					BYTE a=lpBuf[i]>>24;
					if(a==0)
						num_0++;
					else
					if(a!=255)
						num_alpha++;
				}

				if(num_alpha>0)
					is_alpha_blend=true;
				else if(num_0>0)
					is_alpha_test=true;
			}

			if(/*!is_skin &&*/ Texture->frameNumber()==1){
				if(is_alpha_test && !is_alpha_blend){
					Texture->clearAttribute(TEXTURE_ALPHA_BLEND);
					Texture->setAttribute(/*TEXTURE_MIPMAP_POINT_ALPHA|*/TEXTURE_ALPHA_TEST);
				}
				if(!is_alpha_test && !is_alpha_blend)
					Texture->clearAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST);
			}
		}

		if(Texture->GetDDSurface(i))
			Texture->GetDDSurface(i)->Release(); 
		if(Texture->BitMap[i]==0){
			int dx=dx_surface,dy=dy_surface;
			if(force_mipmap>1){
				int shift=force_mipmap-1;
				dx=dx>>shift;
				dy=dy>>shift;
			}

			IDirect3DTexture9* pTex=CreateSurface(
						dx,dy,
						Texture->format(),
						force_mipmap?1:Texture->mipmapNumber(),
						enable_assert,
						Texture->getAttribute());
			Texture->BitMap[i] = pTex;
		}

		if(Texture->BitMap[i]==0) 
			return 2;
		if(FileImage==0) continue;

		if(Texture->getAttribute(TEXTURE_BUMP)){
			bool is_height_map=true;
			for( int n=0; n<dxy; n++ ){//Если всё чёрно белое, то это высоты
				Color4c c=((Color4c*)lpBuf)[n];
				if( c.r!=c.g || c.r!=c.b ){
					is_height_map=false;
					break;
				}
			}
			
			if(is_height_map)
				ConvertDot3(lpBuf,dx,dy,1.0e-2f*current_bump_scale);
			else
				ConvertBumpRGB_UVW(lpBuf,dx,dy);
		}

		if(Texture->getAttribute(TEXTURE_SPECULAR))
			FixSpecularPower(lpBuf,dx,dy);

		RECT rect={0,0,dx,dy};

		IDirect3DTexture9* lpD3DTexture=Texture->GetDDSurface(i);

		RECT rect_out={0,0,dxout,dyout};
		bool fill_mipmap=visDebugMipMapColors;
		fill_mipmap=fill_mipmap && (Texture->mipmapNumber()>1);
		if(fill_mipmap)
			FillMip(0,lpBuf,dx,dy);

		if(force_mipmap<=1){
			IDirect3DSurface9* lpSurface = 0;
			RDCALL(lpD3DTexture->GetSurfaceLevel( 0, &lpSurface ));

			RDCALL(D3DXLoadSurfaceFromMemory(lpSurface, 0, &rect_out, lpBuf, 
				Texture->getAttribute(TEXTURE_BUMP)?D3DFMT_V8U8:D3DFMT_A8R8G8B8,
				Texture->getAttribute(TEXTURE_BUMP)?(2*rect.right):(4*rect.right), 0, &rect, 
				(resample?D3DX_FILTER_TRIANGLE:D3DX_FILTER_POINT), 0));

			lpSurface->Release();
		}

		if(Texture->mipmapNumber()>1) // построение мип мапов
			for(int nMipMap=1;nMipMap<Texture->mipmapNumber();nMipMap++)
			{
				RECT rect={0,0,dx>>nMipMap,dy>>nMipMap};
				xassert(rect.right>0 && rect.bottom>0);
				RECT rect_out={0,0,dxout>>nMipMap,dyout>>nMipMap};
				unsigned int *lpBufNext = new unsigned int [rect.right*rect.bottom];

				BuildMipMap(rect.right,rect.bottom,4, 8*rect.right,lpBuf, 4*rect.right,lpBufNext, 
					Texture->getAttribute(TEXTURE_MIPMAP_POINT|TEXTURE_MIPMAP_POINT_ALPHA|TEXTURE_BUMP));
				if(fill_mipmap)
					FillMip(nMipMap,lpBufNext,rect.right,rect.bottom);

				if(force_mipmap==0 || force_mipmap-1==nMipMap){
					IDirect3DSurface9* lpSurface = 0;
					RDCALL( lpD3DTexture->GetSurfaceLevel( force_mipmap?0:nMipMap, &lpSurface) );
					RDCALL( D3DXLoadSurfaceFromMemory( lpSurface, 0, &rect_out,
						lpBufNext, 
						Texture->getAttribute(TEXTURE_BUMP)?D3DFMT_V8U8:D3DFMT_A8R8G8B8,
						Texture->getAttribute(TEXTURE_BUMP)?(2*rect.right):(4*rect.right), 0, &rect, 
						(resample?D3DX_FILTER_TRIANGLE:D3DX_FILTER_POINT)
						, 0 ));

					lpSurface->Release();
				}
				delete lpBuf; 
				lpBuf = lpBufNext;
			}
		delete lpBuf;
	}

	if(is_alpha_test && !is_alpha_blend){
		Texture->clearAttribute(TEXTURE_ALPHA_BLEND);
		Texture->setAttribute(/*TEXTURE_MIPMAP_POINT_ALPHA|*/TEXTURE_ALPHA_TEST);
	}

	Texture->AddToDefaultPool();
    return 0;
}

unsigned short ColorByNormalWORD(Vect3f n);

static void NormalsToSurface(IDirect3DSurface9* lpSurface,Vect3f* normals,int dx,int dy)
{
	D3DLOCKED_RECT rect;
	RDCALL(lpSurface->LockRect(&rect,0,0));
	for(int y=0;y<dy;y++)
	{
		DWORD* data=(DWORD*)(y*rect.Pitch+(char*)rect.pBits);
		Vect3f* nrm=normals+y*dx;
		for(int x=0;x<dx;x++)
		{
			Vect3f n=nrm[x];
			unsigned int cx=round(n.x*127);
			unsigned int cy=round(n.y*127);
			unsigned int cz=round(n.z*127);
			Color4c color(cz,cy,cx);
			data[x]=color.RGBA();
		}
	}

	RDCALL(lpSurface->UnlockRect());
}

void cD3DRender::BuildNormalMap(cTexture *Texture,Vect3f* normals)
{
	int dx=Texture->GetWidth();
	int dy=Texture->GetHeight();

	for(int i=0;i<Texture->frameNumber();i++)
	{
		IDirect3DTexture9* lpD3DTexture=Texture->GetDDSurface(i);
		IDirect3DSurface9* lpSurface = 0;
		RDCALL( lpD3DTexture->GetSurfaceLevel( 0, &lpSurface ) );
		NormalsToSurface(lpSurface,normals,dx,dy);

		Vect3f* cur=normals;
		if(Texture->mipmapNumber()>1) // построение мип мапов
			for(int nMipMap=1;nMipMap<Texture->mipmapNumber();nMipMap++)
			{
				IDirect3DSurface9* lpSurfaceNext = 0;
				RDCALL( lpD3DTexture->GetSurfaceLevel( nMipMap, &lpSurfaceNext ) );
				RECT rect={0,0,dx>>nMipMap,dy>>nMipMap};
				Vect3f *next=new Vect3f[rect.right*rect.bottom];

				int cur_dx=rect.right*2;
				for(int y=0;y<rect.bottom;y++)
				for(int x=0;x<rect.right;x++)
				{
					int xx=x*2,yy=y*2;
					Vect3f& p00=cur[xx+yy*cur_dx];
					Vect3f& p01=cur[xx+1+yy*cur_dx];
					Vect3f& p10=cur[xx+(yy+1)*cur_dx];
					Vect3f& p11=cur[xx+1+(yy+1)*cur_dx];
					Vect3f sum=p00+p01+p10+p11;
					sum.normalize();
					next[x+y*rect.right]=sum;
				}

				NormalsToSurface(lpSurfaceNext,next,rect.right,rect.bottom);
				if(nMipMap>1)
					delete[] cur;
				lpSurface->Release();
				cur = next;
				lpSurface = lpSurfaceNext;
			}
		delete[] cur;
		lpSurface->Release();
	}

	IDirect3DTexture9* lpD3DTexture=Texture->GetDDSurface(0);
//	RDCALL(D3DXSaveTextureToFile("bump.dds",D3DXIFF_DDS,lpD3DTexture,0));

}

int cD3DRender::CreateCubeTexture(class cTexture *Texture)
{
	bool rendertarget=Texture->getAttribute(TEXTURE_RENDER32|TEXTURE_RENDER16);

	IDirect3DCubeTexture9 *pCubeTexture=0;
	if(FAILED(D3DDevice_->CreateCubeTexture(Texture->GetWidth(),
		1,
		rendertarget?D3DUSAGE_RENDERTARGET:0,
		D3DFMT_A8R8G8B8,
		rendertarget?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,
		&pCubeTexture,
		0
	  )))
		return 1;

	IDirect3DTexture9* pTexture=(IDirect3DTexture9*)pCubeTexture;
	Texture->BitMap[0] = pTexture;
	Texture->setAttribute(TEXTURE_CUBEMAP);
	return 0;
}


int cD3DRender::CreateBumpTexture(class cTexture *Texture)
{
	IDirect3DTexture9* lpTexture=0;
	RDCALL(D3DDevice_->CreateTexture(Texture->GetWidth(),Texture->GetHeight(),
					1,0,D3DFMT_V8U8 ,D3DPOOL_MANAGED,&lpTexture,0))
	Texture->BitMap[0] = lpTexture;
	return 0;
}

int cD3DRender::CreateTextureU16V16(class cTexture *Texture,bool defaultpool)
{
	IDirect3DTexture9* lpTexture=0;
	RDCALL(D3DDevice_->CreateTexture(Texture->GetWidth(),Texture->GetHeight(),
		1,0,D3DFMT_V16U16 ,defaultpool?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,&lpTexture,0))
	Texture->BitMap[0] = lpTexture;
	return 0;
}


int cD3DRender::DeleteTexture(cTexture *Texture)
{ // только освобождает в памяти поверхности 
	for(int nFrame=0;nFrame<Texture->frameNumber();nFrame++)
		if(Texture->GetDDSurface(nFrame)) 
		{
			int ret=Texture->GetDDSurface(nFrame)->Release();
			Texture->GetDDSurface(nFrame)=0;
		}
	return 0;
}
bool cD3DRender::SetScreenShot(const char *fname)
{
	IDirect3DSurface9* lpRenderSurface=0;
	RDCALL(D3DDevice_->GetRenderTarget(0,&lpRenderSurface));
	HRESULT hr=D3DXSaveSurfaceToFile(fname,D3DXIFF_BMP,lpRenderSurface,0,0);
	//HRESULT hr=D3DXSaveSurfaceToFile(fname,D3DXIFF_PNG,lpRenderSurface,0,0);
	
	RELEASE(lpRenderSurface);
	return SUCCEEDED(hr);
}

void* cD3DRender::LockTexture(class cTexture *Texture,int& Pitch)
{
	start_timer_auto();
	D3DLOCKED_RECT d3dLockRect;
	IDirect3DTexture9* lpSurface=Texture->GetDDSurface(0);
	RDCALL(lpSurface->LockRect(0,&d3dLockRect,0,0));

	Pitch=d3dLockRect.Pitch;
	return d3dLockRect.pBits;
}

void* cD3DRender::LockTexture(class cTexture *Texture,int& Pitch,Vect2i lock_min,Vect2i lock_size)
{
	start_timer_auto();
	D3DLOCKED_RECT d3dLockRect;
	IDirect3DTexture9* lpSurface=Texture->GetDDSurface(0);
	RECT rc;
	rc.left=lock_min.x;
	rc.top=lock_min.y;
	rc.right=lock_min.x+lock_size.x;
	rc.bottom=lock_min.y+lock_size.y;

	RDCALL(lpSurface->LockRect(0,&d3dLockRect,&rc,0));

	Pitch=d3dLockRect.Pitch;
	return d3dLockRect.pBits;
}

void cD3DRender::UnlockTexture(cTexture *Texture)
{
	IDirect3DTexture9* lpSurface=Texture->GetDDSurface(0);
	RDCALL(lpSurface->UnlockRect(0));
}

unsigned int ColorByNormalRGBA(Vect3f n)//Возможно бамп покорежился, теперь в inv_light_dir вравильно вектор выставляется (раньше x,y были местами поменяны)
{
	unsigned int x=round(((n.x+1)*127.5f));
	unsigned int y=round(((n.y+1)*127.5f));
	unsigned int z=round(((n.z+1)*127.5f));

	return z+(y<<8)+(x<<16);
}

unsigned int ColorByNormalDWORD(Vect3f n)
{
/*
	unsigned int x=round(((n.x+1)*127.5f));
	unsigned int y=round(((n.y+1)*127.5f));
	unsigned int z=round(((n.z+1)*127.5f));

	return z+(x<<8)+(y<<16);
/*/
	int x=round(n.x*128.f);
	int y=round(n.y*128.f);
	int z=round(n.z*128.f);
	x=clamp(x,-128,+127);
	y=clamp(y,-128,+127);
	z=clamp(z,-128,+127);
	int ilen=x*x+y*y+z*z;
	xassert(ilen>=127*127*0.95f && ilen<127*127*1.05f);

	DWORD cx=(BYTE)x,cy=(BYTE)y,cz=(BYTE)z;

	return DWORD(cx)+(DWORD(cy)<<8)+(DWORD(cz)<<16);
/**/
}
Vect3f NormalByColorDWORD(DWORD d)
{
	Vect3f v;
	v.x = (char)(BYTE)((d) & 0xFF);
	v.y = (char)(BYTE)((d>> 8) & 0xFF);
	v.z = (char)(BYTE)((d>> 16) & 0xFF);
	v*= (1/127.5f);
	return v;
}

unsigned short ColorByNormalWORD(Vect3f n)
{
	int x=round(n.x*128.f);
	int y=round(n.y*128.f);
	x=clamp(x,-128,+127);
	y=clamp(y,-128,+127);

	DWORD cx=(BYTE)x,cy=(BYTE)y;

	return DWORD(cx)+(DWORD(cy)<<8);
}
Vect3f NormalByColorWORD(unsigned short d)
{
	Vect3f v;
	v.x = (char)(BYTE)((d) & 0xFF);
	v.y = (char)(BYTE)((d>> 8) & 0xFF);
	v.x*= (1/127.5f);
	v.y*= (1/127.5f);
	v.z=1-v.x*v.x-v.y*v.y;
	v.z=clamp(v.z,0.0f,1.0f);
	v.z=sqrtf(v.z);
	return v;
}
/*
void cD3DRender::ConvertDot3(unsigned int* ibuf,int dx,int dy,float h_mul)
{
	const int byte_per_pixel=4;
	BYTE* buf=(BYTE*)ibuf;
#define GET(x,y) buf[(clamp(x,0,dx-1)+dx*clamp(y,0,dy-1))*byte_per_pixel]
	WORD* out=new WORD[dx*dy];

	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		Vect3f dl,dn,dsum(0,0,0);
		int h00=GET(x,y);
		int h01=GET(x+1,y);
		int h0m=GET(x-1,y);	
		int h10=GET(x,y+1);
		int hm0=GET(x,y-1);

		dl.set(0,1,(h01-h00)*h_mul);
		dn.set(0,-dl.z,dl.y);
		dsum+=dn.normalize();

		dl.set(0,1,(h00-h0m)*h_mul);
		dn.set(0,-dl.z,dl.y);
		dsum+=dn.normalize();

		dl.set(1,0,(h10-h00)*h_mul);
		dn.set(-dl.z,0,dl.x);
		dsum+=dn.normalize();

		dl.set(1,0,(h00-hm0)*h_mul);
		dn.set(-dl.z,0,dl.x);
		dsum+=dn.normalize();

		out[x+y*dx]=ColorByNormalWORD(dsum.normalize());
	}

	memcpy(buf,out,dx*dy*2);
	delete out;
#undef GET
}
/*/
void cD3DRender::ConvertDot3(unsigned int* ibuf,int dx,int dy,float h_mul)
{
	const int byte_per_pixel=4;
	BYTE* buf=(BYTE*)ibuf;
#define GET(x,y) buf[(clamp(x,0,dx-1)+dx*clamp(y,0,dy-1))*byte_per_pixel]
	WORD* out=new WORD[dx*dy];

	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		Vect3f dn,dsum(0,0,0);
		int h00=GET(x,y);
		int h01=GET(x+1,y);
		int h0m=GET(x-1,y);	
		int h10=GET(x,y+1);
		int hm0=GET(x,y-1);

		dn.set(-(h10-h00)*h_mul,-(h01-h00)*h_mul,1);
		dsum+=dn.normalize();

		dn.set((hm0-h00)*h_mul,(h0m-h00)*h_mul,1);
		dsum+=dn.normalize();

		out[x+y*dx]=ColorByNormalWORD(dsum.normalize());
	}

	memcpy(buf,out,dx*dy*2);
	delete out;
#undef GET
}
/**/

void cD3DRender::ConvertBumpRGB_UVW(unsigned int* buf,int dx,int dy)
{
	const int byte_per_pixel=4;
	WORD* out=new WORD[dx*dy];

	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		Color4c d=((Color4c*)buf)[x+y*dx];
//*
		d.r-=128;
		d.g-=128;
		d.b-=128;
/*/
		Vect3f r(d.r/255.0f*2-1,d.g/255.0f*2-1,d.b/255.0f*2-1);
		r.normalize();
		d.r=(char)(r.x*127.5f);
		d.g=(char)(r.y*127.5f);
		d.b=(char)(r.z*127.5f);
/**/

		//out[x+y*dx]=dout.RGBA();
		out[x+y*dx]=DWORD(d.g)+(DWORD(d.r)<<8);//a
		//out[x+y*dx]=DWORD(d.r)+(DWORD(d.g)<<8);//b
	}

	memcpy(buf,out,dx*dy*2);
	delete out;
}

void cD3DRender::FixSpecularPower(unsigned int* buf,int dx,int dy)
{
	for(int y=0;y<dy;y++)
	for(int x=0;x<dx;x++)
	{
		Color4c& d=((Color4c*)buf)[x+y*dx];
		if(d.a==0)
			d.a=1;
	}

}

bool cTexLibrary::IsDebugMipMapColor()
{
	return visDebugMipMapColors;
}

void cTexLibrary::DebugMipMapColor(bool enable)
{
	visDebugMipMapColors=enable;
	ReloadAllTexture();
}
void cTexLibrary::DebugMipMapForce(int mipmap_level)
{
	visDebugMipMapForce=mipmap_level;
	ReloadAllTexture();
}

int cTexLibrary::GetDebugMipMapForce()
{
	return visDebugMipMapForce;
}
