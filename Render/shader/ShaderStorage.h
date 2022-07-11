#ifndef __SHADER_STORAGE_H_INCLUDED__
#define __SHADER_STORAGE_H_INCLUDED__

struct SHADER_HANDLE
{
	int begin_register;
	int num_register;
	SHADER_HANDLE(){begin_register=0;num_register=0;}
};

struct INDEX_HANDLE
{
	int index;
	INDEX_HANDLE(){index=-1;}
	bool is(){return index>=0;}
};

class cShaderStorageInternal:public cUnknownClass
{
public:
	cShaderStorageInternal();
	~cShaderStorageInternal();

	bool Load(CLoadDirectory file,const char* file_name_);
	SHADER_HANDLE GetConstHandle(const char* name);
	INDEX_HANDLE GetIndexHandle(const char* def_name);

	const char* GetFileName() const {return file_name.c_str();}
public:
	void Clear();

	string file_name;
	struct SHADER
	{
	protected:
		IDirect3DVertexShader9* vertex_shader;
		IDirect3DPixelShader9*  pixel_shader;
	public:
		vector<int> macros_value;

		DWORD *uncompiled_shader_data;
		bool is_pixel_shader;

		__forceinline IDirect3DPixelShader9* GetPS()
		{
			if(uncompiled_shader_data)
				CompilePS();

			xassert(pixel_shader);
			return pixel_shader;
		}
		__forceinline IDirect3DVertexShader9* GetVS()
		{
			if(uncompiled_shader_data)
				CompileVS();

			xassert(vertex_shader);
			return vertex_shader;
		}
		

		SHADER()
		{
			is_pixel_shader=false;
			vertex_shader=NULL;
			pixel_shader=NULL;
			uncompiled_shader_data=NULL;
		}

		void Clear();
	protected:
		void CompilePS();
		void CompileVS();
	};

	vector<SHADER> shaders;

	struct DEFINE
	{
		string name;
		int num_index;
		int mul_index;
		bool static_definition;
		DEFINE(){num_index=mul_index=0;static_definition=false;}
	};

	vector<DEFINE> defines;

	struct NAMED_CONSTANT
	{
		string name;
		SHADER_HANDLE handle;
	};

	vector<NAMED_CONSTANT> constants;
	friend class cShaderStorage;
};

//Класс хранящий в себе информацию о шейдере с разными дефайнами (функциф Select)
//см ShaderBath.exe shaders.txt
class cShaderStorage:public cUnknownClass
{
public:
	cShaderStorage(cShaderStorageInternal* internal_);
	~cShaderStorage();

	SHADER_HANDLE GetConstHandle(const char* name){return internal->GetConstHandle(name);};
	INDEX_HANDLE GetIndexHandle(const char* def_name){return internal->GetIndexHandle(def_name);};
	void Select(INDEX_HANDLE index,int value);
	int GetSelect(INDEX_HANDLE index);
	LPDIRECT3DVERTEXSHADER9 GetVertexShader(){return GetShader().GetVS();}
	IDirect3DPixelShader9* GetPixelShader(){return GetShader().GetPS();};

	void BeginStaticSelect();
	void StaticSelect(const char* name,int value);
	void EndStaticSelect();
public:
	int static_offset;
	bool is_static_select;
	cShaderStorageInternal* internal;
	struct DEFINE_INDEX
	{
		int mul_index;
		int cur_index;
	};

	int num_dynamic_define;
	vector<DEFINE_INDEX> def_index;
	cShaderStorageInternal::SHADER& GetShader();
};

class cShaderLib
{
public:
	cShaderLib();
	~cShaderLib();
	cShaderStorage* Get(const char* name);
protected:
	vector<cShaderStorageInternal*> shaders;

	struct SHADER_FILE
	{
		string name;
		CLoadData* data;
	};
	vector<SHADER_FILE> shader_data;
	void ProcessShaderData();
};

#endif
