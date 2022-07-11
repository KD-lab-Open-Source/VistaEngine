#include "StdAfxRD.h"
#include "FileImage.h"

cTexture::cTexture(const char *TexName)
{ 
	SetName(TexName);
	_x=_y=TimePerFrame=0; 
	number_mipmap=1;
	skin_color.set(255,255,255,0);
	format=SURFMT_BAD;
	TotalTime = 0;
	loaded = false;
}
cTexture::~cTexture()														
{ 
	VISASSERT(!GetAttribute(TEXTURE_NONDELETE));
	if(gb_RenderDevice)
	{
		cD3DRender* rd=(cD3DRender*)gb_RenderDevice;
		rd->DeleteTexture(this);
		rd->TexLibrary.DeleteFromDefaultPool(this);

	}else
		VISASSERT(0 && "Текстура удалена слишком поздно");
}

int cTexture::GetNumberMipMap()
{
	return number_mipmap;
}
void cTexture::SetNumberMipMap(int number)
{
	number_mipmap=number;
}


BYTE* cTexture::LockTexture(int& Pitch)
{
	xassert(GetAttribute(TEXTURE_NODDS));
	return (BYTE*)gb_RenderDevice3D->LockTexture(this,Pitch);
}

BYTE* cTexture::LockTexture(int& Pitch,Vect2i lock_min,Vect2i lock_size)
{
	xassert(GetAttribute(TEXTURE_NODDS));
	return (BYTE*)gb_RenderDevice3D->LockTexture(this,Pitch,lock_min,lock_size);
}

void cTexture::UnlockTexture()
{
	gb_RenderDevice3D->UnlockTexture(this);
}

IDirect3DTextureProxy*& cTexture::GetDDSurface(int n)
{
	return BitMap[n];
}

void cTexture::SetName(const char *Name)
{
	if(Name)name=Name;
	else name.clear();
}

void cTexture::SetWidth(int xTex)
{
	_x = xTex;
	//_x=ReturnBit(xTex);
	//if(GetWidth()!=xTex)
	//	VisError<<"Bad width in texture "<<name<<VERR_END;
}

void cTexture::SetHeight(int yTex)
{
	_y=yTex;
	//_y=ReturnBit(yTex);
	//if(GetHeight()!=yTex)
	//	VisError<<"Bad height in texture "<<name<<VERR_END;
}

int cTexture::GetX() const
{
	int bits=ReturnBit(_x);
	xassert(_x==(1<<bits));
	return bits;
}

int cTexture::GetY() const
{
	int bits=ReturnBit(_y);
	xassert(_y==(1<<bits));
	return bits;
}


void cTexture::CopyTexture(cTexture* pSource)
{
	IDirect3DSurface9 *pDest=NULL;
	IDirect3DSurface9 *pSrc=NULL;
	RDCALL(BitMap[0]->GetSurfaceLevel(0,&pDest));
	RDCALL(pSource->BitMap[0]->GetSurfaceLevel(0,&pSrc));

	
	RDCALL(D3DXLoadSurfaceFromSurface(pDest,NULL,NULL,pSrc,NULL,NULL,D3DX_FILTER_NONE,0));
	RELEASE(pDest);
	RELEASE(pSrc);
}

IDirect3DTextureProxy* ResizeCopy(int level,IDirect3DTextureProxy* pIn)
{
	if(pIn==NULL)
		return NULL;
	D3DSURFACE_DESC desc;
	RDCALL(pIn->GetLevelDesc(0,&desc));
	int number_mipmap=pIn->GetLevelCount();
	if(level>=number_mipmap || level==0 ||
		(desc.Width>>level)<4 || 
		(desc.Height>>level)<4
		)
	{
		pIn->AddRef();
		return pIn;
	}

	LPDIRECT3DTEXTURE9	lpD3DTextureDst=NULL;
	HRESULT hr = gb_RenderDevice3D->lpD3DDevice->CreateTexture(
		desc.Width>>level,
		desc.Height>>level,
		number_mipmap-level,
		desc.Usage,
		desc.Format,
		desc.Pool,&lpD3DTextureDst,NULL);
	xassert(hr==D3D_OK);

	for(int nMipMap=level;nMipMap<number_mipmap;nMipMap++)
	{
		LPDIRECT3DSURFACE9 lpSurfaceSrc = NULL;
		LPDIRECT3DSURFACE9 lpSurfaceDst = NULL;
		RDCALL( pIn->GetSurfaceLevel( nMipMap, &lpSurfaceSrc) );
		RDCALL( lpD3DTextureDst->GetSurfaceLevel( nMipMap-level, &lpSurfaceDst) );
		RDCALL( D3DXLoadSurfaceFromSurface(lpSurfaceDst,NULL,NULL,lpSurfaceSrc,NULL,NULL,D3DX_DEFAULT ,0));
		lpSurfaceSrc->Release();
		lpSurfaceDst->Release();
	}

	return CREATEIDirect3DTextureProxy(lpD3DTextureDst);
}

//Постирать внутри, и ResizeCopy использовать!
void cTexture::Resize(int level)
{
	if(level <= 0 || level >= number_mipmap || 
	   IsTexture2D() || GetAttribute(TEXTURE_DISABLE_DETAIL_LEVEL) ||
	   GetWidth() <= 128 || GetHeight() <= 128)
		return;

	number_mipmap=min(min(number_mipmap,GetX()),GetY());
	int mipCount = number_mipmap-level;
	int bitsx = GetX()-level;
	if (bitsx< 2) return;
	int bitsy = GetY()-level;
	if (bitsy< 2) return;
	SetWidth(1<<bitsx);
	SetHeight(1<<bitsy);
	for (int i=0; i<BitMap.size(); i++)
	{
		IDirect3DTextureProxy* pOut=ResizeCopy(level,BitMap[i]);
		BitMap[i]->Release();
		BitMap[i]=pOut;
	}
}

void cTexture::BuildNormalMap(Vect3f* normals)
{
	gb_RenderDevice3D->BuildNormalMap(this,normals);
}

void cTexture::SaveDDS(const char* file_name,int i,int level)
{
	VISASSERT(!BitMap.empty());

	IDirect3DTextureProxy* pOut=ResizeCopy(level,BitMap[i]);

	HRESULT hr = D3DXSaveTextureToFile(file_name,D3DXIFF_DDS,GETIDirect3DTexture(pOut),NULL);
	if (hr != DD_OK)
	{
		kdError("3d", XBuffer() < ("Ошибка при сохранении файла: ") < file_name < " Error: " <= hr);
	}
	RELEASE(pOut);
}
bool cTexture::LoadDDS(const char* file_name)
{
	VISASSERT(!BitMap.empty());
	char* buf=NULL;
	int size;

	if(!RenderFileRead(file_name,buf,size))
		return false;

	IDirect3DTexture9* pTex=gb_RenderDevice3D->CreateTextureFromMemory(buf,size,true);
	delete buf;
	if (pTex==NULL)
		return false;

	BitMap[0]=CREATEIDirect3DTextureProxy(pTex);
	return true;
}

int cTexture::GetBitsPerPixel()
{
	return GetTextureFormatSize(gb_RenderDevice3D->TexFmtData[GetFmt()]);
}

void cTextureAviScale::Init(class cAviScaleFileImage* pTextures)
{
	SetWidth(pTextures->GetX());
	SetHeight(pTextures->GetY());
	xassert(GetX()*GetY()!=0);
	pos=pTextures->positions;
	sizes = pTextures->sizes;
}

void cTextureAviScale::Init(vector<sRectangle4f>& complexTexturesUV)
{
	pos=complexTexturesUV;
}

int cTexture::CalcTextureSize()
{
	int byte_size=0;
	for(int nFrame=0;nFrame<GetNumberFrame();nFrame++)
	{
		IDirect3DTexture9* surface=GETIDirect3DTexture(GetDDSurface(nFrame));
		if(!surface)
			continue;
		int num_level=surface->GetLevelCount();
		for(int level=0;level<num_level;level++)
		{
			D3DSURFACE_DESC dsc;
			surface->GetLevelDesc( level, &dsc );
			int sz=GetTextureFormatSize(dsc.Format);
			int size=dsc.Width*dsc.Height*sz;
			byte_size += (size/8);
		}
	}
	return byte_size;
}

void cTexture::AddToDefaultPool()
{
	GetTexLibrary()->AddToDefaultPool(this);
}

void cTextureComplex::CloneTexture(cTextureComplex* pSource)
{
	SetAttribute(pSource->GetAttribute());
	SetWidth(pSource->GetWidth());
	SetHeight(pSource->GetHeight());
	SetTotalTime(pSource->GetTotalTime());
	SetNumberMipMap(pSource->GetNumberMipMap());
	xassert(textureNames.size()==pSource->GetFramesCount());
	pos.resize(pSource->GetFramesCount());
	sizes.resize(pSource->GetFramesCount());
	vector<string>& names = pSource->GetNames();
	for (int i=0; i<textureNames.size(); i++)
	{
		for(int j=0; j<names.size(); j++)
		{
			if (textureNames[i] == names[j])
			{
				pos[i] = pSource->pos[j];
				sizes[i] = pSource->sizes[j];
				break;
			}
		}
	}
	cloned = true;
}

#include "TextureAtlas.h"
