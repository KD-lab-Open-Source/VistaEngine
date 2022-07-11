#ifndef __LIB_3DX_H_INCLUDED__
#define __LIB_3DX_H_INCLUDED__

#include "XTL\StaticMap.h"
#include "Static3dx.h"
#include "FileUtils\FileTime.h"

class RENDER_API cLib3dx
{
public:
	typedef vector<cStatic3dx*> ObjectsList;
	cLib3dx();
	~cLib3dx();

	void clearCacheInfo();
	void saveCacheInfo(bool exported);

	bool PreloadElement(const char* filename, bool isLogic);
	bool PreloadElement(const char* filename, Color4c skin_color, const char* emblem_name_);//Только для графических моделей.

	cStatic3dx* GetElement(const char* fname,const char* TexturePath,bool is_logic);
	void GetAllElements(vector<cStatic3dx*>& elements) const;
	bool IsEmpty() const;
	void Compact();

	void Unload(const char* file_name,bool logic_model);

	void MarkAllLoaded();

protected:
	void SaveCache(cStatic3dx* static3dx);
	bool LoadCache(cStatic3dx*& static3dx);
	
	void serialize(Archive& ar);

	string cacheName(const cStatic3dx* object) const;

	struct CacheData
	{
		CacheData(const char* meshName = 0);
		void serialize(Archive& ar);
		bool valid() const;

		string meshName_;
		FileTime meshTime_;
		FileTime furTime_;
		long meshSize_;
	};

	typedef StaticMap<string, CacheData> CacheDataTable;
	CacheDataTable cacheDataTable_;
	int cacheVersion_;
	string cacheDir_;
	bool exported_;

	typedef vector<cStatic3dx*> ObjectMap;
	ObjectMap objects;
	ObjectMap logic;
	MTSection lock;
};

extern RENDER_API cLib3dx* pLibrary3dx;

class cStaticSimply3dx;
class RENDER_API cLibSimply3dx
{
public:
	cLibSimply3dx();
	~cLibSimply3dx();

	void Compact();
	bool PreloadElement(const char* filename, const char* visibleGroup = 0);
	cStaticSimply3dx* GetElement(const char* fname,const char* visible_group,const char* TexturePath);
	void GetAllElements(vector<cStaticSimply3dx*>& elements) const;
	void MarkAllLoaded();
protected:
	MTSection lock;
	typedef vector<cStaticSimply3dx*> ObjectMap;
	ObjectMap objects;
};

extern RENDER_API cLibSimply3dx* pLibrarySimply3dx;

#endif
