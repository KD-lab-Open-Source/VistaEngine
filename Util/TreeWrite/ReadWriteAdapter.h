#pragma once
#include <stdio.h>

class WriteAdapter
{
public:
	virtual void close()=0;
	virtual size_t write(const void* data,size_t data_size)=0;
};

class ReadAdapter
{
public:
	virtual void close()=0;
	virtual size_t read(void* data,size_t data_size)=0;
	virtual size_t size()=0;
};

class ReadWriteFactory
{
public:
	virtual ReadAdapter* open_read(const char* file_name)=0;
	virtual WriteAdapter* open_write(const char* file_name)=0;
};

class FileWriteAdapter:public WriteAdapter
{
	FILE* f;
public:
	FileWriteAdapter(FILE* ff):f(ff){}
	virtual void close()
	{
		fclose(f);
		delete this;
	}
	virtual size_t write(const void* data,size_t data_size)
	{
		return fwrite(data,data_size,1,f);
	}
};

class FileReadAdapter:public ReadAdapter
{
	FILE* f;
	int file_size;
public:
	FileReadAdapter(FILE* ff):f(ff)
	{
		fseek(f,0,SEEK_END);
		file_size=ftell(f);;
		fseek(f,0,SEEK_SET );
	}
	virtual void close()
	{
		fclose(f);
		delete this;
	}

	virtual size_t read(void* data,size_t data_size)
	{
		return fread(data,data_size,1,f);
	}
	virtual size_t size()
	{
		return file_size;
	}
};

class FileReadWriteFactory:public ReadWriteFactory
{
public:
	ReadAdapter* open_read(const char* file_name)
	{
		FILE* f=fopen(file_name,"rb");
		if(f)
			return new FileReadAdapter(f);
		return NULL;
	}

	WriteAdapter* open_write(const char* file_name)
	{
		FILE* f=fopen(file_name,"wb");
		if(f)
			return new FileWriteAdapter(f);
		return NULL;
	}
};

__declspec(selectany)
FileReadWriteFactory default_read_write;