#include "StdAfx.h"
#include "BinaryArchive.h"
#include "xMath.h"
#include "Dictionary.h"
#include "crc.h"

int standartTypes[stEND];

void InitStandartTypeTableMap()
{
	standartTypes[stCHAR]	= 1;
	standartTypes[stSCHAR]	= 1;
	standartTypes[stUCHAR]	= 1;
	standartTypes[stSHORT]	= 2;
	standartTypes[stSSHORT]	= 2;
	standartTypes[stUSHORT]	= 2;
	standartTypes[stINT]	= 4;
	standartTypes[stSINT]	= 4;
	standartTypes[stUINT]	= 4;
	standartTypes[stLONG]	= 4;
	standartTypes[stSLONG]	= 4;
	standartTypes[stULONG]	= 4;
	standartTypes[stFLOAT]	= 4;
	standartTypes[stDOUBLE]	= 8;
	standartTypes[stBOOL]	= 1;
}
int GetSizeStandartType(int type)
{
	if(type >= stEND)
		return -1;
	return standartTypes[type];
}

BinaryOArchive::BinaryOArchive(const char* fname)
{
	InitStandartTypeTableMap();
	open(fname);
}
BinaryOArchive::~BinaryOArchive ()
{
	close();
}

void BinaryOArchive::open(const char* fname)
{
	if(fname)
	{
		fileName = fname;
		saver.push(SAVED_DATA);
	}
}

bool BinaryOArchive::close()
{
	if(fileName.empty())
		return false;
	saver.pop();

	MemorySaver versionBlock;
	versionBlock.push(VERSION);
	versionBlock << archiveVersion;
	versionBlock.pop();
	XStream ff(0);
	if(ff.open(fileName.c_str(), XS_OUT)) 
	{
		ff.write(versionBlock.buffer(),versionBlock.length());
		SaveTypeTable(ff);
		SaveNameTable(ff);
		ff.write(saver.buffer(), saver.length());
	} else {
		XBuffer buf;
		buf < "Ошибка открытия файла на запись:\n" < fileName.c_str();
		xassertStr(0, buf);
	}
	fileName = "";
	return !ff.ioError();
}
bool BinaryOArchive::SaveTypeTable(XStream& ff)
{
	MemorySaver fsaver;
	fsaver.push(TYPE_TABLE);
	TableMap::iterator it;
	FOR_EACH(typeTable,it)
	{
		fsaver.push(TYPE_TABLE_ITEM);
		fsaver << it->first;
		fsaver << it->second;
		fsaver.pop();
	}
	fsaver.pop();
	ff.write(fsaver.buffer(),fsaver.length());
	return true;
}
bool BinaryOArchive::SaveNameTable(XStream& ff)
{
	MemorySaver fsaver;
	fsaver.push(NAME_TABLE);
	TableMap::iterator it;
	FOR_EACH(nameTable,it)
	{
		fsaver.push(NAME_TABLE_ITEM);
		fsaver << it->first;
		fsaver << it->second;
		fsaver.pop();
	}
	fsaver.pop();
	ff.write(fsaver.buffer(),fsaver.length());
	return true;
}
BinaryIArchive::BinaryIArchive(const char* fname)
{
	version_ = 0;
	InitStandartTypeTableMap();
	open(fname);
}
BinaryIArchive::~BinaryIArchive()
{
	close();
}
bool BinaryIArchive::open(const char* fname)
{
	if(!loader.open(fname))
		return false;
	if(loader.openNextBlock(VERSION))
	{
		int ver;
		loader >> ver;
		loader.closeBlock();
		if(ver != archiveVersion)
			return false;
	}else
		return false;
	if(!LoadTypeTable())
		return false;
	if(!LoadNameTable())
		return false;
	if(!loader.openNextBlock(SAVED_DATA))
		return false;
	return true;
}

bool BinaryIArchive::close()
{
	return true;
}

bool BinaryIArchive::LoadTypeTable()
{
	if(loader.openNextBlock(TYPE_TABLE))
	{
		while(loader.openNextBlock(TYPE_TABLE_ITEM))
		{
			string typeName;
			WORD typeID;
			loader >> typeName;
			loader >> typeID;
			typeTable[typeName] = typeID;
			loader.closeBlock();
		}
	}else
		return false;
	loader.closeBlock();
	return true;
}
bool BinaryIArchive::LoadNameTable()
{
	if(loader.openNextBlock(NAME_TABLE))
	{
		while(loader.openNextBlock(NAME_TABLE_ITEM))
		{
			string name;
			WORD nameID;
			loader >> name;
			loader >> nameID;
			nameTable[name] = nameID;
			loader.closeBlock();
		}
	}else
		return false;
	loader.closeBlock();
	return true;
}
bool BinaryIArchive::processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) {
	if(!openNode(name, "Enum"))
		return false;
	string str;
	loader >> str;
	value = descriptor.keyByName(str.c_str());
	closeNode(name);
	return true;
}

unsigned int BinaryIArchive::crc() 
{
	return crc32(loader.getData(), loader.getSize(), startCRC32);
}
