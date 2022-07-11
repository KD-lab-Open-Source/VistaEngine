#ifndef __TREE_EDITOR_H_INCLUDED__
#define __TREE_EDITOR_H_INCLUDED__

#include <string>
#include "Handle.h"
#include "Factory.h"

class TreeNode;
class CWnd;
class CRect;
class LibraryBookmark;

template<class Derived, class DataType> class TreeEditorImpl;

class TreeEditor : public ShareHandleBase {
public:
	enum Icon { // иконки из IDB_ATTRIB_EDITOR_CTRL
		ICON_NONE          = -1,
		ICON_STRUCT		   = 0,
		ICON_STRUCT_EXPANDED,
		ICON_BLOCK,
		ICON_BLOCK_EXPANDED,
		ICON_CONTAINER,
		ICON_CONTAINER_EXPANDED,
		ICON_DROPDOWN,
		ICON_CHECKBOX,
		ICON_ELEMENT,
		ICON_POINTER,
		ICON_FILE,
		ICON_INDEXED,
		ICON_DOTS,
		ICON_REFERENCE,
	};
	virtual ~TreeEditor(){};
	virtual void free(){
		delete this;
	}
	
	virtual bool getLibraryBookmark(LibraryBookmark* bookmark = 0){ return false; }
	virtual bool hasLibrary() const{ return false; }

	virtual CWnd* beginControl (CWnd*, const CRect& rect) { return 0; }
    virtual bool hideContent () const { return false; }
	virtual Icon buttonIcon() const { return ICON_NONE; }
    virtual bool prePaint (HDC, RECT) { return false; }
    virtual bool postPaint (HDC, RECT) { return false; }

	virtual std::string nodeValue () const { return ""; }
    virtual void onChange (const TreeNode&) {};

	virtual bool canBeCleared() const{ return false; }
	virtual void onClear(TreeNode& node) {};

	virtual void endControl (TreeNode& node, CWnd*) {}
	virtual void controlEnded (TreeNode& node) {}
	virtual bool invokeEditor (TreeNode&, HWND) { return false; }
}; 

template<class Derived, class DataType>
class TreeEditorImpl : public TreeEditor {
    bool invokeEditor (TreeNode& node, HWND wnd) {
        loadStructFromNode (data_, node);
        bool result = static_cast<Derived*>(this)->invokeEditor (data_, wnd);
        saveStructToNode (data_, node);
		return result;
    }
    void onChange (const TreeNode& node) {
        loadStructFromNode (data_, node);
        static_cast<Derived*>(this)->onChange ();
    }
	virtual void onClear(TreeNode& node) {
        loadStructFromNode (data_, node);
        static_cast<Derived*>(this)->onClear(data_);
		saveStructToNode (data_, node);
	}
    void endControl (TreeNode& node, CWnd* control) {
        loadStructFromNode (data_, node);
        static_cast<Derived*>(this)->endControl (data_, control);
		saveStructToNode (data_, node);
    }
	void controlEnded (TreeNode& node) {
        loadStructFromNode (data_, node);
        static_cast<Derived*>(this)->controlEnded (data_);
		saveStructToNode (data_, node);
	}
    DataType data_;
protected:
    const DataType& getData () const { return data_; }
	virtual void endControl (DataType&, CWnd* control) {
	}
	virtual void onClear(DataType&) {

	}
	virtual void controlEnded(DataType&) {

	}
	virtual bool invokeEditor(DataType&, HWND wnd) {
		return false;
	}
    virtual void onChange() { return; }
};


class EditorLibraryInterface;

class TreeEditorFactory : public Factory<std::string, TreeEditor>
{
public:
    typedef EditorLibraryInterface&(*LibraryInstanceFunc)(void);

	typedef Factory<std::string, TreeEditor> inherited;
	template<class Derived>
	struct Creator :  inherited::Creator<Derived>{
		Creator() {}
		Creator(const char* dataTypeName, LibraryInstanceFunc libraryInstanceFunc = 0){
			TreeEditorFactory::instance().add(dataTypeName, *this, libraryInstanceFunc);
		}
	};

	void add(const char* dataTypeName, CreatorBase& creator_op, LibraryInstanceFunc func);
	const char* findReferencedLibrary(const char* referenceTypeName) const;
	void buildMap();

	static TreeEditorFactory& instance() {
		return Singleton<TreeEditorFactory>::instance();
	}
private:
	typedef StaticMap<std::string, const char*> LibraryNames;
	typedef StaticMap<std::string, LibraryInstanceFunc> LibraryInstances;
	LibraryNames libraryNames_;
	LibraryInstances libraryInstances_;
};

#define REGISTER_TREE_EDITOR(dataTypeName, editorType, libraryInstanceFunc) \
	TreeEditorFactory::Creator<editorType> TreeEditorFactory##editorType(dataTypeName, reinterpret_cast<TreeEditorFactory::LibraryInstanceFunc>(libraryInstanceFunc)); \
	int factoryDummy##TreeEditorFactory##editorType = 0;

#endif