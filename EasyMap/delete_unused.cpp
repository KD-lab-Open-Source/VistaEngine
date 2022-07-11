#include "stdafx.h"
#include "Runtime3D.h"
#include <set>
#include <direct.h>
#include "..\render\3dx\lib3dx.h"
#include "..\Util\Serialization\AttribEditorInterface.h"

bool test_mode=false;
void normalize_path(const char* in_patch,string& out_patch);
class Demo3D:public Runtime3D
{
public:
	Demo3D();
	~Demo3D();
	virtual void Init();
	virtual void Quant();
protected:
	cFont* pFont;
};

Runtime3D* CreateRuntime3D()
{
	return new Demo3D;
}

Demo3D::Demo3D()
{
	pFont=NULL;
}

Demo3D::~Demo3D()
{
	RELEASE(pFont);
}

/*
	Object3dx,Logic3dx все кешировать, а потом стирать оригиналы.
	У текстур стирать только те оригиналы, которые не подвергаются преобразованию.
	Подвергающиеся преобразованию не кешировать.
*/
extern FILE* balmer_error_log;
FILE* deleted_files=NULL;

typedef set<string> StringSet;
StringSet used_textures;
vector<string> unused_textures;
vector<string> allobject3dx;

void AddEndSlash(string& s)
{
	char c=*(s.end()-1);
	if(c!='\\' || c!='/')
		s+='\\';
}

void GetModelTextures(string fname)
{
	cObject3dx* pModel=terScene->CreateObject3dx(fname.c_str());
	pModel->SetSkinColor(sColor4c(255,0,0),"resource\\FX\\Textures\\001.tga");
	vector<string> names;
	pModel->GetStatic()->GetTextureNames(names);
	for(vector<string>::iterator it=names.begin();it!=names.end();++it)
	{
		string norm;
		normalize_path(it->c_str(),norm);
		used_textures.insert(norm);
	}
	RELEASE(pModel);
	pModel=terScene->CreateLogic3dx(fname.c_str());
	RELEASE(pModel);
}

void BuildListTextures3dx(string dir,bool only_model_names=false)
{
	_finddata_t fileinfo;
	string mask=dir+"*.3dx";
	long hFile=_findfirst(mask.c_str(),&fileinfo );
	if(hFile<0)return;

	RELEASE(terScene);
	terScene=gb_VisGeneric->CreateScene();

	int numfile=0;
	do{
		if(fileinfo.attrib&_A_SUBDIR)
		{
			continue;
		}else
		{
			string in=dir+fileinfo.name;
			printf("Load %s\n",in.c_str());
			allobject3dx.push_back(in);
			if(!only_model_names)
				GetModelTextures(in.c_str());
			numfile++;
		}
#ifdef _DEBUG
		if(numfile==10)
			break;
#endif
	}while(_findnext( hFile, &fileinfo ) ==0);
	_findclose(hFile);

	RELEASE(terScene);
}

void Delete(const char* file_name,const char* temp_dir)
{
	if(false)
	{
		char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME],ext[_MAX_EXT];
		_splitpath(file_name,drive,dir,fname,ext);
		string udir=drive;
		udir+=dir;
		udir+=temp_dir;

		_mkdir(udir.c_str());
		char new_name[ _MAX_PATH];
		_makepath(new_name,"",udir.c_str(),fname,ext);
		rename(file_name,new_name);
	}else
	{
		fprintf(deleted_files,"%s\n",file_name);
		remove(file_name);
	}
}

void Delete3dx()
{
	FILE* f=fopen("3dx.log","wt");
	if(!f)
	{
		printf("Cannot create file 3dx.log\n");
		exit(1);
	}

	for(vector<string>::iterator it=allobject3dx.begin();it!=allobject3dx.end();++it)
	{
		string& s=*it;
		Delete(s.c_str(),"cached");
		fprintf(f,"%s\n",s.c_str());
	}
	fclose(f);
}

void TestListTextures3dx()
{
	RELEASE(terScene);
	terScene=gb_VisGeneric->CreateScene();

	for(vector<string>::iterator it=allobject3dx.begin();it!=allobject3dx.end();++it)
	{
		string& s=*it;
		printf("Test %s\n",s.c_str());
//		pLibrary3dx->PreloadElement(s.c_str(), sColor4c(255,0,0),"resource\\FX\\Textures\\001.tga");
		GetModelTextures(s.c_str());
	}
	RELEASE(terScene);
}

void BuildListTexturesEffect(string dir)
{
	_finddata_t fileinfo;
	string mask=dir+"*.effect";
	long hFile=_findfirst(mask.c_str(),&fileinfo );
	if(hFile<0)return;

	RELEASE(terScene);
	terScene=gb_VisGeneric->CreateScene();
	int numfile=0;
	do{
		if(fileinfo.attrib&_A_SUBDIR)
		{
			if(_stricmp(fileinfo.name,".")==0)continue;
			if(_stricmp(fileinfo.name,"..")==0)continue;
			continue;
		}else
		{
			string in=dir+fileinfo.name;
			printf("Load %s\n",in.c_str());
			vector<string> names;
			GetAllTextureNamesEffectLibrary(in.c_str(),names);
			for(vector<string>::iterator it=names.begin();it!=names.end();++it)
			{
				string norm;
				normalize_path(it->c_str(),norm);
				used_textures.insert(norm);
			}

			numfile++;
		}
	}while(_findnext( hFile, &fileinfo ) ==0);
	_findclose(hFile);

	RELEASE(terScene);
}

class EmptyTerraInterface:public TerraInterface
{
public:
	virtual int SizeX(){return 1024;};
	virtual int SizeY(){return 1024;};
	virtual int GetZ(int x,int y){return 0;};//Высота в точке x,y
	virtual float GetZf(int x,int y){return 0;};

	virtual void GetZMinMax(int tile_x,int tile_y,int tile_dx,int tile_dy,BYTE& out_zmin,BYTE& out_zmax){};
	
	virtual class MultiRegion* GetRegion(){return NULL;};

	virtual void GetTileColor(char* tile,DWORD line_size,int xstart,int ystart,int xend,int yend,int step){};
	virtual void postInit(class cTileMap* tm){};

	//0..65536 - 
	virtual void GetTileZ(char* Texture,DWORD pitch,int xstart,int ystart,int xend,int yend,int step){};
};

void TestListTexturesEffect(string dir)
{
	_finddata_t fileinfo;
	string mask=dir+"*.effect";
	long hFile=_findfirst(mask.c_str(),&fileinfo );
	if(hFile<0)return;

	RELEASE(terScene);
	terScene=gb_VisGeneric->CreateScene();
	cTileMap* tilemap=terScene->CreateMap(new EmptyTerraInterface);

	int numfile=0;
	do{
		if(fileinfo.attrib&_A_SUBDIR)
		{
			if(_stricmp(fileinfo.name,".")==0)continue;
			if(_stricmp(fileinfo.name,"..")==0)continue;
			continue;
		}else
		{
			string in=dir+fileinfo.name;
			printf("Test %s\n",in.c_str());

			EffectKey * ek=gb_EffectLibrary->Get(in.c_str(),1.0f,"resource\\FX\\Textures");
			cEffect* fx=terScene->CreateEffectDetached(*ek,NULL);
			terScene->AttachObj(fx);
			RELEASE(fx);
			numfile++;
		}
	}while(_findnext( hFile, &fileinfo ) ==0);
	_findclose(hFile);

	RELEASE(tilemap);
	RELEASE(terScene);
}

void BuildListUnusedTextures(string dir)
{
	_finddata_t fileinfo;
	string mask=dir+"textures\\*.*";
	long hFile=_findfirst(mask.c_str(),&fileinfo );
	if(hFile<0)return;

	cTexLibrary::UncachedTextures& uncached=GetTexLibrary()->GetUncachedMutableTextures();

	do{
		if(fileinfo.attrib&_A_SUBDIR)
		{
			if(_stricmp(fileinfo.name,".")==0)continue;
			if(_stricmp(fileinfo.name,"..")==0)continue;
			continue;
		}else
		{
			string in=dir+"textures\\";
			in+=fileinfo.name;
			string norm;
			normalize_path(in.c_str(),norm);

			if(used_textures.find(norm)==used_textures.end())
			{
				if(uncached.find(norm)==uncached.end())
					unused_textures.push_back(norm);
				else
				{
					printf("Texture is uncached %s\n",norm.c_str());
				}
			}
		}
	}while(_findnext( hFile, &fileinfo ) ==0);
	_findclose(hFile);
}

void PrintTexturesList()
{
	for(StringSet::iterator it=used_textures.begin();it!=used_textures.end();++it)
	{
		const string& name=*it;
		printf("%s\n",name.c_str());
	}

	for(vector<string>::iterator it=unused_textures.begin();it!=unused_textures.end();++it)
	{
		const string& name=*it;
		printf("U %s\n",name.c_str());
	}
	
}

void MoveUnusedList(string dir)
{
	for(vector<string>::iterator it=unused_textures.begin();it!=unused_textures.end();++it)
	{
		const string& name=*it;
		Delete(name.c_str(),"unused");
	}
	unused_textures.clear();
	used_textures.clear();
}

void MoveCachedListTextures()
{
	cTexLibrary::UncachedTextures& uncached=GetTexLibrary()->GetUncachedMutableTextures();
	vector<string> unused_textures;
	GetTexLibrary()->GetCachedTextures(unused_textures);

	if(unused_textures.empty())
		return;
	for(vector<string>::iterator it=unused_textures.begin();it!=unused_textures.end();++it)
	{
		const string& name=*it;
		if(uncached.find(name)!=uncached.end())
		{
			printf("Cache uncache %s\n",name.c_str());
			continue;
		}

		Delete(name.c_str(),"cached");
	}
}

void EraseDir(string dir)
{
	AddEndSlash(dir);
	_finddata_t fileinfo;
	string mask=dir;
	mask+="*.*";
	long hFile=_findfirst(mask.c_str(),&fileinfo );
	if(hFile>=0)
	{
		do{
			if(!(fileinfo.attrib&_A_SUBDIR))
			{
				string in=dir;
				in+=fileinfo.name;
				remove(in.c_str());
			}
		}while(_findnext( hFile, &fileinfo ) ==0);
		_findclose(hFile);
	}
}

void Demo3D::Init()
{
	//string out_patch;
	//normalize_path("J:\\Maelstrom\\EasyMap\\resource\\Models\\Textures\\\\Lagyt.tga",out_patch);
	//return;

	if(!balmer_error_log)balmer_error_log=fopen("error.log",test_mode?"at":"wt");

	vector<string> adir;
	adir.push_back("resource\\Models");
	adir.push_back("resource\\TerrainData\\Models");
	adir.push_back("resource\\TerrainData\\Sky");
	adir.push_back("resource\\ui\\Models");
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetTextureDetailLevel(2);
	bool process_effect=true;

	if(!test_mode)
	{
		deleted_files=fopen("deleted.log","wt");
		gb_VisGeneric->SetWorkCacheDir("resource\\cacheData\\baseCache");

		EraseDir("cacheData\\Models");
		EraseDir("cacheData\\Textures");
		GetTexLibrary()->SetNotCacheMutableTextures(true);

		if(process_effect)
		{
			BuildListTexturesEffect("resource\\FX\\");
			MoveUnusedList("resource\\FX\\");
		}

		for(int i=0;i<adir.size();i++)
		{
			string dir=adir[i];
			AddEndSlash(dir);
			BuildListTextures3dx(dir);
			BuildListUnusedTextures(dir);
			MoveUnusedList(dir);
		}


		MoveCachedListTextures();
		PrintTexturesList();
	//	terScene=gb_VisGeneric->CreateScene(); 
		pLibrary3dx->Compact();
		pLibrarySimply3dx->Compact();
		GetTexLibrary()->Compact(NULL,true);

		Delete3dx();
		if(deleted_files)
			fclose(deleted_files);
		deleted_files=NULL;
	}else
	{
		gb_VisGeneric->SetBaseCacheDir("resource\\cacheData");

		FILE* f=fopen("3dx.log","rt");
		if(!f)
		{
			printf("Cannot open 3dx.log\n");
			exit(1);
		}

		while(!feof(f))
		{
			const size=512;
			char str[size];
			fgets(str,size-1,f);
			for(char* c=str;*c;c++)if(*c==10)*c=0;
			if(str[0])
				allobject3dx.push_back(str);
		}
		fclose(f);
		//test load again
		TestListTextures3dx();
		if(process_effect)
			TestListTexturesEffect("resource\\FX\\");
	}

	if(balmer_error_log)
	{
		long pos=ftell(balmer_error_log);
		fclose(balmer_error_log);
		if(pos==0)
			remove("error.log");
	}
	balmer_error_log=NULL;
	PostMessage(g_hWnd,WM_QUIT,0,0);
}

void Demo3D::Quant()
{
	__super::Quant();
	terRenderDevice->Fill(128,160,135,0);
	terRenderDevice->BeginScene();

//	__super::DrawFps(256,0);
	terRenderDevice->EndScene();
	terRenderDevice->Flush();
}

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT );
int main(int argc , char *argv[])
{
	if(argc>1)
	{
		char* c=argv[1];
		if(strcmp(c,"-t")==0)
		{
			test_mode=true;
		}
	}

	extern Vect2i winmain_size_window;
	winmain_size_window.set(64,64);
	return WinMain( NULL, 0, 0, 0);
}

class AttribEditorInterface & __cdecl attribEditorInterface(void) 
{ 
	return *(AttribEditorInterface*)0;
}