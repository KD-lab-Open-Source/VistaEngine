#ifndef __TEX_LIBRARY_H_INCLUDED__
#define __TEX_LIBRARY_H_INCLUDED__

#include "MTSection.h"
#include "FileUtils\FileTime.h"
#include "XMath\Rectangle4f.h"
#include "XTL\StaticMap.h"
#include "Render\inc\IVisGenericInternal.h"

class cTexture;
class cTextureScale;
class cTextureAviScale;
class cTextureComplex;

enum eSurfaceFormat;

class RENDER_API cTexLibrary
{
public:
	cTexLibrary();
	~cTexLibrary();

	void clearCacheInfo();
	void saveCacheInfo(bool exported);

	void Free();
	void Compact(bool free_all=false);
	void Unload(const char* file_name);

	bool EnableError(bool enable);
	void AddTexture(cTexture* texture);

	cTexture* GetElement2D(const char *pTextureName);
	cTexture* GetElement2DAviScale(const char *pTextureName);
	cTextureComplex* GetElement2DComplex(vector<string>& textureNames);
	cTexture* GetElement2DScale(const char *pTextureName,Vect2f scale);

	cTexture* GetElement3D(const char *pTextureName,char *pMode=0);
	cTexture* GetElement3DColor(const char *pTextureName,const char* skin_color_name_,Color4c* color,
								const char *SelfIlluminationName, const char* logo_name, const sRectangle4f& logo_position, float logo_angle);
	cTexture* GetElement3DComplex(vector<string>& textureNames, bool allowResize=true,bool line=false);
	cTexture* GetElement3DAviScale(const char *pTextureName);
	cTexture* GetElementMiniDetail(const char* textureName, int tileSize);

	inline int GetNumberTexture()							{ return textures.size(); }
	inline cTexture* GetTexture(int number)					{ return (textures.begin()+number)->second; }

	cTexture* CreateRenderTexture(int width,int height,DWORD attr=0,bool enable_assert=true);
	cTexture* CreateTexture(int sizex,int sizey,bool alpha);
	cTexture* CreateTextureDefaultPool(int sizex,int sizey,bool alpha);
	cTexture* CreateNormalMap(int sizex,int sizey);
	cTexture* CreateAlphaTexture(int sizex,int sizey,class cFileImage* image=0,bool dynamic=false);//Всегда _L8
	cTexture* CreateTexture(int sizex,int sizey,eSurfaceFormat format,bool dynamic);
	MTSection& GetLock(){return lock;}
	void ReloadAllTexture();
	cTexture* GetSpericalTexture();
	cTexture* GetWhileTexture();

	void DebugMipMapColor(bool enable);
	bool IsDebugMipMapColor();

	void DebugMipMapForce(int mipmap_level);
	int GetDebugMipMapForce();

	bool ReLoadTexture(cTexture* Texture);
	cTexture* FindTexture(const char* name);

	void DeleteDefaultTextures();
	void CreateDefaultTextures();

	void MarkAllLoaded();

	string CreateUniqueName(const char* textureName, const char* selfName = 0, const char* logoName = 0, const sRectangle4f* logoPosition = 0, const char* skinName = 0, Color4c* color = 0);

private:
	enum { num_lod = 3 };

	struct CacheData
	{
		struct RectAndNames
		{
			sRectangle4f rect;
			Vect2i size;
			string name;
			FileTime filetime;
			void serialize(Archive& ar);
		};
		string uniqKey;
		string textureName_;
		FileTime textureTime_;
		string selfIlluminationName_;
		FileTime selfIlluminationTime_;
		string logoName_;
		FileTime logoTime_;
		int x;
		int y;
		int TimePerFrame;
		int TotalTime;
		unsigned int attribute;

		typedef vector<RectAndNames> ComplexTexturesData;
		ComplexTexturesData complexTexturesData;

		CacheData();
		string cacheName(const char* cacheDir, int ilod);
		void serialize(Archive& ar);
		bool valid(const cTexture* texture) const;
	};

	typedef StaticMap<string,CacheData> CacheDataTable;
	CacheDataTable cacheDataTable_;
	int cacheVersion_;
	string cacheDir_;
	bool exported_;

	MTSection lock;
	cTexture* pSpericalTexture;
	cTexture* pWhiteTexture;
	vector<cTexture*> default_texture;

	bool enable_error;
	typedef StaticMap<string,cTexture*> TextureArray;
	TextureArray textures;
	vector<string> deletedTextures;

	cTexture* CreateTexture(int sizex,int sizey,bool alpha,bool default_pool);
	void CompactTexturesMap();
	void FreeOne(bool free_all);

	void Error(cTexture* Texture, const char* message = 0);

	void AddToDefaultPool(cTexture* ptr);
	void DeleteFromDefaultPool(cTexture* ptr);
	bool ResizeTexture(cTexture* Texture, cTexture* outTexture);

	void SaveCache(cTexture* texture,  sRectangle4f* logo_position = 0);
	bool LoadCache(cTexture* texture);

	void serialize(Archive& ar);

	friend cTexture;
	friend class cD3DRender;
};

RENDER_API cTexLibrary* GetTexLibrary();

#endif
