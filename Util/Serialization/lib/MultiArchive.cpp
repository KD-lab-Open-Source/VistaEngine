#include "StdAfx.h"
#include "MultiArchive.h"

string makeBinName(const char* fileName, const char* subDir, const char* extention)
{
	string str = fileName;
	if(strlen(subDir)){
		unsigned int pos = str.rfind("\\");
		if(pos == string::npos)
			pos = 0;
		str.insert(pos, "\\");
		++pos;
		str.insert(pos, subDir);
	}
	unsigned int pos = str.rfind(".");
	if(pos != string::npos && pos != 0 && str[pos - 1] != '.')
		str.erase(pos, str.size());
	if(strlen(extention)){
		str += ".";
		str += extention;
	}
	return str;
}

MultiOArchive::MultiOArchive()
: xar_(0)//, bar_(0)
{}

MultiOArchive::MultiOArchive(const char* fname, const char* binSubDir, const char* binExtention)
: xar_(0)//, bar_(0)
{
	open(fname, binSubDir, binExtention);
}

MultiOArchive::~MultiOArchive()
{
	close();
}

void MultiOArchive::open(const char* fname, const char* binSubDir, const char* binExtention)
{
	xar_.open(fname);
	//bar_.open(makeBinName(fname, binSubDir, binExtention).c_str());
}

bool MultiOArchive::close()
{
	return xar_.close();// && bar_.close();
}

MultiIArchive::MultiIArchive()
: ar_(0), crc_(0)
{}

MultiIArchive::MultiIArchive(const char* fname, const char* binSubDir, const char* binExtention)
: ar_(0), crc_(0)
{
	open(fname, binSubDir, binExtention);
}

MultiIArchive::~MultiIArchive()
{
	close();
	delete ar_;
}

bool MultiIArchive::open(const char* fname, const char* binSubDir, const char* binExtention)
{
//#ifdef _FINAL_VERSION_ // В финале нельзя проверять наличие файла (старфорс) и считается, что версия подготовлена корректно
//	ar_ = new BinaryIArchive(binName);
//	return true;
//#else
//	BinaryIArchive* bar = new BinaryIArchive();
//	if(bar->open(makeBinName(fname, binSubDir, binExtention).c_str())){
//		ar_ = bar;
//		crc_ = bar->crc();
//		return true;
//	}
//	else{
//		delete bar;
		XPrmIArchive* xar = new XPrmIArchive();
		if(xar->open(fname)){
			ar_ = xar;
			crc_ = xar->crc();
			return true;
		}
//		else{
//			delete xar;
//			return false;
//		}
//	}
//#endif
	return false;
}

bool MultiIArchive::close()
{
	return ar_ ? ar_->close() : false;
}

