#include "stdafx.h"

#include "Serialization/Serialization.h"
#include "Serialization/RangedWrapper.h"
#include "Serialization/StringTableImpl.h"
#include "XZip.h"

#include "ZipConfig.h"

WRAP_LIBRARY(ZipConfigTable, "ZipConfigTable", "��������� zip �������", "Scripts\\Content\\ZipConfig", 0, 0);

ZipConfig::ZipConfig(const char* name) : StringTableBase(name),
	filesMask_("*.*"),
	excludeFilesMask_(""),
	compressionLevel_(0)
{
}

ZipConfig::~ZipConfig()
{
}

void ZipConfig::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(zipName_, "zipName", "��� zip �����");
	ar.serialize(path_, "path", "���� � ������");
	ar.serialize(filesMask_, "filesMask", "�������� �����");
	ar.serialize(excludeFilesMask_, "excludeFilesMask", "��������� �����");
	ar.serialize(RangedWrapperi(compressionLevel_, 0, 9), "compressionLevel", "������� ������");
}

bool ZipConfig::isEmpty() const
{
	return zipName_.empty() || path_.empty() || filesMask_.empty();
}

bool ZipConfig::initArchives()
{
	ZipConfigTable::Strings::const_iterator it;
	for(it = ZipConfigTable::instance().strings().begin(); it != ZipConfigTable::instance().strings().end(); ++it){
		if(!it->isEmpty()){
			// minizip (ioapi.c) opens the .pak with a raw fopen(). Off-Windows the
			// config's "Resource\\worlds.pak" won't open: '\' is a literal filename
			// character (not a separator) and component case may differ from disk, so
			// the archive never mounts -- taking every pak-resident asset with it
			// (world raster caches, models, textures, sounds). NormalizePath rewrites
			// the path to its real POSIX spelling (separators + per-component case).
			string zipName = it->zipName();
#ifndef _WIN32
			zipName = NormalizePath(zipName.c_str());
#endif
			XZipArchiveManager::instance().openArchive(zipName.c_str());
		}
	}

	return true;
}

namespace ZipConfigCallback{

	static ZipConfig::ProgressCallback totalCallback;
	static int numFiles;
	static int currentFile;

	static void oneFileCallback(int percent, const char* fileName){
		if(totalCallback)
			totalCallback(round((float(currentFile) + float(percent) * 0.01f) / float(numFiles) * 100.0f), percent, fileName);
	}

};

bool ZipConfig::makeArchives(ProgressCallback progressCallback)
{
	ZipConfigTable::Strings::const_iterator it;
	
	ZipConfigCallback::totalCallback = progressCallback;
	ZipConfigCallback::numFiles = ZipConfigTable::instance().strings().size();
	ZipConfigCallback::currentFile = 0;

	for(it = ZipConfigTable::instance().strings().begin(); it != ZipConfigTable::instance().strings().end(); ++it){
		if(!it->isEmpty()){
			XZipArchiveMaker make(it->zipName(), it->path(), it->filesMask(), it->excludeFilesMask(), it->compressionLevel(), true, &ZipConfigCallback::oneFileCallback);
			make.buildArchive();
		}
		++ZipConfigCallback::currentFile;
	}

	return true;
}

