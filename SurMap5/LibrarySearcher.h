#ifndef __LIBRARY_SEARCHER_H_INCLUDED__
#define __LIBRARY_SEARCHER_H_INCLUDED__

#include "Handle.h"
#include "Functor.h"
#include "LibraryWrapper.h"

class Archive;
class EditableLibrary;

typedef std::list<LibraryBookmark> SearchResultItems;
typedef Functor1<void, float> SearchProgressCallback;

class LibrarySearcher : public LibraryWrapper<LibrarySearcher>
{
public:
	void findSubtree(const TreeNode& tree, bool sameName, SearchProgressCallback callback);
	void findSubtree(EditableLibrary& library, SearchResultItems& items, const TreeNode& tree, int& index, int totalCount, SearchProgressCallback progressCallback);

	void serialize(Archive& ar);

	SearchResultItems& items() { return searchResultItems_; }
private:
	SearchResultItems searchResultItems_;
	friend LibraryWrapper<LibrarySearcher>;
	LibrarySearcher(){}
};

#endif
