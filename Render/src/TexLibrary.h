#ifndef __TEX_LIBRARY_H_INCLUDED__
#define __TEX_LIBRARY_H_INCLUDED__
#include "..\..\Util\Serialization\StaticMap.h"
#include <map>

class cTexture;
class cTextureScale;
class cTextureAviScale;
class cTextureComplex;
const int ID_CACHE_TEXTURES		  = 1;
const int ID_CACHE_TEXTURE_OLD	  = 2;
const int ID_CACHE_TEXTURE		  = 3;
const int ID_CACHE_TEXTURE_HEAD	  = 4;
const int ID_CACHE_TEXTURE_TIME	  = 5;
const int ID_CACHE_TEXTURE_UV	  = 6;
const int ID_CACHE_TEXTURES_VERSION= 7;

class cTexLibrary
{
public:
	cTexLibrary();
	~cTexLibrary();
	void Free(FILE* f=NULL);
	void Compact(FILE* f=NULL,bool free_all=false);
	UNLOAD_ERROR Unload(const char* file_name);

	bool EnableError(bool enable);
	void AddTexture(cTexture* texture);
	//cTexture* GetElement(const char *pTextureName,char *pMode=0);
	//cTextureScale* GetElementScale(const char *pTextureName,Vect2f scale);
	//cTexture* GetElementColor(const char *pTextureName,sColor4c color,const char *SelfIlluminationName, const char* texture_name = "", RECT* logo_position = NULL);

	cTexture* GetElement2D(const char *pTextureName);
	cTexture* GetElement2DAviScale(const char *pTextureName);
	cTextureComplex* GetElement2DComplex(vector<string>& textureNames);
	cTexture* GetElement2DScale(const char *pTextureName,Vect2f scale);

	cTexture* GetElement3D(const char *pTextureName,char *pMode=0);
	cTexture* GetElement3DColor(const char *pTextureName,const char* skin_color_name_,sColor4c* color,
		const char *SelfIlluminationName, const char* logo_name, sRectangle4f* logo_position, float logo_angle);
	cTexture* GetElement3DComplex(vector<string>& textureNames, bool allowResize=true,bool line=false);
	cTexture* GetElement3DAviScale(const char *pTextureName);

	inline int GetNumberTexture()							{ return textures.size(); }
	inline cTexture* GetTexture(int number)					{ return (textures.begin()+number)->second; }

	cTexture* CreateRenderTexture(int width,int height,DWORD attr=0,bool enable_assert=true);
	cTexture* CreateTexture(int sizex,int sizey,bool alpha);
	cTexture* CreateTextureDefaultPool(int sizex,int sizey,bool alpha);
	cTexture* CreateNormalMap(int sizex,int sizey);
	cTexture* CreateAlphaTexture(int sizex,int sizey,class cFileImage* image=NULL,bool dynamic=false);//Всегда _L8
	cTexture* CreateTexture(int sizex,int sizey,eSurfaceFormat format,bool dynamic);
	MTSection& GetLock(){return lock;}
	void ReloadAllTexture();
	cTexture* GetSpericalTexture();
	cTexture* GetWhileTexture();


	void DebugMipMapColor(bool enable);
	bool IsDebugMipMapColor();

	void DebugMipMapForce(int mipmap_level);
	int GetDebugMipMapForce();

	bool ReLoadTexture(cTexture* Texture, sRectangle4f* logo_position = NULL,float logo_angle=0);
	cTexture* FindTexture(const char* name);

	void DeleteDefaultTextures();
	void CreateDefaultTextures();

	void MarkAllLoaded();

	void SetNotCacheMutableTextures(bool no);
	bool GetNotCacheMutableTextures(){return not_cache_mutable_textures;}

	void GetCachedTextures(vector<string>& names);
#ifndef _FINAL_VERSION_
	typedef map<string,int> UncachedTextures;
	UncachedTextures& GetUncachedMutableTextures(){return uncached_textures;};
#endif
	void SetWorkCacheDir(const char* dir);
	void SetBaseCacheDir(const char* dir);
	void CreateUniqName(string& outName,
		const char* textureName,
		const char* selfName = NULL,
		const char* logoName = NULL,
		sRectangle4f* logoPosition = NULL,
		sColor4c* color = NULL);
private:
	cTexture* CreateTexture(int sizex,int sizey,bool alpha,bool default_pool);
	void CompactTexturesMap();
	bool enable_error;
	struct ltstr
	{
		bool operator()(string s1,string s2) const
		{
			return s1<s2;
			//return strcmp(s1, s2) < 0;
		}
	};
	typedef StaticMap<string,cTexture*,ltstr> TextureArray;
	//vector<cTexture*> textures;
	TextureArray textures;
	vector<string> deletedTextures;
	void FreeOne(FILE* f, bool free_all);

	//bool LoadTexture(cTexture* Texture,char *pMode,Vect2f kscale, RECT* logo_position = NULL);
	//bool ReLoadTexture(cTexture* Texture,Vect2f kscale, RECT* logo_position = NULL);
	//bool ReLoadAviTexture(cTextureAviScale* Texture);
	//bool ReLoadTexture(cTexture* Texture);
	//bool ReLoadTexture2D(cTexture* Texture);

	void CacheCleanup();

	void Error(cTexture* Texture, const char* message = NULL);

	bool ReLoadDDS(cTexture* Texture);

	MTSection lock;
	cTexture* pSpericalTexture;
	cTexture* pWhiteTexture;
	bool cacheInfoLoading;
	string cacheVersion;

	friend cTexture;
	friend class cD3DRender;
	vector<cTexture*> default_texture;
	void AddToDefaultPool(cTexture* ptr);
	void DeleteFromDefaultPool(cTexture* ptr);
	bool ResizeTexture(cTexture* Texture, cTexture* outTexture);

	enum 
	{
		num_lod=3,
	};

	struct CacheData
	{

		CacheData()
		{
			textureFileTime.dwHighDateTime =0 ;
			textureFileTime.dwLowDateTime =0 ;
			selfIlluminationFileTime.dwHighDateTime =0 ;
			selfIlluminationFileTime.dwLowDateTime =0 ;
			logoFileTime.dwHighDateTime =0 ;
			logoFileTime.dwLowDateTime =0 ;
			x = 0;
			y = 0;
			TimePerFrame = 0;
			TotalTime = 0;
			isBaseCacheData = false;
		}
		struct RectAndNames
		{
			sRectangle4f rect;
			Vect2i size;
			string name;
			FILETIME filetime;
		};
		bool isBaseCacheData;
		string uniqKey;
		string cacheDir;
		string textureName;
		string selfIlluminationName;
		string logoName;
		FILETIME textureFileTime;
		FILETIME selfIlluminationFileTime;
		FILETIME logoFileTime;
		DWORD attribute;
		int x;
		int y;
		int TimePerFrame;
		int TotalTime;

		//vector<FILETIME> complexTexturesFileTime;
		vector<RectAndNames> complexTexturesData;

		string cacheName(int ilod)
		{
			xassert(ilod>=0 && ilod<num_lod);
			string s=cacheDir;
			s+="\\";
			s+='0'+ilod;
			s+="\\";
			s+=uniqKey;
			s+=".DDS";
			return s;
		}
	};

	typedef map<string,CacheData> CacheDataTable;//StaticMap тут точно ни к чему, с его постоянными реаллокациями.

	CacheDataTable cacheDataList;
	bool if_not_exist_load_from_cache;
	bool not_cache_mutable_textures;

#ifndef _FINAL_VERSION_
	UncachedTextures uncached_textures;
#endif

	void LoadCacheDataInfo(string cachePath, bool isBaseCacheData = false);
	void SaveCacheDataInfo();
	void SaveCache(cTexture* texture,  sRectangle4f* logo_position = NULL);
	bool LoadCache(cTexture* texture);
	bool LoadCacheData(cTexture* texture);
	string workCacheDir;
	string baseCacheDir;
};

cTexLibrary* GetTexLibrary();

#endif
