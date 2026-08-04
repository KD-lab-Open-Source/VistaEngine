#ifndef __TEXTURE_H_INCLUDED__
#define __TEXTURE_H_INCLUDED__

#include "XMath/Rectangle4f.h"
#include "Render/inc/IVisGenericInternal.h"

enum eSurfaceFormat : int;
struct IDirect3DTexture9;
class cFileImage;

enum eAttributeTexture
{
	TEXTURE_NONDELETE		=	1<<4,
	TEXTURE_DYNAMIC			=   1<<5,    //�������� ����� ��������
	TEXTURE_ALPHA_BLEND		=	1<<6,	//  �������� �������� �����
	TEXTURE_DISABLE_DETAIL_LEVEL=1<<7, 
	TEXTURE_ALPHA_TEST		=	1<<8,	// �������� �������� ����� � �����
	TEXTURE_BUMP			=   1<<9,
	TEXTURE_NO_COMPACTED	=	1<<10,  // �������� �� ��������� ��� ������ GetTexLibrary()->Compact();
	TEXTURE_SPECULAR		=	1<<11,
	TEXTURE_MIPMAP_POINT	=	1<<18,		// ���������� ������� �������� ���������� �������
	TEXTURE_MIPMAP_POINT_ALPHA=	1<<19,		// ���������� ������� �������� ���������� ������� ������ ��� apha
	TEXTURE_R32F			=   1<<20,		// 32-bit float format 
	TEXTURE_RENDER16		=	1<<21,		// � �������� ���������� ������
	TEXTURE_RENDER32		=	1<<22,
	TEXTURE_ADDED_POOL_DEFAULT	=1<<23,
	TEXTURE_32				=	1<<24,		//������ 32 ������ ������
	TEXTURE_RENDER_SHADOW_9700 =1<<25,
	TEXTURE_D3DPOOL_DEFAULT =	1<<26,
	TEXTURE_GRAY			=   1<<27,
	TEXTURE_UVBUMP			=	1<<28,
	TEXTURE_U16V16			=	1<<29,
	TEXTURE_CUBEMAP			=	1<<30,
	TEXTURE_NODDS			=	1<<31,

};

class RENDER_API cTexture : public UnknownClass, public sAttribute
{	
public:
	Color4c	skin_color;
	typedef vector<IDirect3DTexture9*> Bitmaps;
	Bitmaps	BitMap;

	cTexture(const char* TexName = "");
	~cTexture();

	virtual bool reload();
	void New(int number);

	const char* name() const { return name_.c_str(); }
	void setName(const char* name) { name_ = name; }

	int frameNumber() const {return BitMap.size(); };

	int mipmapNumber() const { return mipmapNumber_; }
	void setMipmapNumber(int number) { mipmapNumber_ = number; }

	int GetTimePerFrame() const { return TimePerFrame; }
	void SetTimePerFrame(int tpf) { TimePerFrame = tpf; }
	void SetTotalTime(int time) {TotalTime = time;}
	int GetTotalTime() const {return TotalTime;}

	int GetX() const;
	int GetY() const;
	int GetWidth()const{return /*1<<GetX()*/sizeX_;};
	int GetHeight()const{return /*1<<GetY()*/sizeY_;};

	void SetWidth(int xTex);
	void SetHeight(int yTex);

	bool isAlpha() const { return getAttribute(TEXTURE_ALPHA_BLEND) ? true : false; }
	bool isAlphaTest() const { return getAttribute(TEXTURE_ALPHA_TEST) ? true : false; }

	/// Whether reload() multiplied this texture's RGB by its alpha. Only the DDS decoder
	/// does (Render/src/DDSImage.cpp, to stop bilinear filtering bleeding transparent-texel
	/// colour into opaque edges); a .tga or an .avi frame arrives with straight alpha. A
	/// renderer that blends (ONE, 1-SRC_ALPHA) has to know which of the two it was handed.
	bool isPremultiplied() const { return premultiplied_; }
	void setPremultiplied(bool on) { premultiplied_ = on; }

	virtual bool IsScaleTexture() const {return false;}
	virtual bool IsAviScaleTexture() const {return false;}
	virtual bool IsComplexTexture() const {return false;}
	virtual bool isMiniDetail() const { return false; }

	IDirect3DTexture9*& GetDDSurface(int n);
	
	eSurfaceFormat format() const;
	void setFormat(eSurfaceFormat format) { format_ = format; }
	int bitsPerPixel() const;

	BYTE* LockTexture(int& Pitch);
	BYTE* LockTexture(int& Pitch,Vect2i lock_min,Vect2i lock_size);
	void UnlockTexture();

	void BuildNormalMap(Vect3f* normals);
	void saveDDS(const char* file_name, int level);
	bool loadDDS(const char* file_name);

	void SetSelfIlluminationName(const char* s){if(s)self_illumination_name=s;else self_illumination_name.clear();}
	const char* GetSelfIlluminationName()const{return self_illumination_name.c_str();}

	void setLogo(const char* name, const sRectangle4f& position, float angle) { logoName_ = name; logoPosition_ = position; logoAngle_ = angle; }
	const char* logoName() const { return logoName_.c_str(); }

	void SetSkinColorName(const char* s){if(s)skin_color_name=s;else skin_color_name.clear();}
	const char* GetSkinColorName()const{return skin_color_name.c_str();}

	int CalcTextureSize();//������������ ������ �������� � ������.
	void AddToDefaultPool();
	void SetTexture2D() {is2DTexture = true;}
	void SetTexture3D() {is2DTexture = false;}
	bool IsTexture2D() const {return is2DTexture;}

	void Resize(int level);

	bool GetLoaded(){return loaded;}
	void SetLoaded(){loaded=true;}

	void SetUniqName(string name) { uniqName = name; }
	const string& GetUniqueName() {return uniqName;}

protected:
	string name_;
	int	sizeX_;
	int sizeY_;
	int	TimePerFrame;	
	int	mipmapNumber_;
	string self_illumination_name;
	string skin_color_name;
	string logoName_;
	sRectangle4f logoPosition_;
	float logoAngle_;
	string uniqName;
	eSurfaceFormat format_;
	bool is2DTexture;
	int TotalTime;
	bool loaded;
	bool premultiplied_ = false;

	void DeleteFromDefaultPool();
	bool reloadDDS();
	virtual cFileImage* createFileImage();
};

class cTextureAviScale : public cTexture
{
public:
	vector<sRectangle4f> pos;
	vector<Vect2i> sizes;
public:
	cTextureAviScale(const char *TexName = "") : cTexture(TexName){}
	void Init(class cAviScaleFileImage* pTextures);
	sRectangle4f& GetFramePos(float phase)
	{
		int i = int(phase*pos.size());
		i=clamp(i,0,(int)pos.size()-1);
		return pos[i];
	}
	sRectangle4f& GetFramePosInt(int phase)//float(1) -> int(65535)
	{
		//return pos[phase/(65535/(pos.size()-1))];//�������, ������� ������� � �������. ����������� �������!
		xassert(phase>=0 && phase<=65535);
		int i=(phase*pos.size())>>16;
		xassert(i>=0 && i<int(pos.size()));
		return pos[i];
	}

	const sRectangle4f& GetFramePosIndex(int index)
	{
		return pos[index];
	}

	int GetFramesCount(){return pos.size();}
	sRectangle4f& GetFramePos(int i)
	{
		xassert((UINT)i<pos.size());
		return pos[i];
	}
	bool IsAviScaleTexture() const {return true;}

protected:
	cFileImage* createFileImage();
}; 

class cTextureScale : public cTexture
{
	Vect2f scale;
	Vect2f uvscale;
	Vect2f inv_size;
public:
	cTextureScale(const char* TexName = "");

	Vect2f GetCreateScale(){return scale;};
	const Vect2f& GetUVScale() const {return uvscale;}
	bool IsScaleTexture() const {return true;}
	bool IsAviScaleTexture() const {return false;}

	void SetScale(Vect2f scale_,Vect2f uvscale_)
	{
		scale=scale_;
		uvscale=uvscale_;
		inv_size.x=1.0f/GetWidth();
		inv_size.y=1.0f/GetHeight();
	}

	void ConvertUV(float& u,float& v)
	{
		u*=uvscale.x;
		v*=uvscale.y;
	}

	void DXY2DUV(int dx,int dy,float& du,float& dv)
	{
		du=dx*inv_size.x;
		dv=dy*inv_size.y;
	}

private:
	cFileImage* createFileImage();
};

class cTextureComplex : public cTextureAviScale
{
public:
	cTextureComplex(vector<string>& names);;
	bool IsComplexTexture() const {return true;}
	bool IsAviScaleTexture() const {return false;}
	vector<string>& GetNames(){return textureNames;}
	bool placeLine;

protected:
	vector<string> textureNames;

	cFileImage* createFileImage();
};

#endif
