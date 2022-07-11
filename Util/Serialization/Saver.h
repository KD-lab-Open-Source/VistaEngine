#ifndef __SAVER_H_INCLUDED__
#define __SAVER_H_INCLUDED__
//** 1999 Creator - Balmer **//

//#include "StdAfx.h"
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#include <my_STL.h>
#include <vector>
#include <string>
#include <xmath.h>

#include <stdio.h>
using namespace std;
#include "Saver.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <assert.h>

class Saver{
public:
	virtual ~Saver() {}
	Saver& operator<<(const char *x){
        if(x)
            write(x, strlen(x)+1);
        else
            write("",strlen("")+1);
        return *this;
    }
	Saver& operator<<(const string& x) {*this<<x.c_str(); return *this;}
	Saver& operator<<(bool x)          {write(x); return *this;};
	Saver& operator<<(char x)          {write(x); return *this;};
	Saver& operator<<(unsigned char x) {write(x); return *this;};
	Saver& operator<<(int x)           {write(x); return *this;};
	Saver& operator<<(unsigned int x)  {write(x); return *this;};
	Saver& operator<<(short x)         {write(x); return *this;};
	Saver& operator<<(unsigned short x){write(x); return *this;};
	Saver& operator<<(long x)          {write(x); return *this;};
	Saver& operator<<(unsigned long x) {write(x);return *this;};
	Saver& operator<<(const float& x)  {write(x);return *this;};
	Saver& operator<<(const double& x) {write(x);return *this;};

	Saver& operator<<(const Vect3f& x){write(x);return *this;};
	Saver& operator<<(const Vect2f& x){write(x);return *this;};
	Saver& operator<<(const MatXf& x){write(x);return *this;};

    template<class T>
    __forceinline int write(T& x){
        return write(reinterpret_cast<const void*>(&x), sizeof(x));
    }

	virtual int write(const void* data,int size) = 0;
	virtual void push(const unsigned long id) = 0; //Вызывать при начале записи блока
	virtual size_t pop() = 0; //Вызывать при окончании записи блока

	DWORD GetData() {return m_Data;}
	DWORD SetData(DWORD dat) {return m_Data = dat;}
protected:
	DWORD m_Data;
};
//void SaveString(Saver& s,LPCSTR str,DWORD ido)
//{
//	if(str==NULL)return;
//	s.push(ido);
//	s<<str;
//	s.pop();
//}
class FileSaver : public Saver
{
public:
	FileSaver(const char* name)
	{
		file_=0;
		Init(name);
	}
	FileSaver()
	{
		file_=0;
	};
	~FileSaver()
	{
		if(file_)
			fclose(file_);
	}

    // virtuals
	int write(const void* data,int size){
        return fwrite(data, 1,size, file_);
    }
    template<class T>
    __forceinline int write(T& x){
        return fwrite(reinterpret_cast<const void*>(&x), 1, sizeof(x), file_);
    }



	bool Init(const char* name)
	{
		close();
		stack_.clear();
		file_ = fopen(name,"w+b");
		return file_!=NULL;
	}

	void close()
	{
		if(file_)
			fclose(file_);
		file_=0;
	}

	void push(const unsigned long id)
	{
		fwrite(&id, sizeof(id), 1, file_);
		push();
	}

	size_t pop()
	{
		fpos_t old_position;
		fgetpos(file_, &old_position);

		int n = stack_.size()-1;
		DWORD min = stack_[n];
		fpos_t tt = min-4;
		fsetpos(file_, &tt);
		DWORD size = DWORD(old_position) - min;
		write(size);

		stack_.pop_back();

		fsetpos(file_, &old_position);
		return size;
	}

private:
	void push()
	{
		DWORD w=0;
		write(w);
		fpos_t t;
		fgetpos(file_,&t);

		stack_.push_back((DWORD)t);
	}
	FILE* file_;

	vector<DWORD> stack_;
};

class MemorySaver : public Saver
{
public:
	MemorySaver::MemorySaver(size_t initial_size=124)
	{
		block_ = (char*)::malloc(initial_size);
		assert(block_);
		position_ = block_;
		allocated_size_ = initial_size;
	}

	MemorySaver::~MemorySaver()
	{
		::free(block_);
	}

    //
    void* buffer() { return block_; };
    size_t length() { return position_ - block_; }

    template<class T>
    __forceinline int write(T& x){
        return MemorySaver::write(reinterpret_cast<const void*>(&x), sizeof(x));
    }

	void push(const unsigned long id)
	{
		MemorySaver::write(&id, sizeof(id));
		unsigned long null = 0;
		write(null);
		size_t offset = position_ - block_;
		stack_.push_back(offset);
	}

	size_t pop()
	{
		size_t old_offset = position_ - block_;
		int n = stack_.size() - 1;
		size_t min = stack_.back();
		position_ = block_ + (min - 4);
		size_t size = old_offset - min;
		write(size);
		stack_.pop_back();
		position_ = block_ + old_offset;
		return size;
	}

	int write(const void* data, int size)
	{
		if(block_ + allocated_size_ < position_ + size){
			int newsize = max(allocated_size_+1024*(size/1024+2),allocated_size_*2);
			reallocate(newsize);
		}
		memcpy(position_, data, size);
		position_ += size;
		return size;
	}


protected:
	void reallocate(size_t new_size)
	{
		size_t offset = position_ - block_;
		block_ = (char*)realloc(block_, new_size);
		assert(block_);
		position_ = block_ + offset;
		allocated_size_ = new_size;
	}

	char* block_;
	char* position_;
	size_t allocated_size_;
    /// вектор смещений, вместо вектора указателей
    vector<size_t> stack_;
};

//void SaveString(Saver& s,LPCSTR str,DWORD ido);

#pragma pack(1)
#pragma warning(disable: 4200)
struct CLoadData
{
	DWORD id;
	int size;
	BYTE data[];
};

#pragma warning(default: 4200)
#pragma pack()

class CLoadDirectory
{
protected:
	BYTE *begin,*cur;
	int size;
public:
	CLoadDirectory::CLoadDirectory(BYTE* data,DWORD _size)
		:begin(data),cur(NULL),size(_size)
	{
	}

	CLoadDirectory::CLoadDirectory(CLoadData* ld)
	{
		begin=ld->data;
		size=ld->size;
		cur=0;
	}

	CLoadData* next()
	{
		if(cur==NULL)cur=begin;
		if(cur>=begin+size)return NULL;

		CLoadData* cl=(CLoadData*)cur;
		cur+=cl->size+2*sizeof(DWORD);
		return cl;
	}


	BYTE* GetData(){return begin;};
	int GetDataSize(){return size;};
	int GetCurPos(){
		if(!cur)
			return 0;
		return begin-cur;
	}
	bool SetCurPos(int pos)
	{
		if(pos>size)
			return false;
		cur = begin+pos;
		return true;
	}
	void rewind(){cur=0;};

	CLoadData* find(DWORD id)
	{
		while(CLoadData* ld=next())
		if(ld->id==id)
			return ld;
		rewind();
		while(CLoadData* ld=next())
		if(ld->id==id)
			return ld;
		return NULL;
	}
};

class CLoadDirectoryFile:public CLoadDirectory
{
public:
	CLoadDirectoryFile::CLoadDirectoryFile()
		:CLoadDirectory(0,0)
	{
	}

	CLoadDirectoryFile::~CLoadDirectoryFile()
	{
		delete begin;
	}

	bool Load(LPCSTR filename)
	{
		int file=_open(filename,_O_RDONLY|_O_BINARY);
		if(file==-1)return false;

		size=_lseek(file,0,SEEK_END);
		if(size<0)return false;
		_lseek(file,0,SEEK_SET);
		begin=new BYTE[size];
		_read(file,begin,size);
		_close(file);

		cur=0;
		return true;
	}
};


class CLoadIterator
{
	CLoadData* ld;
	int rd_cur_pos;
public:
	CLoadIterator(CLoadData* _ld):ld(_ld),rd_cur_pos(0){};
	inline void operator>>(bool& i){read(i);}
	inline void operator>>(char& i){read(i);}
	inline void operator>>(unsigned char& i){read(i);}
	inline void operator>>(signed char& i){read(i);}
	inline void operator>>(short& i){read(i);}
	inline void operator>>(unsigned short& i){read(i);}
	inline void operator>>(int& i){read(i);}
	inline void operator>>(unsigned int& i){read(i);}
	inline void operator>>(long& i){read(i);}
	inline void operator>>(unsigned long& i){read(i);}
	inline void operator>>(float& i){read(i);}
	inline void operator>>(double& i){read(i);}
	inline void operator>>(void *& i){read(i);}

	LPCSTR LoadString()
	{
		if(rd_cur_pos>=ld->size)return "";

		LPCSTR s=(LPCSTR)(ld->data+rd_cur_pos);
		rd_cur_pos+=strlen(s)+1;
		return s;
	}

	inline void operator>>(string& s){s=LoadString();}
	inline void operator>>(Vect3f& i){read(i);}
	inline void operator>>(Vect2f& i){read(i);}
	inline void operator>>(Vect2i& i){read(i);}
	inline void operator>>(MatXf& i){read(i);}

	bool read(void *data, int size)
	{
		bool ok=(rd_cur_pos+size) <= ld->size;
		if (ok)
			memcpy(data,ld->data+rd_cur_pos,size);
		rd_cur_pos+=size;
		return ok;
	}
	int getCurPos(){return rd_cur_pos;}
protected:
    ///Читает лишь в случае, если rd_cur_pos+sizeof(x)<=ld->size
    template<class T>
    __forceinline void read(T& x){
		read(&x,sizeof(x));
    }

};

#define IF_FIND_DIR(id)  \
if(CLoadData* load_data=dir.find(id))  \
   for (CLoadDirectory dir = load_data; load_data; load_data = NULL) 
#define IF_FIND_DATA(id)  \
if(CLoadData* load_data=dir.find(id))  \
   for (CLoadIterator rd = load_data; load_data; load_data = NULL) 

/*
IF_FIND_DIR
IF_FIND_DATA для укорочения кода предназначены.

Пример 
saver.push(ID);
saver<<a;
saver.pop();

IF_FIND_DATA(ID)
{
	rd>>a;
}
*/

template<class T>	
void operator>>(CLoadIterator& it,vector<T>& v)
{
	DWORD size=0;
	it>>size;
	v.resize(size);
	for(int i=0;i<size;i++)
	{
		it>>v[i];
	}
}

template<class T>
Saver& operator<<(Saver& s,vector<T>& v){
	DWORD size = (DWORD)v.size();
	s << size;
	vector<T>::iterator it;
	FOR_EACH(v, it){
		s << *it;
	}
	return s;
}

#endif
