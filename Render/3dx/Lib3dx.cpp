#include "StdAfxRd.h"
#include "Lib3dx.h"
#include "TexLibrary.h"
#include "Simply3dx.h"
#include "Node3dx.h"
#include "VisGeneric.h"
#include "Serialization\InPlaceArchive.h"
#include "Serialization\BinaryArchive.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\SerializationFactory.h"
#include "kdw\ContentUtil.h"

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

RENDER_API cLib3dx* pLibrary3dx=0;

cLib3dx::CacheData::CacheData(const char* meshName)
{
	if(meshName){
		meshName_ = meshName;
		meshTime_ = FileTime(meshName);
		furTime_ = FileTime(setExtention(meshName, "fur").c_str());
		meshSize_ = XZipStream(meshName, XS_IN).size();
	}
	else{
		meshSize_ = 0;
	}
}

void cLib3dx::CacheData::serialize(Archive& ar)
{
	ar.serialize(meshName_, "meshName", 0);
	ar.serialize(meshTime_, "meshTime", 0);
	ar.serialize(furTime_, "furTime", 0);
	ar.serialize(meshSize_, "meshSize", 0);
}

bool cLib3dx::CacheData::valid() const
{
	if(meshTime_ != FileTime(meshName_.c_str()))
		return false;
	if(furTime_ != FileTime(setExtention(meshName_.c_str(), "fur").c_str()))
		return false;
	if(meshSize_ != XZipStream(meshName_.c_str(), XS_IN).size())
		return false;
	return true;
}

cLib3dx::cLib3dx()
{
	pLibrary3dx=this;
	cacheVersion_ = 23;
	cacheDir_ = "cacheData\\Models\\";
	exported_ = false;
	
	createDirectory("cacheData");
	createDirectory(cacheDir_.c_str());

	string cacheInfo = cacheDir_ + "cacheModelInfo";
	BinaryIArchive ia(0);
	if(ia.open(cacheInfo.c_str()))
		serialize(ia);

	kdw::getAllTextureNamesFunc = GetAllTextureNames;
}

cLib3dx::~cLib3dx()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
		(*it)->Release();

	FOR_EACH(logic,it)
		(*it)->Release();

	pLibrary3dx = 0;

	if(Option_UseMeshCache && !exported_)
		saveCacheInfo(false);
}

void cLib3dx::serialize(Archive& ar)
{
	if(ar.isInput()){
		int version;
		ar.serialize(version, "version", "version");
		if(version != cacheVersion_)
			return;
	}
	else
		ar.serialize(cacheVersion_, "version", "version");

	ar.serialize(exported_, "exported", 0);
	ar.serialize(cacheDataTable_, "cacheDataTable", "cacheDataTable");
}

void cLib3dx::clearCacheInfo()
{
	cacheDataTable_.clear();
	objects.clear();
	logic.clear();
}

void cLib3dx::saveCacheInfo(bool exported)
{
	exported_ = exported;
	string cacheInfo = cacheDir_ + "cacheModelInfo";
	BinaryOArchive oa(cacheInfo.c_str());
	serialize(oa);
}

void cLib3dx::SaveCache(cStatic3dx* static3dx)
{
	xassert(static3dx);

	string name = cacheName(static3dx);
	static3dx->saveInPlace(name.c_str());
	cacheDataTable_[name] = CacheData(static3dx->fileName());
}

bool cLib3dx::LoadCache(cStatic3dx*& static3dx)
{
	start_timer_auto();
	string name = cacheName(static3dx);
	CacheDataTable::iterator it = cacheDataTable_.find(name);
	if(it == cacheDataTable_.end())
		return false;

	if(!exported_ && !it->second.valid()){
		cacheDataTable_.erase(it);
		return false;
	}

	VTableFactory::addVtable(*static3dx);

	InPlaceIArchive ia(0);
	if(ia.open(name.c_str())){
		delete static3dx;
		ia.construct(static3dx);
		static3dx->AddRef();
		static3dx->constructInPlace(name.c_str());
		return true;
	}

	return false;
}

void cLib3dx::GetAllElements(vector<cStatic3dx*>& elements) const
{
	elements = objects;
}

void cLib3dx::MarkAllLoaded()
{
	ObjectMap::iterator it;
	FOR_EACH(objects,it)
		(*it)->SetLoaded();
}

cStatic3dx* cLib3dx::GetElement(const char* fname_,const char* TexturePath,bool is_logic)
{
	MTAuto mtlock(lock);
	ObjectMap& obj_map=is_logic?logic:objects;
	string fname = normalizePath(fname_);
	
	for(int i=0;i<obj_map.size();i++){
		cStatic3dx* p=obj_map[i];
		if(!strcmp(p->fileName(), fname.c_str())){
			p->AddRef();
			return p;
		}
	}

	if(!is_logic && Option_DprintfLoad)
		dprintf("Load %s\n",fname.c_str());

	cStatic3dx* pStatic = new cStatic3dx(is_logic, fname.c_str());
	if(!Option_UseMeshCache || !LoadCache(pStatic)){
		if(!pStatic->load(fname.c_str())){
			if(!is_logic)
				VisError<<"Cannot open file: "<<fname.c_str()<<VERR_END;
			delete pStatic;
			return false;
		}
		else if(Option_UseMeshCache){
			pStatic->AddRef();
			cObject3dx* pTempObject=new cObject3dx(pStatic,false);//Нужно для расчёта bound box, bound sphere
			RELEASE(pTempObject);
			SaveCache(pStatic);
		}
	}

	obj_map.push_back(pStatic);
	pStatic->AddRef();

	return pStatic;
}

bool cLib3dx::IsEmpty() const
{
	return objects.empty() && logic.empty();
}

void cLib3dx::Compact()
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
					dprintf("Unload %s\n",p->fileName());
				RELEASE(p);
			}
		}

		remove_null_element(obj_map);
	}
}

void cLib3dx::Unload(const char* file_name,bool logic_model)
{
	MTAuto mtlock(lock);
	
	string out_patch = normalizePath(file_name);

	for(int is_logic=0;is_logic<=1;is_logic++){
		ObjectMap& obj_map=is_logic?logic:objects;
		for(int i=0;i<obj_map.size();i++){
			cStatic3dx*& p=obj_map[i];
			if(out_patch == p->fileName() && logic_model == p->is_logic){
				int numref=p->GetRef();
				TextureNames names;
				p->GetTextureNames(names);
				RELEASE(p);
				obj_map.erase(obj_map.begin()+i);

				//Могут не все текстуры выгрузиться, но на это не обращаем внимания.
				for(vector<string>::iterator it=names.begin();it!=names.end();it++)
					GetTexLibrary()->Unload(it->c_str());
				return;
			}
		}
	}
}


RENDER_API cLibSimply3dx* pLibrarySimply3dx=0;

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

	pLibrarySimply3dx=0;
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
	string fname = normalizePath(fname_);

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
		return 0;

	if(Option_DprintfLoad)
		dprintf("Load simply: %s\n",fname.c_str());
	cStaticSimply3dx* pSimply=new cStaticSimply3dx;
	if(!pSimply->BuildLods(pStatic,visible_group))
	{
		pSimply->Release();
		pStatic->Release();
		return 0;
	}

	pStatic->Release();

	//objects[fname]=pSimply;
	objects.push_back(pSimply);
	pSimply->AddRef();
	return pSimply;
}


void cLibSimply3dx::Compact()
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

bool cLib3dx::PreloadElement(const char* filename, Color4c skin_color, const char* emblem_name_)
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

string cLib3dx::cacheName(const cStatic3dx* object) const
{
	string cacheName = cutPathToResource(object->fileName());
	cacheName += object->is_logic ? "L" : "G";
	replaceSubString(cacheName, "\\", "_");
	cacheName = cacheDir_ + cacheName;
	return cacheName;
}
