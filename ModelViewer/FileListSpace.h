#ifndef __FILE_LIST_H_INCLUDED__
#define __FILE_LIST_H_INCLUDED__

#include "kdw/Space.h"

namespace kdw{
	class Tree;
	class Label;
}

class Archive;

class FileListSpace : public kdw::Space
{
public:
	FileListSpace();

	void serialize(Archive& ar);
	void onLocationChanged();
	void onFileSelected(const char* fileName);
protected:
	kdw::Tree* fileTree_;
	kdw::Label* locationLabel_;
};

#endif
