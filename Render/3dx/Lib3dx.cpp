#include "StdAfxRd.h"
#include "Lib3dx.h"
#include "Simply3dx.h"
#include "Node3dx.h"

int strcmp_null(const char* a,const char* b)
{
	if(a==b)
		return 0;
	if(a==0)
		return b[0]==0?0:-1;
	if(b==0)
		return a[0]==0?0:+1;
	return strcmp(a,b);
}

void normalize_path(const char* in_patch,string& out_patch)
{
	xassert(in_patch != out_patch.c_str());
	out_patch.clear();
	if(in_patch==NULL)
		return;

	if(in_patch[0]=='.' && (in_patch[1]=='\\' || in_patch[1]=='/'))
	{
		in_patch+=2;
	}

	out_patch.reserve(strlen(in_patch));
	for(const char* p=in_patch;*p;p++)
	{
		if(p==in_patch)//Для сетевых путей, типа \\CENTER\MODELS\...
		{
			out_patch+=*p;
			continue;
		}

		if(*p!='\\')
			out_patch+=*p;
		else
		{
			while(p[1]=='\\')
				p++;
			out_patch+=*p;
		}
	}
	strupr((char*)out_patch.c_str());
	xassert(out_patch.size() < _MAX_PATH);
}

vector<string> splitString(const string baseString, const string& delims, unsigned int maxSplits)
{
	// static unsigned dl;
	std::vector<string> ret;
	unsigned int numSplits = 0;

	// Use STL methods 
	size_t start, pos;
	start = 0;
	do 
	{
		pos = baseString.find_first_of(delims, start);
		if (pos == start)
		{
			// Do nothing
			start = pos + 1;
		}
		else if (pos == string::npos || (maxSplits && numSplits == maxSplits))
		{
			// Copy the rest of the string
			ret.push_back( baseString.substr(start) );
			break;
		}
		else
		{
			// Copy up to delimiter
			ret.push_back( baseString.substr(start, pos - start) );
			start = pos + 1;
		}
		// parse up to next real data
		start = baseString.find_first_not_of(delims, start);
		++numSplits;

	} while (pos != string::npos);

	return ret;
}

FILETIME GetFileTime(XStream& fb)
{
	unsigned int date,time;
	fb.gettime(date,time);
	FILETIME filetime={0,0};
	DosDateTimeToFileTime(date,time,&filetime);
	return filetime;
}

FILETIME GetFileTime(XZipStream& fb)
{
	unsigned int date,time;
	fb.gettime(date,time);
	FILETIME filetime={0,0};
	DosDateTimeToFileTime(date,time,&filetime);
	return filetime;
}

cLib3dx* pLibrary3dx=NULL;

cLib3dx::cLib3dx()
{
	pLibrary3dx=this;
	cacheInfoLoading = false;
	cacheVersion = "2.1";
	workCacheDir = "cacheData\\Models";
	CacheCleanup();
	needSaveCacheDataInfo = false;
}

cLib3dx::~cLib3dx()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
	{
		(*it)->Release();
	}

	FOR_EACH(logic,it)
	{
		(*it)->Release();
	}

	pLibrary3dx=NULL;
	if (Option_UseMeshCache && needSaveCacheDataInfo)
		SaveCacheDataInfo();
}
void cLib3dx::LoadCacheDataInfo(string cachePath, bool isBaseCacheData)
{
	cachePath += "\\cacheModelsInfo";
	string version;
	int delete_lod=0;
	CLoadDirectoryFileRender l;
	if (!l.Load(cachePath.c_str()))
		return;
	vector<CacheData> tempCacheData;
	while(CLoadData* ld = l.next())
	{
		if (ld->id == ID_CACHE_MODELS_HEAD)
		{
			CLoadDirectory rd(ld);
			while(CLoadData *l1 = rd.next())
			{
				if (l1->id == ID_CACHE_MODELS_VERSION)
				{
					CLoadIterator li(l1);
					li>>version;
					li>>delete_lod;
				}
			}
		}
		if(ld->id == ID_CACHE_MODEL)
		{
			CLoadIterator li(ld);
			CacheData data;
			li>>data.meshName;
			li>>data.cacheName;
			li>>data.meshFileTime.dwLowDateTime;
			li>>data.meshFileTime.dwHighDateTime;
			li>>data.sizeMeshFile;
			data.isBaseCacheData = isBaseCacheData;
			tempCacheData.push_back(data);
		}
	}
	if(version != cacheVersion || delete_lod!=gb_VisGeneric->GetRestrictionLOD())
		tempCacheData.clear();

	for(int i=0; i<tempCacheData.size(); i++)
	{
		// Ищем уже существующий элемент в таблице
		CacheDataTable::iterator it = cacheDataList.find(tempCacheData[i].cacheName);
		if(it == cacheDataList.end())
		{
			// Если не найден, добавляем в список
			cacheDataList[tempCacheData[i].meshName] = tempCacheData[i];
			continue;
		}
		// Проверяем является ли новый элемент более новым
		if(::CompareFileTime(&tempCacheData[i].meshFileTime, &it->second.meshFileTime)>0)
		{
			// Если более новый то заменяем старые данные новыми
			it->second = tempCacheData[i];
		}
	}
}
void cLib3dx::SaveCacheDataInfo()
{
	string path = workCacheDir + "\\cacheModelsInfo";
	if (cacheDataList.size() != 0)
	{
		vector<CacheData> tempCacheData;
		CacheDataTable::iterator it;
		for(it = cacheDataList.begin(); it != cacheDataList.end(); ++it)
		{
			tempCacheData.push_back(it->second);
		}
		MemorySaver s;
		s.push(ID_CACHE_MODELS_HEAD);
			s.push(ID_CACHE_MODELS_VERSION);
			s<<cacheVersion;
			s<<gb_VisGeneric->GetRestrictionLOD();
			s.pop();
		s.pop();
		for(int i=0; i<tempCacheData.size(); i++)
		{
			CacheData& data = tempCacheData[i];
			if(data.isBaseCacheData)
				continue;
			s.push(ID_CACHE_MODEL);
			s<<data.meshName;
			s<<data.cacheName;
			s<<data.meshFileTime.dwLowDateTime;
			s<<data.meshFileTime.dwHighDateTime;
			s<<data.sizeMeshFile;
			s.pop();
		}

		{
			FileSaver s_real;
			if (!s_real.Init(path.c_str()))
			{
				remove(path.c_str());
				return;
			}
			size_t size=s_real.write(s.buffer(),s.length());
			s_real.close();
			if(size!=s.length())
				remove(path.c_str());
		}
	}

}

void DeleteFilesInDirectory(const char* dir,const char* mask)
{
	WIN32_FIND_DATA FindFileData;
	string str;
	str=dir;
	str+="\\";
	str+=mask;
	HANDLE hFind = FindFirstFile(str.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	do
	{
		str = dir;
		str+="\\";
		str+=FindFileData.cFileName;
		DeleteFile(str.c_str());
	}while(FindNextFile(hFind,&FindFileData));

	FindClose(hFind);
}

void cLib3dx::CacheCleanup()
{
	WIN32_FIND_DATA FindFileData;
	// Удаление кэша если нет управляющего файла
	string str = workCacheDir + "\\cacheModelsInfo";
	HANDLE hFind = FindFirstFile(str.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		DeleteFilesInDirectory(workCacheDir.c_str(),"*.dat");
	else
		FindClose(hFind);
}

void cLib3dx::SaveCache(cStatic3dx* static3dx)
{
	xassert(static3dx);
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	//// Проверка на существование директории для кэша, если директория не существует создаем ее
	vector<string> dirs = splitString(workCacheDir,"\\",0);
	string dir;
	for(int i=0; i<dirs.size(); i++)
	{
		if(i>0)
			dir += "\\";
		dir += dirs[i];
		hFind = FindFirstFile(dir.c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) 
		{
			bool createCashDirResult=CreateDirectory(dir.c_str(), NULL);
			xassert(createCashDirResult);
		}
		else {
			xassert((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0);
			FindClose(hFind);
		}
	}

	string meshName = static3dx->file_name;
	if(static3dx->is_logic)
		meshName += ".logic";

	string::size_type idx = meshName.find("\\");
	while(idx != string::npos)
	{
		meshName.replace(idx,1,"_");
		idx = meshName.find("\\",idx);
	}

	string cacheFileName;
	//cacheFileName = static3dx->file_name.substr(static3dx->file_name.find_last_of("\\"));
	cacheFileName = workCacheDir + "\\" + meshName + ".dat";//cacheFileName + (static3dx->is_logic?".logic":"") + ".dat";

	CacheData data;
	XZipStream fb(0);
	data.cacheName = cacheFileName;
	data.meshName = meshName;

	if(!static3dx->file_name.empty() && fb.open(static3dx->file_name.c_str(), XS_IN))
	{
		FILETIME fileTime=GetFileTime(fb);
		data.meshFileTime = fileTime;
		data.sizeMeshFile = fb.size();
		fb.close();
	}
	static3dx->saveCacheData(cacheFileName.c_str());
	data.isBaseCacheData = false;
	cacheDataList[meshName] = data;
	needSaveCacheDataInfo = true;
}
bool cLib3dx::LoadCache(cStatic3dx* static3dx)
{
	if (!cacheInfoLoading)
	{
		cacheInfoLoading = true;
		string baseDir = baseCacheDir + "\\*.*";
		if(!baseCacheDir.empty())
		{
			WIN32_FIND_DATA FindFileData;
			bool done = false;
			HANDLE hFind = FindFirstFile(baseDir.c_str(), &FindFileData);
			if(hFind != INVALID_HANDLE_VALUE)
			{
				while(!done)
				{
					if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)&&
						strcmp(FindFileData.cFileName,".")!=0&&
						strcmp(FindFileData.cFileName,"..")!=0)
					{
						string dir = baseCacheDir + "\\" + FindFileData.cFileName+"\\Models";
						LoadCacheDataInfo(dir,true);
					}
					if(!FindNextFile(hFind,&FindFileData))
					{
						done = true;
					}
				}
			}
		}
		LoadCacheDataInfo(workCacheDir);
	}
	xassert(static3dx);
	return LoadCacheData(static3dx);
}
bool cLib3dx::LoadCacheData(cStatic3dx* static3dx)
{
	string meshName = static3dx->file_name;
	// Так как логичекие и графичекие модели кэшируются в разные файлики, а 
	// в cacheDataList ключ должен быть уникальным, для логичекой модели изменяем имя файла
	if(static3dx->is_logic)
		meshName += ".logic";

	string::size_type idx = meshName.find("\\");
	while(idx != string::npos)
	{
		meshName.replace(idx,1,"_");
		idx = meshName.find("\\",idx);
	}

	CacheDataTable::iterator it;
	it = cacheDataList.find(meshName);
	if (it == cacheDataList.end())
		return false;

	CacheData &data = it->second;

	if(!data.isBaseCacheData)
	{
		XZipStream fb(0);
		if(!fb.open(static3dx->file_name.c_str(), XS_IN))
		{
			return false;
		}else
		{
			FILETIME ft=GetFileTime(fb);
			if(::CompareFileTime(&ft, &data.meshFileTime)!=0)
				return false;
			if(fb.size() != data.sizeMeshFile)
				return false;
			fb.close();
		}
	}

	if (!static3dx->loadCacheData(data.cacheName.c_str()))
	{
		cacheDataList.erase(meshName);
		return false;
	}
	return true;
}

void cLib3dx::GetAllElements(vector<cStatic3dx*>& elements) const
{
	elements = objects;
}
void cLib3dx::MarkAllLoaded()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
	{
		(*it)->SetLoaded();
	}
}

FILE* balmer_error_log=NULL;
cStatic3dx* cLib3dx::GetElement(const char* fname_,const char* TexturePath,bool is_logic)
{
	MTAuto mtlock(lock);
	ObjectMap& obj_map=is_logic?logic:objects;
	string fname;
	normalize_path(fname_,fname);

	for(int i=0;i<obj_map.size();i++)
	{
		cStatic3dx* p=obj_map[i];
		if(p->file_name==fname)
		{
			p->AddRef();
			return p;
		}
	}

	if(!is_logic && Option_DprintfLoad)
		dprintf("Load %s\n",fname.c_str());

	cStatic3dx* pStatic=NULL;
	CLoadDirectoryFileRender rd;
	pStatic=new cStatic3dx(is_logic,fname.c_str());
	if (!Option_UseMeshCache || !LoadCache(pStatic))
	{
		if(rd.Load(fname.c_str()))
		{
			if(!pStatic->Load(rd))
			{
				delete pStatic;
				return false;
			}
			if(Option_UseMeshCache)
			{
				pStatic->AddRef();
				cObject3dx* pTempObject=new cObject3dx(pStatic,false);//Нужно для расчёта bound box, bound sphere
				SaveCache(pStatic);
				RELEASE(pTempObject);
			}
		}else
		{
			if(!is_logic)
				VisError<<"Cannot open file: "<<fname.c_str()<<VERR_END;
			else
			{
//				if(balmer_error_log)fprintf(balmer_error_log,"Cannot load logic model: %s\n",fname.c_str());
			}

			delete pStatic;
			return NULL;
		}
	}
	obj_map.push_back(pStatic);
	pStatic->AddRef();
	return pStatic;
}
cLib3dx::ObjectsList cLib3dx::GetAllObjects()
{
	MTAuto mtlock(lock);
	ObjectsList objectsList;
	
	for(ObjectMap::iterator it = objects.begin();it != objects.end(); ++it)
	{
		objectsList.push_back(*it);
	}
	return objectsList;
}

bool cLib3dx::IsEmpty() const
{
	return objects.empty() && cacheDataList.empty() && logic.empty();
}

void cLib3dx::Compact(FILE* f)
{
	MTAuto mtlock(lock);

	for(int is_logic=0;is_logic<=1;is_logic++)
	{
		ObjectMap& obj_map=is_logic?logic:objects;
		for(int i=0;i<obj_map.size();i++)
		{
			cStatic3dx*& p=obj_map[i];
			if(p->GetRef()==1)
			{
				if(Option_DprintfLoad)
					dprintf("Unload %s\n",p->file_name.c_str());
				RELEASE(p);
			}
		}

		remove_null_element(obj_map);
	}
}

UNLOAD_ERROR cLib3dx::Unload(const char* file_name,bool logic_model)
{
	MTAuto mtlock(lock);
	
	string out_patch;
	normalize_path(file_name,out_patch);

	for(int is_logic=0;is_logic<=1;is_logic++)
	{
		ObjectMap& obj_map=is_logic?logic:objects;
		for(int i=0;i<obj_map.size();i++)
		{
			cStatic3dx*& p=obj_map[i];
			if(out_patch==p->file_name && logic_model==p->is_logic)
			{
				int numref=p->GetRef();
				vector<string> names;
				p->GetTextureNames(names);
				RELEASE(p);
				obj_map.erase(obj_map.begin()+i);

				//Могут не все текстуры выгрузиться, но на это не обращаем внимания.
				for(vector<string>::iterator it=names.begin();it!=names.end();it++)
					GetTexLibrary()->Unload(it->c_str());
				return numref==1?UNLOAD_E_SUCCES:UNLOAD_E_USED;
			}
		}
	}

	return UNLOAD_E_NOTFOUND;
}


cLibSimply3dx* pLibrarySimply3dx=NULL;

cLibSimply3dx::cLibSimply3dx()
{
	pLibrarySimply3dx=this;
}

cLibSimply3dx::~cLibSimply3dx()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
	{
		(*it)->Release();
	}

	pLibrarySimply3dx=NULL;
}

void cLibSimply3dx::GetAllElements(vector<cStaticSimply3dx*>& elements) const
{
	elements = objects;
}
void cLibSimply3dx::MarkAllLoaded()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
	{
		(*it)->SetLoaded();
	}
}

cStaticSimply3dx* cLibSimply3dx::GetElement(const char* fname_,const char* visible_group,const char* TexturePath)
{
	MTAuto mtlock(lock);
	string fname;
	normalize_path(fname_,fname);

	for(int i=0; i<objects.size(); i++)
	{
		cStaticSimply3dx* obj = objects[i];
		if (stricmp(obj->file_name.c_str(),fname.c_str())==0 && strcmp_null(obj->visibleGroupName.c_str(),visible_group)==0 )
		{
			obj->AddRef();
			return obj;
		}
	}
	cStatic3dx* pStatic=pLibrary3dx->GetElement(fname_,TexturePath,false);
	if(!pStatic)
		return NULL;

	if(Option_DprintfLoad)
		dprintf("Load simply: %s\n",fname.c_str());
	cStaticSimply3dx* pSimply=new cStaticSimply3dx;
	if(!pSimply->BuildLods(pStatic,visible_group))
	{
		pSimply->Release();
		pStatic->Release();
		return NULL;
	}

	pStatic->Release();

	//objects[fname]=pSimply;
	objects.push_back(pSimply);
	pSimply->AddRef();
	return pSimply;
}


void cLibSimply3dx::Compact(FILE* f)
{
	MTAuto mtlock(lock);

	ObjectMap& obj_map=objects;
	for(int i=0;i<obj_map.size();i++)
	{
		cStaticSimply3dx*& p=obj_map[i];
		if(p->GetRef()==1)
		{
//			dprintf("Compacted simply %s\n",p->file_name.c_str());
			RELEASE(p);
		}
	}

	remove_null_element(obj_map);
}

bool cLib3dx::PreloadElement(const char* filename, bool isLogic)
{
	if(!Option_EnablePreload3dx)
		return true;
	if(cStatic3dx* object = pLibrary3dx->GetElement(filename, 0, isLogic)){
		RELEASE(object);
		return true;
	}
	else
		return false;
}

bool cLib3dx::PreloadElement(const char* filename, sColor4c skin_color, const char* emblem_name_)
{
	if(!Option_EnablePreload3dx)
		return true;
	cStatic3dx* pStatic= pLibrary3dx->GetElement(filename, 0, false);
	if(pStatic==0)
		return false;
	cObject3dx *pObject=new cObject3dx(pStatic,false);
	pObject->SetSkinColor(skin_color,emblem_name_);
	RELEASE(pObject);
	return true;
}

bool cLibSimply3dx::PreloadElement(const char* filename, const char* visibleGroup)
{
	if(!Option_EnablePreload3dx)
		return true;
	if(cStaticSimply3dx* object = GetElement(filename, visibleGroup, 0)){
		RELEASE(object);
		return true;
	}
	else
		return false;
}
void cLib3dx::SetWorkCacheDir(const char* dir)
{
	workCacheDir = dir;
	workCacheDir += "\\Models";
}
void cLib3dx::SetBaseCacheDir(const char* dir)
{
	baseCacheDir = dir;
}
