#include "StdAfxRD.h"
#include "ShaderStorage.h"
#include "ShaderData.h"
#include "D3DRender.h"

cShaderStorageInternal::cShaderStorageInternal()
{
}

cShaderStorageInternal::~cShaderStorageInternal()
{
	Clear();
}

void cShaderStorageInternal::SHADER::Clear()
{
	RELEASE(vertex_shader);
	RELEASE(pixel_shader);
}

void cShaderStorageInternal::Clear()
{
	vector<SHADER>::iterator its;
	FOR_EACH(shaders,its)
	{
		its->Clear();
	}
	shaders.clear();
	defines.clear();
	constants.clear();
}

bool cShaderStorageInternal::Load(CLoadDirectory file,const char* file_name_)
{
	Clear();
	file_name=file_name_;

	int cur_mul=1;

	while(CLoadData* ld=file.next())
	switch(ld->id)
	{
	case SD_CONSTANT_BIND:
		{
			CLoadIterator rd(ld);
			DWORD size;
			rd>>size;
			constants.resize(size);
			for(int i=0;i<size;i++)
			{
				NAMED_CONSTANT& c=constants[i];
				rd>>c.name;
				rd>>c.handle.begin_register;
				rd>>c.handle.num_register;
			}
		}
		break;
	case SD_MACROS:
		{
			CLoadDirectory dir(ld);
			while(CLoadData* ld=dir.next())
			switch(ld->id)
			{
			case SD_ONE_MACROS:
				{
					DEFINE def;
					def.mul_index=cur_mul;
					CLoadDirectory dir(ld);
					while(CLoadData* ld=dir.next())
					switch(ld->id)
					{
					case SD_ONE_MACROS_NAME:
						{
							CLoadIterator rd(ld);
							rd>>def.name;
							rd>>def.num_index;
							rd>>def.static_definition;
						}
						break;
					}

					if(def.num_index>1)
					{
						cur_mul*=def.num_index;
						defines.push_back(def);
					}else
					{
						int k=0;
					}

				}
				break;
			}

		}
		break;
	case SD_SHADER:
		{
			CLoadDirectory dir(ld);
			SHADER shader;
			bool passed=true;
			const char* version_to_debug=0;
			while(CLoadData* ld=dir.next())
			switch(ld->id)
			{
			case SD_SHADER_MACROS_VALUE:
				{
					CLoadIterator rd(ld);
					DWORD size;
					rd>>size;
					shader.macros_value.resize(size);
					for(int i=0;i<size;i++)
					{
						rd>>shader.macros_value[i];
					}
				}
				break;
			case SD_SHADER_VERSION:
				{
					const char* version=(const char*)ld->data;
					version_to_debug=version;
					LPCSTR max_version=0;
					xassert(version[0]);
					if(version[0]=='p')
					{
						max_version=D3DXGetPixelShaderProfile(gb_RenderDevice3D->D3DDevice_);
					}else
					{
						xassert(version[0]=='v');
						max_version=D3DXGetVertexShaderProfile(gb_RenderDevice3D->D3DDevice_);
					}

					passed=strcmp(version,max_version)<=0;
				}
				break;
			case SD_SHADER_COMPILED:
				if(passed)
				{
					xassert(shader.uncompiled_shader_data==0);
					shader.uncompiled_shader_data=1+(DWORD*)ld->data;
					shader.is_pixel_shader=false;
				}
				break;
			case SD_SHADER_COMPILED_PIXEL:
				if(passed)
				{
					xassert(shader.uncompiled_shader_data==0);
					shader.is_pixel_shader=true;
					shader.uncompiled_shader_data=1+(DWORD*)ld->data;
				}
				break;
			}

			shaders.push_back(shader);
		}
		break;
	}

	return true;
}

void cShaderStorageInternal::SHADER::CompilePS()
{
	xassert(is_pixel_shader);
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreatePixelShader(uncompiled_shader_data, &pixel_shader));
	uncompiled_shader_data=0;
}

void cShaderStorageInternal::SHADER::CompileVS()
{
	xassert(!is_pixel_shader);
	RDCALL(gb_RenderDevice3D->D3DDevice_->CreateVertexShader(uncompiled_shader_data, &vertex_shader));
	uncompiled_shader_data=0;
}


SHADER_HANDLE cShaderStorageInternal::GetConstHandle(const char* name)
{
	vector<NAMED_CONSTANT>::iterator it;
	FOR_EACH(constants,it)
	{
		string& n=it->name;
		if(n==name)
			return it->handle;
	}

	SHADER_HANDLE sh;
	return sh;
}

INDEX_HANDLE cShaderStorageInternal::GetIndexHandle(const char* def_name)
{
	for(int i=0;i<defines.size();i++)
	{
		DEFINE& d=defines[i];
		if(d.name==def_name)
		{
			if(d.num_index<=1)
				return INDEX_HANDLE();
			INDEX_HANDLE out;
			out.index=i;
			return out;
		}
	}

//����� ����� ���������� ��������� ��-�� ����� ����������	xassert(0);
	return INDEX_HANDLE();
}

cShaderLib::cShaderLib()
{
	ProcessShaderData();
}

cShaderLib::~cShaderLib()
{
	vector<cShaderStorageInternal*>::iterator it;
	FOR_EACH(shaders,it)
	{
		(*it)->Release();
	}
	shaders.clear();
}

cShaderStorage* cShaderLib::Get(const char* file_name)
{
	vector<cShaderStorageInternal*>::iterator it;
	FOR_EACH(shaders,it)
	{
		cShaderStorageInternal* s=*it;
		if(stricmp(s->GetFileName(),file_name)==0)
		{
			s->AddRef();
			return new cShaderStorage(s);
		}

	}

	vector<SHADER_FILE>::iterator itd;
	FOR_EACH(shader_data,itd)
	{
		SHADER_FILE& s=*itd;
		if(stricmp(s.name.c_str(),file_name)==0)
			break;
	}

	if(itd==shader_data.end())
	{
		VisError<<"Cannot found shader\r\n"<<file_name<<VERR_END;
		return 0;
	}

	cShaderStorageInternal* s=new cShaderStorageInternal;
	if(!s->Load(itd->data,file_name))
	{
		delete s;
		VisError<<"Cannot load shader\r\n"<<file_name<<VERR_END;
		return 0;
	}

	shaders.push_back(s);
	s->AddRef();
	return new cShaderStorage(s);
}

#include "all_shaders.inc"
void cShaderLib::ProcessShaderData()
{
	CLoadDirectory dir((BYTE*)shader,shader_size);
	while(CLoadData* ld=dir.next())
	switch(ld->id)
	{
	case SD_SUPER_SHADER:
		{
			SHADER_FILE f;
			f.data=ld;
			CLoadDirectory dir(ld);
			while(CLoadData* ld=dir.next())
			switch(ld->id)
			{
			case SD_SUPER_SHADER_NAME:
				{
					CLoadIterator rd(ld);
					rd>>f.name;
				}
				break;
			}
			shader_data.push_back(f);
		}
		break;
	}

}

//////////////cShaderStorage///////////////
cShaderStorage::cShaderStorage(cShaderStorageInternal* internal_):internal(internal_)
{
	num_dynamic_define=0;
	def_index.resize(internal->defines.size());
	bool no_static_pass=true;
	for(int i=0;i<internal->defines.size();i++)
	{
		bool is_static=internal->defines[i].static_definition;
		if(no_static_pass)
		{
			if(is_static)
				no_static_pass=false;
			else
				num_dynamic_define++;
		}

		if(!no_static_pass)
		{
			xassert(is_static);
		}

		def_index[i].mul_index=internal->defines[i].mul_index;
		def_index[i].cur_index=0;
	}
	static_offset=0;
	is_static_select=false;
}
cShaderStorage::~cShaderStorage()
{
	RELEASE(internal);
}

void cShaderStorage::Select(INDEX_HANDLE index,int value)
{
	xassert(index.index>=0 && index.index<num_dynamic_define);
	def_index[index.index].cur_index=value;
}

int cShaderStorage::GetSelect(INDEX_HANDLE index)
{
	xassert(index.index>=0 && index.index<def_index.size());
	return def_index[index.index].cur_index;
}

cShaderStorageInternal::SHADER& cShaderStorage::GetShader()
{
	int global_index=static_offset;
	vector<DEFINE_INDEX>::iterator end_it=def_index.begin()+num_dynamic_define;
	for(vector<DEFINE_INDEX>::iterator it=def_index.begin();it!=end_it;it++)
	{
		global_index+=it->cur_index*it->mul_index;
	}

	xassert(global_index>=0 && global_index<internal->shaders.size());
	return internal->shaders[global_index];
}


void cShaderStorage::BeginStaticSelect()
{
	is_static_select=true;
}

void cShaderStorage::StaticSelect(const char* name,int value)
{
	xassert(is_static_select);
	int index=GetIndexHandle(name).index;
	if(index==-1)
	{
		xassert(0);
		return;
	}

	xassert(index>=num_dynamic_define && index<def_index.size());
	def_index[index].cur_index=value;
}

void cShaderStorage::EndStaticSelect()
{
	is_static_select=true;

	static_offset=0;
	for(int i=num_dynamic_define;i<def_index.size();i++)
	{
		DEFINE_INDEX& c=def_index[i];
		static_offset+=c.cur_index*c.mul_index;
	}
}
