#include "StdAfxRD.h"
#include "Texture.h"
#include "D3DRender.h"
#include "FileImage.h"
#include "Serialization\ResourceSelector.h"
#include "FileUtils/FileUtils.h"
#include <ddraw.h>

cTexture::cTexture(const char *TexName)
{ 
	name_ = TexName;
	sizeX_ = 0;
	sizeY_ = 0;
	TimePerFrame = 0; 
	mipmapNumber_=1;
	skin_color.set(255,255,255,0);
	format_ = SURFMT_BAD;
	TotalTime = 0;
	loaded = false;
	logoPosition_ = sRectangle4f::ID;
	logoAngle_ = 0;
}

cTexture::~cTexture()														
{ 
	xassert(!getAttribute(TEXTURE_NONDELETE));
	if(gb_RenderDevice)
	{
		cD3DRender* rd=(cD3DRender*)gb_RenderDevice;
		rd->DeleteTexture(this);
		rd->TexLibrary.DeleteFromDefaultPool(this);

	}else
		xassert(0 && "Текстура удалена слишком поздно");
}

bool cTexture::reload()
{
	Bitmaps::iterator it;
	FOR_EACH(BitMap,it)
		if(*it)
			(*it)->Release();
	BitMap.clear();

	xassert(name_.find_first_of("###") == string::npos);

	ExportInterface::export(name());
	ExportInterface::export(GetSelfIlluminationName());
	ExportInterface::export(logoName());
	ExportInterface::export(GetSkinColorName());

	if(getExtention(name()) == "dds")
		return reloadDDS();//Не слишком корректно работает, так как нет возможности узнать, полупрозрачная ли текстура.

	cFileImage* fileImage = createFileImage();
	if(!fileImage)
		return false;

	if(fileImage->GetBitPerPixel()==32)
		setAttribute(TEXTURE_ALPHA_BLEND);

	bool result = !gb_RenderDevice3D->CreateTexture(this, fileImage, -1, -1);
	delete fileImage;
	return result;
}

cFileImage* cTexture::createFileImage()
{
	if(IsTexture2D()){
		cFileImage* fileImage = cFileImage::Create(name());
		if(!fileImage || fileImage->load(name())){
			VisError << "Can`t load 2D texture: " << name() << VERR_END;
			return 0;
		}

		SetWidth(fileImage->GetX());
		SetHeight(fileImage->GetY());
		SetTimePerFrame(0);
		New(fileImage->GetLength());
		SetTotalTime(fileImage->GetTime());
		setAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);

		return fileImage;
	}
	else{
		cFileImage* fileImage = cFileImage::Create(name(), GetSelfIlluminationName(), logoName(), GetSkinColorName(), skin_color, &logoPosition_, logoAngle_);
		if(!fileImage)
			return false;
		if(fileImage->GetX() != 1<<ReturnBit(fileImage->GetX()) || fileImage->GetY() != 1<<ReturnBit(fileImage->GetY())){
			VisError << "Wrong width or height: " << name() << VERR_END;
			return 0;
		}

		New(fileImage->GetLength());
		if(fileImage->GetLength()<=1) 
			SetTimePerFrame(0);
		else
			SetTimePerFrame((fileImage->GetTime() - 1)/(fileImage->GetLength() - 1));
		SetTotalTime(fileImage->GetLength());
		SetWidth(fileImage->GetX());
		SetHeight(fileImage->GetY());
		setMipmapNumber(min(GetY(), GetX()) + 1);

		return fileImage;
	}
}

bool cTexture::reloadDDS()
{
	char* buf = 0;
	int size;

	if(!RenderFileRead(name(),buf,size))
		return false;

	DDSURFACEDESC2* ddsd = (DDSURFACEDESC2*)(1+(DWORD*)buf);
	if(ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP){
		IDirect3DCubeTexture9* cubeTexture = 0;
		if(!FAILED(D3DXCreateCubeTextureFromFileInMemory(gb_RenderDevice3D->D3DDevice_, buf, size, &cubeTexture))){
			BitMap.push_back((IDirect3DTexture9*)cubeTexture);
			setAttribute(TEXTURE_CUBEMAP);

			D3DSURFACE_DESC desc;
			RDCALL(cubeTexture->GetLevelDesc(0,&desc));
			SetWidth(desc.Width);
			SetHeight(desc.Height);
			return true;
		}
	}
	else{
		IDirect3DTexture9* texture9 = gb_RenderDevice3D->CreateTextureFromMemory(buf, size, !IsTexture2D());
		if(texture9){
			int mipmaps = texture9->GetLevelCount();
			setMipmapNumber(mipmaps);
			D3DSURFACE_DESC desc;
			RDCALL(texture9->GetLevelDesc(0,&desc));
			SetWidth(desc.Width);
			SetHeight(desc.Height);

			BitMap.push_back(texture9);
			return true;
		}
	}

	delete[] buf;
	return false;
}

BYTE* cTexture::LockTexture(int& Pitch)
{
	xassert(getAttribute(TEXTURE_NODDS));
	return (BYTE*)gb_RenderDevice3D->LockTexture(this,Pitch);
}

BYTE* cTexture::LockTexture(int& Pitch,Vect2i lock_min,Vect2i lock_size)
{
	xassert(getAttribute(TEXTURE_NODDS));
	return (BYTE*)gb_RenderDevice3D->LockTexture(this,Pitch,lock_min,lock_size);
}

void cTexture::UnlockTexture()
{
	gb_RenderDevice3D->UnlockTexture(this);
}

IDirect3DTexture9*& cTexture::GetDDSurface(int n)
{
	return BitMap[n];
}

void cTexture::SetWidth(int xTex)
{
	sizeX_ = xTex;
	//_x=ReturnBit(xTex);
	//if(GetWidth()!=xTex)
	//	VisError<<"Bad width in texture "<<name<<VERR_END;
}

void cTexture::SetHeight(int yTex)
{
	sizeY_=yTex;
	//_y=ReturnBit(yTex);
	//if(GetHeight()!=yTex)
	//	VisError<<"Bad height in texture "<<name<<VERR_END;
}

int cTexture::GetX() const
{
	int bits=ReturnBit(sizeX_);
	xassert(sizeX_==(1<<bits));
	return bits;
}

int cTexture::GetY() const
{
	int bits=ReturnBit(sizeY_);
	xassert(sizeY_==(1<<bits));
	return bits;
}

IDirect3DTexture9* ResizeCopy(int level,IDirect3DTexture9* pIn)
{
	if(pIn==0)
		return 0;
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

	IDirect3DTexture9*	lpD3DTextureDst=0;
	HRESULT hr = gb_RenderDevice3D->D3DDevice_->CreateTexture(
		desc.Width>>level,
		desc.Height>>level,
		number_mipmap-level,
		desc.Usage,
		desc.Format,
		desc.Pool,&lpD3DTextureDst,0);
	xassert(hr==D3D_OK);

	for(int nMipMap=level;nMipMap<number_mipmap;nMipMap++)
	{
		IDirect3DSurface9* lpSurfaceSrc = 0;
		IDirect3DSurface9* lpSurfaceDst = 0;
		RDCALL( pIn->GetSurfaceLevel( nMipMap, &lpSurfaceSrc) );
		RDCALL( lpD3DTextureDst->GetSurfaceLevel( nMipMap-level, &lpSurfaceDst) );
		RDCALL( D3DXLoadSurfaceFromSurface(lpSurfaceDst,0,0,lpSurfaceSrc,0,0,D3DX_DEFAULT ,0));
		lpSurfaceSrc->Release();
		lpSurfaceDst->Release();
	}

	return lpD3DTextureDst;
}

//Постирать внутри, и ResizeCopy использовать!
void cTexture::Resize(int level)
{
	if(level <= 0 || level >= mipmapNumber_ || 
	   IsTexture2D() || getAttribute(TEXTURE_DISABLE_DETAIL_LEVEL) ||
	   GetWidth() <= 128 || GetHeight() <= 128)
		return;

	mipmapNumber_=min(min(mipmapNumber_,GetX()),GetY());
	int mipCount = mipmapNumber_-level;
	int bitsx = GetX()-level;
	if (bitsx< 2) return;
	int bitsy = GetY()-level;
	if (bitsy< 2) return;
	SetWidth(1<<bitsx);
	SetHeight(1<<bitsy);
	for (int i=0; i<BitMap.size(); i++)
	{
		IDirect3DTexture9* pOut=ResizeCopy(level,BitMap[i]);
		BitMap[i]->Release();
		BitMap[i]=pOut;
	}
}

void cTexture::BuildNormalMap(Vect3f* normals)
{
	gb_RenderDevice3D->BuildNormalMap(this,normals);
}

void cTexture::saveDDS(const char* file_name, int level)
{
	xassert(!BitMap.empty());

	IDirect3DTexture9* pOut = ResizeCopy(level, BitMap[0]);

	HRESULT hr = D3DXSaveTextureToFile(file_name, D3DXIFF_DDS, pOut, 0);
	if(hr != DD_OK)
		kdError("3d", XBuffer() < "Ошибка при сохранении файла: " < file_name < " Error: " <= hr);
	
	RELEASE(pOut);
}

bool cTexture::loadDDS(const char* file_name)
{
	xassert(!BitMap.empty());
	char* buf=0;
	int size;

	if(!RenderFileRead(file_name,buf,size))
		return false;

	IDirect3DTexture9* pTex=gb_RenderDevice3D->CreateTextureFromMemory(buf,size,true);
	delete buf;
	if (pTex==0)
		return false;

	BitMap[0] = pTex;
	return true;
}

int cTexture::bitsPerPixel() const
{
	return GetTextureFormatSize(gb_RenderDevice3D->TexFmtData[format()]);
}

cFileImage* cTextureAviScale::createFileImage()
{
	cAviScaleFileImage* aviImages = new cAviScaleFileImage;
	if(!aviImages->Init(name())){
		VisError<<"Can`t load avi texture: " << name() << VERR_END;
		return 0;
	}
	Init(aviImages);
	SetTimePerFrame(aviImages->GetTime()/aviImages->GetFramesCount());
	SetTotalTime(aviImages->GetTime());
	setAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);
	New(1);
	if(IsTexture2D())
		setMipmapNumber(1);
	else
		setMipmapNumber(aviImages->GetMaxMipLevels());
	return aviImages;
}

void cTextureAviScale::Init(class cAviScaleFileImage* pTextures)
{
	SetWidth(pTextures->GetX());
	SetHeight(pTextures->GetY());
	xassert(GetX()*GetY()!=0);
	pos=pTextures->positions;
	sizes = pTextures->sizes;
}

int cTexture::CalcTextureSize()
{
	int byte_size=0;
	for(int nFrame=0;nFrame<frameNumber();nFrame++)
	{
		IDirect3DTexture9* surface = GetDDSurface(nFrame);
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

void cTexture::New(int number)						
{ 
	Bitmaps::iterator it;
	FOR_EACH(BitMap,it)
		if(*it)
			(*it)->Release();
	BitMap.resize(number); 
	for(unsigned i=0;i<BitMap.size();i++) 
		BitMap[i]=0; 
}

eSurfaceFormat cTexture::format() const
{ 
	if(format_ != SURFMT_BAD)
		return format_;
	if(getAttribute(TEXTURE_GRAY) && getAttribute(TEXTURE_ALPHA_BLEND))
		return SURFMT_L8;
	if(getAttribute(TEXTURE_RENDER_SHADOW_9700))
		return SURFMT_RENDERMAP_FLOAT;
	if(getAttribute(TEXTURE_RENDER16))
		return SURFMT_RENDERMAP16;
	if(getAttribute(TEXTURE_RENDER32))
		return SURFMT_RENDERMAP32;
	if(getAttribute(TEXTURE_32))
		return getAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST) ? SURFMT_COLORALPHA32 : SURFMT_COLOR32;
	if(getAttribute(TEXTURE_BUMP))
		return SURFMT_BUMP;
	if(getAttribute(TEXTURE_UVBUMP))
		return SURFMT_UV;
	if(getAttribute(TEXTURE_U16V16))
		return SURFMT_U16V16;
	if(getAttribute(TEXTURE_R32F))
		return SURFMT_R32F;

	return getAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_ALPHA_TEST) ? SURFMT_COLORALPHA : SURFMT_COLOR;
}

cTextureComplex::cTextureComplex(vector<string>& names)
{
	textureNames = names;
	placeLine = false;
}

cFileImage* cTextureComplex::createFileImage()
{
	cComplexFileImage* fileImage = new cComplexFileImage;
	if(!fileImage->Init(GetNames(),placeLine)){
		VisError << "Can`t load complex texture: " << name() << VERR_END;
		return 0;
	}
	Init(fileImage);
	SetTimePerFrame(0);
	New(1);
	setAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);
	if(IsTexture2D())
		setMipmapNumber(1);
	else
		setMipmapNumber(fileImage->GetMaxMipLevels());
	return fileImage;
}

cTextureScale::cTextureScale(const char* TexName /*= ""*/) :cTexture(TexName)
{
	scale.set(1,1);
	uvscale.set(1,1);
	inv_size.set(1,1);
}

cFileImage* cTextureScale::createFileImage()
{
	cFileImage* fileImage = cFileImage::Create(name());
	if(!fileImage || fileImage->load(name())){
		VisError << "Can`t load scale texture: " << name() <<VERR_END;
		return 0;
	}
	Vect2f scale = GetCreateScale();
	SetWidth(fileImage->GetX()*scale.x);
	SetHeight(fileImage->GetY()*scale.y);
	SetTimePerFrame(0);
	New(1);
	setMipmapNumber(1);
	return fileImage;
}
