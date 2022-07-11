#ifndef __MULTI_ARCHIVE_H__
#define __MULTI_ARCHIVE_H__

#include "Handle.h"
#include "Serialization.h"
#include "BinaryArchive.h"
#include "XPrmArchive.h"

// ѕсевдо-архивы, предоставл€ющие минимальную функциональность (UDT-serialize)

class MultiOArchive 
{
public:
	MultiOArchive();
	MultiOArchive(const char* fname, const char* binSubDir, const char* binExtention);
	~MultiOArchive();

	void open(const char* fname, const char* binSubDir, const char* binExtention); 
	bool close();  // true if there were changes, so file was updated

	template<class T>
	void serialize(T& t, const char* name, const char* nameAlt) {
		//bar_.serialize(t, name, nameAlt);
		xar_.serialize(t, name, nameAlt);
	}

	Archive& xprmArchive() { return xar_; }
	//Archive& binaryArchive() { return bar_; }

private:
	//BinaryOArchive bar_;
	XPrmOArchive xar_;
};

class MultiIArchive 
{
public:
	MultiIArchive();
	MultiIArchive(const char* fname, const char* binSubDir, const char* binExtention);
	~MultiIArchive();

	bool open(const char* fname, const char* binSubDir, const char* binExtention);  // true if file exists
	bool close();

	void setVersion(int version) { ar_->setVersion(version); } 

	unsigned int crc() { return crc_; }

	template<class T>
	void serialize(T& t, const char* name, const char* nameAlt) {
		ar_->serialize(t, name, nameAlt);
	}

	Archive& archive() { return *ar_; }

protected:
	Archive* ar_;
	unsigned int crc_;
};

#endif //__MULTI_ARCHIVE_H__
