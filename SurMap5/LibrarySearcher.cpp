#include "stdafx.h"
#include "LibrarySearcher.h"

#include "mfc\ObjectsTreeCtrl.h"
#include "LibraryBookmark.h"
#include "EditArchive.h"

#include "LibraryWrapper.h"

WRAP_LIBRARY(LibrarySearcher, "LibrarySearcher", "LibrarySearcher", "Scripts\\TreeControlSetups\\LibrarySearcher", 0, false);

struct TreePathLeaf{
	TreePathLeaf(const char* _name = "", int _index = -1)
	: name(_name)
	, index(_index)
	{
	}
	const char* name;
	int index;
};



//////////////////////////////////////////////////////////////////////////////

