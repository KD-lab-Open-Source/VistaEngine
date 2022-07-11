#include "StdAfxRD.h"
#include "FileImage.h"
#include <io.h>
#include <fcntl.h>
#include "TextureAtlas.h"
#include <algorithm>

void normalize_path(const char* in_patch,string& out_patch);
vector<string> splitString(const string baseString, const string& delims, unsigned int maxSplits);
int strcmp_null(const char* a,const char* b);
FILETIME GetFileTime(XStream& fb);
FILETIME GetFileTime(XZipStream& fb);
/*
bool IsDDSFile(const char* fname)
{
	int file=_open(fname,_O_RDONLY|_O_BINARY);
	if(file==-1) return false;
	int hd[2];	
	hd[0]=hd[1] = 0;
	_read(file,&hd,8);
	_close(file);
	return hd[1]==124 && bool(hd[0]&(DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT));
}
*/
#ifdef TEXTURE_NOTFREE
struct BeginNF
{

	BeginNF()
	{
		FILE* f=NULL;
		f=fopen("texture_notfree1.txt","wt");
		if(f)fclose(f);
		f=fopen("obj_notfree1.txt","wt");
		if(f)fclose(f);
	}
};
static BeginNF begin_nf;
#endif

cTexLibrary* GetTexLibrary()
{
	return &gb_RenderDevice3D->TexLibrary;
}
cTexLibrary::cTexLibrary()
{
	pSpericalTexture=NULL;
	pWhiteTexture=NULL;
	enable_error=true;
	cFileImage::InitFileImage();
	workCacheDir = "cacheData\\Textures";
	CacheCleanup();
	cacheInfoLoading = false;
	cacheVersion = "2.3";
	not_cache_mutable_textures=false;
}
cTexLibrary::~cTexLibrary()
{
	if(pSpericalTexture)
		pSpericalTexture->ClearAttribute(TEXTURE_NONDELETE);
	RELEASE(pSpericalTexture);
	if(pWhiteTexture)
		pWhiteTexture->ClearAttribute(TEXTURE_NONDELETE);
	RELEASE(pWhiteTexture);
	Free();
	cFileImage::DoneFileImage();
	SaveCacheDataInfo();
}

bool cTexLibrary::EnableError(bool enable)
{
	bool b=enable_error;
	enable_error=enable;
	return b;
}

void cTexLibrary::FreeOne(FILE* f, bool free_all)
{
	bool close=false;
#ifdef TEXTURE_NOTFREE
	if(!f)
	{
		f=fopen("texture_notfree.txt","at");
		close=true;
	}
#endif 
	if(f)fprintf(f,"Texture freeing\n");

	int compacted=0;
	//vector<cTexture*>::iterator it;
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		cTexture*& p=it->second;
		if(p && p->GetRef()==1 && (free_all || !p->GetAttribute(TEXTURE_NO_COMPACTED)))
		{
			p->ClearAttribute(TEXTURE_NONDELETE);
			if(Option_DprintfLoad)
				dprintf("Unload: %s\n",p->GetUniqName().c_str());
			deletedTextures.push_back(p->GetUniqName());
			p->Release();
			p=NULL;
			compacted++;
		}else
		{
//			VISASSERT(p->GetRef()==1);
			if(f)fprintf(f,"%s - %i\n",p->GetName(),p->GetRef());
		}
	}

	if(f)fprintf(f,"Texture free %i, not free %i\n",compacted,textures.size()-compacted);

	if(f)
	{
		if(close)fclose(f);
		else fflush(f);
	}
}
void cTexLibrary::MarkAllLoaded()
{
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		if(it->second)
			it->second->SetLoaded();
	}
}

void cTexLibrary::Compact(FILE* f,bool free_all)
{
	MTAuto mtenter(lock);
	FreeOne(f,free_all);
	//remove_null_element(textures);
	CompactTexturesMap();
}
void cTexLibrary::CompactTexturesMap()
{
	if (deletedTextures.size()==0)
		return;
	for(int i=0; i<deletedTextures.size();i++)
	{
		TextureArray::iterator it = textures.find(deletedTextures[i].c_str());
		if (it != textures.end())
			textures.erase(it);
	}
	deletedTextures.clear();
}

void cTexLibrary::Free(FILE* f)
{
	MTAuto mtenter(lock);
	FreeOne(f,true);
	textures.clear();
}

UNLOAD_ERROR cTexLibrary::Unload(const char* file_name)
{
	MTAuto mtenter(lock);
	string out_patch;
	normalize_path(file_name,out_patch);
	string uniqName;
	CreateUniqName(uniqName,out_patch.c_str());
	TextureArray::iterator it = textures.find(uniqName.c_str());
	if (it == textures.end())
		return UNLOAD_E_NOTFOUND;

	cTexture* p=it->second;

	UNLOAD_ERROR ret=p->GetRef()>1?UNLOAD_E_USED:UNLOAD_E_SUCCES;

	p->ClearAttribute(TEXTURE_NONDELETE);
	textures.erase(it);
	p->Release();
	return ret;
}

void cTexLibrary::DeleteDefaultTextures()
{
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		if (it->second&&it->second->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
			gb_RenderDevice3D->DeleteTexture(it->second);
	}
	for(int i=0;i<default_texture.size();i++)
	{
		cTexture* pTexture=default_texture[i];
		xassert(pTexture->GetAttribute(TEXTURE_D3DPOOL_DEFAULT));
		gb_RenderDevice3D->DeleteTexture(pTexture);
	}

}
void cTexLibrary::CreateDefaultTextures()
{
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		if (it->second&&it->second->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
		{
			xassert(0);
			if(it->second->GetRef()>1)
				gb_RenderDevice3D->CreateTexture(it->second,0,-1,-1);
		}
	}
	for(int i=0;i<default_texture.size();i++)
	{
		cTexture* pTexture=default_texture[i];
		xassert(pTexture->GetAttribute(TEXTURE_D3DPOOL_DEFAULT));
		gb_RenderDevice3D->CreateTexture(pTexture,0,-1,-1);
	}

}

cTexture* cTexLibrary::CreateRenderTexture(int width,int height,DWORD attr,bool enable_assert)
{
	MTAuto mtenter(lock);
	if(!attr)
		attr=TEXTURE_RENDER16;
	attr|=TEXTURE_D3DPOOL_DEFAULT;
	cTexture *Texture=new cTexture("");
	Texture->New(1);
	Texture->SetTimePerFrame(0);	
	Texture->SetWidth(width);
	Texture->SetHeight(height);
	Texture->SetNumberMipMap(1);
	Texture->SetAttribute(attr);

	int err=gb_RenderDevice3D->CreateTexture(Texture,NULL,-1,-1,enable_assert);
	if(err) { Texture->Release(); return 0; }
	return Texture;
}
void cTexLibrary::LoadCacheDataInfo(string cachePath, bool isBaseCacheData)
{
	string version;
	CLoadDirectoryFileRender l;
	string nameTexturesInfo=cachePath+"\\cacheTexturesInfo";
	if (!l.Load(nameTexturesInfo.c_str()))
		return;
	vector<CacheData> tempCacheData;
	while(CLoadData* ld = l.next())
	{
		if (ld->id == ID_CACHE_TEXTURES)
		{
			CLoadDirectory rd(ld);
			while(CLoadData *ltex = rd.next())
			{
				if (ltex->id == ID_CACHE_TEXTURES_VERSION)
				{
					CLoadIterator li(ltex);
					li>>version;
				}
				if(ltex->id == ID_CACHE_TEXTURE)
				{
					CLoadDirectory rd(ltex);
					CacheData data;
					data.cacheDir=cachePath;

					while(CLoadData *l1 = rd.next())
					switch(l1->id)
					{
					case ID_CACHE_TEXTURE_HEAD:
						{
							CLoadIterator li(l1);
							li>>data.uniqKey;
							li>>data.textureName;
							li>>data.selfIlluminationName;
							li>>data.logoName;
							li>>data.textureFileTime.dwLowDateTime;
							li>>data.textureFileTime.dwHighDateTime;
							li>>data.selfIlluminationFileTime.dwLowDateTime;
							li>>data.selfIlluminationFileTime.dwHighDateTime;
							li>>data.logoFileTime.dwLowDateTime;
							li>>data.logoFileTime.dwHighDateTime;
							li>>data.attribute;
							li>>data.x;
							li>>data.y;
							li>>data.TimePerFrame;
							li>>data.TotalTime;
						}
						break;
					//case ID_CACHE_TEXTURE_TIME:
					//	{
					//		CLoadIterator li(l1);
					//		int size=0;
					//		li>>size;
					//		data.complexTexturesFileTime.resize(size);
					//		for (int i=0;i<size; i++)
					//		{
					//			FILETIME fTime;
					//			li>>fTime.dwLowDateTime;
					//			li>>fTime.dwHighDateTime;
					//			data.complexTexturesFileTime[i]=fTime;
					//		}
					//	}
					//	break;
					case ID_CACHE_TEXTURE_UV:
						{
							CLoadIterator li(l1);
							int size=0;
							li>>size;
							data.complexTexturesData.resize(size);
							for (int i=0;i<size; i++)
							{
								li>>data.complexTexturesData[i].rect;
								li>>data.complexTexturesData[i].name;
								FILETIME fTime;
								li>>fTime.dwLowDateTime;
								li>>fTime.dwHighDateTime;
								data.complexTexturesData[i].filetime=fTime;
								li>>data.complexTexturesData[i].size;
							}
						}
						break;
					}
					if(!data.uniqKey.empty())
					{
						data.isBaseCacheData = isBaseCacheData;
						tempCacheData.push_back(data);
					}
				}
			}
		}
	}
	if(version != cacheVersion)
		tempCacheData.clear();
	for(int i=0; i<tempCacheData.size(); i++)
	{
		// Ищем уже существующий элемент в таблице
		CacheDataTable::iterator it = cacheDataList.find(tempCacheData[i].uniqKey);
		if(it == cacheDataList.end())
		{
			// Если не найден, добавляем в список
			cacheDataList[tempCacheData[i].uniqKey] = tempCacheData[i];
			continue;
		}
		CacheData& oldData = it->second;
		CacheData& newData = tempCacheData[i];
		bool replace = true;
		// Проверяем является ли новый элемент более новым
		if(oldData.complexTexturesData.size() == newData.complexTexturesData.size()&&oldData.complexTexturesData.size()>0)
		{
			for (int i=0; i<oldData.complexTexturesData.size();i++)
			{
				if(::CompareFileTime(&oldData.complexTexturesData[i].filetime,
					&newData.complexTexturesData[i].filetime)>0)
				{
					replace = false;
				}
			}
		}else
		{
			if(::CompareFileTime(&oldData.textureFileTime, &newData.textureFileTime)>0)
			{
				replace = false;
			}

		}
		if(replace)
		{
			// Если более новый то заменяем старые данные новыми
			it->second = tempCacheData[i];
		}
	}

}
void cTexLibrary::SaveCacheDataInfo()
{
	string path = workCacheDir + "\\cacheTexturesInfo";
	if (cacheDataList.size() != 0)
	{
		vector<CacheData> tempCacheData;
		CacheDataTable::iterator it;
		for(it = cacheDataList.begin(); it != cacheDataList.end(); ++it)
		{
			tempCacheData.push_back(it->second);
		}
		MemorySaver s;

		s.push(ID_CACHE_TEXTURES);
			s.push(ID_CACHE_TEXTURES_VERSION);
			s<<cacheVersion;
			s.pop();
		for(int i=0; i<tempCacheData.size(); i++)
		{
			CacheData& data = tempCacheData[i];
			if(data.isBaseCacheData)
				continue;
			s.push(ID_CACHE_TEXTURE);

			{
				s.push(ID_CACHE_TEXTURE_HEAD);
				s<<data.uniqKey;
				s<<data.textureName;
				s<<data.selfIlluminationName;
				s<<data.logoName;
				s<<data.textureFileTime.dwLowDateTime;
				s<<data.textureFileTime.dwHighDateTime;
				s<<data.selfIlluminationFileTime.dwLowDateTime;
				s<<data.selfIlluminationFileTime.dwHighDateTime;
				s<<data.logoFileTime.dwLowDateTime;
				s<<data.logoFileTime.dwHighDateTime;
				s<<data.attribute;
				s<<data.x;
				s<<data.y;
				s<<data.TimePerFrame;
				s<<data.TotalTime;
				s.pop();
			}

			//if(!data.complexTexturesFileTime.empty())
			//{
			//	s.push(ID_CACHE_TEXTURE_TIME);
			//	s<<data.complexTexturesFileTime.size();
			//	for (int i=0; i<data.complexTexturesFileTime.size(); i++)
			//	{
			//		s<<data.complexTexturesFileTime[i].dwLowDateTime;
			//		s<<data.complexTexturesFileTime[i].dwHighDateTime;
			//	}
			//	s.pop();
			//}

			if(!data.complexTexturesData.empty())
			{
				s.push(ID_CACHE_TEXTURE_UV);
				s<<data.complexTexturesData.size();
				for (int i=0; i<data.complexTexturesData.size(); i++)
				{
					s<<data.complexTexturesData[i].rect;
					s<<data.complexTexturesData[i].name;
					s<<data.complexTexturesData[i].filetime.dwLowDateTime;
					s<<data.complexTexturesData[i].filetime.dwHighDateTime;
					s<<data.complexTexturesData[i].size;
				}
				s.pop();
			}
			s.pop();
		}
		s.pop();

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
cTexture* cTexLibrary::CreateTexture(int sizex,int sizey,bool alpha,bool default_pool)
{
	cTexture *Texture=new cTexture;
	Texture->SetNumberMipMap(1);
	Texture->SetAttribute(TEXTURE_32|TEXTURE_NODDS);
	if(alpha)
		Texture->SetAttribute(TEXTURE_ALPHA_BLEND);
	if(default_pool)
		Texture->SetAttribute(TEXTURE_D3DPOOL_DEFAULT);

	Texture->BitMap.resize(1);
	Texture->BitMap[0]=0;
	Texture->SetWidth(sizex);
	Texture->SetHeight(sizey);

	if(gb_RenderDevice3D->CreateTexture(Texture,NULL,-1,-1))
	{
		delete Texture;
		return NULL;
	}

	return Texture;
}

cTexture* cTexLibrary::CreateTextureDefaultPool(int sizex,int sizey,bool alpha)
{
	return CreateTexture(sizex,sizey,alpha,true);
}

cTexture* cTexLibrary::CreateTexture(int sizex,int sizey,bool alpha)
{
	return CreateTexture(sizex,sizey,alpha,false);
}

void DeleteFilesInDirectory(const char* dir,const char* mask);
void cTexLibrary::CacheCleanup()
{
	WIN32_FIND_DATA FindFileData;
	// Удаление кэша если нет управляющего файла
	string str = workCacheDir + "\\cacheTexturesInfo";
	HANDLE hFind = FindFirstFile(str.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		DeleteFilesInDirectory(workCacheDir.c_str(),"*.dds");
	else
		FindClose(hFind);
}
void cTexLibrary::SaveCache(cTexture* texture,  sRectangle4f* logo_position)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	string cacheDir = "cacheData";
	if(not_cache_mutable_textures)
	{
		if(texture->skin_color.a!=0 || 
			texture->IsComplexTexture() || texture->IsAviScaleTexture() || texture->IsScaleTexture() ||
			strstr(texture->GetName(),"RESOURCE\\FX") ||
			texture->GetSelfIlluminationName()[0] || 
			texture->GetLogoName()[0] || 
			texture->GetSkinColorName()[0]
			)
		{
#ifndef _FINAL_VERSION_
			if(texture->GetSelfIlluminationName()[0])
				uncached_textures[texture->GetSelfIlluminationName()]=1;
			if(texture->GetLogoName()[0])
				uncached_textures[texture->GetLogoName()]=1;
			if(texture->GetSkinColorName()[0])
				uncached_textures[texture->GetSkinColorName()]=1;
			{
				string s=texture->GetName();
				int pos=s.find("###");
				if(pos!=string::npos)
					s.resize(pos);
				uncached_textures[s.c_str()]=1;
			}
#endif
			return;
		}
	}

	if (texture->GetNumberFrame()>1)
		return;

	// Проверка на существование директории для кэша, если директория не существует создаем ее
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
			xassert((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0);
			FindClose(hFind);
		}
	}

	for(int ilod=0;ilod<num_lod;ilod++)
	{
		string s=workCacheDir+"\\";
		s+='0'+ilod;
		CreateDirectory(s.c_str(),NULL);
	}

	CacheData data;
	XZipStream fb(0);
	FILETIME fileTime;
	data.uniqKey = texture->GetUniqName();
	data.attribute = texture->GetAttribute();
	data.cacheDir = workCacheDir;
	data.textureName = texture->GetName();
	data.selfIlluminationName = texture->GetSelfIlluminationName();
	data.logoName = texture->GetLogoName();
	data.x = texture->GetWidth();
	data.y = texture->GetHeight();
	if (texture->IsAviScaleTexture())
	{
		cTextureAviScale* t=(cTextureAviScale*)texture;
		data.TimePerFrame = texture->GetTimePerFrame();
		data.TotalTime = t->GetTotalTime();
		data.complexTexturesData.resize(t->GetFramesCount());
		for(int i=0;i<t->GetFramesCount();i++)
		{
			data.complexTexturesData[i].rect=t->GetFramePosIndex(i);
			data.complexTexturesData[i].size=t->sizes[i];
		}
	}
	if (texture->IsComplexTexture())
	{
		cTextureComplex* texCompl = (cTextureComplex*)texture;
		data.TimePerFrame = texture->GetTimePerFrame();
		data.TotalTime = texCompl->GetTotalTime();
		data.complexTexturesData.resize(texCompl->GetFramesCount());
		for(int i=0;i<texCompl->GetFramesCount();i++)
		{
			data.complexTexturesData[i].rect=texCompl->GetFramePosIndex(i);
			data.complexTexturesData[i].size=texCompl->sizes[i];
			if (texCompl->GetNames()[i].empty()||!fb.open(texCompl->GetNames()[i].c_str(), XS_IN))
				continue;
			data.complexTexturesData[i].name = texCompl->GetNames()[i];
			fileTime=GetFileTime(fb);
			data.complexTexturesData[i].filetime = fileTime;
			fb.close();
		}
	}else
	if(!data.textureName.empty() && fb.open(data.textureName.c_str(), XS_IN))
	{
		fileTime=GetFileTime(fb);
		data.textureFileTime = fileTime;
		fb.close();
	}
	if(!data.selfIlluminationName.empty() && fb.open(data.selfIlluminationName.c_str(), XS_IN))
	{
		fileTime=GetFileTime(fb);
		data.selfIlluminationFileTime = fileTime;
		fb.close();
	}
	if(!data.logoName.empty() && fb.open(data.logoName.c_str(), XS_IN))
	{
		fileTime=GetFileTime(fb);
		data.logoFileTime = fileTime;
		fb.close();
	}

	for(int ilod=0;ilod<num_lod;ilod++)
		texture->SaveDDS(data.cacheName(ilod).c_str(),0,ilod);
	data.isBaseCacheData = false;
	cacheDataList[texture->GetUniqName()] = data;
}
void cTexLibrary::CreateUniqName(string& outName,
					const char* textureName,
					const char* selfName,
					const char* logoName,
					sRectangle4f* logoPosition,
					sColor4c* color)
{
	if(!textureName || !textureName[0])
		return;

	char buf[256];

	string name = textureName;
	if(selfName && selfName[0])
	{
		name += "[";
		name += selfName;
		name += "]";
	}
	if(logoName && logoName[0])
	{
		name += "[";
		name += logoName;
		name += "]";
		if(logoPosition)
		{
			sprintf(buf,"[%.3f,%.3f,%.3f,%.3f]",logoPosition->min.x,logoPosition->min.y,logoPosition->max.x,logoPosition->max.y);
			name += buf;
		}
	}
	if(color && color->a != 0)
	{
		sprintf(buf,"[%X]",color->RGBA());
		name += buf;
	}
	string::size_type idx = name.find("\\");
	while(idx != string::npos)
	{
		name.replace(idx,1,"_");
		idx = name.find("\\",idx);
	}
	outName += name;
}



bool cTexLibrary::LoadCache(cTexture *texture)
{
	if (!cacheInfoLoading)
	{
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
					if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
						strcmp(FindFileData.cFileName,".")!=0&&
						strcmp(FindFileData.cFileName,"..")!=0)
					{
						string dir = baseCacheDir + "\\" + FindFileData.cFileName+"\\Textures";
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
		cacheInfoLoading = true;
	}
	return LoadCacheData(texture);
}

bool cTexLibrary::LoadCacheData(cTexture* texture)
{
	CacheDataTable::iterator it;
	it = cacheDataList.find(texture->GetUniqName());
	if (it == cacheDataList.end())
		return false;

	CacheData &data = it->second;

	XZipStream fb(0);
	FILETIME ft;
	if(data.isBaseCacheData)
		goto GoTrue;

	if (texture->IsComplexTexture())
	{
		cTextureComplex* texCompl = (cTextureComplex*)texture; 
		if (texCompl->GetNames().size() != data.complexTexturesData.size())
			return false;
		for (int i=0; i<data.complexTexturesData.size();i++)
		{
			if (data.complexTexturesData[i].name!=texCompl->GetNames()[i])
				goto GoFalse;
			if(data.complexTexturesData[i].name.empty())
				continue;
			if(!fb.open(data.complexTexturesData[i].name.c_str(), XS_IN))
				goto GoFalse;
			FILETIME ft=GetFileTime(fb);
			fb.close();
			if(::CompareFileTime(&ft, &data.complexTexturesData[i].filetime)!=0)
				goto GoFalse;
		}
	}else
	{
		if(!fb.open(data.textureName.c_str(), XS_IN))
			goto GoFalse;
		ft=GetFileTime(fb);
		if(::CompareFileTime(&ft, &data.textureFileTime)!=0)
			goto GoFalse;
		fb.close();
	}
	if (texture->GetSelfIlluminationName()&&strlen(texture->GetSelfIlluminationName())!=0)
	{
		if(!fb.open(texture->GetSelfIlluminationName(), XS_IN))
			goto GoFalse;
		FILETIME ft=GetFileTime(fb);
		if(::CompareFileTime(&ft, &data.selfIlluminationFileTime)!=0)
			goto GoFalse;
		fb.close();
	}
	if(texture->GetLogoName()&&strlen(texture->GetLogoName())!=0)
	{
		if(!fb.open(texture->GetLogoName(), XS_IN))
			goto GoFalse;
		FILETIME ft=GetFileTime(fb);
		if(::CompareFileTime(&ft, &data.logoFileTime)!=0)
			goto GoFalse;
		fb.close();
	}

	goto GoTrue;
GoFalse:
	return false;
GoTrue:

	texture->New(1);
	texture->SetAttribute(data.attribute);
	texture->SetWidth(data.x);
	texture->SetHeight(data.y);
	if (texture->IsAviScaleTexture())
	{
		cTextureAviScale* t=(cTextureAviScale*)texture;
		//t->Init(data.complexTexturesData);
		t->pos.resize(data.complexTexturesData.size());
		t->sizes.resize(data.complexTexturesData.size());
		for(int i=0; i<data.complexTexturesData.size(); i++)
		{
			t->pos[i] = data.complexTexturesData[i].rect;
			t->sizes[i] = data.complexTexturesData[i].size;
		}
		t->SetTotalTime(data.TotalTime);
		t->SetTimePerFrame(data.TimePerFrame);

	}else
	if(texture->IsComplexTexture())
	{
		cTextureComplex* t=(cTextureComplex*)texture;
		//t->Init(data.complexTexturesData);
		t->pos.resize(data.complexTexturesData.size());
		t->sizes.resize(data.complexTexturesData.size());
		for(int i=0; i<data.complexTexturesData.size(); i++)
		{
			for (int j=0; j<t->GetNames().size();j++)
			{
				if (data.complexTexturesData[i].name == t->GetNames()[j])
				{
					t->pos[j]=data.complexTexturesData[i].rect;
					t->sizes[j] = data.complexTexturesData[i].size;
				}
			}
		}
		t->SetTotalTime(data.TotalTime);
		t->SetTimePerFrame(data.TimePerFrame);
	}else
	if (!texture->IsTexture2D())
	{
		texture->SetNumberMipMap(min(texture->GetX(),texture->GetY())+1);
	}
	if (!texture->LoadDDS(data.cacheName(Option_TextureDetailLevel).c_str()))
	{
		cacheDataList.erase(texture->GetUniqName());
		return false;
	}
	texture->SetWidth(max(data.x>>Option_TextureDetailLevel,1));
	texture->SetHeight(max(data.y>>Option_TextureDetailLevel,1));
	return true;
}

void cTexLibrary::AddTexture(cTexture* texture)
{
	xassert(texture);
	texture->SetAttribute(TEXTURE_NONDELETE);
	textures[texture->GetUniqName()] = texture;
	texture->AddRef();
}

bool cTexLibrary::ReLoadTexture(cTexture* Texture,sRectangle4f* logo_position, float logo_angle)
{
	vector<IDirect3DTextureProxy*>::iterator it;
	FOR_EACH(Texture->BitMap,it)
		(*it)->Release();
	Texture->BitMap.clear();
	string name = Texture->GetName();
	int n=0;
	if ((n=name.find_first_of("###")) != string::npos)
		name.erase(n);

	if(Option_DprintfLoad)
	{
		//string outName;
		//CreateUniqName(outName,
		//			Texture->GetName(),
		//			Texture->GetSelfIlluminationName(),
		//			Texture->GetLogoName(),
		//			logo_position,
		//			&Texture->skin_color);
		dprintf("Load: %s\n",Texture->GetUniqName().c_str());
	}
	//if (Texture->IsComplexTexture()&&((cTextureComplex*)Texture)->cloned)
	//{
	//	Texture->New(1);
	//	cTexture* baseTexture = GetTexLibrary()->FindTexture(Texture->GetName());
	//	if (baseTexture && baseTexture!=Texture)
	//	{
	//		Texture->BitMap[0] = baseTexture->BitMap[0];
	//		Texture->BitMap[0]->AddRef();
	//		return true;
	//	}
	//	((cTextureComplex*)Texture)->cloned = false;
	//}
	bool bump=Texture->GetAttribute(TEXTURE_BUMP)?true:false;
	bool pos_bump=gb_VisGeneric->PossibilityBump();

	if(Option_UseTextureCache && LoadCache(Texture))
	{
		return true;
	}
/*
	if(strstr(name.c_str(),".TGA"))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char out_name[_MAX_PATH];
		_splitpath(Texture->GetName(),drive,dir,fname,ext);
		_makepath(out_name,drive,dir,fname,".DDS");
		if(IsDDSFile(out_name))
		{
			Texture->SetName(out_name);
			return ReLoadDDS(Texture);
		}
	}
*/
	// загрузить текстуру из файла
	char fName[1024];
	strcpy(fName,name.c_str());

	if(strstr(fName,".DDS"))
	{
		return ReLoadDDS(Texture);//Не слишком корректно работает, так как нет возможности узнать, полупрозрачная ли текстура.
	}
	int err=1;
	if(Texture->IsAviScaleTexture())
	{
		cAviScaleFileImage avi_images;
		if (avi_images.Init(Texture->GetName())==false)
		{
			VisError<<"Can`t load avi texture: "<<Texture->GetName()<<VERR_END;
			return false;
		}
		((cTextureAviScale*)Texture)->Init(&avi_images);
		Texture->SetTimePerFrame(avi_images.GetTime()/avi_images.GetFramesCount());
		((cTextureAviScale*)Texture)->SetTotalTime(avi_images.GetTime());
		Texture->SetAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);
		if(avi_images.GetBitPerPixel()==32) Texture->SetAttribute(TEXTURE_ALPHA_BLEND);
		Texture->New(1);
		if (Texture->IsTexture2D())
			Texture->SetNumberMipMap(1);
		else
			Texture->SetNumberMipMap(avi_images.GetMaxMipLevels());
        err = gb_RenderDevice3D->CreateTexture(Texture,&avi_images,-1,-1);
	}else
	if (Texture->IsComplexTexture())
	{
		cComplexFileImage complexFileImage;
		if (!complexFileImage.Init(((cTextureComplex*)Texture)->GetNames(),((cTextureComplex*)Texture)->placeLine))
		{
			VisError<<"Can`t load complex texture: "<<Texture->GetName()<<VERR_END;
			return false;
		}
		((cTextureComplex*)Texture)->Init(&complexFileImage);
		Texture->SetTimePerFrame(0);
		Texture->New(1);
		if(complexFileImage.GetBitPerPixel()==32) Texture->SetAttribute(TEXTURE_ALPHA_BLEND);
		Texture->SetAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);
		if (Texture->IsTexture2D())
			Texture->SetNumberMipMap(1);
		else
			Texture->SetNumberMipMap(complexFileImage.GetMaxMipLevels());
		err = gb_RenderDevice3D->CreateTexture(Texture,&complexFileImage,-1,-1);
		

	}else
	if(Texture->IsScaleTexture())
	{
		cFileImage *FileImage=cFileImage::Create(Texture->GetName());
		if (!FileImage || FileImage->load(Texture->GetName()))
		{
			VisError<<"Can`t load scale texture: "<<Texture->GetName()<<VERR_END;
			return false;
		}
		Vect2f scale = ((cTextureScale*)Texture)->GetCreateScale();
		Texture->SetWidth(FileImage->GetX()*scale.x);
		Texture->SetHeight(FileImage->GetY()*scale.y);
		Texture->SetTimePerFrame(0);
		if(FileImage->GetBitPerPixel()==32) Texture->SetAttribute(TEXTURE_ALPHA_BLEND);
		Texture->New(1);
		Texture->SetNumberMipMap(1);
		err = gb_RenderDevice3D->CreateTexture(Texture,FileImage,-1,-1);
		delete FileImage;
	}else
	if (Texture->IsTexture2D())
	{
		cFileImage *FileImage=cFileImage::Create(Texture->GetName());
		if (!FileImage || FileImage->load(Texture->GetName()))
		{
			VisError<<"Can`t load scale texture: "<<Texture->GetName()<<VERR_END;
			return false;
		}
		//if(strstr(Texture->GetName(),"MAEL_PROGRESS"))
		//{
		//	DWORD* buffer=new DWORD[FileImage->GetX()*FileImage->GetY()];
		//	FileImage->GetTexture(buffer,FileImage->GetLength()-1,FileImage->GetX(),FileImage->GetY());
		//	SaveTga("start.tga",FileImage->GetX(),FileImage->GetY(),(BYTE*)buffer,FileImage->GetBitPerPixel()/8);
		//	delete buffer;
		//}
		

		Texture->SetWidth(FileImage->GetX());
		Texture->SetHeight(FileImage->GetY());
		Texture->SetTimePerFrame(0);
		Texture->New(FileImage->GetLength());
		Texture->SetTotalTime(FileImage->GetTime());
		Texture->SetAttribute(TEXTURE_ALPHA_TEST/*TEXTURE_ALPHA_BLEND*/);
		if(FileImage->GetBitPerPixel()==32) Texture->SetAttribute(TEXTURE_ALPHA_BLEND);
		err = gb_RenderDevice3D->CreateTexture(Texture,FileImage,-1,-1);
		delete FileImage;

	}else
	{
		cFileImage *FileImage=cFileImage::Create(fName, Texture->GetSelfIlluminationName(), 
			Texture->GetLogoName(),
			Texture->GetSkinColorName(),Texture->skin_color, logo_position,logo_angle);

		if(FileImage==NULL)
		{
			Error(Texture);
			return false;
		}
		if (FileImage->GetX() != 1<<ReturnBit(FileImage->GetX()) ||
			FileImage->GetY() != 1<<ReturnBit(FileImage->GetY()) )
		{
			Error(Texture,"wrong width or height");
			return false;
		}
		if(FileImage->GetBitPerPixel()==32)
			Texture->SetAttribute(TEXTURE_ALPHA_BLEND);

		Texture->New(FileImage->GetLength());
		if(FileImage->GetLength()<=1) 
			Texture->SetTimePerFrame(0);
		else
			Texture->SetTimePerFrame((FileImage->GetTime()-1)/(FileImage->GetLength()-1));
		Texture->SetTotalTime(FileImage->GetLength());
		Texture->SetWidth(FileImage->GetX());
		Texture->SetHeight(FileImage->GetY());
		Texture->SetNumberMipMap(min(Texture->GetY(),Texture->GetX())+1);

		err=gb_RenderDevice3D->CreateTexture(Texture,FileImage,-1,-1);

		delete FileImage;
	}


	if(err)
	{
		Error(Texture);
		return false; 
	}
	if (Option_UseTextureCache)
	{
		SaveCache(Texture);
	}
	Texture->Resize(Option_TextureDetailLevel);
	return true;
}

void cTexLibrary::Error(cTexture* texture, const char* message)
{
	if(enable_error)
	{
		VisError<<"Error: cTexLibrary::GetElement()\r\n"<<"Texture is bad: "<<message<<"\r\n"<<texture->GetName();
		const char* pSelf=texture->GetSelfIlluminationName();
		if(pSelf && pSelf[0])
			VisError<<"\r\n"<<pSelf;

		const char* pLogo=texture->GetLogoName();
		if(pLogo && pLogo[0])
			VisError<<"\r\n"<<pLogo;
	
		VisError<<VERR_END;
	}
}

void cTexLibrary::ReloadAllTexture()
{
	MTAuto mtenter(lock);

	//vector<cTexture*>::iterator it;
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		cTexture* p=it->second;
		if(p->GetName() && p->GetName()[0])
		{
			ReLoadTexture(p);
		}
	}
}
bool cTexLibrary::ReLoadDDS(cTexture* Texture)
{
	char* buf=NULL;
	int size;

	if(!RenderFileRead(Texture->GetName(),buf,size))
	{
		Error(Texture);
		return false;
	}

	class CAutoDelete
	{
		char* buf;
	public:
		CAutoDelete(char* buf_):buf(buf_){};
		~CAutoDelete()
		{
			delete[] buf;
		}
	} auto_delete(buf);

	DDSURFACEDESC2* ddsd=(DDSURFACEDESC2*)(1+(DWORD*)buf);
	if(ddsd->ddsCaps.dwCaps2&DDSCAPS2_CUBEMAP)
	{
		LPDIRECT3DCUBETEXTURE9 pCubeTexture=NULL;
		if(FAILED(D3DXCreateCubeTextureFromFileInMemory(gb_RenderDevice3D->lpD3DDevice,buf,size,&pCubeTexture)))
		{
			Error(Texture);
			return false;
		}

		Texture->BitMap.push_back(CREATEIDirect3DTextureProxy((IDirect3DTexture9*)pCubeTexture));
		Texture->SetAttribute(TEXTURE_CUBEMAP);

		D3DSURFACE_DESC desc;
		RDCALL(pCubeTexture->GetLevelDesc(0,&desc));
		Texture->SetWidth(desc.Width);
		Texture->SetHeight(desc.Height);
		return true;
	}else
	{
		LPDIRECT3DTEXTURE9 pTexture=gb_RenderDevice3D->CreateTextureFromMemory(buf,size,!Texture->IsTexture2D());
		if(!pTexture)
		{
			Error(Texture);
			return false;
		}

		int mipmaps=pTexture->GetLevelCount();
		Texture->SetNumberMipMap(mipmaps);
		D3DSURFACE_DESC desc;
		RDCALL(pTexture->GetLevelDesc(0,&desc));
		Texture->SetWidth(desc.Width);
		Texture->SetHeight(desc.Height);

		Texture->BitMap.push_back(CREATEIDirect3DTextureProxy(pTexture));
		return true;
	}

	return false;
}

cTexture* cTexLibrary::CreateNormalMap(int sizex,int sizey)
{
	MTAuto mtenter(lock);
	cTexture *Texture=new cTexture("NormalMap");
	Texture->SetNumberMipMap(Option_MipMapLevel);
	Texture->SetAttribute(TEXTURE_BUMP|TEXTURE_NODDS);
	Texture->New(1);
	Texture->SetTimePerFrame(0);
	Texture->SetWidth(sizex);
	Texture->SetHeight(sizey);
	int err=gb_RenderDevice3D->CreateTexture(Texture,NULL,-1,-1);
	if(err)
	{
		Error(Texture);
		Texture->Release();
		return NULL;
	}

	return Texture;
}

cTexture* cTexLibrary::GetSpericalTexture()
{
	if(!pSpericalTexture)
	{
		int size=128;

		cTexture *Texture=new cTexture;
		Texture->SetAttribute(TEXTURE_NONDELETE);
		Texture->SetNumberMipMap(5);
		Texture->SetAttribute(TEXTURE_32);
		Texture->SetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

		Texture->BitMap.resize(1);
		Texture->BitMap[0]=0;
		Texture->SetWidth(size);
		Texture->SetHeight(size);

		sColor4c* data=new sColor4c[size*size];

		float s2=size/2.0f;
		float is2=1/s2;
		for(int y=0;y<size;y++)
		for(int x=0;x<size;x++)
		{
			float xx=(x-s2)*is2;
			float yy=(y-s2)*is2;
			float s=1-xx*xx-yy*yy;
			if(s<0)
				s=0;
			//s=sqrtf(s);
			BYTE c=round(255*s);
			data[x+y*size].set(c,c,c,c);
		}

		cFileImageData fid(size,size,data);

		if(gb_RenderDevice3D->CreateTexture(Texture,&fid,-1,-1))
		{
			delete data;
			delete Texture;
			return NULL;
		}
		delete data;
		pSpericalTexture=Texture;
	}

	pSpericalTexture->AddRef();
	return pSpericalTexture;
}

cTexture* cTexLibrary::GetWhileTexture()
{
	if(!pWhiteTexture)
	{
		int size=4;

		cTexture *Texture=new cTexture;
		Texture->SetAttribute(TEXTURE_NONDELETE);
		Texture->SetNumberMipMap(2);
		Texture->SetAttribute(TEXTURE_32);
		Texture->SetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

		Texture->BitMap.resize(1);
		Texture->BitMap[0]=0;
		Texture->SetWidth(size);
		Texture->SetHeight(size);

		sColor4c* data=new sColor4c[size*size];
		for(int y=0;y<size;y++)
		for(int x=0;x<size;x++)
		{
			BYTE c=255;
			data[x+y*size].set(c,c,c,c);
		}

		cFileImageData* fid=new cFileImageData(size,size,data);

		__try
		{


			if(gb_RenderDevice3D->CreateTexture(Texture,fid,-1,-1))
			{
				delete Texture;
				return NULL;
			}
		}__finally
		{
			delete data;
			delete fid;
		}

		pWhiteTexture=Texture;
	}

	pWhiteTexture->AddRef();
	return pWhiteTexture;
}

cTexture* cTexLibrary::CreateAlphaTexture(int sizex,int sizey,cFileImage* image,bool dynamic)
{
	cTexture* pTexture=new cTexture;
	pTexture->SetNumberMipMap(1);
	pTexture->SetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_GRAY|TEXTURE_NODDS);
	if(dynamic)
		pTexture->SetAttribute(TEXTURE_DYNAMIC|TEXTURE_D3DPOOL_DEFAULT);

	pTexture->New(1);
	pTexture->SetTimePerFrame(0);
	pTexture->SetWidth(sizex);
	pTexture->SetHeight(sizey);

	if(image)
	{
		xassert(image->GetX()==sizex);
		xassert(image->GetY()==sizey);
	}

	bool err=false;
	err=err || gb_RenderDevice3D->CreateTexture(pTexture,image,-1,-1)!=0;

	if(err)
	{
		RELEASE(pTexture);
		return NULL;
	}
	return pTexture;
}

cTexture* cTexLibrary::CreateTexture(int sizex,int sizey,eSurfaceFormat format,bool dynamic)
{
	cTexture* pTexture=new cTexture;
	pTexture->SetNumberMipMap(1);
	pTexture->SetAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_GRAY|TEXTURE_NODDS);
	if(dynamic)
		pTexture->SetAttribute(TEXTURE_DYNAMIC|TEXTURE_D3DPOOL_DEFAULT);

	pTexture->New(1);
	pTexture->SetTimePerFrame(0);
	pTexture->SetWidth(sizex);
	pTexture->SetHeight(sizey);
	pTexture->SetFmt(format);

	int err=gb_RenderDevice3D->CreateTexture(pTexture,NULL,-1,-1,true);
	if(err) { pTexture->Release(); return 0; }
/*
	int Usage=0;
	D3DPOOL Pool=D3DPOOL_MANAGED;
	if(dynamic)
	{
		Pool=D3DPOOL_DEFAULT;
		Usage|=D3DUSAGE_DYNAMIC;
	}
	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateTexture(sizex,sizey,1,Usage,(D3DFORMAT)format,Pool,&pTexture->BitMap[0],NULL);

	if(FAILED(hr))
	{
		RELEASE(pTexture);
		return NULL;
	}

	pTexture->AddToDefaultPool();
*/
	return pTexture;
}

void cTexLibrary::AddToDefaultPool(cTexture* ptr)
{
	if(!ptr->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
		return;
	MTAuto mtenter(lock);
	if(ptr->GetAttribute(TEXTURE_ADDED_POOL_DEFAULT))
		return;
	ptr->SetAttribute(TEXTURE_ADDED_POOL_DEFAULT);
	default_texture.push_back(ptr);
}

void cTexLibrary::DeleteFromDefaultPool(cTexture* ptr)
{
	if(!ptr->GetAttribute(TEXTURE_D3DPOOL_DEFAULT))
		return;
	MTAuto mtenter(lock);
	xassert(ptr->GetAttribute(TEXTURE_ADDED_POOL_DEFAULT));
	ptr->ClearAttribute(TEXTURE_ADDED_POOL_DEFAULT);
	vector<cTexture*>::iterator it=find(default_texture.begin(),default_texture.end(),ptr);
	if(it!=default_texture.end())
	{
		default_texture.erase(it);
	}else
	{
		xassert(0);
	}
}
cTexture* cTexLibrary::FindTexture(const char* name)
{
	TextureArray::iterator it = textures.find(name);
	if (it != textures.end())
		return it->second;
	return NULL;
}

cTexture* cTexLibrary::GetElement2D(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return NULL; // имя текстуры пустое

	string texture_name;
	normalize_path(pTextureName,texture_name);

	string uniqName;
	CreateUniqName(uniqName,texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	Texture = new cTexture(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(1);
	Texture->SetTexture2D();
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	AddTexture(Texture);
	return Texture;

}
cTexture* cTexLibrary::GetElement2DAviScale(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return NULL; // имя текстуры пустое

	string texture_name;
	normalize_path(pTextureName,texture_name);
	xassert(strstr(texture_name.c_str(), ".AVI"));
	
	string uniqName;
	CreateUniqName(uniqName,texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	Texture = new cTextureAviScale(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(1);
	Texture->SetTexture2D();
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	AddTexture(Texture);
	return Texture;
}

cTextureComplex* cTexLibrary::GetElement2DComplex(vector<string>& textureNames)
{
	MTAuto mtenter(lock);
	start_timer_auto();
	xassert(textureNames.size()  > 0);
	if(textureNames.size()  == 0)
		return NULL;

	vector<string> textureNamesNormal(textureNames.size() );
	string name;
	for(int i=0; i<textureNames.size() ; i++)
	{
		if(textureNames[i].empty())
			continue;
		normalize_path(textureNames[i].c_str(),textureNamesNormal[i]);
		name += textureNamesNormal[i].substr(textureNamesNormal[i].find_last_of("\\"));
	}
	if (name.empty())
		return NULL;

	string uniqName;
	CreateUniqName(uniqName,name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return (cTextureComplex*)Texture;
	}

	Texture = new cTextureComplex(textureNamesNormal);
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(1);
	Texture->SetTexture2D();
	Texture->SetName(name.c_str());
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	AddTexture(Texture);
	return (cTextureComplex*)Texture;
}
cTexture* cTexLibrary::GetElement2DScale(const char *pTextureName,Vect2f scale)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return NULL; // имя текстуры пустое

	string texture_name;
	normalize_path(pTextureName,texture_name);

	string uniqName;
	CreateUniqName(uniqName,texture_name.c_str());
	cTextureScale* Texture = (cTextureScale*)FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return Texture;
	}

	Texture = new cTextureScale(pTextureName);
	Texture->SetUniqName(uniqName);
	Texture->SetScale(scale,Vect2f(1.0f,1.0f));
	Texture->SetNumberMipMap(1);
	Texture->SetTexture2D();
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	AddTexture(Texture);
	return Texture;
}
cTexture* cTexLibrary::GetElement3D(const char *pTextureName,char *pMode)
{
	start_timer_auto();
	MTAuto mtenter(lock);
	bool bump=pMode&&strstr(pMode,"Bump");
	string texture_name;
	cTexture *Texture = NULL;
	
	if(pTextureName==0||pTextureName[0]==0) return NULL; // имя текстуры пустое
	normalize_path(pTextureName,texture_name);

	string uniqName;
	CreateUniqName(uniqName,texture_name.c_str());
	Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		bool cur_bump=Texture->GetAttribute(TEXTURE_BUMP);
		xassert(cur_bump==bump);
		Texture->AddRef();
		return Texture;
	}


	Texture=new cTexture(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(Option_MipMapLevel);
	if(pMode)
	{
		if(strstr(pMode,"UVBump"))
			Texture->SetAttribute(TEXTURE_UVBUMP);
		if(strstr(pMode,"Specular"))
			Texture->SetAttribute(TEXTURE_SPECULAR);
	}
	if(bump) Texture->SetAttribute(TEXTURE_BUMP);
	Texture->SetTexture3D();
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	AddTexture(Texture);

	return Texture;
}
cTexture* cTexLibrary::GetElement3DColor(const char *pTextureName,const char* skin_color_name_,sColor4c* color,const char *SelfIlluminationName, const char* logoName, sRectangle4f* logo_position, float logo_angle)
{

	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое
	cTexture* Texture = NULL;
	string texture_name;
	normalize_path(pTextureName,texture_name);
	string self_illumination_name;
	normalize_path(SelfIlluminationName,self_illumination_name);
	string logo_name;
	normalize_path(logoName,logo_name);
	string skin_color_name;
	normalize_path(skin_color_name_,skin_color_name);

	string outName;
	CreateUniqName(outName,texture_name.c_str(),self_illumination_name.c_str(),logo_name.c_str(),logo_position,color);
	Texture = FindTexture(outName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return Texture;
	}

	Texture=new cTexture(texture_name.c_str());
	if (color)
		Texture->skin_color=*color;
	Texture->SetUniqName(outName);
	Texture->SetNumberMipMap(Option_MipMapLevel);

	Texture->SetSelfIlluminationName(self_illumination_name.c_str());
	Texture->SetLogoName(logo_name.c_str());
	Texture->SetSkinColorName(skin_color_name.c_str());

	Texture->SetTexture3D();
	if (!ReLoadTexture(Texture,logo_position,logo_angle))
	{
		delete Texture;
		return NULL;
	}
	

	Texture->SetAttribute(TEXTURE_NONDELETE);
	AddTexture(Texture);
	return Texture;

}
cTexture* cTexLibrary::GetElement3DComplex(vector<string>& textureNames, bool allowResize,bool line)
{
	start_timer_auto();
	MTAuto mtenter(lock);
	xassert(textureNames.size() > 0);
	if(textureNames.size() == 0)
		return NULL;
	
	vector<string> textureNamesNormal(textureNames.size() );
	string name;
	for(int i=0; i<textureNames.size() ; i++)
	{
		if(textureNames[i].empty())
			continue;
		normalize_path(textureNames[i].c_str(),textureNamesNormal[i]);
		name += textureNamesNormal[i].substr(textureNamesNormal[i].find_last_of("\\"));
	}

	if (name.empty())
		return NULL;
	string uniqName;
	CreateUniqName(uniqName,name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return (cTextureComplex*)Texture;
	}
	Texture = new cTextureComplex(textureNamesNormal);
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(3);
	Texture->SetTexture3D();
	Texture->SetName(name.c_str());
	Texture->PutAttribute(TEXTURE_DISABLE_DETAIL_LEVEL,allowResize);
	((cTextureComplex*)Texture)->placeLine = line;
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	Texture->SetAttribute(TEXTURE_NONDELETE);
	AddTexture(Texture);
	return Texture;

}
cTexture* cTexLibrary::GetElement3DAviScale(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return NULL; // имя текстуры пустое

	string texture_name;
	normalize_path(pTextureName,texture_name);
	xassert(strstr(texture_name.c_str(), ".AVI"));

	string uniqName;
	CreateUniqName(uniqName,texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if (Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	Texture = new cTextureAviScale(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->SetNumberMipMap(3);
	Texture->SetTexture3D();
	if (!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return NULL;
	}
	Texture->SetAttribute(TEXTURE_NONDELETE);
	AddTexture(Texture);
	return Texture;

}

bool cTexLibrary::ResizeTexture(cTexture* Texture, cTexture* outTexture)
{
	if (!Texture||!outTexture)
		return false;
	int resizeLevel = Option_TextureDetailLevel;
	int bits = Texture->GetX()-resizeLevel;
	if (bits< 2)
	{
		delete outTexture;
		return false;
	}
	outTexture->SetWidth(1<<bits);
	bits = Texture->GetY()-resizeLevel;
	if (bits< 2)
	{
		delete outTexture;
		return false;
	}
	outTexture->SetHeight(1<<bits);
	int mm = Texture->GetNumberMipMap()-resizeLevel;
	if (mm < 1)
	{
		delete outTexture;
		return false;
	}
	outTexture->SetNumberMipMap(mm);
	outTexture->SetAttribute(Texture->GetAttribute());
	outTexture->New(1);
	if(gb_RenderDevice3D->CreateTexture(outTexture,NULL,-1,-1))
	{
		delete outTexture;
		return false;
	}
	for(int nMipMap=resizeLevel;nMipMap<Texture->GetNumberMipMap();nMipMap++)
	{
		LPDIRECT3DSURFACE9 lpSurfaceSrc = NULL;
		LPDIRECT3DSURFACE9 lpSurfaceDst = NULL;
		LPDIRECT3DTEXTURE9 lpD3DTextureSrc=GETIDirect3DTexture(Texture->GetDDSurface(0));
		LPDIRECT3DTEXTURE9 lpD3DTextureDst=GETIDirect3DTexture(outTexture->GetDDSurface(0));
		RDCALL( lpD3DTextureSrc->GetSurfaceLevel( nMipMap, &lpSurfaceSrc) );
		RDCALL( lpD3DTextureDst->GetSurfaceLevel( nMipMap-resizeLevel, &lpSurfaceDst) );
		RDCALL( D3DXLoadSurfaceFromSurface(lpSurfaceDst,NULL,NULL,lpSurfaceSrc,NULL,NULL,D3DX_DEFAULT ,0));
		lpSurfaceSrc->Release();
		lpSurfaceDst->Release();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
cComplexFileImage::cComplexFileImage() : cAviScaleFileImage()
{
}
cComplexFileImage::~cComplexFileImage()
{
}
bool cComplexFileImage::Init(vector<string>& names,bool line)
{
	if (names.size() == 0)
		return false;
	vector<cFileImage*> tempImages;
	tempImages.resize(names.size(),NULL);
	cTextureAtlas atlas;
	vector<Vect2i> texture_size(names.size(),Vect2i(0,0));
	int max_dx=0,max_dy=0;

	bool ret = true;
	for(int i=0; i<(int)names.size(); i++)
	{
		if (names[i].empty())
			continue;
		cFileImage*& image=tempImages[i];
		string texture_name;
		normalize_path(names[i].c_str(),texture_name);
		
		image = cFileImage::Create(texture_name.c_str());
		if (!image || image->load(names[i].c_str()) == -1 || !image->GetX())
		{
			ret = false;
			break;
		}

		texture_size[i].set(image->GetX(),image->GetY());
		max_dx=max(max_dx,image->GetX());
		max_dy=max(max_dy,image->GetY());
	}
	
	if (ret)
	{
		dx = texture_size[0].x;
		dy = texture_size[0].y;

		if(!atlas.Init(texture_size,line))
		{
			VisError<<"Слишком много кадров!!!\r\n"<<"Попытка создать - "<<(int)tempImages.size()
				<<",а доступно - "<<atlas.GetNumTextures()<<" кадров\r\n";
			for(int i=0;i<names.size();i++)
			{
				VisError<<names[i].c_str()<<",\r\n";
			}
			VisError<<VERR_END;
		}
		maxMipLevels = atlas.GetMaxMip();
		n_count=atlas.GetNumTextures();
		x = atlas.GetDX();
		y = atlas.GetDY();

		delete[] dat;
		positions.resize(n_count);
		sizes.resize(n_count);
		DWORD* lpBuf = new  DWORD[max_dx*max_dy];
		for(int i=0;i<n_count;++i)
		{
			cFileImage*& image=tempImages[i];
			if(image)
			{
				image->GetTexture(lpBuf,i,image->GetX(),image->GetY());
				atlas.FillTexture(i,lpBuf,image->GetX(),image->GetY());
			}
			positions[i]=atlas.GetUV(i);
			sizes[i]=atlas.GetSize(i);
		}
		delete[] lpBuf;

		int num_point=atlas.GetDX()*atlas.GetDY();
		dat = new DWORD[num_point];
		memcpy(dat,atlas.GetTexture(),num_point*4);
	}
	
	for (int i=0; i<(int)tempImages.size(); i++)
	{
		delete tempImages[i];
	}
	return ret;
}

void cTexLibrary::SetNotCacheMutableTextures(bool no)
{
	not_cache_mutable_textures=no;
}

void cTexLibrary::GetCachedTextures(vector<string>& names)
{
	for(CacheDataTable::iterator it=cacheDataList.begin();it!=cacheDataList.end();++it)
	{
		names.push_back(it->second.textureName);
	}
}
void cTexLibrary::SetWorkCacheDir(const char* dir)
{
	workCacheDir = dir;
	workCacheDir += "\\Textures";
}
void cTexLibrary::SetBaseCacheDir(const char* dir)
{
	baseCacheDir = dir;
}
