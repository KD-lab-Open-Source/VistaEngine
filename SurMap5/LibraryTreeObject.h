#ifndef __LIBRARY_TREE_OBJECT_H_INCLUDED__
#define __LIBRARY_TREE_OBJECT_H_INCLUDED__

#include "Serializeable.h"
#include "mfc\ObjectsTreeCtrl.h"
#include "StaticMap.h"

class EditorLibraryInterface;
class PopupMenuItem;
class LibraryElementTreeObject;
class LibraryGroupTreeObject;
class LibraryBookmark;

class LibraryCustomEditor{
public:
	LibraryCustomEditor(EditorLibraryInterface* library);
	virtual ~LibraryCustomEditor();

	virtual LibraryElementTreeObject* createTreeObject(const char* elementName);
	virtual LibraryGroupTreeObject* createGroupTreeObject(const char* groupName);
	virtual Serializeable serializeableByBookmark(const LibraryBookmark& bookmark, bool editOnly);

	EditorLibraryInterface* library() { return library_; }
protected:
	EditorLibraryInterface* library_;
};

// ----------------------------------------------------------------------------

class LibraryTreeObject : public TreeObject{
public:
	LibraryTreeObject(LibraryCustomEditor* customEditor)
	: customEditor_(customEditor){}

	void onSelect(){
		ensureVisible();
	}
	virtual void rebuild() {}
	virtual void onAttribElementChanged(){}
    virtual bool isGroup() const{ return false; }
    virtual bool isElement() const{ return false; }
    virtual bool isBookmark() const { return false; }

	void* getPointer() const { return (void*)(this); };
	const type_info& getTypeInfo() const{ return typeid(this); }
	Serializeable getSerializeable(const char* name,const char* nameAlt){ return Serializeable(); }
	virtual LibraryTreeObject* subElementByName(const char* subElementName = "") { return 0; }
protected:
	LibraryCustomEditor* customEditor_;
};

class LibraryElementTreeObject : public LibraryTreeObject{/*{{{*/
public:
    bool isElement() const{ return true; }
    LibraryElementTreeObject(LibraryCustomEditor* customEditor, const char* elementName);

    virtual void onMenuConstruction(PopupMenuItem& root) {}
	virtual void rebuild() {}
	void onAttribElementChanged();

    void onRightClick();
	void onMenuDelete();
	void onMenuRename();
	void onMenuSearch();
	void onMenuCopy();
	void onMenuPaste();

	bool onDoubleClick() { return false; };
	bool onBeginDrag();
    bool onDragOver(const TreeObjects& objects);
    void onDrop(const TreeObjects& objects);
    void onSelect();

	const char* elementName() const{ return elementName_.c_str(); }
	void setElementName(const char* elementName);
	Serializeable getSerializeable();
private:

	std::string elementName_;
	int elementIndex_;
};/*}}}*/

class LibraryGroupTreeObject : public LibraryTreeObject{/*{{{*/
public:
	LibraryGroupTreeObject(LibraryCustomEditor* customEditor, const char* groupName);
	virtual void onMenuConstruction(PopupMenuItem& root) {}

	void onRightClick();
	std::string fullGroupName() const;
	void onMenuCreateGroup();
	virtual void onMenuCreate();
	void onSelect();
	bool onBeginDrag();
	bool onDragOver(const TreeObjects& objects);
	void onDrop(const TreeObjects& objects);
	bool isGroup() const{ return true; }
	const char* groupName() const { return groupName_.c_str(); }
protected:
	std::string groupName_;
};/*}}}*/

class LibraryCustomEditorFactory{/*{{{*/
public:
    typedef LibraryCustomEditor Product;

    typedef EditorLibraryInterface&(*LibraryInstanceFunc)(void);

    struct CreatorBase{
        virtual Product* create() const = 0;
    };

	template<class Derived>
	struct Creator :  CreatorBase{
		Creator() : libraryInstanceFunc_(0) {}
		Creator(LibraryInstanceFunc libraryInstanceFunc)
        : libraryInstanceFunc_(libraryInstanceFunc)
        {
			LibraryCustomEditorFactory::instance().queueRegistration(*this, libraryInstanceFunc);
		}
        Product* create() const{
            return new Derived(&libraryInstanceFunc_());
        }
        LibraryInstanceFunc libraryInstanceFunc_;
	};


	void add(const char* libraryName, CreatorBase& creator_op);
	void queueRegistration(CreatorBase& creator_op, LibraryInstanceFunc func);

    Product* create(const char* libraryName, bool silent = false);
	const CreatorBase* find(const char* libraryName, bool silent = false);

	void registerQueued();

	static LibraryCustomEditorFactory& instance(){ return Singleton<LibraryCustomEditorFactory>::instance(); }
private:
    typedef StaticMap<std::string, CreatorBase*> Creators;
    Creators creators_;

	typedef StaticMap<LibraryInstanceFunc, CreatorBase*> RegistrationQueue;
	RegistrationQueue registrationQueue_;
};/*}}}*/

#define REGISTER_LIBRARY_CUSTOM_EDITOR(libraryInstanceFunc, editorType) \
	LibraryCustomEditorFactory::Creator<editorType> LibraryCustomEditorFactory##editorType(reinterpret_cast<LibraryCustomEditorFactory::LibraryInstanceFunc>(libraryInstanceFunc)); \
	int factoryDummy##LibraryCustomEditorFactory##editorType = 0;

bool focusObjectByElementName(const char* elementName, const char* subElementName, TreeObject* object);
bool focusObjectByGroupName(const char* elementName, TreeObject* object);
LibraryTreeObject* objectByElementName(const char* elementName, const char* subElementName, TreeObject* parent);

#endif
