#include "stdafxRD.h"
#include "TextureMiniDetail.h"
#include "Render/3dx/umath.h"
#include "Render/Src/FileImage.h"
#include "D3DRender.h"

TextureMiniDetail::TextureMiniDetail(const char* textureName, int tileSize)
: cTexture(textureName)
{
	tileSize_ = tileSize;
}

bool TextureMiniDetail::reload()
{
	cFileImage* fileImage = cFileImage::Create(name());
	if(!fileImage || fileImage->load(name())){
		VisError << "Cannot load tga file - " << name() << VERR_END;
		return 0;
	}

	sizeX_ = fileImage->GetX();
	sizeY_ = fileImage->GetY();
	in_data = new Color4c[sizeX_*sizeY_];
	fileImage->GetTexture(in_data, 0, sizeX_, sizeY_);
	if(!normalize()){
		xassertStr("Плохая мелкодетальная текстура: ", name());
		return false;
	}

	if(!buildDDS()){
		xassertStr("Плохая мелкодетальная текстура: ", name());
		return false;
	}

	delete fileImage;
	delete[] in_data;
	return true;
}

float TextureMiniDetail::GetBrightInterpolate(int x, int y)
{
	x = (x+sizeX_-tileSize_/2) % sizeX_;
	y = (y+sizeY_-tileSize_/2) % sizeY_;
	x = max(x,0);
	y = max(y,0);
	int xx = x/tileSize_;
	int yy = y/tileSize_;

	float b00 = GetBrightTile(xx,yy);
	float b01 = GetBrightTile(xx+1,yy);
	float b10 = GetBrightTile(xx,yy+1);
	float b11 = GetBrightTile(xx+1,yy+1);

	float fx = (x-xx*tileSize_)/(float)tileSize_;
	float fy = (y-yy*tileSize_)/(float)tileSize_;

	float b0 = CosInterpolate(b00, b01, fx);
	float b1 = CosInterpolate(b10, b11, fx);
	float b = CosInterpolate(b0, b1, fy);
	return b;
}

bool TextureMiniDetail::normalize()
{
	tileSize_ = min(min(tileSize_,sizeX_),sizeY_);

	if(!IsPositivePower2(sizeX_))
		return false;
	if(!IsPositivePower2(sizeY_))
		return false;
	if(!IsPositivePower2(tileSize_))
		return false;
	if((sizeX_%tileSize_!=0)||(sizeY_%tileSize_!=0))
		return false;

	brightSizeX_ = sizeX_/tileSize_;
	brightSizeY_ = sizeY_/tileSize_;
	brightData_=new float[brightSizeX_*brightSizeY_];

	//calc bright
	for(int y=0;y<sizeY_;y+=tileSize_)
		for(int x=0;x<sizeX_;x+=tileSize_){
			int ix,iy;
			int sum_r=0,sum_g=0,sum_b=0;
			float& bd=brightData_[(x/tileSize_)+(y/tileSize_)*brightSizeX_];
			bd = 0;
			for(iy=0;iy<tileSize_;iy++)
				for(ix=0;ix<tileSize_;ix++){
					float h,s,v;
					in_data[(y+iy)*sizeX_+x+ix].HSV(h,s,v);
					bd+=v;
				}

			bd /= tileSize_*tileSize_;
		}

	//apply bright
	for(int y=0;y<sizeY_;y+=tileSize_)
		for(int x=0;x<sizeX_;x+=tileSize_){
			int ix,iy;
			int sum_r=0,sum_g=0,sum_b=0;
			int xx=(x/tileSize_),yy=(y/tileSize_);

			for(iy=0;iy<tileSize_;iy++)
				for(ix=0;ix<tileSize_;ix++){
					Color4c& c = in_data[(y+iy)*sizeX_+x+ix];
					float fout = GetBrightInterpolate(x+ix,y+iy);

					fout=max(fout,0.05f);
					float mul=0.5f/fout;
					float h,s,v;
					c.HSV(h,s,v);
					c.setHSV(h,s,clamp(v*mul,0,1));
				}
		}

	delete brightData_;

	return true;
}

bool TextureMiniDetail::buildDDS()
{
#ifndef _WIN32
	// No D3DX off-Windows: create the texture through the cross-platform device and hand
	// it the equalised pixels. Color4c is b,g,r,a in memory, which is the device's staging
	// order, so the rows copy straight in.
	//
	// The D3D path below builds the mip chain by hand, desaturating each level further than
	// the last (s *= (num_bit-i)/num_bit) so the tile loses its colour with distance and
	// only its luminance grain survives. The SDL device generates the chain on the GPU
	// instead; box-filtering noise converges on flat grey, which the shader's `detail - 0.5`
	// turns into nothing, so the grain still fades out -- it just keeps its hue on the way.
	//
	// In practice this path only runs on a texture-cache miss: off-Windows cTexLibrary
	// treats the shipped cache as exported, so the game's own detail textures come back as
	// pre-built DDS (mips and all) through loadDDS, and reload() is never called for them.
	New(1);
	if(gb_RenderDevice->CreateTexture(this, 0, -1, -1) != 0)
		return false;

	int pitch = 0;
	BYTE* dst = LockTexture(pitch);
	if(!dst)
		return false;
	for(int y = 0; y < sizeY_; y++)
		memcpy(dst + y*pitch, in_data + y*sizeX_, sizeX_*sizeof(Color4c));
	UnlockTexture();
	return true;
#else
	New(1);
	HRESULT hr=gb_RenderDevice3D->D3DDevice_->CreateTexture(sizeX_,sizeY_,0,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&BitMap[0],0); // D3DPOOL_SCRATCH
	if(FAILED(hr))
		return false;

	IDirect3DTexture9* texture9 = BitMap[0];
	DWORD levels=texture9->GetLevelCount();
	Color4c* out_data = new Color4c[sizeX_*sizeY_];

	int num_bit=ReturnBit(tileSize_);

	for(int i=0;i<levels;i++){
		float smul = (num_bit-i)/(float)num_bit;
		smul=clamp(smul,0,1);

		for(int y=0;y<sizeY_;y++)
			for(int x=0;x<sizeX_;x++){
				Color4c& c=in_data[y*sizeX_+x];
				Color4c& cout=out_data[y*sizeX_+x];
				float h,s,v;
				c.HSV(h,s,v);
				cout.setHSV(h,s*smul,v);
			}

		LPDIRECT3DSURFACE9 lpSurface = 0;
		hr = texture9->GetSurfaceLevel(i, &lpSurface);
		if(FAILED(hr))
			return false;

		RECT rc;
		rc.left=0;
		rc.top=0;
		rc.right=sizeX_;
		rc.bottom=sizeY_;

		hr=D3DXLoadSurfaceFromMemory(lpSurface, 0, 0, out_data, D3DFMT_A8R8G8B8, 4*sizeX_, 0, &rc, D3DX_FILTER_TRIANGLE, 0);
		if(FAILED(hr))
			return false;
	}

	delete out_data;
	return true;
#endif
}
