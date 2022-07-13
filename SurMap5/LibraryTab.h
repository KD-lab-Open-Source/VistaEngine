#ifndef __LIBRARY_TAB_H_INCLUDED__
#define __LIBRARY_TAB_H_INCLUDED__

#include "Handle.h"
#include "Functor.h"
#include "LibraryBookmark.h"

typedef std::vector<std::string> ComboStrings;
class EditorLibraryInterface;
class LibraryCustomEditor;
class CLibraryEditorWindow;
class TreeNode;
class Archive;
class Serializeable;
class PopupMenuItem;
class TreeObject;

class LibraryTabBase : public ShareHandleBase{
	typedef LibraryTabBase Self;
public:
    LibraryTabBase(CLibraryEditorWindow* window, const LibraryBookmark& bookmark);
	virtual bool isSearchResults() const{ return false; }
	virtual bool singleLayout() const{ return false; }

	virtual bool isValid() const { return true; }
	virtual const char* title() const { return ""; };

	virtual void setWindow(CLibraryEditorWindow* window) { window_ = window; }
    virtual void setBookmark(const LibraryBookmark& bookmark);
    const LibraryBookmark& bookmark() const { return bookmark_; }

	virtual void onElementSelected(const char* elementName, Serializeable& serializeable) {}
	virtual void onAttribElementChanged(const TreeNode* node) {}
	virtual void onAttribElementSelected(const ComboStrings& path) {};
	virtual void onMenuConstruction(PopupMenuItem& root) {};

    virtual void serialize(Archive& ar);
	virtual void onSelect();
	virtual void onClose() {};
	virtual ~LibraryTabBase() {}
protected:
	void onMenuSaveAsText();

    CLibraryEditorWindow* window_;
    LibraryBookmark bookmark_;
};

class LibraryTabEditable : public LibraryTabBase{
	typedef LibraryTabEditable Self;
public:
    LibraryTabEditable(CLibraryEditorWindow* window = 0, const LibraryBookmark& bookmark = LibraryBookmark());

    const char* title() const;
	bool isValid() const;

    void setBookmark(const LibraryBookmark& bookmark);
    void onElementSelected(const char* elementName, Serializeable& serializeable);
	void onAttribElementSelected(const ComboStrings& path);
	void onAttribElementChanged(const TreeNode* node);

	void onSearchLibraryElement(const char* libraryName, const char* elementName);
	void onSearchTreeNode(const TreeNode* node);

	void onMenuSort();
	void onMenuFindUnused();
	void onMenuConstruction(PopupMenuItem& root);
	void setLibrary(const char* libraryName);

    void serialize(Archive& ar);
    void onSelect();
	void onClose();
    EditorLibraryInterface* library() const;

	static void buildLibraryTree(TreeObject* rootObject, LibraryCustomEditor* customEditor);

protected:
	void saveTabState();
	void loadTabState();

private:
	PtrHandle<LibraryCustomEditor> customEditor_;

	bool firstSelect_;
	typedef std::vector<char> AttribEditorState;
	typedef StaticMap<std::string, AttribEditorState> AttribEditorStates;
	AttribEditorStates attribEditorStates_;
};

typedef std::list<LibraryBookmark> SearchResultItems;
typedef Functor1<void, float> SearchProgressCallback;

class LibraryTabSearch : public LibraryTabBase{
public:
	LibraryTabSearch(CLibraryEditorWindow* window = 0, const LibraryBookmark& bookmark = LibraryBookmark());
	bool isSearchResults() const{ return true; }
	bool singleLayout() const{ return true; }
	
	void findUnused(const char* libraryName, SearchProgressCallback progress);
	void findTreeNode(const TreeNode& tree, bool sameName, SearchProgressCallback progress);
	void findLibraryReference(const char* libraryName, const char* elementName, SearchProgressCallback progress);

    void setBookmark(const LibraryBookmark& bookmark);
	void onElementSelected(const char* elementName, Serializeable& serializeable);
	void onMenuConstruction(PopupMenuItem& root);

	void onSelect();

    void serialize(Archive& ar);
	const char* title() const{ return title_.c_str(); }

	SearchResultItems& items() { return searchResultItems_; }
protected:
	void onMenuSaveAsText();
	void onMenuDeleteAll();
	SearchResultItems searchResultItems_;

	int selectedIndex_;
	std::string title_;
};

#endif
