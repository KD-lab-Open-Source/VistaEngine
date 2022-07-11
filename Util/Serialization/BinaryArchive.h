#ifndef __BINARY_ARCHIVE_H__
#define __BINARY_ARCHIVE_H__

#include <vector>
#include <stack>
#include "Handle.h"
#include "Serialization.h"
#include "Saver.h"
#include "StaticMap.h"

const int VERSION			= 1001;
const int STRUCT_RIGID		= 1002;
const int TYPE_TABLE		= 1003;
const int TYPE_TABLE_ITEM	= 1004;
const int SAVED_DATA		= 1005;
const int NAME_TABLE		= 1006;
const int NAME_TABLE_ITEM	= 1007;
const int BIT_ELEMENT		= 1008;

const int archiveVersion = 2;

typedef StaticMap<std::string,WORD> TableMap;
void InitStandartTypeTableMap();
int GetSizeStandartType(int type);

enum {
	stCHAR = 1,
	stSCHAR,
	stUCHAR,
	stSHORT,
	stSSHORT,
	stUSHORT,
	stINT,
	stSINT,
	stUINT,
	stLONG,
	stSLONG,
	stULONG,
	stFLOAT,
	stDOUBLE,
	stBOOL,
	stEND,
};
class BinaryMemorySaver : public MemorySaver
{
public:
	void push_standart(const unsigned long id)
	{
		MemorySaver::write(&id, sizeof(id));
		size_t offset = position_ - block_;
		stack_.push_back(offset);
	}

	size_t pop_standart()
	{
		size_t old_offset = position_ - block_;
		size_t size = old_offset - stack_.back();
		stack_.pop_back();
		return size;
	}

};
struct BlockData
{
	DWORD ID;
	int size;
	BYTE* data;
};
class LoaderBlock
{
protected:
	BYTE *begin,*cur;
	int size;
public:
	LoaderBlock::LoaderBlock(BYTE* data,DWORD _size)
		:begin(data),cur(NULL),size(_size)
	{
	}

	LoaderBlock::LoaderBlock(BlockData bd)
	{
		begin=bd.data;
		size=bd.size;
		cur=0;
	}

	bool next(BlockData& bd)
	{
		if(cur==NULL)cur=begin;
		if(cur>=begin+size)return false;

		int id = *((int*)cur)&0x0000FFFF;
		if(id>255)
		{
			bd.ID = *((int*)cur);
			bd.size = *((int*)(cur+sizeof(DWORD)));
			bd.data = cur+2*sizeof(DWORD);
			cur+=bd.size+2*sizeof(DWORD);
			return true;
		}else
		{
			bd.ID = *((int*)cur);
			bd.size = GetSizeStandartType(id);
			bd.data = cur+sizeof(DWORD);
			cur+=bd.size+sizeof(DWORD);
			return true;
		}

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

};
class LoaderIterator
{
	BlockData ld;
	int rd_cur_pos;
public:
	LoaderIterator(BlockData& _ld):ld(_ld),rd_cur_pos(0){};
	LoaderIterator() : rd_cur_pos(0){};
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
		if(rd_cur_pos>=ld.size)return "";

		LPCSTR s=(LPCSTR)(ld.data+rd_cur_pos);
		rd_cur_pos+=strlen(s)+1;
		return s;
	}
	inline void operator>>(string& s){s=LoadString();}

	bool read(void *data, int size)
	{
		bool ok=(rd_cur_pos+size) <= ld.size;
		if (ok)
			memcpy(data,ld.data+rd_cur_pos,size);
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

struct LoadStackData
{
	LoadStackData(BlockData ldd) : ld(ldd),headPos(0){};
	LoadStackData(BYTE* data,DWORD size) : ld(data,size),headPos(0){};
	int headPos;
	LoaderBlock ld;
};
class Loader
{
public:
	Loader::Loader() : buffer_(NULL){};
	Loader::~Loader()
	{
		delete buffer_;
	};
	bool open(const char* fname)
	{
		XStream ff(0);
		if(!ff.open(fname, XS_IN))
			return false;
		size_ = (int)ff.size();
		buffer_ = new BYTE[size_];
		ff.read(buffer_, ff.size());
		stack_.push_back(LoadStackData(buffer_,size_));
		return true;
	}
	bool openNextBlock(int blockID)
	{
		assert(stack_.size()>0);
		LoaderBlock& rd = stack_.back().ld;
		while(rd.next(curData))
		{
			if(curData.ID == blockID)
			{
				LoadStackData lsd(curData);
				stack_.push_back(lsd);
				curIterator = LoaderIterator(curData);
				return true;
			}
		}
		return false;
	}
	void closeBlock()
	{
		assert(stack_.size()>0);
		stack_.pop_back();
	}
	void resetPointer()
	{
		assert(stack_.size()>0);
		stack_.back().ld.SetCurPos(stack_.back().headPos);
	}
	void setHead()
	{
		assert(stack_.size()>0);
		stack_.back().headPos = curIterator.getCurPos();
		resetPointer();
	}
	unsigned char* getData()
	{
		return (unsigned char*)buffer_;
	}
	int getSize()
	{
		return size_;
	}

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
	inline void operator>>(string& s){read(s);}
	bool read(void *data, int size)
	{
		return curIterator.read(data, size);
	}

protected:
	BYTE* buffer_;
	int size_;
	BlockData curData;
	LoaderIterator curIterator;
	vector<LoadStackData> stack_;

	template<class T>
		__forceinline void read(T& x){
			curIterator >> x;
		}
};
struct TypeInfo
{
	TypeInfo(WORD ID_, int size_): ID(ID_),size(size_){};
	TypeInfo(): ID(0),size(0){};
	WORD ID;
	int size;
};

class BinaryOArchive : public Archive
{
public:
	BinaryOArchive(const char* fname);
	~BinaryOArchive();

	void open(const char* fname); 
	bool close();  // true if there were changes, so file was updated

	bool isOutput() const { return true; }

	void setNodeType (const char*) {}
protected:
	bool processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
	{
		openNode(name, "Binary");
		saver << buffer.size();
		saver.write(buffer.buffer(),buffer.size());
		closeNode(name);
		return true;
	}
	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) {
		openNode(name, descriptor.typeName());
		saver << descriptor.name(value);
		closeNode(name);
		return true;
	}
	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) {
		openNode(name, "BitVector");
		ComboStrings::const_iterator it;
		ComboStrings strings = descriptor.nameCombinationStrings(flags);
		FOR_EACH(strings, it) {
			saver.push(BIT_ELEMENT);
			saver << it->c_str();
			saver.pop();
		}
		closeNode(name);
		return true;
	}

	bool processValue(char& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stCHAR);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(signed char& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stSCHAR);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(signed short& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stSSHORT);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(signed int& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stSINT);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(signed long& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stSLONG);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stUCHAR);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stUSHORT);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stUINT);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stULONG);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(float& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stFLOAT);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(double& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stDOUBLE);
		saver << value;
		closeNodeStandart(name);
		return true;
	}
	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
		openNode(name, "PrmString");
		saver << t.value();
		closeNode(name);
		return true;
	}
	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
		openNode(name, "ComboListString");
		saver << t.value();
		closeNode(name);
		return true;
	}
	bool processValue(string& str, const char* name, const char* nameAlt) { 
		openNode(name, "string");
		saver << str;
		closeNode(name);
		return true;
	}
	bool processValue(bool& value, const char* name, const char* nameAlt) {
		openNodeStandart(name,stBOOL);
		saver << value;
		closeNodeStandart(name);
		return true;
	}


protected:
	bool openNode(const char* name, const char* typeName) 
	{
		saver.push(CreateStoredID(name,typeName));
		return true;
	}
	bool openNodeStandart(const char* name, int typeID) 
	{
		WORD nameID = GetNameIndex(name);
		int storeID = (typeID | int(nameID)<<16);
		saver.push_standart(storeID);
		return true;
	}
	void closeNode(const char* name) 
	{
		saver.pop();
	}
	void closeNodeStandart(const char* name) 
	{
		saver.pop_standart();
	}
	bool openStruct(const char* name, const char* nameAlt, const char* typeName) {
		saver.push(CreateStoredID(name,typeName));
		return true;
	}
	void closeStruct (const char* name) {
		saver.pop();
	}
	bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& size, bool readOnly) {
		saver.push(CreateStoredID(name,"container"));
		saver << size;
		return true;
	}
	void closeContainer(const char* name) {
		saver.pop();
	}
	int openPointer (const char* name, const char* nameAlt,
		const char* baseName, const char* baseNameAlt,
		const char* typeName, const char* typeNameAlt) {

		saver.push(CreateStoredID(name,baseName));
		saver << typeName;
		return NULL_POINTER;
	}

	void closePointer (const char* name, const char* baseName, const char* derivedName) {
		saver.pop();
	}



private:
	BinaryMemorySaver saver;
	string fileName;
	TableMap typeTable;
	TableMap nameTable;

	WORD GetTypeIndex(const char* typeName)
	{
		TableMap::iterator it;
		it = typeTable.find(typeName);
		if(it != typeTable.end())
			return it->second;
		static int UnkID(255);	
		UnkID++;
		typeTable[typeName] = UnkID;
		return UnkID;
	}
	WORD GetNameIndex(const char* name)
	{
		TableMap::iterator it;
		it = nameTable.find(name);
		if(it != nameTable.end())
			return it->second;
		static int UnkNameID(0);	
		UnkNameID++;
		nameTable[name] = UnkNameID;
		return UnkNameID;
	}
	bool SaveTypeTable(XStream& ff);
	bool SaveNameTable(XStream& ff);

	__forceinline int CreateStoredID(const char* name, const char* typeName)
	{
		WORD typeID = GetTypeIndex(typeName);
		WORD nameID = GetNameIndex(name);
		return (typeID | int(nameID)<<16);
	}

};

class BinaryIArchive : public Archive
{
public:
	BinaryIArchive(const char* fname = 0);
	~BinaryIArchive();

	bool open(const char* fname);  // true if file exists
	bool close();

	void setVersion(int version) { version_ = version; } // Для сложной конверсии: вручную записывать, выставлять и кастить архив к XPrmIArchive
	int version() const { return version_; }

	unsigned int crc();

protected:
	
	bool processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
	{
		if(!openNode(name, "Binary"))
			return false;
		int size;
		loader >> size;
		buffer.alloc(size);
		loader.read(buffer.buffer(),size);
		closeNode(name);
		return true;
	}

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt);
	bool processValue(char& value, const char* name, const char* nameAlt) 
	{
		if(!openNodeStandart(name,stCHAR))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	} 
	bool processValue(signed char& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stSCHAR))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(signed short& value, const char* name, const char* nameAlt) 
	{
		if(!openNodeStandart(name,stSSHORT))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(signed int& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stSINT))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(signed long& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stSLONG))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stUCHAR))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stUSHORT))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stUINT))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stULONG))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(float& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stFLOAT))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(double& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stDOUBLE))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
		if(!openNode(name,"PrmString"))
			return false;
		string value;
		loader >> value;
		t = value;
		closeNode(name);
		return true;
	}
	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
		if(!openNode(name,"ComboListString"))
			return false;
		string value;
		loader >> value;
		t = value;
		closeNode(name);
		return true;
	}
	bool processValue(string& str, const char* name, const char* nameAlt) { 
		if(!openNode(name,"string"))
			return false;
		loader >> str;
		closeNode(name);
		return true;
	}
	bool processValue(bool& value, const char* name, const char* nameAlt) {
		if(!openNodeStandart(name,stBOOL))
			return false;
		loader >> value;
		closeNode(name);
		return true;
	}
	ComboInts processBitVector (const ComboInts& flags, const ComboStrings& comboList, const ComboStrings& comboListAlt, const char* name, const char* nameAlt) {
		if(!openNode(name, "BitVector"))
			return ComboInts ();
		ComboInts result;
		while(loader.openNextBlock(BIT_ELEMENT))
		{
			string name;
			loader >> name;
			ComboStrings::const_iterator it;
			it = std::find (comboList.begin (), comboList.end (), name);
			if(it != comboList.end ()) {
				result.push_back (std::distance (comboList.begin (), it));
			}
			loader.closeBlock();
		}
		closeNode(name);
		return result;
	}

protected:
	bool openNode(const char* name, const char* typeName)
	{
		int storedID = CreateStoredID(name,typeName);
		unsigned char pass=0;
		do{
			while(loader.openNextBlock(storedID))
			{
				loader.setHead();
				openTypeBlock(BLOCK_NODE);
				needClose.push_back(true);
				return true;
				loader.closeBlock();
			}
			loader.resetPointer();
			pass++;
		}while(pass<2&&getCurrentType() != BLOCK_CONTAINER);
		return false;
	}
	bool openNodeStandart(const char* name, int typeID)
	{
		WORD nameID = GetNameIndex(name);
		int storedID = (typeID | int(nameID)<<16);
		unsigned char pass=0;
		do{
			while(loader.openNextBlock(storedID))
			{
				loader.setHead();
				openTypeBlock(BLOCK_NODE);
				needClose.push_back(true);
				return true;
				loader.closeBlock();
			}
			loader.resetPointer();
			pass++;
		}while(pass<2&&getCurrentType() != BLOCK_CONTAINER);
		return false;
	}
	void closeNode(const char* name)
	{
		if(needClose.back())
		{
			loader.closeBlock();
			closeTypeBlock();
		}
		needClose.pop_back();
	}
	bool openStruct(const char* name, const char* nameAlt, const char* typeName) {
		int storedID = CreateStoredID(name,typeName);
		unsigned char pass=0;
		do{
			while(loader.openNextBlock(storedID))
			{
				loader.setHead();
				openTypeBlock(BLOCK_STRUCT);
				needClose.push_back(true);
				return true;
				loader.closeBlock();
			}
			loader.resetPointer();
			pass++;
		}while(pass<2&&getCurrentType() != BLOCK_CONTAINER);

		needClose.push_back(false);
		return false;
	}
	void closeStruct (const char* name) {
		if(needClose.back())
		{
			loader.closeBlock();
			closeTypeBlock();
		}
		needClose.pop_back();
	}
	bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& size, bool readOnly) {
		int storedID = CreateStoredID(name,"container");
		unsigned char pass=0;
		do{
			while(loader.openNextBlock(storedID))
			{
				loader >> size;
				loader.setHead();
				openTypeBlock(BLOCK_CONTAINER);
				needClose.push_back(true);
				return true;
				loader.closeBlock();
			}
			loader.resetPointer();
			pass++;
		}while(pass<2&&getCurrentType() != BLOCK_CONTAINER);
		needClose.push_back(false);
		return false;
	}
	void closeContainer (const char* name) {
		if(needClose.back())
		{
			loader.closeBlock();
			closeTypeBlock();
		}
		needClose.pop_back();
	}

	int openPointer (const char* name, const char* nameAlt,
		const char* baseName, const char* baseNameAlt,
		const char* typeName, const char* typeNameAlt) {

		int storedID = CreateStoredID(name,baseName);
		unsigned char pass=0;
		do{
			while(loader.openNextBlock(storedID))
			{
				string type_name;
				loader >> type_name;
				loader.setHead();
				if(type_name.empty())
				{
					openTypeBlock(BLOCK_POINTER);
					needClose.push_back(true);
					return NULL_POINTER;
				}
				int result = indexInComboListString (typeName, type_name.c_str());
				if (result == NULL_POINTER) {
					XBuffer msg;
					msg < "ERROR! no such class registered: ";
					msg < type_name.c_str();
					xassertStr (0, static_cast<const char*>(msg));
					openTypeBlock(BLOCK_POINTER);
					needClose.push_back(true);
					return UNREGISTERED_CLASS;
				}
				openTypeBlock(BLOCK_POINTER);
				needClose.push_back(true);
				return result;

				loader.closeBlock();
			}
			loader.resetPointer();
			pass++;
		}while(pass<2&&getCurrentType() != BLOCK_CONTAINER);
		needClose.push_back(false);
		return NULL_POINTER;
	}

	void closePointer (const char* name, const char* baseName, const char* derivedName) {
		if(needClose.back())
		{
			loader.closeBlock();
			closeTypeBlock();
		}
		needClose.pop_back();
	}

private:
	Loader loader;
	int version_;
	TableMap typeTable;
	TableMap nameTable;
	enum TYPE_BLOCK{
		BLOCK_UNKNOWN,
		BLOCK_NODE,
		BLOCK_STRUCT,
		BLOCK_CONTAINER,
		BLOCK_POINTER,
	};

	int GetTypeIndex(const char* typeName)
	{
		TableMap::iterator it;
		it = typeTable.find(typeName);
		if(it != typeTable.end())
			return (int)it->second;
		return -1;
	}
	int GetNameIndex(const char* name)
	{
		TableMap::iterator it;
		it = nameTable.find(name);
		if(it != nameTable.end())
			return (int)it->second;
		return -1;
	}

	bool LoadTypeTable();
	bool LoadNameTable();
	vector<TYPE_BLOCK> currentType;
	void openTypeBlock(TYPE_BLOCK type)
	{
		currentType.push_back(type);
	}
	TYPE_BLOCK getCurrentType()
	{
		if(currentType.empty())
			return BLOCK_UNKNOWN;
		return currentType.back();
	}
	void closeTypeBlock()
	{
		if(currentType.empty())
			return;
		currentType.pop_back();
	}
	vector<bool> needClose;

	__forceinline void CreateIDs(int storeID, WORD& nameID, WORD& typeID)
	{
		typeID = storeID & 0x0000ffff;
		nameID = storeID>>16;
	}
	__forceinline int CreateStoredID(const char* name, const char* typeName)
	{
		WORD typeID = GetTypeIndex(typeName);
		WORD nameID = GetNameIndex(name);
		return (typeID | int(nameID)<<16);
	}
};
#endif //__BINARY_ARCHIVE_H__
