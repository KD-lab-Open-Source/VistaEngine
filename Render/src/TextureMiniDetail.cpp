#include "StdAfxRD.h"
#include "TextureMiniDetail.h"
#include "Render/3dx/Umath.h"
#include "Render/src/FileImage.h"
#include "FileUtils/FileUtils.h"
#include "D3DRender.h"

TextureMiniDetail::TextureMiniDetail(const char* textureName, int tileSize)
: cTexture(textureName)
{
	tileSize_ = tileSize;
}

bool TextureMiniDetail::reload()
{
	// A mini-detail texture that is already one. Everything below builds the tiled, normalized
	// image from a source .tga, and P2's worlds name exactly that -- but Maelstrom's ship the
	// built result beside the source, one per tile size ("D_Ground_001.tga" ->
	// "D_Ground_001_n8/_n16/_n32.dds"), and name the .dds in the world. cFileImage::Create
	// knows only .tga/.avi/.jpg, so the load below fails on those outright and the material
	// gets no detail texture at all. cTexture::reload already dispatches a .dds name to the
	// DDS decoder; take that route and skip the build.
	if(getExtention(name()) == "dds")
		return cTexture::reload();

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
	// D3DX built this; it went with D3D9. Create the texture through the device and hand it
	// the equalised pixels. Color4c is b,g,r,a in memory, which is the device's staging
	// order, so the rows copy straight in.
	//
	// The D3D path below builds the mip chain by hand, desaturating each level further than
	// the last (s *= (num_bit-i)/num_bit) so the tile loses its colour with distance and
	// only its luminance grain survives. The SDL device generates the chain on the GPU
	// instead; box-filtering noise converges on flat grey, which the shader's `detail - 0.5`
	// turns into nothing, so the grain still fades out -- it just keeps its hue on the way.
	//
	// In practice this path only runs on a texture-cache miss: cTexLibrary treats the
	// shipped cache as exported, so the game's own detail textures come back as pre-built
	// DDS (mips and all) through loadDDS, and reload() is never called for them.
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
}
