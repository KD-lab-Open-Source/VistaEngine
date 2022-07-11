#include "StdAfxRD.h"
#include "TexLibrary.h"
#include "D3DRender.h"
#include "VisGeneric.h"
#include "FileImage.h"
#include "TextureAtlas.h"
#include "Serialization\BinaryArchive.h"
#include "FileUtils\FileUtils.h"
#include "Render\src\TextureMiniDetail.h"

RENDER_API cTexLibrary* GetTexLibrary()
{
	return &gb_RenderDevice3D->TexLibrary;
}

cTexLibrary::cTexLibrary()
{
	pSpericalTexture=0;
	pWhiteTexture=0;
	enable_error=true;

	cFileImage::InitFileImage();

	cacheDir_ = "cacheData\\Textures\\";
	cacheVersion_ = 8;
	exported_ = false;

	createDirectory("cacheData");
	createDirectory(cacheDir_.c_str());
	for(int ilod=0; ilod < num_lod; ilod++){
		string s = cacheDir_;
		s += '0' + ilod;
		createDirectory(s.c_str());
	}

	string cacheInfo = cacheDir_ + "cacheTexturesInfo";
	BinaryIArchive ia(0);
	if(ia.open(cacheInfo.c_str()))
		serialize(ia);
}

cTexLibrary::~cTexLibrary()
{
	if(pSpericalTexture)
		pSpericalTexture->clearAttribute(TEXTURE_NONDELETE);
	RELEASE(pSpericalTexture);
	if(pWhiteTexture)
		pWhiteTexture->clearAttribute(TEXTURE_NONDELETE);
	RELEASE(pWhiteTexture);
	Free();
	cFileImage::DoneFileImage();

	if(Option_UseTextureCache && !exported_)
		saveCacheInfo(false);
}

void cTexLibrary::serialize(Archive& ar)
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

void cTexLibrary::clearCacheInfo()
{
	cacheDataTable_.clear();
	textures.clear();
}

void cTexLibrary::saveCacheInfo(bool exported)
{
	exported_ = exported;
	string cacheInfo = cacheDir_ + "cacheTexturesInfo";
	BinaryOArchive oa(cacheInfo.c_str());
	serialize(oa);
}

bool cTexLibrary::EnableError(bool enable)
{
	bool b=enable_error;
	enable_error=enable;
	return b;
}

void cTexLibrary::FreeOne(bool free_all)
{
	FILE* f = 0;
#ifndef _FINAL_VERSION_
	//f = fopen("texture_notfree.txt","at");
	//fprintf(f,"Texture freeing\n");
#endif 

	int compacted = 0;
	TextureArray::iterator it;
	FOR_EACH(textures,it){
		cTexture*& p=it->second;
		if(p && p->GetRef()==1 && (free_all || !p->getAttribute(TEXTURE_NO_COMPACTED))){
			p->clearAttribute(TEXTURE_NONDELETE);
			if(Option_DprintfLoad)
				dprintf("Unload: %s\n",p->GetUniqueName().c_str());
			deletedTextures.push_back(p->GetUniqueName());
			p->Release();
			p=0;
			compacted++;
		}
		else{
			if(f)
				fprintf(f,"%s - %i\n",p->name(),p->GetRef());
		}
	}

	if(f){
		fprintf(f,"Texture free %i, not free %i\n",compacted,textures.size()-compacted);
		fclose(f);
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

void cTexLibrary::Compact(bool free_all)
{
	MTAuto mtenter(lock);
	FreeOne(free_all);
	//remove_null_element(textures);
	CompactTexturesMap();
}

void cTexLibrary::CompactTexturesMap()
{
	if(deletedTextures.size()==0)
		return;
	for(int i=0; i<deletedTextures.size();i++)
	{
		TextureArray::iterator it = textures.find(deletedTextures[i].c_str());
		if(it != textures.end())
			textures.erase(it);
	}
	deletedTextures.clear();
}

void cTexLibrary::Free()
{
	MTAuto mtenter(lock);
	FreeOne(true);
	textures.clear();
}

void cTexLibrary::Unload(const char* file_name)
{
	MTAuto mtenter(lock);
	string out_patch = normalizePath(file_name);
	string uniqName = CreateUniqueName(out_patch.c_str());
	TextureArray::iterator it = textures.find(uniqName.c_str());
	if(it == textures.end())
		return;

	cTexture* p=it->second;
	p->clearAttribute(TEXTURE_NONDELETE);
	textures.erase(it);
	p->Release();
}

void cTexLibrary::DeleteDefaultTextures()
{
	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		if(it->second&&it->second->getAttribute(TEXTURE_D3DPOOL_DEFAULT))
			gb_RenderDevice3D->DeleteTexture(it->second);
	}
	for(int i=0;i<default_texture.size();i++)
	{
		cTexture* pTexture=default_texture[i];
		xassert(pTexture->getAttribute(TEXTURE_D3DPOOL_DEFAULT));
		gb_RenderDevice3D->DeleteTexture(pTexture);
	}

}

void cTexLibrary::CreateDefaultTextures()
{
	TextureArray::iterator it;
	FOR_EACH(textures,it){
		if(it->second&&it->second->getAttribute(TEXTURE_D3DPOOL_DEFAULT)){
			xassert(0);
			if(it->second->GetRef()>1)
				gb_RenderDevice3D->CreateTexture(it->second,0,-1,-1);
		}
	}
	for(int i=0;i<default_texture.size();i++){
		cTexture* pTexture=default_texture[i];
		xassert(pTexture->getAttribute(TEXTURE_D3DPOOL_DEFAULT));
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
	Texture->setMipmapNumber(1);
	Texture->setAttribute(attr);

	int err=gb_RenderDevice3D->CreateTexture(Texture,0,-1,-1,enable_assert);
	if(err){ 
		Texture->Release(); 
		return 0; 
	}
	return Texture;
}

cTexture* cTexLibrary::CreateTexture(int sizex,int sizey,bool alpha,bool default_pool)
{
	cTexture *Texture=new cTexture;
	Texture->setMipmapNumber(1);
	Texture->setAttribute(TEXTURE_32|TEXTURE_NODDS);
	if(alpha)
		Texture->setAttribute(TEXTURE_ALPHA_BLEND);
	if(default_pool)
		Texture->setAttribute(TEXTURE_D3DPOOL_DEFAULT);

	Texture->BitMap.resize(1);
	Texture->BitMap[0]=0;
	Texture->SetWidth(sizex);
	Texture->SetHeight(sizey);

	if(gb_RenderDevice3D->CreateTexture(Texture,0,-1,-1))
	{
		delete Texture;
		return 0;
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

void cTexLibrary::SaveCache(cTexture* texture,  sRectangle4f* logo_position)
{
	if(texture->frameNumber() > 1)
		return;

	CacheData data;
	data.uniqKey = texture->GetUniqueName();
	data.textureName_ = texture->name();
	data.selfIlluminationName_ = texture->GetSelfIlluminationName();
	data.logoName_ = texture->logoName();
	data.x = texture->GetWidth();
	data.y = texture->GetHeight();
	data.attribute = texture->getAttribute();
	if(texture->IsAviScaleTexture()){
		cTextureAviScale* t=(cTextureAviScale*)texture;
		data.TimePerFrame = texture->GetTimePerFrame();
		data.TotalTime = t->GetTotalTime();
		data.complexTexturesData.resize(t->GetFramesCount());
		for(int i=0;i<t->GetFramesCount();i++){
			data.complexTexturesData[i].rect=t->GetFramePosIndex(i);
			data.complexTexturesData[i].size=t->sizes[i];
		}
	}
	if(texture->IsComplexTexture()){
		cTextureComplex* complexTexture = (cTextureComplex*)texture;
		data.TimePerFrame = texture->GetTimePerFrame();
		data.TotalTime = complexTexture->GetTotalTime();
		data.complexTexturesData.resize(complexTexture->GetFramesCount());
		for(int i = 0; i < complexTexture->GetFramesCount();i++){
			data.complexTexturesData[i].rect=complexTexture->GetFramePosIndex(i);
			data.complexTexturesData[i].size=complexTexture->sizes[i];
			data.complexTexturesData[i].name = complexTexture->GetNames()[i];
			data.complexTexturesData[i].filetime = FileTime(complexTexture->GetNames()[i].c_str());
		}
	}
	else if(!data.textureName_.empty())
		data.textureTime_ = FileTime(data.textureName_.c_str());
	
	data.selfIlluminationTime_ = FileTime(data.selfIlluminationName_.c_str());
	data.logoTime_ = FileTime(data.logoName_.c_str());

	for(int ilod=0; ilod < num_lod; ilod++)
		texture->saveDDS(data.cacheName(cacheDir_.c_str(), ilod).c_str(), ilod);

	cacheDataTable_[texture->GetUniqueName()] = data;
}

string cTexLibrary::CreateUniqueName(const char* textureName, const char* selfName, const char* logoName, const sRectangle4f* logoPosition, const char* skinName, Color4c* color)
{
	if(!textureName || !textureName[0])
		return "";

	XBuffer name;
	name < transliterate(cutPathToResource(textureName).c_str()).c_str();
	if(selfName && selfName[0])
		name < "_SF_" < extractFileBase(selfName).c_str();
	if(logoName && logoName[0]){
		name < "_L_" < extractFileBase(logoName).c_str();
		if(logoPosition){
			name < "_" <= round(logoPosition->min.x*1000) < "_" <= round(logoPosition->min.y*1000);
			name < "_" <= round(logoPosition->max.x*1000) < "_" <= round(logoPosition->max.y*1000);
		}
	}
	if(color && color->a != 0){
		name.SetRadix(16);
		name < "_" <= color->RGBA();
	}
	if(skinName && skinName[0]/* && strcmp(skinName, textureName) != 0*/)
		name < "_S_" < extractFileBase(skinName).c_str();
	return name.c_str();
}

bool cTexLibrary::LoadCache(cTexture* texture)
{
	CacheDataTable::iterator it = cacheDataTable_.find(texture->GetUniqueName());
	if(it == cacheDataTable_.end())
		return false;

	if(!exported_ && !it->second.valid(texture)){
		cacheDataTable_.erase(it);
		return false;
	}

	CacheData& data = it->second;

	texture->New(1);
	texture->setAttribute(data.attribute);
	texture->SetWidth(data.x);
	texture->SetHeight(data.y);
	if(texture->IsAviScaleTexture()){
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

	}
	else if(texture->IsComplexTexture()){
		cTextureComplex* t=(cTextureComplex*)texture;
		//t->Init(data.complexTexturesData);
		t->pos.resize(data.complexTexturesData.size());
		t->sizes.resize(data.complexTexturesData.size());
		for(int i=0; i<data.complexTexturesData.size(); i++)
		{
			for (int j=0; j<t->GetNames().size();j++)
			{
				if(data.complexTexturesData[i].name == t->GetNames()[j])
				{
					t->pos[j]=data.complexTexturesData[i].rect;
					t->sizes[j] = data.complexTexturesData[i].size;
				}
			}
		}
		t->SetTotalTime(data.TotalTime);
		t->SetTimePerFrame(data.TimePerFrame);
	}
	else if(!texture->IsTexture2D()){
		texture->setMipmapNumber(min(texture->GetX(),texture->GetY())+1);
	}
	
	if(!texture->loadDDS(data.cacheName(cacheDir_.c_str(), texture->getAttribute(TEXTURE_DISABLE_DETAIL_LEVEL)?0:Option_TextureDetailLevel).c_str())){
		cacheDataTable_.erase(texture->GetUniqueName());
		return false;
	}
	
	if(!texture->IsTexture2D()){
		texture->SetWidth(max(data.x>>Option_TextureDetailLevel,1));
		texture->SetHeight(max(data.y>>Option_TextureDetailLevel,1));
	}
	return true;
}

void cTexLibrary::AddTexture(cTexture* texture)
{
	xassert(texture);
	texture->setAttribute(TEXTURE_NONDELETE);
	textures[texture->GetUniqueName()] = texture;
	texture->AddRef();
}

bool cTexLibrary::ReLoadTexture(cTexture* texture)
{
	if(Option_DprintfLoad)
		dprintf("Load: %s\n",texture->GetUniqueName().c_str());

	if(Option_UseTextureCache && LoadCache(texture))
		return true;

	if(!texture->reload()){
		Error(texture);
		return false; 
	}

	if(Option_UseTextureCache)
		SaveCache(texture);
	
	texture->Resize(Option_TextureDetailLevel);
	return true;
}

void cTexLibrary::Error(cTexture* texture, const char* message)
{
	if(enable_error){
		VisError<<"Error: cTexLibrary::GetElement()\r\n"<<"Texture is bad: "<<message<<"\r\n"<<texture->name();
		const char* pSelf=texture->GetSelfIlluminationName();
		if(pSelf && pSelf[0])
			VisError<<"\r\n"<<pSelf;

		const char* pLogo=texture->logoName();
		if(pLogo && pLogo[0])
			VisError<<"\r\n"<<pLogo;
	
		VisError<<VERR_END;
	}
}

void cTexLibrary::ReloadAllTexture()
{
	MTAuto mtenter(lock);

	TextureArray::iterator it;
	FOR_EACH(textures,it)
	{
		cTexture* p=it->second;
		if(p->name() && p->name()[0])
		{
			ReLoadTexture(p);
		}
	}
}

cTexture* cTexLibrary::CreateNormalMap(int sizex,int sizey)
{
	MTAuto mtenter(lock);
	cTexture *Texture=new cTexture("NormalMap");
	Texture->setMipmapNumber(Option_MipMapLevel);
	Texture->setAttribute(TEXTURE_BUMP|TEXTURE_NODDS);
	Texture->New(1);
	Texture->SetTimePerFrame(0);
	Texture->SetWidth(sizex);
	Texture->SetHeight(sizey);
	int err=gb_RenderDevice3D->CreateTexture(Texture,0,-1,-1);
	if(err)
	{
		Error(Texture);
		Texture->Release();
		return 0;
	}

	return Texture;
}

cTexture* cTexLibrary::GetSpericalTexture()
{
	if(!pSpericalTexture){
		int size=128;

		cTexture *Texture=new cTexture;
		Texture->setAttribute(TEXTURE_NONDELETE);
		Texture->setMipmapNumber(5);
		Texture->setAttribute(TEXTURE_32);
		Texture->setAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

		Texture->BitMap.resize(1);
		Texture->BitMap[0]=0;
		Texture->SetWidth(size);
		Texture->SetHeight(size);

		Color4c* data=new Color4c[size*size];

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
			return 0;
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
		Texture->setAttribute(TEXTURE_NONDELETE);
		Texture->setMipmapNumber(2);
		Texture->setAttribute(TEXTURE_32);
		Texture->setAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_NODDS);

		Texture->BitMap.resize(1);
		Texture->BitMap[0]=0;
		Texture->SetWidth(size);
		Texture->SetHeight(size);

		Color4c* data=new Color4c[size*size];
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
				return 0;
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
	pTexture->setMipmapNumber(1);
	pTexture->setAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_GRAY|TEXTURE_NODDS);
	if(dynamic)
		pTexture->setAttribute(TEXTURE_DYNAMIC|TEXTURE_D3DPOOL_DEFAULT);

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
		return 0;
	}
	return pTexture;
}

cTexture* cTexLibrary::CreateTexture(int sizex,int sizey,eSurfaceFormat format,bool dynamic)
{
	cTexture* pTexture=new cTexture;
	pTexture->setMipmapNumber(1);
	pTexture->setAttribute(TEXTURE_ALPHA_BLEND|TEXTURE_GRAY|TEXTURE_NODDS);
	if(dynamic)
		pTexture->setAttribute(TEXTURE_DYNAMIC|TEXTURE_D3DPOOL_DEFAULT);

	pTexture->New(1);
	pTexture->SetTimePerFrame(0);
	pTexture->SetWidth(sizex);
	pTexture->SetHeight(sizey);
	pTexture->setFormat(format);

	int err=gb_RenderDevice3D->CreateTexture(pTexture,0,-1,-1,true);
	if(err) { pTexture->Release(); return 0; }
/*
	int Usage=0;
	D3DPOOL Pool=D3DPOOL_MANAGED;
	if(dynamic)
	{
		Pool=D3DPOOL_DEFAULT;
		Usage|=D3DUSAGE_DYNAMIC;
	}
	HRESULT hr=gb_RenderDevice3D->lpD3DDevice->CreateTexture(sizex,sizey,1,Usage,(D3DFORMAT)format,Pool,&pTexture->BitMap[0],0);

	if(FAILED(hr))
	{
		RELEASE(pTexture);
		return 0;
	}

	pTexture->AddToDefaultPool();
*/
	return pTexture;
}

void cTexLibrary::AddToDefaultPool(cTexture* ptr)
{
	if(!ptr->getAttribute(TEXTURE_D3DPOOL_DEFAULT))
		return;
	MTAuto mtenter(lock);
	if(ptr->getAttribute(TEXTURE_ADDED_POOL_DEFAULT))
		return;
	ptr->setAttribute(TEXTURE_ADDED_POOL_DEFAULT);
	default_texture.push_back(ptr);
}

void cTexLibrary::DeleteFromDefaultPool(cTexture* ptr)
{
	if(!ptr->getAttribute(TEXTURE_D3DPOOL_DEFAULT))
		return;
	MTAuto mtenter(lock);
 		xassert(ptr->getAttribute(TEXTURE_ADDED_POOL_DEFAULT));
	ptr->clearAttribute(TEXTURE_ADDED_POOL_DEFAULT);
	vector<cTexture*>::iterator it=find(default_texture.begin(),default_texture.end(),ptr);
	if(it!=default_texture.end())
		default_texture.erase(it);
	else
		xassert(0);
}

cTexture* cTexLibrary::FindTexture(const char* name)
{
	TextureArray::iterator it = textures.find(name);
	if(it != textures.end())
		return it->second;
	return 0;
}

cTexture* cTexLibrary::GetElement2D(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое

	string texture_name = normalizePath(pTextureName);

	string uniqName = CreateUniqueName(texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	//kdWarning("UI", (string("loadTexture() ") + pTextureName).c_str());

	Texture = new cTexture(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(1);
	Texture->SetTexture2D();
	if(!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);
	return Texture;

}

cTexture* cTexLibrary::GetElement2DAviScale(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое

	string texture_name = normalizePath(pTextureName);
	xassert(strstr(texture_name.c_str(), ".AVI"));
	
	string uniqName = CreateUniqueName(texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	Texture = new cTextureAviScale(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(1);
	Texture->SetTexture2D();
	if(!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return 0;
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
		return 0;

	vector<string> textureNamesNormal(textureNames.size() );
	string name;
	for(int i=0; i<textureNames.size() ; i++)
	{
		if(textureNames[i].empty())
			continue;
		textureNamesNormal[i] = normalizePath(textureNames[i].c_str());
		name += textureNamesNormal[i].substr(textureNamesNormal[i].find_last_of("\\"));
	}
	if(name.empty())
		return 0;

	string uniqName = CreateUniqueName(name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		Texture->AddRef();
		return (cTextureComplex*)Texture;
	}

	Texture = new cTextureComplex(textureNamesNormal);
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(1);
	Texture->SetTexture2D();
	Texture->setName(name.c_str());
	if(!ReLoadTexture(Texture)){
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);
	return (cTextureComplex*)Texture;
}

cTexture* cTexLibrary::GetElement2DScale(const char *pTextureName,Vect2f scale)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое

	string texture_name = normalizePath(pTextureName);

	string uniqName = CreateUniqueName(texture_name.c_str());
	cTextureScale* Texture = (cTextureScale*)FindTexture(uniqName.c_str());
	if(Texture){
		Texture->AddRef();
		return Texture;
	}

	Texture = new cTextureScale(pTextureName);
	Texture->SetUniqName(uniqName);
	Texture->SetScale(scale,Vect2f(1.0f,1.0f));
	Texture->setMipmapNumber(1);
	Texture->SetTexture2D();
	if(!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);
	return Texture;
}

cTexture* cTexLibrary::GetElement3D(const char *pTextureName,char *pMode)
{
	start_timer_auto();
	MTAuto mtenter(lock);
	bool bump = pMode && strstr(pMode,"Bump");
	
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое
	
	string texture_name = normalizePath(pTextureName);

	string uniqName = CreateUniqueName(texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		bool cur_bump=Texture->getAttribute(TEXTURE_BUMP);
		xassert(cur_bump==bump);
		Texture->AddRef();
		return Texture;
	}

	Texture=new cTexture(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(Option_MipMapLevel);
	if(pMode)
	{
		if(strstr(pMode,"UVBump"))
			Texture->setAttribute(TEXTURE_UVBUMP);
		if(strstr(pMode,"Specular"))
			Texture->setAttribute(TEXTURE_SPECULAR);
	}
	if(bump) Texture->setAttribute(TEXTURE_BUMP);

	if(pMode&&strstr(pMode,"NoResize"))
		Texture->setAttribute(TEXTURE_DISABLE_DETAIL_LEVEL);
	Texture->SetTexture3D();
	if(!ReLoadTexture(Texture)){
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);

	return Texture;
}

cTexture* cTexLibrary::GetElementMiniDetail(const char* textureName, int tileSize)
{
	MTAuto mtenter(lock);

	if(textureName==0||textureName[0]==0) 
		return 0; 

	string texture_name = normalizePath(textureName);

	string uniqName = CreateUniqueName(texture_name.c_str());
	uniqName += XBuffer() < "_r" <= tileSize;
	cTexture* texture = FindTexture(uniqName.c_str());
	if(texture){
		texture->AddRef();
		return texture;
	}

	texture = new TextureMiniDetail(texture_name.c_str(), tileSize);
	texture->SetUniqName(uniqName);
	texture->setMipmapNumber(Option_MipMapLevel);
	texture->SetTexture3D();
	if(!ReLoadTexture(texture)){
		RELEASE(texture);
		return 0;
	}
	AddTexture(texture);
	return texture;
}

cTexture* cTexLibrary::GetElement3DColor(const char *pTextureName,const char* skin_color_name_,
										 Color4c* color,const char *SelfIlluminationName, const char* logoName, 
										 const sRectangle4f& logo_position, float logo_angle)
{

	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое
	string texture_name = normalizePath(pTextureName);
	string self_illumination_name = normalizePath(SelfIlluminationName);
	string logo_name = normalizePath(logoName);
	string skin_color_name = normalizePath(skin_color_name_);

	string outName = CreateUniqueName(texture_name.c_str(), self_illumination_name.c_str(),
		             logo_name.c_str(), &logo_position,
					 skin_color_name.c_str(), color);
	cTexture* Texture = FindTexture(outName.c_str());
	if(Texture){
		Texture->AddRef();
		return Texture;
	}

	Texture=new cTexture(texture_name.c_str());
	if(color)
		Texture->skin_color=*color;
	Texture->SetUniqName(outName);
	Texture->setMipmapNumber(Option_MipMapLevel);

	Texture->SetSelfIlluminationName(self_illumination_name.c_str());
	Texture->setLogo(logo_name.c_str(), logo_position, logo_angle);
	Texture->SetSkinColorName(skin_color_name.c_str());

	Texture->SetTexture3D();
	if(!ReLoadTexture(Texture))
	{
		delete Texture;
		return 0;
	}
	AddTexture(Texture);
	return Texture;
}

cTexture* cTexLibrary::GetElement3DComplex(vector<string>& textureNames, bool allowResize,bool line)
{
	start_timer_auto();
	MTAuto mtenter(lock);
	xassert(textureNames.size() > 0);
	if(textureNames.size() == 0)
		return 0;
	
	vector<string> textureNamesNormal(textureNames.size() );
	string name;
	for(int i=0; i<textureNames.size() ; i++)
	{
		if(textureNames[i].empty())
			continue;
		textureNamesNormal[i] = normalizePath(textureNames[i].c_str());
		name += textureNamesNormal[i].substr(textureNamesNormal[i].find_last_of("\\"));
	}

	if(name.empty())
		return 0;
	string uniqName = CreateUniqueName(name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		Texture->AddRef();
		return (cTextureComplex*)Texture;
	}
	Texture = new cTextureComplex(textureNamesNormal);
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(3);
	Texture->SetTexture3D();
	Texture->setName(name.c_str());
	Texture->putAttribute(TEXTURE_DISABLE_DETAIL_LEVEL,allowResize);
	((cTextureComplex*)Texture)->placeLine = line;
	if(!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);
	return Texture;

}

cTexture* cTexLibrary::GetElement3DAviScale(const char *pTextureName)
{
	MTAuto mtenter(lock);
	if(pTextureName==0||pTextureName[0]==0) return 0; // имя текстуры пустое

	string texture_name = normalizePath(pTextureName);
	xassert(strstr(texture_name.c_str(), ".AVI"));

	string uniqName = CreateUniqueName(texture_name.c_str());
	cTexture* Texture = FindTexture(uniqName.c_str());
	if(Texture)
	{
		Texture->AddRef();
		return Texture;
	}
	Texture = new cTextureAviScale(texture_name.c_str());
	Texture->SetUniqName(uniqName);
	Texture->setMipmapNumber(3);
	Texture->SetTexture3D();
	if(!ReLoadTexture(Texture))
	{
		RELEASE(Texture);
		return 0;
	}
	AddTexture(Texture);
	return Texture;

}

bool cTexLibrary::ResizeTexture(cTexture* Texture, cTexture* outTexture)
{
	if(!Texture||!outTexture)
		return false;
	int resizeLevel = Option_TextureDetailLevel;
	int bits = Texture->GetX()-resizeLevel;
	if(bits< 2)
	{
		delete outTexture;
		return false;
	}
	outTexture->SetWidth(1<<bits);
	bits = Texture->GetY()-resizeLevel;
	if(bits< 2)
	{
		delete outTexture;
		return false;
	}
	outTexture->SetHeight(1<<bits);
	int mm = Texture->mipmapNumber()-resizeLevel;
	if(mm < 1)
	{
		delete outTexture;
		return false;
	}
	outTexture->setMipmapNumber(mm);
	outTexture->setAttribute(Texture->getAttribute());
	outTexture->New(1);
	if(gb_RenderDevice3D->CreateTexture(outTexture,0,-1,-1))
	{
		delete outTexture;
		return false;
	}
	for(int nMipMap=resizeLevel;nMipMap<Texture->mipmapNumber();nMipMap++)
	{
		IDirect3DSurface9* lpSurfaceSrc = 0;
		IDirect3DSurface9* lpSurfaceDst = 0;
		IDirect3DTexture9* lpD3DTextureSrc = Texture->GetDDSurface(0);
		IDirect3DTexture9* lpD3DTextureDst = outTexture->GetDDSurface(0);
		RDCALL( lpD3DTextureSrc->GetSurfaceLevel( nMipMap, &lpSurfaceSrc) );
		RDCALL( lpD3DTextureDst->GetSurfaceLevel( nMipMap-resizeLevel, &lpSurfaceDst) );
		RDCALL( D3DXLoadSurfaceFromSurface(lpSurfaceDst,0,0,lpSurfaceSrc,0,0,D3DX_DEFAULT ,0));
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
	if(names.size() == 0)
		return false;
	vector<cFileImage*> tempImages;
	tempImages.resize(names.size(),0);
	cTextureAtlas atlas;
	vector<Vect2i> texture_size(names.size(),Vect2i(0,0));
	int max_dx=0,max_dy=0;

	bool ret = true;
	for(int i=0; i<(int)names.size(); i++)
	{
		if(names[i].empty())
			continue;
		cFileImage*& image=tempImages[i];
		string texture_name = normalizePath(names[i].c_str());
		
		image = cFileImage::Create(texture_name.c_str());
		if(!image || image->load(names[i].c_str()) == -1 || !image->GetX())
		{
			ret = false;
			break;
		}

		texture_size[i].set(image->GetX(),image->GetY());
		max_dx=max(max_dx,image->GetX());
		max_dy=max(max_dy,image->GetY());
	}
	
	if(ret)
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

//////////////////////////////////////////////////////////////////////
cTexLibrary::CacheData::CacheData() 
{
	x = 0;
	y = 0;
	TimePerFrame = 0;
	TotalTime = 0;
	attribute = 0;
}

string cTexLibrary::CacheData::cacheName(const char* cacheDir, int ilod) 
{
	xassert(ilod>=0 && ilod<num_lod);
	string s = cacheDir;
	s+='0'+ilod;
	s+="\\";
	s+=uniqKey;
	s+=".DDS";
	return s;
}

void cTexLibrary::CacheData::serialize(Archive& ar)
{
	ar.serialize(uniqKey, "uniqKey", "uniqKey");
	ar.serialize(textureName_, "textureName", "textureName");
	ar.serialize(selfIlluminationName_, "selfIlluminationName", "selfIlluminationName");
	ar.serialize(logoName_, "logoName", "logoName");
	ar.serialize(textureTime_, "textureFileTime", "textureFileTime");
	ar.serialize(selfIlluminationTime_, "selfIlluminationFileTime", "selfIlluminationFileTime");
	ar.serialize(logoTime_, "logoFileTime", "logoFileTime");
	ar.serialize(x, "x", "x");
	ar.serialize(y, "y", "y");
	ar.serialize(TimePerFrame, "TimePerFrame", "TimePerFrame");
	ar.serialize(TotalTime, "TotalTime", "TotalTime");
	ar.serialize(attribute, "attribute", "attribute");
	ar.serialize(complexTexturesData, "complexTexturesData", "complexTexturesData");
}

void cTexLibrary::CacheData::RectAndNames::serialize(Archive& ar) 
{
	ar.serialize(rect, "rect", "rect");
	ar.serialize(size, "size", "size");
	ar.serialize(name, "name", "name");
	ar.serialize(filetime, "filetime", "filetime");
}

bool cTexLibrary::CacheData::valid(const cTexture* texture) const
{
	if(textureTime_ != FileTime(textureName_.c_str()))
		return false;
	if(selfIlluminationTime_ != FileTime(selfIlluminationName_.c_str()))
		return false;
	if(logoTime_ != FileTime(logoName_.c_str()))
		return false;

	if(texture->IsComplexTexture()){
		cTextureComplex* complexTexture = (cTextureComplex*)texture; 
		if(complexTexture->GetNames().size() != complexTexturesData.size())
			return false;
		for(int i = 0; i < complexTexturesData.size(); i++){
			if(complexTexturesData[i].name != complexTexture->GetNames()[i])
				return false;
			if(complexTexturesData[i].filetime != FileTime(complexTexturesData[i].name.c_str()))
				return false;
		}
	}

	return true;
}
