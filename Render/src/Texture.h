#ifndef __TEXTURE_H_INCLUDED__
#define __TEXTURE_H_INCLUDED__
//interface IDirect3DTexture9;
enum eSurfaceFormat;
#include "IDirect3DTextureProxy.h"

class cTexture : public cUnknownClass, public sAttribute
{	// класс с анимацией, является динамическим указателем, то есть может удалzться через Release()
	string		name;				// имя файла из которого загружена текстура
	int		_x,_y;					// размер текстуры
	int			TimePerFrame;		// фремя проигрывания
	int			number_mipmap;
	string		self_illumination_name;
	string		logo_name;
	string		skin_color_name;
	string		uniqName;
	eSurfaceFormat format;
	bool is2DTexture;
	int TotalTime;
	bool loaded;
public:
	sColor4c	skin_color;
	vector<IDirect3DTextureProxy*>	BitMap;

	cTexture(const char *TexName=0);
	~cTexture();
	inline const char* GetName()const{return name.c_str();};
	int GetNumberMipMap();
	void SetNumberMipMap(int number);

	inline int GetTimePerFrame();
	inline void SetTimePerFrame(int tpf);
	inline int GetNumberFrame() const {return BitMap.size(); };
	int GetX() const;
	int GetY() const;
	inline int GetWidth()const{return /*1<<GetX()*/_x;};
	inline int GetHeight()const{return /*1<<GetY()*/_y;};

	void SetWidth(int xTex);
	void SetHeight(int yTex);

	inline bool IsAlpha();
	inline bool IsAlphaTest();

	IDirect3DTextureProxy*& GetDDSurface(int n);
	inline void New(int number);
	inline eSurfaceFormat GetFmt();
	void SetFmt(eSurfaceFormat format_){format=format_;}
	int GetBitsPerPixel();

	BYTE* LockTexture(int& Pitch);
	BYTE* LockTexture(int& Pitch,Vect2i lock_min,Vect2i lock_size);
	void UnlockTexture();
	virtual bool IsScaleTexture(){return false;}
	virtual bool IsAviScaleTexture(){return false;}
	virtual bool IsComplexTexture(){return false;}
	void CopyTexture(cTexture* pSource);
	void BuildNormalMap(Vect3f* normals);
	void SaveDDS(const char* file_name,int i=0,int level=0);
	bool LoadDDS(const char* file_name);
	void SetName(const char *Name);

	void SetSelfIlluminationName(const char* s){if(s)self_illumination_name=s;else self_illumination_name.clear();}
	inline const char* GetSelfIlluminationName()const{return self_illumination_name.c_str();};

	void SetLogoName(const char* s){if(s)logo_name=s;else logo_name.clear();}
	inline const char* GetLogoName()const{return logo_name.c_str();};

	void SetSkinColorName(const char* s){if(s)skin_color_name=s;else skin_color_name.clear();}
	inline const char* GetSkinColorName()const{return skin_color_name.c_str();};

	int CalcTextureSize();//Рассчитывает размер текстуры в байтах.
	void AddToDefaultPool();
	void SetTexture2D() {is2DTexture = true;}
	void SetTexture3D() {is2DTexture = false;}
	bool IsTexture2D() const {return is2DTexture;}
	void SetTotalTime(int time) {TotalTime = time;}
	inline int GetTotalTime() const {return TotalTime;}
	void Resize(int level);
	bool GetLoaded(){return loaded;}
	void SetLoaded(){loaded=true;}
	void SetUniqName(string name) {xassert(!name.empty());uniqName = name;}
	string& GetUniqName() {return uniqName;}
protected:
	void DeleteFromDefaultPool();
};

class cTextureAviScale : public cTexture
{
public:
	vector<sRectangle4f> pos;
	vector<Vect2i> sizes;
public:
	cTextureAviScale(const char *TexName=0):cTexture(TexName){};
	void Init(class cAviScaleFileImage* pTextures);
	void Init(vector<sRectangle4f>& complexTexturesUV);
	inline const sRectangle4f& GetFramePos(float phase)
	{
		int i = int(phase*pos.size());
		i=clamp(i,0,(int)pos.size()-1);
		return pos[i];
	}
	inline const sRectangle4f& GetFramePosInt(int phase)//float(1) -> int(65535)
	{
		//return pos[phase/(65535/(pos.size()-1))];//Даааааа, спасибо Освальд с Алексом. Исполнители хреновы!
		xassert(phase>=0 && phase<=65535);
		int i=(phase*pos.size())>>16;
		xassert(i>=0 && i<pos.size());
		return pos[i];
	}

	inline const sRectangle4f& GetFramePosIndex(int index)
	{
		return pos[index];
	}

	int GetFramesCount(){return pos.size();}
	inline sRectangle4f& GetFramePos(int i)
	{
		xassert((UINT)i<pos.size());
		return pos[i];
	}
	virtual bool IsAviScaleTexture(){return true;}
}; 

class cTextureScale:public cTexture
{
	Vect2f scale;
	Vect2f uvscale;
	Vect2f inv_size;
public:
	cTextureScale(const char *TexName=0):cTexture(TexName)
	{
		scale.set(1,1);
		uvscale.set(1,1);
		inv_size.set(1,1);
	}

	inline Vect2f GetCreateScale(){return scale;};
	inline const Vect2f& GetUVScale(){return uvscale;}
	virtual bool IsScaleTexture(){return true;}
	virtual bool IsAviScaleTexture(){return false;}

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
};

class cTextureComplex : public cTextureAviScale
{
public:
	cTextureComplex(vector<string>& names):cTextureAviScale(NULL)
	{
		string name;
		//for(int i=0; i<names.size(); i++)
		//{
		//	if(names[i].empty())
		//		continue;
		//	name += names[i].substr(names[i].find_last_of("\\"));
		//}
		textureNames = names;
		cloned = false;
		placeLine = false;
		//SetName(name.c_str());
	};
	virtual bool IsComplexTexture(){return true;}
	virtual bool IsAviScaleTexture(){return false;}
	vector<string>& GetNames(){return textureNames;}
	void CloneTexture(cTextureComplex* pSource);
	bool cloned;
	bool placeLine;
protected:
	vector<string> textureNames;
};

#endif
