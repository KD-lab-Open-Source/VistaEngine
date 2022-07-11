#ifndef __LIB_3DX_H_INCLUDED__
#define __LIB_3DX_H_INCLUDED__
#include "Static3dx.h"

const int ID_CACHE_MODELS_HEAD  = 1;
const int ID_CACHE_MODEL		= 2;
const int ID_CACHE_MODELS_VERSION= 3;

class cLib3dx
{
public:
	typedef vector<cStatic3dx*> ObjectsList;
	cLib3dx();
	~cLib3dx();
	struct CacheData
	{

		CacheData()
		{
			meshFileTime.dwHighDateTime =0 ;
			meshFileTime.dwLowDateTime =0 ;
			sizeMeshFile = 0;
			isBaseCacheData = false;
		}
		bool isBaseCacheData;
		string meshName;
		string cacheName;
		FILETIME meshFileTime;
		long sizeMeshFile;
	};

	bool PreloadElement(const char* filename, bool isLogic);
	bool PreloadElement(const char* filename, sColor4c skin_color, const char* emblem_name_);//Только для графических моделей.

	cStatic3dx* GetElement(const char* fname,const char* TexturePath,bool is_logic);
	void GetAllElements(vector<cStatic3dx*>& elements) const;
	bool IsEmpty() const;
	MTSection& GetLock(){return lock;}
	ObjectsList GetAllObjects();//Стереть!
	void Compact(FILE* f=NULL);

	UNLOAD_ERROR Unload(const char* file_name,bool logic_model);

	void MarkAllLoaded();

	void SetWorkCacheDir(const char* dir);
	void SetBaseCacheDir(const char* dir);
protected:
	void LoadCacheDataInfo(string cachePath, bool isBaseCache = false);
	void SaveCacheDataInfo();
	void SaveCache(cStatic3dx* static3dx);
	bool LoadCache(cStatic3dx* static3dx);
	bool LoadCacheData(cStatic3dx* static3dx);
	void CacheCleanup();


	typedef StaticMap<string,CacheData> CacheDataTable;
	typedef vector<cStatic3dx*> ObjectMap;
	CacheDataTable cacheDataList;
	bool cacheInfoLoading;
	ObjectMap objects;
	ObjectMap logic;
	MTSection lock;
	string cacheVersion;
	bool needSaveCacheDataInfo;

	string workCacheDir;
	string baseCacheDir;
};

extern cLib3dx* pLibrary3dx;

class cStaticSimply3dx;
class cLibSimply3dx
{
public:
	cLibSimply3dx();
	~cLibSimply3dx();

	void Compact(FILE* f=NULL);
	bool PreloadElement(const char* filename, const char* visibleGroup = 0);
	cStaticSimply3dx* GetElement(const char* fname,const char* visible_group,const char* TexturePath);
	void GetAllElements(vector<cStaticSimply3dx*>& elements) const;
	void MarkAllLoaded();
protected:
	MTSection lock;
	typedef vector<cStaticSimply3dx*> ObjectMap;
	ObjectMap objects;
};

extern cLibSimply3dx* pLibrarySimply3dx;

#endif
